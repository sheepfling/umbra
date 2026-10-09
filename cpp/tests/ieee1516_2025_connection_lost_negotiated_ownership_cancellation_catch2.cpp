#include "ieee1516_2025_federation_management_fixture_support.hpp"

namespace {

TEST_CASE(
    "Embedded transport loss cancels a pending negotiated ownership transfer",
    "[integration][development-profile][federation-management][transport]"
    "[ownership-management][connection-lost-negotiated-cancellation]"
    "[rti.service.connection-lost]"
    "[rti.service.get-automatic-resign-directive]"
    "[rti.service.set-automatic-resign-directive]"
    "[rti.service.attribute-ownership-acquisition]"
    "[rti.service.negotiated-attribute-ownership-divestiture]"
    "[federate.callback.connection-lost]"
    "[federate.callback.request-divestiture-confirmation]") {
  ReportingFederateAmbassador ownerReports;
  ReportingFederateAmbassador lostReports;
  auto owner = makeRti();
  auto lost = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();

  REQUIRE_NOTHROW(owner->connect(ownerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(lost->connect(lostReports, HLA_EVOKED));
  REQUIRE_NOTHROW(owner->createFederationExecution(
      federationName,
      fomModule,
      standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(owner->joinFederationExecution(
      L"automatic-negotiated-owner",
      L"publisher",
      federationName));
  REQUIRE_NOTHROW(lost->joinFederationExecution(
      L"automatic-negotiated-lost",
      L"candidate",
      federationName));

  auto const server = owner->getObjectClassHandle(fixture_hla::fom::employee_server);
  auto const efficiency = owner->getAttributeHandle(server, fixture_hla::fixture::efficiency);
  AttributeHandleSet const efficiencyOnly{efficiency};
  REQUIRE_NOTHROW(owner->publishObjectClassAttributes(server, efficiencyOnly));
  REQUIRE_NOTHROW(lost->subscribeObjectClassAttributes(server, efficiencyOnly));

  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = owner->registerObjectInstance(server));
  REQUIRE_FALSE(lost->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(lostReports.objectDiscoveryReports.size() == 1);

  // The lost member is an eligible regular acquirer. The owner then starts
  // negotiated divestiture before its release callback is consumed, selecting
  // the same pending acquisition for Request Divestiture Confirmation.
  REQUIRE_NOTHROW(lost->publishObjectClassAttributes(server, efficiencyOnly));
  unsigned char const acquisitionTagBytes[] = {0xE6, 0x25};
  unsigned char const divestitureTagBytes[] = {0xE7, 0x25};
  VariableLengthData const acquisitionTag(acquisitionTagBytes, sizeof(acquisitionTagBytes));
  VariableLengthData const divestitureTag(divestitureTagBytes, sizeof(divestitureTagBytes));
  REQUIRE_NOTHROW(lost->attributeOwnershipAcquisition(
      objectInstance,
      efficiencyOnly,
      acquisitionTag));
  REQUIRE(ownerReports.attributeOwnershipReleaseRequestReports.empty());
  REQUIRE_NOTHROW(owner->negotiatedAttributeOwnershipDivestiture(
      objectInstance,
      efficiencyOnly,
      divestitureTag));
  REQUIRE(ownerReports.divestitureConfirmationReports.empty());
  REQUIRE(owner->isAttributeOwnedByFederate(objectInstance, efficiency));
  REQUIRE_FALSE(lost->isAttributeOwnedByFederate(objectInstance, efficiency));

  REQUIRE_NOTHROW(lost->setAutomaticResignDirective(
      rti1516_2025::CANCEL_PENDING_OWNERSHIP_ACQUISITIONS));
  REQUIRE(
      lost->getAutomaticResignDirective() ==
      rti1516_2025::CANCEL_PENDING_OWNERSHIP_ACQUISITIONS);
  REQUIRE(umbra::detail::failEmbeddedTransportConnectionForTesting(
      *lost,
      L"automatic negotiated-cancellation transport fault"));

  REQUIRE_FALSE(lost->evokeCallback(0.0));
  REQUIRE(lostReports.faultDescriptions == std::vector<std::wstring>{
      L"automatic negotiated-cancellation transport fault"});
  // Both owner-side callbacks were queued before the fault. Forced directive
  // three must remove their pending state before callback delivery, leaving
  // ownership with the surviving owner and no stale confirmation path.
  while (owner->evokeCallback(0.0)) {
  }
  REQUIRE(ownerReports.attributeOwnershipReleaseRequestReports.empty());
  REQUIRE(ownerReports.divestitureConfirmationReports.empty());
  REQUIRE(owner->isAttributeOwnedByFederate(objectInstance, efficiency));

  REQUIRE_NOTHROW(owner->resignFederationExecution(rti1516_2025::DELETE_OBJECTS));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(lost->connect(lostReports, HLA_EVOKED));
  REQUIRE_NOTHROW(lost->disconnect());
  REQUIRE_NOTHROW(owner->disconnect());
}

}  // namespace
