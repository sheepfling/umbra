#include "ieee1516_2025_federation_registry_test_support.hpp"

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
