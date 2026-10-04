#include "ieee1516_2025_federation_management_fixture_support.hpp"

namespace {
TEST_CASE(
    "Embedded resign action rejects pending ownership acquisition work",
    "[integration][development-profile][federation-management][ownership-management]"
    "[resign-action-pending-acquisition]"
    "[rti.service.resign-federation-execution]"
    "[rti.service.attribute-ownership-acquisition]"
    "[federate.callback.request-attribute-ownership-release]") {
  TestFederateAmbassador ownerFederate;
  ReportingFederateAmbassador requesterReports;
  auto owner = makeRti();
  auto requester = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();

  REQUIRE_NOTHROW(owner->connect(ownerFederate, HLA_EVOKED));
  REQUIRE_NOTHROW(requester->connect(requesterReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      owner->createFederationExecution(federationName, fomModule, standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(
      owner->joinFederationExecution(L"pending-owner", L"subscriber", federationName));
  REQUIRE_NOTHROW(
      requester->joinFederationExecution(L"pending-requester", L"publisher", federationName));

  auto const server = owner->getObjectClassHandle(fixture_hla::fom::employee_server);
  auto const efficiency = owner->getAttributeHandle(server, fixture_hla::fixture::efficiency);
  AttributeHandleSet const efficiencyOnly{efficiency};
  REQUIRE_NOTHROW(owner->subscribeObjectClassAttributes(server, efficiencyOnly));
  REQUIRE_NOTHROW(requester->publishObjectClassAttributes(server, efficiencyOnly));
  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = requester->registerObjectInstance(server));
  REQUIRE_FALSE(owner->evokeMultipleCallbacks(0.0, 0.0));

  unsigned char const acquisitionTagBytes[] = {0xD2, 0x25};
  VariableLengthData const acquisitionTag(acquisitionTagBytes, sizeof(acquisitionTagBytes));
  REQUIRE_NOTHROW(owner->publishObjectClassAttributes(server, efficiencyOnly));
  REQUIRE_NOTHROW(owner->attributeOwnershipAcquisition(
      objectInstance,
      efficiencyOnly,
      acquisitionTag));
  REQUIRE_THROWS_AS(
      owner->resignFederationExecution(rti1516_2025::UNCONDITIONALLY_DIVEST_ATTRIBUTES),
      rti1516_2025::OwnershipAcquisitionPending);

  // Directive 5 explicitly resolves the pending request before the owner
  // leaves; the remaining producer can then delete its own object.
  REQUIRE_NOTHROW(
      owner->resignFederationExecution(rti1516_2025::CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(requester->resignFederationExecution(rti1516_2025::DELETE_OBJECTS));
  REQUIRE_NOTHROW(requester->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(owner->disconnect());
  REQUIRE_NOTHROW(requester->disconnect());
}
} // namespace
