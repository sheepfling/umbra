#include "ieee1516_2025_federation_registry_test_support.hpp"

TEST_CASE(
    "Federation restore rehydrates accepted reflection callback count",
    "[unit][kernel][federation-registry][save-restore][durable-save][restore]"
    "[membership-reflection-state]") {
  auto store = std::make_shared<umbra::detail::MemoryFederationSaveCommitStore>();
  EmbeddedFederationRegistry registry({}, store);
  REQUIRE(registry.create(L"exercise", validDefinition()).status ==
      FederationRegistryStatus::applied);
  auto joined = registry.join(
      L"exercise", L"trainer", L"alice", noOpCallbackRoute());
  REQUIRE(joined.status == FederationRegistryStatus::applied);
  REQUIRE(joined.membership);

  auto const federateId = joined.membership->id;
  REQUIRE(registry.recordSuccessfulReflectionReceipt(
      L"exercise", federateId, 19U, "HLAreliable") ==
      FederationRegistryStatus::applied);
  REQUIRE(registry.requestFederationSave(
      L"exercise", federateId, L"reflection-restore").status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(registry.federateSaveBegun(
      L"exercise", federateId).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(registry.federateSaveComplete(
      L"exercise", federateId).saveCompletedSuccessfully);

  // A later accepted callback must not survive restore of the earlier
  // joined-federate lifetime statistic.
  REQUIRE(registry.recordSuccessfulReflectionReceipt(
      L"exercise", federateId, 20U, "HLAbestEffort") ==
      FederationRegistryStatus::applied);
  auto restore = registry.requestFederationRestore(
      L"exercise", federateId, L"reflection-restore");
  REQUIRE(restore.status == umbra::detail::FederationRestoreControlStatus::applied);
  REQUIRE(registry.federateRestoreComplete(
      L"exercise", federateId).status ==
      umbra::detail::FederationRestoreControlStatus::applied);

  REQUIRE(registry.requestFederationSave(
      L"exercise", federateId, L"reflection-after-restore").status ==
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
  REQUIRE(restoredImage.members.front().successfulReflectionsReceivedCount == 1U);
}
