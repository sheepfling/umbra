#include "ieee1516_2025_federation_registry_test_support.hpp"

TEST_CASE(
    "Filesystem fresh-registry restore preserves one timestamped regional attribute-update payload across source-region mutation and resignation",
    "[unit][kernel][federation-registry][save-restore][durable-save][filesystem][restore]"
    "[process-restart][tso-queue-state][tso-payload-state][tso-attribute-update-state]"
    "[tso-regional-attribute-update-state][tso-regional-attribute-update-resignation-state]") {
  auto const directory = temporarySaveCommitDirectory();
  std::error_code ignored;
  std::filesystem::remove_all(directory, ignored);
  auto store = std::make_shared<umbra::detail::FilesystemFederationSaveCommitStore>(
      directory);

  EmbeddedFederationRegistry source({}, store);
  REQUIRE(source.create(L"exercise", composedRestaurantDefinition()).status ==
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

  auto const dimension = source.dimensionHandleFor(L"exercise", "ServerId");
  REQUIRE(dimension.has_value());
  auto sourceRegion = source.createRegion(
      L"exercise", producer.membership->id, {*dimension});
  REQUIRE(sourceRegion.status == umbra::detail::RegionServiceStatus::applied);
  REQUIRE(sourceRegion.regionHandle != 0U);
  REQUIRE(source.setRangeBounds(
      L"exercise", producer.membership->id, sourceRegion.regionHandle, *dimension,
      umbra::detail::RegionRangeBounds{2UL, 4UL}) ==
      umbra::detail::RegionServiceStatus::applied);
  REQUIRE(source.commitRegionModifications(
      L"exercise", producer.membership->id, {sourceRegion.regionHandle}) ==
      umbra::detail::RegionServiceStatus::applied);

  std::string const payloadBytes{"\x63\x64", 2U};
  std::string const tagBytes{"regionalfs", 10U};
  umbra::detail::RegionSpecificationSnapshot const regionSnapshot{
      std::set<std::uint64_t>{*dimension},
      std::map<std::uint64_t, umbra::detail::RegionRangeBounds>{
          {*dimension, umbra::detail::RegionRangeBounds{2UL, 4UL}}},
      true};
  umbra::detail::TsoAttributeUpdateMessage attributeUpdate;
  attributeUpdate.producingFederateId = producer.membership->id;
  attributeUpdate.objectInstanceHandle = 93U;
  attributeUpdate.attributes = {{
      5U,
      rti1516_2025::VariableLengthData(payloadBytes.data(), payloadBytes.size()),
  }};
  attributeUpdate.userSuppliedTag = rti1516_2025::VariableLengthData(
      tagBytes.data(), tagBytes.size());
  attributeUpdate.timestamp =
      std::make_shared<rti1516_2025::HLAinteger64Time>(6);
  attributeUpdate.sentRegionSnapshots.emplace(sourceRegion.regionHandle, regionSnapshot);
  umbra::detail::TsoAttributeUpdatePassel regionalPassel;
  regionalPassel.transportationName = "HLAreliable";
  regionalPassel.sentAttributeHandles = {5U};
  regionalPassel.sentRegionHandles = {sourceRegion.regionHandle};
  regionalPassel.sentRegionSnapshots.emplace(sourceRegion.regionHandle, regionSnapshot);
  regionalPassel.preferredOrderType = rti1516_2025::TIMESTAMP;
  for (auto const recipientId : {
           receiverA.membership->id,
           receiverB.membership->id,
       }) {
    attributeUpdate.passelsByRecipient.emplace(
        recipientId,
        std::vector<umbra::detail::TsoAttributeUpdatePassel>{regionalPassel});
  }
  auto const enqueued = source.enqueueTsoAttributeUpdate(
      L"exercise",
      std::move(attributeUpdate),
      {receiverA.membership->id, receiverB.membership->id},
      {receiverA.membership->id, receiverB.membership->id});
  REQUIRE(enqueued.status == umbra::detail::FederationTsoRegistryStatus::applied);
  REQUIRE(enqueued.queueStatus == umbra::detail::TsoMessageQueueStatus::applied);
  REQUIRE(enqueued.enqueuedRecipientCount == 2U);

  auto inTransit = source.beginTsoPayloadDelivery(
      L"exercise",
      receiverA.membership->id,
      rti1516_2025::HLAinteger64Time(6),
      true);
  REQUIRE(inTransit.status == umbra::detail::FederationTsoRegistryStatus::applied);
  REQUIRE(inTransit.deliveryStatus ==
      umbra::detail::FederationTsoDeliveryStatus::applied);
  REQUIRE(inTransit.deliveries.size() == 1U);
  auto const* inTransitAttribute =
      std::get_if<umbra::detail::TsoAttributeUpdateDelivery>(
          &inTransit.deliveries.front());
  REQUIRE(inTransitAttribute != nullptr);
  REQUIRE(inTransitAttribute->message.messageId == enqueued.messageId);
  auto const inTransitMessage = inTransitAttribute->queuedMessage;

  REQUIRE(source.requestFederationSave(
      L"exercise",
      receiverA.membership->id,
      L"regional-attribute-tso-payload-process-restart")
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

  auto committed = store->load(
      L"exercise", L"regional-attribute-tso-payload-process-restart");
  REQUIRE(committed.has_value());
  auto image = umbra::detail::FederationStateImageCodec::decode(
      committed->stateImage);
  REQUIRE(image.tsoAttributeUpdateMessages.size() == 1U);
  auto const& savedMessage = image.tsoAttributeUpdateMessages.front();
  REQUIRE(savedMessage.messageId == enqueued.messageId);
  REQUIRE(savedMessage.objectInstanceHandle == 93U);
  REQUIRE(savedMessage.attributes.size() == 1U);
  REQUIRE(savedMessage.attributes.front().value == payloadBytes);
  REQUIRE(savedMessage.userSuppliedTag == tagBytes);
  REQUIRE(savedMessage.passelsByRecipient.size() == 2U);
  REQUIRE(savedMessage.sentRegionSnapshots.size() == 1U);
  REQUIRE(savedMessage.sentRegionSnapshots.front().regionHandle == sourceRegion.regionHandle);
  REQUIRE(savedMessage.sentRegionSnapshots.front().dimensionHandles ==
      std::vector<std::uint64_t>{*dimension});
  REQUIRE(savedMessage.sentRegionSnapshots.front().committedRangeBounds.size() == 1U);
  REQUIRE(savedMessage.sentRegionSnapshots.front().committedRangeBounds.front().dimensionHandle ==
      *dimension);
  REQUIRE(savedMessage.sentRegionSnapshots.front().committedRangeBounds.front().lowerBound ==
      2U);
  REQUIRE(savedMessage.sentRegionSnapshots.front().committedRangeBounds.front().upperBound ==
      4U);
  for (auto const& recipient : savedMessage.passelsByRecipient) {
    REQUIRE(recipient.passels.size() == 1U);
    auto const& savedPassel = recipient.passels.front();
    REQUIRE(savedPassel.sentAttributeHandles == std::vector<std::uint64_t>{5U});
    REQUIRE(savedPassel.sentRegionHandles ==
        std::vector<std::uint64_t>{sourceRegion.regionHandle});
    REQUIRE(savedPassel.sentRegionSnapshots.size() == 1U);
    REQUIRE(savedPassel.sentRegionSnapshots.front().regionHandle == sourceRegion.regionHandle);
    REQUIRE(savedPassel.sentRegionSnapshots.front().dimensionHandles ==
        std::vector<std::uint64_t>{*dimension});
    REQUIRE(savedPassel.sentRegionSnapshots.front().committedRangeBounds.front().lowerBound ==
        2U);
    REQUIRE(savedPassel.sentRegionSnapshots.front().committedRangeBounds.front().upperBound ==
        4U);
  }
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

  // The saved image has already captured the [2, 4) invocation snapshot.
  // Mutate the live source region and then resign its owner; the queued and
  // in-transit payload must retain the immutable snapshot rather than reread
  // the source region (which no longer exists after resignation).
  REQUIRE(source.setRangeBounds(
      L"exercise", producer.membership->id, sourceRegion.regionHandle, *dimension,
      umbra::detail::RegionRangeBounds{8UL, 10UL}) ==
      umbra::detail::RegionServiceStatus::applied);
  REQUIRE(source.commitRegionModifications(
      L"exercise", producer.membership->id, {sourceRegion.regionHandle}) ==
      umbra::detail::RegionServiceStatus::applied);
  REQUIRE(source.resign(
      L"exercise", producer.membership->id, rti1516_2025::NO_ACTION).status ==
      FederationRegistryStatus::applied);

  EmbeddedFederationRegistry restarted({}, store);
  REQUIRE(restarted.create(L"exercise", composedRestaurantDefinition()).status ==
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
      L"regional-attribute-tso-payload-process-restart")
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

  // The durable region specification is restored with the invocation-time
  // bounds, then changed again in the restarted live registry.  Delivery must
  // still expose [2, 4) from the saved message/passel snapshot.
  auto restoredRegionDimensions = restarted.dimensionHandleSetForRegion(
      L"exercise", restartedProducer.membership->id, sourceRegion.regionHandle);
  REQUIRE(restoredRegionDimensions.status ==
      umbra::detail::RegionServiceStatus::applied);
  REQUIRE(restoredRegionDimensions.dimensionHandles ==
      std::set<std::uint64_t>{*dimension});
  auto restoredRegionBounds = restarted.rangeBoundsForRegion(
      L"exercise", restartedProducer.membership->id, sourceRegion.regionHandle, *dimension);
  REQUIRE(restoredRegionBounds.status == umbra::detail::RegionServiceStatus::applied);
  REQUIRE(restoredRegionBounds.range.lowerBound == 2UL);
  REQUIRE(restoredRegionBounds.range.upperBound == 4UL);
  REQUIRE(restarted.setRangeBounds(
      L"exercise", restartedProducer.membership->id, sourceRegion.regionHandle, *dimension,
      umbra::detail::RegionRangeBounds{9UL, 11UL}) ==
      umbra::detail::RegionServiceStatus::applied);
  REQUIRE(restarted.commitRegionModifications(
      L"exercise", restartedProducer.membership->id, {sourceRegion.regionHandle}) ==
      umbra::detail::RegionServiceStatus::applied);

  REQUIRE(restarted.completeTsoDelivery(
      L"exercise", inTransitMessage).delivery.status ==
      umbra::detail::FederationTsoDeliveryStatus::applied);
  auto restored = restarted.beginTsoPayloadDelivery(
      L"exercise",
      restartedReceiverB.membership->id,
      rti1516_2025::HLAinteger64Time(6),
      true);
  REQUIRE(restored.status == umbra::detail::FederationTsoRegistryStatus::applied);
  REQUIRE(restored.deliveryStatus ==
      umbra::detail::FederationTsoDeliveryStatus::applied);
  REQUIRE(restored.deliveries.size() == 1U);
  auto const* restoredAttribute =
      std::get_if<umbra::detail::TsoAttributeUpdateDelivery>(
          &restored.deliveries.front());
  REQUIRE(restoredAttribute != nullptr);
  REQUIRE(restoredAttribute->message.messageId == enqueued.messageId);
  REQUIRE(restoredAttribute->message.objectInstanceHandle == 93U);
  REQUIRE(restoredAttribute->message.attributes.size() == 1U);
  REQUIRE(restoredAttribute->message.attributes.front().second.size() ==
      payloadBytes.size());
  REQUIRE(std::string(
              static_cast<char const*>(
                  restoredAttribute->message.attributes.front().second.data()),
              payloadBytes.size()) == payloadBytes);
  REQUIRE(restoredAttribute->message.userSuppliedTag.size() == tagBytes.size());
  REQUIRE(restoredAttribute->message.sentRegionSnapshots.size() == 1U);
  REQUIRE(restoredAttribute->message.sentRegionSnapshots.at(sourceRegion.regionHandle)
              .dimensionHandles == std::set<std::uint64_t>{*dimension});
  REQUIRE(restoredAttribute->message.sentRegionSnapshots.at(sourceRegion.regionHandle)
              .committedRangeBounds.at(*dimension).lowerBound == 2U);
  REQUIRE(restoredAttribute->message.sentRegionSnapshots.at(sourceRegion.regionHandle)
              .committedRangeBounds.at(*dimension).upperBound == 4U);
  auto const passel = restoredAttribute->message.passelsByRecipient.find(
      restartedReceiverB.membership->id);
  REQUIRE(passel != restoredAttribute->message.passelsByRecipient.end());
  REQUIRE(passel->second.size() == 1U);
  REQUIRE(passel->second.front().sentRegionHandles ==
      std::set<std::uint64_t>{sourceRegion.regionHandle});
  REQUIRE(passel->second.front().sentRegionSnapshots.size() == 1U);
  REQUIRE(passel->second.front().sentRegionSnapshots.at(sourceRegion.regionHandle)
              .committedRangeBounds.at(*dimension).lowerBound == 2U);
  REQUIRE(passel->second.front().sentRegionSnapshots.at(sourceRegion.regionHandle)
              .committedRangeBounds.at(*dimension).upperBound == 4U);
  REQUIRE(restoredAttribute->message.timestamp->implementationName() ==
      L"HLAinteger64Time");
  auto const* restoredTimestamp = dynamic_cast<rti1516_2025::HLAinteger64Time const*>(
      restoredAttribute->message.timestamp.get());
  REQUIRE(restoredTimestamp != nullptr);
  REQUIRE(restoredTimestamp->getTime() == 6);
  REQUIRE(restarted.completeTsoDelivery(
      L"exercise", restoredAttribute->queuedMessage).delivery.status ==
      umbra::detail::FederationTsoDeliveryStatus::applied);
  REQUIRE(restarted.beginTsoPayloadDelivery(
      L"exercise",
      restartedReceiverB.membership->id,
      rti1516_2025::HLAinteger64Time(6),
      true).deliveryStatus == umbra::detail::FederationTsoDeliveryStatus::no_messages);

  std::filesystem::remove_all(directory, ignored);
}
