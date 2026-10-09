#include "ieee1516_2025_federation_registry_test_support.hpp"

TEST_CASE(
    "Filesystem state image restores a committed interaction transportation-type override in a fresh registry",
    "[unit][kernel][federation-registry][save-restore][durable-save][filesystem][restore]"
    "[process-restart][process-restart-interaction-transportation-type-override]"
    "[interaction-declaration-state]"
    "[interaction-transportation-type-change][transportation-management]") {
  auto const directory = temporarySaveCommitDirectory();
  std::error_code ignored;
  std::filesystem::remove_all(directory, ignored);
  auto store = std::make_shared<umbra::detail::FilesystemFederationSaveCommitStore>(
      directory);

  EmbeddedFederationRegistry source({}, store);
  REQUIRE(source.create(L"exercise", composedRestaurantDefinition()).status ==
      FederationRegistryStatus::applied);
  auto sourceJoined = source.join(
      L"exercise", L"publisher", L"owner", noOpCallbackRoute());
  REQUIRE(sourceJoined.status == FederationRegistryStatus::applied);
  REQUIRE(sourceJoined.membership);
  auto const ownerId = sourceJoined.membership->id;

  auto const takeOrder = source.interactionClassHandleFor(
      L"exercise", "HLAinteractionRoot.ServerAction.TakeOrder");
  REQUIRE(takeOrder.has_value());
  REQUIRE(source.setInteractionClassPublication(
      L"exercise", ownerId, *takeOrder, true) ==
      umbra::detail::InteractionClassDeclarationStatus::applied);

  auto change = source.planInteractionTransportationTypeChange(
      L"exercise", ownerId, *takeOrder, "HLAbestEffort");
  REQUIRE(change.status ==
      umbra::detail::InteractionTransportationTypeChangeStatus::applied);
  auto committedChange = source.beginInteractionTransportationTypeChange(
      L"exercise", ownerId, *takeOrder);
  REQUIRE(committedChange.has_value());
  REQUIRE(*committedChange == "HLAbestEffort");
  REQUIRE_FALSE(source.beginInteractionTransportationTypeChange(
      L"exercise", ownerId, *takeOrder));

  auto sourceQuery = source.interactionTransportationTypeQueryFor(
      L"exercise", ownerId, ownerId, *takeOrder);
  REQUIRE(sourceQuery.has_value());
  REQUIRE(sourceQuery->transportationName == "HLAbestEffort");

  std::wstring const saveLabel =
      L"interaction-transportation-type-override-process-restart";
  REQUIRE(source.requestFederationSave(
      L"exercise", ownerId, saveLabel).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(source.federateSaveBegun(
      L"exercise", ownerId).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(source.federateSaveComplete(
      L"exercise", ownerId).saveCompletedSuccessfully);

  auto committed = store->load(L"exercise", saveLabel);
  REQUIRE(committed.has_value());
  auto image = umbra::detail::FederationStateImageCodec::decode(
      committed->stateImage);
  REQUIRE(image.interactionDeclarationCount == 1U);
  REQUIRE(image.interactionDeclarations.size() == 1U);
  auto const& savedDeclaration = image.interactionDeclarations.front();
  REQUIRE(savedDeclaration.federateId == ownerId);
  REQUIRE(savedDeclaration.publishedInteractionClasses ==
      std::vector<std::uint64_t>{*takeOrder});
  REQUIRE(savedDeclaration.subscribedInteractionClasses.empty());
  REQUIRE(savedDeclaration.regionalSubscribedInteractionClasses.empty());
  REQUIRE(savedDeclaration.interactionTransportationTypes.size() == 1U);
  REQUIRE(savedDeclaration.interactionTransportationTypes.front().interactionClassHandle ==
      *takeOrder);
  REQUIRE(savedDeclaration.interactionTransportationTypes.front().value ==
      "HLAbestEffort");
  REQUIRE(savedDeclaration.interactionOrderTypes.empty());
  REQUIRE(savedDeclaration.pendingInteractionTransportationTypeChanges.empty());
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
      L"exercise", L"publisher", L"owner", noOpCallbackRoute());
  REQUIRE(restartedJoined.status == FederationRegistryStatus::applied);
  REQUIRE(restartedJoined.membership);
  REQUIRE(restartedJoined.membership->id == ownerId);
  REQUIRE(restarted.requestFederationRestore(
      L"exercise", ownerId, saveLabel).status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  auto restored = restarted.federateRestoreComplete(L"exercise", ownerId);
  REQUIRE(restored.status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  REQUIRE(restored.interactionTransportationTypeChangeWorkItems.empty());

  auto restoredDeclaration = restarted.interactionClassDeclarationFor(
      L"exercise", ownerId, *takeOrder);
  REQUIRE(restoredDeclaration.has_value());
  REQUIRE(restoredDeclaration->published);
  auto restoredQuery = restarted.interactionTransportationTypeQueryFor(
      L"exercise", ownerId, ownerId, *takeOrder);
  REQUIRE(restoredQuery.has_value());
  REQUIRE(restoredQuery->transportationName == "HLAbestEffort");

  std::filesystem::remove_all(directory, ignored);
}
