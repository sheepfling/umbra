#include "internal/federation/process_federation_service.hpp"
#include "internal/federation/process_federation_service_helpers.hpp"

#include <algorithm>
#include <cstdint>
#include <memory>
#include <utility>
#include <vector>

namespace umbra::detail {

TransportServiceMessage
ProcessFederationService::handleRequestAttributeValueUpdate(
    ProcessTransportSession& session,
    TransportServiceMessage const& request) {
  auto const requestValue =
      decodeProcessFederationRequestAttributeValueUpdateRequest(request.payload);
  std::set<std::uint64_t> requestedAttributeHandles(
      requestValue.requestedAttributeHandles.begin(),
      requestValue.requestedAttributeHandles.end());
  {
    std::scoped_lock lock(mutex_);
    auto const state = sessions_.find(&session);
    if (state == sessions_.end() ||
        !state->second.federationName.has_value() ||
        *state->second.federationName != requestValue.federationName ||
        state->second.federateId != requestValue.requestingFederateId ||
        !registry_.memberById(
            requestValue.federationName,
            requestValue.requestingFederateId)) {
      return rejected(request);
    }
  }

  // RTI-owned joined-federate MOM instances are not stored in the ordinary
  // objectInstances table. Resolve a requested value update against the MOM
  // ledger first and reflect its immutable/request-time values directly to
  // the requesting process session; these values never induce a federate
  // Provide Attribute Value Update callback.
  auto const momPlan = registry_.planJoinedFederateMomAttributeValueUpdate(
      requestValue.federationName,
      requestValue.requestingFederateId,
      requestValue.objectInstanceHandle,
      requestedAttributeHandles);
  if (momPlan.rtiOwnedMomObject) {
    if (momPlan.status !=
            JoinedFederateMomAttributeValueUpdateStatus::applied ||
        !momPlan.recipient) {
      return rejected(request);
    }

    ProcessFederationAttributeUpdateEvent event;
    event.receivingFederateId = requestValue.requestingFederateId;
    event.objectInstanceHandle = momPlan.recipient->objectInstanceHandle;
    event.userSuppliedTag = requestValue.userSuppliedTag;
    event.transportationName = "HLAreliable";
    event.rtiOwnedMomObject = true;
    event.attributeValues.reserve(momPlan.recipient->attributeValues.size());
    for (auto const& [attributeHandle, encodedValue] :
         momPlan.recipient->attributeValues) {
      std::vector<std::uint8_t> bytes(encodedValue.size());
      if (!bytes.empty()) {
        auto const* data = static_cast<std::uint8_t const*>(encodedValue.data());
        if (data == nullptr) {
          return internalError(request);
        }
        std::copy(data, data + bytes.size(), bytes.begin());
      }
      event.attributeValues.emplace_back(attributeHandle, std::move(bytes));
    }

    if (options_.pushReceiveOrderEvents) {
      if (!session.send(TransportServiceMessage{
              TransportServiceMessageKind::event,
              TransportServiceOperation::receive_attribute_update,
              TransportServiceStatus::ok,
              0U,
              encodeProcessFederationReceiveAttributeUpdateResult(
                  ProcessFederationReceiveAttributeUpdateResult{
                      std::move(event)})})) {
        return internalError(request);
      }
    } else {
      std::scoped_lock lock(mutex_);
      auto const state = sessions_.find(&session);
      if (state == sessions_.end() ||
          !state->second.federationName.has_value() ||
          *state->second.federationName != requestValue.federationName ||
          state->second.federateId != requestValue.requestingFederateId ||
          !registry_.memberById(
              requestValue.federationName,
              requestValue.requestingFederateId)) {
        return rejected(request);
      }
      state->second.attributeUpdateEvents.push_back(std::move(event));
    }
    return responseFor(
        request,
        TransportServiceStatus::ok,
        encodeProcessFederationRequestAttributeValueUpdateResult(
            ProcessFederationRequestAttributeValueUpdateResult{1U}));
  }

  auto const plan = registry_.planAttributeValueUpdateRequest(
      requestValue.federationName,
      requestValue.requestingFederateId,
      requestValue.objectInstanceHandle,
      requestedAttributeHandles);
  if (plan.status != AttributeValueUpdateRequestStatus::applied) {
    return rejected(request);
  }

  std::vector<std::pair<ProcessTransportSession*,
                        ProcessFederationAttributeValueUpdateRequestEvent>>
      pushedEvents;
  std::uint32_t recipientCount = 0U;
  for (auto const& planned : plan.recipients) {
    ProcessTransportSession* providingSession = nullptr;
    {
      std::scoped_lock lock(mutex_);
      auto const found = sessionsByFederateId_.find(planned.providingFederateId);
      if (found != sessionsByFederateId_.end()) {
        auto const state = sessions_.find(found->second);
        if (state != sessions_.end() &&
            state->second.federationName.has_value() &&
            *state->second.federationName == requestValue.federationName &&
            state->second.federateId == planned.providingFederateId) {
          providingSession = found->second;
        }
      }
    }
    if (providingSession == nullptr) {
      continue;
    }

    // Persist and consume the private request at the process delivery
    // boundary. The process callback bridge has no registry reference, so the
    // service carries the already revalidated attribute subset in the event.
    auto const requestId = registry_.registerAttributeValueUpdateRequest(
        requestValue.federationName,
        requestValue.requestingFederateId,
        planned.providingFederateId,
        planned.objectInstanceHandle,
        planned.requestedAttributeHandles,
        requestValue.userSuppliedTag);
    if (!requestId) {
      return internalError(request);
    }
    auto const projection = registry_.beginAttributeValueUpdateProvideRecipientFor(
        requestValue.federationName,
        *requestId,
        requestValue.requestingFederateId,
        planned.providingFederateId,
        planned.objectInstanceHandle,
        planned.requestedAttributeHandles);
    if (!projection) {
      continue;
    }
    ProcessFederationAttributeValueUpdateRequestEvent event{
        requestValue.requestingFederateId,
        planned.providingFederateId,
        projection->objectInstanceHandle,
        projection->requestedAttributeHandles,
        requestValue.userSuppliedTag};
    if (options_.pushReceiveOrderEvents) {
      pushedEvents.emplace_back(providingSession, std::move(event));
    } else {
      std::scoped_lock lock(mutex_);
      auto const state = sessions_.find(providingSession);
      if (state != sessions_.end() &&
          state->second.federationName.has_value() &&
          *state->second.federationName == requestValue.federationName &&
          state->second.federateId == planned.providingFederateId) {
        state->second.attributeValueUpdateRequestEvents.push_back(
            std::move(event));
      } else {
        continue;
      }
    }
    if (recipientCount != std::numeric_limits<std::uint32_t>::max()) {
      ++recipientCount;
    }
  }

  if (options_.pushReceiveOrderEvents) {
    for (auto& pushedEvent : pushedEvents) {
      ProcessFederationReceiveInteractionResult result;
      result.attributeValueUpdateRequestEvent = std::move(pushedEvent.second);
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
      encodeProcessFederationRequestAttributeValueUpdateResult(
          ProcessFederationRequestAttributeValueUpdateResult{recipientCount}));
}
TransportServiceMessage
ProcessFederationService::handleRequestAttributeValueUpdateClass(
    ProcessTransportSession& session,
    TransportServiceMessage const& request) {
  auto const requestValue =
      decodeProcessFederationRequestAttributeValueUpdateClassRequest(
          request.payload);
  std::set<std::uint64_t> requestedAttributeHandles(
      requestValue.requestedAttributeHandles.begin(),
      requestValue.requestedAttributeHandles.end());
  {
    std::scoped_lock lock(mutex_);
    auto const state = sessions_.find(&session);
    if (state == sessions_.end() ||
        !state->second.federationName.has_value() ||
        *state->second.federationName != requestValue.federationName ||
        state->second.federateId != requestValue.requestingFederateId ||
        !registry_.memberById(
            requestValue.federationName,
            requestValue.requestingFederateId)) {
      return rejected(request);
    }
  }

  auto const plan = registry_.planAttributeValueUpdateClassRequest(
      requestValue.federationName,
      requestValue.requestingFederateId,
      requestValue.objectClassHandle,
      requestedAttributeHandles);
  if (plan.status != AttributeValueUpdateClassRequestStatus::applied) {
    return rejected(request);
  }

  std::vector<std::pair<ProcessTransportSession*,
                        ProcessFederationAttributeValueUpdateRequestEvent>>
      pushedEvents;
  std::uint32_t recipientCount = 0U;
  for (auto const& planned : plan.recipients) {
    ProcessTransportSession* providingSession = nullptr;
    {
      std::scoped_lock lock(mutex_);
      auto const found =
          sessionsByFederateId_.find(planned.providingFederateId);
      if (found != sessionsByFederateId_.end()) {
        auto const state = sessions_.find(found->second);
        if (state != sessions_.end() &&
            state->second.federationName.has_value() &&
            *state->second.federationName == requestValue.federationName &&
            state->second.federateId == planned.providingFederateId) {
          providingSession = found->second;
        }
      }
    }
    if (providingSession == nullptr) {
      continue;
    }

    auto const requestId = registry_.registerAttributeValueUpdateClassRequest(
        requestValue.federationName,
        requestValue.requestingFederateId,
        planned.providingFederateId,
        planned.objectInstanceHandle,
        requestValue.objectClassHandle,
        planned.requestedAttributeHandles,
        requestValue.userSuppliedTag);
    if (!requestId) {
      return internalError(request);
    }
    auto const projection =
        registry_.beginAttributeValueUpdateClassProvideRecipientFor(
            requestValue.federationName,
            *requestId,
            requestValue.requestingFederateId,
            planned.providingFederateId,
            planned.objectInstanceHandle,
            requestValue.objectClassHandle,
            planned.requestedAttributeHandles);
    if (!projection) {
      continue;
    }
    ProcessFederationAttributeValueUpdateRequestEvent event{
        requestValue.requestingFederateId,
        planned.providingFederateId,
        projection->objectInstanceHandle,
        projection->requestedAttributeHandles,
        requestValue.userSuppliedTag};
    if (options_.pushReceiveOrderEvents) {
      pushedEvents.emplace_back(providingSession, std::move(event));
    } else {
      std::scoped_lock lock(mutex_);
      auto const state = sessions_.find(providingSession);
      if (state != sessions_.end() &&
          state->second.federationName.has_value() &&
          *state->second.federationName == requestValue.federationName &&
          state->second.federateId == planned.providingFederateId) {
        state->second.attributeValueUpdateRequestEvents.push_back(
            std::move(event));
      } else {
        continue;
      }
    }
    if (recipientCount != std::numeric_limits<std::uint32_t>::max()) {
      ++recipientCount;
    }
  }

  if (options_.pushReceiveOrderEvents) {
    for (auto& pushedEvent : pushedEvents) {
      ProcessFederationReceiveInteractionResult result;
      result.attributeValueUpdateRequestEvent = std::move(pushedEvent.second);
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
      encodeProcessFederationRequestAttributeValueUpdateResult(
          ProcessFederationRequestAttributeValueUpdateResult{recipientCount}));
}

TransportServiceMessage
ProcessFederationService::handleRequestAttributeValueUpdateClassWithRegions(
    ProcessTransportSession& session,
    TransportServiceMessage const& request) {
  auto const requestValue =
      decodeProcessFederationRequestAttributeValueUpdateClassWithRegionsRequest(
          request.payload);
  std::set<std::uint64_t> requestedAttributeHandles(
      requestValue.requestedAttributeHandles.begin(),
      requestValue.requestedAttributeHandles.end());
  {
    std::scoped_lock lock(mutex_);
    auto const state = sessions_.find(&session);
    if (state == sessions_.end() ||
        !state->second.federationName.has_value() ||
        *state->second.federationName != requestValue.federationName ||
        state->second.federateId != requestValue.requestingFederateId ||
        !registry_.memberById(
            requestValue.federationName, requestValue.requestingFederateId)) {
      return rejected(request);
    }
  }

  auto const plan = registry_.planAttributeValueUpdateClassRequest(
      requestValue.federationName,
      requestValue.requestingFederateId,
      requestValue.objectClassHandle,
      requestedAttributeHandles,
      &requestValue.requestRegionsByAttribute);
  if (plan.status != AttributeValueUpdateClassRequestStatus::applied) {
    return responseFor(
        request,
        TransportServiceStatus::rejected,
        encodeProcessFederationRequestAttributeValueUpdateResult(
            ProcessFederationRequestAttributeValueUpdateResult{0U, plan.status}));
  }

  std::vector<std::pair<ProcessTransportSession*,
                        ProcessFederationAttributeValueUpdateRequestEvent>>
      pushedEvents;
  std::uint32_t recipientCount = 0U;
  for (auto const& planned : plan.recipients) {
    ProcessTransportSession* providingSession = nullptr;
    {
      std::scoped_lock lock(mutex_);
      auto const found =
          sessionsByFederateId_.find(planned.providingFederateId);
      if (found != sessionsByFederateId_.end()) {
        auto const state = sessions_.find(found->second);
        if (state != sessions_.end() &&
            state->second.federationName.has_value() &&
            *state->second.federationName == requestValue.federationName &&
            state->second.federateId == planned.providingFederateId) {
          providingSession = found->second;
        }
      }
    }
    if (providingSession == nullptr) {
      continue;
    }

    // Each provider group may own only a subset of the requested attributes;
    // persist the matching region pairs rather than the requester's full map.
    std::map<std::uint64_t, std::set<std::uint64_t>> providerRegions;
    for (std::uint64_t const attributeHandle :
         planned.requestedAttributeHandles) {
      auto const region = requestValue.requestRegionsByAttribute.find(
          attributeHandle);
      if (region == requestValue.requestRegionsByAttribute.end()) {
        return internalError(request);
      }
      providerRegions.emplace(attributeHandle, region->second);
    }
    auto const requestId = registry_.registerAttributeValueUpdateRegionalRequest(
        requestValue.federationName,
        requestValue.requestingFederateId,
        planned.providingFederateId,
        planned.objectInstanceHandle,
        requestValue.objectClassHandle,
        planned.requestedAttributeHandles,
        providerRegions,
        requestValue.userSuppliedTag);
    if (!requestId) {
      return internalError(request);
    }
    auto const projection =
        registry_.beginAttributeValueUpdateRegionalProvideRecipientFor(
            requestValue.federationName,
            *requestId,
            requestValue.requestingFederateId,
            planned.providingFederateId,
            planned.objectInstanceHandle,
            requestValue.objectClassHandle,
            planned.requestedAttributeHandles,
            providerRegions);
    if (!projection) {
      continue;
    }
    ProcessFederationAttributeValueUpdateRequestEvent event{
        requestValue.requestingFederateId,
        planned.providingFederateId,
        projection->objectInstanceHandle,
        projection->requestedAttributeHandles,
        requestValue.userSuppliedTag};
    if (options_.pushReceiveOrderEvents) {
      pushedEvents.emplace_back(providingSession, std::move(event));
    } else {
      std::scoped_lock lock(mutex_);
      auto const state = sessions_.find(providingSession);
      if (state != sessions_.end() &&
          state->second.federationName.has_value() &&
          *state->second.federationName == requestValue.federationName &&
          state->second.federateId == planned.providingFederateId) {
        state->second.attributeValueUpdateRequestEvents.push_back(
            std::move(event));
      } else {
        continue;
      }
    }
    if (recipientCount != std::numeric_limits<std::uint32_t>::max()) {
      ++recipientCount;
    }
  }

  if (options_.pushReceiveOrderEvents) {
    for (auto& pushedEvent : pushedEvents) {
      ProcessFederationReceiveInteractionResult result;
      result.attributeValueUpdateRequestEvent = std::move(pushedEvent.second);
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
      encodeProcessFederationRequestAttributeValueUpdateResult(
          ProcessFederationRequestAttributeValueUpdateResult{recipientCount}));
}
}  // namespace umbra::detail
