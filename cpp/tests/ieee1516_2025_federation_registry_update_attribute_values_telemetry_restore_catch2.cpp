#include "ieee1516_2025_federation_registry_test_support.hpp"

TEST_CASE(
    "Federation restore rehydrates accepted Update Attribute Values telemetry",
    "[unit][kernel][federation-registry][save-restore][durable-save][restore]"
    "[membership-update-state]") {
  auto store = std::make_shared<umbra::detail::MemoryFederationSaveCommitStore>();
  EmbeddedFederationRegistry registry({}, store);
  REQUIRE(registry.create(L"exercise", validDefinition()).status ==
      FederationRegistryStatus::applied);
  auto joined = registry.join(
      L"exercise", L"trainer", L"alice", noOpCallbackRoute());
  REQUIRE(joined.status == FederationRegistryStatus::applied);
  REQUIRE(joined.membership);

  auto const federateId = joined.membership->id;
  REQUIRE(registry.recordSuccessfulUpdateAttributeValues(
      L"exercise", federateId, 19U, 4U, {"HLAreliable"}) ==
      FederationRegistryStatus::applied);
  REQUIRE(registry.requestFederationSave(
      L"exercise", federateId, L"telemetry-restore").status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(registry.federateSaveBegun(
      L"exercise", federateId).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(registry.federateSaveComplete(
      L"exercise", federateId).saveCompletedSuccessfully);

  // Mutate the live lifetime ledger after the save. Restore must return to the
  // accepted count and distinct-object projection captured in the image.
  REQUIRE(registry.recordSuccessfulUpdateAttributeValues(
      L"exercise", federateId, 20U, 4U, {"HLAreliable"}) ==
      FederationRegistryStatus::applied);
  auto restore = registry.requestFederationRestore(
      L"exercise", federateId, L"telemetry-restore");
  REQUIRE(restore.status == umbra::detail::FederationRestoreControlStatus::applied);
  REQUIRE(registry.federateRestoreComplete(
      L"exercise", federateId).status ==
      umbra::detail::FederationRestoreControlStatus::applied);

  REQUIRE(registry.requestFederationSave(
      L"exercise", federateId, L"telemetry-after-restore").status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(registry.federateSaveBegun(
      L"exercise", federateId).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(registry.federateSaveComplete(
      L"exercise", federateId).saveCompletedSuccessfully);
  auto commits = store->snapshotCommits();
  REQUIRE(commits.size() == 2U);
  auto restoredImage = umbra::detail::FederationStateImageCodec::decode(
      commits.back().stateImage);
  REQUIRE(restoredImage.members.size() == 1U);
  REQUIRE(restoredImage.members.front().successfulUpdateAttributeValuesCount == 1U);
  REQUIRE(restoredImage.members.front().successfullyUpdatedObjectInstanceHandles ==
      std::vector<std::uint64_t>{19U});
  REQUIRE(restoredImage.members.front().successfullyUpdatedObjectInstanceClassHandles.size() ==
      1U);
  REQUIRE(restoredImage.members.front().successfulUpdateCountsByClassAndTransportation.front()
              .count == 1U);
}
