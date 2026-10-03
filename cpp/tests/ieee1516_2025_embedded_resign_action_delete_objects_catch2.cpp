#include "ieee1516_2025_federation_management_fixture_support.hpp"

namespace {
TEST_CASE(
    "Embedded resign action deletes delete-privileged objects and reports removal",
    "[integration][development-profile][federation-management][object-management]"
    "[rti.service.resign-federation-execution]"
    "[federate.callback.remove-object-instance]"
    "[embedded-resign-action-delete-objects]") {
  TestFederateAmbassador ownerFederate;
  ReportingFederateAmbassador peerReports;
  auto owner = makeRti();
  auto peer = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();

  REQUIRE_NOTHROW(owner->connect(ownerFederate, HLA_EVOKED));
  REQUIRE_NOTHROW(peer->connect(peerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      owner->createFederationExecution(federationName, fomModule, standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(
      owner->joinFederationExecution(L"deleting-owner", L"publisher", federationName));
  REQUIRE_NOTHROW(
      peer->joinFederationExecution(L"deleting-peer", L"subscriber", federationName));

  auto const server = owner->getObjectClassHandle(fixture_hla::fom::employee_server);
  auto const efficiency = owner->getAttributeHandle(server, fixture_hla::fixture::efficiency);
  AttributeHandleSet const efficiencyOnly{efficiency};
  REQUIRE_NOTHROW(owner->publishObjectClassAttributes(server, efficiencyOnly));
  REQUIRE_NOTHROW(peer->subscribeObjectClassAttributes(server, efficiencyOnly));

  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = owner->registerObjectInstance(server));
  auto const objectInstanceName = owner->getObjectInstanceName(objectInstance);
  REQUIRE_FALSE(peer->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(peerReports.objectDiscoveryReports.size() == 1);
  REQUIRE_THROWS_AS(
      owner->resignFederationExecution(NO_ACTION),
      rti1516_2025::FederateOwnsAttributes);

  REQUIRE_NOTHROW(owner->resignFederationExecution(rti1516_2025::DELETE_OBJECTS));
  REQUIRE(peerReports.objectRemovalReports.empty());
  REQUIRE_FALSE(peer->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(peerReports.objectRemovalReports.size() == 1);
  REQUIRE(peerReports.objectRemovalReports.front().objectInstance == objectInstance);
  REQUIRE_THROWS_AS(
      peer->getObjectInstanceHandle(objectInstanceName),
      rti1516_2025::ObjectInstanceNotKnown);

  REQUIRE_NOTHROW(peer->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(peer->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(owner->disconnect());
  REQUIRE_NOTHROW(peer->disconnect());
}
}  // namespace
