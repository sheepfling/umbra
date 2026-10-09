#include "ieee1516_2025_federation_registry_test_support.hpp"

TEST_CASE(
    "Filesystem federation save commit permits restore admission after reload",
    "[unit][kernel][federation-registry][save-restore][durable-save][filesystem][restore]") {
  auto const directory = temporarySaveCommitDirectory();
  std::error_code ignored;
  std::filesystem::remove_all(directory, ignored);
  auto store = std::make_shared<umbra::detail::FilesystemFederationSaveCommitStore>(directory);
  EmbeddedFederationRegistry registry({}, store);
  REQUIRE(registry.create(L"exercise", validDefinition()).status ==
      FederationRegistryStatus::applied);
  auto joined = registry.join(
      L"exercise", L"trainer", L"alice", noOpCallbackRoute());
  REQUIRE(joined.status == FederationRegistryStatus::applied);
  REQUIRE(joined.membership);
  REQUIRE(registry.requestFederationSave(
      L"exercise", joined.membership->id, L"checkpoint").status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(registry.federateSaveBegun(
      L"exercise", joined.membership->id).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(registry.federateSaveComplete(
      L"exercise", joined.membership->id).saveCompletedSuccessfully);

  auto restore = registry.requestFederationRestore(
      L"exercise", joined.membership->id, L"checkpoint");
  REQUIRE(restore.status == umbra::detail::FederationRestoreControlStatus::applied);
  REQUIRE(restore.notifications.size() == 3U);
  auto restored = registry.federateRestoreComplete(
      L"exercise", joined.membership->id);
  REQUIRE(restored.notifications.size() == 1U);
  REQUIRE(restored.notifications.front().successful);

  std::filesystem::remove_all(directory, ignored);
}
