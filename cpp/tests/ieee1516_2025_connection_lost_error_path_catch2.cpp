#include "ieee1516_2025_federation_management_fixture_support.hpp"

namespace {

TEST_CASE(
    "Embedded transport loss forces the official connection-lost transition",
    "[integration][development-profile][federation-management][transport]"
    "[connection-lost-error-path]"
    "[rti.service.connection-lost][federate.callback.connection-lost]") {
  FederationEventFederateAmbassador lostReports;
  ReportingFederateAmbassador survivingReports;
  auto lost = makeRti();
  auto surviving = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();

  REQUIRE_NOTHROW(lost->connect(lostReports, HLA_EVOKED));
  REQUIRE_NOTHROW(surviving->connect(survivingReports, HLA_EVOKED));
  REQUIRE_NOTHROW(lost->createFederationExecution(
      federationName,
      fomModule,
      standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(lost->joinFederationExecution(
      L"transport-lost",
      L"observer",
      federationName));
  REQUIRE_NOTHROW(surviving->joinFederationExecution(
      L"transport-survivor",
      L"observer",
      federationName));

  REQUIRE(umbra::detail::failEmbeddedTransportConnectionForTesting(
      *lost,
      L"embedded loopback transport closed"));
  REQUIRE_FALSE(umbra::detail::failEmbeddedTransportConnectionForTesting(
      *lost,
      L"duplicate fault"));

  REQUIRE_FALSE(lost->evokeCallback(0.0));
  REQUIRE(lostReports.faultDescriptions ==
          std::vector<std::wstring>{L"embedded loopback transport closed"});
  REQUIRE(lostReports.resignationDescriptions.empty());
  REQUIRE_THROWS_AS(lost->disconnect(), rti1516_2025::NotConnected);
  REQUIRE_THROWS_AS(
      lost->joinFederationExecution(L"observer", federationName),
      rti1516_2025::NotConnected);

  // The forced membership cleanup is observable from a surviving federate:
  // the lost endpoint no longer appears in the execution member report.
  REQUIRE_NOTHROW(surviving->listFederationExecutionMembers(federationName));
  REQUIRE_FALSE(surviving->evokeCallback(0.0));
  REQUIRE(survivingReports.federationExecutionMemberReports.size() == 1);
  REQUIRE(survivingReports.federationExecutionMemberReports.front().members.size() == 1);
  REQUIRE(
      survivingReports.federationExecutionMemberReports.front().members.front().federateName ==
      L"transport-survivor");

  // The forced transition returns the RTI ambassador to the ordinary
  // disconnected lifecycle, so the same object can establish a fresh
  // official connection after the fault callback has been evoked.
  REQUIRE_NOTHROW(lost->connect(lostReports, HLA_EVOKED));
  REQUIRE_NOTHROW(lost->disconnect());
  REQUIRE_NOTHROW(surviving->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(surviving->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(surviving->disconnect());
}

} // namespace
