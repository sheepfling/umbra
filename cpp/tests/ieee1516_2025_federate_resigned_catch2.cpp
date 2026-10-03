#include "ieee1516_2025_federation_management_fixture_support.hpp"

namespace {
TEST_CASE(
    "Embedded RTI control forces the official federate-resigned transition",
    "[integration][development-profile][federation-management][transport]"
    "[federate.callback.federate-resigned]") {
  FederationEventFederateAmbassador forcedReports;
  FederationEventFederateAmbassador selfResignedReports;
  auto forced = makeRti();
  auto selfResigned = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();

  REQUIRE_NOTHROW(forced->connect(forcedReports, HLA_EVOKED));
  REQUIRE_NOTHROW(selfResigned->connect(selfResignedReports, HLA_EVOKED));
  REQUIRE_NOTHROW(forced->createFederationExecution(
      federationName,
      fomModule,
      standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(forced->joinFederationExecution(
      L"forced-resignation-target",
      L"observer",
      federationName));
  REQUIRE_NOTHROW(selfResigned->joinFederationExecution(
      L"self-resignation-control",
      L"observer",
      federationName));

  // A federate-initiated resignation must not produce Federate Resigned at
  // that same federate. The RTI-originated control path below is distinct.
  REQUIRE_NOTHROW(selfResigned->resignFederationExecution(NO_ACTION));
  REQUIRE_FALSE(selfResigned->evokeCallback(0.0));
  REQUIRE(selfResignedReports.resignationDescriptions.empty());

  REQUIRE(umbra::detail::forceEmbeddedFederateResignationForTesting(
      *forced,
      L"embedded RTI membership control removed this federate"));
  REQUIRE_FALSE(umbra::detail::forceEmbeddedFederateResignationForTesting(
      *forced,
      L"duplicate RTI membership control"));
  REQUIRE(forcedReports.resignationDescriptions.empty());
  REQUIRE(forcedReports.faultDescriptions.empty());

  REQUIRE_FALSE(forced->evokeCallback(0.0));
  REQUIRE(forcedReports.resignationDescriptions == std::vector<std::wstring>{
      L"embedded RTI membership control removed this federate"});
  REQUIRE(forcedReports.faultDescriptions.empty());

  // Federate Resigned leaves the connection alive but ends membership, so the
  // same RTI ambassador can immediately join the execution again.
  REQUIRE_NOTHROW(forced->joinFederationExecution(
      L"forced-resignation-rejoined",
      L"observer",
      federationName));
  REQUIRE_NOTHROW(forced->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(forced->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(forced->disconnect());
  REQUIRE_NOTHROW(selfResigned->disconnect());
}
}  // namespace
