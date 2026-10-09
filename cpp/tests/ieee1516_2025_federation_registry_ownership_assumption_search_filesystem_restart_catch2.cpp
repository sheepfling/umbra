#include "ieee1516_2025_federation_registry_test_support.hpp"

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
