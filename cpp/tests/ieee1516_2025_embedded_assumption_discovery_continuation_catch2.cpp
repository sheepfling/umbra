#include "ieee1516_2025_federation_management_fixture_support.hpp"

namespace {
TEST_CASE(
    "Embedded assumption search continues after a later join and discovery",
    "[integration][development-profile][ownership-management][federation-management]"
    "[multi-federate-callback-ordering]"
    "[rti.service.resign-federation-execution]"
    "[federate.callback.discover-object-instance]"
    "[federate.callback.request-attribute-ownership-assumption]"
    "[resign-action-assumption-discovery-continuation]") {
  TestFederateAmbassador ownerFederate;
  TestFederateAmbassador observerFederate;
  ReportingFederateAmbassador lateReports;
  auto owner = makeRti();
  auto observer = makeRti();
  auto late = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();

  REQUIRE_NOTHROW(owner->connect(ownerFederate, HLA_EVOKED));
  REQUIRE_NOTHROW(observer->connect(observerFederate, HLA_EVOKED));
  REQUIRE_NOTHROW(late->connect(lateReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      owner->createFederationExecution(federationName, fomModule, standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(owner->joinFederationExecution(
      L"search-owner", L"publisher", federationName));
  REQUIRE_NOTHROW(observer->joinFederationExecution(
      L"search-observer", L"observer", federationName));

  auto const server = owner->getObjectClassHandle(fixture_hla::fom::employee_server);
  auto const efficiency = owner->getAttributeHandle(server, fixture_hla::fixture::efficiency);
  auto const privilegeToDelete = owner->getAttributeHandle(
      server,
      standard_hla::mom::privilege_to_delete_object);
  AttributeHandleSet const efficiencyOnly{efficiency};
  AttributeHandleSet const expectedAssumption{efficiency, privilegeToDelete};
  REQUIRE_NOTHROW(owner->publishObjectClassAttributes(server, efficiencyOnly));
  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = owner->registerObjectInstance(server));

  // Keep the execution alive with an unrelated member while the owner
  // resigns. The object remains, but no assumption recipient is yet known.
  REQUIRE_NOTHROW(
      owner->resignFederationExecution(rti1516_2025::UNCONDITIONALLY_DIVEST_ATTRIBUTES));

  REQUIRE_NOTHROW(late->joinFederationExecution(
      L"search-late", L"candidate", federationName));
  REQUIRE_NOTHROW(late->subscribeObjectClassAttributes(server, efficiencyOnly));
  REQUIRE_FALSE(late->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(lateReports.objectDiscoveryReports.size() == 1);
  REQUIRE(lateReports.objectDiscoveryReports.front().objectInstance == objectInstance);
  REQUIRE(lateReports.attributeOwnershipAssumptionReports.empty());

  REQUIRE_NOTHROW(late->publishObjectClassAttributes(server, efficiencyOnly));
  REQUIRE_FALSE(late->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(lateReports.attributeOwnershipAssumptionReports.size() == 1);
  REQUIRE(lateReports.attributeOwnershipAssumptionReports.front().objectInstance ==
          objectInstance);
  REQUIRE(lateReports.attributeOwnershipAssumptionReports.front().attributes ==
          expectedAssumption);

  REQUIRE_NOTHROW(late->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(observer->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(observer->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(owner->disconnect());
  REQUIRE_NOTHROW(observer->disconnect());
  REQUIRE_NOTHROW(late->disconnect());
}

} // namespace
