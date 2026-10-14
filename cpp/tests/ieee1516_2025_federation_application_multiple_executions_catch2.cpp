#include "ieee1516_2025_federation_management_fixture_support.hpp"

namespace {
TEST_CASE(
    "One application may join multiple federates across multiple federation executions",
    "[integration][development-profile][federation-management][participation][2025]"
    "[hla-federation-multi-federate-multi-execution]"
    "[rti.service.connect][rti.service.create-federation-execution]"
    "[rti.service.join-federation-execution][rti.service.resign-federation-execution]"
    "[rti.service.destroy-federation-execution][rti.service.disconnect]") {
  ReportingFederateAmbassador creatorReports;
  ReportingFederateAmbassador firstExecutionOneReports;
  ReportingFederateAmbassador firstExecutionTwoReports;
  ReportingFederateAmbassador secondExecutionOneReports;
  ReportingFederateAmbassador secondExecutionTwoReports;
  auto creator = makeRti();
  auto firstExecutionOne = makeRti();
  auto firstExecutionTwo = makeRti();
  auto secondExecutionOne = makeRti();
  auto secondExecutionTwo = makeRti();

  auto const firstFederationName = nextFederationName();
  auto const secondFederationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();

  REQUIRE_NOTHROW(creator->connect(creatorReports, HLA_EVOKED));
  REQUIRE_NOTHROW(firstExecutionOne->connect(firstExecutionOneReports, HLA_EVOKED));
  REQUIRE_NOTHROW(firstExecutionTwo->connect(firstExecutionTwoReports, HLA_EVOKED));
  REQUIRE_NOTHROW(secondExecutionOne->connect(secondExecutionOneReports, HLA_EVOKED));
  REQUIRE_NOTHROW(secondExecutionTwo->connect(secondExecutionTwoReports, HLA_EVOKED));
  REQUIRE_NOTHROW(creator->createFederationExecution(
      firstFederationName, fomModule, standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(creator->createFederationExecution(
      secondFederationName, fomModule, standard_hla::mom::integer64_time));

  auto const firstExecutionOneHandle = firstExecutionOne->joinFederationExecution(
      L"participation-first-one", L"participant", firstFederationName);
  auto const secondExecutionOneHandle = secondExecutionOne->joinFederationExecution(
      L"participation-second-one", L"participant", secondFederationName);
  auto const firstExecutionTwoHandle = firstExecutionTwo->joinFederationExecution(
      L"participation-first-two", L"participant", firstFederationName);
  auto const secondExecutionTwoHandle = secondExecutionTwo->joinFederationExecution(
      L"participation-second-two", L"participant", secondFederationName);
  REQUIRE(firstExecutionOneHandle.isValid());
  REQUIRE(secondExecutionOneHandle.isValid());
  REQUIRE(firstExecutionTwoHandle.isValid());
  REQUIRE(secondExecutionTwoHandle.isValid());

  // Leave and rejoin in an order different from the joins while the remaining
  // federates continue participating in both independent executions.
  REQUIRE_NOTHROW(firstExecutionTwo->resignFederationExecution(NO_ACTION));
  auto const rejoinedHandle = firstExecutionTwo->joinFederationExecution(
      L"participation-first-two-rejoined", L"participant", secondFederationName);
  REQUIRE(rejoinedHandle.isValid());
  REQUIRE_NOTHROW(secondExecutionOne->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(firstExecutionOne->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(secondExecutionTwo->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(firstExecutionTwo->resignFederationExecution(NO_ACTION));

  REQUIRE_NOTHROW(creator->destroyFederationExecution(firstFederationName));
  REQUIRE_NOTHROW(creator->destroyFederationExecution(secondFederationName));
  REQUIRE_NOTHROW(creator->disconnect());
  REQUIRE_NOTHROW(firstExecutionOne->disconnect());
  REQUIRE_NOTHROW(firstExecutionTwo->disconnect());
  REQUIRE_NOTHROW(secondExecutionOne->disconnect());
  REQUIRE_NOTHROW(secondExecutionTwo->disconnect());
}
}
