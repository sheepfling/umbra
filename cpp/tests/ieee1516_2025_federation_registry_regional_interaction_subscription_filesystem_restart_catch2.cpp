#include "ieee1516_2025_federation_registry_test_support.hpp"

TEST_CASE(
    "Filesystem state image restores a regional interaction subscription in a fresh registry",
    "[unit][kernel][federation-registry][save-restore][durable-save][filesystem][restore]"
    "[process-restart][process-restart-interaction-declaration][interaction-declaration-state]"
    "[interaction-management][ddm][regional-interaction]") {
  auto const directory = temporarySaveCommitDirectory();
  std::error_code ignored;
  std::filesystem::remove_all(directory, ignored);
  auto store = std::make_shared<umbra::detail::FilesystemFederationSaveCommitStore>(
      directory);

  EmbeddedFederationRegistry source({}, store);
  REQUIRE(source.create(L"exercise", composedRestaurantDefinition()).status ==
      FederationRegistryStatus::applied);
  auto sourceJoined = source.join(
      L"exercise", L"regional-subscriber", L"regional", noOpCallbackRoute());
  REQUIRE(sourceJoined.status == FederationRegistryStatus::applied);
  REQUIRE(sourceJoined.membership);
  auto const federateId = sourceJoined.membership->id;

  auto const dimension = source.dimensionHandleFor(L"exercise", "ServerId");
  auto const takeOrder = source.interactionClassHandleFor(
      L"exercise",
      "HLAinteractionRoot.CustomerTransactions.FoodServed.MainCourseServed");
  REQUIRE(dimension.has_value());
  REQUIRE(takeOrder.has_value());
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
  REQUIRE(source.setInteractionClassRegionalSubscription(
      L"exercise", federateId, *takeOrder, {created.regionHandle}, true) ==
      umbra::detail::RegionalInteractionClassDeclarationStatus::applied);

  std::wstring const saveLabel = L"interaction-regional-subscription-process-restart";
  REQUIRE(source.requestFederationSave(
      L"exercise", federateId, saveLabel).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(source.federateSaveBegun(
      L"exercise", federateId).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(source.federateSaveComplete(
      L"exercise", federateId).saveCompletedSuccessfully);

  auto committed = store->load(L"exercise", saveLabel);
  REQUIRE(committed.has_value());
  auto image = umbra::detail::FederationStateImageCodec::decode(
      committed->stateImage);
  REQUIRE(image.interactionDeclarationCount == 1U);
  REQUIRE(image.interactionDeclarations.size() == 1U);
  auto const& savedDeclaration = image.interactionDeclarations.front();
  REQUIRE(savedDeclaration.federateId == federateId);
  REQUIRE(savedDeclaration.publishedInteractionClasses.empty());
  REQUIRE(savedDeclaration.subscribedInteractionClasses.empty());
  REQUIRE(savedDeclaration.regionalSubscribedInteractionClasses.size() == 1U);
  auto const& savedSubscription =
      savedDeclaration.regionalSubscribedInteractionClasses.front();
  REQUIRE(savedSubscription.interactionClassHandle == *takeOrder);
  REQUIRE(savedSubscription.regionHandle == created.regionHandle);
  REQUIRE(savedSubscription.active);
  REQUIRE(image.regions.size() == 1U);
  REQUIRE(image.objects.empty());
  REQUIRE(image.tsoInteractionMessages.empty());
  REQUIRE(image.tsoAttributeUpdateMessages.empty());
  REQUIRE(image.tsoObjectDeletionMessages.empty());
  REQUIRE(image.tsoDirectedInteractionMessages.empty());
  REQUIRE(image.tsoRequestRetractionRecords.empty());
  REQUIRE(image.tsoQueueEntries.empty());

  EmbeddedFederationRegistry restarted({}, store);
  REQUIRE(restarted.create(L"exercise", composedRestaurantDefinition()).status ==
      FederationRegistryStatus::applied);
  auto restartedJoined = restarted.join(
      L"exercise", L"regional-subscriber", L"regional", noOpCallbackRoute());
  REQUIRE(restartedJoined.status == FederationRegistryStatus::applied);
  REQUIRE(restartedJoined.membership);
  REQUIRE(restartedJoined.membership->id == federateId);
  REQUIRE(restarted.requestFederationRestore(
      L"exercise", federateId, saveLabel).status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  auto restored = restarted.federateRestoreComplete(L"exercise", federateId);
  REQUIRE(restored.status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  REQUIRE(restored.interactionTransportationTypeChangeWorkItems.empty());

  auto dimensions = restarted.dimensionHandleSetForRegion(
      L"exercise", federateId, created.regionHandle);
  REQUIRE(dimensions.status == umbra::detail::RegionServiceStatus::applied);
  REQUIRE(dimensions.dimensionHandles == std::set<std::uint64_t>{*dimension});
  auto bounds = restarted.rangeBoundsForRegion(
      L"exercise", federateId, created.regionHandle, *dimension);
  REQUIRE(bounds.status == umbra::detail::RegionServiceStatus::applied);
  REQUIRE(bounds.range.lowerBound == 2UL);
  REQUIRE(bounds.range.upperBound == 5UL);

  std::wstring const roundTripLabel =
      L"interaction-regional-subscription-round-trip";
  REQUIRE(restarted.requestFederationSave(
      L"exercise", federateId, roundTripLabel).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(restarted.federateSaveBegun(
      L"exercise", federateId).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(restarted.federateSaveComplete(
      L"exercise", federateId).saveCompletedSuccessfully);
  auto roundTrip = store->load(L"exercise", roundTripLabel);
  REQUIRE(roundTrip.has_value());
  auto roundTripImage = umbra::detail::FederationStateImageCodec::decode(
      roundTrip->stateImage);
  REQUIRE(roundTripImage.interactionDeclarations.size() == 1U);
  REQUIRE(roundTripImage.interactionDeclarations.front()
              .regionalSubscribedInteractionClasses.size() == 1U);
  REQUIRE(roundTripImage.interactionDeclarations.front()
              .regionalSubscribedInteractionClasses.front()
              .regionHandle == created.regionHandle);

  std::filesystem::remove_all(directory, ignored);
}
