#include "ieee1516_2025_federation_management_fixture_support.hpp"

namespace {
TEST_CASE(
    "Embedded resign action unconditionally divests attributes for the 2025 ownership model",
    "[integration][development-profile][federation-management][ownership-management]"
    "[rti.service.resign-federation-execution]"
    "[federate.callback.request-attribute-ownership-assumption]"
    "[rti.service.attribute-ownership-acquisition]"
    "[embedded-resign-action-unconditional-divestiture]") {
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
      owner->joinFederationExecution(L"resigning-owner", L"publisher", federationName));
  REQUIRE_NOTHROW(
      peer->joinFederationExecution(L"resigning-peer", L"subscriber", federationName));

  auto const server = owner->getObjectClassHandle(fixture_hla::fom::employee_server);
  auto const efficiency = owner->getAttributeHandle(server, fixture_hla::fixture::efficiency);
  auto const privilegeToDelete = owner->getAttributeHandle(
      server,
      standard_hla::mom::privilege_to_delete_object);
  AttributeHandleSet const efficiencyOnly{efficiency};
  AttributeHandleSet const divestedAttributes{efficiency, privilegeToDelete};
  REQUIRE_NOTHROW(owner->publishObjectClassAttributes(server, efficiencyOnly));
  REQUIRE_NOTHROW(peer->subscribeObjectClassAttributes(server, efficiencyOnly));
  REQUIRE_NOTHROW(peer->publishObjectClassAttributes(server, efficiencyOnly));

  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = owner->registerObjectInstance(server));
  REQUIRE_FALSE(peer->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(peerReports.objectDiscoveryReports.size() == 1);

  // Directive 6 cannot strand attributes owned by the resigning federate.
  REQUIRE_THROWS_AS(
      owner->resignFederationExecution(NO_ACTION),
      rti1516_2025::FederateOwnsAttributes);

  REQUIRE_NOTHROW(
      owner->resignFederationExecution(rti1516_2025::UNCONDITIONALLY_DIVEST_ATTRIBUTES));
  REQUIRE_FALSE(peer->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(peerReports.attributeOwnershipAssumptionReports.size() == 1);
  auto const& assumption = peerReports.attributeOwnershipAssumptionReports.front();
  REQUIRE(assumption.objectInstance == objectInstance);
  REQUIRE(assumption.attributes == divestedAttributes);

  unsigned char const acquisitionTagBytes[] = {0xD1, 0x25};
  VariableLengthData const acquisitionTag(acquisitionTagBytes, sizeof(acquisitionTagBytes));
  REQUIRE_NOTHROW(peer->attributeOwnershipAcquisition(
      objectInstance,
      efficiencyOnly,
      acquisitionTag));
  REQUIRE_FALSE(peer->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(peerReports.attributeOwnershipAcquisitionReports.size() == 1);
  REQUIRE(
      peerReports.attributeOwnershipAcquisitionReports.front().kind ==
      ReportingFederateAmbassador::AttributeOwnershipAcquisitionReport::Kind::notification);
  REQUIRE(peer->isAttributeOwnedByFederate(objectInstance, efficiency));

  // The remaining federate can divest the acquired attribute before the
  // federation is destroyed; the object itself was not deleted by directive 1.
  REQUIRE_NOTHROW(
      peer->resignFederationExecution(rti1516_2025::UNCONDITIONALLY_DIVEST_ATTRIBUTES));
  REQUIRE_NOTHROW(peer->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(owner->disconnect());
  REQUIRE_NOTHROW(peer->disconnect());
}
}  // namespace
