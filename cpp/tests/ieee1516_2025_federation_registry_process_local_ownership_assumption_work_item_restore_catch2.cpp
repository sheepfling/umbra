#include "ieee1516_2025_federation_registry_test_support.hpp"

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
