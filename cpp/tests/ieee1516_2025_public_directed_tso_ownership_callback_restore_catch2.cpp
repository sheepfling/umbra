#include "ieee1516_2025_federation_management_test_support.hpp"
#include "ieee1516_2025_federation_management_fixture_support.hpp"
#include "ieee1516_2025_public_federation_restore_test_support.hpp"

namespace {
TEST_CASE(
    "Embedded public fresh-registry restore suppresses stale directed TSO callback after by-ownership handoff under HLA_EVOKED and HLA_IMMEDIATE",
    "[integration][development-profile][federation-management][ownership-management]"
    "[interaction-management][directed-interaction][directed-routing][time-management][tso]"
    "[ownership-ledger-state][tso-queue-state][tso-retraction-state]"
    "[save-restore][durable-save][filesystem][process-restart][restore]"
    "[process-restart-directed-interaction-tso-ownership-callback]"
    "[public-process-restart-directed-interaction-tso-ownership-callback]"
    "[callback-immediate]"
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
    "[rti.service.attribute-ownership-acquisition]"
    "[rti.service.attribute-ownership-divestiture-if-wanted]"
    "[rti.service.request-federation-save][rti.service.federate-save-begun]"
    "[rti.service.federate-save-complete][rti.service.request-federation-restore]"
    "[rti.service.federate-restore-complete][rti.service.resign-federation-execution]"
    "[federate.callback.discover-object-instance][federate.callback.reflect-attribute-values]"
    "[federate.callback.receive-directed-interaction][federate.callback.request-retraction]"
    "[federate.callback.request-attribute-ownership-release]"
    "[federate.callback.attribute-ownership-acquisition-notification]"
    "[federate.callback.time-advance-grant][federate.callback.flush-queue-grant]"
    "[federate.callback.initiate-federate-save][federate.callback.federation-saved]"
    "[federate.callback.federation-restore-begun]"
    "[federate.callback.initiate-federate-restore][federate.callback.federation-restored]") {
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
      L"public-directed-interaction-tso-ownership-callback-process-restart";
  std::string const routeTagBytes{"tso-owner-handoff"};
  VariableLengthData const routeTag(routeTagBytes.data(), routeTagBytes.size());
  std::string const acquisitionTagBytes{"tso-owner-acquire"};
  VariableLengthData const acquisitionTag(
      acquisitionTagBytes.data(), acquisitionTagBytes.size());
  std::string const divestitureTagBytes{"tso-owner-divest"};
  VariableLengthData const divestitureTag(
      divestitureTagBytes.data(), divestitureTagBytes.size());
  FederateHandle sourcePublisherHandle;
  ObjectInstanceHandle sourceTarget;
  rti1516_2025::MessageRetractionHandle sourceRetraction;
  std::filesystem::path sourceReportFile;

  auto const drainAll = [](RTIambassador& first,
                            RTIambassador& second,
                            RTIambassador& third) {
    for (int pass = 0; pass != 32; ++pass) {
      static_cast<void>(first.evokeCallback(0.0));
      static_cast<void>(second.evokeCallback(0.0));
      static_cast<void>(third.evokeCallback(0.0));
    }
  };

  {
    auto const sourceRegistry =
        std::make_shared<umbra::detail::EmbeddedFederationRegistry>(
            nullptr, saveStore);
    ScopedEmbeddedFederationRegistry sourceScope(sourceRegistry);
    ReportingFederateAmbassador publisherReports;
    ReportingFederateAmbassador initialOwnerReports;
    ReportingFederateAmbassador handoffOwnerReports;
    auto publisher = makeRti();
    auto initialOwner = makeRti();
    auto handoffOwner = makeRti();

    REQUIRE_NOTHROW(
        publisher->connect(publisherReports, callbackModel, sourceConfiguration));
    REQUIRE_NOTHROW(initialOwner->connect(initialOwnerReports, callbackModel));
    REQUIRE_NOTHROW(handoffOwner->connect(handoffOwnerReports, callbackModel));
    REQUIRE_NOTHROW(publisher->createFederationExecution(
        federationName, fomModules, standard_hla::mom::integer64_time));
    REQUIRE_NOTHROW(sourcePublisherHandle = publisher->joinFederationExecution(
        L"public-directed-tso-callback-publisher", L"publisher", federationName));
    REQUIRE_NOTHROW(initialOwner->joinFederationExecution(
        L"public-directed-tso-callback-initial-owner", L"owner", federationName));
    REQUIRE_NOTHROW(handoffOwner->joinFederationExecution(
        L"public-directed-tso-callback-new-owner", L"owner", federationName));
    suppressDeclarationRelevanceAdvisories(*publisher);
    suppressDeclarationRelevanceAdvisories(*initialOwner);
    suppressDeclarationRelevanceAdvisories(*handoffOwner);

    auto const filesAtJoin = serviceReportFiles(reportDirectory.path());
    REQUIRE(filesAtJoin.size() == 1U);
    sourceReportFile = filesAtJoin.front();
    REQUIRE(std::filesystem::absolute(sourceReportFile).lexically_normal() ==
            sourceReportFile.lexically_normal());
    auto const initialReportText = readTextFile(sourceReportFile);

    auto const objectClass = initialOwner->getObjectClassHandle(
        fixture_hla::fom::directed_fixture_object);
    auto const marker = initialOwner->getAttributeHandle(
        objectClass, fixture_hla::fixture::directed_target_marker);
    auto const privilegeToDelete = initialOwner->getAttributeHandle(
        objectClass, L"HLAprivilegeToDeleteObject");
    auto const interactionClass = publisher->getInteractionClassHandle(
        fixture_hla::fom::directed_fixture_interaction);
    InteractionClassHandleSet const directedClasses{interactionClass};
    REQUIRE(objectClass.isValid());
    REQUIRE(marker.isValid());
    REQUIRE(privilegeToDelete.isValid());
    REQUIRE(interactionClass.isValid());
    REQUIRE_NOTHROW(publisher->subscribeObjectClassAttributes(
        objectClass, AttributeHandleSet{marker}));
    REQUIRE_NOTHROW(initialOwner->publishObjectClassAttributes(
        objectClass, AttributeHandleSet{marker}));
    REQUIRE_NOTHROW(handoffOwner->publishObjectClassAttributes(
        objectClass, AttributeHandleSet{marker}));
    REQUIRE_NOTHROW(initialOwner->subscribeObjectClassAttributes(
        objectClass, AttributeHandleSet{marker}));
    REQUIRE_NOTHROW(handoffOwner->subscribeObjectClassAttributes(
        objectClass, AttributeHandleSet{marker}));
    REQUIRE_NOTHROW(publisher->publishObjectClassDirectedInteractions(
        objectClass, directedClasses));
    REQUIRE_NOTHROW(publisher->changeInteractionOrderType(
        interactionClass, TIMESTAMP));
    REQUIRE_NOTHROW(initialOwner->subscribeObjectClassDirectedInteractions(
        objectClass, directedClasses, false));
    REQUIRE_NOTHROW(handoffOwner->subscribeObjectClassDirectedInteractions(
        objectClass, directedClasses, false));

    REQUIRE_NOTHROW(sourceTarget = initialOwner->registerObjectInstance(objectClass));
    REQUIRE(sourceTarget.isValid());
    drainAll(*publisher, *initialOwner, *handoffOwner);
    REQUIRE(publisherReports.objectDiscoveryReports.size() == 1U);
    REQUIRE(handoffOwnerReports.objectDiscoveryReports.size() == 1U);

    AttributeHandleValueMap markerValues;
    markerValues.emplace(marker, routeTag);
    REQUIRE_NOTHROW(initialOwner->updateAttributeValues(
        sourceTarget, markerValues, routeTag));
    drainAll(*publisher, *initialOwner, *handoffOwner);
    REQUIRE(publisherReports.attributeReflectionReports.size() == 1U);
    REQUIRE(handoffOwnerReports.attributeReflectionReports.size() == 1U);
    publisherReports.attributeReflectionReports.clear();
    handoffOwnerReports.attributeReflectionReports.clear();
    publisherReports.callbackOrder.clear();
    initialOwnerReports.callbackOrder.clear();
    handoffOwnerReports.callbackOrder.clear();

    REQUIRE_NOTHROW(initialOwner->enableTimeConstrained());
    REQUIRE_FALSE(initialOwner->evokeCallback(0.0));
    REQUIRE_NOTHROW(handoffOwner->enableTimeConstrained());
    REQUIRE_FALSE(handoffOwner->evokeCallback(0.0));
    REQUIRE_NOTHROW(publisher->enableTimeRegulation(
        rti1516_2025::HLAinteger64Interval(1)));
    REQUIRE_FALSE(publisher->evokeCallback(0.0));

    REQUIRE_NOTHROW(sourceRetraction = publisher->sendDirectedInteraction(
        interactionClass,
        sourceTarget,
        ParameterHandleValueMap{},
        routeTag,
        rti1516_2025::HLAinteger64Time(5)));
    REQUIRE(sourceRetraction.isValid());
    REQUIRE(initialOwnerReports.directedInteractionReports.empty());
    REQUIRE(handoffOwnerReports.directedInteractionReports.empty());

    REQUIRE_NOTHROW(publisher->requestFederationSave(
        saveLabel, rti1516_2025::HLAinteger64Time(3)));
    if (callbackModel == rti1516_2025::HLA_IMMEDIATE) {
      REQUIRE_NOTHROW(publisher->disableCallbacks());
      REQUIRE_NOTHROW(initialOwner->disableCallbacks());
      REQUIRE_NOTHROW(handoffOwner->disableCallbacks());
    }
    REQUIRE_NOTHROW(publisher->timeAdvanceRequest(
        rti1516_2025::HLAinteger64Time(3)));
    REQUIRE_NOTHROW(initialOwner->timeAdvanceRequest(
        rti1516_2025::HLAinteger64Time(3)));
    REQUIRE_NOTHROW(handoffOwner->timeAdvanceRequest(
        rti1516_2025::HLAinteger64Time(3)));
    if (callbackModel == rti1516_2025::HLA_IMMEDIATE) {
      REQUIRE_NOTHROW(publisher->enableCallbacks());
      REQUIRE_NOTHROW(initialOwner->enableCallbacks());
      REQUIRE_NOTHROW(handoffOwner->enableCallbacks());
    }
    drainAll(*publisher, *initialOwner, *handoffOwner);
    REQUIRE(publisherReports.initiateFederateSaveReports ==
            std::vector<std::wstring>{saveLabel});
    REQUIRE(initialOwnerReports.initiateFederateSaveReports ==
            std::vector<std::wstring>{saveLabel});
    REQUIRE(handoffOwnerReports.initiateFederateSaveReports ==
            std::vector<std::wstring>{saveLabel});
    REQUIRE_NOTHROW(publisher->federateSaveBegun());
    REQUIRE_NOTHROW(initialOwner->federateSaveBegun());
    REQUIRE_NOTHROW(handoffOwner->federateSaveBegun());
    REQUIRE_NOTHROW(publisher->federateSaveComplete());
    REQUIRE_NOTHROW(initialOwner->federateSaveComplete());
    REQUIRE_NOTHROW(handoffOwner->federateSaveComplete());
    drainAll(*publisher, *initialOwner, *handoffOwner);
    REQUIRE(publisherReports.federationSavedReportCount == 1U);
    REQUIRE(initialOwnerReports.federationSavedReportCount == 1U);
    REQUIRE(handoffOwnerReports.federationSavedReportCount == 1U);

    auto const durable = saveStore->load(federationName, saveLabel);
    REQUIRE(durable.has_value());
    auto const durableImage =
        umbra::detail::FederationStateImageCodec::decode(durable->stateImage);
    REQUIRE(durableImage.tsoDirectedInteractionMessages.size() == 1U);
    // The public switch-support FOM retains both by-ownership candidates in
    // the persisted directed recipient ledger.  Only the current owner's
    // route has a concrete queue entry; callback-time ownership filtering
    // decides which candidate can enter user code after restore.
    REQUIRE(durableImage.tsoDirectedInteractionMessages.front().recipients.size() == 2U);
    REQUIRE(durableImage.tsoRequestRetractionRecords.size() == 1U);
    REQUIRE(durableImage.tsoRequestRetractionRecords.front().recipientStates.size() == 2U);
    REQUIRE(durableImage.tsoQueueEntries.size() == 2U);

    auto const reportAfterSave = readTextFile(sourceReportFile);
    REQUIRE(reportAfterSave.size() > initialReportText.size());
    REQUIRE(reportAfterSave.find("RequestFederationSave") != std::string::npos);
    REQUIRE(reportAfterSave.find("FederateSaveComplete") != std::string::npos);

    REQUIRE_NOTHROW(handoffOwner->resignFederationExecution(
        CANCEL_THEN_DELETE_THEN_DIVEST));
    REQUIRE_NOTHROW(initialOwner->resignFederationExecution(
        CANCEL_THEN_DELETE_THEN_DIVEST));
    REQUIRE_NOTHROW(publisher->resignFederationExecution(
        CANCEL_THEN_DELETE_THEN_DIVEST));
    REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
    REQUIRE_NOTHROW(handoffOwner->disconnect());
    REQUIRE_NOTHROW(initialOwner->disconnect());
    REQUIRE_NOTHROW(publisher->disconnect());
  }

  {
    auto const freshRegistry =
        std::make_shared<umbra::detail::EmbeddedFederationRegistry>(
            nullptr, saveStore);
    ScopedEmbeddedFederationRegistry freshScope(freshRegistry);
    ReportingFederateAmbassador publisherReports;
    ReportingFederateAmbassador initialOwnerReports;
    ReportingFederateAmbassador handoffOwnerReports;
    auto publisher = makeRti();
    auto initialOwner = makeRti();
    auto handoffOwner = makeRti();
    auto freshConfiguration =
        configurationForServiceReportDirectory(reportDirectory.path());
    freshConfiguration.withRtiAddress(L"in-process");

    REQUIRE_NOTHROW(publisher->connect(
        publisherReports, HLA_EVOKED, freshConfiguration));
    REQUIRE_NOTHROW(initialOwner->connect(initialOwnerReports, callbackModel));
    REQUIRE_NOTHROW(handoffOwner->connect(handoffOwnerReports, callbackModel));
    REQUIRE_NOTHROW(publisher->createFederationExecution(
        federationName, fomModules, standard_hla::mom::integer64_time));
    FederateHandle freshPublisherHandle;
    REQUIRE_NOTHROW(freshPublisherHandle = publisher->joinFederationExecution(
        L"public-directed-tso-callback-publisher", L"publisher", federationName));
    REQUIRE_NOTHROW(initialOwner->joinFederationExecution(
        L"public-directed-tso-callback-initial-owner", L"owner", federationName));
    REQUIRE_NOTHROW(handoffOwner->joinFederationExecution(
        L"public-directed-tso-callback-new-owner", L"owner", federationName));
    REQUIRE(freshPublisherHandle == sourcePublisherHandle);
    suppressDeclarationRelevanceAdvisories(*publisher);
    suppressDeclarationRelevanceAdvisories(*initialOwner);
    suppressDeclarationRelevanceAdvisories(*handoffOwner);

    auto const filesAfterFreshJoin = serviceReportFiles(reportDirectory.path());
    REQUIRE(filesAfterFreshJoin.size() == 2U);
    REQUIRE(std::find(filesAfterFreshJoin.begin(), filesAfterFreshJoin.end(),
                      sourceReportFile) != filesAfterFreshJoin.end());
    auto const freshReportFile = filesAfterFreshJoin.front() == sourceReportFile
        ? filesAfterFreshJoin.back()
        : filesAfterFreshJoin.front();
    REQUIRE(freshReportFile != sourceReportFile);
    auto const freshInitialReportText = readTextFile(freshReportFile);

    auto const freshObjectClass = initialOwner->getObjectClassHandle(
        fixture_hla::fom::directed_fixture_object);
    auto const freshMarker = initialOwner->getAttributeHandle(
        freshObjectClass, fixture_hla::fixture::directed_target_marker);
    auto const freshPrivilegeToDelete = initialOwner->getAttributeHandle(
        freshObjectClass, L"HLAprivilegeToDeleteObject");
    auto const freshInteractionClass = publisher->getInteractionClassHandle(
        fixture_hla::fom::directed_fixture_interaction);
    REQUIRE(freshObjectClass.isValid());
    REQUIRE(freshMarker.isValid());
    REQUIRE(freshPrivilegeToDelete.isValid());
    REQUIRE(freshInteractionClass.isValid());
    REQUIRE_NOTHROW(publisher->requestFederationRestore(saveLabel));
    drainAll(*publisher, *initialOwner, *handoffOwner);
    REQUIRE(publisherReports.requestFederationRestoreSucceededReports ==
            std::vector<std::wstring>{saveLabel});
    REQUIRE(publisherReports.federationRestoreBegunReportCount == 1U);
    REQUIRE(initialOwnerReports.federationRestoreBegunReportCount == 1U);
    REQUIRE(handoffOwnerReports.federationRestoreBegunReportCount == 1U);
    REQUIRE(publisherReports.initiateFederateRestoreReports.size() == 1U);
    REQUIRE(initialOwnerReports.initiateFederateRestoreReports.size() == 1U);
    REQUIRE(handoffOwnerReports.initiateFederateRestoreReports.size() == 1U);
    REQUIRE_NOTHROW(publisher->federateRestoreComplete());
    REQUIRE_NOTHROW(initialOwner->federateRestoreComplete());
    REQUIRE_NOTHROW(handoffOwner->federateRestoreComplete());
    drainAll(*publisher, *initialOwner, *handoffOwner);
    REQUIRE(publisherReports.federationRestoredReportCount == 1U);
    REQUIRE(initialOwnerReports.federationRestoredReportCount == 1U);
    REQUIRE(handoffOwnerReports.federationRestoredReportCount == 1U);
    REQUIRE(initialOwner->isAttributeOwnedByFederate(sourceTarget, freshMarker));
    REQUIRE_FALSE(handoffOwner->isAttributeOwnedByFederate(
        sourceTarget, freshMarker));

    // Transfer the complete registered set, including the implicit
    // HLAprivilegeToDeleteObject attribute, before crossing the old owner's
    // timestamped callback boundary.
    AttributeHandleSet const handoffAttributes{
        freshMarker, freshPrivilegeToDelete};
    REQUIRE_NOTHROW(handoffOwner->attributeOwnershipAcquisition(
        sourceTarget, handoffAttributes, acquisitionTag));
    AttributeHandleSet divestedAttributes;
    REQUIRE_NOTHROW(initialOwner->attributeOwnershipDivestitureIfWanted(
        sourceTarget,
        handoffAttributes,
        divestitureTag,
        divestedAttributes));
    REQUIRE(divestedAttributes == handoffAttributes);
    drainAll(*publisher, *initialOwner, *handoffOwner);
    REQUIRE(handoffOwnerReports.attributeOwnershipAcquisitionReports.size() == 1U);
    auto const& notification =
        handoffOwnerReports.attributeOwnershipAcquisitionReports.front();
    REQUIRE(notification.kind ==
            ReportingFederateAmbassador::AttributeOwnershipAcquisitionReport::Kind::notification);
    REQUIRE(notification.objectInstance == sourceTarget);
    REQUIRE(notification.attributes == handoffAttributes);
    REQUIRE(variableLengthDataBytes(notification.userSuppliedTag) ==
            std::vector<unsigned char>(divestitureTagBytes.begin(),
                                       divestitureTagBytes.end()));
    REQUIRE_FALSE(initialOwner->isAttributeOwnedByFederate(
        sourceTarget, freshMarker));
    REQUIRE(handoffOwner->isAttributeOwnedByFederate(
        sourceTarget, freshMarker));

    // The queued timestamped passel was admitted for the old owner.  Crossing
    // its callback boundary must re-check current by-ownership eligibility,
    // suppress the stale application callback, and still complete the grant.
    REQUIRE_NOTHROW(publisher->timeAdvanceRequest(
        rti1516_2025::HLAinteger64Time(4)));
    drainAll(*publisher, *initialOwner, *handoffOwner);
    initialOwnerReports.callbackOrder.clear();
    initialOwnerReports.flushQueueGrantReports.clear();
    initialOwnerReports.requestRetractionReports.clear();
    REQUIRE_NOTHROW(initialOwner->flushQueueRequest(
        rti1516_2025::HLAinteger64Time(5)));
    while (initialOwner->evokeCallback(0.0)) {
    }
    REQUIRE(initialOwnerReports.directedInteractionReports.empty());
    REQUIRE(initialOwnerReports.requestRetractionReports.empty());
    REQUIRE(initialOwnerReports.flushQueueGrantReports.size() == 1U);
    REQUIRE(initialOwnerReports.flushQueueGrantReports.front().value == L"5");
    REQUIRE(initialOwnerReports.callbackOrder ==
            std::vector<std::string>{"flush-grant"});

    // Reaching the timestamped callback boundary also advances the producer's
    // strict Retract lower bound to the message time.  The designator is no
    // longer legally retractable, and no Request Retraction callback is
    // generated because the stale recipient never entered user code.
    REQUIRE_THROWS_AS(
        publisher->retract(sourceRetraction),
        rti1516_2025::MessageCanNoLongerBeRetracted);
    drainAll(*publisher, *initialOwner, *handoffOwner);
    REQUIRE(initialOwnerReports.requestRetractionReports.empty());
    REQUIRE(handoffOwnerReports.requestRetractionReports.empty());
    REQUIRE(publisherReports.requestRetractionReports.empty());

    auto const filesAfterRestore = serviceReportFiles(reportDirectory.path());
    REQUIRE(filesAfterRestore == filesAfterFreshJoin);
    auto const freshReportAfterRestore = readTextFile(freshReportFile);
    REQUIRE(freshReportAfterRestore.size() > freshInitialReportText.size());
    REQUIRE(freshReportAfterRestore.find("RequestFederationRestore") !=
            std::string::npos);
    REQUIRE(freshReportAfterRestore.find("FederateRestoreComplete") !=
            std::string::npos);

    REQUIRE_NOTHROW(handoffOwner->resignFederationExecution(
        CANCEL_THEN_DELETE_THEN_DIVEST));
    REQUIRE_NOTHROW(initialOwner->resignFederationExecution(
        CANCEL_THEN_DELETE_THEN_DIVEST));
    REQUIRE_NOTHROW(publisher->resignFederationExecution(
        CANCEL_THEN_DELETE_THEN_DIVEST));
    REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
    REQUIRE_NOTHROW(handoffOwner->disconnect());
    REQUIRE_NOTHROW(initialOwner->disconnect());
    REQUIRE_NOTHROW(publisher->disconnect());
  }
  };
  runScenario(HLA_EVOKED);
  runScenario(rti1516_2025::HLA_IMMEDIATE);
}
}
