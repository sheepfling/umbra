#include "internal/federation/process_federation_service.hpp"
#include "internal/federation/process_federation_service_helpers.hpp"

#include <algorithm>
#include <cstdint>
#include <optional>
#include <utility>
#include <vector>

namespace umbra::detail {

bool ProcessFederationService::enqueueObjectInstanceRemovals(
    std::wstring const& federationName,
    std::vector<ObjectInstanceRemovalRecipient> removals,
    std::vector<std::uint8_t> userSuppliedTag,
    std::optional<ProcessFederationLogicalTime> timestamp,
    std::uint64_t retractionMessageId,
    bool provideRetraction) {
  std::vector<std::pair<ProcessTransportSession*,
                        ProcessFederationObjectInstanceRemovalEvent>>
      pushedEvents;
  std::vector<ProcessTransportSession*> retractionRecipients;
  for (auto& planned : removals) {
    ProcessTransportSession* receivingSession = nullptr;
    {
      std::scoped_lock lock(mutex_);
      auto const session = sessionsByFederateId_.find(planned.receivingFederateId);
      if (session != sessionsByFederateId_.end()) {
        auto const state = sessions_.find(session->second);
        if (state != sessions_.end() &&
            state->second.federationName.has_value() &&
            *state->second.federationName == federationName &&
            state->second.federateId == planned.receivingFederateId) {
          receivingSession = session->second;
        }
      }
    }
    if (receivingSession == nullptr) {
      // The registry reserved this callback before the session lookup. A
      // detached process cannot receive an ordinary callback, so release the
      // ordinary reservation. A timestamped removal owns a TSO retraction
      // ledger entry instead; keep that entry until the producer retracts it
      // or the federation lifecycle cleans it up.
      if (retractionMessageId == 0U) {
        if (planned.rtiOwnedMomObject) {
          registry_.cancelJoinedFederateMomObjectRemoval(
              federationName,
              planned.receivingFederateId,
              planned.objectInstanceHandle);
        } else {
          registry_.cancelObjectInstanceRemoval(
              federationName,
              planned.receivingFederateId,
              planned.objectInstanceHandle);
        }
      }
      continue;
    }

    ProcessFederationObjectInstanceRemovalEvent event{
        planned.receivingFederateId,
        planned.objectInstanceHandle,
        0U,
        userSuppliedTag};
    event.rtiOwnedMomObject = planned.rtiOwnedMomObject;
    event.timestamp = timestamp;
    if (retractionMessageId != 0U) {
      event.retractionMessageId = retractionMessageId;
      event.provideRetraction = provideRetraction;
      event.sentOrderType = planned.sentOrderType;
      event.receivedOrderType = planned.receivedOrderType;
    }
    if (options_.pushReceiveOrderEvents) {
      auto const snapshot = planned.rtiOwnedMomObject
          ? registry_.beginJoinedFederateMomObjectRemoval(
                federationName,
                planned.receivingFederateId,
                planned.objectInstanceHandle)
          : retractionMessageId == 0U
          ? registry_.beginObjectInstanceRemoval(
                federationName,
                planned.receivingFederateId,
                planned.objectInstanceHandle)
          : registry_.beginTsoObjectInstanceRemoval(
                federationName,
                planned.receivingFederateId,
                planned.objectInstanceHandle,
                retractionMessageId);
      if (!snapshot) {
        continue;
      }
      event.producingFederateId = snapshot->producingFederateId;
      pushedEvents.emplace_back(receivingSession, std::move(event));
      if (retractionMessageId != 0U) {
        retractionRecipients.push_back(receivingSession);
      }
      continue;
    }

    // Pull-mode sessions retain the pending reservation until their next
    // receive poll, where handleReceiveInteraction commits the callback-time
    // registry transition and fills the producer identity.
    std::scoped_lock lock(mutex_);
    auto const state = sessions_.find(receivingSession);
    if (state != sessions_.end() &&
        state->second.federationName.has_value() &&
        *state->second.federationName == federationName &&
        state->second.federateId == planned.receivingFederateId) {
      event.callbackOrderSequence =
          state->second.nextObjectLifecycleCallbackOrderSequence++;
      state->second.objectInstanceRemovalEvents.push_back(std::move(event));
      if (retractionMessageId != 0U) {
        retractionRecipients.push_back(receivingSession);
      }
    } else {
      if (retractionMessageId == 0U) {
        if (planned.rtiOwnedMomObject) {
          registry_.cancelJoinedFederateMomObjectRemoval(
              federationName,
              planned.receivingFederateId,
              planned.objectInstanceHandle);
        } else {
          registry_.cancelObjectInstanceRemoval(
              federationName,
              planned.receivingFederateId,
              planned.objectInstanceHandle);
        }
      }
    }
  }

  if (retractionMessageId != 0U) {
    std::scoped_lock lock(mutex_);
    pendingPushedRetractionRecipients_.emplace(
        retractionMessageId,
        std::move(retractionRecipients));
  }

  if (!options_.pushReceiveOrderEvents) {
    return true;
  }
  for (auto& pushedEvent : pushedEvents) {
    ProcessFederationReceiveInteractionResult result;
    result.removalEvent = std::move(pushedEvent.second);
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
