#include "ieee1516_2025_federation_management_fixture_support.hpp"

TEST_CASE(
    "Embedded Delay Subscription Evaluation defers timestamped regional attribute eligibility",
    "[integration][development-profile][object-management][ddm][time-management]"
    "[delay-subscription-evaluation][timestamped-regional-attribute-update][tso]"
    "[delay-subscription-evaluation-timestamped-regional-attribute]"
    "[rti.service.update-attribute-values]"
    "[rti.service.change-default-attribute-order-type]"
    "[rti.service.get-delay-subscription-evaluation-switch]"
    "[rti.service.create-region][rti.service.set-range-bounds]"
    "[rti.service.commit-region-modifications]"
    "[rti.service.register-object-instance-with-regions]"
    "[rti.service.associate-regions-for-updates]"
    "[rti.service.unassociate-regions-for-updates]"
    "[rti.service.subscribe-object-class-attributes-with-regions]"
    "[rti.service.unsubscribe-object-class-attributes-with-regions]"
    "[rti.service.enable-time-regulation][rti.service.enable-time-constrained]"
    "[rti.service.time-advance-request]"
    "[federate.callback.discover-object-instance]"
    "[federate.callback.reflect-attribute-values]"
    "[federate.callback.time-advance-grant]") {
  auto runScenario = [](bool const delaySubscriptionEvaluationEnabled) {
    ReportingFederateAmbassador publisherReports;
    ReportingFederateAmbassador receiverReports;
    auto publisher = makeRti();
    auto receiver = makeRti();
    auto const federationName = nextFederationName();
    auto const restaurantFom = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
    auto const switchesFom = (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
                              "cpp" / "tests" / "data" /
                              "switch-support-enabled-fom.xml")
                                 .wstring();
    std::vector<std::wstring> fomModules{restaurantFom};
    if (delaySubscriptionEvaluationEnabled) {
      fomModules.push_back(switchesFom);
    }
    unsigned char const firstValueBytes[] = {0x44, 0x53, 0x52, 0x31};
    unsigned char const secondValueBytes[] = {0x44, 0x53, 0x52, 0x32};
    AttributeHandleValueMap firstValues;
    AttributeHandleValueMap secondValues;

    REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED));
    REQUIRE_NOTHROW(receiver->connect(receiverReports, HLA_EVOKED));
    REQUIRE_NOTHROW(publisher->createFederationExecution(
        federationName,
        fomModules,
        standard_hla::mom::integer64_time));
    REQUIRE_NOTHROW(publisher->joinFederationExecution(
        L"delay-regional-attribute-publisher",
        L"publisher",
        federationName));
    REQUIRE_NOTHROW(receiver->joinFederationExecution(
        L"delay-regional-attribute-receiver",
        L"subscriber",
        federationName));

    auto const soda = publisher->getObjectClassHandle(fixture_hla::fom::food_drink_soda);
    auto const flavor = publisher->getAttributeHandle(soda, fixture_hla::fixture::flavor);
    auto const sodaFlavor = publisher->getDimensionHandle(fixture_hla::fixture::soda_flavor);
    REQUIRE(soda.isValid());
    REQUIRE(flavor.isValid());
    REQUIRE(sodaFlavor.isValid());
    AttributeHandleSet const flavorOnly{flavor};
    firstValues.emplace(
        flavor,
        VariableLengthData(firstValueBytes, sizeof(firstValueBytes)));
    secondValues.emplace(
        flavor,
        VariableLengthData(secondValueBytes, sizeof(secondValueBytes)));
    REQUIRE(publisher->getDelaySubscriptionEvaluationSwitch() ==
            delaySubscriptionEvaluationEnabled);
    REQUIRE(receiver->getDelaySubscriptionEvaluationSwitch() ==
            delaySubscriptionEvaluationEnabled);
    REQUIRE_NOTHROW(publisher->publishObjectClassAttributes(soda, flavorOnly));
    REQUIRE_NOTHROW(publisher->changeDefaultAttributeOrderType(
        soda,
        flavorOnly,
        TIMESTAMP));

    auto const sourceRegion = publisher->createRegion(DimensionHandleSet{sodaFlavor});
    auto const receiverRegion = receiver->createRegion(DimensionHandleSet{sodaFlavor});
    REQUIRE_NOTHROW(publisher->setRangeBounds(
        sourceRegion,
        sodaFlavor,
        RangeBounds(0UL, 1UL)));
    REQUIRE_NOTHROW(publisher->commitRegionModifications(RegionHandleSet{sourceRegion}));
    REQUIRE_NOTHROW(receiver->setRangeBounds(
        receiverRegion,
        sodaFlavor,
        RangeBounds(0UL, 1UL)));
    REQUIRE_NOTHROW(receiver->commitRegionModifications(RegionHandleSet{receiverRegion}));
    AttributeHandleSetRegionHandleSetPairVector const sourcePair{{
        flavorOnly,
        RegionHandleSet{sourceRegion},
    }};
    AttributeHandleSetRegionHandleSetPairVector const receiverPair{{
        flavorOnly,
        RegionHandleSet{receiverRegion},
    }};
    ObjectInstanceHandle objectInstance;
    REQUIRE_NOTHROW(objectInstance = publisher->registerObjectInstanceWithRegions(
        soda,
        sourcePair));
    REQUIRE_NOTHROW(receiver->subscribeObjectClassAttributesWithRegions(soda, receiverPair));
    while (receiver->evokeCallback(0.0)) {
    }
    REQUIRE(receiverReports.objectDiscoveryReports.size() == 1U);
    REQUIRE_NOTHROW(receiver->unsubscribeObjectClassAttributesWithRegions(soda, receiverPair));
    REQUIRE_NOTHROW(receiver->setConveyRegionDesignatorSetsSwitch(true));
    REQUIRE_NOTHROW(receiver->enableTimeConstrained());
    REQUIRE_FALSE(receiver->evokeCallback(0.0));
    REQUIRE(receiverReports.timeConstrainedEnabledReports.size() == 1U);
    REQUIRE_NOTHROW(publisher->enableTimeRegulation(
        rti1516_2025::HLAinteger64Interval(1)));
    while (publisher->evokeCallback(0.0)) {
    }
    REQUIRE(publisherReports.timeRegulationEnabledReports.size() == 1U);

    // The first passel is accepted while no regional declaration is active.
    // Enabled delay evaluation retains the joined recipient route so that the
    // declaration added below can make this passel eligible at its grant.
    auto const firstRetraction = publisher->updateAttributeValues(
        objectInstance,
        firstValues,
        VariableLengthData(),
        rti1516_2025::HLAinteger64Time(2));
    REQUIRE(firstRetraction.isValid());
    REQUIRE(receiverReports.attributeReflectionReports.empty());
    REQUIRE_NOTHROW(receiver->subscribeObjectClassAttributesWithRegions(soda, receiverPair));
    REQUIRE_NOTHROW(receiver->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(2)));
    REQUIRE_NOTHROW(publisher->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(2)));
    while (publisher->evokeCallback(0.0)) {
    }
    while (receiver->evokeCallback(0.0)) {
    }
    std::size_t const expectedReflections = delaySubscriptionEvaluationEnabled ? 1U : 0U;
    REQUIRE(receiverReports.attributeReflectionReports.size() == expectedReflections);
    REQUIRE(receiverReports.timeAdvanceGrantReports.size() == 1U);
    if (delaySubscriptionEvaluationEnabled) {
      auto const& reflection = receiverReports.attributeReflectionReports.front();
      REQUIRE(reflection.objectInstance == objectInstance);
      REQUIRE(reflection.attributeValues.size() == 1U);
      REQUIRE(reflection.attributeValues.contains(flavor));
      REQUIRE(variableLengthDataBytes(reflection.attributeValues.at(flavor)) ==
              std::vector<unsigned char>(
                  firstValueBytes,
                  firstValueBytes + sizeof(firstValueBytes)));
      REQUIRE(reflection.timeValue == L"2");
      REQUIRE(reflection.sentOrderType == TIMESTAMP);
      REQUIRE(reflection.receivedOrderType == TIMESTAMP);
      REQUIRE(reflection.sentRegionsSupplied);
      REQUIRE(reflection.sentRegions.contains(sourceRegion));
    }

    // The second passel is generated while subscribed, then the declaration is
    // removed before its callback boundary. Current projection suppresses it
    // in both switch modes; delayed evaluation is not a promise to deliver a
    // stale callback after the declaration disappears.
    auto const secondRetraction = publisher->updateAttributeValues(
        objectInstance,
        secondValues,
        VariableLengthData(),
        rti1516_2025::HLAinteger64Time(3));
    REQUIRE(secondRetraction.isValid());
    REQUIRE_NOTHROW(receiver->unsubscribeObjectClassAttributesWithRegions(soda, receiverPair));
    REQUIRE_NOTHROW(receiver->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(3)));
    REQUIRE_NOTHROW(publisher->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(3)));
    while (publisher->evokeCallback(0.0)) {
    }
    while (receiver->evokeCallback(0.0)) {
    }
    REQUIRE(receiverReports.attributeReflectionReports.size() == expectedReflections);
    REQUIRE(receiverReports.timeAdvanceGrantReports.size() == 2U);

    REQUIRE_NOTHROW(publisher->unassociateRegionsForUpdates(objectInstance, sourcePair));
    REQUIRE_NOTHROW(receiver->deleteRegion(receiverRegion));
    REQUIRE_NOTHROW(publisher->deleteRegion(sourceRegion));
    REQUIRE_NOTHROW(receiver->resignFederationExecution(NO_ACTION));
    REQUIRE_NOTHROW(publisher->resignFederationExecution(CANCEL_THEN_DELETE_THEN_DIVEST));
    REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
    REQUIRE_NOTHROW(receiver->disconnect());
    REQUIRE_NOTHROW(publisher->disconnect());
  };

  SECTION("the creation-time switch is Enabled") {
    runScenario(true);
  }
  SECTION("the omitted switch uses the Disabled default") {
    runScenario(false);
  }
}
