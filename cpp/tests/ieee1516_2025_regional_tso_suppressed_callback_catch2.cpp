#include "ieee1516_2025_federation_management_test_support.hpp"
#include "ieee1516_2025_federation_management_fixture_support.hpp"

TEST_CASE(
    "Embedded suppressed timestamped regional interaction callback does not request retraction",
    "[integration][development-profile][interaction-management][ddm][time-management][tso][retract]"
    "[rti.service.send-interaction-with-regions][rti.service.set-range-bounds]"
    "[rti.service.commit-region-modifications][rti.service.retract]"
    "[federate.callback.receive-interaction][federate.callback.request-retraction]"
    "[timestamped-regional-interaction-suppressed-callback-no-retraction][2025]") {
  ReportingFederateAmbassador publisherReports;
  ReportingFederateAmbassador receiverReports;
  auto publisher = makeRti();
  auto receiver = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  unsigned char const parameterBytes[] = {0x53, 0x55, 0x50, 0x52, 0x45, 0x47};
  ParameterHandleValueMap parameterValues;

  REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED));
  REQUIRE_NOTHROW(receiver->connect(receiverReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      publisher->createFederationExecution(federationName, fomModule, standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(publisher->joinFederationExecution(
      L"suppressed-regional-interaction-retraction-publisher",
      L"publisher",
      federationName));
  REQUIRE_NOTHROW(receiver->joinFederationExecution(
      L"suppressed-regional-interaction-retraction-receiver",
      L"subscriber",
      federationName));
  suppressDeclarationRelevanceAdvisories(*publisher);

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
  REQUIRE_NOTHROW(publisher->changeInteractionOrderType(interactionClass, TIMESTAMP));

  auto const publisherRegion = publisher->createRegion(DimensionHandleSet{serverId});
  auto const receiverRegion = receiver->createRegion(DimensionHandleSet{serverId});
  REQUIRE_NOTHROW(publisher->setRangeBounds(
      publisherRegion,
      serverId,
      RangeBounds(0UL, 10UL)));
  REQUIRE_NOTHROW(receiver->setRangeBounds(
      receiverRegion,
      serverId,
      RangeBounds(5UL, 15UL)));
  REQUIRE_NOTHROW(publisher->commitRegionModifications(RegionHandleSet{publisherRegion}));
  REQUIRE_NOTHROW(receiver->commitRegionModifications(RegionHandleSet{receiverRegion}));
  REQUIRE_NOTHROW(receiver->subscribeInteractionClassWithRegions(
      interactionClass,
      RegionHandleSet{receiverRegion}));
  while (receiver->evokeCallback(0.0)) {
  }
  REQUIRE_NOTHROW(publisher->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(1)));
  REQUIRE_FALSE(publisher->evokeCallback(0.0));

  auto const retraction = publisher->sendInteractionWithRegions(
      interactionClass,
      parameterValues,
      RegionHandleSet{publisherRegion},
      VariableLengthData(),
      rti1516_2025::HLAinteger64Time(2));
  REQUIRE(retraction.isValid());

  // The interaction was overlap-qualified when accepted. Move the recipient's
  // committed region away before its queued callback begins; current DDM
  // projection suppresses it, and the recipient ledger must become terminal.
  REQUIRE_NOTHROW(receiver->setRangeBounds(
      receiverRegion,
      serverId,
      RangeBounds(15UL, 20UL)));
  REQUIRE_NOTHROW(receiver->commitRegionModifications(RegionHandleSet{receiverRegion}));
  while (receiver->evokeCallback(0.0)) {
  }
  REQUIRE(receiverReports.timestampedInteractionReports.empty());
  REQUIRE_NOTHROW(publisher->retract(retraction));
  while (receiver->evokeCallback(0.0)) {
  }
  REQUIRE(receiverReports.requestRetractionReports.empty());

  REQUIRE_NOTHROW(receiver->unsubscribeInteractionClassWithRegions(
      interactionClass,
      RegionHandleSet{receiverRegion}));
  REQUIRE_NOTHROW(receiver->deleteRegion(receiverRegion));
  REQUIRE_NOTHROW(publisher->deleteRegion(publisherRegion));
  REQUIRE_NOTHROW(receiver->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(receiver->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
}
