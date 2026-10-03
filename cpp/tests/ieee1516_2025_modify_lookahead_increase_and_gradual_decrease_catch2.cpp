#include "ieee1516_2025_federation_management_test_support.hpp"
#include "ieee1516_2025_federation_management_fixture_support.hpp"

TEST_CASE(
    "Embedded Modify Lookahead applies increases immediately and decreases gradually",
    "[integration][development-profile][time-management][lookahead][time-role][modify-lookahead]"
    "[rti.service.modify-lookahead][rti.service.query-lookahead]"
    "[rti.service.time-advance-request][federate.callback.time-advance-grant]"
    "[modify-lookahead-increase-and-gradual-decrease][2025]") {
  ReportingFederateAmbassador reports;
  auto rti = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  rti1516_2025::HLAinteger64Interval lookahead;

  REQUIRE_THROWS_AS(
      rti->modifyLookahead(rti1516_2025::HLAinteger64Interval(2)),
      rti1516_2025::NotConnected);
  REQUIRE_NOTHROW(rti->connect(reports, HLA_EVOKED));
  REQUIRE_THROWS_AS(
      rti->modifyLookahead(rti1516_2025::HLAinteger64Interval(2)),
      rti1516_2025::FederateNotExecutionMember);
  REQUIRE_NOTHROW(
      rti->createFederationExecution(federationName, fomModule, standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(rti->joinFederationExecution(L"lookahead-client", federationName));
  REQUIRE_THROWS_AS(
      rti->modifyLookahead(rti1516_2025::HLAinteger64Interval(2)),
      rti1516_2025::TimeRegulationIsNotEnabled);

  REQUIRE_NOTHROW(rti->enableTimeRegulation(rti1516_2025::HLAinteger64Interval(2)));
  REQUIRE_FALSE(rti->evokeCallback(0.0));
  REQUIRE_NOTHROW(rti->queryLookahead(lookahead));
  REQUIRE(lookahead.getInterval() == 2);

  REQUIRE_NOTHROW(rti->modifyLookahead(rti1516_2025::HLAinteger64Interval(5)));
  REQUIRE_NOTHROW(rti->queryLookahead(lookahead));
  REQUIRE(lookahead.getInterval() == 5);

  // A decrease is announced immediately but the actual lookahead remains at
  // five until logical time advances.
  REQUIRE_NOTHROW(rti->modifyLookahead(rti1516_2025::HLAinteger64Interval(1)));
  REQUIRE_NOTHROW(rti->queryLookahead(lookahead));
  REQUIRE(lookahead.getInterval() == 5);

  REQUIRE_NOTHROW(rti->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(3)));
  REQUIRE_THROWS_AS(
      rti->modifyLookahead(rti1516_2025::HLAinteger64Interval(2)),
      rti1516_2025::InTimeAdvancingState);
  REQUIRE_FALSE(rti->evokeCallback(0.0));
  REQUIRE_NOTHROW(rti->queryLookahead(lookahead));
  REQUIRE(lookahead.getInterval() == 2);

  REQUIRE_NOTHROW(rti->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(5)));
  REQUIRE_FALSE(rti->evokeCallback(0.0));
  REQUIRE_NOTHROW(rti->queryLookahead(lookahead));
  REQUIRE(lookahead.getInterval() == 1);

  REQUIRE_NOTHROW(rti->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(rti->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(rti->disconnect());
}
