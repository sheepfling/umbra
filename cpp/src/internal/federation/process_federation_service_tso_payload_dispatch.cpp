#include "internal/federation/process_federation_service.hpp"
#include "internal/federation/process_federation_service_helpers.hpp"
#include "internal/federation/process_federation_service_payload_helpers.hpp"

#include <cstdint>
#include <utility>

namespace umbra::detail {

using process_federation_payload::parameterVector;

void ProcessFederationService::dispatchTsoInteractionPayloads(
    std::wstring const& federationName,
    std::uint64_t receivingFederateId,
    rti1516_2025::LogicalTime const& boundary) {
  auto delivery = registry_.beginTsoPayloadDelivery(
      federationName,
      receivingFederateId,
      boundary,
      true);
  if (delivery.status != FederationTsoRegistryStatus::applied ||
      (delivery.deliveryStatus != FederationTsoDeliveryStatus::applied &&
       delivery.deliveryStatus != FederationTsoDeliveryStatus::no_messages)) {
    throw std::runtime_error(
        "The process federation could not begin timestamped interaction delivery.");
  }

  for (auto const& typedDelivery : delivery.deliveries) {
    if (auto const* attribute =
            std::get_if<TsoAttributeUpdateDelivery>(&typedDelivery)) {
      dispatchTsoAttributeUpdateDelivery(
          federationName,
          receivingFederateId,
          *attribute);
      continue;
    }

    if (auto const* directed =
            std::get_if<TsoDirectedInteractionDelivery>(&typedDelivery)) {
      auto const& message = directed->message;
      if (!message.timestamp) {
        throw std::runtime_error(
            "The process federation encountered a timestamped directed interaction without a timestamp.");
      }

      ProcessTransportSession* receivingSession = nullptr;
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
            directed->queuedMessage);
        if (completed.status != FederationTsoRegistryStatus::applied ||
            (completed.delivery.status != FederationTsoDeliveryStatus::applied &&
             completed.delivery.status !=
                 FederationTsoDeliveryStatus::message_already_completed)) {
          throw std::runtime_error(
              "The process federation could not complete timestamped directed-interaction delivery.");
        }
      };

      if (receivingSession == nullptr) {
        static_cast<void>(registry_.finishTsoRecipientCallbackSuppressed(
            federationName,
            receivingFederateId,
            message.messageId));
        completeDelivery();
        continue;
      }

      auto const projection = registry_.timestampedDirectedInteractionRecipientFor(
          federationName,
          message.producingFederateId,
          receivingFederateId,
          message.objectInstanceHandle,
          message.sentInteractionClassHandle,
          message.sentParameterHandles,
          message.messageId);
      if (!projection || !registry_.beginTsoInteractionCallback(
                             federationName,
                             receivingFederateId,
                             message.messageId)) {
        static_cast<void>(registry_.finishTsoRecipientCallbackSuppressed(
            federationName,
            receivingFederateId,
            message.messageId));
        completeDelivery();
        continue;
      }

      ProcessFederationInteractionEnvelope envelope;
      envelope.userSuppliedTag.resize(message.userSuppliedTag.size());
      if (!envelope.userSuppliedTag.empty()) {
        auto const* data = static_cast<std::uint8_t const*>(
            message.userSuppliedTag.data());
        if (data == nullptr) {
          throw std::runtime_error(
              "The process federation could not copy a timestamped directed-interaction tag.");
        }
        std::copy(
            data,
            data + message.userSuppliedTag.size(),
            envelope.userSuppliedTag.begin());
      }
      envelope.parameterValues.reserve(message.parameters.size());
      for (auto const& [parameterHandle, parameterValue] : message.parameters) {
        std::vector<std::uint8_t> bytes(parameterValue.size());
        if (!bytes.empty()) {
          auto const* data = static_cast<std::uint8_t const*>(
              parameterValue.data());
          if (data == nullptr) {
            throw std::runtime_error(
                "The process federation could not copy a timestamped directed-interaction parameter.");
          }
          std::copy(data, data + bytes.size(), bytes.begin());
        }
        envelope.parameterValues.emplace_back(parameterHandle, std::move(bytes));
      }

      auto const timestamp = encodeProcessLogicalTime(message.timestamp);
      if (!timestamp) {
        throw std::runtime_error(
            "The process federation could not encode a timestamped directed-interaction time.");
      }
      ProcessFederationInteractionEvent event{
          message.producingFederateId,
          receivingFederateId,
          projection->receivedInteractionClassHandle,
          parameterVector(projection->receivedParameterHandles),
          encodeProcessFederationInteractionEnvelope(envelope),
          message.transportationName,
          std::move(timestamp),
          projection->objectInstanceHandle,
          message.messageId};
      event.sentOrderType = rti1516_2025::TIMESTAMP;
      event.receivedOrderType = rti1516_2025::TIMESTAMP;
      if (!receivingSession->send(TransportServiceMessage{
              TransportServiceMessageKind::event,
              TransportServiceOperation::receive_interaction,
              TransportServiceStatus::ok,
              0U,
              encodeProcessFederationReceiveInteractionResult(
                  ProcessFederationReceiveInteractionResult{event})})) {
        throw std::runtime_error(
            "The process federation could not deliver a timestamped directed-interaction event.");
      }
      {
        std::scoped_lock lock(mutex_);
        pendingPushedRetractionRecipients_[message.messageId].push_back(
            receivingSession);
      }
      // Keep the coordinator entry in-transit until the process receiver has
      // crossed its callback boundary and sends acknowledge_tso_delivery.
      continue;
    }

    if (auto const* deletion =
            std::get_if<TsoObjectDeletionDelivery>(&typedDelivery)) {
      auto const& message = deletion->message;
      if (!message.timestamp) {
        static_cast<void>(registry_.finishTsoRecipientCallbackSuppressed(
            federationName,
            receivingFederateId,
            message.messageId));
        throw std::runtime_error(
            "The process federation encountered a timestamped object deletion without a timestamp.");
      }

      ProcessTransportSession* receivingSession = nullptr;
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
            deletion->queuedMessage);
        if (completed.status != FederationTsoRegistryStatus::applied ||
            (completed.delivery.status != FederationTsoDeliveryStatus::applied &&
             completed.delivery.status !=
                 FederationTsoDeliveryStatus::message_already_completed)) {
          throw std::runtime_error(
              "The process federation could not complete timestamped object-deletion delivery.");
        }
      };

      if (receivingSession == nullptr) {
        static_cast<void>(registry_.finishTsoRecipientCallbackSuppressed(
            federationName,
            receivingFederateId,
            message.messageId));
        completeDelivery();
        continue;
      }

      auto const target = std::find_if(
          message.recipients.begin(),
          message.recipients.end(),
          [receivingFederateId](TsoObjectDeletionRecipient const& candidate) {
            return candidate.receivingFederateId == receivingFederateId;
          });
      if (target == message.recipients.end()) {
        static_cast<void>(registry_.finishTsoRecipientCallbackSuppressed(
            federationName,
            receivingFederateId,
            message.messageId));
        completeDelivery();
        continue;
      }

      std::optional<RemovedObjectInstanceSnapshot> removal;
      {
        std::scoped_lock lock(mutex_);
        removal = registry_.beginTsoObjectInstanceRemoval(
            federationName,
            receivingFederateId,
            message.objectInstanceHandle,
            message.messageId);
      }
      if (!removal) {
        static_cast<void>(registry_.finishTsoRecipientCallbackSuppressed(
            federationName,
            receivingFederateId,
            message.messageId));
        completeDelivery();
        continue;
      }

      ProcessFederationObjectInstanceRemovalEvent event{
          receivingFederateId,
          removal->objectInstanceHandle,
          removal->producingFederateId,
          {},
          encodeProcessLogicalTime(message.timestamp),
          message.messageId,
          true,
          message.sentOrderType,
          rti1516_2025::TIMESTAMP};
      event.userSuppliedTag.resize(message.userSuppliedTag.size());
      if (!event.userSuppliedTag.empty()) {
        auto const* data = static_cast<std::uint8_t const*>(
            message.userSuppliedTag.data());
        if (data == nullptr) {
          throw std::runtime_error(
              "The process federation could not copy a timestamped deletion tag.");
        }
        std::copy(
            data,
            data + message.userSuppliedTag.size(),
            event.userSuppliedTag.begin());
      }
      if (!event.timestamp) {
        throw std::runtime_error(
            "The process federation could not encode a timestamped object-deletion time.");
      }

      ProcessFederationReceiveInteractionResult result;
      result.removalEvent = std::move(event);
      if (!receivingSession->send(TransportServiceMessage{
              TransportServiceMessageKind::event,
              TransportServiceOperation::receive_interaction,
              TransportServiceStatus::ok,
              0U,
              encodeProcessFederationReceiveInteractionResult(
                  std::move(result))})) {
        throw std::runtime_error(
            "The process federation could not deliver a timestamped object-deletion event.");
      }
      {
        std::scoped_lock lock(mutex_);
        pendingPushedRetractionRecipients_[message.messageId].push_back(
            receivingSession);
      }
      // Keep the coordinator entry in-transit until the process receiver has
      // crossed its callback boundary and sends acknowledge_tso_delivery.
      continue;
    }

    auto const* interaction = std::get_if<TsoInteractionDelivery>(&typedDelivery);
    if (interaction == nullptr) {
      // The process profile currently reconstructs regular and directed
      // timestamped interactions. Timestamped object/removal payloads retain
      // their existing bounded immediate process paths; if a future slice
      // admits one here, fail loudly instead of manufacturing a callback with
      // the wrong event shape.
      throw std::runtime_error(
          "The process federation encountered an unsupported timestamped payload.");
    }

    dispatchTsoInteractionDelivery(
        federationName,
        receivingFederateId,
        *interaction);
    continue;
  }
}

}  // namespace umbra::detail
