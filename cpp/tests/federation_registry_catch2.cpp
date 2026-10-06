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

TEST_CASE(
    "The embedded federation registry reports a directed interaction transportation override",
    "[unit][kernel][federation-registry][interaction-management][declaration-management]"
    "[directed-interaction][directed-routing][interaction-transportation-type-change]"
    "[transportation-management][federation-registry-directed-transportation-override][rti.service.publish-object-class-directed-interactions]"
    "[rti.service.request-interaction-transportation-type-change]"
    "[rti.service.query-interaction-transportation-type]") {
  EmbeddedFederationRegistry registry;
  REQUIRE(registry.create(
                        L"exercise",
                        composedDirectedInteractionDefinition())
              .status == FederationRegistryStatus::applied);
  auto joined = registry.join(
      L"exercise", L"directed-publisher", L"directed-publisher", noOpCallbackRoute());
  REQUIRE(joined.status == FederationRegistryStatus::applied);
  REQUIRE(joined.membership.has_value());
  auto const federateId = joined.membership->id;

  auto const objectClass = registry.objectClassHandleFor(
      L"exercise", "HLAobjectRoot.UmbraDirectedFixtureObject");
  auto const interactionClass = registry.interactionClassHandleFor(
      L"exercise", "HLAinteractionRoot.UmbraDirectedFixtureInteraction");
  REQUIRE(objectClass.has_value());
  REQUIRE(interactionClass.has_value());
  REQUIRE(registry.publishObjectClassDirectedInteractions(
               L"exercise", federateId, *objectClass, {*interactionClass}) ==
      umbra::detail::DirectedInteractionDeclarationStatus::applied);

  auto defaultQuery = registry.interactionTransportationTypeQueryFor(
      L"exercise", federateId, federateId, *interactionClass);
  REQUIRE(defaultQuery.has_value());
  REQUIRE(defaultQuery->transportationName == "HLAreliable");

  auto change = registry.planInteractionTransportationTypeChange(
      L"exercise", federateId, *interactionClass, "HLAbestEffort");
  REQUIRE(change.status ==
      umbra::detail::InteractionTransportationTypeChangeStatus::applied);
  auto committed = registry.beginInteractionTransportationTypeChange(
      L"exercise", federateId, *interactionClass);
  REQUIRE(committed.has_value());
  REQUIRE(*committed == "HLAbestEffort");

  auto overrideQuery = registry.interactionTransportationTypeQueryFor(
      L"exercise", federateId, federateId, *interactionClass);
  REQUIRE(overrideQuery.has_value());
  REQUIRE(overrideQuery->transportationName == "HLAbestEffort");
}

TEST_CASE(
    "Filesystem fresh-registry restore rebinds a pending object-instance Request Attribute Value Update",
    "[unit][kernel][federation-registry][save-restore][durable-save][filesystem][restore]"
    "[process-restart][process-restart-pending-attribute-value-update][pending-application-request-state][attribute-value-update]"
    "[object-management][object-lifecycle-state][application-value-state]") {
  auto const directory = temporarySaveCommitDirectory();
  std::error_code ignored;
  std::filesystem::remove_all(directory, ignored);
  auto store = std::make_shared<umbra::detail::FilesystemFederationSaveCommitStore>(
      directory);

  EmbeddedFederationRegistry source({}, store);
  REQUIRE(source.create(L"exercise", composedRestaurantDefinition()).status ==
      FederationRegistryStatus::applied);
  auto owner = source.join(
      L"exercise", L"publisher", L"owner", noOpCallbackRoute());
  auto requester = source.join(
      L"exercise", L"subscriber", L"requester", noOpCallbackRoute());
  REQUIRE(owner.membership);
  REQUIRE(requester.membership);

  auto const server = source.objectClassHandleFor(
      L"exercise", "HLAobjectRoot.Employee.Server");
  auto const efficiency = source.attributeHandleFor(
      L"exercise", "HLAobjectRoot.Employee.Server", "Efficiency");
  REQUIRE(server.has_value());
  REQUIRE(efficiency.has_value());
  std::set<std::uint64_t> const efficiencyOnly{*efficiency};
  REQUIRE(source.setObjectClassAttributePublication(
      L"exercise", owner.membership->id, *server, efficiencyOnly, true) ==
      umbra::detail::ObjectClassAttributeDeclarationStatus::applied);
  REQUIRE(source.setObjectClassAttributeSubscription(
      L"exercise", requester.membership->id, *server, efficiencyOnly, true) ==
      umbra::detail::ObjectClassAttributeDeclarationStatus::applied);

  auto registered = source.registerObjectInstance(
      L"exercise", owner.membership->id, *server);
  REQUIRE(registered.status ==
      umbra::detail::ObjectInstanceRegistrationStatus::applied);
  auto discoveries = source.planObjectInstanceDiscoveriesForInstance(
      L"exercise", registered.objectInstanceHandle);
  REQUIRE(discoveries.size() == 1U);
  REQUIRE(source.beginObjectInstanceDiscovery(
      L"exercise", requester.membership->id, registered.objectInstanceHandle));

  std::string const valueBytes{"\x73\x74", 2U};
  std::vector<std::pair<std::uint64_t, rti1516_2025::VariableLengthData>> values{{
      *efficiency,
      rti1516_2025::VariableLengthData(valueBytes.data(), valueBytes.size()),
  }};
  REQUIRE(source.recordSuccessfulUpdateAttributeValues(
      L"exercise",
      owner.membership->id,
      registered.objectInstanceHandle,
      *server,
      {"HLAreliable"},
      &values) == FederationRegistryStatus::applied);

  auto planned = source.planAttributeValueUpdateRequest(
      L"exercise",
      requester.membership->id,
      registered.objectInstanceHandle,
      efficiencyOnly);
  REQUIRE(planned.status ==
      umbra::detail::AttributeValueUpdateRequestStatus::applied);
  REQUIRE(planned.recipients.size() == 1U);
  REQUIRE(planned.recipients.front().providingFederateId == owner.membership->id);
  std::vector<unsigned char> const requestTag{'a', 'v', 'u', '-', 'r', 'e', 's', 't', 'a', 'r', 't'};
  auto requestId = source.registerAttributeValueUpdateRequest(
      L"exercise",
      requester.membership->id,
      owner.membership->id,
      registered.objectInstanceHandle,
      efficiencyOnly,
      requestTag);
  REQUIRE(requestId.has_value());

  std::wstring const saveLabel = L"pending-attribute-value-update-process-restart";
  REQUIRE(source.requestFederationSave(
      L"exercise", owner.membership->id, saveLabel).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(source.federateSaveBegun(
      L"exercise", owner.membership->id).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(source.federateSaveBegun(
      L"exercise", requester.membership->id).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE_FALSE(source.federateSaveComplete(
      L"exercise", owner.membership->id).saveCompletedSuccessfully);
  REQUIRE(source.federateSaveComplete(
      L"exercise", requester.membership->id).saveCompletedSuccessfully);

  auto committed = store->load(L"exercise", saveLabel);
  REQUIRE(committed.has_value());
  auto image = umbra::detail::FederationStateImageCodec::decode(
      committed->stateImage);
  REQUIRE(image.objects.size() == 1U);
  auto const& savedObject = image.objects.front();
  REQUIRE(savedObject.pendingAttributeValueUpdateRequests.size() == 1U);
  auto const& savedRequest =
      savedObject.pendingAttributeValueUpdateRequests.front();
  REQUIRE(savedRequest.requestId == *requestId);
  REQUIRE(savedRequest.requestingFederateId == requester.membership->id);
  REQUIRE(savedRequest.providingFederateId == owner.membership->id);
  REQUIRE(savedRequest.requestedAttributeHandles ==
      std::vector<std::uint64_t>{*efficiency});
  REQUIRE(savedRequest.userSuppliedTag ==
      std::string(requestTag.begin(), requestTag.end()));
  EmbeddedFederationRegistry restarted({}, store);
  REQUIRE(restarted.create(L"exercise", composedRestaurantDefinition()).status ==
      FederationRegistryStatus::applied);
  auto restartedOwner = restarted.join(
      L"exercise", L"publisher", L"owner", noOpCallbackRoute());
  auto restartedRequester = restarted.join(
      L"exercise", L"subscriber", L"requester", noOpCallbackRoute());
  REQUIRE(restartedOwner.membership);
  REQUIRE(restartedRequester.membership);
  REQUIRE(restartedOwner.membership->id == owner.membership->id);
  REQUIRE(restartedRequester.membership->id == requester.membership->id);

  REQUIRE(restarted.requestFederationRestore(
      L"exercise", restartedRequester.membership->id, saveLabel).status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  auto firstRestore = restarted.federateRestoreComplete(
      L"exercise", restartedOwner.membership->id);
  REQUIRE(firstRestore.status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  REQUIRE(firstRestore.attributeValueUpdateProvideWorkItems.empty());
  auto restored = restarted.federateRestoreComplete(
      L"exercise", restartedRequester.membership->id);
  REQUIRE(restored.status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  REQUIRE(restored.attributeValueUpdateProvideWorkItems.size() == 1U);
  auto const& restoredWork =
      restored.attributeValueUpdateProvideWorkItems.front();
  REQUIRE(restoredWork.requestId == *requestId);
  REQUIRE(restoredWork.requestingFederateId == restartedRequester.membership->id);
  REQUIRE(restoredWork.providingFederateId == restartedOwner.membership->id);
  REQUIRE(restoredWork.objectInstanceHandle == registered.objectInstanceHandle);
  REQUIRE(restoredWork.requestedAttributeHandles == efficiencyOnly);
  REQUIRE(restoredWork.userSuppliedTag == requestTag);

  auto provider = restarted.beginAttributeValueUpdateProvideRecipientFor(
      L"exercise",
      restoredWork.requestId,
      restoredWork.requestingFederateId,
      restoredWork.providingFederateId,
      restoredWork.objectInstanceHandle,
      restoredWork.requestedAttributeHandles);
  REQUIRE(provider.has_value());
  REQUIRE(provider->requestedAttributeHandles == efficiencyOnly);
  REQUIRE_FALSE(restarted.beginAttributeValueUpdateProvideRecipientFor(
      L"exercise",
      restoredWork.requestId,
      restoredWork.requestingFederateId,
      restoredWork.providingFederateId,
      restoredWork.objectInstanceHandle,
      restoredWork.requestedAttributeHandles));

  std::filesystem::remove_all(directory, ignored);
}

TEST_CASE(
    "Filesystem fresh-registry restore rebinds a pending object-class Request Attribute Value Update",
    "[unit][kernel][federation-registry][save-restore][durable-save][filesystem][restore]"
    "[process-restart][process-restart-class-pending-attribute-value-update][pending-application-request-state][attribute-value-update]"
    "[object-management][object-lifecycle-state][application-value-state]") {
  auto const directory = temporarySaveCommitDirectory();
  std::error_code ignored;
  std::filesystem::remove_all(directory, ignored);
  auto store = std::make_shared<umbra::detail::FilesystemFederationSaveCommitStore>(
      directory);

  EmbeddedFederationRegistry source({}, store);
  REQUIRE(source.create(L"exercise", composedRestaurantDefinition()).status ==
      FederationRegistryStatus::applied);
  auto owner = source.join(
      L"exercise", L"publisher", L"owner", noOpCallbackRoute());
  auto requester = source.join(
      L"exercise", L"subscriber", L"requester", noOpCallbackRoute());
  REQUIRE(owner.membership);
  REQUIRE(requester.membership);

  auto const server = source.objectClassHandleFor(
      L"exercise", "HLAobjectRoot.Employee.Server");
  auto const efficiency = source.attributeHandleFor(
      L"exercise", "HLAobjectRoot.Employee.Server", "Efficiency");
  REQUIRE(server.has_value());
  REQUIRE(efficiency.has_value());
  std::set<std::uint64_t> const efficiencyOnly{*efficiency};
  REQUIRE(source.setObjectClassAttributePublication(
      L"exercise", owner.membership->id, *server, efficiencyOnly, true) ==
      umbra::detail::ObjectClassAttributeDeclarationStatus::applied);
  auto registered = source.registerObjectInstance(
      L"exercise", owner.membership->id, *server);
  REQUIRE(registered.status ==
      umbra::detail::ObjectInstanceRegistrationStatus::applied);

  std::string const valueBytes{"xy", 2U};
  std::vector<std::pair<std::uint64_t, rti1516_2025::VariableLengthData>> values{{
      *efficiency,
      rti1516_2025::VariableLengthData(valueBytes.data(), valueBytes.size()),
  }};
  REQUIRE(source.recordSuccessfulUpdateAttributeValues(
      L"exercise",
      owner.membership->id,
      registered.objectInstanceHandle,
      *server,
      {"HLAreliable"},
      &values) == FederationRegistryStatus::applied);

  auto planned = source.planAttributeValueUpdateClassRequest(
      L"exercise",
      requester.membership->id,
      *server,
      efficiencyOnly);
  REQUIRE(planned.status ==
      umbra::detail::AttributeValueUpdateClassRequestStatus::applied);
  REQUIRE(planned.recipients.size() == 1U);
  REQUIRE(planned.recipients.front().objectInstanceHandle ==
      registered.objectInstanceHandle);
  REQUIRE(planned.recipients.front().providingFederateId == owner.membership->id);
  std::vector<unsigned char> const requestTag{'c', 'l', 'a', 's', 's'};
  auto requestId = source.registerAttributeValueUpdateClassRequest(
      L"exercise",
      requester.membership->id,
      owner.membership->id,
      registered.objectInstanceHandle,
      *server,
      efficiencyOnly,
      requestTag);
  REQUIRE(requestId.has_value());

  std::wstring const saveLabel = L"pending-class-attribute-value-update-process-restart";
  REQUIRE(source.requestFederationSave(
      L"exercise", owner.membership->id, saveLabel).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(source.federateSaveBegun(
      L"exercise", owner.membership->id).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(source.federateSaveBegun(
      L"exercise", requester.membership->id).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE_FALSE(source.federateSaveComplete(
      L"exercise", owner.membership->id).saveCompletedSuccessfully);
  REQUIRE(source.federateSaveComplete(
      L"exercise", requester.membership->id).saveCompletedSuccessfully);

  auto committed = store->load(L"exercise", saveLabel);
  REQUIRE(committed.has_value());
  auto image = umbra::detail::FederationStateImageCodec::decode(
      committed->stateImage);
  REQUIRE(image.objects.size() == 1U);
  auto const& savedObject = image.objects.front();
  REQUIRE(savedObject.pendingAttributeValueUpdateClassRequests.size() == 1U);
  auto const& savedRequest =
      savedObject.pendingAttributeValueUpdateClassRequests.front();
  REQUIRE(savedRequest.requestId == *requestId);
  REQUIRE(savedRequest.requestingFederateId == requester.membership->id);
  REQUIRE(savedRequest.providingFederateId == owner.membership->id);
  REQUIRE(savedRequest.requestedObjectClassHandle == *server);
  REQUIRE(savedRequest.requestedAttributeHandles ==
      std::vector<std::uint64_t>{*efficiency});
  REQUIRE(savedRequest.userSuppliedTag ==
      std::string(requestTag.begin(), requestTag.end()));

  EmbeddedFederationRegistry restarted({}, store);
  REQUIRE(restarted.create(L"exercise", composedRestaurantDefinition()).status ==
      FederationRegistryStatus::applied);
  auto restartedOwner = restarted.join(
      L"exercise", L"publisher", L"owner", noOpCallbackRoute());
  auto restartedRequester = restarted.join(
      L"exercise", L"subscriber", L"requester", noOpCallbackRoute());
  REQUIRE(restartedOwner.membership);
  REQUIRE(restartedRequester.membership);
  REQUIRE(restartedOwner.membership->id == owner.membership->id);
  REQUIRE(restartedRequester.membership->id == requester.membership->id);

  REQUIRE(restarted.requestFederationRestore(
      L"exercise", restartedRequester.membership->id, saveLabel).status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  auto firstRestore = restarted.federateRestoreComplete(
      L"exercise", restartedOwner.membership->id);
  REQUIRE(firstRestore.status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  REQUIRE(firstRestore.attributeValueUpdateClassProvideWorkItems.empty());
  auto restored = restarted.federateRestoreComplete(
      L"exercise", restartedRequester.membership->id);
  REQUIRE(restored.status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  REQUIRE(restored.attributeValueUpdateClassProvideWorkItems.size() == 1U);
  auto const& restoredWork =
      restored.attributeValueUpdateClassProvideWorkItems.front();
  REQUIRE(restoredWork.requestId == *requestId);
  REQUIRE(restoredWork.requestingFederateId == restartedRequester.membership->id);
  REQUIRE(restoredWork.providingFederateId == restartedOwner.membership->id);
  REQUIRE(restoredWork.objectInstanceHandle == registered.objectInstanceHandle);
  REQUIRE(restoredWork.requestedObjectClassHandle == *server);
  REQUIRE(restoredWork.requestedAttributeHandles == efficiencyOnly);
  REQUIRE(restoredWork.userSuppliedTag == requestTag);

  auto provider = restarted.beginAttributeValueUpdateClassProvideRecipientFor(
      L"exercise",
      restoredWork.requestId,
      restoredWork.requestingFederateId,
      restoredWork.providingFederateId,
      restoredWork.objectInstanceHandle,
      restoredWork.requestedObjectClassHandle,
      restoredWork.requestedAttributeHandles);
  REQUIRE(provider.has_value());
  REQUIRE(provider->requestedAttributeHandles == efficiencyOnly);
  REQUIRE_FALSE(restarted.beginAttributeValueUpdateClassProvideRecipientFor(
      L"exercise",
      restoredWork.requestId,
      restoredWork.requestingFederateId,
      restoredWork.providingFederateId,
      restoredWork.objectInstanceHandle,
      restoredWork.requestedObjectClassHandle,
      restoredWork.requestedAttributeHandles));

  std::filesystem::remove_all(directory, ignored);
}

TEST_CASE(
    "Filesystem federation save commits publish unique durable envelopes",
    "[unit][kernel][federation-registry][save-restore][durable-save][filesystem]") {
  auto const directory = temporarySaveCommitDirectory();
  std::error_code ignored;
  std::filesystem::remove_all(directory, ignored);
  umbra::detail::FilesystemFederationSaveCommitStore store(directory);
  umbra::detail::FederationSaveCommitDescriptor descriptor{
      L"exercise / unsafe",
      L"checkpoint \"one\"",
      L"HLAinteger64Time",
      {7U, 42U},
      true};
  umbra::detail::FederationStateImage stateImage;
  stateImage.federationName = descriptor.federationName;
  stateImage.logicalTimeImplementationName = descriptor.logicalTimeImplementationName;
  stateImage.members = {{7U, L"alice", L"trainer", 0U, 0U, 0, 0U}};
  descriptor.stateImage = umbra::detail::FederationStateImageCodec::encode(stateImage);

  REQUIRE_NOTHROW(store.commit(descriptor));
  REQUIRE_NOTHROW(store.commit(descriptor));
  REQUIRE(std::filesystem::is_directory(directory));

  std::vector<std::filesystem::path> manifests;
  for (auto const& entry : std::filesystem::directory_iterator(directory)) {
    if (entry.is_regular_file() && entry.path().extension() == ".json") {
      manifests.push_back(entry.path());
    }
  }
  REQUIRE(manifests.size() == 2U);
  std::ifstream input(manifests.front(), std::ios::binary);
  std::string contents{std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
  REQUIRE(contents.find("umbra-federation-save-commit/v1") != std::string::npos);
  REQUIRE(contents.find("exercise / unsafe") != std::string::npos);
  REQUIRE(contents.find("checkpoint \\\"one\\\"") != std::string::npos);
  REQUIRE(contents.find("\"timed\": true") != std::string::npos);
  REQUIRE(contents.find("\"memberFederateIds\": [7, 42]") != std::string::npos);

  auto loaded = store.load(L"exercise / unsafe", L"checkpoint \"one\"");
  REQUIRE(loaded.has_value());
  REQUIRE(loaded->logicalTimeImplementationName == L"HLAinteger64Time");
  REQUIRE(loaded->memberFederateIds == std::vector<std::uint64_t>{7U, 42U});
  REQUIRE(loaded->timed);
  REQUIRE(loaded->stateImage == descriptor.stateImage);
  REQUIRE(umbra::detail::FederationStateImageCodec::decode(loaded->stateImage)
      .federationName == descriptor.federationName);
  REQUIRE_FALSE(store.load(L"missing", L"checkpoint").has_value());

  std::filesystem::remove_all(directory, ignored);
}

TEST_CASE(
    "Filesystem state image restores a by-ownership directed interaction and follows target ownership handoff",
    "[unit][kernel][federation-registry][save-restore][durable-save][filesystem][restore]"
    "[process-restart][process-restart-directed-interaction-ownership-handoff][interaction-declaration-state]"
    "[directed-interaction][directed-routing][ownership-ledger-state][object-visibility-state]"
    "[application-value-state][ownership-management][declaration-management]") {
  auto const directory = temporarySaveCommitDirectory();
  std::error_code ignored;
  std::filesystem::remove_all(directory, ignored);
  auto store = std::make_shared<umbra::detail::FilesystemFederationSaveCommitStore>(
      directory);

  EmbeddedFederationRegistry source({}, store);
  REQUIRE(source.create(L"exercise", composedDirectedInteractionDefinition()).status ==
      FederationRegistryStatus::applied);
  auto publisher = source.join(
      L"exercise", L"publisher", L"publisher", noOpCallbackRoute());
  auto initialOwner = source.join(
      L"exercise", L"owner", L"initial-owner", noOpCallbackRoute());
  auto handoffOwner = source.join(
      L"exercise", L"owner", L"handoff-owner", noOpCallbackRoute());
  REQUIRE(publisher.status == FederationRegistryStatus::applied);
  REQUIRE(initialOwner.status == FederationRegistryStatus::applied);
  REQUIRE(handoffOwner.status == FederationRegistryStatus::applied);
  REQUIRE(publisher.membership);
  REQUIRE(initialOwner.membership);
  REQUIRE(handoffOwner.membership);
  auto const publisherId = publisher.membership->id;
  auto const initialOwnerId = initialOwner.membership->id;
  auto const handoffOwnerId = handoffOwner.membership->id;

  auto const objectClass = source.objectClassHandleFor(
      L"exercise", "HLAobjectRoot.UmbraDirectedFixtureObject");
  auto const interactionClass = source.interactionClassHandleFor(
      L"exercise", "HLAinteractionRoot.UmbraDirectedFixtureInteraction");
  auto const marker = source.attributeHandleFor(
      L"exercise", "HLAobjectRoot.UmbraDirectedFixtureObject", "DirectedTargetMarker");
  auto const privilegeToDelete = source.attributeHandleFor(
      L"exercise", "HLAobjectRoot.UmbraDirectedFixtureObject",
      "HLAprivilegeToDeleteObject");
  REQUIRE(objectClass.has_value());
  REQUIRE(interactionClass.has_value());
  REQUIRE(marker.has_value());
  REQUIRE(privilegeToDelete.has_value());

  std::set<std::uint64_t> const markerOnly{*marker};
  std::set<std::uint64_t> const ownershipTargetAttributes{
      *marker, *privilegeToDelete};
  std::set<std::uint64_t> const directedOnly{*interactionClass};
  REQUIRE(source.setObjectClassAttributePublication(
      L"exercise", initialOwnerId, *objectClass, markerOnly, true) ==
      umbra::detail::ObjectClassAttributeDeclarationStatus::applied);
  REQUIRE(source.setObjectClassAttributePublication(
      L"exercise", handoffOwnerId, *objectClass, markerOnly, true) ==
      umbra::detail::ObjectClassAttributeDeclarationStatus::applied);
  REQUIRE(source.setObjectClassAttributeSubscription(
      L"exercise", publisherId, *objectClass, markerOnly, true) ==
      umbra::detail::ObjectClassAttributeDeclarationStatus::applied);
  REQUIRE(source.setObjectClassAttributeSubscription(
      L"exercise", handoffOwnerId, *objectClass, markerOnly, true) ==
      umbra::detail::ObjectClassAttributeDeclarationStatus::applied);
  REQUIRE(source.publishObjectClassDirectedInteractions(
      L"exercise", publisherId, *objectClass, directedOnly) ==
      umbra::detail::DirectedInteractionDeclarationStatus::applied);
  REQUIRE(source.subscribeObjectClassDirectedInteractions(
      L"exercise", initialOwnerId, *objectClass, directedOnly, false) ==
      umbra::detail::DirectedInteractionDeclarationStatus::applied);
  REQUIRE(source.subscribeObjectClassDirectedInteractions(
      L"exercise", handoffOwnerId, *objectClass, directedOnly, false) ==
      umbra::detail::DirectedInteractionDeclarationStatus::applied);

  auto registered = source.registerObjectInstance(
      L"exercise", initialOwnerId, *objectClass);
  REQUIRE(registered.status ==
      umbra::detail::ObjectInstanceRegistrationStatus::applied);
  REQUIRE(registered.objectInstanceHandle != 0U);
  auto discoveries = source.planObjectInstanceDiscoveriesForInstance(
      L"exercise", registered.objectInstanceHandle);
  REQUIRE(discoveries.size() == 2U);
  std::set<std::uint64_t> discoveredFederates;
  for (auto const& discovery : discoveries) {
    discoveredFederates.insert(discovery.receivingFederateId);
  }
  REQUIRE(discoveredFederates == std::set<std::uint64_t>{publisherId, handoffOwnerId});
  REQUIRE(source.beginObjectInstanceDiscovery(
      L"exercise", publisherId, registered.objectInstanceHandle).has_value());
  REQUIRE(source.beginObjectInstanceDiscovery(
      L"exercise", handoffOwnerId, registered.objectInstanceHandle).has_value());

  std::string const markerValue{"directed-owner-handoff", 22U};
  std::vector<std::pair<std::uint64_t, rti1516_2025::VariableLengthData>> values{{
      *marker,
      rti1516_2025::VariableLengthData(markerValue.data(), markerValue.size()),
  }};
  REQUIRE(source.recordSuccessfulUpdateAttributeValues(
      L"exercise", initialOwnerId, registered.objectInstanceHandle, *objectClass,
      {"HLAreliable"}, &values) == FederationRegistryStatus::applied);

  auto sourcePlan = source.planReceiveOrderDirectedInteraction(
      L"exercise", publisherId, registered.objectInstanceHandle,
      *interactionClass, {});
  REQUIRE(sourcePlan.status ==
      umbra::detail::ReceiveOrderDirectedInteractionStatus::applied);
  REQUIRE(sourcePlan.recipients.size() == 1U);
  REQUIRE(sourcePlan.recipients.front().federateId == initialOwnerId);

  std::wstring const saveLabel =
      L"directed-interaction-ownership-handoff-process-restart";
  REQUIRE(source.requestFederationSave(
      L"exercise", publisherId, saveLabel).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(source.federateSaveBegun(
      L"exercise", publisherId).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(source.federateSaveBegun(
      L"exercise", initialOwnerId).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(source.federateSaveBegun(
      L"exercise", handoffOwnerId).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE_FALSE(source.federateSaveComplete(
      L"exercise", publisherId).saveCompletedSuccessfully);
  REQUIRE_FALSE(source.federateSaveComplete(
      L"exercise", initialOwnerId).saveCompletedSuccessfully);
  REQUIRE(source.federateSaveComplete(
      L"exercise", handoffOwnerId).saveCompletedSuccessfully);

  auto committed = store->load(L"exercise", saveLabel);
  REQUIRE(committed.has_value());
  auto image = umbra::detail::FederationStateImageCodec::decode(
      committed->stateImage);
  REQUIRE(image.objects.size() == 1U);
  REQUIRE(image.objects.front().knownObjectClassHandlesByFederate.size() == 3U);
  REQUIRE(image.objects.front().attributeValuesPresent);
  auto const savedMarker = std::find_if(
      image.objects.front().attributes.begin(),
      image.objects.front().attributes.end(),
      [marker](auto const& attribute) { return attribute.handle == *marker; });
  REQUIRE(savedMarker != image.objects.front().attributes.end());
  REQUIRE(savedMarker->ownerFederateId == initialOwnerId);
  REQUIRE(image.interactionDeclarations.size() == 3U);
  REQUIRE(image.interactionDeclarations[0]
              .publishedObjectClassDirectedInteractions.size() == 1U);
  REQUIRE(image.interactionDeclarations[1]
              .subscribedObjectClassDirectedInteractions.size() == 1U);
  REQUIRE(image.interactionDeclarations[2]
              .subscribedObjectClassDirectedInteractions.size() == 1U);

  EmbeddedFederationRegistry restarted({}, store);
  REQUIRE(restarted.create(L"exercise", composedDirectedInteractionDefinition()).status ==
      FederationRegistryStatus::applied);
  auto restartedPublisher = restarted.join(
      L"exercise", L"publisher", L"publisher", noOpCallbackRoute());
  auto restartedInitialOwner = restarted.join(
      L"exercise", L"owner", L"initial-owner", noOpCallbackRoute());
  auto restartedHandoffOwner = restarted.join(
      L"exercise", L"owner", L"handoff-owner", noOpCallbackRoute());
  REQUIRE(restartedPublisher.status == FederationRegistryStatus::applied);
  REQUIRE(restartedInitialOwner.status == FederationRegistryStatus::applied);
  REQUIRE(restartedHandoffOwner.status == FederationRegistryStatus::applied);
  REQUIRE(restartedPublisher.membership);
  REQUIRE(restartedInitialOwner.membership);
  REQUIRE(restartedHandoffOwner.membership);
  REQUIRE(restartedPublisher.membership->id == publisherId);
  REQUIRE(restartedInitialOwner.membership->id == initialOwnerId);
  REQUIRE(restartedHandoffOwner.membership->id == handoffOwnerId);
  REQUIRE(restarted.requestFederationRestore(
      L"exercise", publisherId, saveLabel).status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  REQUIRE(restarted.federateRestoreComplete(
      L"exercise", publisherId).status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  REQUIRE(restarted.federateRestoreComplete(
      L"exercise", initialOwnerId).status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  auto restored = restarted.federateRestoreComplete(
      L"exercise", handoffOwnerId);
  REQUIRE(restored.status ==
      umbra::detail::FederationRestoreControlStatus::applied);

  auto restoredInitialOwnerState = restarted.attributeOwnedByFederate(
      L"exercise", initialOwnerId, registered.objectInstanceHandle, *marker);
  REQUIRE(restoredInitialOwnerState.status ==
      umbra::detail::AttributeOwnershipCheckStatus::applied);
  REQUIRE(restoredInitialOwnerState.ownedByRequestingFederate);
  auto restoredHandoffOwnerState = restarted.attributeOwnedByFederate(
      L"exercise", handoffOwnerId, registered.objectInstanceHandle, *marker);
  REQUIRE(restoredHandoffOwnerState.status ==
      umbra::detail::AttributeOwnershipCheckStatus::applied);
  REQUIRE_FALSE(restoredHandoffOwnerState.ownedByRequestingFederate);

  auto restoredBeforeHandoff = restarted.planReceiveOrderDirectedInteraction(
      L"exercise", publisherId, registered.objectInstanceHandle,
      *interactionClass, {});
  REQUIRE(restoredBeforeHandoff.status ==
      umbra::detail::ReceiveOrderDirectedInteractionStatus::applied);
  REQUIRE(restoredBeforeHandoff.recipients.size() == 1U);
  REQUIRE(restoredBeforeHandoff.recipients.front().federateId == initialOwnerId);

  std::vector<unsigned char> const acquisitionTag{
      'o', 'w', 'n', 'e', 'r', '-', 'h', 'a', 'n', 'd', 'o', 'f', 'f'};
  auto acquisition = restarted.planAttributeOwnershipAcquisition(
      L"exercise", handoffOwnerId, registered.objectInstanceHandle,
      ownershipTargetAttributes, acquisitionTag);
  REQUIRE(acquisition.status ==
      umbra::detail::AttributeOwnershipAcquisitionStatus::applied);
  REQUIRE(acquisition.workItems.size() == 1U);
  auto const& releaseWork = acquisition.workItems.front();
  REQUIRE(releaseWork.kind ==
      umbra::detail::AttributeOwnershipAcquisitionWorkKind::request_release);
  REQUIRE(releaseWork.requestingFederateId == handoffOwnerId);
  REQUIRE(releaseWork.receivingFederateId == initialOwnerId);
  REQUIRE(releaseWork.attributeHandles == ownershipTargetAttributes);
  auto releaseDelivery = restarted.beginAttributeOwnershipAcquisitionRelease(
      L"exercise", handoffOwnerId, initialOwnerId,
      registered.objectInstanceHandle, releaseWork.requestId,
      releaseWork.attributeHandles);
  REQUIRE(releaseDelivery.has_value());
  REQUIRE(releaseDelivery->candidateAttributeHandles == ownershipTargetAttributes);

  std::vector<unsigned char> const divestitureTag{
      'd', 'i', 'r', 'e', 'c', 't', 'e', 'd', '-', 'h', 'a', 'n', 'd', 'o', 'f', 'f'};
  auto divestiture = restarted.planAttributeOwnershipDivestitureIfWanted(
      L"exercise", initialOwnerId, registered.objectInstanceHandle,
      ownershipTargetAttributes, divestitureTag);
  REQUIRE(divestiture.status ==
      umbra::detail::AttributeOwnershipDivestitureIfWantedStatus::applied);
  REQUIRE(divestiture.divestedAttributeHandles == ownershipTargetAttributes);
  REQUIRE(divestiture.notifications.size() == 1U);
  auto const& notification = divestiture.notifications.front();
  REQUIRE(notification.receivingFederateId == handoffOwnerId);
  REQUIRE(notification.attributeHandles == ownershipTargetAttributes);
  REQUIRE(notification.userSuppliedTag == divestitureTag);
  auto notificationDelivery =
      restarted.beginAttributeOwnershipDivestitureIfWantedNotification(
          L"exercise", handoffOwnerId, registered.objectInstanceHandle,
          notification.notificationId, ownershipTargetAttributes);
  REQUIRE(notificationDelivery.has_value());
  REQUIRE(notificationDelivery->securedAttributeHandles == ownershipTargetAttributes);
  REQUIRE(notificationDelivery->followupWorkItems.empty());

  auto handedOffInitialOwnerState = restarted.attributeOwnedByFederate(
      L"exercise", initialOwnerId, registered.objectInstanceHandle, *marker);
  REQUIRE(handedOffInitialOwnerState.status ==
      umbra::detail::AttributeOwnershipCheckStatus::applied);
  REQUIRE_FALSE(handedOffInitialOwnerState.ownedByRequestingFederate);
  auto handedOffOwnerState = restarted.attributeOwnedByFederate(
      L"exercise", handoffOwnerId, registered.objectInstanceHandle, *marker);
  REQUIRE(handedOffOwnerState.status ==
      umbra::detail::AttributeOwnershipCheckStatus::applied);
  REQUIRE(handedOffOwnerState.ownedByRequestingFederate);

  auto afterHandoff = restarted.planReceiveOrderDirectedInteraction(
      L"exercise", publisherId, registered.objectInstanceHandle,
      *interactionClass, {});
  REQUIRE(afterHandoff.status ==
      umbra::detail::ReceiveOrderDirectedInteractionStatus::applied);
  REQUIRE(afterHandoff.recipients.size() == 1U);
  REQUIRE(afterHandoff.recipients.front().federateId == handoffOwnerId);
  REQUIRE(afterHandoff.recipients.front().objectInstanceHandle ==
      registered.objectInstanceHandle);
  REQUIRE(afterHandoff.recipients.front().receivedInteractionClassHandle ==
      *interactionClass);

  std::wstring const roundTripLabel =
      L"directed-interaction-ownership-handoff-round-trip";
  REQUIRE(restarted.requestFederationSave(
      L"exercise", publisherId, roundTripLabel).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(restarted.federateSaveBegun(
      L"exercise", publisherId).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(restarted.federateSaveBegun(
      L"exercise", initialOwnerId).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(restarted.federateSaveBegun(
      L"exercise", handoffOwnerId).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE_FALSE(restarted.federateSaveComplete(
      L"exercise", publisherId).saveCompletedSuccessfully);
  REQUIRE_FALSE(restarted.federateSaveComplete(
      L"exercise", initialOwnerId).saveCompletedSuccessfully);
  REQUIRE(restarted.federateSaveComplete(
      L"exercise", handoffOwnerId).saveCompletedSuccessfully);
  auto roundTrip = store->load(L"exercise", roundTripLabel);
  REQUIRE(roundTrip.has_value());
  auto roundTripImage = umbra::detail::FederationStateImageCodec::decode(
      roundTrip->stateImage);
  REQUIRE(roundTripImage.objects.size() == 1U);
  auto const roundTripMarker = std::find_if(
      roundTripImage.objects.front().attributes.begin(),
      roundTripImage.objects.front().attributes.end(),
      [marker](auto const& attribute) { return attribute.handle == *marker; });
  REQUIRE(roundTripMarker != roundTripImage.objects.front().attributes.end());
  REQUIRE(roundTripMarker->ownerFederateId == handoffOwnerId);
  REQUIRE(roundTripImage.interactionDeclarations.size() == 3U);

  std::filesystem::remove_all(directory, ignored);
}

TEST_CASE(
    "Filesystem fresh-registry directed TSO rechecks target ownership before callback and retracts suppressed delivery",
    "[unit][kernel][federation-registry][save-restore][durable-save][filesystem][restore]"
    "[process-restart][process-restart-directed-interaction-tso-ownership-callback]"
    "[tso-directed-interaction-state][tso-retraction-ledger-state][directed-interaction]"
    "[directed-routing][ownership-ledger-state][object-visibility-state]"
    "[ownership-management][time-management]") {
  auto const directory = temporarySaveCommitDirectory();
  std::error_code ignored;
  std::filesystem::remove_all(directory, ignored);
  auto store = std::make_shared<umbra::detail::FilesystemFederationSaveCommitStore>(
      directory);

  EmbeddedFederationRegistry source({}, store);
  REQUIRE(source.create(L"exercise", composedDirectedInteractionDefinition()).status ==
      FederationRegistryStatus::applied);
  auto publisher = source.join(
      L"exercise", L"publisher", L"publisher", noOpCallbackRoute());
  auto initialOwner = source.join(
      L"exercise", L"owner", L"initial-owner", noOpCallbackRoute());
  auto handoffOwner = source.join(
      L"exercise", L"owner", L"handoff-owner", noOpCallbackRoute());
  REQUIRE(publisher.membership);
  REQUIRE(initialOwner.membership);
  REQUIRE(handoffOwner.membership);
  auto const publisherId = publisher.membership->id;
  auto const initialOwnerId = initialOwner.membership->id;
  auto const handoffOwnerId = handoffOwner.membership->id;

  auto const objectClass = source.objectClassHandleFor(
      L"exercise", "HLAobjectRoot.UmbraDirectedFixtureObject");
  auto const interactionClass = source.interactionClassHandleFor(
      L"exercise", "HLAinteractionRoot.UmbraDirectedFixtureInteraction");
  auto const marker = source.attributeHandleFor(
      L"exercise", "HLAobjectRoot.UmbraDirectedFixtureObject", "DirectedTargetMarker");
  auto const privilegeToDelete = source.attributeHandleFor(
      L"exercise", "HLAobjectRoot.UmbraDirectedFixtureObject",
      "HLAprivilegeToDeleteObject");
  REQUIRE(objectClass);
  REQUIRE(interactionClass);
  REQUIRE(marker);
  REQUIRE(privilegeToDelete);

  std::set<std::uint64_t> const markerOnly{*marker};
  std::set<std::uint64_t> const ownershipTargetAttributes{
      *marker, *privilegeToDelete};
  std::set<std::uint64_t> const directedOnly{*interactionClass};
  REQUIRE(source.setObjectClassAttributePublication(
      L"exercise", initialOwnerId, *objectClass, markerOnly, true) ==
      umbra::detail::ObjectClassAttributeDeclarationStatus::applied);
  REQUIRE(source.setObjectClassAttributePublication(
      L"exercise", handoffOwnerId, *objectClass, markerOnly, true) ==
      umbra::detail::ObjectClassAttributeDeclarationStatus::applied);
  REQUIRE(source.setObjectClassAttributeSubscription(
      L"exercise", publisherId, *objectClass, markerOnly, true) ==
      umbra::detail::ObjectClassAttributeDeclarationStatus::applied);
  REQUIRE(source.setObjectClassAttributeSubscription(
      L"exercise", handoffOwnerId, *objectClass, markerOnly, true) ==
      umbra::detail::ObjectClassAttributeDeclarationStatus::applied);
  REQUIRE(source.publishObjectClassDirectedInteractions(
      L"exercise", publisherId, *objectClass, directedOnly) ==
      umbra::detail::DirectedInteractionDeclarationStatus::applied);
  REQUIRE(source.subscribeObjectClassDirectedInteractions(
      L"exercise", initialOwnerId, *objectClass, directedOnly, false) ==
      umbra::detail::DirectedInteractionDeclarationStatus::applied);
  REQUIRE(source.subscribeObjectClassDirectedInteractions(
      L"exercise", handoffOwnerId, *objectClass, directedOnly, false) ==
      umbra::detail::DirectedInteractionDeclarationStatus::applied);

  auto registered = source.registerObjectInstance(
      L"exercise", initialOwnerId, *objectClass);
  REQUIRE(registered.status ==
      umbra::detail::ObjectInstanceRegistrationStatus::applied);
  REQUIRE(registered.objectInstanceHandle != 0U);
  auto discoveries = source.planObjectInstanceDiscoveriesForInstance(
      L"exercise", registered.objectInstanceHandle);
  // Both the publisher and the future handoff owner are subscribed to the
  // registered class's marker attribute. Object discovery is driven by
  // object-class attribute subscriptions, independently of the directed
  // interaction's by-ownership selector, so each joined federate receives one
  // discovery candidate before ownership changes.
  REQUIRE(discoveries.size() == 2U);
  std::set<std::uint64_t> discoveredFederates;
  for (auto const& discovery : discoveries) {
    discoveredFederates.insert(discovery.receivingFederateId);
  }
  REQUIRE(discoveredFederates == std::set<std::uint64_t>{publisherId, handoffOwnerId});
  REQUIRE(source.beginObjectInstanceDiscovery(
      L"exercise", publisherId, registered.objectInstanceHandle));
  REQUIRE(source.beginObjectInstanceDiscovery(
      L"exercise", handoffOwnerId, registered.objectInstanceHandle));

  std::string const markerValue{"directed-tso-ownership", 22U};
  std::vector<std::pair<std::uint64_t, rti1516_2025::VariableLengthData>> values{{
      *marker,
      rti1516_2025::VariableLengthData(markerValue.data(), markerValue.size()),
  }};
  REQUIRE(source.recordSuccessfulUpdateAttributeValues(
      L"exercise", initialOwnerId, registered.objectInstanceHandle, *objectClass,
      {"HLAreliable"}, &values) == FederationRegistryStatus::applied);

  auto sourcePlan = source.planReceiveOrderDirectedInteraction(
      L"exercise", publisherId, registered.objectInstanceHandle,
      *interactionClass, {});
  REQUIRE(sourcePlan.status ==
      umbra::detail::ReceiveOrderDirectedInteractionStatus::applied);
  REQUIRE(sourcePlan.recipients.size() == 1U);
  REQUIRE(sourcePlan.recipients.front().federateId == initialOwnerId);

  std::string const tagBytes{"ownership-tso", 12U};
  umbra::detail::TsoDirectedInteractionMessage message;
  message.producingFederateId = publisherId;
  message.objectInstanceHandle = registered.objectInstanceHandle;
  message.sentInteractionClassHandle = *interactionClass;
  message.userSuppliedTag = rti1516_2025::VariableLengthData(
      tagBytes.data(), tagBytes.size());
  message.transportationName = "HLAreliable";
  message.timestamp = std::make_shared<rti1516_2025::HLAinteger64Time>(5);
  auto const& selected = sourcePlan.recipients.front();
  message.recipients.push_back({
      selected.federateId,
      selected.objectInstanceHandle,
      selected.receivedInteractionClassHandle,
      selected.receivedParameterHandles,
      selected.callbackRoute});
  auto const enqueued = source.enqueueTsoDirectedInteraction(
      L"exercise", std::move(message), {initialOwnerId});
  REQUIRE(enqueued.status ==
      umbra::detail::FederationTsoRegistryStatus::applied);
  REQUIRE(enqueued.queueStatus ==
      umbra::detail::TsoMessageQueueStatus::applied);
  REQUIRE(enqueued.enqueuedRecipientCount == 1U);

  auto saveAll = [&](EmbeddedFederationRegistry& registry,
                     std::wstring const& label) {
    REQUIRE(registry.requestFederationSave(
        L"exercise", publisherId, label).status ==
        umbra::detail::FederationSaveControlStatus::applied);
    REQUIRE(registry.federateSaveBegun(
        L"exercise", publisherId).status ==
        umbra::detail::FederationSaveControlStatus::applied);
    REQUIRE(registry.federateSaveBegun(
        L"exercise", initialOwnerId).status ==
        umbra::detail::FederationSaveControlStatus::applied);
    REQUIRE(registry.federateSaveBegun(
        L"exercise", handoffOwnerId).status ==
        umbra::detail::FederationSaveControlStatus::applied);
    REQUIRE_FALSE(registry.federateSaveComplete(
        L"exercise", publisherId).saveCompletedSuccessfully);
    REQUIRE_FALSE(registry.federateSaveComplete(
        L"exercise", initialOwnerId).saveCompletedSuccessfully);
    REQUIRE(registry.federateSaveComplete(
        L"exercise", handoffOwnerId).saveCompletedSuccessfully);
  };

  std::wstring const saveLabel =
      L"directed-interaction-tso-ownership-callback-process-restart";
  saveAll(source, saveLabel);
  auto committed = store->load(L"exercise", saveLabel);
  REQUIRE(committed);
  auto image = umbra::detail::FederationStateImageCodec::decode(
      committed->stateImage);
  REQUIRE(image.tsoDirectedInteractionMessages.size() == 1U);
  REQUIRE(image.tsoDirectedInteractionMessages.front().messageId ==
      enqueued.messageId);
  REQUIRE(image.tsoDirectedInteractionMessages.front().recipients.size() == 1U);
  REQUIRE(image.tsoRequestRetractionRecords.size() == 1U);
  REQUIRE(image.tsoRequestRetractionRecords.front().recipientStates.size() == 1U);
  REQUIRE(image.tsoRequestRetractionRecords.front().recipientStates.front().state == 0U);
  REQUIRE(image.tsoQueueEntries.size() == 1U);
  REQUIRE(image.tsoQueueEntries.front().phase == 0U);

  EmbeddedFederationRegistry restarted({}, store);
  REQUIRE(restarted.create(L"exercise", composedDirectedInteractionDefinition()).status ==
      FederationRegistryStatus::applied);
  auto restartedPublisher = restarted.join(
      L"exercise", L"publisher", L"publisher", noOpCallbackRoute());
  auto restartedInitialOwner = restarted.join(
      L"exercise", L"owner", L"initial-owner", noOpCallbackRoute());
  auto restartedHandoffOwner = restarted.join(
      L"exercise", L"owner", L"handoff-owner", noOpCallbackRoute());
  REQUIRE(restartedPublisher.membership);
  REQUIRE(restartedInitialOwner.membership);
  REQUIRE(restartedHandoffOwner.membership);
  REQUIRE(restartedPublisher.membership->id == publisherId);
  REQUIRE(restartedInitialOwner.membership->id == initialOwnerId);
  REQUIRE(restartedHandoffOwner.membership->id == handoffOwnerId);
  REQUIRE(restarted.requestFederationRestore(
      L"exercise", publisherId, saveLabel).status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  REQUIRE(restarted.federateRestoreComplete(
      L"exercise", publisherId).status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  REQUIRE(restarted.federateRestoreComplete(
      L"exercise", initialOwnerId).status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  REQUIRE(restarted.federateRestoreComplete(
      L"exercise", handoffOwnerId).status ==
      umbra::detail::FederationRestoreControlStatus::applied);

  auto const restartedObjectClass = restarted.objectClassHandleFor(
      L"exercise", "HLAobjectRoot.UmbraDirectedFixtureObject");
  auto const restartedInteractionClass = restarted.interactionClassHandleFor(
      L"exercise", "HLAinteractionRoot.UmbraDirectedFixtureInteraction");
  auto const restartedMarker = restarted.attributeHandleFor(
      L"exercise", "HLAobjectRoot.UmbraDirectedFixtureObject", "DirectedTargetMarker");
  auto const restartedPrivilege = restarted.attributeHandleFor(
      L"exercise", "HLAobjectRoot.UmbraDirectedFixtureObject",
      "HLAprivilegeToDeleteObject");
  REQUIRE(restartedObjectClass);
  REQUIRE(restartedInteractionClass);
  REQUIRE(restartedMarker);
  REQUIRE(restartedPrivilege);
  REQUIRE(*restartedObjectClass == *objectClass);
  REQUIRE(*restartedInteractionClass == *interactionClass);
  REQUIRE(*restartedMarker == *marker);
  REQUIRE(*restartedPrivilege == *privilegeToDelete);

  auto restoredBeforeHandoff = restarted.planReceiveOrderDirectedInteraction(
      L"exercise", publisherId, registered.objectInstanceHandle,
      *restartedInteractionClass, {});
  REQUIRE(restoredBeforeHandoff.status ==
      umbra::detail::ReceiveOrderDirectedInteractionStatus::applied);
  REQUIRE(restoredBeforeHandoff.recipients.size() == 1U);
  REQUIRE(restoredBeforeHandoff.recipients.front().federateId == initialOwnerId);

  std::vector<unsigned char> const acquisitionTag{
      't', 's', 'o', '-', 'o', 'w', 'n', 'e', 'r'};
  auto acquisition = restarted.planAttributeOwnershipAcquisition(
      L"exercise", handoffOwnerId, registered.objectInstanceHandle,
      ownershipTargetAttributes, acquisitionTag);
  REQUIRE(acquisition.status ==
      umbra::detail::AttributeOwnershipAcquisitionStatus::applied);
  REQUIRE(acquisition.workItems.size() == 1U);
  auto const& releaseWork = acquisition.workItems.front();
  REQUIRE(releaseWork.kind ==
      umbra::detail::AttributeOwnershipAcquisitionWorkKind::request_release);
  auto releaseDelivery = restarted.beginAttributeOwnershipAcquisitionRelease(
      L"exercise", handoffOwnerId, initialOwnerId,
      registered.objectInstanceHandle, releaseWork.requestId,
      releaseWork.attributeHandles);
  REQUIRE(releaseDelivery);
  auto divestiture = restarted.planAttributeOwnershipDivestitureIfWanted(
      L"exercise", initialOwnerId, registered.objectInstanceHandle,
      ownershipTargetAttributes, acquisitionTag);
  REQUIRE(divestiture.status ==
      umbra::detail::AttributeOwnershipDivestitureIfWantedStatus::applied);
  REQUIRE(divestiture.notifications.size() == 1U);
  auto const& notification = divestiture.notifications.front();
  auto notificationDelivery =
      restarted.beginAttributeOwnershipDivestitureIfWantedNotification(
          L"exercise", handoffOwnerId, registered.objectInstanceHandle,
          notification.notificationId, ownershipTargetAttributes);
  REQUIRE(notificationDelivery);

  auto handedOffRoute = restarted.planReceiveOrderDirectedInteraction(
      L"exercise", publisherId, registered.objectInstanceHandle,
      *restartedInteractionClass, {});
  REQUIRE(handedOffRoute.status ==
      umbra::detail::ReceiveOrderDirectedInteractionStatus::applied);
  REQUIRE(handedOffRoute.recipients.size() == 1U);
  REQUIRE(handedOffRoute.recipients.front().federateId == handoffOwnerId);

  auto delivery = restarted.beginTsoPayloadDelivery(
      L"exercise", initialOwnerId,
      rti1516_2025::HLAinteger64Time(5), true);
  REQUIRE(delivery.status ==
      umbra::detail::FederationTsoRegistryStatus::applied);
  REQUIRE(delivery.deliveryStatus ==
      umbra::detail::FederationTsoDeliveryStatus::applied);
  REQUIRE(delivery.deliveries.size() == 1U);
  auto const* directed = std::get_if<umbra::detail::TsoDirectedInteractionDelivery>(
      &delivery.deliveries.front());
  REQUIRE(directed);
  REQUIRE(directed->message.messageId == enqueued.messageId);
  REQUIRE_FALSE(restarted.timestampedDirectedInteractionRecipientFor(
      L"exercise", publisherId, initialOwnerId,
      registered.objectInstanceHandle, *restartedInteractionClass, {},
      enqueued.messageId));
  REQUIRE(restarted.finishTsoRecipientCallbackSuppressed(
      L"exercise", initialOwnerId, enqueued.messageId));
  REQUIRE_FALSE(restarted.beginTsoInteractionCallback(
      L"exercise", initialOwnerId, enqueued.messageId));
  REQUIRE(restarted.completeTsoDelivery(
      L"exercise", directed->queuedMessage).delivery.status ==
      umbra::detail::FederationTsoDeliveryStatus::applied);

  auto const retracted = restarted.retractTsoMessageForProducer(
      L"exercise", publisherId, enqueued.messageId,
      std::make_shared<rti1516_2025::HLAinteger64Time>(0));
  REQUIRE(retracted.status ==
      umbra::detail::FederationTsoRegistryStatus::applied);
  REQUIRE(retracted.queueResult.status ==
      umbra::detail::TsoMessageQueueStatus::applied);
  REQUIRE(retracted.requestRetractionNotifications.empty());
  REQUIRE_FALSE(restarted.canDeliverTsoRequestRetraction(
      L"exercise", initialOwnerId, enqueued.messageId));

  std::wstring const terminalSaveLabel =
      L"directed-interaction-tso-ownership-callback-terminal";
  saveAll(restarted, terminalSaveLabel);
  auto terminal = store->load(L"exercise", terminalSaveLabel);
  REQUIRE(terminal);
  auto terminalImage = umbra::detail::FederationStateImageCodec::decode(
      terminal->stateImage);
  REQUIRE(terminalImage.tsoDirectedInteractionMessages.empty());
  REQUIRE(terminalImage.tsoRequestRetractionRecords.size() == 1U);
  REQUIRE(terminalImage.tsoRequestRetractionRecords.front().retractionApplied);
  REQUIRE(terminalImage.tsoRequestRetractionRecords.front().terminal);

  std::filesystem::remove_all(directory, ignored);
}

TEST_CASE(
    "Filesystem fresh-registry directed TSO delivers an eligible by-ownership recipient and issues Request Retraction",
    "[unit][kernel][federation-registry][save-restore][durable-save][filesystem][restore]"
    "[process-restart][process-restart-directed-interaction-tso-ownership-delivery-retraction]"
    "[tso-directed-interaction-state][tso-retraction-ledger-state][directed-interaction]"
    "[directed-routing][ownership-ledger-state][object-visibility-state]"
    "[ownership-management][time-management]") {
  auto const directory = temporarySaveCommitDirectory();
  std::error_code ignored;
  std::filesystem::remove_all(directory, ignored);
  auto store = std::make_shared<umbra::detail::FilesystemFederationSaveCommitStore>(
      directory);

  EmbeddedFederationRegistry source({}, store);
  REQUIRE(source.create(L"exercise", composedDirectedInteractionDefinition()).status ==
      FederationRegistryStatus::applied);
  auto publisher = source.join(
      L"exercise", L"publisher", L"publisher", noOpCallbackRoute());
  auto owner = source.join(
      L"exercise", L"owner", L"owner", noOpCallbackRoute());
  REQUIRE(publisher.membership);
  REQUIRE(owner.membership);
  auto const publisherId = publisher.membership->id;
  auto const ownerId = owner.membership->id;

  auto const objectClass = source.objectClassHandleFor(
      L"exercise", "HLAobjectRoot.UmbraDirectedFixtureObject");
  auto const interactionClass = source.interactionClassHandleFor(
      L"exercise", "HLAinteractionRoot.UmbraDirectedFixtureInteraction");
  auto const marker = source.attributeHandleFor(
      L"exercise", "HLAobjectRoot.UmbraDirectedFixtureObject", "DirectedTargetMarker");
  REQUIRE(objectClass);
  REQUIRE(interactionClass);
  REQUIRE(marker);

  std::set<std::uint64_t> const markerOnly{*marker};
  std::set<std::uint64_t> const directedOnly{*interactionClass};
  REQUIRE(source.setObjectClassAttributePublication(
      L"exercise", ownerId, *objectClass, markerOnly, true) ==
      umbra::detail::ObjectClassAttributeDeclarationStatus::applied);
  REQUIRE(source.setObjectClassAttributeSubscription(
      L"exercise", publisherId, *objectClass, markerOnly, true) ==
      umbra::detail::ObjectClassAttributeDeclarationStatus::applied);
  REQUIRE(source.publishObjectClassDirectedInteractions(
      L"exercise", publisherId, *objectClass, directedOnly) ==
      umbra::detail::DirectedInteractionDeclarationStatus::applied);
  REQUIRE(source.subscribeObjectClassDirectedInteractions(
      L"exercise", ownerId, *objectClass, directedOnly, false) ==
      umbra::detail::DirectedInteractionDeclarationStatus::applied);

  auto registered = source.registerObjectInstance(
      L"exercise", ownerId, *objectClass);
  REQUIRE(registered.status ==
      umbra::detail::ObjectInstanceRegistrationStatus::applied);
  REQUIRE(registered.objectInstanceHandle != 0U);
  auto discoveries = source.planObjectInstanceDiscoveriesForInstance(
      L"exercise", registered.objectInstanceHandle);
  REQUIRE(discoveries.size() == 1U);
  REQUIRE(source.beginObjectInstanceDiscovery(
      L"exercise", publisherId, registered.objectInstanceHandle));

  std::string const markerValue{"eligible-directed-tso", 21U};
  std::vector<std::pair<std::uint64_t, rti1516_2025::VariableLengthData>> values{{
      *marker,
      rti1516_2025::VariableLengthData(markerValue.data(), markerValue.size()),
  }};
  REQUIRE(source.recordSuccessfulUpdateAttributeValues(
      L"exercise", ownerId, registered.objectInstanceHandle, *objectClass,
      {"HLAreliable"}, &values) == FederationRegistryStatus::applied);

  auto sourcePlan = source.planReceiveOrderDirectedInteraction(
      L"exercise", publisherId, registered.objectInstanceHandle,
      *interactionClass, {});
  REQUIRE(sourcePlan.status ==
      umbra::detail::ReceiveOrderDirectedInteractionStatus::applied);
  REQUIRE(sourcePlan.recipients.size() == 1U);
  REQUIRE(sourcePlan.recipients.front().federateId == ownerId);

  std::string const tagBytes{"eligible-tso", 12U};
  umbra::detail::TsoDirectedInteractionMessage message;
  message.producingFederateId = publisherId;
  message.objectInstanceHandle = registered.objectInstanceHandle;
  message.sentInteractionClassHandle = *interactionClass;
  message.userSuppliedTag = rti1516_2025::VariableLengthData(
      tagBytes.data(), tagBytes.size());
  message.transportationName = "HLAreliable";
  message.timestamp = std::make_shared<rti1516_2025::HLAinteger64Time>(5);
  auto const& selected = sourcePlan.recipients.front();
  message.recipients.push_back({
      selected.federateId,
      selected.objectInstanceHandle,
      selected.receivedInteractionClassHandle,
      selected.receivedParameterHandles,
      selected.callbackRoute});
  auto const enqueued = source.enqueueTsoDirectedInteraction(
      L"exercise", std::move(message), {ownerId});
  REQUIRE(enqueued.status ==
      umbra::detail::FederationTsoRegistryStatus::applied);
  REQUIRE(enqueued.queueStatus ==
      umbra::detail::TsoMessageQueueStatus::applied);
  REQUIRE(enqueued.enqueuedRecipientCount == 1U);

  auto saveAll = [&](EmbeddedFederationRegistry& registry,
                     std::wstring const& label) {
    REQUIRE(registry.requestFederationSave(
        L"exercise", publisherId, label).status ==
        umbra::detail::FederationSaveControlStatus::applied);
    REQUIRE(registry.federateSaveBegun(
        L"exercise", publisherId).status ==
        umbra::detail::FederationSaveControlStatus::applied);
    REQUIRE(registry.federateSaveBegun(
        L"exercise", ownerId).status ==
        umbra::detail::FederationSaveControlStatus::applied);
    REQUIRE_FALSE(registry.federateSaveComplete(
        L"exercise", publisherId).saveCompletedSuccessfully);
    REQUIRE(registry.federateSaveComplete(
        L"exercise", ownerId).saveCompletedSuccessfully);
  };

  std::wstring const saveLabel =
      L"directed-interaction-tso-ownership-delivery-retraction-process-restart";
  saveAll(source, saveLabel);
  auto committed = store->load(L"exercise", saveLabel);
  REQUIRE(committed);
  auto image = umbra::detail::FederationStateImageCodec::decode(
      committed->stateImage);
  REQUIRE(image.tsoDirectedInteractionMessages.size() == 1U);
  REQUIRE(image.tsoDirectedInteractionMessages.front().messageId ==
      enqueued.messageId);
  REQUIRE(image.tsoDirectedInteractionMessages.front().recipients.size() == 1U);
  REQUIRE(image.tsoRequestRetractionRecords.size() == 1U);
  REQUIRE(image.tsoRequestRetractionRecords.front().recipientStates.size() == 1U);
  REQUIRE(image.tsoRequestRetractionRecords.front().recipientStates.front().state == 0U);
  REQUIRE(image.tsoQueueEntries.size() == 1U);
  REQUIRE(image.tsoQueueEntries.front().messageId == enqueued.messageId);
  REQUIRE(image.tsoQueueEntries.front().recipientFederateId == ownerId);
  REQUIRE(image.tsoQueueEntries.front().phase == 0U);

  EmbeddedFederationRegistry restarted({}, store);
  REQUIRE(restarted.create(L"exercise", composedDirectedInteractionDefinition()).status ==
      FederationRegistryStatus::applied);
  auto restartedPublisher = restarted.join(
      L"exercise", L"publisher", L"publisher", noOpCallbackRoute());
  auto restartedOwner = restarted.join(
      L"exercise", L"owner", L"owner", noOpCallbackRoute());
  REQUIRE(restartedPublisher.membership);
  REQUIRE(restartedOwner.membership);
  REQUIRE(restartedPublisher.membership->id == publisherId);
  REQUIRE(restartedOwner.membership->id == ownerId);
  REQUIRE(restarted.requestFederationRestore(
      L"exercise", publisherId, saveLabel).status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  REQUIRE(restarted.federateRestoreComplete(
      L"exercise", publisherId).status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  REQUIRE(restarted.federateRestoreComplete(
      L"exercise", ownerId).status ==
      umbra::detail::FederationRestoreControlStatus::applied);

  auto const restartedObjectClass = restarted.objectClassHandleFor(
      L"exercise", "HLAobjectRoot.UmbraDirectedFixtureObject");
  auto const restartedInteractionClass = restarted.interactionClassHandleFor(
      L"exercise", "HLAinteractionRoot.UmbraDirectedFixtureInteraction");
  REQUIRE(restartedObjectClass);
  REQUIRE(restartedInteractionClass);
  REQUIRE(*restartedObjectClass == *objectClass);
  REQUIRE(*restartedInteractionClass == *interactionClass);

  auto restoredRoute = restarted.planReceiveOrderDirectedInteraction(
      L"exercise", publisherId, registered.objectInstanceHandle,
      *restartedInteractionClass, {});
  REQUIRE(restoredRoute.status ==
      umbra::detail::ReceiveOrderDirectedInteractionStatus::applied);
  REQUIRE(restoredRoute.recipients.size() == 1U);
  REQUIRE(restoredRoute.recipients.front().federateId == ownerId);
  auto restoredRecipient = restarted.timestampedDirectedInteractionRecipientFor(
      L"exercise", publisherId, ownerId, registered.objectInstanceHandle,
      *restartedInteractionClass, {}, enqueued.messageId);
  REQUIRE(restoredRecipient);
  REQUIRE(restoredRecipient->federateId == ownerId);

  auto delivery = restarted.beginTsoPayloadDelivery(
      L"exercise", ownerId, rti1516_2025::HLAinteger64Time(5), true);
  REQUIRE(delivery.status ==
      umbra::detail::FederationTsoRegistryStatus::applied);
  REQUIRE(delivery.deliveryStatus ==
      umbra::detail::FederationTsoDeliveryStatus::applied);
  REQUIRE(delivery.deliveries.size() == 1U);
  auto const* directed = std::get_if<umbra::detail::TsoDirectedInteractionDelivery>(
      &delivery.deliveries.front());
  REQUIRE(directed);
  REQUIRE(directed->message.messageId == enqueued.messageId);
  REQUIRE(restarted.beginTsoInteractionCallback(
      L"exercise", ownerId, enqueued.messageId));
  REQUIRE(restarted.completeTsoDelivery(
      L"exercise", directed->queuedMessage).delivery.status ==
      umbra::detail::FederationTsoDeliveryStatus::applied);

  auto const retracted = restarted.retractTsoMessageForProducer(
      L"exercise", publisherId, enqueued.messageId,
      std::make_shared<rti1516_2025::HLAinteger64Time>(0));
  REQUIRE(retracted.status ==
      umbra::detail::FederationTsoRegistryStatus::applied);
  REQUIRE(retracted.timestampEligible);
  REQUIRE(retracted.queueResult.status ==
      umbra::detail::TsoMessageQueueStatus::applied);
  REQUIRE(retracted.requestRetractionNotifications.size() == 1U);
  REQUIRE(retracted.requestRetractionNotifications.front().receivingFederateId == ownerId);
  REQUIRE(retracted.requestRetractionNotifications.front().messageId == enqueued.messageId);
  REQUIRE(restarted.canDeliverTsoRequestRetraction(
      L"exercise", ownerId, enqueued.messageId));

  std::wstring const terminalSaveLabel =
      L"directed-interaction-tso-ownership-delivery-retraction-terminal";
  saveAll(restarted, terminalSaveLabel);
  auto terminal = store->load(L"exercise", terminalSaveLabel);
  REQUIRE(terminal);
  auto terminalImage = umbra::detail::FederationStateImageCodec::decode(
      terminal->stateImage);
  REQUIRE(terminalImage.tsoDirectedInteractionMessages.empty());
  REQUIRE(terminalImage.tsoRequestRetractionRecords.size() == 1U);
  REQUIRE(terminalImage.tsoRequestRetractionRecords.front().retractionApplied);
  REQUIRE(terminalImage.tsoRequestRetractionRecords.front().terminal);
  REQUIRE(terminalImage.tsoRequestRetractionRecords.front().recipientStates.front().state == 3U);
  REQUIRE(terminalImage.tsoQueueEntries.size() == 1U);
  REQUIRE(terminalImage.tsoQueueEntries.front().messageId == enqueued.messageId);
  REQUIRE(terminalImage.tsoQueueEntries.front().recipientFederateId == ownerId);
  REQUIRE(terminalImage.tsoQueueEntries.front().phase == 2U);

  std::filesystem::remove_all(directory, ignored);
}

TEST_CASE(
    "Filesystem federation save commits do not leave a temporary file on encoding failure",
    "[unit][kernel][federation-registry][save-restore][durable-save][filesystem][failure]") {
  auto const directory = temporarySaveCommitDirectory();
  std::error_code ignored;
  std::filesystem::remove_all(directory, ignored);
  umbra::detail::FilesystemFederationSaveCommitStore store(directory);
  umbra::detail::FederationSaveCommitDescriptor descriptor{
      L"exercise",
      std::wstring{L"checkpoint-"} + std::wstring(1U, static_cast<wchar_t>(0xD800U)),
      L"HLAinteger64Time",
      {7U},
      false};

  REQUIRE_THROWS(store.commit(descriptor));
  REQUIRE(std::filesystem::is_directory(directory));
  REQUIRE(std::filesystem::is_empty(directory));
  std::filesystem::remove_all(directory, ignored);
}

TEST_CASE(
    "Filesystem federation save commit permits restore admission after reload",
    "[unit][kernel][federation-registry][save-restore][durable-save][filesystem][restore]") {
  auto const directory = temporarySaveCommitDirectory();
  std::error_code ignored;
  std::filesystem::remove_all(directory, ignored);
  auto store = std::make_shared<umbra::detail::FilesystemFederationSaveCommitStore>(directory);
  EmbeddedFederationRegistry registry({}, store);
  REQUIRE(registry.create(L"exercise", validDefinition()).status ==
      FederationRegistryStatus::applied);
  auto joined = registry.join(
      L"exercise", L"trainer", L"alice", noOpCallbackRoute());
  REQUIRE(joined.status == FederationRegistryStatus::applied);
  REQUIRE(joined.membership);
  REQUIRE(registry.requestFederationSave(
      L"exercise", joined.membership->id, L"checkpoint").status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(registry.federateSaveBegun(
      L"exercise", joined.membership->id).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(registry.federateSaveComplete(
      L"exercise", joined.membership->id).saveCompletedSuccessfully);

  auto restore = registry.requestFederationRestore(
      L"exercise", joined.membership->id, L"checkpoint");
  REQUIRE(restore.status == umbra::detail::FederationRestoreControlStatus::applied);
  REQUIRE(restore.notifications.size() == 3U);
  auto restored = registry.federateRestoreComplete(
      L"exercise", joined.membership->id);
  REQUIRE(restored.notifications.size() == 1U);
  REQUIRE(restored.notifications.front().successful);

  std::filesystem::remove_all(directory, ignored);
}

TEST_CASE(
    "Filesystem state image restores route-free control and temporal state in a fresh registry",
    "[unit][kernel][federation-registry][save-restore][durable-save][filesystem][restore]"
    "[process-restart][pending-application-request-state]") {
  auto const directory = temporarySaveCommitDirectory();
  std::error_code ignored;
  std::filesystem::remove_all(directory, ignored);
  auto store = std::make_shared<umbra::detail::FilesystemFederationSaveCommitStore>(
      directory);

  EmbeddedFederationRegistry source({}, store);
  REQUIRE(source.create(L"exercise", validDefinition()).status ==
      FederationRegistryStatus::applied);
  auto sourceTime = std::make_shared<umbra::detail::FederateTimeState>(
      L"HLAinteger64Time",
      std::make_shared<rti1516_2025::HLAinteger64Time>());
  auto sourceJoined = source.joinWithTimeState(
      L"exercise",
      sourceTime,
      L"trainer",
      L"alice",
      noOpCallbackRoute());
  REQUIRE(sourceJoined.status == FederationRegistryStatus::applied);
  REQUIRE(sourceJoined.membership);

  auto sourceRoleRequest = sourceTime->requestTimeRegulation(
      std::make_shared<rti1516_2025::HLAinteger64Interval>(2));
  REQUIRE(sourceRoleRequest.status ==
      umbra::detail::FederateTimeEnableStatus::applied);
  REQUIRE(sourceTime->snapshot().timeRegulationPending);

  REQUIRE(source.requestFederationSave(
      L"exercise", sourceJoined.membership->id, L"process-restart-checkpoint")
      .status == umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(source.federateSaveBegun(
      L"exercise", sourceJoined.membership->id).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(source.federateSaveComplete(
      L"exercise", sourceJoined.membership->id).saveCompletedSuccessfully);

  // A new registry has no process-local Federation snapshot. It joins the
  // same member identity, loads only the durable route-free image, and keeps
  // its new live callback route/factory.
  EmbeddedFederationRegistry restarted({}, store);
  REQUIRE(restarted.create(L"exercise", validDefinition()).status ==
      FederationRegistryStatus::applied);
  auto restartedTime = std::make_shared<umbra::detail::FederateTimeState>(
      L"HLAinteger64Time",
      std::make_shared<rti1516_2025::HLAinteger64Time>());
  std::size_t roleDispatchCount = 0U;
  std::size_t roleGrantCount = 0U;
  auto roleFactory = [restartedTime, &roleDispatchCount, &roleGrantCount](
      std::uint64_t federateId,
      std::uint64_t generation,
      umbra::detail::FederationTimeRoleEnableKind kind)
      -> umbra::detail::FederationTimeGrantDispatch {
    REQUIRE(federateId != 0U);
    REQUIRE(generation != 0U);
    auto const callbackEpoch = restartedTime->callbackEpoch();
    return [restartedTime,
            generation,
            kind,
            callbackEpoch,
            &roleDispatchCount,
            &roleGrantCount] {
      ++roleDispatchCount;
      auto enabledTime = kind == umbra::detail::FederationTimeRoleEnableKind::regulation
          ? restartedTime->grantTimeRegulationIfCurrent(generation, callbackEpoch)
          : restartedTime->grantTimeConstrainedIfCurrent(generation, callbackEpoch);
      if (enabledTime) {
        ++roleGrantCount;
      }
    };
  };
  auto restartedJoined = restarted.joinWithTimeState(
      L"exercise",
      restartedTime,
      L"trainer",
      L"alice",
      noOpCallbackRoute(),
      {},
      std::move(roleFactory));
  REQUIRE(restartedJoined.status == FederationRegistryStatus::applied);
  REQUIRE(restartedJoined.membership);
  REQUIRE(restartedJoined.membership->id == sourceJoined.membership->id);

  auto restore = restarted.requestFederationRestore(
      L"exercise",
      restartedJoined.membership->id,
      L"process-restart-checkpoint");
  REQUIRE(restore.status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  auto restored = restarted.federateRestoreComplete(
      L"exercise", restartedJoined.membership->id);
  REQUIRE(restored.status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  REQUIRE(restored.timeRoleEnableDispatches.size() == 1U);
  REQUIRE_FALSE(restartedTime->snapshot().timeRegulating);
  REQUIRE(restartedTime->snapshot().timeRegulationPending);
  REQUIRE(restartedTime->snapshot().nextGeneration ==
      sourceTime->snapshot().nextGeneration);

  auto dispatch = std::move(restored.timeRoleEnableDispatches.front());
  dispatch();
  REQUIRE(roleDispatchCount == 1U);
  REQUIRE(roleGrantCount == 1U);
  REQUIRE(restartedTime->snapshot().timeRegulating);
  REQUIRE_FALSE(restartedTime->snapshot().timeRegulationPending);

  std::filesystem::remove_all(directory, ignored);
}

TEST_CASE(
    "Filesystem state image restores pending time advance and deferred lookahead in a fresh registry",
    "[unit][kernel][federation-registry][save-restore][durable-save][filesystem][restore]"
    "[process-restart][pending-application-request-state]") {
  auto const directory = temporarySaveCommitDirectory();
  std::error_code ignored;
  std::filesystem::remove_all(directory, ignored);
  auto store = std::make_shared<umbra::detail::FilesystemFederationSaveCommitStore>(
      directory);

  EmbeddedFederationRegistry source({}, store);
  REQUIRE(source.create(L"exercise", validDefinition()).status ==
      FederationRegistryStatus::applied);
  auto sourceTime = std::make_shared<umbra::detail::FederateTimeState>(
      L"HLAinteger64Time",
      std::make_shared<rti1516_2025::HLAinteger64Time>(0));
  std::size_t sourceFactoryCalls = 0U;
  auto sourceTimeAdvanceFactory = [&source, sourceTime, &sourceFactoryCalls](
      std::uint64_t federateId,
      std::uint64_t generation,
      std::uint64_t dispatchIdentity)
      -> umbra::detail::FederationTimeGrantDispatch {
    ++sourceFactoryCalls;
    return [&source,
            sourceTime,
            federateId,
            generation,
            dispatchIdentity] {
      if (source.beginTimeAdvanceGrant(
              L"exercise",
              federateId,
              generation,
              dispatchIdentity) ==
          FederationTimeGrantStatus::applied) {
        static_cast<void>(sourceTime->grant(generation));
      }
    };
  };
  auto sourceJoined = source.joinWithTimeState(
      L"exercise",
      sourceTime,
      L"trainer",
      L"alice",
      noOpCallbackRoute(),
      std::move(sourceTimeAdvanceFactory));
  REQUIRE(sourceJoined.status == FederationRegistryStatus::applied);
  REQUIRE(sourceJoined.membership);

  auto regulation = sourceTime->requestTimeRegulation(
      std::make_shared<rti1516_2025::HLAinteger64Interval>(5));
  REQUIRE(regulation.status == umbra::detail::FederateTimeEnableStatus::applied);
  REQUIRE(sourceTime->grantTimeRegulation(regulation.generation));

  auto deferredLookahead = sourceTime->modifyLookahead(
      std::make_shared<rti1516_2025::HLAinteger64Interval>(1));
  REQUIRE(deferredLookahead ==
      umbra::detail::FederateTimeModifyLookaheadStatus::applied);
  auto advance = sourceTime->requestAdvance(
      std::make_shared<rti1516_2025::HLAinteger64Time>(5));
  REQUIRE(advance.status == umbra::detail::FederateTimeAdvanceStatus::applied);
  REQUIRE(sourceTime->snapshot().timeAdvancePending);
  REQUIRE(sourceTime->snapshot().pendingModifiedLookahead);
  auto scheduled = source.requestTimeAdvanceGrant(
      L"exercise",
      sourceJoined.membership->id,
      advance.generation);
  REQUIRE(scheduled.status == FederationTimeGrantStatus::applied);
  REQUIRE(scheduled.dispatches.size() == 1U);
  REQUIRE(sourceFactoryCalls == 1U);

  REQUIRE(source.requestFederationSave(
      L"exercise",
      sourceJoined.membership->id,
      L"pending-application-request-checkpoint")
      .status == umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(source.federateSaveBegun(
      L"exercise",
      sourceJoined.membership->id)
      .status == umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(source.federateSaveComplete(
      L"exercise",
      sourceJoined.membership->id)
      .saveCompletedSuccessfully);
  std::move(scheduled.dispatches.front())();
  REQUIRE_FALSE(sourceTime->snapshot().timeAdvancePending);

  EmbeddedFederationRegistry restarted({}, store);
  REQUIRE(restarted.create(L"exercise", validDefinition()).status ==
      FederationRegistryStatus::applied);
  auto restartedTime = std::make_shared<umbra::detail::FederateTimeState>(
      L"HLAinteger64Time",
      std::make_shared<rti1516_2025::HLAinteger64Time>(0));
  std::size_t restartedFactoryCalls = 0U;
  auto restartedTimeAdvanceFactory = [&restarted, restartedTime, &restartedFactoryCalls](
      std::uint64_t federateId,
      std::uint64_t generation,
      std::uint64_t dispatchIdentity)
      -> umbra::detail::FederationTimeGrantDispatch {
    ++restartedFactoryCalls;
    return [&restarted,
            restartedTime,
            federateId,
            generation,
            dispatchIdentity] {
      if (restarted.beginTimeAdvanceGrant(
              L"exercise",
              federateId,
              generation,
              dispatchIdentity) ==
          FederationTimeGrantStatus::applied) {
        static_cast<void>(restartedTime->grant(generation));
      }
    };
  };
  auto restartedJoined = restarted.joinWithTimeState(
      L"exercise",
      restartedTime,
      L"trainer",
      L"alice",
      noOpCallbackRoute(),
      std::move(restartedTimeAdvanceFactory));
  REQUIRE(restartedJoined.status == FederationRegistryStatus::applied);
  REQUIRE(restartedJoined.membership);
  REQUIRE(restartedJoined.membership->id == sourceJoined.membership->id);

  REQUIRE(restarted.requestFederationRestore(
      L"exercise",
      restartedJoined.membership->id,
      L"pending-application-request-checkpoint")
      .status == umbra::detail::FederationRestoreControlStatus::applied);
  auto restored = restarted.federateRestoreComplete(
      L"exercise",
      restartedJoined.membership->id);
  REQUIRE(restored.status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  REQUIRE(restartedFactoryCalls == 1U);
  REQUIRE(restored.timeAdvanceGrantDispatches.size() == 1U);
  std::move(restored.timeAdvanceGrantDispatches.front())();

  auto restoredTime = restartedTime->snapshot();
  REQUIRE_FALSE(restoredTime.timeAdvancePending);
  REQUIRE(restoredTime.advanceMode ==
      umbra::detail::FederateTimeAdvanceMode::none);
  REQUIRE(restoredTime.lookahead);
  auto const* restoredLookahead =
      dynamic_cast<rti1516_2025::HLAinteger64Interval const*>(
          restoredTime.lookahead.get());
  REQUIRE(restoredLookahead);
  REQUIRE(restoredLookahead->getInterval() == 1);
  REQUIRE_FALSE(restoredTime.pendingModifiedLookahead);
  auto const* restoredCurrentTime =
      dynamic_cast<rti1516_2025::HLAinteger64Time const*>(
          restoredTime.currentTime.get());
  REQUIRE(restoredCurrentTime);
  REQUIRE(restoredCurrentTime->getTime() == 5);

  std::filesystem::remove_all(directory, ignored);
}

TEST_CASE(
    "Filesystem state image restores object-instance-name reservations in a fresh registry",
    "[unit][kernel][federation-registry][save-restore][durable-save][filesystem][restore]"
    "[process-restart][object-name-reservation-state]") {
  auto const directory = temporarySaveCommitDirectory();
  std::error_code ignored;
  std::filesystem::remove_all(directory, ignored);
  auto store = std::make_shared<umbra::detail::FilesystemFederationSaveCommitStore>(
      directory);

  EmbeddedFederationRegistry source({}, store);
  REQUIRE(source.create(L"exercise", validDefinition()).status ==
      FederationRegistryStatus::applied);
  auto sourceJoined = source.join(
      L"exercise", L"trainer", L"alice", noOpCallbackRoute());
  REQUIRE(sourceJoined.status == FederationRegistryStatus::applied);
  REQUIRE(sourceJoined.membership);
  auto const federateId = sourceJoined.membership->id;
  auto reservation = source.reserveObjectInstanceName(
      L"exercise", federateId, L"process-restart-reserved-table");
  REQUIRE(reservation.status == ObjectInstanceNameReservationStatus::applied);
  REQUIRE(reservation.succeeded);

  REQUIRE(source.requestFederationSave(
      L"exercise", federateId, L"process-restart-name-checkpoint").status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(source.federateSaveBegun(L"exercise", federateId).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(source.federateSaveComplete(L"exercise", federateId)
      .saveCompletedSuccessfully);

  EmbeddedFederationRegistry restarted({}, store);
  REQUIRE(restarted.create(L"exercise", validDefinition()).status ==
      FederationRegistryStatus::applied);
  auto restartedJoined = restarted.join(
      L"exercise", L"trainer", L"alice", noOpCallbackRoute());
  REQUIRE(restartedJoined.status == FederationRegistryStatus::applied);
  REQUIRE(restartedJoined.membership);
  REQUIRE(restartedJoined.membership->id == federateId);

  REQUIRE(restarted.requestFederationRestore(
      L"exercise", federateId, L"process-restart-name-checkpoint").status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  REQUIRE(restarted.federateRestoreComplete(L"exercise", federateId).status ==
      umbra::detail::FederationRestoreControlStatus::applied);

  auto duplicate = restarted.reserveObjectInstanceName(
      L"exercise", federateId, L"process-restart-reserved-table");
  REQUIRE(duplicate.status == ObjectInstanceNameReservationStatus::applied);
  REQUIRE_FALSE(duplicate.succeeded);
  REQUIRE(restarted.releaseObjectInstanceName(
      L"exercise", federateId, L"process-restart-reserved-table") ==
      ObjectInstanceNameReservationStatus::applied);
  auto available = restarted.reserveObjectInstanceName(
      L"exercise", federateId, L"process-restart-reserved-table");
  REQUIRE(available.status == ObjectInstanceNameReservationStatus::applied);
  REQUIRE(available.succeeded);

  std::filesystem::remove_all(directory, ignored);
}

TEST_CASE(
    "Filesystem state image restores synchronization points in a fresh registry",
    "[unit][kernel][federation-registry][save-restore][durable-save][filesystem][restore]"
    "[process-restart][synchronization-state]") {
  auto const directory = temporarySaveCommitDirectory();
  std::error_code ignored;
  std::filesystem::remove_all(directory, ignored);
  auto store = std::make_shared<umbra::detail::FilesystemFederationSaveCommitStore>(
      directory);

  EmbeddedFederationRegistry source({}, store);
  REQUIRE(source.create(L"exercise", validDefinition()).status ==
      FederationRegistryStatus::applied);
  auto sourceJoined = source.join(
      L"exercise", L"trainer", L"alice", noOpCallbackRoute());
  REQUIRE(sourceJoined.status == FederationRegistryStatus::applied);
  REQUIRE(sourceJoined.membership);
  auto const federateId = sourceJoined.membership->id;
  auto registered = source.registerSynchronizationPoint(
      L"exercise",
      federateId,
      L"process-restart-barrier",
      {0x01U, 0x02U},
      {federateId},
      true);
  REQUIRE(registered.status ==
      umbra::detail::SynchronizationPointRegistrationStatus::applied);

  REQUIRE(source.requestFederationSave(
      L"exercise", federateId, L"process-restart-sync-checkpoint").status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(source.federateSaveBegun(L"exercise", federateId).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(source.federateSaveComplete(L"exercise", federateId)
      .saveCompletedSuccessfully);

  EmbeddedFederationRegistry restarted({}, store);
  REQUIRE(restarted.create(L"exercise", validDefinition()).status ==
      FederationRegistryStatus::applied);
  auto restartedJoined = restarted.join(
      L"exercise", L"trainer", L"alice", noOpCallbackRoute());
  REQUIRE(restartedJoined.status == FederationRegistryStatus::applied);
  REQUIRE(restartedJoined.membership);
  REQUIRE(restartedJoined.membership->id == federateId);
  REQUIRE(restarted.requestFederationRestore(
      L"exercise", federateId, L"process-restart-sync-checkpoint").status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  REQUIRE(restarted.federateRestoreComplete(L"exercise", federateId).status ==
      umbra::detail::FederationRestoreControlStatus::applied);

  REQUIRE(restarted.requestFederationSave(
      L"exercise", federateId, L"process-restart-sync-after-restore").status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(restarted.federateSaveBegun(L"exercise", federateId).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(restarted.federateSaveComplete(L"exercise", federateId)
      .saveCompletedSuccessfully);
  auto committed = store->load(
      L"exercise", L"process-restart-sync-after-restore");
  REQUIRE(committed.has_value());
  auto image = umbra::detail::FederationStateImageCodec::decode(
      committed->stateImage);
  REQUIRE(image.synchronizationPoints.size() == 1U);
  REQUIRE(image.synchronizationPoints.front().label ==
      L"process-restart-barrier");
  REQUIRE(image.synchronizationPoints.front().userSuppliedTag ==
      std::string{"\x01\x02", 2U});
  REQUIRE(image.synchronizationPoints.front().synchronizationSet ==
      std::vector<std::uint64_t>{federateId});
  REQUIRE_FALSE(image.synchronizationPoints.front().lateJoinExpansionAllowed);

  std::filesystem::remove_all(directory, ignored);
}

TEST_CASE(
    "Synchronization-point late-join expansion respects an explicit synchronization set",
    "[unit][kernel][federation-registry][synchronization][late-join]") {
  EmbeddedFederationRegistry registry;
  REQUIRE(registry.create(L"exercise", validDefinition()).status ==
      FederationRegistryStatus::applied);

  auto first = registry.join(
      L"exercise", L"trainer", L"first", noOpCallbackRoute());
  REQUIRE(first.status == FederationRegistryStatus::applied);
  REQUIRE(first.membership);
  auto const firstId = first.membership->id;

  auto explicitRegistration = registry.registerSynchronizationPoint(
      L"exercise",
      firstId,
      L"explicit-scope",
      {0x01U},
      {firstId},
      true);
  REQUIRE(explicitRegistration.status ==
      umbra::detail::SynchronizationPointRegistrationStatus::applied);
  REQUIRE(explicitRegistration.succeeded);

  auto second = registry.join(
      L"exercise", L"trainer", L"second", noOpCallbackRoute());
  REQUIRE(second.status == FederationRegistryStatus::applied);
  REQUIRE(second.membership);
  auto const secondId = second.membership->id;
  auto explicitAnnouncements = registry.announcePendingSynchronizationPoints(
      L"exercise", secondId);
  REQUIRE(explicitAnnouncements.status ==
      umbra::detail::SynchronizationPointAnnouncementStatus::applied);
  REQUIRE(explicitAnnouncements.announcements.empty());

  auto defaultRegistration = registry.registerSynchronizationPoint(
      L"exercise",
      firstId,
      L"default-scope",
      {0x02U},
      {},
      false);
  REQUIRE(defaultRegistration.status ==
      umbra::detail::SynchronizationPointRegistrationStatus::applied);
  REQUIRE(defaultRegistration.succeeded);

  auto third = registry.join(
      L"exercise", L"trainer", L"third", noOpCallbackRoute());
  REQUIRE(third.status == FederationRegistryStatus::applied);
  REQUIRE(third.membership);
  auto const thirdId = third.membership->id;
  auto announcements = registry.announcePendingSynchronizationPoints(
      L"exercise", thirdId);
  REQUIRE(announcements.status ==
      umbra::detail::SynchronizationPointAnnouncementStatus::applied);
  REQUIRE(announcements.announcements.size() == 1U);
  REQUIRE(announcements.announcements.front().label == L"default-scope");
  REQUIRE(announcements.announcements.front().receivingFederateId == thirdId);
  REQUIRE(announcements.announcements.front().userSuppliedTag ==
      std::vector<unsigned char>{0x02U});
}

TEST_CASE(
    "Filesystem state image restores federation-owned regions in a fresh registry",
    "[unit][kernel][federation-registry][save-restore][durable-save][filesystem][restore]"
    "[process-restart][region-state]") {
  auto const directory = temporarySaveCommitDirectory();
  std::error_code ignored;
  std::filesystem::remove_all(directory, ignored);
  auto store = std::make_shared<umbra::detail::FilesystemFederationSaveCommitStore>(
      directory);

  EmbeddedFederationRegistry source({}, store);
  REQUIRE(source.create(L"exercise", composedRestaurantDefinition()).status ==
      FederationRegistryStatus::applied);
  auto sourceJoined = source.join(
      L"exercise", L"trainer", L"alice", noOpCallbackRoute());
  REQUIRE(sourceJoined.status == FederationRegistryStatus::applied);
  REQUIRE(sourceJoined.membership);
  auto const federateId = sourceJoined.membership->id;
  auto dimension = source.dimensionHandleFor(L"exercise", "ServerId");
  REQUIRE(dimension.has_value());
  auto created = source.createRegion(L"exercise", federateId, {*dimension});
  REQUIRE(created.status == umbra::detail::RegionServiceStatus::applied);
  REQUIRE(created.regionHandle != 0U);
  REQUIRE(source.setRangeBounds(
      L"exercise", federateId, created.regionHandle, *dimension,
      umbra::detail::RegionRangeBounds{2UL, 5UL}) ==
      umbra::detail::RegionServiceStatus::applied);
  REQUIRE(source.commitRegionModifications(
      L"exercise", federateId, {created.regionHandle}) ==
      umbra::detail::RegionServiceStatus::applied);

  REQUIRE(source.requestFederationSave(
      L"exercise", federateId, L"process-restart-region-checkpoint").status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(source.federateSaveBegun(L"exercise", federateId).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(source.federateSaveComplete(L"exercise", federateId)
      .saveCompletedSuccessfully);

  EmbeddedFederationRegistry restarted({}, store);
  REQUIRE(restarted.create(L"exercise", composedRestaurantDefinition()).status ==
      FederationRegistryStatus::applied);
  auto restartedJoined = restarted.join(
      L"exercise", L"trainer", L"alice", noOpCallbackRoute());
  REQUIRE(restartedJoined.status == FederationRegistryStatus::applied);
  REQUIRE(restartedJoined.membership);
  REQUIRE(restartedJoined.membership->id == federateId);
  REQUIRE(restarted.requestFederationRestore(
      L"exercise", federateId, L"process-restart-region-checkpoint").status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  REQUIRE(restarted.federateRestoreComplete(L"exercise", federateId).status ==
      umbra::detail::FederationRestoreControlStatus::applied);

  auto dimensions = restarted.dimensionHandleSetForRegion(
      L"exercise", federateId, created.regionHandle);
  REQUIRE(dimensions.status == umbra::detail::RegionServiceStatus::applied);
  REQUIRE(dimensions.dimensionHandles == std::set<std::uint64_t>{*dimension});
  auto bounds = restarted.rangeBoundsForRegion(
      L"exercise", federateId, created.regionHandle, *dimension);
  REQUIRE(bounds.status == umbra::detail::RegionServiceStatus::applied);
  REQUIRE(bounds.range.lowerBound == 2UL);
  REQUIRE(bounds.range.upperBound == 5UL);

  std::filesystem::remove_all(directory, ignored);
}

TEST_CASE(
    "Filesystem state image restores object-class attribute declarations in a fresh registry",
    "[unit][kernel][federation-registry][save-restore][durable-save][filesystem][restore]"
    "[process-restart][object-class-declaration-state]") {
  auto const directory = temporarySaveCommitDirectory();
  std::error_code ignored;
  std::filesystem::remove_all(directory, ignored);
  auto store = std::make_shared<umbra::detail::FilesystemFederationSaveCommitStore>(
      directory);

  EmbeddedFederationRegistry source({}, store);
  REQUIRE(source.create(L"exercise", composedRestaurantDefinition()).status ==
      FederationRegistryStatus::applied);
  auto sourceJoined = source.join(
      L"exercise", L"trainer", L"alice", noOpCallbackRoute());
  REQUIRE(sourceJoined.status == FederationRegistryStatus::applied);
  REQUIRE(sourceJoined.membership);
  auto const federateId = sourceJoined.membership->id;
  auto const server = source.objectClassHandleFor(
      L"exercise", "HLAobjectRoot.Employee.Server");
  auto const efficiency = source.attributeHandleFor(
      L"exercise", "HLAobjectRoot.Employee.Server", "Efficiency");
  REQUIRE(server.has_value());
  REQUIRE(efficiency.has_value());

  REQUIRE(source.setObjectClassAttributePublication(
      L"exercise", federateId, *server, {*efficiency}, true) ==
      umbra::detail::ObjectClassAttributeDeclarationStatus::applied);
  REQUIRE(source.setObjectClassAttributeSubscription(
      L"exercise", federateId, *server, {*efficiency}, true, "High") ==
      umbra::detail::ObjectClassAttributeDeclarationStatus::applied);
  REQUIRE(source.changeDefaultAttributeTransportationType(
      L"exercise", federateId, *server, {*efficiency}, "HLAreliable") ==
      umbra::detail::AttributeTransportationTypeDefaultStatus::applied);
  REQUIRE(source.changeDefaultAttributeOrderType(
      L"exercise", federateId, *server, {*efficiency}, rti1516_2025::TIMESTAMP) ==
      umbra::detail::AttributeOrderTypeDefaultStatus::applied);

  auto sourceDeclaration = source.objectClassAttributeDeclarationFor(
      L"exercise", federateId, *server);
  REQUIRE(sourceDeclaration.has_value());
  REQUIRE(sourceDeclaration->explicitlyPublishedAttributes ==
      std::set<std::uint64_t>{*efficiency});
  REQUIRE(sourceDeclaration->subscribedAttributes.at(*efficiency));
  REQUIRE(sourceDeclaration->subscribedUpdateRateDesignators.at(*efficiency) ==
      "High");
  auto const savedGeneration = sourceDeclaration->subscriptionGeneration;
  REQUIRE(savedGeneration != 0U);

  REQUIRE(source.requestFederationSave(
      L"exercise", federateId, L"process-restart-object-class-checkpoint").status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(source.federateSaveBegun(L"exercise", federateId).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(source.federateSaveComplete(L"exercise", federateId)
      .saveCompletedSuccessfully);

  EmbeddedFederationRegistry restarted({}, store);
  REQUIRE(restarted.create(L"exercise", composedRestaurantDefinition()).status ==
      FederationRegistryStatus::applied);
  auto restartedJoined = restarted.join(
      L"exercise", L"trainer", L"alice", noOpCallbackRoute());
  REQUIRE(restartedJoined.status == FederationRegistryStatus::applied);
  REQUIRE(restartedJoined.membership);
  REQUIRE(restartedJoined.membership->id == federateId);
  REQUIRE(restarted.requestFederationRestore(
      L"exercise", federateId, L"process-restart-object-class-checkpoint").status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  REQUIRE(restarted.federateRestoreComplete(L"exercise", federateId).status ==
      umbra::detail::FederationRestoreControlStatus::applied);

  auto restoredDeclaration = restarted.objectClassAttributeDeclarationFor(
      L"exercise", federateId, *server);
  REQUIRE(restoredDeclaration.has_value());
  REQUIRE(restoredDeclaration->explicitlyPublishedAttributes ==
      std::set<std::uint64_t>{*efficiency});
  REQUIRE(restoredDeclaration->subscribedAttributes.at(*efficiency));
  REQUIRE(restoredDeclaration->subscribedUpdateRateDesignators.at(*efficiency) ==
      "High");
  REQUIRE(restoredDeclaration->subscriptionGeneration == savedGeneration);

  REQUIRE(restarted.requestFederationSave(
      L"exercise", federateId, L"process-restart-object-class-after-restore").status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(restarted.federateSaveBegun(L"exercise", federateId).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(restarted.federateSaveComplete(L"exercise", federateId)
      .saveCompletedSuccessfully);
  auto committed = store->load(
      L"exercise", L"process-restart-object-class-after-restore");
  REQUIRE(committed.has_value());
  auto image = umbra::detail::FederationStateImageCodec::decode(
      committed->stateImage);
  REQUIRE(image.objectClassAttributeDeclarations.size() == 1U);
  REQUIRE(image.objectClassAttributeDeclarations.front().federateId == federateId);
  REQUIRE(image.objectClassAttributeDeclarations.front().subscriptionGeneration ==
      savedGeneration);
  REQUIRE(image.objectClassAttributeDeclarations.front().classes.size() == 1U);
  auto const& restoredClass =
      image.objectClassAttributeDeclarations.front().classes.front();
  REQUIRE(restoredClass.objectClassHandle == *server);
  REQUIRE(restoredClass.explicitlyPublishedAttributeHandles ==
      std::vector<std::uint64_t>{*efficiency});
  REQUIRE(restoredClass.subscribedAttributes.size() == 1U);
  REQUIRE(restoredClass.subscribedAttributes.front().attributeHandle == *efficiency);
  REQUIRE(restoredClass.subscribedAttributes.front().active);
  REQUIRE(restoredClass.subscribedUpdateRateDesignators.size() == 1U);
  REQUIRE(restoredClass.subscribedUpdateRateDesignators.front().attributeHandle == *efficiency);
  REQUIRE(restoredClass.subscribedUpdateRateDesignators.front().value == "High");
  REQUIRE(restoredClass.defaultTransportationTypes.size() == 1U);
  REQUIRE(restoredClass.defaultTransportationTypes.front().attributeHandle == *efficiency);
  REQUIRE(restoredClass.defaultTransportationTypes.front().value == "HLAreliable");
  REQUIRE(restoredClass.defaultOrderTypes.size() == 1U);
  REQUIRE(restoredClass.defaultOrderTypes.front().attributeHandle == *efficiency);
  REQUIRE(restoredClass.defaultOrderTypes.front().orderType == 2U);

  std::filesystem::remove_all(directory, ignored);
}

TEST_CASE(
    "Filesystem state image restores latest object application values in a fresh registry",
    "[unit][kernel][federation-registry][save-restore][durable-save][filesystem][restore]"
    "[process-restart][application-value-state]") {
  auto const directory = temporarySaveCommitDirectory();
  std::error_code ignored;
  std::filesystem::remove_all(directory, ignored);
  auto store = std::make_shared<umbra::detail::FilesystemFederationSaveCommitStore>(
      directory);

  EmbeddedFederationRegistry source({}, store);
  REQUIRE(source.create(L"exercise", composedRestaurantDefinition()).status ==
      FederationRegistryStatus::applied);
  auto sourceJoined = source.join(
      L"exercise", L"trainer", L"alice", noOpCallbackRoute());
  REQUIRE(sourceJoined.status == FederationRegistryStatus::applied);
  REQUIRE(sourceJoined.membership);
  auto const federateId = sourceJoined.membership->id;
  auto const server = source.objectClassHandleFor(
      L"exercise", "HLAobjectRoot.Employee.Server");
  auto const efficiency = source.attributeHandleFor(
      L"exercise", "HLAobjectRoot.Employee.Server", "Efficiency");
  REQUIRE(server.has_value());
  REQUIRE(efficiency.has_value());
  REQUIRE(source.setObjectClassAttributePublication(
      L"exercise", federateId, *server, {*efficiency}, true) ==
      umbra::detail::ObjectClassAttributeDeclarationStatus::applied);

  auto registered = source.registerObjectInstance(
      L"exercise", federateId, *server);
  REQUIRE(registered.status ==
      umbra::detail::ObjectInstanceRegistrationStatus::applied);
  REQUIRE(registered.objectInstanceHandle != 0U);

  std::vector<rti1516_2025::Octet> valueBytes{
      static_cast<rti1516_2025::Octet>(0x01U),
      static_cast<rti1516_2025::Octet>(0xfeU),
      static_cast<rti1516_2025::Octet>(0x7fU)};
  std::vector<std::pair<std::uint64_t, rti1516_2025::VariableLengthData>> values{
      {*efficiency,
       rti1516_2025::VariableLengthData(valueBytes.data(), valueBytes.size())},
  };
  REQUIRE(source.recordSuccessfulUpdateAttributeValues(
      L"exercise",
      federateId,
      registered.objectInstanceHandle,
      *server,
      {"HLAreliable"},
      &values) == FederationRegistryStatus::applied);

  REQUIRE(source.requestFederationSave(
      L"exercise", federateId, L"process-restart-application-value-checkpoint").status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(source.federateSaveBegun(L"exercise", federateId).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(source.federateSaveComplete(L"exercise", federateId)
      .saveCompletedSuccessfully);

  EmbeddedFederationRegistry restarted({}, store);
  REQUIRE(restarted.create(L"exercise", composedRestaurantDefinition()).status ==
      FederationRegistryStatus::applied);
  auto restartedJoined = restarted.join(
      L"exercise", L"trainer", L"alice", noOpCallbackRoute());
  REQUIRE(restartedJoined.status == FederationRegistryStatus::applied);
  REQUIRE(restartedJoined.membership);
  REQUIRE(restartedJoined.membership->id == federateId);
  REQUIRE(restarted.requestFederationRestore(
      L"exercise", federateId, L"process-restart-application-value-checkpoint").status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  REQUIRE(restarted.federateRestoreComplete(L"exercise", federateId).status ==
      umbra::detail::FederationRestoreControlStatus::applied);

  // A fresh registry has no process-local object snapshot. Saving immediately
  // after restore therefore proves that object identity and the latest value
  // were materialized from the durable image rather than retained in memory.
  REQUIRE(restarted.requestFederationSave(
      L"exercise", federateId, L"process-restart-application-value-after-restore").status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(restarted.federateSaveBegun(L"exercise", federateId).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(restarted.federateSaveComplete(L"exercise", federateId)
      .saveCompletedSuccessfully);

  auto committed = store->load(
      L"exercise", L"process-restart-application-value-after-restore");
  REQUIRE(committed.has_value());
  auto image = umbra::detail::FederationStateImageCodec::decode(
      committed->stateImage);
  REQUIRE(image.objects.size() == 1U);
  auto const& object = image.objects.front();
  REQUIRE(object.handle == registered.objectInstanceHandle);
  REQUIRE(object.name == registered.objectInstanceName);
  REQUIRE(object.registeredObjectClassHandle == *server);
  REQUIRE(object.attributeValuesPresent);
  REQUIRE(object.attributeValues.size() == 1U);
  REQUIRE(object.attributeValues.front().attributeHandle == *efficiency);
  REQUIRE(object.attributeValues.front().value ==
      std::string{reinterpret_cast<char const*>(valueBytes.data()), valueBytes.size()});

  std::filesystem::remove_all(directory, ignored);
}

TEST_CASE(
    "Filesystem state image restores ownership assumption search state and continues with a newly eligible federate",
    "[unit][kernel][federation-registry][save-restore][durable-save][filesystem][restore]"
    "[process-restart][process-restart-ownership-assumption-search][ownership-ledger-state]"
    "[ownership-assumption-research]") {
  auto const directory = temporarySaveCommitDirectory();
  std::error_code ignored;
  std::filesystem::remove_all(directory, ignored);
  auto store = std::make_shared<umbra::detail::FilesystemFederationSaveCommitStore>(
      directory);

  EmbeddedFederationRegistry source({}, store);
  REQUIRE(source.create(L"exercise", composedRestaurantDefinition()).status ==
      FederationRegistryStatus::applied);
  auto owner = source.join(
      L"exercise", L"publisher", L"owner", noOpCallbackRoute());
  auto firstCandidate = source.join(
      L"exercise", L"candidate-one", L"candidate-one", noOpCallbackRoute());
  auto secondCandidate = source.join(
      L"exercise", L"candidate-two", L"candidate-two", noOpCallbackRoute());
  REQUIRE(owner.membership);
  REQUIRE(firstCandidate.membership);
  REQUIRE(secondCandidate.membership);

  auto const server = source.objectClassHandleFor(
      L"exercise", "HLAobjectRoot.Employee.Server");
  auto const efficiency = source.attributeHandleFor(
      L"exercise", "HLAobjectRoot.Employee.Server", "Efficiency");
  REQUIRE(server.has_value());
  REQUIRE(efficiency.has_value());
  std::set<std::uint64_t> const efficiencyOnly{*efficiency};
  REQUIRE(source.setObjectClassAttributePublication(
      L"exercise", owner.membership->id, *server, efficiencyOnly, true) ==
      umbra::detail::ObjectClassAttributeDeclarationStatus::applied);
  REQUIRE(source.setObjectClassAttributePublication(
      L"exercise", firstCandidate.membership->id, *server, efficiencyOnly, true) ==
      umbra::detail::ObjectClassAttributeDeclarationStatus::applied);
  REQUIRE(source.setObjectClassAttributeSubscription(
      L"exercise", firstCandidate.membership->id, *server, efficiencyOnly, true) ==
      umbra::detail::ObjectClassAttributeDeclarationStatus::applied);

  auto registered = source.registerObjectInstance(
      L"exercise", owner.membership->id, *server);
  REQUIRE(registered.status ==
      umbra::detail::ObjectInstanceRegistrationStatus::applied);
  REQUIRE(registered.objectInstanceHandle != 0U);
  auto discoveries = source.planObjectInstanceDiscoveriesForInstance(
      L"exercise", registered.objectInstanceHandle);
  REQUIRE(discoveries.size() == 1U);
  auto const& firstDiscovery = discoveries.front();
  REQUIRE(firstDiscovery.receivingFederateId == firstCandidate.membership->id);
  REQUIRE(source.beginObjectInstanceDiscovery(
      L"exercise",
      firstDiscovery.receivingFederateId,
      registered.objectInstanceHandle)
      .has_value());

  std::string const valueBytes{"\x51\x52", 2U};
  std::vector<std::pair<std::uint64_t, rti1516_2025::VariableLengthData>> values{{
      *efficiency,
      rti1516_2025::VariableLengthData(valueBytes.data(), valueBytes.size()),
  }};
  REQUIRE(source.recordSuccessfulUpdateAttributeValues(
      L"exercise",
      owner.membership->id,
      registered.objectInstanceHandle,
      *server,
      {"HLAreliable"},
      &values) == FederationRegistryStatus::applied);
  std::vector<unsigned char> const divestitureTag{'a', 's', 's', 'u', 'm', 'e'};
  auto divestiture = source.planUnconditionalAttributeOwnershipDivestiture(
      L"exercise",
      owner.membership->id,
      registered.objectInstanceHandle,
      efficiencyOnly,
      divestitureTag);
  REQUIRE(divestiture.status ==
      umbra::detail::UnconditionalAttributeOwnershipDivestitureStatus::applied);
  REQUIRE(divestiture.assumptionRecipients.size() == 1U);
  REQUIRE(divestiture.assumptionRecipients.front().receivingFederateId ==
      firstCandidate.membership->id);
  REQUIRE(divestiture.assumptionRecipients.front().attributeHandles == efficiencyOnly);

  std::wstring const saveLabel = L"ownership-assumption-process-restart";
  REQUIRE(source.requestFederationSave(
      L"exercise", owner.membership->id, saveLabel).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(source.federateSaveBegun(
      L"exercise", owner.membership->id).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(source.federateSaveBegun(
      L"exercise", firstCandidate.membership->id).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(source.federateSaveBegun(
      L"exercise", secondCandidate.membership->id).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE_FALSE(source.federateSaveComplete(
      L"exercise", owner.membership->id).saveCompletedSuccessfully);
  REQUIRE_FALSE(source.federateSaveComplete(
      L"exercise", firstCandidate.membership->id).saveCompletedSuccessfully);
  REQUIRE(source.federateSaveComplete(
      L"exercise", secondCandidate.membership->id).saveCompletedSuccessfully);

  auto committed = store->load(L"exercise", saveLabel);
  REQUIRE(committed.has_value());
  auto image = umbra::detail::FederationStateImageCodec::decode(
      committed->stateImage);
  REQUIRE(image.objects.size() == 1U);
  auto const& savedObject = image.objects.front();
  REQUIRE(savedObject.ownershipAssumptionRecipientsByAttribute.size() == 1U);
  REQUIRE(savedObject.ownershipAssumptionRecipientsByAttribute.front().attributeHandle ==
      *efficiency);
  REQUIRE(savedObject.ownershipAssumptionRecipientsByAttribute.front().recipientFederateIds ==
      std::vector<std::uint64_t>{firstCandidate.membership->id});
  REQUIRE(savedObject.ownershipAssumptionUserSuppliedTagsByAttribute.size() == 1U);
  REQUIRE(savedObject.ownershipAssumptionUserSuppliedTagsByAttribute.front().attributeHandle ==
      *efficiency);
  REQUIRE(savedObject.ownershipAssumptionUserSuppliedTagsByAttribute.front().userSuppliedTag ==
      std::string(divestitureTag.begin(), divestitureTag.end()));
  REQUIRE(image.pendingAttributeOwnershipAssumptionsPresent);
  REQUIRE(image.pendingAttributeOwnershipAssumptions.size() == 1U);
  REQUIRE(image.pendingAttributeOwnershipAssumptions.front().objectInstanceHandle ==
      registered.objectInstanceHandle);
  REQUIRE(image.pendingAttributeOwnershipAssumptions.front().receivingFederateId ==
      firstCandidate.membership->id);
  REQUIRE(image.pendingAttributeOwnershipAssumptions.front().attributeHandles ==
      std::vector<std::uint64_t>{*efficiency});
  REQUIRE(image.pendingAttributeOwnershipAssumptions.front().userSuppliedTag ==
      std::string(divestitureTag.begin(), divestitureTag.end()));
  REQUIRE(savedObject.pendingOperationCount == 3U);
  REQUIRE(savedObject.attributes.size() == 2U);

  EmbeddedFederationRegistry restarted({}, store);
  REQUIRE(restarted.create(L"exercise", composedRestaurantDefinition()).status ==
      FederationRegistryStatus::applied);
  auto restartedOwner = restarted.join(
      L"exercise", L"publisher", L"owner", noOpCallbackRoute());
  auto restartedFirstCandidate = restarted.join(
      L"exercise", L"candidate-one", L"candidate-one", noOpCallbackRoute());
  auto restartedSecondCandidate = restarted.join(
      L"exercise", L"candidate-two", L"candidate-two", noOpCallbackRoute());
  REQUIRE(restartedOwner.membership);
  REQUIRE(restartedFirstCandidate.membership);
  REQUIRE(restartedSecondCandidate.membership);
  REQUIRE(restartedOwner.membership->id == owner.membership->id);
  REQUIRE(restartedFirstCandidate.membership->id == firstCandidate.membership->id);
  REQUIRE(restartedSecondCandidate.membership->id == secondCandidate.membership->id);

  REQUIRE(restarted.requestFederationRestore(
      L"exercise", restartedOwner.membership->id, saveLabel).status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  auto firstRestore = restarted.federateRestoreComplete(
      L"exercise", restartedOwner.membership->id);
  REQUIRE(firstRestore.status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  auto secondRestore = restarted.federateRestoreComplete(
      L"exercise", restartedFirstCandidate.membership->id);
  REQUIRE(secondRestore.status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  auto finalRestore = restarted.federateRestoreComplete(
      L"exercise", restartedSecondCandidate.membership->id);
  REQUIRE(finalRestore.status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  REQUIRE(finalRestore.attributeOwnershipAssumptionWorkItems.size() == 1U);
  REQUIRE(finalRestore.attributeOwnershipAssumptionWorkItems.front().receivingFederateId ==
      restartedFirstCandidate.membership->id);
  REQUIRE(finalRestore.attributeOwnershipAssumptionWorkItems.front().attributeHandles ==
      efficiencyOnly);
  REQUIRE(finalRestore.attributeOwnershipAssumptionWorkItems.front().userSuppliedTag ==
      divestitureTag);
  auto reboundDelivery = restarted.attributeOwnershipAssumptionDeliveryFor(
      L"exercise",
      restartedFirstCandidate.membership->id,
      registered.objectInstanceHandle,
      efficiencyOnly);
  REQUIRE(reboundDelivery.has_value());
  REQUIRE(reboundDelivery->attributeHandles == efficiencyOnly);
  // Candidate two was not known at save time. Its later discovery and
  // publication continue the restored search without repeating candidate
  // one's already-recorded offer.
  REQUIRE(restarted.setObjectClassAttributeSubscription(
      L"exercise",
      restartedSecondCandidate.membership->id,
      *server,
      efficiencyOnly,
      true) == umbra::detail::ObjectClassAttributeDeclarationStatus::applied);
  auto restartedDiscoveries = restarted.planObjectInstanceDiscoveriesForInstance(
      L"exercise", registered.objectInstanceHandle);
  REQUIRE(restartedDiscoveries.size() == 1U);
  REQUIRE(restartedDiscoveries.front().receivingFederateId ==
      restartedSecondCandidate.membership->id);
  REQUIRE(restarted.beginObjectInstanceDiscovery(
      L"exercise",
      restartedSecondCandidate.membership->id,
      registered.objectInstanceHandle)
      .has_value());
  REQUIRE(restarted.setObjectClassAttributePublication(
      L"exercise",
      restartedSecondCandidate.membership->id,
      *server,
      efficiencyOnly,
      true) == umbra::detail::ObjectClassAttributeDeclarationStatus::applied);
  auto continuation = restarted.planAttributeOwnershipAssumptionsForFederate(
      L"exercise",
      restartedSecondCandidate.membership->id,
      registered.objectInstanceHandle,
      &efficiencyOnly);
  REQUIRE(continuation.size() == 1U);
  REQUIRE(continuation.front().receivingFederateId ==
      restartedSecondCandidate.membership->id);
  REQUIRE(continuation.front().objectInstanceHandle ==
      registered.objectInstanceHandle);
  REQUIRE(continuation.front().attributeHandles == efficiencyOnly);
  REQUIRE(continuation.front().userSuppliedTag == divestitureTag);

  std::filesystem::remove_all(directory, ignored);
}

TEST_CASE(
    "Process-local restore returns route-free ownership-assumption work for the process endpoint",
    "[unit][kernel][federation-registry][save-restore][ownership-management]"
    "[process-boundary][process-local-restore][ownership-assumption]"
    "[2025]") {
  auto const directory = temporarySaveCommitDirectory();
  std::error_code ignored;
  std::filesystem::remove_all(directory, ignored);
  auto store = std::make_shared<umbra::detail::FilesystemFederationSaveCommitStore>(
      directory);

  EmbeddedFederationRegistry registry({}, store);
  REQUIRE(registry.create(L"exercise", composedRestaurantDefinition()).status ==
      FederationRegistryStatus::applied);
  auto owner = registry.join(
      L"exercise", L"publisher", L"owner", noOpCallbackRoute());
  auto candidate = registry.join(
      L"exercise", L"candidate", L"candidate", noOpCallbackRoute());
  REQUIRE(owner.membership);
  REQUIRE(candidate.membership);

  auto const server = registry.objectClassHandleFor(
      L"exercise", "HLAobjectRoot.Employee.Server");
  auto const efficiency = registry.attributeHandleFor(
      L"exercise", "HLAobjectRoot.Employee.Server", "Efficiency");
  REQUIRE(server.has_value());
  REQUIRE(efficiency.has_value());
  std::set<std::uint64_t> const efficiencyOnly{*efficiency};
  REQUIRE(registry.setObjectClassAttributePublication(
      L"exercise", owner.membership->id, *server, efficiencyOnly, true) ==
      umbra::detail::ObjectClassAttributeDeclarationStatus::applied);
  REQUIRE(registry.setObjectClassAttributePublication(
      L"exercise", candidate.membership->id, *server, efficiencyOnly, true) ==
      umbra::detail::ObjectClassAttributeDeclarationStatus::applied);
  REQUIRE(registry.setObjectClassAttributeSubscription(
      L"exercise", candidate.membership->id, *server, efficiencyOnly, true) ==
      umbra::detail::ObjectClassAttributeDeclarationStatus::applied);

  auto registered = registry.registerObjectInstance(
      L"exercise", owner.membership->id, *server);
  REQUIRE(registered.status ==
      umbra::detail::ObjectInstanceRegistrationStatus::applied);
  auto discoveries = registry.planObjectInstanceDiscoveriesForInstance(
      L"exercise", registered.objectInstanceHandle);
  REQUIRE(discoveries.size() == 1U);
  REQUIRE(registry.beginObjectInstanceDiscovery(
      L"exercise",
      candidate.membership->id,
      registered.objectInstanceHandle)
      .has_value());

  std::vector<unsigned char> const divestitureTag{'p', 'r', 'c'};
  auto divestiture = registry.planUnconditionalAttributeOwnershipDivestiture(
      L"exercise",
      owner.membership->id,
      registered.objectInstanceHandle,
      efficiencyOnly,
      divestitureTag);
  REQUIRE(divestiture.status ==
      umbra::detail::UnconditionalAttributeOwnershipDivestitureStatus::applied);
  REQUIRE(divestiture.assumptionRecipients.size() == 1U);

  std::wstring const saveLabel = L"process-local-assumption-work";
  REQUIRE(registry.requestFederationSave(
      L"exercise", owner.membership->id, saveLabel).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(registry.federateSaveBegun(
      L"exercise", owner.membership->id).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(registry.federateSaveBegun(
      L"exercise", candidate.membership->id).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE_FALSE(registry.federateSaveComplete(
      L"exercise", owner.membership->id).saveCompletedSuccessfully);
  REQUIRE(registry.federateSaveComplete(
      L"exercise", candidate.membership->id).saveCompletedSuccessfully);
  auto committed = store->load(L"exercise", saveLabel);
  REQUIRE(committed.has_value());
  auto image = umbra::detail::FederationStateImageCodec::decode(
      committed->stateImage);
  REQUIRE(image.pendingAttributeOwnershipAssumptions.size() == 1U);

  REQUIRE(registry.requestFederationRestore(
      L"exercise", owner.membership->id, saveLabel).status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  REQUIRE(registry.federateRestoreComplete(
      L"exercise", owner.membership->id).status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  auto restored = registry.federateRestoreComplete(
      L"exercise", candidate.membership->id);
  REQUIRE(restored.status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  REQUIRE(restored.attributeOwnershipAssumptionWorkItems.size() == 1U);
  auto const& work = restored.attributeOwnershipAssumptionWorkItems.front();
  REQUIRE(work.receivingFederateId == candidate.membership->id);
  REQUIRE(work.objectInstanceHandle == registered.objectInstanceHandle);
  REQUIRE(work.attributeHandles == efficiencyOnly);
  REQUIRE(work.userSuppliedTag == divestitureTag);

  std::filesystem::remove_all(directory, ignored);
}

TEST_CASE(
    "Filesystem state image restores federation save conditionals in a fresh registry",
    "[unit][kernel][federation-registry][save-restore][durable-save][filesystem][restore]"
    "[process-restart][application-ledger-state][federation-mom-save-conditionals]") {
  auto const directory = temporarySaveCommitDirectory();
  std::error_code ignored;
  std::filesystem::remove_all(directory, ignored);
  auto store = std::make_shared<umbra::detail::FilesystemFederationSaveCommitStore>(
      directory);

  auto completeSave = [](EmbeddedFederationRegistry& registry,
                         std::wstring const& label,
                         std::uint64_t federateId) {
    REQUIRE(registry.requestFederationSave(
        L"exercise", federateId, label).status ==
        umbra::detail::FederationSaveControlStatus::applied);
    REQUIRE(registry.federateSaveBegun(L"exercise", federateId).status ==
        umbra::detail::FederationSaveControlStatus::applied);
    REQUIRE(registry.federateSaveComplete(L"exercise", federateId)
        .saveCompletedSuccessfully);
  };

  EmbeddedFederationRegistry source({}, store);
  REQUIRE(source.create(L"exercise", composedRestaurantDefinition()).status ==
      FederationRegistryStatus::applied);
  auto joined = source.join(
      L"exercise", L"trainer", L"alice", noOpCallbackRoute());
  REQUIRE(joined.status == FederationRegistryStatus::applied);
  REQUIRE(joined.membership);
  auto const federateId = joined.membership->id;

  std::wstring const firstLabel = L"save-history-first";
  std::wstring const secondLabel = L"save-history-second";
  completeSave(source, firstLabel, federateId);
  completeSave(source, secondLabel, federateId);

  auto secondCommit = store->load(L"exercise", secondLabel);
  REQUIRE(secondCommit.has_value());
  auto secondImage = umbra::detail::FederationStateImageCodec::decode(
      secondCommit->stateImage);
  REQUIRE(secondImage.saveHistoryPresent);
  // Save completion updates HLAlastSave* after the durable snapshot is
  // committed, so the second image must retain the first completed label.
  REQUIRE(secondImage.lastSaveName == firstLabel);
  REQUIRE_FALSE(secondImage.lastSaveTimeEncoding.has_value());
  REQUIRE(secondImage.nextSaveName.empty());
  REQUIRE_FALSE(secondImage.nextSaveTimeEncoding.has_value());

  EmbeddedFederationRegistry restarted({}, store);
  REQUIRE(restarted.create(L"exercise", composedRestaurantDefinition()).status ==
      FederationRegistryStatus::applied);
  auto restartedJoined = restarted.join(
      L"exercise", L"trainer", L"alice", noOpCallbackRoute());
  REQUIRE(restartedJoined.status == FederationRegistryStatus::applied);
  REQUIRE(restartedJoined.membership);
  REQUIRE(restartedJoined.membership->id == federateId);
  REQUIRE(restarted.requestFederationRestore(
      L"exercise", federateId, secondLabel).status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  REQUIRE(restarted.federateRestoreComplete(L"exercise", federateId).status ==
      umbra::detail::FederationRestoreControlStatus::applied);

  // A subsequent save snapshots the rehydrated application-visible
  // HLAlastSave* values. If restore only rebuilt control/temporal state, this
  // image would incorrectly lose the first label.
  auto const thirdLabel = L"save-history-after-restore";
  completeSave(restarted, thirdLabel, federateId);
  auto thirdCommit = store->load(L"exercise", thirdLabel);
  REQUIRE(thirdCommit.has_value());
  auto thirdImage = umbra::detail::FederationStateImageCodec::decode(
      thirdCommit->stateImage);
  REQUIRE(thirdImage.saveHistoryPresent);
  REQUIRE(thirdImage.lastSaveName == firstLabel);
  REQUIRE_FALSE(thirdImage.lastSaveTimeEncoding.has_value());

  std::filesystem::remove_all(directory, ignored);
}

TEST_CASE(
    "Federation restore rejects an in-memory snapshot when its durable envelope is gone",
    "[unit][kernel][federation-registry][save-restore][durable-save][restore][failure]") {
  class CommitThenForgetStore final : public umbra::detail::FederationSaveCommitStore {
   public:
    void commit(umbra::detail::FederationSaveCommitDescriptor const& descriptor) override {
      lastCommit = descriptor;
    }

    [[nodiscard]] std::optional<umbra::detail::FederationSaveCommitDescriptor> load(
        std::wstring const&,
        std::wstring const&) const override {
      return std::nullopt;
    }

    umbra::detail::FederationSaveCommitDescriptor lastCommit;
  };

  auto store = std::make_shared<CommitThenForgetStore>();
  EmbeddedFederationRegistry registry({}, store);
  REQUIRE(registry.create(L"exercise", validDefinition()).status ==
      FederationRegistryStatus::applied);
  auto joined = registry.join(
      L"exercise", L"trainer", L"alice", noOpCallbackRoute());
  REQUIRE(joined.status == FederationRegistryStatus::applied);
  REQUIRE(joined.membership);
  REQUIRE(registry.requestFederationSave(
      L"exercise", joined.membership->id, L"checkpoint").status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(registry.federateSaveBegun(
      L"exercise", joined.membership->id).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(registry.federateSaveComplete(
      L"exercise", joined.membership->id).saveCompletedSuccessfully);

  auto restore = registry.requestFederationRestore(
      L"exercise", joined.membership->id, L"checkpoint");
  REQUIRE(restore.status == umbra::detail::FederationRestoreControlStatus::snapshot_not_found);
  REQUIRE(restore.notifications.size() == 1U);
  REQUIRE(restore.notifications.front().kind ==
      umbra::detail::FederationRestoreNotificationKind::request_failed);
}

TEST_CASE(
    "Filesystem state image restores pending regular ownership release work in a fresh registry",
    "[unit][kernel][federation-registry][save-restore][durable-save][filesystem][restore]"
    "[process-restart][ownership-ledger-state][attribute-ownership-acquisition]") {
  auto const directory = temporarySaveCommitDirectory();
  std::error_code ignored;
  std::filesystem::remove_all(directory, ignored);
  auto store = std::make_shared<umbra::detail::FilesystemFederationSaveCommitStore>(
      directory);

  EmbeddedFederationRegistry source({}, store);
  REQUIRE(source.create(L"exercise", composedRestaurantDefinition()).status ==
      FederationRegistryStatus::applied);
  auto owner = source.join(
      L"exercise", L"publisher", L"owner", noOpCallbackRoute());
  auto requester = source.join(
      L"exercise", L"subscriber", L"requester", noOpCallbackRoute());
  REQUIRE(owner.status == FederationRegistryStatus::applied);
  REQUIRE(requester.status == FederationRegistryStatus::applied);
  REQUIRE(owner.membership);
  REQUIRE(requester.membership);

  auto const server = source.objectClassHandleFor(
      L"exercise", "HLAobjectRoot.Employee.Server");
  auto const efficiency = source.attributeHandleFor(
      L"exercise", "HLAobjectRoot.Employee.Server", "Efficiency");
  REQUIRE(server.has_value());
  REQUIRE(efficiency.has_value());
  std::set<std::uint64_t> const efficiencyOnly{*efficiency};
  REQUIRE(source.setObjectClassAttributePublication(
      L"exercise", owner.membership->id, *server, efficiencyOnly, true) ==
      umbra::detail::ObjectClassAttributeDeclarationStatus::applied);
  REQUIRE(source.setObjectClassAttributeSubscription(
      L"exercise", requester.membership->id, *server, efficiencyOnly, true) ==
      umbra::detail::ObjectClassAttributeDeclarationStatus::applied);

  auto registered = source.registerObjectInstance(
      L"exercise", owner.membership->id, *server);
  REQUIRE(registered.status ==
      umbra::detail::ObjectInstanceRegistrationStatus::applied);
  REQUIRE(registered.objectInstanceHandle != 0U);
  auto discoveries = source.planObjectInstanceDiscoveriesForInstance(
      L"exercise", registered.objectInstanceHandle);
  REQUIRE(discoveries.size() == 1U);
  REQUIRE(discoveries.front().receivingFederateId == requester.membership->id);
  REQUIRE(source.beginObjectInstanceDiscovery(
      L"exercise",
      requester.membership->id,
      registered.objectInstanceHandle)
      .has_value());

  // The requester must publish after discovery so the saved object contains
  // both live known-class projections and a valid publication precondition.
  REQUIRE(source.setObjectClassAttributePublication(
      L"exercise", requester.membership->id, *server, efficiencyOnly, true) ==
      umbra::detail::ObjectClassAttributeDeclarationStatus::applied);
  std::string const valueBytes{"\x71\x72", 2U};
  std::vector<std::pair<std::uint64_t, rti1516_2025::VariableLengthData>> values{{
      *efficiency,
      rti1516_2025::VariableLengthData(valueBytes.data(), valueBytes.size()),
  }};
  REQUIRE(source.recordSuccessfulUpdateAttributeValues(
      L"exercise",
      owner.membership->id,
      registered.objectInstanceHandle,
      *server,
      {"HLAreliable"},
      &values) == FederationRegistryStatus::applied);

  std::vector<unsigned char> const acquisitionTag{'r', 'e', 's', 't', 'a', 'r', 't'};
  auto acquisition = source.planAttributeOwnershipAcquisition(
      L"exercise",
      requester.membership->id,
      registered.objectInstanceHandle,
      efficiencyOnly,
      acquisitionTag);
  REQUIRE(acquisition.status ==
      umbra::detail::AttributeOwnershipAcquisitionStatus::applied);
  REQUIRE(acquisition.workItems.size() == 1U);
  REQUIRE(acquisition.workItems.front().kind ==
      umbra::detail::AttributeOwnershipAcquisitionWorkKind::request_release);
  REQUIRE(acquisition.workItems.front().requestingFederateId == requester.membership->id);
  REQUIRE(acquisition.workItems.front().receivingFederateId == owner.membership->id);
  REQUIRE(acquisition.workItems.front().objectInstanceHandle ==
      registered.objectInstanceHandle);
  REQUIRE(acquisition.workItems.front().attributeHandles == efficiencyOnly);
  REQUIRE(acquisition.workItems.front().userSuppliedTag == acquisitionTag);

  std::wstring const saveLabel = L"ownership-process-restart";
  REQUIRE(source.requestFederationSave(
      L"exercise", owner.membership->id, saveLabel).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(source.federateSaveBegun(
      L"exercise", owner.membership->id).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(source.federateSaveBegun(
      L"exercise", requester.membership->id).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE_FALSE(source.federateSaveComplete(
      L"exercise", owner.membership->id).saveCompletedSuccessfully);
  REQUIRE(source.federateSaveComplete(
      L"exercise", requester.membership->id).saveCompletedSuccessfully);

  auto committed = store->load(L"exercise", saveLabel);
  REQUIRE(committed.has_value());
  auto image = umbra::detail::FederationStateImageCodec::decode(
      committed->stateImage);
  REQUIRE(image.objects.size() == 1U);
  auto const& savedObject = image.objects.front();
  REQUIRE(savedObject.handle == registered.objectInstanceHandle);
  REQUIRE(savedObject.attributeValuesPresent);
  REQUIRE(savedObject.attributeValues.size() == 1U);
  REQUIRE(savedObject.attributeValues.front().attributeHandle == *efficiency);
  REQUIRE(savedObject.attributeValues.front().value == valueBytes);
  REQUIRE(savedObject.pendingAttributeOwnershipAcquisitionRequests.size() == 1U);
  auto const& savedRequest =
      savedObject.pendingAttributeOwnershipAcquisitionRequests.front();
  REQUIRE(savedRequest.requestId == acquisition.workItems.front().requestId);
  REQUIRE(savedRequest.requestingFederateId == requester.membership->id);
  REQUIRE(savedRequest.desiredAttributeHandles ==
      std::vector<std::uint64_t>{*efficiency});
  REQUIRE(savedRequest.unavailableQueuedAttributeHandles.empty());
  REQUIRE(savedRequest.releaseCallbacksQueuedByOwningFederate.size() == 1U);
  REQUIRE(savedRequest.releaseCallbacksQueuedByOwningFederate.front().first ==
      owner.membership->id);
  REQUIRE(savedRequest.releaseCallbacksQueuedByOwningFederate.front().second ==
      std::vector<std::uint64_t>{*efficiency});

  EmbeddedFederationRegistry restarted({}, store);
  REQUIRE(restarted.create(L"exercise", composedRestaurantDefinition()).status ==
      FederationRegistryStatus::applied);
  auto restartedOwner = restarted.join(
      L"exercise", L"publisher", L"owner", noOpCallbackRoute());
  auto restartedRequester = restarted.join(
      L"exercise", L"subscriber", L"requester", noOpCallbackRoute());
  REQUIRE(restartedOwner.membership);
  REQUIRE(restartedRequester.membership);
  REQUIRE(restartedOwner.membership->id == owner.membership->id);
  REQUIRE(restartedRequester.membership->id == requester.membership->id);

  REQUIRE(restarted.requestFederationRestore(
      L"exercise", restartedRequester.membership->id, saveLabel).status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  auto firstRestore = restarted.federateRestoreComplete(
      L"exercise", restartedOwner.membership->id);
  REQUIRE(firstRestore.status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  REQUIRE(firstRestore.ownershipAcquisitionWorkItems.empty());
  auto restored = restarted.federateRestoreComplete(
      L"exercise", restartedRequester.membership->id);
  REQUIRE(restored.status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  REQUIRE(restored.ownershipAcquisitionWorkItems.size() == 1U);
  auto const& restoredWork = restored.ownershipAcquisitionWorkItems.front();
  REQUIRE(restoredWork.kind ==
      umbra::detail::AttributeOwnershipAcquisitionWorkKind::request_release);
  REQUIRE(restoredWork.requestingFederateId == restartedRequester.membership->id);
  REQUIRE(restoredWork.receivingFederateId == restartedOwner.membership->id);
  REQUIRE(restoredWork.objectInstanceHandle == registered.objectInstanceHandle);
  REQUIRE(restoredWork.requestId == savedRequest.requestId);
  REQUIRE(restoredWork.attributeHandles == efficiencyOnly);
  REQUIRE(restoredWork.userSuppliedTag == acquisitionTag);

  auto restoredOwnership = restarted.attributeOwnedByFederate(
      L"exercise",
      restartedOwner.membership->id,
      registered.objectInstanceHandle,
      *efficiency);
  REQUIRE(restoredOwnership.status ==
      umbra::detail::AttributeOwnershipCheckStatus::applied);
  REQUIRE(restoredOwnership.ownedByRequestingFederate);
  auto requesterOwnership = restarted.attributeOwnedByFederate(
      L"exercise",
      restartedRequester.membership->id,
      registered.objectInstanceHandle,
      *efficiency);
  REQUIRE(requesterOwnership.status ==
      umbra::detail::AttributeOwnershipCheckStatus::applied);
  REQUIRE_FALSE(requesterOwnership.ownedByRequestingFederate);

  auto releaseDelivery = restarted.beginAttributeOwnershipAcquisitionRelease(
      L"exercise",
      restoredWork.requestingFederateId,
      restoredWork.receivingFederateId,
      restoredWork.objectInstanceHandle,
      restoredWork.requestId,
      restoredWork.attributeHandles);
  REQUIRE(releaseDelivery.has_value());
  REQUIRE(releaseDelivery->objectInstanceHandle == registered.objectInstanceHandle);
  REQUIRE(releaseDelivery->candidateAttributeHandles == efficiencyOnly);

  std::filesystem::remove_all(directory, ignored);
}

TEST_CASE(
    "Filesystem state image restores pending If Available ownership callback in a fresh registry",
    "[unit][kernel][federation-registry][save-restore][durable-save][filesystem][restore]"
    "[process-restart][ownership-ledger-state][attribute-ownership-acquisition]"
    "[attribute-ownership-acquisition-if-available]") {
  auto const directory = temporarySaveCommitDirectory();
  std::error_code ignored;
  std::filesystem::remove_all(directory, ignored);
  auto store = std::make_shared<umbra::detail::FilesystemFederationSaveCommitStore>(
      directory);

  EmbeddedFederationRegistry source({}, store);
  REQUIRE(source.create(L"exercise", composedRestaurantDefinition()).status ==
      FederationRegistryStatus::applied);
  auto owner = source.join(
      L"exercise", L"publisher", L"owner", noOpCallbackRoute());
  auto requester = source.join(
      L"exercise", L"subscriber", L"requester", noOpCallbackRoute());
  REQUIRE(owner.status == FederationRegistryStatus::applied);
  REQUIRE(requester.status == FederationRegistryStatus::applied);
  REQUIRE(owner.membership);
  REQUIRE(requester.membership);

  auto const server = source.objectClassHandleFor(
      L"exercise", "HLAobjectRoot.Employee.Server");
  auto const efficiency = source.attributeHandleFor(
      L"exercise", "HLAobjectRoot.Employee.Server", "Efficiency");
  REQUIRE(server.has_value());
  REQUIRE(efficiency.has_value());
  std::set<std::uint64_t> const efficiencyOnly{*efficiency};
  REQUIRE(source.setObjectClassAttributePublication(
      L"exercise", owner.membership->id, *server, efficiencyOnly, true) ==
      umbra::detail::ObjectClassAttributeDeclarationStatus::applied);
  REQUIRE(source.setObjectClassAttributeSubscription(
      L"exercise", requester.membership->id, *server, efficiencyOnly, true) ==
      umbra::detail::ObjectClassAttributeDeclarationStatus::applied);

  auto registered = source.registerObjectInstance(
      L"exercise", owner.membership->id, *server);
  REQUIRE(registered.status ==
      umbra::detail::ObjectInstanceRegistrationStatus::applied);
  REQUIRE(registered.objectInstanceHandle != 0U);
  auto discoveries = source.planObjectInstanceDiscoveriesForInstance(
      L"exercise", registered.objectInstanceHandle);
  REQUIRE(discoveries.size() == 1U);
  REQUIRE(discoveries.front().receivingFederateId == requester.membership->id);
  REQUIRE(source.beginObjectInstanceDiscovery(
      L"exercise",
      requester.membership->id,
      registered.objectInstanceHandle)
      .has_value());

  // The requester must publish after discovery so the saved object contains
  // both live known-class projections and a valid publication precondition.
  REQUIRE(source.setObjectClassAttributePublication(
      L"exercise", requester.membership->id, *server, efficiencyOnly, true) ==
      umbra::detail::ObjectClassAttributeDeclarationStatus::applied);
  std::string const valueBytes{"\x73\x74", 2U};
  std::vector<std::pair<std::uint64_t, rti1516_2025::VariableLengthData>> values{{
      *efficiency,
      rti1516_2025::VariableLengthData(valueBytes.data(), valueBytes.size()),
  }};
  REQUIRE(source.recordSuccessfulUpdateAttributeValues(
      L"exercise",
      owner.membership->id,
      registered.objectInstanceHandle,
      *server,
      {"HLAreliable"},
      &values) == FederationRegistryStatus::applied);

  std::vector<unsigned char> const acquisitionTag{'w', 't', 'a', '-', 'r', 'e', 's', 't', 'a', 'r', 't'};
  auto acquisition = source.planAttributeOwnershipAcquisitionIfAvailable(
      L"exercise",
      requester.membership->id,
      registered.objectInstanceHandle,
      efficiencyOnly,
      acquisitionTag);
  REQUIRE(acquisition.status ==
      umbra::detail::AttributeOwnershipAcquisitionIfAvailableStatus::applied);
  REQUIRE(acquisition.requestId != 0U);
  REQUIRE(acquisition.callbackRoute);

  std::wstring const saveLabel = L"ownership-if-available-process-restart";
  REQUIRE(source.requestFederationSave(
      L"exercise", owner.membership->id, saveLabel).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(source.federateSaveBegun(
      L"exercise", owner.membership->id).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(source.federateSaveBegun(
      L"exercise", requester.membership->id).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE_FALSE(source.federateSaveComplete(
      L"exercise", owner.membership->id).saveCompletedSuccessfully);
  REQUIRE(source.federateSaveComplete(
      L"exercise", requester.membership->id).saveCompletedSuccessfully);

  auto committed = store->load(L"exercise", saveLabel);
  REQUIRE(committed.has_value());
  auto image = umbra::detail::FederationStateImageCodec::decode(
      committed->stateImage);
  REQUIRE(image.objects.size() == 1U);
  auto const& savedObject = image.objects.front();
  REQUIRE(savedObject.handle == registered.objectInstanceHandle);
  REQUIRE(savedObject.attributeValuesPresent);
  REQUIRE(savedObject.attributeValues.size() == 1U);
  REQUIRE(savedObject.attributeValues.front().attributeHandle == *efficiency);
  REQUIRE(savedObject.attributeValues.front().value == valueBytes);
  REQUIRE(savedObject.pendingAttributeOwnershipAcquisitionIfAvailableRequests.size() == 1U);
  auto const& savedRequest =
      savedObject.pendingAttributeOwnershipAcquisitionIfAvailableRequests.front();
  REQUIRE(savedRequest.requestId == acquisition.requestId);
  REQUIRE(savedRequest.requestingFederateId == requester.membership->id);
  REQUIRE(savedRequest.requestSequence != 0U);
  REQUIRE(savedRequest.desiredAttributeHandles ==
      std::vector<std::uint64_t>{*efficiency});
  REQUIRE(savedRequest.userSuppliedTag ==
      std::string(acquisitionTag.begin(), acquisitionTag.end()));

  EmbeddedFederationRegistry restarted({}, store);
  REQUIRE(restarted.create(L"exercise", composedRestaurantDefinition()).status ==
      FederationRegistryStatus::applied);
  auto restartedOwner = restarted.join(
      L"exercise", L"publisher", L"owner", noOpCallbackRoute());
  auto restartedRequester = restarted.join(
      L"exercise", L"subscriber", L"requester", noOpCallbackRoute());
  REQUIRE(restartedOwner.membership);
  REQUIRE(restartedRequester.membership);
  REQUIRE(restartedOwner.membership->id == owner.membership->id);
  REQUIRE(restartedRequester.membership->id == requester.membership->id);

  REQUIRE(restarted.requestFederationRestore(
      L"exercise", restartedRequester.membership->id, saveLabel).status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  auto firstRestore = restarted.federateRestoreComplete(
      L"exercise", restartedOwner.membership->id);
  REQUIRE(firstRestore.status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  REQUIRE(firstRestore.ownershipAcquisitionWorkItems.empty());
  auto restored = restarted.federateRestoreComplete(
      L"exercise", restartedRequester.membership->id);
  REQUIRE(restored.status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  REQUIRE(restored.ownershipAcquisitionWorkItems.size() == 1U);
  auto const& restoredWork = restored.ownershipAcquisitionWorkItems.front();
  REQUIRE(restoredWork.kind ==
      umbra::detail::AttributeOwnershipAcquisitionWorkKind::if_available_notification);
  REQUIRE(restoredWork.requestingFederateId == restartedRequester.membership->id);
  REQUIRE(restoredWork.receivingFederateId == restartedRequester.membership->id);
  REQUIRE(restoredWork.objectInstanceHandle == registered.objectInstanceHandle);
  REQUIRE(restoredWork.requestId == savedRequest.requestId);
  REQUIRE(restoredWork.attributeHandles.empty());
  REQUIRE(restoredWork.userSuppliedTag == acquisitionTag);
  REQUIRE(restoredWork.candidateIsIfAvailable);

  auto restoredOwner = restarted.attributeOwnedByFederate(
      L"exercise",
      restartedOwner.membership->id,
      registered.objectInstanceHandle,
      *efficiency);
  REQUIRE(restoredOwner.status ==
      umbra::detail::AttributeOwnershipCheckStatus::applied);
  REQUIRE(restoredOwner.ownedByRequestingFederate);
  auto restoredRequesterOwnership = restarted.attributeOwnedByFederate(
      L"exercise",
      restartedRequester.membership->id,
      registered.objectInstanceHandle,
      *efficiency);
  REQUIRE(restoredRequesterOwnership.status ==
      umbra::detail::AttributeOwnershipCheckStatus::applied);
  REQUIRE_FALSE(restoredRequesterOwnership.ownedByRequestingFederate);

  auto unavailable = restarted.beginAttributeOwnershipAcquisitionIfAvailable(
      L"exercise",
      restartedRequester.membership->id,
      registered.objectInstanceHandle,
      restoredWork.requestId);
  REQUIRE(unavailable.has_value());
  REQUIRE(unavailable->securedAttributeHandles.empty());
  REQUIRE(unavailable->unavailableAttributeHandles == efficiencyOnly);
  REQUIRE_FALSE(restarted.beginAttributeOwnershipAcquisitionIfAvailable(
      L"exercise",
      restartedRequester.membership->id,
      registered.objectInstanceHandle,
      restoredWork.requestId));

  std::filesystem::remove_all(directory, ignored);
}

TEST_CASE(
    "Filesystem state image restores pending negotiated owner confirmation in a fresh registry",
    "[unit][kernel][federation-registry][save-restore][durable-save][filesystem][restore]"
    "[process-restart][ownership-ledger-state][attribute-ownership-acquisition]"
    "[negotiated-attribute-ownership-divestiture]"
    "[process-restart-attribute-ownership-negotiated-owner-confirmation]") {
  auto const directory = temporarySaveCommitDirectory();
  std::error_code ignored;
  std::filesystem::remove_all(directory, ignored);
  auto store = std::make_shared<umbra::detail::FilesystemFederationSaveCommitStore>(
      directory);

  EmbeddedFederationRegistry source({}, store);
  REQUIRE(source.create(L"exercise", composedRestaurantDefinition()).status ==
      FederationRegistryStatus::applied);
  auto owner = source.join(
      L"exercise", L"publisher", L"owner", noOpCallbackRoute());
  auto requester = source.join(
      L"exercise", L"subscriber", L"requester", noOpCallbackRoute());
  REQUIRE(owner.status == FederationRegistryStatus::applied);
  REQUIRE(requester.status == FederationRegistryStatus::applied);
  REQUIRE(owner.membership);
  REQUIRE(requester.membership);

  auto const server = source.objectClassHandleFor(
      L"exercise", "HLAobjectRoot.Employee.Server");
  auto const efficiency = source.attributeHandleFor(
      L"exercise", "HLAobjectRoot.Employee.Server", "Efficiency");
  REQUIRE(server.has_value());
  REQUIRE(efficiency.has_value());
  std::set<std::uint64_t> const efficiencyOnly{*efficiency};
  REQUIRE(source.setObjectClassAttributePublication(
      L"exercise", owner.membership->id, *server, efficiencyOnly, true) ==
      umbra::detail::ObjectClassAttributeDeclarationStatus::applied);
  REQUIRE(source.setObjectClassAttributeSubscription(
      L"exercise", requester.membership->id, *server, efficiencyOnly, true) ==
      umbra::detail::ObjectClassAttributeDeclarationStatus::applied);

  auto registered = source.registerObjectInstance(
      L"exercise", owner.membership->id, *server);
  REQUIRE(registered.status ==
      umbra::detail::ObjectInstanceRegistrationStatus::applied);
  REQUIRE(registered.objectInstanceHandle != 0U);
  auto discoveries = source.planObjectInstanceDiscoveriesForInstance(
      L"exercise", registered.objectInstanceHandle);
  REQUIRE(discoveries.size() == 1U);
  REQUIRE(discoveries.front().receivingFederateId == requester.membership->id);
  REQUIRE(source.beginObjectInstanceDiscovery(
      L"exercise",
      requester.membership->id,
      registered.objectInstanceHandle)
      .has_value());

  // The requester must publish after discovery so the saved object contains
  // both live known-class projections and a valid publication precondition.
  REQUIRE(source.setObjectClassAttributePublication(
      L"exercise", requester.membership->id, *server, efficiencyOnly, true) ==
      umbra::detail::ObjectClassAttributeDeclarationStatus::applied);
  std::string const valueBytes{"\x75\x76", 2U};
  std::vector<std::pair<std::uint64_t, rti1516_2025::VariableLengthData>> values{{
      *efficiency,
      rti1516_2025::VariableLengthData(valueBytes.data(), valueBytes.size()),
  }};
  REQUIRE(source.recordSuccessfulUpdateAttributeValues(
      L"exercise",
      owner.membership->id,
      registered.objectInstanceHandle,
      *server,
      {"HLAreliable"},
      &values) == FederationRegistryStatus::applied);

  std::vector<unsigned char> const acquisitionTag{'n', 'e', 'g', '-', 'r', 'e', 's', 't', 'a', 'r', 't'};
  auto acquisition = source.planAttributeOwnershipAcquisition(
      L"exercise",
      requester.membership->id,
      registered.objectInstanceHandle,
      efficiencyOnly,
      acquisitionTag);
  REQUIRE(acquisition.status ==
      umbra::detail::AttributeOwnershipAcquisitionStatus::applied);
  REQUIRE(acquisition.workItems.size() == 1U);
  REQUIRE(acquisition.workItems.front().kind ==
      umbra::detail::AttributeOwnershipAcquisitionWorkKind::request_release);

  std::vector<unsigned char> const divestitureTag{'d', 'i', 'v', '-', 'r', 'e', 's', 't', 'a', 'r', 't'};
  auto divestiture = source.planNegotiatedAttributeOwnershipDivestiture(
      L"exercise",
      owner.membership->id,
      registered.objectInstanceHandle,
      efficiencyOnly,
      divestitureTag);
  REQUIRE(divestiture.status ==
      umbra::detail::NegotiatedAttributeOwnershipDivestitureStatus::applied);
  REQUIRE(divestiture.workItems.size() == 1U);
  auto const& sourceConfirmation = divestiture.workItems.front();
  REQUIRE(sourceConfirmation.kind ==
      umbra::detail::AttributeOwnershipAcquisitionWorkKind::request_divestiture_confirmation);
  REQUIRE(sourceConfirmation.requestingFederateId == requester.membership->id);
  REQUIRE(sourceConfirmation.receivingFederateId == owner.membership->id);
  REQUIRE(sourceConfirmation.objectInstanceHandle == registered.objectInstanceHandle);
  REQUIRE(sourceConfirmation.attributeHandles == efficiencyOnly);
  REQUIRE(sourceConfirmation.userSuppliedTag == acquisitionTag);

  std::wstring const saveLabel = L"ownership-negotiated-process-restart";
  REQUIRE(source.requestFederationSave(
      L"exercise", owner.membership->id, saveLabel).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(source.federateSaveBegun(
      L"exercise", owner.membership->id).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(source.federateSaveBegun(
      L"exercise", requester.membership->id).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE_FALSE(source.federateSaveComplete(
      L"exercise", owner.membership->id).saveCompletedSuccessfully);
  REQUIRE(source.federateSaveComplete(
      L"exercise", requester.membership->id).saveCompletedSuccessfully);

  auto committed = store->load(L"exercise", saveLabel);
  REQUIRE(committed.has_value());
  auto image = umbra::detail::FederationStateImageCodec::decode(
      committed->stateImage);
  REQUIRE(image.objects.size() == 1U);
  auto const& savedObject = image.objects.front();
  REQUIRE(savedObject.handle == registered.objectInstanceHandle);
  REQUIRE(savedObject.attributeValuesPresent);
  REQUIRE(savedObject.attributeValues.size() == 1U);
  REQUIRE(savedObject.attributeValues.front().attributeHandle == *efficiency);
  REQUIRE(savedObject.attributeValues.front().value == valueBytes);
  REQUIRE(savedObject.pendingAttributeOwnershipAcquisitionRequests.size() == 1U);
  auto const& savedRequest = savedObject.pendingAttributeOwnershipAcquisitionRequests.front();
  REQUIRE(savedRequest.requestId == sourceConfirmation.requestId);
  REQUIRE(savedRequest.requestingFederateId == requester.membership->id);
  REQUIRE(savedRequest.desiredAttributeHandles ==
      std::vector<std::uint64_t>{*efficiency});
  REQUIRE(savedRequest.unavailableQueuedAttributeHandles.empty());
  REQUIRE(savedRequest.releaseCallbacksQueuedByOwningFederate.size() == 1U);
  REQUIRE(savedRequest.releaseCallbacksQueuedByOwningFederate.front().first ==
      owner.membership->id);
  REQUIRE(savedRequest.releaseCallbacksQueuedByOwningFederate.front().second ==
      std::vector<std::uint64_t>{*efficiency});
  REQUIRE(savedObject.pendingNegotiatedAttributeOwnershipDivestitures.size() == 1U);
  auto const& savedDivestiture =
      savedObject.pendingNegotiatedAttributeOwnershipDivestitures.front();
  REQUIRE(savedDivestiture.attributeHandle == *efficiency);
  REQUIRE(savedDivestiture.divestingFederateId == owner.membership->id);
  REQUIRE(savedDivestiture.acquiringFederateId == requester.membership->id);
  REQUIRE(savedDivestiture.acquisitionRequestId == savedRequest.requestId);
  REQUIRE_FALSE(savedDivestiture.acquiringFederateIsIfAvailable);
  REQUIRE(savedDivestiture.confirmationQueued);
  REQUIRE_FALSE(savedDivestiture.confirmationDelivered);
  REQUIRE(savedDivestiture.userSuppliedTag ==
      std::string(divestitureTag.begin(), divestitureTag.end()));

  EmbeddedFederationRegistry restarted({}, store);
  REQUIRE(restarted.create(L"exercise", composedRestaurantDefinition()).status ==
      FederationRegistryStatus::applied);
  auto restartedOwner = restarted.join(
      L"exercise", L"publisher", L"owner", noOpCallbackRoute());
  auto restartedRequester = restarted.join(
      L"exercise", L"subscriber", L"requester", noOpCallbackRoute());
  REQUIRE(restartedOwner.membership);
  REQUIRE(restartedRequester.membership);
  REQUIRE(restartedOwner.membership->id == owner.membership->id);
  REQUIRE(restartedRequester.membership->id == requester.membership->id);

  REQUIRE(restarted.requestFederationRestore(
      L"exercise", restartedRequester.membership->id, saveLabel).status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  auto firstRestore = restarted.federateRestoreComplete(
      L"exercise", restartedOwner.membership->id);
  REQUIRE(firstRestore.status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  REQUIRE(firstRestore.ownershipAcquisitionWorkItems.empty());
  auto restored = restarted.federateRestoreComplete(
      L"exercise", restartedRequester.membership->id);
  REQUIRE(restored.status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  REQUIRE(restored.ownershipAcquisitionWorkItems.size() == 1U);
  auto const& restoredWork = restored.ownershipAcquisitionWorkItems.front();
  REQUIRE(restoredWork.kind ==
      umbra::detail::AttributeOwnershipAcquisitionWorkKind::request_divestiture_confirmation);
  REQUIRE(restoredWork.requestingFederateId == restartedRequester.membership->id);
  REQUIRE(restoredWork.receivingFederateId == restartedOwner.membership->id);
  REQUIRE(restoredWork.objectInstanceHandle == registered.objectInstanceHandle);
  REQUIRE(restoredWork.requestId == savedRequest.requestId);
  REQUIRE(restoredWork.attributeHandles == efficiencyOnly);
  REQUIRE(restoredWork.userSuppliedTag == acquisitionTag);
  REQUIRE_FALSE(restoredWork.candidateIsIfAvailable);

  auto restoredOwnerState = restarted.attributeOwnedByFederate(
      L"exercise",
      restartedOwner.membership->id,
      registered.objectInstanceHandle,
      *efficiency);
  REQUIRE(restoredOwnerState.status ==
      umbra::detail::AttributeOwnershipCheckStatus::applied);
  REQUIRE(restoredOwnerState.ownedByRequestingFederate);
  auto confirmation = restarted.beginRequestDivestitureConfirmation(
      L"exercise",
      restartedOwner.membership->id,
      restartedRequester.membership->id,
      registered.objectInstanceHandle,
      restoredWork.requestId,
      false,
      restoredWork.attributeHandles);
  REQUIRE(confirmation.has_value());
  REQUIRE(confirmation->objectInstanceHandle == registered.objectInstanceHandle);
  REQUIRE(confirmation->releasedAttributeHandles == efficiencyOnly);

  std::filesystem::remove_all(directory, ignored);
}

TEST_CASE(
    "Filesystem state image restores pending negotiated If Available owner confirmation in a fresh registry",
    "[unit][kernel][federation-registry][save-restore][durable-save][filesystem][restore]"
    "[process-restart][ownership-ledger-state][attribute-ownership-acquisition]"
    "[attribute-ownership-acquisition-if-available][negotiated-attribute-ownership-divestiture]"
    "[process-restart-attribute-ownership-negotiated-if-available-owner-confirmation]") {
  auto const directory = temporarySaveCommitDirectory();
  std::error_code ignored;
  std::filesystem::remove_all(directory, ignored);
  auto store = std::make_shared<umbra::detail::FilesystemFederationSaveCommitStore>(
      directory);

  EmbeddedFederationRegistry source({}, store);
  REQUIRE(source.create(L"exercise", composedRestaurantDefinition()).status ==
      FederationRegistryStatus::applied);
  auto owner = source.join(
      L"exercise", L"publisher", L"owner", noOpCallbackRoute());
  auto requester = source.join(
      L"exercise", L"subscriber", L"requester", noOpCallbackRoute());
  REQUIRE(owner.status == FederationRegistryStatus::applied);
  REQUIRE(requester.status == FederationRegistryStatus::applied);
  REQUIRE(owner.membership);
  REQUIRE(requester.membership);

  auto const server = source.objectClassHandleFor(
      L"exercise", "HLAobjectRoot.Employee.Server");
  auto const efficiency = source.attributeHandleFor(
      L"exercise", "HLAobjectRoot.Employee.Server", "Efficiency");
  REQUIRE(server.has_value());
  REQUIRE(efficiency.has_value());
  std::set<std::uint64_t> const efficiencyOnly{*efficiency};
  REQUIRE(source.setObjectClassAttributePublication(
      L"exercise", owner.membership->id, *server, efficiencyOnly, true) ==
      umbra::detail::ObjectClassAttributeDeclarationStatus::applied);
  REQUIRE(source.setObjectClassAttributeSubscription(
      L"exercise", requester.membership->id, *server, efficiencyOnly, true) ==
      umbra::detail::ObjectClassAttributeDeclarationStatus::applied);

  auto registered = source.registerObjectInstance(
      L"exercise", owner.membership->id, *server);
  REQUIRE(registered.status ==
      umbra::detail::ObjectInstanceRegistrationStatus::applied);
  REQUIRE(registered.objectInstanceHandle != 0U);
  auto discoveries = source.planObjectInstanceDiscoveriesForInstance(
      L"exercise", registered.objectInstanceHandle);
  REQUIRE(discoveries.size() == 1U);
  REQUIRE(discoveries.front().receivingFederateId == requester.membership->id);
  REQUIRE(source.beginObjectInstanceDiscovery(
      L"exercise",
      requester.membership->id,
      registered.objectInstanceHandle)
      .has_value());

  // Keep the requester published after discovery so the saved object carries
  // the same live publication precondition used by the callback boundary.
  REQUIRE(source.setObjectClassAttributePublication(
      L"exercise", requester.membership->id, *server, efficiencyOnly, true) ==
      umbra::detail::ObjectClassAttributeDeclarationStatus::applied);
  std::string const valueBytes{"\x77\x78", 2U};
  std::vector<std::pair<std::uint64_t, rti1516_2025::VariableLengthData>> values{{
      *efficiency,
      rti1516_2025::VariableLengthData(valueBytes.data(), valueBytes.size()),
  }};
  REQUIRE(source.recordSuccessfulUpdateAttributeValues(
      L"exercise",
      owner.membership->id,
      registered.objectInstanceHandle,
      *server,
      {"HLAreliable"},
      &values) == FederationRegistryStatus::applied);

  std::vector<unsigned char> const acquisitionTag{
      'w', 't', 'a', '-', 'n', 'e', 'g', '-', 'r', 'e', 's', 't', 'a', 'r', 't'};
  auto acquisition = source.planAttributeOwnershipAcquisitionIfAvailable(
      L"exercise",
      requester.membership->id,
      registered.objectInstanceHandle,
      efficiencyOnly,
      acquisitionTag);
  REQUIRE(acquisition.status ==
      umbra::detail::AttributeOwnershipAcquisitionIfAvailableStatus::applied);
  REQUIRE(acquisition.requestId != 0U);
  REQUIRE(acquisition.callbackRoute);

  std::vector<unsigned char> const divestitureTag{
      'd', 'i', 'v', '-', 'w', 't', 'a', '-', 'r', 'e', 's', 't', 'a', 'r', 't'};
  auto divestiture = source.planNegotiatedAttributeOwnershipDivestiture(
      L"exercise",
      owner.membership->id,
      registered.objectInstanceHandle,
      efficiencyOnly,
      divestitureTag);
  REQUIRE(divestiture.status ==
      umbra::detail::NegotiatedAttributeOwnershipDivestitureStatus::applied);
  REQUIRE(divestiture.workItems.size() == 1U);
  auto const& sourceConfirmation = divestiture.workItems.front();
  REQUIRE(sourceConfirmation.kind ==
      umbra::detail::AttributeOwnershipAcquisitionWorkKind::request_divestiture_confirmation);
  REQUIRE(sourceConfirmation.requestingFederateId == requester.membership->id);
  REQUIRE(sourceConfirmation.receivingFederateId == owner.membership->id);
  REQUIRE(sourceConfirmation.objectInstanceHandle == registered.objectInstanceHandle);
  REQUIRE(sourceConfirmation.requestId == acquisition.requestId);
  REQUIRE(sourceConfirmation.attributeHandles == efficiencyOnly);
  REQUIRE(sourceConfirmation.userSuppliedTag == acquisitionTag);
  REQUIRE(sourceConfirmation.candidateIsIfAvailable);

  std::wstring const saveLabel = L"ownership-negotiated-if-available-process-restart";
  REQUIRE(source.requestFederationSave(
      L"exercise", owner.membership->id, saveLabel).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(source.federateSaveBegun(
      L"exercise", owner.membership->id).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(source.federateSaveBegun(
      L"exercise", requester.membership->id).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE_FALSE(source.federateSaveComplete(
      L"exercise", owner.membership->id).saveCompletedSuccessfully);
  REQUIRE(source.federateSaveComplete(
      L"exercise", requester.membership->id).saveCompletedSuccessfully);

  auto committed = store->load(L"exercise", saveLabel);
  REQUIRE(committed.has_value());
  auto image = umbra::detail::FederationStateImageCodec::decode(
      committed->stateImage);
  REQUIRE(image.objects.size() == 1U);
  auto const& savedObject = image.objects.front();
  REQUIRE(savedObject.handle == registered.objectInstanceHandle);
  REQUIRE(savedObject.attributeValuesPresent);
  REQUIRE(savedObject.attributeValues.size() == 1U);
  REQUIRE(savedObject.attributeValues.front().attributeHandle == *efficiency);
  REQUIRE(savedObject.attributeValues.front().value == valueBytes);
  REQUIRE(savedObject.pendingAttributeOwnershipAcquisitionRequests.empty());
  REQUIRE(savedObject.pendingAttributeOwnershipAcquisitionIfAvailableRequests.size() == 1U);
  auto const& savedRequest =
      savedObject.pendingAttributeOwnershipAcquisitionIfAvailableRequests.front();
  REQUIRE(savedRequest.requestId == acquisition.requestId);
  REQUIRE(savedRequest.requestingFederateId == requester.membership->id);
  REQUIRE(savedRequest.requestSequence != 0U);
  REQUIRE(savedRequest.desiredAttributeHandles ==
      std::vector<std::uint64_t>{*efficiency});
  REQUIRE(savedRequest.userSuppliedTag ==
      std::string(acquisitionTag.begin(), acquisitionTag.end()));
  REQUIRE(savedObject.pendingNegotiatedAttributeOwnershipDivestitures.size() == 1U);
  auto const& savedDivestiture =
      savedObject.pendingNegotiatedAttributeOwnershipDivestitures.front();
  REQUIRE(savedDivestiture.attributeHandle == *efficiency);
  REQUIRE(savedDivestiture.divestingFederateId == owner.membership->id);
  REQUIRE(savedDivestiture.acquiringFederateId == requester.membership->id);
  REQUIRE(savedDivestiture.acquisitionRequestId == savedRequest.requestId);
  REQUIRE(savedDivestiture.acquiringFederateIsIfAvailable);
  REQUIRE(savedDivestiture.confirmationQueued);
  REQUIRE_FALSE(savedDivestiture.confirmationDelivered);
  REQUIRE(savedDivestiture.userSuppliedTag ==
      std::string(divestitureTag.begin(), divestitureTag.end()));

  EmbeddedFederationRegistry restarted({}, store);
  REQUIRE(restarted.create(L"exercise", composedRestaurantDefinition()).status ==
      FederationRegistryStatus::applied);
  auto restartedOwner = restarted.join(
      L"exercise", L"publisher", L"owner", noOpCallbackRoute());
  auto restartedRequester = restarted.join(
      L"exercise", L"subscriber", L"requester", noOpCallbackRoute());
  REQUIRE(restartedOwner.membership);
  REQUIRE(restartedRequester.membership);
  REQUIRE(restartedOwner.membership->id == owner.membership->id);
  REQUIRE(restartedRequester.membership->id == requester.membership->id);

  REQUIRE(restarted.requestFederationRestore(
      L"exercise", restartedRequester.membership->id, saveLabel).status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  auto firstRestore = restarted.federateRestoreComplete(
      L"exercise", restartedOwner.membership->id);
  REQUIRE(firstRestore.status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  REQUIRE(firstRestore.ownershipAcquisitionWorkItems.empty());
  auto restored = restarted.federateRestoreComplete(
      L"exercise", restartedRequester.membership->id);
  REQUIRE(restored.status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  REQUIRE(restored.ownershipAcquisitionWorkItems.size() == 1U);
  auto const& restoredWork = restored.ownershipAcquisitionWorkItems.front();
  REQUIRE(restoredWork.kind ==
      umbra::detail::AttributeOwnershipAcquisitionWorkKind::request_divestiture_confirmation);
  REQUIRE(restoredWork.requestingFederateId == restartedRequester.membership->id);
  REQUIRE(restoredWork.receivingFederateId == restartedOwner.membership->id);
  REQUIRE(restoredWork.objectInstanceHandle == registered.objectInstanceHandle);
  REQUIRE(restoredWork.requestId == savedRequest.requestId);
  REQUIRE(restoredWork.attributeHandles == efficiencyOnly);
  REQUIRE(restoredWork.userSuppliedTag == acquisitionTag);
  REQUIRE(restoredWork.candidateIsIfAvailable);

  auto restoredOwnerState = restarted.attributeOwnedByFederate(
      L"exercise",
      restartedOwner.membership->id,
      registered.objectInstanceHandle,
      *efficiency);
  REQUIRE(restoredOwnerState.status ==
      umbra::detail::AttributeOwnershipCheckStatus::applied);
  REQUIRE(restoredOwnerState.ownedByRequestingFederate);
  auto confirmation = restarted.beginRequestDivestitureConfirmation(
      L"exercise",
      restartedOwner.membership->id,
      restartedRequester.membership->id,
      registered.objectInstanceHandle,
      restoredWork.requestId,
      true,
      restoredWork.attributeHandles);
  REQUIRE(confirmation.has_value());
  REQUIRE(confirmation->objectInstanceHandle == registered.objectInstanceHandle);
  REQUIRE(confirmation->releasedAttributeHandles == efficiencyOnly);

  auto confirmed = restarted.planConfirmDivestiture(
      L"exercise",
      restartedOwner.membership->id,
      registered.objectInstanceHandle,
      efficiencyOnly,
      divestitureTag);
  REQUIRE(confirmed.status == umbra::detail::ConfirmDivestitureStatus::applied);
  REQUIRE(confirmed.notifications.size() == 1U);
  auto const& notification = confirmed.notifications.front();
  REQUIRE(notification.receivingFederateId == restartedRequester.membership->id);
  REQUIRE(notification.objectInstanceHandle == registered.objectInstanceHandle);
  REQUIRE(notification.attributeHandles == efficiencyOnly);
  auto notificationDelivery = restarted.beginConfirmDivestitureNotification(
      L"exercise",
      restartedRequester.membership->id,
      registered.objectInstanceHandle,
      notification.notificationId,
      notification.attributeHandles);
  REQUIRE(notificationDelivery.has_value());
  REQUIRE(notificationDelivery->securedAttributeHandles == efficiencyOnly);

  auto requesterOwnership = restarted.attributeOwnedByFederate(
      L"exercise",
      restartedRequester.membership->id,
      registered.objectInstanceHandle,
      *efficiency);
  REQUIRE(requesterOwnership.status ==
      umbra::detail::AttributeOwnershipCheckStatus::applied);
  REQUIRE(requesterOwnership.ownedByRequestingFederate);

  std::filesystem::remove_all(directory, ignored);
}

TEST_CASE(
    "Filesystem state image restores a mixed negotiated ownership ledger in a fresh registry",
    "[unit][kernel][federation-registry][save-restore][durable-save][filesystem][restore]"
    "[process-restart][ownership-ledger-state][attribute-ownership-acquisition]"
    "[attribute-ownership-acquisition-if-available][negotiated-attribute-ownership-divestiture]"
    "[process-restart-attribute-ownership-mixed-negotiated-ownership]") {
  auto const directory = temporarySaveCommitDirectory();
  std::error_code ignored;
  std::filesystem::remove_all(directory, ignored);
  auto store = std::make_shared<umbra::detail::FilesystemFederationSaveCommitStore>(
      directory);

  EmbeddedFederationRegistry source({}, store);
  REQUIRE(source.create(L"exercise", composedRestaurantDefinition()).status ==
      FederationRegistryStatus::applied);
  auto owner = source.join(
      L"exercise", L"publisher", L"owner", noOpCallbackRoute());
  auto requester = source.join(
      L"exercise", L"subscriber", L"requester", noOpCallbackRoute());
  REQUIRE(owner.status == FederationRegistryStatus::applied);
  REQUIRE(requester.status == FederationRegistryStatus::applied);
  REQUIRE(owner.membership);
  REQUIRE(requester.membership);

  auto const server = source.objectClassHandleFor(
      L"exercise", "HLAobjectRoot.Employee.Server");
  auto const efficiency = source.attributeHandleFor(
      L"exercise", "HLAobjectRoot.Employee.Server", "Efficiency");
  auto const cheerfulness = source.attributeHandleFor(
      L"exercise", "HLAobjectRoot.Employee.Server", "Cheerfulness");
  REQUIRE(server.has_value());
  REQUIRE(efficiency.has_value());
  REQUIRE(cheerfulness.has_value());
  std::set<std::uint64_t> const mixedAttributes{*efficiency, *cheerfulness};
  std::set<std::uint64_t> const efficiencyOnly{*efficiency};
  std::set<std::uint64_t> const cheerfulnessOnly{*cheerfulness};
  REQUIRE(source.setObjectClassAttributePublication(
      L"exercise", owner.membership->id, *server, mixedAttributes, true) ==
      umbra::detail::ObjectClassAttributeDeclarationStatus::applied);
  REQUIRE(source.setObjectClassAttributeSubscription(
      L"exercise", requester.membership->id, *server, mixedAttributes, true) ==
      umbra::detail::ObjectClassAttributeDeclarationStatus::applied);

  auto registered = source.registerObjectInstance(
      L"exercise", owner.membership->id, *server);
  REQUIRE(registered.status ==
      umbra::detail::ObjectInstanceRegistrationStatus::applied);
  REQUIRE(registered.objectInstanceHandle != 0U);
  auto discoveries = source.planObjectInstanceDiscoveriesForInstance(
      L"exercise", registered.objectInstanceHandle);
  REQUIRE(discoveries.size() == 1U);
  REQUIRE(discoveries.front().receivingFederateId == requester.membership->id);
  REQUIRE(source.beginObjectInstanceDiscovery(
      L"exercise",
      requester.membership->id,
      registered.objectInstanceHandle)
      .has_value());
  REQUIRE(source.setObjectClassAttributePublication(
      L"exercise", requester.membership->id, *server, mixedAttributes, true) ==
      umbra::detail::ObjectClassAttributeDeclarationStatus::applied);

  std::string const efficiencyValue{"\x75\x76", 2U};
  std::string const cheerfulnessValue{"\x77\x78", 2U};
  std::vector<std::pair<std::uint64_t, rti1516_2025::VariableLengthData>> values{
      {*efficiency,
       rti1516_2025::VariableLengthData(efficiencyValue.data(), efficiencyValue.size())},
      {*cheerfulness,
       rti1516_2025::VariableLengthData(cheerfulnessValue.data(), cheerfulnessValue.size())},
  };
  REQUIRE(source.recordSuccessfulUpdateAttributeValues(
      L"exercise",
      owner.membership->id,
      registered.objectInstanceHandle,
      *server,
      {"HLAreliable"},
      &values) == FederationRegistryStatus::applied);

  std::vector<unsigned char> const regularTag{
      'r', 'e', 'g', '-', 'm', 'i', 'x', '-', 'r', 'e', 's', 't', 'a', 'r', 't'};
  auto regular = source.planAttributeOwnershipAcquisition(
      L"exercise",
      requester.membership->id,
      registered.objectInstanceHandle,
      efficiencyOnly,
      regularTag);
  REQUIRE(regular.status ==
      umbra::detail::AttributeOwnershipAcquisitionStatus::applied);
  REQUIRE(regular.workItems.size() == 1U);
  REQUIRE(regular.workItems.front().kind ==
      umbra::detail::AttributeOwnershipAcquisitionWorkKind::request_release);
  auto const regularRequestId = regular.workItems.front().requestId;

  std::vector<unsigned char> const ifAvailableTag{
      'w', 't', 'a', '-', 'm', 'i', 'x', '-', 'r', 'e', 's', 't', 'a', 'r', 't'};
  auto ifAvailable = source.planAttributeOwnershipAcquisitionIfAvailable(
      L"exercise",
      requester.membership->id,
      registered.objectInstanceHandle,
      cheerfulnessOnly,
      ifAvailableTag);
  REQUIRE(ifAvailable.status ==
      umbra::detail::AttributeOwnershipAcquisitionIfAvailableStatus::applied);
  REQUIRE(ifAvailable.requestId != 0U);

  std::vector<unsigned char> const divestitureTag{
      'd', 'i', 'v', '-', 'm', 'i', 'x', '-', 'r', 'e', 's', 't', 'a', 'r', 't'};
  auto divestiture = source.planNegotiatedAttributeOwnershipDivestiture(
      L"exercise",
      owner.membership->id,
      registered.objectInstanceHandle,
      mixedAttributes,
      divestitureTag);
  REQUIRE(divestiture.status ==
      umbra::detail::NegotiatedAttributeOwnershipDivestitureStatus::applied);
  REQUIRE(divestiture.workItems.size() == 2U);
  auto const regularConfirmation = std::ranges::find_if(
      divestiture.workItems,
      [](umbra::detail::AttributeOwnershipAcquisitionWorkItem const& work) {
        return work.candidateIsIfAvailable == false;
      });
  auto const ifAvailableConfirmation = std::ranges::find_if(
      divestiture.workItems,
      [](umbra::detail::AttributeOwnershipAcquisitionWorkItem const& work) {
        return work.candidateIsIfAvailable;
      });
  REQUIRE(regularConfirmation != divestiture.workItems.end());
  REQUIRE(ifAvailableConfirmation != divestiture.workItems.end());
  REQUIRE(regularConfirmation->requestingFederateId == requester.membership->id);
  REQUIRE(regularConfirmation->receivingFederateId == owner.membership->id);
  REQUIRE(regularConfirmation->requestId == regularRequestId);
  REQUIRE(regularConfirmation->attributeHandles == efficiencyOnly);
  REQUIRE(regularConfirmation->userSuppliedTag == regularTag);
  REQUIRE(ifAvailableConfirmation->requestingFederateId == requester.membership->id);
  REQUIRE(ifAvailableConfirmation->receivingFederateId == owner.membership->id);
  REQUIRE(ifAvailableConfirmation->requestId == ifAvailable.requestId);
  REQUIRE(ifAvailableConfirmation->attributeHandles == cheerfulnessOnly);
  REQUIRE(ifAvailableConfirmation->userSuppliedTag == ifAvailableTag);

  std::wstring const saveLabel = L"ownership-negotiated-mixed-process-restart";
  REQUIRE(source.requestFederationSave(
      L"exercise", owner.membership->id, saveLabel).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(source.federateSaveBegun(
      L"exercise", owner.membership->id).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(source.federateSaveBegun(
      L"exercise", requester.membership->id).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE_FALSE(source.federateSaveComplete(
      L"exercise", owner.membership->id).saveCompletedSuccessfully);
  REQUIRE(source.federateSaveComplete(
      L"exercise", requester.membership->id).saveCompletedSuccessfully);

  auto committed = store->load(L"exercise", saveLabel);
  REQUIRE(committed.has_value());
  auto image = umbra::detail::FederationStateImageCodec::decode(
      committed->stateImage);
  REQUIRE(image.objects.size() == 1U);
  auto const& savedObject = image.objects.front();
  REQUIRE(savedObject.handle == registered.objectInstanceHandle);
  REQUIRE(savedObject.attributeValuesPresent);
  REQUIRE(savedObject.attributeValues.size() == 2U);
  REQUIRE(savedObject.pendingAttributeOwnershipAcquisitionRequests.size() == 1U);
  REQUIRE(savedObject.pendingAttributeOwnershipAcquisitionIfAvailableRequests.size() == 1U);
  auto const& savedRegular = savedObject.pendingAttributeOwnershipAcquisitionRequests.front();
  auto const& savedIfAvailable =
      savedObject.pendingAttributeOwnershipAcquisitionIfAvailableRequests.front();
  REQUIRE(savedRegular.requestId == regularRequestId);
  REQUIRE(savedRegular.requestingFederateId == requester.membership->id);
  REQUIRE(savedRegular.desiredAttributeHandles ==
      std::vector<std::uint64_t>{*efficiency});
  REQUIRE(savedRegular.releaseCallbacksQueuedByOwningFederate.size() == 1U);
  REQUIRE(savedIfAvailable.requestId == ifAvailable.requestId);
  REQUIRE(savedIfAvailable.requestingFederateId == requester.membership->id);
  REQUIRE(savedIfAvailable.desiredAttributeHandles ==
      std::vector<std::uint64_t>{*cheerfulness});
  REQUIRE(savedObject.pendingNegotiatedAttributeOwnershipDivestitures.size() == 2U);
  auto const savedRegularDivestiture = std::ranges::find_if(
      savedObject.pendingNegotiatedAttributeOwnershipDivestitures,
      [&](umbra::detail::FederationStateImagePendingNegotiatedAttributeOwnershipDivestiture const& saved) {
        return saved.attributeHandle == *efficiency;
      });
  auto const savedIfAvailableDivestiture = std::ranges::find_if(
      savedObject.pendingNegotiatedAttributeOwnershipDivestitures,
      [&](umbra::detail::FederationStateImagePendingNegotiatedAttributeOwnershipDivestiture const& saved) {
        return saved.attributeHandle == *cheerfulness;
      });
  REQUIRE(savedRegularDivestiture !=
      savedObject.pendingNegotiatedAttributeOwnershipDivestitures.end());
  REQUIRE(savedIfAvailableDivestiture !=
      savedObject.pendingNegotiatedAttributeOwnershipDivestitures.end());
  REQUIRE_FALSE(savedRegularDivestiture->acquiringFederateIsIfAvailable);
  REQUIRE(savedRegularDivestiture->confirmationQueued);
  REQUIRE_FALSE(savedRegularDivestiture->confirmationDelivered);
  REQUIRE(savedIfAvailableDivestiture->acquiringFederateIsIfAvailable);
  REQUIRE(savedIfAvailableDivestiture->confirmationQueued);
  REQUIRE_FALSE(savedIfAvailableDivestiture->confirmationDelivered);

  EmbeddedFederationRegistry restarted({}, store);
  REQUIRE(restarted.create(L"exercise", composedRestaurantDefinition()).status ==
      FederationRegistryStatus::applied);
  auto restartedOwner = restarted.join(
      L"exercise", L"publisher", L"owner", noOpCallbackRoute());
  auto restartedRequester = restarted.join(
      L"exercise", L"subscriber", L"requester", noOpCallbackRoute());
  REQUIRE(restartedOwner.membership);
  REQUIRE(restartedRequester.membership);
  REQUIRE(restartedOwner.membership->id == owner.membership->id);
  REQUIRE(restartedRequester.membership->id == requester.membership->id);
  REQUIRE(restarted.requestFederationRestore(
      L"exercise", restartedRequester.membership->id, saveLabel).status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  auto firstRestore = restarted.federateRestoreComplete(
      L"exercise", restartedOwner.membership->id);
  REQUIRE(firstRestore.status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  REQUIRE(firstRestore.ownershipAcquisitionWorkItems.empty());
  auto restored = restarted.federateRestoreComplete(
      L"exercise", restartedRequester.membership->id);
  REQUIRE(restored.status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  REQUIRE(restored.ownershipAcquisitionWorkItems.size() == 2U);
  auto const restoredRegularConfirmation = std::ranges::find_if(
      restored.ownershipAcquisitionWorkItems,
      [](umbra::detail::AttributeOwnershipAcquisitionWorkItem const& work) {
        return !work.candidateIsIfAvailable;
      });
  auto const restoredIfAvailableConfirmation = std::ranges::find_if(
      restored.ownershipAcquisitionWorkItems,
      [](umbra::detail::AttributeOwnershipAcquisitionWorkItem const& work) {
        return work.candidateIsIfAvailable;
      });
  REQUIRE(restoredRegularConfirmation != restored.ownershipAcquisitionWorkItems.end());
  REQUIRE(restoredIfAvailableConfirmation != restored.ownershipAcquisitionWorkItems.end());
  REQUIRE(restoredRegularConfirmation->kind ==
      umbra::detail::AttributeOwnershipAcquisitionWorkKind::request_divestiture_confirmation);
  REQUIRE(restoredIfAvailableConfirmation->kind ==
      umbra::detail::AttributeOwnershipAcquisitionWorkKind::request_divestiture_confirmation);
  REQUIRE(restoredRegularConfirmation->requestId == savedRegular.requestId);
  REQUIRE(restoredRegularConfirmation->attributeHandles == efficiencyOnly);
  REQUIRE(restoredRegularConfirmation->userSuppliedTag == regularTag);
  REQUIRE(restoredIfAvailableConfirmation->requestId == savedIfAvailable.requestId);
  REQUIRE(restoredIfAvailableConfirmation->attributeHandles == cheerfulnessOnly);
  REQUIRE(restoredIfAvailableConfirmation->userSuppliedTag == ifAvailableTag);

  REQUIRE(restarted.attributeOwnedByFederate(
      L"exercise", restartedOwner.membership->id,
      registered.objectInstanceHandle, *efficiency).ownedByRequestingFederate);
  REQUIRE(restarted.attributeOwnedByFederate(
      L"exercise", restartedOwner.membership->id,
      registered.objectInstanceHandle, *cheerfulness).ownedByRequestingFederate);
  REQUIRE(restarted.beginRequestDivestitureConfirmation(
      L"exercise", restartedOwner.membership->id,
      restartedRequester.membership->id,
      registered.objectInstanceHandle,
      restoredRegularConfirmation->requestId,
      false,
      efficiencyOnly));
  REQUIRE(restarted.beginRequestDivestitureConfirmation(
      L"exercise", restartedOwner.membership->id,
      restartedRequester.membership->id,
      registered.objectInstanceHandle,
      restoredIfAvailableConfirmation->requestId,
      true,
      cheerfulnessOnly));

  auto confirmed = restarted.planConfirmDivestiture(
      L"exercise",
      restartedOwner.membership->id,
      registered.objectInstanceHandle,
      mixedAttributes,
      divestitureTag);
  REQUIRE(confirmed.status == umbra::detail::ConfirmDivestitureStatus::applied);
  REQUIRE(confirmed.notifications.size() == 1U);
  auto const& notification = confirmed.notifications.front();
  REQUIRE(notification.receivingFederateId == restartedRequester.membership->id);
  REQUIRE(notification.attributeHandles == mixedAttributes);
  auto notificationDelivery = restarted.beginConfirmDivestitureNotification(
      L"exercise",
      restartedRequester.membership->id,
      registered.objectInstanceHandle,
      notification.notificationId,
      notification.attributeHandles);
  REQUIRE(notificationDelivery.has_value());
  REQUIRE(notificationDelivery->securedAttributeHandles == mixedAttributes);
  REQUIRE(restarted.attributeOwnedByFederate(
      L"exercise", restartedRequester.membership->id,
      registered.objectInstanceHandle, *efficiency).ownedByRequestingFederate);
  REQUIRE(restarted.attributeOwnedByFederate(
      L"exercise", restartedRequester.membership->id,
      registered.objectInstanceHandle, *cheerfulness).ownedByRequestingFederate);

  std::filesystem::remove_all(directory, ignored);
}

TEST_CASE(
    "Filesystem state image restores the reverse asymmetric mixed negotiated confirmation in a fresh registry",
    "[unit][kernel][federation-registry][save-restore][durable-save][filesystem][restore]"
    "[process-restart][process-restart-asymmetric-mixed-confirmation-reverse][ownership-ledger-state]"
    "[attribute-ownership-acquisition][attribute-ownership-acquisition-if-available]"
    "[negotiated-attribute-ownership-divestiture]") {
  auto const directory = temporarySaveCommitDirectory();
  std::error_code ignored;
  std::filesystem::remove_all(directory, ignored);
  auto store = std::make_shared<umbra::detail::FilesystemFederationSaveCommitStore>(
      directory);

  EmbeddedFederationRegistry source({}, store);
  REQUIRE(source.create(L"exercise", composedRestaurantDefinition()).status ==
      FederationRegistryStatus::applied);
  auto owner = source.join(
      L"exercise", L"publisher", L"owner", noOpCallbackRoute());
  auto requester = source.join(
      L"exercise", L"subscriber", L"requester", noOpCallbackRoute());
  REQUIRE(owner.membership);
  REQUIRE(requester.membership);

  auto const server = source.objectClassHandleFor(
      L"exercise", "HLAobjectRoot.Employee.Server");
  auto const efficiency = source.attributeHandleFor(
      L"exercise", "HLAobjectRoot.Employee.Server", "Efficiency");
  auto const cheerfulness = source.attributeHandleFor(
      L"exercise", "HLAobjectRoot.Employee.Server", "Cheerfulness");
  REQUIRE(server.has_value());
  REQUIRE(efficiency.has_value());
  REQUIRE(cheerfulness.has_value());
  std::set<std::uint64_t> const mixedAttributes{*efficiency, *cheerfulness};
  std::set<std::uint64_t> const efficiencyOnly{*efficiency};
  std::set<std::uint64_t> const cheerfulnessOnly{*cheerfulness};
  REQUIRE(source.setObjectClassAttributePublication(
      L"exercise", owner.membership->id, *server, mixedAttributes, true) ==
      umbra::detail::ObjectClassAttributeDeclarationStatus::applied);
  REQUIRE(source.setObjectClassAttributeSubscription(
      L"exercise", requester.membership->id, *server, mixedAttributes, true) ==
      umbra::detail::ObjectClassAttributeDeclarationStatus::applied);

  auto registered = source.registerObjectInstance(
      L"exercise", owner.membership->id, *server);
  REQUIRE(registered.status ==
      umbra::detail::ObjectInstanceRegistrationStatus::applied);
  auto discoveries = source.planObjectInstanceDiscoveriesForInstance(
      L"exercise", registered.objectInstanceHandle);
  REQUIRE(discoveries.size() == 1U);
  REQUIRE(source.beginObjectInstanceDiscovery(
      L"exercise", requester.membership->id,
      registered.objectInstanceHandle));
  REQUIRE(source.setObjectClassAttributePublication(
      L"exercise", requester.membership->id, *server, mixedAttributes, true) ==
      umbra::detail::ObjectClassAttributeDeclarationStatus::applied);

  std::string const efficiencyValue{"\x85\x86", 2U};
  std::string const cheerfulnessValue{"\x87\x88", 2U};
  std::vector<std::pair<std::uint64_t, rti1516_2025::VariableLengthData>> values{
      {*efficiency,
       rti1516_2025::VariableLengthData(efficiencyValue.data(), efficiencyValue.size())},
      {*cheerfulness,
       rti1516_2025::VariableLengthData(cheerfulnessValue.data(), cheerfulnessValue.size())},
  };
  REQUIRE(source.recordSuccessfulUpdateAttributeValues(
      L"exercise", owner.membership->id, registered.objectInstanceHandle,
      *server, {"HLAreliable"}, &values) == FederationRegistryStatus::applied);

  std::vector<unsigned char> const regularTag{
      'r', 'e', 'g', '-', 'm', 'i', 'x', '-', 'r', 'e', 'v'};
  auto regular = source.planAttributeOwnershipAcquisition(
      L"exercise", requester.membership->id, registered.objectInstanceHandle,
      efficiencyOnly, regularTag);
  REQUIRE(regular.status ==
      umbra::detail::AttributeOwnershipAcquisitionStatus::applied);
  REQUIRE(regular.workItems.size() == 1U);
  auto const regularRequestId = regular.workItems.front().requestId;

  std::vector<unsigned char> const ifAvailableTag{
      'w', 't', 'a', '-', 'm', 'i', 'x', '-', 'r', 'e', 'v'};
  auto ifAvailable = source.planAttributeOwnershipAcquisitionIfAvailable(
      L"exercise", requester.membership->id, registered.objectInstanceHandle,
      cheerfulnessOnly, ifAvailableTag);
  REQUIRE(ifAvailable.status ==
      umbra::detail::AttributeOwnershipAcquisitionIfAvailableStatus::applied);
  REQUIRE(ifAvailable.requestId != 0U);

  std::vector<unsigned char> const divestitureTag{
      'd', 'i', 'v', '-', 'm', 'i', 'x', '-', 'r', 'e', 'v'};
  auto divestiture = source.planNegotiatedAttributeOwnershipDivestiture(
      L"exercise", owner.membership->id, registered.objectInstanceHandle,
      mixedAttributes, divestitureTag);
  REQUIRE(divestiture.status ==
      umbra::detail::NegotiatedAttributeOwnershipDivestitureStatus::applied);
  REQUIRE(divestiture.workItems.size() == 2U);
  auto const regularConfirmation = std::ranges::find_if(
      divestiture.workItems,
      [](umbra::detail::AttributeOwnershipAcquisitionWorkItem const& work) {
        return !work.candidateIsIfAvailable;
      });
  auto const ifAvailableConfirmation = std::ranges::find_if(
      divestiture.workItems,
      [](umbra::detail::AttributeOwnershipAcquisitionWorkItem const& work) {
        return work.candidateIsIfAvailable;
      });
  REQUIRE(regularConfirmation != divestiture.workItems.end());
  REQUIRE(ifAvailableConfirmation != divestiture.workItems.end());
  REQUIRE(regularConfirmation->requestId == regularRequestId);
  REQUIRE(ifAvailableConfirmation->requestId == ifAvailable.requestId);
  REQUIRE(source.beginRequestDivestitureConfirmation(
      L"exercise", owner.membership->id, requester.membership->id,
      registered.objectInstanceHandle, ifAvailable.requestId, true,
      cheerfulnessOnly));
  REQUIRE_FALSE(source.beginRequestDivestitureConfirmation(
      L"exercise", owner.membership->id, requester.membership->id,
      registered.objectInstanceHandle, ifAvailable.requestId, true,
      cheerfulnessOnly));

  std::wstring const saveLabel = L"ownership-negotiated-mixed-asymmetric-reverse-process-restart";
  REQUIRE(source.requestFederationSave(
      L"exercise", owner.membership->id, saveLabel).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(source.federateSaveBegun(
      L"exercise", owner.membership->id).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(source.federateSaveBegun(
      L"exercise", requester.membership->id).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE_FALSE(source.federateSaveComplete(
      L"exercise", owner.membership->id).saveCompletedSuccessfully);
  REQUIRE(source.federateSaveComplete(
      L"exercise", requester.membership->id).saveCompletedSuccessfully);

  auto committed = store->load(L"exercise", saveLabel);
  REQUIRE(committed.has_value());
  auto image = umbra::detail::FederationStateImageCodec::decode(
      committed->stateImage);
  REQUIRE(image.objects.size() == 1U);
  auto const& savedObject = image.objects.front();
  REQUIRE(savedObject.attributeValuesPresent);
  REQUIRE(savedObject.attributeValues.size() == 2U);
  REQUIRE(savedObject.pendingAttributeOwnershipAcquisitionRequests.size() == 1U);
  REQUIRE(savedObject.pendingAttributeOwnershipAcquisitionIfAvailableRequests.size() == 1U);
  REQUIRE(savedObject.pendingNegotiatedAttributeOwnershipDivestitures.size() == 2U);
  auto const savedRegular = std::ranges::find_if(
      savedObject.pendingNegotiatedAttributeOwnershipDivestitures,
      [&](umbra::detail::FederationStateImagePendingNegotiatedAttributeOwnershipDivestiture const& saved) {
        return saved.attributeHandle == *efficiency;
      });
  auto const savedIfAvailable = std::ranges::find_if(
      savedObject.pendingNegotiatedAttributeOwnershipDivestitures,
      [&](umbra::detail::FederationStateImagePendingNegotiatedAttributeOwnershipDivestiture const& saved) {
        return saved.attributeHandle == *cheerfulness;
      });
  REQUIRE(savedRegular != savedObject.pendingNegotiatedAttributeOwnershipDivestitures.end());
  REQUIRE(savedIfAvailable != savedObject.pendingNegotiatedAttributeOwnershipDivestitures.end());
  REQUIRE_FALSE(savedRegular->confirmationDelivered);
  REQUIRE(savedIfAvailable->confirmationQueued);
  REQUIRE(savedIfAvailable->confirmationDelivered);

  EmbeddedFederationRegistry restarted({}, store);
  REQUIRE(restarted.create(L"exercise", composedRestaurantDefinition()).status ==
      FederationRegistryStatus::applied);
  auto restartedOwner = restarted.join(
      L"exercise", L"publisher", L"owner", noOpCallbackRoute());
  auto restartedRequester = restarted.join(
      L"exercise", L"subscriber", L"requester", noOpCallbackRoute());
  REQUIRE(restartedOwner.membership);
  REQUIRE(restartedRequester.membership);
  REQUIRE(restartedOwner.membership->id == owner.membership->id);
  REQUIRE(restartedRequester.membership->id == requester.membership->id);
  REQUIRE(restarted.requestFederationRestore(
      L"exercise", restartedRequester.membership->id, saveLabel).status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  auto firstRestore = restarted.federateRestoreComplete(
      L"exercise", restartedOwner.membership->id);
  REQUIRE(firstRestore.status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  REQUIRE(firstRestore.ownershipAcquisitionWorkItems.empty());
  auto restored = restarted.federateRestoreComplete(
      L"exercise", restartedRequester.membership->id);
  REQUIRE(restored.status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  REQUIRE(restored.ownershipAcquisitionWorkItems.size() == 1U);
  auto const restoredRegularConfirmation = std::ranges::find_if(
      restored.ownershipAcquisitionWorkItems,
      [](umbra::detail::AttributeOwnershipAcquisitionWorkItem const& work) {
        return !work.candidateIsIfAvailable;
      });
  REQUIRE(restoredRegularConfirmation != restored.ownershipAcquisitionWorkItems.end());
  REQUIRE(restoredRegularConfirmation->kind ==
      umbra::detail::AttributeOwnershipAcquisitionWorkKind::request_divestiture_confirmation);
  REQUIRE(restoredRegularConfirmation->requestId == regularRequestId);
  REQUIRE(restoredRegularConfirmation->attributeHandles == efficiencyOnly);
  REQUIRE_FALSE(restarted.beginRequestDivestitureConfirmation(
      L"exercise", restartedOwner.membership->id,
      restartedRequester.membership->id, registered.objectInstanceHandle,
      ifAvailable.requestId, true, cheerfulnessOnly));
  REQUIRE(restarted.beginRequestDivestitureConfirmation(
      L"exercise", restartedOwner.membership->id,
      restartedRequester.membership->id, registered.objectInstanceHandle,
      regularRequestId, false, efficiencyOnly));
  REQUIRE_FALSE(restarted.beginRequestDivestitureConfirmation(
      L"exercise", restartedOwner.membership->id,
      restartedRequester.membership->id, registered.objectInstanceHandle,
      regularRequestId, false, efficiencyOnly));
  REQUIRE(restarted.attributeOwnedByFederate(
      L"exercise", restartedOwner.membership->id,
      registered.objectInstanceHandle, *efficiency).ownedByRequestingFederate);
  REQUIRE(restarted.attributeOwnedByFederate(
      L"exercise", restartedOwner.membership->id,
      registered.objectInstanceHandle, *cheerfulness).ownedByRequestingFederate);

  auto confirmed = restarted.planConfirmDivestiture(
      L"exercise", restartedOwner.membership->id,
      registered.objectInstanceHandle, mixedAttributes, divestitureTag);
  REQUIRE(confirmed.status == umbra::detail::ConfirmDivestitureStatus::applied);
  REQUIRE(confirmed.notifications.size() == 1U);
  auto const& notification = confirmed.notifications.front();
  REQUIRE(notification.attributeHandles == mixedAttributes);
  auto notificationDelivery = restarted.beginConfirmDivestitureNotification(
      L"exercise", restartedRequester.membership->id,
      registered.objectInstanceHandle, notification.notificationId,
      notification.attributeHandles);
  REQUIRE(notificationDelivery.has_value());
  REQUIRE(notificationDelivery->securedAttributeHandles == mixedAttributes);
  REQUIRE(restarted.attributeOwnedByFederate(
      L"exercise", restartedRequester.membership->id,
      registered.objectInstanceHandle, *efficiency).ownedByRequestingFederate);
  REQUIRE(restarted.attributeOwnedByFederate(
      L"exercise", restartedRequester.membership->id,
      registered.objectInstanceHandle, *cheerfulness).ownedByRequestingFederate);

  std::filesystem::remove_all(directory, ignored);
}

TEST_CASE(
    "Filesystem state image restores an asymmetric mixed negotiated confirmation in a fresh registry",
    "[unit][kernel][federation-registry][save-restore][durable-save][filesystem][restore]"
    "[process-restart][process-restart-asymmetric-mixed-confirmation][ownership-ledger-state]"
    "[attribute-ownership-acquisition][attribute-ownership-acquisition-if-available]"
    "[negotiated-attribute-ownership-divestiture]") {
  auto const directory = temporarySaveCommitDirectory();
  std::error_code ignored;
  std::filesystem::remove_all(directory, ignored);
  auto store = std::make_shared<umbra::detail::FilesystemFederationSaveCommitStore>(
      directory);

  EmbeddedFederationRegistry source({}, store);
  REQUIRE(source.create(L"exercise", composedRestaurantDefinition()).status ==
      FederationRegistryStatus::applied);
  auto owner = source.join(
      L"exercise", L"publisher", L"owner", noOpCallbackRoute());
  auto requester = source.join(
      L"exercise", L"subscriber", L"requester", noOpCallbackRoute());
  REQUIRE(owner.membership);
  REQUIRE(requester.membership);

  auto const server = source.objectClassHandleFor(
      L"exercise", "HLAobjectRoot.Employee.Server");
  auto const efficiency = source.attributeHandleFor(
      L"exercise", "HLAobjectRoot.Employee.Server", "Efficiency");
  auto const cheerfulness = source.attributeHandleFor(
      L"exercise", "HLAobjectRoot.Employee.Server", "Cheerfulness");
  REQUIRE(server.has_value());
  REQUIRE(efficiency.has_value());
  REQUIRE(cheerfulness.has_value());
  std::set<std::uint64_t> const mixedAttributes{*efficiency, *cheerfulness};
  std::set<std::uint64_t> const efficiencyOnly{*efficiency};
  std::set<std::uint64_t> const cheerfulnessOnly{*cheerfulness};
  REQUIRE(source.setObjectClassAttributePublication(
      L"exercise", owner.membership->id, *server, mixedAttributes, true) ==
      umbra::detail::ObjectClassAttributeDeclarationStatus::applied);
  REQUIRE(source.setObjectClassAttributeSubscription(
      L"exercise", requester.membership->id, *server, mixedAttributes, true) ==
      umbra::detail::ObjectClassAttributeDeclarationStatus::applied);

  auto registered = source.registerObjectInstance(
      L"exercise", owner.membership->id, *server);
  REQUIRE(registered.status ==
      umbra::detail::ObjectInstanceRegistrationStatus::applied);
  auto discoveries = source.planObjectInstanceDiscoveriesForInstance(
      L"exercise", registered.objectInstanceHandle);
  REQUIRE(discoveries.size() == 1U);
  REQUIRE(source.beginObjectInstanceDiscovery(
      L"exercise", requester.membership->id,
      registered.objectInstanceHandle));
  REQUIRE(source.setObjectClassAttributePublication(
      L"exercise", requester.membership->id, *server, mixedAttributes, true) ==
      umbra::detail::ObjectClassAttributeDeclarationStatus::applied);

  std::string const efficiencyValue{"\x81\x82", 2U};
  std::string const cheerfulnessValue{"\x83\x84", 2U};
  std::vector<std::pair<std::uint64_t, rti1516_2025::VariableLengthData>> values{
      {*efficiency,
       rti1516_2025::VariableLengthData(efficiencyValue.data(), efficiencyValue.size())},
      {*cheerfulness,
       rti1516_2025::VariableLengthData(cheerfulnessValue.data(), cheerfulnessValue.size())},
  };
  REQUIRE(source.recordSuccessfulUpdateAttributeValues(
      L"exercise", owner.membership->id, registered.objectInstanceHandle,
      *server, {"HLAreliable"}, &values) == FederationRegistryStatus::applied);

  std::vector<unsigned char> const regularTag{
      'r', 'e', 'g', '-', 'm', 'i', 'x', '-', 'a', 's', 'y', 'm'};
  auto regular = source.planAttributeOwnershipAcquisition(
      L"exercise", requester.membership->id, registered.objectInstanceHandle,
      efficiencyOnly, regularTag);
  REQUIRE(regular.status ==
      umbra::detail::AttributeOwnershipAcquisitionStatus::applied);
  REQUIRE(regular.workItems.size() == 1U);
  auto const regularRequestId = regular.workItems.front().requestId;

  std::vector<unsigned char> const ifAvailableTag{
      'w', 't', 'a', '-', 'm', 'i', 'x', '-', 'a', 's', 'y', 'm'};
  auto ifAvailable = source.planAttributeOwnershipAcquisitionIfAvailable(
      L"exercise", requester.membership->id, registered.objectInstanceHandle,
      cheerfulnessOnly, ifAvailableTag);
  REQUIRE(ifAvailable.status ==
      umbra::detail::AttributeOwnershipAcquisitionIfAvailableStatus::applied);
  REQUIRE(ifAvailable.requestId != 0U);

  std::vector<unsigned char> const divestitureTag{
      'd', 'i', 'v', '-', 'm', 'i', 'x', '-', 'a', 's', 'y', 'm'};
  auto divestiture = source.planNegotiatedAttributeOwnershipDivestiture(
      L"exercise", owner.membership->id, registered.objectInstanceHandle,
      mixedAttributes, divestitureTag);
  REQUIRE(divestiture.status ==
      umbra::detail::NegotiatedAttributeOwnershipDivestitureStatus::applied);
  REQUIRE(divestiture.workItems.size() == 2U);
  auto const regularConfirmation = std::ranges::find_if(
      divestiture.workItems,
      [](umbra::detail::AttributeOwnershipAcquisitionWorkItem const& work) {
        return !work.candidateIsIfAvailable;
      });
  auto const ifAvailableConfirmation = std::ranges::find_if(
      divestiture.workItems,
      [](umbra::detail::AttributeOwnershipAcquisitionWorkItem const& work) {
        return work.candidateIsIfAvailable;
      });
  REQUIRE(regularConfirmation != divestiture.workItems.end());
  REQUIRE(ifAvailableConfirmation != divestiture.workItems.end());
  REQUIRE(regularConfirmation->requestId == regularRequestId);
  REQUIRE(ifAvailableConfirmation->requestId == ifAvailable.requestId);
  REQUIRE(source.beginRequestDivestitureConfirmation(
      L"exercise", owner.membership->id, requester.membership->id,
      registered.objectInstanceHandle, regularRequestId, false,
      efficiencyOnly));
  REQUIRE_FALSE(source.beginRequestDivestitureConfirmation(
      L"exercise", owner.membership->id, requester.membership->id,
      registered.objectInstanceHandle, regularRequestId, false, efficiencyOnly));

  std::wstring const saveLabel = L"ownership-negotiated-mixed-asymmetric-process-restart";
  REQUIRE(source.requestFederationSave(
      L"exercise", owner.membership->id, saveLabel).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(source.federateSaveBegun(
      L"exercise", owner.membership->id).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(source.federateSaveBegun(
      L"exercise", requester.membership->id).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE_FALSE(source.federateSaveComplete(
      L"exercise", owner.membership->id).saveCompletedSuccessfully);
  REQUIRE(source.federateSaveComplete(
      L"exercise", requester.membership->id).saveCompletedSuccessfully);

  auto committed = store->load(L"exercise", saveLabel);
  REQUIRE(committed.has_value());
  auto image = umbra::detail::FederationStateImageCodec::decode(
      committed->stateImage);
  REQUIRE(image.objects.size() == 1U);
  auto const& savedObject = image.objects.front();
  REQUIRE(savedObject.attributeValuesPresent);
  REQUIRE(savedObject.attributeValues.size() == 2U);
  REQUIRE(savedObject.pendingAttributeOwnershipAcquisitionRequests.size() == 1U);
  REQUIRE(savedObject.pendingAttributeOwnershipAcquisitionIfAvailableRequests.size() == 1U);
  REQUIRE(savedObject.pendingNegotiatedAttributeOwnershipDivestitures.size() == 2U);
  auto const savedRegular = std::ranges::find_if(
      savedObject.pendingNegotiatedAttributeOwnershipDivestitures,
      [&](umbra::detail::FederationStateImagePendingNegotiatedAttributeOwnershipDivestiture const& saved) {
        return saved.attributeHandle == *efficiency;
      });
  auto const savedIfAvailable = std::ranges::find_if(
      savedObject.pendingNegotiatedAttributeOwnershipDivestitures,
      [&](umbra::detail::FederationStateImagePendingNegotiatedAttributeOwnershipDivestiture const& saved) {
        return saved.attributeHandle == *cheerfulness;
      });
  REQUIRE(savedRegular != savedObject.pendingNegotiatedAttributeOwnershipDivestitures.end());
  REQUIRE(savedIfAvailable != savedObject.pendingNegotiatedAttributeOwnershipDivestitures.end());
  REQUIRE(savedRegular->confirmationQueued);
  REQUIRE(savedRegular->confirmationDelivered);
  REQUIRE(savedIfAvailable->acquiringFederateIsIfAvailable);
  REQUIRE(savedIfAvailable->confirmationQueued);
  REQUIRE_FALSE(savedIfAvailable->confirmationDelivered);

  EmbeddedFederationRegistry restarted({}, store);
  REQUIRE(restarted.create(L"exercise", composedRestaurantDefinition()).status ==
      FederationRegistryStatus::applied);
  auto restartedOwner = restarted.join(
      L"exercise", L"publisher", L"owner", noOpCallbackRoute());
  auto restartedRequester = restarted.join(
      L"exercise", L"subscriber", L"requester", noOpCallbackRoute());
  REQUIRE(restartedOwner.membership);
  REQUIRE(restartedRequester.membership);
  REQUIRE(restartedOwner.membership->id == owner.membership->id);
  REQUIRE(restartedRequester.membership->id == requester.membership->id);
  REQUIRE(restarted.requestFederationRestore(
      L"exercise", restartedRequester.membership->id, saveLabel).status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  auto firstRestore = restarted.federateRestoreComplete(
      L"exercise", restartedOwner.membership->id);
  REQUIRE(firstRestore.status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  REQUIRE(firstRestore.ownershipAcquisitionWorkItems.empty());
  auto restored = restarted.federateRestoreComplete(
      L"exercise", restartedRequester.membership->id);
  REQUIRE(restored.status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  REQUIRE(restored.ownershipAcquisitionWorkItems.size() == 1U);
  auto const restoredIfAvailableConfirmation = std::ranges::find_if(
      restored.ownershipAcquisitionWorkItems,
      [](umbra::detail::AttributeOwnershipAcquisitionWorkItem const& work) {
        return work.candidateIsIfAvailable;
      });
  REQUIRE(restoredIfAvailableConfirmation != restored.ownershipAcquisitionWorkItems.end());
  REQUIRE(restoredIfAvailableConfirmation->kind ==
      umbra::detail::AttributeOwnershipAcquisitionWorkKind::request_divestiture_confirmation);
  REQUIRE(restoredIfAvailableConfirmation->requestId == ifAvailable.requestId);
  REQUIRE(restoredIfAvailableConfirmation->attributeHandles == cheerfulnessOnly);
  REQUIRE_FALSE(restarted.beginRequestDivestitureConfirmation(
      L"exercise", restartedOwner.membership->id,
      restartedRequester.membership->id, registered.objectInstanceHandle,
      regularRequestId, false, efficiencyOnly));
  REQUIRE(restarted.beginRequestDivestitureConfirmation(
      L"exercise", restartedOwner.membership->id,
      restartedRequester.membership->id, registered.objectInstanceHandle,
      ifAvailable.requestId, true, cheerfulnessOnly));
  REQUIRE_FALSE(restarted.beginRequestDivestitureConfirmation(
      L"exercise", restartedOwner.membership->id,
      restartedRequester.membership->id, registered.objectInstanceHandle,
      ifAvailable.requestId, true, cheerfulnessOnly));
  REQUIRE(restarted.attributeOwnedByFederate(
      L"exercise", restartedOwner.membership->id,
      registered.objectInstanceHandle, *efficiency).ownedByRequestingFederate);
  REQUIRE(restarted.attributeOwnedByFederate(
      L"exercise", restartedOwner.membership->id,
      registered.objectInstanceHandle, *cheerfulness).ownedByRequestingFederate);

  auto confirmed = restarted.planConfirmDivestiture(
      L"exercise", restartedOwner.membership->id,
      registered.objectInstanceHandle, mixedAttributes, divestitureTag);
  REQUIRE(confirmed.status == umbra::detail::ConfirmDivestitureStatus::applied);
  REQUIRE(confirmed.notifications.size() == 1U);
  auto const& notification = confirmed.notifications.front();
  REQUIRE(notification.attributeHandles == mixedAttributes);
  auto notificationDelivery = restarted.beginConfirmDivestitureNotification(
      L"exercise", restartedRequester.membership->id,
      registered.objectInstanceHandle, notification.notificationId,
      notification.attributeHandles);
  REQUIRE(notificationDelivery.has_value());
  REQUIRE(notificationDelivery->securedAttributeHandles == mixedAttributes);
  REQUIRE(restarted.attributeOwnedByFederate(
      L"exercise", restartedRequester.membership->id,
      registered.objectInstanceHandle, *efficiency).ownedByRequestingFederate);
  REQUIRE(restarted.attributeOwnedByFederate(
      L"exercise", restartedRequester.membership->id,
      registered.objectInstanceHandle, *cheerfulness).ownedByRequestingFederate);

  std::filesystem::remove_all(directory, ignored);
}

TEST_CASE(
    "Filesystem state image preserves mixed delivered negotiated confirmations in a fresh registry",
    "[unit][kernel][federation-registry][save-restore][durable-save][filesystem][restore]"
    "[process-restart][ownership-ledger-state][attribute-ownership-acquisition]"
    "[attribute-ownership-acquisition-if-available][negotiated-attribute-ownership-divestiture]"
    "[process-restart-mixed-confirmation-delivered][process-restart-negotiated-mixed-confirmation-delivered]") {
  auto const directory = temporarySaveCommitDirectory();
  std::error_code ignored;
  std::filesystem::remove_all(directory, ignored);
  auto store = std::make_shared<umbra::detail::FilesystemFederationSaveCommitStore>(
      directory);

  EmbeddedFederationRegistry source({}, store);
  REQUIRE(source.create(L"exercise", composedRestaurantDefinition()).status ==
      FederationRegistryStatus::applied);
  auto owner = source.join(
      L"exercise", L"publisher", L"owner", noOpCallbackRoute());
  auto requester = source.join(
      L"exercise", L"subscriber", L"requester", noOpCallbackRoute());
  REQUIRE(owner.membership);
  REQUIRE(requester.membership);

  auto const server = source.objectClassHandleFor(
      L"exercise", "HLAobjectRoot.Employee.Server");
  auto const efficiency = source.attributeHandleFor(
      L"exercise", "HLAobjectRoot.Employee.Server", "Efficiency");
  auto const cheerfulness = source.attributeHandleFor(
      L"exercise", "HLAobjectRoot.Employee.Server", "Cheerfulness");
  REQUIRE(server.has_value());
  REQUIRE(efficiency.has_value());
  REQUIRE(cheerfulness.has_value());
  std::set<std::uint64_t> const mixedAttributes{*efficiency, *cheerfulness};
  std::set<std::uint64_t> const efficiencyOnly{*efficiency};
  std::set<std::uint64_t> const cheerfulnessOnly{*cheerfulness};
  REQUIRE(source.setObjectClassAttributePublication(
      L"exercise", owner.membership->id, *server, mixedAttributes, true) ==
      umbra::detail::ObjectClassAttributeDeclarationStatus::applied);
  REQUIRE(source.setObjectClassAttributeSubscription(
      L"exercise", requester.membership->id, *server, mixedAttributes, true) ==
      umbra::detail::ObjectClassAttributeDeclarationStatus::applied);

  auto registered = source.registerObjectInstance(
      L"exercise", owner.membership->id, *server);
  REQUIRE(registered.status ==
      umbra::detail::ObjectInstanceRegistrationStatus::applied);
  auto discoveries = source.planObjectInstanceDiscoveriesForInstance(
      L"exercise", registered.objectInstanceHandle);
  REQUIRE(discoveries.size() == 1U);
  REQUIRE(source.beginObjectInstanceDiscovery(
      L"exercise", requester.membership->id,
      registered.objectInstanceHandle));
  REQUIRE(source.setObjectClassAttributePublication(
      L"exercise", requester.membership->id, *server, mixedAttributes, true) ==
      umbra::detail::ObjectClassAttributeDeclarationStatus::applied);

  std::string const efficiencyValue{"\x79\x7a", 2U};
  std::string const cheerfulnessValue{"\x7d\x7e", 2U};
  std::vector<std::pair<std::uint64_t, rti1516_2025::VariableLengthData>> values{
      {*efficiency,
       rti1516_2025::VariableLengthData(efficiencyValue.data(), efficiencyValue.size())},
      {*cheerfulness,
       rti1516_2025::VariableLengthData(cheerfulnessValue.data(), cheerfulnessValue.size())},
  };
  REQUIRE(source.recordSuccessfulUpdateAttributeValues(
      L"exercise", owner.membership->id, registered.objectInstanceHandle,
      *server, {"HLAreliable"}, &values) == FederationRegistryStatus::applied);

  std::vector<unsigned char> const regularTag{
      'r', 'e', 'g', '-', 'm', 'i', 'x', '-', 'd', 'e', 'l', 'i', 'v', 'e', 'r', 'e', 'd'};
  auto regular = source.planAttributeOwnershipAcquisition(
      L"exercise", requester.membership->id, registered.objectInstanceHandle,
      efficiencyOnly, regularTag);
  REQUIRE(regular.status ==
      umbra::detail::AttributeOwnershipAcquisitionStatus::applied);
  REQUIRE(regular.workItems.size() == 1U);
  auto const regularRequestId = regular.workItems.front().requestId;

  std::vector<unsigned char> const ifAvailableTag{
      'w', 't', 'a', '-', 'm', 'i', 'x', '-', 'd', 'e', 'l', 'i', 'v', 'e', 'r', 'e', 'd'};
  auto ifAvailable = source.planAttributeOwnershipAcquisitionIfAvailable(
      L"exercise", requester.membership->id, registered.objectInstanceHandle,
      cheerfulnessOnly, ifAvailableTag);
  REQUIRE(ifAvailable.status ==
      umbra::detail::AttributeOwnershipAcquisitionIfAvailableStatus::applied);
  REQUIRE(ifAvailable.requestId != 0U);

  std::vector<unsigned char> const divestitureTag{
      'd', 'i', 'v', '-', 'm', 'i', 'x', '-', 'd', 'e', 'l', 'i', 'v', 'e', 'r', 'e', 'd'};
  auto divestiture = source.planNegotiatedAttributeOwnershipDivestiture(
      L"exercise", owner.membership->id, registered.objectInstanceHandle,
      mixedAttributes, divestitureTag);
  REQUIRE(divestiture.status ==
      umbra::detail::NegotiatedAttributeOwnershipDivestitureStatus::applied);
  REQUIRE(divestiture.workItems.size() == 2U);
  auto const regularConfirmation = std::ranges::find_if(
      divestiture.workItems,
      [](umbra::detail::AttributeOwnershipAcquisitionWorkItem const& work) {
        return !work.candidateIsIfAvailable;
      });
  auto const ifAvailableConfirmation = std::ranges::find_if(
      divestiture.workItems,
      [](umbra::detail::AttributeOwnershipAcquisitionWorkItem const& work) {
        return work.candidateIsIfAvailable;
      });
  REQUIRE(regularConfirmation != divestiture.workItems.end());
  REQUIRE(ifAvailableConfirmation != divestiture.workItems.end());
  REQUIRE(regularConfirmation->requestId == regularRequestId);
  REQUIRE(ifAvailableConfirmation->requestId == ifAvailable.requestId);
  REQUIRE(regularConfirmation->attributeHandles == efficiencyOnly);
  REQUIRE(ifAvailableConfirmation->attributeHandles == cheerfulnessOnly);
  REQUIRE(source.beginRequestDivestitureConfirmation(
      L"exercise", owner.membership->id, requester.membership->id,
      registered.objectInstanceHandle, regularRequestId, false,
      efficiencyOnly));
  REQUIRE(source.beginRequestDivestitureConfirmation(
      L"exercise", owner.membership->id, requester.membership->id,
      registered.objectInstanceHandle, ifAvailable.requestId, true,
      cheerfulnessOnly));
  REQUIRE_FALSE(source.beginRequestDivestitureConfirmation(
      L"exercise", owner.membership->id, requester.membership->id,
      registered.objectInstanceHandle, regularRequestId, false, efficiencyOnly));
  REQUIRE_FALSE(source.beginRequestDivestitureConfirmation(
      L"exercise", owner.membership->id, requester.membership->id,
      registered.objectInstanceHandle, ifAvailable.requestId, true, cheerfulnessOnly));

  std::wstring const saveLabel = L"ownership-negotiated-mixed-delivered-process-restart";
  REQUIRE(source.requestFederationSave(
      L"exercise", owner.membership->id, saveLabel).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(source.federateSaveBegun(
      L"exercise", owner.membership->id).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(source.federateSaveBegun(
      L"exercise", requester.membership->id).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE_FALSE(source.federateSaveComplete(
      L"exercise", owner.membership->id).saveCompletedSuccessfully);
  REQUIRE(source.federateSaveComplete(
      L"exercise", requester.membership->id).saveCompletedSuccessfully);

  auto committed = store->load(L"exercise", saveLabel);
  REQUIRE(committed.has_value());
  auto image = umbra::detail::FederationStateImageCodec::decode(
      committed->stateImage);
  REQUIRE(image.objects.size() == 1U);
  auto const& savedObject = image.objects.front();
  REQUIRE(savedObject.attributeValuesPresent);
  REQUIRE(savedObject.attributeValues.size() == 2U);
  REQUIRE(savedObject.pendingAttributeOwnershipAcquisitionRequests.size() == 1U);
  REQUIRE(savedObject.pendingAttributeOwnershipAcquisitionIfAvailableRequests.size() == 1U);
  REQUIRE(savedObject.pendingNegotiatedAttributeOwnershipDivestitures.size() == 2U);
  auto const savedRegular = std::ranges::find_if(
      savedObject.pendingNegotiatedAttributeOwnershipDivestitures,
      [&](umbra::detail::FederationStateImagePendingNegotiatedAttributeOwnershipDivestiture const& saved) {
        return saved.attributeHandle == *efficiency;
      });
  auto const savedIfAvailable = std::ranges::find_if(
      savedObject.pendingNegotiatedAttributeOwnershipDivestitures,
      [&](umbra::detail::FederationStateImagePendingNegotiatedAttributeOwnershipDivestiture const& saved) {
        return saved.attributeHandle == *cheerfulness;
      });
  REQUIRE(savedRegular != savedObject.pendingNegotiatedAttributeOwnershipDivestitures.end());
  REQUIRE(savedIfAvailable != savedObject.pendingNegotiatedAttributeOwnershipDivestitures.end());
  REQUIRE(savedRegular->confirmationQueued);
  REQUIRE(savedRegular->confirmationDelivered);
  REQUIRE(savedIfAvailable->acquiringFederateIsIfAvailable);
  REQUIRE(savedIfAvailable->confirmationQueued);
  REQUIRE(savedIfAvailable->confirmationDelivered);

  EmbeddedFederationRegistry restarted({}, store);
  REQUIRE(restarted.create(L"exercise", composedRestaurantDefinition()).status ==
      FederationRegistryStatus::applied);
  auto restartedOwner = restarted.join(
      L"exercise", L"publisher", L"owner", noOpCallbackRoute());
  auto restartedRequester = restarted.join(
      L"exercise", L"subscriber", L"requester", noOpCallbackRoute());
  REQUIRE(restartedOwner.membership);
  REQUIRE(restartedRequester.membership);
  REQUIRE(restartedOwner.membership->id == owner.membership->id);
  REQUIRE(restartedRequester.membership->id == requester.membership->id);
  REQUIRE(restarted.requestFederationRestore(
      L"exercise", restartedRequester.membership->id, saveLabel).status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  auto firstRestore = restarted.federateRestoreComplete(
      L"exercise", restartedOwner.membership->id);
  REQUIRE(firstRestore.status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  REQUIRE(firstRestore.ownershipAcquisitionWorkItems.empty());
  auto restored = restarted.federateRestoreComplete(
      L"exercise", restartedRequester.membership->id);
  REQUIRE(restored.status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  REQUIRE(restored.ownershipAcquisitionWorkItems.empty());
  REQUIRE_FALSE(restarted.beginRequestDivestitureConfirmation(
      L"exercise", restartedOwner.membership->id,
      restartedRequester.membership->id, registered.objectInstanceHandle,
      regularRequestId, false, efficiencyOnly));
  REQUIRE_FALSE(restarted.beginRequestDivestitureConfirmation(
      L"exercise", restartedOwner.membership->id,
      restartedRequester.membership->id, registered.objectInstanceHandle,
      ifAvailable.requestId, true, cheerfulnessOnly));
  REQUIRE(restarted.attributeOwnedByFederate(
      L"exercise", restartedOwner.membership->id,
      registered.objectInstanceHandle, *efficiency).ownedByRequestingFederate);
  REQUIRE(restarted.attributeOwnedByFederate(
      L"exercise", restartedOwner.membership->id,
      registered.objectInstanceHandle, *cheerfulness).ownedByRequestingFederate);

  auto confirmed = restarted.planConfirmDivestiture(
      L"exercise", restartedOwner.membership->id,
      registered.objectInstanceHandle, mixedAttributes, divestitureTag);
  REQUIRE(confirmed.status == umbra::detail::ConfirmDivestitureStatus::applied);
  REQUIRE(confirmed.notifications.size() == 1U);
  auto const& notification = confirmed.notifications.front();
  REQUIRE(notification.attributeHandles == mixedAttributes);
  auto notificationDelivery = restarted.beginConfirmDivestitureNotification(
      L"exercise", restartedRequester.membership->id,
      registered.objectInstanceHandle, notification.notificationId,
      notification.attributeHandles);
  REQUIRE(notificationDelivery.has_value());
  REQUIRE(notificationDelivery->securedAttributeHandles == mixedAttributes);
  REQUIRE(restarted.attributeOwnedByFederate(
      L"exercise", restartedRequester.membership->id,
      registered.objectInstanceHandle, *efficiency).ownedByRequestingFederate);
  REQUIRE(restarted.attributeOwnedByFederate(
      L"exercise", restartedRequester.membership->id,
      registered.objectInstanceHandle, *cheerfulness).ownedByRequestingFederate);

  std::filesystem::remove_all(directory, ignored);
}

TEST_CASE(
    "Filesystem state image preserves delivered negotiated owner confirmation in a fresh registry",
    "[unit][kernel][federation-registry][save-restore][durable-save][filesystem][restore]"
    "[process-restart][process-restart-negotiated-confirmation-delivered][ownership-ledger-state][attribute-ownership-acquisition]"
    "[negotiated-attribute-ownership-divestiture]") {
  auto const directory = temporarySaveCommitDirectory();
  std::error_code ignored;
  std::filesystem::remove_all(directory, ignored);
  auto store = std::make_shared<umbra::detail::FilesystemFederationSaveCommitStore>(
      directory);

  EmbeddedFederationRegistry source({}, store);
  REQUIRE(source.create(L"exercise", composedRestaurantDefinition()).status ==
      FederationRegistryStatus::applied);
  auto owner = source.join(
      L"exercise", L"publisher", L"owner", noOpCallbackRoute());
  auto requester = source.join(
      L"exercise", L"subscriber", L"requester", noOpCallbackRoute());
  REQUIRE(owner.status == FederationRegistryStatus::applied);
  REQUIRE(requester.status == FederationRegistryStatus::applied);
  REQUIRE(owner.membership);
  REQUIRE(requester.membership);

  auto const server = source.objectClassHandleFor(
      L"exercise", "HLAobjectRoot.Employee.Server");
  auto const efficiency = source.attributeHandleFor(
      L"exercise", "HLAobjectRoot.Employee.Server", "Efficiency");
  REQUIRE(server.has_value());
  REQUIRE(efficiency.has_value());
  std::set<std::uint64_t> const efficiencyOnly{*efficiency};
  REQUIRE(source.setObjectClassAttributePublication(
      L"exercise", owner.membership->id, *server, efficiencyOnly, true) ==
      umbra::detail::ObjectClassAttributeDeclarationStatus::applied);
  REQUIRE(source.setObjectClassAttributeSubscription(
      L"exercise", requester.membership->id, *server, efficiencyOnly, true) ==
      umbra::detail::ObjectClassAttributeDeclarationStatus::applied);

  auto registered = source.registerObjectInstance(
      L"exercise", owner.membership->id, *server);
  REQUIRE(registered.status ==
      umbra::detail::ObjectInstanceRegistrationStatus::applied);
  REQUIRE(registered.objectInstanceHandle != 0U);
  auto discoveries = source.planObjectInstanceDiscoveriesForInstance(
      L"exercise", registered.objectInstanceHandle);
  REQUIRE(discoveries.size() == 1U);
  REQUIRE(discoveries.front().receivingFederateId == requester.membership->id);
  REQUIRE(source.beginObjectInstanceDiscovery(
      L"exercise",
      requester.membership->id,
      registered.objectInstanceHandle)
      .has_value());
  REQUIRE(source.setObjectClassAttributePublication(
      L"exercise", requester.membership->id, *server, efficiencyOnly, true) ==
      umbra::detail::ObjectClassAttributeDeclarationStatus::applied);

  std::string const valueBytes{"\x79\x7a", 2U};
  std::vector<std::pair<std::uint64_t, rti1516_2025::VariableLengthData>> values{{
      *efficiency,
      rti1516_2025::VariableLengthData(valueBytes.data(), valueBytes.size()),
  }};
  REQUIRE(source.recordSuccessfulUpdateAttributeValues(
      L"exercise",
      owner.membership->id,
      registered.objectInstanceHandle,
      *server,
      {"HLAreliable"},
      &values) == FederationRegistryStatus::applied);

  std::vector<unsigned char> const acquisitionTag{
      'r', 'e', 'g', '-', 'd', 'e', 'l', 'i', 'v', 'e', 'r', 'e', 'd'};
  auto acquisition = source.planAttributeOwnershipAcquisition(
      L"exercise",
      requester.membership->id,
      registered.objectInstanceHandle,
      efficiencyOnly,
      acquisitionTag);
  REQUIRE(acquisition.status ==
      umbra::detail::AttributeOwnershipAcquisitionStatus::applied);
  REQUIRE(acquisition.workItems.size() == 1U);
  REQUIRE(acquisition.workItems.front().kind ==
      umbra::detail::AttributeOwnershipAcquisitionWorkKind::request_release);
  auto const acquisitionRequestId = acquisition.workItems.front().requestId;

  std::vector<unsigned char> const divestitureTag{
      'd', 'i', 'v', '-', 'd', 'e', 'l', 'i', 'v', 'e', 'r', 'e', 'd'};
  auto divestiture = source.planNegotiatedAttributeOwnershipDivestiture(
      L"exercise",
      owner.membership->id,
      registered.objectInstanceHandle,
      efficiencyOnly,
      divestitureTag);
  REQUIRE(divestiture.status ==
      umbra::detail::NegotiatedAttributeOwnershipDivestitureStatus::applied);
  REQUIRE(divestiture.workItems.size() == 1U);
  auto const& sourceConfirmation = divestiture.workItems.front();
  REQUIRE(sourceConfirmation.requestId == acquisitionRequestId);
  REQUIRE_FALSE(sourceConfirmation.candidateIsIfAvailable);
  auto deliveredBeforeSave = source.beginRequestDivestitureConfirmation(
      L"exercise",
      owner.membership->id,
      requester.membership->id,
      registered.objectInstanceHandle,
      sourceConfirmation.requestId,
      false,
      sourceConfirmation.attributeHandles);
  REQUIRE(deliveredBeforeSave.has_value());
  REQUIRE(deliveredBeforeSave->releasedAttributeHandles == efficiencyOnly);
  REQUIRE_FALSE(source.beginRequestDivestitureConfirmation(
      L"exercise",
      owner.membership->id,
      requester.membership->id,
      registered.objectInstanceHandle,
      sourceConfirmation.requestId,
      false,
      sourceConfirmation.attributeHandles));

  std::wstring const saveLabel = L"ownership-negotiated-delivered-process-restart";
  REQUIRE(source.requestFederationSave(
      L"exercise", owner.membership->id, saveLabel).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(source.federateSaveBegun(
      L"exercise", owner.membership->id).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(source.federateSaveBegun(
      L"exercise", requester.membership->id).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE_FALSE(source.federateSaveComplete(
      L"exercise", owner.membership->id).saveCompletedSuccessfully);
  REQUIRE(source.federateSaveComplete(
      L"exercise", requester.membership->id).saveCompletedSuccessfully);

  auto committed = store->load(L"exercise", saveLabel);
  REQUIRE(committed.has_value());
  auto image = umbra::detail::FederationStateImageCodec::decode(
      committed->stateImage);
  REQUIRE(image.objects.size() == 1U);
  auto const& savedObject = image.objects.front();
  REQUIRE(savedObject.attributeValuesPresent);
  REQUIRE(savedObject.attributeValues.size() == 1U);
  REQUIRE(savedObject.attributeValues.front().value == valueBytes);
  REQUIRE(savedObject.pendingAttributeOwnershipAcquisitionRequests.size() == 1U);
  REQUIRE(savedObject.pendingNegotiatedAttributeOwnershipDivestitures.size() == 1U);
  auto const& savedRequest = savedObject.pendingAttributeOwnershipAcquisitionRequests.front();
  auto const& savedDivestiture =
      savedObject.pendingNegotiatedAttributeOwnershipDivestitures.front();
  REQUIRE(savedRequest.requestId == acquisitionRequestId);
  REQUIRE(savedRequest.releaseCallbacksQueuedByOwningFederate.size() == 1U);
  REQUIRE(savedDivestiture.acquisitionRequestId == acquisitionRequestId);
  REQUIRE(savedDivestiture.confirmationQueued);
  REQUIRE(savedDivestiture.confirmationDelivered);

  EmbeddedFederationRegistry restarted({}, store);
  REQUIRE(restarted.create(L"exercise", composedRestaurantDefinition()).status ==
      FederationRegistryStatus::applied);
  auto restartedOwner = restarted.join(
      L"exercise", L"publisher", L"owner", noOpCallbackRoute());
  auto restartedRequester = restarted.join(
      L"exercise", L"subscriber", L"requester", noOpCallbackRoute());
  REQUIRE(restartedOwner.membership);
  REQUIRE(restartedRequester.membership);
  REQUIRE(restartedOwner.membership->id == owner.membership->id);
  REQUIRE(restartedRequester.membership->id == requester.membership->id);
  REQUIRE(restarted.requestFederationRestore(
      L"exercise", restartedRequester.membership->id, saveLabel).status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  auto firstRestore = restarted.federateRestoreComplete(
      L"exercise", restartedOwner.membership->id);
  REQUIRE(firstRestore.status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  REQUIRE(firstRestore.ownershipAcquisitionWorkItems.empty());
  auto restored = restarted.federateRestoreComplete(
      L"exercise", restartedRequester.membership->id);
  REQUIRE(restored.status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  REQUIRE(restored.ownershipAcquisitionWorkItems.empty());
  REQUIRE_FALSE(restarted.beginRequestDivestitureConfirmation(
      L"exercise",
      restartedOwner.membership->id,
      restartedRequester.membership->id,
      registered.objectInstanceHandle,
      acquisitionRequestId,
      false,
      efficiencyOnly));

  auto restoredOwnerState = restarted.attributeOwnedByFederate(
      L"exercise",
      restartedOwner.membership->id,
      registered.objectInstanceHandle,
      *efficiency);
  REQUIRE(restoredOwnerState.status ==
      umbra::detail::AttributeOwnershipCheckStatus::applied);
  REQUIRE(restoredOwnerState.ownedByRequestingFederate);
  auto confirmed = restarted.planConfirmDivestiture(
      L"exercise",
      restartedOwner.membership->id,
      registered.objectInstanceHandle,
      efficiencyOnly,
      divestitureTag);
  REQUIRE(confirmed.status == umbra::detail::ConfirmDivestitureStatus::applied);
  REQUIRE(confirmed.notifications.size() == 1U);
  auto const& notification = confirmed.notifications.front();
  auto notificationDelivery = restarted.beginConfirmDivestitureNotification(
      L"exercise",
      restartedRequester.membership->id,
      registered.objectInstanceHandle,
      notification.notificationId,
      notification.attributeHandles);
  REQUIRE(notificationDelivery.has_value());
  REQUIRE(notificationDelivery->securedAttributeHandles == efficiencyOnly);
  auto requesterOwnership = restarted.attributeOwnedByFederate(
      L"exercise",
      restartedRequester.membership->id,
      registered.objectInstanceHandle,
      *efficiency);
  REQUIRE(requesterOwnership.status ==
      umbra::detail::AttributeOwnershipCheckStatus::applied);
  REQUIRE(requesterOwnership.ownedByRequestingFederate);

  std::filesystem::remove_all(directory, ignored);
}

TEST_CASE(
    "Filesystem state image preserves delivered negotiated If Available confirmation in a fresh registry",
    "[unit][kernel][federation-registry][save-restore][durable-save][filesystem][restore]"
    "[process-restart][process-restart-negotiated-if-available-confirmation-delivered]"
    "[ownership-ledger-state][attribute-ownership-acquisition]"
    "[attribute-ownership-acquisition-if-available][negotiated-attribute-ownership-divestiture]") {
  auto const directory = temporarySaveCommitDirectory();
  std::error_code ignored;
  std::filesystem::remove_all(directory, ignored);
  auto store = std::make_shared<umbra::detail::FilesystemFederationSaveCommitStore>(
      directory);

  EmbeddedFederationRegistry source({}, store);
  REQUIRE(source.create(L"exercise", composedRestaurantDefinition()).status ==
      FederationRegistryStatus::applied);
  auto owner = source.join(
      L"exercise", L"publisher", L"owner", noOpCallbackRoute());
  auto requester = source.join(
      L"exercise", L"subscriber", L"requester", noOpCallbackRoute());
  REQUIRE(owner.status == FederationRegistryStatus::applied);
  REQUIRE(requester.status == FederationRegistryStatus::applied);
  REQUIRE(owner.membership);
  REQUIRE(requester.membership);

  auto const server = source.objectClassHandleFor(
      L"exercise", "HLAobjectRoot.Employee.Server");
  auto const efficiency = source.attributeHandleFor(
      L"exercise", "HLAobjectRoot.Employee.Server", "Efficiency");
  REQUIRE(server.has_value());
  REQUIRE(efficiency.has_value());
  std::set<std::uint64_t> const efficiencyOnly{*efficiency};
  REQUIRE(source.setObjectClassAttributePublication(
      L"exercise", owner.membership->id, *server, efficiencyOnly, true) ==
      umbra::detail::ObjectClassAttributeDeclarationStatus::applied);
  REQUIRE(source.setObjectClassAttributeSubscription(
      L"exercise", requester.membership->id, *server, efficiencyOnly, true) ==
      umbra::detail::ObjectClassAttributeDeclarationStatus::applied);

  auto registered = source.registerObjectInstance(
      L"exercise", owner.membership->id, *server);
  REQUIRE(registered.status ==
      umbra::detail::ObjectInstanceRegistrationStatus::applied);
  REQUIRE(registered.objectInstanceHandle != 0U);
  auto discoveries = source.planObjectInstanceDiscoveriesForInstance(
      L"exercise", registered.objectInstanceHandle);
  REQUIRE(discoveries.size() == 1U);
  REQUIRE(discoveries.front().receivingFederateId == requester.membership->id);
  REQUIRE(source.beginObjectInstanceDiscovery(
      L"exercise",
      requester.membership->id,
      registered.objectInstanceHandle)
      .has_value());
  REQUIRE(source.setObjectClassAttributePublication(
      L"exercise", requester.membership->id, *server, efficiencyOnly, true) ==
      umbra::detail::ObjectClassAttributeDeclarationStatus::applied);

  std::string const valueBytes{"\x7b\x7c", 2U};
  std::vector<std::pair<std::uint64_t, rti1516_2025::VariableLengthData>> values{{
      *efficiency,
      rti1516_2025::VariableLengthData(valueBytes.data(), valueBytes.size()),
  }};
  REQUIRE(source.recordSuccessfulUpdateAttributeValues(
      L"exercise",
      owner.membership->id,
      registered.objectInstanceHandle,
      *server,
      {"HLAreliable"},
      &values) == FederationRegistryStatus::applied);

  std::vector<unsigned char> const acquisitionTag{
      'w', 't', 'a', '-', 'd', 'e', 'l', 'i', 'v', 'e', 'r', 'e', 'd'};
  auto acquisition = source.planAttributeOwnershipAcquisitionIfAvailable(
      L"exercise",
      requester.membership->id,
      registered.objectInstanceHandle,
      efficiencyOnly,
      acquisitionTag);
  REQUIRE(acquisition.status ==
      umbra::detail::AttributeOwnershipAcquisitionIfAvailableStatus::applied);
  REQUIRE(acquisition.requestId != 0U);

  std::vector<unsigned char> const divestitureTag{
      'd', 'i', 'v', '-', 'w', 't', 'a', '-', 'd', 'e', 'l', 'i', 'v', 'e', 'r', 'e', 'd'};
  auto divestiture = source.planNegotiatedAttributeOwnershipDivestiture(
      L"exercise",
      owner.membership->id,
      registered.objectInstanceHandle,
      efficiencyOnly,
      divestitureTag);
  REQUIRE(divestiture.status ==
      umbra::detail::NegotiatedAttributeOwnershipDivestitureStatus::applied);
  REQUIRE(divestiture.workItems.size() == 1U);
  auto const& sourceConfirmation = divestiture.workItems.front();
  REQUIRE(sourceConfirmation.requestId == acquisition.requestId);
  REQUIRE(sourceConfirmation.candidateIsIfAvailable);
  auto deliveredBeforeSave = source.beginRequestDivestitureConfirmation(
      L"exercise",
      owner.membership->id,
      requester.membership->id,
      registered.objectInstanceHandle,
      sourceConfirmation.requestId,
      true,
      sourceConfirmation.attributeHandles);
  REQUIRE(deliveredBeforeSave.has_value());
  REQUIRE(deliveredBeforeSave->releasedAttributeHandles == efficiencyOnly);
  REQUIRE_FALSE(source.beginRequestDivestitureConfirmation(
      L"exercise",
      owner.membership->id,
      requester.membership->id,
      registered.objectInstanceHandle,
      sourceConfirmation.requestId,
      true,
      sourceConfirmation.attributeHandles));

  std::wstring const saveLabel = L"ownership-negotiated-if-available-delivered-process-restart";
  REQUIRE(source.requestFederationSave(
      L"exercise", owner.membership->id, saveLabel).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(source.federateSaveBegun(
      L"exercise", owner.membership->id).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(source.federateSaveBegun(
      L"exercise", requester.membership->id).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE_FALSE(source.federateSaveComplete(
      L"exercise", owner.membership->id).saveCompletedSuccessfully);
  REQUIRE(source.federateSaveComplete(
      L"exercise", requester.membership->id).saveCompletedSuccessfully);

  auto committed = store->load(L"exercise", saveLabel);
  REQUIRE(committed.has_value());
  auto image = umbra::detail::FederationStateImageCodec::decode(
      committed->stateImage);
  REQUIRE(image.objects.size() == 1U);
  auto const& savedObject = image.objects.front();
  REQUIRE(savedObject.attributeValuesPresent);
  REQUIRE(savedObject.attributeValues.size() == 1U);
  REQUIRE(savedObject.attributeValues.front().value == valueBytes);
  REQUIRE(savedObject.pendingAttributeOwnershipAcquisitionRequests.empty());
  REQUIRE(savedObject.pendingAttributeOwnershipAcquisitionIfAvailableRequests.size() == 1U);
  auto const& savedRequest =
      savedObject.pendingAttributeOwnershipAcquisitionIfAvailableRequests.front();
  REQUIRE(savedRequest.requestId == acquisition.requestId);
  REQUIRE(savedRequest.requestingFederateId == requester.membership->id);
  REQUIRE(savedRequest.desiredAttributeHandles ==
      std::vector<std::uint64_t>{*efficiency});
  REQUIRE(savedObject.pendingNegotiatedAttributeOwnershipDivestitures.size() == 1U);
  auto const& savedDivestiture =
      savedObject.pendingNegotiatedAttributeOwnershipDivestitures.front();
  REQUIRE(savedDivestiture.acquiringFederateIsIfAvailable);
  REQUIRE(savedDivestiture.confirmationQueued);
  REQUIRE(savedDivestiture.confirmationDelivered);

  EmbeddedFederationRegistry restarted({}, store);
  REQUIRE(restarted.create(L"exercise", composedRestaurantDefinition()).status ==
      FederationRegistryStatus::applied);
  auto restartedOwner = restarted.join(
      L"exercise", L"publisher", L"owner", noOpCallbackRoute());
  auto restartedRequester = restarted.join(
      L"exercise", L"subscriber", L"requester", noOpCallbackRoute());
  REQUIRE(restartedOwner.membership);
  REQUIRE(restartedRequester.membership);
  REQUIRE(restartedOwner.membership->id == owner.membership->id);
  REQUIRE(restartedRequester.membership->id == requester.membership->id);
  REQUIRE(restarted.requestFederationRestore(
      L"exercise", restartedRequester.membership->id, saveLabel).status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  auto firstRestore = restarted.federateRestoreComplete(
      L"exercise", restartedOwner.membership->id);
  REQUIRE(firstRestore.status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  REQUIRE(firstRestore.ownershipAcquisitionWorkItems.empty());
  auto restored = restarted.federateRestoreComplete(
      L"exercise", restartedRequester.membership->id);
  REQUIRE(restored.status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  REQUIRE(restored.ownershipAcquisitionWorkItems.empty());
  REQUIRE_FALSE(restarted.beginRequestDivestitureConfirmation(
      L"exercise",
      restartedOwner.membership->id,
      restartedRequester.membership->id,
      registered.objectInstanceHandle,
      acquisition.requestId,
      true,
      efficiencyOnly));

  auto restoredOwnerState = restarted.attributeOwnedByFederate(
      L"exercise",
      restartedOwner.membership->id,
      registered.objectInstanceHandle,
      *efficiency);
  REQUIRE(restoredOwnerState.status ==
      umbra::detail::AttributeOwnershipCheckStatus::applied);
  REQUIRE(restoredOwnerState.ownedByRequestingFederate);
  auto confirmed = restarted.planConfirmDivestiture(
      L"exercise",
      restartedOwner.membership->id,
      registered.objectInstanceHandle,
      efficiencyOnly,
      divestitureTag);
  REQUIRE(confirmed.status == umbra::detail::ConfirmDivestitureStatus::applied);
  REQUIRE(confirmed.notifications.size() == 1U);
  auto const& notification = confirmed.notifications.front();
  auto notificationDelivery = restarted.beginConfirmDivestitureNotification(
      L"exercise",
      restartedRequester.membership->id,
      registered.objectInstanceHandle,
      notification.notificationId,
      notification.attributeHandles);
  REQUIRE(notificationDelivery.has_value());
  REQUIRE(notificationDelivery->securedAttributeHandles == efficiencyOnly);
  auto requesterOwnership = restarted.attributeOwnedByFederate(
      L"exercise",
      restartedRequester.membership->id,
      registered.objectInstanceHandle,
      *efficiency);
  REQUIRE(requesterOwnership.status ==
      umbra::detail::AttributeOwnershipCheckStatus::applied);
  REQUIRE(requesterOwnership.ownedByRequestingFederate);

  std::filesystem::remove_all(directory, ignored);
}

TEST_CASE(
    "Federation restore rejects malformed mixed negotiated confirmation images",
    "[unit][kernel][federation-registry][save-restore][durable-save][filesystem][restore][failure]"
    "[process-restart][process-restart-malformed-mixed-confirmation][ownership-ledger-state]"
    "[attribute-ownership-acquisition][attribute-ownership-acquisition-if-available]"
    "[negotiated-attribute-ownership-divestiture]") {
  enum class Mutation {
    missingIfAvailableCandidate,
    mismatchedIfAvailableCandidate,
    staleConfirmationFlags,
  };
  class MutatingStore final : public umbra::detail::FederationSaveCommitStore {
   public:
    MutatingStore(
        umbra::detail::FederationSaveCommitDescriptor descriptor,
        Mutation mutation)
        : committed(std::move(descriptor)), mutation(mutation) {}

    void commit(umbra::detail::FederationSaveCommitDescriptor const&) override {}

    [[nodiscard]] std::optional<umbra::detail::FederationSaveCommitDescriptor> load(
        std::wstring const& federationName,
        std::wstring const& label) const override {
      if (committed.federationName != federationName || committed.label != label) {
        return std::nullopt;
      }
      auto result = committed;
      auto image = umbra::detail::FederationStateImageCodec::decode(
          result.stateImage);
      REQUIRE(image.objects.size() == 1U);
      auto& object = image.objects.front();
      REQUIRE(object.pendingAttributeOwnershipAcquisitionRequests.size() == 1U);
      REQUIRE(object.pendingAttributeOwnershipAcquisitionIfAvailableRequests.size() == 1U);
      REQUIRE(object.pendingNegotiatedAttributeOwnershipDivestitures.size() == 2U);
      switch (mutation) {
        case Mutation::missingIfAvailableCandidate:
          object.pendingAttributeOwnershipAcquisitionIfAvailableRequests.clear();
          REQUIRE(object.pendingOperationCount > 0U);
          --object.pendingOperationCount;
          break;
        case Mutation::mismatchedIfAvailableCandidate:
          for (auto& divestiture :
               object.pendingNegotiatedAttributeOwnershipDivestitures) {
            if (divestiture.acquiringFederateIsIfAvailable) {
              ++divestiture.acquisitionRequestId;
            }
          }
          break;
        case Mutation::staleConfirmationFlags:
          for (auto& divestiture :
               object.pendingNegotiatedAttributeOwnershipDivestitures) {
            if (!divestiture.acquiringFederateIsIfAvailable) {
              divestiture.confirmationQueued = false;
              divestiture.confirmationDelivered = true;
            }
          }
          break;
      }
      result.stateImage = umbra::detail::FederationStateImageCodec::encode(image);
      return result;
    }

   private:
    umbra::detail::FederationSaveCommitDescriptor committed;
    Mutation mutation;
  };

  auto sourceStore = std::make_shared<umbra::detail::MemoryFederationSaveCommitStore>();
  EmbeddedFederationRegistry source({}, sourceStore);
  REQUIRE(source.create(L"exercise", composedRestaurantDefinition()).status ==
      FederationRegistryStatus::applied);
  auto owner = source.join(
      L"exercise", L"publisher", L"owner", noOpCallbackRoute());
  auto requester = source.join(
      L"exercise", L"subscriber", L"requester", noOpCallbackRoute());
  REQUIRE(owner.membership);
  REQUIRE(requester.membership);

  auto const server = source.objectClassHandleFor(
      L"exercise", "HLAobjectRoot.Employee.Server");
  auto const efficiency = source.attributeHandleFor(
      L"exercise", "HLAobjectRoot.Employee.Server", "Efficiency");
  auto const cheerfulness = source.attributeHandleFor(
      L"exercise", "HLAobjectRoot.Employee.Server", "Cheerfulness");
  REQUIRE(server.has_value());
  REQUIRE(efficiency.has_value());
  REQUIRE(cheerfulness.has_value());
  std::set<std::uint64_t> const mixedAttributes{*efficiency, *cheerfulness};
  std::set<std::uint64_t> const efficiencyOnly{*efficiency};
  std::set<std::uint64_t> const cheerfulnessOnly{*cheerfulness};
  REQUIRE(source.setObjectClassAttributePublication(
      L"exercise", owner.membership->id, *server, mixedAttributes, true) ==
      umbra::detail::ObjectClassAttributeDeclarationStatus::applied);
  REQUIRE(source.setObjectClassAttributeSubscription(
      L"exercise", requester.membership->id, *server, mixedAttributes, true) ==
      umbra::detail::ObjectClassAttributeDeclarationStatus::applied);
  auto registered = source.registerObjectInstance(
      L"exercise", owner.membership->id, *server);
  REQUIRE(registered.status ==
      umbra::detail::ObjectInstanceRegistrationStatus::applied);
  auto discoveries = source.planObjectInstanceDiscoveriesForInstance(
      L"exercise", registered.objectInstanceHandle);
  REQUIRE(discoveries.size() == 1U);
  REQUIRE(source.beginObjectInstanceDiscovery(
      L"exercise", requester.membership->id,
      registered.objectInstanceHandle));
  REQUIRE(source.setObjectClassAttributePublication(
      L"exercise", requester.membership->id, *server, mixedAttributes, true) ==
      umbra::detail::ObjectClassAttributeDeclarationStatus::applied);

  std::string const efficiencyValue{"\x91\x92", 2U};
  std::string const cheerfulnessValue{"\x93\x94", 2U};
  std::vector<std::pair<std::uint64_t, rti1516_2025::VariableLengthData>> values{
      {*efficiency,
       rti1516_2025::VariableLengthData(efficiencyValue.data(), efficiencyValue.size())},
      {*cheerfulness,
       rti1516_2025::VariableLengthData(cheerfulnessValue.data(), cheerfulnessValue.size())},
  };
  REQUIRE(source.recordSuccessfulUpdateAttributeValues(
      L"exercise", owner.membership->id, registered.objectInstanceHandle,
      *server, {"HLAreliable"}, &values) == FederationRegistryStatus::applied);
  auto regular = source.planAttributeOwnershipAcquisition(
      L"exercise", requester.membership->id, registered.objectInstanceHandle,
      efficiencyOnly, {'r', 'e', 'g', '-', 'm', 'a', 'l', 'f', 'o', 'r', 'm'});
  REQUIRE(regular.status ==
      umbra::detail::AttributeOwnershipAcquisitionStatus::applied);
  REQUIRE(regular.workItems.size() == 1U);
  auto ifAvailable = source.planAttributeOwnershipAcquisitionIfAvailable(
      L"exercise", requester.membership->id, registered.objectInstanceHandle,
      cheerfulnessOnly, {'w', 't', 'a', '-', 'm', 'a', 'l', 'f', 'o', 'r', 'm'});
  REQUIRE(ifAvailable.status ==
      umbra::detail::AttributeOwnershipAcquisitionIfAvailableStatus::applied);
  REQUIRE(ifAvailable.requestId != 0U);
  auto divestiture = source.planNegotiatedAttributeOwnershipDivestiture(
      L"exercise", owner.membership->id, registered.objectInstanceHandle,
      mixedAttributes, {'d', 'i', 'v', '-', 'm', 'a', 'l', 'f', 'o', 'r', 'm'});
  REQUIRE(divestiture.status ==
      umbra::detail::NegotiatedAttributeOwnershipDivestitureStatus::applied);
  REQUIRE(divestiture.workItems.size() == 2U);

  std::wstring const saveLabel = L"ownership-negotiated-malformed-process-restart";
  REQUIRE(source.requestFederationSave(
      L"exercise", owner.membership->id, saveLabel).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(source.federateSaveBegun(
      L"exercise", owner.membership->id).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(source.federateSaveBegun(
      L"exercise", requester.membership->id).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE_FALSE(source.federateSaveComplete(
      L"exercise", owner.membership->id).saveCompletedSuccessfully);
  REQUIRE(source.federateSaveComplete(
      L"exercise", requester.membership->id).saveCompletedSuccessfully);
  auto commits = sourceStore->snapshotCommits();
  REQUIRE(commits.size() == 1U);
  auto const baseCommit = commits.front();

  for (auto const mutation : {
           Mutation::missingIfAvailableCandidate,
           Mutation::mismatchedIfAvailableCandidate,
           Mutation::staleConfirmationFlags}) {
    auto store = std::make_shared<MutatingStore>(baseCommit, mutation);
    EmbeddedFederationRegistry restarted({}, store);
    REQUIRE(restarted.create(L"exercise", composedRestaurantDefinition()).status ==
        FederationRegistryStatus::applied);
    auto restartedOwner = restarted.join(
        L"exercise", L"publisher", L"owner", noOpCallbackRoute());
    auto restartedRequester = restarted.join(
        L"exercise", L"subscriber", L"requester", noOpCallbackRoute());
    REQUIRE(restartedOwner.membership);
    REQUIRE(restartedRequester.membership);
    auto restore = restarted.requestFederationRestore(
        L"exercise", restartedRequester.membership->id, saveLabel);
    REQUIRE(restore.status ==
        umbra::detail::FederationRestoreControlStatus::snapshot_not_found);
    REQUIRE(restore.notifications.size() == 1U);
    REQUIRE(restore.notifications.front().kind ==
        umbra::detail::FederationRestoreNotificationKind::request_failed);
    REQUIRE(restore.notifications.front().receivingFederateId ==
        restartedRequester.membership->id);
  }
}

TEST_CASE(
    "Filesystem state image restores a pending Divestiture If Wanted notification in a fresh registry",
    "[unit][kernel][federation-registry][save-restore][durable-save][filesystem][restore]"
    "[process-restart][process-restart-divestiture-if-wanted][ownership-ledger-state]"
    "[attribute-ownership-divestiture-if-wanted][attribute-ownership-acquisition]"
    "[attribute-ownership-acquisition-if-available]") {
  auto const directory = temporarySaveCommitDirectory();
  std::error_code ignored;
  std::filesystem::remove_all(directory, ignored);
  auto store = std::make_shared<umbra::detail::FilesystemFederationSaveCommitStore>(
      directory);

  EmbeddedFederationRegistry source({}, store);
  REQUIRE(source.create(L"exercise", composedRestaurantDefinition()).status ==
      FederationRegistryStatus::applied);
  auto owner = source.join(
      L"exercise", L"publisher", L"owner", noOpCallbackRoute());
  auto requester = source.join(
      L"exercise", L"subscriber", L"requester", noOpCallbackRoute());
  REQUIRE(owner.status == FederationRegistryStatus::applied);
  REQUIRE(requester.status == FederationRegistryStatus::applied);
  REQUIRE(owner.membership);
  REQUIRE(requester.membership);

  auto const server = source.objectClassHandleFor(
      L"exercise", "HLAobjectRoot.Employee.Server");
  auto const efficiency = source.attributeHandleFor(
      L"exercise", "HLAobjectRoot.Employee.Server", "Efficiency");
  REQUIRE(server.has_value());
  REQUIRE(efficiency.has_value());
  std::set<std::uint64_t> const efficiencyOnly{*efficiency};
  REQUIRE(source.setObjectClassAttributePublication(
      L"exercise", owner.membership->id, *server, efficiencyOnly, true) ==
      umbra::detail::ObjectClassAttributeDeclarationStatus::applied);
  REQUIRE(source.setObjectClassAttributeSubscription(
      L"exercise", requester.membership->id, *server, efficiencyOnly, true) ==
      umbra::detail::ObjectClassAttributeDeclarationStatus::applied);

  auto registered = source.registerObjectInstance(
      L"exercise", owner.membership->id, *server);
  REQUIRE(registered.status ==
      umbra::detail::ObjectInstanceRegistrationStatus::applied);
  REQUIRE(registered.objectInstanceHandle != 0U);
  auto discoveries = source.planObjectInstanceDiscoveriesForInstance(
      L"exercise", registered.objectInstanceHandle);
  REQUIRE(discoveries.size() == 1U);
  REQUIRE(discoveries.front().receivingFederateId == requester.membership->id);
  REQUIRE(source.beginObjectInstanceDiscovery(
      L"exercise", requester.membership->id,
      registered.objectInstanceHandle)
      .has_value());
  // The requester must publish after discovery to remain an eligible acquirer
  // when the owner evaluates Divestiture If Wanted.
  REQUIRE(source.setObjectClassAttributePublication(
      L"exercise", requester.membership->id, *server, efficiencyOnly, true) ==
      umbra::detail::ObjectClassAttributeDeclarationStatus::applied);

  std::string const valueBytes{"\x7d\x7e", 2U};
  std::vector<std::pair<std::uint64_t, rti1516_2025::VariableLengthData>> values{{
      *efficiency,
      rti1516_2025::VariableLengthData(valueBytes.data(), valueBytes.size()),
  }};
  REQUIRE(source.recordSuccessfulUpdateAttributeValues(
      L"exercise", owner.membership->id, registered.objectInstanceHandle,
      *server, {"HLAreliable"}, &values) == FederationRegistryStatus::applied);

  std::vector<unsigned char> const acquisitionTag{
      'r', 'e', 'g', '-', 'd', 'i', 'v', '-', 'w', 'a', 'n', 't'};
  auto acquisition = source.planAttributeOwnershipAcquisition(
      L"exercise", requester.membership->id,
      registered.objectInstanceHandle, efficiencyOnly, acquisitionTag);
  REQUIRE(acquisition.status ==
      umbra::detail::AttributeOwnershipAcquisitionStatus::applied);
  REQUIRE(acquisition.workItems.size() == 1U);
  REQUIRE(acquisition.workItems.front().kind ==
      umbra::detail::AttributeOwnershipAcquisitionWorkKind::request_release);

  std::vector<unsigned char> const divestitureTag{
      'd', 'i', 'v', '-', 'i', 'f', '-', 'w', 'a', 'n', 't'};
  auto divestiture = source.planAttributeOwnershipDivestitureIfWanted(
      L"exercise", owner.membership->id, registered.objectInstanceHandle,
      efficiencyOnly, divestitureTag);
  REQUIRE(divestiture.status ==
      umbra::detail::AttributeOwnershipDivestitureIfWantedStatus::applied);
  REQUIRE(divestiture.divestedAttributeHandles == efficiencyOnly);
  REQUIRE(divestiture.notifications.size() == 1U);
  auto const& sourceNotification = divestiture.notifications.front();
  REQUIRE(sourceNotification.notificationId != 0U);
  REQUIRE(sourceNotification.receivingFederateId == requester.membership->id);
  REQUIRE(sourceNotification.objectInstanceHandle ==
      registered.objectInstanceHandle);
  REQUIRE(sourceNotification.attributeHandles == efficiencyOnly);
  REQUIRE(sourceNotification.userSuppliedTag == divestitureTag);
  // The notification is intentionally left pending for the save/restart
  // boundary; its callback begins only after restore.
  auto sourceOwnership = source.attributeOwnedByFederate(
      L"exercise", requester.membership->id,
      registered.objectInstanceHandle, *efficiency);
  REQUIRE(sourceOwnership.status ==
      umbra::detail::AttributeOwnershipCheckStatus::applied);
  REQUIRE(sourceOwnership.ownedByRequestingFederate);
  auto sourceOwnerState = source.attributeOwnedByFederate(
      L"exercise", owner.membership->id,
      registered.objectInstanceHandle, *efficiency);
  REQUIRE(sourceOwnerState.status ==
      umbra::detail::AttributeOwnershipCheckStatus::applied);
  REQUIRE_FALSE(sourceOwnerState.ownedByRequestingFederate);

  std::wstring const saveLabel = L"ownership-divestiture-if-wanted-process-restart";
  REQUIRE(source.requestFederationSave(
      L"exercise", owner.membership->id, saveLabel).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(source.federateSaveBegun(
      L"exercise", owner.membership->id).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(source.federateSaveBegun(
      L"exercise", requester.membership->id).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE_FALSE(source.federateSaveComplete(
      L"exercise", owner.membership->id).saveCompletedSuccessfully);
  REQUIRE(source.federateSaveComplete(
      L"exercise", requester.membership->id).saveCompletedSuccessfully);

  auto committed = store->load(L"exercise", saveLabel);
  REQUIRE(committed.has_value());
  auto image = umbra::detail::FederationStateImageCodec::decode(
      committed->stateImage);
  REQUIRE(image.objects.size() == 1U);
  auto const& savedObject = image.objects.front();
  REQUIRE(savedObject.handle == registered.objectInstanceHandle);
  REQUIRE(savedObject.attributeValuesPresent);
  REQUIRE(savedObject.attributeValues.size() == 1U);
  REQUIRE(savedObject.attributeValues.front().attributeHandle == *efficiency);
  REQUIRE(savedObject.attributeValues.front().value == valueBytes);
  REQUIRE(savedObject.pendingAttributeOwnershipAcquisitionRequests.empty());
  REQUIRE(savedObject.pendingAttributeOwnershipAcquisitionIfAvailableRequests.empty());
  REQUIRE(savedObject.pendingAttributeOwnershipAcquisitionCancellations.empty());
  REQUIRE(savedObject.pendingAttributeOwnershipDivestitureIfWantedNotifications.size() == 1U);
  auto const& savedNotification =
      savedObject.pendingAttributeOwnershipDivestitureIfWantedNotifications.front();
  REQUIRE(savedNotification.notificationId == sourceNotification.notificationId);
  REQUIRE(savedNotification.receivingFederateId == requester.membership->id);
  REQUIRE(savedNotification.attributeHandles ==
      std::vector<std::uint64_t>{*efficiency});
  REQUIRE(savedObject.pendingOperationCount == 1U);

  EmbeddedFederationRegistry restarted({}, store);
  REQUIRE(restarted.create(L"exercise", composedRestaurantDefinition()).status ==
      FederationRegistryStatus::applied);
  auto restartedOwner = restarted.join(
      L"exercise", L"publisher", L"owner", noOpCallbackRoute());
  auto restartedRequester = restarted.join(
      L"exercise", L"subscriber", L"requester", noOpCallbackRoute());
  REQUIRE(restartedOwner.membership);
  REQUIRE(restartedRequester.membership);
  REQUIRE(restartedOwner.membership->id == owner.membership->id);
  REQUIRE(restartedRequester.membership->id == requester.membership->id);
  REQUIRE(restarted.requestFederationRestore(
      L"exercise", restartedRequester.membership->id, saveLabel).status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  auto firstRestore = restarted.federateRestoreComplete(
      L"exercise", restartedOwner.membership->id);
  REQUIRE(firstRestore.status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  REQUIRE(firstRestore.ownershipAcquisitionWorkItems.empty());
  auto restored = restarted.federateRestoreComplete(
      L"exercise", restartedRequester.membership->id);
  REQUIRE(restored.status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  REQUIRE(restored.ownershipAcquisitionWorkItems.empty());

  auto restoredOwnerState = restarted.attributeOwnedByFederate(
      L"exercise", restartedOwner.membership->id,
      registered.objectInstanceHandle, *efficiency);
  REQUIRE(restoredOwnerState.status ==
      umbra::detail::AttributeOwnershipCheckStatus::applied);
  REQUIRE_FALSE(restoredOwnerState.ownedByRequestingFederate);
  auto restoredRequesterState = restarted.attributeOwnedByFederate(
      L"exercise", restartedRequester.membership->id,
      registered.objectInstanceHandle, *efficiency);
  REQUIRE(restoredRequesterState.status ==
      umbra::detail::AttributeOwnershipCheckStatus::applied);
  REQUIRE(restoredRequesterState.ownedByRequestingFederate);

  auto notificationDelivery =
      restarted.beginAttributeOwnershipDivestitureIfWantedNotification(
          L"exercise", restartedRequester.membership->id,
          registered.objectInstanceHandle, savedNotification.notificationId,
          efficiencyOnly);
  REQUIRE(notificationDelivery.has_value());
  REQUIRE(notificationDelivery->objectInstanceHandle ==
      registered.objectInstanceHandle);
  REQUIRE(notificationDelivery->securedAttributeHandles == efficiencyOnly);
  REQUIRE(notificationDelivery->followupWorkItems.empty());
  REQUIRE_FALSE(restarted.beginAttributeOwnershipDivestitureIfWantedNotification(
      L"exercise", restartedRequester.membership->id,
      registered.objectInstanceHandle, savedNotification.notificationId,
      efficiencyOnly));

  std::filesystem::remove_all(directory, ignored);
}

TEST_CASE(
    "Filesystem state image restores a pending Confirm Divestiture notification in a fresh registry",
    "[unit][kernel][federation-registry][save-restore][durable-save][filesystem][restore]"
    "[process-restart][process-restart-confirm-divestiture][ownership-ledger-state]"
    "[confirm-divestiture][negotiated-attribute-ownership-divestiture]"
    "[attribute-ownership-acquisition]") {
  auto const directory = temporarySaveCommitDirectory();
  std::error_code ignored;
  std::filesystem::remove_all(directory, ignored);
  auto store = std::make_shared<umbra::detail::FilesystemFederationSaveCommitStore>(
      directory);

  EmbeddedFederationRegistry source({}, store);
  REQUIRE(source.create(L"exercise", composedRestaurantDefinition()).status ==
      FederationRegistryStatus::applied);
  auto owner = source.join(
      L"exercise", L"publisher", L"owner", noOpCallbackRoute());
  auto requester = source.join(
      L"exercise", L"subscriber", L"requester", noOpCallbackRoute());
  REQUIRE(owner.status == FederationRegistryStatus::applied);
  REQUIRE(requester.status == FederationRegistryStatus::applied);
  REQUIRE(owner.membership);
  REQUIRE(requester.membership);

  auto const server = source.objectClassHandleFor(
      L"exercise", "HLAobjectRoot.Employee.Server");
  auto const efficiency = source.attributeHandleFor(
      L"exercise", "HLAobjectRoot.Employee.Server", "Efficiency");
  REQUIRE(server.has_value());
  REQUIRE(efficiency.has_value());
  std::set<std::uint64_t> const efficiencyOnly{*efficiency};
  REQUIRE(source.setObjectClassAttributePublication(
      L"exercise", owner.membership->id, *server, efficiencyOnly, true) ==
      umbra::detail::ObjectClassAttributeDeclarationStatus::applied);
  REQUIRE(source.setObjectClassAttributeSubscription(
      L"exercise", requester.membership->id, *server, efficiencyOnly, true) ==
      umbra::detail::ObjectClassAttributeDeclarationStatus::applied);

  auto registered = source.registerObjectInstance(
      L"exercise", owner.membership->id, *server);
  REQUIRE(registered.status ==
      umbra::detail::ObjectInstanceRegistrationStatus::applied);
  REQUIRE(registered.objectInstanceHandle != 0U);
  auto discoveries = source.planObjectInstanceDiscoveriesForInstance(
      L"exercise", registered.objectInstanceHandle);
  REQUIRE(discoveries.size() == 1U);
  REQUIRE(discoveries.front().receivingFederateId == requester.membership->id);
  REQUIRE(source.beginObjectInstanceDiscovery(
      L"exercise", requester.membership->id,
      registered.objectInstanceHandle)
      .has_value());
  // The requester must publish after discovery so it remains an eligible
  // acquisition target through the negotiated confirmation boundary.
  REQUIRE(source.setObjectClassAttributePublication(
      L"exercise", requester.membership->id, *server, efficiencyOnly, true) ==
      umbra::detail::ObjectClassAttributeDeclarationStatus::applied);

  std::string const valueBytes{"\x7f\x01", 2U};
  std::vector<std::pair<std::uint64_t, rti1516_2025::VariableLengthData>> values{{
      *efficiency,
      rti1516_2025::VariableLengthData(valueBytes.data(), valueBytes.size()),
  }};
  REQUIRE(source.recordSuccessfulUpdateAttributeValues(
      L"exercise", owner.membership->id, registered.objectInstanceHandle,
      *server, {"HLAreliable"}, &values) == FederationRegistryStatus::applied);

  std::vector<unsigned char> const acquisitionTag{
      'r', 'e', 'g', '-', 'c', 'o', 'n', 'f', 'i', 'r', 'm'};
  auto acquisition = source.planAttributeOwnershipAcquisition(
      L"exercise", requester.membership->id,
      registered.objectInstanceHandle, efficiencyOnly, acquisitionTag);
  REQUIRE(acquisition.status ==
      umbra::detail::AttributeOwnershipAcquisitionStatus::applied);
  REQUIRE(acquisition.workItems.size() == 1U);
  REQUIRE(acquisition.workItems.front().kind ==
      umbra::detail::AttributeOwnershipAcquisitionWorkKind::request_release);
  auto const acquisitionRequestId = acquisition.workItems.front().requestId;

  std::vector<unsigned char> const divestitureTag{
      'd', 'i', 'v', '-', 'c', 'o', 'n', 'f', 'i', 'r', 'm'};
  auto divestiture = source.planNegotiatedAttributeOwnershipDivestiture(
      L"exercise", owner.membership->id, registered.objectInstanceHandle,
      efficiencyOnly, divestitureTag);
  REQUIRE(divestiture.status ==
      umbra::detail::NegotiatedAttributeOwnershipDivestitureStatus::applied);
  REQUIRE(divestiture.workItems.size() == 1U);
  auto const& sourceConfirmation = divestiture.workItems.front();
  REQUIRE(sourceConfirmation.requestId == acquisitionRequestId);
  REQUIRE_FALSE(sourceConfirmation.candidateIsIfAvailable);
  auto released = source.beginRequestDivestitureConfirmation(
      L"exercise", owner.membership->id, requester.membership->id,
      registered.objectInstanceHandle, sourceConfirmation.requestId, false,
      efficiencyOnly);
  REQUIRE(released.has_value());
  REQUIRE(released->releasedAttributeHandles == efficiencyOnly);

  std::vector<unsigned char> const confirmTag{
      'c', 'o', 'n', 'f', '-', 'd', 'i', 'v'};
  auto confirmed = source.planConfirmDivestiture(
      L"exercise", owner.membership->id, registered.objectInstanceHandle,
      efficiencyOnly, confirmTag);
  REQUIRE(confirmed.status ==
      umbra::detail::ConfirmDivestitureStatus::applied);
  REQUIRE(confirmed.notifications.size() == 1U);
  auto const& sourceNotification = confirmed.notifications.front();
  REQUIRE(sourceNotification.notificationId != 0U);
  REQUIRE(sourceNotification.receivingFederateId == requester.membership->id);
  REQUIRE(sourceNotification.objectInstanceHandle ==
      registered.objectInstanceHandle);
  REQUIRE(sourceNotification.attributeHandles == efficiencyOnly);
  REQUIRE(sourceNotification.userSuppliedTag == confirmTag);

  std::wstring const saveLabel = L"ownership-confirm-divestiture-process-restart";
  REQUIRE(source.requestFederationSave(
      L"exercise", owner.membership->id, saveLabel).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(source.federateSaveBegun(
      L"exercise", owner.membership->id).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(source.federateSaveBegun(
      L"exercise", requester.membership->id).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE_FALSE(source.federateSaveComplete(
      L"exercise", owner.membership->id).saveCompletedSuccessfully);
  REQUIRE(source.federateSaveComplete(
      L"exercise", requester.membership->id).saveCompletedSuccessfully);

  auto committed = store->load(L"exercise", saveLabel);
  REQUIRE(committed.has_value());
  auto image = umbra::detail::FederationStateImageCodec::decode(
      committed->stateImage);
  REQUIRE(image.objects.size() == 1U);
  auto const& savedObject = image.objects.front();
  REQUIRE(savedObject.handle == registered.objectInstanceHandle);
  REQUIRE(savedObject.attributeValuesPresent);
  REQUIRE(savedObject.attributeValues.size() == 1U);
  REQUIRE(savedObject.attributeValues.front().attributeHandle == *efficiency);
  REQUIRE(savedObject.attributeValues.front().value == valueBytes);
  REQUIRE(savedObject.pendingAttributeOwnershipAcquisitionRequests.empty());
  REQUIRE(savedObject.pendingAttributeOwnershipAcquisitionIfAvailableRequests.empty());
  REQUIRE(savedObject.pendingAttributeOwnershipAcquisitionCancellations.empty());
  REQUIRE(savedObject.pendingAttributeOwnershipDivestitureIfWantedNotifications.empty());
  REQUIRE(savedObject.pendingNegotiatedAttributeOwnershipDivestitures.empty());
  REQUIRE(savedObject.pendingConfirmDivestitureNotifications.size() == 1U);
  auto const& savedNotification =
      savedObject.pendingConfirmDivestitureNotifications.front();
  REQUIRE(savedNotification.notificationId == sourceNotification.notificationId);
  REQUIRE(savedNotification.receivingFederateId == requester.membership->id);
  REQUIRE(savedNotification.attributeHandles ==
      std::vector<std::uint64_t>{*efficiency});
  REQUIRE(savedNotification.userSuppliedTag ==
      std::string(reinterpret_cast<char const*>(confirmTag.data()),
                  confirmTag.size()));
  REQUIRE(savedObject.pendingOperationCount == 1U);

  EmbeddedFederationRegistry restarted({}, store);
  REQUIRE(restarted.create(L"exercise", composedRestaurantDefinition()).status ==
      FederationRegistryStatus::applied);
  auto restartedOwner = restarted.join(
      L"exercise", L"publisher", L"owner", noOpCallbackRoute());
  auto restartedRequester = restarted.join(
      L"exercise", L"subscriber", L"requester", noOpCallbackRoute());
  REQUIRE(restartedOwner.membership);
  REQUIRE(restartedRequester.membership);
  REQUIRE(restartedOwner.membership->id == owner.membership->id);
  REQUIRE(restartedRequester.membership->id == requester.membership->id);
  REQUIRE(restarted.requestFederationRestore(
      L"exercise", restartedRequester.membership->id, saveLabel).status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  auto firstRestore = restarted.federateRestoreComplete(
      L"exercise", restartedOwner.membership->id);
  REQUIRE(firstRestore.status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  REQUIRE(firstRestore.ownershipAcquisitionWorkItems.empty());
  auto restored = restarted.federateRestoreComplete(
      L"exercise", restartedRequester.membership->id);
  REQUIRE(restored.status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  REQUIRE(restored.ownershipAcquisitionWorkItems.empty());

  auto restoredOwnerState = restarted.attributeOwnedByFederate(
      L"exercise", restartedOwner.membership->id,
      registered.objectInstanceHandle, *efficiency);
  REQUIRE(restoredOwnerState.status ==
      umbra::detail::AttributeOwnershipCheckStatus::applied);
  REQUIRE_FALSE(restoredOwnerState.ownedByRequestingFederate);
  auto restoredRequesterState = restarted.attributeOwnedByFederate(
      L"exercise", restartedRequester.membership->id,
      registered.objectInstanceHandle, *efficiency);
  REQUIRE(restoredRequesterState.status ==
      umbra::detail::AttributeOwnershipCheckStatus::applied);
  REQUIRE(restoredRequesterState.ownedByRequestingFederate);

  auto notificationDelivery = restarted.beginConfirmDivestitureNotification(
      L"exercise", restartedRequester.membership->id,
      registered.objectInstanceHandle, savedNotification.notificationId,
      efficiencyOnly);
  REQUIRE(notificationDelivery.has_value());
  REQUIRE(notificationDelivery->objectInstanceHandle ==
      registered.objectInstanceHandle);
  REQUIRE(notificationDelivery->securedAttributeHandles == efficiencyOnly);
  REQUIRE(notificationDelivery->followupWorkItems.empty());
  REQUIRE_FALSE(restarted.beginConfirmDivestitureNotification(
      L"exercise", restartedRequester.membership->id,
      registered.objectInstanceHandle, savedNotification.notificationId,
      efficiencyOnly));

  std::filesystem::remove_all(directory, ignored);
}

TEST_CASE(
    "Filesystem state image restores a pending ownership-acquisition cancellation in a fresh registry",
    "[unit][kernel][federation-registry][save-restore][durable-save][filesystem][restore]"
    "[process-restart][process-restart-ownership-acquisition-cancellation][ownership-ledger-state]"
    "[attribute-ownership-acquisition][attribute-ownership-acquisition-cancellation]") {
  auto const directory = temporarySaveCommitDirectory();
  std::error_code ignored;
  std::filesystem::remove_all(directory, ignored);
  auto store = std::make_shared<umbra::detail::FilesystemFederationSaveCommitStore>(
      directory);

  EmbeddedFederationRegistry source({}, store);
  REQUIRE(source.create(L"exercise", composedRestaurantDefinition()).status ==
      FederationRegistryStatus::applied);
  auto owner = source.join(
      L"exercise", L"publisher", L"owner", noOpCallbackRoute());
  auto requester = source.join(
      L"exercise", L"subscriber", L"requester", noOpCallbackRoute());
  REQUIRE(owner.status == FederationRegistryStatus::applied);
  REQUIRE(requester.status == FederationRegistryStatus::applied);
  REQUIRE(owner.membership);
  REQUIRE(requester.membership);

  auto const server = source.objectClassHandleFor(
      L"exercise", "HLAobjectRoot.Employee.Server");
  auto const efficiency = source.attributeHandleFor(
      L"exercise", "HLAobjectRoot.Employee.Server", "Efficiency");
  REQUIRE(server.has_value());
  REQUIRE(efficiency.has_value());
  std::set<std::uint64_t> const efficiencyOnly{*efficiency};
  REQUIRE(source.setObjectClassAttributePublication(
      L"exercise", owner.membership->id, *server, efficiencyOnly, true) ==
      umbra::detail::ObjectClassAttributeDeclarationStatus::applied);
  REQUIRE(source.setObjectClassAttributeSubscription(
      L"exercise", requester.membership->id, *server, efficiencyOnly, true) ==
      umbra::detail::ObjectClassAttributeDeclarationStatus::applied);

  auto registered = source.registerObjectInstance(
      L"exercise", owner.membership->id, *server);
  REQUIRE(registered.status ==
      umbra::detail::ObjectInstanceRegistrationStatus::applied);
  REQUIRE(registered.objectInstanceHandle != 0U);
  auto discoveries = source.planObjectInstanceDiscoveriesForInstance(
      L"exercise", registered.objectInstanceHandle);
  REQUIRE(discoveries.size() == 1U);
  REQUIRE(discoveries.front().receivingFederateId == requester.membership->id);
  REQUIRE(source.beginObjectInstanceDiscovery(
      L"exercise", requester.membership->id,
      registered.objectInstanceHandle)
      .has_value());
  REQUIRE(source.setObjectClassAttributePublication(
      L"exercise", requester.membership->id, *server, efficiencyOnly, true) ==
      umbra::detail::ObjectClassAttributeDeclarationStatus::applied);

  std::string const valueBytes{"\x7b\x02", 2U};
  std::vector<std::pair<std::uint64_t, rti1516_2025::VariableLengthData>> values{{
      *efficiency,
      rti1516_2025::VariableLengthData(valueBytes.data(), valueBytes.size()),
  }};
  REQUIRE(source.recordSuccessfulUpdateAttributeValues(
      L"exercise", owner.membership->id, registered.objectInstanceHandle,
      *server, {"HLAreliable"}, &values) == FederationRegistryStatus::applied);

  std::vector<unsigned char> const acquisitionTag{
      'r', 'e', 'g', '-', 'c', 'a', 'n', 'c', 'e', 'l'};
  auto acquisition = source.planAttributeOwnershipAcquisition(
      L"exercise", requester.membership->id,
      registered.objectInstanceHandle, efficiencyOnly, acquisitionTag);
  REQUIRE(acquisition.status ==
      umbra::detail::AttributeOwnershipAcquisitionStatus::applied);
  REQUIRE(acquisition.workItems.size() == 1U);
  REQUIRE(acquisition.workItems.front().kind ==
      umbra::detail::AttributeOwnershipAcquisitionWorkKind::request_release);

  auto cancellation = source.planAttributeOwnershipAcquisitionCancellation(
      L"exercise", requester.membership->id,
      registered.objectInstanceHandle, efficiencyOnly);
  REQUIRE(cancellation.status ==
      umbra::detail::AttributeOwnershipAcquisitionCancellationStatus::applied);
  REQUIRE(cancellation.cancellationId != 0U);
  REQUIRE(cancellation.attributeHandles == efficiencyOnly);
  REQUIRE(cancellation.callbackRoute);

  std::wstring const saveLabel = L"ownership-acquisition-cancellation-process-restart";
  REQUIRE(source.requestFederationSave(
      L"exercise", owner.membership->id, saveLabel).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(source.federateSaveBegun(
      L"exercise", owner.membership->id).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(source.federateSaveBegun(
      L"exercise", requester.membership->id).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE_FALSE(source.federateSaveComplete(
      L"exercise", owner.membership->id).saveCompletedSuccessfully);
  REQUIRE(source.federateSaveComplete(
      L"exercise", requester.membership->id).saveCompletedSuccessfully);

  auto committed = store->load(L"exercise", saveLabel);
  REQUIRE(committed.has_value());
  auto image = umbra::detail::FederationStateImageCodec::decode(
      committed->stateImage);
  REQUIRE(image.objects.size() == 1U);
  auto const& savedObject = image.objects.front();
  REQUIRE(savedObject.handle == registered.objectInstanceHandle);
  REQUIRE(savedObject.attributeValuesPresent);
  REQUIRE(savedObject.attributeValues.size() == 1U);
  REQUIRE(savedObject.attributeValues.front().attributeHandle == *efficiency);
  REQUIRE(savedObject.attributeValues.front().value == valueBytes);
  REQUIRE(savedObject.pendingAttributeOwnershipAcquisitionRequests.size() == 1U);
  auto const& savedRequest = savedObject.pendingAttributeOwnershipAcquisitionRequests.front();
  REQUIRE(savedRequest.requestingFederateId == requester.membership->id);
  REQUIRE(savedRequest.desiredAttributeHandles ==
      std::vector<std::uint64_t>{*efficiency});
  REQUIRE(savedRequest.userSuppliedTag ==
      std::string(acquisitionTag.begin(), acquisitionTag.end()));
  REQUIRE(savedObject.pendingAttributeOwnershipAcquisitionCancellations.size() == 1U);
  auto const& savedCancellation =
      savedObject.pendingAttributeOwnershipAcquisitionCancellations.front();
  REQUIRE(savedCancellation.cancellationId == cancellation.cancellationId);
  REQUIRE(savedCancellation.requestingFederateId == requester.membership->id);
  REQUIRE(savedCancellation.attributeHandles ==
      std::vector<std::uint64_t>{*efficiency});
  REQUIRE(savedObject.pendingOperationCount == 2U);

  EmbeddedFederationRegistry restarted({}, store);
  REQUIRE(restarted.create(L"exercise", composedRestaurantDefinition()).status ==
      FederationRegistryStatus::applied);
  auto restartedOwner = restarted.join(
      L"exercise", L"publisher", L"owner", noOpCallbackRoute());
  auto restartedRequester = restarted.join(
      L"exercise", L"subscriber", L"requester", noOpCallbackRoute());
  REQUIRE(restartedOwner.membership);
  REQUIRE(restartedRequester.membership);
  REQUIRE(restartedOwner.membership->id == owner.membership->id);
  REQUIRE(restartedRequester.membership->id == requester.membership->id);
  REQUIRE(restarted.requestFederationRestore(
      L"exercise", restartedRequester.membership->id, saveLabel).status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  auto firstRestore = restarted.federateRestoreComplete(
      L"exercise", restartedOwner.membership->id);
  REQUIRE(firstRestore.status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  REQUIRE(firstRestore.ownershipAcquisitionWorkItems.empty());
  REQUIRE(firstRestore.ownershipAcquisitionCancellationWorkItems.empty());
  auto restored = restarted.federateRestoreComplete(
      L"exercise", restartedRequester.membership->id);
  REQUIRE(restored.status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  REQUIRE(restored.ownershipAcquisitionWorkItems.empty());
  REQUIRE(restored.ownershipAcquisitionCancellationWorkItems.size() == 1U);
  auto const& restoredCancellation =
      restored.ownershipAcquisitionCancellationWorkItems.front();
  REQUIRE(restoredCancellation.requestingFederateId ==
      restartedRequester.membership->id);
  REQUIRE(restoredCancellation.objectInstanceHandle ==
      registered.objectInstanceHandle);
  REQUIRE(restoredCancellation.cancellationId == cancellation.cancellationId);
  REQUIRE(restoredCancellation.attributeHandles == efficiencyOnly);
  REQUIRE(restoredCancellation.callbackRoute);

  auto delivered = restarted.beginAttributeOwnershipAcquisitionCancellation(
      L"exercise", restartedRequester.membership->id,
      registered.objectInstanceHandle, restoredCancellation.cancellationId,
      efficiencyOnly);
  REQUIRE(delivered.has_value());
  REQUIRE(delivered->objectInstanceHandle == registered.objectInstanceHandle);
  REQUIRE(delivered->confirmedAttributeHandles == efficiencyOnly);
  REQUIRE(delivered->followupWorkItems.empty());
  REQUIRE_FALSE(restarted.beginAttributeOwnershipAcquisitionCancellation(
      L"exercise", restartedRequester.membership->id,
      registered.objectInstanceHandle, restoredCancellation.cancellationId,
      efficiencyOnly));

  auto restoredOwnerState = restarted.attributeOwnedByFederate(
      L"exercise", restartedOwner.membership->id,
      registered.objectInstanceHandle, *efficiency);
  REQUIRE(restoredOwnerState.status ==
      umbra::detail::AttributeOwnershipCheckStatus::applied);
  REQUIRE(restoredOwnerState.ownedByRequestingFederate);
  auto restoredRequesterState = restarted.attributeOwnedByFederate(
      L"exercise", restartedRequester.membership->id,
      registered.objectInstanceHandle, *efficiency);
  REQUIRE(restoredRequesterState.status ==
      umbra::detail::AttributeOwnershipCheckStatus::applied);
  REQUIRE_FALSE(restoredRequesterState.ownedByRequestingFederate);

  std::filesystem::remove_all(directory, ignored);
}

TEST_CASE(
    "Filesystem state image restores a pending attribute transportation-type change in a fresh registry",
    "[unit][kernel][federation-registry][save-restore][durable-save][filesystem][restore]"
    "[process-restart][process-restart-attribute-transportation-type-change][ownership-ledger-state]"
    "[attribute-transportation-type-change][transportation-management]") {
  auto const directory = temporarySaveCommitDirectory();
  std::error_code ignored;
  std::filesystem::remove_all(directory, ignored);
  auto store = std::make_shared<umbra::detail::FilesystemFederationSaveCommitStore>(
      directory);

  EmbeddedFederationRegistry source({}, store);
  REQUIRE(source.create(L"exercise", composedRestaurantDefinition()).status ==
      FederationRegistryStatus::applied);
  auto sourceJoined = source.join(
      L"exercise", L"publisher", L"owner", noOpCallbackRoute());
  REQUIRE(sourceJoined.status == FederationRegistryStatus::applied);
  REQUIRE(sourceJoined.membership);
  auto const ownerId = sourceJoined.membership->id;

  auto const server = source.objectClassHandleFor(
      L"exercise", "HLAobjectRoot.Employee.Server");
  auto const efficiency = source.attributeHandleFor(
      L"exercise", "HLAobjectRoot.Employee.Server", "Efficiency");
  REQUIRE(server.has_value());
  REQUIRE(efficiency.has_value());
  std::set<std::uint64_t> const efficiencyOnly{*efficiency};
  REQUIRE(source.setObjectClassAttributePublication(
      L"exercise", ownerId, *server, efficiencyOnly, true) ==
      umbra::detail::ObjectClassAttributeDeclarationStatus::applied);

  auto registered = source.registerObjectInstance(
      L"exercise", ownerId, *server);
  REQUIRE(registered.status ==
      umbra::detail::ObjectInstanceRegistrationStatus::applied);
  REQUIRE(registered.objectInstanceHandle != 0U);

  std::string const valueBytes{"\x4a\x06", 2U};
  std::vector<std::pair<std::uint64_t, rti1516_2025::VariableLengthData>> values{{
      *efficiency,
      rti1516_2025::VariableLengthData(valueBytes.data(), valueBytes.size()),
  }};
  REQUIRE(source.recordSuccessfulUpdateAttributeValues(
      L"exercise", ownerId, registered.objectInstanceHandle,
      *server, {"HLAreliable"}, &values) == FederationRegistryStatus::applied);

  auto change = source.planAttributeTransportationTypeChange(
      L"exercise", ownerId, registered.objectInstanceHandle,
      efficiencyOnly, "HLAbestEffort");
  REQUIRE(change.status ==
      umbra::detail::AttributeTransportationTypeChangeStatus::applied);
  REQUIRE(change.requestId != 0U);
  REQUIRE(change.objectInstanceHandle == registered.objectInstanceHandle);
  REQUIRE(change.attributeHandles == efficiencyOnly);
  REQUIRE(change.transportationName == "HLAbestEffort");
  REQUIRE(change.callbackRoute);

  std::wstring const saveLabel = L"attribute-transportation-type-change-process-restart";
  REQUIRE(source.requestFederationSave(
      L"exercise", ownerId, saveLabel).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(source.federateSaveBegun(
      L"exercise", ownerId).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(source.federateSaveComplete(
      L"exercise", ownerId).saveCompletedSuccessfully);

  auto committed = store->load(L"exercise", saveLabel);
  REQUIRE(committed.has_value());
  auto image = umbra::detail::FederationStateImageCodec::decode(
      committed->stateImage);
  REQUIRE(image.objects.size() == 1U);
  auto const& savedObject = image.objects.front();
  REQUIRE(savedObject.handle == registered.objectInstanceHandle);
  REQUIRE(savedObject.attributeValuesPresent);
  REQUIRE(savedObject.attributeValues.size() == 1U);
  REQUIRE(savedObject.attributeValues.front().attributeHandle == *efficiency);
  REQUIRE(savedObject.attributeValues.front().value == valueBytes);
  REQUIRE(savedObject.pendingAttributeTransportationTypeChanges.size() == 1U);
  auto const& savedChange =
      savedObject.pendingAttributeTransportationTypeChanges.front();
  REQUIRE(savedChange.requestId == change.requestId);
  REQUIRE(savedChange.requestingFederateId == ownerId);
  REQUIRE(savedChange.attributeHandles ==
      std::vector<std::uint64_t>{*efficiency});
  REQUIRE(savedChange.transportationName == "HLAbestEffort");
  REQUIRE(savedObject.pendingOperationCount == 1U);

  EmbeddedFederationRegistry restarted({}, store);
  REQUIRE(restarted.create(L"exercise", composedRestaurantDefinition()).status ==
      FederationRegistryStatus::applied);
  auto restartedJoined = restarted.join(
      L"exercise", L"publisher", L"owner", noOpCallbackRoute());
  REQUIRE(restartedJoined.status == FederationRegistryStatus::applied);
  REQUIRE(restartedJoined.membership);
  REQUIRE(restartedJoined.membership->id == ownerId);
  REQUIRE(restarted.requestFederationRestore(
      L"exercise", ownerId, saveLabel).status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  auto restored = restarted.federateRestoreComplete(L"exercise", ownerId);
  REQUIRE(restored.status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  REQUIRE(restored.attributeTransportationTypeChangeWorkItems.size() == 1U);
  auto const& restoredChange =
      restored.attributeTransportationTypeChangeWorkItems.front();
  REQUIRE(restoredChange.requestingFederateId == ownerId);
  REQUIRE(restoredChange.objectInstanceHandle ==
      registered.objectInstanceHandle);
  REQUIRE(restoredChange.requestId == change.requestId);
  REQUIRE(restoredChange.callbackRoute);

  auto delivered = restarted.beginAttributeTransportationTypeChange(
      L"exercise", ownerId, restoredChange.requestId);
  REQUIRE(delivered.has_value());
  REQUIRE(delivered->objectInstanceHandle == registered.objectInstanceHandle);
  REQUIRE(delivered->attributeHandles == efficiencyOnly);
  REQUIRE(delivered->transportationName == "HLAbestEffort");
  REQUIRE_FALSE(restarted.beginAttributeTransportationTypeChange(
      L"exercise", ownerId, restoredChange.requestId));

  auto query = restarted.attributeTransportationTypeQueryFor(
      L"exercise", ownerId, registered.objectInstanceHandle, *efficiency);
  REQUIRE(query.has_value());
  REQUIRE(query->transportationName == "HLAbestEffort");

  std::filesystem::remove_all(directory, ignored);
}

TEST_CASE(
    "Filesystem state image restores a pending interaction transportation-type change in a fresh registry",
    "[unit][kernel][federation-registry][save-restore][durable-save][filesystem][restore]"
    "[process-restart][process-restart-interaction-transportation-type-change][interaction-declaration-state]"
    "[interaction-transportation-type-change][transportation-management]") {
  auto const directory = temporarySaveCommitDirectory();
  std::error_code ignored;
  std::filesystem::remove_all(directory, ignored);
  auto store = std::make_shared<umbra::detail::FilesystemFederationSaveCommitStore>(
      directory);

  EmbeddedFederationRegistry source({}, store);
  REQUIRE(source.create(L"exercise", composedRestaurantDefinition()).status ==
      FederationRegistryStatus::applied);
  auto sourceJoined = source.join(
      L"exercise", L"publisher", L"owner", noOpCallbackRoute());
  REQUIRE(sourceJoined.status == FederationRegistryStatus::applied);
  REQUIRE(sourceJoined.membership);
  auto const ownerId = sourceJoined.membership->id;

  auto const takeOrder = source.interactionClassHandleFor(
      L"exercise", "HLAinteractionRoot.ServerAction.TakeOrder");
  REQUIRE(takeOrder.has_value());
  REQUIRE(source.setInteractionClassPublication(
      L"exercise", ownerId, *takeOrder, true) ==
      umbra::detail::InteractionClassDeclarationStatus::applied);

  auto change = source.planInteractionTransportationTypeChange(
      L"exercise", ownerId, *takeOrder, "HLAbestEffort");
  REQUIRE(change.status ==
      umbra::detail::InteractionTransportationTypeChangeStatus::applied);
  REQUIRE(change.interactionClassHandle == *takeOrder);
  REQUIRE(change.transportationName == "HLAbestEffort");
  REQUIRE(change.callbackRoute);

  std::wstring const saveLabel =
      L"interaction-transportation-type-change-process-restart";
  REQUIRE(source.requestFederationSave(
      L"exercise", ownerId, saveLabel).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(source.federateSaveBegun(
      L"exercise", ownerId).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(source.federateSaveComplete(
      L"exercise", ownerId).saveCompletedSuccessfully);

  auto committed = store->load(L"exercise", saveLabel);
  REQUIRE(committed.has_value());
  auto image = umbra::detail::FederationStateImageCodec::decode(
      committed->stateImage);
  REQUIRE(image.interactionDeclarationCount == 1U);
  REQUIRE(image.interactionDeclarations.size() == 1U);
  auto const& savedDeclaration = image.interactionDeclarations.front();
  REQUIRE(savedDeclaration.federateId == ownerId);
  REQUIRE(savedDeclaration.publishedInteractionClasses ==
      std::vector<std::uint64_t>{*takeOrder});
  REQUIRE(savedDeclaration.subscribedInteractionClasses.empty());
  REQUIRE(savedDeclaration.regionalSubscribedInteractionClasses.empty());
  REQUIRE(savedDeclaration.publishedObjectClassDirectedInteractions.empty());
  REQUIRE(savedDeclaration.subscribedObjectClassDirectedInteractions.empty());
  REQUIRE(savedDeclaration.interactionTransportationTypes.empty());
  REQUIRE(savedDeclaration.interactionOrderTypes.empty());
  REQUIRE(savedDeclaration.pendingInteractionTransportationTypeChanges.size() == 1U);
  auto const& savedChange =
      savedDeclaration.pendingInteractionTransportationTypeChanges.front();
  REQUIRE(savedChange.interactionClassHandle == *takeOrder);
  REQUIRE(savedChange.value == "HLAbestEffort");
  REQUIRE(image.objects.empty());
  REQUIRE(image.tsoInteractionMessages.empty());
  REQUIRE(image.tsoAttributeUpdateMessages.empty());
  REQUIRE(image.tsoObjectDeletionMessages.empty());
  REQUIRE(image.tsoDirectedInteractionMessages.empty());
  REQUIRE(image.tsoRequestRetractionRecords.empty());
  REQUIRE(image.tsoQueueEntries.empty());

  EmbeddedFederationRegistry restarted({}, store);
  REQUIRE(restarted.create(L"exercise", composedRestaurantDefinition()).status ==
      FederationRegistryStatus::applied);
  auto restartedJoined = restarted.join(
      L"exercise", L"publisher", L"owner", noOpCallbackRoute());
  REQUIRE(restartedJoined.status == FederationRegistryStatus::applied);
  REQUIRE(restartedJoined.membership);
  REQUIRE(restartedJoined.membership->id == ownerId);
  REQUIRE(restarted.requestFederationRestore(
      L"exercise", ownerId, saveLabel).status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  auto restored = restarted.federateRestoreComplete(L"exercise", ownerId);
  REQUIRE(restored.status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  REQUIRE(restored.interactionTransportationTypeChangeWorkItems.size() == 1U);
  auto const& restoredChange =
      restored.interactionTransportationTypeChangeWorkItems.front();
  REQUIRE(restoredChange.requestingFederateId == ownerId);
  REQUIRE(restoredChange.interactionClassHandle == *takeOrder);
  REQUIRE(restoredChange.callbackRoute);

  auto delivered = restarted.beginInteractionTransportationTypeChange(
      L"exercise", ownerId, restoredChange.interactionClassHandle);
  REQUIRE(delivered.has_value());
  REQUIRE(*delivered == "HLAbestEffort");
  REQUIRE_FALSE(restarted.beginInteractionTransportationTypeChange(
      L"exercise", ownerId, restoredChange.interactionClassHandle));

  auto query = restarted.interactionTransportationTypeQueryFor(
      L"exercise", ownerId, ownerId, *takeOrder);
  REQUIRE(query.has_value());
  REQUIRE(query->transportationName == "HLAbestEffort");

  std::filesystem::remove_all(directory, ignored);
}

TEST_CASE(
    "Federation restore rejects a corrupt durable state image",
    "[unit][kernel][federation-registry][save-restore][durable-save][state-image][restore][failure]") {
  class CorruptStateImageStore final : public umbra::detail::FederationSaveCommitStore {
   public:
    void commit(umbra::detail::FederationSaveCommitDescriptor const& descriptor) override {
      committed = descriptor;
    }

    [[nodiscard]] std::optional<umbra::detail::FederationSaveCommitDescriptor> load(
        std::wstring const&,
        std::wstring const&) const override {
      auto result = committed;
      auto image = umbra::detail::FederationStateImageCodec::decode(
          result.stateImage);
      ++image.normalizationSeed;
      if (image.interactionDeclarations.empty() && !image.members.empty()) {
        umbra::detail::FederationStateImageInteractionDeclaration declaration;
        declaration.federateId = image.members.front().id;
        declaration.publishedInteractionClasses = {1U};
        image.interactionDeclarations.push_back(std::move(declaration));
        image.interactionDeclarationCount = image.interactionDeclarations.size();
      } else if (!image.interactionDeclarations.empty()) {
        image.interactionDeclarations.front().publishedInteractionClasses.push_back(
            image.interactionDeclarations.front().publishedInteractionClasses.empty()
                ? 1U
                : image.interactionDeclarations.front().publishedInteractionClasses.back() + 1U);
      }
      result.stateImage = umbra::detail::FederationStateImageCodec::encode(image);
      return result;
    }

    umbra::detail::FederationSaveCommitDescriptor committed;
  };

  auto store = std::make_shared<CorruptStateImageStore>();
  EmbeddedFederationRegistry registry({}, store);
  REQUIRE(registry.create(L"exercise", validDefinition()).status ==
      FederationRegistryStatus::applied);
  auto joined = registry.join(
      L"exercise", L"trainer", L"alice", noOpCallbackRoute());
  REQUIRE(joined.status == FederationRegistryStatus::applied);
  REQUIRE(joined.membership);
  REQUIRE(registry.requestFederationSave(
      L"exercise", joined.membership->id, L"checkpoint").status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(registry.federateSaveBegun(
      L"exercise", joined.membership->id).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(registry.federateSaveComplete(
      L"exercise", joined.membership->id).saveCompletedSuccessfully);

  auto restore = registry.requestFederationRestore(
      L"exercise", joined.membership->id, L"checkpoint");
  REQUIRE(restore.status == umbra::detail::FederationRestoreControlStatus::snapshot_not_found);
  REQUIRE(restore.notifications.size() == 1U);
}

TEST_CASE(
    "Filesystem state image restores a published interaction declaration in a fresh registry",
    "[unit][kernel][federation-registry][save-restore][durable-save][filesystem][restore]"
    "[process-restart][process-restart-interaction-declaration][interaction-declaration-state]"
    "[interaction-management][declaration-management]") {
  auto const directory = temporarySaveCommitDirectory();
  std::error_code ignored;
  std::filesystem::remove_all(directory, ignored);
  auto store = std::make_shared<umbra::detail::FilesystemFederationSaveCommitStore>(
      directory);

  EmbeddedFederationRegistry source({}, store);
  REQUIRE(source.create(L"exercise", composedRestaurantDefinition()).status ==
      FederationRegistryStatus::applied);
  auto sourceJoined = source.join(
      L"exercise", L"publisher", L"owner", noOpCallbackRoute());
  REQUIRE(sourceJoined.status == FederationRegistryStatus::applied);
  REQUIRE(sourceJoined.membership);
  auto const ownerId = sourceJoined.membership->id;

  auto const takeOrder = source.interactionClassHandleFor(
      L"exercise", "HLAinteractionRoot.ServerAction.TakeOrder");
  REQUIRE(takeOrder.has_value());
  REQUIRE(source.setInteractionClassPublication(
      L"exercise", ownerId, *takeOrder, true) ==
      umbra::detail::InteractionClassDeclarationStatus::applied);

  std::wstring const saveLabel = L"interaction-declaration-process-restart";
  REQUIRE(source.requestFederationSave(
      L"exercise", ownerId, saveLabel).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(source.federateSaveBegun(
      L"exercise", ownerId).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(source.federateSaveComplete(
      L"exercise", ownerId).saveCompletedSuccessfully);

  auto committed = store->load(L"exercise", saveLabel);
  REQUIRE(committed.has_value());
  auto image = umbra::detail::FederationStateImageCodec::decode(
      committed->stateImage);
  REQUIRE(image.interactionDeclarationCount == 1U);
  REQUIRE(image.interactionDeclarations.size() == 1U);
  auto const& savedDeclaration = image.interactionDeclarations.front();
  REQUIRE(savedDeclaration.federateId == ownerId);
  REQUIRE(savedDeclaration.publishedInteractionClasses ==
      std::vector<std::uint64_t>{*takeOrder});
  REQUIRE(savedDeclaration.subscribedInteractionClasses.empty());
  REQUIRE(savedDeclaration.regionalSubscribedInteractionClasses.empty());
  REQUIRE(savedDeclaration.publishedObjectClassDirectedInteractions.empty());
  REQUIRE(savedDeclaration.subscribedObjectClassDirectedInteractions.empty());
  REQUIRE(savedDeclaration.interactionTransportationTypes.empty());
  REQUIRE(savedDeclaration.interactionOrderTypes.empty());
  REQUIRE(savedDeclaration.pendingInteractionTransportationTypeChanges.empty());
  REQUIRE(image.objects.empty());
  REQUIRE(image.tsoInteractionMessages.empty());
  REQUIRE(image.tsoAttributeUpdateMessages.empty());
  REQUIRE(image.tsoObjectDeletionMessages.empty());
  REQUIRE(image.tsoDirectedInteractionMessages.empty());
  REQUIRE(image.tsoRequestRetractionRecords.empty());
  REQUIRE(image.tsoQueueEntries.empty());

  EmbeddedFederationRegistry restarted({}, store);
  REQUIRE(restarted.create(L"exercise", composedRestaurantDefinition()).status ==
      FederationRegistryStatus::applied);
  auto restartedJoined = restarted.join(
      L"exercise", L"publisher", L"owner", noOpCallbackRoute());
  REQUIRE(restartedJoined.status == FederationRegistryStatus::applied);
  REQUIRE(restartedJoined.membership);
  REQUIRE(restartedJoined.membership->id == ownerId);
  REQUIRE(restarted.requestFederationRestore(
      L"exercise", ownerId, saveLabel).status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  auto restored = restarted.federateRestoreComplete(L"exercise", ownerId);
  REQUIRE(restored.status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  REQUIRE(restored.interactionTransportationTypeChangeWorkItems.empty());

  auto declaration = restarted.interactionClassDeclarationFor(
      L"exercise", ownerId, *takeOrder);
  REQUIRE(declaration.has_value());
  REQUIRE(declaration->published);
  REQUIRE_FALSE(declaration->subscriptionActive.has_value());

  std::filesystem::remove_all(directory, ignored);
}

TEST_CASE(
    "Filesystem state image restores an interaction subscription declaration in a fresh registry",
    "[unit][kernel][federation-registry][save-restore][durable-save][filesystem][restore]"
    "[process-restart][process-restart-interaction-declaration][interaction-declaration-state]"
    "[interaction-management][declaration-management]") {
  auto const directory = temporarySaveCommitDirectory();
  std::error_code ignored;
  std::filesystem::remove_all(directory, ignored);
  auto store = std::make_shared<umbra::detail::FilesystemFederationSaveCommitStore>(
      directory);

  EmbeddedFederationRegistry source({}, store);
  REQUIRE(source.create(L"exercise", composedRestaurantDefinition()).status ==
      FederationRegistryStatus::applied);
  auto sourceJoined = source.join(
      L"exercise", L"subscriber", L"subscriber", noOpCallbackRoute());
  REQUIRE(sourceJoined.status == FederationRegistryStatus::applied);
  REQUIRE(sourceJoined.membership);
  auto const subscriberId = sourceJoined.membership->id;

  auto const takeOrder = source.interactionClassHandleFor(
      L"exercise", "HLAinteractionRoot.ServerAction.TakeOrder");
  REQUIRE(takeOrder.has_value());
  REQUIRE(source.setInteractionClassSubscription(
      L"exercise", subscriberId, *takeOrder, true) ==
      umbra::detail::InteractionClassDeclarationStatus::applied);

  std::wstring const saveLabel = L"interaction-subscription-process-restart";
  REQUIRE(source.requestFederationSave(
      L"exercise", subscriberId, saveLabel).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(source.federateSaveBegun(
      L"exercise", subscriberId).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(source.federateSaveComplete(
      L"exercise", subscriberId).saveCompletedSuccessfully);

  auto committed = store->load(L"exercise", saveLabel);
  REQUIRE(committed.has_value());
  auto image = umbra::detail::FederationStateImageCodec::decode(
      committed->stateImage);
  REQUIRE(image.interactionDeclarationCount == 1U);
  REQUIRE(image.interactionDeclarations.size() == 1U);
  auto const& savedDeclaration = image.interactionDeclarations.front();
  REQUIRE(savedDeclaration.federateId == subscriberId);
  REQUIRE(savedDeclaration.publishedInteractionClasses.empty());
  REQUIRE(savedDeclaration.subscribedInteractionClasses.size() == 1U);
  REQUIRE(savedDeclaration.subscribedInteractionClasses.front().interactionClassHandle ==
      *takeOrder);
  REQUIRE(savedDeclaration.subscribedInteractionClasses.front().active);
  REQUIRE(savedDeclaration.regionalSubscribedInteractionClasses.empty());
  REQUIRE(savedDeclaration.publishedObjectClassDirectedInteractions.empty());
  REQUIRE(savedDeclaration.subscribedObjectClassDirectedInteractions.empty());
  REQUIRE(savedDeclaration.interactionTransportationTypes.empty());
  REQUIRE(savedDeclaration.interactionOrderTypes.empty());
  REQUIRE(savedDeclaration.pendingInteractionTransportationTypeChanges.empty());
  REQUIRE(image.objects.empty());
  REQUIRE(image.tsoInteractionMessages.empty());
  REQUIRE(image.tsoAttributeUpdateMessages.empty());
  REQUIRE(image.tsoObjectDeletionMessages.empty());
  REQUIRE(image.tsoDirectedInteractionMessages.empty());
  REQUIRE(image.tsoRequestRetractionRecords.empty());
  REQUIRE(image.tsoQueueEntries.empty());

  EmbeddedFederationRegistry restarted({}, store);
  REQUIRE(restarted.create(L"exercise", composedRestaurantDefinition()).status ==
      FederationRegistryStatus::applied);
  auto restartedJoined = restarted.join(
      L"exercise", L"subscriber", L"subscriber", noOpCallbackRoute());
  REQUIRE(restartedJoined.status == FederationRegistryStatus::applied);
  REQUIRE(restartedJoined.membership);
  REQUIRE(restartedJoined.membership->id == subscriberId);
  REQUIRE(restarted.requestFederationRestore(
      L"exercise", subscriberId, saveLabel).status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  auto restored = restarted.federateRestoreComplete(L"exercise", subscriberId);
  REQUIRE(restored.status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  REQUIRE(restored.interactionTransportationTypeChangeWorkItems.empty());

  auto declaration = restarted.interactionClassDeclarationFor(
      L"exercise", subscriberId, *takeOrder);
  REQUIRE(declaration.has_value());
  REQUIRE_FALSE(declaration->published);
  REQUIRE(declaration->subscriptionActive.has_value());
  REQUIRE(*declaration->subscriptionActive);

  std::filesystem::remove_all(directory, ignored);
}

TEST_CASE(
    "Filesystem state image restores a same-class interaction publication and subscription in a fresh registry",
    "[unit][kernel][federation-registry][save-restore][durable-save][filesystem][restore]"
    "[process-restart][process-restart-interaction-declaration][interaction-declaration-state]"
    "[interaction-management][declaration-management]") {
  auto const directory = temporarySaveCommitDirectory();
  std::error_code ignored;
  std::filesystem::remove_all(directory, ignored);
  auto store = std::make_shared<umbra::detail::FilesystemFederationSaveCommitStore>(
      directory);

  EmbeddedFederationRegistry source({}, store);
  REQUIRE(source.create(L"exercise", composedRestaurantDefinition()).status ==
      FederationRegistryStatus::applied);
  auto sourceJoined = source.join(
      L"exercise", L"publisher-subscriber", L"dual", noOpCallbackRoute());
  REQUIRE(sourceJoined.status == FederationRegistryStatus::applied);
  REQUIRE(sourceJoined.membership);
  auto const federateId = sourceJoined.membership->id;

  auto const takeOrder = source.interactionClassHandleFor(
      L"exercise", "HLAinteractionRoot.ServerAction.TakeOrder");
  REQUIRE(takeOrder.has_value());
  REQUIRE(source.setInteractionClassPublication(
      L"exercise", federateId, *takeOrder, true) ==
      umbra::detail::InteractionClassDeclarationStatus::applied);
  REQUIRE(source.setInteractionClassSubscription(
      L"exercise", federateId, *takeOrder, true) ==
      umbra::detail::InteractionClassDeclarationStatus::applied);

  std::wstring const saveLabel = L"interaction-publication-subscription-process-restart";
  REQUIRE(source.requestFederationSave(
      L"exercise", federateId, saveLabel).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(source.federateSaveBegun(
      L"exercise", federateId).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(source.federateSaveComplete(
      L"exercise", federateId).saveCompletedSuccessfully);

  auto committed = store->load(L"exercise", saveLabel);
  REQUIRE(committed.has_value());
  auto image = umbra::detail::FederationStateImageCodec::decode(
      committed->stateImage);
  REQUIRE(image.interactionDeclarationCount == 1U);
  REQUIRE(image.interactionDeclarations.size() == 1U);
  auto const& savedDeclaration = image.interactionDeclarations.front();
  REQUIRE(savedDeclaration.federateId == federateId);
  REQUIRE(savedDeclaration.publishedInteractionClasses ==
      std::vector<std::uint64_t>{*takeOrder});
  REQUIRE(savedDeclaration.subscribedInteractionClasses.size() == 1U);
  REQUIRE(savedDeclaration.subscribedInteractionClasses.front().interactionClassHandle ==
      *takeOrder);
  REQUIRE(savedDeclaration.subscribedInteractionClasses.front().active);
  REQUIRE(savedDeclaration.regionalSubscribedInteractionClasses.empty());
  REQUIRE(savedDeclaration.publishedObjectClassDirectedInteractions.empty());
  REQUIRE(savedDeclaration.subscribedObjectClassDirectedInteractions.empty());
  REQUIRE(savedDeclaration.interactionTransportationTypes.empty());
  REQUIRE(savedDeclaration.interactionOrderTypes.empty());
  REQUIRE(savedDeclaration.pendingInteractionTransportationTypeChanges.empty());
  REQUIRE(image.objects.empty());
  REQUIRE(image.tsoInteractionMessages.empty());
  REQUIRE(image.tsoAttributeUpdateMessages.empty());
  REQUIRE(image.tsoObjectDeletionMessages.empty());
  REQUIRE(image.tsoDirectedInteractionMessages.empty());
  REQUIRE(image.tsoRequestRetractionRecords.empty());
  REQUIRE(image.tsoQueueEntries.empty());

  EmbeddedFederationRegistry restarted({}, store);
  REQUIRE(restarted.create(L"exercise", composedRestaurantDefinition()).status ==
      FederationRegistryStatus::applied);
  auto restartedJoined = restarted.join(
      L"exercise", L"publisher-subscriber", L"dual", noOpCallbackRoute());
  REQUIRE(restartedJoined.status == FederationRegistryStatus::applied);
  REQUIRE(restartedJoined.membership);
  REQUIRE(restartedJoined.membership->id == federateId);
  REQUIRE(restarted.requestFederationRestore(
      L"exercise", federateId, saveLabel).status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  auto restored = restarted.federateRestoreComplete(L"exercise", federateId);
  REQUIRE(restored.status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  REQUIRE(restored.interactionTransportationTypeChangeWorkItems.empty());

  auto declaration = restarted.interactionClassDeclarationFor(
      L"exercise", federateId, *takeOrder);
  REQUIRE(declaration.has_value());
  REQUIRE(declaration->published);
  REQUIRE(declaration->subscriptionActive.has_value());
  REQUIRE(*declaration->subscriptionActive);

  std::filesystem::remove_all(directory, ignored);
}

TEST_CASE(
    "Filesystem state image restores a regional interaction subscription in a fresh registry",
    "[unit][kernel][federation-registry][save-restore][durable-save][filesystem][restore]"
    "[process-restart][process-restart-interaction-declaration][interaction-declaration-state]"
    "[interaction-management][ddm][regional-interaction]") {
  auto const directory = temporarySaveCommitDirectory();
  std::error_code ignored;
  std::filesystem::remove_all(directory, ignored);
  auto store = std::make_shared<umbra::detail::FilesystemFederationSaveCommitStore>(
      directory);

  EmbeddedFederationRegistry source({}, store);
  REQUIRE(source.create(L"exercise", composedRestaurantDefinition()).status ==
      FederationRegistryStatus::applied);
  auto sourceJoined = source.join(
      L"exercise", L"regional-subscriber", L"regional", noOpCallbackRoute());
  REQUIRE(sourceJoined.status == FederationRegistryStatus::applied);
  REQUIRE(sourceJoined.membership);
  auto const federateId = sourceJoined.membership->id;

  auto const dimension = source.dimensionHandleFor(L"exercise", "ServerId");
  auto const takeOrder = source.interactionClassHandleFor(
      L"exercise",
      "HLAinteractionRoot.CustomerTransactions.FoodServed.MainCourseServed");
  REQUIRE(dimension.has_value());
  REQUIRE(takeOrder.has_value());
  auto created = source.createRegion(L"exercise", federateId, {*dimension});
  REQUIRE(created.status == umbra::detail::RegionServiceStatus::applied);
  REQUIRE(created.regionHandle != 0U);
  REQUIRE(source.setRangeBounds(
      L"exercise", federateId, created.regionHandle, *dimension,
      umbra::detail::RegionRangeBounds{2UL, 5UL}) ==
      umbra::detail::RegionServiceStatus::applied);
  REQUIRE(source.commitRegionModifications(
      L"exercise", federateId, {created.regionHandle}) ==
      umbra::detail::RegionServiceStatus::applied);
  REQUIRE(source.setInteractionClassRegionalSubscription(
      L"exercise", federateId, *takeOrder, {created.regionHandle}, true) ==
      umbra::detail::RegionalInteractionClassDeclarationStatus::applied);

  std::wstring const saveLabel = L"interaction-regional-subscription-process-restart";
  REQUIRE(source.requestFederationSave(
      L"exercise", federateId, saveLabel).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(source.federateSaveBegun(
      L"exercise", federateId).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(source.federateSaveComplete(
      L"exercise", federateId).saveCompletedSuccessfully);

  auto committed = store->load(L"exercise", saveLabel);
  REQUIRE(committed.has_value());
  auto image = umbra::detail::FederationStateImageCodec::decode(
      committed->stateImage);
  REQUIRE(image.interactionDeclarationCount == 1U);
  REQUIRE(image.interactionDeclarations.size() == 1U);
  auto const& savedDeclaration = image.interactionDeclarations.front();
  REQUIRE(savedDeclaration.federateId == federateId);
  REQUIRE(savedDeclaration.publishedInteractionClasses.empty());
  REQUIRE(savedDeclaration.subscribedInteractionClasses.empty());
  REQUIRE(savedDeclaration.regionalSubscribedInteractionClasses.size() == 1U);
  auto const& savedSubscription =
      savedDeclaration.regionalSubscribedInteractionClasses.front();
  REQUIRE(savedSubscription.interactionClassHandle == *takeOrder);
  REQUIRE(savedSubscription.regionHandle == created.regionHandle);
  REQUIRE(savedSubscription.active);
  REQUIRE(image.regions.size() == 1U);
  REQUIRE(image.objects.empty());
  REQUIRE(image.tsoInteractionMessages.empty());
  REQUIRE(image.tsoAttributeUpdateMessages.empty());
  REQUIRE(image.tsoObjectDeletionMessages.empty());
  REQUIRE(image.tsoDirectedInteractionMessages.empty());
  REQUIRE(image.tsoRequestRetractionRecords.empty());
  REQUIRE(image.tsoQueueEntries.empty());

  EmbeddedFederationRegistry restarted({}, store);
  REQUIRE(restarted.create(L"exercise", composedRestaurantDefinition()).status ==
      FederationRegistryStatus::applied);
  auto restartedJoined = restarted.join(
      L"exercise", L"regional-subscriber", L"regional", noOpCallbackRoute());
  REQUIRE(restartedJoined.status == FederationRegistryStatus::applied);
  REQUIRE(restartedJoined.membership);
  REQUIRE(restartedJoined.membership->id == federateId);
  REQUIRE(restarted.requestFederationRestore(
      L"exercise", federateId, saveLabel).status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  auto restored = restarted.federateRestoreComplete(L"exercise", federateId);
  REQUIRE(restored.status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  REQUIRE(restored.interactionTransportationTypeChangeWorkItems.empty());

  auto dimensions = restarted.dimensionHandleSetForRegion(
      L"exercise", federateId, created.regionHandle);
  REQUIRE(dimensions.status == umbra::detail::RegionServiceStatus::applied);
  REQUIRE(dimensions.dimensionHandles == std::set<std::uint64_t>{*dimension});
  auto bounds = restarted.rangeBoundsForRegion(
      L"exercise", federateId, created.regionHandle, *dimension);
  REQUIRE(bounds.status == umbra::detail::RegionServiceStatus::applied);
  REQUIRE(bounds.range.lowerBound == 2UL);
  REQUIRE(bounds.range.upperBound == 5UL);

  std::wstring const roundTripLabel =
      L"interaction-regional-subscription-round-trip";
  REQUIRE(restarted.requestFederationSave(
      L"exercise", federateId, roundTripLabel).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(restarted.federateSaveBegun(
      L"exercise", federateId).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(restarted.federateSaveComplete(
      L"exercise", federateId).saveCompletedSuccessfully);
  auto roundTrip = store->load(L"exercise", roundTripLabel);
  REQUIRE(roundTrip.has_value());
  auto roundTripImage = umbra::detail::FederationStateImageCodec::decode(
      roundTrip->stateImage);
  REQUIRE(roundTripImage.interactionDeclarations.size() == 1U);
  REQUIRE(roundTripImage.interactionDeclarations.front()
              .regionalSubscribedInteractionClasses.size() == 1U);
  REQUIRE(roundTripImage.interactionDeclarations.front()
              .regionalSubscribedInteractionClasses.front()
              .regionHandle == created.regionHandle);

  std::filesystem::remove_all(directory, ignored);
}

TEST_CASE(
    "Filesystem state image restores multiple interaction declaration entries in a fresh registry",
    "[unit][kernel][federation-registry][save-restore][durable-save][filesystem][restore]"
    "[process-restart][process-restart-interaction-declaration][interaction-declaration-state]"
    "[interaction-management][declaration-management]") {
  auto const directory = temporarySaveCommitDirectory();
  std::error_code ignored;
  std::filesystem::remove_all(directory, ignored);
  auto store = std::make_shared<umbra::detail::FilesystemFederationSaveCommitStore>(
      directory);

  EmbeddedFederationRegistry source({}, store);
  REQUIRE(source.create(L"exercise", composedRestaurantDefinition()).status ==
      FederationRegistryStatus::applied);
  auto sourceJoined = source.join(
      L"exercise", L"publisher", L"owner", noOpCallbackRoute());
  REQUIRE(sourceJoined.status == FederationRegistryStatus::applied);
  REQUIRE(sourceJoined.membership);
  auto const ownerId = sourceJoined.membership->id;

  auto const customerSeated = source.interactionClassHandleFor(
      L"exercise", "HLAinteractionRoot.CustomerTransactions.CustomerSeated");
  auto const takeOrder = source.interactionClassHandleFor(
      L"exercise", "HLAinteractionRoot.ServerAction.TakeOrder");
  REQUIRE(customerSeated.has_value());
  REQUIRE(takeOrder.has_value());
  std::vector<std::uint64_t> expectedPublished{
      *customerSeated,
      *takeOrder,
  };
  std::ranges::sort(expectedPublished);
  REQUIRE(source.setInteractionClassPublication(
      L"exercise", ownerId, *customerSeated, true) ==
      umbra::detail::InteractionClassDeclarationStatus::applied);
  REQUIRE(source.setInteractionClassPublication(
      L"exercise", ownerId, *takeOrder, true) ==
      umbra::detail::InteractionClassDeclarationStatus::applied);

  std::wstring const saveLabel = L"interaction-multiple-declarations-process-restart";
  REQUIRE(source.requestFederationSave(
      L"exercise", ownerId, saveLabel).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(source.federateSaveBegun(
      L"exercise", ownerId).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(source.federateSaveComplete(
      L"exercise", ownerId).saveCompletedSuccessfully);

  auto committed = store->load(L"exercise", saveLabel);
  REQUIRE(committed.has_value());
  auto image = umbra::detail::FederationStateImageCodec::decode(
      committed->stateImage);
  REQUIRE(image.interactionDeclarationCount == 1U);
  REQUIRE(image.interactionDeclarations.size() == 1U);
  auto const& savedDeclaration = image.interactionDeclarations.front();
  REQUIRE(savedDeclaration.federateId == ownerId);
  REQUIRE(savedDeclaration.publishedInteractionClasses == expectedPublished);
  REQUIRE(savedDeclaration.subscribedInteractionClasses.empty());
  REQUIRE(savedDeclaration.regionalSubscribedInteractionClasses.empty());
  REQUIRE(savedDeclaration.publishedObjectClassDirectedInteractions.empty());
  REQUIRE(savedDeclaration.subscribedObjectClassDirectedInteractions.empty());
  REQUIRE(savedDeclaration.interactionTransportationTypes.empty());
  REQUIRE(savedDeclaration.interactionOrderTypes.empty());
  REQUIRE(savedDeclaration.pendingInteractionTransportationTypeChanges.empty());
  REQUIRE(image.objects.empty());
  REQUIRE(image.tsoInteractionMessages.empty());
  REQUIRE(image.tsoAttributeUpdateMessages.empty());
  REQUIRE(image.tsoObjectDeletionMessages.empty());
  REQUIRE(image.tsoDirectedInteractionMessages.empty());
  REQUIRE(image.tsoRequestRetractionRecords.empty());
  REQUIRE(image.tsoQueueEntries.empty());

  EmbeddedFederationRegistry restarted({}, store);
  REQUIRE(restarted.create(L"exercise", composedRestaurantDefinition()).status ==
      FederationRegistryStatus::applied);
  auto restartedJoined = restarted.join(
      L"exercise", L"publisher", L"owner", noOpCallbackRoute());
  REQUIRE(restartedJoined.status == FederationRegistryStatus::applied);
  REQUIRE(restartedJoined.membership);
  REQUIRE(restartedJoined.membership->id == ownerId);
  REQUIRE(restarted.requestFederationRestore(
      L"exercise", ownerId, saveLabel).status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  auto restored = restarted.federateRestoreComplete(L"exercise", ownerId);
  REQUIRE(restored.status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  REQUIRE(restored.interactionTransportationTypeChangeWorkItems.empty());

  auto customerDeclaration = restarted.interactionClassDeclarationFor(
      L"exercise", ownerId, *customerSeated);
  auto takeOrderDeclaration = restarted.interactionClassDeclarationFor(
      L"exercise", ownerId, *takeOrder);
  REQUIRE(customerDeclaration.has_value());
  REQUIRE(customerDeclaration->published);
  REQUIRE_FALSE(customerDeclaration->subscriptionActive.has_value());
  REQUIRE(takeOrderDeclaration.has_value());
  REQUIRE(takeOrderDeclaration->published);
  REQUIRE_FALSE(takeOrderDeclaration->subscriptionActive.has_value());

  std::filesystem::remove_all(directory, ignored);
}

TEST_CASE(
    "Filesystem state image restores independent interaction declarations for multiple federates",
    "[unit][kernel][federation-registry][save-restore][durable-save][filesystem][restore]"
    "[process-restart][process-restart-interaction-declaration][interaction-declaration-state]"
    "[interaction-management][declaration-management][multi-federate-callback-ordering]") {
  auto const directory = temporarySaveCommitDirectory();
  std::error_code ignored;
  std::filesystem::remove_all(directory, ignored);
  auto store = std::make_shared<umbra::detail::FilesystemFederationSaveCommitStore>(
      directory);

  EmbeddedFederationRegistry source({}, store);
  REQUIRE(source.create(L"exercise", composedRestaurantDefinition()).status ==
      FederationRegistryStatus::applied);
  auto first = source.join(
      L"exercise", L"publisher-a", L"publisher-a", noOpCallbackRoute());
  auto second = source.join(
      L"exercise", L"publisher-b", L"publisher-b", noOpCallbackRoute());
  REQUIRE(first.status == FederationRegistryStatus::applied);
  REQUIRE(second.status == FederationRegistryStatus::applied);
  REQUIRE(first.membership);
  REQUIRE(second.membership);
  auto const firstId = first.membership->id;
  auto const secondId = second.membership->id;

  auto const firstClass = source.interactionClassHandleFor(
      L"exercise", "HLAinteractionRoot.CustomerTransactions.CustomerSeated");
  auto const secondClass = source.interactionClassHandleFor(
      L"exercise", "HLAinteractionRoot.ServerAction.TakeOrder");
  REQUIRE(firstClass.has_value());
  REQUIRE(secondClass.has_value());
  REQUIRE(source.setInteractionClassPublication(
      L"exercise", firstId, *firstClass, true) ==
      umbra::detail::InteractionClassDeclarationStatus::applied);
  REQUIRE(source.setInteractionClassPublication(
      L"exercise", secondId, *secondClass, true) ==
      umbra::detail::InteractionClassDeclarationStatus::applied);

  std::wstring const saveLabel = L"interaction-multi-federate-process-restart";
  REQUIRE(source.requestFederationSave(
      L"exercise", firstId, saveLabel).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(source.federateSaveBegun(
      L"exercise", firstId).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(source.federateSaveBegun(
      L"exercise", secondId).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE_FALSE(source.federateSaveComplete(
      L"exercise", firstId).saveCompletedSuccessfully);
  REQUIRE(source.federateSaveComplete(
      L"exercise", secondId).saveCompletedSuccessfully);

  auto committed = store->load(L"exercise", saveLabel);
  REQUIRE(committed.has_value());
  auto image = umbra::detail::FederationStateImageCodec::decode(
      committed->stateImage);
  REQUIRE(image.interactionDeclarationCount == 2U);
  REQUIRE(image.interactionDeclarations.size() == 2U);
  REQUIRE(image.interactionDeclarations[0].federateId == firstId);
  REQUIRE(image.interactionDeclarations[1].federateId == secondId);
  REQUIRE(image.interactionDeclarations[0].publishedInteractionClasses ==
      std::vector<std::uint64_t>{*firstClass});
  REQUIRE(image.interactionDeclarations[1].publishedInteractionClasses ==
      std::vector<std::uint64_t>{*secondClass});
  REQUIRE(image.objects.empty());
  REQUIRE(image.tsoInteractionMessages.empty());
  REQUIRE(image.tsoAttributeUpdateMessages.empty());
  REQUIRE(image.tsoObjectDeletionMessages.empty());
  REQUIRE(image.tsoDirectedInteractionMessages.empty());
  REQUIRE(image.tsoRequestRetractionRecords.empty());
  REQUIRE(image.tsoQueueEntries.empty());

  EmbeddedFederationRegistry restarted({}, store);
  REQUIRE(restarted.create(L"exercise", composedRestaurantDefinition()).status ==
      FederationRegistryStatus::applied);
  auto restartedFirst = restarted.join(
      L"exercise", L"publisher-a", L"publisher-a", noOpCallbackRoute());
  auto restartedSecond = restarted.join(
      L"exercise", L"publisher-b", L"publisher-b", noOpCallbackRoute());
  REQUIRE(restartedFirst.status == FederationRegistryStatus::applied);
  REQUIRE(restartedSecond.status == FederationRegistryStatus::applied);
  REQUIRE(restartedFirst.membership);
  REQUIRE(restartedSecond.membership);
  REQUIRE(restartedFirst.membership->id == firstId);
  REQUIRE(restartedSecond.membership->id == secondId);
  REQUIRE(restarted.requestFederationRestore(
      L"exercise", firstId, saveLabel).status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  REQUIRE(restarted.federateRestoreComplete(
      L"exercise", firstId).status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  auto restored = restarted.federateRestoreComplete(
      L"exercise", secondId);
  REQUIRE(restored.status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  REQUIRE(restored.interactionTransportationTypeChangeWorkItems.empty());

  auto firstDeclaration = restarted.interactionClassDeclarationFor(
      L"exercise", firstId, *firstClass);
  auto secondDeclaration = restarted.interactionClassDeclarationFor(
      L"exercise", secondId, *secondClass);
  REQUIRE(firstDeclaration.has_value());
  REQUIRE(firstDeclaration->published);
  REQUIRE(secondDeclaration.has_value());
  REQUIRE(secondDeclaration->published);

  std::filesystem::remove_all(directory, ignored);
}

TEST_CASE(
    "Filesystem state image restores a committed interaction transportation-type override in a fresh registry",
    "[unit][kernel][federation-registry][save-restore][durable-save][filesystem][restore]"
    "[process-restart][process-restart-interaction-transportation-type-override]"
    "[interaction-declaration-state]"
    "[interaction-transportation-type-change][transportation-management]") {
  auto const directory = temporarySaveCommitDirectory();
  std::error_code ignored;
  std::filesystem::remove_all(directory, ignored);
  auto store = std::make_shared<umbra::detail::FilesystemFederationSaveCommitStore>(
      directory);

  EmbeddedFederationRegistry source({}, store);
  REQUIRE(source.create(L"exercise", composedRestaurantDefinition()).status ==
      FederationRegistryStatus::applied);
  auto sourceJoined = source.join(
      L"exercise", L"publisher", L"owner", noOpCallbackRoute());
  REQUIRE(sourceJoined.status == FederationRegistryStatus::applied);
  REQUIRE(sourceJoined.membership);
  auto const ownerId = sourceJoined.membership->id;

  auto const takeOrder = source.interactionClassHandleFor(
      L"exercise", "HLAinteractionRoot.ServerAction.TakeOrder");
  REQUIRE(takeOrder.has_value());
  REQUIRE(source.setInteractionClassPublication(
      L"exercise", ownerId, *takeOrder, true) ==
      umbra::detail::InteractionClassDeclarationStatus::applied);

  auto change = source.planInteractionTransportationTypeChange(
      L"exercise", ownerId, *takeOrder, "HLAbestEffort");
  REQUIRE(change.status ==
      umbra::detail::InteractionTransportationTypeChangeStatus::applied);
  auto committedChange = source.beginInteractionTransportationTypeChange(
      L"exercise", ownerId, *takeOrder);
  REQUIRE(committedChange.has_value());
  REQUIRE(*committedChange == "HLAbestEffort");
  REQUIRE_FALSE(source.beginInteractionTransportationTypeChange(
      L"exercise", ownerId, *takeOrder));

  auto sourceQuery = source.interactionTransportationTypeQueryFor(
      L"exercise", ownerId, ownerId, *takeOrder);
  REQUIRE(sourceQuery.has_value());
  REQUIRE(sourceQuery->transportationName == "HLAbestEffort");

  std::wstring const saveLabel =
      L"interaction-transportation-type-override-process-restart";
  REQUIRE(source.requestFederationSave(
      L"exercise", ownerId, saveLabel).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(source.federateSaveBegun(
      L"exercise", ownerId).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(source.federateSaveComplete(
      L"exercise", ownerId).saveCompletedSuccessfully);

  auto committed = store->load(L"exercise", saveLabel);
  REQUIRE(committed.has_value());
  auto image = umbra::detail::FederationStateImageCodec::decode(
      committed->stateImage);
  REQUIRE(image.interactionDeclarationCount == 1U);
  REQUIRE(image.interactionDeclarations.size() == 1U);
  auto const& savedDeclaration = image.interactionDeclarations.front();
  REQUIRE(savedDeclaration.federateId == ownerId);
  REQUIRE(savedDeclaration.publishedInteractionClasses ==
      std::vector<std::uint64_t>{*takeOrder});
  REQUIRE(savedDeclaration.subscribedInteractionClasses.empty());
  REQUIRE(savedDeclaration.regionalSubscribedInteractionClasses.empty());
  REQUIRE(savedDeclaration.interactionTransportationTypes.size() == 1U);
  REQUIRE(savedDeclaration.interactionTransportationTypes.front().interactionClassHandle ==
      *takeOrder);
  REQUIRE(savedDeclaration.interactionTransportationTypes.front().value ==
      "HLAbestEffort");
  REQUIRE(savedDeclaration.interactionOrderTypes.empty());
  REQUIRE(savedDeclaration.pendingInteractionTransportationTypeChanges.empty());
  REQUIRE(image.objects.empty());
  REQUIRE(image.tsoInteractionMessages.empty());
  REQUIRE(image.tsoAttributeUpdateMessages.empty());
  REQUIRE(image.tsoObjectDeletionMessages.empty());
  REQUIRE(image.tsoDirectedInteractionMessages.empty());
  REQUIRE(image.tsoRequestRetractionRecords.empty());
  REQUIRE(image.tsoQueueEntries.empty());

  EmbeddedFederationRegistry restarted({}, store);
  REQUIRE(restarted.create(L"exercise", composedRestaurantDefinition()).status ==
      FederationRegistryStatus::applied);
  auto restartedJoined = restarted.join(
      L"exercise", L"publisher", L"owner", noOpCallbackRoute());
  REQUIRE(restartedJoined.status == FederationRegistryStatus::applied);
  REQUIRE(restartedJoined.membership);
  REQUIRE(restartedJoined.membership->id == ownerId);
  REQUIRE(restarted.requestFederationRestore(
      L"exercise", ownerId, saveLabel).status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  auto restored = restarted.federateRestoreComplete(L"exercise", ownerId);
  REQUIRE(restored.status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  REQUIRE(restored.interactionTransportationTypeChangeWorkItems.empty());

  auto restoredDeclaration = restarted.interactionClassDeclarationFor(
      L"exercise", ownerId, *takeOrder);
  REQUIRE(restoredDeclaration.has_value());
  REQUIRE(restoredDeclaration->published);
  auto restoredQuery = restarted.interactionTransportationTypeQueryFor(
      L"exercise", ownerId, ownerId, *takeOrder);
  REQUIRE(restoredQuery.has_value());
  REQUIRE(restoredQuery->transportationName == "HLAbestEffort");

  std::filesystem::remove_all(directory, ignored);
}

TEST_CASE(
    "Filesystem state image restores a mixed interaction publication and subscription for multiple federates",
    "[unit][kernel][federation-registry][save-restore][durable-save][filesystem][restore]"
    "[process-restart][process-restart-interaction-declaration][interaction-declaration-state]"
    "[interaction-management][declaration-management][multi-federate-callback-ordering]") {
  auto const directory = temporarySaveCommitDirectory();
  std::error_code ignored;
  std::filesystem::remove_all(directory, ignored);
  auto store = std::make_shared<umbra::detail::FilesystemFederationSaveCommitStore>(
      directory);

  EmbeddedFederationRegistry source({}, store);
  REQUIRE(source.create(L"exercise", composedRestaurantDefinition()).status ==
      FederationRegistryStatus::applied);
  auto publisher = source.join(
      L"exercise", L"publisher", L"publisher", noOpCallbackRoute());
  auto subscriber = source.join(
      L"exercise", L"subscriber", L"subscriber", noOpCallbackRoute());
  REQUIRE(publisher.status == FederationRegistryStatus::applied);
  REQUIRE(subscriber.status == FederationRegistryStatus::applied);
  REQUIRE(publisher.membership);
  REQUIRE(subscriber.membership);
  auto const publisherId = publisher.membership->id;
  auto const subscriberId = subscriber.membership->id;

  auto const publishedClass = source.interactionClassHandleFor(
      L"exercise", "HLAinteractionRoot.CustomerTransactions.CustomerSeated");
  auto const subscribedClass = source.interactionClassHandleFor(
      L"exercise", "HLAinteractionRoot.ServerAction.TakeOrder");
  REQUIRE(publishedClass.has_value());
  REQUIRE(subscribedClass.has_value());
  REQUIRE(source.setInteractionClassPublication(
      L"exercise", publisherId, *publishedClass, true) ==
      umbra::detail::InteractionClassDeclarationStatus::applied);
  REQUIRE(source.setInteractionClassSubscription(
      L"exercise", subscriberId, *subscribedClass, true) ==
      umbra::detail::InteractionClassDeclarationStatus::applied);

  std::wstring const saveLabel = L"interaction-mixed-multi-federate-process-restart";
  REQUIRE(source.requestFederationSave(
      L"exercise", publisherId, saveLabel).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(source.federateSaveBegun(
      L"exercise", publisherId).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(source.federateSaveBegun(
      L"exercise", subscriberId).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE_FALSE(source.federateSaveComplete(
      L"exercise", publisherId).saveCompletedSuccessfully);
  REQUIRE(source.federateSaveComplete(
      L"exercise", subscriberId).saveCompletedSuccessfully);

  auto committed = store->load(L"exercise", saveLabel);
  REQUIRE(committed.has_value());
  auto image = umbra::detail::FederationStateImageCodec::decode(
      committed->stateImage);
  REQUIRE(image.interactionDeclarationCount == 2U);
  REQUIRE(image.interactionDeclarations.size() == 2U);
  REQUIRE(image.interactionDeclarations[0].federateId == publisherId);
  REQUIRE(image.interactionDeclarations[1].federateId == subscriberId);
  REQUIRE(image.interactionDeclarations[0].publishedInteractionClasses ==
      std::vector<std::uint64_t>{*publishedClass});
  REQUIRE(image.interactionDeclarations[0].subscribedInteractionClasses.empty());
  REQUIRE(image.interactionDeclarations[1].publishedInteractionClasses.empty());
  REQUIRE(image.interactionDeclarations[1].subscribedInteractionClasses.size() == 1U);
  REQUIRE(image.interactionDeclarations[1].subscribedInteractionClasses.front().
      interactionClassHandle == *subscribedClass);
  REQUIRE(image.interactionDeclarations[1].subscribedInteractionClasses.front().active);
  REQUIRE(image.objects.empty());
  REQUIRE(image.tsoInteractionMessages.empty());
  REQUIRE(image.tsoAttributeUpdateMessages.empty());
  REQUIRE(image.tsoObjectDeletionMessages.empty());
  REQUIRE(image.tsoDirectedInteractionMessages.empty());
  REQUIRE(image.tsoRequestRetractionRecords.empty());
  REQUIRE(image.tsoQueueEntries.empty());

  EmbeddedFederationRegistry restarted({}, store);
  REQUIRE(restarted.create(L"exercise", composedRestaurantDefinition()).status ==
      FederationRegistryStatus::applied);
  auto restartedPublisher = restarted.join(
      L"exercise", L"publisher", L"publisher", noOpCallbackRoute());
  auto restartedSubscriber = restarted.join(
      L"exercise", L"subscriber", L"subscriber", noOpCallbackRoute());
  REQUIRE(restartedPublisher.status == FederationRegistryStatus::applied);
  REQUIRE(restartedSubscriber.status == FederationRegistryStatus::applied);
  REQUIRE(restartedPublisher.membership);
  REQUIRE(restartedSubscriber.membership);
  REQUIRE(restartedPublisher.membership->id == publisherId);
  REQUIRE(restartedSubscriber.membership->id == subscriberId);
  REQUIRE(restarted.requestFederationRestore(
      L"exercise", publisherId, saveLabel).status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  REQUIRE(restarted.federateRestoreComplete(
      L"exercise", publisherId).status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  auto restored = restarted.federateRestoreComplete(
      L"exercise", subscriberId);
  REQUIRE(restored.status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  REQUIRE(restored.interactionTransportationTypeChangeWorkItems.empty());

  auto restoredPublisherDeclaration = restarted.interactionClassDeclarationFor(
      L"exercise", publisherId, *publishedClass);
  REQUIRE(restoredPublisherDeclaration.has_value());
  REQUIRE(restoredPublisherDeclaration->published);
  REQUIRE_FALSE(restoredPublisherDeclaration->subscriptionActive.has_value());
  auto restoredSubscriberDeclaration = restarted.interactionClassDeclarationFor(
      L"exercise", subscriberId, *subscribedClass);
  REQUIRE(restoredSubscriberDeclaration.has_value());
  REQUIRE_FALSE(restoredSubscriberDeclaration->published);
  REQUIRE(restoredSubscriberDeclaration->subscriptionActive.has_value());
  REQUIRE(*restoredSubscriberDeclaration->subscriptionActive);

  auto publisherOtherClass = restarted.interactionClassDeclarationFor(
      L"exercise", publisherId, *subscribedClass);
  auto subscriberOtherClass = restarted.interactionClassDeclarationFor(
      L"exercise", subscriberId, *publishedClass);
  REQUIRE(publisherOtherClass.has_value());
  REQUIRE_FALSE(publisherOtherClass->published);
  REQUIRE_FALSE(publisherOtherClass->subscriptionActive.has_value());
  REQUIRE(subscriberOtherClass.has_value());
  REQUIRE_FALSE(subscriberOtherClass->published);
  REQUIRE_FALSE(subscriberOtherClass->subscriptionActive.has_value());

  std::filesystem::remove_all(directory, ignored);
}

TEST_CASE(
    "Filesystem state image restores a mixed interaction override with a subscription for multiple federates",
    "[unit][kernel][federation-registry][save-restore][durable-save][filesystem][restore]"
    "[process-restart][process-restart-interaction-mixed-override][interaction-declaration-state]"
    "[interaction-transportation-type-change][transportation-management]"
    "[interaction-management][declaration-management][multi-federate-callback-ordering]") {
  auto const directory = temporarySaveCommitDirectory();
  std::error_code ignored;
  std::filesystem::remove_all(directory, ignored);
  auto store = std::make_shared<umbra::detail::FilesystemFederationSaveCommitStore>(
      directory);

  EmbeddedFederationRegistry source({}, store);
  REQUIRE(source.create(L"exercise", composedRestaurantDefinition()).status ==
      FederationRegistryStatus::applied);
  auto publisher = source.join(
      L"exercise", L"publisher", L"publisher", noOpCallbackRoute());
  auto subscriber = source.join(
      L"exercise", L"subscriber", L"subscriber", noOpCallbackRoute());
  REQUIRE(publisher.status == FederationRegistryStatus::applied);
  REQUIRE(subscriber.status == FederationRegistryStatus::applied);
  REQUIRE(publisher.membership);
  REQUIRE(subscriber.membership);
  auto const publisherId = publisher.membership->id;
  auto const subscriberId = subscriber.membership->id;

  auto const publishedClass = source.interactionClassHandleFor(
      L"exercise", "HLAinteractionRoot.CustomerTransactions.CustomerSeated");
  auto const subscribedClass = source.interactionClassHandleFor(
      L"exercise", "HLAinteractionRoot.ServerAction.TakeOrder");
  REQUIRE(publishedClass.has_value());
  REQUIRE(subscribedClass.has_value());
  REQUIRE(source.setInteractionClassPublication(
      L"exercise", publisherId, *publishedClass, true) ==
      umbra::detail::InteractionClassDeclarationStatus::applied);
  REQUIRE(source.setInteractionClassSubscription(
      L"exercise", subscriberId, *subscribedClass, true) ==
      umbra::detail::InteractionClassDeclarationStatus::applied);
  REQUIRE(source.planInteractionTransportationTypeChange(
      L"exercise", publisherId, *publishedClass, "HLAbestEffort").status ==
      umbra::detail::InteractionTransportationTypeChangeStatus::applied);
  auto committedChange = source.beginInteractionTransportationTypeChange(
      L"exercise", publisherId, *publishedClass);
  REQUIRE(committedChange.has_value());
  REQUIRE(*committedChange == "HLAbestEffort");

  std::wstring const saveLabel = L"interaction-mixed-override-process-restart";
  REQUIRE(source.requestFederationSave(
      L"exercise", publisherId, saveLabel).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(source.federateSaveBegun(
      L"exercise", publisherId).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(source.federateSaveBegun(
      L"exercise", subscriberId).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE_FALSE(source.federateSaveComplete(
      L"exercise", publisherId).saveCompletedSuccessfully);
  REQUIRE(source.federateSaveComplete(
      L"exercise", subscriberId).saveCompletedSuccessfully);

  auto committed = store->load(L"exercise", saveLabel);
  REQUIRE(committed.has_value());
  auto image = umbra::detail::FederationStateImageCodec::decode(
      committed->stateImage);
  REQUIRE(image.interactionDeclarationCount == 2U);
  REQUIRE(image.interactionDeclarations.size() == 2U);
  REQUIRE(image.interactionDeclarations[0].federateId == publisherId);
  REQUIRE(image.interactionDeclarations[1].federateId == subscriberId);
  REQUIRE(image.interactionDeclarations[0].publishedInteractionClasses ==
      std::vector<std::uint64_t>{*publishedClass});
  REQUIRE(image.interactionDeclarations[0].interactionTransportationTypes.size() == 1U);
  REQUIRE(image.interactionDeclarations[0].interactionTransportationTypes.front().
      interactionClassHandle == *publishedClass);
  REQUIRE(image.interactionDeclarations[0].interactionTransportationTypes.front().value ==
      "HLAbestEffort");
  REQUIRE(image.interactionDeclarations[1].publishedInteractionClasses.empty());
  REQUIRE(image.interactionDeclarations[1].subscribedInteractionClasses.size() == 1U);
  REQUIRE(image.interactionDeclarations[1].subscribedInteractionClasses.front().
      interactionClassHandle == *subscribedClass);
  REQUIRE(image.interactionDeclarations[1].subscribedInteractionClasses.front().active);

  EmbeddedFederationRegistry restarted({}, store);
  REQUIRE(restarted.create(L"exercise", composedRestaurantDefinition()).status ==
      FederationRegistryStatus::applied);
  auto restartedPublisher = restarted.join(
      L"exercise", L"publisher", L"publisher", noOpCallbackRoute());
  auto restartedSubscriber = restarted.join(
      L"exercise", L"subscriber", L"subscriber", noOpCallbackRoute());
  REQUIRE(restartedPublisher.status == FederationRegistryStatus::applied);
  REQUIRE(restartedSubscriber.status == FederationRegistryStatus::applied);
  REQUIRE(restartedPublisher.membership);
  REQUIRE(restartedSubscriber.membership);
  REQUIRE(restartedPublisher.membership->id == publisherId);
  REQUIRE(restartedSubscriber.membership->id == subscriberId);
  REQUIRE(restarted.requestFederationRestore(
      L"exercise", publisherId, saveLabel).status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  REQUIRE(restarted.federateRestoreComplete(
      L"exercise", publisherId).status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  auto restored = restarted.federateRestoreComplete(
      L"exercise", subscriberId);
  REQUIRE(restored.status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  REQUIRE(restored.interactionTransportationTypeChangeWorkItems.empty());

  auto restoredQuery = restarted.interactionTransportationTypeQueryFor(
      L"exercise", publisherId, publisherId, *publishedClass);
  REQUIRE(restoredQuery.has_value());
  REQUIRE(restoredQuery->transportationName == "HLAbestEffort");
  auto restoredSubscriberDeclaration = restarted.interactionClassDeclarationFor(
      L"exercise", subscriberId, *subscribedClass);
  REQUIRE(restoredSubscriberDeclaration.has_value());
  REQUIRE_FALSE(restoredSubscriberDeclaration->published);
  REQUIRE(restoredSubscriberDeclaration->subscriptionActive.has_value());
  REQUIRE(*restoredSubscriberDeclaration->subscriptionActive);

  std::filesystem::remove_all(directory, ignored);
}

TEST_CASE("The embedded federation registry preserves a prevalidated definition", "[unit][kernel][federation-registry][foundation][federation-management]") {
  EmbeddedFederationRegistry registry;

  REQUIRE(registry.create(L"exercise", validDefinition()).status == FederationRegistryStatus::applied);
  REQUIRE(registry.contains(L"exercise"));

  auto definition = registry.definitionFor(L"exercise");
  REQUIRE(definition.has_value());
  REQUIRE(definition->logicalTimeImplementationName == L"HLAinteger64Time");
  REQUIRE(definition->fomModules.size() == 2);
  REQUIRE(definition->fomModules.front().designator == L"file:///fom/base.xml");
  REQUIRE(definition->fomModules.front().schemaDesignator == L"IEEE1516-DIF-2025.xsd");
}

TEST_CASE(
    "The embedded federation registry exposes internal operation timing",
    "[unit][kernel][federation-registry][instrumentation][foundation][federation-management]") {
  auto instrumentation = std::make_shared<umbra::detail::RuntimeInstrumentation>();
  EmbeddedFederationRegistry registry(instrumentation);

  REQUIRE(registry.create(L"instrumented", validDefinition()).status ==
      FederationRegistryStatus::applied);

  auto const snapshot = registry.runtimeInstrumentationSnapshotForTesting();
  auto const found = std::find_if(
      snapshot.operations.begin(),
      snapshot.operations.end(),
      [](umbra::detail::InstrumentationOperationSnapshot const& operation) {
        return operation.layer == InstrumentationLayer::federation_registry &&
            operation.name == "create";
      });
  REQUIRE(found != snapshot.operations.end());
  REQUIRE(found->calls == 1);
  REQUIRE(found->totalDurationNanoseconds > 0);
}

TEST_CASE("The embedded federation registry rejects missing definitions without imposing name policy", "[unit][kernel][federation-registry][foundation][federation-management]") {
  EmbeddedFederationRegistry registry;
  FederationDefinition noModules;
  FederationDefinition repeatedDesignators{
      {
          module(L"file:///fom/base.xml", L"C:/fom/base.xml"),
          module(L"file:///fom/base.xml", L"C:/fom/base.xml"),
      },
      L"",
  };

  REQUIRE(registry.create(L"", validDefinition()).status == FederationRegistryStatus::applied);
  REQUIRE(registry.create(L"no-modules", noModules).status == FederationRegistryStatus::invalid_request);
  REQUIRE(
      registry.create(L"repeated-module-designators", repeatedDesignators).status ==
      FederationRegistryStatus::applied);
  REQUIRE(registry.create(L"exercise", validDefinition()).status == FederationRegistryStatus::applied);
  REQUIRE(
      registry.create(L"exercise", validDefinition()).status ==
      FederationRegistryStatus::federation_already_exists);

  auto explicitEmptyName = registry.join(L"exercise", L"", L"");
  REQUIRE(explicitEmptyName.status == FederationRegistryStatus::applied);
  REQUIRE(explicitEmptyName.membership.has_value());
  REQUIRE(explicitEmptyName.membership->name.empty());
  REQUIRE(explicitEmptyName.membership->type.empty());
}

TEST_CASE("The embedded federation registry maintains active membership and destroy invariants", "[unit][kernel][federation-registry][foundation][federation-management]") {
  EmbeddedFederationRegistry registry;
  REQUIRE(registry.create(L"exercise", validDefinition()).status == FederationRegistryStatus::applied);

  auto named = registry.join(L"exercise", L"trainer", L"alice");
  REQUIRE(named.status == FederationRegistryStatus::applied);
  REQUIRE(named.membership.has_value());
  REQUIRE(named.membership->name == L"alice");
  REQUIRE(named.membership->id != 0);

  auto generated = registry.join(L"exercise", L"observer");
  REQUIRE(generated.status == FederationRegistryStatus::applied);
  REQUIRE(generated.membership.has_value());
  REQUIRE(generated.membership->name == L"federate-2");
  REQUIRE(generated.membership->id != named.membership->id);
  REQUIRE(registry.memberCount(L"exercise") == 2);

  auto namedByName = registry.memberByName(L"exercise", L"alice");
  REQUIRE(namedByName.has_value());
  REQUIRE(namedByName->id == named.membership->id);
  auto generatedById = registry.memberById(L"exercise", generated.membership->id);
  REQUIRE(generatedById.has_value());
  REQUIRE(generatedById->name == generated.membership->name);
  REQUIRE_FALSE(registry.memberByName(L"exercise", L"missing").has_value());
  REQUIRE_FALSE(registry.memberById(L"missing", named.membership->id).has_value());

  REQUIRE(
      registry.join(L"exercise", L"trainer", L"alice").status ==
      FederationRegistryStatus::federate_name_already_in_use);
  REQUIRE(
      registry.destroy(L"exercise").status == FederationRegistryStatus::federates_currently_joined);

  REQUIRE(
      registry.resign(L"exercise", named.membership->id).status == FederationRegistryStatus::applied);
  REQUIRE_FALSE(registry.memberByName(L"exercise", L"alice").has_value());
  REQUIRE_FALSE(registry.memberById(L"exercise", named.membership->id).has_value());
  REQUIRE(
      registry.resign(L"exercise", generated.membership->id).status == FederationRegistryStatus::applied);
  REQUIRE(registry.memberCount(L"exercise") == 0);
  REQUIRE(registry.destroy(L"exercise").status == FederationRegistryStatus::applied);
  REQUIRE_FALSE(registry.contains(L"exercise"));
}

TEST_CASE(
    "The registry retains captured regional attribute TSO snapshots instead of rereading live regions",
    "[unit][kernel][federation-registry][tso][ddm][timestamped-regional-attribute-update]") {
  EmbeddedFederationRegistry registry;
  REQUIRE(registry.create(L"exercise", validDefinition()).status ==
      FederationRegistryStatus::applied);
  auto producer = registry.join(L"exercise", L"producer", L"producer");
  auto receiver = registry.join(L"exercise", L"receiver", L"receiver");
  REQUIRE(producer.membership.has_value());
  REQUIRE(receiver.membership.has_value());

  // The opaque source handle is deliberately absent from the live registry.
  // A timestamped service has already accepted its committed specification,
  // so queue admission must retain the supplied invocation snapshot rather
  // than attempting a second lookup against mutable federation state.
  umbra::detail::TsoAttributeUpdateMessage message;
  message.producingFederateId = producer.membership->id;
  message.objectInstanceHandle = 91;
  message.attributes.emplace_back(1, rti1516_2025::VariableLengthData{});
  message.timestamp = std::make_shared<rti1516_2025::HLAinteger64Time>(7);

  umbra::detail::TsoAttributeUpdatePassel passel;
  passel.sentAttributeHandles = {1};
  passel.sentRegionHandles = {42};
  passel.sentRegionSnapshots.emplace(
      42,
      umbra::detail::RegionSpecificationSnapshot{
          {17},
          {{17, umbra::detail::RegionRangeBounds{0, 5}}},
          true});
  message.sentRegionSnapshots = passel.sentRegionSnapshots;
  message.passelsByRecipient.emplace(
      receiver.membership->id,
      std::vector<umbra::detail::TsoAttributeUpdatePassel>{passel});

  auto const enqueued = registry.enqueueTsoAttributeUpdate(
      L"exercise",
      std::move(message),
      {receiver.membership->id},
      {receiver.membership->id});
  REQUIRE(enqueued.status == umbra::detail::FederationTsoRegistryStatus::applied);
  REQUIRE(enqueued.queueStatus == umbra::detail::TsoMessageQueueStatus::applied);
  REQUIRE(enqueued.messageId != 0);

  auto const delivery = registry.beginTsoPayloadDelivery(
      L"exercise",
      receiver.membership->id,
      rti1516_2025::HLAinteger64Time(7),
      true);
  REQUIRE(delivery.status == umbra::detail::FederationTsoRegistryStatus::applied);
  REQUIRE(delivery.deliveryStatus == umbra::detail::FederationTsoDeliveryStatus::applied);
  REQUIRE(delivery.deliveries.size() == 1);
  auto const* attributeDelivery =
      std::get_if<umbra::detail::TsoAttributeUpdateDelivery>(&delivery.deliveries.front());
  REQUIRE(attributeDelivery != nullptr);
  REQUIRE(attributeDelivery->message.sentRegionSnapshots.size() == 1);
  REQUIRE(attributeDelivery->message.sentRegionSnapshots.contains(42));
  auto const& snapshot = attributeDelivery->message.sentRegionSnapshots.at(42);
  REQUIRE(snapshot.dimensionHandles == std::set<std::uint64_t>{17});
  REQUIRE(snapshot.committedRangeBounds.at(17).lowerBound == 0);
  REQUIRE(snapshot.committedRangeBounds.at(17).upperBound == 5);
}

TEST_CASE("The embedded federation registry makes generated names unique despite user lookalikes", "[unit][kernel][federation-registry][foundation][federation-management]") {
  EmbeddedFederationRegistry registry;
  REQUIRE(registry.create(L"exercise", validDefinition()).status == FederationRegistryStatus::applied);

  auto lookalike = registry.join(L"exercise", L"trainer", L"federate-1");
  REQUIRE(lookalike.status == FederationRegistryStatus::applied);
  auto generated = registry.join(L"exercise", L"observer");
  REQUIRE(generated.status == FederationRegistryStatus::applied);
  REQUIRE(generated.membership.has_value());
  REQUIRE(generated.membership->name == L"federate-2");
  REQUIRE(generated.membership->id == 2);
}

TEST_CASE("The embedded federation registry commits an additional-module definition with membership", "[unit][kernel][federation-registry][foundation][federation-management]") {
  EmbeddedFederationRegistry registry;
  auto original = validDefinition();
  auto replacement = validDefinition();
  replacement.logicalTimeImplementationName = L"HLAfloat64Time";

  REQUIRE(registry.create(L"exercise", original).status == FederationRegistryStatus::applied);
  auto joined = registry.joinWithDefinition(L"exercise", replacement, L"observer", L"bob");
  REQUIRE(joined.status == FederationRegistryStatus::applied);
  REQUIRE(joined.membership.has_value());
  REQUIRE(registry.memberCount(L"exercise") == 1);
  auto afterJoin = registry.definitionFor(L"exercise");
  REQUIRE(afterJoin.has_value());
  REQUIRE(afterJoin->logicalTimeImplementationName == L"HLAfloat64Time");

  auto incompatibleReplacement = validDefinition();
  incompatibleReplacement.logicalTimeImplementationName = L"HLAinteger64Time";
  auto rejected = registry.joinWithDefinition(
      L"exercise",
      incompatibleReplacement,
      L"observer",
      L"carol");
  REQUIRE(rejected.status == FederationRegistryStatus::invalid_request);
  REQUIRE(registry.memberCount(L"exercise") == 1);
  auto afterRejectedJoin = registry.definitionFor(L"exercise");
  REQUIRE(afterRejectedJoin.has_value());
  REQUIRE(afterRejectedJoin->logicalTimeImplementationName == L"HLAfloat64Time");

  auto duplicate = registry.joinWithDefinition(
      L"exercise",
      replacement,
      L"observer",
      L"bob");
  REQUIRE(duplicate.status == FederationRegistryStatus::federate_name_already_in_use);
}

TEST_CASE("The embedded federation registry reports missing federation and membership distinctly", "[unit][kernel][federation-registry][foundation][federation-management]") {
  EmbeddedFederationRegistry registry;

  REQUIRE(
      registry.join(L"missing", L"trainer").status ==
      FederationRegistryStatus::federation_does_not_exist);
  REQUIRE(
      registry.destroy(L"missing").status == FederationRegistryStatus::federation_does_not_exist);

  REQUIRE(registry.create(L"exercise", validDefinition()).status == FederationRegistryStatus::applied);
  REQUIRE(
      registry.resign(L"exercise", 42).status == FederationRegistryStatus::federate_not_member);
}

TEST_CASE(
    "The registry commits runtime time state with its federate membership",
    "[unit][kernel][federation-registry][time-management]") {
  EmbeddedFederationRegistry registry;
  REQUIRE(registry.create(L"exercise", validDefinition()).status == FederationRegistryStatus::applied);

  auto timeState = std::make_shared<umbra::detail::FederateTimeState>(
      L"HLAinteger64Time",
      std::make_shared<rti1516_2025::HLAinteger64Time>());
  auto joined = registry.joinWithTimeState(L"exercise", timeState, L"trainer", L"alice");
  REQUIRE(joined.status == FederationRegistryStatus::applied);
  REQUIRE(joined.membership.has_value());

  auto advance = timeState->requestAdvance(std::make_shared<rti1516_2025::HLAinteger64Time>(3));
  REQUIRE(advance.generation != 0);
  timeState.reset();

  auto snapshot = registry.timeSnapshotFor(L"exercise");
  REQUIRE(snapshot.has_value());
  REQUIRE(snapshot->definition.logicalTimeImplementationName == L"HLAinteger64Time");
  REQUIRE(snapshot->federates.size() == 1);
  REQUIRE(snapshot->federates.front().membership.id == joined.membership->id);
  REQUIRE(snapshot->federates.front().membership.name == L"alice");
  REQUIRE(snapshot->federates.front().time.timeAdvancePending);
  REQUIRE(snapshot->federates.front().time.requestedTime);

  REQUIRE(
      registry.resign(L"exercise", joined.membership->id).status ==
      FederationRegistryStatus::applied);
  snapshot = registry.timeSnapshotFor(L"exercise");
  REQUIRE(snapshot.has_value());
  REQUIRE(snapshot->federates.empty());
}

TEST_CASE(
    "The registry projects private TSO coordination into its time snapshot",
    "[unit][kernel][federation-registry][time-management][tso][lits]") {
  EmbeddedFederationRegistry registry;
  REQUIRE(registry.create(L"exercise", validDefinition()).status == FederationRegistryStatus::applied);

  auto receiverTime = std::make_shared<umbra::detail::FederateTimeState>(
      L"HLAinteger64Time",
      std::make_shared<rti1516_2025::HLAinteger64Time>());
  auto observerTime = std::make_shared<umbra::detail::FederateTimeState>(
      L"HLAinteger64Time",
      std::make_shared<rti1516_2025::HLAinteger64Time>());
  auto receiver = registry.joinWithTimeState(L"exercise", receiverTime, L"receiver", L"receiver");
  auto observer = registry.joinWithTimeState(L"exercise", observerTime, L"observer", L"observer");
  REQUIRE(receiver.status == FederationRegistryStatus::applied);
  REQUIRE(observer.status == FederationRegistryStatus::applied);
  REQUIRE(receiver.membership);
  REQUIRE(observer.membership);

  auto const messageId = registry.allocateTsoMessageId(L"exercise");
  REQUIRE(messageId.status == umbra::detail::FederationTsoRegistryStatus::applied);
  REQUIRE(messageId.messageId != 0);
  auto const timestamp = std::make_shared<rti1516_2025::HLAinteger64Time>(5);
  REQUIRE(
      registry.enqueueTsoMessage(
          L"exercise",
          messageId.messageId,
          receiver.membership->id,
          timestamp)
          .queueStatus == umbra::detail::TsoMessageQueueStatus::applied);
  REQUIRE(
      registry.enqueueTsoMessage(
          L"exercise",
          messageId.messageId,
          observer.membership->id,
          timestamp)
          .queueStatus == umbra::detail::TsoMessageQueueStatus::applied);

  auto snapshot = registry.timeSnapshotFor(L"exercise");
  REQUIRE(snapshot.has_value());
  REQUIRE(snapshot->federates.size() == 2);
  REQUIRE(snapshot->federates[0].queuedTsoMessages.size() == 1);
  REQUIRE(snapshot->federates[0].inTransitTsoMessages.empty());
  REQUIRE(snapshot->federates[0].deliveredTsoMessagesSinceLastAdvance.empty());

  auto delivery = registry.beginTsoDelivery(
      L"exercise",
      receiver.membership->id,
      rti1516_2025::HLAinteger64Time(5),
      true);
  REQUIRE(delivery.status == umbra::detail::FederationTsoRegistryStatus::applied);
  REQUIRE(delivery.delivery.status == umbra::detail::FederationTsoDeliveryStatus::applied);
  REQUIRE(delivery.delivery.messages.size() == 1);

  snapshot = registry.timeSnapshotFor(L"exercise");
  REQUIRE(snapshot.has_value());
  REQUIRE(snapshot->federates[0].queuedTsoMessages.empty());
  REQUIRE(snapshot->federates[0].inTransitTsoMessages.size() == 1);
  REQUIRE(snapshot->federates[1].queuedTsoMessages.size() == 1);

  auto completed = registry.completeTsoDelivery(L"exercise", delivery.delivery.messages.front());
  REQUIRE(completed.status == umbra::detail::FederationTsoRegistryStatus::applied);
  REQUIRE(completed.delivery.status == umbra::detail::FederationTsoDeliveryStatus::applied);
  snapshot = registry.timeSnapshotFor(L"exercise");
  REQUIRE(snapshot.has_value());
  REQUIRE(snapshot->federates[0].inTransitTsoMessages.empty());
  REQUIRE(snapshot->federates[0].deliveredTsoMessagesSinceLastAdvance.size() == 1);

  auto const retraction = registry.retractTsoMessage(L"exercise", messageId.messageId);
  REQUIRE(retraction.status == umbra::detail::FederationTsoRegistryStatus::applied);
  REQUIRE(retraction.queueResult.status == umbra::detail::TsoMessageQueueStatus::message_already_delivered);

  REQUIRE(
      registry.resign(L"exercise", receiver.membership->id).status ==
      FederationRegistryStatus::applied);
  snapshot = registry.timeSnapshotFor(L"exercise");
  REQUIRE(snapshot.has_value());
  REQUIRE(snapshot->federates.size() == 1);
  REQUIRE(snapshot->federates.front().membership.id == observer.membership->id);
  REQUIRE(snapshot->federates.front().queuedTsoMessages.size() == 1);
}

TEST_CASE(
    "The TSO recipient ledger makes a suppressed callback terminal",
    "[unit][kernel][federation-registry][time-management][tso][request-retraction]") {
  EmbeddedFederationRegistry registry;
  REQUIRE(registry.create(L"exercise", validDefinition()).status ==
      FederationRegistryStatus::applied);

  auto producer = registry.join(L"exercise", L"producer", L"producer");
  umbra::detail::InteractionCallbackRoute receiverRoute;
  receiverRoute.submit = [](umbra::detail::FederateCallbackInvocation) {};
  auto receiver = registry.join(
      L"exercise",
      L"receiver",
      L"receiver",
      std::move(receiverRoute));
  REQUIRE(producer.membership);
  REQUIRE(receiver.membership);

  umbra::detail::TsoInteractionMessage message;
  message.producingFederateId = producer.membership->id;
  message.sentInteractionClassHandle = 1;
  message.timestamp = std::make_shared<rti1516_2025::HLAinteger64Time>(7);
  auto const enqueued = registry.enqueueTsoInteraction(
      L"exercise",
      std::move(message),
      {receiver.membership->id},
      {receiver.membership->id});
  REQUIRE(enqueued.status == umbra::detail::FederationTsoRegistryStatus::applied);
  REQUIRE(enqueued.queueStatus == umbra::detail::TsoMessageQueueStatus::applied);
  REQUIRE(enqueued.messageId != 0);

  // A callback can be invalidated after TSO admission (for example, by a
  // late projection failure or a malformed private payload).  The delivery
  // path must close that recipient as suppressed rather than leave it
  // pending or report a Request Retraction for a callback that never began.
  REQUIRE(registry.finishTsoRecipientCallbackSuppressed(
      L"exercise",
      receiver.membership->id,
      enqueued.messageId));
  REQUIRE_FALSE(registry.beginTsoInteractionCallback(
      L"exercise",
      receiver.membership->id,
      enqueued.messageId));

  auto const retracted = registry.retractTsoMessageForProducer(
      L"exercise",
      producer.membership->id,
      enqueued.messageId,
      std::make_shared<rti1516_2025::HLAinteger64Time>(0));
  REQUIRE(retracted.status == umbra::detail::FederationTsoRegistryStatus::applied);
  REQUIRE(retracted.queueResult.status == umbra::detail::TsoMessageQueueStatus::applied);
  REQUIRE(retracted.requestRetractionNotifications.empty());
  REQUIRE_FALSE(registry.canDeliverTsoRequestRetraction(
      L"exercise",
      receiver.membership->id,
      enqueued.messageId));
}

TEST_CASE(
    "The registry schedules a newly eligible constrained TAR after a regulator advances",
    "[unit][kernel][federation-registry][time-management][galt][time-advance-grant]") {
  EmbeddedFederationRegistry registry;
  REQUIRE(registry.create(L"exercise", validDefinition()).status == FederationRegistryStatus::applied);

  auto receiverTime = std::make_shared<umbra::detail::FederateTimeState>(
      L"HLAinteger64Time",
      std::make_shared<rti1516_2025::HLAinteger64Time>());
  auto regulatorTime = std::make_shared<umbra::detail::FederateTimeState>(
      L"HLAinteger64Time",
      std::make_shared<rti1516_2025::HLAinteger64Time>());
  auto receiver = registry.joinWithTimeState(L"exercise", receiverTime, L"receiver", L"receiver");
  auto regulator = registry.joinWithTimeState(L"exercise", regulatorTime, L"regulator", L"regulator");
  REQUIRE(receiver.membership);
  REQUIRE(regulator.membership);

  auto constrained = receiverTime->requestTimeConstrained();
  REQUIRE(constrained.generation != 0);
  REQUIRE(receiverTime->grantTimeConstrained(constrained.generation));
  auto regulation = regulatorTime->requestTimeRegulation(
      std::make_shared<rti1516_2025::HLAinteger64Interval>(2));
  REQUIRE(regulation.generation != 0);
  REQUIRE(regulatorTime->grantTimeRegulation(regulation.generation));

  auto receiverAdvance = receiverTime->requestAdvance(
      std::make_shared<rti1516_2025::HLAinteger64Time>(2));
  REQUIRE(receiverAdvance.generation != 0);
  std::size_t receiverDispatches = 0;
  auto waiting = registry.requestTimeAdvanceGrant(
      L"exercise",
      receiver.membership->id,
      receiverAdvance.generation,
      [&receiverDispatches] { ++receiverDispatches; });
  REQUIRE(waiting.status == FederationTimeGrantStatus::applied);
  REQUIRE(waiting.dispatches.empty());

  // The regulator's pending target 1 plus lookahead 2 raises the receiver's
  // GALT from 2 to 3, making the receiver's TAR(2) strictly below the bound
  // even before the regulator receives its own grant.
  auto regulatorAdvance = regulatorTime->requestAdvance(
      std::make_shared<rti1516_2025::HLAinteger64Time>(1));
  REQUIRE(regulatorAdvance.generation != 0);
  std::size_t regulatorDispatches = 0;
  auto eligible = registry.requestTimeAdvanceGrant(
      L"exercise",
      regulator.membership->id,
      regulatorAdvance.generation,
      [&regulatorDispatches] { ++regulatorDispatches; });
  REQUIRE(eligible.status == FederationTimeGrantStatus::applied);
  REQUIRE(eligible.dispatches.size() == 2);
  for (auto& dispatch : eligible.dispatches) {
    dispatch();
  }
  REQUIRE(receiverDispatches == 1);
  REQUIRE(regulatorDispatches == 1);

  REQUIRE(
      registry.beginTimeAdvanceGrant(
          L"exercise",
          receiver.membership->id,
          receiverAdvance.generation) == FederationTimeGrantStatus::applied);
  REQUIRE(receiverTime->grant(receiverAdvance.generation));
  REQUIRE(
      registry.beginTimeAdvanceGrant(
          L"exercise",
          regulator.membership->id,
          regulatorAdvance.generation) == FederationTimeGrantStatus::applied);
  REQUIRE(regulatorTime->grant(regulatorAdvance.generation));

  auto receiverSnapshot = receiverTime->snapshot();
  auto regulatorSnapshot = regulatorTime->snapshot();
  REQUIRE_FALSE(receiverSnapshot.timeAdvancePending);
  REQUIRE_FALSE(regulatorSnapshot.timeAdvancePending);
}

TEST_CASE(
    "Filesystem state image restores a directed interaction publication and subscription for multiple federates",
    "[unit][kernel][federation-registry][save-restore][durable-save][filesystem][restore]"
    "[process-restart][process-restart-directed-interaction-declaration][interaction-declaration-state]"
    "[directed-interaction][directed-declaration][declaration-management][multi-federate-callback-ordering]") {
  auto const directory = temporarySaveCommitDirectory();
  std::error_code ignored;
  std::filesystem::remove_all(directory, ignored);
  auto store = std::make_shared<umbra::detail::FilesystemFederationSaveCommitStore>(
      directory);

  EmbeddedFederationRegistry source({}, store);
  REQUIRE(source.create(L"exercise", composedDirectedInteractionDefinition()).status ==
      FederationRegistryStatus::applied);
  auto publisher = source.join(
      L"exercise", L"publisher", L"publisher", noOpCallbackRoute());
  auto subscriber = source.join(
      L"exercise", L"subscriber", L"subscriber", noOpCallbackRoute());
  REQUIRE(publisher.status == FederationRegistryStatus::applied);
  REQUIRE(subscriber.status == FederationRegistryStatus::applied);
  REQUIRE(publisher.membership);
  REQUIRE(subscriber.membership);
  auto const publisherId = publisher.membership->id;
  auto const subscriberId = subscriber.membership->id;

  auto const objectClass = source.objectClassHandleFor(
      L"exercise", "HLAobjectRoot.UmbraDirectedFixtureObject");
  auto const interactionClass = source.interactionClassHandleFor(
      L"exercise", "HLAinteractionRoot.UmbraDirectedFixtureInteraction");
  REQUIRE(objectClass.has_value());
  REQUIRE(interactionClass.has_value());
  std::set<std::uint64_t> directedClasses{*interactionClass};
  REQUIRE(source.publishObjectClassDirectedInteractions(
      L"exercise", publisherId, *objectClass, directedClasses) ==
      umbra::detail::DirectedInteractionDeclarationStatus::applied);
  REQUIRE(source.subscribeObjectClassDirectedInteractions(
      L"exercise", subscriberId, *objectClass, directedClasses, true) ==
      umbra::detail::DirectedInteractionDeclarationStatus::applied);

  std::wstring const saveLabel = L"directed-interaction-declaration-process-restart";
  REQUIRE(source.requestFederationSave(
      L"exercise", publisherId, saveLabel).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(source.federateSaveBegun(
      L"exercise", publisherId).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(source.federateSaveBegun(
      L"exercise", subscriberId).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE_FALSE(source.federateSaveComplete(
      L"exercise", publisherId).saveCompletedSuccessfully);
  REQUIRE(source.federateSaveComplete(
      L"exercise", subscriberId).saveCompletedSuccessfully);

  auto committed = store->load(L"exercise", saveLabel);
  REQUIRE(committed.has_value());
  auto image = umbra::detail::FederationStateImageCodec::decode(
      committed->stateImage);
  REQUIRE(image.interactionDeclarationCount == 2U);
  REQUIRE(image.interactionDeclarations.size() == 2U);
  REQUIRE(image.interactionDeclarations[0].federateId == publisherId);
  REQUIRE(image.interactionDeclarations[1].federateId == subscriberId);
  auto const& savedPublisher = image.interactionDeclarations[0];
  auto const& savedSubscriber = image.interactionDeclarations[1];
  REQUIRE(savedPublisher.publishedInteractionClasses.empty());
  REQUIRE(savedPublisher.subscribedInteractionClasses.empty());
  REQUIRE(savedPublisher.regionalSubscribedInteractionClasses.empty());
  REQUIRE(savedPublisher.publishedObjectClassDirectedInteractions.size() == 1U);
  REQUIRE(savedPublisher.publishedObjectClassDirectedInteractions.front().objectClassHandle ==
      *objectClass);
  REQUIRE(savedPublisher.publishedObjectClassDirectedInteractions.front().interactionClassHandle ==
      *interactionClass);
  REQUIRE(savedPublisher.subscribedObjectClassDirectedInteractions.empty());
  REQUIRE(savedPublisher.interactionTransportationTypes.empty());
  REQUIRE(savedPublisher.interactionOrderTypes.empty());
  REQUIRE(savedPublisher.pendingInteractionTransportationTypeChanges.empty());
  REQUIRE(savedSubscriber.publishedInteractionClasses.empty());
  REQUIRE(savedSubscriber.subscribedInteractionClasses.empty());
  REQUIRE(savedSubscriber.regionalSubscribedInteractionClasses.empty());
  REQUIRE(savedSubscriber.publishedObjectClassDirectedInteractions.empty());
  REQUIRE(savedSubscriber.subscribedObjectClassDirectedInteractions.size() == 1U);
  REQUIRE(savedSubscriber.subscribedObjectClassDirectedInteractions.front().objectClassHandle ==
      *objectClass);
  REQUIRE(savedSubscriber.subscribedObjectClassDirectedInteractions.front().interactionClassHandle ==
      *interactionClass);
  REQUIRE(savedSubscriber.subscribedObjectClassDirectedInteractions.front().active);
  REQUIRE(savedSubscriber.interactionTransportationTypes.empty());
  REQUIRE(savedSubscriber.interactionOrderTypes.empty());
  REQUIRE(savedSubscriber.pendingInteractionTransportationTypeChanges.empty());
  REQUIRE(image.objects.empty());
  REQUIRE(image.tsoInteractionMessages.empty());
  REQUIRE(image.tsoAttributeUpdateMessages.empty());
  REQUIRE(image.tsoObjectDeletionMessages.empty());
  REQUIRE(image.tsoDirectedInteractionMessages.empty());
  REQUIRE(image.tsoRequestRetractionRecords.empty());
  REQUIRE(image.tsoQueueEntries.empty());

  EmbeddedFederationRegistry restarted({}, store);
  REQUIRE(restarted.create(L"exercise", composedDirectedInteractionDefinition()).status ==
      FederationRegistryStatus::applied);
  auto restartedPublisher = restarted.join(
      L"exercise", L"publisher", L"publisher", noOpCallbackRoute());
  auto restartedSubscriber = restarted.join(
      L"exercise", L"subscriber", L"subscriber", noOpCallbackRoute());
  REQUIRE(restartedPublisher.status == FederationRegistryStatus::applied);
  REQUIRE(restartedSubscriber.status == FederationRegistryStatus::applied);
  REQUIRE(restartedPublisher.membership);
  REQUIRE(restartedSubscriber.membership);
  REQUIRE(restartedPublisher.membership->id == publisherId);
  REQUIRE(restartedSubscriber.membership->id == subscriberId);
  REQUIRE(restarted.requestFederationRestore(
      L"exercise", publisherId, saveLabel).status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  REQUIRE(restarted.federateRestoreComplete(
      L"exercise", publisherId).status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  auto restored = restarted.federateRestoreComplete(
      L"exercise", subscriberId);
  REQUIRE(restored.status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  REQUIRE(restored.interactionTransportationTypeChangeWorkItems.empty());

  // The second commit proves the restored runtime maps serialize back to the
  // same directed pair instead of merely accepting the original image.
  std::wstring const roundTripLabel =
      L"directed-interaction-declaration-round-trip";
  REQUIRE(restarted.requestFederationSave(
      L"exercise", publisherId, roundTripLabel).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(restarted.federateSaveBegun(
      L"exercise", publisherId).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(restarted.federateSaveBegun(
      L"exercise", subscriberId).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE_FALSE(restarted.federateSaveComplete(
      L"exercise", publisherId).saveCompletedSuccessfully);
  REQUIRE(restarted.federateSaveComplete(
      L"exercise", subscriberId).saveCompletedSuccessfully);
  auto roundTrip = store->load(L"exercise", roundTripLabel);
  REQUIRE(roundTrip.has_value());
  auto roundTripImage = umbra::detail::FederationStateImageCodec::decode(
      roundTrip->stateImage);
  REQUIRE(roundTripImage.interactionDeclarations.size() == 2U);
  REQUIRE(roundTripImage.interactionDeclarations[0]
              .publishedObjectClassDirectedInteractions.size() == 1U);
  REQUIRE(roundTripImage.interactionDeclarations[0]
              .publishedObjectClassDirectedInteractions.front()
              .objectClassHandle == *objectClass);
  REQUIRE(roundTripImage.interactionDeclarations[0]
              .publishedObjectClassDirectedInteractions.front()
              .interactionClassHandle == *interactionClass);
  REQUIRE(roundTripImage.interactionDeclarations[1]
              .subscribedObjectClassDirectedInteractions.size() == 1U);
  REQUIRE(roundTripImage.interactionDeclarations[1]
              .subscribedObjectClassDirectedInteractions.front()
              .objectClassHandle == *objectClass);
  REQUIRE(roundTripImage.interactionDeclarations[1]
              .subscribedObjectClassDirectedInteractions.front()
              .interactionClassHandle == *interactionClass);
  REQUIRE(roundTripImage.interactionDeclarations[1]
              .subscribedObjectClassDirectedInteractions.front()
              .active);

  std::filesystem::remove_all(directory, ignored);
}

TEST_CASE(
    "Filesystem state image restores a directed interaction target and receive-order route",
    "[unit][kernel][federation-registry][save-restore][durable-save][filesystem][restore]"
    "[process-restart][process-restart-directed-interaction-routing][interaction-declaration-state]"
    "[directed-interaction][directed-routing][object-visibility-state][object-lifecycle-state]"
    "[object-class-declaration-state][interaction-management][declaration-management]") {
  auto const directory = temporarySaveCommitDirectory();
  std::error_code ignored;
  std::filesystem::remove_all(directory, ignored);
  auto store = std::make_shared<umbra::detail::FilesystemFederationSaveCommitStore>(
      directory);

  EmbeddedFederationRegistry source({}, store);
  REQUIRE(source.create(L"exercise", composedDirectedInteractionDefinition()).status ==
      FederationRegistryStatus::applied);
  auto publisher = source.join(
      L"exercise", L"publisher", L"publisher", noOpCallbackRoute());
  auto subscriber = source.join(
      L"exercise", L"subscriber", L"subscriber", noOpCallbackRoute());
  REQUIRE(publisher.status == FederationRegistryStatus::applied);
  REQUIRE(subscriber.status == FederationRegistryStatus::applied);
  REQUIRE(publisher.membership);
  REQUIRE(subscriber.membership);
  auto const publisherId = publisher.membership->id;
  auto const subscriberId = subscriber.membership->id;

  auto const objectClass = source.objectClassHandleFor(
      L"exercise", "HLAobjectRoot.UmbraDirectedFixtureObject");
  auto const interactionClass = source.interactionClassHandleFor(
      L"exercise", "HLAinteractionRoot.UmbraDirectedFixtureInteraction");
  auto const marker = source.attributeHandleFor(
      L"exercise", "HLAobjectRoot.UmbraDirectedFixtureObject", "DirectedTargetMarker");
  REQUIRE(objectClass.has_value());
  REQUIRE(interactionClass.has_value());
  REQUIRE(marker.has_value());

  std::set<std::uint64_t> const markerOnly{*marker};
  std::set<std::uint64_t> const directedOnly{*interactionClass};
  REQUIRE(source.setObjectClassAttributePublication(
      L"exercise", publisherId, *objectClass, markerOnly, true) ==
      umbra::detail::ObjectClassAttributeDeclarationStatus::applied);
  REQUIRE(source.setObjectClassAttributeSubscription(
      L"exercise", subscriberId, *objectClass, markerOnly, true) ==
      umbra::detail::ObjectClassAttributeDeclarationStatus::applied);
  REQUIRE(source.publishObjectClassDirectedInteractions(
      L"exercise", publisherId, *objectClass, directedOnly) ==
      umbra::detail::DirectedInteractionDeclarationStatus::applied);
  REQUIRE(source.subscribeObjectClassDirectedInteractions(
      L"exercise", subscriberId, *objectClass, directedOnly, true) ==
      umbra::detail::DirectedInteractionDeclarationStatus::applied);

  auto registered = source.registerObjectInstance(
      L"exercise", publisherId, *objectClass);
  REQUIRE(registered.status ==
      umbra::detail::ObjectInstanceRegistrationStatus::applied);
  REQUIRE(registered.objectInstanceHandle != 0U);
  auto discoveries = source.planObjectInstanceDiscoveriesForInstance(
      L"exercise", registered.objectInstanceHandle);
  REQUIRE(discoveries.size() == 1U);
  REQUIRE(discoveries.front().receivingFederateId == subscriberId);
  REQUIRE(source.beginObjectInstanceDiscovery(
      L"exercise", subscriberId, registered.objectInstanceHandle).has_value());

  std::string const markerValue{"directed-target", 15U};
  std::vector<std::pair<std::uint64_t, rti1516_2025::VariableLengthData>> values{{
      *marker,
      rti1516_2025::VariableLengthData(markerValue.data(), markerValue.size()),
  }};
  REQUIRE(source.recordSuccessfulUpdateAttributeValues(
      L"exercise", publisherId, registered.objectInstanceHandle, *objectClass,
      {"HLAreliable"}, &values) == FederationRegistryStatus::applied);

  std::wstring const saveLabel = L"directed-interaction-routing-process-restart";
  REQUIRE(source.requestFederationSave(
      L"exercise", publisherId, saveLabel).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(source.federateSaveBegun(
      L"exercise", publisherId).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(source.federateSaveBegun(
      L"exercise", subscriberId).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE_FALSE(source.federateSaveComplete(
      L"exercise", publisherId).saveCompletedSuccessfully);
  REQUIRE(source.federateSaveComplete(
      L"exercise", subscriberId).saveCompletedSuccessfully);

  auto committed = store->load(L"exercise", saveLabel);
  REQUIRE(committed.has_value());
  auto image = umbra::detail::FederationStateImageCodec::decode(
      committed->stateImage);
  REQUIRE(image.objects.size() == 1U);
  REQUIRE(image.objects.front().handle == registered.objectInstanceHandle);
  REQUIRE(image.objects.front().knownObjectClassHandlesByFederate.size() == 2U);
  REQUIRE(image.objects.front().attributeValuesPresent);
  REQUIRE(image.interactionDeclarations.size() == 2U);
  REQUIRE(image.interactionDeclarations[0]
              .publishedObjectClassDirectedInteractions.size() == 1U);
  REQUIRE(image.interactionDeclarations[1]
              .subscribedObjectClassDirectedInteractions.size() == 1U);

  EmbeddedFederationRegistry restarted({}, store);
  REQUIRE(restarted.create(L"exercise", composedDirectedInteractionDefinition()).status ==
      FederationRegistryStatus::applied);
  auto restartedPublisher = restarted.join(
      L"exercise", L"publisher", L"publisher", noOpCallbackRoute());
  auto restartedSubscriber = restarted.join(
      L"exercise", L"subscriber", L"subscriber", noOpCallbackRoute());
  REQUIRE(restartedPublisher.status == FederationRegistryStatus::applied);
  REQUIRE(restartedSubscriber.status == FederationRegistryStatus::applied);
  REQUIRE(restartedPublisher.membership);
  REQUIRE(restartedSubscriber.membership);
  REQUIRE(restartedPublisher.membership->id == publisherId);
  REQUIRE(restartedSubscriber.membership->id == subscriberId);
  REQUIRE(restarted.requestFederationRestore(
      L"exercise", publisherId, saveLabel).status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  REQUIRE(restarted.federateRestoreComplete(
      L"exercise", publisherId).status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  auto restored = restarted.federateRestoreComplete(
      L"exercise", subscriberId);
  REQUIRE(restored.status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  REQUIRE(restored.interactionTransportationTypeChangeWorkItems.empty());

  auto restoredPlan = restarted.planReceiveOrderDirectedInteraction(
      L"exercise", publisherId, registered.objectInstanceHandle,
      *interactionClass, {});
  REQUIRE(restoredPlan.status ==
      umbra::detail::ReceiveOrderDirectedInteractionStatus::applied);
  REQUIRE(restoredPlan.transportationName == "HLAreliable");
  REQUIRE(restoredPlan.recipients.size() == 1U);
  REQUIRE(restoredPlan.recipients.front().federateId == subscriberId);
  REQUIRE(restoredPlan.recipients.front().objectInstanceHandle ==
      registered.objectInstanceHandle);
  REQUIRE(restoredPlan.recipients.front().receivedInteractionClassHandle ==
      *interactionClass);
  REQUIRE(restoredPlan.recipients.front().callbackRoute);

  std::wstring const roundTripLabel =
      L"directed-interaction-routing-round-trip";
  REQUIRE(restarted.requestFederationSave(
      L"exercise", publisherId, roundTripLabel).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(restarted.federateSaveBegun(
      L"exercise", publisherId).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(restarted.federateSaveBegun(
      L"exercise", subscriberId).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE_FALSE(restarted.federateSaveComplete(
      L"exercise", publisherId).saveCompletedSuccessfully);
  REQUIRE(restarted.federateSaveComplete(
      L"exercise", subscriberId).saveCompletedSuccessfully);
  auto roundTrip = store->load(L"exercise", roundTripLabel);
  REQUIRE(roundTrip.has_value());
  auto roundTripImage = umbra::detail::FederationStateImageCodec::decode(
      roundTrip->stateImage);
  REQUIRE(roundTripImage.objects.size() == 1U);
  REQUIRE(roundTripImage.objects.front().handle == registered.objectInstanceHandle);
  REQUIRE(roundTripImage.objects.front().attributeValuesPresent);
  REQUIRE(roundTripImage.interactionDeclarations.size() == 2U);
  REQUIRE(roundTripImage.interactionDeclarations[0]
              .publishedObjectClassDirectedInteractions.size() == 1U);
  REQUIRE(roundTripImage.interactionDeclarations[1]
              .subscribedObjectClassDirectedInteractions.size() == 1U);
  REQUIRE(roundTripImage.interactionDeclarations[1]
              .subscribedObjectClassDirectedInteractions.front().active);

  std::filesystem::remove_all(directory, ignored);
}

TEST_CASE(
    "Federation state images preserve deferred update-region associations across a v1 codec round trip",
    "[unit][kernel][federation-registry][save-restore][durable-save][state-image]"
    "[ownership-management][ddm][deferred-update-region-association]"
    "[deferred-update-region-association-state-image][process-restart]") {
  umbra::detail::FederationStateImage image;
  image.federationName = L"exercise";
  image.logicalTimeImplementationName = L"HLAinteger64Time";
  image.members = {
      {1U, L"owner", L"trainer", 0U, 0U, 0, 0U},
      {2U, L"acquirer", L"trainer", 0U, 0U, 0, 0U},
  };
  image.regionCount = 2U;
  image.regions = {
      {90U, 2U, {}, {}, {}, false, false},
      {91U, 2U, {}, {}, {}, false, false},
  };
  image.objectInstanceCount = 1U;
  image.objects.push_back({
      42U,
      L"table-42",
      4U,
      1U,
      false,
      0U,
      {{3U, 1U, "HLAreliable", 0U, {}}},
  });
  image.deferredUpdateRegionAssociationsPresent = true;
  image.deferredUpdateRegionAssociations.push_back({42U, 2U, 3U, {91U, 90U}});

  auto const encoded = umbra::detail::FederationStateImageCodec::encode(image);
  REQUIRE(encoded.find("deferredUpdateRegionAssociations=1\n") != std::string::npos);

  auto const decoded = umbra::detail::FederationStateImageCodec::decode(encoded);
  REQUIRE(decoded.deferredUpdateRegionAssociationsPresent);
  REQUIRE(decoded.deferredUpdateRegionAssociations.size() == 1U);
  auto const& association = decoded.deferredUpdateRegionAssociations.front();
  REQUIRE(association.objectInstanceHandle == 42U);
  REQUIRE(association.federateId == 2U);
  REQUIRE(association.attributeHandle == 3U);
  REQUIRE(association.regionHandles == std::vector<std::uint64_t>{90U, 91U});
  REQUIRE(umbra::detail::FederationStateImageCodec::encode(decoded) == encoded);

  auto malformed = image;
  malformed.deferredUpdateRegionAssociations.push_back({42U, 2U, 3U, {90U}});
  REQUIRE_THROWS(umbra::detail::FederationStateImageCodec::encode(malformed));
}

TEST_CASE(
    "Filesystem fresh-registry restore promotes a deferred update-region association after ownership acquisition",
    "[integration][development-profile][federation-registry][save-restore][durable-save]"
    "[filesystem][process-restart][ownership-management][ddm]"
    "[deferred-update-region-association-state-image-restore]"
    "[rti.service.request-federation-save][rti.service.federate-save-begun]"
    "[rti.service.federate-save-complete][rti.service.request-federation-restore]"
    "[rti.service.federate-restore-complete][rti.service.associate-regions-for-updates]"
    "[rti.service.attribute-ownership-acquisition-if-available]"
    "[rti.service.attribute-ownership-divestiture-if-wanted]") {
  auto const directory = temporarySaveCommitDirectory();
  std::error_code ignored;
  std::filesystem::remove_all(directory, ignored);
  auto store = std::make_shared<umbra::detail::FilesystemFederationSaveCommitStore>(
      directory);

  auto regionalDefinition = [&] {
    std::filesystem::path const testData =
        std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data";
    std::vector<PrevalidatedFomModule> modules{
        validatedModule(
            resourcePath("mim/HLAstandardMIM-2025.xml"),
            FomModuleKind::mim,
            L"urn:umbra:test:state-image-restore-mim"),
        validatedModule(
            testData / "regional-ownership-fanout-fom.xml",
            FomModuleKind::fom,
            L"urn:umbra:test:state-image-restore-ownership"),
    };
    LibXml2FomModuleComposer composer(
        resourcePath("schemas/IEEE1516-FDD-2025.xsd"));
    auto result = composer.compose(modules);
    CAPTURE(result.diagnostics);
    REQUIRE(result.status == FomCompositionStatus::valid);
    REQUIRE(result.catalog);
    REQUIRE(result.fdd);
    return FederationDefinition{
        std::move(result.modules),
        L"HLAinteger64Time",
        std::move(result.catalog),
        std::move(result.fdd),
    };
  }();
  auto restartDefinition = regionalDefinition;

  EmbeddedFederationRegistry source({}, store);
  REQUIRE(source.create(L"exercise", std::move(regionalDefinition)).status ==
      FederationRegistryStatus::applied);
  auto owner = source.join(
      L"exercise", L"owner-type", L"owner", noOpCallbackRoute());
  auto newOwner = source.join(
      L"exercise", L"owner-type", L"new-owner", noOpCallbackRoute());
  auto receiver = source.join(
      L"exercise", L"receiver-type", L"receiver", noOpCallbackRoute());
  REQUIRE(owner.membership);
  REQUIRE(newOwner.membership);
  REQUIRE(receiver.membership);

  auto const objectClass = source.objectClassHandleFor(
      L"exercise", "HLAobjectRoot.UmbraRegionalOwnershipFanout");
  auto const attribute = source.attributeHandleFor(
      L"exercise", "HLAobjectRoot.UmbraRegionalOwnershipFanout", "ProviderBValue");
  auto const dimension = source.dimensionHandleFor(L"exercise", "UmbraRegionX");
  REQUIRE(objectClass.has_value());
  REQUIRE(attribute.has_value());
  REQUIRE(dimension.has_value());
  std::set<std::uint64_t> const attributes{*attribute};
  REQUIRE(source.setObjectClassAttributePublication(
               L"exercise", owner.membership->id, *objectClass, attributes, true) ==
      umbra::detail::ObjectClassAttributeDeclarationStatus::applied);
  REQUIRE(source.setObjectClassAttributePublication(
               L"exercise", newOwner.membership->id, *objectClass, attributes, true) ==
      umbra::detail::ObjectClassAttributeDeclarationStatus::applied);

  auto ownerRegionResult = source.createRegion(
      L"exercise", owner.membership->id, {*dimension});
  auto deferredRegionResult = source.createRegion(
      L"exercise", newOwner.membership->id, {*dimension});
  auto receiverRegionResult = source.createRegion(
      L"exercise", receiver.membership->id, {*dimension});
  REQUIRE(ownerRegionResult.status == umbra::detail::RegionServiceStatus::applied);
  REQUIRE(deferredRegionResult.status == umbra::detail::RegionServiceStatus::applied);
  REQUIRE(receiverRegionResult.status == umbra::detail::RegionServiceStatus::applied);
  auto const ownerRegion = ownerRegionResult.regionHandle;
  auto const deferredRegion = deferredRegionResult.regionHandle;
  auto const receiverRegion = receiverRegionResult.regionHandle;
  for (auto const [federateId, regionHandle] : std::vector<std::pair<std::uint64_t, std::uint64_t>>{
           {owner.membership->id, ownerRegion},
           {newOwner.membership->id, deferredRegion},
           {receiver.membership->id, receiverRegion}}) {
    REQUIRE(source.setRangeBounds(
                 L"exercise", federateId, regionHandle, *dimension,
                 umbra::detail::RegionRangeBounds{0UL, 2UL}) ==
        umbra::detail::RegionServiceStatus::applied);
    REQUIRE(source.commitRegionModifications(
                 L"exercise", federateId, {regionHandle}) ==
        umbra::detail::RegionServiceStatus::applied);
  }

  std::map<std::uint64_t, std::set<std::uint64_t>> const deferredSubscription{
      {*attribute, {deferredRegion}}};
  std::map<std::uint64_t, std::set<std::uint64_t>> const receiverSubscription{
      {*attribute, {receiverRegion}}};
  REQUIRE(source.setObjectClassAttributeRegionalSubscription(
               L"exercise", newOwner.membership->id, *objectClass,
               deferredSubscription, true) ==
      umbra::detail::RegionalObjectClassAttributeDeclarationStatus::applied);
  REQUIRE(source.setObjectClassAttributeRegionalSubscription(
               L"exercise", receiver.membership->id, *objectClass,
               receiverSubscription, true) ==
      umbra::detail::RegionalObjectClassAttributeDeclarationStatus::applied);

  std::map<std::uint64_t, std::set<std::uint64_t>> const ownerRegistration{
      {*attribute, {ownerRegion}}};
  auto registered = source.registerObjectInstance(
      L"exercise", owner.membership->id, *objectClass, &ownerRegistration);
  REQUIRE(registered.status ==
      umbra::detail::ObjectInstanceRegistrationStatus::applied);
  REQUIRE(registered.objectInstanceHandle != 0U);
  auto discoveries = source.planObjectInstanceDiscoveriesForInstance(
      L"exercise", registered.objectInstanceHandle);
  REQUIRE(discoveries.size() == 2U);
  REQUIRE(source.beginObjectInstanceDiscovery(
      L"exercise", newOwner.membership->id, registered.objectInstanceHandle));
  REQUIRE(source.beginObjectInstanceDiscovery(
      L"exercise", receiver.membership->id, registered.objectInstanceHandle));

  // Fresh-registry materialization is intentionally bounded to an object
  // whose latest application value is durable.  Seed that value through the
  // same accepted update boundary used by the process service so the restore
  // slice exercises the regional ownership ledger rather than an empty
  // registration shell.
  std::string const valueBytes{"restore"};
  std::vector<std::pair<std::uint64_t, rti1516_2025::VariableLengthData>> values{{
      *attribute,
      rti1516_2025::VariableLengthData(valueBytes.data(), valueBytes.size()),
  }};
  REQUIRE(source.recordSuccessfulUpdateAttributeValues(
      L"exercise",
      owner.membership->id,
      registered.objectInstanceHandle,
      *objectClass,
      {"HLAreliable"},
      &values) == FederationRegistryStatus::applied);

  REQUIRE(source.associateRegionsForUpdates(
               L"exercise", newOwner.membership->id,
               registered.objectInstanceHandle, deferredSubscription) ==
      umbra::detail::ObjectInstanceRegionAssociationStatus::applied);
  auto beforeTransfer = source.planReceiveOrderAttributeUpdate(
      L"exercise", owner.membership->id, registered.objectInstanceHandle, {*attribute});
  REQUIRE(beforeTransfer.status ==
      umbra::detail::ReceiveOrderAttributeUpdateStatus::applied);
  REQUIRE(beforeTransfer.passels.size() == 1U);
  REQUIRE(beforeTransfer.passels.front().sentRegionHandles ==
      std::set<std::uint64_t>{ownerRegion});

  std::wstring const saveLabel = L"deferred-update-region-association-restore";
  REQUIRE(source.requestFederationSave(
               L"exercise", owner.membership->id, saveLabel).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(source.federateSaveBegun(
               L"exercise", owner.membership->id).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(source.federateSaveBegun(
               L"exercise", newOwner.membership->id).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(source.federateSaveBegun(
               L"exercise", receiver.membership->id).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE_FALSE(source.federateSaveComplete(
      L"exercise", owner.membership->id).saveCompletedSuccessfully);
  REQUIRE_FALSE(source.federateSaveComplete(
      L"exercise", newOwner.membership->id).saveCompletedSuccessfully);
  REQUIRE(source.federateSaveComplete(
      L"exercise", receiver.membership->id).saveCompletedSuccessfully);

  auto committed = store->load(L"exercise", saveLabel);
  REQUIRE(committed.has_value());
  auto savedImage = umbra::detail::FederationStateImageCodec::decode(
      committed->stateImage);
  REQUIRE(savedImage.deferredUpdateRegionAssociationsPresent);
  REQUIRE(savedImage.deferredUpdateRegionAssociations.size() == 1U);
  auto const& savedAssociation = savedImage.deferredUpdateRegionAssociations.front();
  REQUIRE(savedAssociation.objectInstanceHandle == registered.objectInstanceHandle);
  REQUIRE(savedAssociation.federateId == newOwner.membership->id);
  REQUIRE(savedAssociation.attributeHandle == *attribute);
  REQUIRE(savedAssociation.regionHandles == std::vector<std::uint64_t>{deferredRegion});

  EmbeddedFederationRegistry restarted({}, store);
  REQUIRE(restarted.create(L"exercise", std::move(restartDefinition)).status ==
      FederationRegistryStatus::applied);
  auto restartedOwner = restarted.join(
      L"exercise", L"owner-type", L"owner", noOpCallbackRoute());
  auto restartedNewOwner = restarted.join(
      L"exercise", L"owner-type", L"new-owner", noOpCallbackRoute());
  auto restartedReceiver = restarted.join(
      L"exercise", L"receiver-type", L"receiver", noOpCallbackRoute());
  REQUIRE(restartedOwner.membership);
  REQUIRE(restartedNewOwner.membership);
  REQUIRE(restartedReceiver.membership);
  REQUIRE(restartedOwner.membership->id == owner.membership->id);
  REQUIRE(restartedNewOwner.membership->id == newOwner.membership->id);
  REQUIRE(restartedReceiver.membership->id == receiver.membership->id);

  REQUIRE(restarted.requestFederationRestore(
               L"exercise", restartedOwner.membership->id, saveLabel).status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  REQUIRE(restarted.federateRestoreComplete(
               L"exercise", restartedOwner.membership->id).status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  REQUIRE(restarted.federateRestoreComplete(
               L"exercise", restartedNewOwner.membership->id).status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  REQUIRE(restarted.federateRestoreComplete(
               L"exercise", restartedReceiver.membership->id).status ==
      umbra::detail::FederationRestoreControlStatus::applied);

  auto const restoredAttribute = restarted.attributeHandleFor(
      L"exercise", "HLAobjectRoot.UmbraRegionalOwnershipFanout", "ProviderBValue");
  REQUIRE(restoredAttribute.has_value());
  REQUIRE(*restoredAttribute == *attribute);
  auto restoredBeforeTransfer = restarted.planReceiveOrderAttributeUpdate(
      L"exercise", restartedOwner.membership->id,
      registered.objectInstanceHandle, {*restoredAttribute});
  REQUIRE(restoredBeforeTransfer.status ==
      umbra::detail::ReceiveOrderAttributeUpdateStatus::applied);
  REQUIRE(restoredBeforeTransfer.passels.size() == 1U);
  REQUIRE(restoredBeforeTransfer.passels.front().sentRegionHandles ==
      std::set<std::uint64_t>{ownerRegion});

  std::vector<unsigned char> const acquisitionTag{'r', 'e', 's', 't', 'o', 'r', 'e'};
  auto acquisition = restarted.planAttributeOwnershipAcquisitionIfAvailable(
      L"exercise", restartedNewOwner.membership->id,
      registered.objectInstanceHandle, {*restoredAttribute}, acquisitionTag);
  REQUIRE(acquisition.status ==
      umbra::detail::AttributeOwnershipAcquisitionIfAvailableStatus::applied);
  REQUIRE(acquisition.requestId != 0U);
  std::vector<unsigned char> const divestitureTag{'d', 'i', 'v', '-', 'r', 'e', 's', 't', 'o', 'r', 'e'};
  auto divestiture = restarted.planAttributeOwnershipDivestitureIfWanted(
      L"exercise", restartedOwner.membership->id,
      registered.objectInstanceHandle, {*restoredAttribute}, divestitureTag);
  REQUIRE(divestiture.status ==
      umbra::detail::AttributeOwnershipDivestitureIfWantedStatus::applied);
  REQUIRE(divestiture.divestedAttributeHandles == std::set<std::uint64_t>{*restoredAttribute});
  REQUIRE(divestiture.notifications.size() == 1U);
  auto const& notification = divestiture.notifications.front();
  auto notificationDelivery = restarted.beginAttributeOwnershipDivestitureIfWantedNotification(
      L"exercise", restartedNewOwner.membership->id,
      registered.objectInstanceHandle, notification.notificationId,
      notification.attributeHandles);
  REQUIRE(notificationDelivery.has_value());
  REQUIRE(notificationDelivery->securedAttributeHandles ==
      std::set<std::uint64_t>{*restoredAttribute});
  auto restoredOwnership = restarted.attributeOwnedByFederate(
      L"exercise", restartedNewOwner.membership->id,
      registered.objectInstanceHandle, *restoredAttribute);
  REQUIRE(restoredOwnership.status ==
      umbra::detail::AttributeOwnershipCheckStatus::applied);
  REQUIRE(restoredOwnership.ownedByRequestingFederate);

  auto afterTransfer = restarted.planReceiveOrderAttributeUpdate(
      L"exercise", restartedNewOwner.membership->id,
      registered.objectInstanceHandle, {*restoredAttribute});
  REQUIRE(afterTransfer.status ==
      umbra::detail::ReceiveOrderAttributeUpdateStatus::applied);
  REQUIRE(afterTransfer.passels.size() == 1U);
  REQUIRE(afterTransfer.passels.front().sentRegionHandles ==
      std::set<std::uint64_t>{deferredRegion});

  std::filesystem::remove_all(directory, ignored);
}

TEST_CASE(
    "Register Object Instance With Regions uses the default region and execution-wide generated names",
    "[unit][kernel][federation-registry][object-management][ddm][default-region]"
    "[object-instance-name][rti.service.register-object-instance-with-regions]"
    "[rti.service.publish-object-class-attributes][rti.service.subscribe-object-class-attributes]"
    "[federate.callback.discover-object-instance]") {
  auto store = std::make_shared<umbra::detail::MemoryFederationSaveCommitStore>();
  EmbeddedFederationRegistry registry({}, store);
  REQUIRE(registry.create(L"exercise", composedRestaurantDefinition()).status ==
      FederationRegistryStatus::applied);

  auto owner = registry.join(
      L"exercise", L"publisher", L"owner", noOpCallbackRoute());
  auto observer = registry.join(
      L"exercise", L"subscriber", L"observer", noOpCallbackRoute());
  REQUIRE(owner.membership);
  REQUIRE(observer.membership);

  auto const soda = registry.objectClassHandleFor(
      L"exercise", "HLAobjectRoot.Food.Drink.Soda");
  auto const flavor = registry.attributeHandleFor(
      L"exercise", "HLAobjectRoot.Food.Drink.Soda", "Flavor");
  auto const sodaDimension = registry.dimensionHandleFor(
      L"exercise", "SodaFlavor");
  REQUIRE(soda.has_value());
  REQUIRE(flavor.has_value());
  REQUIRE(sodaDimension.has_value());
  std::set<std::uint64_t> const flavorOnly{*flavor};
  REQUIRE(registry.setObjectClassAttributePublication(
      L"exercise", owner.membership->id, *soda, flavorOnly, true) ==
      umbra::detail::ObjectClassAttributeDeclarationStatus::applied);
  REQUIRE(registry.setObjectClassAttributeSubscription(
      L"exercise", observer.membership->id, *soda, flavorOnly, true) ==
      umbra::detail::ObjectClassAttributeDeclarationStatus::applied);

  // An empty region set in the supplied pair is the official default-region
  // form for a class attribute that has available dimensions.
  std::map<std::uint64_t, std::set<std::uint64_t>> emptyRegionPair{{*flavor, {}}};
  auto first = registry.registerObjectInstance(
      L"exercise", owner.membership->id, *soda, &emptyRegionPair);
  REQUIRE(first.status ==
      umbra::detail::ObjectInstanceRegistrationStatus::applied);
  REQUIRE(first.objectInstanceHandle != 0U);
  REQUIRE(first.objectInstanceName.starts_with(L"UmbraObjectInstance-"));

  auto firstDiscoveries = registry.planObjectInstanceDiscoveriesForInstance(
      L"exercise", first.objectInstanceHandle);
  REQUIRE(firstDiscoveries.size() == 1U);
  REQUIRE(registry.beginObjectInstanceDiscovery(
      L"exercise", observer.membership->id, first.objectInstanceHandle));
  auto firstPlan = registry.planReceiveOrderAttributeUpdate(
      L"exercise", owner.membership->id, first.objectInstanceHandle,
      {*flavor});
  REQUIRE(firstPlan.status ==
      umbra::detail::ReceiveOrderAttributeUpdateStatus::applied);
  REQUIRE(firstPlan.passels.size() == 1U);
  REQUIRE(firstPlan.passels.front().sentAttributeHandles ==
      std::vector<std::uint64_t>{*flavor});
  REQUIRE(firstPlan.passels.front().sentRegionHandles.empty());
  REQUIRE(firstPlan.passels.front().defaultRegionUsed);
  REQUIRE(firstPlan.passels.front().recipients.size() == 1U);
  REQUIRE(firstPlan.passels.front().recipients.front().federateId ==
      observer.membership->id);

  // Omitting the optional pair argument has the same default-region result,
  // and every generated name remains unique within the federation execution.
  auto second = registry.registerObjectInstance(
      L"exercise", owner.membership->id, *soda);
  REQUIRE(second.status ==
      umbra::detail::ObjectInstanceRegistrationStatus::applied);
  REQUIRE(second.objectInstanceHandle != first.objectInstanceHandle);
  REQUIRE(second.objectInstanceName.starts_with(L"UmbraObjectInstance-"));
  REQUIRE(second.objectInstanceName != first.objectInstanceName);
  auto secondPlan = registry.planReceiveOrderAttributeUpdate(
      L"exercise", owner.membership->id, second.objectInstanceHandle,
      {*flavor});
  REQUIRE(secondPlan.status ==
      umbra::detail::ReceiveOrderAttributeUpdateStatus::applied);
  REQUIRE(secondPlan.passels.size() == 1U);
  REQUIRE(secondPlan.passels.front().sentRegionHandles.empty());
  REQUIRE(secondPlan.passels.front().defaultRegionUsed);

  // An empty attribute/region-pair collection also leaves every available
  // instance attribute on the default region for this atomic service.
  std::map<std::uint64_t, std::set<std::uint64_t>> noRegionPairs;
  auto emptyCollectionRegistration = registry.registerObjectInstance(
      L"exercise", owner.membership->id, *soda, &noRegionPairs);
  REQUIRE(emptyCollectionRegistration.status ==
      umbra::detail::ObjectInstanceRegistrationStatus::applied);
  auto emptyCollectionPlan = registry.planReceiveOrderAttributeUpdate(
      L"exercise", owner.membership->id,
      emptyCollectionRegistration.objectInstanceHandle, {*flavor});
  REQUIRE(emptyCollectionPlan.status ==
      umbra::detail::ReceiveOrderAttributeUpdateStatus::applied);
  REQUIRE(emptyCollectionPlan.passels.size() == 1U);
  REQUIRE(emptyCollectionPlan.passels.front().sentRegionHandles.empty());
  REQUIRE(emptyCollectionPlan.passels.front().defaultRegionUsed);

  // A non-empty pair remains distinguishable from the default-region form.
  auto sourceRegion = registry.createRegion(
      L"exercise", owner.membership->id, {*sodaDimension});
  REQUIRE(sourceRegion.status == umbra::detail::RegionServiceStatus::applied);
  REQUIRE(registry.setRangeBounds(
      L"exercise", owner.membership->id, sourceRegion.regionHandle,
      *sodaDimension, {0UL, 1UL}) ==
      umbra::detail::RegionServiceStatus::applied);
  REQUIRE(registry.commitRegionModifications(
      L"exercise", owner.membership->id, {sourceRegion.regionHandle}) ==
      umbra::detail::RegionServiceStatus::applied);
  std::map<std::uint64_t, std::set<std::uint64_t>> explicitRegionPair{{
      *flavor, {sourceRegion.regionHandle}}};
  auto explicitRegistration = registry.registerObjectInstance(
      L"exercise", owner.membership->id, *soda, &explicitRegionPair);
  REQUIRE(explicitRegistration.status ==
      umbra::detail::ObjectInstanceRegistrationStatus::applied);
  auto explicitPlan = registry.planReceiveOrderAttributeUpdate(
      L"exercise", owner.membership->id,
      explicitRegistration.objectInstanceHandle, {*flavor});
  REQUIRE(explicitPlan.status ==
      umbra::detail::ReceiveOrderAttributeUpdateStatus::applied);
  REQUIRE(explicitPlan.passels.size() == 1U);
  REQUIRE(explicitPlan.passels.front().sentRegionHandles ==
      std::set<std::uint64_t>{sourceRegion.regionHandle});
  REQUIRE_FALSE(explicitPlan.passels.front().defaultRegionUsed);
}

TEST_CASE(
    "Filesystem fresh-registry restore seeds declaration relevance before the next mutation",
    "[unit][kernel][federation-registry][save-restore][durable-save][filesystem][restore]"
    "[process-restart][declaration-relevance-state][declaration-management][declaration-relevance-restore-baseline]") {
  auto const directory = temporarySaveCommitDirectory();
  std::error_code ignored;
  std::filesystem::remove_all(directory, ignored);
  auto store = std::make_shared<umbra::detail::FilesystemFederationSaveCommitStore>(
      directory);

  EmbeddedFederationRegistry source({}, store);
  REQUIRE(source.create(L"exercise", composedRestaurantDefinition()).status ==
      FederationRegistryStatus::applied);
  auto publisher = source.join(
      L"exercise", L"publisher", L"publisher", noOpCallbackRoute());
  auto subscriber = source.join(
      L"exercise", L"subscriber", L"subscriber", noOpCallbackRoute());
  REQUIRE(publisher.membership);
  REQUIRE(subscriber.membership);

  auto const publisherId = publisher.membership->id;
  auto const subscriberId = subscriber.membership->id;
  auto const objectClass = source.objectClassHandleFor(
      L"exercise", "HLAobjectRoot.Food.Drink.Soda");
  auto const attribute = source.attributeHandleFor(
      L"exercise", "HLAobjectRoot.Food.Drink.Soda", "Flavor");
  auto const interactionClass = source.interactionClassHandleFor(
      L"exercise", "HLAinteractionRoot.ServerAction.TakeOrder");
  REQUIRE(objectClass.has_value());
  REQUIRE(attribute.has_value());
  REQUIRE(interactionClass.has_value());

  REQUIRE(source.setObjectClassAttributePublication(
               L"exercise", publisherId, *objectClass, {*attribute}, true) ==
      umbra::detail::ObjectClassAttributeDeclarationStatus::applied);
  REQUIRE(source.setObjectClassAttributeSubscription(
               L"exercise", subscriberId, *objectClass, {*attribute}, true) ==
      umbra::detail::ObjectClassAttributeDeclarationStatus::applied);
  REQUIRE(source.setInteractionClassPublication(
               L"exercise", publisherId, *interactionClass, true) ==
      umbra::detail::InteractionClassDeclarationStatus::applied);
  REQUIRE(source.setInteractionClassSubscription(
               L"exercise", subscriberId, *interactionClass, true) ==
      umbra::detail::InteractionClassDeclarationStatus::applied);

  auto sourceBaseline = source.planDeclarationAdvisories(L"exercise");
  REQUIRE(sourceBaseline.size() == 2U);

  std::wstring const saveLabel = L"declaration-relevance-process-restart";
  REQUIRE(source.requestFederationSave(
               L"exercise", publisherId, saveLabel).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(source.federateSaveBegun(L"exercise", publisherId).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(source.federateSaveBegun(L"exercise", subscriberId).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE_FALSE(source.federateSaveComplete(
      L"exercise", publisherId).saveCompletedSuccessfully);
  REQUIRE(source.federateSaveComplete(
      L"exercise", subscriberId).saveCompletedSuccessfully);

  EmbeddedFederationRegistry restarted({}, store);
  REQUIRE(restarted.create(L"exercise", composedRestaurantDefinition()).status ==
      FederationRegistryStatus::applied);
  auto restartedPublisher = restarted.join(
      L"exercise", L"publisher", L"publisher", noOpCallbackRoute());
  auto restartedSubscriber = restarted.join(
      L"exercise", L"subscriber", L"subscriber", noOpCallbackRoute());
  REQUIRE(restartedPublisher.membership);
  REQUIRE(restartedSubscriber.membership);
  REQUIRE(restartedPublisher.membership->id == publisherId);
  REQUIRE(restartedSubscriber.membership->id == subscriberId);

  REQUIRE(restarted.requestFederationRestore(
               L"exercise", publisherId, saveLabel).status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  REQUIRE(restarted.federateRestoreComplete(
               L"exercise", publisherId).status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  REQUIRE(restarted.federateRestoreComplete(
               L"exercise", subscriberId).status ==
      umbra::detail::FederationRestoreControlStatus::applied);

  // Restoring declarations must establish the current relevance baseline,
  // so an immediate planner pass has no synthetic Start/Turn-On edges.
  auto baseline = restarted.planDeclarationAdvisories(L"exercise");
  REQUIRE(baseline.empty());

  // A real edge after restore remains visible: disabling both subscriptions
  // produces exactly the matching Stop/Turn-Off transitions.
  REQUIRE(restarted.setObjectClassAttributeSubscription(
               L"exercise", subscriberId, *objectClass, {*attribute}, false) ==
      umbra::detail::ObjectClassAttributeDeclarationStatus::applied);
  REQUIRE(restarted.setInteractionClassSubscription(
               L"exercise", subscriberId, *interactionClass, false) ==
      umbra::detail::InteractionClassDeclarationStatus::applied);
  auto transitions = restarted.planDeclarationAdvisories(L"exercise");
  REQUIRE(transitions.size() == 2U);
  REQUIRE(std::count_if(
              transitions.begin(), transitions.end(), [](auto const& advisory) {
                return advisory.kind ==
                    umbra::detail::DeclarationAdvisoryKind::stop_registration_for_object_class;
              }) == 1U);
  REQUIRE(std::count_if(
              transitions.begin(), transitions.end(), [](auto const& advisory) {
                return advisory.kind ==
                    umbra::detail::DeclarationAdvisoryKind::turn_interactions_off;
              }) == 1U);

  std::filesystem::remove_all(directory, ignored);
}
