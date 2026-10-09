#include "ieee1516_2025_federation_registry_test_support.hpp"

TEST_CASE(
    "Filesystem state image restores federation-owned regions in a fresh registry",
    "[unit][kernel][federation-registry][save-restore][durable-save][filesystem][restore]"
    "[process-restart][region-state]") {
  auto const directory = temporarySaveCommitDirectory();
  std::error_code ignored;
  std::filesystem::remove_all(directory, ignored);
  auto store = std::make_shared<umbra::detail::FilesystemFederationSaveCommitStore>(
      directory);

  EmbeddedFederationRegistry source({}, store);
  REQUIRE(source.create(L"exercise", composedRestaurantDefinition()).status ==
      FederationRegistryStatus::applied);
  auto sourceJoined = source.join(
      L"exercise", L"trainer", L"alice", noOpCallbackRoute());
  REQUIRE(sourceJoined.status == FederationRegistryStatus::applied);
  REQUIRE(sourceJoined.membership);
  auto const federateId = sourceJoined.membership->id;
  auto dimension = source.dimensionHandleFor(L"exercise", "ServerId");
  REQUIRE(dimension.has_value());
  auto created = source.createRegion(L"exercise", federateId, {*dimension});
  REQUIRE(created.status == umbra::detail::RegionServiceStatus::applied);
  REQUIRE(created.regionHandle != 0U);
  REQUIRE(source.setRangeBounds(
      L"exercise", federateId, created.regionHandle, *dimension,
      umbra::detail::RegionRangeBounds{2UL, 5UL}) ==
      umbra::detail::RegionServiceStatus::applied);
  REQUIRE(source.commitRegionModifications(
      L"exercise", federateId, {created.regionHandle}) ==
      umbra::detail::RegionServiceStatus::applied);

  REQUIRE(source.requestFederationSave(
      L"exercise", federateId, L"process-restart-region-checkpoint").status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(source.federateSaveBegun(L"exercise", federateId).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(source.federateSaveComplete(L"exercise", federateId)
      .saveCompletedSuccessfully);

  EmbeddedFederationRegistry restarted({}, store);
  REQUIRE(restarted.create(L"exercise", composedRestaurantDefinition()).status ==
      FederationRegistryStatus::applied);
  auto restartedJoined = restarted.join(
      L"exercise", L"trainer", L"alice", noOpCallbackRoute());
  REQUIRE(restartedJoined.status == FederationRegistryStatus::applied);
  REQUIRE(restartedJoined.membership);
  REQUIRE(restartedJoined.membership->id == federateId);
  REQUIRE(restarted.requestFederationRestore(
      L"exercise", federateId, L"process-restart-region-checkpoint").status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  REQUIRE(restarted.federateRestoreComplete(L"exercise", federateId).status ==
      umbra::detail::FederationRestoreControlStatus::applied);

  auto dimensions = restarted.dimensionHandleSetForRegion(
      L"exercise", federateId, created.regionHandle);
  REQUIRE(dimensions.status == umbra::detail::RegionServiceStatus::applied);
  REQUIRE(dimensions.dimensionHandles == std::set<std::uint64_t>{*dimension});
  auto bounds = restarted.rangeBoundsForRegion(
      L"exercise", federateId, created.regionHandle, *dimension);
  REQUIRE(bounds.status == umbra::detail::RegionServiceStatus::applied);
  REQUIRE(bounds.range.lowerBound == 2UL);
  REQUIRE(bounds.range.upperBound == 5UL);

  std::filesystem::remove_all(directory, ignored);
}
