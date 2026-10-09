#include "ieee1516_2025_federation_registry_test_support.hpp"

TEST_CASE(
    "Filesystem state image restores multiple interaction declaration entries in a fresh registry",
    "[unit][kernel][federation-registry][save-restore][durable-save][filesystem][restore]"
    "[process-restart][process-restart-interaction-declaration][interaction-declaration-state]"
    "[interaction-management][declaration-management]") {
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

  auto const customerSeated = source.interactionClassHandleFor(
      L"exercise", "HLAinteractionRoot.CustomerTransactions.CustomerSeated");
  auto const takeOrder = source.interactionClassHandleFor(
      L"exercise", "HLAinteractionRoot.ServerAction.TakeOrder");
  REQUIRE(customerSeated.has_value());
  REQUIRE(takeOrder.has_value());
  std::vector<std::uint64_t> expectedPublished{
      *customerSeated,
      *takeOrder,
  };
  std::ranges::sort(expectedPublished);
  REQUIRE(source.setInteractionClassPublication(
      L"exercise", ownerId, *customerSeated, true) ==
      umbra::detail::InteractionClassDeclarationStatus::applied);
  REQUIRE(source.setInteractionClassPublication(
      L"exercise", ownerId, *takeOrder, true) ==
      umbra::detail::InteractionClassDeclarationStatus::applied);

  std::wstring const saveLabel = L"interaction-multiple-declarations-process-restart";
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
  REQUIRE(savedDeclaration.publishedInteractionClasses == expectedPublished);
  REQUIRE(savedDeclaration.subscribedInteractionClasses.empty());
  REQUIRE(savedDeclaration.regionalSubscribedInteractionClasses.empty());
  REQUIRE(savedDeclaration.publishedObjectClassDirectedInteractions.empty());
  REQUIRE(savedDeclaration.subscribedObjectClassDirectedInteractions.empty());
  REQUIRE(savedDeclaration.interactionTransportationTypes.empty());
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

  auto customerDeclaration = restarted.interactionClassDeclarationFor(
      L"exercise", ownerId, *customerSeated);
  auto takeOrderDeclaration = restarted.interactionClassDeclarationFor(
      L"exercise", ownerId, *takeOrder);
  REQUIRE(customerDeclaration.has_value());
  REQUIRE(customerDeclaration->published);
  REQUIRE_FALSE(customerDeclaration->subscriptionActive.has_value());
  REQUIRE(takeOrderDeclaration.has_value());
  REQUIRE(takeOrderDeclaration->published);
  REQUIRE_FALSE(takeOrderDeclaration->subscriptionActive.has_value());

  std::filesystem::remove_all(directory, ignored);
}
