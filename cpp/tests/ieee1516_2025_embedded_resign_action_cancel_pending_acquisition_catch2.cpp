#include "ieee1516_2025_federation_management_fixture_support.hpp"

namespace {
TEST_CASE(
    "Embedded resign action cancels pending ownership acquisition work",
    "[integration][development-profile][federation-management][ownership-management]"
    "[resign-action-cancel-pending-acquisition]"
    "[rti.service.resign-federation-execution]"
    "[rti.service.attribute-ownership-acquisition]"
    "[federate.callback.request-attribute-ownership-release]") {
  ReportingFederateAmbassador requestingFederateReports;
  ReportingFederateAmbassador remoteOwnerReports;
  auto requestingFederate = makeRti();
  auto remoteOwner = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();

  REQUIRE_NOTHROW(requestingFederate->connect(requestingFederateReports, HLA_EVOKED));
  REQUIRE_NOTHROW(remoteOwner->connect(remoteOwnerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      requestingFederate->createFederationExecution(
          federationName,
          fomModule,
          standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(
      requestingFederate->joinFederationExecution(
          L"canceling-requester",
          L"subscriber",
          federationName));
  REQUIRE_NOTHROW(
      remoteOwner->joinFederationExecution(
          L"canceling-owner",
          L"publisher",
          federationName));

  auto const server = requestingFederate->getObjectClassHandle(
      fixture_hla::fom::employee_server);
  auto const efficiency = requestingFederate->getAttributeHandle(server, fixture_hla::fixture::efficiency);
  AttributeHandleSet const efficiencyOnly{efficiency};
  REQUIRE_NOTHROW(
      requestingFederate->subscribeObjectClassAttributes(server, efficiencyOnly));
  REQUIRE_NOTHROW(remoteOwner->publishObjectClassAttributes(server, efficiencyOnly));

  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = remoteOwner->registerObjectInstance(server));
  REQUIRE_FALSE(requestingFederate->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(requestingFederateReports.objectDiscoveryReports.size() == 1);

  // Publish after discovery so the requester is eligible for the regular
  // acquisition handshake, then leave that handshake pending.
  REQUIRE_NOTHROW(
      requestingFederate->publishObjectClassAttributes(server, efficiencyOnly));
  unsigned char const acquisitionTagBytes[] = {0xD8, 0x25};
  VariableLengthData const acquisitionTag(acquisitionTagBytes, sizeof(acquisitionTagBytes));
  REQUIRE_NOTHROW(requestingFederate->attributeOwnershipAcquisition(
      objectInstance,
      efficiencyOnly,
      acquisitionTag));
  REQUIRE(remoteOwnerReports.attributeOwnershipReleaseRequestReports.empty());

  // Directive 3 cancels only the resigning federate's pending acquisition.
  // The already queued release request must not reach the surviving owner.
  REQUIRE_NOTHROW(requestingFederate->resignFederationExecution(
      rti1516_2025::CANCEL_PENDING_OWNERSHIP_ACQUISITIONS));
  while (remoteOwner->evokeCallback(0.0)) {
  }
  REQUIRE(remoteOwnerReports.attributeOwnershipReleaseRequestReports.empty());

  REQUIRE_NOTHROW(remoteOwner->resignFederationExecution(rti1516_2025::DELETE_OBJECTS));
  REQUIRE_NOTHROW(remoteOwner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(requestingFederate->disconnect());
  REQUIRE_NOTHROW(remoteOwner->disconnect());
}
} // namespace
