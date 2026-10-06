#include "ieee1516_2025_federation_management_fixture_support.hpp"

namespace {
TEST_CASE(
    "Embedded Allow Relaxed DDM expands only touching regional interaction ranges",
    "[integration][development-profile][interaction-management][ddm][allow-relaxed-ddm]"
    "[rti.service.get-allow-relaxed-ddm-switch]"
    "[rti.service.create-region]"
    "[rti.service.set-range-bounds]"
    "[rti.service.commit-region-modifications]"
    "[rti.service.subscribe-interaction-class-with-regions]"
    "[rti.service.send-interaction-with-regions]"
    "[federate.callback.receive-interaction]") {
  auto runScenario = [](bool const relaxedDdmEnabled) {
    ReportingFederateAmbassador publisherReports;
    ReportingFederateAmbassador subscriberReports;
    auto publisher = makeRti();
    auto subscriber = makeRti();
    auto const federationName = nextFederationName();
    auto const restaurantFom =
        resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
    auto const relaxedDdmFom = (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
                                "cpp" / "tests" / "data" /
                                "allow-relaxed-ddm-enabled-fom.xml")
                                   .wstring();
    std::vector<std::wstring> fomModules{restaurantFom};
    if (relaxedDdmEnabled) {
      fomModules.push_back(relaxedDdmFom);
    }
    unsigned char const parameterBytes[] = {0x6B, 0x51};
    ParameterHandleValueMap parameterValues;
    VariableLengthData const tag;

    REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED));
    REQUIRE_NOTHROW(subscriber->connect(subscriberReports, HLA_EVOKED));
    REQUIRE_NOTHROW(publisher->createFederationExecution(
        federationName, fomModules, standard_hla::mom::integer64_time));
    REQUIRE_NOTHROW(publisher->joinFederationExecution(
        L"relaxed-ddm-publisher", L"publisher", federationName));
    REQUIRE_NOTHROW(subscriber->joinFederationExecution(
        L"relaxed-ddm-subscriber", L"subscriber", federationName));
    REQUIRE(publisher->getAllowRelaxedDDMSwitch() == relaxedDdmEnabled);
    REQUIRE(subscriber->getAllowRelaxedDDMSwitch() == relaxedDdmEnabled);

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
    REQUIRE_NOTHROW(publisher->publishInteractionClass(interactionClass));

    auto const sourceRegion = publisher->createRegion(DimensionHandleSet{serverId});
    auto const subscriptionRegion = subscriber->createRegion(DimensionHandleSet{serverId});
    REQUIRE_NOTHROW(publisher->setRangeBounds(
        sourceRegion,
        serverId,
        RangeBounds(0UL, 10UL)));
    REQUIRE_NOTHROW(publisher->commitRegionModifications(RegionHandleSet{sourceRegion}));
    REQUIRE_NOTHROW(subscriber->setRangeBounds(
        subscriptionRegion,
        serverId,
        RangeBounds(10UL, 20UL)));
    REQUIRE_NOTHROW(subscriber->commitRegionModifications(
        RegionHandleSet{subscriptionRegion}));
    REQUIRE_NOTHROW(subscriber->subscribeInteractionClassWithRegions(
        interactionClass,
        RegionHandleSet{subscriptionRegion}));

    // [0, 10) and [10, 20) do not strictly overlap.  Umbra's explicit
    // Relaxed DDM policy admits this exact-boundary pair only when the
    // federation-wide switch is enabled.
    REQUIRE_NOTHROW(publisher->sendInteractionWithRegions(
        interactionClass,
        parameterValues,
        RegionHandleSet{sourceRegion},
        tag));
    static_cast<void>(subscriber->evokeMultipleCallbacks(0.0, 0.0));
    std::size_t expectedCallbacks = relaxedDdmEnabled ? 1U : 0U;
    REQUIRE(subscriberReports.interactionReports.size() == expectedCallbacks);

    // A nonzero gap is never treated as relaxed overlap.
    REQUIRE_NOTHROW(subscriber->setRangeBounds(
        subscriptionRegion,
        serverId,
        RangeBounds(11UL, 20UL)));
    REQUIRE_NOTHROW(subscriber->commitRegionModifications(
        RegionHandleSet{subscriptionRegion}));
    REQUIRE_NOTHROW(publisher->sendInteractionWithRegions(
        interactionClass,
        parameterValues,
        RegionHandleSet{sourceRegion},
        tag));
    static_cast<void>(subscriber->evokeMultipleCallbacks(0.0, 0.0));
    REQUIRE(subscriberReports.interactionReports.size() == expectedCallbacks);

    // Relaxation is monotonic: an already strict overlap remains eligible in
    // both federation configurations.
    REQUIRE_NOTHROW(subscriber->setRangeBounds(
        subscriptionRegion,
        serverId,
        RangeBounds(5UL, 15UL)));
    REQUIRE_NOTHROW(subscriber->commitRegionModifications(
        RegionHandleSet{subscriptionRegion}));
    REQUIRE_NOTHROW(publisher->sendInteractionWithRegions(
        interactionClass,
        parameterValues,
        RegionHandleSet{sourceRegion},
        tag));
    static_cast<void>(subscriber->evokeMultipleCallbacks(0.0, 0.0));
    ++expectedCallbacks;
    REQUIRE(subscriberReports.interactionReports.size() == expectedCallbacks);

    REQUIRE_NOTHROW(subscriber->unsubscribeInteractionClassWithRegions(
        interactionClass,
        RegionHandleSet{subscriptionRegion}));
    REQUIRE_NOTHROW(subscriber->deleteRegion(subscriptionRegion));
    REQUIRE_NOTHROW(publisher->deleteRegion(sourceRegion));
    REQUIRE_NOTHROW(subscriber->resignFederationExecution(NO_ACTION));
    REQUIRE_NOTHROW(publisher->resignFederationExecution(NO_ACTION));
    REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
    REQUIRE_NOTHROW(subscriber->disconnect());
    REQUIRE_NOTHROW(publisher->disconnect());
  };

  SECTION("the FDD enables Relaxed DDM") {
    runScenario(true);
  }
  SECTION("the FDD leaves Relaxed DDM disabled") {
    runScenario(false);
  }
}
}
