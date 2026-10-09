#include "ieee1516_2025_federation_registry_test_support.hpp"

TEST_CASE(
    "Filesystem state image restores independent interaction declarations for multiple federates",
    "[unit][kernel][federation-registry][save-restore][durable-save][filesystem][restore]"
    "[process-restart][process-restart-interaction-declaration][interaction-declaration-state]"
    "[interaction-management][declaration-management][multi-federate-callback-ordering]") {
  auto const directory = temporarySaveCommitDirectory();
  std::error_code ignored;
  std::filesystem::remove_all(directory, ignored);
  auto store = std::make_shared<umbra::detail::FilesystemFederationSaveCommitStore>(
      directory);

  EmbeddedFederationRegistry source({}, store);
  REQUIRE(source.create(L"exercise", composedRestaurantDefinition()).status ==
      FederationRegistryStatus::applied);
  auto first = source.join(
      L"exercise", L"publisher-a", L"publisher-a", noOpCallbackRoute());
  auto second = source.join(
      L"exercise", L"publisher-b", L"publisher-b", noOpCallbackRoute());
  REQUIRE(first.status == FederationRegistryStatus::applied);
  REQUIRE(second.status == FederationRegistryStatus::applied);
  REQUIRE(first.membership);
  REQUIRE(second.membership);
  auto const firstId = first.membership->id;
  auto const secondId = second.membership->id;

  auto const firstClass = source.interactionClassHandleFor(
      L"exercise", "HLAinteractionRoot.CustomerTransactions.CustomerSeated");
  auto const secondClass = source.interactionClassHandleFor(
      L"exercise", "HLAinteractionRoot.ServerAction.TakeOrder");
  REQUIRE(firstClass.has_value());
  REQUIRE(secondClass.has_value());
  REQUIRE(source.setInteractionClassPublication(
      L"exercise", firstId, *firstClass, true) ==
      umbra::detail::InteractionClassDeclarationStatus::applied);
  REQUIRE(source.setInteractionClassPublication(
      L"exercise", secondId, *secondClass, true) ==
      umbra::detail::InteractionClassDeclarationStatus::applied);

  std::wstring const saveLabel = L"interaction-multi-federate-process-restart";
  REQUIRE(source.requestFederationSave(
      L"exercise", firstId, saveLabel).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(source.federateSaveBegun(
      L"exercise", firstId).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(source.federateSaveBegun(
      L"exercise", secondId).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE_FALSE(source.federateSaveComplete(
      L"exercise", firstId).saveCompletedSuccessfully);
  REQUIRE(source.federateSaveComplete(
      L"exercise", secondId).saveCompletedSuccessfully);

  auto committed = store->load(L"exercise", saveLabel);
  REQUIRE(committed.has_value());
  auto image = umbra::detail::FederationStateImageCodec::decode(
      committed->stateImage);
  REQUIRE(image.interactionDeclarationCount == 2U);
  REQUIRE(image.interactionDeclarations.size() == 2U);
  REQUIRE(image.interactionDeclarations[0].federateId == firstId);
  REQUIRE(image.interactionDeclarations[1].federateId == secondId);
  REQUIRE(image.interactionDeclarations[0].publishedInteractionClasses ==
      std::vector<std::uint64_t>{*firstClass});
  REQUIRE(image.interactionDeclarations[1].publishedInteractionClasses ==
      std::vector<std::uint64_t>{*secondClass});
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
  auto restartedFirst = restarted.join(
      L"exercise", L"publisher-a", L"publisher-a", noOpCallbackRoute());
  auto restartedSecond = restarted.join(
      L"exercise", L"publisher-b", L"publisher-b", noOpCallbackRoute());
  REQUIRE(restartedFirst.status == FederationRegistryStatus::applied);
  REQUIRE(restartedSecond.status == FederationRegistryStatus::applied);
  REQUIRE(restartedFirst.membership);
  REQUIRE(restartedSecond.membership);
  REQUIRE(restartedFirst.membership->id == firstId);
  REQUIRE(restartedSecond.membership->id == secondId);
  REQUIRE(restarted.requestFederationRestore(
      L"exercise", firstId, saveLabel).status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  REQUIRE(restarted.federateRestoreComplete(
      L"exercise", firstId).status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  auto restored = restarted.federateRestoreComplete(
      L"exercise", secondId);
  REQUIRE(restored.status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  REQUIRE(restored.interactionTransportationTypeChangeWorkItems.empty());

  auto firstDeclaration = restarted.interactionClassDeclarationFor(
      L"exercise", firstId, *firstClass);
  auto secondDeclaration = restarted.interactionClassDeclarationFor(
      L"exercise", secondId, *secondClass);
  REQUIRE(firstDeclaration.has_value());
  REQUIRE(firstDeclaration->published);
  REQUIRE(secondDeclaration.has_value());
  REQUIRE(secondDeclaration->published);

  std::filesystem::remove_all(directory, ignored);
}
