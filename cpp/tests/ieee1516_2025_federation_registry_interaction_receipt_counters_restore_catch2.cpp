#include "ieee1516_2025_federation_registry_test_support.hpp"

TEST_CASE(
    "Federation restore rehydrates accepted interaction-receipt counters",
    "[unit][kernel][federation-registry][save-restore][durable-save][restore]"
    "[membership-interaction-receive-state]") {
  auto store = std::make_shared<umbra::detail::MemoryFederationSaveCommitStore>();
  EmbeddedFederationRegistry registry({}, store);
  REQUIRE(registry.create(L"exercise", validDefinition()).status ==
      FederationRegistryStatus::applied);
  auto joined = registry.join(
      L"exercise", L"trainer", L"alice", noOpCallbackRoute());
  REQUIRE(joined.status == FederationRegistryStatus::applied);
  REQUIRE(joined.membership);

  auto const federateId = joined.membership->id;
  REQUIRE(registry.recordSuccessfulInteractionReceipt(
      L"exercise", federateId, 0U, "", false) ==
      FederationRegistryStatus::applied);
  REQUIRE(registry.recordSuccessfulInteractionReceipt(
      L"exercise", federateId, 0U, "", true) ==
      FederationRegistryStatus::applied);
  REQUIRE(registry.requestFederationSave(
      L"exercise", federateId, L"interaction-receipt-restore").status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(registry.federateSaveBegun(
      L"exercise", federateId).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(registry.federateSaveComplete(
      L"exercise", federateId).saveCompletedSuccessfully);

  REQUIRE(registry.recordSuccessfulInteractionReceipt(
      L"exercise", federateId, 0U, "", true) ==
      FederationRegistryStatus::applied);
  auto restore = registry.requestFederationRestore(
      L"exercise", federateId, L"interaction-receipt-restore");
  REQUIRE(restore.status == umbra::detail::FederationRestoreControlStatus::applied);
  REQUIRE(registry.federateRestoreComplete(
      L"exercise", federateId).status ==
      umbra::detail::FederationRestoreControlStatus::applied);

  REQUIRE(registry.requestFederationSave(
      L"exercise", federateId, L"interaction-receipt-after-restore").status ==
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
  REQUIRE(restoredImage.members.front().successfulInteractionsReceivedCount == 2U);
  REQUIRE(restoredImage.members.front().successfulDirectedInteractionsReceivedCount == 1U);
}
