#include "internal/federation/process_federation_service.hpp"
#include "internal/federation/process_federation_service_helpers.hpp"

#include <algorithm>
#include <cstdint>
#include <memory>
#include <optional>
#include <set>
#include <stdexcept>
#include <utility>
#include <vector>

namespace umbra::detail {

TransportServiceMessage ProcessFederationService::handleAttributeOwnershipCheck(
    ProcessTransportSession& session,
    TransportServiceMessage const& request) {
  auto const checkRequest =
      decodeProcessFederationAttributeOwnershipCheckRequest(request.payload);
  {
    std::scoped_lock lock(mutex_);
    auto const state = sessions_.find(&session);
    if (state == sessions_.end() ||
        !state->second.federationName.has_value() ||
        *state->second.federationName != checkRequest.federationName ||
        state->second.federateId != checkRequest.requestingFederateId ||
        !registry_.memberById(
            checkRequest.federationName,
            checkRequest.requestingFederateId)) {
      return rejected(request);
    }
  }

  auto const result = registry_.attributeOwnedByFederate(
      checkRequest.federationName,
      checkRequest.requestingFederateId,
      checkRequest.objectInstanceHandle,
      checkRequest.attributeHandle);
  ProcessFederationAttributeOwnershipCheckResult processResult{
      result.status,
      result.ownedByRequestingFederate};
  return responseFor(
      request,
      TransportServiceStatus::ok,
      encodeProcessFederationAttributeOwnershipCheckResult(processResult));
}

TransportServiceMessage ProcessFederationService::handleQueryAttributeOwnership(
    ProcessTransportSession& session,
    TransportServiceMessage const& request) {
  auto const queryRequest =
      decodeProcessFederationAttributeOwnershipQueryRequest(request.payload);
  {
    std::scoped_lock lock(mutex_);
    auto const state = sessions_.find(&session);
    if (state == sessions_.end() ||
        !state->second.federationName.has_value() ||
        *state->second.federationName != queryRequest.federationName ||
        state->second.federateId != queryRequest.requestingFederateId ||
        !registry_.memberById(
            queryRequest.federationName,
            queryRequest.requestingFederateId)) {
      return rejected(request);
    }
  }

  auto const plan = registry_.planAttributeOwnershipQuery(
      queryRequest.federationName,
      queryRequest.requestingFederateId,
      queryRequest.objectInstanceHandle,
      std::set<std::uint64_t>(
          queryRequest.requestedAttributeHandles.begin(),
          queryRequest.requestedAttributeHandles.end()));
  if (plan.status != AttributeOwnershipQueryStatus::applied) {
    return responseFor(
        request,
        TransportServiceStatus::ok,
        encodeProcessFederationAttributeOwnershipQueryResult(
            ProcessFederationAttributeOwnershipQueryResult{plan.status, 0U}));
  }

  std::vector<
      std::pair<ProcessTransportSession*,
                ProcessFederationAttributeOwnershipQueryEvent>> pushedEvents;
  std::uint32_t recipientCount = 0U;
  for (auto const& planned : plan.recipients) {
    if (planned.receivingFederateId != queryRequest.requestingFederateId ||
        planned.requestId == 0U || planned.attributeHandles.empty()) {
      return internalError(request);
    }
    ProcessTransportSession* receivingSession = nullptr;
    {
      std::scoped_lock lock(mutex_);
      auto const found =
          sessionsByFederateId_.find(planned.receivingFederateId);
      if (found != sessionsByFederateId_.end()) {
        auto const state = sessions_.find(found->second);
        if (state != sessions_.end() &&
            state->second.federationName.has_value() &&
            *state->second.federationName == queryRequest.federationName &&
            state->second.federateId == planned.receivingFederateId) {
          receivingSession = found->second;
        }
      }
      if (receivingSession == nullptr) {
        return internalError(request);
      }
      ProcessFederationAttributeOwnershipQueryEvent event{
          planned.requestId,
          planned.receivingFederateId,
          planned.objectInstanceHandle,
          planned.reportKind,
          planned.owningFederateId,
          planned.attributeHandles};
      if (options_.pushReceiveOrderEvents) {
        pushedEvents.emplace_back(receivingSession, std::move(event));
      } else {
        auto const state = sessions_.find(receivingSession);
        if (state == sessions_.end()) {
          return internalError(request);
        }
        state->second.attributeOwnershipQueryEvents.push_back(
            std::move(event));
      }
    }
    if (recipientCount != std::numeric_limits<std::uint32_t>::max()) {
      ++recipientCount;
    }
  }

  if (options_.pushReceiveOrderEvents) {
    for (auto& pushedEvent : pushedEvents) {
      ProcessFederationReceiveInteractionResult result;
      result.attributeOwnershipQueryEvent = std::move(pushedEvent.second);
      if (pushedEvent.first == nullptr ||
          !pushedEvent.first->send(TransportServiceMessage{
              TransportServiceMessageKind::event,
              TransportServiceOperation::receive_interaction,
              TransportServiceStatus::ok,
              0U,
              encodeProcessFederationReceiveInteractionResult(result)})) {
        return internalError(request);
      }
    }
  }
  return responseFor(
      request,
      TransportServiceStatus::ok,
      encodeProcessFederationAttributeOwnershipQueryResult(
          ProcessFederationAttributeOwnershipQueryResult{
              AttributeOwnershipQueryStatus::applied,
              recipientCount}));
}

TransportServiceMessage
ProcessFederationService::handleAttributeOwnershipAcquisition(
    ProcessTransportSession& session,
    TransportServiceMessage const& request) {
  auto const acquisitionRequest =
      decodeProcessFederationAttributeOwnershipAcquisitionRequest(
          request.payload);
  {
    std::scoped_lock lock(mutex_);
    auto const state = sessions_.find(&session);
    if (state == sessions_.end() ||
        !state->second.federationName.has_value() ||
        *state->second.federationName != acquisitionRequest.federationName ||
        state->second.federateId != acquisitionRequest.requestingFederateId ||
        !registry_.memberById(
            acquisitionRequest.federationName,
            acquisitionRequest.requestingFederateId)) {
      return rejected(request);
    }
  }

  auto plan = registry_.planAttributeOwnershipAcquisition(
      acquisitionRequest.federationName,
      acquisitionRequest.requestingFederateId,
      acquisitionRequest.objectInstanceHandle,
      std::set<std::uint64_t>(
          acquisitionRequest.desiredAttributeHandles.begin(),
          acquisitionRequest.desiredAttributeHandles.end()),
      acquisitionRequest.userSuppliedTag);
  if (plan.status != AttributeOwnershipAcquisitionStatus::applied) {
    return responseFor(
        request,
        TransportServiceStatus::ok,
        encodeProcessFederationAttributeOwnershipAcquisitionResult(
            ProcessFederationAttributeOwnershipAcquisitionResult{
                plan.status,
                0U}));
  }
  auto const recipientCount = static_cast<std::uint32_t>(std::min<std::size_t>(
      plan.workItems.size(), std::numeric_limits<std::uint32_t>::max()));
  if (!plan.workItems.empty() &&
      !enqueueAttributeOwnershipAcquisitionWorkItems(
          acquisitionRequest.federationName,
          std::move(plan.workItems))) {
    return internalError(request);
  }
  return responseFor(
      request,
      TransportServiceStatus::ok,
      encodeProcessFederationAttributeOwnershipAcquisitionResult(
          ProcessFederationAttributeOwnershipAcquisitionResult{
              AttributeOwnershipAcquisitionStatus::applied,
              recipientCount}));
}

TransportServiceMessage
ProcessFederationService::handleAttributeOwnershipAcquisitionIfAvailable(
    ProcessTransportSession& session,
    TransportServiceMessage const& request) {
  auto const acquisitionRequest =
      decodeProcessFederationAttributeOwnershipAcquisitionIfAvailableRequest(
          request.payload);
  {
    std::scoped_lock lock(mutex_);
    auto const state = sessions_.find(&session);
    if (state == sessions_.end() ||
        !state->second.federationName.has_value() ||
        *state->second.federationName != acquisitionRequest.federationName ||
        state->second.federateId != acquisitionRequest.requestingFederateId ||
        !registry_.memberById(
            acquisitionRequest.federationName,
            acquisitionRequest.requestingFederateId)) {
      return rejected(request);
    }
  }

  auto const plan = registry_.planAttributeOwnershipAcquisitionIfAvailable(
      acquisitionRequest.federationName,
      acquisitionRequest.requestingFederateId,
      acquisitionRequest.objectInstanceHandle,
      std::set<std::uint64_t>(
          acquisitionRequest.desiredAttributeHandles.begin(),
          acquisitionRequest.desiredAttributeHandles.end()),
      acquisitionRequest.userSuppliedTag);
  if (plan.status != AttributeOwnershipAcquisitionIfAvailableStatus::applied) {
    return responseFor(
        request,
        TransportServiceStatus::ok,
        encodeProcessFederationAttributeOwnershipAcquisitionIfAvailableResult(
            ProcessFederationAttributeOwnershipAcquisitionIfAvailableResult{
                plan.status,
                0U}));
  }
  if (plan.requestId == 0U) {
    return responseFor(
        request,
        TransportServiceStatus::ok,
        encodeProcessFederationAttributeOwnershipAcquisitionIfAvailableResult(
            ProcessFederationAttributeOwnershipAcquisitionIfAvailableResult{
                AttributeOwnershipAcquisitionIfAvailableStatus::applied,
                0U}));
  }

  ProcessFederationAttributeOwnershipAcquisitionIfAvailableEvent event{
      plan.requestId,
      acquisitionRequest.requestingFederateId,
      acquisitionRequest.objectInstanceHandle,
      {},
      {},
      acquisitionRequest.userSuppliedTag};
  if (options_.pushReceiveOrderEvents) {
    auto const delivery = registry_.beginAttributeOwnershipAcquisitionIfAvailable(
        acquisitionRequest.federationName,
        acquisitionRequest.requestingFederateId,
        acquisitionRequest.objectInstanceHandle,
        plan.requestId);
    if (!delivery) {
      return internalError(request);
    }
    event.securedAttributeHandles = delivery->securedAttributeHandles;
    event.unavailableAttributeHandles = delivery->unavailableAttributeHandles;
    ProcessFederationReceiveInteractionResult pushedResult;
    pushedResult.attributeOwnershipAcquisitionIfAvailableEvent =
        std::move(event);
    if (!session.send(TransportServiceMessage{
            TransportServiceMessageKind::event,
            TransportServiceOperation::receive_interaction,
            TransportServiceStatus::ok,
            0U,
            encodeProcessFederationReceiveInteractionResult(pushedResult)})) {
      return internalError(request);
    }
  } else {
    std::scoped_lock lock(mutex_);
    auto const state = sessions_.find(&session);
    if (state == sessions_.end() ||
        !state->second.federationName.has_value() ||
        *state->second.federationName != acquisitionRequest.federationName ||
        state->second.federateId != acquisitionRequest.requestingFederateId) {
      registry_.cancelAttributeOwnershipAcquisitionIfAvailable(
          acquisitionRequest.federationName,
          acquisitionRequest.requestingFederateId,
          acquisitionRequest.objectInstanceHandle,
          plan.requestId);
      return rejected(request);
    }
    state->second.attributeOwnershipAcquisitionIfAvailableEvents.push_back(
        std::move(event));
  }
  return responseFor(
      request,
      TransportServiceStatus::ok,
      encodeProcessFederationAttributeOwnershipAcquisitionIfAvailableResult(
      ProcessFederationAttributeOwnershipAcquisitionIfAvailableResult{
              AttributeOwnershipAcquisitionIfAvailableStatus::applied,
              1U}));
}

TransportServiceMessage
ProcessFederationService::handleAttributeOwnershipReleaseDenied(
    ProcessTransportSession& session,
    TransportServiceMessage const& request) {
  auto const denialRequest =
      decodeProcessFederationAttributeOwnershipReleaseDeniedRequest(
          request.payload);
  {
    std::scoped_lock lock(mutex_);
    auto const state = sessions_.find(&session);
    if (state == sessions_.end() ||
        !state->second.federationName.has_value() ||
        *state->second.federationName != denialRequest.federationName ||
        state->second.federateId != denialRequest.owningFederateId ||
        !registry_.memberById(
            denialRequest.federationName,
            denialRequest.owningFederateId)) {
      return rejected(request);
    }
  }

  auto plan = registry_.planAttributeOwnershipReleaseDenied(
      denialRequest.federationName,
      denialRequest.owningFederateId,
      denialRequest.objectInstanceHandle,
      std::set<std::uint64_t>(
          denialRequest.attributeHandles.begin(),
          denialRequest.attributeHandles.end()));
  if (plan.status != AttributeOwnershipReleaseDeniedStatus::applied) {
    return responseFor(
        request,
        TransportServiceStatus::ok,
        encodeProcessFederationAttributeOwnershipReleaseDeniedResult(
            ProcessFederationAttributeOwnershipReleaseDeniedResult{
                plan.status,
                0U}));
  }

  auto const recipientCount = static_cast<std::uint32_t>(std::min<std::size_t>(
      plan.recipients.size(), std::numeric_limits<std::uint32_t>::max()));
  if (!plan.recipients.empty() &&
      !enqueueAttributeOwnershipUnavailableRecipients(
          denialRequest.federationName,
          std::move(plan.recipients),
          denialRequest.userSuppliedTag)) {
    return internalError(request);
  }
  if (!plan.followupWorkItems.empty() &&
      !enqueueAttributeOwnershipAcquisitionWorkItems(
          denialRequest.federationName,
          std::move(plan.followupWorkItems))) {
    return internalError(request);
  }
  return responseFor(
      request,
      TransportServiceStatus::ok,
      encodeProcessFederationAttributeOwnershipReleaseDeniedResult(
          ProcessFederationAttributeOwnershipReleaseDeniedResult{
              AttributeOwnershipReleaseDeniedStatus::applied,
              recipientCount}));
}

TransportServiceMessage
ProcessFederationService::handleAttributeOwnershipAcquisitionCancellation(
    ProcessTransportSession& session,
    TransportServiceMessage const& request) {
  auto const cancellationRequest =
      decodeProcessFederationAttributeOwnershipAcquisitionCancellationRequest(
          request.payload);
  {
    std::scoped_lock lock(mutex_);
    auto const state = sessions_.find(&session);
    if (state == sessions_.end() ||
        !state->second.federationName.has_value() ||
        *state->second.federationName != cancellationRequest.federationName ||
        state->second.federateId != cancellationRequest.requestingFederateId ||
        !registry_.memberById(
            cancellationRequest.federationName,
            cancellationRequest.requestingFederateId)) {
      return rejected(request);
    }
  }

  auto plan = registry_.planAttributeOwnershipAcquisitionCancellation(
      cancellationRequest.federationName,
      cancellationRequest.requestingFederateId,
      cancellationRequest.objectInstanceHandle,
      std::set<std::uint64_t>(
          cancellationRequest.attributeHandles.begin(),
          cancellationRequest.attributeHandles.end()));
  if (plan.status !=
      AttributeOwnershipAcquisitionCancellationStatus::applied) {
    return responseFor(
        request,
        TransportServiceStatus::ok,
        encodeProcessFederationAttributeOwnershipAcquisitionCancellationResult(
            ProcessFederationAttributeOwnershipAcquisitionCancellationResult{
                plan.status,
                0U}));
  }
  // A supplied-empty cancellation is successful but has no callback
  // reservation. Preserve that distinction in the typed result.
  if (plan.cancellationId == 0U || plan.attributeHandles.empty()) {
    return responseFor(
        request,
        TransportServiceStatus::ok,
        encodeProcessFederationAttributeOwnershipAcquisitionCancellationResult(
            ProcessFederationAttributeOwnershipAcquisitionCancellationResult{
                AttributeOwnershipAcquisitionCancellationStatus::applied,
                0U}));
  }

  ProcessFederationAttributeOwnershipAcquisitionEvent event{
      ProcessFederationAttributeOwnershipAcquisitionEventKind::
          cancellation_confirmation,
      plan.cancellationId,
      cancellationRequest.requestingFederateId,
      cancellationRequest.requestingFederateId,
      cancellationRequest.objectInstanceHandle,
      plan.attributeHandles,
      {}};
  std::uint32_t recipientCount = 0U;
  if (options_.pushReceiveOrderEvents) {
    // Pushed process events cross the callback fence here, before the
    // unsolicited payload is sent to the client. Pull mode performs the same
    // transition in handleReceiveInteraction below.
    auto const delivery = registry_.beginAttributeOwnershipAcquisitionCancellation(
        cancellationRequest.federationName,
        cancellationRequest.requestingFederateId,
        cancellationRequest.objectInstanceHandle,
        plan.cancellationId,
        plan.attributeHandles);
    if (delivery && !delivery->confirmedAttributeHandles.empty()) {
      event.attributeHandles = delivery->confirmedAttributeHandles;
      ProcessFederationReceiveInteractionResult pushedResult;
      pushedResult.attributeOwnershipAcquisitionEvent = event;
      if (!session.send(TransportServiceMessage{
              TransportServiceMessageKind::event,
              TransportServiceOperation::receive_interaction,
              TransportServiceStatus::ok,
              0U,
              encodeProcessFederationReceiveInteractionResult(pushedResult)})) {
        return internalError(request);
      }
      recipientCount = 1U;
      if (!delivery->followupWorkItems.empty() &&
          !enqueueAttributeOwnershipAcquisitionWorkItems(
              cancellationRequest.federationName,
              std::move(delivery->followupWorkItems))) {
        return internalError(request);
      }
    }
  } else {
    std::scoped_lock lock(mutex_);
    auto const state = sessions_.find(&session);
    if (state == sessions_.end() ||
        !state->second.federationName.has_value() ||
        *state->second.federationName != cancellationRequest.federationName ||
        state->second.federateId != cancellationRequest.requestingFederateId) {
      return rejected(request);
    }
    state->second.attributeOwnershipAcquisitionEvents.push_back(
        std::move(event));
    recipientCount = 1U;
  }

  return responseFor(
      request,
      TransportServiceStatus::ok,
      encodeProcessFederationAttributeOwnershipAcquisitionCancellationResult(
          ProcessFederationAttributeOwnershipAcquisitionCancellationResult{
              AttributeOwnershipAcquisitionCancellationStatus::applied,
              recipientCount}));
}

TransportServiceMessage
ProcessFederationService::handleCancelNegotiatedAttributeOwnershipDivestiture(
    ProcessTransportSession& session,
    TransportServiceMessage const& request) {
  auto const cancellationRequest =
      decodeProcessFederationCancelNegotiatedAttributeOwnershipDivestitureRequest(
          request.payload);
  {
    std::scoped_lock lock(mutex_);
    auto const state = sessions_.find(&session);
    if (state == sessions_.end() ||
        !state->second.federationName.has_value() ||
        *state->second.federationName != cancellationRequest.federationName ||
        state->second.federateId != cancellationRequest.divestingFederateId ||
        !registry_.memberById(
            cancellationRequest.federationName,
            cancellationRequest.divestingFederateId)) {
      return rejected(request);
    }
  }

  auto plan = registry_.planCancelNegotiatedAttributeOwnershipDivestiture(
      cancellationRequest.federationName,
      cancellationRequest.divestingFederateId,
      cancellationRequest.objectInstanceHandle,
      std::set<std::uint64_t>(
          cancellationRequest.attributeHandles.begin(),
          cancellationRequest.attributeHandles.end()));
  if (plan.status !=
      CancelNegotiatedAttributeOwnershipDivestitureStatus::applied) {
    return responseFor(
        request,
        TransportServiceStatus::ok,
        encodeProcessFederationCancelNegotiatedAttributeOwnershipDivestitureResult(
            ProcessFederationCancelNegotiatedAttributeOwnershipDivestitureResult{
                plan.status,
                0U}));
  }

  auto const recipientCount = static_cast<std::uint32_t>(std::min<std::size_t>(
      plan.followupWorkItems.size(), std::numeric_limits<std::uint32_t>::max()));
  if (!plan.followupWorkItems.empty() &&
      !enqueueAttributeOwnershipAcquisitionWorkItems(
          cancellationRequest.federationName,
          std::move(plan.followupWorkItems))) {
    return internalError(request);
  }
  return responseFor(
      request,
      TransportServiceStatus::ok,
      encodeProcessFederationCancelNegotiatedAttributeOwnershipDivestitureResult(
          ProcessFederationCancelNegotiatedAttributeOwnershipDivestitureResult{
              CancelNegotiatedAttributeOwnershipDivestitureStatus::applied,
              recipientCount}));
}

TransportServiceMessage
ProcessFederationService::handleNegotiatedAttributeOwnershipDivestiture(
    ProcessTransportSession& session,
    TransportServiceMessage const& request) {
  auto const divestitureRequest =
      decodeProcessFederationNegotiatedAttributeOwnershipDivestitureRequest(
          request.payload);
  {
    std::scoped_lock lock(mutex_);
    auto const state = sessions_.find(&session);
    if (state == sessions_.end() ||
        !state->second.federationName.has_value() ||
        *state->second.federationName != divestitureRequest.federationName ||
        state->second.federateId != divestitureRequest.divestingFederateId ||
        !registry_.memberById(
            divestitureRequest.federationName,
            divestitureRequest.divestingFederateId)) {
      return rejected(request);
    }
  }

  auto plan = registry_.planNegotiatedAttributeOwnershipDivestiture(
      divestitureRequest.federationName,
      divestitureRequest.divestingFederateId,
      divestitureRequest.objectInstanceHandle,
      std::set<std::uint64_t>(
          divestitureRequest.attributeHandles.begin(),
          divestitureRequest.attributeHandles.end()),
      divestitureRequest.userSuppliedTag);
  if (plan.status !=
      NegotiatedAttributeOwnershipDivestitureStatus::applied) {
    return responseFor(
        request,
        TransportServiceStatus::ok,
        encodeProcessFederationNegotiatedAttributeOwnershipDivestitureResult(
            ProcessFederationNegotiatedAttributeOwnershipDivestitureResult{
                plan.status,
                0U}));
  }

  auto const workItemCount = plan.workItems.size();
  auto const assumptionCount = plan.assumptionRecipients.size();
  auto const recipientCount = static_cast<std::uint32_t>(std::min<std::size_t>(
      workItemCount + assumptionCount,
      std::numeric_limits<std::uint32_t>::max()));
  if (!plan.workItems.empty() &&
      !enqueueAttributeOwnershipAcquisitionWorkItems(
          divestitureRequest.federationName,
          std::move(plan.workItems))) {
    return internalError(request);
  }
  if (!plan.assumptionRecipients.empty() &&
      !enqueueAttributeOwnershipAssumptionRecipients(
          divestitureRequest.federationName,
          std::move(plan.assumptionRecipients))) {
    return internalError(request);
  }
  return responseFor(
      request,
      TransportServiceStatus::ok,
      encodeProcessFederationNegotiatedAttributeOwnershipDivestitureResult(
          ProcessFederationNegotiatedAttributeOwnershipDivestitureResult{
              NegotiatedAttributeOwnershipDivestitureStatus::applied,
              recipientCount}));
}

TransportServiceMessage ProcessFederationService::handleConfirmDivestiture(
    ProcessTransportSession& session,
    TransportServiceMessage const& request) {
  auto const confirmRequest =
      decodeProcessFederationConfirmDivestitureRequest(request.payload);
  {
    std::scoped_lock lock(mutex_);
    auto const state = sessions_.find(&session);
    if (state == sessions_.end() ||
        !state->second.federationName.has_value() ||
        *state->second.federationName != confirmRequest.federationName ||
        state->second.federateId != confirmRequest.divestingFederateId ||
        !registry_.memberById(
            confirmRequest.federationName,
            confirmRequest.divestingFederateId)) {
      return rejected(request);
    }
  }

  auto plan = registry_.planConfirmDivestiture(
      confirmRequest.federationName,
      confirmRequest.divestingFederateId,
      confirmRequest.objectInstanceHandle,
      std::set<std::uint64_t>(
          confirmRequest.attributeHandles.begin(),
          confirmRequest.attributeHandles.end()),
      confirmRequest.userSuppliedTag);
  if (plan.status != ConfirmDivestitureStatus::applied) {
    return responseFor(
        request,
        TransportServiceStatus::ok,
        encodeProcessFederationConfirmDivestitureResult(
            ProcessFederationConfirmDivestitureResult{plan.status, 0U}));
  }

  auto const recipientCount = static_cast<std::uint32_t>(std::min<std::size_t>(
      plan.notifications.size(), std::numeric_limits<std::uint32_t>::max()));
  if (!plan.notifications.empty() &&
      !enqueueConfirmDivestitureNotifications(
          confirmRequest.federationName, std::move(plan.notifications))) {
    return internalError(request);
  }
  return responseFor(
      request,
      TransportServiceStatus::ok,
      encodeProcessFederationConfirmDivestitureResult(
          ProcessFederationConfirmDivestitureResult{
              ConfirmDivestitureStatus::applied,
              recipientCount}));
}

TransportServiceMessage
ProcessFederationService::handleUnconditionalAttributeOwnershipDivestiture(
    ProcessTransportSession& session,
    TransportServiceMessage const& request) {
  // The private request shape is shared with regular acquisition: both
  // services carry one federation/member/object/attribute-set/tag tuple. The
  // operation identity keeps the state transition distinct at the service
  // boundary.
  auto const divestitureRequest =
      decodeProcessFederationAttributeOwnershipAcquisitionRequest(
          request.payload);
  {
    std::scoped_lock lock(mutex_);
    auto const state = sessions_.find(&session);
    if (state == sessions_.end() ||
        !state->second.federationName.has_value() ||
        *state->second.federationName != divestitureRequest.federationName ||
        state->second.federateId != divestitureRequest.requestingFederateId ||
        !registry_.memberById(
            divestitureRequest.federationName,
            divestitureRequest.requestingFederateId)) {
      return rejected(request);
    }
  }

  auto const plan = registry_.planUnconditionalAttributeOwnershipDivestiture(
      divestitureRequest.federationName,
      divestitureRequest.requestingFederateId,
      divestitureRequest.objectInstanceHandle,
      std::set<std::uint64_t>(
          divestitureRequest.desiredAttributeHandles.begin(),
          divestitureRequest.desiredAttributeHandles.end()),
      divestitureRequest.userSuppliedTag);
  if (plan.status !=
      UnconditionalAttributeOwnershipDivestitureStatus::applied) {
    return rejected(request);
  }

  if (!plan.acquisitionWorkItems.empty() &&
      !enqueueAttributeOwnershipAcquisitionWorkItems(
          divestitureRequest.federationName,
          std::move(plan.acquisitionWorkItems))) {
    return internalError(request);
  }
  if (!plan.assumptionRecipients.empty() &&
      !enqueueAttributeOwnershipAssumptionRecipients(
          divestitureRequest.federationName,
          std::move(plan.assumptionRecipients),
          !divestitureRequest.callbacksEnabled)) {
    return internalError(request);
  }
  return responseFor(
      request,
      TransportServiceStatus::ok,
      encodeProcessFederationBooleanResult(
          ProcessFederationBooleanResult{true}));
}
}  // namespace umbra::detail
