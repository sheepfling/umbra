#include "ieee1516_2025_federation_management_fixture_support.hpp"

namespace {

TEST_CASE(
    "Embedded transport loss DDM-delivers HLAreportFederateLost immediately to matching subscribers",
    "[integration][development-profile][federation-management][transport][mom][ddm]"
    "[callback-immediate][rti.service.connection-lost]"
    "[rti.service.subscribe-interaction-class-with-regions]"
    "[federate.callback.connection-lost][federate.callback.receive-interaction]") {
  ReportingFederateAmbassador lostReports;
  ReportingFederateAmbassador matchingReports;
  ReportingFederateAmbassador disjointReports;
  auto lost = makeRti();
  auto matching = makeRti();
  auto disjoint = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  std::wstring const reportClassName =
      standard_hla::mom::report_federate_lost;

  REQUIRE_NOTHROW(lost->connect(lostReports, rti1516_2025::HLA_IMMEDIATE));
  REQUIRE_NOTHROW(matching->connect(matchingReports, rti1516_2025::HLA_IMMEDIATE));
  REQUIRE_NOTHROW(disjoint->connect(disjointReports, rti1516_2025::HLA_IMMEDIATE));
  REQUIRE_NOTHROW(lost->createFederationExecution(
      federationName,
      fomModule,
      standard_hla::mom::integer64_time));
  FederateHandle lostFederate;
  REQUIRE_NOTHROW(lostFederate = lost->joinFederationExecution(
      L"immediate-lost-regional-report-federate",
      L"publisher",
      federationName));
  REQUIRE_NOTHROW(matching->joinFederationExecution(
      L"immediate-matching-regional-report-observer",
      L"observer",
      federationName));
  REQUIRE_NOTHROW(disjoint->joinFederationExecution(
      L"immediate-disjoint-regional-report-observer",
      L"observer",
      federationName));

  auto const reportClass = matching->getInteractionClassHandle(reportClassName);
  auto const federateDimension = matching->getDimensionHandle(standard_hla::mom::federate);
  REQUIRE(reportClass.isValid());
  REQUIRE(federateDimension.isValid());
  auto const normalizedLost = matching->normalizeFederateHandle(lostFederate);
  REQUIRE(normalizedLost < std::numeric_limits<unsigned long>::max());
  auto const disjointPoint = normalizedLost == 0UL ? 1UL : normalizedLost - 1UL;
  auto const matchingRegion = matching->createRegion(DimensionHandleSet{federateDimension});
  auto const disjointRegion = disjoint->createRegion(DimensionHandleSet{federateDimension});
  REQUIRE_NOTHROW(matching->setRangeBounds(
      matchingRegion,
      federateDimension,
      RangeBounds(normalizedLost, normalizedLost + 1UL)));
  REQUIRE_NOTHROW(disjoint->setRangeBounds(
      disjointRegion,
      federateDimension,
      RangeBounds(disjointPoint, disjointPoint + 1UL)));
  REQUIRE_NOTHROW(matching->commitRegionModifications(RegionHandleSet{matchingRegion}));
  REQUIRE_NOTHROW(disjoint->commitRegionModifications(RegionHandleSet{disjointRegion}));
  REQUIRE_NOTHROW(matching->subscribeInteractionClassWithRegions(
      reportClass,
      RegionHandleSet{matchingRegion}));
  REQUIRE_NOTHROW(disjoint->subscribeInteractionClassWithRegions(
      reportClass,
      RegionHandleSet{disjointRegion}));
  REQUIRE_NOTHROW(lost->enableTimeRegulation(rti1516_2025::HLAinteger64Interval(1)));
  REQUIRE(lostReports.timeRegulationEnabledReports.size() == 1U);

  std::wstring const faultDescription = L"immediate regional loss-report transport fault";
  REQUIRE(umbra::detail::failEmbeddedTransportConnectionForTesting(
      *lost,
      faultDescription));

  // The route remains DDM-filtered in the immediate model. Neither survivor
  // is evoked: the matching point-range has already received the report while
  // the adjacent non-overlap remains silent.
  REQUIRE(lostReports.faultDescriptions == std::vector<std::wstring>{faultDescription});
  REQUIRE(matchingReports.interactionReports.size() == 1U);
  REQUIRE(matchingReports.interactionReports.front().interactionClass == reportClass);
  REQUIRE(matchingReports.interactionReports.front().parameterValues.size() == 4U);
  REQUIRE_FALSE(matchingReports.interactionReports.front().producingFederate.isValid());
  REQUIRE_FALSE(matchingReports.interactionReports.front().sentRegionsSupplied);
  REQUIRE(disjointReports.interactionReports.empty());

  REQUIRE_NOTHROW(lost->connect(lostReports, rti1516_2025::HLA_IMMEDIATE));
  REQUIRE_NOTHROW(lost->disconnect());
  REQUIRE_NOTHROW(matching->unsubscribeInteractionClassWithRegions(
      reportClass,
      RegionHandleSet{matchingRegion}));
  REQUIRE_NOTHROW(disjoint->unsubscribeInteractionClassWithRegions(
      reportClass,
      RegionHandleSet{disjointRegion}));
  REQUIRE_NOTHROW(matching->deleteRegion(matchingRegion));
  REQUIRE_NOTHROW(disjoint->deleteRegion(disjointRegion));
  REQUIRE_NOTHROW(matching->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(disjoint->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(matching->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(matching->disconnect());
  REQUIRE_NOTHROW(disjoint->disconnect());
}

} // namespace
