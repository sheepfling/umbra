#include "ieee1516_2025_federation_management_test_support.hpp"
#include "ieee1516_2025_federation_management_fixture_support.hpp"
#include "ieee1516_2025_public_federation_restore_test_support.hpp"

namespace {
TEST_CASE(
    "Embedded public fresh-registry restore delivers eligible directed TSO and issues Request Retraction under HLA_EVOKED and HLA_IMMEDIATE",
    "[integration][development-profile][federation-management][ownership-management]"
    "[interaction-management][directed-interaction][directed-routing][time-management][tso]"
    "[ownership-ledger-state][tso-queue-state][tso-retraction-state]"
    "[save-restore][durable-save][filesystem][process-restart][restore]"
    "[process-restart-directed-interaction-tso-ownership-delivery-retraction]"
    "[public-process-restart-directed-interaction-tso-ownership-delivery-retraction]"
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
    "[rti.service.enable-time-constrained][rti.service.time-advance-request]"
    "[rti.service.request-federation-save][rti.service.federate-save-begun]"
    "[rti.service.federate-save-complete][rti.service.request-federation-restore]"
    "[rti.service.federate-restore-complete][rti.service.resign-federation-execution]"
    "[federate.callback.discover-object-instance][federate.callback.reflect-attribute-values]"
    "[federate.callback.receive-directed-interaction][federate.callback.request-retraction]"
    "[federate.callback.time-advance-grant][federate.callback.flush-queue-grant]"
    "[federate.callback.initiate-federate-save][federate.callback.federation-saved]"
    "[federate.callback.federation-restore-begun]"
    "[federate.callback.initiate-federate-restore][federate.callback.federation-restored]"
    "[rti.service.disable-callbacks][rti.service.enable-callbacks]"
    "[callback-immediate]") {
  using public_federation_restore_test_support::ScopedEmbeddedFederationRegistry;
  using public_federation_restore_test_support::configurationForServiceReportDirectory;
  using public_federation_restore_test_support::readTextFile;
  using public_federation_restore_test_support::serviceReportFiles;
  using public_federation_restore_test_support::temporaryFederationSaveDirectory;
  using public_federation_restore_test_support::temporaryServiceReportDirectory;
  auto runScenario = [](CallbackModel const callbackModel) {
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
      (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" /
       "data" / "directed-interaction-object-consumer-fom.xml")
          .wstring();
  auto const interactionProvider =
      (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" /
       "data" / "directed-interaction-interaction-provider-fom.xml")
          .wstring();
  auto const switchFom =
      (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" /
       "data" / "switch-support-enabled-fom.xml")
          .wstring();
  std::vector<std::wstring> const fomModules{
      objectConsumer, interactionProvider, switchFom};
  std::wstring const saveLabel =
      L"public-directed-interaction-tso-ownership-delivery-retraction-process-restart";
  std::string const tagBytes{"eligible-tso"};
  VariableLengthData const tag(tagBytes.data(), tagBytes.size());
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
    auto const sourceRegistry =
        std::make_shared<umbra::detail::EmbeddedFederationRegistry>(
            nullptr, saveStore);
    ScopedEmbeddedFederationRegistry sourceScope(sourceRegistry);
    ReportingFederateAmbassador senderReports;
    ReportingFederateAmbassador ownerReports;
    auto sender = makeRti();
    auto owner = makeRti();

    REQUIRE_NOTHROW(
        sender->connect(senderReports, callbackModel, sourceConfiguration));
    REQUIRE_NOTHROW(owner->connect(ownerReports, callbackModel));
    REQUIRE_NOTHROW(sender->createFederationExecution(
        federationName, fomModules, standard_hla::mom::integer64_time));
    REQUIRE_NOTHROW(sourceSenderHandle = sender->joinFederationExecution(
        L"public-directed-tso-delivery-sender", L"publisher", federationName));
    REQUIRE_NOTHROW(owner->joinFederationExecution(
        L"public-directed-tso-delivery-owner", L"owner", federationName));
    suppressDeclarationRelevanceAdvisories(*sender);
    suppressDeclarationRelevanceAdvisories(*owner);

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
    REQUIRE_NOTHROW(sender->publishObjectClassDirectedInteractions(
        objectClass, directedClasses));
    REQUIRE_NOTHROW(sender->changeInteractionOrderType(
        interactionClass, TIMESTAMP));
    REQUIRE_NOTHROW(owner->subscribeObjectClassDirectedInteractions(
        objectClass, directedClasses, false));

    REQUIRE_NOTHROW(sourceTarget = owner->registerObjectInstance(objectClass));
    REQUIRE(sourceTarget.isValid());
    drainAll(*sender, *owner);
    REQUIRE(senderReports.objectDiscoveryReports.size() == 1U);

    AttributeHandleValueMap markerValues;
    markerValues.emplace(marker, tag);
    REQUIRE_NOTHROW(owner->updateAttributeValues(
        sourceTarget, markerValues, tag));
    drainAll(*sender, *owner);
    REQUIRE(senderReports.attributeReflectionReports.size() == 1U);
    senderReports.attributeReflectionReports.clear();
    senderReports.callbackOrder.clear();
    ownerReports.callbackOrder.clear();

    REQUIRE_NOTHROW(owner->enableTimeConstrained());
    REQUIRE_FALSE(owner->evokeCallback(0.0));
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

    REQUIRE_NOTHROW(sender->requestFederationSave(
        saveLabel, rti1516_2025::HLAinteger64Time(7)));
    // HLA_IMMEDIATE executes an admitted grant synchronously. Hold both
    // routes while the sender and constrained owner cross the timed-save
    // boundary so the durable ownership/retraction image is identical to the
    // evoked run.
    if (callbackModel == rti1516_2025::HLA_IMMEDIATE) {
      REQUIRE_NOTHROW(sender->disableCallbacks());
      REQUIRE_NOTHROW(owner->disableCallbacks());
    }
    REQUIRE_NOTHROW(sender->timeAdvanceRequest(
        rti1516_2025::HLAinteger64Time(7)));
    REQUIRE_NOTHROW(owner->timeAdvanceRequest(
        rti1516_2025::HLAinteger64Time(7)));
    if (callbackModel == rti1516_2025::HLA_IMMEDIATE) {
      REQUIRE_NOTHROW(sender->enableCallbacks());
      REQUIRE_NOTHROW(owner->enableCallbacks());
    }
    drainAll(*sender, *owner);
    REQUIRE(senderReports.initiateFederateSaveReports ==
            std::vector<std::wstring>{saveLabel});
    REQUIRE(ownerReports.initiateFederateSaveReports ==
            std::vector<std::wstring>{saveLabel});
    REQUIRE_NOTHROW(sender->federateSaveBegun());
    REQUIRE_NOTHROW(owner->federateSaveBegun());
    REQUIRE_NOTHROW(sender->federateSaveComplete());
    REQUIRE_NOTHROW(owner->federateSaveComplete());
    drainAll(*sender, *owner);
    REQUIRE(senderReports.federationSavedReportCount == 1U);
    REQUIRE(ownerReports.federationSavedReportCount == 1U);

    auto const durable = saveStore->load(federationName, saveLabel);
    REQUIRE(durable.has_value());
    auto const durableImage =
        umbra::detail::FederationStateImageCodec::decode(durable->stateImage);
    REQUIRE(durableImage.tsoDirectedInteractionMessages.size() == 1U);
    REQUIRE(durableImage.tsoDirectedInteractionMessages.front().recipients.size() == 1U);
    REQUIRE(durableImage.tsoRequestRetractionRecords.size() == 1U);
    REQUIRE(durableImage.tsoRequestRetractionRecords.front().recipientStates.size() == 1U);
    REQUIRE(durableImage.tsoQueueEntries.size() == 1U);

    auto const reportAfterSave = readTextFile(sourceReportFile);
    REQUIRE(reportAfterSave.size() > initialReportText.size());
    REQUIRE(reportAfterSave.find("RequestFederationSave") != std::string::npos);
    REQUIRE(reportAfterSave.find("FederateSaveComplete") != std::string::npos);

    REQUIRE_NOTHROW(owner->resignFederationExecution(
        CANCEL_THEN_DELETE_THEN_DIVEST));
    REQUIRE_NOTHROW(sender->resignFederationExecution(
        CANCEL_THEN_DELETE_THEN_DIVEST));
    REQUIRE_NOTHROW(sender->destroyFederationExecution(federationName));
    REQUIRE_NOTHROW(owner->disconnect());
    REQUIRE_NOTHROW(sender->disconnect());
  }

  {
    auto const freshRegistry =
        std::make_shared<umbra::detail::EmbeddedFederationRegistry>(
            nullptr, saveStore);
    ScopedEmbeddedFederationRegistry freshScope(freshRegistry);
    ReportingFederateAmbassador senderReports;
    ReportingFederateAmbassador ownerReports;
    auto sender = makeRti();
    auto owner = makeRti();
    auto freshConfiguration =
        configurationForServiceReportDirectory(reportDirectory.path());
    freshConfiguration.withRtiAddress(L"in-process");

    REQUIRE_NOTHROW(sender->connect(senderReports, callbackModel, freshConfiguration));
    REQUIRE_NOTHROW(owner->connect(ownerReports, callbackModel));
    REQUIRE_NOTHROW(sender->createFederationExecution(
        federationName, fomModules, standard_hla::mom::integer64_time));
    FederateHandle freshSenderHandle;
    REQUIRE_NOTHROW(freshSenderHandle = sender->joinFederationExecution(
        L"public-directed-tso-delivery-sender", L"publisher", federationName));
    REQUIRE_NOTHROW(owner->joinFederationExecution(
        L"public-directed-tso-delivery-owner", L"owner", federationName));
    REQUIRE(freshSenderHandle == sourceSenderHandle);
    suppressDeclarationRelevanceAdvisories(*sender);
    suppressDeclarationRelevanceAdvisories(*owner);

    auto const filesAfterFreshJoin = serviceReportFiles(reportDirectory.path());
    REQUIRE(filesAfterFreshJoin.size() == 2U);
    REQUIRE(std::find(filesAfterFreshJoin.begin(), filesAfterFreshJoin.end(),
                      sourceReportFile) != filesAfterFreshJoin.end());
    auto const freshReportFile = filesAfterFreshJoin.front() == sourceReportFile
        ? filesAfterFreshJoin.back()
        : filesAfterFreshJoin.front();
    REQUIRE(freshReportFile != sourceReportFile);
    auto const freshInitialReportText = readTextFile(freshReportFile);

    auto const freshInteractionClass = sender->getInteractionClassHandle(
        fixture_hla::fom::directed_fixture_interaction);
    REQUIRE(freshInteractionClass.isValid());
    REQUIRE_NOTHROW(sender->requestFederationRestore(saveLabel));
    drainAll(*sender, *owner);
    REQUIRE(senderReports.requestFederationRestoreSucceededReports ==
            std::vector<std::wstring>{saveLabel});
    REQUIRE(senderReports.federationRestoreBegunReportCount == 1U);
    REQUIRE(ownerReports.federationRestoreBegunReportCount == 1U);
    REQUIRE(senderReports.initiateFederateRestoreReports.size() == 1U);
    REQUIRE(ownerReports.initiateFederateRestoreReports.size() == 1U);
    REQUIRE_NOTHROW(sender->federateRestoreComplete());
    REQUIRE_NOTHROW(owner->federateRestoreComplete());
    drainAll(*sender, *owner);
    REQUIRE(senderReports.federationRestoredReportCount == 1U);
    REQUIRE(ownerReports.federationRestoredReportCount == 1U);
    REQUIRE(ownerReports.directedInteractionReports.empty());

    ownerReports.callbackOrder.clear();
    ownerReports.flushQueueGrantReports.clear();
    ownerReports.directedInteractionReports.clear();
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
    drainAll(*sender, *owner);
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
    REQUIRE_NOTHROW(sender->resignFederationExecution(
        CANCEL_THEN_DELETE_THEN_DIVEST));
    REQUIRE_NOTHROW(sender->destroyFederationExecution(federationName));
    REQUIRE_NOTHROW(owner->disconnect());
    REQUIRE_NOTHROW(sender->disconnect());
  }
  };
  runScenario(rti1516_2025::HLA_EVOKED);
  runScenario(rti1516_2025::HLA_IMMEDIATE);
}
}
