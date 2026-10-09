#include "ieee1516_2025_federation_management_fixture_support.hpp"

namespace {

TEST_CASE(
    "Embedded transport loss cancels the lost federate's pending ownership acquisition",
    "[integration][development-profile][federation-management][transport]"
    "[ownership-management][rti.service.connection-lost]"
    "[rti.service.get-automatic-resign-directive]"
    "[rti.service.set-automatic-resign-directive]"
    "[rti.service.attribute-ownership-acquisition]"
    "[federate.callback.connection-lost]"
    "[federate.callback.request-attribute-ownership-assumption]") {
  ReportingFederateAmbassador ownerReports;
  ReportingFederateAmbassador lostReports;
  ReportingFederateAmbassador candidateReports;
  auto owner = makeRti();
  auto lost = makeRti();
  auto candidate = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();

  REQUIRE_NOTHROW(owner->connect(ownerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(lost->connect(lostReports, HLA_EVOKED));
  REQUIRE_NOTHROW(candidate->connect(candidateReports, HLA_EVOKED));
  REQUIRE_NOTHROW(owner->createFederationExecution(
      federationName,
      fomModule,
      standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(owner->joinFederationExecution(
      L"automatic-cancel-owner",
      L"publisher",
      federationName));
  REQUIRE_NOTHROW(lost->joinFederationExecution(
      L"automatic-cancel-lost",
      L"candidate",
      federationName));
  REQUIRE_NOTHROW(candidate->joinFederationExecution(
      L"automatic-cancel-survivor",
      L"candidate",
      federationName));

  auto const server = owner->getObjectClassHandle(fixture_hla::fom::employee_server);
  auto const efficiency = owner->getAttributeHandle(server, fixture_hla::fixture::efficiency);
  AttributeHandleSet const efficiencyOnly{efficiency};
  REQUIRE_NOTHROW(owner->publishObjectClassAttributes(server, efficiencyOnly));
  REQUIRE_NOTHROW(lost->publishObjectClassAttributes(server, efficiencyOnly));
  REQUIRE_NOTHROW(lost->subscribeObjectClassAttributes(server, efficiencyOnly));
  REQUIRE_NOTHROW(candidate->publishObjectClassAttributes(server, efficiencyOnly));
  REQUIRE_NOTHROW(candidate->subscribeObjectClassAttributes(server, efficiencyOnly));

  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = owner->registerObjectInstance(server));
  while (lost->evokeCallback(0.0)) {
  }
  while (candidate->evokeCallback(0.0)) {
  }
  REQUIRE(lostReports.objectDiscoveryReports.size() == 1);
  REQUIRE(candidateReports.objectDiscoveryReports.size() == 1);

  unsigned char const acquisitionTagBytes[] = {0xD6, 0x25};
  VariableLengthData const acquisitionTag(acquisitionTagBytes, sizeof(acquisitionTagBytes));
  REQUIRE_NOTHROW(lost->attributeOwnershipAcquisition(
      objectInstance,
      efficiencyOnly,
      acquisitionTag));
  REQUIRE(ownerReports.attributeOwnershipReleaseRequestReports.empty());
  REQUIRE_NOTHROW(lost->setAutomaticResignDirective(
      rti1516_2025::CANCEL_PENDING_OWNERSHIP_ACQUISITIONS));
  REQUIRE(
      lost->getAutomaticResignDirective() ==
      rti1516_2025::CANCEL_PENDING_OWNERSHIP_ACQUISITIONS);

  REQUIRE(umbra::detail::failEmbeddedTransportConnectionForTesting(
      *lost,
      L"automatic pending-acquisition cancellation transport fault"));
  REQUIRE_FALSE(lost->evokeCallback(0.0));
  REQUIRE(lostReports.faultDescriptions == std::vector<std::wstring>{
      L"automatic pending-acquisition cancellation transport fault"});

  // The old requester is gone before the owner can receive its queued release
  // request. Delivery must recheck registry state and suppress that stale
  // work, then the actual surviving candidate becomes eligible for a later
  // unconditional divestiture offer.
  while (owner->evokeCallback(0.0)) {
  }
  REQUIRE(ownerReports.attributeOwnershipReleaseRequestReports.empty());
  unsigned char const divestitureTagBytes[] = {0xD7, 0x25};
  VariableLengthData const divestitureTag(divestitureTagBytes, sizeof(divestitureTagBytes));
  REQUIRE_NOTHROW(owner->unconditionalAttributeOwnershipDivestiture(
      objectInstance,
      efficiencyOnly,
      divestitureTag));
  while (candidate->evokeCallback(0.0)) {
  }
  REQUIRE(candidateReports.attributeOwnershipAssumptionReports.size() == 1);
  auto const& assumption = candidateReports.attributeOwnershipAssumptionReports.front();
  REQUIRE(assumption.objectInstance == objectInstance);
  REQUIRE(assumption.attributes == efficiencyOnly);
  REQUIRE_FALSE(candidate->isAttributeOwnedByFederate(objectInstance, efficiency));

  REQUIRE_NOTHROW(candidate->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->resignFederationExecution(rti1516_2025::DELETE_OBJECTS));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(lost->connect(lostReports, HLA_EVOKED));
  REQUIRE_NOTHROW(lost->disconnect());
  REQUIRE_NOTHROW(candidate->disconnect());
  REQUIRE_NOTHROW(owner->disconnect());
}

} // namespace
