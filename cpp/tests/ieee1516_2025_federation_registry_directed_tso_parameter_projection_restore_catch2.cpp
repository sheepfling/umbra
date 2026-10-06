#include "ieee1516_2025_federation_registry_test_support.hpp"

TEST_CASE(
    "Filesystem fresh-registry parameterized directed TSO preserves parameter projection and timestamped order metadata",
    "[unit][kernel][federation-registry][save-restore][durable-save][filesystem][restore]"
    "[process-restart][process-restart-directed-interaction-tso-parameter-projection]"
    "[tso-directed-interaction-state][tso-payload-state][directed-interaction]"
    "[directed-routing][object-visibility-state][time-management]") {
  auto const directory = temporarySaveCommitDirectory();
  std::error_code ignored;
  std::filesystem::remove_all(directory, ignored);
  auto store = std::make_shared<umbra::detail::FilesystemFederationSaveCommitStore>(
      directory);

  EmbeddedFederationRegistry source({}, store);
  REQUIRE(source.create(
      L"exercise", composedParameterizedDirectedInteractionDefinition()).status ==
      FederationRegistryStatus::applied);
  auto publisher = source.join(
      L"exercise", L"publisher", L"publisher", noOpCallbackRoute());
  auto owner = source.join(
      L"exercise", L"owner", L"owner", noOpCallbackRoute());
  REQUIRE(publisher.membership);
  REQUIRE(owner.membership);
  auto const publisherId = publisher.membership->id;
  auto const ownerId = owner.membership->id;

  auto const objectClass = source.objectClassHandleFor(
      L"exercise", "HLAobjectRoot.UmbraDirectedParameterFixtureObject");
  auto const interactionClass = source.interactionClassHandleFor(
      L"exercise", "HLAinteractionRoot.UmbraDirectedParameterFixtureInteraction");
  auto const marker = source.attributeHandleFor(
      L"exercise", "HLAobjectRoot.UmbraDirectedParameterFixtureObject",
      "DirectedTargetMarker");
  auto const payload = source.parameterHandleFor(
      L"exercise", "HLAinteractionRoot.UmbraDirectedParameterFixtureInteraction",
      "DirectedPayload");
  REQUIRE(objectClass);
  REQUIRE(interactionClass);
  REQUIRE(marker);
  REQUIRE(payload);
  REQUIRE(source.parameterNameFor(
      L"exercise", "HLAinteractionRoot.UmbraDirectedParameterFixtureInteraction",
      *payload) == std::optional<std::string>{"DirectedPayload"});

  std::set<std::uint64_t> const markerOnly{*marker};
  std::set<std::uint64_t> const directedOnly{*interactionClass};
  REQUIRE(source.setObjectClassAttributePublication(
      L"exercise", ownerId, *objectClass, markerOnly, true) ==
      umbra::detail::ObjectClassAttributeDeclarationStatus::applied);
  REQUIRE(source.setObjectClassAttributeSubscription(
      L"exercise", publisherId, *objectClass, markerOnly, true) ==
      umbra::detail::ObjectClassAttributeDeclarationStatus::applied);
  REQUIRE(source.publishObjectClassDirectedInteractions(
      L"exercise", publisherId, *objectClass, directedOnly) ==
      umbra::detail::DirectedInteractionDeclarationStatus::applied);
  REQUIRE(source.subscribeObjectClassDirectedInteractions(
      L"exercise", ownerId, *objectClass, directedOnly, false) ==
      umbra::detail::DirectedInteractionDeclarationStatus::applied);

  auto registered = source.registerObjectInstance(
      L"exercise", ownerId, *objectClass);
  REQUIRE(registered.status ==
      umbra::detail::ObjectInstanceRegistrationStatus::applied);
  REQUIRE(registered.objectInstanceHandle != 0U);
  auto discoveries = source.planObjectInstanceDiscoveriesForInstance(
      L"exercise", registered.objectInstanceHandle);
  REQUIRE(discoveries.size() == 1U);
  REQUIRE(source.beginObjectInstanceDiscovery(
      L"exercise", publisherId, registered.objectInstanceHandle));

  std::string const markerValue{"parameterized-directed", 22U};
  std::vector<std::pair<std::uint64_t, rti1516_2025::VariableLengthData>> values{{
      *marker,
      rti1516_2025::VariableLengthData(markerValue.data(), markerValue.size()),
  }};
  REQUIRE(source.recordSuccessfulUpdateAttributeValues(
      L"exercise", ownerId, registered.objectInstanceHandle, *objectClass,
      {"HLAreliable"}, &values) == FederationRegistryStatus::applied);

  auto sourcePlan = source.planReceiveOrderDirectedInteraction(
      L"exercise", publisherId, registered.objectInstanceHandle,
      *interactionClass, {*payload});
  REQUIRE(sourcePlan.status ==
      umbra::detail::ReceiveOrderDirectedInteractionStatus::applied);
  REQUIRE(sourcePlan.transportationName == "HLAreliable");
  REQUIRE(sourcePlan.preferredOrderType == rti1516_2025::TIMESTAMP);
  REQUIRE(sourcePlan.recipients.size() == 1U);
  REQUIRE(sourcePlan.recipients.front().federateId == ownerId);
  REQUIRE(sourcePlan.recipients.front().receivedParameterHandles ==
      std::set<std::uint64_t>{*payload});

  std::string const payloadBytes{"parameterized-directed", 22U};
  std::string const tagBytes{"parameter-tso", 13U};
  umbra::detail::TsoDirectedInteractionMessage message;
  message.producingFederateId = publisherId;
  message.objectInstanceHandle = registered.objectInstanceHandle;
  message.sentInteractionClassHandle = *interactionClass;
  message.sentParameterHandles = {*payload};
  message.parameters = {{
      *payload,
      rti1516_2025::VariableLengthData(payloadBytes.data(), payloadBytes.size()),
  }};
  message.userSuppliedTag = rti1516_2025::VariableLengthData(
      tagBytes.data(), tagBytes.size());
  message.transportationName = sourcePlan.transportationName;
  message.sentOrderType = rti1516_2025::TIMESTAMP;
  message.receivedOrderType = rti1516_2025::TIMESTAMP;
  auto const& selected = sourcePlan.recipients.front();
  message.recipients.push_back({
      selected.federateId,
      selected.objectInstanceHandle,
      selected.receivedInteractionClassHandle,
      selected.receivedParameterHandles,
      selected.callbackRoute});
  message.timestamp = std::make_shared<rti1516_2025::HLAinteger64Time>(7);
  auto const enqueued = source.enqueueTsoDirectedInteraction(
      L"exercise", std::move(message), {ownerId});
  REQUIRE(enqueued.status ==
      umbra::detail::FederationTsoRegistryStatus::applied);
  REQUIRE(enqueued.queueStatus ==
      umbra::detail::TsoMessageQueueStatus::applied);
  REQUIRE(enqueued.enqueuedRecipientCount == 1U);

  auto saveAll = [&](EmbeddedFederationRegistry& registry,
                     std::wstring const& label) {
    REQUIRE(registry.requestFederationSave(
        L"exercise", publisherId, label).status ==
        umbra::detail::FederationSaveControlStatus::applied);
    REQUIRE(registry.federateSaveBegun(
        L"exercise", publisherId).status ==
        umbra::detail::FederationSaveControlStatus::applied);
    REQUIRE(registry.federateSaveBegun(
        L"exercise", ownerId).status ==
        umbra::detail::FederationSaveControlStatus::applied);
    REQUIRE_FALSE(registry.federateSaveComplete(
        L"exercise", publisherId).saveCompletedSuccessfully);
    REQUIRE(registry.federateSaveComplete(
        L"exercise", ownerId).saveCompletedSuccessfully);
  };

  std::wstring const saveLabel =
      L"directed-interaction-tso-parameter-projection-process-restart";
  saveAll(source, saveLabel);
  auto committed = store->load(L"exercise", saveLabel);
  REQUIRE(committed);
  auto image = umbra::detail::FederationStateImageCodec::decode(
      committed->stateImage);
  REQUIRE(image.tsoDirectedInteractionMessages.size() == 1U);
  auto const& savedMessage = image.tsoDirectedInteractionMessages.front();
  REQUIRE(savedMessage.messageId == enqueued.messageId);
  REQUIRE(savedMessage.sentParameterHandles == std::vector<std::uint64_t>{*payload});
  REQUIRE(savedMessage.parameters.size() == 1U);
  REQUIRE(savedMessage.parameters.front().parameterHandle == *payload);
  REQUIRE(savedMessage.parameters.front().value == payloadBytes);
  REQUIRE(savedMessage.userSuppliedTag == tagBytes);
  REQUIRE(savedMessage.transportationName == "HLAreliable");
  REQUIRE(savedMessage.sentOrderType ==
      static_cast<std::uint32_t>(rti1516_2025::TIMESTAMP));
  REQUIRE(savedMessage.receivedOrderType ==
      static_cast<std::uint32_t>(rti1516_2025::TIMESTAMP));
  REQUIRE(savedMessage.recipients.size() == 1U);
  REQUIRE(savedMessage.recipients.front().receivingFederateId == ownerId);
  REQUIRE(savedMessage.recipients.front().receivedParameterHandles ==
      std::vector<std::uint64_t>{*payload});
  REQUIRE(image.tsoRequestRetractionRecords.size() == 1U);
  REQUIRE(image.tsoQueueEntries.size() == 1U);
  REQUIRE(image.tsoQueueEntries.front().recipientFederateId == ownerId);
  REQUIRE(image.tsoQueueEntries.front().phase == 0U);

  EmbeddedFederationRegistry restarted({}, store);
  REQUIRE(restarted.create(
      L"exercise", composedParameterizedDirectedInteractionDefinition()).status ==
      FederationRegistryStatus::applied);
  auto restartedPublisher = restarted.join(
      L"exercise", L"publisher", L"publisher", noOpCallbackRoute());
  auto restartedOwner = restarted.join(
      L"exercise", L"owner", L"owner", noOpCallbackRoute());
  REQUIRE(restartedPublisher.membership);
  REQUIRE(restartedOwner.membership);
  REQUIRE(restartedPublisher.membership->id == publisherId);
  REQUIRE(restartedOwner.membership->id == ownerId);
  REQUIRE(restarted.requestFederationRestore(
      L"exercise", publisherId, saveLabel).status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  REQUIRE(restarted.federateRestoreComplete(
      L"exercise", publisherId).status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  REQUIRE(restarted.federateRestoreComplete(
      L"exercise", ownerId).status ==
      umbra::detail::FederationRestoreControlStatus::applied);

  auto const restartedObjectClass = restarted.objectClassHandleFor(
      L"exercise", "HLAobjectRoot.UmbraDirectedParameterFixtureObject");
  auto const restartedInteractionClass = restarted.interactionClassHandleFor(
      L"exercise", "HLAinteractionRoot.UmbraDirectedParameterFixtureInteraction");
  auto const restartedPayload = restarted.parameterHandleFor(
      L"exercise", "HLAinteractionRoot.UmbraDirectedParameterFixtureInteraction",
      "DirectedPayload");
  REQUIRE(restartedObjectClass);
  REQUIRE(restartedInteractionClass);
  REQUIRE(restartedPayload);
  REQUIRE(*restartedObjectClass == *objectClass);
  REQUIRE(*restartedInteractionClass == *interactionClass);
  REQUIRE(*restartedPayload == *payload);

  auto restoredRoute = restarted.planReceiveOrderDirectedInteraction(
      L"exercise", publisherId, registered.objectInstanceHandle,
      *restartedInteractionClass, {*restartedPayload});
  REQUIRE(restoredRoute.status ==
      umbra::detail::ReceiveOrderDirectedInteractionStatus::applied);
  REQUIRE(restoredRoute.transportationName == "HLAreliable");
  REQUIRE(restoredRoute.preferredOrderType == rti1516_2025::TIMESTAMP);
  REQUIRE(restoredRoute.recipients.size() == 1U);
  REQUIRE(restoredRoute.recipients.front().federateId == ownerId);
  REQUIRE(restoredRoute.recipients.front().receivedParameterHandles ==
      std::set<std::uint64_t>{*restartedPayload});
  auto restoredRecipient = restarted.timestampedDirectedInteractionRecipientFor(
      L"exercise", publisherId, ownerId, registered.objectInstanceHandle,
      *restartedInteractionClass, {*restartedPayload}, enqueued.messageId);
  REQUIRE(restoredRecipient);
  REQUIRE(restoredRecipient->receivedParameterHandles ==
      std::set<std::uint64_t>{*restartedPayload});

  auto delivery = restarted.beginTsoPayloadDelivery(
      L"exercise", ownerId, rti1516_2025::HLAinteger64Time(7), true);
  REQUIRE(delivery.status ==
      umbra::detail::FederationTsoRegistryStatus::applied);
  REQUIRE(delivery.deliveryStatus ==
      umbra::detail::FederationTsoDeliveryStatus::applied);
  REQUIRE(delivery.deliveries.size() == 1U);
  auto const* directed = std::get_if<umbra::detail::TsoDirectedInteractionDelivery>(
      &delivery.deliveries.front());
  REQUIRE(directed);
  REQUIRE(directed->message.messageId == enqueued.messageId);
  REQUIRE(directed->message.sentParameterHandles ==
      std::vector<std::uint64_t>{*restartedPayload});
  REQUIRE(directed->message.parameters.size() == 1U);
  REQUIRE(directed->message.parameters.front().first == *restartedPayload);
  REQUIRE(std::string(
              static_cast<char const*>(directed->message.parameters.front().second.data()),
              payloadBytes.size()) == payloadBytes);
  REQUIRE(directed->message.transportationName == "HLAreliable");
  REQUIRE(directed->message.sentOrderType == rti1516_2025::TIMESTAMP);
  REQUIRE(directed->message.receivedOrderType == rti1516_2025::TIMESTAMP);
  REQUIRE(restarted.beginTsoInteractionCallback(
      L"exercise", ownerId, enqueued.messageId));
  REQUIRE(restarted.completeTsoDelivery(
      L"exercise", directed->queuedMessage).delivery.status ==
      umbra::detail::FederationTsoDeliveryStatus::applied);

  auto const retracted = restarted.retractTsoMessageForProducer(
      L"exercise", publisherId, enqueued.messageId,
      std::make_shared<rti1516_2025::HLAinteger64Time>(0));
  REQUIRE(retracted.status ==
      umbra::detail::FederationTsoRegistryStatus::applied);
  REQUIRE(retracted.timestampEligible);
  REQUIRE(retracted.requestRetractionNotifications.size() == 1U);
  REQUIRE(retracted.requestRetractionNotifications.front().receivingFederateId ==
      ownerId);
  REQUIRE(restarted.canDeliverTsoRequestRetraction(
      L"exercise", ownerId, enqueued.messageId));

  std::filesystem::remove_all(directory, ignored);
}
