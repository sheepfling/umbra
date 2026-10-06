#include "ieee1516_2025_federation_registry_test_support.hpp"

TEST_CASE(
    "Federation restore rehydrates reserved object-instance names",
    "[unit][kernel][federation-registry][save-restore][durable-save][restore]"
    "[object-name-reservation-state]") {
  auto store = std::make_shared<umbra::detail::MemoryFederationSaveCommitStore>();
  EmbeddedFederationRegistry registry({}, store);
  REQUIRE(registry.create(L"exercise", validDefinition()).status ==
      FederationRegistryStatus::applied);
  auto joined = registry.join(
      L"exercise", L"trainer", L"alice", noOpCallbackRoute());
  REQUIRE(joined.status == FederationRegistryStatus::applied);
  REQUIRE(joined.membership);

  auto const federateId = joined.membership->id;
  auto reservation = registry.reserveObjectInstanceName(
      L"exercise", federateId, L"reserved-table");
  REQUIRE(reservation.status == ObjectInstanceNameReservationStatus::applied);
  REQUIRE(reservation.succeeded);
  REQUIRE(registry.requestFederationSave(
      L"exercise", federateId, L"object-name-reservation-restore").status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(registry.federateSaveBegun(
      L"exercise", federateId).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(registry.federateSaveComplete(
      L"exercise", federateId).saveCompletedSuccessfully);

  REQUIRE(registry.releaseObjectInstanceName(
      L"exercise", federateId, L"reserved-table") ==
      ObjectInstanceNameReservationStatus::applied);
  auto restore = registry.requestFederationRestore(
      L"exercise", federateId, L"object-name-reservation-restore");
  REQUIRE(restore.status == umbra::detail::FederationRestoreControlStatus::applied);
  REQUIRE(registry.federateRestoreComplete(
      L"exercise", federateId).status ==
      umbra::detail::FederationRestoreControlStatus::applied);

  auto duplicate = registry.reserveObjectInstanceName(
      L"exercise", federateId, L"reserved-table");
  REQUIRE(duplicate.status == ObjectInstanceNameReservationStatus::applied);
  REQUIRE_FALSE(duplicate.succeeded);
  REQUIRE(registry.requestFederationSave(
      L"exercise", federateId, L"object-name-reservation-after-restore").status ==
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
  REQUIRE(restoredImage.reservedObjectInstanceNames.size() == 1U);
  REQUIRE(restoredImage.reservedObjectInstanceNames.front().federateId == federateId);
  REQUIRE(restoredImage.reservedObjectInstanceNames.front().objectInstanceName ==
      L"reserved-table");
}
