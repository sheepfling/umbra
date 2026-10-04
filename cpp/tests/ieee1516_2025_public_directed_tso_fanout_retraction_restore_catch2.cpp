#include "ieee1516_2025_federation_management_fixture_support.hpp"
#include "ieee1516_2025_public_federation_restore_test_support.hpp"

namespace restore_support = public_federation_restore_test_support;

namespace {
TEST_CASE(
    "Embedded public fresh-registry restore rebinds directed TSO fan-out and retracts only the delivered recipient under HLA_EVOKED and HLA_IMMEDIATE",
    "[integration][development-profile][federation-management][interaction-management][directed][save-restore]"
    "[durable-save][filesystem][process-restart][restore][time-management][tso]"
    "[directed-interaction][directed-routing][tso-retraction-state][request-retraction]"
    "[service-report-file][service-reporting]"
    "[process-restart-directed-interaction-tso-fanout]"
    "[public-process-restart-directed-interaction-tso-fanout]"
    "[rti.service.get-object-class-handle][rti.service.get-attribute-handle]"
    "[rti.service.get-interaction-class-handle][rti.service.get-transportation-type-handle]"
    "[rti.service.publish-object-class-attributes]"
    "[rti.service.subscribe-object-class-attributes]"
    "[rti.service.publish-object-class-directed-interactions]"
    "[rti.service.subscribe-object-class-directed-interactions]"
    "[rti.service.change-interaction-order-type]"
    "[rti.service.register-object-instance][rti.service.send-directed-interaction]"
    "[rti.service.flush-queue-request][rti.service.retract]"
    "[rti.service.disable-callbacks][rti.service.enable-callbacks]"
    "[rti.service.enable-time-regulation][rti.service.enable-time-constrained]"
    "[rti.service.request-federation-save][rti.service.time-advance-request]"
    "[rti.service.federate-save-begun][rti.service.federate-save-complete]"
    "[rti.service.request-federation-restore][rti.service.federate-restore-complete]"
    "[rti.service.resign-federation-execution]"
    "[federate.callback.discover-object-instance][federate.callback.receive-directed-interaction]"
    "[federate.callback.request-retraction][federate.callback.flush-queue-grant]"
    "[federate.callback.initiate-federate-save][federate.callback.federation-saved]"
    "[federate.callback.federation-restore-begun]"
    "[federate.callback.initiate-federate-restore][federate.callback.federation-restored]"
    "[federate.callback.time-advance-grant]"
    "[callback-immediate]"
    "[directed-tso-fanout-retraction-restore]") {
  auto runScenario = [](CallbackModel callbackModel) {
  auto const saveDirectory = restore_support::temporaryFederationSaveDirectory();
  auto const saveStore = std::make_shared<umbra::detail::FilesystemFederationSaveCommitStore>(
      saveDirectory.path());
  auto const reportDirectory = restore_support::temporaryServiceReportDirectory();
  auto sourceConfiguration = restore_support::configurationForServiceReportDirectory(
      reportDirectory.path());
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
  std::vector<std::wstring> const fomModules{objectConsumer, interactionProvider, switchFom};
  std::wstring const saveLabel = L"public-fresh-directed-tso-fanout";
  unsigned char const tagBytes[] = {0x44, 0x54, 0x53, 0x4F};
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));
  FederateHandle sourceSenderHandle;
  ObjectInstanceHandle sourceTarget;
  rti1516_2025::MessageRetractionHandle sourceRetraction;
  std::filesystem::path sourceReportFile;

  auto const drainAll = [&](RTIambassador& first,
                            RTIambassador& second,
                            RTIambassador& third,
                            RTIambassador& fourth) {
    for (int pass = 0; pass != 32; ++pass) {
      static_cast<void>(first.evokeCallback(0.0));
      static_cast<void>(second.evokeCallback(0.0));
      static_cast<void>(third.evokeCallback(0.0));
      static_cast<void>(fourth.evokeCallback(0.0));
    }
  };

  {
    auto const sourceRegistry = std::make_shared<umbra::detail::EmbeddedFederationRegistry>(
        nullptr,
        saveStore);
    restore_support::ScopedEmbeddedFederationRegistry sourceScope(sourceRegistry);
    ReportingFederateAmbassador ownerReports;
    ReportingFederateAmbassador senderReports;
    ReportingFederateAmbassador receiverAReports;
    ReportingFederateAmbassador receiverBReports;
    auto owner = makeRti();
    auto sender = makeRti();
    auto receiverA = makeRti();
    auto receiverB = makeRti();

    REQUIRE_NOTHROW(owner->connect(ownerReports, callbackModel));
    REQUIRE_NOTHROW(sender->connect(senderReports, callbackModel, sourceConfiguration));
    REQUIRE_NOTHROW(receiverA->connect(receiverAReports, callbackModel));
    REQUIRE_NOTHROW(receiverB->connect(receiverBReports, callbackModel));
    REQUIRE_NOTHROW(owner->createFederationExecution(
        federationName,
        fomModules,
        standard_hla::mom::integer64_time));
    REQUIRE_NOTHROW(owner->joinFederationExecution(
        L"public-fresh-directed-owner",
        L"owner",
        federationName));
    REQUIRE_NOTHROW(sourceSenderHandle = sender->joinFederationExecution(
        L"public-fresh-directed-sender",
        L"publisher",
        federationName));
    REQUIRE_NOTHROW(receiverA->joinFederationExecution(
        L"public-fresh-directed-receiver-a",
        L"subscriber",
        federationName));
    REQUIRE_NOTHROW(receiverB->joinFederationExecution(
        L"public-fresh-directed-receiver-b",
        L"subscriber",
        federationName));
    suppressDeclarationRelevanceAdvisories(*owner);
    suppressDeclarationRelevanceAdvisories(*sender);
    suppressDeclarationRelevanceAdvisories(*receiverA);
    suppressDeclarationRelevanceAdvisories(*receiverB);

    auto const filesAtJoin = restore_support::serviceReportFiles(reportDirectory.path());
    REQUIRE(filesAtJoin.size() == 1U);
    sourceReportFile = filesAtJoin.front();
    REQUIRE(std::filesystem::absolute(sourceReportFile).lexically_normal() ==
            sourceReportFile.lexically_normal());

    auto const objectClass = owner->getObjectClassHandle(
        fixture_hla::fom::directed_fixture_object);
    auto const marker = owner->getAttributeHandle(
        objectClass,
        fixture_hla::fixture::directed_target_marker);
    auto const interactionClass = sender->getInteractionClassHandle(
        fixture_hla::fom::directed_fixture_interaction);
    InteractionClassHandleSet const directedClasses{interactionClass};
    REQUIRE(objectClass.isValid());
    REQUIRE(marker.isValid());
    REQUIRE(interactionClass.isValid());
    REQUIRE_NOTHROW(owner->publishObjectClassAttributes(objectClass, {marker}));
    REQUIRE_NOTHROW(sender->subscribeObjectClassAttributes(objectClass, {marker}));
    REQUIRE_NOTHROW(receiverA->subscribeObjectClassAttributes(objectClass, {marker}));
    REQUIRE_NOTHROW(receiverB->subscribeObjectClassAttributes(objectClass, {marker}));
    REQUIRE_NOTHROW(sender->publishObjectClassDirectedInteractions(
        objectClass,
        directedClasses));
    REQUIRE_NOTHROW(sender->changeInteractionOrderType(interactionClass, TIMESTAMP));
    REQUIRE_NOTHROW(receiverA->subscribeObjectClassDirectedInteractions(
        objectClass,
        directedClasses,
        true));
    REQUIRE_NOTHROW(receiverB->subscribeObjectClassDirectedInteractions(
        objectClass,
        directedClasses,
        true));

    REQUIRE_NOTHROW(sourceTarget = owner->registerObjectInstance(objectClass));
    REQUIRE(sourceTarget.isValid());
    drainAll(*owner, *sender, *receiverA, *receiverB);
    REQUIRE(senderReports.objectDiscoveryReports.size() == 1U);
    REQUIRE(receiverAReports.objectDiscoveryReports.size() == 1U);
    REQUIRE(receiverBReports.objectDiscoveryReports.size() == 1U);

    // Give the restored target a route-free application-value projection.
    // The fresh-registry restore contract admits an object ledger only when
    // its registered object carries at least one published attribute value;
    // the directed interaction itself remains the pending timestamped work.
    AttributeHandleValueMap markerValues;
    markerValues.emplace(marker, tag);
    REQUIRE_NOTHROW(owner->updateAttributeValues(sourceTarget, markerValues, tag));
    drainAll(*owner, *sender, *receiverA, *receiverB);
    REQUIRE(senderReports.attributeReflectionReports.size() == 1U);
    REQUIRE(receiverAReports.attributeReflectionReports.size() == 1U);
    REQUIRE(receiverBReports.attributeReflectionReports.size() == 1U);
    senderReports.attributeReflectionReports.clear();
    receiverAReports.attributeReflectionReports.clear();
    receiverBReports.attributeReflectionReports.clear();
    senderReports.callbackOrder.clear();
    receiverAReports.callbackOrder.clear();
    receiverBReports.callbackOrder.clear();

    REQUIRE_NOTHROW(receiverA->enableTimeConstrained());
    REQUIRE_FALSE(receiverA->evokeCallback(0.0));
    REQUIRE_NOTHROW(receiverB->enableTimeConstrained());
    REQUIRE_FALSE(receiverB->evokeCallback(0.0));
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
    REQUIRE(receiverAReports.directedInteractionReports.empty());
    REQUIRE(receiverBReports.directedInteractionReports.empty());

    REQUIRE_NOTHROW(sender->requestFederationSave(
        saveLabel,
        rti1516_2025::HLAinteger64Time(7)));
    // HLA_IMMEDIATE would otherwise execute the sender's grant before both
    // constrained subscribers have staged their requests.  Hold the four
    // dispatchers while building the shared save boundary, then release the
    // non-constrained routes followed by the two constrained recipients.
    if (callbackModel == rti1516_2025::HLA_IMMEDIATE) {
      REQUIRE_NOTHROW(owner->disableCallbacks());
      REQUIRE_NOTHROW(sender->disableCallbacks());
      REQUIRE_NOTHROW(receiverA->disableCallbacks());
      REQUIRE_NOTHROW(receiverB->disableCallbacks());
    }
    REQUIRE_NOTHROW(sender->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(7)));
    REQUIRE_FALSE(sender->evokeCallback(0.0));
    REQUIRE_NOTHROW(receiverA->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(7)));
    REQUIRE_NOTHROW(receiverB->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(7)));
    if (callbackModel == rti1516_2025::HLA_IMMEDIATE) {
      REQUIRE_NOTHROW(owner->enableCallbacks());
      REQUIRE_NOTHROW(sender->enableCallbacks());
      REQUIRE_NOTHROW(receiverA->enableCallbacks());
      REQUIRE_NOTHROW(receiverB->enableCallbacks());
    }
    drainAll(*owner, *sender, *receiverA, *receiverB);
    REQUIRE(ownerReports.initiateFederateSaveReports ==
            std::vector<std::wstring>{saveLabel});
    REQUIRE(senderReports.initiateFederateSaveReports ==
            std::vector<std::wstring>{saveLabel});
    REQUIRE(receiverAReports.initiateFederateSaveReports ==
            std::vector<std::wstring>{saveLabel});
    REQUIRE(receiverBReports.initiateFederateSaveReports ==
            std::vector<std::wstring>{saveLabel});
    REQUIRE_NOTHROW(owner->federateSaveBegun());
    REQUIRE_NOTHROW(sender->federateSaveBegun());
    REQUIRE_NOTHROW(receiverA->federateSaveBegun());
    REQUIRE_NOTHROW(receiverB->federateSaveBegun());
    REQUIRE_NOTHROW(owner->federateSaveComplete());
    REQUIRE_NOTHROW(sender->federateSaveComplete());
    REQUIRE_NOTHROW(receiverA->federateSaveComplete());
    REQUIRE_NOTHROW(receiverB->federateSaveComplete());
    drainAll(*owner, *sender, *receiverA, *receiverB);
    REQUIRE(ownerReports.federationSavedReportCount == 1U);
    REQUIRE(senderReports.federationSavedReportCount == 1U);
    REQUIRE(receiverAReports.federationSavedReportCount == 1U);
    REQUIRE(receiverBReports.federationSavedReportCount == 1U);

    auto const durable = saveStore->load(federationName, saveLabel);
    REQUIRE(durable.has_value());
    auto const durableImage = umbra::detail::FederationStateImageCodec::decode(
        durable->stateImage);
    REQUIRE(durableImage.tsoDirectedInteractionMessages.size() == 1U);
    // The switch-support FOM enables delayed subscription evaluation.  The
    // owner has no directed subscription, but its live callback route is
    // retained as a deferred recipient; receiver A/B are the two explicit
    // directed subscribers.  All three copies must survive save/restore so
    // Retract can terminalize the undelivered routes deterministically.
    REQUIRE(durableImage.tsoDirectedInteractionMessages.front().recipients.size() == 3U);
    REQUIRE(durableImage.tsoRequestRetractionRecords.size() == 1U);
    REQUIRE(durableImage.tsoRequestRetractionRecords.front().recipientStates.size() == 3U);
    // Only the two time-constrained subscribers have concrete queue entries;
    // the delayed owner route is represented by the recipient state above.
    REQUIRE(durableImage.tsoQueueEntries.size() == 2U);

    REQUIRE_NOTHROW(receiverA->resignFederationExecution(NO_ACTION));
    REQUIRE_NOTHROW(receiverB->resignFederationExecution(NO_ACTION));
    REQUIRE_NOTHROW(sender->resignFederationExecution(NO_ACTION));
    REQUIRE_NOTHROW(owner->resignFederationExecution(NO_ACTION));
    REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
    REQUIRE_NOTHROW(receiverA->disconnect());
    REQUIRE_NOTHROW(receiverB->disconnect());
    REQUIRE_NOTHROW(sender->disconnect());
    REQUIRE_NOTHROW(owner->disconnect());
  }

  {
    auto const freshRegistry = std::make_shared<umbra::detail::EmbeddedFederationRegistry>(
        nullptr,
        saveStore);
    restore_support::ScopedEmbeddedFederationRegistry freshScope(freshRegistry);
    ReportingFederateAmbassador ownerReports;
    ReportingFederateAmbassador senderReports;
    ReportingFederateAmbassador receiverAReports;
    ReportingFederateAmbassador receiverBReports;
    auto owner = makeRti();
    auto sender = makeRti();
    auto receiverA = makeRti();
    auto receiverB = makeRti();
    auto freshConfiguration = configurationForServiceReportDirectory(reportDirectory.path());
    freshConfiguration.withRtiAddress(L"in-process");

    REQUIRE_NOTHROW(owner->connect(ownerReports, callbackModel));
    REQUIRE_NOTHROW(sender->connect(senderReports, callbackModel, freshConfiguration));
    REQUIRE_NOTHROW(receiverA->connect(receiverAReports, callbackModel));
    REQUIRE_NOTHROW(receiverB->connect(receiverBReports, callbackModel));
    REQUIRE_NOTHROW(owner->createFederationExecution(
        federationName,
        fomModules,
        standard_hla::mom::integer64_time));
    REQUIRE_NOTHROW(owner->joinFederationExecution(
        L"public-fresh-directed-owner",
        L"owner",
        federationName));
    FederateHandle freshSenderHandle;
    REQUIRE_NOTHROW(freshSenderHandle = sender->joinFederationExecution(
        L"public-fresh-directed-sender",
        L"publisher",
        federationName));
    REQUIRE_NOTHROW(receiverA->joinFederationExecution(
        L"public-fresh-directed-receiver-a",
        L"subscriber",
        federationName));
    REQUIRE_NOTHROW(receiverB->joinFederationExecution(
        L"public-fresh-directed-receiver-b",
        L"subscriber",
        federationName));
    REQUIRE(freshSenderHandle == sourceSenderHandle);
    suppressDeclarationRelevanceAdvisories(*owner);
    suppressDeclarationRelevanceAdvisories(*sender);
    suppressDeclarationRelevanceAdvisories(*receiverA);
    suppressDeclarationRelevanceAdvisories(*receiverB);

    auto const filesAfterFreshJoin = restore_support::serviceReportFiles(reportDirectory.path());
    REQUIRE(filesAfterFreshJoin.size() == 2U);
    REQUIRE(std::find(filesAfterFreshJoin.begin(), filesAfterFreshJoin.end(), sourceReportFile) !=
            filesAfterFreshJoin.end());
    auto const freshReportFile = filesAfterFreshJoin.front() == sourceReportFile
        ? filesAfterFreshJoin.back()
        : filesAfterFreshJoin.front();
    REQUIRE(freshReportFile != sourceReportFile);

    auto const interactionClass = sender->getInteractionClassHandle(
        fixture_hla::fom::directed_fixture_interaction);
    REQUIRE(interactionClass.isValid());
    REQUIRE_NOTHROW(sender->requestFederationRestore(saveLabel));
    drainAll(*owner, *sender, *receiverA, *receiverB);
    REQUIRE(senderReports.requestFederationRestoreSucceededReports ==
            std::vector<std::wstring>{saveLabel});
    REQUIRE(ownerReports.federationRestoreBegunReportCount == 1U);
    REQUIRE(senderReports.federationRestoreBegunReportCount == 1U);
    REQUIRE(receiverAReports.federationRestoreBegunReportCount == 1U);
    REQUIRE(receiverBReports.federationRestoreBegunReportCount == 1U);
    REQUIRE_NOTHROW(owner->federateRestoreComplete());
    REQUIRE_NOTHROW(sender->federateRestoreComplete());
    REQUIRE_NOTHROW(receiverA->federateRestoreComplete());
    REQUIRE_NOTHROW(receiverB->federateRestoreComplete());
    drainAll(*owner, *sender, *receiverA, *receiverB);
    REQUIRE(ownerReports.federationRestoredReportCount == 1U);
    REQUIRE(senderReports.federationRestoredReportCount == 1U);
    REQUIRE(receiverAReports.federationRestoredReportCount == 1U);
    REQUIRE(receiverBReports.federationRestoredReportCount == 1U);
    REQUIRE(receiverAReports.directedInteractionReports.empty());
    REQUIRE(receiverBReports.directedInteractionReports.empty());
    REQUIRE(ownerReports.directedInteractionReports.empty());

    receiverAReports.callbackOrder.clear();
    receiverBReports.callbackOrder.clear();
    REQUIRE_NOTHROW(receiverA->flushQueueRequest(
        rti1516_2025::HLAinteger64Time(9)));
    while (receiverA->evokeCallback(0.0)) {
    }
    REQUIRE(receiverAReports.directedInteractionReports.size() == 1U);
    REQUIRE(receiverAReports.flushQueueGrantReports.size() == 1U);
    REQUIRE(receiverAReports.callbackOrder ==
            std::vector<std::string>{"directed", "flush-grant"});
    REQUIRE(receiverBReports.directedInteractionReports.empty());
    auto const reliable = sender->getTransportationTypeHandle(standard_hla::mom::reliable);
    auto const& delivered = receiverAReports.directedInteractionReports.front();
    REQUIRE(delivered.interactionClass == interactionClass);
    REQUIRE(delivered.objectInstance == sourceTarget);
    REQUIRE(delivered.parameterValues.empty());
    REQUIRE(variableLengthDataBytes(delivered.userSuppliedTag) ==
            std::vector<unsigned char>(tagBytes, tagBytes + sizeof(tagBytes)));
    REQUIRE(delivered.transportationType == reliable);
    REQUIRE(delivered.producingFederate == freshSenderHandle);
    REQUIRE(delivered.timeImplementationName == standard_hla::mom::integer64_time);
    REQUIRE(delivered.timeValue == L"9");
    REQUIRE(delivered.sentOrderType == TIMESTAMP);
    REQUIRE(delivered.receivedOrderType == TIMESTAMP);
    REQUIRE(delivered.retractionSupplied);
    REQUIRE(delivered.retractionValid);

    REQUIRE_NOTHROW(sender->retract(sourceRetraction));
    while (receiverA->evokeCallback(0.0)) {
    }
    while (receiverB->evokeCallback(0.0)) {
    }
    REQUIRE(receiverAReports.requestRetractionReports.size() == 1U);
    REQUIRE(receiverAReports.requestRetractionReports.front().retractionValid);
    REQUIRE(variableLengthDataBytes(
                receiverAReports.requestRetractionReports.front().encodedRetraction) ==
            variableLengthDataBytes(sourceRetraction.encode()));
    REQUIRE(receiverAReports.callbackOrder.back() == "request-retraction");
    REQUIRE(receiverBReports.requestRetractionReports.empty());
    REQUIRE(receiverBReports.directedInteractionReports.empty());
    REQUIRE(ownerReports.requestRetractionReports.empty());
    REQUIRE(ownerReports.directedInteractionReports.empty());

    // Cross the still-queued recipient's callback boundary after Retract. The
    // restored copy is suppressed, so it must not enter user code or generate
    // a second Request Retraction.
    REQUIRE_NOTHROW(receiverB->flushQueueRequest(
        rti1516_2025::HLAinteger64Time(9)));
    while (receiverB->evokeCallback(0.0)) {
    }
    REQUIRE(receiverBReports.directedInteractionReports.empty());
    REQUIRE(receiverBReports.requestRetractionReports.empty());
    REQUIRE(serviceReportFiles(reportDirectory.path()) == filesAfterFreshJoin);

    REQUIRE_NOTHROW(receiverA->resignFederationExecution(NO_ACTION));
    REQUIRE_NOTHROW(receiverB->resignFederationExecution(NO_ACTION));
    REQUIRE_NOTHROW(sender->resignFederationExecution(NO_ACTION));
    REQUIRE_NOTHROW(owner->resignFederationExecution(NO_ACTION));
    REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
    REQUIRE_NOTHROW(receiverA->disconnect());
    REQUIRE_NOTHROW(receiverB->disconnect());
    REQUIRE_NOTHROW(sender->disconnect());
    REQUIRE_NOTHROW(owner->disconnect());
  }
  };

  SECTION("HLA_EVOKED") {
    runScenario(HLA_EVOKED);
  }
  SECTION("HLA_IMMEDIATE") {
    runScenario(rti1516_2025::HLA_IMMEDIATE);
  }
}
} // namespace
