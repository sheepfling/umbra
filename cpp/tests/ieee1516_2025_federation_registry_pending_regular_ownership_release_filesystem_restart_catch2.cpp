#include "ieee1516_2025_federation_registry_test_support.hpp"

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
