#include "internal/federation/process_federation_service.hpp"
#include "internal/federation/process_federation_service_helpers.hpp"

#include <algorithm>
#include <cstdint>
#include <limits>
#include <memory>
#include <utility>
#include <vector>

namespace umbra::detail {

TransportServiceMessage ProcessFederationService::handleUpdateAttributeValues(
    ProcessTransportSession& session,
    TransportServiceMessage const& request) {
  auto const updateRequest =
      decodeProcessFederationUpdateAttributeValuesRequest(request.payload);
  if (updateRequest.timestamp) {
    validateProcessLogicalTime(
        *updateRequest.timestamp,
        federationDefinition_->logicalTimeImplementationName);
  }
  std::shared_ptr<FederateTimeState> producingTimeState;
  {
    std::scoped_lock lock(mutex_);
    auto const state = sessions_.find(&session);
    if (state == sessions_.end() ||
        !state->second.federationName.has_value() ||
        *state->second.federationName != updateRequest.federationName ||
        state->second.federateId != updateRequest.producingFederateId ||
        !registry_.memberById(
            updateRequest.federationName, updateRequest.producingFederateId)) {
      return rejected(request);
    }
    producingTimeState = state->second.timeState;
  }

  std::vector<std::uint64_t> sentAttributeHandles;
  sentAttributeHandles.reserve(updateRequest.attributeValues.size());
  for (auto const& [attributeHandle, value] : updateRequest.attributeValues) {
    static_cast<void>(value);
    sentAttributeHandles.push_back(attributeHandle);
  }
  auto const plan = registry_.planReceiveOrderAttributeUpdate(
      updateRequest.federationName,
      updateRequest.producingFederateId,
      updateRequest.objectInstanceHandle,
      sentAttributeHandles);
  if (plan.status != ReceiveOrderAttributeUpdateStatus::applied) {
    return rejected(request);
  }

  auto const producingTimeSnapshot = producingTimeState
      ? producingTimeState->snapshot()
      : FederateTimeSnapshot{};
  bool const queueTimestampedAttributeUpdate =
      updateRequest.timestamp && producingTimeSnapshot.timeRegulating &&
      std::any_of(
          plan.passels.begin(),
          plan.passels.end(),
          [](ReceiveOrderAttributeUpdatePassel const& passel) {
            return passel.preferredOrderType == rti1516_2025::TIMESTAMP;
          });
  std::set<std::uint64_t> timeConstrainedRecipients;
  if (queueTimestampedAttributeUpdate) {
    auto const execution = registry_.timeSnapshotFor(updateRequest.federationName);
    if (!execution) {
      return internalError(request);
    }
    for (auto const& federate : execution->federates) {
      if (federate.time.timeConstrained) {
        timeConstrainedRecipients.insert(federate.membership.id);
      }
    }
  }

  std::uint64_t messageId = 0U;
  if (queueTimestampedAttributeUpdate) {
    TsoAttributeUpdateMessage message;
    message.producingFederateId = updateRequest.producingFederateId;
    message.objectInstanceHandle = updateRequest.objectInstanceHandle;
    message.userSuppliedTag.setData(
        updateRequest.userSuppliedTag.data(),
        updateRequest.userSuppliedTag.size());
    message.timestamp = decodeProcessLogicalTime(
        *updateRequest.timestamp,
        federationDefinition_->logicalTimeImplementationName);
    message.attributes.reserve(updateRequest.attributeValues.size());
    for (auto const& [attributeHandle, value] : updateRequest.attributeValues) {
      rti1516_2025::VariableLengthData encodedValue;
      if (!value.empty()) {
        encodedValue.setData(value.data(), value.size());
      }
      message.attributes.emplace_back(attributeHandle, std::move(encodedValue));
    }

    auto const snapshotsEqual = [](
        RegionSpecificationSnapshot const& left,
        RegionSpecificationSnapshot const& right) {
      if (left.dimensionHandles != right.dimensionHandles ||
          left.specificationCommitted != right.specificationCommitted ||
          left.committedRangeBounds.size() != right.committedRangeBounds.size()) {
        return false;
      }
      for (auto const& [dimensionHandle, leftBounds] : left.committedRangeBounds) {
        auto const rightBounds = right.committedRangeBounds.find(dimensionHandle);
        if (rightBounds == right.committedRangeBounds.end() ||
            leftBounds.lowerBound != rightBounds->second.lowerBound ||
            leftBounds.upperBound != rightBounds->second.upperBound) {
          return false;
        }
      }
      return true;
    };
    std::vector<std::uint64_t> queuedRecipients;
    std::vector<std::uint64_t> allTimestampedRecipients;
    for (auto const& passel : plan.passels) {
      if (passel.preferredOrderType != rti1516_2025::TIMESTAMP) {
        continue;
      }
      TsoAttributeUpdatePassel payloadPassel;
      payloadPassel.transportationName = passel.transportationName;
      payloadPassel.sentAttributeHandles = passel.sentAttributeHandles;
      payloadPassel.sentRegionHandles = passel.sentRegionHandles;
      payloadPassel.sentRegionSnapshots = passel.sentRegionSnapshots;
      payloadPassel.defaultRegionUsed = passel.defaultRegionUsed;
      payloadPassel.preferredOrderType = passel.preferredOrderType;
      for (auto const& plannedRecipient : passel.recipients) {
        allTimestampedRecipients.push_back(plannedRecipient.federateId);
        message.passelsByRecipient[plannedRecipient.federateId].push_back(
            payloadPassel);
        if (timeConstrainedRecipients.contains(plannedRecipient.federateId)) {
          queuedRecipients.push_back(plannedRecipient.federateId);
        }
        for (auto const& [regionHandle, snapshot] : passel.sentRegionSnapshots) {
          auto const [position, inserted] =
              message.sentRegionSnapshots.emplace(regionHandle, snapshot);
          if (!inserted && !snapshotsEqual(position->second, snapshot)) {
            return internalError(request);
          }
        }
      }
    }

    auto const result = registry_.enqueueTsoAttributeUpdate(
        updateRequest.federationName,
        std::move(message),
        queuedRecipients,
        allTimestampedRecipients);
    if (result.status != FederationTsoRegistryStatus::applied ||
        result.queueStatus != TsoMessageQueueStatus::applied ||
        result.messageId == 0U) {
      return rejected(request);
    }
    messageId = result.messageId;
    std::scoped_lock lock(mutex_);
    processTsoMessageProducers_.emplace(
        messageId,
        updateRequest.producingFederateId);
  }

  std::vector<std::pair<
      std::uint64_t,
      rti1516_2025::VariableLengthData>> acceptedAttributeValues;
  acceptedAttributeValues.reserve(updateRequest.attributeValues.size());
  for (auto const& [attributeHandle, value] : updateRequest.attributeValues) {
    rti1516_2025::VariableLengthData encodedValue;
    if (!value.empty()) {
      encodedValue.setData(value.data(), value.size());
    }
    acceptedAttributeValues.emplace_back(attributeHandle, std::move(encodedValue));
  }
  std::set<std::string> transportationNames;
  for (auto const& passel : plan.passels) {
    transportationNames.insert(passel.transportationName);
  }
  if (registry_.recordSuccessfulUpdateAttributeValues(
          updateRequest.federationName,
          updateRequest.producingFederateId,
          updateRequest.objectInstanceHandle,
          plan.registeredObjectClassHandle,
          transportationNames,
          queueTimestampedAttributeUpdate ? nullptr : &acceptedAttributeValues) !=
      FederationRegistryStatus::applied) {
    return internalError(request);
  }

  std::vector<ProcessFederationAttributeUpdateEvent> events;
  std::uint32_t recipientCount = 0U;
  std::uint32_t queuedRecipientCount = 0U;
  for (auto const& passel : plan.passels) {
    for (auto const& plannedRecipient : passel.recipients) {
      if (queueTimestampedAttributeUpdate &&
          passel.preferredOrderType == rti1516_2025::TIMESTAMP &&
          timeConstrainedRecipients.contains(plannedRecipient.federateId)) {
        if (queuedRecipientCount != std::numeric_limits<std::uint32_t>::max()) {
          ++queuedRecipientCount;
        }
        continue;
      }
      auto recipient = plannedRecipient;
      if (recipient.receivedAttributeHandles.empty()) {
        auto const current = registry_.receiveOrderAttributeUpdateRecipientFor(
            updateRequest.federationName,
            updateRequest.producingFederateId,
            plannedRecipient.federateId,
            updateRequest.objectInstanceHandle,
            passel.sentAttributeHandles);
        if (!current) {
          continue;
        }
        recipient = *current;
      }

      ProcessFederationAttributeUpdateEvent event;
      event.producingFederateId = updateRequest.producingFederateId;
      event.receivingFederateId = recipient.federateId;
      event.objectInstanceHandle = updateRequest.objectInstanceHandle;
      event.userSuppliedTag = updateRequest.userSuppliedTag;
      event.transportationName = passel.transportationName;
      event.sentRegionHandles = passel.sentRegionHandles.empty()
          ? std::nullopt
          : std::optional<std::set<std::uint64_t>>(passel.sentRegionHandles);
      event.defaultRegionUsed = passel.defaultRegionUsed;
      event.timestamp = updateRequest.timestamp;
      if (queueTimestampedAttributeUpdate &&
          passel.preferredOrderType == rti1516_2025::TIMESTAMP) {
        event.retractionMessageId = messageId;
      }
      for (auto const attributeHandle : passel.sentAttributeHandles) {
        if (!recipient.receivedAttributeHandles.contains(attributeHandle)) {
          continue;
        }
        auto const value = std::find_if(
            updateRequest.attributeValues.begin(),
            updateRequest.attributeValues.end(),
            [attributeHandle](ProcessFederationAttributeValue const& candidate) {
              return candidate.first == attributeHandle;
            });
        if (value != updateRequest.attributeValues.end()) {
          event.attributeValues.push_back(*value);
        }
      }
      if (event.attributeValues.empty()) {
        continue;
      }
      events.push_back(std::move(event));
    }
  }

  if (queueTimestampedAttributeUpdate && options_.pushReceiveOrderEvents) {
    events.erase(
        std::remove_if(
            events.begin(),
            events.end(),
            [&](ProcessFederationAttributeUpdateEvent const& event) {
              return event.retractionMessageId &&
                  !registry_.beginTsoAttributeUpdateCallback(
                      updateRequest.federationName,
                      event.receivingFederateId,
                      *event.retractionMessageId);
            }),
        events.end());
  }

  std::vector<std::pair<ProcessTransportSession*, ProcessFederationAttributeUpdateEvent>>
      pushedEvents;
  {
    std::scoped_lock lock(mutex_);
    for (auto& event : events) {
        auto const receivingSession =
            sessionsByFederateId_.find(event.receivingFederateId);
        if (receivingSession == sessionsByFederateId_.end()) {
          continue;
        }
        auto const receivingState = sessions_.find(receivingSession->second);
        if (receivingState == sessions_.end()) {
          continue;
        }
        auto* const receivingSessionPointer = receivingSession->second;
        bool const hasRetraction = event.retractionMessageId.has_value();
      if (options_.pushReceiveOrderEvents) {
        pushedEvents.emplace_back(receivingSession->second, std::move(event));
      } else {
        receivingState->second.attributeUpdateEvents.push_back(std::move(event));
      }
      if (hasRetraction) {
        // The push frame crosses the process callback boundary below. Pull
        // mode records the same recipient now so a producer Retract can
        // suppress a queued event before the receiver polls it.
        pendingPushedRetractionRecipients_[messageId].push_back(
            receivingSessionPointer);
      }
      if (recipientCount != std::numeric_limits<std::uint32_t>::max()) {
        ++recipientCount;
      }
    }
  }

  if (options_.pushReceiveOrderEvents) {
    for (auto const& pushedEvent : pushedEvents) {
      auto const eventMessage = TransportServiceMessage{
          TransportServiceMessageKind::event,
          TransportServiceOperation::receive_attribute_update,
          TransportServiceStatus::ok,
          0U,
          encodeProcessFederationReceiveAttributeUpdateResult(
              ProcessFederationReceiveAttributeUpdateResult{pushedEvent.second})};
      if (pushedEvent.first == nullptr || !pushedEvent.first->send(eventMessage)) {
        return internalError(request);
      }
    }
  }
  if (queueTimestampedAttributeUpdate) {
    auto const totalCount = static_cast<std::uint64_t>(recipientCount) +
        queuedRecipientCount;
    recipientCount = static_cast<std::uint32_t>(std::min<std::uint64_t>(
        totalCount,
        std::numeric_limits<std::uint32_t>::max()));
  }
  return responseFor(
      request,
      TransportServiceStatus::ok,
      encodeProcessFederationUpdateAttributeValuesResult(
          ProcessFederationUpdateAttributeValuesResult{recipientCount, messageId}));
}

}  // namespace umbra::detail
