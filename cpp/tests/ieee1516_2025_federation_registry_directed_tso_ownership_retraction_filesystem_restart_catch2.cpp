#include "ieee1516_2025_federation_registry_test_support.hpp"

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
