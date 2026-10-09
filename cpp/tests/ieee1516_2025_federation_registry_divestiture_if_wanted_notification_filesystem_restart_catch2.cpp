#include "ieee1516_2025_federation_registry_test_support.hpp"

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
