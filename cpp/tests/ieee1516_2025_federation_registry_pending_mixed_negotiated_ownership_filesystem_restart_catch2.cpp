#include "ieee1516_2025_federation_registry_test_support.hpp"

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
