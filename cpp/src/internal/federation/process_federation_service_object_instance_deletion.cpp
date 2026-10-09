#include "internal/federation/process_federation_service.hpp"
#include "internal/federation/process_federation_service_helpers.hpp"

#include <utility>

namespace umbra::detail {

TransportServiceMessage ProcessFederationService::handleLocalDeleteObjectInstance(
    ProcessTransportSession &session,
    TransportServiceMessage const &request) {
  auto const deletionRequest =
      decodeProcessFederationLocalDeleteObjectInstanceRequest(request.payload);
  {
    std::scoped_lock lock(mutex_);
    auto const state = sessions_.find(&session);
    if (state == sessions_.end() ||
        !state->second.federationName.has_value() ||
        *state->second.federationName != deletionRequest.federationName ||
        state->second.federateId != deletionRequest.federateId ||
        !registry_.memberById(
            deletionRequest.federationName, deletionRequest.federateId)) {
      return rejected(request);
    }
  }

  auto const status = registry_.localDeleteObjectInstance(
      deletionRequest.federationName,
      deletionRequest.federateId,
      deletionRequest.objectInstanceHandle);
  return responseFor(
      request,
      TransportServiceStatus::ok,
      encodeProcessFederationLocalDeleteObjectInstanceResult(
          ProcessFederationLocalDeleteObjectInstanceResult{status}));
}

TransportServiceMessage ProcessFederationService::handleDeleteObjectInstance(
    ProcessTransportSession &session,
    TransportServiceMessage const &request) {
  auto const deletionRequest =
      decodeProcessFederationDeleteObjectInstanceRequest(request.payload);
  std::shared_ptr<FederateTimeState> producingTimeState;
  {
    std::scoped_lock lock(mutex_);
    auto const state = sessions_.find(&session);
    if (state == sessions_.end() ||
        !state->second.federationName.has_value() ||
        *state->second.federationName != deletionRequest.federationName ||
        state->second.federateId != deletionRequest.federateId ||
        !registry_.memberById(
            deletionRequest.federationName, deletionRequest.federateId)) {
      return rejected(request);
    }
    producingTimeState = state->second.timeState;
  }

  if (deletionRequest.timestamp) {
    // The process endpoint carries the official logical-time value across the
    // boundary and retains the registry's timestamped deletion ledger so the
    // callback carries the same immutable recipient snapshot and message
    // identity as the embedded path.  The public retraction designator is
    // projected only when the producer is time-regulating and the effective
    // HLAprivilegeToDeleteObject order is timestamp-ordered.
    auto const timestamp = decodeProcessLogicalTime(
        *deletionRequest.timestamp,
        federationDefinition_->logicalTimeImplementationName);
    auto const plan = registry_.planTsoObjectInstanceDeletion(
        deletionRequest.federationName,
        deletionRequest.federateId,
        deletionRequest.objectInstanceHandle);
    if (plan.status != ObjectInstanceDeletionStatus::applied) {
      return responseFor(
          request,
          TransportServiceStatus::ok,
          encodeProcessFederationDeleteObjectInstanceResult(
              ProcessFederationDeleteObjectInstanceResult{plan.status, 0U, 0U}));
    }
    auto const producingTimeSnapshot = producingTimeState
        ? producingTimeState->snapshot()
        : FederateTimeSnapshot{};
    bool const provideRetraction =
        producingTimeSnapshot.timeRegulating &&
        plan.preferredOrderType == rti1516_2025::TIMESTAMP;
    bool const queueTimestampedDeletion = provideRetraction;
    std::vector<std::uint64_t> queuedRecipientFederateIds;
    if (queueTimestampedDeletion && !plan.recipients.empty()) {
      auto const execution = registry_.timeSnapshotFor(
          deletionRequest.federationName);
      if (!execution) {
        return internalError(request);
      }
      std::set<std::uint64_t> timeConstrainedRecipients;
      for (auto const &federate : execution->federates) {
        if (federate.time.timeConstrained) {
          timeConstrainedRecipients.insert(federate.membership.id);
        }
      }
      queuedRecipientFederateIds.reserve(plan.recipients.size());
      for (auto const &recipient : plan.recipients) {
        if (timeConstrainedRecipients.contains(recipient.receivingFederateId)) {
          queuedRecipientFederateIds.push_back(recipient.receivingFederateId);
        }
      }
    }

    TsoObjectDeletionMessage message;
    message.producingFederateId = deletionRequest.federateId;
    message.objectInstanceHandle = deletionRequest.objectInstanceHandle;
    if (!deletionRequest.userSuppliedTag.empty()) {
      message.userSuppliedTag.setData(
          deletionRequest.userSuppliedTag.data(),
          deletionRequest.userSuppliedTag.size());
    }
    message.timestamp = timestamp;
    message.sentOrderType = plan.preferredOrderType;
    message.recipients.reserve(plan.recipients.size());
    for (auto const &recipient : plan.recipients) {
      message.recipients.push_back({
          recipient.receivingFederateId,
          recipient.objectInstanceHandle,
          recipient.callbackRoute,
          {}});
    }

    auto const enqueueResult = registry_.enqueueTsoObjectDeletion(
        deletionRequest.federationName,
        deletionRequest.federateId,
        deletionRequest.objectInstanceHandle,
        std::move(message),
        queuedRecipientFederateIds);
    if (enqueueResult.status != FederationTsoRegistryStatus::applied ||
        enqueueResult.queueStatus != TsoMessageQueueStatus::applied ||
        enqueueResult.messageId == 0U) {
      return rejected(request);
    }
    if (enqueueResult.recipients.size() >
        std::numeric_limits<std::uint32_t>::max()) {
      return internalError(request);
    }

    std::vector<ObjectInstanceRemovalRecipient> removals;
    removals.reserve(enqueueResult.recipients.size());
    std::set<std::uint64_t> queuedRecipients(
        queuedRecipientFederateIds.begin(), queuedRecipientFederateIds.end());
    for (auto const &recipient : enqueueResult.recipients) {
      if (queuedRecipients.contains(recipient.receivingFederateId)) {
        continue;
      }
      removals.push_back({
          recipient.receivingFederateId,
          recipient.objectInstanceHandle,
          recipient.callbackRoute,
          recipient.serviceReportRoute,
          false,
          plan.preferredOrderType,
          rti1516_2025::RECEIVE});
    }
    if (!enqueueObjectInstanceRemovals(
            deletionRequest.federationName,
            std::move(removals),
            deletionRequest.userSuppliedTag,
            deletionRequest.timestamp,
            enqueueResult.messageId,
            provideRetraction)) {
      return internalError(request);
    }
    {
      std::scoped_lock lock(mutex_);
      processTsoMessageProducers_.emplace(
          enqueueResult.messageId,
          deletionRequest.federateId);
    }
    return responseFor(
        request,
        TransportServiceStatus::ok,
        encodeProcessFederationDeleteObjectInstanceResult(
            ProcessFederationDeleteObjectInstanceResult{
                ObjectInstanceDeletionStatus::applied,
                static_cast<std::uint32_t>(enqueueResult.recipients.size()),
                provideRetraction ? enqueueResult.messageId : 0U}));
  }

  auto deletion = registry_.deleteObjectInstance(
      deletionRequest.federationName,
      deletionRequest.federateId,
      deletionRequest.objectInstanceHandle);
  if (deletion.status != ObjectInstanceDeletionStatus::applied) {
    return responseFor(
        request,
        TransportServiceStatus::ok,
        encodeProcessFederationDeleteObjectInstanceResult(
            ProcessFederationDeleteObjectInstanceResult{deletion.status, 0U}));
  }
  auto const recipientCount = deletion.recipients.size();
  if (recipientCount > std::numeric_limits<std::uint32_t>::max()) {
    return internalError(request);
  }
  if (!enqueueObjectInstanceRemovals(
          deletionRequest.federationName,
          std::move(deletion.recipients),
          deletionRequest.userSuppliedTag)) {
    return internalError(request);
  }
  return responseFor(
      request,
      TransportServiceStatus::ok,
      encodeProcessFederationDeleteObjectInstanceResult(
          ProcessFederationDeleteObjectInstanceResult{
              deletion.status,
              static_cast<std::uint32_t>(recipientCount)}));
}

}  // namespace umbra::detail
