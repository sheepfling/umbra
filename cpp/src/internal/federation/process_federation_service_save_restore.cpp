#include "internal/federation/process_federation_service.hpp"
#include "internal/federation/process_federation_service_helpers.hpp"

#include <memory>
#include <mutex>
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
  auto result = timestamp
      ? registry_.requestFederationSave(
            saveRequest.federationName,
            saveRequest.federateId,
            saveRequest.label,
            std::move(timestamp))
      : registry_.requestFederationSave(
            saveRequest.federationName,
            saveRequest.federateId,
            saveRequest.label);
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
