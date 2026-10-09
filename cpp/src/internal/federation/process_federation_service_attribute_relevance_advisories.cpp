#include "internal/federation/process_federation_service.hpp"
#include "internal/federation/process_federation_service_helpers.hpp"

#include <algorithm>
#include <cstdint>
#include <utility>
#include <vector>

namespace umbra::detail {

bool ProcessFederationService::enqueueAttributeRelevanceAdvisories(
    std::wstring const& federationName,
    std::vector<AttributeRelevanceAdvisoryRecipient> advisories) {
  std::vector<std::pair<ProcessTransportSession*,
                        ProcessFederationAttributeRelevanceAdvisoryEvent>>
      pushedEvents;
  for (auto const& planned : advisories) {
    if (planned.attributeHandles.empty() || planned.providingFederateId == 0U) {
      continue;
    }

    // Attribute Relevance Advisory is delivered to the providing/owning
    // federate, not to the receiving subscription federate.  Resolve that
    // session before crossing the process boundary; a detached owner cannot
    // receive a callback and therefore must not retain stale queued work.
    ProcessTransportSession* providingSession = nullptr;
    {
      std::scoped_lock lock(mutex_);
      auto const session = sessionsByFederateId_.find(planned.providingFederateId);
      if (session != sessionsByFederateId_.end()) {
        auto const state = sessions_.find(session->second);
        if (state != sessions_.end() &&
            state->second.federationName.has_value() &&
            *state->second.federationName == federationName &&
            state->second.federateId == planned.providingFederateId) {
          providingSession = session->second;
        }
      }
    }
    if (providingSession == nullptr) {
      continue;
    }

    // Recheck the switch, ownership, object lifetime, and relevance before
    // admission.  The registry deliberately accepts receivingFederateId == 0
    // for this owner-directed callback family.
    auto const eligible = registry_.attributeRelevanceAdvisoryAttributes(
        federationName,
        planned.providingFederateId,
        planned.receivingFederateId,
        planned.objectInstanceHandle,
        planned.attributeHandles,
        planned.turnUpdatesOn);
    if (eligible.empty()) {
      continue;
    }

    std::map<std::optional<std::string>, std::set<std::uint64_t>> byRate;
    for (std::uint64_t const attributeHandle : eligible) {
      auto const updateRateDesignator = planned.turnUpdatesOn
          ? registry_.attributeRelevanceAdvisoryUpdateRateDesignatorFor(
                federationName,
                planned.receivingFederateId,
                planned.objectInstanceHandle,
                attributeHandle)
          : std::nullopt;
      byRate[updateRateDesignator].insert(attributeHandle);
    }
    for (auto& [updateRateDesignator, attributeHandles] : byRate) {
      ProcessFederationAttributeRelevanceAdvisoryEvent event{
          planned.providingFederateId,
          planned.receivingFederateId,
          planned.objectInstanceHandle,
          std::move(attributeHandles),
          planned.turnUpdatesOn,
          std::move(updateRateDesignator)};
      if (options_.pushReceiveOrderEvents) {
        pushedEvents.emplace_back(providingSession, std::move(event));
      } else {
        std::scoped_lock lock(mutex_);
        auto const state = sessions_.find(providingSession);
        if (state != sessions_.end() &&
            state->second.federationName.has_value() &&
            *state->second.federationName == federationName &&
            state->second.federateId == planned.providingFederateId) {
          state->second.attributeRelevanceAdvisoryEvents.push_back(
              std::move(event));
        }
      }
    }
  }

  if (!options_.pushReceiveOrderEvents) {
    return true;
  }
  for (auto& pushedEvent : pushedEvents) {
    ProcessFederationReceiveInteractionResult result;
    result.attributeRelevanceAdvisoryEvent = std::move(pushedEvent.second);
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
