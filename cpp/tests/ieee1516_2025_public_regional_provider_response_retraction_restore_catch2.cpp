#include "ieee1516_2025_federation_management_fixture_support.hpp"
#include "ieee1516_2025_public_federation_restore_test_support.hpp"

namespace restore_support = public_federation_restore_test_support;

namespace {
TEST_CASE(
    "Embedded public fresh-registry restore rebinds a timestamped regional provider response and retraction",
    "[integration][development-profile][federation-management][object-management][save-restore]"
    "[durable-save][filesystem][process-restart][restore][time-management][ddm][tso]"
    "[tso-attribute-update-state][tso-regional-attribute-update-state]"
    "[durable-save-regional-pending-attribute-value-update-response-retraction]"
    "[public-durable-save-regional-pending-attribute-value-update-response-retraction]"
    "[fresh-registry-tso-regional-provider-response-retraction]"
    "[rti.service.get-object-class-handle][rti.service.get-attribute-handle]"
    "[rti.service.get-dimension-handle][rti.service.publish-object-class-attributes]"
    "[rti.service.change-default-attribute-order-type]"
    "[rti.service.create-region][rti.service.set-range-bounds]"
    "[rti.service.commit-region-modifications]"
    "[rti.service.register-object-instance-with-regions]"
    "[rti.service.subscribe-object-class-attributes-with-regions]"
    "[rti.service.set-convey-region-designator-sets-switch]"
    "[rti.service.enable-time-regulation][rti.service.enable-time-constrained]"
    "[rti.service.request-attribute-value-update-with-regions]"
    "[rti.service.update-attribute-values]"
    "[rti.service.retract]"
    "[rti.service.request-federation-save][rti.service.time-advance-request]"
    "[rti.service.federate-save-begun][rti.service.federate-save-complete]"
    "[rti.service.request-federation-restore][rti.service.federate-restore-complete]"
    "[rti.service.resign-federation-execution]"
    "[federate.callback.provide-attribute-value-update]"
    "[federate.callback.reflect-attribute-values]"
    "[federate.callback.initiate-federate-save][federate.callback.federation-saved]"
    "[federate.callback.federation-restore-begun]"
    "[federate.callback.initiate-federate-restore][federate.callback.federation-restored]"
    "[federate.callback.time-constrained-enabled][federate.callback.time-regulation-enabled]"
    "[federate.callback.time-advance-grant]") {
  auto const saveDirectory = restore_support::temporaryFederationSaveDirectory();
  auto const saveStore = std::make_shared<umbra::detail::FilesystemFederationSaveCommitStore>(
      saveDirectory.path());
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  std::wstring const saveLabel = L"public-fresh-regional-provider-response";
  unsigned char const requestTagBytes[] = {0x50, 0x52, 0x51};
  unsigned char const responseTagBytes[] = {0x50, 0x52, 0x53};
  unsigned char const responseValueBytes[] = {0x52, 0x56, 0x31};
  unsigned char const baselineValueBytes[] = {0x42, 0x53, 0x4C};
  unsigned char const baselineTagBytes[] = {0x42, 0x53};
  VariableLengthData const requestTag(requestTagBytes, sizeof(requestTagBytes));
  VariableLengthData const responseTag(responseTagBytes, sizeof(responseTagBytes));
  VariableLengthData const baselineTag(baselineTagBytes, sizeof(baselineTagBytes));
  std::vector<unsigned char> const responseValue(
      responseValueBytes,
      responseValueBytes + sizeof(responseValueBytes));
  FederateHandle sourceOwnerHandle;
  ObjectInstanceHandle sourceObjectInstance;
  ObjectClassHandle sourceSoda;
  AttributeHandle sourceFlavor;
  DimensionHandle sourceSodaFlavor;
  RegionHandle sourceOwnerRegion;
  rti1516_2025::MessageRetractionHandle responseRetraction;

  auto const drainAll = [&](RTIambassador& first, RTIambassador& second) {
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
    ReportingFederateAmbassador requesterReports;
    auto owner = makeRti();
    auto requester = makeRti();

    REQUIRE_NOTHROW(owner->connect(ownerReports, HLA_EVOKED));
    REQUIRE_NOTHROW(requester->connect(requesterReports, HLA_EVOKED));
    REQUIRE_NOTHROW(owner->createFederationExecution(
        federationName,
        fomModule,
        standard_hla::mom::integer64_time));
    REQUIRE_NOTHROW(sourceOwnerHandle = owner->joinFederationExecution(
        L"public-fresh-provider-response-owner",
        L"publisher",
        federationName));
    REQUIRE_NOTHROW(requester->joinFederationExecution(
        L"public-fresh-provider-response-requester",
        L"subscriber",
        federationName));

    sourceSoda = owner->getObjectClassHandle(fixture_hla::fom::food_drink_soda);
    sourceFlavor = owner->getAttributeHandle(sourceSoda, fixture_hla::fixture::flavor);
    sourceSodaFlavor = owner->getDimensionHandle(fixture_hla::fixture::soda_flavor);
    REQUIRE(sourceSoda.isValid());
    REQUIRE(sourceFlavor.isValid());
    REQUIRE(sourceSodaFlavor.isValid());
    AttributeHandleSet const sourceFlavorOnly{sourceFlavor};
    REQUIRE_NOTHROW(owner->publishObjectClassAttributes(sourceSoda, sourceFlavorOnly));
    REQUIRE_NOTHROW(owner->changeDefaultAttributeOrderType(
        sourceSoda,
        sourceFlavorOnly,
        TIMESTAMP));

    REQUIRE_NOTHROW(sourceOwnerRegion = owner->createRegion(
        DimensionHandleSet{sourceSodaFlavor}));
    auto const requesterRegion = requester->createRegion(
        DimensionHandleSet{requester->getDimensionHandle(fixture_hla::fixture::soda_flavor)});
    REQUIRE_NOTHROW(owner->setRangeBounds(
        sourceOwnerRegion,
        sourceSodaFlavor,
        RangeBounds(0UL, 1UL)));
    auto const requesterSodaFlavor = requester->getDimensionHandle(
        fixture_hla::fixture::soda_flavor);
    REQUIRE_NOTHROW(requester->setRangeBounds(
        requesterRegion,
        requesterSodaFlavor,
        RangeBounds(0UL, 1UL)));
    REQUIRE_NOTHROW(owner->commitRegionModifications(RegionHandleSet{sourceOwnerRegion}));
    REQUIRE_NOTHROW(requester->commitRegionModifications(RegionHandleSet{requesterRegion}));
    auto const requesterSoda = requester->getObjectClassHandle(
        fixture_hla::fom::food_drink_soda);
    auto const requesterFlavor = requester->getAttributeHandle(
        requesterSoda,
        fixture_hla::fixture::flavor);
    AttributeHandleSetRegionHandleSetPairVector const sourcePair{{
        sourceFlavorOnly,
        RegionHandleSet{sourceOwnerRegion},
    }};
    AttributeHandleSetRegionHandleSetPairVector const requesterPair{{
        AttributeHandleSet{requesterFlavor},
        RegionHandleSet{requesterRegion},
    }};
    REQUIRE_NOTHROW(requester->subscribeObjectClassAttributesWithRegions(
        requesterSoda,
        requesterPair));
    REQUIRE_NOTHROW(requester->setConveyRegionDesignatorSetsSwitch(true));

    REQUIRE_NOTHROW(sourceObjectInstance = owner->registerObjectInstanceWithRegions(
        sourceSoda,
        sourcePair));
    drainAll(*owner, *requester);
    REQUIRE(requesterReports.objectDiscoveryReports.size() == 1U);
    REQUIRE(requesterReports.objectDiscoveryReports.front().objectInstance ==
            sourceObjectInstance);

    // Establish the route-free application-value ledger that the fresh
    // registry restore uses alongside the queued timestamped response.
    AttributeHandleValueMap baselineValues;
    baselineValues.emplace(
        sourceFlavor,
        VariableLengthData(baselineValueBytes, sizeof(baselineValueBytes)));
    REQUIRE_NOTHROW(owner->updateAttributeValues(
        sourceObjectInstance,
        baselineValues,
        baselineTag));
    drainAll(*owner, *requester);
    REQUIRE(requesterReports.attributeReflectionReports.size() == 1U);
    requesterReports.attributeReflectionReports.clear();
    requesterReports.callbackOrder.clear();

    // Drain Restaurant's automatic-provision setup before installing the
    // explicit response that this durable boundary exercises.
    drainAll(*owner, *requester);
    auto const initialProvideReportCount =
        ownerReports.attributeValueUpdateRequestReports.size();
    std::vector<unsigned char> observedRequestTag;
    ownerReports.provideAttributeValueUpdateHandler = [
        &owner,
        &observedRequestTag,
        &responseRetraction,
        responseValue,
        responseTag](
        ObjectInstanceHandle const& callbackObject,
        AttributeHandleSet const& callbackAttributes,
        VariableLengthData const& callbackTag) {
      observedRequestTag = variableLengthDataBytes(callbackTag);
      AttributeHandleValueMap values;
      for (AttributeHandle const& attribute : callbackAttributes) {
        values.emplace(
            attribute,
            VariableLengthData(responseValue.data(), responseValue.size()));
      }
      responseRetraction = owner->updateAttributeValues(
          callbackObject,
          values,
          responseTag,
          rti1516_2025::HLAinteger64Time(2));
    };

    REQUIRE_NOTHROW(requester->enableTimeConstrained());
    REQUIRE_FALSE(requester->evokeCallback(0.0));
    REQUIRE(requesterReports.timeConstrainedEnabledReports.size() == 1U);
    REQUIRE_NOTHROW(owner->enableTimeRegulation(
        rti1516_2025::HLAinteger64Interval(1)));
    REQUIRE_FALSE(owner->evokeCallback(0.0));
    REQUIRE(ownerReports.timeRegulationEnabledReports.size() == 1U);

    REQUIRE_NOTHROW(requester->requestAttributeValueUpdateWithRegions(
        requesterSoda,
        requesterPair,
        requestTag));
    REQUIRE(ownerReports.attributeValueUpdateRequestReports.size() ==
            initialProvideReportCount);
    REQUIRE(requesterReports.attributeReflectionReports.empty());
    drainAll(*owner, *requester);
    REQUIRE(ownerReports.attributeValueUpdateRequestReports.size() ==
            initialProvideReportCount + 1U);
    REQUIRE(responseRetraction.isValid());
    REQUIRE(observedRequestTag ==
            std::vector<unsigned char>(
                requestTagBytes,
                requestTagBytes + sizeof(requestTagBytes)));
    REQUIRE(requesterReports.attributeReflectionReports.empty());

    // Save at one while the provider's timestamp-two response is still in
    // the constrained recipient's queue.  The image must carry the payload,
    // source-region snapshot, queue entry, and retraction ledger together.
    REQUIRE_NOTHROW(owner->requestFederationSave(
        saveLabel,
        rti1516_2025::HLAinteger64Time(1)));
    REQUIRE_NOTHROW(owner->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(1)));
    REQUIRE_NOTHROW(requester->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(1)));
    drainAll(*owner, *requester);
    REQUIRE(ownerReports.initiateFederateSaveReports ==
            std::vector<std::wstring>{saveLabel});
    REQUIRE(requesterReports.initiateFederateSaveReports ==
            std::vector<std::wstring>{saveLabel});
    REQUIRE_NOTHROW(owner->federateSaveBegun());
    REQUIRE_NOTHROW(requester->federateSaveBegun());
    REQUIRE_NOTHROW(owner->federateSaveComplete());
    REQUIRE_NOTHROW(requester->federateSaveComplete());
    drainAll(*owner, *requester);
    REQUIRE(ownerReports.federationSavedReportCount == 1U);
    REQUIRE(requesterReports.federationSavedReportCount == 1U);

    auto const durable = saveStore->load(federationName, saveLabel);
    REQUIRE(durable.has_value());
    auto const image = umbra::detail::FederationStateImageCodec::decode(
        durable->stateImage);
    REQUIRE(image.tsoAttributeUpdateMessages.size() == 1U);
    REQUIRE(image.tsoAttributeUpdateMessages.front().objectInstanceHandle != 0U);
    REQUIRE(image.tsoAttributeUpdateMessages.front().userSuppliedTag ==
            std::string(reinterpret_cast<char const*>(responseTagBytes), sizeof(responseTagBytes)));
    REQUIRE(image.tsoAttributeUpdateMessages.front().sentRegionSnapshots.size() == 1U);
    REQUIRE(image.tsoQueueEntries.size() == 1U);
    REQUIRE(image.tsoRequestRetractionRecords.size() == 1U);

    REQUIRE_NOTHROW(requester->resignFederationExecution(NO_ACTION));
    REQUIRE_NOTHROW(owner->resignFederationExecution(CANCEL_THEN_DELETE_THEN_DIVEST));
    REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
    REQUIRE_NOTHROW(requester->disconnect());
    REQUIRE_NOTHROW(owner->disconnect());
  }

  {
    auto const freshRegistry = std::make_shared<umbra::detail::EmbeddedFederationRegistry>(
        nullptr,
        saveStore);
    restore_support::ScopedEmbeddedFederationRegistry freshScope(freshRegistry);
    ReportingFederateAmbassador ownerReports;
    ReportingFederateAmbassador requesterReports;
    auto owner = makeRti();
    auto requester = makeRti();
    REQUIRE_NOTHROW(owner->connect(ownerReports, HLA_EVOKED));
    REQUIRE_NOTHROW(requester->connect(requesterReports, HLA_EVOKED));
    REQUIRE_NOTHROW(owner->createFederationExecution(
        federationName,
        fomModule,
        standard_hla::mom::integer64_time));
    FederateHandle freshOwnerHandle;
    REQUIRE_NOTHROW(freshOwnerHandle = owner->joinFederationExecution(
        L"public-fresh-provider-response-owner",
        L"publisher",
        federationName));
    REQUIRE_NOTHROW(requester->joinFederationExecution(
        L"public-fresh-provider-response-requester",
        L"subscriber",
        federationName));
    REQUIRE(freshOwnerHandle == sourceOwnerHandle);
    REQUIRE_NOTHROW(owner->requestFederationRestore(saveLabel));
    drainAll(*owner, *requester);
    REQUIRE(ownerReports.requestFederationRestoreSucceededReports ==
            std::vector<std::wstring>{saveLabel});
    REQUIRE(ownerReports.federationRestoreBegunReportCount == 1U);
    REQUIRE(requesterReports.federationRestoreBegunReportCount == 1U);
    REQUIRE_NOTHROW(owner->federateRestoreComplete());
    REQUIRE_NOTHROW(requester->federateRestoreComplete());
    drainAll(*owner, *requester);
    REQUIRE(ownerReports.federationRestoredReportCount == 1U);
    REQUIRE(requesterReports.federationRestoredReportCount == 1U);
    REQUIRE(requesterReports.attributeReflectionReports.empty());
    requesterReports.callbackOrder.clear();

    auto const freshSoda = owner->getObjectClassHandle(fixture_hla::fom::food_drink_soda);
    auto const freshFlavor = owner->getAttributeHandle(
        freshSoda,
        fixture_hla::fixture::flavor);
    auto const freshSodaFlavor = owner->getDimensionHandle(
        fixture_hla::fixture::soda_flavor);
    REQUIRE(freshSoda.isValid());
    REQUIRE(freshFlavor.isValid());
    REQUIRE(freshSodaFlavor.isValid());

    REQUIRE_NOTHROW(owner->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(2)));
    REQUIRE_NOTHROW(requester->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(2)));
    drainAll(*owner, *requester);
    REQUIRE(requesterReports.attributeReflectionReports.size() == 1U);
    REQUIRE(requesterReports.timeAdvanceGrantReports.size() == 1U);
    REQUIRE(requesterReports.callbackOrder ==
            std::vector<std::string>{"reflect", "grant"});
    auto const& reflection = requesterReports.attributeReflectionReports.front();
    REQUIRE(reflection.objectInstance == sourceObjectInstance);
    REQUIRE(reflection.attributeValues.size() == 1U);
    REQUIRE(reflection.attributeValues.contains(requester->getAttributeHandle(
        requester->getObjectClassHandle(fixture_hla::fom::food_drink_soda),
        fixture_hla::fixture::flavor)));
    REQUIRE(variableLengthDataBytes(reflection.attributeValues.begin()->second) ==
            responseValue);
    REQUIRE(variableLengthDataBytes(reflection.userSuppliedTag) ==
            std::vector<unsigned char>(
                responseTagBytes,
                responseTagBytes + sizeof(responseTagBytes)));
    REQUIRE(reflection.producingFederate == freshOwnerHandle);
    REQUIRE(reflection.sentRegionsSupplied);
    REQUIRE(reflection.sentRegions == RegionHandleSet{sourceOwnerRegion});
    REQUIRE(reflection.timeValue == L"2");
    REQUIRE(reflection.sentOrderType == TIMESTAMP);
    REQUIRE(reflection.receivedOrderType == TIMESTAMP);
    REQUIRE(reflection.retractionSupplied);
    REQUIRE(reflection.retractionValid);
    REQUIRE_THROWS_AS(
        owner->retract(responseRetraction),
        rti1516_2025::MessageCanNoLongerBeRetracted);

    REQUIRE_NOTHROW(requester->resignFederationExecution(NO_ACTION));
    REQUIRE_NOTHROW(owner->resignFederationExecution(CANCEL_THEN_DELETE_THEN_DIVEST));
    REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
    REQUIRE_NOTHROW(requester->disconnect());
    REQUIRE_NOTHROW(owner->disconnect());
  }

  std::error_code ignored;
  std::filesystem::remove_all(saveDirectory.path(), ignored);
}
}  // namespace
