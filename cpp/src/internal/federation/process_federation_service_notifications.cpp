#include "internal/federation/process_federation_service.hpp"
#include "internal/federation/process_federation_service_helpers.hpp"
#include "internal/fom/hla_names.hpp"

#include <algorithm>
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <utility>
#include <vector>

namespace umbra::detail {

void ProcessFederationService::dispatchTimeAdvanceGrants(
    std::wstring const& federationName,
    std::vector<FederationTimeGrantDispatch> dispatches) {
  // Each dispatch may mutate the shared temporal snapshot, so re-evaluate
  // between rounds instead of assuming that every grant selected from one
  // snapshot remains eligible after the first callback is committed.
  constexpr std::size_t maximumRounds = 1024U;
  std::size_t rounds = 0U;
  while (!dispatches.empty()) {
    if (++rounds > maximumRounds) {
      throw std::runtime_error(
          "The process federation time-grant scheduler did not converge.");
    }
    for (auto& dispatch : dispatches) {
      if (dispatch) {
        dispatch();
      }
    }
    auto const reevaluated = registry_.reevaluateTimeAdvanceGrants(federationName);
    if (reevaluated.status != FederationTimeGrantStatus::applied) {
      return;
    }
    dispatches = reevaluated.dispatches;
  }
}

bool ProcessFederationService::enqueueJoinedFederateMomConditionalAttributeUpdate(
    std::wstring const& federationName,
    std::uint64_t federateId,
    std::vector<std::string> const& attributeNames) {
  auto const object = registry_.joinedFederateMomObjectFor(
      federationName, federateId);
  if (!object) {
    return true;
  }
  std::set<std::uint64_t> attributeHandles;
  for (auto const& attributeName : attributeNames) {
    auto const handle = registry_.attributeHandleFor(
        federationName,
        std::string(hla::utf8::mom::federate_object_class),
        attributeName);
    if (!handle) {
      return true;
    }
    attributeHandles.insert(*handle);
  }
  auto const plan = registry_.planJoinedFederateMomAttributeValueUpdateForObject(
      federationName, object->objectInstanceHandle, attributeHandles, true);
  for (auto const& recipient : plan.recipients) {
    if (recipient.attributeValues.empty()) {
      continue;
    }
    ProcessFederationAttributeUpdateEvent event;
    event.receivingFederateId = recipient.receivingFederateId;
    event.objectInstanceHandle = recipient.objectInstanceHandle;
    event.transportationName = "HLAreliable";
    event.rtiOwnedMomObject = true;
    event.attributeValues.reserve(recipient.attributeValues.size());
    for (auto const& [attributeHandle, encodedValue] : recipient.attributeValues) {
      std::vector<std::uint8_t> bytes(encodedValue.size());
      if (!bytes.empty()) {
        auto const* data = static_cast<std::uint8_t const*>(encodedValue.data());
        if (data == nullptr) {
          return false;
        }
        std::copy(data, data + bytes.size(), bytes.begin());
      }
      event.attributeValues.emplace_back(attributeHandle, std::move(bytes));
    }
    ProcessTransportSession* receivingSession = nullptr;
    {
      std::scoped_lock lock(mutex_);
      auto const session = sessionsByFederateId_.find(recipient.receivingFederateId);
      if (session == sessionsByFederateId_.end()) {
        continue;
      }
      auto const state = sessions_.find(session->second);
      if (state == sessions_.end() ||
          !state->second.federationName.has_value() ||
          *state->second.federationName != federationName ||
          state->second.federateId != recipient.receivingFederateId) {
        continue;
      }
      if (!options_.pushReceiveOrderEvents) {
        state->second.attributeUpdateEvents.push_back(std::move(event));
        continue;
      }
      receivingSession = session->second;
    }
    if (!receivingSession->send(TransportServiceMessage{
            TransportServiceMessageKind::event,
            TransportServiceOperation::receive_attribute_update,
            TransportServiceStatus::ok,
            0U,
            encodeProcessFederationReceiveAttributeUpdateResult(
                ProcessFederationReceiveAttributeUpdateResult{std::move(event)})})) {
      return false;
    }
  }
  return true;
}

bool ProcessFederationService::enqueueObjectInstanceScopeChanges(
    std::wstring const& federationName,
    std::vector<ObjectInstanceScopeChangeRecipient> changes) {
  std::vector<std::pair<ProcessTransportSession*,
                        ProcessFederationObjectInstanceScopeChangeEvent>>
      pushedEvents;
  for (auto const& planned : changes) {
    if (planned.attributeHandles.empty()) {
      continue;
    }
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
      continue;
    }

    // The registry owns the callback-time predicate. Recheck it before the
    // event crosses the process boundary so disabled switches, resignation,
    // deletion, or a second region/subscription mutation suppress stale work.
    auto const eligible = registry_.objectInstanceScopeAttributes(
        federationName,
        planned.receivingFederateId,
        planned.objectInstanceHandle,
        planned.attributeHandles,
        planned.inScope);
    if (eligible.empty()) {
      continue;
    }
    ProcessFederationObjectInstanceScopeChangeEvent event{
        planned.receivingFederateId,
        planned.objectInstanceHandle,
        eligible,
        planned.inScope};
    if (options_.pushReceiveOrderEvents) {
      pushedEvents.emplace_back(receivingSession, std::move(event));
      continue;
    }

    std::scoped_lock lock(mutex_);
    auto const state = sessions_.find(receivingSession);
    if (state != sessions_.end() &&
        state->second.federationName.has_value() &&
        *state->second.federationName == federationName &&
        state->second.federateId == planned.receivingFederateId) {
      state->second.objectInstanceScopeChangeEvents.push_back(std::move(event));
    }
  }

  if (!options_.pushReceiveOrderEvents) {
    return true;
  }
  for (auto& pushedEvent : pushedEvents) {
    ProcessFederationReceiveInteractionResult result;
    result.scopeChangeEvent = std::move(pushedEvent.second);
    if (pushedEvent.first == nullptr || !pushedEvent.first->send(
            TransportServiceMessage{
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
bool ProcessFederationService::enqueueSynchronizationPointAnnouncements(
    std::wstring const& federationName,
    std::vector<SynchronizationPointAnnouncement> announcements) {
  std::vector<std::pair<ProcessTransportSession*,
                        ProcessFederationSynchronizationPointAnnouncementEvent>>
      pushedEvents;
  for (auto& planned : announcements) {
    ProcessTransportSession* receivingSession = nullptr;
    {
      std::scoped_lock lock(mutex_);
      auto const session = sessionsByFederateId_.find(planned.receivingFederateId);
      if (session != sessionsByFederateId_.end()) {
        auto const state = sessions_.find(session->second);
        if (state != sessions_.end() && state->second.federationName.has_value() &&
            *state->second.federationName == federationName &&
            state->second.federateId == planned.receivingFederateId) {
          receivingSession = session->second;
        }
      }
    }
    if (receivingSession == nullptr) {
      continue;
    }
    ProcessFederationSynchronizationPointAnnouncementEvent event{
        planned.receivingFederateId,
        planned.label,
        std::vector<std::uint8_t>(
            planned.userSuppliedTag.begin(), planned.userSuppliedTag.end())};
    if (options_.pushReceiveOrderEvents) {
      pushedEvents.emplace_back(receivingSession, std::move(event));
      continue;
    }
    std::scoped_lock lock(mutex_);
    auto const state = sessions_.find(receivingSession);
    if (state != sessions_.end() && state->second.federationName.has_value() &&
        *state->second.federationName == federationName &&
        state->second.federateId == planned.receivingFederateId) {
      state->second.synchronizationPointAnnouncementEvents.push_back(
          std::move(event));
    }
  }
  if (!options_.pushReceiveOrderEvents) {
    return true;
  }
  for (auto& pushedEvent : pushedEvents) {
    ProcessFederationReceiveInteractionResult result;
    result.synchronizationPointAnnouncementEvent = std::move(pushedEvent.second);
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

bool ProcessFederationService::enqueueFederationSynchronizedNotifications(
    std::wstring const& federationName,
    std::vector<FederationSynchronizedNotification> notifications) {
  std::vector<std::pair<ProcessTransportSession*,
                        ProcessFederationFederationSynchronizedEvent>>
      pushedEvents;
  for (auto& planned : notifications) {
    if (planned.receivingFederateId == 0U) {
      continue;
    }
    ProcessTransportSession* receivingSession = nullptr;
    {
      std::scoped_lock lock(mutex_);
      auto const session = sessionsByFederateId_.find(planned.receivingFederateId);
      if (session != sessionsByFederateId_.end()) {
        auto const state = sessions_.find(session->second);
        if (state != sessions_.end() && state->second.federationName.has_value() &&
            *state->second.federationName == federationName &&
            state->second.federateId == planned.receivingFederateId) {
          receivingSession = session->second;
        }
      }
    }
    if (receivingSession == nullptr) {
      continue;
    }
    ProcessFederationFederationSynchronizedEvent event{
        planned.receivingFederateId,
        planned.label,
        planned.failedToSyncFederateIds};
    if (options_.pushReceiveOrderEvents) {
      pushedEvents.emplace_back(receivingSession, std::move(event));
      continue;
    }
    std::scoped_lock lock(mutex_);
    auto const state = sessions_.find(receivingSession);
    if (state != sessions_.end() && state->second.federationName.has_value() &&
        *state->second.federationName == federationName &&
        state->second.federateId == planned.receivingFederateId) {
      state->second.federationSynchronizedEvents.push_back(std::move(event));
    }
  }
  if (!options_.pushReceiveOrderEvents) {
    return true;
  }
  for (auto& pushedEvent : pushedEvents) {
    ProcessFederationReceiveInteractionResult result;
    result.federationSynchronizedEvent = std::move(pushedEvent.second);
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
bool ProcessFederationService::enqueueFederationSaveNotifications(
    std::wstring const& federationName,
    std::vector<FederationSaveNotification> notifications) {
  std::vector<std::pair<ProcessTransportSession*, ProcessFederationSaveEvent>>
      pushedEvents;
  for (auto& planned : notifications) {
    ProcessTransportSession* receivingSession = nullptr;
    {
      std::scoped_lock lock(mutex_);
      auto const session = sessionsByFederateId_.find(planned.receivingFederateId);
      if (session != sessionsByFederateId_.end()) {
        auto const state = sessions_.find(session->second);
        if (state != sessions_.end() && state->second.federationName.has_value() &&
            *state->second.federationName == federationName &&
            state->second.federateId == planned.receivingFederateId) {
          receivingSession = session->second;
        }
      }
    }
    if (receivingSession == nullptr) {
      continue;
    }
    ProcessFederationSaveEvent event{
        planned.kind,
        planned.receivingFederateId,
        planned.label,
        planned.successful,
        planned.failureReason,
        std::move(planned.statuses)};
    event.timestamp = encodeProcessLogicalTime(planned.timestamp);
    if (planned.timestamp && !event.timestamp) {
      return false;
    }
    if (options_.pushReceiveOrderEvents) {
      pushedEvents.emplace_back(receivingSession, std::move(event));
      continue;
    }
    std::scoped_lock lock(mutex_);
    auto const state = sessions_.find(receivingSession);
    if (state != sessions_.end() && state->second.federationName.has_value() &&
        *state->second.federationName == federationName &&
        state->second.federateId == planned.receivingFederateId) {
      state->second.saveEvents.push_back(std::move(event));
    }
  }
  if (!options_.pushReceiveOrderEvents) {
    return true;
  }
  for (auto& pushedEvent : pushedEvents) {
    ProcessFederationReceiveInteractionResult result;
    result.saveEvent = std::move(pushedEvent.second);
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

bool ProcessFederationService::enqueueFederationRestoreNotifications(
    std::wstring const& federationName,
    std::vector<FederationRestoreNotification> notifications) {
  std::vector<std::pair<ProcessTransportSession*, ProcessFederationRestoreEvent>>
      pushedEvents;
  for (auto& planned : notifications) {
    ProcessTransportSession* receivingSession = nullptr;
    {
      std::scoped_lock lock(mutex_);
      auto const session = sessionsByFederateId_.find(planned.receivingFederateId);
      if (session != sessionsByFederateId_.end()) {
        auto const state = sessions_.find(session->second);
        if (state != sessions_.end() && state->second.federationName.has_value() &&
            *state->second.federationName == federationName &&
            state->second.federateId == planned.receivingFederateId) {
          receivingSession = session->second;
        }
      }
    }
    if (receivingSession == nullptr) {
      continue;
    }
    ProcessFederationRestoreEvent event;
    event.kind = planned.kind;
    event.receivingFederateId = planned.receivingFederateId;
    event.label = planned.label;
    event.federateName = planned.federateName;
    event.preRestoreFederateId = planned.preRestoreFederateId;
    event.postRestoreFederateId = planned.postRestoreFederateId;
    event.successful = planned.successful;
    event.failureReason = planned.failureReason;
    event.statuses.reserve(planned.statuses.size());
    for (auto const& status : planned.statuses) {
      event.statuses.push_back(ProcessFederationRestoreEvent::StatusRecord{
          status.preRestoreFederateId,
          status.postRestoreFederateId,
          status.status});
    }
    if (options_.pushReceiveOrderEvents) {
      pushedEvents.emplace_back(receivingSession, std::move(event));
      continue;
    }
    std::scoped_lock lock(mutex_);
    auto const state = sessions_.find(receivingSession);
    if (state != sessions_.end() && state->second.federationName.has_value() &&
        *state->second.federationName == federationName &&
        state->second.federateId == planned.receivingFederateId) {
      state->second.restoreEvents.push_back(std::move(event));
    }
  }
  if (!options_.pushReceiveOrderEvents) {
    return true;
  }
  for (auto& pushedEvent : pushedEvents) {
    ProcessFederationReceiveInteractionResult result;
    result.restoreEvent = std::move(pushedEvent.second);
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
