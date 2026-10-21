#include "ieee1516_2025_federation_management_fixture_support.hpp"

namespace {
TEST_CASE(
    "Embedded resign action cancels a pending If Available acquisition for the departing federate",
    "[integration][development-profile][federation-management][ownership-management]"
    "[resign-action-cancel-if-available-pending]"
    "[rti.service.resign-federation-execution]"
    "[rti.service.cancel-pending-ownership-acquisitions]"
    "[rti.service.attribute-ownership-acquisition-if-available]"
    "[federate.callback.attribute-ownership-acquisition-notification]"
    "[federate.callback.attribute-ownership-unavailable]") {
  ReportingFederateAmbassador ownerReports;
  ReportingFederateAmbassador requesterReports;
  auto owner = makeRti();
  auto requester = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();

  REQUIRE_NOTHROW(owner->connect(ownerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(requester->connect(requesterReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      owner->createFederationExecution(federationName, fomModule, standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(owner->joinFederationExecution(
      L"if-available-resign-owner", L"publisher", federationName));
  REQUIRE_NOTHROW(requester->joinFederationExecution(
      L"if-available-resign-requester", L"publisher", federationName));

  auto const server = owner->getObjectClassHandle(fixture_hla::fom::employee_server);
  auto const efficiency = owner->getAttributeHandle(server, fixture_hla::fixture::efficiency);
  AttributeHandleSet const efficiencyOnly{efficiency};
  REQUIRE_NOTHROW(owner->publishObjectClassAttributes(server, efficiencyOnly));
  REQUIRE_NOTHROW(requester->subscribeObjectClassAttributes(server, efficiencyOnly));

  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = owner->registerObjectInstance(server));
  REQUIRE_FALSE(requester->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(requesterReports.objectDiscoveryReports.size() == 1);
  REQUIRE_NOTHROW(requester->publishObjectClassAttributes(server, efficiencyOnly));

  // The owner still holds Efficiency, so If Available enters Willing to
  // Acquire and queues a requester-side terminal callback without changing
  // ownership. Directive 3 must remove that private reservation as the
  // requester leaves.
  unsigned char const acquisitionTagBytes[] = {0xE5, 0x25};
  VariableLengthData const acquisitionTag(acquisitionTagBytes, sizeof(acquisitionTagBytes));
  REQUIRE_NOTHROW(requester->attributeOwnershipAcquisitionIfAvailable(
      objectInstance,
      efficiencyOnly,
      acquisitionTag));
  REQUIRE(requesterReports.attributeOwnershipAcquisitionReports.empty());
  REQUIRE(owner->isAttributeOwnedByFederate(objectInstance, efficiency));
  REQUIRE_FALSE(requester->isAttributeOwnedByFederate(objectInstance, efficiency));

  REQUIRE_NOTHROW(requester->resignFederationExecution(
      rti1516_2025::CANCEL_PENDING_OWNERSHIP_ACQUISITIONS));
  // The requester-side If Available callback was already queued. It must be
  // consumed as a no-delivery callback after the registry removes the WTA
  // reservation, not report acquisition to a federate that has resigned.
  while (requester->evokeCallback(0.0)) {
  }
  REQUIRE(requesterReports.attributeOwnershipAcquisitionReports.empty());
  REQUIRE(owner->isAttributeOwnedByFederate(objectInstance, efficiency));

  REQUIRE_NOTHROW(owner->resignFederationExecution(rti1516_2025::DELETE_OBJECTS));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(requester->disconnect());
  REQUIRE_NOTHROW(owner->disconnect());
}

TEST_CASE(
    "Embedded regular acquisition cancels a pending If Available request",
    "[integration][development-profile][federation-management][ownership-management]"
    "[ownership-if-available-overridden-by-regular-acquisition]"
    "[rti.service.attribute-ownership-acquisition-if-available]"
    "[rti.service.attribute-ownership-acquisition]"
    "[rti.service.cancel-attribute-ownership-acquisition]"
    "[federate.callback.attribute-ownership-unavailable]"
    "[federate.callback.confirm-attribute-ownership-acquisition-cancellation]") {
  ReportingFederateAmbassador ownerReports;
  ReportingFederateAmbassador requesterReports;
  auto owner = makeRti();
  auto requester = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  auto const drain = [](RTIambassador& ambassador) {
    while (ambassador.evokeCallback(0.0)) {
    }
  };

  REQUIRE_NOTHROW(owner->connect(ownerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(requester->connect(requesterReports, HLA_EVOKED));
  REQUIRE_NOTHROW(owner->createFederationExecution(
      federationName, fomModule, standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(owner->joinFederationExecution(
      L"override-if-available-owner", L"publisher", federationName));
  REQUIRE_NOTHROW(requester->joinFederationExecution(
      L"override-if-available-requester", L"publisher", federationName));

  auto const server = owner->getObjectClassHandle(fixture_hla::fom::employee_server);
  auto const efficiency = owner->getAttributeHandle(server, fixture_hla::fixture::efficiency);
  AttributeHandleSet const efficiencyOnly{efficiency};
  REQUIRE_NOTHROW(owner->publishObjectClassAttributes(server, efficiencyOnly));
  REQUIRE_NOTHROW(requester->subscribeObjectClassAttributes(server, efficiencyOnly));
  REQUIRE_NOTHROW(requester->publishObjectClassAttributes(server, efficiencyOnly));
  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = owner->registerObjectInstance(server));
  drain(*requester);
  REQUIRE(requesterReports.objectDiscoveryReports.size() == 1U);

  unsigned char const ifAvailableTagBytes[] = {0x49U, 0x46U, 0x41U};
  unsigned char const regularTagBytes[] = {0x52U, 0x45U, 0x47U};
  VariableLengthData const ifAvailableTag(ifAvailableTagBytes, sizeof(ifAvailableTagBytes));
  VariableLengthData const regularTag(regularTagBytes, sizeof(regularTagBytes));
  REQUIRE_NOTHROW(requester->attributeOwnershipAcquisitionIfAvailable(
      objectInstance, efficiencyOnly, ifAvailableTag));
  REQUIRE(requesterReports.attributeOwnershipAcquisitionReports.empty());
  REQUIRE(owner->isAttributeOwnedByFederate(objectInstance, efficiency));
  REQUIRE_FALSE(requester->isAttributeOwnedByFederate(objectInstance, efficiency));

  // A regular Acquisition replaces the still-pending If Available request.
  // The regular request stays pending until it is cancelled below.
  REQUIRE_NOTHROW(requester->attributeOwnershipAcquisition(
      objectInstance, efficiencyOnly, regularTag));
  drain(*requester);
  REQUIRE(requesterReports.attributeOwnershipAcquisitionReports.empty());
  drain(*owner);
  REQUIRE(ownerReports.attributeOwnershipReleaseRequestReports.size() == 1U);
  REQUIRE(ownerReports.attributeOwnershipReleaseRequestReports.front().objectInstance ==
          objectInstance);
  REQUIRE(ownerReports.attributeOwnershipReleaseRequestReports.front().attributes ==
          efficiencyOnly);
  REQUIRE(owner->isAttributeOwnedByFederate(objectInstance, efficiency));
  REQUIRE_FALSE(requester->isAttributeOwnedByFederate(objectInstance, efficiency));

  REQUIRE_NOTHROW(requester->cancelAttributeOwnershipAcquisition(
      objectInstance, efficiencyOnly));
  drain(*requester);
  REQUIRE(requesterReports.attributeOwnershipAcquisitionCancellationReports.size() == 1U);
  REQUIRE(requesterReports.attributeOwnershipAcquisitionCancellationReports.front().objectInstance ==
          objectInstance);
  REQUIRE(requesterReports.attributeOwnershipAcquisitionCancellationReports.front().attributes ==
          efficiencyOnly);
  REQUIRE(requesterReports.attributeOwnershipAcquisitionReports.empty());

  REQUIRE_NOTHROW(owner->resignFederationExecution(rti1516_2025::DELETE_OBJECTS));
  REQUIRE_NOTHROW(requester->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(requester->disconnect());
  REQUIRE_NOTHROW(owner->disconnect());
}
} // namespace
