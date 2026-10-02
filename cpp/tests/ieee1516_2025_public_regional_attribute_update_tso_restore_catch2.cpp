#include "ieee1516_2025_federation_management_test_support.hpp"
#include "ieee1516_2025_federation_management_fixture_support.hpp"
#include "ieee1516_2025_public_federation_restore_test_support.hpp"

namespace {
TEST_CASE(
    "Embedded public fresh-registry restore rebinds queued timestamped regional attribute update and preserves report-file lifetimes under HLA_EVOKED and HLA_IMMEDIATE",
    "[integration][development-profile][federation-management][object-management][save-restore]"
    "[durable-save][filesystem][process-restart][restore][time-management][ddm]"
    "[service-report-file][service-reporting][tso-queue-state][tso-payload-state]"
    "[tso-attribute-update-state][tso-regional-attribute-update-state]"
    "[process-restart-regional-attribute-update-tso-ddm]"
    "[public-process-restart-regional-attribute-update-tso-ddm]"
    "[rti.service.get-object-class-handle][rti.service.get-attribute-handle]"
    "[rti.service.get-dimension-handle][rti.service.publish-object-class-attributes]"
    "[rti.service.change-default-attribute-order-type]"
    "[rti.service.create-region][rti.service.set-range-bounds]"
    "[rti.service.commit-region-modifications]"
    "[rti.service.register-object-instance]"
    "[rti.service.associate-regions-for-updates][rti.service.unassociate-regions-for-updates]"
    "[rti.service.change-default-attribute-order-type]"
    "[rti.service.enable-time-regulation][rti.service.enable-time-constrained]"
    "[rti.service.time-advance-request][rti.service.flush-queue-request][rti.service.retract]"
    "[federate.callback.initiate-federate-save][federate.callback.time-advance-grant]"
    "[federate.callback.reflect-attribute-values][federate.callback.request-retraction]"
    "[federate.callback.federation-saved][federate.callback.federation-restored]"
    "[federate.callback.flush-queue-grant]"
    "[rti.service.subscribe-object-class-attributes-with-regions]"
    "[rti.service.set-convey-region-designator-sets-switch]"
    "[rti.service.enable-time-regulation][rti.service.enable-time-constrained]"
    "[rti.service.update-attribute-values]"
    "[rti.service.request-federation-save][rti.service.time-advance-request]"
    "[rti.service.federate-save-begun][rti.service.federate-save-complete]"
    "[rti.service.request-federation-restore][rti.service.federate-restore-complete]"
    "[rti.service.get-range-bounds]"
    "[federate.callback.reflect-attribute-values]"
    "[federate.callback.initiate-federate-save][federate.callback.federation-saved]"
    "[federate.callback.federation-restore-begun]"
    "[federate.callback.initiate-federate-restore][federate.callback.federation-restored]"
    "[federate.callback.time-advance-grant]"
    "[federate.callback.time-constrained-enabled][federate.callback.time-regulation-enabled]"
    "[rti.service.disable-callbacks][rti.service.enable-callbacks]"
    "[callback-immediate]") {
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
  std::wstring const saveLabel = L"public-fresh-regional-attribute-restore-queued";
  unsigned char const baselineBytes[] = {0x42, 0x41, 0x53, 0x45};
  unsigned char const valueBytes[] = {0x41, 0x54, 0x54, 0x52};
  unsigned char const baselineTagBytes[] = {0x42, 0x41, 0x53, 0x45};
  unsigned char const tagBytes[] = {0x50, 0x52, 0x46, 0x41};
  VariableLengthData const baselineTag(baselineTagBytes, sizeof(baselineTagBytes));
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));
  FederateHandle sourceOwnerHandle;
  ObjectInstanceHandle sourceObjectInstance;
  ObjectClassHandle sourceSoda;
  AttributeHandle sourceFlavor;
  DimensionHandle sourceSodaFlavor;
  RegionHandle sourceRegion;
  RegionHandle sourceReceiverARegion;
  RegionHandle sourceReceiverBRegion;
  std::filesystem::path sourceReportFile;

  auto const drainAll = [&](RTIambassador& first,
                            RTIambassador& second,
                            RTIambassador& third) {
    // Callback work can cross routes after the route's turn in a pass.  Keep
    // this bounded and deterministic so save/restore assertions never depend
    // on an unbounded polling loop.
    for (int pass = 0; pass != 32; ++pass) {
      static_cast<void>(first.evokeCallback(0.0));
      static_cast<void>(second.evokeCallback(0.0));
      static_cast<void>(third.evokeCallback(0.0));
    }
  };

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
        L"public-fresh-regional-attribute-owner",
        L"publisher",
        federationName));
    REQUIRE_NOTHROW(receiverA->joinFederationExecution(
        L"public-fresh-regional-attribute-receiver-a",
        L"subscriber",
        federationName));
    REQUIRE_NOTHROW(receiverB->joinFederationExecution(
        L"public-fresh-regional-attribute-receiver-b",
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

    sourceSoda = owner->getObjectClassHandle(fixture_hla::fom::food_drink_soda);
    sourceFlavor = owner->getAttributeHandle(sourceSoda, fixture_hla::fixture::flavor);
    sourceSodaFlavor = owner->getDimensionHandle(fixture_hla::fixture::soda_flavor);
    auto const receiverASoda = receiverA->getObjectClassHandle(
        fixture_hla::fom::food_drink_soda);
    auto const receiverAFlavor = receiverA->getAttributeHandle(
        receiverASoda, fixture_hla::fixture::flavor);
    auto const receiverASodaFlavor = receiverA->getDimensionHandle(
        fixture_hla::fixture::soda_flavor);
    auto const receiverBSoda = receiverB->getObjectClassHandle(
        fixture_hla::fom::food_drink_soda);
    auto const receiverBFlavor = receiverB->getAttributeHandle(
        receiverBSoda, fixture_hla::fixture::flavor);
    auto const receiverBSodaFlavor = receiverB->getDimensionHandle(
        fixture_hla::fixture::soda_flavor);
    REQUIRE(sourceSoda.isValid());
    REQUIRE(sourceFlavor.isValid());
    REQUIRE(sourceSodaFlavor.isValid());
    REQUIRE(receiverASoda.isValid());
    REQUIRE(receiverAFlavor.isValid());
    REQUIRE(receiverASodaFlavor.isValid());
    REQUIRE(receiverBSoda.isValid());
    REQUIRE(receiverBFlavor.isValid());
    REQUIRE(receiverBSodaFlavor.isValid());
    AttributeHandleSet const sourceFlavorOnly{sourceFlavor};
    AttributeHandleSet const receiverAFlavorOnly{receiverAFlavor};
    AttributeHandleSet const receiverBFlavorOnly{receiverBFlavor};
    REQUIRE_NOTHROW(owner->publishObjectClassAttributes(sourceSoda, sourceFlavorOnly));
    REQUIRE_NOTHROW(owner->changeDefaultAttributeOrderType(
        sourceSoda,
        sourceFlavorOnly,
        TIMESTAMP));

    REQUIRE_NOTHROW(sourceRegion = owner->createRegion(
        DimensionHandleSet{sourceSodaFlavor}));
    REQUIRE_NOTHROW(sourceReceiverARegion = receiverA->createRegion(
        DimensionHandleSet{receiverASodaFlavor}));
    REQUIRE_NOTHROW(sourceReceiverBRegion = receiverB->createRegion(
        DimensionHandleSet{receiverBSodaFlavor}));
    REQUIRE_NOTHROW(owner->setRangeBounds(
        sourceRegion,
        sourceSodaFlavor,
        RangeBounds(2UL, 4UL)));
    REQUIRE_NOTHROW(owner->commitRegionModifications(RegionHandleSet{sourceRegion}));
    REQUIRE_NOTHROW(receiverA->setRangeBounds(
        sourceReceiverARegion,
        receiverASodaFlavor,
        RangeBounds(2UL, 4UL)));
    REQUIRE_NOTHROW(receiverA->commitRegionModifications(
        RegionHandleSet{sourceReceiverARegion}));
    REQUIRE_NOTHROW(receiverB->setRangeBounds(
        sourceReceiverBRegion,
        receiverBSodaFlavor,
        RangeBounds(2UL, 4UL)));
    REQUIRE_NOTHROW(receiverB->commitRegionModifications(
        RegionHandleSet{sourceReceiverBRegion}));
    AttributeHandleSetRegionHandleSetPairVector const receiverAPair{{
        receiverAFlavorOnly,
        RegionHandleSet{sourceReceiverARegion},
    }};
    AttributeHandleSetRegionHandleSetPairVector const receiverBPair{{
        receiverBFlavorOnly,
        RegionHandleSet{sourceReceiverBRegion},
    }};
    AttributeHandleSetRegionHandleSetPairVector const sourcePair{{
        sourceFlavorOnly,
        RegionHandleSet{sourceRegion},
    }};
    REQUIRE_NOTHROW(receiverA->subscribeObjectClassAttributesWithRegions(
        receiverASoda,
        receiverAPair));
    REQUIRE_NOTHROW(receiverB->subscribeObjectClassAttributesWithRegions(
        receiverBSoda,
        receiverBPair));
    REQUIRE_NOTHROW(receiverA->setConveyRegionDesignatorSetsSwitch(true));
    REQUIRE_NOTHROW(receiverB->setConveyRegionDesignatorSetsSwitch(true));

    REQUIRE_NOTHROW(sourceObjectInstance = owner->registerObjectInstance(sourceSoda));
    drainAll(*owner, *receiverA, *receiverB);
    REQUIRE(receiverAReports.objectDiscoveryReports.size() == 1U);
    REQUIRE(receiverBReports.objectDiscoveryReports.size() == 1U);
    REQUIRE(receiverAReports.objectDiscoveryReports.front().objectInstance ==
            sourceObjectInstance);
    REQUIRE(receiverBReports.objectDiscoveryReports.front().objectInstance ==
            sourceObjectInstance);

    // Establish one durable application value before the queued TSO update.
    // The source-region association remains live through the save so the
    // persisted object ledger and passel retain the same route identity; the
    // source is unassociated only during teardown after the fresh restore.
    AttributeHandleValueMap baselineValues;
    baselineValues.emplace(
        sourceFlavor,
        VariableLengthData(baselineBytes, sizeof(baselineBytes)));
    REQUIRE_NOTHROW(owner->updateAttributeValues(
        sourceObjectInstance,
        baselineValues,
        baselineTag));
    drainAll(*owner, *receiverA, *receiverB);
    REQUIRE(receiverAReports.attributeReflectionReports.size() == 1U);
    REQUIRE(receiverBReports.attributeReflectionReports.size() == 1U);
    receiverAReports.attributeReflectionReports.clear();
    receiverBReports.attributeReflectionReports.clear();
    receiverAReports.callbackOrder.clear();
    receiverBReports.callbackOrder.clear();

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

    REQUIRE_NOTHROW(owner->associateRegionsForUpdates(
        sourceObjectInstance,
        sourcePair));
    AttributeHandleValueMap values;
    values.emplace(
        sourceFlavor,
        VariableLengthData(valueBytes, sizeof(valueBytes)));
    auto const retraction = owner->updateAttributeValues(
        sourceObjectInstance,
        values,
        tag,
        rti1516_2025::HLAinteger64Time(9));
    REQUIRE(retraction.isValid());
    REQUIRE(receiverAReports.attributeReflectionReports.empty());
    REQUIRE(receiverBReports.attributeReflectionReports.empty());
    REQUIRE_NOTHROW(owner->requestFederationSave(
        saveLabel,
        rti1516_2025::HLAinteger64Time(7)));
    // HLA_IMMEDIATE executes an admitted grant synchronously. Hold every
    // route while the three members cross the timed-save boundary so the
    // durable image has the same membership and queue frontier as HLA_EVOKED.
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
    REQUIRE(durableImage.objects.size() == 1U);
    REQUIRE(durableImage.regions.size() == 3U);
    REQUIRE(durableImage.tsoAttributeUpdateMessages.size() == 1U);
    REQUIRE(durableImage.tsoQueueEntries.size() == 2U);
    REQUIRE(durableImage.tsoAttributeUpdateMessages.front().objectInstanceHandle != 0U);
    REQUIRE(durableImage.tsoAttributeUpdateMessages.front().sentRegionSnapshots.size() ==
            1U);

    REQUIRE_NOTHROW(owner->setRangeBounds(
        sourceRegion,
        sourceSodaFlavor,
        RangeBounds(0UL, 2UL)));
    REQUIRE_NOTHROW(owner->commitRegionModifications(RegionHandleSet{sourceRegion}));
    auto const mutatedBounds = owner->getRangeBounds(sourceRegion, sourceSodaFlavor);
    REQUIRE(mutatedBounds.getLowerBound() == 0UL);
    REQUIRE(mutatedBounds.getUpperBound() == 2UL);

    REQUIRE_NOTHROW(receiverA->resignFederationExecution(NO_ACTION));
    REQUIRE_NOTHROW(receiverB->resignFederationExecution(NO_ACTION));
    REQUIRE_NOTHROW(owner->resignFederationExecution(
        rti1516_2025::UNCONDITIONALLY_DIVEST_ATTRIBUTES));
    REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
    REQUIRE_NOTHROW(receiverA->disconnect());
    REQUIRE_NOTHROW(receiverB->disconnect());
    REQUIRE_NOTHROW(owner->disconnect());
  }

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
        L"public-fresh-regional-attribute-owner",
        L"publisher",
        federationName));
    REQUIRE_NOTHROW(receiverA->joinFederationExecution(
        L"public-fresh-regional-attribute-receiver-a",
        L"subscriber",
        federationName));
    REQUIRE_NOTHROW(receiverB->joinFederationExecution(
        L"public-fresh-regional-attribute-receiver-b",
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

    auto const freshSoda = owner->getObjectClassHandle(fixture_hla::fom::food_drink_soda);
    auto const freshFlavor = owner->getAttributeHandle(
        freshSoda,
        fixture_hla::fixture::flavor);
    auto const freshSodaFlavor = owner->getDimensionHandle(fixture_hla::fixture::soda_flavor);
    REQUIRE(freshSoda.isValid());
    REQUIRE(freshFlavor.isValid());
    REQUIRE(freshSodaFlavor.isValid());
    AttributeHandleSetRegionHandleSetPairVector const freshSourcePair{{
        AttributeHandleSet{freshFlavor},
        RegionHandleSet{sourceRegion},
    }};

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
    REQUIRE(receiverAReports.attributeReflectionReports.empty());
    REQUIRE(receiverBReports.attributeReflectionReports.empty());

    auto const restoredBounds = owner->getRangeBounds(sourceRegion, freshSodaFlavor);
    REQUIRE(restoredBounds.getLowerBound() == 2UL);
    REQUIRE(restoredBounds.getUpperBound() == 4UL);
    auto const filesAfterRestore = serviceReportFiles(reportDirectory.path());
    REQUIRE(filesAfterRestore == filesAfterFreshJoin);
    auto const freshAfterRestoreReportText = readTextFile(freshReportFile);
    REQUIRE(freshAfterRestoreReportText.size() > freshInitialReportText.size());
    REQUIRE(freshAfterRestoreReportText.find("RequestFederationRestore") !=
            std::string::npos);
    REQUIRE(freshAfterRestoreReportText.find("FederateRestoreComplete") !=
            std::string::npos);

    REQUIRE_NOTHROW(owner->setRangeBounds(
        sourceRegion,
        freshSodaFlavor,
        RangeBounds(0UL, 2UL)));
    REQUIRE_NOTHROW(owner->commitRegionModifications(RegionHandleSet{sourceRegion}));
    auto const freshMutatedBounds = owner->getRangeBounds(sourceRegion, freshSodaFlavor);
    REQUIRE(freshMutatedBounds.getLowerBound() == 0UL);
    REQUIRE(freshMutatedBounds.getUpperBound() == 2UL);

    REQUIRE_NOTHROW(owner->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(8)));
    REQUIRE_FALSE(owner->evokeCallback(0.0));
    REQUIRE_NOTHROW(receiverA->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(9)));
    REQUIRE_NOTHROW(receiverB->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(9)));
    drainAll(*owner, *receiverA, *receiverB);
    REQUIRE(receiverAReports.attributeReflectionReports.size() == 1U);
    REQUIRE(receiverBReports.attributeReflectionReports.size() == 1U);
    auto const verifyReflection = [&](ReportingFederateAmbassador const& reports,
                                      AttributeHandle const& flavor) {
      auto const& reflection = reports.attributeReflectionReports.front();
      REQUIRE(reflection.objectInstance == sourceObjectInstance);
      REQUIRE(reflection.attributeValues.size() == 1U);
      REQUIRE(reflection.attributeValues.contains(flavor));
      REQUIRE(variableLengthDataBytes(reflection.attributeValues.at(flavor)) ==
              std::vector<unsigned char>(valueBytes, valueBytes + sizeof(valueBytes)));
      REQUIRE(variableLengthDataBytes(reflection.userSuppliedTag) ==
              std::vector<unsigned char>(tagBytes, tagBytes + sizeof(tagBytes)));
      REQUIRE(reflection.producingFederate == freshOwnerHandle);
      REQUIRE(reflection.sentRegionsSupplied);
      REQUIRE(reflection.sentRegions == RegionHandleSet{sourceRegion});
      REQUIRE(reflection.timeImplementationName == standard_hla::mom::integer64_time);
      REQUIRE(reflection.timeValue == L"9");
      REQUIRE(reflection.sentOrderType == TIMESTAMP);
      REQUIRE(reflection.receivedOrderType == TIMESTAMP);
      REQUIRE(reflection.retractionSupplied);
      REQUIRE(reflection.retractionValid);
    };
    verifyReflection(receiverAReports, receiverA->getAttributeHandle(
        receiverA->getObjectClassHandle(fixture_hla::fom::food_drink_soda),
        fixture_hla::fixture::flavor));
    verifyReflection(receiverBReports, receiverB->getAttributeHandle(
        receiverB->getObjectClassHandle(fixture_hla::fom::food_drink_soda),
        fixture_hla::fixture::flavor));

    auto const freshSodaA = receiverA->getObjectClassHandle(fixture_hla::fom::food_drink_soda);
    auto const freshFlavorA = receiverA->getAttributeHandle(
        freshSodaA,
        fixture_hla::fixture::flavor);
    auto const freshSodaB = receiverB->getObjectClassHandle(fixture_hla::fom::food_drink_soda);
    auto const freshFlavorB = receiverB->getAttributeHandle(
        freshSodaB,
        fixture_hla::fixture::flavor);
    AttributeHandleSetRegionHandleSetPairVector const freshReceiverAPair{{
        AttributeHandleSet{freshFlavorA},
        RegionHandleSet{sourceReceiverARegion},
    }};
    AttributeHandleSetRegionHandleSetPairVector const freshReceiverBPair{{
        AttributeHandleSet{freshFlavorB},
        RegionHandleSet{sourceReceiverBRegion},
    }};
    REQUIRE_NOTHROW(receiverA->unsubscribeObjectClassAttributesWithRegions(
        freshSodaA,
        freshReceiverAPair));
    REQUIRE_NOTHROW(receiverB->unsubscribeObjectClassAttributesWithRegions(
        freshSodaB,
        freshReceiverBPair));
    REQUIRE_NOTHROW(receiverA->deleteRegion(sourceReceiverARegion));
    REQUIRE_NOTHROW(receiverB->deleteRegion(sourceReceiverBRegion));
    REQUIRE_NOTHROW(owner->unassociateRegionsForUpdates(
        sourceObjectInstance,
        freshSourcePair));
    REQUIRE_NOTHROW(owner->deleteRegion(sourceRegion));
    REQUIRE_NOTHROW(receiverA->resignFederationExecution(NO_ACTION));
    REQUIRE_NOTHROW(receiverB->resignFederationExecution(NO_ACTION));
    REQUIRE_NOTHROW(owner->resignFederationExecution(
        rti1516_2025::UNCONDITIONALLY_DIVEST_ATTRIBUTES));
    REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
    REQUIRE_NOTHROW(receiverA->disconnect());
    REQUIRE_NOTHROW(receiverB->disconnect());
    REQUIRE_NOTHROW(owner->disconnect());
  }
  };
  runScenario(rti1516_2025::HLA_EVOKED);
  runScenario(rti1516_2025::HLA_IMMEDIATE);
}
}
