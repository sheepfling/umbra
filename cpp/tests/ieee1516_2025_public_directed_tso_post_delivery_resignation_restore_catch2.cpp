#include "ieee1516_2025_federation_management_test_support.hpp"
#include "ieee1516_2025_federation_management_fixture_support.hpp"
#include "ieee1516_2025_public_federation_restore_test_support.hpp"

namespace {
TEST_CASE(
    "Embedded public fresh-registry restore keeps an ownership-qualified directed TSO route after a peer's post-delivery resignation under HLA_EVOKED and HLA_IMMEDIATE",
    "[integration][development-profile][federation-management][ownership-management]"
    "[interaction-management][directed-interaction][directed-routing][time-management][tso]"
    "[ownership-ledger-state][tso-queue-state][tso-retraction-state][post-delivery-resignation]"
    "[save-restore][durable-save][filesystem][process-restart][restore]"
    "[process-restart-directed-interaction-tso-ownership-delivery-retraction]"
    "[public-process-restart-directed-interaction-tso-ownership-delivery-retraction]"
    "[public-process-restart-directed-interaction-tso-post-delivery-resignation]"
    "[service-report-file][service-reporting]"
    "[rti.service.get-object-class-handle][rti.service.get-attribute-handle]"
    "[rti.service.get-interaction-class-handle][rti.service.get-transportation-type-handle]"
    "[rti.service.publish-object-class-attributes][rti.service.subscribe-object-class-attributes]"
    "[rti.service.publish-object-class-directed-interactions]"
    "[rti.service.subscribe-object-class-directed-interactions]"
    "[rti.service.change-interaction-order-type]"
    "[rti.service.register-object-instance][rti.service.update-attribute-values]"
    "[rti.service.send-directed-interaction][rti.service.flush-queue-request]"
    "[rti.service.retract][rti.service.enable-time-regulation]"
    "[rti.service.disable-callbacks][rti.service.enable-callbacks]"
    "[rti.service.enable-time-constrained][rti.service.time-advance-request]"
    "[rti.service.request-federation-save][rti.service.federate-save-begun]"
    "[rti.service.federate-save-complete][rti.service.request-federation-restore]"
    "[rti.service.federate-restore-complete][rti.service.resign-federation-execution]"
    "[federate.callback.discover-object-instance][federate.callback.reflect-attribute-values]"
    "[federate.callback.receive-directed-interaction][federate.callback.request-retraction]"
    "[federate.callback.flush-queue-grant]"
    "[federate.callback.initiate-federate-save][federate.callback.federation-saved]"
    "[federate.callback.federation-restore-begun]"
    "[federate.callback.initiate-federate-restore][federate.callback.federation-restored]"
    "[federate.callback.time-advance-grant]"
    "[callback-immediate]") {
  using public_federation_restore_test_support::ScopedEmbeddedFederationRegistry;
  using public_federation_restore_test_support::configurationForServiceReportDirectory;
  using public_federation_restore_test_support::readTextFile;
  using public_federation_restore_test_support::serviceReportFiles;
  using public_federation_restore_test_support::temporaryFederationSaveDirectory;
  using public_federation_restore_test_support::temporaryServiceReportDirectory;
  auto runScenario = [](CallbackModel callbackModel) {
  auto const saveDirectory = temporaryFederationSaveDirectory();
  auto const saveStore =
      std::make_shared<umbra::detail::FilesystemFederationSaveCommitStore>(
          saveDirectory.path());
  auto const reportDirectory = temporaryServiceReportDirectory();
  auto sourceConfiguration =
      configurationForServiceReportDirectory(reportDirectory.path());
  sourceConfiguration.withRtiAddress(L"in-process");
  auto const federationName = nextFederationName();
  auto const objectConsumer =
      (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data" /
       "directed-interaction-object-consumer-fom.xml")
          .wstring();
  auto const interactionProvider =
      (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data" /
       "directed-interaction-interaction-provider-fom.xml")
          .wstring();
  auto const switchFom =
      (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data" /
       "switch-support-enabled-fom.xml")
          .wstring();
  std::vector<std::wstring> const fomModules{
      objectConsumer, interactionProvider, switchFom};
  std::wstring const saveLabel =
      L"public-directed-tso-post-delivery-resignation";
  std::string const tagBytes{"post-resign-tso"};
  VariableLengthData const tag(tagBytes.data(), tagBytes.size());
  FederateHandle sourceOwnerHandle;
  FederateHandle sourceSenderHandle;
  FederateHandle sourcePeerHandle;
  ObjectInstanceHandle sourceTarget;
  rti1516_2025::MessageRetractionHandle sourceRetraction;
  std::filesystem::path sourceReportFile;

  auto const drainAll = [](RTIambassador& owner,
                           RTIambassador& sender,
                           RTIambassador& peer) {
    for (int pass = 0; pass != 32; ++pass) {
      static_cast<void>(owner.evokeCallback(0.0));
      static_cast<void>(sender.evokeCallback(0.0));
      static_cast<void>(peer.evokeCallback(0.0));
    }
  };
  auto const drainPair = [](RTIambassador& owner, RTIambassador& sender) {
    for (int pass = 0; pass != 32; ++pass) {
      static_cast<void>(owner.evokeCallback(0.0));
      static_cast<void>(sender.evokeCallback(0.0));
    }
  };

  {
    auto const sourceRegistry =
        std::make_shared<umbra::detail::EmbeddedFederationRegistry>(
            nullptr, saveStore);
    ScopedEmbeddedFederationRegistry sourceScope(sourceRegistry);
    ReportingFederateAmbassador ownerReports;
    ReportingFederateAmbassador senderReports;
    ReportingFederateAmbassador peerReports;
    auto owner = makeRti();
    auto sender = makeRti();
    auto peer = makeRti();

    REQUIRE_NOTHROW(owner->connect(ownerReports, callbackModel));
    REQUIRE_NOTHROW(sender->connect(senderReports, callbackModel, sourceConfiguration));
    REQUIRE_NOTHROW(peer->connect(peerReports, callbackModel));
    REQUIRE_NOTHROW(owner->createFederationExecution(
        federationName, fomModules, standard_hla::mom::integer64_time));
    REQUIRE_NOTHROW(sourceOwnerHandle = owner->joinFederationExecution(
        L"public-directed-post-resign-owner", L"owner", federationName));
    REQUIRE_NOTHROW(sourceSenderHandle = sender->joinFederationExecution(
        L"public-directed-post-resign-sender", L"publisher", federationName));
    REQUIRE_NOTHROW(sourcePeerHandle = peer->joinFederationExecution(
        L"public-directed-post-resign-peer", L"subscriber", federationName));
    suppressDeclarationRelevanceAdvisories(*owner);
    suppressDeclarationRelevanceAdvisories(*sender);
    suppressDeclarationRelevanceAdvisories(*peer);

    auto const filesAtJoin = serviceReportFiles(reportDirectory.path());
    REQUIRE(filesAtJoin.size() == 1U);
    sourceReportFile = filesAtJoin.front();
    REQUIRE(std::filesystem::absolute(sourceReportFile).lexically_normal() ==
            sourceReportFile.lexically_normal());
    auto const initialReportText = readTextFile(sourceReportFile);

    auto const objectClass = owner->getObjectClassHandle(
        fixture_hla::fom::directed_fixture_object);
    auto const marker = owner->getAttributeHandle(
        objectClass, fixture_hla::fixture::directed_target_marker);
    auto const interactionClass = sender->getInteractionClassHandle(
        fixture_hla::fom::directed_fixture_interaction);
    InteractionClassHandleSet const directedClasses{interactionClass};
    REQUIRE(objectClass.isValid());
    REQUIRE(marker.isValid());
    REQUIRE(interactionClass.isValid());
    REQUIRE_NOTHROW(owner->publishObjectClassAttributes(
        objectClass, AttributeHandleSet{marker}));
    REQUIRE_NOTHROW(sender->subscribeObjectClassAttributes(
        objectClass, AttributeHandleSet{marker}));
    REQUIRE_NOTHROW(peer->subscribeObjectClassAttributes(
        objectClass, AttributeHandleSet{marker}));
    REQUIRE_NOTHROW(sender->publishObjectClassDirectedInteractions(
        objectClass, directedClasses));
    REQUIRE_NOTHROW(sender->changeInteractionOrderType(
        interactionClass, TIMESTAMP));
    // The owner remains the ownership-qualified recipient after the peer
    // resigns; the peer uses the universal selector only to create the route
    // that is delivered before resignation.
    REQUIRE_NOTHROW(owner->subscribeObjectClassDirectedInteractions(
        objectClass, directedClasses, false));
    REQUIRE_NOTHROW(peer->subscribeObjectClassDirectedInteractions(
        objectClass, directedClasses, true));

    REQUIRE_NOTHROW(sourceTarget = owner->registerObjectInstance(objectClass));
    REQUIRE(sourceTarget.isValid());
    drainAll(*owner, *sender, *peer);
    REQUIRE(senderReports.objectDiscoveryReports.size() == 1U);
    REQUIRE(peerReports.objectDiscoveryReports.size() == 1U);
    REQUIRE(owner->isAttributeOwnedByFederate(sourceTarget, marker));

    AttributeHandleValueMap markerValues;
    markerValues.emplace(marker, tag);
    REQUIRE_NOTHROW(owner->updateAttributeValues(
        sourceTarget, markerValues, tag));
    drainAll(*owner, *sender, *peer);
    REQUIRE(senderReports.attributeReflectionReports.size() == 1U);
    REQUIRE(peerReports.attributeReflectionReports.size() == 1U);
    senderReports.attributeReflectionReports.clear();
    peerReports.attributeReflectionReports.clear();

    REQUIRE_NOTHROW(owner->enableTimeConstrained());
    REQUIRE_FALSE(owner->evokeCallback(0.0));
    REQUIRE_NOTHROW(peer->enableTimeConstrained());
    REQUIRE_FALSE(peer->evokeCallback(0.0));
    REQUIRE_NOTHROW(sender->enableTimeRegulation(
        rti1516_2025::HLAinteger64Interval(1)));
    REQUIRE_FALSE(sender->evokeCallback(0.0));

    REQUIRE_NOTHROW(sourceRetraction = sender->sendDirectedInteraction(
        interactionClass,
        sourceTarget,
        ParameterHandleValueMap{},
        tag,
        rti1516_2025::HLAinteger64Time(9)));
    REQUIRE(sourceRetraction.isValid());
    REQUIRE(ownerReports.directedInteractionReports.empty());
    REQUIRE(peerReports.directedInteractionReports.empty());

    REQUIRE_NOTHROW(peer->flushQueueRequest(
        rti1516_2025::HLAinteger64Time(9)));
    while (peer->evokeCallback(0.0)) {
    }
    REQUIRE(peerReports.directedInteractionReports.size() == 1U);
    REQUIRE(peerReports.flushQueueGrantReports.size() == 1U);
    REQUIRE(peerReports.callbackOrder ==
            std::vector<std::string>{"directed", "flush-grant"});
    REQUIRE(ownerReports.directedInteractionReports.empty());

    // The peer has already crossed the directed callback boundary. Its
    // resignation must not invalidate the owner's still-queued,
    // ownership-qualified copy of the same TSO message.
    REQUIRE_NOTHROW(peer->resignFederationExecution(NO_ACTION));
    REQUIRE_NOTHROW(peer->disconnect());

    REQUIRE_NOTHROW(sender->requestFederationSave(
        saveLabel, rti1516_2025::HLAinteger64Time(7)));
    // HLA_IMMEDIATE executes a grant as soon as its request is admitted. Hold
    // both remaining dispatchers while building the shared save boundary so
    // the sender cannot run ahead of the ownership-qualified recipient.
    if (callbackModel == rti1516_2025::HLA_IMMEDIATE) {
      REQUIRE_NOTHROW(owner->disableCallbacks());
      REQUIRE_NOTHROW(sender->disableCallbacks());
    }
    REQUIRE_NOTHROW(sender->timeAdvanceRequest(
        rti1516_2025::HLAinteger64Time(7)));
    REQUIRE_FALSE(sender->evokeCallback(0.0));
    REQUIRE_NOTHROW(owner->timeAdvanceRequest(
        rti1516_2025::HLAinteger64Time(7)));
    if (callbackModel == rti1516_2025::HLA_IMMEDIATE) {
      REQUIRE_NOTHROW(owner->enableCallbacks());
      REQUIRE_NOTHROW(sender->enableCallbacks());
    }
    drainPair(*owner, *sender);
    REQUIRE(ownerReports.initiateFederateSaveReports ==
            std::vector<std::wstring>{saveLabel});
    REQUIRE(senderReports.initiateFederateSaveReports ==
            std::vector<std::wstring>{saveLabel});
    REQUIRE_NOTHROW(owner->federateSaveBegun());
    REQUIRE_NOTHROW(sender->federateSaveBegun());
    REQUIRE_NOTHROW(owner->federateSaveComplete());
    REQUIRE_NOTHROW(sender->federateSaveComplete());
    drainPair(*owner, *sender);
    REQUIRE(ownerReports.federationSavedReportCount == 1U);
    REQUIRE(senderReports.federationSavedReportCount == 1U);

    auto const durable = saveStore->load(federationName, saveLabel);
    REQUIRE(durable.has_value());
    auto const durableImage =
        umbra::detail::FederationStateImageCodec::decode(durable->stateImage);
    REQUIRE(durableImage.members.size() == 2U);
    REQUIRE(durableImage.objects.size() == 1U);
    auto const& savedObject = durableImage.objects.front();
    REQUIRE(savedObject.pendingConfirmDivestitureNotifications.empty());
    REQUIRE(savedObject.ownershipAssumptionRecipientsByAttribute.empty());
    REQUIRE(durableImage.tsoDirectedInteractionMessages.size() == 1U);
    auto const& savedMessage =
        durableImage.tsoDirectedInteractionMessages.front();
    REQUIRE(savedMessage.recipients.size() == 2U);
    REQUIRE(durableImage.tsoRequestRetractionRecords.size() == 1U);
    auto const& savedRetraction =
        durableImage.tsoRequestRetractionRecords.front();
    REQUIRE(savedRetraction.recipientStates.size() == 2U);
    REQUIRE(durableImage.tsoQueueEntries.size() == 1U);

    auto const ownerValue =
        rti1516_2025::umbra_binding_detail::federateHandleValue(
            sourceOwnerHandle);
    auto const senderValue =
        rti1516_2025::umbra_binding_detail::federateHandleValue(
            sourceSenderHandle);
    auto const peerValue =
        rti1516_2025::umbra_binding_detail::federateHandleValue(
            sourcePeerHandle);
    REQUIRE(ownerValue.has_value());
    REQUIRE(senderValue.has_value());
    REQUIRE(peerValue.has_value());
    REQUIRE(std::none_of(
        durableImage.members.begin(),
        durableImage.members.end(),
        [&](umbra::detail::FederationStateImageMember const& member) {
          return member.id == *peerValue;
        }));
    REQUIRE(std::any_of(
        durableImage.members.begin(),
        durableImage.members.end(),
        [&](umbra::detail::FederationStateImageMember const& member) {
          return member.id == *ownerValue;
        }));
    REQUIRE(std::any_of(
        durableImage.members.begin(),
        durableImage.members.end(),
        [&](umbra::detail::FederationStateImageMember const& member) {
          return member.id == *senderValue;
        }));
    REQUIRE(std::any_of(
        savedMessage.recipients.begin(),
        savedMessage.recipients.end(),
        [&](umbra::detail::FederationStateImageTsoDirectedInteractionRecipient const& recipient) {
          return recipient.receivingFederateId == *ownerValue;
        }));
    REQUIRE(std::any_of(
        savedMessage.recipients.begin(),
        savedMessage.recipients.end(),
        [&](umbra::detail::FederationStateImageTsoDirectedInteractionRecipient const& recipient) {
          return recipient.receivingFederateId == *peerValue;
        }));
    auto const savedOwnerState = std::find_if(
        savedRetraction.recipientStates.begin(),
        savedRetraction.recipientStates.end(),
        [&](umbra::detail::FederationStateImageTsoRequestRetractionRecipient const& recipient) {
          return recipient.receivingFederateId == *ownerValue;
        });
    auto const savedPeerState = std::find_if(
        savedRetraction.recipientStates.begin(),
        savedRetraction.recipientStates.end(),
        [&](umbra::detail::FederationStateImageTsoRequestRetractionRecipient const& recipient) {
          return recipient.receivingFederateId == *peerValue;
        });
    REQUIRE(savedOwnerState != savedRetraction.recipientStates.end());
    REQUIRE(savedPeerState != savedRetraction.recipientStates.end());
    REQUIRE(savedOwnerState->state == 0U);
    REQUIRE(savedPeerState->state == 1U);
    REQUIRE(savedMessage.receivedOrderType ==
            static_cast<std::uint32_t>(TIMESTAMP));
    REQUIRE(durableImage.tsoQueueEntries.front().recipientFederateId == *ownerValue);
    REQUIRE(durableImage.tsoQueueEntries.front().phase == 0U);

    auto const reportAfterSave = readTextFile(sourceReportFile);
    REQUIRE(reportAfterSave.size() > initialReportText.size());
    REQUIRE(reportAfterSave.find("RequestFederationSave") != std::string::npos);
    REQUIRE(reportAfterSave.find("FederateSaveComplete") != std::string::npos);

    REQUIRE_NOTHROW(owner->resignFederationExecution(
        CANCEL_THEN_DELETE_THEN_DIVEST));
    REQUIRE_NOTHROW(sender->resignFederationExecution(NO_ACTION));
    REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
    REQUIRE_NOTHROW(owner->disconnect());
    REQUIRE_NOTHROW(sender->disconnect());
  }

  {
    auto const freshRegistry =
        std::make_shared<umbra::detail::EmbeddedFederationRegistry>(
            nullptr, saveStore);
    ScopedEmbeddedFederationRegistry freshScope(freshRegistry);
    ReportingFederateAmbassador ownerReports;
    ReportingFederateAmbassador senderReports;
    auto owner = makeRti();
    auto sender = makeRti();
    auto freshConfiguration =
        configurationForServiceReportDirectory(reportDirectory.path());
    freshConfiguration.withRtiAddress(L"in-process");

    REQUIRE_NOTHROW(owner->connect(ownerReports, callbackModel));
    REQUIRE_NOTHROW(sender->connect(senderReports, callbackModel, freshConfiguration));
    REQUIRE_NOTHROW(owner->createFederationExecution(
        federationName, fomModules, standard_hla::mom::integer64_time));
    FederateHandle freshOwnerHandle;
    FederateHandle freshSenderHandle;
    REQUIRE_NOTHROW(freshOwnerHandle = owner->joinFederationExecution(
        L"public-directed-post-resign-owner", L"owner", federationName));
    REQUIRE_NOTHROW(freshSenderHandle = sender->joinFederationExecution(
        L"public-directed-post-resign-sender", L"publisher", federationName));
    REQUIRE(freshOwnerHandle == sourceOwnerHandle);
    REQUIRE(freshSenderHandle == sourceSenderHandle);
    suppressDeclarationRelevanceAdvisories(*owner);
    suppressDeclarationRelevanceAdvisories(*sender);

    auto const filesAfterFreshJoin = serviceReportFiles(reportDirectory.path());
    REQUIRE(filesAfterFreshJoin.size() == 2U);
    REQUIRE(std::find(filesAfterFreshJoin.begin(), filesAfterFreshJoin.end(),
                      sourceReportFile) != filesAfterFreshJoin.end());
    auto const freshReportFile =
        filesAfterFreshJoin.front() == sourceReportFile
            ? filesAfterFreshJoin.back()
            : filesAfterFreshJoin.front();
    REQUIRE(freshReportFile != sourceReportFile);
    auto const freshInitialReportText = readTextFile(freshReportFile);

    auto const freshInteractionClass = sender->getInteractionClassHandle(
        fixture_hla::fom::directed_fixture_interaction);
    REQUIRE(freshInteractionClass.isValid());
    // The sender owns the configured service-report file; use it as the
    // restore requester so the public MOM audit remains observable in the
    // same per-joined-federate file across the restart boundary.
    REQUIRE_NOTHROW(sender->requestFederationRestore(saveLabel));
    drainPair(*owner, *sender);
    REQUIRE(senderReports.requestFederationRestoreSucceededReports ==
            std::vector<std::wstring>{saveLabel});
    REQUIRE(ownerReports.federationRestoreBegunReportCount == 1U);
    REQUIRE(senderReports.federationRestoreBegunReportCount == 1U);
    REQUIRE(ownerReports.initiateFederateRestoreReports.size() == 1U);
    REQUIRE(senderReports.initiateFederateRestoreReports.size() == 1U);
    REQUIRE_NOTHROW(owner->federateRestoreComplete());
    REQUIRE_NOTHROW(sender->federateRestoreComplete());
    drainPair(*owner, *sender);
    REQUIRE(ownerReports.federationRestoredReportCount == 1U);
    REQUIRE(senderReports.federationRestoredReportCount == 1U);
    REQUIRE(ownerReports.directedInteractionReports.empty());

    auto const freshObjectClass = owner->getObjectClassHandle(
        fixture_hla::fom::directed_fixture_object);
    auto const freshMarker = owner->getAttributeHandle(
        freshObjectClass, fixture_hla::fixture::directed_target_marker);
    REQUIRE(freshObjectClass.isValid());
    REQUIRE(freshMarker.isValid());
    ownerReports.callbackOrder.clear();
    REQUIRE_NOTHROW(owner->flushQueueRequest(
        rti1516_2025::HLAinteger64Time(9)));
    while (owner->evokeCallback(0.0)) {
    }
    REQUIRE(ownerReports.directedInteractionReports.size() == 1U);
    REQUIRE(ownerReports.flushQueueGrantReports.size() == 1U);
    REQUIRE(ownerReports.callbackOrder ==
            std::vector<std::string>{"directed", "flush-grant"});
    auto const reliable = sender->getTransportationTypeHandle(
        standard_hla::mom::reliable);
    auto const& delivered = ownerReports.directedInteractionReports.front();
    REQUIRE(delivered.interactionClass == freshInteractionClass);
    REQUIRE(delivered.objectInstance == sourceTarget);
    REQUIRE(delivered.parameterValues.empty());
    REQUIRE(variableLengthDataBytes(delivered.userSuppliedTag) ==
            std::vector<unsigned char>(tagBytes.begin(), tagBytes.end()));
    REQUIRE(delivered.transportationType == reliable);
    REQUIRE(delivered.producingFederate == freshSenderHandle);
    REQUIRE(delivered.timeImplementationName == standard_hla::mom::integer64_time);
    REQUIRE(delivered.timeValue == L"9");
    REQUIRE(delivered.sentOrderType == TIMESTAMP);
    REQUIRE(delivered.receivedOrderType == TIMESTAMP);
    REQUIRE(delivered.retractionSupplied);
    REQUIRE(delivered.retractionValid);

    REQUIRE_NOTHROW(sender->retract(sourceRetraction));
    drainPair(*owner, *sender);
    REQUIRE(ownerReports.requestRetractionReports.size() == 1U);
    REQUIRE(ownerReports.requestRetractionReports.front().retractionValid);
    REQUIRE(variableLengthDataBytes(
                ownerReports.requestRetractionReports.front().encodedRetraction) ==
            variableLengthDataBytes(sourceRetraction.encode()));
    REQUIRE(ownerReports.callbackOrder.back() == "request-retraction");
    REQUIRE(senderReports.requestRetractionReports.empty());

    auto const filesAfterRestore = serviceReportFiles(reportDirectory.path());
    REQUIRE(filesAfterRestore == filesAfterFreshJoin);
    auto const freshReportAfterRestore = readTextFile(freshReportFile);
    REQUIRE(freshReportAfterRestore.size() > freshInitialReportText.size());
    REQUIRE(freshReportAfterRestore.find("RequestFederationRestore") !=
            std::string::npos);
    REQUIRE(freshReportAfterRestore.find("FederateRestoreComplete") !=
            std::string::npos);

    REQUIRE_NOTHROW(owner->resignFederationExecution(
        CANCEL_THEN_DELETE_THEN_DIVEST));
    REQUIRE_NOTHROW(sender->resignFederationExecution(NO_ACTION));
    REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
    REQUIRE_NOTHROW(owner->disconnect());
    REQUIRE_NOTHROW(sender->disconnect());
  }
  };

  SECTION("HLA_EVOKED") {
    runScenario(HLA_EVOKED);
  }
  SECTION("HLA_IMMEDIATE") {
    runScenario(rti1516_2025::HLA_IMMEDIATE);
  }
}
}
