#include "ieee1516_2025_federation_management_test_support.hpp"
#include "ieee1516_2025_federation_management_fixture_support.hpp"

TEST_CASE(
    "Embedded Query GALT and Query LITS observe other regulator time and pending advances",
    "[integration][development-profile][time-management][galt][lits]"
    "[rti.service.query-galt][rti.service.query-lits]"
    "[query-galt-lits-other-regulator-pending-advances][2025]") {
  ReportingFederateAmbassador receiverReports;
  ReportingFederateAmbassador regulatorReports;
  auto receiver = makeRti();
  auto regulator = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  rti1516_2025::HLAinteger64Time galt;
  rti1516_2025::HLAinteger64Time lits;

  REQUIRE_THROWS_AS(receiver->queryGALT(galt), rti1516_2025::NotConnected);
  REQUIRE_THROWS_AS(receiver->queryLITS(lits), rti1516_2025::NotConnected);
  REQUIRE_NOTHROW(receiver->connect(receiverReports, HLA_EVOKED));
  REQUIRE_THROWS_AS(receiver->queryGALT(galt), rti1516_2025::FederateNotExecutionMember);
  REQUIRE_THROWS_AS(receiver->queryLITS(lits), rti1516_2025::FederateNotExecutionMember);

  REQUIRE_NOTHROW(
      receiver->createFederationExecution(federationName, fomModule, standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(receiver->joinFederationExecution(L"receiver", federationName));
  REQUIRE_NOTHROW(regulator->connect(regulatorReports, HLA_EVOKED));
  REQUIRE_NOTHROW(regulator->joinFederationExecution(L"regulator", federationName));

  // A regulator is not active until the Time Regulation Enabled callback; no
  // other regulator means both no-TSO bounds are correctly undefined.
  REQUIRE_FALSE(receiver->queryGALT(galt));
  REQUIRE_FALSE(receiver->queryLITS(lits));
  REQUIRE_NOTHROW(regulator->enableTimeRegulation(rti1516_2025::HLAinteger64Interval(2)));
  REQUIRE_FALSE(receiver->queryGALT(galt));
  REQUIRE_FALSE(regulator->evokeCallback(0.0));

  REQUIRE(receiver->queryGALT(galt));
  REQUIRE(galt.getTime() == 2);
  REQUIRE(receiver->queryLITS(lits));
  REQUIRE(lits.getTime() == 2);
  rti1516_2025::HLAfloat64Time mismatchedOutput;
  REQUIRE_THROWS_AS(receiver->queryGALT(mismatchedOutput), rti1516_2025::RTIinternalError);

  // While the regulator is Time Advancing, its requested time rather than its
  // prior granted time constrains its earliest possible future TSO timestamp.
  REQUIRE_NOTHROW(regulator->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(5)));
  REQUIRE(receiver->queryGALT(galt));
  REQUIRE(galt.getTime() == 7);
  REQUIRE(receiver->queryLITS(lits));
  REQUIRE(lits.getTime() == 7);
  REQUIRE_FALSE(regulator->evokeCallback(0.0));
  REQUIRE(receiver->queryGALT(galt));
  REQUIRE(galt.getTime() == 7);

  REQUIRE_NOTHROW(regulator->disableTimeRegulation());
  REQUIRE_FALSE(receiver->queryGALT(galt));
  REQUIRE_FALSE(receiver->queryLITS(lits));

  REQUIRE_NOTHROW(regulator->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(receiver->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(receiver->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(regulator->disconnect());
  REQUIRE_NOTHROW(receiver->disconnect());
}
