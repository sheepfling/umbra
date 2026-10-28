#include "internal/federation/process_federation_client.hpp"
#include "internal/federation/process_federation_client_event_templates.hpp"

#include <RTI/Exception.h>

#include <algorithm>
#include <limits>
#include <type_traits>
#include <utility>

namespace umbra::detail {

ProcessFederationAsynchronousDeliveryStatus
ProcessFederationClient::setAsynchronousDeliveryEnabled(bool enabled) {
  return requireCallbackBridge()->setAsynchronousDeliveryEnabled(enabled);
}

void ProcessFederationClient::setTimeConstrainedEnabled(bool enabled) {
  requireCallbackBridge()->setTimeConstrainedEnabled(enabled);
}

void ProcessFederationClient::setTimeAdvancePending(bool pending) {
  requireCallbackBridge()->setTimeAdvancePending(pending);
}

void ProcessFederationClient::dispatchPushedReceiveOrder() {
  dispatchPendingPushedEvents();
}

void ProcessFederationClient::dispatchReceiveOrder(
    ProcessFederationInteractionEvent event) {
  auto bridge = requireCallbackBridge();
  bridge->submitReceiveOrder(std::move(event));
}

void ProcessFederationClient::dispatchAttributeUpdate(
    ProcessFederationAttributeUpdateEvent event) {
  auto bridge = requireCallbackBridge();
  bridge->submitAttributeUpdate(std::move(event));
}

void ProcessFederationClient::dispatchPushedAttributeUpdate() {
  dispatchPendingPushedEvents();
}

void ProcessFederationClient::dispatchAttributeValueUpdateRequest(
    ProcessFederationAttributeValueUpdateRequestEvent event) {
  auto bridge = requireCallbackBridge();
  bridge->submitAttributeValueUpdateRequest(std::move(event));
}

void ProcessFederationClient::dispatchPushedAttributeValueUpdateRequest() {
  dispatchPendingPushedEvents();
}

void ProcessFederationClient::dispatchAttributeOwnershipQuery(
    ProcessFederationAttributeOwnershipQueryEvent event) {
  auto bridge = requireCallbackBridge();
  bridge->submitAttributeOwnershipQuery(std::move(event));
}

void ProcessFederationClient::dispatchPushedAttributeOwnershipQuery() {
  dispatchPendingPushedEvents();
}

void ProcessFederationClient::dispatchAttributeOwnershipAcquisitionIfAvailable(
    ProcessFederationAttributeOwnershipAcquisitionIfAvailableEvent event) {
  auto bridge = requireCallbackBridge();
  bridge->submitAttributeOwnershipAcquisitionIfAvailable(
      std::move(event));
}

void ProcessFederationClient::dispatchPushedAttributeOwnershipAcquisitionIfAvailable() {
  dispatchPendingPushedEvents();
}

void ProcessFederationClient::dispatchAttributeOwnershipAcquisition(
    ProcessFederationAttributeOwnershipAcquisitionEvent event) {
  auto bridge = requireCallbackBridge();
  bridge->submitAttributeOwnershipAcquisition(std::move(event));
}

void ProcessFederationClient::dispatchPushedAttributeOwnershipAcquisition() {
  dispatchPendingPushedEvents();
}

void ProcessFederationClient::dispatchAttributeOwnershipUnavailable(
    ProcessFederationAttributeOwnershipUnavailableEvent event) {
  auto bridge = requireCallbackBridge();
  bridge->submitAttributeOwnershipUnavailable(std::move(event));
}

void ProcessFederationClient::dispatchPushedAttributeOwnershipUnavailable() {
  dispatchPendingPushedEvents();
}

void ProcessFederationClient::dispatchObjectInstanceDiscovery(
    ProcessFederationObjectInstanceDiscoveryEvent event) {
  auto bridge = requireCallbackBridge();
  bridge->submitObjectInstanceDiscovery(std::move(event));
}

void ProcessFederationClient::dispatchPushedObjectInstanceDiscovery() {
  dispatchPendingPushedEvents();
}

void ProcessFederationClient::dispatchObjectInstanceRemoval(
    ProcessFederationObjectInstanceRemovalEvent event) {
  auto bridge = requireCallbackBridge();
  bridge->submitObjectInstanceRemoval(std::move(event));
}

void ProcessFederationClient::dispatchPushedObjectInstanceRemoval() {
  dispatchPendingPushedEvents();
}

void ProcessFederationClient::dispatchObjectInstanceScopeChange(
    ProcessFederationObjectInstanceScopeChangeEvent event) {
  auto bridge = requireCallbackBridge();
  bridge->submitObjectInstanceScopeChange(std::move(event));
}

void ProcessFederationClient::dispatchPushedObjectInstanceScopeChange() {
  dispatchPendingPushedEvents();
}

void ProcessFederationClient::dispatchAttributeRelevanceAdvisory(
    ProcessFederationAttributeRelevanceAdvisoryEvent event) {
  auto bridge = requireCallbackBridge();
  bridge->submitAttributeRelevanceAdvisory(std::move(event));
}

void ProcessFederationClient::dispatchPushedAttributeRelevanceAdvisory() {
  dispatchPendingPushedEvents();
}

void ProcessFederationClient::dispatchAttributeTransportationTypeChange(
    ProcessFederationAttributeTransportationTypeChangeEvent event) {
  auto bridge = requireCallbackBridge();
  bridge->submitAttributeTransportationTypeChange(std::move(event));
}

void ProcessFederationClient::dispatchPushedAttributeTransportationTypeChange() {
  dispatchPendingPushedEvents();
}

void ProcessFederationClient::dispatchAttributeTransportationTypeQuery(
    ProcessFederationAttributeTransportationTypeQueryEvent event) {
  auto bridge = requireCallbackBridge();
  bridge->submitAttributeTransportationTypeQuery(std::move(event));
}

void ProcessFederationClient::dispatchPushedAttributeTransportationTypeQuery() {
  dispatchPendingPushedEvents();
}

void ProcessFederationClient::dispatchInteractionTransportationTypeChange(
    ProcessFederationInteractionTransportationTypeChangeEvent event) {
  auto bridge = requireCallbackBridge();
  bridge->submitInteractionTransportationTypeChange(std::move(event));
}

void ProcessFederationClient::dispatchPushedInteractionTransportationTypeChange() {
  dispatchPendingPushedEvents();
}

void ProcessFederationClient::dispatchInteractionTransportationTypeQuery(
    ProcessFederationInteractionTransportationTypeQueryEvent event) {
  auto bridge = requireCallbackBridge();
  bridge->submitInteractionTransportationTypeQuery(std::move(event));
}

void ProcessFederationClient::dispatchPushedInteractionTransportationTypeQuery() {
  dispatchPendingPushedEvents();
}

void ProcessFederationClient::dispatchSynchronizationPointRegistrationSucceeded(
    std::wstring label) {
  auto bridge = requireCallbackBridge();
  bridge->submitSynchronizationPointRegistrationSucceeded(
      std::move(label));
}

void ProcessFederationClient::dispatchSynchronizationPointRegistrationFailed(
    std::wstring label,
    rti1516_2025::SynchronizationPointFailureReason failureReason) {
  auto bridge = requireCallbackBridge();
  bridge->submitSynchronizationPointRegistrationFailed(
      std::move(label), failureReason);
}

void ProcessFederationClient::dispatchSynchronizationPointAnnouncement(
    ProcessFederationSynchronizationPointAnnouncementEvent event) {
  auto bridge = requireCallbackBridge();
  bridge->submitSynchronizationPointAnnouncement(std::move(event));
}

void ProcessFederationClient::dispatchPushedSynchronizationPointAnnouncement() {
  dispatchPendingPushedEvents();
}

void ProcessFederationClient::dispatchFederationSynchronized(
    ProcessFederationFederationSynchronizedEvent event) {
  auto bridge = requireCallbackBridge();
  bridge->submitFederationSynchronized(std::move(event));
}

void ProcessFederationClient::dispatchPushedFederationSynchronized() {
  dispatchPendingPushedEvents();
}

void ProcessFederationClient::dispatchFederationSave(
    ProcessFederationSaveEvent event) {
  auto bridge = requireCallbackBridge();
  bridge->submitFederationSave(std::move(event));
}

void ProcessFederationClient::dispatchPushedFederationSave() {
  dispatchPendingPushedEvents();
}

void ProcessFederationClient::dispatchFederationRestore(
    ProcessFederationRestoreEvent event) {
  auto bridge = requireCallbackBridge();
  bridge->submitFederationRestore(std::move(event));
}

void ProcessFederationClient::dispatchPushedFederationRestore() {
  dispatchPendingPushedEvents();
}

void ProcessFederationClient::dispatchTimeRegulationEnabled(
    ProcessFederationLogicalTime event) {
  auto bridge = requireCallbackBridge();
  bridge->submitTimeRegulationEnabled(std::move(event));
}

void ProcessFederationClient::dispatchTimeConstrainedEnabled(
    ProcessFederationLogicalTime event) {
  auto bridge = requireCallbackBridge();
  bridge->submitTimeConstrainedEnabled(std::move(event));
}

void ProcessFederationClient::dispatchTimeAdvanceGrant(
    ProcessFederationLogicalTime event) {
  auto bridge = requireCallbackBridge();
  bridge->submitTimeAdvanceGrant(std::move(event));
}

void ProcessFederationClient::dispatchPushedTimeAdvanceGrant() {
  dispatchPendingPushedEvents();
}

void ProcessFederationClient::dispatchFlushQueueGrant(
    ProcessFederationLogicalTime grantedTime,
    ProcessFederationLogicalTime optimisticTime) {
  auto bridge = requireCallbackBridge();
  bridge->submitFlushQueueGrant(
      std::move(grantedTime), std::move(optimisticTime));
}

void ProcessFederationClient::dispatchPushedFlushQueueGrant() {
  dispatchPendingPushedEvents();
}

void ProcessFederationClient::setTimeRoleEnableCompletionHandlers(
    CallbackCompletionHandler regulation,
    CallbackCompletionHandler constrained) {
  auto bridge = requireCallbackBridge();
  bridge->setTimeRoleEnableCompletionHandlers(
      std::move(regulation), std::move(constrained));
}

void ProcessFederationClient::dispatchPendingPushedEvents() {
  dispatchPendingPushedEvents(false);
}

void ProcessFederationClient::dispatchPendingPushedEvents(
    bool allowRequestOwnedSuppression) {
  std::unique_lock lock(transactionMutex_);
  if (!callbackBridge_ ||
      (dispatchDepth_ != 0U && dispatchOwner_ != std::this_thread::get_id())) {
    return;
  }
  dispatchOwner_ = std::this_thread::get_id();
  ++dispatchDepth_;
  auto finish = [this] {
    if (--dispatchDepth_ == 0U) {
      dispatchOwner_ = {};
      callbackEventsAvailable_.notify_all();
    }
  };
  try {
    // Resign suppresses callback admission while waiting for any callback
    // already in progress to finish. Recheck suppression between callbacks;
    // otherwise this dispatcher could drain the whole queue and prevent the
    // resigning service from reaching the server-side membership transition.
    while (callbackBridge_ && !pendingEvents_.empty() &&
           (allowRequestOwnedSuppression ||
            callbackDispatchSuppressionDepth_ == 0U)) {
      auto bridge = callbackBridge_;
      auto event = std::move(pendingEvents_.front());
      pendingEvents_.pop_front();
      lock.unlock();
      std::visit([&bridge](auto value) {
        using Event = std::decay_t<decltype(value)>;
        if constexpr (std::is_same_v<Event, ProcessFederationInteractionEvent>) {
          bridge->submitReceiveOrder(std::move(value));
        }
        else if constexpr (std::is_same_v<Event, ProcessFederationAttributeUpdateEvent>) {
          bridge->submitAttributeUpdate(std::move(value));
        }
        else if constexpr (std::is_same_v<Event, PendingDeferredAttributeUpdate>) {
          bridge->submitDeferredAttributeUpdate(
              std::move(value.federationName), value.receivingFederateId);
        }
        else if constexpr (std::is_same_v<Event, ProcessFederationAttributeValueUpdateRequestEvent>) {
          bridge->submitAttributeValueUpdateRequest(std::move(value));
        }
        else if constexpr (std::is_same_v<Event, ProcessFederationAttributeOwnershipQueryEvent>) {
          bridge->submitAttributeOwnershipQuery(std::move(value));
        }
        else if constexpr (std::is_same_v<Event, ProcessFederationAttributeOwnershipAcquisitionIfAvailableEvent>) {
          bridge->submitAttributeOwnershipAcquisitionIfAvailable(std::move(value));
        }
        else if constexpr (std::is_same_v<Event, ProcessFederationAttributeOwnershipAcquisitionEvent>) {
          bridge->submitAttributeOwnershipAcquisition(std::move(value));
        }
        else if constexpr (std::is_same_v<Event, ProcessFederationAttributeOwnershipUnavailableEvent>) {
          bridge->submitAttributeOwnershipUnavailable(std::move(value));
        }
        else if constexpr (std::is_same_v<Event, ProcessFederationObjectInstanceDiscoveryEvent>) {
          bridge->submitObjectInstanceDiscovery(std::move(value));
        }
        else if constexpr (std::is_same_v<Event, ProcessFederationObjectInstanceRemovalEvent>) {
          bridge->submitObjectInstanceRemoval(std::move(value));
        }
        else if constexpr (std::is_same_v<Event, ProcessFederationObjectInstanceScopeChangeEvent>) {
          bridge->submitObjectInstanceScopeChange(std::move(value));
        }
        else if constexpr (std::is_same_v<Event, ProcessFederationAttributeRelevanceAdvisoryEvent>) {
          bridge->submitAttributeRelevanceAdvisory(std::move(value));
        }
        else if constexpr (std::is_same_v<Event, ProcessFederationAttributeTransportationTypeChangeEvent>) {
          bridge->submitAttributeTransportationTypeChange(std::move(value));
        }
        else if constexpr (std::is_same_v<Event, ProcessFederationAttributeTransportationTypeQueryEvent>) {
          bridge->submitAttributeTransportationTypeQuery(std::move(value));
        }
        else if constexpr (std::is_same_v<Event, ProcessFederationInteractionTransportationTypeChangeEvent>) {
          bridge->submitInteractionTransportationTypeChange(std::move(value));
        }
        else if constexpr (std::is_same_v<Event, ProcessFederationInteractionTransportationTypeQueryEvent>) {
          bridge->submitInteractionTransportationTypeQuery(std::move(value));
        }
        else if constexpr (std::is_same_v<Event, ProcessFederationSynchronizationPointAnnouncementEvent>) {
          bridge->submitSynchronizationPointAnnouncement(std::move(value));
        }
        else if constexpr (std::is_same_v<Event, ProcessFederationFederationSynchronizedEvent>) {
          bridge->submitFederationSynchronized(std::move(value));
        }
        else if constexpr (std::is_same_v<Event, ProcessFederationSaveEvent>) {
          bridge->submitFederationSave(std::move(value));
        }
        else if constexpr (std::is_same_v<Event, ProcessFederationRestoreEvent>) {
          bridge->submitFederationRestore(std::move(value));
        }
        else if constexpr (std::is_same_v<Event, ProcessFederationLogicalTime>) {
          bridge->submitTimeAdvanceGrant(std::move(value));
        }
        else if constexpr (std::is_same_v<Event, PendingFlushQueueGrant>) {
          bridge->submitFlushQueueGrant(
              std::move(value.grantedTime), std::move(value.optimisticTime));
        }
        else if constexpr (std::is_same_v<Event, PendingTimeRegulationEnabled>) {
          bridge->submitTimeRegulationEnabled(std::move(value.enabledTime));
        }
        else if constexpr (std::is_same_v<Event, PendingTimeConstrainedEnabled>) {
          bridge->submitTimeConstrainedEnabled(std::move(value.enabledTime));
        }
        else if constexpr (std::is_same_v<Event, PendingRequestRetraction>) {
          bridge->submitRequestRetraction(value.messageId);
        }
      }, std::move(event));
      lock.lock();
    }
    finish();
  } catch (...) {
    if (!lock.owns_lock()) {
      lock.lock();
    }
    finish();
    throw;
  }
}

std::size_t ProcessFederationClient::pendingPushedEventCount() const noexcept {
  return pendingEventCount<ProcessFederationInteractionEvent>();
}

std::size_t ProcessFederationClient::pendingPushedAttributeUpdateCount() const noexcept {
  return pendingEventCount<ProcessFederationAttributeUpdateEvent>();
}

std::size_t
ProcessFederationClient::pendingPushedAttributeValueUpdateRequestCount()
    const noexcept {
  return pendingEventCount<ProcessFederationAttributeValueUpdateRequestEvent>();
}

std::size_t ProcessFederationClient::pendingPushedAttributeOwnershipQueryCount()
    const noexcept {
  return pendingEventCount<ProcessFederationAttributeOwnershipQueryEvent>();
}

std::size_t
ProcessFederationClient::pendingPushedAttributeOwnershipAcquisitionIfAvailableCount()
    const noexcept {
  return pendingEventCount<ProcessFederationAttributeOwnershipAcquisitionIfAvailableEvent>();
}

std::size_t ProcessFederationClient::pendingPushedAttributeOwnershipAcquisitionCount()
    const noexcept {
  return pendingEventCount<ProcessFederationAttributeOwnershipAcquisitionEvent>();
}

std::size_t
ProcessFederationClient::pendingPushedAttributeOwnershipUnavailableCount()
    const noexcept {
  return pendingEventCount<ProcessFederationAttributeOwnershipUnavailableEvent>();
}

std::size_t ProcessFederationClient::pendingPushedObjectInstanceDiscoveryCount()
    const noexcept {
  return pendingEventCount<ProcessFederationObjectInstanceDiscoveryEvent>();
}

std::size_t ProcessFederationClient::pendingPushedObjectInstanceRemovalCount()
    const noexcept {
  return pendingEventCount<ProcessFederationObjectInstanceRemovalEvent>();
}

std::size_t ProcessFederationClient::pendingPushedObjectInstanceScopeChangeCount()
    const noexcept {
  return pendingEventCount<ProcessFederationObjectInstanceScopeChangeEvent>();
}

std::size_t ProcessFederationClient::pendingPushedAttributeRelevanceAdvisoryCount()
    const noexcept {
  return pendingEventCount<ProcessFederationAttributeRelevanceAdvisoryEvent>();
}

std::size_t
ProcessFederationClient::pendingPushedAttributeTransportationTypeChangeCount()
    const noexcept {
  return pendingEventCount<ProcessFederationAttributeTransportationTypeChangeEvent>();
}

std::size_t
ProcessFederationClient::pendingPushedAttributeTransportationTypeQueryCount()
    const noexcept {
  return pendingEventCount<ProcessFederationAttributeTransportationTypeQueryEvent>();
}

std::size_t
ProcessFederationClient::pendingPushedInteractionTransportationTypeChangeCount()
    const noexcept {
  return pendingEventCount<ProcessFederationInteractionTransportationTypeChangeEvent>();
}

std::size_t
ProcessFederationClient::pendingPushedInteractionTransportationTypeQueryCount()
    const noexcept {
  return pendingEventCount<ProcessFederationInteractionTransportationTypeQueryEvent>();
}

std::size_t
ProcessFederationClient::pendingPushedSynchronizationPointAnnouncementCount()
    const noexcept {
  return pendingEventCount<ProcessFederationSynchronizationPointAnnouncementEvent>();
}

std::size_t ProcessFederationClient::pendingPushedFederationSynchronizedCount()
    const noexcept {
  return pendingEventCount<ProcessFederationFederationSynchronizedEvent>();
}

std::size_t ProcessFederationClient::pendingPushedFederationSaveCount()
    const noexcept {
  return pendingEventCount<ProcessFederationSaveEvent>();
}

std::size_t ProcessFederationClient::pendingPushedFederationRestoreCount()
    const noexcept {
  return pendingEventCount<ProcessFederationRestoreEvent>();
}

std::size_t ProcessFederationClient::pendingPushedTimeAdvanceGrantCount()
    const noexcept {
  return pendingEventCount<ProcessFederationLogicalTime>();
}

std::size_t ProcessFederationClient::pendingPushedFlushQueueGrantCount()
    const noexcept {
  return pendingEventCount<PendingFlushQueueGrant>();
}

void ProcessFederationClient::attachCallbackBridge(
    rti1516_2025::FederateAmbassador& recipient,
    CallbackDispatchModel model) {
  std::scoped_lock lock(transactionMutex_);
  if (callbackBridge_) {
    throw ProcessFederationClientError(
        "A process federation callback bridge is already attached.");
  }
  callbackBridge_ = std::make_shared<ProcessFederationCallbackBridge>(
      recipient, model);
  callbackBridge_->setAttributeRelevanceAdvisorySwitchState(
      attributeRelevanceAdvisorySwitchState_);
  callbackBridge_->setTsoDeliveryCompletionHandler(
      [this](std::uint64_t messageId) { acknowledgeTsoDelivery(messageId); });
  callbackBridge_->setPushAttributeDeliveryCompletionHandler(
      [this](std::uint64_t deliveryId) {
        acknowledgePushedAttributeUpdate(deliveryId);
      });
  callbackBridge_->setExceptionReportProjectionHandler(
      [this](ProcessFederationInteractionEvent event) {
        return recheckExceptionReport(std::move(event));
      });
  callbackBridge_->setDeferredAttributeUpdateHandler(
      [this](std::wstring const& federationName, std::uint64_t receivingFederateId) {
        return claimDeferredAttributeUpdate(federationName, receivingFederateId);
      });
  callbackEventsAvailable_.notify_all();
}

void ProcessFederationClient::attachCallbackBridge(
    std::shared_ptr<CallbackDispatcher> dispatcher,
    std::shared_ptr<rti1516_2025::umbra_binding_detail::CallbackSession>
        callbackSession) {
  std::scoped_lock lock(transactionMutex_);
  if (callbackBridge_) {
    throw ProcessFederationClientError(
        "A process federation callback bridge is already attached.");
  }
  callbackBridge_ = std::make_shared<ProcessFederationCallbackBridge>(
      std::move(dispatcher), std::move(callbackSession));
  callbackBridge_->setAttributeRelevanceAdvisorySwitchState(
      attributeRelevanceAdvisorySwitchState_);
  callbackBridge_->setTsoDeliveryCompletionHandler(
      [this](std::uint64_t messageId) { acknowledgeTsoDelivery(messageId); });
  callbackBridge_->setPushAttributeDeliveryCompletionHandler(
      [this](std::uint64_t deliveryId) {
        acknowledgePushedAttributeUpdate(deliveryId);
      });
  callbackBridge_->setExceptionReportProjectionHandler(
      [this](ProcessFederationInteractionEvent event) {
        return recheckExceptionReport(std::move(event));
      });
  callbackBridge_->setDeferredAttributeUpdateHandler(
      [this](std::wstring const& federationName, std::uint64_t receivingFederateId) {
        return claimDeferredAttributeUpdate(federationName, receivingFederateId);
      });
  callbackEventsAvailable_.notify_all();
}

bool ProcessFederationClient::evokeOne(
    std::chrono::milliseconds minimumWait) {
  auto bridge = requireCallbackBridge();
  dispatchPendingPushedEvents();
  flushDeferredTsoDeliveryAcknowledgements();
  return bridge->evokeOne(minimumWait);
}

bool ProcessFederationClient::evokeMultiple(
    std::chrono::milliseconds minimumWait,
    std::chrono::milliseconds maximumWait) {
  auto bridge = requireCallbackBridge();
  dispatchPendingPushedEvents();
  flushDeferredTsoDeliveryAcknowledgements();
  return bridge->evokeMultiple(minimumWait, maximumWait);
}

std::size_t ProcessFederationClient::pendingCallbackCount() const {
  std::shared_ptr<ProcessFederationCallbackBridge> bridge;
  {
    std::scoped_lock lock(transactionMutex_);
    bridge = callbackBridge_;
  }
  return bridge ? bridge->pendingCount() : 0U;
}

std::shared_ptr<ProcessTransportConnection>
ProcessFederationClient::connection() const noexcept {
  std::scoped_lock lock(transactionMutex_);
  return connection_;
}

void ProcessFederationClient::close() noexcept {
  closing_.store(true, std::memory_order_release);
  // This immutable shared owner is intentionally closed before taking the
  // transaction lock: a request may be holding that lock while blocked in a
  // socket receive, and closing the socket is what releases that request.
  auto const shutdownConnection = shutdownConnection_;
  if (shutdownConnection) {
    shutdownConnection->close();
  }
  // Closing is published before socket shutdown so a blocked send/receive can
  // unwind. Synchronize with each wait mutex *after* shutdown: otherwise a
  // waiter may see closing_ == false, then miss the notification before its
  // condition-variable wait atomically releases the mutex and blocks.
  {
    std::scoped_lock lock(responseMutex_);
    responseAvailable_.notify_all();
  }
  {
    std::scoped_lock lock(transactionMutex_);
    callbackEventsAvailable_.notify_all();
  }

  auto const currentThread = std::this_thread::get_id();
  if (streamReaderThread_.joinable() &&
      streamReaderThread_.get_id() != currentThread) {
    streamReaderThread_.join();
  }
  if (callbackDispatchThread_.joinable() &&
      callbackDispatchThread_.get_id() != currentThread) {
    callbackDispatchThread_.join();
  }

  std::shared_ptr<ProcessFederationCallbackBridge> bridge;
  std::unique_ptr<ProcessTransportSession> session;
  {
    std::scoped_lock lock(transactionMutex_);
    bridge = std::move(callbackBridge_);
    session = std::move(session_);
    connection_.reset();
    pendingEvents_.clear();
    pendingTransportFailures_.clear();
    deferredDeliveryAcknowledgements_.clear();
    joinedFederationName_.reset();
    joinedFederateId_ = 0U;
  }
  // Bridge close may wait for callback-time projection/user invocation. Keep
  // that wait outside both the client exchange lock and the transport.
  if (bridge) {
    bridge->close();
  }
}



}  // namespace umbra::detail
