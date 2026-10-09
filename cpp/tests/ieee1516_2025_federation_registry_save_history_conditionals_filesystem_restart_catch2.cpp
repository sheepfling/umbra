#include "ieee1516_2025_federation_registry_test_support.hpp"

TEST_CASE(
    "Filesystem state image restores federation save conditionals in a fresh registry",
    "[unit][kernel][federation-registry][save-restore][durable-save][filesystem][restore]"
    "[process-restart][application-ledger-state][federation-mom-save-conditionals]") {
  auto const directory = temporarySaveCommitDirectory();
  std::error_code ignored;
  std::filesystem::remove_all(directory, ignored);
  auto store = std::make_shared<umbra::detail::FilesystemFederationSaveCommitStore>(
      directory);

  auto completeSave = [](EmbeddedFederationRegistry& registry,
                         std::wstring const& label,
                         std::uint64_t federateId) {
    REQUIRE(registry.requestFederationSave(
        L"exercise", federateId, label).status ==
        umbra::detail::FederationSaveControlStatus::applied);
    REQUIRE(registry.federateSaveBegun(L"exercise", federateId).status ==
        umbra::detail::FederationSaveControlStatus::applied);
    REQUIRE(registry.federateSaveComplete(L"exercise", federateId)
        .saveCompletedSuccessfully);
  };

  EmbeddedFederationRegistry source({}, store);
  REQUIRE(source.create(L"exercise", composedRestaurantDefinition()).status ==
      FederationRegistryStatus::applied);
  auto joined = source.join(
      L"exercise", L"trainer", L"alice", noOpCallbackRoute());
  REQUIRE(joined.status == FederationRegistryStatus::applied);
  REQUIRE(joined.membership);
  auto const federateId = joined.membership->id;

  std::wstring const firstLabel = L"save-history-first";
  std::wstring const secondLabel = L"save-history-second";
  completeSave(source, firstLabel, federateId);
  completeSave(source, secondLabel, federateId);

  auto secondCommit = store->load(L"exercise", secondLabel);
  REQUIRE(secondCommit.has_value());
  auto secondImage = umbra::detail::FederationStateImageCodec::decode(
      secondCommit->stateImage);
  REQUIRE(secondImage.saveHistoryPresent);
  // Save completion updates HLAlastSave* after the durable snapshot is
  // committed, so the second image must retain the first completed label.
  REQUIRE(secondImage.lastSaveName == firstLabel);
  REQUIRE_FALSE(secondImage.lastSaveTimeEncoding.has_value());
  REQUIRE(secondImage.nextSaveName.empty());
  REQUIRE_FALSE(secondImage.nextSaveTimeEncoding.has_value());

  EmbeddedFederationRegistry restarted({}, store);
  REQUIRE(restarted.create(L"exercise", composedRestaurantDefinition()).status ==
      FederationRegistryStatus::applied);
  auto restartedJoined = restarted.join(
      L"exercise", L"trainer", L"alice", noOpCallbackRoute());
  REQUIRE(restartedJoined.status == FederationRegistryStatus::applied);
  REQUIRE(restartedJoined.membership);
  REQUIRE(restartedJoined.membership->id == federateId);
  REQUIRE(restarted.requestFederationRestore(
      L"exercise", federateId, secondLabel).status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  REQUIRE(restarted.federateRestoreComplete(L"exercise", federateId).status ==
      umbra::detail::FederationRestoreControlStatus::applied);

  // A subsequent save snapshots the rehydrated application-visible
  // HLAlastSave* values. If restore only rebuilt control/temporal state, this
  // image would incorrectly lose the first label.
  auto const thirdLabel = L"save-history-after-restore";
  completeSave(restarted, thirdLabel, federateId);
  auto thirdCommit = store->load(L"exercise", thirdLabel);
  REQUIRE(thirdCommit.has_value());
  auto thirdImage = umbra::detail::FederationStateImageCodec::decode(
      thirdCommit->stateImage);
  REQUIRE(thirdImage.saveHistoryPresent);
  REQUIRE(thirdImage.lastSaveName == firstLabel);
  REQUIRE_FALSE(thirdImage.lastSaveTimeEncoding.has_value());

  std::filesystem::remove_all(directory, ignored);
}
