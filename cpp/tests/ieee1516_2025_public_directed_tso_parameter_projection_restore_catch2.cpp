#include "ieee1516_2025_federation_management_fixture_support.hpp"
#include "ieee1516_2025_public_federation_restore_test_support.hpp"

namespace restore_support = public_federation_restore_test_support;

namespace {
TEST_CASE(
    "Embedded public fresh-registry parameterized directed TSO preserves parameter projection and timestamped order metadata under HLA_EVOKED and HLA_IMMEDIATE",
    "[integration][development-profile][federation-management][interaction-management][directed][save-restore]"
    "[durable-save][filesystem][process-restart][restore][time-management][tso]"
    "[directed-interaction][directed-routing][tso-payload-state]"
    "[service-report-file][service-reporting]"
    "[process-restart-directed-interaction-tso-parameter-projection]"
    "[public-process-restart-directed-interaction-tso-parameter-projection][public-directed-tso-parameter-projection-restore]"
    "[rti.service.get-object-class-handle][rti.service.get-attribute-handle]"
    "[rti.service.get-interaction-class-handle][rti.service.get-parameter-handle]"
    "[rti.service.get-transportation-type-handle]"
    "[rti.service.publish-object-class-attributes]"
    "[rti.service.subscribe-object-class-attributes]"
    "[rti.service.publish-object-class-directed-interactions]"
    "[rti.service.subscribe-object-class-directed-interactions]"
    "[rti.service.change-interaction-order-type]"
    "[rti.service.register-object-instance][rti.service.send-directed-interaction]"
    "[rti.service.flush-queue-request]"
    "[rti.service.disable-callbacks][rti.service.enable-callbacks]"
    "[rti.service.enable-time-regulation][rti.service.enable-time-constrained]"
    "[rti.service.request-federation-save]"
    "[rti.service.federate-save-begun][rti.service.federate-save-complete]"
    "[rti.service.request-federation-restore][rti.service.federate-restore-complete]"
    "[rti.service.resign-federation-execution]"
    "[federate.callback.discover-object-instance][federate.callback.receive-directed-interaction]"
    "[federate.callback.flush-queue-grant]"
    "[federate.callback.initiate-federate-save][federate.callback.federation-saved]"
    "[federate.callback.federation-restore-begun]"
    "[federate.callback.initiate-federate-restore][federate.callback.federation-restored]"
    "[federate.callback.time-advance-grant]"
    "[callback-immediate]") {
  auto runScenario = [](CallbackModel callbackModel) {
  auto const saveDirectory = restore_support::temporaryFederationSaveDirectory();
  auto const saveStore = std::make_shared<umbra::detail::FilesystemFederationSaveCommitStore>(
      saveDirectory.path());
  auto const reportDirectory = restore_support::temporaryServiceReportDirectory();
  auto sourceConfiguration = restore_support::configurationForServiceReportDirectory(reportDirectory.path());
  sourceConfiguration.withRtiAddress(L"in-process");
  auto const federationName = nextFederationName();
  auto const objectConsumer =
      (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data" /
       "directed-parameter-interaction-object-consumer-fom.xml")
          .wstring();
  auto const interactionProvider =
      (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data" /
       "directed-parameter-interaction-interaction-provider-fom.xml")
          .wstring();
  auto const switchFom =
      (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data" /
       "switch-support-enabled-fom.xml")
          .wstring();
  std::vector<std::wstring> const fomModules{objectConsumer, interactionProvider, switchFom};
  std::wstring const saveLabel = L"public-fresh-directed-tso-parameter-projection";
  std::string const payloadBytes{"parameterized-directed", 22U};
  std::string const tagBytes{"parameter-tso", 13U};
  VariableLengthData const tag(tagBytes.data(), tagBytes.size());
  std::vector<unsigned char> const expectedPayload(payloadBytes.begin(), payloadBytes.end());
  std::vector<unsigned char> const expectedTag(tagBytes.begin(), tagBytes.end());
  FederateHandle sourceSenderHandle;
  ObjectInstanceHandle sourceTarget;
  rti1516_2025::MessageRetractionHandle sourceRetraction;
  std::filesystem::path sourceReportFile;

  auto const drainAll = [](RTIambassador& first, RTIambassador& second) {
    for (int pass = 0; pass != 32; ++pass) {
      static_cast<void>(first.evokeCallback(0.0));
      static_cast<void>(second.evokeCallback(0.0));
    }
  };

  {
    auto const sourceRegistry = std::make_shared<umbra::detail::EmbeddedFederationRegistry>(
        nullptr,
        saveStore);
    restore_support::ScopedEmbeddedFederationRegistry sourceScope(sourceRegistry);
    ReportingFederateAmbassador ownerReports;
    ReportingFederateAmbassador senderReports;
    auto owner = makeRti();
    auto sender = makeRti();

    REQUIRE_NOTHROW(owner->connect(ownerReports, callbackModel));
    REQUIRE_NOTHROW(sender->connect(senderReports, callbackModel, sourceConfiguration));
    REQUIRE_NOTHROW(owner->createFederationExecution(
        federationName,
        fomModules,
        standard_hla::mom::integer64_time));
    REQUIRE_NOTHROW(owner->joinFederationExecution(
        L"public-fresh-directed-parameter-owner",
        L"owner",
        federationName));
    REQUIRE_NOTHROW(sourceSenderHandle = sender->joinFederationExecution(
        L"public-fresh-directed-parameter-sender",
        L"publisher",
        federationName));
    suppressDeclarationRelevanceAdvisories(*owner);
    suppressDeclarationRelevanceAdvisories(*sender);

    auto const filesAtJoin = restore_support::serviceReportFiles(reportDirectory.path());
    REQUIRE(filesAtJoin.size() == 1U);
    sourceReportFile = filesAtJoin.front();
    REQUIRE(std::filesystem::absolute(sourceReportFile).lexically_normal() ==
            sourceReportFile.lexically_normal());

    auto const objectClass = owner->getObjectClassHandle(
        L"HLAobjectRoot.UmbraDirectedParameterFixtureObject");
    auto const marker = owner->getAttributeHandle(objectClass, L"DirectedTargetMarker");
    auto const interactionClass = sender->getInteractionClassHandle(
        L"HLAinteractionRoot.UmbraDirectedParameterFixtureInteraction");
    auto const payload = sender->getParameterHandle(interactionClass, L"DirectedPayload");
    InteractionClassHandleSet const directedClasses{interactionClass};
    REQUIRE(objectClass.isValid());
    REQUIRE(marker.isValid());
    REQUIRE(interactionClass.isValid());
    REQUIRE(payload.isValid());
    REQUIRE_NOTHROW(owner->publishObjectClassAttributes(
        objectClass,
        AttributeHandleSet{marker}));
    REQUIRE_NOTHROW(sender->subscribeObjectClassAttributes(
        objectClass,
        AttributeHandleSet{marker}));
    REQUIRE_NOTHROW(sender->publishObjectClassDirectedInteractions(
        objectClass,
        directedClasses));
    REQUIRE_NOTHROW(sender->changeInteractionOrderType(interactionClass, TIMESTAMP));
    REQUIRE_NOTHROW(owner->subscribeObjectClassDirectedInteractions(
        objectClass,
        directedClasses,
        false));

    REQUIRE_NOTHROW(sourceTarget = owner->registerObjectInstance(objectClass));
    REQUIRE(sourceTarget.isValid());
    drainAll(*owner, *sender);
    REQUIRE(senderReports.objectDiscoveryReports.size() == 1U);

    // Preserve a route-free application-value projection for the object ledger;
    // the parameterized directed interaction remains the pending TSO payload.
    AttributeHandleValueMap markerValues;
    markerValues.emplace(
        marker,
        VariableLengthData(payloadBytes.data(), payloadBytes.size()));
    REQUIRE_NOTHROW(owner->updateAttributeValues(sourceTarget, markerValues, tag));
    drainAll(*owner, *sender);
    REQUIRE(senderReports.attributeReflectionReports.size() == 1U);
    senderReports.attributeReflectionReports.clear();
    senderReports.callbackOrder.clear();

    REQUIRE_NOTHROW(owner->enableTimeConstrained());
    REQUIRE_FALSE(owner->evokeCallback(0.0));
    REQUIRE_NOTHROW(sender->enableTimeRegulation(
        rti1516_2025::HLAinteger64Interval(1)));
    REQUIRE_FALSE(sender->evokeCallback(0.0));

    ParameterHandleValueMap parameterValues;
    parameterValues.emplace(
        payload,
        VariableLengthData(payloadBytes.data(), payloadBytes.size()));
    REQUIRE_NOTHROW(sourceRetraction = sender->sendDirectedInteraction(
        interactionClass,
        sourceTarget,
        parameterValues,
        tag,
        rti1516_2025::HLAinteger64Time(7)));
    REQUIRE(sourceRetraction.isValid());
    REQUIRE(ownerReports.directedInteractionReports.empty());

    REQUIRE_NOTHROW(sender->requestFederationSave(
        saveLabel,
        rti1516_2025::HLAinteger64Time(5)));
    // In HLA_IMMEDIATE, a grant dispatch runs as soon as its request is
    // submitted.  Hold both dispatchers while staging the sender and owner
    // requests so the timed-save admission observes the complete boundary;
    // the HLA_EVOKED path naturally stages them before drainAll().
    if (callbackModel == rti1516_2025::HLA_IMMEDIATE) {
      REQUIRE_NOTHROW(owner->disableCallbacks());
      REQUIRE_NOTHROW(sender->disableCallbacks());
    }
    REQUIRE_NOTHROW(sender->timeAdvanceRequest(
        rti1516_2025::HLAinteger64Time(5)));
    REQUIRE_NOTHROW(owner->timeAdvanceRequest(
        rti1516_2025::HLAinteger64Time(5)));
    if (callbackModel == rti1516_2025::HLA_IMMEDIATE) {
      REQUIRE_NOTHROW(owner->enableCallbacks());
      REQUIRE_NOTHROW(sender->enableCallbacks());
    }
    drainAll(*owner, *sender);
    REQUIRE(ownerReports.initiateFederateSaveReports ==
            std::vector<std::wstring>{saveLabel});
    REQUIRE(senderReports.initiateFederateSaveReports ==
            std::vector<std::wstring>{saveLabel});
    REQUIRE_NOTHROW(owner->federateSaveBegun());
    REQUIRE_NOTHROW(sender->federateSaveBegun());
    REQUIRE_NOTHROW(owner->federateSaveComplete());
    REQUIRE_NOTHROW(sender->federateSaveComplete());
    drainAll(*owner, *sender);
    REQUIRE(ownerReports.federationSavedReportCount == 1U);
    REQUIRE(senderReports.federationSavedReportCount == 1U);

    auto const durable = saveStore->load(federationName, saveLabel);
    REQUIRE(durable.has_value());
    auto const durableImage = umbra::detail::FederationStateImageCodec::decode(
        durable->stateImage);
    REQUIRE(durableImage.tsoDirectedInteractionMessages.size() == 1U);
    auto const& savedMessage = durableImage.tsoDirectedInteractionMessages.front();
    REQUIRE(savedMessage.sentParameterHandles.size() == 1U);
    REQUIRE(savedMessage.parameters.size() == 1U);
    REQUIRE(savedMessage.parameters.front().value == payloadBytes);
    REQUIRE(savedMessage.userSuppliedTag == tagBytes);
    REQUIRE(savedMessage.transportationName == "HLAreliable");
    REQUIRE(savedMessage.sentOrderType ==
            static_cast<std::uint32_t>(TIMESTAMP));
    REQUIRE(savedMessage.receivedOrderType ==
            static_cast<std::uint32_t>(TIMESTAMP));
    REQUIRE(savedMessage.recipients.size() == 1U);
    // This fixture has one concrete owner recipient; the payload ledger and
    // queue therefore each contain one entry. Fan-out queue cardinality is
    // exercised separately by the mixed-recipient lane.
    REQUIRE(durableImage.tsoQueueEntries.size() == 1U);

    REQUIRE_NOTHROW(sender->resignFederationExecution(NO_ACTION));
    REQUIRE_NOTHROW(owner->resignFederationExecution(CANCEL_THEN_DELETE_THEN_DIVEST));
    REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
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
    auto owner = makeRti();
    auto sender = makeRti();
    auto freshConfiguration = restore_support::configurationForServiceReportDirectory(reportDirectory.path());
    freshConfiguration.withRtiAddress(L"in-process");

    REQUIRE_NOTHROW(owner->connect(ownerReports, callbackModel));
    REQUIRE_NOTHROW(sender->connect(senderReports, callbackModel, freshConfiguration));
    REQUIRE_NOTHROW(owner->createFederationExecution(
        federationName,
        fomModules,
        standard_hla::mom::integer64_time));
    REQUIRE_NOTHROW(owner->joinFederationExecution(
        L"public-fresh-directed-parameter-owner",
        L"owner",
        federationName));
    FederateHandle freshSenderHandle;
    REQUIRE_NOTHROW(freshSenderHandle = sender->joinFederationExecution(
        L"public-fresh-directed-parameter-sender",
        L"publisher",
        federationName));
    REQUIRE(freshSenderHandle == sourceSenderHandle);
    suppressDeclarationRelevanceAdvisories(*owner);
    suppressDeclarationRelevanceAdvisories(*sender);

    auto const filesAfterFreshJoin = restore_support::serviceReportFiles(reportDirectory.path());
    REQUIRE(filesAfterFreshJoin.size() == 2U);
    REQUIRE(std::find(filesAfterFreshJoin.begin(), filesAfterFreshJoin.end(), sourceReportFile) !=
            filesAfterFreshJoin.end());
    auto const freshReportFile = filesAfterFreshJoin.front() == sourceReportFile
        ? filesAfterFreshJoin.back()
        : filesAfterFreshJoin.front();
    REQUIRE(freshReportFile != sourceReportFile);

    auto const interactionClass = sender->getInteractionClassHandle(
        L"HLAinteractionRoot.UmbraDirectedParameterFixtureInteraction");
    auto const freshPayload = sender->getParameterHandle(interactionClass, L"DirectedPayload");
    REQUIRE(interactionClass.isValid());
    REQUIRE(freshPayload.isValid());
    REQUIRE_NOTHROW(sender->requestFederationRestore(saveLabel));
    drainAll(*owner, *sender);
    REQUIRE(senderReports.requestFederationRestoreSucceededReports ==
            std::vector<std::wstring>{saveLabel});
    REQUIRE(ownerReports.federationRestoreBegunReportCount == 1U);
    REQUIRE(senderReports.federationRestoreBegunReportCount == 1U);
    REQUIRE_NOTHROW(owner->federateRestoreComplete());
    REQUIRE_NOTHROW(sender->federateRestoreComplete());
    drainAll(*owner, *sender);
    REQUIRE(ownerReports.federationRestoredReportCount == 1U);
    REQUIRE(senderReports.federationRestoredReportCount == 1U);
    REQUIRE(ownerReports.directedInteractionReports.empty());

    ownerReports.callbackOrder.clear();
    REQUIRE_NOTHROW(owner->flushQueueRequest(rti1516_2025::HLAinteger64Time(7)));
    while (owner->evokeCallback(0.0)) {
    }
    REQUIRE(ownerReports.directedInteractionReports.size() == 1U);
    REQUIRE(ownerReports.flushQueueGrantReports.size() == 1U);
    REQUIRE(ownerReports.callbackOrder ==
            std::vector<std::string>{"directed", "flush-grant"});

    auto const reliable = sender->getTransportationTypeHandle(standard_hla::mom::reliable);
    auto const& delivered = ownerReports.directedInteractionReports.front();
    REQUIRE(delivered.interactionClass == interactionClass);
    REQUIRE(delivered.objectInstance == sourceTarget);
    REQUIRE(delivered.parameterValues.size() == 1U);
    REQUIRE(delivered.parameterValues.contains(freshPayload));
    REQUIRE(variableLengthDataBytes(delivered.parameterValues.at(freshPayload)) ==
            expectedPayload);
    REQUIRE(variableLengthDataBytes(delivered.userSuppliedTag) == expectedTag);
    REQUIRE(delivered.transportationType == reliable);
    REQUIRE(delivered.producingFederate == freshSenderHandle);
    REQUIRE(delivered.timeImplementationName == standard_hla::mom::integer64_time);
    REQUIRE(delivered.timeValue == L"7");
    REQUIRE(delivered.sentOrderType == TIMESTAMP);
    REQUIRE(delivered.receivedOrderType == TIMESTAMP);
    REQUIRE(delivered.retractionSupplied);
    REQUIRE(delivered.retractionValid);
    REQUIRE(restore_support::serviceReportFiles(reportDirectory.path()) == filesAfterFreshJoin);

    REQUIRE_NOTHROW(owner->resignFederationExecution(CANCEL_THEN_DELETE_THEN_DIVEST));
    REQUIRE_NOTHROW(sender->resignFederationExecution(NO_ACTION));
    REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
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
}  // namespace
