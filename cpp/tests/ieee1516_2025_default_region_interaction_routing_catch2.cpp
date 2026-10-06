#include "ieee1516_2025_federation_management_fixture_support.hpp"

namespace {
TEST_CASE(
    "Embedded default-region interaction routing derives 2025 ordinary and regional effectiveness",
    "[integration][development-profile][interaction-management][ddm][default-region]"
    "[rti.service.subscribe-interaction-class]"
    "[rti.service.subscribe-interaction-class-with-regions]"
    "[rti.service.unsubscribe-interaction-class-with-regions]"
    "[rti.service.send-interaction]"
    "[rti.service.send-interaction-with-regions]"
    "[federate.callback.receive-interaction]") {
  ReportingFederateAmbassador publisherReports;
  ReportingFederateAmbassador regionalReports;
  ReportingFederateAmbassador mixedReports;
  auto publisher = makeRti();
  auto regional = makeRti();
  auto mixed = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  unsigned char const parameterBytes[] = {0xD1, 0x04};
  ParameterHandleValueMap parameters;

  REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED));
  REQUIRE_NOTHROW(regional->connect(regionalReports, HLA_EVOKED));
  REQUIRE_NOTHROW(mixed->connect(mixedReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      publisher->createFederationExecution(federationName, fomModule, standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(publisher->joinFederationExecution(
      L"default-region-interaction-publisher", L"publisher", federationName));
  REQUIRE_NOTHROW(regional->joinFederationExecution(
      L"default-region-interaction-regional", L"subscriber", federationName));
  REQUIRE_NOTHROW(mixed->joinFederationExecution(
      L"default-region-interaction-mixed", L"subscriber", federationName));

  auto const interactionClass = publisher->getInteractionClassHandle(
      fixture_hla::fom::main_course_served);
  auto const temperatureOk = publisher->getParameterHandle(interactionClass, fixture_hla::fixture::temperature_ok);
  auto const serverId = publisher->getDimensionHandle(fixture_hla::fixture::server_id);
  REQUIRE(interactionClass.isValid());
  REQUIRE(temperatureOk.isValid());
  REQUIRE(serverId.isValid());
  parameters.emplace(
      temperatureOk,
      VariableLengthData(parameterBytes, sizeof(parameterBytes)));
  REQUIRE_NOTHROW(publisher->publishInteractionClass(interactionClass));

  auto const sourceRegion = publisher->createRegion(DimensionHandleSet{serverId});
  auto const regionalRegion = regional->createRegion(DimensionHandleSet{serverId});
  auto const mixedRegion = mixed->createRegion(DimensionHandleSet{serverId});
  REQUIRE_NOTHROW(publisher->setRangeBounds(
      sourceRegion,
      serverId,
      RangeBounds(0UL, 1UL)));
  REQUIRE_NOTHROW(publisher->commitRegionModifications(RegionHandleSet{sourceRegion}));
  REQUIRE_NOTHROW(regional->setRangeBounds(
      regionalRegion,
      serverId,
      RangeBounds(0UL, 1UL)));
  REQUIRE_NOTHROW(regional->commitRegionModifications(RegionHandleSet{regionalRegion}));
  REQUIRE_NOTHROW(mixed->setRangeBounds(
      mixedRegion,
      serverId,
      RangeBounds(2UL, 3UL)));
  REQUIRE_NOTHROW(mixed->commitRegionModifications(RegionHandleSet{mixedRegion}));

  REQUIRE_NOTHROW(regional->subscribeInteractionClassWithRegions(
      interactionClass,
      RegionHandleSet{regionalRegion}));
  REQUIRE_NOTHROW(mixed->subscribeInteractionClass(interactionClass));
  REQUIRE_NOTHROW(mixed->subscribeInteractionClassWithRegions(
      interactionClass,
      RegionHandleSet{mixedRegion}));

  VariableLengthData const tag;
  // The mixed federate retains an ordinary subscription, but the explicit,
  // disjoint regional declaration is its effective realization for this
  // class. It must not receive an explicit source-region interaction through
  // the ordinary default region.
  REQUIRE_NOTHROW(publisher->sendInteractionWithRegions(
      interactionClass,
      parameters,
      RegionHandleSet{sourceRegion},
      tag));
  static_cast<void>(regional->evokeMultipleCallbacks(0.0, 0.0));
  static_cast<void>(mixed->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(regionalReports.interactionReports.size() == 1);
  REQUIRE(mixedReports.interactionReports.empty());

  // Once its explicit subscription is removed, the retained ordinary
  // subscription again uses the default region and qualifies for any valid
  // explicit source-region interaction.
  REQUIRE_NOTHROW(mixed->unsubscribeInteractionClassWithRegions(
      interactionClass,
      RegionHandleSet{mixedRegion}));
  REQUIRE_NOTHROW(publisher->sendInteractionWithRegions(
      interactionClass,
      parameters,
      RegionHandleSet{sourceRegion},
      tag));
  static_cast<void>(regional->evokeMultipleCallbacks(0.0, 0.0));
  static_cast<void>(mixed->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(regionalReports.interactionReports.size() == 2);
  REQUIRE(mixedReports.interactionReports.size() == 1);

  // Restore the disjoint regional realization. An ordinary Send Interaction
  // uses the invisible RTI-provided default region, which overlaps each
  // committed non-empty subscription region. An enabled convey switch must
  // expose that fact as a supplied, empty RegionHandleSet.
  REQUIRE_NOTHROW(mixed->subscribeInteractionClassWithRegions(
      interactionClass,
      RegionHandleSet{mixedRegion}));
  REQUIRE_NOTHROW(regional->setConveyRegionDesignatorSetsSwitch(true));
  REQUIRE_NOTHROW(mixed->setConveyRegionDesignatorSetsSwitch(true));
  REQUIRE_NOTHROW(publisher->sendInteraction(interactionClass, parameters, tag));
  static_cast<void>(regional->evokeMultipleCallbacks(0.0, 0.0));
  static_cast<void>(mixed->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(regionalReports.interactionReports.size() == 3);
  REQUIRE(mixedReports.interactionReports.size() == 2);
  REQUIRE(regionalReports.interactionReports.back().sentRegionsSupplied);
  REQUIRE(regionalReports.interactionReports.back().sentRegions.empty());
  REQUIRE(mixedReports.interactionReports.back().sentRegionsSupplied);
  REQUIRE(mixedReports.interactionReports.back().sentRegions.empty());

  REQUIRE_NOTHROW(mixed->unsubscribeInteractionClassWithRegions(
      interactionClass,
      RegionHandleSet{mixedRegion}));
  REQUIRE_NOTHROW(mixed->unsubscribeInteractionClass(interactionClass));
  REQUIRE_NOTHROW(regional->unsubscribeInteractionClassWithRegions(
      interactionClass,
      RegionHandleSet{regionalRegion}));
  REQUIRE_NOTHROW(publisher->deleteRegion(sourceRegion));
  REQUIRE_NOTHROW(regional->deleteRegion(regionalRegion));
  REQUIRE_NOTHROW(mixed->deleteRegion(mixedRegion));
  REQUIRE_NOTHROW(mixed->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(regional->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(mixed->disconnect());
  REQUIRE_NOTHROW(regional->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
}
}
