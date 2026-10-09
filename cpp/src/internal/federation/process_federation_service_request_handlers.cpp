#include "internal/federation/process_federation_service.hpp"
#include "internal/federation/process_federation_service_helpers.hpp"
#include "internal/fom/hla_names.hpp"
#include "internal/handles/dimension_handle.hpp"
#include "internal/handles/federate_handle.hpp"
#include "internal/handles/interaction_class_handle.hpp"
#include "internal/handles/object_class_handle.hpp"
#include "internal/handles/region_handle.hpp"
#include "internal/handles/transportation_type_handle.hpp"
#include "internal/observability/mom_service_report_encoding.hpp"
#include "internal/runtime/utf8_string.hpp"

#include <cstdint>
#include <iterator>
#include <limits>
#include <memory>
#include <mutex>
#include <optional>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace umbra::detail {

TransportServiceMessage ProcessFederationService::handleGetFederateHandle(
    ProcessTransportSession &session,
    TransportServiceMessage const &request) {
  auto const lookupRequest =
      decodeProcessFederationGetFederateHandleRequest(request.payload);
  std::optional<FederateMembership> membership;
  {
    std::scoped_lock lock(mutex_);
    auto const state = sessions_.find(&session);
    if (state == sessions_.end() ||
        !state->second.federationName.has_value() ||
        *state->second.federationName != lookupRequest.federationName ||
        state->second.federateId != lookupRequest.federateId ||
        !registry_.memberById(
            lookupRequest.federationName, lookupRequest.federateId)) {
      return rejected(request);
    }
    membership = registry_.memberByName(
        lookupRequest.federationName, lookupRequest.federateName);
  }
  if (!membership || membership->id == 0U) {
    return rejected(request);
  }
  return responseFor(
      request,
      TransportServiceStatus::ok,
      encodeProcessFederationHandleResult(
          ProcessFederationHandleResult{membership->id}));
}

TransportServiceMessage ProcessFederationService::handleGetFederateName(
    ProcessTransportSession &session,
    TransportServiceMessage const &request) {
  auto const lookupRequest =
      decodeProcessFederationGetFederateNameRequest(request.payload);
  std::optional<std::wstring> federateName;
  {
    std::scoped_lock lock(mutex_);
    auto const state = sessions_.find(&session);
    if (state == sessions_.end() ||
        !state->second.federationName.has_value() ||
        *state->second.federationName != lookupRequest.federationName ||
        state->second.federateId != lookupRequest.federateId ||
        !registry_.memberById(
            lookupRequest.federationName, lookupRequest.federateId)) {
      return rejected(request);
    }
    federateName = registry_.federateNameFor(
        lookupRequest.federationName, lookupRequest.targetFederateId);
  }
  if (!federateName || federateName->empty()) {
    return rejected(request);
  }
  return responseFor(
      request,
      TransportServiceStatus::ok,
      encodeProcessFederationStringResult(
          ProcessFederationStringResult{std::move(*federateName)}));
}

TransportServiceMessage ProcessFederationService::handleNormalizeHandle(
    ProcessTransportSession &session,
    TransportServiceMessage const &request) {
  auto const normalizeRequest =
      decodeProcessFederationNormalizeHandleRequest(request.payload);
  std::optional<unsigned long> normalized;
  {
    std::scoped_lock lock(mutex_);
    auto const state = sessions_.find(&session);
    if (state == sessions_.end() ||
        !state->second.federationName.has_value() ||
        *state->second.federationName != normalizeRequest.federationName ||
        state->second.federateId != normalizeRequest.federateId ||
        !registry_.memberById(
            normalizeRequest.federationName, normalizeRequest.federateId)) {
      return rejected(request);
    }

    switch (request.operation) {
      case TransportServiceOperation::normalize_federate_handle:
        normalized = registry_.normalizedFederateHandleValueFor(
            normalizeRequest.federationName, normalizeRequest.handle);
        break;
      case TransportServiceOperation::normalize_object_class_handle:
        normalized = registry_.normalizedObjectClassHandleValueFor(
            normalizeRequest.federationName, normalizeRequest.handle);
        break;
      case TransportServiceOperation::normalize_interaction_class_handle:
        normalized = registry_.normalizedInteractionClassHandleValueFor(
            normalizeRequest.federationName, normalizeRequest.handle);
        break;
      case TransportServiceOperation::normalize_object_instance_handle:
        normalized = registry_.normalizedObjectInstanceHandleValueFor(
            normalizeRequest.federationName, normalizeRequest.handle);
        break;
      default:
        return invalid(request);
    }
  }
  if (!normalized || *normalized == 0UL) {
    return rejected(request);
  }
  return responseFor(
      request,
      TransportServiceStatus::ok,
      encodeProcessFederationHandleResult(
          ProcessFederationHandleResult{
              static_cast<std::uint64_t>(*normalized)}));
}

TransportServiceMessage ProcessFederationService::handleResign(
    ProcessTransportSession &session,
    TransportServiceMessage const &request) {
  auto const resignRequest = decodeProcessFederationResignRequest(request.payload);
  {
    std::scoped_lock lock(mutex_);
    auto const state = sessions_.find(&session);
    if (state == sessions_.end() ||
        !state->second.federationName.has_value() ||
        *state->second.federationName != resignRequest.federationName ||
        state->second.federateId != resignRequest.federateId ||
        !registry_.memberById(
            resignRequest.federationName, resignRequest.federateId)) {
      return rejected(request);
    }
  }

  auto const result = registry_.resign(
      resignRequest.federationName,
      resignRequest.federateId,
      resignRequest.resignAction);
  if (result.status != FederationRegistryStatus::applied) {
    return rejected(request);
  }
  if (!dispatchConnectionLossResult(
          resignRequest.federationName,
          result,
          resignRequest.federateId)) {
    return internalError(request);
  }
  auto scheduled = registry_.reevaluateTimeAdvanceGrants(
      resignRequest.federationName);
  if (scheduled.status != FederationTimeGrantStatus::applied) {
    return internalError(request);
  }
  dispatchTimeAdvanceGrants(
      resignRequest.federationName,
      std::move(scheduled.dispatches));
  {
    std::scoped_lock lock(mutex_);
    auto const state = sessions_.find(&session);
    if (state != sessions_.end()) {
      sessionsByFederateId_.erase(state->second.federateId);
      for (auto position = processTsoMessageProducers_.begin();
           position != processTsoMessageProducers_.end();) {
        if (position->second == state->second.federateId) {
          pendingPushedRetractionRecipients_.erase(position->first);
          position = processTsoMessageProducers_.erase(position);
        } else {
          ++position;
        }
      }
      state->second.federationName.reset();
      state->second.federateId = 0U;
      state->second.timeState.reset();
      state->second.serviceReportWriter.reset();
      state->second.serviceReportLocation.clear();
      state->second.interactionEvents.clear();
      state->second.attributeUpdateEvents.clear();
      state->second.objectInstanceDiscoveryEvents.clear();
      state->second.objectInstanceRemovalEvents.clear();
      state->second.objectInstanceScopeChangeEvents.clear();
      state->second.attributeRelevanceAdvisoryEvents.clear();
      state->second.attributeTransportationTypeChangeEvents.clear();
      state->second.attributeTransportationTypeQueryEvents.clear();
      state->second.interactionTransportationTypeChangeEvents.clear();
      state->second.interactionTransportationTypeQueryEvents.clear();
      state->second.attributeOwnershipQueryEvents.clear();
      state->second.attributeOwnershipAcquisitionIfAvailableEvents.clear();
      state->second.attributeOwnershipAcquisitionEvents.clear();
      state->second.attributeOwnershipUnavailableEvents.clear();
      state->second.synchronizationPointAnnouncementEvents.clear();
      state->second.federationSynchronizedEvents.clear();
      state->second.saveEvents.clear();
      state->second.restoreEvents.clear();
    }
  }
  return responseFor(request, TransportServiceStatus::ok);
}

TransportServiceMessage
ProcessFederationService::handleRegisterFederationSynchronizationPoint(
    ProcessTransportSession &session,
    TransportServiceMessage const &request) {
  auto const registration =
      decodeProcessFederationRegisterSynchronizationPointRequest(
          request.payload);
  {
    std::scoped_lock lock(mutex_);
    auto const state = sessions_.find(&session);
    if (state == sessions_.end() ||
        !state->second.federationName.has_value() ||
        *state->second.federationName != registration.federationName ||
        state->second.federateId != registration.federateId ||
        !registry_.memberById(
            registration.federationName, registration.federateId)) {
      return rejected(request);
    }
  }

  std::set<std::uint64_t> synchronizationSet(
      registration.synchronizationSet.begin(),
      registration.synchronizationSet.end());
  auto plan = registry_.registerSynchronizationPoint(
      registration.federationName,
      registration.federateId,
      registration.label,
      registration.userSuppliedTag,
      synchronizationSet,
      registration.synchronizationSetWasSupplied);
  ProcessFederationRegisterSynchronizationPointResult result;
  switch (plan.status) {
    case SynchronizationPointRegistrationStatus::applied:
      result.status =
          ProcessFederationSynchronizationPointRegistrationStatus::applied;
      break;
    case SynchronizationPointRegistrationStatus::federation_does_not_exist:
      result.status = ProcessFederationSynchronizationPointRegistrationStatus::
          federation_does_not_exist;
      break;
    case SynchronizationPointRegistrationStatus::federate_not_member:
      result.status =
          ProcessFederationSynchronizationPointRegistrationStatus::federate_not_member;
      break;
    case SynchronizationPointRegistrationStatus::callback_route_missing:
      result.status = ProcessFederationSynchronizationPointRegistrationStatus::
          callback_route_missing;
      break;
  }
  result.succeeded = plan.succeeded;
  result.failureReason = plan.failureReason;
  if (plan.status == SynchronizationPointRegistrationStatus::applied &&
      !plan.announcements.empty() &&
      !enqueueSynchronizationPointAnnouncements(
          registration.federationName,
          std::move(plan.announcements))) {
    return internalError(request);
  }
  return responseFor(
      request,
      TransportServiceStatus::ok,
      encodeProcessFederationRegisterSynchronizationPointResult(result));
}

TransportServiceMessage ProcessFederationService::handleSynchronizationPointAchieved(
    ProcessTransportSession &session,
    TransportServiceMessage const &request) {
  auto const achievement =
      decodeProcessFederationSynchronizationPointAchievedRequest(
          request.payload);
  {
    std::scoped_lock lock(mutex_);
    auto const state = sessions_.find(&session);
    if (state == sessions_.end() ||
        !state->second.federationName.has_value() ||
        *state->second.federationName != achievement.federationName ||
        state->second.federateId != achievement.federateId ||
        !registry_.memberById(
            achievement.federationName, achievement.federateId)) {
      return rejected(request);
    }
  }

  auto plan = registry_.achieveSynchronizationPoint(
      achievement.federationName,
      achievement.federateId,
      achievement.label,
      achievement.successfully);
  ProcessFederationSynchronizationPointAchievedResult result;
  switch (plan.status) {
    case SynchronizationPointAchievedStatus::applied:
      result.status =
          ProcessFederationSynchronizationPointAchievedStatus::applied;
      break;
    case SynchronizationPointAchievedStatus::federation_does_not_exist:
      result.status = ProcessFederationSynchronizationPointAchievedStatus::
          federation_does_not_exist;
      break;
    case SynchronizationPointAchievedStatus::federate_not_member:
      result.status =
          ProcessFederationSynchronizationPointAchievedStatus::federate_not_member;
      break;
    case SynchronizationPointAchievedStatus::
        synchronization_point_label_not_announced:
      result.status = ProcessFederationSynchronizationPointAchievedStatus::
          synchronization_point_label_not_announced;
      break;
  }
  if (plan.status == SynchronizationPointAchievedStatus::applied &&
      !plan.synchronizationNotifications.empty() &&
      !enqueueFederationSynchronizedNotifications(
          achievement.federationName,
          std::move(plan.synchronizationNotifications))) {
    return internalError(request);
  }
  return responseFor(
      request,
      TransportServiceStatus::ok,
      encodeProcessFederationSynchronizationPointAchievedResult(result));
}





TransportServiceMessage ProcessFederationService::handleInteractionClassDeclaration(
    ProcessTransportSession &session,
    TransportServiceMessage const &request) {
  auto const declarationRequest =
      decodeProcessFederationInteractionClassDeclarationRequest(request.payload);
  {
    std::scoped_lock lock(mutex_);
    auto const state = sessions_.find(&session);
    if (state == sessions_.end() ||
        !state->second.federationName.has_value() ||
        *state->second.federationName != declarationRequest.federationName ||
        state->second.federateId != declarationRequest.federateId ||
        !registry_.memberById(
            declarationRequest.federationName, declarationRequest.federateId)) {
      return rejected(request);
    }
  }

  InteractionClassDeclarationStatus result =
      InteractionClassDeclarationStatus::federation_does_not_exist;
  switch (request.operation) {
    case TransportServiceOperation::publish_interaction_class:
      result = registry_.setInteractionClassPublication(
          declarationRequest.federationName,
          declarationRequest.federateId,
          declarationRequest.interactionClassHandle,
          true);
      break;
    case TransportServiceOperation::unpublish_interaction_class:
      result = registry_.setInteractionClassPublication(
          declarationRequest.federationName,
          declarationRequest.federateId,
          declarationRequest.interactionClassHandle,
          false);
      break;
    case TransportServiceOperation::subscribe_interaction_class:
      result = registry_.setInteractionClassSubscription(
          declarationRequest.federationName,
          declarationRequest.federateId,
          declarationRequest.interactionClassHandle,
          declarationRequest.active);
      break;
    case TransportServiceOperation::unsubscribe_interaction_class:
      result = registry_.setInteractionClassSubscription(
          declarationRequest.federationName,
          declarationRequest.federateId,
          declarationRequest.interactionClassHandle,
          std::nullopt);
      break;
    default:
      return invalid(request);
  }
  if (result == InteractionClassDeclarationStatus::
          federate_service_invocations_are_being_reported_via_mom) {
    return responseFor(
        request,
        TransportServiceStatus::service_reporting_interlock);
  }
  if (result != InteractionClassDeclarationStatus::applied) {
    return rejected(request);
  }
  return responseFor(request, TransportServiceStatus::ok);
}

TransportServiceMessage ProcessFederationService::handleChangeInteractionOrderType(
    ProcessTransportSession &session,
    TransportServiceMessage const &request) {
  auto const changeRequest =
      decodeProcessFederationChangeInteractionOrderTypeRequest(request.payload);
  {
    std::scoped_lock lock(mutex_);
    auto const state = sessions_.find(&session);
    if (state == sessions_.end() ||
        !state->second.federationName.has_value() ||
        *state->second.federationName != changeRequest.federationName ||
        state->second.federateId != changeRequest.federateId ||
        !registry_.memberById(
            changeRequest.federationName, changeRequest.federateId)) {
      return rejected(request);
    }
  }
  auto const status = registry_.changeInteractionOrderType(
      changeRequest.federationName,
      changeRequest.federateId,
      changeRequest.interactionClassHandle,
      changeRequest.orderType);
  return responseFor(
      request,
      TransportServiceStatus::ok,
      encodeProcessFederationInteractionOrderTypeChangeResult(
          ProcessFederationInteractionOrderTypeChangeResult{status}));
}

TransportServiceMessage ProcessFederationService::handleChangeAttributeOrderType(
    ProcessTransportSession &session,
    TransportServiceMessage const &request) {
  auto const changeRequest =
      decodeProcessFederationChangeAttributeOrderTypeRequest(request.payload);
  {
    std::scoped_lock lock(mutex_);
    auto const state = sessions_.find(&session);
    if (state == sessions_.end() ||
        !state->second.federationName.has_value() ||
        *state->second.federationName != changeRequest.federationName ||
        state->second.federateId != changeRequest.federateId ||
        !registry_.memberById(
            changeRequest.federationName, changeRequest.federateId)) {
      return rejected(request);
    }
  }
  std::set<std::uint64_t> attributeHandles(
      changeRequest.attributeHandles.begin(),
      changeRequest.attributeHandles.end());
  auto const status = registry_.changeAttributeOrderType(
      changeRequest.federationName,
      changeRequest.federateId,
      changeRequest.objectInstanceHandle,
      attributeHandles,
      changeRequest.orderType);
  return responseFor(
      request,
      TransportServiceStatus::ok,
      encodeProcessFederationAttributeOrderTypeChangeResult(
          ProcessFederationAttributeOrderTypeChangeResult{status}));
}

TransportServiceMessage
ProcessFederationService::handleChangeDefaultAttributeOrderType(
    ProcessTransportSession &session,
    TransportServiceMessage const &request) {
  auto const changeRequest =
      decodeProcessFederationChangeDefaultAttributeOrderTypeRequest(
          request.payload);
  {
    std::scoped_lock lock(mutex_);
    auto const state = sessions_.find(&session);
    if (state == sessions_.end() ||
        !state->second.federationName.has_value() ||
        *state->second.federationName != changeRequest.federationName ||
        state->second.federateId != changeRequest.federateId ||
        !registry_.memberById(
            changeRequest.federationName, changeRequest.federateId)) {
      return rejected(request);
    }
  }
  std::set<std::uint64_t> attributeHandles(
      changeRequest.attributeHandles.begin(),
      changeRequest.attributeHandles.end());
  auto const status = registry_.changeDefaultAttributeOrderType(
      changeRequest.federationName,
      changeRequest.federateId,
      changeRequest.objectClassHandle,
      attributeHandles,
      changeRequest.orderType);
  return responseFor(
      request,
      TransportServiceStatus::ok,
      encodeProcessFederationAttributeOrderTypeDefaultResult(
          ProcessFederationAttributeOrderTypeDefaultResult{status}));
}

TransportServiceMessage ProcessFederationService::handleRegisterObjectInstance(
    ProcessTransportSession &session,
    TransportServiceMessage const &request) {
  auto const registrationRequest =
      decodeProcessFederationRegisterObjectInstanceRequest(request.payload);
  {
    std::scoped_lock lock(mutex_);
    auto const state = sessions_.find(&session);
    if (state == sessions_.end() ||
        !state->second.federationName.has_value() ||
        *state->second.federationName != registrationRequest.federationName ||
        state->second.federateId != registrationRequest.federateId ||
        !registry_.memberById(
            registrationRequest.federationName, registrationRequest.federateId)) {
      return rejected(request);
    }
  }

  std::wstring const *requestedName = nullptr;
  if (registrationRequest.requestedObjectInstanceName.has_value()) {
    requestedName = &*registrationRequest.requestedObjectInstanceName;
  }
  auto const result = registry_.registerObjectInstance(
      registrationRequest.federationName,
      registrationRequest.federateId,
      registrationRequest.objectClassHandle,
      nullptr,
      requestedName);
  if (result.status != ObjectInstanceRegistrationStatus::applied) {
    return responseFor(
        request,
        TransportServiceStatus::ok,
        encodeProcessFederationRegisterObjectInstanceResult(
            ProcessFederationRegisterObjectInstanceResult{
                0U,
                {},
                result.status}));
  }
  auto discoveries = registry_.planObjectInstanceDiscoveriesForInstance(
      registrationRequest.federationName,
      result.objectInstanceHandle);
  if (!enqueueObjectInstanceDiscoveries(
          registrationRequest.federationName,
          std::move(discoveries))) {
    return internalError(request);
  }
  return responseFor(
      request,
      TransportServiceStatus::ok,
      encodeProcessFederationRegisterObjectInstanceResult(
          ProcessFederationRegisterObjectInstanceResult{
              result.objectInstanceHandle,
              result.objectInstanceName,
              result.status}));
}

TransportServiceMessage ProcessFederationService::handleReserveObjectInstanceName(
    ProcessTransportSession &session,
    TransportServiceMessage const &request) {
  auto const reservationRequest =
      decodeProcessFederationReserveObjectInstanceNameRequest(request.payload);
  {
    std::scoped_lock lock(mutex_);
    auto const state = sessions_.find(&session);
    if (state == sessions_.end() ||
        !state->second.federationName.has_value() ||
        *state->second.federationName != reservationRequest.federationName ||
        state->second.federateId != reservationRequest.federateId ||
        !registry_.memberById(
            reservationRequest.federationName, reservationRequest.federateId)) {
      return rejected(request);
    }
  }

  auto const result = registry_.reserveObjectInstanceName(
      reservationRequest.federationName,
      reservationRequest.federateId,
      reservationRequest.objectInstanceName);
  // Reservation callbacks remain local to the public process adapter for this
  // bounded slice. The process service owns the reservation ledger and returns
  // the accepted outcome; the client queues the official callback through its
  // existing dispatcher without transporting a registry callback closure.
  return responseFor(
      request,
      TransportServiceStatus::ok,
      encodeProcessFederationReserveObjectInstanceNameResult(
          ProcessFederationReserveObjectInstanceNameResult{
              result.succeeded,
              result.objectInstanceName,
              result.status}));
}

TransportServiceMessage ProcessFederationService::handleReleaseObjectInstanceName(
    ProcessTransportSession &session,
    TransportServiceMessage const &request) {
  auto const releaseRequest =
      decodeProcessFederationReserveObjectInstanceNameRequest(request.payload);
  {
    std::scoped_lock lock(mutex_);
    auto const state = sessions_.find(&session);
    if (state == sessions_.end() ||
        !state->second.federationName.has_value() ||
        *state->second.federationName != releaseRequest.federationName ||
        state->second.federateId != releaseRequest.federateId ||
        !registry_.memberById(
            releaseRequest.federationName, releaseRequest.federateId)) {
      return rejected(request);
    }
  }

  auto const status = registry_.releaseObjectInstanceName(
      releaseRequest.federationName,
      releaseRequest.federateId,
      releaseRequest.objectInstanceName);
  return responseFor(
      request,
      TransportServiceStatus::ok,
      encodeProcessFederationObjectInstanceNameReleaseResult(
          ProcessFederationObjectInstanceNameReleaseResult{status}));
}

TransportServiceMessage
ProcessFederationService::handleReserveMultipleObjectInstanceNames(
    ProcessTransportSession &session,
    TransportServiceMessage const &request) {
  auto const reservationRequest =
      decodeProcessFederationReserveMultipleObjectInstanceNamesRequest(
          request.payload);
  {
    std::scoped_lock lock(mutex_);
    auto const state = sessions_.find(&session);
    if (state == sessions_.end() ||
        !state->second.federationName.has_value() ||
        *state->second.federationName != reservationRequest.federationName ||
        state->second.federateId != reservationRequest.federateId ||
        !registry_.memberById(
            reservationRequest.federationName, reservationRequest.federateId)) {
      return rejected(request);
    }
  }

  auto const result = registry_.reserveMultipleObjectInstanceNames(
      reservationRequest.federationName,
      reservationRequest.federateId,
      reservationRequest.objectInstanceNames);
  return responseFor(
      request,
      TransportServiceStatus::ok,
      encodeProcessFederationReserveMultipleObjectInstanceNamesResult(
          ProcessFederationReserveMultipleObjectInstanceNamesResult{
              result.status, result.succeededNames, result.failedNames}));
}

TransportServiceMessage
ProcessFederationService::handleReleaseMultipleObjectInstanceNames(
    ProcessTransportSession &session,
    TransportServiceMessage const &request) {
  auto const releaseRequest =
      decodeProcessFederationReserveMultipleObjectInstanceNamesRequest(
          request.payload);
  {
    std::scoped_lock lock(mutex_);
    auto const state = sessions_.find(&session);
    if (state == sessions_.end() ||
        !state->second.federationName.has_value() ||
        *state->second.federationName != releaseRequest.federationName ||
        state->second.federateId != releaseRequest.federateId ||
        !registry_.memberById(
            releaseRequest.federationName, releaseRequest.federateId)) {
      return rejected(request);
    }
  }

  auto const status = registry_.releaseMultipleObjectInstanceNames(
      releaseRequest.federationName,
      releaseRequest.federateId,
      releaseRequest.objectInstanceNames);
  return responseFor(
      request,
      TransportServiceStatus::ok,
      encodeProcessFederationReleaseMultipleObjectInstanceNamesResult(
          ProcessFederationReleaseMultipleObjectInstanceNamesResult{status}));
}

TransportServiceMessage ProcessFederationService::handleGetDimensionHandle(
    ProcessTransportSession &session,
    TransportServiceMessage const &request) {
  auto const lookupRequest =
      decodeProcessFederationGetDimensionHandleRequest(request.payload);
  {
    std::scoped_lock lock(mutex_);
    auto const state = sessions_.find(&session);
    if (state == sessions_.end() ||
        !state->second.federationName.has_value() ||
        *state->second.federationName != lookupRequest.federationName ||
        state->second.federateId != lookupRequest.federateId ||
        !registry_.memberById(
            lookupRequest.federationName, lookupRequest.federateId)) {
      return rejected(request);
    }
  }
  auto const handle = registry_.dimensionHandleFor(
      lookupRequest.federationName, lookupRequest.dimensionName);
  if (!handle) {
    return rejected(request);
  }
  auto const dimensionName = wideFromUtf8(lookupRequest.dimensionName);
  if (!dimensionName ||
      !appendSelectedServiceReportRecord(
          session,
          lookupRequest.federationName,
          lookupRequest.federateId,
          static_cast<std::uint16_t>(MomServiceType::support_services),
          [dimensionName = *dimensionName,
           dimensionHandle = *handle](std::uint32_t serialNumber) {
            return formatMomSuccessfulServiceReportRecord(
                serialNumber,
                L"GetDimensionHandle",
                {{MomArgumentType::string,
                  L"Dimension name",
                  formatMomString(dimensionName)}},
                {MomArgumentType::dimension_handle,
                 L"Dimension handle",
                 formatMomDimensionHandle(
                     rti1516_2025::umbra_binding_detail::makeDimensionHandle(
                         dimensionHandle))});
          })) {
    return internalError(request);
  }
  return responseFor(
      request,
      TransportServiceStatus::ok,
      encodeProcessFederationHandleResult(ProcessFederationHandleResult{*handle}));
}

TransportServiceMessage ProcessFederationService::handleGetDimensionName(
    ProcessTransportSession &session,
    TransportServiceMessage const &request) {
  auto const lookupRequest =
      decodeProcessFederationDimensionRequest(request.payload);
  std::optional<std::string> encodedName;
  {
    std::scoped_lock lock(mutex_);
    auto const state = sessions_.find(&session);
    if (state == sessions_.end() ||
        !state->second.federationName.has_value() ||
        *state->second.federationName != lookupRequest.federationName ||
        state->second.federateId != lookupRequest.federateId ||
        !registry_.memberById(
            lookupRequest.federationName, lookupRequest.federateId)) {
      return rejected(request);
    }
    encodedName = registry_.dimensionNameFor(
        lookupRequest.federationName, lookupRequest.dimensionHandle);
  }
  if (!encodedName) {
    return rejected(request);
  }
  auto const decodedName = wideFromUtf8(*encodedName);
  if (!decodedName) {
    return internalError(request);
  }
  if (!appendSelectedServiceReportRecord(
          session,
          lookupRequest.federationName,
          lookupRequest.federateId,
          static_cast<std::uint16_t>(MomServiceType::support_services),
          [dimensionHandle = lookupRequest.dimensionHandle,
           dimensionName = *decodedName](std::uint32_t serialNumber) {
            return formatMomSuccessfulServiceReportRecord(
                serialNumber,
                L"GetDimensionName",
                {{MomArgumentType::dimension_handle,
                  L"Dimension handle",
                  formatMomDimensionHandle(
                      rti1516_2025::umbra_binding_detail::makeDimensionHandle(
                          dimensionHandle))}},
                {MomArgumentType::string,
                 L"Dimension name",
                 formatMomString(dimensionName)});
          })) {
    return internalError(request);
  }
  return responseFor(
      request,
      TransportServiceStatus::ok,
      encodeProcessFederationStringResult(
          ProcessFederationStringResult{*decodedName}));
}

TransportServiceMessage
ProcessFederationService::handleGetTransportationTypeHandle(
    ProcessTransportSession &session,
    TransportServiceMessage const &request) {
  auto const lookupRequest =
      decodeProcessFederationGetTransportationTypeHandleRequest(
          request.payload);
  std::optional<std::uint64_t> handle;
  {
    std::scoped_lock lock(mutex_);
    auto const state = sessions_.find(&session);
    if (state == sessions_.end() ||
        !state->second.federationName.has_value() ||
        *state->second.federationName != lookupRequest.federationName ||
        state->second.federateId != lookupRequest.federateId ||
        !registry_.memberById(
            lookupRequest.federationName, lookupRequest.federateId)) {
      return rejected(request);
    }
    handle = registry_.transportationTypeHandleFor(
        lookupRequest.federationName, lookupRequest.transportationTypeName);
  }
  if (!handle) {
    return rejected(request);
  }
  return responseFor(
      request,
      TransportServiceStatus::ok,
      encodeProcessFederationHandleResult(ProcessFederationHandleResult{*handle}));
}

TransportServiceMessage
ProcessFederationService::handleGetTransportationTypeName(
    ProcessTransportSession &session,
    TransportServiceMessage const &request) {
  auto const lookupRequest =
      decodeProcessFederationTransportationTypeRequest(request.payload);
  std::optional<std::string> encodedName;
  {
    std::scoped_lock lock(mutex_);
    auto const state = sessions_.find(&session);
    if (state == sessions_.end() ||
        !state->second.federationName.has_value() ||
        *state->second.federationName != lookupRequest.federationName ||
        state->second.federateId != lookupRequest.federateId ||
        !registry_.memberById(
            lookupRequest.federationName, lookupRequest.federateId)) {
      return rejected(request);
    }
    encodedName = registry_.transportationTypeNameFor(
        lookupRequest.federationName,
        lookupRequest.transportationTypeHandle);
  }
  if (!encodedName) {
    return rejected(request);
  }
  auto const decodedName = wideFromUtf8(*encodedName);
  if (!decodedName) {
    return internalError(request);
  }
  if (!appendSelectedServiceReportRecord(
          session,
          lookupRequest.federationName,
          lookupRequest.federateId,
          static_cast<std::uint16_t>(MomServiceType::support_services),
          [transportationTypeHandle = lookupRequest.transportationTypeHandle,
           transportationTypeName = *decodedName](std::uint32_t serialNumber) {
            return formatMomSuccessfulServiceReportRecord(
                serialNumber,
                L"GetTransportationTypeName",
                {{MomArgumentType::transportation_type_handle,
                  L"Transportation type handle",
                  formatMomTransportationTypeHandle(
                      rti1516_2025::umbra_binding_detail::
                          makeTransportationTypeHandle(
                              transportationTypeHandle))}},
                {MomArgumentType::string,
                 L"Transportation type name",
                 formatMomString(transportationTypeName)});
          })) {
    return internalError(request);
  }
  return responseFor(
      request,
      TransportServiceStatus::ok,
      encodeProcessFederationStringResult(
          ProcessFederationStringResult{*decodedName}));
}

TransportServiceMessage
ProcessFederationService::handleGetAutomaticResignDirective(
    ProcessTransportSession &session,
    TransportServiceMessage const &request) {
  auto const directiveRequest =
      decodeProcessFederationResignRequest(request.payload);
  {
    std::scoped_lock lock(mutex_);
    auto const state = sessions_.find(&session);
    if (state == sessions_.end() ||
        !state->second.federationName.has_value() ||
        *state->second.federationName != directiveRequest.federationName ||
        state->second.federateId != directiveRequest.federateId ||
        !registry_.memberById(
            directiveRequest.federationName, directiveRequest.federateId)) {
      return rejected(request);
    }
  }
  auto const action = registry_.automaticResignActionFor(
      directiveRequest.federationName, directiveRequest.federateId);
  if (!action) {
    return rejected(request);
  }
  return responseFor(
      request,
      TransportServiceStatus::ok,
      encodeProcessFederationResignRequest(
          ProcessFederationResignRequest{
              directiveRequest.federationName,
              directiveRequest.federateId,
              *action}));
}

TransportServiceMessage
ProcessFederationService::handleSetAutomaticResignDirective(
    ProcessTransportSession &session,
    TransportServiceMessage const &request) {
  auto const directiveRequest =
      decodeProcessFederationResignRequest(request.payload);
  {
    std::scoped_lock lock(mutex_);
    auto const state = sessions_.find(&session);
    if (state == sessions_.end() ||
        !state->second.federationName.has_value() ||
        *state->second.federationName != directiveRequest.federationName ||
        state->second.federateId != directiveRequest.federateId ||
        !registry_.memberById(
            directiveRequest.federationName, directiveRequest.federateId)) {
      return rejected(request);
    }
  }
  auto const status = registry_.setAutomaticResignAction(
      directiveRequest.federationName,
      directiveRequest.federateId,
      directiveRequest.resignAction);
  if (status != FederationRegistryStatus::applied) {
    return rejected(request);
  }
  return responseFor(
      request,
      TransportServiceStatus::ok,
      encodeProcessFederationBooleanResult(
          ProcessFederationBooleanResult{true}));
}

}  // namespace umbra::detail
