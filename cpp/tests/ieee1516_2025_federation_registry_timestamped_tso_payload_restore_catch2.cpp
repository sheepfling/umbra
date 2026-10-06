#include "ieee1516_2025_federation_registry_test_support.hpp"

TEST_CASE(
    "Federation restore rehydrates timestamped application payloads from the durable image",
    "[unit][kernel][federation-registry][save-restore][durable-save][restore][tso-payload-state][tso-interaction-state][tso-directed-interaction-state][tso-attribute-update-state]") {
  auto store = std::make_shared<umbra::detail::MemoryFederationSaveCommitStore>();
  EmbeddedFederationRegistry registry({}, store);
  REQUIRE(registry.create(L"exercise", validDefinition()).status ==
      FederationRegistryStatus::applied);

  auto receiverTime = std::make_shared<umbra::detail::FederateTimeState>(
      L"HLAinteger64Time",
      std::make_shared<rti1516_2025::HLAinteger64Time>());
  auto producer = registry.join(
      L"exercise", L"producer", L"producer", noOpCallbackRoute());
  auto receiver = registry.joinWithTimeState(
      L"exercise",
      receiverTime,
      L"receiver",
      L"receiver",
      noOpCallbackRoute());
  REQUIRE(producer.membership);
  REQUIRE(receiver.membership);

  umbra::detail::TsoInteractionMessage interaction;
  interaction.producingFederateId = producer.membership->id;
  interaction.sentInteractionClassHandle = 1U;
  interaction.sentParameterHandles = {2U};
  interaction.parameters = {{
      2U,
      rti1516_2025::VariableLengthData(
          std::string{"\x10\x11", 2U}.data(),
          2U),
  }};
  interaction.userSuppliedTag = rti1516_2025::VariableLengthData(
      std::string{"ordinary", 8U}.data(),
      8U);
  interaction.transportationName = "HLAreliable";
  interaction.timestamp =
      std::make_shared<rti1516_2025::HLAinteger64Time>(7);
  auto const ordinary = registry.enqueueTsoInteraction(
      L"exercise",
      std::move(interaction),
      {receiver.membership->id},
      {receiver.membership->id});
  REQUIRE(ordinary.status == umbra::detail::FederationTsoRegistryStatus::applied);

  umbra::detail::TsoDirectedInteractionMessage directed;
  directed.producingFederateId = producer.membership->id;
  directed.objectInstanceHandle = 91U;
  directed.sentInteractionClassHandle = 3U;
  directed.sentParameterHandles = {4U};
  directed.parameters = {{
      4U,
      rti1516_2025::VariableLengthData(
          std::string{"\x20\x21", 2U}.data(),
          2U),
  }};
  directed.userSuppliedTag = rti1516_2025::VariableLengthData(
      std::string{"directed", 8U}.data(),
      8U);
  directed.transportationName = "HLAreliable";
  directed.recipients = {{
      receiver.membership->id,
      91U,
      3U,
      {4U},
      noOpCallbackRoute(),
  }};
  directed.timestamp =
      std::make_shared<rti1516_2025::HLAinteger64Time>(8);
  auto const directedResult = registry.enqueueTsoDirectedInteraction(
      L"exercise",
      std::move(directed),
      {receiver.membership->id});
  REQUIRE(directedResult.status ==
      umbra::detail::FederationTsoRegistryStatus::applied);

  umbra::detail::TsoAttributeUpdateMessage attributeUpdate;
  attributeUpdate.producingFederateId = producer.membership->id;
  attributeUpdate.objectInstanceHandle = 92U;
  attributeUpdate.attributes = {{
      5U,
      rti1516_2025::VariableLengthData(
          std::string{"\x30\x31", 2U}.data(),
          2U),
  }};
  attributeUpdate.userSuppliedTag = rti1516_2025::VariableLengthData(
      std::string{"attribute", 9U}.data(),
      9U);
  umbra::detail::TsoAttributeUpdatePassel passel;
  passel.transportationName = "HLAreliable";
  passel.sentAttributeHandles = {5U};
  passel.preferredOrderType = rti1516_2025::TIMESTAMP;
  attributeUpdate.passelsByRecipient.emplace(
      receiver.membership->id,
      std::vector<umbra::detail::TsoAttributeUpdatePassel>{passel});
  attributeUpdate.timestamp =
      std::make_shared<rti1516_2025::HLAinteger64Time>(9);
  auto const attributeResult = registry.enqueueTsoAttributeUpdate(
      L"exercise",
      std::move(attributeUpdate),
      {receiver.membership->id},
      {receiver.membership->id});
  REQUIRE(attributeResult.status ==
      umbra::detail::FederationTsoRegistryStatus::applied);

  REQUIRE(registry.requestFederationSave(
      L"exercise", receiver.membership->id, L"payload-restore").status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(registry.federateSaveBegun(
      L"exercise", receiver.membership->id).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(registry.federateSaveBegun(
      L"exercise", producer.membership->id).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE_FALSE(registry.federateSaveComplete(
      L"exercise", receiver.membership->id).saveCompletedSuccessfully);
  REQUIRE(registry.federateSaveComplete(
      L"exercise", producer.membership->id).saveCompletedSuccessfully);

  // Consume the live queue after the save. Restore must put all payloads back
  // from the durable image rather than retaining this post-save delivery state.
  auto delivered = registry.beginTsoPayloadDelivery(
      L"exercise",
      receiver.membership->id,
      rti1516_2025::HLAinteger64Time(9),
      true);
  REQUIRE(delivered.deliveryStatus ==
      umbra::detail::FederationTsoDeliveryStatus::applied);
  REQUIRE(delivered.deliveries.size() == 3U);
  for (auto const& payload : delivered.deliveries) {
    std::visit(
        [&](auto const& typed) {
          REQUIRE(registry.completeTsoDelivery(
              L"exercise", typed.queuedMessage).delivery.status ==
              umbra::detail::FederationTsoDeliveryStatus::applied);
        },
        payload);
  }

  REQUIRE(registry.requestFederationRestore(
      L"exercise", receiver.membership->id, L"payload-restore").status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  REQUIRE(registry.federateRestoreComplete(
      L"exercise", receiver.membership->id).status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  REQUIRE(registry.federateRestoreComplete(
      L"exercise", producer.membership->id).status ==
      umbra::detail::FederationRestoreControlStatus::applied);

  auto restored = registry.beginTsoPayloadDelivery(
      L"exercise",
      receiver.membership->id,
      rti1516_2025::HLAinteger64Time(9),
      true);
  REQUIRE(restored.status == umbra::detail::FederationTsoRegistryStatus::applied);
  REQUIRE(restored.deliveryStatus ==
      umbra::detail::FederationTsoDeliveryStatus::applied);
  REQUIRE(restored.deliveries.size() == 3U);

  auto const interactionIt = std::find_if(
      restored.deliveries.begin(),
      restored.deliveries.end(),
      [](umbra::detail::TsoPayloadDelivery const& payload) {
        return std::holds_alternative<umbra::detail::TsoInteractionDelivery>(payload);
      });
  REQUIRE(interactionIt != restored.deliveries.end());
  auto const* restoredInteraction =
      std::get_if<umbra::detail::TsoInteractionDelivery>(&*interactionIt);
  REQUIRE(restoredInteraction != nullptr);
  REQUIRE(restoredInteraction->message.messageId == ordinary.messageId);
  REQUIRE(restoredInteraction->message.parameters.size() == 1U);
  REQUIRE(restoredInteraction->message.parameters.front().first == 2U);
  REQUIRE(restoredInteraction->message.parameters.front().second.size() == 2U);
  REQUIRE(std::string(
              static_cast<char const*>(
                  restoredInteraction->message.parameters.front().second.data()),
              2U) == std::string{"\x10\x11", 2U});
  REQUIRE(restoredInteraction->message.userSuppliedTag.size() == 8U);
  REQUIRE(restoredInteraction->message.timestamp->implementationName() ==
      L"HLAinteger64Time");

  auto const directedIt = std::find_if(
      restored.deliveries.begin(),
      restored.deliveries.end(),
      [](umbra::detail::TsoPayloadDelivery const& payload) {
        return std::holds_alternative<umbra::detail::TsoDirectedInteractionDelivery>(payload);
      });
  REQUIRE(directedIt != restored.deliveries.end());
  auto const* restoredDirected =
      std::get_if<umbra::detail::TsoDirectedInteractionDelivery>(&*directedIt);
  REQUIRE(restoredDirected != nullptr);
  REQUIRE(restoredDirected->message.messageId == directedResult.messageId);
  REQUIRE(restoredDirected->message.recipients.size() == 1U);
  REQUIRE(restoredDirected->message.recipients.front().receivingFederateId ==
      receiver.membership->id);
  REQUIRE(restoredDirected->message.recipients.front().callbackRoute);
  REQUIRE(restoredDirected->message.parameters.front().second.size() == 2U);
  REQUIRE(std::string(
              static_cast<char const*>(
                  restoredDirected->message.parameters.front().second.data()),
              2U) == std::string{"\x20\x21", 2U});

  auto const attributeIt = std::find_if(
      restored.deliveries.begin(),
      restored.deliveries.end(),
      [](umbra::detail::TsoPayloadDelivery const& payload) {
        return std::holds_alternative<umbra::detail::TsoAttributeUpdateDelivery>(payload);
      });
  REQUIRE(attributeIt != restored.deliveries.end());
  auto const* restoredAttribute =
      std::get_if<umbra::detail::TsoAttributeUpdateDelivery>(&*attributeIt);
  REQUIRE(restoredAttribute != nullptr);
  REQUIRE(restoredAttribute->message.messageId == attributeResult.messageId);
  REQUIRE(restoredAttribute->message.attributes.size() == 1U);
  REQUIRE(restoredAttribute->message.attributes.front().first == 5U);
  REQUIRE(restoredAttribute->message.attributes.front().second.size() == 2U);
  REQUIRE(std::string(
              static_cast<char const*>(
                  restoredAttribute->message.attributes.front().second.data()),
              2U) == std::string{"\x30\x31", 2U});
  REQUIRE(restoredAttribute->message.passelsByRecipient.size() == 1U);
  REQUIRE(restoredAttribute->message.passelsByRecipient.begin()->second.size() == 1U);
  REQUIRE(restoredAttribute->message.passelsByRecipient.begin()->second.front()
              .sentAttributeHandles == std::vector<std::uint64_t>{5U});

  for (auto const& payload : restored.deliveries) {
    std::visit(
        [&](auto const& typed) {
          REQUIRE(registry.completeTsoDelivery(
              L"exercise", typed.queuedMessage).delivery.status ==
              umbra::detail::FederationTsoDeliveryStatus::applied);
        },
        payload);
  }
}
