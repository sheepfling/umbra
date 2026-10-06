#include "ieee1516_2025_federation_registry_test_support.hpp"

TEST_CASE(
    "Filesystem fresh-registry directed TSO fan-out preserves an eligible recipient and suppresses an unsubscribed recipient",
    "[unit][kernel][federation-registry][save-restore][durable-save][filesystem][restore]"
    "[process-restart][process-restart-directed-interaction-tso-fanout-positive-negative]"
    "[tso-directed-interaction-state][tso-retraction-ledger-state][directed-interaction]"
    "[directed-routing][object-visibility-state][time-management]") {
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
  auto eligible = source.join(
      L"exercise", L"owner", L"eligible-owner", noOpCallbackRoute());
  auto unsubscribed = source.join(
      L"exercise", L"observer", L"unsubscribed-observer", noOpCallbackRoute());
  REQUIRE(publisher.membership);
  REQUIRE(eligible.membership);
  REQUIRE(unsubscribed.membership);
  auto const publisherId = publisher.membership->id;
  auto const eligibleId = eligible.membership->id;
  auto const unsubscribedId = unsubscribed.membership->id;

  auto const objectClass = source.objectClassHandleFor(
      L"exercise", "HLAobjectRoot.UmbraDirectedFixtureObject");
  auto const interactionClass = source.interactionClassHandleFor(
      L"exercise", "HLAinteractionRoot.UmbraDirectedFixtureInteraction");
  auto const marker = source.attributeHandleFor(
      L"exercise", "HLAobjectRoot.UmbraDirectedFixtureObject", "DirectedTargetMarker");
  REQUIRE(objectClass);
  REQUIRE(interactionClass);
  REQUIRE(marker);

  std::set<std::uint64_t> const markerOnly{*marker};
  std::set<std::uint64_t> const directedOnly{*interactionClass};
  REQUIRE(source.setObjectClassAttributePublication(
      L"exercise", eligibleId, *objectClass, markerOnly, true) ==
      umbra::detail::ObjectClassAttributeDeclarationStatus::applied);
  REQUIRE(source.setObjectClassAttributeSubscription(
      L"exercise", publisherId, *objectClass, markerOnly, true) ==
      umbra::detail::ObjectClassAttributeDeclarationStatus::applied);
  REQUIRE(source.setObjectClassAttributeSubscription(
      L"exercise", unsubscribedId, *objectClass, markerOnly, true) ==
      umbra::detail::ObjectClassAttributeDeclarationStatus::applied);
  REQUIRE(source.publishObjectClassDirectedInteractions(
      L"exercise", publisherId, *objectClass, directedOnly) ==
      umbra::detail::DirectedInteractionDeclarationStatus::applied);
  REQUIRE(source.subscribeObjectClassDirectedInteractions(
      L"exercise", eligibleId, *objectClass, directedOnly, true) ==
      umbra::detail::DirectedInteractionDeclarationStatus::applied);
  REQUIRE(source.subscribeObjectClassDirectedInteractions(
      L"exercise", unsubscribedId, *objectClass, directedOnly, true) ==
      umbra::detail::DirectedInteractionDeclarationStatus::applied);

  auto registered = source.registerObjectInstance(
      L"exercise", eligibleId, *objectClass);
  REQUIRE(registered.status ==
      umbra::detail::ObjectInstanceRegistrationStatus::applied);
  REQUIRE(registered.objectInstanceHandle != 0U);
  auto discoveries = source.planObjectInstanceDiscoveriesForInstance(
      L"exercise", registered.objectInstanceHandle);
  REQUIRE(discoveries.size() == 2U);
  REQUIRE(source.beginObjectInstanceDiscovery(
      L"exercise", publisherId, registered.objectInstanceHandle));
  REQUIRE(source.beginObjectInstanceDiscovery(
      L"exercise", unsubscribedId, registered.objectInstanceHandle));

  std::string const markerValue{"directed-tso-fanout", 19U};
  std::vector<std::pair<std::uint64_t, rti1516_2025::VariableLengthData>> values{{
      *marker,
      rti1516_2025::VariableLengthData(markerValue.data(), markerValue.size()),
  }};
  REQUIRE(source.recordSuccessfulUpdateAttributeValues(
      L"exercise", eligibleId, registered.objectInstanceHandle, *objectClass,
      {"HLAreliable"}, &values) == FederationRegistryStatus::applied);

  auto sourcePlan = source.planReceiveOrderDirectedInteraction(
      L"exercise", publisherId, registered.objectInstanceHandle,
      *interactionClass, {});
  REQUIRE(sourcePlan.status ==
      umbra::detail::ReceiveOrderDirectedInteractionStatus::applied);
  REQUIRE(sourcePlan.recipients.size() == 2U);
  auto const eligibleSelection = std::find_if(
      sourcePlan.recipients.begin(), sourcePlan.recipients.end(),
      [eligibleId](auto const& recipient) {
        return recipient.federateId == eligibleId;
      });
  auto const unsubscribedSelection = std::find_if(
      sourcePlan.recipients.begin(), sourcePlan.recipients.end(),
      [unsubscribedId](auto const& recipient) {
        return recipient.federateId == unsubscribedId;
      });
  REQUIRE(eligibleSelection != sourcePlan.recipients.end());
  REQUIRE(unsubscribedSelection != sourcePlan.recipients.end());

  std::string const tagBytes{"fanout-tso", 10U};
  umbra::detail::TsoDirectedInteractionMessage message;
  message.producingFederateId = publisherId;
  message.objectInstanceHandle = registered.objectInstanceHandle;
  message.sentInteractionClassHandle = *interactionClass;
  message.userSuppliedTag = rti1516_2025::VariableLengthData(
      tagBytes.data(), tagBytes.size());
  message.transportationName = "HLAreliable";
  message.timestamp = std::make_shared<rti1516_2025::HLAinteger64Time>(5);
  message.recipients.push_back({
      eligibleSelection->federateId,
      eligibleSelection->objectInstanceHandle,
      eligibleSelection->receivedInteractionClassHandle,
      eligibleSelection->receivedParameterHandles,
      eligibleSelection->callbackRoute});
  message.recipients.push_back({
      unsubscribedSelection->federateId,
      unsubscribedSelection->objectInstanceHandle,
      unsubscribedSelection->receivedInteractionClassHandle,
      unsubscribedSelection->receivedParameterHandles,
      unsubscribedSelection->callbackRoute});
  auto const enqueued = source.enqueueTsoDirectedInteraction(
      L"exercise", std::move(message), {eligibleId, unsubscribedId});
  REQUIRE(enqueued.status ==
      umbra::detail::FederationTsoRegistryStatus::applied);
  REQUIRE(enqueued.queueStatus ==
      umbra::detail::TsoMessageQueueStatus::applied);
  REQUIRE(enqueued.enqueuedRecipientCount == 2U);

  auto saveAll = [&](EmbeddedFederationRegistry& registry,
                     std::wstring const& label) {
    REQUIRE(registry.requestFederationSave(
        L"exercise", publisherId, label).status ==
        umbra::detail::FederationSaveControlStatus::applied);
    REQUIRE(registry.federateSaveBegun(
        L"exercise", publisherId).status ==
        umbra::detail::FederationSaveControlStatus::applied);
    REQUIRE(registry.federateSaveBegun(
        L"exercise", eligibleId).status ==
        umbra::detail::FederationSaveControlStatus::applied);
    REQUIRE(registry.federateSaveBegun(
        L"exercise", unsubscribedId).status ==
        umbra::detail::FederationSaveControlStatus::applied);
    REQUIRE_FALSE(registry.federateSaveComplete(
        L"exercise", publisherId).saveCompletedSuccessfully);
    REQUIRE_FALSE(registry.federateSaveComplete(
        L"exercise", eligibleId).saveCompletedSuccessfully);
    REQUIRE(registry.federateSaveComplete(
        L"exercise", unsubscribedId).saveCompletedSuccessfully);
  };

  std::wstring const saveLabel =
      L"directed-interaction-tso-fanout-process-restart";
  saveAll(source, saveLabel);
  auto committed = store->load(L"exercise", saveLabel);
  REQUIRE(committed);
  auto image = umbra::detail::FederationStateImageCodec::decode(
      committed->stateImage);
  REQUIRE(image.tsoDirectedInteractionMessages.size() == 1U);
  REQUIRE(image.tsoDirectedInteractionMessages.front().messageId ==
      enqueued.messageId);
  REQUIRE(image.tsoDirectedInteractionMessages.front().recipients.size() == 2U);
  std::set<std::uint64_t> savedRecipients;
  for (auto const& recipient : image.tsoDirectedInteractionMessages.front().recipients) {
    savedRecipients.insert(recipient.receivingFederateId);
  }
  REQUIRE(savedRecipients == std::set<std::uint64_t>{eligibleId, unsubscribedId});
  REQUIRE(image.tsoRequestRetractionRecords.size() == 1U);
  REQUIRE(image.tsoRequestRetractionRecords.front().recipientStates.size() == 2U);
  for (auto const& recipient : image.tsoRequestRetractionRecords.front().recipientStates) {
    REQUIRE(recipient.state == 0U);
  }
  REQUIRE(image.tsoQueueEntries.size() == 2U);
  for (auto const& queueEntry : image.tsoQueueEntries) {
    REQUIRE(queueEntry.messageId == enqueued.messageId);
    REQUIRE(queueEntry.phase == 0U);
  }

  EmbeddedFederationRegistry restarted({}, store);
  REQUIRE(restarted.create(L"exercise", composedDirectedInteractionDefinition()).status ==
      FederationRegistryStatus::applied);
  auto restartedPublisher = restarted.join(
      L"exercise", L"publisher", L"publisher", noOpCallbackRoute());
  auto restartedEligible = restarted.join(
      L"exercise", L"owner", L"eligible-owner", noOpCallbackRoute());
  auto restartedUnsubscribed = restarted.join(
      L"exercise", L"observer", L"unsubscribed-observer", noOpCallbackRoute());
  REQUIRE(restartedPublisher.membership);
  REQUIRE(restartedEligible.membership);
  REQUIRE(restartedUnsubscribed.membership);
  REQUIRE(restartedPublisher.membership->id == publisherId);
  REQUIRE(restartedEligible.membership->id == eligibleId);
  REQUIRE(restartedUnsubscribed.membership->id == unsubscribedId);
  REQUIRE(restarted.requestFederationRestore(
      L"exercise", publisherId, saveLabel).status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  REQUIRE(restarted.federateRestoreComplete(
      L"exercise", publisherId).status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  REQUIRE(restarted.federateRestoreComplete(
      L"exercise", eligibleId).status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  REQUIRE(restarted.federateRestoreComplete(
      L"exercise", unsubscribedId).status ==
      umbra::detail::FederationRestoreControlStatus::applied);

  auto const restartedObjectClass = restarted.objectClassHandleFor(
      L"exercise", "HLAobjectRoot.UmbraDirectedFixtureObject");
  auto const restartedInteractionClass = restarted.interactionClassHandleFor(
      L"exercise", "HLAinteractionRoot.UmbraDirectedFixtureInteraction");
  auto const restartedMarker = restarted.attributeHandleFor(
      L"exercise", "HLAobjectRoot.UmbraDirectedFixtureObject", "DirectedTargetMarker");
  REQUIRE(restartedObjectClass);
  REQUIRE(restartedInteractionClass);
  REQUIRE(restartedMarker);
  REQUIRE(*restartedObjectClass == *objectClass);
  REQUIRE(*restartedInteractionClass == *interactionClass);
  REQUIRE(*restartedMarker == *marker);

  auto restoredRoute = restarted.planReceiveOrderDirectedInteraction(
      L"exercise", publisherId, registered.objectInstanceHandle,
      *restartedInteractionClass, {});
  REQUIRE(restoredRoute.status ==
      umbra::detail::ReceiveOrderDirectedInteractionStatus::applied);
  REQUIRE(restoredRoute.recipients.size() == 2U);
  REQUIRE(restarted.timestampedDirectedInteractionRecipientFor(
      L"exercise", publisherId, eligibleId, registered.objectInstanceHandle,
      *restartedInteractionClass, {}, enqueued.messageId));
  REQUIRE(restarted.timestampedDirectedInteractionRecipientFor(
      L"exercise", publisherId, unsubscribedId, registered.objectInstanceHandle,
      *restartedInteractionClass, {}, enqueued.messageId));

  REQUIRE(restarted.unsubscribeObjectClassDirectedInteractions(
      L"exercise", unsubscribedId, *restartedObjectClass,
      std::optional<std::set<std::uint64_t>>{directedOnly}) ==
      umbra::detail::DirectedInteractionDeclarationStatus::applied);
  auto routeAfterUnsubscribe = restarted.planReceiveOrderDirectedInteraction(
      L"exercise", publisherId, registered.objectInstanceHandle,
      *restartedInteractionClass, {});
  REQUIRE(routeAfterUnsubscribe.status ==
      umbra::detail::ReceiveOrderDirectedInteractionStatus::applied);
  REQUIRE(routeAfterUnsubscribe.recipients.size() == 1U);
  REQUIRE(routeAfterUnsubscribe.recipients.front().federateId == eligibleId);
  REQUIRE(restarted.timestampedDirectedInteractionRecipientFor(
      L"exercise", publisherId, eligibleId, registered.objectInstanceHandle,
      *restartedInteractionClass, {}, enqueued.messageId));
  REQUIRE_FALSE(restarted.timestampedDirectedInteractionRecipientFor(
      L"exercise", publisherId, unsubscribedId, registered.objectInstanceHandle,
      *restartedInteractionClass, {}, enqueued.messageId));

  auto eligibleDelivery = restarted.beginTsoPayloadDelivery(
      L"exercise", eligibleId, rti1516_2025::HLAinteger64Time(5), true);
  REQUIRE(eligibleDelivery.status ==
      umbra::detail::FederationTsoRegistryStatus::applied);
  REQUIRE(eligibleDelivery.deliveryStatus ==
      umbra::detail::FederationTsoDeliveryStatus::applied);
  REQUIRE(eligibleDelivery.deliveries.size() == 1U);
  auto const* eligibleDirected = std::get_if<umbra::detail::TsoDirectedInteractionDelivery>(
      &eligibleDelivery.deliveries.front());
  REQUIRE(eligibleDirected);
  REQUIRE(eligibleDirected->message.messageId == enqueued.messageId);
  REQUIRE(restarted.beginTsoInteractionCallback(
      L"exercise", eligibleId, enqueued.messageId));
  REQUIRE(restarted.completeTsoDelivery(
      L"exercise", eligibleDirected->queuedMessage).delivery.status ==
      umbra::detail::FederationTsoDeliveryStatus::applied);

  auto unsubscribedDelivery = restarted.beginTsoPayloadDelivery(
      L"exercise", unsubscribedId, rti1516_2025::HLAinteger64Time(5), true);
  REQUIRE(unsubscribedDelivery.status ==
      umbra::detail::FederationTsoRegistryStatus::applied);
  REQUIRE(unsubscribedDelivery.deliveryStatus ==
      umbra::detail::FederationTsoDeliveryStatus::applied);
  REQUIRE(unsubscribedDelivery.deliveries.size() == 1U);
  auto const* unsubscribedDirected = std::get_if<umbra::detail::TsoDirectedInteractionDelivery>(
      &unsubscribedDelivery.deliveries.front());
  REQUIRE(unsubscribedDirected);
  REQUIRE(unsubscribedDirected->message.messageId == enqueued.messageId);
  REQUIRE(restarted.finishTsoRecipientCallbackSuppressed(
      L"exercise", unsubscribedId, enqueued.messageId));
  REQUIRE_FALSE(restarted.beginTsoInteractionCallback(
      L"exercise", unsubscribedId, enqueued.messageId));
  REQUIRE(restarted.completeTsoDelivery(
      L"exercise", unsubscribedDirected->queuedMessage).delivery.status ==
      umbra::detail::FederationTsoDeliveryStatus::applied);

  auto const retracted = restarted.retractTsoMessageForProducer(
      L"exercise", publisherId, enqueued.messageId,
      std::make_shared<rti1516_2025::HLAinteger64Time>(0));
  REQUIRE(retracted.status ==
      umbra::detail::FederationTsoRegistryStatus::applied);
  REQUIRE(retracted.timestampEligible);
  REQUIRE(retracted.queueResult.status ==
      umbra::detail::TsoMessageQueueStatus::applied);
  REQUIRE(retracted.requestRetractionNotifications.size() == 1U);
  REQUIRE(retracted.requestRetractionNotifications.front().receivingFederateId ==
      eligibleId);
  REQUIRE(retracted.requestRetractionNotifications.front().messageId ==
      enqueued.messageId);
  REQUIRE(restarted.canDeliverTsoRequestRetraction(
      L"exercise", eligibleId, enqueued.messageId));
  REQUIRE_FALSE(restarted.canDeliverTsoRequestRetraction(
      L"exercise", unsubscribedId, enqueued.messageId));

  std::wstring const terminalSaveLabel =
      L"directed-interaction-tso-fanout-terminal";
  saveAll(restarted, terminalSaveLabel);
  auto terminal = store->load(L"exercise", terminalSaveLabel);
  REQUIRE(terminal);
  auto terminalImage = umbra::detail::FederationStateImageCodec::decode(
      terminal->stateImage);
  REQUIRE(terminalImage.tsoDirectedInteractionMessages.empty());
  REQUIRE(terminalImage.tsoRequestRetractionRecords.size() == 1U);
  auto const& terminalRecord = terminalImage.tsoRequestRetractionRecords.front();
  REQUIRE(terminalRecord.retractionApplied);
  REQUIRE(terminalRecord.terminal);
  REQUIRE(terminalRecord.recipientStates.size() == 2U);
  auto const eligibleState = std::find_if(
      terminalRecord.recipientStates.begin(), terminalRecord.recipientStates.end(),
      [eligibleId](auto const& recipient) {
        return recipient.receivingFederateId == eligibleId;
      });
  auto const unsubscribedState = std::find_if(
      terminalRecord.recipientStates.begin(), terminalRecord.recipientStates.end(),
      [unsubscribedId](auto const& recipient) {
        return recipient.receivingFederateId == unsubscribedId;
      });
  REQUIRE(eligibleState != terminalRecord.recipientStates.end());
  REQUIRE(unsubscribedState != terminalRecord.recipientStates.end());
  REQUIRE(eligibleState->state == 3U);
  REQUIRE(unsubscribedState->state == 2U);
  REQUIRE(terminalImage.tsoQueueEntries.size() == 2U);
  for (auto const& queueEntry : terminalImage.tsoQueueEntries) {
    REQUIRE(queueEntry.messageId == enqueued.messageId);
    REQUIRE(queueEntry.phase == 2U);
  }

  std::filesystem::remove_all(directory, ignored);
}
