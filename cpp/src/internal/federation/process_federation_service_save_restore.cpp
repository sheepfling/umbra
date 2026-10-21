#include "internal/federation/process_federation_service.hpp"
#include "internal/federation/process_federation_service_helpers.hpp"

#include <memory>
#include <mutex>
#include <algorithm>
#include <utility>

namespace umbra::detail {

TransportServiceMessage ProcessFederationService::handleRequestFederationSave(
    ProcessTransportSession& session,
    TransportServiceMessage const& request) {
  auto const saveRequest = decodeProcessFederationSaveRequest(request.payload);
  if (saveRequest.label.empty()) {
    return rejected(request);
  }
  {
    std::scoped_lock lock(mutex_);
    auto const state = sessions_.find(&session);
    if (state == sessions_.end() ||
        !state->second.federationName.has_value() ||
        *state->second.federationName != saveRequest.federationName ||
        state->second.federateId != saveRequest.federateId ||
        !registry_.memberById(
            saveRequest.federationName, saveRequest.federateId)) {
      return rejected(request);
    }
  }
  std::shared_ptr<rti1516_2025::LogicalTime const> timestamp;
  if (saveRequest.timestamp) {
    auto const definition = registry_.definitionFor(saveRequest.federationName);
    if (!definition) {
      return responseFor(
          request,
          TransportServiceStatus::ok,
          encodeProcessFederationSaveControlResult(
              ProcessFederationSaveControlResult{
                  FederationSaveControlStatus::invalid_timed_save}));
    }
    timestamp = decodeProcessLogicalTime(
        *saveRequest.timestamp,
        definition->logicalTimeImplementationName);
  }
  FederationSaveControlResult result;
  {
    std::scoped_lock lock(mutex_);
    std::vector<FederationStateImageReceiveOrderInteraction> queued;
    std::vector<FederationStateImageReceiveOrderAttributeUpdate>
        queuedAttributeUpdates;
    if (!options_.pushReceiveOrderEvents) {
      for (auto const& [sessionPointer, state] : sessions_) {
        static_cast<void>(sessionPointer);
        if (!state.federationName.has_value() ||
            *state.federationName != saveRequest.federationName) {
          continue;
        }
        std::uint64_t sequence = 0U;
        for (auto const& event : state.interactionEvents) {
          // This first restore slice persists only ordinary, undirected RO
          // application interactions. Reject a save before admission rather
          // than silently omitting an unsupported queued interaction family.
          if (event.timestamp || event.objectInstanceHandle ||
              event.retractionMessageId || event.sentRegionHandles ||
              event.defaultRegionUsed || event.sentOrderType ||
              event.receivedOrderType || event.rtiOwnedMomInteraction ||
              event.exceptionReportFederateId ||
              event.producingFederateId == 0U ||
              event.receivingFederateId == 0U ||
              event.interactionClassHandle == 0U ||
              event.transportationName.empty()) {
            return internalError(request);
          }
          queued.push_back({
              ++sequence,
              event.producingFederateId,
              event.receivingFederateId,
              event.interactionClassHandle,
              event.parameterHandles,
              std::string(event.payload.begin(), event.payload.end()),
              event.transportationName});
        }
        sequence = 0U;
        for (auto const& event : state.attributeUpdateEvents) {
          // Attribute callbacks use the same route-free polling image only
          // for ordinary, nonregional receive-order application updates.
          // Reject unsupported queued families instead of making them vanish
          // at a successful save boundary.
          if (event.timestamp || event.retractionMessageId ||
              event.sentRegionHandles || event.defaultRegionUsed ||
              event.rtiOwnedMomObject ||
              event.producingFederateId == 0U ||
              event.receivingFederateId == 0U ||
              event.objectInstanceHandle == 0U ||
              event.transportationName.empty() ||
              event.attributeValues.empty()) {
            return internalError(request);
          }
          FederationStateImageReceiveOrderAttributeUpdate saved;
          saved.sequence = ++sequence;
          saved.producingFederateId = event.producingFederateId;
          saved.receivingFederateId = event.receivingFederateId;
          saved.objectInstanceHandle = event.objectInstanceHandle;
          saved.userSuppliedTag.assign(
              event.userSuppliedTag.begin(), event.userSuppliedTag.end());
          saved.transportationName = event.transportationName;
          saved.recheckAtReceive = event.recheckAtReceive;
          saved.pushDeliveryId = event.pushDeliveryId;
          saved.values.reserve(event.attributeValues.size());
          for (auto const& [attributeHandle, value] : event.attributeValues) {
            if (attributeHandle == 0U) {
              return internalError(request);
            }
            saved.values.push_back({
                attributeHandle,
                std::string(value.begin(), value.end())});
          }
          queuedAttributeUpdates.push_back(std::move(saved));
        }
      }
    }
    if (options_.pushReceiveOrderEvents) {
      for (auto const& [sessionPointer, state] : sessions_) {
        static_cast<void>(sessionPointer);
        if (!state.federationName ||
            *state.federationName != saveRequest.federationName) {
          continue;
        }
        std::map<std::uint64_t, std::uint64_t> nextSequenceByRecipient;
        for (auto const& [deliveryId, event] :
             state.pendingPushedAttributeUpdates) {
          static_cast<void>(deliveryId);
          if (event.timestamp || event.recheckAtReceive ||
              event.sentRegionHandles || event.defaultRegionUsed ||
              event.rtiOwnedMomObject || event.retractionMessageId ||
              event.producingFederateId == 0U ||
              event.receivingFederateId == 0U ||
              event.objectInstanceHandle == 0U ||
              event.transportationName.empty() ||
              event.attributeValues.empty()) {
            return internalError(request);
          }
          FederationStateImageReceiveOrderAttributeUpdate saved;
          saved.sequence = ++nextSequenceByRecipient[event.receivingFederateId];
          saved.producingFederateId = event.producingFederateId;
          saved.receivingFederateId = event.receivingFederateId;
          saved.objectInstanceHandle = event.objectInstanceHandle;
          saved.userSuppliedTag.assign(
              event.userSuppliedTag.begin(), event.userSuppliedTag.end());
          saved.transportationName = event.transportationName;
          saved.pushDeliveryId = event.pushDeliveryId;
          saved.values.reserve(event.attributeValues.size());
          for (auto const& [attributeHandle, value] : event.attributeValues) {
            if (attributeHandle == 0U) {
              return internalError(request);
            }
            saved.values.push_back({
                attributeHandle,
                std::string(value.begin(), value.end())});
          }
          queuedAttributeUpdates.push_back(std::move(saved));
        }
      }
    }
    std::sort(
        queued.begin(),
        queued.end(),
        [](auto const& first, auto const& second) {
          return std::pair{first.receivingFederateId, first.sequence} <
              std::pair{second.receivingFederateId, second.sequence};
        });
    std::sort(
        queuedAttributeUpdates.begin(),
        queuedAttributeUpdates.end(),
        [](auto const& first, auto const& second) {
          return std::pair{first.receivingFederateId, first.sequence} <
              std::pair{second.receivingFederateId, second.sequence};
        });
    registry_.setQueuedReceiveOrderInteractionsForSave(
        saveRequest.federationName, std::move(queued));
    registry_.setQueuedReceiveOrderAttributeUpdatesForSave(
        saveRequest.federationName, std::move(queuedAttributeUpdates));
    result = timestamp
        ? registry_.requestFederationSave(
              saveRequest.federationName,
              saveRequest.federateId,
              saveRequest.label,
              std::move(timestamp))
        : registry_.requestFederationSave(
              saveRequest.federationName,
              saveRequest.federateId,
              saveRequest.label);
  }
  if (result.status == FederationSaveControlStatus::applied &&
      !result.notifications.empty() &&
      !enqueueFederationSaveNotifications(
          saveRequest.federationName,
          std::move(result.notifications))) {
    return internalError(request);
  }
  return responseFor(
      request,
      TransportServiceStatus::ok,
      encodeProcessFederationSaveControlResult(
          ProcessFederationSaveControlResult{result.status}));
}

TransportServiceMessage ProcessFederationService::handleFederateSaveControl(
    ProcessTransportSession& session,
    TransportServiceMessage const& request,
    TransportServiceOperation operation) {
  auto const saveRequest = decodeProcessFederationSaveRequest(request.payload);
  if (!saveRequest.label.empty()) {
    return rejected(request);
  }
  {
    std::scoped_lock lock(mutex_);
    auto const state = sessions_.find(&session);
    if (state == sessions_.end() ||
        !state->second.federationName.has_value() ||
        *state->second.federationName != saveRequest.federationName ||
        state->second.federateId != saveRequest.federateId ||
        !registry_.memberById(
            saveRequest.federationName, saveRequest.federateId)) {
      return rejected(request);
    }
  }
  FederationSaveControlResult result;
  if (operation == TransportServiceOperation::federate_save_begun) {
    result = registry_.federateSaveBegun(
        saveRequest.federationName, saveRequest.federateId);
  } else if (operation == TransportServiceOperation::federate_save_complete) {
    result = registry_.federateSaveComplete(
        saveRequest.federationName, saveRequest.federateId);
  } else {
    result = registry_.federateSaveNotComplete(
        saveRequest.federationName, saveRequest.federateId);
  }
  if (result.status == FederationSaveControlStatus::applied &&
      !result.notifications.empty() &&
      !enqueueFederationSaveNotifications(
          saveRequest.federationName,
          std::move(result.notifications))) {
    return internalError(request);
  }
  return responseFor(
      request,
      TransportServiceStatus::ok,
      encodeProcessFederationSaveControlResult(
          ProcessFederationSaveControlResult{result.status}));
}

TransportServiceMessage ProcessFederationService::handleQueryFederationSaveStatus(
    ProcessTransportSession& session,
    TransportServiceMessage const& request) {
  auto const saveRequest = decodeProcessFederationSaveRequest(request.payload);
  if (!saveRequest.label.empty()) {
    return rejected(request);
  }
  {
    std::scoped_lock lock(mutex_);
    auto const state = sessions_.find(&session);
    if (state == sessions_.end() ||
        !state->second.federationName.has_value() ||
        *state->second.federationName != saveRequest.federationName ||
        state->second.federateId != saveRequest.federateId ||
        !registry_.memberById(
            saveRequest.federationName, saveRequest.federateId)) {
      return rejected(request);
    }
  }
  auto result = registry_.queryFederationSaveStatus(
      saveRequest.federationName, saveRequest.federateId);
  if (result.status == FederationSaveControlStatus::applied &&
      !result.notifications.empty() &&
      !enqueueFederationSaveNotifications(
          saveRequest.federationName,
          std::move(result.notifications))) {
    return internalError(request);
  }
  return responseFor(
      request,
      TransportServiceStatus::ok,
      encodeProcessFederationSaveControlResult(
          ProcessFederationSaveControlResult{result.status}));
}

TransportServiceMessage ProcessFederationService::handleAbortFederationSave(
    ProcessTransportSession& session,
    TransportServiceMessage const& request) {
  auto const saveRequest = decodeProcessFederationSaveRequest(request.payload);
  if (!saveRequest.label.empty()) {
    return rejected(request);
  }
  {
    std::scoped_lock lock(mutex_);
    auto const state = sessions_.find(&session);
    if (state == sessions_.end() ||
        !state->second.federationName.has_value() ||
        *state->second.federationName != saveRequest.federationName ||
        state->second.federateId != saveRequest.federateId ||
        !registry_.memberById(
            saveRequest.federationName, saveRequest.federateId)) {
      return rejected(request);
    }
  }
  auto result = registry_.abortFederationSave(
      saveRequest.federationName, saveRequest.federateId);
  if (result.status == FederationSaveControlStatus::applied &&
      !result.notifications.empty() &&
      !enqueueFederationSaveNotifications(
          saveRequest.federationName,
          std::move(result.notifications))) {
    return internalError(request);
  }
  return responseFor(
      request,
      TransportServiceStatus::ok,
      encodeProcessFederationSaveControlResult(
          ProcessFederationSaveControlResult{result.status}));
}

TransportServiceMessage ProcessFederationService::handleRequestFederationRestore(
    ProcessTransportSession& session,
    TransportServiceMessage const& request) {
  auto const restoreRequest =
      decodeProcessFederationRestoreRequest(request.payload);
  if (restoreRequest.label.empty()) {
    return rejected(request);
  }
  {
    std::scoped_lock lock(mutex_);
    auto const state = sessions_.find(&session);
    if (state == sessions_.end() ||
        !state->second.federationName.has_value() ||
        *state->second.federationName != restoreRequest.federationName ||
        state->second.federateId != restoreRequest.federateId ||
        !registry_.memberById(
            restoreRequest.federationName, restoreRequest.federateId)) {
      return rejected(request);
    }
  }

  auto result = registry_.requestFederationRestore(
      restoreRequest.federationName,
      restoreRequest.federateId,
      restoreRequest.label);
  if (!result.notifications.empty() &&
      !enqueueFederationRestoreNotifications(
          restoreRequest.federationName,
          std::move(result.notifications))) {
    return internalError(request);
  }
  return responseFor(
      request,
      TransportServiceStatus::ok,
      encodeProcessFederationRestoreControlResult(
          ProcessFederationRestoreControlResult{result.status}));
}

TransportServiceMessage ProcessFederationService::handleFederateRestoreComplete(
    ProcessTransportSession& session,
    TransportServiceMessage const& request) {
  auto const restoreRequest =
      decodeProcessFederationRestoreRequest(request.payload);
  if (!restoreRequest.label.empty()) {
    return rejected(request);
  }
  {
    std::scoped_lock lock(mutex_);
    auto const state = sessions_.find(&session);
    if (state == sessions_.end() ||
        !state->second.federationName.has_value() ||
        *state->second.federationName != restoreRequest.federationName ||
        state->second.federateId != restoreRequest.federateId ||
        !registry_.memberById(
            restoreRequest.federationName, restoreRequest.federateId)) {
      return rejected(request);
    }
  }

  auto result = registry_.federateRestoreComplete(
      restoreRequest.federationName,
      restoreRequest.federateId);
  bool const successfulRestoreCompleted = std::any_of(
      result.notifications.begin(),
      result.notifications.end(),
      [](FederationRestoreNotification const& notification) {
        return notification.kind == FederationRestoreNotificationKind::completed &&
            notification.successful;
      });
  if (!result.notifications.empty() &&
      !enqueueFederationRestoreNotifications(
          restoreRequest.federationName,
          std::move(result.notifications))) {
    return internalError(request);
  }
  // Restore completion is the callback-order boundary.  The registry returns
  // value-only ownership-assumption work for the current process sessions;
  // enqueue it only after the lifecycle notifications so the existing
  // receive fence delivers Federation Restored before the assumption offer.
  // Other restore work-item families remain separate slices and are not
  // silently projected through this path.
  if (!result.attributeOwnershipAssumptionWorkItems.empty() &&
      !enqueueAttributeOwnershipAssumptionRecipients(
          restoreRequest.federationName,
          std::move(result.attributeOwnershipAssumptionWorkItems),
          !restoreRequest.callbacksEnabled)) {
    return internalError(request);
  }
  if (successfulRestoreCompleted && !options_.pushReceiveOrderEvents) {
    // The service's live polling queue is outside the registry snapshot. A
    // successful process-local restore rolls that queue back to the saved
    // image, so discard its pre-restore copy before re-enqueuing the captured
    // receive-order attribute updates below. Otherwise each saved event would
    // be delivered once from the retained live queue and once from restore.
    std::scoped_lock lock(mutex_);
    for (auto& [sessionPointer, state] : sessions_) {
      static_cast<void>(sessionPointer);
      if (state.federationName &&
          *state.federationName == restoreRequest.federationName) {
        state.attributeUpdateEvents.clear();
      }
    }
  }
  if (!result.restoredQueuedReceiveOrderInteractions.empty()) {
    std::scoped_lock lock(mutex_);
    for (auto const& saved : result.restoredQueuedReceiveOrderInteractions) {
      auto const receivingSession = sessionsByFederateId_.find(
          saved.receivingFederateId);
      if (receivingSession == sessionsByFederateId_.end()) {
        return internalError(request);
      }
      auto const state = sessions_.find(receivingSession->second);
      if (state == sessions_.end() || !state->second.federationName ||
          *state->second.federationName != restoreRequest.federationName) {
        return internalError(request);
      }
      ProcessFederationInteractionEvent event;
      event.producingFederateId = saved.producingFederateId;
      event.receivingFederateId = saved.receivingFederateId;
      event.interactionClassHandle = saved.interactionClassHandle;
      event.parameterHandles = saved.parameterHandles;
      event.payload.assign(saved.payload.begin(), saved.payload.end());
      event.transportationName = saved.transportationName;
      state->second.interactionEvents.push_back(std::move(event));
    }
  }
  if (!result.restoredQueuedReceiveOrderAttributeUpdates.empty()) {
    std::scoped_lock lock(mutex_);
    for (auto const& saved : result.restoredQueuedReceiveOrderAttributeUpdates) {
      auto const receivingSession = sessionsByFederateId_.find(
          saved.receivingFederateId);
      if (receivingSession == sessionsByFederateId_.end()) {
        return internalError(request);
      }
      auto const state = sessions_.find(receivingSession->second);
      if (state == sessions_.end() || !state->second.federationName ||
          *state->second.federationName != restoreRequest.federationName) {
        return internalError(request);
      }
      ProcessFederationAttributeUpdateEvent event;
      event.producingFederateId = saved.producingFederateId;
      event.receivingFederateId = saved.receivingFederateId;
      event.objectInstanceHandle = saved.objectInstanceHandle;
      event.userSuppliedTag.assign(
          saved.userSuppliedTag.begin(), saved.userSuppliedTag.end());
      event.transportationName = saved.transportationName;
      event.recheckAtReceive = saved.recheckAtReceive;
      event.pushDeliveryId = saved.pushDeliveryId;
      event.attributeValues.reserve(saved.values.size());
      for (auto const& value : saved.values) {
        event.attributeValues.emplace_back(
            value.attributeHandle,
            std::vector<std::uint8_t>(value.value.begin(), value.value.end()));
      }
      if (options_.pushReceiveOrderEvents) {
        if (event.pushDeliveryId == 0U &&
            nextPushedAttributeDeliveryId_ == 0U) {
          return internalError(request);
        }
        if (event.pushDeliveryId == 0U) {
          event.pushDeliveryId = nextPushedAttributeDeliveryId_++;
        } else if (event.pushDeliveryId >= nextPushedAttributeDeliveryId_) {
          if (event.pushDeliveryId == std::numeric_limits<std::uint64_t>::max()) {
            return internalError(request);
          }
          nextPushedAttributeDeliveryId_ = event.pushDeliveryId + 1U;
        }
        auto const inserted = state->second.pendingPushedAttributeUpdates
            .emplace(event.pushDeliveryId, event).second;
        if (inserted && !receivingSession->second->send(TransportServiceMessage{
                TransportServiceMessageKind::event,
                TransportServiceOperation::receive_attribute_update,
                TransportServiceStatus::ok,
                0U,
                encodeProcessFederationReceiveAttributeUpdateResult(
                    ProcessFederationReceiveAttributeUpdateResult{event})})) {
          return internalError(request);
        }
      } else {
        state->second.attributeUpdateEvents.push_back(std::move(event));
      }
    }
  }
  return responseFor(
      request,
      TransportServiceStatus::ok,
      encodeProcessFederationRestoreControlResult(
          ProcessFederationRestoreControlResult{result.status}));
}

TransportServiceMessage
ProcessFederationService::handleFederateRestoreNotComplete(
    ProcessTransportSession& session,
    TransportServiceMessage const& request) {
  auto const restoreRequest =
      decodeProcessFederationRestoreRequest(request.payload);
  if (!restoreRequest.label.empty()) {
    return rejected(request);
  }
  {
    std::scoped_lock lock(mutex_);
    auto const state = sessions_.find(&session);
    if (state == sessions_.end() ||
        !state->second.federationName.has_value() ||
        *state->second.federationName != restoreRequest.federationName ||
        state->second.federateId != restoreRequest.federateId ||
        !registry_.memberById(
            restoreRequest.federationName, restoreRequest.federateId)) {
      return rejected(request);
    }
  }

  auto result = registry_.federateRestoreNotComplete(
      restoreRequest.federationName,
      restoreRequest.federateId);
  if (!result.notifications.empty() &&
      !enqueueFederationRestoreNotifications(
          restoreRequest.federationName,
          std::move(result.notifications))) {
    return internalError(request);
  }
  return responseFor(
      request,
      TransportServiceStatus::ok,
      encodeProcessFederationRestoreControlResult(
          ProcessFederationRestoreControlResult{result.status}));
}

TransportServiceMessage
ProcessFederationService::handleAbortFederationRestore(
    ProcessTransportSession& session,
    TransportServiceMessage const& request) {
  auto const restoreRequest =
      decodeProcessFederationRestoreRequest(request.payload);
  if (!restoreRequest.label.empty()) {
    return rejected(request);
  }
  {
    std::scoped_lock lock(mutex_);
    auto const state = sessions_.find(&session);
    if (state == sessions_.end() ||
        !state->second.federationName.has_value() ||
        *state->second.federationName != restoreRequest.federationName ||
        state->second.federateId != restoreRequest.federateId ||
        !registry_.memberById(
            restoreRequest.federationName, restoreRequest.federateId)) {
      return rejected(request);
    }
  }

  auto result = registry_.abortFederationRestore(
      restoreRequest.federationName,
      restoreRequest.federateId);
  if (!result.notifications.empty() &&
      !enqueueFederationRestoreNotifications(
          restoreRequest.federationName,
          std::move(result.notifications))) {
    return internalError(request);
  }
  return responseFor(
      request,
      TransportServiceStatus::ok,
      encodeProcessFederationRestoreControlResult(
          ProcessFederationRestoreControlResult{result.status}));
}

TransportServiceMessage
ProcessFederationService::handleQueryFederationRestoreStatus(
    ProcessTransportSession& session,
    TransportServiceMessage const& request) {
  auto const restoreRequest =
      decodeProcessFederationRestoreRequest(request.payload);
  if (!restoreRequest.label.empty()) {
    return rejected(request);
  }
  {
    std::scoped_lock lock(mutex_);
    auto const state = sessions_.find(&session);
    if (state == sessions_.end() ||
        !state->second.federationName.has_value() ||
        *state->second.federationName != restoreRequest.federationName ||
        state->second.federateId != restoreRequest.federateId ||
        !registry_.memberById(
            restoreRequest.federationName, restoreRequest.federateId)) {
      return rejected(request);
    }
  }

  auto result = registry_.queryFederationRestoreStatus(
      restoreRequest.federationName,
      restoreRequest.federateId);
  if (!result.notifications.empty() &&
      !enqueueFederationRestoreNotifications(
          restoreRequest.federationName,
          std::move(result.notifications))) {
    return internalError(request);
  }
  return responseFor(
      request,
      TransportServiceStatus::ok,
      encodeProcessFederationRestoreControlResult(
          ProcessFederationRestoreControlResult{result.status}));
}

}  // namespace umbra::detail
