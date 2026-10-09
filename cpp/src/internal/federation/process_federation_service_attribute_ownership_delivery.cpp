#include "internal/federation/process_federation_service.hpp"
#include "internal/federation/process_federation_service_helpers.hpp"

#include <algorithm>
#include <cstdint>
#include <memory>
#include <utility>
#include <vector>

namespace umbra::detail {

bool ProcessFederationService::enqueueAttributeOwnershipAcquisitionWorkItems(
    std::wstring const& federationName,
    std::vector<AttributeOwnershipAcquisitionWorkItem> workItems) {
  std::vector<std::pair<ProcessTransportSession*,
                        ProcessFederationAttributeOwnershipAcquisitionEvent>>
      pushedEvents;

  // Follow-up work may be exposed by the callback-boundary registry
  // transition (for example, the next queued requester after an unowned
  // notification). Keep it in the same deterministic order as the registry
  // returned it, without recursing through the service mutex.
  for (std::size_t index = 0U; index < workItems.size(); ++index) {
    auto workItem = std::move(workItems[index]);
    ProcessFederationAttributeOwnershipAcquisitionEventKind eventKind;
    std::uint64_t receivingFederateId = 0U;
    if (workItem.kind == AttributeOwnershipAcquisitionWorkKind::
                            acquisition_notification) {
      eventKind = ProcessFederationAttributeOwnershipAcquisitionEventKind::
          acquisition_notification;
      receivingFederateId = workItem.requestingFederateId;
    } else if (workItem.kind == AttributeOwnershipAcquisitionWorkKind::
                                    request_release) {
      eventKind = ProcessFederationAttributeOwnershipAcquisitionEventKind::
          request_release;
      receivingFederateId = workItem.receivingFederateId;
    } else if (workItem.kind == AttributeOwnershipAcquisitionWorkKind::
                                    request_divestiture_confirmation) {
      eventKind = ProcessFederationAttributeOwnershipAcquisitionEventKind::
          request_divestiture_confirmation;
      receivingFederateId = workItem.receivingFederateId;
    } else {
      // If Available delivery is projected through its own result slot.
      return false;
    }
    if (workItem.requestId == 0U || workItem.requestingFederateId == 0U ||
        receivingFederateId == 0U || workItem.objectInstanceHandle == 0U ||
        workItem.attributeHandles.empty()) {
      return false;
    }

    ProcessTransportSession* receivingSession = nullptr;
    {
      std::scoped_lock lock(mutex_);
      auto const session = sessionsByFederateId_.find(receivingFederateId);
      if (session != sessionsByFederateId_.end()) {
        auto const state = sessions_.find(session->second);
        if (state != sessions_.end() && state->second.federationName &&
            *state->second.federationName == federationName &&
            state->second.federateId == receivingFederateId) {
          receivingSession = session->second;
        }
      }
    }
    if (receivingSession == nullptr) {
      return false;
    }

    ProcessFederationAttributeOwnershipAcquisitionEvent event{
        eventKind,
        workItem.requestId,
        workItem.requestingFederateId,
        receivingFederateId,
        workItem.objectInstanceHandle,
        workItem.attributeHandles,
        workItem.userSuppliedTag,
        workItem.candidateIsIfAvailable};

    // A release request is a callback-boundary reservation, not an already
    // delivered callback. Keep it in the service queue until the recipient
    // polls so automatic connection-loss cleanup can cancel it atomically.
    if (options_.pushReceiveOrderEvents &&
        eventKind !=
            ProcessFederationAttributeOwnershipAcquisitionEventKind::request_release) {
      if (eventKind ==
          ProcessFederationAttributeOwnershipAcquisitionEventKind::
              acquisition_notification) {
        auto const delivery = registry_.beginAttributeOwnershipAcquisitionNotification(
            federationName,
            workItem.requestingFederateId,
            workItem.objectInstanceHandle,
            workItem.requestId,
            workItem.attributeHandles);
        if (!delivery) {
          continue;
        }
        event.attributeHandles = delivery->securedAttributeHandles;
        for (auto& followup : delivery->followupWorkItems) {
          workItems.push_back(std::move(followup));
        }
      } else if (eventKind ==
                 ProcessFederationAttributeOwnershipAcquisitionEventKind::
                     request_release) {
        auto const delivery = registry_.beginAttributeOwnershipAcquisitionRelease(
            federationName,
            workItem.requestingFederateId,
            workItem.receivingFederateId,
            workItem.objectInstanceHandle,
            workItem.requestId,
            workItem.attributeHandles);
        if (!delivery) {
          continue;
        }
        event.attributeHandles = delivery->candidateAttributeHandles;
      } else {
        auto const delivery = registry_.beginRequestDivestitureConfirmation(
            federationName,
            workItem.receivingFederateId,
            workItem.requestingFederateId,
            workItem.objectInstanceHandle,
            workItem.requestId,
            workItem.candidateIsIfAvailable,
            workItem.attributeHandles);
        if (!delivery) {
          continue;
        }
        event.attributeHandles = delivery->releasedAttributeHandles;
      }
      if (event.attributeHandles.empty()) {
        continue;
      }
      pushedEvents.emplace_back(receivingSession, std::move(event));
      continue;
    }

    std::scoped_lock lock(mutex_);
    auto const state = sessions_.find(receivingSession);
    if (state == sessions_.end() || !state->second.federationName ||
        *state->second.federationName != federationName ||
        state->second.federateId != receivingFederateId) {
      return false;
    }
    state->second.attributeOwnershipAcquisitionEvents.push_back(
        std::move(event));
  }

  if (!options_.pushReceiveOrderEvents) {
    return true;
  }
  for (auto& pushedEvent : pushedEvents) {
    ProcessFederationReceiveInteractionResult result;
    result.attributeOwnershipAcquisitionEvent = std::move(pushedEvent.second);
    if (pushedEvent.first == nullptr ||
        !pushedEvent.first->send(TransportServiceMessage{
            TransportServiceMessageKind::event,
            TransportServiceOperation::receive_interaction,
            TransportServiceStatus::ok,
            0U,
            encodeProcessFederationReceiveInteractionResult(result)})) {
      return false;
    }
  }
  return true;
}
bool ProcessFederationService::enqueueAttributeOwnershipAssumptionRecipients(
    std::wstring const& federationName,
    std::vector<AttributeOwnershipAssumptionRecipient> recipients,
    bool deferUntilCallbackEnabled) {
  for (auto& recipient : recipients) {
    if (recipient.receivingFederateId == 0U ||
        recipient.objectInstanceHandle == 0U ||
        recipient.attributeHandles.empty()) {
      return false;
    }
    ProcessTransportSession* receivingSession = nullptr;
    {
      std::scoped_lock lock(mutex_);
      auto const session = sessionsByFederateId_.find(recipient.receivingFederateId);
      if (session != sessionsByFederateId_.end()) {
        auto const state = sessions_.find(session->second);
        if (state != sessions_.end() && state->second.federationName &&
            *state->second.federationName == federationName &&
            state->second.federateId == recipient.receivingFederateId) {
          receivingSession = session->second;
        }
      }
    }
    if (receivingSession == nullptr) {
      return false;
    }

    // The ordinary ownership event frame keeps all process callback queues
    // ordered by one receive fence. Assumption has no request id, so the
    // candidate identity is repeated in both federate fields as a private
    // routing invariant and the callback bridge projects the official form.
    ProcessFederationAttributeOwnershipAcquisitionEvent event{
        ProcessFederationAttributeOwnershipAcquisitionEventKind::
            ownership_assumption,
        0U,
        recipient.receivingFederateId,
        recipient.receivingFederateId,
        recipient.objectInstanceHandle,
        recipient.attributeHandles,
        recipient.userSuppliedTag,
        false};
    if (options_.pushReceiveOrderEvents) {
      if (deferUntilCallbackEnabled) {
        std::scoped_lock lock(mutex_);
        auto const state = sessions_.find(receivingSession);
        if (state == sessions_.end() || !state->second.federationName ||
            *state->second.federationName != federationName ||
            state->second.federateId != recipient.receivingFederateId) {
          return false;
        }
        auto const duplicate = std::any_of(
            state->second.attributeOwnershipAcquisitionEvents.begin(),
            state->second.attributeOwnershipAcquisitionEvents.end(),
            [&event](ProcessFederationAttributeOwnershipAcquisitionEvent const& queued) {
              return queued.kind ==
                         ProcessFederationAttributeOwnershipAcquisitionEventKind::
                             ownership_assumption &&
                  queued.receivingFederateId == event.receivingFederateId &&
                  queued.objectInstanceHandle == event.objectInstanceHandle &&
                  queued.attributeHandles == event.attributeHandles &&
                  queued.userSuppliedTag == event.userSuppliedTag;
            });
        if (!duplicate) {
          state->second.attributeOwnershipAcquisitionEvents.push_back(
              std::move(event));
        }
        continue;
      }
      auto const delivery = registry_.attributeOwnershipAssumptionDeliveryFor(
          federationName,
          recipient.receivingFederateId,
          recipient.objectInstanceHandle,
          recipient.attributeHandles);
      if (!delivery || delivery->attributeHandles.empty()) {
        continue;
      }
      event.attributeHandles = delivery->attributeHandles;
      ProcessFederationReceiveInteractionResult pushedResult;
      pushedResult.attributeOwnershipAcquisitionEvent = std::move(event);
      if (!receivingSession->send(TransportServiceMessage{
              TransportServiceMessageKind::event,
              TransportServiceOperation::receive_interaction,
              TransportServiceStatus::ok,
              0U,
              encodeProcessFederationReceiveInteractionResult(pushedResult)})) {
        return false;
      }
      continue;
    }
    std::scoped_lock lock(mutex_);
    auto const state = sessions_.find(receivingSession);
    if (state == sessions_.end() || !state->second.federationName ||
        *state->second.federationName != federationName ||
        state->second.federateId != recipient.receivingFederateId) {
      return false;
    }
    state->second.attributeOwnershipAcquisitionEvents.push_back(std::move(event));
  }
  return true;
}

bool ProcessFederationService::
    flushDeferredAttributeOwnershipAssumptionEvents(
        ProcessTransportSession& session,
        std::wstring const& federationName,
        std::uint64_t receivingFederateId) {
  std::vector<ProcessFederationAttributeOwnershipAcquisitionEvent> deferred;
  {
    std::scoped_lock lock(mutex_);
    auto const state = sessions_.find(&session);
    if (state == sessions_.end() || !state->second.federationName ||
        *state->second.federationName != federationName ||
        state->second.federateId != receivingFederateId) {
      return false;
    }
    auto& queued = state->second.attributeOwnershipAcquisitionEvents;
    for (auto iterator = queued.begin(); iterator != queued.end();) {
      if (iterator->kind !=
              ProcessFederationAttributeOwnershipAcquisitionEventKind::
                  ownership_assumption ||
          iterator->receivingFederateId != receivingFederateId) {
        ++iterator;
        continue;
      }
      deferred.push_back(std::move(*iterator));
      iterator = queued.erase(iterator);
    }
  }

  for (auto& event : deferred) {
    auto const delivery = registry_.attributeOwnershipAssumptionDeliveryFor(
        federationName,
        receivingFederateId,
        event.objectInstanceHandle,
        event.attributeHandles);
    if (!delivery || delivery->attributeHandles.empty()) {
      continue;
    }
    event.attributeHandles = delivery->attributeHandles;
    ProcessFederationReceiveInteractionResult pushedResult;
    pushedResult.attributeOwnershipAcquisitionEvent = std::move(event);
    if (!session.send(TransportServiceMessage{
            TransportServiceMessageKind::event,
            TransportServiceOperation::receive_interaction,
            TransportServiceStatus::ok,
            0U,
            encodeProcessFederationReceiveInteractionResult(pushedResult)})) {
      return false;
    }
  }
  return true;
}

bool ProcessFederationService::enqueueConfirmDivestitureNotifications(
    std::wstring const& federationName,
    std::vector<ConfirmDivestitureNotification> notifications) {
  std::vector<std::pair<ProcessTransportSession*,
                        ProcessFederationAttributeOwnershipAcquisitionEvent>>
      pushedEvents;
  std::vector<AttributeOwnershipAcquisitionWorkItem> followupWorkItems;
  for (auto& notification : notifications) {
    if (notification.notificationId == 0U ||
        notification.receivingFederateId == 0U ||
        notification.objectInstanceHandle == 0U ||
        notification.attributeHandles.empty()) {
      return false;
    }

    ProcessTransportSession* receivingSession = nullptr;
    {
      std::scoped_lock lock(mutex_);
      auto const session =
          sessionsByFederateId_.find(notification.receivingFederateId);
      if (session != sessionsByFederateId_.end()) {
        auto const state = sessions_.find(session->second);
        if (state != sessions_.end() && state->second.federationName &&
            *state->second.federationName == federationName &&
            state->second.federateId == notification.receivingFederateId) {
          receivingSession = session->second;
        }
      }
    }
    if (receivingSession == nullptr) {
      return false;
    }

    // Confirm Divestiture has already committed ownership in the registry.
    // The callback boundary below consumes its typed reservation and makes
    // the secured set visible to the receiving federate in one receive-order
    // event frame.  Keep the private requester/recipient identities equal so
    // the public bridge can project the official acquisition notification.
    ProcessFederationAttributeOwnershipAcquisitionEvent event{
        ProcessFederationAttributeOwnershipAcquisitionEventKind::
            confirm_divestiture_notification,
        notification.notificationId,
        notification.receivingFederateId,
        notification.receivingFederateId,
        notification.objectInstanceHandle,
        notification.attributeHandles,
        notification.userSuppliedTag,
        false};

    if (options_.pushReceiveOrderEvents) {
      auto const delivery = registry_.beginConfirmDivestitureNotification(
          federationName,
          notification.receivingFederateId,
          notification.objectInstanceHandle,
          notification.notificationId,
          notification.attributeHandles);
      if (!delivery) {
        continue;
      }
      event.attributeHandles = delivery->securedAttributeHandles;
      if (!event.attributeHandles.empty()) {
        pushedEvents.emplace_back(receivingSession, std::move(event));
      }
      for (auto& followup : delivery->followupWorkItems) {
        followupWorkItems.push_back(std::move(followup));
      }
      continue;
    }

    std::scoped_lock lock(mutex_);
    auto const state = sessions_.find(receivingSession);
    if (state == sessions_.end() || !state->second.federationName ||
        *state->second.federationName != federationName ||
        state->second.federateId != notification.receivingFederateId) {
      return false;
    }
    state->second.attributeOwnershipAcquisitionEvents.push_back(
        std::move(event));
  }

  if (!options_.pushReceiveOrderEvents) {
    return true;
  }
  for (auto& pushedEvent : pushedEvents) {
    ProcessFederationReceiveInteractionResult result;
    result.attributeOwnershipAcquisitionEvent = std::move(pushedEvent.second);
    if (pushedEvent.first == nullptr ||
        !pushedEvent.first->send(TransportServiceMessage{
            TransportServiceMessageKind::event,
            TransportServiceOperation::receive_interaction,
            TransportServiceStatus::ok,
            0U,
            encodeProcessFederationReceiveInteractionResult(result)})) {
      return false;
    }
  }
  if (!followupWorkItems.empty() &&
      !enqueueAttributeOwnershipAcquisitionWorkItems(
          federationName, std::move(followupWorkItems))) {
    return false;
  }
  return true;
}
bool ProcessFederationService::enqueueAttributeOwnershipUnavailableRecipients(
    std::wstring const& federationName,
    std::vector<AttributeOwnershipUnavailableRecipient> recipients,
    std::vector<std::uint8_t> userSuppliedTag) {
  std::vector<std::pair<ProcessTransportSession*,
                        ProcessFederationAttributeOwnershipUnavailableEvent>>
      pushedEvents;
  for (auto& planned : recipients) {
    if (planned.receivingFederateId == 0U ||
        planned.objectInstanceHandle == 0U || planned.attributeHandles.empty()) {
      return false;
    }
    ProcessTransportSession* receivingSession = nullptr;
    {
      std::scoped_lock lock(mutex_);
      auto const session = sessionsByFederateId_.find(planned.receivingFederateId);
      if (session != sessionsByFederateId_.end()) {
        auto const state = sessions_.find(session->second);
        if (state != sessions_.end() && state->second.federationName &&
            *state->second.federationName == federationName &&
            state->second.federateId == planned.receivingFederateId) {
          receivingSession = session->second;
        }
      }
    }
    if (receivingSession == nullptr) {
      return false;
    }

    ProcessFederationAttributeOwnershipUnavailableEvent event{
        planned.receivingFederateId,
        planned.objectInstanceHandle,
        planned.attributeHandles,
        userSuppliedTag};
    if (options_.pushReceiveOrderEvents) {
      // Commit the callback reservation at the service boundary for pushed
      // events. Pull mode performs the same revalidation in receiveInteraction
      // immediately before exposing the event to the client.
      auto const delivery = registry_.attributeOwnershipUnavailableRecipientFor(
          federationName,
          event.receivingFederateId,
          event.objectInstanceHandle,
          event.attributeHandles);
      if (!delivery || delivery->attributeHandles.empty()) {
        continue;
      }
      event.attributeHandles = delivery->attributeHandles;
      pushedEvents.emplace_back(receivingSession, std::move(event));
    } else {
      std::scoped_lock lock(mutex_);
      auto const state = sessions_.find(receivingSession);
      if (state == sessions_.end() || !state->second.federationName ||
          *state->second.federationName != federationName ||
          state->second.federateId != planned.receivingFederateId) {
        return false;
      }
      state->second.attributeOwnershipUnavailableEvents.push_back(
          std::move(event));
    }
  }

  if (!options_.pushReceiveOrderEvents) {
    return true;
  }
  for (auto& pushedEvent : pushedEvents) {
    ProcessFederationReceiveInteractionResult result;
    result.attributeOwnershipUnavailableEvent = std::move(pushedEvent.second);
    if (pushedEvent.first == nullptr ||
        !pushedEvent.first->send(TransportServiceMessage{
            TransportServiceMessageKind::event,
            TransportServiceOperation::receive_interaction,
            TransportServiceStatus::ok,
            0U,
            encodeProcessFederationReceiveInteractionResult(result)})) {
      return false;
    }
  }
  return true;
}

}  // namespace umbra::detail
