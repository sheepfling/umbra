#include "ieee1516_2025_federation_management_fixture_support.hpp"

TEST_CASE(
    "Embedded Delay Subscription Evaluation defers timestamped regional interaction eligibility",
    "[integration][development-profile][interaction-management][ddm][time-management]"
    "[delay-subscription-evaluation][timestamped-regional-interaction][tso]"
    "[delay-subscription-evaluation-timestamped-regional-interaction]"
    "[rti.service.send-interaction-with-regions]"
    "[rti.service.change-interaction-order-type]"
    "[rti.service.get-delay-subscription-evaluation-switch]"
    "[rti.service.create-region][rti.service.set-range-bounds]"
    "[rti.service.commit-region-modifications]"
    "[rti.service.subscribe-interaction-class-with-regions]"
    "[rti.service.unsubscribe-interaction-class-with-regions]"
    "[rti.service.set-convey-region-designator-sets-switch]"
    "[rti.service.enable-time-regulation][rti.service.enable-time-constrained]"
    "[rti.service.time-advance-request]"
    "[federate.callback.receive-interaction]"
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
    unsigned char const parameterBytes[] = {0x44, 0x53, 0x52, 0x49};
    unsigned char const tagBytes[] = {0x44, 0x53, 0x52, 0x54};
    ParameterHandleValueMap parameterValues;
    VariableLengthData const tag(tagBytes, sizeof(tagBytes));

    REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED));
    REQUIRE_NOTHROW(receiver->connect(receiverReports, HLA_EVOKED));
    REQUIRE_NOTHROW(publisher->createFederationExecution(
        federationName,
        fomModules,
        standard_hla::mom::integer64_time));
    REQUIRE_NOTHROW(publisher->joinFederationExecution(
        L"delay-regional-interaction-publisher",
        L"publisher",
        federationName));
    REQUIRE_NOTHROW(receiver->joinFederationExecution(
        L"delay-regional-interaction-receiver",
        L"subscriber",
        federationName));

    auto const interactionClass = publisher->getInteractionClassHandle(
        fixture_hla::fom::main_course_served);
    auto const temperatureOk = publisher->getParameterHandle(interactionClass, fixture_hla::fixture::temperature_ok);
    auto const serverId = publisher->getDimensionHandle(fixture_hla::fixture::server_id);
    REQUIRE(interactionClass.isValid());
    REQUIRE(temperatureOk.isValid());
    REQUIRE(serverId.isValid());
    parameterValues.emplace(
        temperatureOk,
        VariableLengthData(parameterBytes, sizeof(parameterBytes)));
    REQUIRE(publisher->getDelaySubscriptionEvaluationSwitch() ==
            delaySubscriptionEvaluationEnabled);
    REQUIRE(receiver->getDelaySubscriptionEvaluationSwitch() ==
            delaySubscriptionEvaluationEnabled);
    REQUIRE_NOTHROW(publisher->publishInteractionClass(interactionClass));
    REQUIRE_NOTHROW(publisher->changeInteractionOrderType(interactionClass, TIMESTAMP));

    auto const sourceRegion = publisher->createRegion(DimensionHandleSet{serverId});
    auto const receiverRegion = receiver->createRegion(DimensionHandleSet{serverId});
    REQUIRE_NOTHROW(publisher->setRangeBounds(
        sourceRegion,
        serverId,
        RangeBounds(0UL, 10UL)));
    REQUIRE_NOTHROW(publisher->commitRegionModifications(RegionHandleSet{sourceRegion}));
    REQUIRE_NOTHROW(receiver->setRangeBounds(
        receiverRegion,
        serverId,
        RangeBounds(5UL, 15UL)));
    REQUIRE_NOTHROW(receiver->commitRegionModifications(RegionHandleSet{receiverRegion}));
    REQUIRE_NOTHROW(receiver->setConveyRegionDesignatorSetsSwitch(true));
    REQUIRE_NOTHROW(receiver->enableTimeConstrained());
    while (receiver->evokeCallback(0.0)) {
    }
    REQUIRE(receiverReports.timeConstrainedEnabledReports.size() == 1U);
    REQUIRE_NOTHROW(publisher->enableTimeRegulation(
        rti1516_2025::HLAinteger64Interval(1)));
    while (publisher->evokeCallback(0.0)) {
    }
    REQUIRE(publisherReports.timeRegulationEnabledReports.size() == 1U);

    // The first passel is generated while no regional declaration is active.
    // Enabled delay evaluation keeps the joined recipient route so the
    // declaration added below can make the passel eligible at its grant;
    // Disabled evaluation has no recipient to recover.
    auto const firstRetraction = publisher->sendInteractionWithRegions(
        interactionClass,
        parameterValues,
        RegionHandleSet{sourceRegion},
        tag,
        rti1516_2025::HLAinteger64Time(2));
    REQUIRE(firstRetraction.isValid());
    REQUIRE(receiverReports.timestampedInteractionReports.empty());
    REQUIRE_NOTHROW(receiver->subscribeInteractionClassWithRegions(
        interactionClass,
        RegionHandleSet{receiverRegion}));
    REQUIRE_NOTHROW(receiver->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(2)));
    REQUIRE_NOTHROW(publisher->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(2)));
    while (publisher->evokeCallback(0.0)) {
    }
    while (receiver->evokeCallback(0.0)) {
    }
    std::size_t const expectedInteractions = delaySubscriptionEvaluationEnabled ? 1U : 0U;
    REQUIRE(receiverReports.timestampedInteractionReports.size() == expectedInteractions);
    REQUIRE(receiverReports.timeAdvanceGrantReports.size() == 1U);
    if (delaySubscriptionEvaluationEnabled) {
      auto const& interaction = receiverReports.timestampedInteractionReports.front();
      REQUIRE(interaction.interactionClass == interactionClass);
      REQUIRE(interaction.parameterValues.size() == 1U);
      REQUIRE(interaction.parameterValues.contains(temperatureOk));
      REQUIRE(variableLengthDataBytes(interaction.parameterValues.at(temperatureOk)) ==
              std::vector<unsigned char>(
                  parameterBytes,
                  parameterBytes + sizeof(parameterBytes)));
      REQUIRE(interaction.timeValue == L"2");
      REQUIRE(interaction.sentOrderType == TIMESTAMP);
      REQUIRE(interaction.receivedOrderType == TIMESTAMP);
      REQUIRE(interaction.sentRegionsSupplied);
      REQUIRE(interaction.sentRegions.contains(sourceRegion));
    }

    // The second passel is generated while the regional subscription is
    // active, then the declaration is removed before its callback boundary.
    // Current projection suppresses it in both switch modes.
    auto const secondRetraction = publisher->sendInteractionWithRegions(
        interactionClass,
        parameterValues,
        RegionHandleSet{sourceRegion},
        tag,
        rti1516_2025::HLAinteger64Time(3));
    REQUIRE(secondRetraction.isValid());
    REQUIRE_NOTHROW(receiver->unsubscribeInteractionClassWithRegions(
        interactionClass,
        RegionHandleSet{receiverRegion}));
    REQUIRE_NOTHROW(receiver->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(3)));
    REQUIRE_NOTHROW(publisher->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(3)));
    while (publisher->evokeCallback(0.0)) {
    }
    while (receiver->evokeCallback(0.0)) {
    }
    REQUIRE(receiverReports.timestampedInteractionReports.size() == expectedInteractions);
    REQUIRE(receiverReports.timeAdvanceGrantReports.size() == 2U);

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
