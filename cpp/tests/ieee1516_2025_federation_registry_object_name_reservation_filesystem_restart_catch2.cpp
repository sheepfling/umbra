#include "ieee1516_2025_federation_registry_test_support.hpp"

TEST_CASE(
    "Filesystem state image restores object-instance-name reservations in a fresh registry",
    "[unit][kernel][federation-registry][save-restore][durable-save][filesystem][restore]"
    "[process-restart][object-name-reservation-state]") {
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
  auto reservation = source.reserveObjectInstanceName(
      L"exercise", federateId, L"process-restart-reserved-table");
  REQUIRE(reservation.status == ObjectInstanceNameReservationStatus::applied);
  REQUIRE(reservation.succeeded);

  REQUIRE(source.requestFederationSave(
      L"exercise", federateId, L"process-restart-name-checkpoint").status ==
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
      L"exercise", federateId, L"process-restart-name-checkpoint").status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  REQUIRE(restarted.federateRestoreComplete(L"exercise", federateId).status ==
      umbra::detail::FederationRestoreControlStatus::applied);

  auto duplicate = restarted.reserveObjectInstanceName(
      L"exercise", federateId, L"process-restart-reserved-table");
  REQUIRE(duplicate.status == ObjectInstanceNameReservationStatus::applied);
  REQUIRE_FALSE(duplicate.succeeded);
  REQUIRE(restarted.releaseObjectInstanceName(
      L"exercise", federateId, L"process-restart-reserved-table") ==
      ObjectInstanceNameReservationStatus::applied);
  auto available = restarted.reserveObjectInstanceName(
      L"exercise", federateId, L"process-restart-reserved-table");
  REQUIRE(available.status == ObjectInstanceNameReservationStatus::applied);
  REQUIRE(available.succeeded);

  std::filesystem::remove_all(directory, ignored);
}
