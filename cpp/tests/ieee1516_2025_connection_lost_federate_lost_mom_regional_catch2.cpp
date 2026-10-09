#include "ieee1516_2025_federation_management_fixture_support.hpp"

namespace {

TEST_CASE(
    "Embedded transport loss DDM-filters HLAreportFederateLost regional subscriptions",
    "[integration][development-profile][federation-management][transport][mom][ddm]"
    "[rti.service.connection-lost][rti.service.subscribe-interaction-class-with-regions]"
    "[federate.callback.receive-interaction]") {
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

  REQUIRE_NOTHROW(lost->connect(lostReports, HLA_EVOKED));
  REQUIRE_NOTHROW(matching->connect(matchingReports, HLA_EVOKED));
  REQUIRE_NOTHROW(disjoint->connect(disjointReports, HLA_EVOKED));
  REQUIRE_NOTHROW(lost->createFederationExecution(
      federationName,
      fomModule,
      standard_hla::mom::integer64_time));
  FederateHandle lostFederate;
  REQUIRE_NOTHROW(lostFederate = lost->joinFederationExecution(
      L"lost-regional-report-federate",
      L"publisher",
      federationName));
  REQUIRE_NOTHROW(matching->joinFederationExecution(
      L"matching-regional-report-observer",
      L"observer",
      federationName));
  REQUIRE_NOTHROW(disjoint->joinFederationExecution(
      L"disjoint-regional-report-observer",
      L"observer",
      federationName));

  auto const reportClass = matching->getInteractionClassHandle(reportClassName);
  auto const federateDimension = matching->getDimensionHandle(standard_hla::mom::federate);
  REQUIRE(reportClass.isValid());
  REQUIRE(federateDimension.isValid());
  auto const normalizedLost = matching->normalizeFederateHandle(lostFederate);
  REQUIRE(normalizedLost < std::numeric_limits<unsigned long>::max());
  // Normalization does not promise unique or sequential values, so choose a
  // point immediately beside the lost federate's point rather than comparing
  // another federate's opaque coordinate.
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

  REQUIRE(umbra::detail::failEmbeddedTransportConnectionForTesting(
      *lost,
      L"regional loss-report transport fault"));
  REQUIRE_FALSE(matching->evokeCallback(0.0));
  REQUIRE(matchingReports.interactionReports.size() == 1U);
  REQUIRE(matchingReports.interactionReports.front().interactionClass == reportClass);
  REQUIRE_FALSE(matchingReports.interactionReports.front().sentRegionsSupplied);
  REQUIRE_FALSE(disjoint->evokeCallback(0.0));
  REQUIRE(disjointReports.interactionReports.empty());

  REQUIRE_FALSE(lost->evokeCallback(0.0));
  REQUIRE(lostReports.faultDescriptions ==
          std::vector<std::wstring>{L"regional loss-report transport fault"});
  REQUIRE_NOTHROW(lost->connect(lostReports, HLA_EVOKED));
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
