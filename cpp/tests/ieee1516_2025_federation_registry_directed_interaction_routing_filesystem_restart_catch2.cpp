#include "ieee1516_2025_federation_registry_test_support.hpp"

TEST_CASE(
    "Filesystem state image restores a directed interaction target and receive-order route",
    "[unit][kernel][federation-registry][save-restore][durable-save][filesystem][restore]"
    "[process-restart][process-restart-directed-interaction-routing][interaction-declaration-state]"
    "[directed-interaction][directed-routing][object-visibility-state][object-lifecycle-state]"
    "[object-class-declaration-state][interaction-management][declaration-management]") {
  auto const directory = temporarySaveCommitDirectory();
  std::error_code ignored;
  std::filesystem::remove_all(directory, ignored);
  auto store = std::make_shared<umbra::detail::FilesystemFederationSaveCommitStore>(
      directory);

  EmbeddedFederationRegistry source({}, store);
  REQUIRE(source.create(L"exercise", composedDirectedInteractionDefinition()).status ==
      FederationRegistryStatus::applied);
  auto publisher = source.join(
      L"exercise", L"publisher", L"publisher", noOpCallbackRoute());
  auto subscriber = source.join(
      L"exercise", L"subscriber", L"subscriber", noOpCallbackRoute());
  REQUIRE(publisher.status == FederationRegistryStatus::applied);
  REQUIRE(subscriber.status == FederationRegistryStatus::applied);
  REQUIRE(publisher.membership);
  REQUIRE(subscriber.membership);
  auto const publisherId = publisher.membership->id;
  auto const subscriberId = subscriber.membership->id;

  auto const objectClass = source.objectClassHandleFor(
      L"exercise", "HLAobjectRoot.UmbraDirectedFixtureObject");
  auto const interactionClass = source.interactionClassHandleFor(
      L"exercise", "HLAinteractionRoot.UmbraDirectedFixtureInteraction");
  auto const marker = source.attributeHandleFor(
      L"exercise", "HLAobjectRoot.UmbraDirectedFixtureObject", "DirectedTargetMarker");
  REQUIRE(objectClass.has_value());
  REQUIRE(interactionClass.has_value());
  REQUIRE(marker.has_value());

  std::set<std::uint64_t> const markerOnly{*marker};
  std::set<std::uint64_t> const directedOnly{*interactionClass};
  REQUIRE(source.setObjectClassAttributePublication(
      L"exercise", publisherId, *objectClass, markerOnly, true) ==
      umbra::detail::ObjectClassAttributeDeclarationStatus::applied);
  REQUIRE(source.setObjectClassAttributeSubscription(
      L"exercise", subscriberId, *objectClass, markerOnly, true) ==
      umbra::detail::ObjectClassAttributeDeclarationStatus::applied);
  REQUIRE(source.publishObjectClassDirectedInteractions(
      L"exercise", publisherId, *objectClass, directedOnly) ==
      umbra::detail::DirectedInteractionDeclarationStatus::applied);
  REQUIRE(source.subscribeObjectClassDirectedInteractions(
      L"exercise", subscriberId, *objectClass, directedOnly, true) ==
      umbra::detail::DirectedInteractionDeclarationStatus::applied);

  auto registered = source.registerObjectInstance(
      L"exercise", publisherId, *objectClass);
  REQUIRE(registered.status ==
      umbra::detail::ObjectInstanceRegistrationStatus::applied);
  REQUIRE(registered.objectInstanceHandle != 0U);
  auto discoveries = source.planObjectInstanceDiscoveriesForInstance(
      L"exercise", registered.objectInstanceHandle);
  REQUIRE(discoveries.size() == 1U);
  REQUIRE(discoveries.front().receivingFederateId == subscriberId);
  REQUIRE(source.beginObjectInstanceDiscovery(
      L"exercise", subscriberId, registered.objectInstanceHandle).has_value());

  std::string const markerValue{"directed-target", 15U};
  std::vector<std::pair<std::uint64_t, rti1516_2025::VariableLengthData>> values{{
      *marker,
      rti1516_2025::VariableLengthData(markerValue.data(), markerValue.size()),
  }};
  REQUIRE(source.recordSuccessfulUpdateAttributeValues(
      L"exercise", publisherId, registered.objectInstanceHandle, *objectClass,
      {"HLAreliable"}, &values) == FederationRegistryStatus::applied);

  std::wstring const saveLabel = L"directed-interaction-routing-process-restart";
  REQUIRE(source.requestFederationSave(
      L"exercise", publisherId, saveLabel).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(source.federateSaveBegun(
      L"exercise", publisherId).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(source.federateSaveBegun(
      L"exercise", subscriberId).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE_FALSE(source.federateSaveComplete(
      L"exercise", publisherId).saveCompletedSuccessfully);
  REQUIRE(source.federateSaveComplete(
      L"exercise", subscriberId).saveCompletedSuccessfully);

  auto committed = store->load(L"exercise", saveLabel);
  REQUIRE(committed.has_value());
  auto image = umbra::detail::FederationStateImageCodec::decode(
      committed->stateImage);
  REQUIRE(image.objects.size() == 1U);
  REQUIRE(image.objects.front().handle == registered.objectInstanceHandle);
  REQUIRE(image.objects.front().knownObjectClassHandlesByFederate.size() == 2U);
  REQUIRE(image.objects.front().attributeValuesPresent);
  REQUIRE(image.interactionDeclarations.size() == 2U);
  REQUIRE(image.interactionDeclarations[0]
              .publishedObjectClassDirectedInteractions.size() == 1U);
  REQUIRE(image.interactionDeclarations[1]
              .subscribedObjectClassDirectedInteractions.size() == 1U);

  EmbeddedFederationRegistry restarted({}, store);
  REQUIRE(restarted.create(L"exercise", composedDirectedInteractionDefinition()).status ==
      FederationRegistryStatus::applied);
  auto restartedPublisher = restarted.join(
      L"exercise", L"publisher", L"publisher", noOpCallbackRoute());
  auto restartedSubscriber = restarted.join(
      L"exercise", L"subscriber", L"subscriber", noOpCallbackRoute());
  REQUIRE(restartedPublisher.status == FederationRegistryStatus::applied);
  REQUIRE(restartedSubscriber.status == FederationRegistryStatus::applied);
  REQUIRE(restartedPublisher.membership);
  REQUIRE(restartedSubscriber.membership);
  REQUIRE(restartedPublisher.membership->id == publisherId);
  REQUIRE(restartedSubscriber.membership->id == subscriberId);
  REQUIRE(restarted.requestFederationRestore(
      L"exercise", publisherId, saveLabel).status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  REQUIRE(restarted.federateRestoreComplete(
      L"exercise", publisherId).status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  auto restored = restarted.federateRestoreComplete(
      L"exercise", subscriberId);
  REQUIRE(restored.status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  REQUIRE(restored.interactionTransportationTypeChangeWorkItems.empty());

  auto restoredPlan = restarted.planReceiveOrderDirectedInteraction(
      L"exercise", publisherId, registered.objectInstanceHandle,
      *interactionClass, {});
  REQUIRE(restoredPlan.status ==
      umbra::detail::ReceiveOrderDirectedInteractionStatus::applied);
  REQUIRE(restoredPlan.transportationName == "HLAreliable");
  REQUIRE(restoredPlan.recipients.size() == 1U);
  REQUIRE(restoredPlan.recipients.front().federateId == subscriberId);
  REQUIRE(restoredPlan.recipients.front().objectInstanceHandle ==
      registered.objectInstanceHandle);
  REQUIRE(restoredPlan.recipients.front().receivedInteractionClassHandle ==
      *interactionClass);
  REQUIRE(restoredPlan.recipients.front().callbackRoute);

  std::wstring const roundTripLabel =
      L"directed-interaction-routing-round-trip";
  REQUIRE(restarted.requestFederationSave(
      L"exercise", publisherId, roundTripLabel).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(restarted.federateSaveBegun(
      L"exercise", publisherId).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(restarted.federateSaveBegun(
      L"exercise", subscriberId).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE_FALSE(restarted.federateSaveComplete(
      L"exercise", publisherId).saveCompletedSuccessfully);
  REQUIRE(restarted.federateSaveComplete(
      L"exercise", subscriberId).saveCompletedSuccessfully);
  auto roundTrip = store->load(L"exercise", roundTripLabel);
  REQUIRE(roundTrip.has_value());
  auto roundTripImage = umbra::detail::FederationStateImageCodec::decode(
      roundTrip->stateImage);
  REQUIRE(roundTripImage.objects.size() == 1U);
  REQUIRE(roundTripImage.objects.front().handle == registered.objectInstanceHandle);
  REQUIRE(roundTripImage.objects.front().attributeValuesPresent);
  REQUIRE(roundTripImage.interactionDeclarations.size() == 2U);
  REQUIRE(roundTripImage.interactionDeclarations[0]
              .publishedObjectClassDirectedInteractions.size() == 1U);
  REQUIRE(roundTripImage.interactionDeclarations[1]
              .subscribedObjectClassDirectedInteractions.size() == 1U);
  REQUIRE(roundTripImage.interactionDeclarations[1]
              .subscribedObjectClassDirectedInteractions.front().active);

  std::filesystem::remove_all(directory, ignored);
}
