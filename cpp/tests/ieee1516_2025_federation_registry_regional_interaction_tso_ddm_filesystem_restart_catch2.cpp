#include "ieee1516_2025_federation_registry_test_support.hpp"

TEST_CASE(
    "Filesystem fresh-registry restore preserves one timestamped regional interaction payload and source-region snapshot",
    "[unit][kernel][federation-registry][save-restore][durable-save][filesystem][restore]"
    "[process-restart][process-restart-regional-interaction-tso-ddm][tso-queue-state]"
    "[tso-payload-state][tso-interaction-state][tso-regional-interaction-state]"
    "[interaction-management][ddm][time-management]"
    "[rti.service.create-region][rti.service.set-range-bounds]"
    "[rti.service.commit-region-modifications]"
    "[rti.service.subscribe-interaction-class-with-regions]"
    "[rti.service.send-interaction-with-regions]"
    "[federate.callback.receive-interaction]"
    "[rti.service.request-federation-save][rti.service.federate-save-begun]"
    "[rti.service.federate-save-complete][rti.service.request-federation-restore]"
    "[rti.service.federate-restore-complete][federate.callback.federation-restored]") {
  auto const directory = temporarySaveCommitDirectory();
  std::error_code ignored;
  std::filesystem::remove_all(directory, ignored);
  auto store = std::make_shared<umbra::detail::FilesystemFederationSaveCommitStore>(
      directory);

  EmbeddedFederationRegistry source({}, store);
  REQUIRE(source.create(L"exercise", composedRestaurantDefinition()).status ==
      FederationRegistryStatus::applied);
  auto producer = source.join(
      L"exercise", L"regional-producer", L"regional-producer", noOpCallbackRoute());
  auto receiverATime = std::make_shared<umbra::detail::FederateTimeState>(
      L"HLAinteger64Time",
      std::make_shared<rti1516_2025::HLAinteger64Time>(0));
  auto receiverA = source.joinWithTimeState(
      L"exercise", receiverATime, L"regional-receiver-a", L"regional-receiver-a",
      noOpCallbackRoute());
  auto receiverBTime = std::make_shared<umbra::detail::FederateTimeState>(
      L"HLAinteger64Time",
      std::make_shared<rti1516_2025::HLAinteger64Time>(0));
  auto receiverB = source.joinWithTimeState(
      L"exercise", receiverBTime, L"regional-receiver-b", L"regional-receiver-b",
      noOpCallbackRoute());
  REQUIRE(producer.membership);
  REQUIRE(receiverA.membership);
  REQUIRE(receiverB.membership);

  auto const interactionClass = source.interactionClassHandleFor(
      L"exercise",
      "HLAinteractionRoot.CustomerTransactions.FoodServed.MainCourseServed");
  auto const dimension = source.dimensionHandleFor(L"exercise", "ServerId");
  REQUIRE(interactionClass.has_value());
  REQUIRE(dimension.has_value());
  REQUIRE(source.setInteractionClassPublication(
      L"exercise", producer.membership->id, *interactionClass, true) ==
      umbra::detail::InteractionClassDeclarationStatus::applied);

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

  auto makeReceiverRegion = [&](std::uint64_t receiverId) {
    auto region = source.createRegion(L"exercise", receiverId, {*dimension});
    REQUIRE(region.status == umbra::detail::RegionServiceStatus::applied);
    REQUIRE(region.regionHandle != 0U);
    REQUIRE(source.setRangeBounds(
        L"exercise", receiverId, region.regionHandle, *dimension,
        umbra::detail::RegionRangeBounds{2UL, 4UL}) ==
        umbra::detail::RegionServiceStatus::applied);
    REQUIRE(source.commitRegionModifications(
        L"exercise", receiverId, {region.regionHandle}) ==
        umbra::detail::RegionServiceStatus::applied);
    REQUIRE(source.setInteractionClassRegionalSubscription(
        L"exercise", receiverId, *interactionClass, {region.regionHandle}, true) ==
        umbra::detail::RegionalInteractionClassDeclarationStatus::applied);
    return region.regionHandle;
  };
  auto const receiverARegion = makeReceiverRegion(receiverA.membership->id);
  auto const receiverBRegion = makeReceiverRegion(receiverB.membership->id);

  std::set<std::uint64_t> const sentRegionHandles{sourceRegion.regionHandle};
  auto plan = source.planReceiveOrderInteraction(
      L"exercise", producer.membership->id, *interactionClass, {},
      &sentRegionHandles);
  REQUIRE(plan.status == umbra::detail::ReceiveOrderInteractionStatus::applied);
  REQUIRE(plan.transportationName == "HLAreliable");
  REQUIRE(plan.preferredOrderType == rti1516_2025::TIMESTAMP);
  REQUIRE(plan.defaultRegionUsed == false);
  REQUIRE(plan.sentRegionSnapshots.size() == 1U);
  REQUIRE(plan.sentRegionSnapshots.contains(sourceRegion.regionHandle));
  REQUIRE(plan.recipients.size() == 2U);
  REQUIRE(std::set<std::uint64_t>{
              plan.recipients[0].federateId,
              plan.recipients[1].federateId} ==
      std::set<std::uint64_t>{receiverA.membership->id, receiverB.membership->id});

  std::string const tagBytes{"regional-interaction-fs", 22U};
  umbra::detail::TsoInteractionMessage interaction;
  interaction.producingFederateId = producer.membership->id;
  interaction.sentInteractionClassHandle = *interactionClass;
  interaction.userSuppliedTag = rti1516_2025::VariableLengthData(
      tagBytes.data(), tagBytes.size());
  interaction.transportationName = plan.transportationName;
  interaction.sentRegionHandles = sentRegionHandles;
  interaction.sentRegionSnapshots = plan.sentRegionSnapshots;
  interaction.defaultRegionUsed = plan.defaultRegionUsed;
  interaction.timestamp =
      std::make_shared<rti1516_2025::HLAinteger64Time>(6);
  interaction.sentOrderType = plan.preferredOrderType;
  interaction.receivedOrderType = plan.preferredOrderType;
  auto const enqueued = source.enqueueTsoInteraction(
      L"exercise", std::move(interaction),
      {receiverA.membership->id, receiverB.membership->id},
      {receiverA.membership->id, receiverB.membership->id});
  REQUIRE(enqueued.status == umbra::detail::FederationTsoRegistryStatus::applied);
  REQUIRE(enqueued.queueStatus == umbra::detail::TsoMessageQueueStatus::applied);
  REQUIRE(enqueued.enqueuedRecipientCount == 2U);

  auto inTransit = source.beginTsoPayloadDelivery(
      L"exercise", receiverA.membership->id,
      rti1516_2025::HLAinteger64Time(6), true);
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

  std::wstring const saveLabel = L"regional-interaction-tso-ddm-process-restart";
  REQUIRE(source.requestFederationSave(
      L"exercise", receiverA.membership->id, saveLabel).status ==
      umbra::detail::FederationSaveControlStatus::applied);
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

  auto committed = store->load(L"exercise", saveLabel);
  REQUIRE(committed.has_value());
  auto image = umbra::detail::FederationStateImageCodec::decode(
      committed->stateImage);
  REQUIRE(image.tsoInteractionMessages.size() == 1U);
  auto const& savedMessage = image.tsoInteractionMessages.front();
  REQUIRE(savedMessage.messageId == enqueued.messageId);
  REQUIRE(savedMessage.sentInteractionClassHandle == *interactionClass);
  REQUIRE(savedMessage.transportationName == "HLAreliable");
  REQUIRE(savedMessage.sentRegionHandles ==
      std::vector<std::uint64_t>{sourceRegion.regionHandle});
  REQUIRE(savedMessage.sentRegionSnapshots.size() == 1U);
  REQUIRE(savedMessage.sentRegionSnapshots.front().regionHandle ==
      sourceRegion.regionHandle);
  REQUIRE(savedMessage.sentRegionSnapshots.front().dimensionHandles ==
      std::vector<std::uint64_t>{*dimension});
  REQUIRE(savedMessage.sentRegionSnapshots.front().committedRangeBounds.size() == 1U);
  REQUIRE(savedMessage.sentRegionSnapshots.front().committedRangeBounds.front().lowerBound ==
      2U);
  REQUIRE(savedMessage.sentRegionSnapshots.front().committedRangeBounds.front().upperBound ==
      4U);
  REQUIRE(savedMessage.sentOrderType ==
      static_cast<std::uint32_t>(rti1516_2025::TIMESTAMP));
  REQUIRE(savedMessage.receivedOrderType ==
      static_cast<std::uint32_t>(rti1516_2025::TIMESTAMP));
  REQUIRE(savedMessage.timestampEncoding.has_value());
  REQUIRE(image.tsoRequestRetractionRecords.size() == 1U);
  REQUIRE(image.tsoRequestRetractionRecords.front().recipientStates.size() == 2U);
  REQUIRE(image.tsoQueueEntries.size() == 2U);

  // Mutating and resigning the live source after the save must not change the
  // invocation-time region realization retained by the TSO payload.
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
      L"exercise", L"regional-producer", L"regional-producer", noOpCallbackRoute());
  auto restartedReceiverATime = std::make_shared<umbra::detail::FederateTimeState>(
      L"HLAinteger64Time",
      std::make_shared<rti1516_2025::HLAinteger64Time>(0));
  auto restartedReceiverA = restarted.joinWithTimeState(
      L"exercise", restartedReceiverATime, L"regional-receiver-a", L"regional-receiver-a",
      noOpCallbackRoute());
  auto restartedReceiverBTime = std::make_shared<umbra::detail::FederateTimeState>(
      L"HLAinteger64Time",
      std::make_shared<rti1516_2025::HLAinteger64Time>(0));
  auto restartedReceiverB = restarted.joinWithTimeState(
      L"exercise", restartedReceiverBTime, L"regional-receiver-b", L"regional-receiver-b",
      noOpCallbackRoute());
  REQUIRE(restartedProducer.membership);
  REQUIRE(restartedReceiverA.membership);
  REQUIRE(restartedReceiverB.membership);
  REQUIRE(restartedProducer.membership->id == producer.membership->id);
  REQUIRE(restartedReceiverA.membership->id == receiverA.membership->id);
  REQUIRE(restartedReceiverB.membership->id == receiverB.membership->id);

  REQUIRE(restarted.requestFederationRestore(
      L"exercise", restartedReceiverA.membership->id, saveLabel).status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  REQUIRE(restarted.federateRestoreComplete(
      L"exercise", restartedReceiverA.membership->id).status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  REQUIRE(restarted.federateRestoreComplete(
      L"exercise", restartedReceiverB.membership->id).status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  REQUIRE(restarted.federateRestoreComplete(
      L"exercise", restartedProducer.membership->id).status ==
      umbra::detail::FederationRestoreControlStatus::applied);

  auto const restartedInteraction = restarted.interactionClassHandleFor(
      L"exercise",
      "HLAinteractionRoot.CustomerTransactions.FoodServed.MainCourseServed");
  auto const restartedDimension = restarted.dimensionHandleFor(L"exercise", "ServerId");
  REQUIRE(restartedInteraction.has_value());
  REQUIRE(restartedDimension.has_value());
  REQUIRE(*restartedInteraction == *interactionClass);
  REQUIRE(*restartedDimension == *dimension);
  auto restoredSourceBounds = restarted.rangeBoundsForRegion(
      L"exercise", restartedProducer.membership->id, sourceRegion.regionHandle,
      *restartedDimension);
  REQUIRE(restoredSourceBounds.status == umbra::detail::RegionServiceStatus::applied);
  REQUIRE(restoredSourceBounds.range.lowerBound == 2UL);
  REQUIRE(restoredSourceBounds.range.upperBound == 4UL);
  auto restoredReceiverADimensions = restarted.dimensionHandleSetForRegion(
      L"exercise", restartedReceiverA.membership->id, receiverARegion);
  auto restoredReceiverBDimensions = restarted.dimensionHandleSetForRegion(
      L"exercise", restartedReceiverB.membership->id, receiverBRegion);
  REQUIRE(restoredReceiverADimensions.dimensionHandles ==
      std::set<std::uint64_t>{*restartedDimension});
  REQUIRE(restoredReceiverBDimensions.dimensionHandles ==
      std::set<std::uint64_t>{*restartedDimension});

  // The live source region is now disjoint, but the restored payload carries
  // the accepted [2, 4) snapshot for callback-boundary DDM evaluation.
  REQUIRE(restarted.setRangeBounds(
      L"exercise", restartedProducer.membership->id, sourceRegion.regionHandle,
      *restartedDimension, umbra::detail::RegionRangeBounds{9UL, 11UL}) ==
      umbra::detail::RegionServiceStatus::applied);
  REQUIRE(restarted.commitRegionModifications(
      L"exercise", restartedProducer.membership->id, {sourceRegion.regionHandle}) ==
      umbra::detail::RegionServiceStatus::applied);
  auto liveNoOverlap = restarted.receiveOrderInteractionRecipientFor(
      L"exercise", restartedProducer.membership->id, restartedReceiverB.membership->id,
      *restartedInteraction, {}, &sentRegionHandles);
  REQUIRE_FALSE(liveNoOverlap.has_value());

  REQUIRE(restarted.completeTsoDelivery(
      L"exercise", inTransitMessage).delivery.status ==
      umbra::detail::FederationTsoDeliveryStatus::applied);
  auto restored = restarted.beginTsoPayloadDelivery(
      L"exercise", restartedReceiverB.membership->id,
      rti1516_2025::HLAinteger64Time(6), true);
  REQUIRE(restored.status == umbra::detail::FederationTsoRegistryStatus::applied);
  REQUIRE(restored.deliveryStatus ==
      umbra::detail::FederationTsoDeliveryStatus::applied);
  REQUIRE(restored.deliveries.size() == 1U);
  auto const* restoredInteractionPayload =
      std::get_if<umbra::detail::TsoInteractionDelivery>(
          &restored.deliveries.front());
  REQUIRE(restoredInteractionPayload != nullptr);
  auto snapshotRecipient = restarted.receiveOrderInteractionRecipientFor(
      L"exercise", restartedProducer.membership->id,
      restartedReceiverB.membership->id, *restartedInteraction, {},
      &sentRegionHandles,
      &restoredInteractionPayload->message.sentRegionSnapshots);
  REQUIRE(snapshotRecipient.has_value());
  REQUIRE(snapshotRecipient->federateId == restartedReceiverB.membership->id);
  REQUIRE(restoredInteractionPayload->message.messageId == enqueued.messageId);
  REQUIRE(restoredInteractionPayload->message.sentRegionHandles == sentRegionHandles);
  REQUIRE(restoredInteractionPayload->message.sentRegionSnapshots.size() == 1U);
  REQUIRE(restoredInteractionPayload->message.sentRegionSnapshots.at(
              sourceRegion.regionHandle).committedRangeBounds.at(*dimension).lowerBound == 2U);
  REQUIRE(restoredInteractionPayload->message.sentRegionSnapshots.at(
              sourceRegion.regionHandle).committedRangeBounds.at(*dimension).upperBound == 4U);
  REQUIRE(restoredInteractionPayload->message.transportationName == "HLAreliable");
  REQUIRE(restoredInteractionPayload->message.sentOrderType == rti1516_2025::TIMESTAMP);
  REQUIRE(restoredInteractionPayload->message.receivedOrderType == rti1516_2025::TIMESTAMP);
  REQUIRE(restoredInteractionPayload->message.userSuppliedTag.size() == tagBytes.size());
  REQUIRE(std::string(
              static_cast<char const*>(
                  restoredInteractionPayload->message.userSuppliedTag.data()),
              tagBytes.size()) == tagBytes);
  auto const* restoredTimestamp = dynamic_cast<rti1516_2025::HLAinteger64Time const*>(
      restoredInteractionPayload->message.timestamp.get());
  REQUIRE(restoredTimestamp != nullptr);
  REQUIRE(restoredTimestamp->getTime() == 6);
  REQUIRE(restarted.completeTsoDelivery(
      L"exercise", restoredInteractionPayload->queuedMessage).delivery.status ==
      umbra::detail::FederationTsoDeliveryStatus::applied);
  REQUIRE(restarted.beginTsoPayloadDelivery(
      L"exercise", restartedReceiverB.membership->id,
      rti1516_2025::HLAinteger64Time(6), true).deliveryStatus ==
      umbra::detail::FederationTsoDeliveryStatus::no_messages);

  std::filesystem::remove_all(directory, ignored);
}
