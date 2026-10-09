#include "ieee1516_2025_federation_registry_test_support.hpp"

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
