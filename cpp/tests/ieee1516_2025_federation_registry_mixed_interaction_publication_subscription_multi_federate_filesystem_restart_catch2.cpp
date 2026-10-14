#include "ieee1516_2025_federation_registry_test_support.hpp"

TEST_CASE(
    "Filesystem state image restores a mixed interaction publication and active/passive subscriptions for multiple federates",
    "[unit][kernel][federation-registry][save-restore][durable-save][filesystem][restore]"
    "[process-restart][process-restart-interaction-declaration][interaction-declaration-state]"
    "[interaction-management][declaration-management][multi-federate-callback-ordering]"
    "[multi-federate-mixed-subscription-restore]") {
  auto const directory = temporarySaveCommitDirectory();
  std::error_code ignored;
  std::filesystem::remove_all(directory, ignored);
  auto store = std::make_shared<umbra::detail::FilesystemFederationSaveCommitStore>(
      directory);

  EmbeddedFederationRegistry source({}, store);
  REQUIRE(source.create(L"exercise", composedRestaurantDefinition()).status ==
      FederationRegistryStatus::applied);
  auto publisher = source.join(
      L"exercise", L"publisher", L"publisher", noOpCallbackRoute());
  auto subscriber = source.join(
      L"exercise", L"subscriber", L"subscriber", noOpCallbackRoute());
  auto secondSubscriber = source.join(
      L"exercise", L"second-subscriber", L"second-subscriber", noOpCallbackRoute());
  REQUIRE(publisher.status == FederationRegistryStatus::applied);
  REQUIRE(subscriber.status == FederationRegistryStatus::applied);
  REQUIRE(secondSubscriber.status == FederationRegistryStatus::applied);
  REQUIRE(publisher.membership);
  REQUIRE(subscriber.membership);
  REQUIRE(secondSubscriber.membership);
  auto const publisherId = publisher.membership->id;
  auto const subscriberId = subscriber.membership->id;
  auto const secondSubscriberId = secondSubscriber.membership->id;

  auto const publishedClass = source.interactionClassHandleFor(
      L"exercise", "HLAinteractionRoot.CustomerTransactions.CustomerSeated");
  auto const subscribedClass = source.interactionClassHandleFor(
      L"exercise", "HLAinteractionRoot.ServerAction.TakeOrder");
  auto const passiveSubscribedClass = source.interactionClassHandleFor(
      L"exercise", "HLAinteractionRoot.CustomerTransactions.FoodServed");
  auto const secondSubscriberActiveClass = source.interactionClassHandleFor(
      L"exercise", "HLAinteractionRoot.CustomerTransactions.CustomerSeated");
  auto const secondSubscriberPassiveClass = source.interactionClassHandleFor(
      L"exercise", "HLAinteractionRoot.CustomerTransactions.FoodServed.MainCourseServed");
  REQUIRE(publishedClass.has_value());
  REQUIRE(subscribedClass.has_value());
  REQUIRE(passiveSubscribedClass.has_value());
  REQUIRE(secondSubscriberActiveClass.has_value());
  REQUIRE(secondSubscriberPassiveClass.has_value());
  REQUIRE(*subscribedClass != *passiveSubscribedClass);
  REQUIRE(*secondSubscriberActiveClass != *secondSubscriberPassiveClass);
  REQUIRE(source.setInteractionClassPublication(
      L"exercise", publisherId, *publishedClass, true) ==
      umbra::detail::InteractionClassDeclarationStatus::applied);
  REQUIRE(source.setInteractionClassSubscription(
      L"exercise", subscriberId, *subscribedClass, true) ==
      umbra::detail::InteractionClassDeclarationStatus::applied);
  REQUIRE(source.setInteractionClassSubscription(
      L"exercise", subscriberId, *passiveSubscribedClass, false) ==
      umbra::detail::InteractionClassDeclarationStatus::applied);
  REQUIRE(source.setInteractionClassSubscription(
      L"exercise", secondSubscriberId, *secondSubscriberActiveClass, true) ==
      umbra::detail::InteractionClassDeclarationStatus::applied);
  REQUIRE(source.setInteractionClassSubscription(
      L"exercise", secondSubscriberId, *secondSubscriberPassiveClass, false) ==
      umbra::detail::InteractionClassDeclarationStatus::applied);

  std::wstring const saveLabel = L"interaction-mixed-multi-federate-process-restart";
  REQUIRE(source.requestFederationSave(
      L"exercise", publisherId, saveLabel).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(source.federateSaveBegun(
      L"exercise", publisherId).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(source.federateSaveBegun(
      L"exercise", subscriberId).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(source.federateSaveBegun(
      L"exercise", secondSubscriberId).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE_FALSE(source.federateSaveComplete(
      L"exercise", publisherId).saveCompletedSuccessfully);
  REQUIRE_FALSE(source.federateSaveComplete(
      L"exercise", subscriberId).saveCompletedSuccessfully);
  REQUIRE(source.federateSaveComplete(
      L"exercise", secondSubscriberId).saveCompletedSuccessfully);

  auto committed = store->load(L"exercise", saveLabel);
  REQUIRE(committed.has_value());
  auto image = umbra::detail::FederationStateImageCodec::decode(
      committed->stateImage);
  REQUIRE(image.interactionDeclarationCount == 3U);
  REQUIRE(image.interactionDeclarations.size() == 3U);
  REQUIRE(image.interactionDeclarations[0].federateId == publisherId);
  REQUIRE(image.interactionDeclarations[1].federateId == subscriberId);
  REQUIRE(image.interactionDeclarations[2].federateId == secondSubscriberId);
  REQUIRE(image.interactionDeclarations[0].publishedInteractionClasses ==
      std::vector<std::uint64_t>{*publishedClass});
  REQUIRE(image.interactionDeclarations[0].subscribedInteractionClasses.empty());
  REQUIRE(image.interactionDeclarations[1].publishedInteractionClasses.empty());
  REQUIRE(image.interactionDeclarations[1].subscribedInteractionClasses.size() == 2U);
  REQUIRE(image.interactionDeclarations[1].subscribedInteractionClasses[0].
      interactionClassHandle == std::min(*subscribedClass, *passiveSubscribedClass));
  REQUIRE(image.interactionDeclarations[1].subscribedInteractionClasses[1].
      interactionClassHandle == std::max(*subscribedClass, *passiveSubscribedClass));
  REQUIRE(image.interactionDeclarations[1].subscribedInteractionClasses[0].active ==
      (std::min(*subscribedClass, *passiveSubscribedClass) == *subscribedClass));
  REQUIRE(image.interactionDeclarations[1].subscribedInteractionClasses[1].active ==
      (std::max(*subscribedClass, *passiveSubscribedClass) == *subscribedClass));
  REQUIRE(image.interactionDeclarations[2].publishedInteractionClasses.empty());
  REQUIRE(image.interactionDeclarations[2].subscribedInteractionClasses.size() == 2U);
  REQUIRE(image.interactionDeclarations[2].subscribedInteractionClasses[0].
      interactionClassHandle ==
          std::min(*secondSubscriberActiveClass, *secondSubscriberPassiveClass));
  REQUIRE(image.interactionDeclarations[2].subscribedInteractionClasses[1].
      interactionClassHandle ==
          std::max(*secondSubscriberActiveClass, *secondSubscriberPassiveClass));
  REQUIRE(image.interactionDeclarations[2].subscribedInteractionClasses[0].active ==
      (std::min(*secondSubscriberActiveClass, *secondSubscriberPassiveClass) ==
       *secondSubscriberActiveClass));
  REQUIRE(image.interactionDeclarations[2].subscribedInteractionClasses[1].active ==
      (std::max(*secondSubscriberActiveClass, *secondSubscriberPassiveClass) ==
       *secondSubscriberActiveClass));
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
  auto restartedPublisher = restarted.join(
      L"exercise", L"publisher", L"publisher", noOpCallbackRoute());
  auto restartedSubscriber = restarted.join(
      L"exercise", L"subscriber", L"subscriber", noOpCallbackRoute());
  auto restartedSecondSubscriber = restarted.join(
      L"exercise", L"second-subscriber", L"second-subscriber", noOpCallbackRoute());
  REQUIRE(restartedPublisher.status == FederationRegistryStatus::applied);
  REQUIRE(restartedSubscriber.status == FederationRegistryStatus::applied);
  REQUIRE(restartedSecondSubscriber.status == FederationRegistryStatus::applied);
  REQUIRE(restartedPublisher.membership);
  REQUIRE(restartedSubscriber.membership);
  REQUIRE(restartedSecondSubscriber.membership);
  REQUIRE(restartedPublisher.membership->id == publisherId);
  REQUIRE(restartedSubscriber.membership->id == subscriberId);
  REQUIRE(restartedSecondSubscriber.membership->id == secondSubscriberId);
  REQUIRE(restarted.requestFederationRestore(
      L"exercise", publisherId, saveLabel).status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  REQUIRE(restarted.federateRestoreComplete(
      L"exercise", publisherId).status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  auto restored = restarted.federateRestoreComplete(
      L"exercise", subscriberId);
  auto restoredSecondSubscriber = restarted.federateRestoreComplete(
      L"exercise", secondSubscriberId);
  REQUIRE(restoredSecondSubscriber.status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  REQUIRE(restored.status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  REQUIRE(restored.interactionTransportationTypeChangeWorkItems.empty());
  REQUIRE(restoredSecondSubscriber.interactionTransportationTypeChangeWorkItems.empty());

  auto restoredPublisherDeclaration = restarted.interactionClassDeclarationFor(
      L"exercise", publisherId, *publishedClass);
  REQUIRE(restoredPublisherDeclaration.has_value());
  REQUIRE(restoredPublisherDeclaration->published);
  REQUIRE_FALSE(restoredPublisherDeclaration->subscriptionActive.has_value());
  auto restoredSubscriberDeclaration = restarted.interactionClassDeclarationFor(
      L"exercise", subscriberId, *subscribedClass);
  REQUIRE(restoredSubscriberDeclaration.has_value());
  REQUIRE_FALSE(restoredSubscriberDeclaration->published);
  REQUIRE(restoredSubscriberDeclaration->subscriptionActive.has_value());
  REQUIRE(*restoredSubscriberDeclaration->subscriptionActive);
  auto restoredPassiveSubscriberDeclaration = restarted.interactionClassDeclarationFor(
      L"exercise", subscriberId, *passiveSubscribedClass);
  REQUIRE(restoredPassiveSubscriberDeclaration.has_value());
  REQUIRE_FALSE(restoredPassiveSubscriberDeclaration->published);
  REQUIRE(restoredPassiveSubscriberDeclaration->subscriptionActive.has_value());
  REQUIRE_FALSE(*restoredPassiveSubscriberDeclaration->subscriptionActive);
  auto restoredSecondSubscriberActiveDeclaration = restarted.interactionClassDeclarationFor(
      L"exercise", secondSubscriberId, *secondSubscriberActiveClass);
  REQUIRE(restoredSecondSubscriberActiveDeclaration.has_value());
  REQUIRE_FALSE(restoredSecondSubscriberActiveDeclaration->published);
  REQUIRE(restoredSecondSubscriberActiveDeclaration->subscriptionActive.has_value());
  REQUIRE(*restoredSecondSubscriberActiveDeclaration->subscriptionActive);
  auto restoredSecondSubscriberPassiveDeclaration = restarted.interactionClassDeclarationFor(
      L"exercise", secondSubscriberId, *secondSubscriberPassiveClass);
  REQUIRE(restoredSecondSubscriberPassiveDeclaration.has_value());
  REQUIRE_FALSE(restoredSecondSubscriberPassiveDeclaration->published);
  REQUIRE(restoredSecondSubscriberPassiveDeclaration->subscriptionActive.has_value());
  REQUIRE_FALSE(*restoredSecondSubscriberPassiveDeclaration->subscriptionActive);

  auto publisherOtherClass = restarted.interactionClassDeclarationFor(
      L"exercise", publisherId, *subscribedClass);
  auto subscriberOtherClass = restarted.interactionClassDeclarationFor(
      L"exercise", subscriberId, *publishedClass);
  auto secondSubscriberOtherClass = restarted.interactionClassDeclarationFor(
      L"exercise", secondSubscriberId, *subscribedClass);
  REQUIRE(publisherOtherClass.has_value());
  REQUIRE_FALSE(publisherOtherClass->published);
  REQUIRE_FALSE(publisherOtherClass->subscriptionActive.has_value());
  REQUIRE(subscriberOtherClass.has_value());
  REQUIRE_FALSE(subscriberOtherClass->published);
  REQUIRE_FALSE(subscriberOtherClass->subscriptionActive.has_value());
  REQUIRE(secondSubscriberOtherClass.has_value());
  REQUIRE_FALSE(secondSubscriberOtherClass->published);
  REQUIRE_FALSE(secondSubscriberOtherClass->subscriptionActive.has_value());

  std::filesystem::remove_all(directory, ignored);
}
