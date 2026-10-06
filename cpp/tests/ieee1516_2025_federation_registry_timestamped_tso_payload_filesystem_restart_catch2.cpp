#include "ieee1516_2025_federation_registry_test_support.hpp"

TEST_CASE(
    "Filesystem fresh-registry restore rehydrates one timestamped interaction payload across queued and in-transit recipients",
    "[unit][kernel][federation-registry][save-restore][durable-save][filesystem][restore]"
    "[process-restart][tso-queue-state][tso-payload-state][tso-interaction-state]") {
  auto const directory = temporarySaveCommitDirectory();
  std::error_code ignored;
  std::filesystem::remove_all(directory, ignored);
  auto store = std::make_shared<umbra::detail::FilesystemFederationSaveCommitStore>(
      directory);

  EmbeddedFederationRegistry source({}, store);
  REQUIRE(source.create(L"exercise", validDefinition()).status ==
      FederationRegistryStatus::applied);

  auto producer = source.join(
      L"exercise", L"producer", L"producer", noOpCallbackRoute());
  auto receiverATime = std::make_shared<umbra::detail::FederateTimeState>(
      L"HLAinteger64Time",
      std::make_shared<rti1516_2025::HLAinteger64Time>(0));
  auto receiverA = source.joinWithTimeState(
      L"exercise",
      receiverATime,
      L"receiver-a",
      L"receiver-a",
      noOpCallbackRoute());
  auto receiverBTime = std::make_shared<umbra::detail::FederateTimeState>(
      L"HLAinteger64Time",
      std::make_shared<rti1516_2025::HLAinteger64Time>(0));
  auto receiverB = source.joinWithTimeState(
      L"exercise",
      receiverBTime,
      L"receiver-b",
      L"receiver-b",
      noOpCallbackRoute());
  REQUIRE(producer.membership);
  REQUIRE(receiverA.membership);
  REQUIRE(receiverB.membership);

  std::string const payloadBytes{"\x40\x41", 2U};
  std::string const tagBytes{"filesystem", 10U};
  umbra::detail::TsoInteractionMessage interaction;
  interaction.producingFederateId = producer.membership->id;
  interaction.sentInteractionClassHandle = 1U;
  interaction.sentParameterHandles = {2U};
  interaction.parameters = {{
      2U,
      rti1516_2025::VariableLengthData(payloadBytes.data(), payloadBytes.size()),
  }};
  interaction.userSuppliedTag = rti1516_2025::VariableLengthData(
      tagBytes.data(), tagBytes.size());
  interaction.transportationName = "HLAreliable";
  interaction.timestamp =
      std::make_shared<rti1516_2025::HLAinteger64Time>(5);
  auto const enqueued = source.enqueueTsoInteraction(
      L"exercise",
      std::move(interaction),
      {receiverA.membership->id, receiverB.membership->id},
      {receiverA.membership->id, receiverB.membership->id});
  REQUIRE(enqueued.status == umbra::detail::FederationTsoRegistryStatus::applied);
  REQUIRE(enqueued.queueStatus == umbra::detail::TsoMessageQueueStatus::applied);
  REQUIRE(enqueued.enqueuedRecipientCount == 2U);

  // Begin only receiver A's delivery and hold it in transit. Receiver B's
  // queue entry remains queued at the same timestamp when the durable image is
  // captured.
  auto inTransit = source.beginTsoPayloadDelivery(
      L"exercise",
      receiverA.membership->id,
      rti1516_2025::HLAinteger64Time(5),
      true);
  REQUIRE(inTransit.status == umbra::detail::FederationTsoRegistryStatus::applied);
  REQUIRE(inTransit.deliveryStatus ==
      umbra::detail::FederationTsoDeliveryStatus::applied);
  REQUIRE(inTransit.deliveries.size() == 1U);
  auto const* inTransitInteraction =
      std::get_if<umbra::detail::TsoInteractionDelivery>(
          &inTransit.deliveries.front());
  REQUIRE(inTransitInteraction != nullptr);
  REQUIRE(inTransitInteraction->message.messageId == enqueued.messageId);
  auto const inTransitMessage = inTransitInteraction->queuedMessage;

  REQUIRE(source.requestFederationSave(
      L"exercise",
      receiverA.membership->id,
      L"tso-payload-process-restart")
      .status == umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(source.federateSaveBegun(
      L"exercise", receiverA.membership->id).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(source.federateSaveBegun(
      L"exercise", receiverB.membership->id).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(source.federateSaveBegun(
      L"exercise", producer.membership->id).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE_FALSE(source.federateSaveComplete(
      L"exercise", receiverA.membership->id).saveCompletedSuccessfully);
  REQUIRE_FALSE(source.federateSaveComplete(
      L"exercise", receiverB.membership->id).saveCompletedSuccessfully);
  REQUIRE(source.federateSaveComplete(
      L"exercise", producer.membership->id).saveCompletedSuccessfully);

  auto committed = store->load(L"exercise", L"tso-payload-process-restart");
  REQUIRE(committed.has_value());
  auto image = umbra::detail::FederationStateImageCodec::decode(
      committed->stateImage);
  REQUIRE(image.tsoInteractionMessages.size() == 1U);
  REQUIRE(image.tsoInteractionMessages.front().messageId == enqueued.messageId);
  REQUIRE(image.tsoInteractionMessages.front().parameters.size() == 1U);
  REQUIRE(image.tsoInteractionMessages.front().parameters.front().value ==
      payloadBytes);
  REQUIRE(image.tsoInteractionMessages.front().userSuppliedTag == tagBytes);
  REQUIRE(image.tsoInteractionMessages.front().timestampEncoding.has_value());
  REQUIRE(image.tsoRequestRetractionRecords.size() == 1U);
  REQUIRE(image.tsoRequestRetractionRecords.front().recipientStates.size() == 2U);
  REQUIRE(image.tsoQueueEntries.size() == 2U);
  auto const phaseFor = [&](std::uint64_t recipientId) {
    auto const entry = std::find_if(
        image.tsoQueueEntries.begin(),
        image.tsoQueueEntries.end(),
        [recipientId, messageId = enqueued.messageId](
            umbra::detail::FederationStateImageTsoQueueEntry const& candidate) {
          return candidate.messageId == messageId &&
              candidate.recipientFederateId == recipientId;
        });
    REQUIRE(entry != image.tsoQueueEntries.end());
    return entry->phase;
  };
  REQUIRE(phaseFor(receiverA.membership->id) == 1U);
  REQUIRE(phaseFor(receiverB.membership->id) == 0U);

  // Rebuild the federation from the durable file in a new registry. Joining in
  // the original order gives the route-free image the same federate identities,
  // while each new member supplies a fresh callback/time-state route.
  EmbeddedFederationRegistry restarted({}, store);
  REQUIRE(restarted.create(L"exercise", validDefinition()).status ==
      FederationRegistryStatus::applied);
  auto restartedProducer = restarted.join(
      L"exercise", L"producer", L"producer", noOpCallbackRoute());
  auto restartedReceiverATime = std::make_shared<umbra::detail::FederateTimeState>(
      L"HLAinteger64Time",
      std::make_shared<rti1516_2025::HLAinteger64Time>(0));
  auto restartedReceiverA = restarted.joinWithTimeState(
      L"exercise",
      restartedReceiverATime,
      L"receiver-a",
      L"receiver-a",
      noOpCallbackRoute());
  auto restartedReceiverBTime = std::make_shared<umbra::detail::FederateTimeState>(
      L"HLAinteger64Time",
      std::make_shared<rti1516_2025::HLAinteger64Time>(0));
  auto restartedReceiverB = restarted.joinWithTimeState(
      L"exercise",
      restartedReceiverBTime,
      L"receiver-b",
      L"receiver-b",
      noOpCallbackRoute());
  REQUIRE(restartedProducer.membership);
  REQUIRE(restartedReceiverA.membership);
  REQUIRE(restartedReceiverB.membership);
  REQUIRE(restartedProducer.membership->id == producer.membership->id);
  REQUIRE(restartedReceiverA.membership->id == receiverA.membership->id);
  REQUIRE(restartedReceiverB.membership->id == receiverB.membership->id);

  REQUIRE(restarted.requestFederationRestore(
      L"exercise",
      restartedReceiverA.membership->id,
      L"tso-payload-process-restart")
      .status == umbra::detail::FederationRestoreControlStatus::applied);
  REQUIRE(restarted.federateRestoreComplete(
      L"exercise", restartedReceiverA.membership->id).status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  REQUIRE(restarted.federateRestoreComplete(
      L"exercise", restartedReceiverB.membership->id).status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  REQUIRE(restarted.federateRestoreComplete(
      L"exercise", restartedProducer.membership->id).status ==
      umbra::detail::FederationRestoreControlStatus::applied);

  // The saved in-transit record is completed using its durable identity; the
  // other recipient can then consume the restored payload from its queued
  // phase, including the original bytes, tag, order, transport, and timestamp.
  REQUIRE(restarted.completeTsoDelivery(
      L"exercise", inTransitMessage).delivery.status ==
      umbra::detail::FederationTsoDeliveryStatus::applied);
  auto restored = restarted.beginTsoPayloadDelivery(
      L"exercise",
      restartedReceiverB.membership->id,
      rti1516_2025::HLAinteger64Time(5),
      true);
  REQUIRE(restored.status == umbra::detail::FederationTsoRegistryStatus::applied);
  REQUIRE(restored.deliveryStatus ==
      umbra::detail::FederationTsoDeliveryStatus::applied);
  REQUIRE(restored.deliveries.size() == 1U);
  auto const* restoredInteraction =
      std::get_if<umbra::detail::TsoInteractionDelivery>(
          &restored.deliveries.front());
  REQUIRE(restoredInteraction != nullptr);
  REQUIRE(restoredInteraction->message.messageId == enqueued.messageId);
  REQUIRE(restoredInteraction->message.parameters.size() == 1U);
  REQUIRE(restoredInteraction->message.parameters.front().second.size() ==
      payloadBytes.size());
  REQUIRE(std::string(
              static_cast<char const*>(
                  restoredInteraction->message.parameters.front().second.data()),
              payloadBytes.size()) == payloadBytes);
  REQUIRE(restoredInteraction->message.userSuppliedTag.size() == tagBytes.size());
  REQUIRE(restoredInteraction->message.transportationName == "HLAreliable");
  REQUIRE(restoredInteraction->message.timestamp->implementationName() ==
      L"HLAinteger64Time");
  auto const* restoredTimestamp =
      dynamic_cast<rti1516_2025::HLAinteger64Time const*>(
          restoredInteraction->message.timestamp.get());
  REQUIRE(restoredTimestamp != nullptr);
  REQUIRE(restoredTimestamp->getTime() == 5);
  REQUIRE(restarted.completeTsoDelivery(
      L"exercise", restoredInteraction->queuedMessage).delivery.status ==
      umbra::detail::FederationTsoDeliveryStatus::applied);
  REQUIRE(restarted.beginTsoPayloadDelivery(
      L"exercise",
      restartedReceiverB.membership->id,
      rti1516_2025::HLAinteger64Time(5),
      true).deliveryStatus == umbra::detail::FederationTsoDeliveryStatus::no_messages);

  std::filesystem::remove_all(directory, ignored);
}
