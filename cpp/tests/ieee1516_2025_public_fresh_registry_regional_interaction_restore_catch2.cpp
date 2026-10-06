#include "ieee1516_2025_federation_management_fixture_support.hpp"

TEST_CASE(
    "Embedded public fresh-registry restore rebinds queued timestamped regional interaction and preserves report-file lifetimes under HLA_EVOKED and HLA_IMMEDIATE",
    "[integration][development-profile][federation-management][save-restore]"
    "[durable-save][filesystem][process-restart][restore][time-management][ddm]"
    "[service-report-file][service-reporting][tso-queue-state][tso-payload-state]"
    "[tso-regional-interaction-state]"
    "[process-restart-regional-interaction-tso-ddm]"
    "[public-process-restart-regional-interaction-tso-ddm]"
    "[rti.service.create-region][rti.service.set-range-bounds]"
    "[rti.service.commit-region-modifications]"
    "[rti.service.subscribe-interaction-class-with-regions]"
    "[rti.service.send-interaction-with-regions]"
    "[rti.service.request-federation-save][rti.service.time-advance-request]"
    "[rti.service.federate-save-begun][rti.service.federate-save-complete]"
    "[rti.service.request-federation-restore]"
    "[rti.service.federate-restore-complete][rti.service.get-range-bounds]"
    "[rti.service.enable-time-regulation][rti.service.enable-time-constrained]"
    "[rti.service.disable-callbacks][rti.service.enable-callbacks]"
    "[federate.callback.receive-interaction][federate.callback.federation-restored]"
    "[federate.callback.initiate-federate-save][federate.callback.federation-saved]"
    "[callback-immediate][2025]") {
  auto runScenario = [](CallbackModel const callbackModel) {
  auto const saveDirectory = temporaryFederationSaveDirectory();
  auto const saveStore = std::make_shared<umbra::detail::FilesystemFederationSaveCommitStore>(
      saveDirectory.path());
  auto const reportDirectory = temporaryServiceReportDirectory();
  auto sourceConfiguration = configurationForServiceReportDirectory(reportDirectory.path());
  sourceConfiguration.withRtiAddress(L"in-process");
  auto const federationName = nextFederationName();
  auto const restaurantFom = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  auto const switchFom =
      (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data" /
       "switch-support-enabled-fom.xml")
          .wstring();
  std::vector<std::wstring> const fomModules{restaurantFom, switchFom};
  std::wstring const saveLabel = L"public-fresh-regional-restore-queued";
  unsigned char const parameterBytes[] = {0xD1, 0x37};
  unsigned char const tagBytes[] = {0x50, 0x52, 0x46, 0x52};
  ParameterHandleValueMap parameterValues;
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));
  FederateHandle sourceOwnerHandle;
  RegionHandle sourceRegion;
  RegionHandle sourceReceiverARegion;
  RegionHandle sourceReceiverBRegion;
  InteractionClassHandle interactionClass;
  ParameterHandle temperatureOk;
  DimensionHandle serverId;
  std::filesystem::path sourceReportFile;

  auto const drainAll = [&](RTIambassador& first,
                            RTIambassador& second,
                            RTIambassador& third) {
    // A callback on one route may submit the next callback to another route
    // after that route's turn in this pass (for example, the last constrained
    // grant submits the non-constrained owner's Initiate Federate Save).
    // EvokeCallback reports whether work remains *after* its one callback, so
    // a single boolean-driven pass can miss that cross-route enqueue.  Give
    // the three routes a bounded number of callback turns instead.
    for (int pass = 0; pass != 32; ++pass) {
      static_cast<void>(first.evokeCallback(0.0));
      static_cast<void>(second.evokeCallback(0.0));
      static_cast<void>(third.evokeCallback(0.0));
    }
  };

  // The source registry is deliberately backed by the durable filesystem
  // store.  After the source ambassadors resign, the fresh registry below
  // can load the same route-free image without retaining source closures.
  {
    auto const sourceRegistry = std::make_shared<umbra::detail::EmbeddedFederationRegistry>(
        nullptr,
        saveStore);
    ScopedEmbeddedFederationRegistry sourceScope(sourceRegistry);
    ReportingFederateAmbassador ownerReports;
    ReportingFederateAmbassador receiverAReports;
    ReportingFederateAmbassador receiverBReports;
    auto owner = makeRti();
    auto receiverA = makeRti();
    auto receiverB = makeRti();

    REQUIRE_NOTHROW(owner->connect(ownerReports, callbackModel, sourceConfiguration));
    REQUIRE_NOTHROW(receiverA->connect(receiverAReports, callbackModel));
    REQUIRE_NOTHROW(receiverB->connect(receiverBReports, callbackModel));
    REQUIRE_NOTHROW(owner->createFederationExecution(
        federationName,
        fomModules,
        standard_hla::mom::integer64_time));
    REQUIRE_NOTHROW(sourceOwnerHandle = owner->joinFederationExecution(
        L"public-fresh-regional-owner",
        L"publisher",
        federationName));
    REQUIRE_NOTHROW(receiverA->joinFederationExecution(
        L"public-fresh-regional-receiver-a",
        L"subscriber",
        federationName));
    REQUIRE_NOTHROW(receiverB->joinFederationExecution(
        L"public-fresh-regional-receiver-b",
        L"subscriber",
        federationName));
    suppressDeclarationRelevanceAdvisories(*owner);
    suppressDeclarationRelevanceAdvisories(*receiverA);
    suppressDeclarationRelevanceAdvisories(*receiverB);

    auto const filesAtJoin = serviceReportFiles(reportDirectory.path());
    REQUIRE(filesAtJoin.size() == 1U);
    sourceReportFile = filesAtJoin.front();
    REQUIRE(std::filesystem::absolute(sourceReportFile).lexically_normal() ==
            sourceReportFile.lexically_normal());
    auto const initialReportText = readTextFile(sourceReportFile);

    interactionClass = owner->getInteractionClassHandle(
        fixture_hla::fom::main_course_served);
    temperatureOk = owner->getParameterHandle(
        interactionClass,
        fixture_hla::fixture::temperature_ok);
    serverId = owner->getDimensionHandle(fixture_hla::fixture::server_id);
    REQUIRE(interactionClass.isValid());
    REQUIRE(temperatureOk.isValid());
    REQUIRE(serverId.isValid());
    parameterValues.emplace(
        temperatureOk,
        VariableLengthData(parameterBytes, sizeof(parameterBytes)));
    REQUIRE_NOTHROW(owner->publishInteractionClass(interactionClass));
    REQUIRE_NOTHROW(owner->changeInteractionOrderType(interactionClass, TIMESTAMP));

    REQUIRE_NOTHROW(sourceRegion = owner->createRegion(DimensionHandleSet{serverId}));
    REQUIRE_NOTHROW(sourceReceiverARegion =
        receiverA->createRegion(DimensionHandleSet{serverId}));
    REQUIRE_NOTHROW(sourceReceiverBRegion =
        receiverB->createRegion(DimensionHandleSet{serverId}));
    REQUIRE_NOTHROW(owner->setRangeBounds(
        sourceRegion,
        serverId,
        RangeBounds(2UL, 4UL)));
    REQUIRE_NOTHROW(owner->commitRegionModifications(RegionHandleSet{sourceRegion}));
    REQUIRE_NOTHROW(receiverA->setRangeBounds(
        sourceReceiverARegion,
        serverId,
        RangeBounds(2UL, 4UL)));
    REQUIRE_NOTHROW(receiverA->commitRegionModifications(
        RegionHandleSet{sourceReceiverARegion}));
    REQUIRE_NOTHROW(receiverB->setRangeBounds(
        sourceReceiverBRegion,
        serverId,
        RangeBounds(2UL, 4UL)));
    REQUIRE_NOTHROW(receiverB->commitRegionModifications(
        RegionHandleSet{sourceReceiverBRegion}));
    REQUIRE_NOTHROW(receiverA->subscribeInteractionClassWithRegions(
        interactionClass,
        RegionHandleSet{sourceReceiverARegion}));
    REQUIRE_NOTHROW(receiverB->subscribeInteractionClassWithRegions(
        interactionClass,
        RegionHandleSet{sourceReceiverBRegion}));
    REQUIRE_NOTHROW(receiverA->setConveyRegionDesignatorSetsSwitch(true));
    REQUIRE_NOTHROW(receiverB->setConveyRegionDesignatorSetsSwitch(true));

    REQUIRE_NOTHROW(receiverA->enableTimeConstrained());
    REQUIRE_FALSE(receiverA->evokeCallback(0.0));
    REQUIRE(receiverAReports.timeConstrainedEnabledReports.size() == 1U);
    REQUIRE_NOTHROW(receiverB->enableTimeConstrained());
    REQUIRE_FALSE(receiverB->evokeCallback(0.0));
    REQUIRE(receiverBReports.timeConstrainedEnabledReports.size() == 1U);
    REQUIRE_NOTHROW(owner->enableTimeRegulation(
        rti1516_2025::HLAinteger64Interval(1)));
    REQUIRE_FALSE(owner->evokeCallback(0.0));
    REQUIRE(ownerReports.timeRegulationEnabledReports.size() == 1U);

    auto const retraction = owner->sendInteractionWithRegions(
        interactionClass,
        parameterValues,
        RegionHandleSet{sourceRegion},
        tag,
        rti1516_2025::HLAinteger64Time(9));
    REQUIRE(retraction.isValid());
    REQUIRE(receiverAReports.timestampedInteractionReports.empty());
    REQUIRE(receiverBReports.timestampedInteractionReports.empty());

    REQUIRE_NOTHROW(owner->requestFederationSave(
        saveLabel,
        rti1516_2025::HLAinteger64Time(7)));
    // HLA_IMMEDIATE executes an admitted grant synchronously.  Hold every
    // route while the three members cross the timed-save boundary so the
    // durable image is admitted with the same membership and queue frontier
    // as the evoked run.
    if (callbackModel == rti1516_2025::HLA_IMMEDIATE) {
      REQUIRE_NOTHROW(owner->disableCallbacks());
      REQUIRE_NOTHROW(receiverA->disableCallbacks());
      REQUIRE_NOTHROW(receiverB->disableCallbacks());
    }
    REQUIRE_NOTHROW(owner->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(7)));
    REQUIRE_FALSE(owner->evokeCallback(0.0));
    if (callbackModel == rti1516_2025::HLA_EVOKED) {
      REQUIRE(ownerReports.timeAdvanceGrantReports.size() == 1U);
    }
    REQUIRE_NOTHROW(receiverA->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(7)));
    REQUIRE_NOTHROW(receiverB->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(7)));
    if (callbackModel == rti1516_2025::HLA_IMMEDIATE) {
      REQUIRE_NOTHROW(owner->enableCallbacks());
      REQUIRE_NOTHROW(receiverA->enableCallbacks());
      REQUIRE_NOTHROW(receiverB->enableCallbacks());
    }
    drainAll(*owner, *receiverA, *receiverB);
    REQUIRE(ownerReports.initiateFederateSaveReports ==
            std::vector<std::wstring>{saveLabel});
    REQUIRE(receiverAReports.initiateFederateSaveReports ==
            std::vector<std::wstring>{saveLabel});
    REQUIRE(receiverBReports.initiateFederateSaveReports ==
            std::vector<std::wstring>{saveLabel});

    REQUIRE_NOTHROW(owner->federateSaveBegun());
    REQUIRE_NOTHROW(receiverA->federateSaveBegun());
    REQUIRE_NOTHROW(receiverB->federateSaveBegun());
    REQUIRE_NOTHROW(owner->federateSaveComplete());
    REQUIRE_NOTHROW(receiverA->federateSaveComplete());
    REQUIRE_NOTHROW(receiverB->federateSaveComplete());
    drainAll(*owner, *receiverA, *receiverB);
    REQUIRE(ownerReports.federationSavedReportCount == 1U);
    REQUIRE(receiverAReports.federationSavedReportCount == 1U);
    REQUIRE(receiverBReports.federationSavedReportCount == 1U);
    auto const reportAfterSave = readTextFile(sourceReportFile);
    REQUIRE(reportAfterSave.size() > initialReportText.size());
    REQUIRE(reportAfterSave.find("RequestFederationSave") != std::string::npos);
    REQUIRE(reportAfterSave.find("FederateSaveComplete") != std::string::npos);

    auto const durable = saveStore->load(federationName, saveLabel);
    REQUIRE(durable.has_value());
    REQUIRE_FALSE(durable->stateImage.empty());
    auto const durableImage = umbra::detail::FederationStateImageCodec::decode(
        durable->stateImage);
    // The source has one publication plus two regional subscriptions; all
    // three declaration records are part of the durable image.
    REQUIRE(durableImage.interactionDeclarations.size() == 3U);
    REQUIRE(durableImage.regions.size() == 3U);
    REQUIRE(durableImage.tsoInteractionMessages.size() == 1U);
    REQUIRE(durableImage.tsoQueueEntries.size() == 2U);

    // The source region mutation happens after the durable image is complete.
    // The fresh registry must restore [2,4) and the invocation snapshot even
    // though this source lifetime is then resigned.
    REQUIRE_NOTHROW(owner->setRangeBounds(
        sourceRegion,
        serverId,
        RangeBounds(9UL, 11UL)));
    REQUIRE_NOTHROW(owner->commitRegionModifications(RegionHandleSet{sourceRegion}));
    auto const mutatedBounds = owner->getRangeBounds(sourceRegion, serverId);
    REQUIRE(mutatedBounds.getLowerBound() == 9UL);
    REQUIRE(mutatedBounds.getUpperBound() == 11UL);

    REQUIRE_NOTHROW(receiverA->resignFederationExecution(NO_ACTION));
    REQUIRE_NOTHROW(receiverB->resignFederationExecution(NO_ACTION));
    REQUIRE_NOTHROW(owner->resignFederationExecution(CANCEL_THEN_DELETE_THEN_DIVEST));
    REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
    REQUIRE_NOTHROW(receiverA->disconnect());
    REQUIRE_NOTHROW(receiverB->disconnect());
    REQUIRE_NOTHROW(owner->disconnect());
  }

  // Recreate the federation through the public facade against a new registry
  // instance.  The public callback routes below are therefore fresh objects;
  // the filesystem commit is the only source of the queued payload state.
  {
    auto const freshRegistry = std::make_shared<umbra::detail::EmbeddedFederationRegistry>(
        nullptr,
        saveStore);
    ScopedEmbeddedFederationRegistry freshScope(freshRegistry);
    ReportingFederateAmbassador ownerReports;
    ReportingFederateAmbassador receiverAReports;
    ReportingFederateAmbassador receiverBReports;
    auto owner = makeRti();
    auto receiverA = makeRti();
    auto receiverB = makeRti();
    auto freshConfiguration = configurationForServiceReportDirectory(reportDirectory.path());
    freshConfiguration.withRtiAddress(L"in-process");

    REQUIRE_NOTHROW(owner->connect(ownerReports, callbackModel, freshConfiguration));
    REQUIRE_NOTHROW(receiverA->connect(receiverAReports, callbackModel));
    REQUIRE_NOTHROW(receiverB->connect(receiverBReports, callbackModel));
    REQUIRE_NOTHROW(owner->createFederationExecution(
        federationName,
        fomModules,
        standard_hla::mom::integer64_time));
    FederateHandle freshOwnerHandle;
    REQUIRE_NOTHROW(freshOwnerHandle = owner->joinFederationExecution(
        L"public-fresh-regional-owner",
        L"publisher",
        federationName));
    REQUIRE_NOTHROW(receiverA->joinFederationExecution(
        L"public-fresh-regional-receiver-a",
        L"subscriber",
        federationName));
    REQUIRE_NOTHROW(receiverB->joinFederationExecution(
        L"public-fresh-regional-receiver-b",
        L"subscriber",
        federationName));
    REQUIRE(freshOwnerHandle == sourceOwnerHandle);
    suppressDeclarationRelevanceAdvisories(*owner);
    suppressDeclarationRelevanceAdvisories(*receiverA);
    suppressDeclarationRelevanceAdvisories(*receiverB);

    auto const filesAfterFreshJoin = serviceReportFiles(reportDirectory.path());
    REQUIRE(filesAfterFreshJoin.size() == 2U);
    REQUIRE(std::find(filesAfterFreshJoin.begin(), filesAfterFreshJoin.end(), sourceReportFile) !=
            filesAfterFreshJoin.end());
    auto const freshReportFile = filesAfterFreshJoin.front() == sourceReportFile
        ? filesAfterFreshJoin.back()
        : filesAfterFreshJoin.front();
    REQUIRE(freshReportFile != sourceReportFile);
    auto const freshInitialReportText = readTextFile(freshReportFile);

    REQUIRE_NOTHROW(owner->requestFederationRestore(saveLabel));
    drainAll(*owner, *receiverA, *receiverB);
    REQUIRE(ownerReports.requestFederationRestoreSucceededReports ==
            std::vector<std::wstring>{saveLabel});
    REQUIRE(ownerReports.federationRestoreBegunReportCount == 1U);
    REQUIRE(receiverAReports.federationRestoreBegunReportCount == 1U);
    REQUIRE(receiverBReports.federationRestoreBegunReportCount == 1U);
    REQUIRE(ownerReports.initiateFederateRestoreReports.size() == 1U);
    REQUIRE(receiverAReports.initiateFederateRestoreReports.size() == 1U);
    REQUIRE(receiverBReports.initiateFederateRestoreReports.size() == 1U);

    REQUIRE_NOTHROW(owner->federateRestoreComplete());
    REQUIRE_NOTHROW(receiverA->federateRestoreComplete());
    REQUIRE_NOTHROW(receiverB->federateRestoreComplete());
    drainAll(*owner, *receiverA, *receiverB);
    REQUIRE(ownerReports.federationRestoredReportCount == 1U);
    REQUIRE(receiverAReports.federationRestoredReportCount == 1U);
    REQUIRE(receiverBReports.federationRestoredReportCount == 1U);
    REQUIRE(receiverAReports.timestampedInteractionReports.empty());
    REQUIRE(receiverBReports.timestampedInteractionReports.empty());

    auto const restoredBounds = owner->getRangeBounds(sourceRegion, serverId);
    REQUIRE(restoredBounds.getLowerBound() == 2UL);
    REQUIRE(restoredBounds.getUpperBound() == 4UL);
    auto const filesAfterRestore = serviceReportFiles(reportDirectory.path());
    REQUIRE(filesAfterRestore == filesAfterFreshJoin);
    auto const freshAfterRestoreReportText = readTextFile(freshReportFile);
    REQUIRE(freshAfterRestoreReportText.size() > freshInitialReportText.size());
    REQUIRE(freshAfterRestoreReportText.find("RequestFederationRestore") != std::string::npos);
    REQUIRE(freshAfterRestoreReportText.find("FederateRestoreComplete") != std::string::npos);

    // Cross the restored queue entry from the fresh callback routes.  The
    // saved [2,4) source snapshot must win over the resigned source's [9,11)
    // mutation, and both recipient-specific queue entries must deliver once.
    REQUIRE_NOTHROW(owner->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(8)));
    REQUIRE_FALSE(owner->evokeCallback(0.0));
    REQUIRE_NOTHROW(receiverA->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(9)));
    REQUIRE_NOTHROW(receiverB->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(9)));
    drainAll(*owner, *receiverA, *receiverB);
    REQUIRE(receiverAReports.timestampedInteractionReports.size() == 1U);
    REQUIRE(receiverBReports.timestampedInteractionReports.size() == 1U);
    for (auto const* report : {
             &receiverAReports.timestampedInteractionReports.front(),
             &receiverBReports.timestampedInteractionReports.front()}) {
      REQUIRE(report->interactionClass == interactionClass);
      REQUIRE(report->parameterValues.size() == 1U);
      REQUIRE(report->parameterValues.contains(temperatureOk));
      REQUIRE(variableLengthDataBytes(report->parameterValues.at(temperatureOk)) ==
              std::vector<unsigned char>(
                  parameterBytes,
                  parameterBytes + sizeof(parameterBytes)));
      REQUIRE(variableLengthDataBytes(report->userSuppliedTag) ==
              std::vector<unsigned char>(tagBytes, tagBytes + sizeof(tagBytes)));
      REQUIRE(report->producingFederate == freshOwnerHandle);
      REQUIRE(report->timeImplementationName == standard_hla::mom::integer64_time);
      REQUIRE(report->timeValue == L"9");
      REQUIRE(report->sentOrderType == TIMESTAMP);
      REQUIRE(report->receivedOrderType == TIMESTAMP);
      REQUIRE(report->sentRegionsSupplied);
      REQUIRE(report->sentRegions == RegionHandleSet{sourceRegion});
      REQUIRE(report->retractionSupplied);
      REQUIRE(report->retractionValid);
    }

    REQUIRE_NOTHROW(receiverA->resignFederationExecution(NO_ACTION));
    REQUIRE_NOTHROW(receiverB->resignFederationExecution(NO_ACTION));
    REQUIRE_NOTHROW(owner->resignFederationExecution(CANCEL_THEN_DELETE_THEN_DIVEST));
    REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
    REQUIRE_NOTHROW(receiverA->disconnect());
    REQUIRE_NOTHROW(receiverB->disconnect());
    REQUIRE_NOTHROW(owner->disconnect());
  }
  };
  runScenario(rti1516_2025::HLA_EVOKED);
  runScenario(rti1516_2025::HLA_IMMEDIATE);
}
