#include "ieee1516_2025_federation_registry_test_support.hpp"

TEST_CASE(
    "Filesystem fresh-registry restore rehydrates one timestamped directed interaction payload across queued and in-transit recipients",
    "[unit][kernel][federation-registry][save-restore][durable-save][filesystem][restore]"
    "[process-restart][tso-queue-state][tso-payload-state][tso-directed-interaction-state]") {
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

  std::string const payloadBytes{"\x50\x51", 2U};
  std::string const tagBytes{"directedfs", 10U};
  umbra::detail::TsoDirectedInteractionMessage directed;
  directed.producingFederateId = producer.membership->id;
  directed.objectInstanceHandle = 91U;
  directed.sentInteractionClassHandle = 3U;
  directed.sentParameterHandles = {4U};
  directed.parameters = {{
      4U,
      rti1516_2025::VariableLengthData(payloadBytes.data(), payloadBytes.size()),
  }};
  directed.userSuppliedTag = rti1516_2025::VariableLengthData(
      tagBytes.data(), tagBytes.size());
  directed.transportationName = "HLAreliable";
  directed.recipients = {{
      receiverA.membership->id,
      91U,
      3U,
      {4U},
      noOpCallbackRoute(),
  }, {
      receiverB.membership->id,
      91U,
      3U,
      {4U},
      noOpCallbackRoute(),
  }};
  directed.timestamp =
      std::make_shared<rti1516_2025::HLAinteger64Time>(5);
  auto const enqueued = source.enqueueTsoDirectedInteraction(
      L"exercise",
      std::move(directed),
      {receiverA.membership->id, receiverB.membership->id});
  REQUIRE(enqueued.status == umbra::detail::FederationTsoRegistryStatus::applied);
  REQUIRE(enqueued.queueStatus == umbra::detail::TsoMessageQueueStatus::applied);
  REQUIRE(enqueued.enqueuedRecipientCount == 2U);

  auto inTransit = source.beginTsoPayloadDelivery(
      L"exercise",
      receiverA.membership->id,
      rti1516_2025::HLAinteger64Time(5),
      true);
  REQUIRE(inTransit.status == umbra::detail::FederationTsoRegistryStatus::applied);
  REQUIRE(inTransit.deliveryStatus ==
      umbra::detail::FederationTsoDeliveryStatus::applied);
  REQUIRE(inTransit.deliveries.size() == 1U);
  auto const* inTransitDirected =
      std::get_if<umbra::detail::TsoDirectedInteractionDelivery>(
          &inTransit.deliveries.front());
  REQUIRE(inTransitDirected != nullptr);
  REQUIRE(inTransitDirected->message.messageId == enqueued.messageId);
  auto const inTransitMessage = inTransitDirected->queuedMessage;

  REQUIRE(source.requestFederationSave(
      L"exercise",
      receiverA.membership->id,
      L"directed-tso-payload-process-restart")
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

  auto committed = store->load(L"exercise", L"directed-tso-payload-process-restart");
  REQUIRE(committed.has_value());
  auto image = umbra::detail::FederationStateImageCodec::decode(
      committed->stateImage);
  REQUIRE(image.tsoDirectedInteractionMessages.size() == 1U);
  REQUIRE(image.tsoDirectedInteractionMessages.front().messageId == enqueued.messageId);
  REQUIRE(image.tsoDirectedInteractionMessages.front().objectInstanceHandle == 91U);
  REQUIRE(image.tsoDirectedInteractionMessages.front().parameters.size() == 1U);
  REQUIRE(image.tsoDirectedInteractionMessages.front().parameters.front().value ==
      payloadBytes);
  REQUIRE(image.tsoDirectedInteractionMessages.front().userSuppliedTag == tagBytes);
  REQUIRE(image.tsoDirectedInteractionMessages.front().recipients.size() == 2U);
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
      L"directed-tso-payload-process-restart")
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
  auto const* restoredDirected =
      std::get_if<umbra::detail::TsoDirectedInteractionDelivery>(
          &restored.deliveries.front());
  REQUIRE(restoredDirected != nullptr);
  REQUIRE(restoredDirected->message.messageId == enqueued.messageId);
  REQUIRE(restoredDirected->message.objectInstanceHandle == 91U);
  REQUIRE(restoredDirected->message.parameters.size() == 1U);
  REQUIRE(restoredDirected->message.parameters.front().second.size() ==
      payloadBytes.size());
  REQUIRE(std::string(
              static_cast<char const*>(
                  restoredDirected->message.parameters.front().second.data()),
              payloadBytes.size()) == payloadBytes);
  REQUIRE(restoredDirected->message.userSuppliedTag.size() == tagBytes.size());
  REQUIRE(restoredDirected->message.transportationName == "HLAreliable");
  REQUIRE(restoredDirected->message.recipients.size() == 2U);
  auto const recipient = std::find_if(
      restoredDirected->message.recipients.begin(),
      restoredDirected->message.recipients.end(),
      [id = restartedReceiverB.membership->id](
          umbra::detail::TsoDirectedInteractionRecipient const& candidate) {
        return candidate.receivingFederateId == id;
      });
  REQUIRE(recipient != restoredDirected->message.recipients.end());
  REQUIRE(recipient->objectInstanceHandle == 91U);
  REQUIRE(recipient->receivedInteractionClassHandle == 3U);
  REQUIRE(recipient->receivedParameterHandles == std::set<std::uint64_t>{4U});
  REQUIRE(recipient->callbackRoute);
  REQUIRE(restoredDirected->message.timestamp->implementationName() ==
      L"HLAinteger64Time");
  auto const* restoredTimestamp =
      dynamic_cast<rti1516_2025::HLAinteger64Time const*>(
          restoredDirected->message.timestamp.get());
  REQUIRE(restoredTimestamp != nullptr);
  REQUIRE(restoredTimestamp->getTime() == 5);
  REQUIRE(restarted.completeTsoDelivery(
      L"exercise", restoredDirected->queuedMessage).delivery.status ==
      umbra::detail::FederationTsoDeliveryStatus::applied);
  REQUIRE(restarted.beginTsoPayloadDelivery(
      L"exercise",
      restartedReceiverB.membership->id,
      rti1516_2025::HLAinteger64Time(5),
      true).deliveryStatus == umbra::detail::FederationTsoDeliveryStatus::no_messages);

  std::filesystem::remove_all(directory, ignored);
}
