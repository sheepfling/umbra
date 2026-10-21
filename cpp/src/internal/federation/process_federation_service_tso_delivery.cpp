#include "internal/federation/process_federation_service.hpp"
#include "internal/federation/process_federation_service_helpers.hpp"
#include "internal/federation/process_federation_service_payload_helpers.hpp"

#include <algorithm>
#include <cstdint>
#include <stdexcept>
#include <utility>
#include <vector>

namespace umbra::detail {

using process_federation_payload::parameterVector;

std::optional<ProcessFederationAttributeUpdateEvent>
projectProcessTsoAttributeUpdatePasselToCallback(
    EmbeddedFederationRegistry& registry,
    std::wstring const& federationName,
    std::uint64_t receivingFederateId,
    TsoAttributeUpdateMessage const& message,
    TsoAttributeUpdatePassel const& passel,
    ReceiveOrderAttributeUpdateRecipient const& projection,
    ProcessTsoCallbackRegionDesignatorCopyLease& callbackRegionCopies,
    bool asReceiveOrder) {
  if (!asReceiveOrder && !message.timestamp) {
    throw std::runtime_error(
        "The process federation encountered a timestamped attribute update without a timestamp.");
  }

  ProcessFederationAttributeUpdateEvent event;
  event.producingFederateId = message.producingFederateId;
  event.receivingFederateId = receivingFederateId;
  event.objectInstanceHandle = message.objectInstanceHandle;
  event.transportationName = passel.transportationName;
  event.defaultRegionUsed = passel.defaultRegionUsed;
  if (projection.conveyRegionDesignatorSets &&
      (!passel.sentRegionHandles.empty() || passel.defaultRegionUsed)) {
    if (passel.sentRegionHandles.empty()) {
      event.sentRegionHandles = passel.sentRegionHandles;
    } else {
      CallbackRegionDesignatorCopyMap copies;
      auto const copyStatus = registry.createCallbackScopedRegionDesignatorCopies(
          federationName,
          receivingFederateId,
          passel.sentRegionHandles,
          passel.sentRegionSnapshots,
          copies);
      if (copyStatus != RegionServiceStatus::applied) {
        throw std::runtime_error(
            "The process federation could not create timestamped callback region designator copies.");
      }
      auto const& callbackCopies = callbackRegionCopies.add(std::move(copies));
      std::set<std::uint64_t> conveyedCopyHandles;
      for (auto const& [sourceRegionHandle, copiedRegionHandle] :
           callbackCopies) {
        static_cast<void>(sourceRegionHandle);
        conveyedCopyHandles.insert(copiedRegionHandle);
      }
      event.sentRegionHandles = std::move(conveyedCopyHandles);
    }
  }
  if (!asReceiveOrder) {
    event.timestamp = encodeProcessLogicalTime(message.timestamp);
  }
  event.retractionMessageId = message.messageId;
  if (message.userSuppliedTag.size() != 0U) {
    auto const* data = static_cast<std::uint8_t const*>(
        message.userSuppliedTag.data());
    if (data == nullptr) {
      throw std::runtime_error(
          "The process federation could not copy a timestamped attribute tag.");
    }
    event.userSuppliedTag.assign(
        data, data + message.userSuppliedTag.size());
  }

  for (auto const attributeHandle : passel.sentAttributeHandles) {
    if (!projection.receivedAttributeHandles.contains(attributeHandle)) {
      continue;
    }
    auto const value = std::find_if(
        message.attributes.begin(),
        message.attributes.end(),
        [attributeHandle](TsoAttributeValue const& candidate) {
          return candidate.first == attributeHandle;
        });
    if (value == message.attributes.end()) {
      continue;
    }
    std::vector<std::uint8_t> bytes(value->second.size());
    if (!bytes.empty()) {
      auto const* data = static_cast<std::uint8_t const*>(value->second.data());
      if (data == nullptr) {
        throw std::runtime_error(
            "The process federation could not copy a timestamped attribute value.");
      }
      std::copy(data, data + bytes.size(), bytes.begin());
    }
    event.attributeValues.emplace_back(attributeHandle, std::move(bytes));
  }
  if (event.attributeValues.empty()) {
    return std::nullopt;
  }
  return event;
}

void ProcessFederationService::dispatchTsoAttributeUpdateDelivery(
    std::wstring const &federationName,
    std::uint64_t receivingFederateId,
    TsoAttributeUpdateDelivery const &attribute,
    bool asReceiveOrder) {
  auto const &message = attribute.message;
  if (!asReceiveOrder && !message.timestamp) {
    throw std::runtime_error(
        "The process federation encountered a timestamped attribute update without a timestamp.");
  }
  ProcessTransportSession *receivingSession = nullptr;
  {
    std::scoped_lock lock(mutex_);
    auto const session = sessionsByFederateId_.find(receivingFederateId);
    if (session != sessionsByFederateId_.end()) {
      auto const state = sessions_.find(session->second);
      if (state != sessions_.end() && state->second.federationName.has_value() &&
          *state->second.federationName == federationName &&
          state->second.federateId == receivingFederateId) {
        receivingSession = session->second;
      }
    }
  }

  auto completeDelivery = [&] {
    auto const completed = registry_.completeTsoDelivery(
        federationName,
        attribute.queuedMessage);
    if (completed.status != FederationTsoRegistryStatus::applied ||
        (completed.delivery.status != FederationTsoDeliveryStatus::applied &&
         completed.delivery.status !=
             FederationTsoDeliveryStatus::message_already_completed)) {
      throw std::runtime_error(
          "The process federation could not complete timestamped attribute delivery.");
    }
  };

  if (receivingSession == nullptr) {
    static_cast<void>(registry_.finishTsoRecipientCallbackSuppressed(
        federationName,
        receivingFederateId,
        message.messageId));
    completeDelivery();
    return;
  }

  std::vector<ProcessFederationAttributeUpdateEvent> events;
  ProcessTsoCallbackRegionDesignatorCopyLease callbackRegionCopies(
      registry_, federationName, receivingFederateId);
  auto const passels = message.passelsByRecipient.find(receivingFederateId);
  if (passels != message.passelsByRecipient.end()) {
    events.reserve(passels->second.size());
    callbackRegionCopies.reserve(passels->second.size());
    for (auto const &passel : passels->second) {
      auto projection = registry_.receiveOrderAttributeUpdateRecipientFor(
          federationName,
          message.producingFederateId,
          receivingFederateId,
          message.objectInstanceHandle,
          passel.sentAttributeHandles,
          passel.sentRegionHandles.empty() ? nullptr : &passel.sentRegionHandles,
          passel.sentRegionSnapshots.empty()
              ? nullptr
              : &passel.sentRegionSnapshots);
      if (!projection) {
        continue;
      }
      auto event = projectProcessTsoAttributeUpdatePasselToCallback(
          registry_,
          federationName,
          receivingFederateId,
          message,
          passel,
          *projection,
          callbackRegionCopies,
          asReceiveOrder);
      if (event) {
        events.push_back(std::move(*event));
      }
    }
  }

  for (std::size_t index = 0U; index < events.size(); ++index) {
    events[index].tsoCallbackGroupPosition =
        index + 1U == events.size()
            ? ProcessFederationTsoAttributeUpdateCallbackGroupPosition::final
            : ProcessFederationTsoAttributeUpdateCallbackGroupPosition::more;
  }

  if (events.empty() ||
      !registry_.beginTsoAttributeUpdateCallback(
          federationName,
          receivingFederateId,
          message.messageId,
          asReceiveOrder)) {
    static_cast<void>(registry_.finishTsoRecipientCallbackSuppressed(
        federationName,
        receivingFederateId,
        message.messageId));
    completeDelivery();
    return;
  }

  if (!callbackRegionCopies.empty()) {
    std::scoped_lock lock(mutex_);
    auto const state = sessions_.find(receivingSession);
    if (state == sessions_.end() ||
        !state->second.federationName.has_value() ||
        *state->second.federationName != federationName ||
        state->second.federateId != receivingFederateId) {
      throw std::runtime_error(
          "The process federation lost the timestamped attribute recipient before retaining its callback region copies.");
    }
    auto const [pending, inserted] = state->second
                                         .pendingTsoAttributeCallbackRegionDesignatorCopies
                                         .try_emplace(
                                             message.messageId,
                                             std::move(callbackRegionCopies));
    static_cast<void>(pending);
    if (!inserted) {
      throw std::runtime_error(
          "The process federation already retains timestamped callback region copies for this message.");
    }
  }

  for (auto &event : events) {
    if (!receivingSession->send(TransportServiceMessage{
            TransportServiceMessageKind::event,
            TransportServiceOperation::receive_attribute_update,
            TransportServiceStatus::ok,
            0U,
            encodeProcessFederationReceiveAttributeUpdateResult(
                ProcessFederationReceiveAttributeUpdateResult{event})})) {
      throw std::runtime_error(
          "The process federation could not deliver a timestamped attribute event.");
    }
  }
  {
    std::scoped_lock lock(mutex_);
    pendingPushedRetractionRecipients_[message.messageId].push_back(
        receivingSession);
  }
  // Keep the coordinator entry in-transit until the process receiver has
  // crossed its callback boundary and sends acknowledge_tso_delivery.
}

void ProcessFederationService::dispatchTsoInteractionDelivery(
    std::wstring const &federationName,
    std::uint64_t receivingFederateId,
    TsoInteractionDelivery const &interaction,
    bool asReceiveOrder) {
  auto const &message = interaction.message;
  if (!asReceiveOrder && !message.timestamp) {
    throw std::runtime_error(
        "The process federation encountered a timestamped interaction without a timestamp.");
  }
  ProcessTransportSession *receivingSession = nullptr;
  {
    std::scoped_lock lock(mutex_);
    auto const session = sessionsByFederateId_.find(receivingFederateId);
    if (session != sessionsByFederateId_.end()) {
      auto const state = sessions_.find(session->second);
      if (state != sessions_.end() && state->second.federationName.has_value() &&
          *state->second.federationName == federationName &&
          state->second.federateId == receivingFederateId) {
        receivingSession = session->second;
      }
    }
  }

  auto completeDelivery = [&] {
    auto const completed = registry_.completeTsoDelivery(
        federationName,
        interaction.queuedMessage);
    if (completed.status != FederationTsoRegistryStatus::applied ||
        (completed.delivery.status != FederationTsoDeliveryStatus::applied &&
         completed.delivery.status !=
             FederationTsoDeliveryStatus::message_already_completed)) {
      throw std::runtime_error(
          "The process federation could not complete timestamped interaction delivery.");
    }
  };

  if (receivingSession == nullptr) {
    static_cast<void>(registry_.finishTsoRecipientCallbackSuppressed(
        federationName,
        receivingFederateId,
        message.messageId));
    completeDelivery();
    return;
  }

  auto projection = registry_.receiveOrderInteractionRecipientFor(
      federationName,
      message.producingFederateId,
      receivingFederateId,
      message.sentInteractionClassHandle,
      message.sentParameterHandles,
      message.sentRegionHandles.empty() ? nullptr : &message.sentRegionHandles,
      message.sentRegionSnapshots.empty() ? nullptr : &message.sentRegionSnapshots);
  if (!projection || !registry_.beginTsoInteractionCallback(
                         federationName,
                         receivingFederateId,
                         message.messageId,
                         asReceiveOrder)) {
    static_cast<void>(registry_.finishTsoRecipientCallbackSuppressed(
        federationName,
        receivingFederateId,
        message.messageId));
    completeDelivery();
    return;
  }

  ProcessFederationInteractionEnvelope envelope;
  envelope.userSuppliedTag.resize(message.userSuppliedTag.size());
  if (!envelope.userSuppliedTag.empty()) {
    auto const *data = static_cast<std::uint8_t const *>(
        message.userSuppliedTag.data());
    if (data == nullptr) {
      throw std::runtime_error(
          "The process federation could not copy a timestamped interaction tag.");
    }
    std::copy(data,
              data + message.userSuppliedTag.size(),
              envelope.userSuppliedTag.begin());
  }
  envelope.parameterValues.reserve(message.parameters.size());
  for (auto const &[parameterHandle, parameterValue] : message.parameters) {
    std::vector<std::uint8_t> bytes(parameterValue.size());
    if (!bytes.empty()) {
      auto const *data = static_cast<std::uint8_t const *>(parameterValue.data());
      if (data == nullptr) {
        throw std::runtime_error(
            "The process federation could not copy a timestamped interaction parameter.");
      }
      std::copy(data, data + bytes.size(), bytes.begin());
    }
    envelope.parameterValues.emplace_back(parameterHandle, std::move(bytes));
  }

  std::optional<ProcessFederationLogicalTime> timestamp;
  if (!asReceiveOrder) {
    timestamp = encodeProcessLogicalTime(message.timestamp);
    if (!timestamp) {
      throw std::runtime_error(
          "The process federation could not encode a timestamped interaction time.");
    }
  }
  ProcessFederationInteractionEvent event{
      message.producingFederateId,
      receivingFederateId,
      projection->receivedInteractionClassHandle,
      parameterVector(projection->receivedParameterHandles),
      encodeProcessFederationInteractionEnvelope(envelope),
      message.transportationName,
      std::move(timestamp),
      std::nullopt,
      message.messageId};
  if (!asReceiveOrder) {
    event.sentOrderType = rti1516_2025::TIMESTAMP;
    event.receivedOrderType = rti1516_2025::TIMESTAMP;
  }
  event.defaultRegionUsed =
      projection->conveyRegionDesignatorSets && message.defaultRegionUsed;
  if (projection->conveyRegionDesignatorSets &&
      (!message.sentRegionHandles.empty() || message.defaultRegionUsed)) {
    event.sentRegionHandles = message.sentRegionHandles;
  }

  // TSO payloads must cross the process event boundary before the matching
  // grant event, even for the compatibility polling service option.  The client
  // accepts this unsolicited interaction frame and orders it ahead of the
  // queued grant callback; the service option continues to govern ordinary
  // receive-order traffic only.
  if (!receivingSession->send(TransportServiceMessage{
          TransportServiceMessageKind::event,
          TransportServiceOperation::receive_interaction,
          TransportServiceStatus::ok,
          0U,
          encodeProcessFederationReceiveInteractionResult(
              ProcessFederationReceiveInteractionResult{event})})) {
    throw std::runtime_error(
        "The process federation could not deliver a timestamped interaction event.");
  }
  {
    std::scoped_lock lock(mutex_);
    pendingPushedRetractionRecipients_[message.messageId].push_back(
        receivingSession);
  }
  // Keep the coordinator entry in-transit until the process receiver has
  // crossed its callback boundary and sends acknowledge_tso_delivery.
}

}  // namespace umbra::detail
