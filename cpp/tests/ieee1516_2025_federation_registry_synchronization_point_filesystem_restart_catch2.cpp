#include "ieee1516_2025_federation_registry_test_support.hpp"

TEST_CASE(
    "Filesystem state image restores synchronization points in a fresh registry",
    "[unit][kernel][federation-registry][save-restore][durable-save][filesystem][restore]"
    "[process-restart][synchronization-state]") {
  auto const directory = temporarySaveCommitDirectory();
  std::error_code ignored;
  std::filesystem::remove_all(directory, ignored);
  auto store = std::make_shared<umbra::detail::FilesystemFederationSaveCommitStore>(
      directory);

  EmbeddedFederationRegistry source({}, store);
  REQUIRE(source.create(L"exercise", validDefinition()).status ==
      FederationRegistryStatus::applied);
  auto sourceJoined = source.join(
      L"exercise", L"trainer", L"alice", noOpCallbackRoute());
  REQUIRE(sourceJoined.status == FederationRegistryStatus::applied);
  REQUIRE(sourceJoined.membership);
  auto const federateId = sourceJoined.membership->id;
  auto registered = source.registerSynchronizationPoint(
      L"exercise",
      federateId,
      L"process-restart-barrier",
      {0x01U, 0x02U},
      {federateId},
      true);
  REQUIRE(registered.status ==
      umbra::detail::SynchronizationPointRegistrationStatus::applied);

  REQUIRE(source.requestFederationSave(
      L"exercise", federateId, L"process-restart-sync-checkpoint").status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(source.federateSaveBegun(L"exercise", federateId).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(source.federateSaveComplete(L"exercise", federateId)
      .saveCompletedSuccessfully);

  EmbeddedFederationRegistry restarted({}, store);
  REQUIRE(restarted.create(L"exercise", validDefinition()).status ==
      FederationRegistryStatus::applied);
  auto restartedJoined = restarted.join(
      L"exercise", L"trainer", L"alice", noOpCallbackRoute());
  REQUIRE(restartedJoined.status == FederationRegistryStatus::applied);
  REQUIRE(restartedJoined.membership);
  REQUIRE(restartedJoined.membership->id == federateId);
  REQUIRE(restarted.requestFederationRestore(
      L"exercise", federateId, L"process-restart-sync-checkpoint").status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  REQUIRE(restarted.federateRestoreComplete(L"exercise", federateId).status ==
      umbra::detail::FederationRestoreControlStatus::applied);

  REQUIRE(restarted.requestFederationSave(
      L"exercise", federateId, L"process-restart-sync-after-restore").status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(restarted.federateSaveBegun(L"exercise", federateId).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(restarted.federateSaveComplete(L"exercise", federateId)
      .saveCompletedSuccessfully);
  auto committed = store->load(
      L"exercise", L"process-restart-sync-after-restore");
  REQUIRE(committed.has_value());
  auto image = umbra::detail::FederationStateImageCodec::decode(
      committed->stateImage);
  REQUIRE(image.synchronizationPoints.size() == 1U);
  REQUIRE(image.synchronizationPoints.front().label ==
      L"process-restart-barrier");
  REQUIRE(image.synchronizationPoints.front().userSuppliedTag ==
      std::string{"\x01\x02", 2U});
  REQUIRE(image.synchronizationPoints.front().synchronizationSet ==
      std::vector<std::uint64_t>{federateId});
  REQUIRE_FALSE(image.synchronizationPoints.front().lateJoinExpansionAllowed);

  std::filesystem::remove_all(directory, ignored);
}
