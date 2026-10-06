#include "ieee1516_2025_federation_registry_test_support.hpp"

TEST_CASE(
    "Filesystem fresh-registry restore rehydrates one timestamped object-deletion payload across queued and in-transit recipients",
    "[unit][kernel][federation-registry][save-restore][durable-save][filesystem][restore]"
    "[process-restart][tso-queue-state][tso-payload-state][tso-object-deletion-state]") {
  auto const directory = temporarySaveCommitDirectory();
  std::error_code ignored;
  std::filesystem::remove_all(directory, ignored);
  auto store = std::make_shared<umbra::detail::FilesystemFederationSaveCommitStore>(
      directory);

  EmbeddedFederationRegistry source({}, store);
  REQUIRE(source.create(L"exercise", composedRestaurantDefinition()).status ==
      FederationRegistryStatus::applied);
  auto producer = source.join(
      L"exercise", L"producer", L"producer", noOpCallbackRoute());
  auto receiverATime = std::make_shared<umbra::detail::FederateTimeState>(
      L"HLAinteger64Time",
      std::make_shared<rti1516_2025::HLAinteger64Time>(0));
  auto receiverA = source.joinWithTimeState(
      L"exercise",
      receiverATime,
      L"receiver-a",
      L"receiver-a",
      noOpCallbackRoute());
  auto receiverBTime = std::make_shared<umbra::detail::FederateTimeState>(
      L"HLAinteger64Time",
      std::make_shared<rti1516_2025::HLAinteger64Time>(0));
  auto receiverB = source.joinWithTimeState(
      L"exercise",
      receiverBTime,
      L"receiver-b",
      L"receiver-b",
      noOpCallbackRoute());
  REQUIRE(producer.membership);
  REQUIRE(receiverA.membership);
  REQUIRE(receiverB.membership);

  auto const server = source.objectClassHandleFor(
      L"exercise", "HLAobjectRoot.Employee.Server");
  auto const deletePrivilege = source.attributeHandleFor(
      L"exercise", "HLAobjectRoot.Employee.Server", "HLAprivilegeToDeleteObject");
  REQUIRE(server.has_value());
  REQUIRE(deletePrivilege.has_value());
  std::set<std::uint64_t> const deleteAttribute{*deletePrivilege};
  REQUIRE(source.setObjectClassAttributePublication(
      L"exercise", producer.membership->id, *server, deleteAttribute, true) ==
      umbra::detail::ObjectClassAttributeDeclarationStatus::applied);
  REQUIRE(source.setObjectClassAttributeSubscription(
      L"exercise", receiverA.membership->id, *server, deleteAttribute, true) ==
      umbra::detail::ObjectClassAttributeDeclarationStatus::applied);
  REQUIRE(source.setObjectClassAttributeSubscription(
      L"exercise", receiverB.membership->id, *server, deleteAttribute, true) ==
      umbra::detail::ObjectClassAttributeDeclarationStatus::applied);

  auto registered = source.registerObjectInstance(
      L"exercise", producer.membership->id, *server);
  REQUIRE(registered.status ==
      umbra::detail::ObjectInstanceRegistrationStatus::applied);
  REQUIRE(registered.objectInstanceHandle != 0U);
  auto discoveries = source.planObjectInstanceDiscoveriesForInstance(
      L"exercise", registered.objectInstanceHandle);
  REQUIRE(discoveries.size() == 2U);
  for (auto const& discovery : discoveries) {
    REQUIRE(source.beginObjectInstanceDiscovery(
        L"exercise",
        discovery.receivingFederateId,
        discovery.objectInstanceHandle)
                 .has_value());
  }

  std::string const valueBytes{"\x71\x72", 2U};
  std::vector<std::pair<std::uint64_t, rti1516_2025::VariableLengthData>> values{{
      *deletePrivilege,
      rti1516_2025::VariableLengthData(valueBytes.data(), valueBytes.size()),
  }};
  REQUIRE(source.recordSuccessfulUpdateAttributeValues(
      L"exercise",
      producer.membership->id,
      registered.objectInstanceHandle,
      *server,
      {"HLAreliable"},
      &values) == FederationRegistryStatus::applied);

  std::string const tagBytes{"deletefs", 8U};
  umbra::detail::TsoObjectDeletionMessage deletion;
  deletion.userSuppliedTag = rti1516_2025::VariableLengthData(
      tagBytes.data(), tagBytes.size());
  deletion.timestamp =
      std::make_shared<rti1516_2025::HLAinteger64Time>(5);
  deletion.sentOrderType = rti1516_2025::TIMESTAMP;
  auto const enqueued = source.enqueueTsoObjectDeletion(
      L"exercise",
      producer.membership->id,
      registered.objectInstanceHandle,
      std::move(deletion),
      {receiverA.membership->id, receiverB.membership->id});
  REQUIRE(enqueued.status == umbra::detail::FederationTsoRegistryStatus::applied);
  REQUIRE(enqueued.queueStatus == umbra::detail::TsoMessageQueueStatus::applied);
  REQUIRE(enqueued.deletionStatus ==
      umbra::detail::ObjectInstanceDeletionStatus::applied);
  REQUIRE(enqueued.enqueuedRecipientCount == 2U);
  REQUIRE(enqueued.recipients.size() == 2U);

  auto inTransit = source.beginTsoPayloadDelivery(
      L"exercise",
      receiverA.membership->id,
      rti1516_2025::HLAinteger64Time(5),
      true);
  REQUIRE(inTransit.status == umbra::detail::FederationTsoRegistryStatus::applied);
  REQUIRE(inTransit.deliveryStatus ==
      umbra::detail::FederationTsoDeliveryStatus::applied);
  REQUIRE(inTransit.deliveries.size() == 1U);
  auto const* inTransitDeletion =
      std::get_if<umbra::detail::TsoObjectDeletionDelivery>(
          &inTransit.deliveries.front());
  REQUIRE(inTransitDeletion != nullptr);
  REQUIRE(inTransitDeletion->message.messageId == enqueued.messageId);
  auto const inTransitMessage = inTransitDeletion->queuedMessage;

  std::wstring const saveLabel = L"deletion-tso-payload-process-restart";
  REQUIRE(source.requestFederationSave(
      L"exercise", receiverA.membership->id, saveLabel)
      .status == umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(source.federateSaveBegun(
      L"exercise", receiverA.membership->id).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(source.federateSaveBegun(
      L"exercise", receiverB.membership->id).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(source.federateSaveBegun(
      L"exercise", producer.membership->id).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE_FALSE(source.federateSaveComplete(
      L"exercise", receiverA.membership->id).saveCompletedSuccessfully);
  REQUIRE_FALSE(source.federateSaveComplete(
      L"exercise", receiverB.membership->id).saveCompletedSuccessfully);
  REQUIRE(source.federateSaveComplete(
      L"exercise", producer.membership->id).saveCompletedSuccessfully);

  auto committed = store->load(L"exercise", saveLabel);
  REQUIRE(committed.has_value());
  auto image = umbra::detail::FederationStateImageCodec::decode(
      committed->stateImage);
  REQUIRE(image.objects.size() == 1U);
  auto const& savedObject = image.objects.front();
  REQUIRE(savedObject.handle == registered.objectInstanceHandle);
  REQUIRE(savedObject.pendingTimestampedDeletionMessageId == enqueued.messageId);
  REQUIRE(savedObject.pendingTimestampedRemovalFederateIds ==
      std::vector<std::uint64_t>{receiverA.membership->id, receiverB.membership->id});
  REQUIRE(savedObject.attributeValuesPresent);
  REQUIRE(savedObject.attributeValues.size() == 1U);
  REQUIRE(savedObject.attributeValues.front().attributeHandle == *deletePrivilege);
  REQUIRE(savedObject.attributeValues.front().value == valueBytes);
  REQUIRE(image.tsoObjectDeletionMessages.size() == 1U);
  auto const& savedDeletion = image.tsoObjectDeletionMessages.front();
  REQUIRE((savedDeletion.messageId == enqueued.messageId &&
      savedDeletion.sentOrderType ==
          static_cast<std::uint32_t>(rti1516_2025::TIMESTAMP)));
  REQUIRE(savedDeletion.objectInstanceHandle == registered.objectInstanceHandle);
  REQUIRE(savedDeletion.userSuppliedTag == tagBytes);
  REQUIRE(savedDeletion.recipients.size() == 2U);
  REQUIRE(savedDeletion.timestampEncoding.has_value());
  REQUIRE(savedDeletion.reconstitution.has_value());
  REQUIRE(savedDeletion.reconstitution->object.handle == registered.objectInstanceHandle);
  REQUIRE(savedDeletion.reconstitution->object.attributeValues.size() == 1U);
  REQUIRE(savedDeletion.reconstitution->knownObjectClassHandlesByFederate.size() == 3U);
  REQUIRE(image.tsoRequestRetractionRecords.size() == 1U);
  REQUIRE(image.tsoRequestRetractionRecords.front().messageId == enqueued.messageId);
  REQUIRE(image.tsoRequestRetractionRecords.front().recipientStates.size() == 2U);
  REQUIRE(image.tsoQueueEntries.size() == 2U);
  auto const phaseFor = [&](std::uint64_t recipientId) {
    auto const entry = std::find_if(
        image.tsoQueueEntries.begin(),
        image.tsoQueueEntries.end(),
        [recipientId, messageId = enqueued.messageId](
            umbra::detail::FederationStateImageTsoQueueEntry const& candidate) {
          return candidate.messageId == messageId &&
              candidate.recipientFederateId == recipientId;
        });
    REQUIRE(entry != image.tsoQueueEntries.end());
    return entry->phase;
  };
  REQUIRE(phaseFor(receiverA.membership->id) == 1U);
  REQUIRE(phaseFor(receiverB.membership->id) == 0U);

  EmbeddedFederationRegistry restarted({}, store);
  REQUIRE(restarted.create(L"exercise", composedRestaurantDefinition()).status ==
      FederationRegistryStatus::applied);
  auto restartedProducer = restarted.join(
      L"exercise", L"producer", L"producer", noOpCallbackRoute());
  auto restartedReceiverA = restarted.joinWithTimeState(
      L"exercise",
      std::make_shared<umbra::detail::FederateTimeState>(
          L"HLAinteger64Time",
          std::make_shared<rti1516_2025::HLAinteger64Time>(0)),
      L"receiver-a",
      L"receiver-a",
      noOpCallbackRoute());
  auto restartedReceiverB = restarted.joinWithTimeState(
      L"exercise",
      std::make_shared<umbra::detail::FederateTimeState>(
          L"HLAinteger64Time",
          std::make_shared<rti1516_2025::HLAinteger64Time>(0)),
      L"receiver-b",
      L"receiver-b",
      noOpCallbackRoute());
  REQUIRE(restartedProducer.membership);
  REQUIRE(restartedReceiverA.membership);
  REQUIRE(restartedReceiverB.membership);
  REQUIRE(restartedProducer.membership->id == producer.membership->id);
  REQUIRE(restartedReceiverA.membership->id == receiverA.membership->id);
  REQUIRE(restartedReceiverB.membership->id == receiverB.membership->id);

  REQUIRE(restarted.requestFederationRestore(
      L"exercise", restartedReceiverA.membership->id, saveLabel)
      .status == umbra::detail::FederationRestoreControlStatus::applied);
  REQUIRE(restarted.federateRestoreComplete(
      L"exercise", restartedReceiverA.membership->id).status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  REQUIRE(restarted.federateRestoreComplete(
      L"exercise", restartedReceiverB.membership->id).status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  REQUIRE(restarted.federateRestoreComplete(
      L"exercise", restartedProducer.membership->id).status ==
      umbra::detail::FederationRestoreControlStatus::applied);

  REQUIRE(restarted.completeTsoDelivery(
      L"exercise", inTransitMessage).delivery.status ==
      umbra::detail::FederationTsoDeliveryStatus::applied);
  auto restored = restarted.beginTsoPayloadDelivery(
      L"exercise",
      restartedReceiverB.membership->id,
      rti1516_2025::HLAinteger64Time(5),
      true);
  REQUIRE(restored.status == umbra::detail::FederationTsoRegistryStatus::applied);
  REQUIRE(restored.deliveryStatus ==
      umbra::detail::FederationTsoDeliveryStatus::applied);
  REQUIRE(restored.deliveries.size() == 1U);
  auto const* restoredDeletion =
      std::get_if<umbra::detail::TsoObjectDeletionDelivery>(
          &restored.deliveries.front());
  REQUIRE(restoredDeletion != nullptr);
  REQUIRE((restoredDeletion->message.messageId == enqueued.messageId &&
      restoredDeletion->message.sentOrderType == rti1516_2025::TIMESTAMP));
  REQUIRE(restoredDeletion->message.objectInstanceHandle ==
      registered.objectInstanceHandle);
  REQUIRE(restoredDeletion->message.userSuppliedTag.size() == tagBytes.size());
  REQUIRE(restoredDeletion->message.recipients.size() == 2U);
  auto const recipient = std::find_if(
      restoredDeletion->message.recipients.begin(),
      restoredDeletion->message.recipients.end(),
      [id = restartedReceiverB.membership->id](
          umbra::detail::TsoObjectDeletionRecipient const& candidate) {
        return candidate.receivingFederateId == id;
      });
  REQUIRE(recipient != restoredDeletion->message.recipients.end());
  REQUIRE(recipient->objectInstanceHandle == registered.objectInstanceHandle);
  REQUIRE(recipient->callbackRoute);
  REQUIRE(restoredDeletion->message.timestamp->implementationName() ==
      L"HLAinteger64Time");
  auto const* restoredTimestamp = dynamic_cast<rti1516_2025::HLAinteger64Time const*>(
      restoredDeletion->message.timestamp.get());
  REQUIRE(restoredTimestamp != nullptr);
  REQUIRE(restoredTimestamp->getTime() == 5);
  REQUIRE(restarted.completeTsoDelivery(
      L"exercise", restoredDeletion->queuedMessage).delivery.status ==
      umbra::detail::FederationTsoDeliveryStatus::applied);
  REQUIRE(restarted.beginTsoPayloadDelivery(
      L"exercise",
      restartedReceiverB.membership->id,
      rti1516_2025::HLAinteger64Time(5),
      true).deliveryStatus == umbra::detail::FederationTsoDeliveryStatus::no_messages);

  std::filesystem::remove_all(directory, ignored);
}
