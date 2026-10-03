#include "ieee1516_2025_federation_management_test_support.hpp"
#include "ieee1516_2025_federation_management_fixture_support.hpp"

TEST_CASE(
    "Embedded time-role services keep enable requests callback-gated before TSO support",
    "[integration][development-profile][time-management][time-role][callbacks]"
    "[rti.service.enable-time-regulation][rti.service.disable-time-regulation]"
    "[rti.service.enable-time-constrained][rti.service.disable-time-constrained]"
    "[rti.service.query-lookahead]"
    "[federate.callback.time-regulation-enabled]"
    "[federate.callback.time-constrained-enabled]"
    "[time-role-enable-callback-gating][2025]") {
  ReportingFederateAmbassador evokedReports;
  ReportingFederateAmbassador immediateReports;
  auto evoked = makeRti();
  auto immediate = makeRti();
  auto const evokedFederationName = nextFederationName();
  auto const immediateFederationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();

  rti1516_2025::HLAinteger64Interval queriedLookahead;
  REQUIRE_THROWS_AS(
      evoked->enableTimeRegulation(rti1516_2025::HLAinteger64Interval(2)),
      rti1516_2025::NotConnected);
  REQUIRE_THROWS_AS(evoked->disableTimeRegulation(), rti1516_2025::NotConnected);
  REQUIRE_THROWS_AS(evoked->enableTimeConstrained(), rti1516_2025::NotConnected);
  REQUIRE_THROWS_AS(evoked->disableTimeConstrained(), rti1516_2025::NotConnected);
  REQUIRE_THROWS_AS(evoked->queryLookahead(queriedLookahead), rti1516_2025::NotConnected);

  REQUIRE_NOTHROW(evoked->connect(evokedReports, HLA_EVOKED));
  REQUIRE_THROWS_AS(
      evoked->enableTimeRegulation(rti1516_2025::HLAinteger64Interval(2)),
      rti1516_2025::FederateNotExecutionMember);
  REQUIRE_THROWS_AS(evoked->enableTimeConstrained(), rti1516_2025::FederateNotExecutionMember);
  REQUIRE_THROWS_AS(
      evoked->queryLookahead(queriedLookahead),
      rti1516_2025::FederateNotExecutionMember);

  REQUIRE_NOTHROW(
      evoked->createFederationExecution(
          evokedFederationName,
          fomModule,
          standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(
      evoked->joinFederationExecution(L"evoked-time-role-client", evokedFederationName));
  REQUIRE_THROWS_AS(
      evoked->queryLookahead(queriedLookahead),
      rti1516_2025::TimeRegulationIsNotEnabled);
  REQUIRE_THROWS_AS(
      evoked->enableTimeRegulation(rti1516_2025::HLAfloat64Interval(2.0)),
      rti1516_2025::InvalidLookahead);
  // The official reference interval constructors reject a negative value
  // before a caller can invoke the service; the mismatched reference type
  // above exercises Umbra's service-boundary InvalidLookahead mapping.

  REQUIRE_NOTHROW(evoked->enableTimeRegulation(rti1516_2025::HLAinteger64Interval(2)));
  REQUIRE(evokedReports.timeRegulationEnabledReports.empty());
  REQUIRE_THROWS_AS(
      evoked->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(1)),
      rti1516_2025::RequestForTimeRegulationPending);
  REQUIRE_THROWS_AS(
      evoked->enableTimeRegulation(rti1516_2025::HLAinteger64Interval(3)),
      rti1516_2025::RequestForTimeRegulationPending);
  REQUIRE_FALSE(evoked->evokeCallback(0.0));
  REQUIRE(evokedReports.timeRegulationEnabledReports.size() == 1);
  REQUIRE(evokedReports.timeRegulationEnabledReports.front().implementationName == standard_hla::mom::integer64_time);
  REQUIRE(evokedReports.timeRegulationEnabledReports.front().value == L"0");
  REQUIRE_NOTHROW(evoked->queryLookahead(queriedLookahead));
  REQUIRE(queriedLookahead.getInterval() == 2);
  rti1516_2025::HLAfloat64Interval mismatchedQueryLookahead;
  REQUIRE_THROWS_AS(
      evoked->queryLookahead(mismatchedQueryLookahead),
      rti1516_2025::RTIinternalError);
  REQUIRE_NOTHROW(evoked->disableTimeRegulation());
  REQUIRE_THROWS_AS(
      evoked->queryLookahead(queriedLookahead),
      rti1516_2025::TimeRegulationIsNotEnabled);
  REQUIRE_THROWS_AS(
      evoked->disableTimeRegulation(),
      rti1516_2025::TimeRegulationIsNotEnabled);

  REQUIRE_NOTHROW(evoked->enableTimeConstrained());
  REQUIRE(evokedReports.timeConstrainedEnabledReports.empty());
  REQUIRE_THROWS_AS(
      evoked->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(1)),
      rti1516_2025::RequestForTimeConstrainedPending);
  REQUIRE_THROWS_AS(
      evoked->enableTimeConstrained(),
      rti1516_2025::RequestForTimeConstrainedPending);
  REQUIRE_FALSE(evoked->evokeCallback(0.0));
  REQUIRE(evokedReports.timeConstrainedEnabledReports.size() == 1);
  REQUIRE(evokedReports.timeConstrainedEnabledReports.front().value == L"0");
  REQUIRE_NOTHROW(evoked->disableTimeConstrained());
  REQUIRE_THROWS_AS(
      evoked->disableTimeConstrained(),
      rti1516_2025::TimeConstrainedIsNotEnabled);

  REQUIRE_NOTHROW(immediate->connect(immediateReports, rti1516_2025::HLA_IMMEDIATE));
  REQUIRE_NOTHROW(
      immediate->createFederationExecution(
          immediateFederationName,
          fomModule,
          standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(
      immediate->joinFederationExecution(L"immediate-time-role-client", immediateFederationName));
  REQUIRE_NOTHROW(immediate->enableTimeRegulation(rti1516_2025::HLAinteger64Interval(5)));
  REQUIRE(immediateReports.timeRegulationEnabledReports.size() == 1);
  rti1516_2025::HLAinteger64Interval immediateLookahead;
  REQUIRE_NOTHROW(immediate->queryLookahead(immediateLookahead));
  REQUIRE(immediateLookahead.getInterval() == 5);
  REQUIRE_NOTHROW(immediate->enableTimeConstrained());
  REQUIRE(immediateReports.timeConstrainedEnabledReports.size() == 1);

  REQUIRE_NOTHROW(evoked->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(evoked->destroyFederationExecution(evokedFederationName));
  REQUIRE_NOTHROW(immediate->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(immediate->destroyFederationExecution(immediateFederationName));
  REQUIRE_NOTHROW(evoked->disconnect());
  REQUIRE_NOTHROW(immediate->disconnect());
}
