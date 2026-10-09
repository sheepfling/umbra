#include "internal/federation/process_federation_service.hpp"
#include "internal/federation/process_federation_service_helpers.hpp"

#include "internal/handles/interaction_class_handle.hpp"
#include "internal/handles/object_class_handle.hpp"
#include "internal/observability/mom_service_report_encoding.hpp"
#include "internal/runtime/utf8_string.hpp"

#include <mutex>
#include <optional>
#include <utility>

namespace umbra::detail {
namespace {

[[nodiscard]] ProcessFederationUpdateRateValueStatus
processUpdateRateValueStatus(UpdateRateValueStatus status) noexcept {
  switch (status) {
    case UpdateRateValueStatus::applied:
      return ProcessFederationUpdateRateValueStatus::applied;
    case UpdateRateValueStatus::federation_does_not_exist:
      return ProcessFederationUpdateRateValueStatus::federation_does_not_exist;
    case UpdateRateValueStatus::federate_not_member:
      return ProcessFederationUpdateRateValueStatus::federate_not_member;
    case UpdateRateValueStatus::invalid_update_rate_designator:
      return ProcessFederationUpdateRateValueStatus::invalid_update_rate_designator;
    case UpdateRateValueStatus::object_instance_not_known:
      return ProcessFederationUpdateRateValueStatus::object_instance_not_known;
    case UpdateRateValueStatus::attribute_not_defined:
      return ProcessFederationUpdateRateValueStatus::attribute_not_defined;
    case UpdateRateValueStatus::inconsistent_catalog:
      return ProcessFederationUpdateRateValueStatus::inconsistent_catalog;
  }
  return ProcessFederationUpdateRateValueStatus::inconsistent_catalog;
}

}  // namespace

TransportServiceMessage ProcessFederationService::handleGetInteractionClassHandle(
    ProcessTransportSession& session,
    TransportServiceMessage const& request) {
  auto const lookupRequest =
      decodeProcessFederationGetInteractionClassHandleRequest(request.payload);
  auto const encodedName = utf8FromWide(lookupRequest.interactionClassName);
  if (!encodedName) {
    return rejected(request);
  }

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
    handle = registry_.interactionClassHandleFor(
        lookupRequest.federationName, *encodedName);
  }
  if (!handle) {
    return rejected(request);
  }
  return responseFor(
      request,
      TransportServiceStatus::ok,
      encodeProcessFederationHandleResult(ProcessFederationHandleResult{*handle}));
}

TransportServiceMessage ProcessFederationService::handleGetObjectClassHandle(
    ProcessTransportSession& session,
    TransportServiceMessage const& request) {
  auto const lookupRequest =
      decodeProcessFederationGetObjectClassHandleRequest(request.payload);
  auto const encodedName = utf8FromWide(lookupRequest.objectClassName);
  if (!encodedName) {
    return rejected(request);
  }

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
    handle = registry_.objectClassHandleFor(
        lookupRequest.federationName, *encodedName);
  }
  if (!handle) {
    return rejected(request);
  }
  if (lookupRequest.callbacksEnabled &&
      !flushDeferredAttributeOwnershipAssumptionEvents(
          session,
          lookupRequest.federationName,
          lookupRequest.federateId)) {
    return internalError(request);
  }
  if (!appendSelectedServiceReportRecord(
          session,
          lookupRequest.federationName,
          lookupRequest.federateId,
          static_cast<std::uint16_t>(MomServiceType::support_services),
          [objectClassName = lookupRequest.objectClassName,
           objectClassHandle = *handle](std::uint32_t serialNumber) {
            return formatMomSuccessfulServiceReportRecord(
                serialNumber,
                L"GetObjectClassHandle",
                {{MomArgumentType::string,
                  L"Object class name",
                  formatMomString(objectClassName)}},
                {MomArgumentType::object_class_handle,
                 L"Object class handle",
                 formatMomObjectClassHandle(
                     rti1516_2025::umbra_binding_detail::makeObjectClassHandle(
                         objectClassHandle))});
          })) {
    return internalError(request);
  }
  return responseFor(
      request,
      TransportServiceStatus::ok,
      encodeProcessFederationHandleResult(ProcessFederationHandleResult{*handle}));
}

TransportServiceMessage ProcessFederationService::handleGetParameterHandle(
    ProcessTransportSession& session,
    TransportServiceMessage const& request) {
  auto const lookupRequest =
      decodeProcessFederationGetParameterHandleRequest(request.payload);
  auto const encodedName = utf8FromWide(lookupRequest.parameterName);
  if (!encodedName) {
    return rejected(request);
  }

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
    auto const interactionClassName = registry_.interactionClassNameFor(
        lookupRequest.federationName, lookupRequest.interactionClassHandle);
    if (!interactionClassName) {
      return rejected(request);
    }
    handle = registry_.parameterHandleFor(
        lookupRequest.federationName, *interactionClassName, *encodedName);
  }
  if (!handle) {
    return rejected(request);
  }
  return responseFor(
      request,
      TransportServiceStatus::ok,
      encodeProcessFederationHandleResult(ProcessFederationHandleResult{*handle}));
}

TransportServiceMessage ProcessFederationService::handleGetAttributeHandle(
    ProcessTransportSession& session,
    TransportServiceMessage const& request) {
  auto const lookupRequest =
      decodeProcessFederationGetAttributeHandleRequest(request.payload);
  auto const encodedName = utf8FromWide(lookupRequest.attributeName);
  if (!encodedName) {
    return rejected(request);
  }

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
    auto const objectClassName = registry_.objectClassNameFor(
        lookupRequest.federationName, lookupRequest.objectClassHandle);
    if (!objectClassName) {
      return rejected(request);
    }
    handle = registry_.attributeHandleFor(
        lookupRequest.federationName, *objectClassName, *encodedName);
  }
  if (!handle) {
    return rejected(request);
  }
  return responseFor(
      request,
      TransportServiceStatus::ok,
      encodeProcessFederationHandleResult(ProcessFederationHandleResult{*handle}));
}

TransportServiceMessage ProcessFederationService::handleGetObjectInstanceHandle(
    ProcessTransportSession& session,
    TransportServiceMessage const& request) {
  auto const lookupRequest =
      decodeProcessFederationGetObjectInstanceHandleRequest(request.payload);
  std::optional<KnownObjectInstanceSnapshot> known;
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
    known = registry_.knownObjectInstanceByNameFor(
        lookupRequest.federationName,
        lookupRequest.federateId,
        lookupRequest.objectInstanceName);
  }
  if (!known || known->objectInstanceHandle == 0U) {
    return rejected(request);
  }
  return responseFor(
      request,
      TransportServiceStatus::ok,
      encodeProcessFederationHandleResult(
          ProcessFederationHandleResult{known->objectInstanceHandle}));
}

TransportServiceMessage ProcessFederationService::handleGetObjectInstanceName(
    ProcessTransportSession& session,
    TransportServiceMessage const& request) {
  auto const lookupRequest =
      decodeProcessFederationGetObjectInstanceNameRequest(request.payload);
  std::optional<KnownObjectInstanceSnapshot> known;
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
    known = registry_.knownObjectInstanceFor(
        lookupRequest.federationName,
        lookupRequest.federateId,
        lookupRequest.objectInstanceHandle);
  }
  if (!known || known->objectInstanceName.empty()) {
    return rejected(request);
  }
  return responseFor(
      request,
      TransportServiceStatus::ok,
      encodeProcessFederationStringResult(
          ProcessFederationStringResult{std::move(known->objectInstanceName)}));
}

TransportServiceMessage
ProcessFederationService::handleGetKnownObjectClassHandle(
    ProcessTransportSession& session,
    TransportServiceMessage const& request) {
  auto const lookupRequest =
      decodeProcessFederationGetKnownObjectClassHandleRequest(request.payload);
  std::optional<KnownObjectInstanceSnapshot> known;
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
    known = registry_.knownObjectInstanceFor(
        lookupRequest.federationName,
        lookupRequest.federateId,
        lookupRequest.objectInstanceHandle);
  }
  if (!known || known->knownObjectClassHandle == 0U) {
    return rejected(request);
  }
  return responseFor(
      request,
      TransportServiceStatus::ok,
      encodeProcessFederationHandleResult(
          ProcessFederationHandleResult{known->knownObjectClassHandle}));
}

TransportServiceMessage ProcessFederationService::handleGetUpdateRateValue(
    ProcessTransportSession& session,
    TransportServiceMessage const& request) {
  auto const lookupRequest =
      decodeProcessFederationGetUpdateRateValueRequest(request.payload);
  UpdateRateValueResult result;
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
    result = registry_.updateRateValueForDesignator(
        lookupRequest.federationName,
        lookupRequest.federateId,
        lookupRequest.updateRateDesignator);
  }
  return responseFor(
      request,
      TransportServiceStatus::ok,
      encodeProcessFederationUpdateRateValueResult(
          ProcessFederationUpdateRateValueResult{
              processUpdateRateValueStatus(result.status), result.value}));
}

TransportServiceMessage
ProcessFederationService::handleGetUpdateRateValueForAttribute(
    ProcessTransportSession& session,
    TransportServiceMessage const& request) {
  auto const lookupRequest =
      decodeProcessFederationGetUpdateRateValueForAttributeRequest(
          request.payload);
  UpdateRateValueResult result;
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
    result = registry_.updateRateValueForAttribute(
        lookupRequest.federationName,
        lookupRequest.federateId,
        lookupRequest.objectInstanceHandle,
        lookupRequest.attributeHandle);
  }
  return responseFor(
      request,
      TransportServiceStatus::ok,
      encodeProcessFederationUpdateRateValueResult(
          ProcessFederationUpdateRateValueResult{
              processUpdateRateValueStatus(result.status), result.value}));
}

TransportServiceMessage ProcessFederationService::handleGetObjectClassName(
    ProcessTransportSession& session,
    TransportServiceMessage const& request) {
  auto const lookupRequest =
      decodeProcessFederationGetObjectClassNameRequest(request.payload);
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
    encodedName = registry_.objectClassNameFor(
        lookupRequest.federationName, lookupRequest.objectClassHandle);
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
          [objectClassHandle = lookupRequest.objectClassHandle,
           objectClassName = *decodedName](std::uint32_t serialNumber) {
            return formatMomSuccessfulServiceReportRecord(
                serialNumber,
                L"GetObjectClassName",
                {{MomArgumentType::object_class_handle,
                  L"Object class handle",
                  formatMomObjectClassHandle(
                      rti1516_2025::umbra_binding_detail::makeObjectClassHandle(
                          objectClassHandle))}},
                {MomArgumentType::string,
                 L"Object class name",
                 formatMomString(objectClassName)});
          })) {
    return internalError(request);
  }
  return responseFor(
      request,
      TransportServiceStatus::ok,
      encodeProcessFederationStringResult(
          ProcessFederationStringResult{*decodedName}));
}

TransportServiceMessage ProcessFederationService::handleGetInteractionClassName(
    ProcessTransportSession& session,
    TransportServiceMessage const& request) {
  auto const lookupRequest =
      decodeProcessFederationGetInteractionClassNameRequest(request.payload);
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
    encodedName = registry_.interactionClassNameFor(
        lookupRequest.federationName, lookupRequest.interactionClassHandle);
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
          [interactionClassHandle = lookupRequest.interactionClassHandle,
           interactionClassName = *decodedName](std::uint32_t serialNumber) {
            return formatMomSuccessfulServiceReportRecord(
                serialNumber,
                L"GetInteractionClassName",
                {{MomArgumentType::interaction_class_handle,
                  L"Interaction class handle",
                  formatMomInteractionClassHandle(
                      rti1516_2025::umbra_binding_detail::makeInteractionClassHandle(
                          interactionClassHandle))}},
                {MomArgumentType::string,
                 L"Interaction class name",
                 formatMomString(interactionClassName)});
          })) {
    return internalError(request);
  }
  return responseFor(
      request,
      TransportServiceStatus::ok,
      encodeProcessFederationStringResult(
          ProcessFederationStringResult{*decodedName}));
}

TransportServiceMessage ProcessFederationService::handleGetAttributeName(
    ProcessTransportSession& session,
    TransportServiceMessage const& request) {
  auto const lookupRequest =
      decodeProcessFederationGetAttributeNameRequest(request.payload);
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
    auto const objectClassName = registry_.objectClassNameFor(
        lookupRequest.federationName, lookupRequest.objectClassHandle);
    if (!objectClassName) {
      return rejected(request);
    }
    encodedName = registry_.attributeNameFor(
        lookupRequest.federationName,
        *objectClassName,
        lookupRequest.attributeHandle);
  }
  if (!encodedName) {
    return rejected(request);
  }
  auto const decodedName = wideFromUtf8(*encodedName);
  if (!decodedName) {
    return internalError(request);
  }
  return responseFor(
      request,
      TransportServiceStatus::ok,
      encodeProcessFederationStringResult(
          ProcessFederationStringResult{*decodedName}));
}

TransportServiceMessage ProcessFederationService::handleGetParameterName(
    ProcessTransportSession& session,
    TransportServiceMessage const& request) {
  auto const lookupRequest =
      decodeProcessFederationGetParameterNameRequest(request.payload);
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
    auto const interactionClassName = registry_.interactionClassNameFor(
        lookupRequest.federationName, lookupRequest.interactionClassHandle);
    if (!interactionClassName) {
      return rejected(request);
    }
    encodedName = registry_.parameterNameFor(
        lookupRequest.federationName,
        *interactionClassName,
        lookupRequest.parameterHandle);
  }
  if (!encodedName) {
    return rejected(request);
  }
  auto const decodedName = wideFromUtf8(*encodedName);
  if (!decodedName) {
    return internalError(request);
  }
  return responseFor(
      request,
      TransportServiceStatus::ok,
      encodeProcessFederationStringResult(
          ProcessFederationStringResult{*decodedName}));
}

}  // namespace umbra::detail
