#include "ieee1516_2025_federation_management_fixture_support.hpp"

namespace {
TEST_CASE(
    "Embedded assumption search continues after a later eligible publication",
    "[integration][development-profile][ownership-management][federation-management]"
    "[multi-federate-callback-ordering]"
    "[rti.service.resign-federation-execution]"
    "[federate.callback.request-attribute-ownership-assumption]"
    "[embedded-assumption-search-publication-continuation]") {
  TestFederateAmbassador ownerFederate;
  ReportingFederateAmbassador candidateReports;
  auto owner = makeRti();
  auto candidate = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();

  REQUIRE_NOTHROW(owner->connect(ownerFederate, HLA_EVOKED));
  REQUIRE_NOTHROW(candidate->connect(candidateReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      owner->createFederationExecution(federationName, fomModule, standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(owner->joinFederationExecution(
      L"assumption-owner", L"publisher", federationName));
  REQUIRE_NOTHROW(candidate->joinFederationExecution(
      L"assumption-candidate", L"candidate", federationName));

  auto const server = owner->getObjectClassHandle(fixture_hla::fom::employee_server);
  auto const efficiency = owner->getAttributeHandle(server, fixture_hla::fixture::efficiency);
  auto const privilegeToDelete = owner->getAttributeHandle(
      server,
      standard_hla::mom::privilege_to_delete_object);
  AttributeHandleSet const efficiencyOnly{efficiency};
  AttributeHandleSet const expectedAssumption{efficiency, privilegeToDelete};
  REQUIRE_NOTHROW(owner->publishObjectClassAttributes(server, efficiencyOnly));
  REQUIRE_NOTHROW(candidate->subscribeObjectClassAttributes(server, efficiencyOnly));

  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = owner->registerObjectInstance(server));
  REQUIRE_FALSE(candidate->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(candidateReports.objectDiscoveryReports.size() == 1);

  REQUIRE_NOTHROW(
      owner->resignFederationExecution(rti1516_2025::UNCONDITIONALLY_DIVEST_ATTRIBUTES));
  REQUIRE_FALSE(candidate->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(candidateReports.attributeOwnershipAssumptionReports.empty());

  // The candidate was known but not yet publishing when the owner resigned.
  // Publishing later makes it eligible and must continue the prior search.
  REQUIRE_NOTHROW(candidate->publishObjectClassAttributes(server, efficiencyOnly));
  REQUIRE_FALSE(candidate->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(candidateReports.attributeOwnershipAssumptionReports.size() == 1);
  REQUIRE(candidateReports.attributeOwnershipAssumptionReports.front().objectInstance ==
          objectInstance);
  REQUIRE(candidateReports.attributeOwnershipAssumptionReports.front().attributes ==
          expectedAssumption);

  REQUIRE_NOTHROW(candidate->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(candidate->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(owner->disconnect());
  REQUIRE_NOTHROW(candidate->disconnect());
}
}  // namespace
