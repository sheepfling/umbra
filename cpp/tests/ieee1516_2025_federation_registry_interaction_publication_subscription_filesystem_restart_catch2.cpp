#include "ieee1516_2025_federation_registry_test_support.hpp"

TEST_CASE(
    "Filesystem state image restores a same-class interaction publication and subscription in a fresh registry",
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
      L"exercise", L"publisher-subscriber", L"dual", noOpCallbackRoute());
  REQUIRE(sourceJoined.status == FederationRegistryStatus::applied);
  REQUIRE(sourceJoined.membership);
  auto const federateId = sourceJoined.membership->id;

  auto const takeOrder = source.interactionClassHandleFor(
      L"exercise", "HLAinteractionRoot.ServerAction.TakeOrder");
  REQUIRE(takeOrder.has_value());
  REQUIRE(source.setInteractionClassPublication(
      L"exercise", federateId, *takeOrder, true) ==
      umbra::detail::InteractionClassDeclarationStatus::applied);
  REQUIRE(source.setInteractionClassSubscription(
      L"exercise", federateId, *takeOrder, true) ==
      umbra::detail::InteractionClassDeclarationStatus::applied);

  std::wstring const saveLabel = L"interaction-publication-subscription-process-restart";
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
  REQUIRE(savedDeclaration.publishedInteractionClasses ==
      std::vector<std::uint64_t>{*takeOrder});
  REQUIRE(savedDeclaration.subscribedInteractionClasses.size() == 1U);
  REQUIRE(savedDeclaration.subscribedInteractionClasses.front().interactionClassHandle ==
      *takeOrder);
  REQUIRE(savedDeclaration.subscribedInteractionClasses.front().active);
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
      L"exercise", L"publisher-subscriber", L"dual", noOpCallbackRoute());
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

  auto declaration = restarted.interactionClassDeclarationFor(
      L"exercise", federateId, *takeOrder);
  REQUIRE(declaration.has_value());
  REQUIRE(declaration->published);
  REQUIRE(declaration->subscriptionActive.has_value());
  REQUIRE(*declaration->subscriptionActive);

  std::filesystem::remove_all(directory, ignored);
}
