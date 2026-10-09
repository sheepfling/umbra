#include "ieee1516_2025_federation_registry_test_support.hpp"

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
