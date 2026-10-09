#include "ieee1516_2025_federation_registry_test_support.hpp"

TEST_CASE(
    "The embedded federation registry reports a directed interaction transportation override",
    "[unit][kernel][federation-registry][interaction-management][declaration-management]"
    "[directed-interaction][directed-routing][interaction-transportation-type-change]"
    "[transportation-management][federation-registry-directed-transportation-override][rti.service.publish-object-class-directed-interactions]"
    "[rti.service.request-interaction-transportation-type-change]"
    "[rti.service.query-interaction-transportation-type]") {
  EmbeddedFederationRegistry registry;
  REQUIRE(registry.create(
                        L"exercise",
                        composedDirectedInteractionDefinition())
              .status == FederationRegistryStatus::applied);
  auto joined = registry.join(
      L"exercise", L"directed-publisher", L"directed-publisher", noOpCallbackRoute());
  REQUIRE(joined.status == FederationRegistryStatus::applied);
  REQUIRE(joined.membership.has_value());
  auto const federateId = joined.membership->id;

  auto const objectClass = registry.objectClassHandleFor(
      L"exercise", "HLAobjectRoot.UmbraDirectedFixtureObject");
  auto const interactionClass = registry.interactionClassHandleFor(
      L"exercise", "HLAinteractionRoot.UmbraDirectedFixtureInteraction");
  REQUIRE(objectClass.has_value());
  REQUIRE(interactionClass.has_value());
  REQUIRE(registry.publishObjectClassDirectedInteractions(
               L"exercise", federateId, *objectClass, {*interactionClass}) ==
      umbra::detail::DirectedInteractionDeclarationStatus::applied);

  auto defaultQuery = registry.interactionTransportationTypeQueryFor(
      L"exercise", federateId, federateId, *interactionClass);
  REQUIRE(defaultQuery.has_value());
  REQUIRE(defaultQuery->transportationName == "HLAreliable");

  auto change = registry.planInteractionTransportationTypeChange(
      L"exercise", federateId, *interactionClass, "HLAbestEffort");
  REQUIRE(change.status ==
      umbra::detail::InteractionTransportationTypeChangeStatus::applied);
  auto committed = registry.beginInteractionTransportationTypeChange(
      L"exercise", federateId, *interactionClass);
  REQUIRE(committed.has_value());
  REQUIRE(*committed == "HLAbestEffort");

  auto overrideQuery = registry.interactionTransportationTypeQueryFor(
      L"exercise", federateId, federateId, *interactionClass);
  REQUIRE(overrideQuery.has_value());
  REQUIRE(overrideQuery->transportationName == "HLAbestEffort");
}

TEST_CASE(
    "Filesystem fresh-registry restore rebinds a pending object-instance Request Attribute Value Update",
    "[unit][kernel][federation-registry][save-restore][durable-save][filesystem][restore]"
    "[process-restart][process-restart-pending-attribute-value-update][pending-application-request-state][attribute-value-update]"
    "[object-management][object-lifecycle-state][application-value-state]") {
  auto const directory = temporarySaveCommitDirectory();
  std::error_code ignored;
  std::filesystem::remove_all(directory, ignored);
  auto store = std::make_shared<umbra::detail::FilesystemFederationSaveCommitStore>(
      directory);

  EmbeddedFederationRegistry source({}, store);
  REQUIRE(source.create(L"exercise", composedRestaurantDefinition()).status ==
      FederationRegistryStatus::applied);
  auto owner = source.join(
      L"exercise", L"publisher", L"owner", noOpCallbackRoute());
  auto requester = source.join(
      L"exercise", L"subscriber", L"requester", noOpCallbackRoute());
  REQUIRE(owner.membership);
  REQUIRE(requester.membership);

  auto const server = source.objectClassHandleFor(
      L"exercise", "HLAobjectRoot.Employee.Server");
  auto const efficiency = source.attributeHandleFor(
      L"exercise", "HLAobjectRoot.Employee.Server", "Efficiency");
  REQUIRE(server.has_value());
  REQUIRE(efficiency.has_value());
  std::set<std::uint64_t> const efficiencyOnly{*efficiency};
  REQUIRE(source.setObjectClassAttributePublication(
      L"exercise", owner.membership->id, *server, efficiencyOnly, true) ==
      umbra::detail::ObjectClassAttributeDeclarationStatus::applied);
  REQUIRE(source.setObjectClassAttributeSubscription(
      L"exercise", requester.membership->id, *server, efficiencyOnly, true) ==
      umbra::detail::ObjectClassAttributeDeclarationStatus::applied);

  auto registered = source.registerObjectInstance(
      L"exercise", owner.membership->id, *server);
  REQUIRE(registered.status ==
      umbra::detail::ObjectInstanceRegistrationStatus::applied);
  auto discoveries = source.planObjectInstanceDiscoveriesForInstance(
      L"exercise", registered.objectInstanceHandle);
  REQUIRE(discoveries.size() == 1U);
  REQUIRE(source.beginObjectInstanceDiscovery(
      L"exercise", requester.membership->id, registered.objectInstanceHandle));

  std::string const valueBytes{"\x73\x74", 2U};
  std::vector<std::pair<std::uint64_t, rti1516_2025::VariableLengthData>> values{{
      *efficiency,
      rti1516_2025::VariableLengthData(valueBytes.data(), valueBytes.size()),
  }};
  REQUIRE(source.recordSuccessfulUpdateAttributeValues(
      L"exercise",
      owner.membership->id,
      registered.objectInstanceHandle,
      *server,
      {"HLAreliable"},
      &values) == FederationRegistryStatus::applied);

  auto planned = source.planAttributeValueUpdateRequest(
      L"exercise",
      requester.membership->id,
      registered.objectInstanceHandle,
      efficiencyOnly);
  REQUIRE(planned.status ==
      umbra::detail::AttributeValueUpdateRequestStatus::applied);
  REQUIRE(planned.recipients.size() == 1U);
  REQUIRE(planned.recipients.front().providingFederateId == owner.membership->id);
  std::vector<unsigned char> const requestTag{'a', 'v', 'u', '-', 'r', 'e', 's', 't', 'a', 'r', 't'};
  auto requestId = source.registerAttributeValueUpdateRequest(
      L"exercise",
      requester.membership->id,
      owner.membership->id,
      registered.objectInstanceHandle,
      efficiencyOnly,
      requestTag);
  REQUIRE(requestId.has_value());

  std::wstring const saveLabel = L"pending-attribute-value-update-process-restart";
  REQUIRE(source.requestFederationSave(
      L"exercise", owner.membership->id, saveLabel).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(source.federateSaveBegun(
      L"exercise", owner.membership->id).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(source.federateSaveBegun(
      L"exercise", requester.membership->id).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE_FALSE(source.federateSaveComplete(
      L"exercise", owner.membership->id).saveCompletedSuccessfully);
  REQUIRE(source.federateSaveComplete(
      L"exercise", requester.membership->id).saveCompletedSuccessfully);

  auto committed = store->load(L"exercise", saveLabel);
  REQUIRE(committed.has_value());
  auto image = umbra::detail::FederationStateImageCodec::decode(
      committed->stateImage);
  REQUIRE(image.objects.size() == 1U);
  auto const& savedObject = image.objects.front();
  REQUIRE(savedObject.pendingAttributeValueUpdateRequests.size() == 1U);
  auto const& savedRequest =
      savedObject.pendingAttributeValueUpdateRequests.front();
  REQUIRE(savedRequest.requestId == *requestId);
  REQUIRE(savedRequest.requestingFederateId == requester.membership->id);
  REQUIRE(savedRequest.providingFederateId == owner.membership->id);
  REQUIRE(savedRequest.requestedAttributeHandles ==
      std::vector<std::uint64_t>{*efficiency});
  REQUIRE(savedRequest.userSuppliedTag ==
      std::string(requestTag.begin(), requestTag.end()));
  EmbeddedFederationRegistry restarted({}, store);
  REQUIRE(restarted.create(L"exercise", composedRestaurantDefinition()).status ==
      FederationRegistryStatus::applied);
  auto restartedOwner = restarted.join(
      L"exercise", L"publisher", L"owner", noOpCallbackRoute());
  auto restartedRequester = restarted.join(
      L"exercise", L"subscriber", L"requester", noOpCallbackRoute());
  REQUIRE(restartedOwner.membership);
  REQUIRE(restartedRequester.membership);
  REQUIRE(restartedOwner.membership->id == owner.membership->id);
  REQUIRE(restartedRequester.membership->id == requester.membership->id);

  REQUIRE(restarted.requestFederationRestore(
      L"exercise", restartedRequester.membership->id, saveLabel).status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  auto firstRestore = restarted.federateRestoreComplete(
      L"exercise", restartedOwner.membership->id);
  REQUIRE(firstRestore.status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  REQUIRE(firstRestore.attributeValueUpdateProvideWorkItems.empty());
  auto restored = restarted.federateRestoreComplete(
      L"exercise", restartedRequester.membership->id);
  REQUIRE(restored.status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  REQUIRE(restored.attributeValueUpdateProvideWorkItems.size() == 1U);
  auto const& restoredWork =
      restored.attributeValueUpdateProvideWorkItems.front();
  REQUIRE(restoredWork.requestId == *requestId);
  REQUIRE(restoredWork.requestingFederateId == restartedRequester.membership->id);
  REQUIRE(restoredWork.providingFederateId == restartedOwner.membership->id);
  REQUIRE(restoredWork.objectInstanceHandle == registered.objectInstanceHandle);
  REQUIRE(restoredWork.requestedAttributeHandles == efficiencyOnly);
  REQUIRE(restoredWork.userSuppliedTag == requestTag);

  auto provider = restarted.beginAttributeValueUpdateProvideRecipientFor(
      L"exercise",
      restoredWork.requestId,
      restoredWork.requestingFederateId,
      restoredWork.providingFederateId,
      restoredWork.objectInstanceHandle,
      restoredWork.requestedAttributeHandles);
  REQUIRE(provider.has_value());
  REQUIRE(provider->requestedAttributeHandles == efficiencyOnly);
  REQUIRE_FALSE(restarted.beginAttributeValueUpdateProvideRecipientFor(
      L"exercise",
      restoredWork.requestId,
      restoredWork.requestingFederateId,
      restoredWork.providingFederateId,
      restoredWork.objectInstanceHandle,
      restoredWork.requestedAttributeHandles));

  std::filesystem::remove_all(directory, ignored);
}

TEST_CASE(
    "Filesystem fresh-registry restore rebinds a pending object-class Request Attribute Value Update",
    "[unit][kernel][federation-registry][save-restore][durable-save][filesystem][restore]"
    "[process-restart][process-restart-class-pending-attribute-value-update][pending-application-request-state][attribute-value-update]"
    "[object-management][object-lifecycle-state][application-value-state]") {
  auto const directory = temporarySaveCommitDirectory();
  std::error_code ignored;
  std::filesystem::remove_all(directory, ignored);
  auto store = std::make_shared<umbra::detail::FilesystemFederationSaveCommitStore>(
      directory);

  EmbeddedFederationRegistry source({}, store);
  REQUIRE(source.create(L"exercise", composedRestaurantDefinition()).status ==
      FederationRegistryStatus::applied);
  auto owner = source.join(
      L"exercise", L"publisher", L"owner", noOpCallbackRoute());
  auto requester = source.join(
      L"exercise", L"subscriber", L"requester", noOpCallbackRoute());
  REQUIRE(owner.membership);
  REQUIRE(requester.membership);

  auto const server = source.objectClassHandleFor(
      L"exercise", "HLAobjectRoot.Employee.Server");
  auto const efficiency = source.attributeHandleFor(
      L"exercise", "HLAobjectRoot.Employee.Server", "Efficiency");
  REQUIRE(server.has_value());
  REQUIRE(efficiency.has_value());
  std::set<std::uint64_t> const efficiencyOnly{*efficiency};
  REQUIRE(source.setObjectClassAttributePublication(
      L"exercise", owner.membership->id, *server, efficiencyOnly, true) ==
      umbra::detail::ObjectClassAttributeDeclarationStatus::applied);
  auto registered = source.registerObjectInstance(
      L"exercise", owner.membership->id, *server);
  REQUIRE(registered.status ==
      umbra::detail::ObjectInstanceRegistrationStatus::applied);

  std::string const valueBytes{"xy", 2U};
  std::vector<std::pair<std::uint64_t, rti1516_2025::VariableLengthData>> values{{
      *efficiency,
      rti1516_2025::VariableLengthData(valueBytes.data(), valueBytes.size()),
  }};
  REQUIRE(source.recordSuccessfulUpdateAttributeValues(
      L"exercise",
      owner.membership->id,
      registered.objectInstanceHandle,
      *server,
      {"HLAreliable"},
      &values) == FederationRegistryStatus::applied);

  auto planned = source.planAttributeValueUpdateClassRequest(
      L"exercise",
      requester.membership->id,
      *server,
      efficiencyOnly);
  REQUIRE(planned.status ==
      umbra::detail::AttributeValueUpdateClassRequestStatus::applied);
  REQUIRE(planned.recipients.size() == 1U);
  REQUIRE(planned.recipients.front().objectInstanceHandle ==
      registered.objectInstanceHandle);
  REQUIRE(planned.recipients.front().providingFederateId == owner.membership->id);
  std::vector<unsigned char> const requestTag{'c', 'l', 'a', 's', 's'};
  auto requestId = source.registerAttributeValueUpdateClassRequest(
      L"exercise",
      requester.membership->id,
      owner.membership->id,
      registered.objectInstanceHandle,
      *server,
      efficiencyOnly,
      requestTag);
  REQUIRE(requestId.has_value());

  std::wstring const saveLabel = L"pending-class-attribute-value-update-process-restart";
  REQUIRE(source.requestFederationSave(
      L"exercise", owner.membership->id, saveLabel).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(source.federateSaveBegun(
      L"exercise", owner.membership->id).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(source.federateSaveBegun(
      L"exercise", requester.membership->id).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE_FALSE(source.federateSaveComplete(
      L"exercise", owner.membership->id).saveCompletedSuccessfully);
  REQUIRE(source.federateSaveComplete(
      L"exercise", requester.membership->id).saveCompletedSuccessfully);

  auto committed = store->load(L"exercise", saveLabel);
  REQUIRE(committed.has_value());
  auto image = umbra::detail::FederationStateImageCodec::decode(
      committed->stateImage);
  REQUIRE(image.objects.size() == 1U);
  auto const& savedObject = image.objects.front();
  REQUIRE(savedObject.pendingAttributeValueUpdateClassRequests.size() == 1U);
  auto const& savedRequest =
      savedObject.pendingAttributeValueUpdateClassRequests.front();
  REQUIRE(savedRequest.requestId == *requestId);
  REQUIRE(savedRequest.requestingFederateId == requester.membership->id);
  REQUIRE(savedRequest.providingFederateId == owner.membership->id);
  REQUIRE(savedRequest.requestedObjectClassHandle == *server);
  REQUIRE(savedRequest.requestedAttributeHandles ==
      std::vector<std::uint64_t>{*efficiency});
  REQUIRE(savedRequest.userSuppliedTag ==
      std::string(requestTag.begin(), requestTag.end()));

  EmbeddedFederationRegistry restarted({}, store);
  REQUIRE(restarted.create(L"exercise", composedRestaurantDefinition()).status ==
      FederationRegistryStatus::applied);
  auto restartedOwner = restarted.join(
      L"exercise", L"publisher", L"owner", noOpCallbackRoute());
  auto restartedRequester = restarted.join(
      L"exercise", L"subscriber", L"requester", noOpCallbackRoute());
  REQUIRE(restartedOwner.membership);
  REQUIRE(restartedRequester.membership);
  REQUIRE(restartedOwner.membership->id == owner.membership->id);
  REQUIRE(restartedRequester.membership->id == requester.membership->id);

  REQUIRE(restarted.requestFederationRestore(
      L"exercise", restartedRequester.membership->id, saveLabel).status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  auto firstRestore = restarted.federateRestoreComplete(
      L"exercise", restartedOwner.membership->id);
  REQUIRE(firstRestore.status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  REQUIRE(firstRestore.attributeValueUpdateClassProvideWorkItems.empty());
  auto restored = restarted.federateRestoreComplete(
      L"exercise", restartedRequester.membership->id);
  REQUIRE(restored.status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  REQUIRE(restored.attributeValueUpdateClassProvideWorkItems.size() == 1U);
  auto const& restoredWork =
      restored.attributeValueUpdateClassProvideWorkItems.front();
  REQUIRE(restoredWork.requestId == *requestId);
  REQUIRE(restoredWork.requestingFederateId == restartedRequester.membership->id);
  REQUIRE(restoredWork.providingFederateId == restartedOwner.membership->id);
  REQUIRE(restoredWork.objectInstanceHandle == registered.objectInstanceHandle);
  REQUIRE(restoredWork.requestedObjectClassHandle == *server);
  REQUIRE(restoredWork.requestedAttributeHandles == efficiencyOnly);
  REQUIRE(restoredWork.userSuppliedTag == requestTag);

  auto provider = restarted.beginAttributeValueUpdateClassProvideRecipientFor(
      L"exercise",
      restoredWork.requestId,
      restoredWork.requestingFederateId,
      restoredWork.providingFederateId,
      restoredWork.objectInstanceHandle,
      restoredWork.requestedObjectClassHandle,
      restoredWork.requestedAttributeHandles);
  REQUIRE(provider.has_value());
  REQUIRE(provider->requestedAttributeHandles == efficiencyOnly);
  REQUIRE_FALSE(restarted.beginAttributeValueUpdateClassProvideRecipientFor(
      L"exercise",
      restoredWork.requestId,
      restoredWork.requestingFederateId,
      restoredWork.providingFederateId,
      restoredWork.objectInstanceHandle,
      restoredWork.requestedObjectClassHandle,
      restoredWork.requestedAttributeHandles));

  std::filesystem::remove_all(directory, ignored);
}

TEST_CASE(
    "Filesystem federation save commits publish unique durable envelopes",
    "[unit][kernel][federation-registry][save-restore][durable-save][filesystem]") {
  auto const directory = temporarySaveCommitDirectory();
  std::error_code ignored;
  std::filesystem::remove_all(directory, ignored);
  umbra::detail::FilesystemFederationSaveCommitStore store(directory);
  umbra::detail::FederationSaveCommitDescriptor descriptor{
      L"exercise / unsafe",
      L"checkpoint \"one\"",
      L"HLAinteger64Time",
      {7U, 42U},
      true};
  umbra::detail::FederationStateImage stateImage;
  stateImage.federationName = descriptor.federationName;
  stateImage.logicalTimeImplementationName = descriptor.logicalTimeImplementationName;
  stateImage.members = {{7U, L"alice", L"trainer", 0U, 0U, 0, 0U}};
  descriptor.stateImage = umbra::detail::FederationStateImageCodec::encode(stateImage);

  REQUIRE_NOTHROW(store.commit(descriptor));
  REQUIRE_NOTHROW(store.commit(descriptor));
  REQUIRE(std::filesystem::is_directory(directory));

  std::vector<std::filesystem::path> manifests;
  for (auto const& entry : std::filesystem::directory_iterator(directory)) {
    if (entry.is_regular_file() && entry.path().extension() == ".json") {
      manifests.push_back(entry.path());
    }
  }
  REQUIRE(manifests.size() == 2U);
  std::ifstream input(manifests.front(), std::ios::binary);
  std::string contents{std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
  REQUIRE(contents.find("umbra-federation-save-commit/v1") != std::string::npos);
  REQUIRE(contents.find("exercise / unsafe") != std::string::npos);
  REQUIRE(contents.find("checkpoint \\\"one\\\"") != std::string::npos);
  REQUIRE(contents.find("\"timed\": true") != std::string::npos);
  REQUIRE(contents.find("\"memberFederateIds\": [7, 42]") != std::string::npos);

  auto loaded = store.load(L"exercise / unsafe", L"checkpoint \"one\"");
  REQUIRE(loaded.has_value());
  REQUIRE(loaded->logicalTimeImplementationName == L"HLAinteger64Time");
  REQUIRE(loaded->memberFederateIds == std::vector<std::uint64_t>{7U, 42U});
  REQUIRE(loaded->timed);
  REQUIRE(loaded->stateImage == descriptor.stateImage);
  REQUIRE(umbra::detail::FederationStateImageCodec::decode(loaded->stateImage)
      .federationName == descriptor.federationName);
  REQUIRE_FALSE(store.load(L"missing", L"checkpoint").has_value());

  std::filesystem::remove_all(directory, ignored);
}

TEST_CASE(
    "Filesystem federation save commits do not leave a temporary file on encoding failure",
    "[unit][kernel][federation-registry][save-restore][durable-save][filesystem][failure]") {
  auto const directory = temporarySaveCommitDirectory();
  std::error_code ignored;
  std::filesystem::remove_all(directory, ignored);
  umbra::detail::FilesystemFederationSaveCommitStore store(directory);
  umbra::detail::FederationSaveCommitDescriptor descriptor{
      L"exercise",
      std::wstring{L"checkpoint-"} + std::wstring(1U, static_cast<wchar_t>(0xD800U)),
      L"HLAinteger64Time",
      {7U},
      false};

  REQUIRE_THROWS(store.commit(descriptor));
  REQUIRE(std::filesystem::is_directory(directory));
  REQUIRE(std::filesystem::is_empty(directory));
  std::filesystem::remove_all(directory, ignored);
}


















TEST_CASE(
    "The registry commits runtime time state with its federate membership",
    "[unit][kernel][federation-registry][time-management]") {
  EmbeddedFederationRegistry registry;
  REQUIRE(registry.create(L"exercise", validDefinition()).status == FederationRegistryStatus::applied);

  auto timeState = std::make_shared<umbra::detail::FederateTimeState>(
      L"HLAinteger64Time",
      std::make_shared<rti1516_2025::HLAinteger64Time>());
  auto joined = registry.joinWithTimeState(L"exercise", timeState, L"trainer", L"alice");
  REQUIRE(joined.status == FederationRegistryStatus::applied);
  REQUIRE(joined.membership.has_value());

  auto advance = timeState->requestAdvance(std::make_shared<rti1516_2025::HLAinteger64Time>(3));
  REQUIRE(advance.generation != 0);
  timeState.reset();

  auto snapshot = registry.timeSnapshotFor(L"exercise");
  REQUIRE(snapshot.has_value());
  REQUIRE(snapshot->definition.logicalTimeImplementationName == L"HLAinteger64Time");
  REQUIRE(snapshot->federates.size() == 1);
  REQUIRE(snapshot->federates.front().membership.id == joined.membership->id);
  REQUIRE(snapshot->federates.front().membership.name == L"alice");
  REQUIRE(snapshot->federates.front().time.timeAdvancePending);
  REQUIRE(snapshot->federates.front().time.requestedTime);

  REQUIRE(
      registry.resign(L"exercise", joined.membership->id).status ==
      FederationRegistryStatus::applied);
  snapshot = registry.timeSnapshotFor(L"exercise");
  REQUIRE(snapshot.has_value());
  REQUIRE(snapshot->federates.empty());
}

TEST_CASE(
    "The registry projects private TSO coordination into its time snapshot",
    "[unit][kernel][federation-registry][time-management][tso][lits]") {
  EmbeddedFederationRegistry registry;
  REQUIRE(registry.create(L"exercise", validDefinition()).status == FederationRegistryStatus::applied);

  auto receiverTime = std::make_shared<umbra::detail::FederateTimeState>(
      L"HLAinteger64Time",
      std::make_shared<rti1516_2025::HLAinteger64Time>());
  auto observerTime = std::make_shared<umbra::detail::FederateTimeState>(
      L"HLAinteger64Time",
      std::make_shared<rti1516_2025::HLAinteger64Time>());
  auto receiver = registry.joinWithTimeState(L"exercise", receiverTime, L"receiver", L"receiver");
  auto observer = registry.joinWithTimeState(L"exercise", observerTime, L"observer", L"observer");
  REQUIRE(receiver.status == FederationRegistryStatus::applied);
  REQUIRE(observer.status == FederationRegistryStatus::applied);
  REQUIRE(receiver.membership);
  REQUIRE(observer.membership);

  auto const messageId = registry.allocateTsoMessageId(L"exercise");
  REQUIRE(messageId.status == umbra::detail::FederationTsoRegistryStatus::applied);
  REQUIRE(messageId.messageId != 0);
  auto const timestamp = std::make_shared<rti1516_2025::HLAinteger64Time>(5);
  REQUIRE(
      registry.enqueueTsoMessage(
          L"exercise",
          messageId.messageId,
          receiver.membership->id,
          timestamp)
          .queueStatus == umbra::detail::TsoMessageQueueStatus::applied);
  REQUIRE(
      registry.enqueueTsoMessage(
          L"exercise",
          messageId.messageId,
          observer.membership->id,
          timestamp)
          .queueStatus == umbra::detail::TsoMessageQueueStatus::applied);

  auto snapshot = registry.timeSnapshotFor(L"exercise");
  REQUIRE(snapshot.has_value());
  REQUIRE(snapshot->federates.size() == 2);
  REQUIRE(snapshot->federates[0].queuedTsoMessages.size() == 1);
  REQUIRE(snapshot->federates[0].inTransitTsoMessages.empty());
  REQUIRE(snapshot->federates[0].deliveredTsoMessagesSinceLastAdvance.empty());

  auto delivery = registry.beginTsoDelivery(
      L"exercise",
      receiver.membership->id,
      rti1516_2025::HLAinteger64Time(5),
      true);
  REQUIRE(delivery.status == umbra::detail::FederationTsoRegistryStatus::applied);
  REQUIRE(delivery.delivery.status == umbra::detail::FederationTsoDeliveryStatus::applied);
  REQUIRE(delivery.delivery.messages.size() == 1);

  snapshot = registry.timeSnapshotFor(L"exercise");
  REQUIRE(snapshot.has_value());
  REQUIRE(snapshot->federates[0].queuedTsoMessages.empty());
  REQUIRE(snapshot->federates[0].inTransitTsoMessages.size() == 1);
  REQUIRE(snapshot->federates[1].queuedTsoMessages.size() == 1);

  auto completed = registry.completeTsoDelivery(L"exercise", delivery.delivery.messages.front());
  REQUIRE(completed.status == umbra::detail::FederationTsoRegistryStatus::applied);
  REQUIRE(completed.delivery.status == umbra::detail::FederationTsoDeliveryStatus::applied);
  snapshot = registry.timeSnapshotFor(L"exercise");
  REQUIRE(snapshot.has_value());
  REQUIRE(snapshot->federates[0].inTransitTsoMessages.empty());
  REQUIRE(snapshot->federates[0].deliveredTsoMessagesSinceLastAdvance.size() == 1);

  auto const retraction = registry.retractTsoMessage(L"exercise", messageId.messageId);
  REQUIRE(retraction.status == umbra::detail::FederationTsoRegistryStatus::applied);
  REQUIRE(retraction.queueResult.status == umbra::detail::TsoMessageQueueStatus::message_already_delivered);

  REQUIRE(
      registry.resign(L"exercise", receiver.membership->id).status ==
      FederationRegistryStatus::applied);
  snapshot = registry.timeSnapshotFor(L"exercise");
  REQUIRE(snapshot.has_value());
  REQUIRE(snapshot->federates.size() == 1);
  REQUIRE(snapshot->federates.front().membership.id == observer.membership->id);
  REQUIRE(snapshot->federates.front().queuedTsoMessages.size() == 1);
}

TEST_CASE(
    "The TSO recipient ledger makes a suppressed callback terminal",
    "[unit][kernel][federation-registry][time-management][tso][request-retraction]") {
  EmbeddedFederationRegistry registry;
  REQUIRE(registry.create(L"exercise", validDefinition()).status ==
      FederationRegistryStatus::applied);

  auto producer = registry.join(L"exercise", L"producer", L"producer");
  umbra::detail::InteractionCallbackRoute receiverRoute;
  receiverRoute.submit = [](umbra::detail::FederateCallbackInvocation) {};
  auto receiver = registry.join(
      L"exercise",
      L"receiver",
      L"receiver",
      std::move(receiverRoute));
  REQUIRE(producer.membership);
  REQUIRE(receiver.membership);

  umbra::detail::TsoInteractionMessage message;
  message.producingFederateId = producer.membership->id;
  message.sentInteractionClassHandle = 1;
  message.timestamp = std::make_shared<rti1516_2025::HLAinteger64Time>(7);
  auto const enqueued = registry.enqueueTsoInteraction(
      L"exercise",
      std::move(message),
      {receiver.membership->id},
      {receiver.membership->id});
  REQUIRE(enqueued.status == umbra::detail::FederationTsoRegistryStatus::applied);
  REQUIRE(enqueued.queueStatus == umbra::detail::TsoMessageQueueStatus::applied);
  REQUIRE(enqueued.messageId != 0);

  // A callback can be invalidated after TSO admission (for example, by a
  // late projection failure or a malformed private payload).  The delivery
  // path must close that recipient as suppressed rather than leave it
  // pending or report a Request Retraction for a callback that never began.
  REQUIRE(registry.finishTsoRecipientCallbackSuppressed(
      L"exercise",
      receiver.membership->id,
      enqueued.messageId));
  REQUIRE_FALSE(registry.beginTsoInteractionCallback(
      L"exercise",
      receiver.membership->id,
      enqueued.messageId));

  auto const retracted = registry.retractTsoMessageForProducer(
      L"exercise",
      producer.membership->id,
      enqueued.messageId,
      std::make_shared<rti1516_2025::HLAinteger64Time>(0));
  REQUIRE(retracted.status == umbra::detail::FederationTsoRegistryStatus::applied);
  REQUIRE(retracted.queueResult.status == umbra::detail::TsoMessageQueueStatus::applied);
  REQUIRE(retracted.requestRetractionNotifications.empty());
  REQUIRE_FALSE(registry.canDeliverTsoRequestRetraction(
      L"exercise",
      receiver.membership->id,
      enqueued.messageId));
}

TEST_CASE(
    "The registry schedules a newly eligible constrained TAR after a regulator advances",
    "[unit][kernel][federation-registry][time-management][galt][time-advance-grant]") {
  EmbeddedFederationRegistry registry;
  REQUIRE(registry.create(L"exercise", validDefinition()).status == FederationRegistryStatus::applied);

  auto receiverTime = std::make_shared<umbra::detail::FederateTimeState>(
      L"HLAinteger64Time",
      std::make_shared<rti1516_2025::HLAinteger64Time>());
  auto regulatorTime = std::make_shared<umbra::detail::FederateTimeState>(
      L"HLAinteger64Time",
      std::make_shared<rti1516_2025::HLAinteger64Time>());
  auto receiver = registry.joinWithTimeState(L"exercise", receiverTime, L"receiver", L"receiver");
  auto regulator = registry.joinWithTimeState(L"exercise", regulatorTime, L"regulator", L"regulator");
  REQUIRE(receiver.membership);
  REQUIRE(regulator.membership);

  auto constrained = receiverTime->requestTimeConstrained();
  REQUIRE(constrained.generation != 0);
  REQUIRE(receiverTime->grantTimeConstrained(constrained.generation));
  auto regulation = regulatorTime->requestTimeRegulation(
      std::make_shared<rti1516_2025::HLAinteger64Interval>(2));
  REQUIRE(regulation.generation != 0);
  REQUIRE(regulatorTime->grantTimeRegulation(regulation.generation));

  auto receiverAdvance = receiverTime->requestAdvance(
      std::make_shared<rti1516_2025::HLAinteger64Time>(2));
  REQUIRE(receiverAdvance.generation != 0);
  std::size_t receiverDispatches = 0;
  auto waiting = registry.requestTimeAdvanceGrant(
      L"exercise",
      receiver.membership->id,
      receiverAdvance.generation,
      [&receiverDispatches] { ++receiverDispatches; });
  REQUIRE(waiting.status == FederationTimeGrantStatus::applied);
  REQUIRE(waiting.dispatches.empty());

  // The regulator's pending target 1 plus lookahead 2 raises the receiver's
  // GALT from 2 to 3, making the receiver's TAR(2) strictly below the bound
  // even before the regulator receives its own grant.
  auto regulatorAdvance = regulatorTime->requestAdvance(
      std::make_shared<rti1516_2025::HLAinteger64Time>(1));
  REQUIRE(regulatorAdvance.generation != 0);
  std::size_t regulatorDispatches = 0;
  auto eligible = registry.requestTimeAdvanceGrant(
      L"exercise",
      regulator.membership->id,
      regulatorAdvance.generation,
      [&regulatorDispatches] { ++regulatorDispatches; });
  REQUIRE(eligible.status == FederationTimeGrantStatus::applied);
  REQUIRE(eligible.dispatches.size() == 2);
  for (auto& dispatch : eligible.dispatches) {
    dispatch();
  }
  REQUIRE(receiverDispatches == 1);
  REQUIRE(regulatorDispatches == 1);

  REQUIRE(
      registry.beginTimeAdvanceGrant(
          L"exercise",
          receiver.membership->id,
          receiverAdvance.generation) == FederationTimeGrantStatus::applied);
  REQUIRE(receiverTime->grant(receiverAdvance.generation));
  REQUIRE(
      registry.beginTimeAdvanceGrant(
          L"exercise",
          regulator.membership->id,
          regulatorAdvance.generation) == FederationTimeGrantStatus::applied);
  REQUIRE(regulatorTime->grant(regulatorAdvance.generation));

  auto receiverSnapshot = receiverTime->snapshot();
  auto regulatorSnapshot = regulatorTime->snapshot();
  REQUIRE_FALSE(receiverSnapshot.timeAdvancePending);
  REQUIRE_FALSE(regulatorSnapshot.timeAdvancePending);
}

TEST_CASE(
    "Filesystem state image restores a directed interaction publication and subscription for multiple federates",
    "[unit][kernel][federation-registry][save-restore][durable-save][filesystem][restore]"
    "[process-restart][process-restart-directed-interaction-declaration][interaction-declaration-state]"
    "[directed-interaction][directed-declaration][declaration-management][multi-federate-callback-ordering]") {
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
  REQUIRE(objectClass.has_value());
  REQUIRE(interactionClass.has_value());
  std::set<std::uint64_t> directedClasses{*interactionClass};
  REQUIRE(source.publishObjectClassDirectedInteractions(
      L"exercise", publisherId, *objectClass, directedClasses) ==
      umbra::detail::DirectedInteractionDeclarationStatus::applied);
  REQUIRE(source.subscribeObjectClassDirectedInteractions(
      L"exercise", subscriberId, *objectClass, directedClasses, true) ==
      umbra::detail::DirectedInteractionDeclarationStatus::applied);

  std::wstring const saveLabel = L"directed-interaction-declaration-process-restart";
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
  REQUIRE(image.interactionDeclarationCount == 2U);
  REQUIRE(image.interactionDeclarations.size() == 2U);
  REQUIRE(image.interactionDeclarations[0].federateId == publisherId);
  REQUIRE(image.interactionDeclarations[1].federateId == subscriberId);
  auto const& savedPublisher = image.interactionDeclarations[0];
  auto const& savedSubscriber = image.interactionDeclarations[1];
  REQUIRE(savedPublisher.publishedInteractionClasses.empty());
  REQUIRE(savedPublisher.subscribedInteractionClasses.empty());
  REQUIRE(savedPublisher.regionalSubscribedInteractionClasses.empty());
  REQUIRE(savedPublisher.publishedObjectClassDirectedInteractions.size() == 1U);
  REQUIRE(savedPublisher.publishedObjectClassDirectedInteractions.front().objectClassHandle ==
      *objectClass);
  REQUIRE(savedPublisher.publishedObjectClassDirectedInteractions.front().interactionClassHandle ==
      *interactionClass);
  REQUIRE(savedPublisher.subscribedObjectClassDirectedInteractions.empty());
  REQUIRE(savedPublisher.interactionTransportationTypes.empty());
  REQUIRE(savedPublisher.interactionOrderTypes.empty());
  REQUIRE(savedPublisher.pendingInteractionTransportationTypeChanges.empty());
  REQUIRE(savedSubscriber.publishedInteractionClasses.empty());
  REQUIRE(savedSubscriber.subscribedInteractionClasses.empty());
  REQUIRE(savedSubscriber.regionalSubscribedInteractionClasses.empty());
  REQUIRE(savedSubscriber.publishedObjectClassDirectedInteractions.empty());
  REQUIRE(savedSubscriber.subscribedObjectClassDirectedInteractions.size() == 1U);
  REQUIRE(savedSubscriber.subscribedObjectClassDirectedInteractions.front().objectClassHandle ==
      *objectClass);
  REQUIRE(savedSubscriber.subscribedObjectClassDirectedInteractions.front().interactionClassHandle ==
      *interactionClass);
  REQUIRE(savedSubscriber.subscribedObjectClassDirectedInteractions.front().active);
  REQUIRE(savedSubscriber.interactionTransportationTypes.empty());
  REQUIRE(savedSubscriber.interactionOrderTypes.empty());
  REQUIRE(savedSubscriber.pendingInteractionTransportationTypeChanges.empty());
  REQUIRE(image.objects.empty());
  REQUIRE(image.tsoInteractionMessages.empty());
  REQUIRE(image.tsoAttributeUpdateMessages.empty());
  REQUIRE(image.tsoObjectDeletionMessages.empty());
  REQUIRE(image.tsoDirectedInteractionMessages.empty());
  REQUIRE(image.tsoRequestRetractionRecords.empty());
  REQUIRE(image.tsoQueueEntries.empty());

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

  // The second commit proves the restored runtime maps serialize back to the
  // same directed pair instead of merely accepting the original image.
  std::wstring const roundTripLabel =
      L"directed-interaction-declaration-round-trip";
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
  REQUIRE(roundTripImage.interactionDeclarations.size() == 2U);
  REQUIRE(roundTripImage.interactionDeclarations[0]
              .publishedObjectClassDirectedInteractions.size() == 1U);
  REQUIRE(roundTripImage.interactionDeclarations[0]
              .publishedObjectClassDirectedInteractions.front()
              .objectClassHandle == *objectClass);
  REQUIRE(roundTripImage.interactionDeclarations[0]
              .publishedObjectClassDirectedInteractions.front()
              .interactionClassHandle == *interactionClass);
  REQUIRE(roundTripImage.interactionDeclarations[1]
              .subscribedObjectClassDirectedInteractions.size() == 1U);
  REQUIRE(roundTripImage.interactionDeclarations[1]
              .subscribedObjectClassDirectedInteractions.front()
              .objectClassHandle == *objectClass);
  REQUIRE(roundTripImage.interactionDeclarations[1]
              .subscribedObjectClassDirectedInteractions.front()
              .interactionClassHandle == *interactionClass);
  REQUIRE(roundTripImage.interactionDeclarations[1]
              .subscribedObjectClassDirectedInteractions.front()
              .active);

  std::filesystem::remove_all(directory, ignored);
}

TEST_CASE(
    "Federation state images preserve deferred update-region associations across a v1 codec round trip",
    "[unit][kernel][federation-registry][save-restore][durable-save][state-image]"
    "[ownership-management][ddm][deferred-update-region-association]"
    "[deferred-update-region-association-state-image][process-restart]") {
  umbra::detail::FederationStateImage image;
  image.federationName = L"exercise";
  image.logicalTimeImplementationName = L"HLAinteger64Time";
  image.members = {
      {1U, L"owner", L"trainer", 0U, 0U, 0, 0U},
      {2U, L"acquirer", L"trainer", 0U, 0U, 0, 0U},
  };
  image.regionCount = 2U;
  image.regions = {
      {90U, 2U, {}, {}, {}, false, false},
      {91U, 2U, {}, {}, {}, false, false},
  };
  image.objectInstanceCount = 1U;
  image.objects.push_back({
      42U,
      L"table-42",
      4U,
      1U,
      false,
      0U,
      {{3U, 1U, "HLAreliable", 0U, {}}},
  });
  image.deferredUpdateRegionAssociationsPresent = true;
  image.deferredUpdateRegionAssociations.push_back({42U, 2U, 3U, {91U, 90U}});

  auto const encoded = umbra::detail::FederationStateImageCodec::encode(image);
  REQUIRE(encoded.find("deferredUpdateRegionAssociations=1\n") != std::string::npos);

  auto const decoded = umbra::detail::FederationStateImageCodec::decode(encoded);
  REQUIRE(decoded.deferredUpdateRegionAssociationsPresent);
  REQUIRE(decoded.deferredUpdateRegionAssociations.size() == 1U);
  auto const& association = decoded.deferredUpdateRegionAssociations.front();
  REQUIRE(association.objectInstanceHandle == 42U);
  REQUIRE(association.federateId == 2U);
  REQUIRE(association.attributeHandle == 3U);
  REQUIRE(association.regionHandles == std::vector<std::uint64_t>{90U, 91U});
  REQUIRE(umbra::detail::FederationStateImageCodec::encode(decoded) == encoded);

  auto malformed = image;
  malformed.deferredUpdateRegionAssociations.push_back({42U, 2U, 3U, {90U}});
  REQUIRE_THROWS(umbra::detail::FederationStateImageCodec::encode(malformed));
}

TEST_CASE(
    "Register Object Instance With Regions uses the default region and execution-wide generated names",
    "[unit][kernel][federation-registry][object-management][ddm][default-region]"
    "[object-instance-name][rti.service.register-object-instance-with-regions]"
    "[rti.service.publish-object-class-attributes][rti.service.subscribe-object-class-attributes]"
    "[federate.callback.discover-object-instance]") {
  auto store = std::make_shared<umbra::detail::MemoryFederationSaveCommitStore>();
  EmbeddedFederationRegistry registry({}, store);
  REQUIRE(registry.create(L"exercise", composedRestaurantDefinition()).status ==
      FederationRegistryStatus::applied);

  auto owner = registry.join(
      L"exercise", L"publisher", L"owner", noOpCallbackRoute());
  auto observer = registry.join(
      L"exercise", L"subscriber", L"observer", noOpCallbackRoute());
  REQUIRE(owner.membership);
  REQUIRE(observer.membership);

  auto const soda = registry.objectClassHandleFor(
      L"exercise", "HLAobjectRoot.Food.Drink.Soda");
  auto const flavor = registry.attributeHandleFor(
      L"exercise", "HLAobjectRoot.Food.Drink.Soda", "Flavor");
  auto const sodaDimension = registry.dimensionHandleFor(
      L"exercise", "SodaFlavor");
  REQUIRE(soda.has_value());
  REQUIRE(flavor.has_value());
  REQUIRE(sodaDimension.has_value());
  std::set<std::uint64_t> const flavorOnly{*flavor};
  REQUIRE(registry.setObjectClassAttributePublication(
      L"exercise", owner.membership->id, *soda, flavorOnly, true) ==
      umbra::detail::ObjectClassAttributeDeclarationStatus::applied);
  REQUIRE(registry.setObjectClassAttributeSubscription(
      L"exercise", observer.membership->id, *soda, flavorOnly, true) ==
      umbra::detail::ObjectClassAttributeDeclarationStatus::applied);

  // An empty region set in the supplied pair is the official default-region
  // form for a class attribute that has available dimensions.
  std::map<std::uint64_t, std::set<std::uint64_t>> emptyRegionPair{{*flavor, {}}};
  auto first = registry.registerObjectInstance(
      L"exercise", owner.membership->id, *soda, &emptyRegionPair);
  REQUIRE(first.status ==
      umbra::detail::ObjectInstanceRegistrationStatus::applied);
  REQUIRE(first.objectInstanceHandle != 0U);
  REQUIRE(first.objectInstanceName.starts_with(L"UmbraObjectInstance-"));

  auto firstDiscoveries = registry.planObjectInstanceDiscoveriesForInstance(
      L"exercise", first.objectInstanceHandle);
  REQUIRE(firstDiscoveries.size() == 1U);
  REQUIRE(registry.beginObjectInstanceDiscovery(
      L"exercise", observer.membership->id, first.objectInstanceHandle));
  auto firstPlan = registry.planReceiveOrderAttributeUpdate(
      L"exercise", owner.membership->id, first.objectInstanceHandle,
      {*flavor});
  REQUIRE(firstPlan.status ==
      umbra::detail::ReceiveOrderAttributeUpdateStatus::applied);
  REQUIRE(firstPlan.passels.size() == 1U);
  REQUIRE(firstPlan.passels.front().sentAttributeHandles ==
      std::vector<std::uint64_t>{*flavor});
  REQUIRE(firstPlan.passels.front().sentRegionHandles.empty());
  REQUIRE(firstPlan.passels.front().defaultRegionUsed);
  REQUIRE(firstPlan.passels.front().recipients.size() == 1U);
  REQUIRE(firstPlan.passels.front().recipients.front().federateId ==
      observer.membership->id);

  // Omitting the optional pair argument has the same default-region result,
  // and every generated name remains unique within the federation execution.
  auto second = registry.registerObjectInstance(
      L"exercise", owner.membership->id, *soda);
  REQUIRE(second.status ==
      umbra::detail::ObjectInstanceRegistrationStatus::applied);
  REQUIRE(second.objectInstanceHandle != first.objectInstanceHandle);
  REQUIRE(second.objectInstanceName.starts_with(L"UmbraObjectInstance-"));
  REQUIRE(second.objectInstanceName != first.objectInstanceName);
  auto secondPlan = registry.planReceiveOrderAttributeUpdate(
      L"exercise", owner.membership->id, second.objectInstanceHandle,
      {*flavor});
  REQUIRE(secondPlan.status ==
      umbra::detail::ReceiveOrderAttributeUpdateStatus::applied);
  REQUIRE(secondPlan.passels.size() == 1U);
  REQUIRE(secondPlan.passels.front().sentRegionHandles.empty());
  REQUIRE(secondPlan.passels.front().defaultRegionUsed);

  // An empty attribute/region-pair collection also leaves every available
  // instance attribute on the default region for this atomic service.
  std::map<std::uint64_t, std::set<std::uint64_t>> noRegionPairs;
  auto emptyCollectionRegistration = registry.registerObjectInstance(
      L"exercise", owner.membership->id, *soda, &noRegionPairs);
  REQUIRE(emptyCollectionRegistration.status ==
      umbra::detail::ObjectInstanceRegistrationStatus::applied);
  auto emptyCollectionPlan = registry.planReceiveOrderAttributeUpdate(
      L"exercise", owner.membership->id,
      emptyCollectionRegistration.objectInstanceHandle, {*flavor});
  REQUIRE(emptyCollectionPlan.status ==
      umbra::detail::ReceiveOrderAttributeUpdateStatus::applied);
  REQUIRE(emptyCollectionPlan.passels.size() == 1U);
  REQUIRE(emptyCollectionPlan.passels.front().sentRegionHandles.empty());
  REQUIRE(emptyCollectionPlan.passels.front().defaultRegionUsed);

  // A non-empty pair remains distinguishable from the default-region form.
  auto sourceRegion = registry.createRegion(
      L"exercise", owner.membership->id, {*sodaDimension});
  REQUIRE(sourceRegion.status == umbra::detail::RegionServiceStatus::applied);
  REQUIRE(registry.setRangeBounds(
      L"exercise", owner.membership->id, sourceRegion.regionHandle,
      *sodaDimension, {0UL, 1UL}) ==
      umbra::detail::RegionServiceStatus::applied);
  REQUIRE(registry.commitRegionModifications(
      L"exercise", owner.membership->id, {sourceRegion.regionHandle}) ==
      umbra::detail::RegionServiceStatus::applied);
  std::map<std::uint64_t, std::set<std::uint64_t>> explicitRegionPair{{
      *flavor, {sourceRegion.regionHandle}}};
  auto explicitRegistration = registry.registerObjectInstance(
      L"exercise", owner.membership->id, *soda, &explicitRegionPair);
  REQUIRE(explicitRegistration.status ==
      umbra::detail::ObjectInstanceRegistrationStatus::applied);
  auto explicitPlan = registry.planReceiveOrderAttributeUpdate(
      L"exercise", owner.membership->id,
      explicitRegistration.objectInstanceHandle, {*flavor});
  REQUIRE(explicitPlan.status ==
      umbra::detail::ReceiveOrderAttributeUpdateStatus::applied);
  REQUIRE(explicitPlan.passels.size() == 1U);
  REQUIRE(explicitPlan.passels.front().sentRegionHandles ==
      std::set<std::uint64_t>{sourceRegion.regionHandle});
  REQUIRE_FALSE(explicitPlan.passels.front().defaultRegionUsed);
}

TEST_CASE(
    "Filesystem fresh-registry restore seeds declaration relevance before the next mutation",
    "[unit][kernel][federation-registry][save-restore][durable-save][filesystem][restore]"
    "[process-restart][declaration-relevance-state][declaration-management][declaration-relevance-restore-baseline]") {
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
  REQUIRE(publisher.membership);
  REQUIRE(subscriber.membership);

  auto const publisherId = publisher.membership->id;
  auto const subscriberId = subscriber.membership->id;
  auto const objectClass = source.objectClassHandleFor(
      L"exercise", "HLAobjectRoot.Food.Drink.Soda");
  auto const attribute = source.attributeHandleFor(
      L"exercise", "HLAobjectRoot.Food.Drink.Soda", "Flavor");
  auto const interactionClass = source.interactionClassHandleFor(
      L"exercise", "HLAinteractionRoot.ServerAction.TakeOrder");
  REQUIRE(objectClass.has_value());
  REQUIRE(attribute.has_value());
  REQUIRE(interactionClass.has_value());

  REQUIRE(source.setObjectClassAttributePublication(
               L"exercise", publisherId, *objectClass, {*attribute}, true) ==
      umbra::detail::ObjectClassAttributeDeclarationStatus::applied);
  REQUIRE(source.setObjectClassAttributeSubscription(
               L"exercise", subscriberId, *objectClass, {*attribute}, true) ==
      umbra::detail::ObjectClassAttributeDeclarationStatus::applied);
  REQUIRE(source.setInteractionClassPublication(
               L"exercise", publisherId, *interactionClass, true) ==
      umbra::detail::InteractionClassDeclarationStatus::applied);
  REQUIRE(source.setInteractionClassSubscription(
               L"exercise", subscriberId, *interactionClass, true) ==
      umbra::detail::InteractionClassDeclarationStatus::applied);

  auto sourceBaseline = source.planDeclarationAdvisories(L"exercise");
  REQUIRE(sourceBaseline.size() == 2U);

  std::wstring const saveLabel = L"declaration-relevance-process-restart";
  REQUIRE(source.requestFederationSave(
               L"exercise", publisherId, saveLabel).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(source.federateSaveBegun(L"exercise", publisherId).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(source.federateSaveBegun(L"exercise", subscriberId).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE_FALSE(source.federateSaveComplete(
      L"exercise", publisherId).saveCompletedSuccessfully);
  REQUIRE(source.federateSaveComplete(
      L"exercise", subscriberId).saveCompletedSuccessfully);

  EmbeddedFederationRegistry restarted({}, store);
  REQUIRE(restarted.create(L"exercise", composedRestaurantDefinition()).status ==
      FederationRegistryStatus::applied);
  auto restartedPublisher = restarted.join(
      L"exercise", L"publisher", L"publisher", noOpCallbackRoute());
  auto restartedSubscriber = restarted.join(
      L"exercise", L"subscriber", L"subscriber", noOpCallbackRoute());
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
  REQUIRE(restarted.federateRestoreComplete(
               L"exercise", subscriberId).status ==
      umbra::detail::FederationRestoreControlStatus::applied);

  // Restoring declarations must establish the current relevance baseline,
  // so an immediate planner pass has no synthetic Start/Turn-On edges.
  auto baseline = restarted.planDeclarationAdvisories(L"exercise");
  REQUIRE(baseline.empty());

  // A real edge after restore remains visible: disabling both subscriptions
  // produces exactly the matching Stop/Turn-Off transitions.
  REQUIRE(restarted.setObjectClassAttributeSubscription(
               L"exercise", subscriberId, *objectClass, {*attribute}, false) ==
      umbra::detail::ObjectClassAttributeDeclarationStatus::applied);
  REQUIRE(restarted.setInteractionClassSubscription(
               L"exercise", subscriberId, *interactionClass, false) ==
      umbra::detail::InteractionClassDeclarationStatus::applied);
  auto transitions = restarted.planDeclarationAdvisories(L"exercise");
  REQUIRE(transitions.size() == 2U);
  REQUIRE(std::count_if(
              transitions.begin(), transitions.end(), [](auto const& advisory) {
                return advisory.kind ==
                    umbra::detail::DeclarationAdvisoryKind::stop_registration_for_object_class;
              }) == 1U);
  REQUIRE(std::count_if(
              transitions.begin(), transitions.end(), [](auto const& advisory) {
                return advisory.kind ==
                    umbra::detail::DeclarationAdvisoryKind::turn_interactions_off;
              }) == 1U);

  std::filesystem::remove_all(directory, ignored);
}
