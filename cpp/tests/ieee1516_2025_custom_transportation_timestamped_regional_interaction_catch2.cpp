#include "ieee1516_2025_federation_management_fixture_support.hpp"

namespace {
TEST_CASE(
    "Embedded timestamped regional delivery accepts a declared custom FOM transportation",
    "[integration][development-profile][federation-management]"
    "[fom][transportation-type-lookup][transportation][ddm][time-management][tso]"
    "[timestamped-regional-interaction][interaction-management]"
    "[custom-transportation]"
    "[custom-transportation-timestamped-regional-interaction]"
    "[rti.service.get-transportation-type-handle]"
    "[rti.service.get-interaction-class-handle][rti.service.publish-interaction-class]"
    "[rti.service.subscribe-interaction-class-with-regions]"
    "[rti.service.change-interaction-order-type][rti.service.create-region]"
    "[rti.service.set-range-bounds][rti.service.commit-region-modifications]"
    "[rti.service.set-convey-region-designator-sets-switch]"
    "[rti.service.enable-time-constrained][rti.service.enable-time-regulation]"
    "[rti.service.send-interaction-with-regions][rti.service.time-advance-request]"
    "[rti.service.evoke-callback]"
    "[federate.callback.receive-interaction][federate.callback.time-advance-grant]") {
  ReportingFederateAmbassador producerReports;
  ReportingFederateAmbassador consumerReports;
  auto producer = makeRti();
  auto consumer = makeRti();
  auto const federationName = nextFederationName();
  auto const testData = std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data";
  std::vector<std::wstring> const fomModules{
      (testData / "transportation-regional-reference-consumer-fom.xml").wstring(),
      (testData / "transportation-reference-provider-fom.xml").wstring(),
  };

  REQUIRE_NOTHROW(producer->connect(producerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(consumer->connect(consumerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(producer->createFederationExecution(
      federationName,
      fomModules,
      standard_hla::mom::integer64_time));
  FederateHandle producerHandle;
  REQUIRE_NOTHROW(producerHandle = producer->joinFederationExecution(
      L"timestamped-regional-producer",
      L"producer",
      federationName));
  REQUIRE_NOTHROW(consumer->joinFederationExecution(
      L"timestamped-regional-consumer",
      L"consumer",
      federationName));

  auto const custom = producer->getTransportationTypeHandle(
      fixture_hla::fixture::umbra_transportation_fixture);
  auto const interaction = producer->getInteractionClassHandle(
      fixture_hla::fom::transportation_regional_interaction);
  auto const consumerInteraction = consumer->getInteractionClassHandle(
      fixture_hla::fom::transportation_regional_interaction);
  REQUIRE(custom.isValid());
  REQUIRE(interaction.isValid());
  REQUIRE(consumerInteraction.isValid());
  REQUIRE_NOTHROW(producer->publishInteractionClass(interaction));
  REQUIRE_NOTHROW(producer->changeInteractionOrderType(interaction, TIMESTAMP));

  DimensionHandle const producerX = producer->getDimensionHandle(
      fixture_hla::fixture::umbra_region_x);
  DimensionHandle const producerY = producer->getDimensionHandle(
      fixture_hla::fixture::umbra_region_y);
  DimensionHandle const consumerX = consumer->getDimensionHandle(
      fixture_hla::fixture::umbra_region_x);
  DimensionHandle const consumerY = consumer->getDimensionHandle(
      fixture_hla::fixture::umbra_region_y);
  REQUIRE(producerX.isValid());
  REQUIRE(producerY.isValid());
  REQUIRE(consumerX.isValid());
  REQUIRE(consumerY.isValid());

  RegionHandle const sourceRegion = producer->createRegion(
      DimensionHandleSet{producerX, producerY});
  RegionHandle const receiverRegion = consumer->createRegion(
      DimensionHandleSet{consumerX, consumerY});
  REQUIRE(sourceRegion.isValid());
  REQUIRE(receiverRegion.isValid());
  REQUIRE_NOTHROW(producer->setRangeBounds(
      sourceRegion,
      producerX,
      RangeBounds(0UL, 5UL)));
  REQUIRE_NOTHROW(producer->setRangeBounds(
      sourceRegion,
      producerY,
      RangeBounds(0UL, 5UL)));
  REQUIRE_NOTHROW(consumer->setRangeBounds(
      receiverRegion,
      consumerX,
      RangeBounds(0UL, 5UL)));
  REQUIRE_NOTHROW(consumer->setRangeBounds(
      receiverRegion,
      consumerY,
      RangeBounds(0UL, 5UL)));
  REQUIRE_NOTHROW(producer->commitRegionModifications(
      RegionHandleSet{sourceRegion}));
  REQUIRE_NOTHROW(consumer->commitRegionModifications(
      RegionHandleSet{receiverRegion}));
  REQUIRE_NOTHROW(consumer->subscribeInteractionClassWithRegions(
      consumerInteraction,
      RegionHandleSet{receiverRegion}));
  REQUIRE_NOTHROW(consumer->setConveyRegionDesignatorSetsSwitch(true));

  REQUIRE_NOTHROW(consumer->enableTimeConstrained());
  while (consumer->evokeCallback(0.0)) {
  }
  REQUIRE(consumerReports.timeConstrainedEnabledReports.size() == 1U);
  REQUIRE_NOTHROW(producer->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(1)));
  while (producer->evokeCallback(0.0)) {
  }
  REQUIRE(producerReports.timeRegulationEnabledReports.size() == 1U);

  auto const retraction = producer->sendInteractionWithRegions(
      interaction,
      ParameterHandleValueMap{},
      RegionHandleSet{sourceRegion},
      VariableLengthData(),
      rti1516_2025::HLAinteger64Time(2));
  REQUIRE(retraction.isValid());
  REQUIRE(consumerReports.timestampedInteractionReports.empty());
  REQUIRE_NOTHROW(consumer->timeAdvanceRequest(
      rti1516_2025::HLAinteger64Time(2)));
  REQUIRE_NOTHROW(producer->timeAdvanceRequest(
      rti1516_2025::HLAinteger64Time(2)));
  while (producer->evokeCallback(0.0)) {
  }
  while (consumer->evokeCallback(0.0)) {
  }

  REQUIRE(consumerReports.timestampedInteractionReports.size() == 1U);
  REQUIRE(consumerReports.timeAdvanceGrantReports.size() == 1U);
  REQUIRE(consumerReports.timeAdvanceGrantReports.front().value == L"2");
  REQUIRE(consumerReports.callbackOrder ==
          std::vector<std::string>{"interaction", "grant"});
  auto const& report = consumerReports.timestampedInteractionReports.front();
  REQUIRE(report.interactionClass == consumerInteraction);
  REQUIRE(report.parameterValues.empty());
  REQUIRE(report.producingFederate == producerHandle);
  REQUIRE(report.transportationType == custom);
  REQUIRE(report.timeImplementationName == standard_hla::mom::integer64_time);
  REQUIRE(report.timeValue == L"2");
  REQUIRE(report.sentOrderType == TIMESTAMP);
  REQUIRE(report.receivedOrderType == TIMESTAMP);
  REQUIRE(report.retractionSupplied);
  REQUIRE(report.retractionValid);
  REQUIRE(report.sentRegionsSupplied);
  REQUIRE(report.sentRegions.contains(sourceRegion));

  REQUIRE_NOTHROW(consumer->unsubscribeInteractionClassWithRegions(
      consumerInteraction,
      RegionHandleSet{receiverRegion}));
  REQUIRE_NOTHROW(consumer->deleteRegion(receiverRegion));
  REQUIRE_NOTHROW(producer->deleteRegion(sourceRegion));
  REQUIRE_NOTHROW(consumer->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(producer->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(producer->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(consumer->disconnect());
  REQUIRE_NOTHROW(producer->disconnect());
}
}
