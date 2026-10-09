#include "internal/federation/process_federation_service.hpp"
#include "internal/federation/process_federation_service_helpers.hpp"

#include <algorithm>
#include <cstdint>
#include <utility>
#include <vector>

namespace umbra::detail {

bool ProcessFederationService::enqueueObjectInstanceDiscoveries(
    std::wstring const& federationName,
    std::vector<ObjectInstanceDiscoveryRecipient> discoveries) {
  std::vector<std::pair<ProcessTransportSession*,
                        ProcessFederationObjectInstanceDiscoveryEvent>>
      pushedEvents;
  std::vector<std::pair<ProcessTransportSession*,
                        ProcessFederationAttributeUpdateEvent>>
      pushedMomAttributeEvents;
  std::vector<AttributeRelevanceAdvisoryRecipient> initialAdvisories;
  for (auto const& planned : discoveries) {
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
      // The registry reservation was made before the session lookup. Release
      // it when a detached process can no longer receive the event so a later
      // declaration change can retry the discovery.
      if (planned.rtiOwnedMomObject) {
        registry_.cancelJoinedFederateMomObjectDiscovery(
            federationName,
            planned.receivingFederateId,
            planned.objectInstanceHandle);
      } else {
        registry_.cancelObjectInstanceDiscovery(
            federationName,
            planned.receivingFederateId,
            planned.objectInstanceHandle);
      }
      continue;
    }

    // Recheck the discovery predicate and establish the receiving federate's
    // known-instance state before exposing the event. The process service has
    // no user callback route of its own, so this registry transition is the
    // process-boundary delivery commit; the client then projects the exact
    // snapshot through the official callback bridge.
    auto const snapshot = planned.rtiOwnedMomObject
        ? registry_.beginJoinedFederateMomObjectDiscovery(
              federationName,
              planned.receivingFederateId,
              planned.objectInstanceHandle)
        : registry_.beginObjectInstanceDiscovery(
              federationName,
              planned.receivingFederateId,
              planned.objectInstanceHandle);
    if (!snapshot) {
      continue;
    }
    if (!planned.rtiOwnedMomObject) {
      auto plannedAdvisories = registry_
          .planInitialAttributeRelevanceAdvisoriesForDiscovery(
              federationName,
              planned.receivingFederateId,
              planned.objectInstanceHandle);
      initialAdvisories.insert(
          initialAdvisories.end(),
          std::make_move_iterator(plannedAdvisories.begin()),
          std::make_move_iterator(plannedAdvisories.end()));
    }
    ProcessFederationObjectInstanceDiscoveryEvent event{
        planned.receivingFederateId,
        snapshot->objectInstanceHandle,
        snapshot->knownObjectClassHandle,
        snapshot->objectInstanceName,
        snapshot->producingFederateId,
        planned.rtiOwnedMomObject};

    // A joined-federate MOM discovery is immediately followed by its
    // subscribed initial values.  Keep that ordering on both pull and push
    // process seams; the public callback bridge then projects an invalid
    // producer handle exactly as the embedded profile does.
    std::optional<ProcessFederationAttributeUpdateEvent> initialMomValues;
    if (planned.rtiOwnedMomObject && !snapshot->initialAttributeHandles.empty()) {
      auto const initialPlan = registry_.planJoinedFederateMomAttributeValueUpdate(
          federationName,
          planned.receivingFederateId,
          snapshot->objectInstanceHandle,
          snapshot->initialAttributeHandles,
          true);
      if (initialPlan.status ==
              JoinedFederateMomAttributeValueUpdateStatus::applied &&
          initialPlan.recipient &&
          !initialPlan.recipient->attributeValues.empty()) {
        ProcessFederationAttributeUpdateEvent values;
        values.receivingFederateId = planned.receivingFederateId;
        values.objectInstanceHandle = snapshot->objectInstanceHandle;
        values.transportationName = "HLAreliable";
        values.rtiOwnedMomObject = true;
        values.attributeValues.reserve(
            initialPlan.recipient->attributeValues.size());
        for (auto const& [attributeHandle, encodedValue] :
             initialPlan.recipient->attributeValues) {
          std::vector<std::uint8_t> bytes(encodedValue.size());
          if (!bytes.empty()) {
            auto const* data = static_cast<std::uint8_t const*>(
                encodedValue.data());
            if (data == nullptr) {
              return false;
            }
            std::copy(data, data + bytes.size(), bytes.begin());
          }
          values.attributeValues.emplace_back(attributeHandle, std::move(bytes));
        }
        initialMomValues = std::move(values);
      }
    }
    if (options_.pushReceiveOrderEvents) {
      pushedEvents.emplace_back(receivingSession, std::move(event));
      if (initialMomValues) {
        pushedMomAttributeEvents.emplace_back(
            receivingSession, std::move(*initialMomValues));
      }
      continue;
    }

    std::scoped_lock lock(mutex_);
    auto const state = sessions_.find(receivingSession);
    if (state != sessions_.end() &&
        state->second.federationName.has_value() &&
        *state->second.federationName == federationName &&
        state->second.federateId == planned.receivingFederateId) {
      event.callbackOrderSequence =
          state->second.nextObjectLifecycleCallbackOrderSequence++;
      state->second.objectInstanceDiscoveryEvents.push_back(std::move(event));
      if (initialMomValues) {
        state->second.attributeUpdateEvents.push_back(std::move(*initialMomValues));
      }
    }
  }

  if (!options_.pushReceiveOrderEvents) {
    return enqueueAttributeRelevanceAdvisories(
        federationName,
        std::move(initialAdvisories));
  }
  for (auto const& pushedEvent : pushedEvents) {
    if (pushedEvent.first == nullptr || !pushedEvent.first->send(
            TransportServiceMessage{
                TransportServiceMessageKind::event,
                TransportServiceOperation::receive_object_instance_discovery,
                TransportServiceStatus::ok,
                0U,
                encodeProcessFederationReceiveObjectInstanceDiscoveryResult(
                    ProcessFederationReceiveObjectInstanceDiscoveryResult{
                        pushedEvent.second})})) {
      return false;
    }
  }
  for (auto const& pushedEvent : pushedMomAttributeEvents) {
    if (pushedEvent.first == nullptr || !pushedEvent.first->send(
            TransportServiceMessage{
                TransportServiceMessageKind::event,
                TransportServiceOperation::receive_attribute_update,
                TransportServiceStatus::ok,
                0U,
                encodeProcessFederationReceiveAttributeUpdateResult(
                    ProcessFederationReceiveAttributeUpdateResult{
                        pushedEvent.second})})) {
      return false;
    }
  }
  return enqueueAttributeRelevanceAdvisories(
      federationName,
      std::move(initialAdvisories));
}

}  // namespace umbra::detail
