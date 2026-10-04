#include "ieee1516_2025_federation_management_fixture_support.hpp"

namespace {
TEST_CASE(
    "Embedded resign action deletes then divests in one voluntary transition",
    "[integration][development-profile][federation-management][ownership-management]"
    "[resign-action-delete-then-divest]"
    "[multi-federate-callback-ordering]"
    "[object-management][rti.service.resign-federation-execution]"
    "[federate.callback.remove-object-instance]"
    "[federate.callback.request-attribute-ownership-assumption]") {
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
      owner->joinFederationExecution(L"voluntary-delete-then-divest-owner", L"publisher", federationName));
  REQUIRE_NOTHROW(
      peer->joinFederationExecution(L"voluntary-delete-then-divest-peer", L"subscriber", federationName));

  auto const server = owner->getObjectClassHandle(fixture_hla::fom::employee_server);
  auto const efficiency = owner->getAttributeHandle(server, fixture_hla::fixture::efficiency);
  auto const privilegeToDelete = owner->getAttributeHandle(
      server,
      standard_hla::mom::privilege_to_delete_object);
  AttributeHandleSet const efficiencyOnly{efficiency};
  AttributeHandleSet const privilegeOnly{privilegeToDelete};
  REQUIRE_NOTHROW(owner->publishObjectClassAttributes(server, efficiencyOnly));
  REQUIRE_NOTHROW(peer->subscribeObjectClassAttributes(server, efficiencyOnly));
  peerReports.recordReceiveOrderObjectRemovalInCallbackOrder = true;
  peerReports.onRequestAttributeOwnershipAssumption = [&peerReports]() {
    peerReports.callbackOrder.push_back("assumption");
  };
  REQUIRE_NOTHROW(peer->publishObjectClassAttributes(server, efficiencyOnly));

  // The first object keeps its Efficiency ownership but loses the implicit
  // delete privilege when the owner unpublishes that special attribute.
  ObjectInstanceHandle retainedObject;
  REQUIRE_NOTHROW(retainedObject = owner->registerObjectInstance(server));
  auto const retainedObjectName = owner->getObjectInstanceName(retainedObject);
  REQUIRE_FALSE(peer->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(peerReports.objectDiscoveryReports.size() == 1);

  REQUIRE_NOTHROW(owner->unpublishObjectClassAttributes(server, privilegeOnly));
  REQUIRE_NOTHROW(owner->publishObjectClassAttributes(server, privilegeOnly));

  // Republished HLAprivilegeToDeleteObject starts a new implicit epoch for
  // later registrations; it does not restore the retained object's privilege.
  ObjectInstanceHandle deletedObject;
  REQUIRE_NOTHROW(deletedObject = owner->registerObjectInstance(server));
  auto const deletedObjectName = owner->getObjectInstanceName(deletedObject);
  REQUIRE_FALSE(peer->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(peerReports.objectDiscoveryReports.size() == 2);

  REQUIRE_NOTHROW(owner->resignFederationExecution(
      rti1516_2025::DELETE_OBJECTS_THEN_DIVEST));
  peerReports.callbackOrder.clear();
  REQUIRE(peerReports.objectRemovalReports.empty());
  REQUIRE(peerReports.attributeOwnershipAssumptionReports.empty());
  REQUIRE(peerReports.callbackOrder.empty());
  while (peer->evokeCallback(0.0)) {
  }
  REQUIRE(peerReports.objectRemovalReports.size() == 1);
  REQUIRE(peerReports.objectRemovalReports.front().objectInstance == deletedObject);
  REQUIRE(peerReports.attributeOwnershipAssumptionReports.size() == 1);
  auto const& assumption = peerReports.attributeOwnershipAssumptionReports.front();
  REQUIRE(assumption.objectInstance == retainedObject);
  REQUIRE(assumption.attributes == efficiencyOnly);
  // The embedded ambassador drains the ownership-assumption work list before
  // the object-removal work list.  Keep this recipient-local sequence explicit
  // without treating it as a cross-service ordering guarantee from the
  // standard; the resign-action contract covers the state transition itself.
  REQUIRE(peerReports.callbackOrder ==
          std::vector<std::string>{"assumption", "remove"});
  REQUIRE(peer->getObjectInstanceHandle(retainedObjectName) == retainedObject);
  REQUIRE_THROWS_AS(
      peer->getObjectInstanceHandle(deletedObjectName),
      rti1516_2025::ObjectInstanceNotKnown);

  REQUIRE_NOTHROW(peer->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(peer->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(owner->disconnect());
  REQUIRE_NOTHROW(peer->disconnect());
}
} // namespace
