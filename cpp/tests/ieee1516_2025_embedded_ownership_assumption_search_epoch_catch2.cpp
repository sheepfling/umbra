#include "ieee1516_2025_federation_management_fixture_support.hpp"

namespace {
TEST_CASE(
    "Embedded ownership assumption search starts a fresh epoch after transfer",
    "[integration][development-profile][ownership-management][federation-management]"
    "[rti.service.unconditional-attribute-ownership-divestiture]"
    "[rti.service.attribute-ownership-acquisition]"
    "[rti.service.publish-object-class-attributes]"
    "[rti.service.unpublish-object-class]"
    "[federate.callback.request-attribute-ownership-assumption]"
    "[federate.callback.attribute-ownership-acquisition-notification]"
    "[ownership-assumption-search-epoch]") {
  ReportingFederateAmbassador ownerReports;
  ReportingFederateAmbassador firstCandidateReports;
  ReportingFederateAmbassador secondCandidateReports;
  auto owner = makeRti();
  auto firstCandidate = makeRti();
  auto secondCandidate = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();

  REQUIRE_NOTHROW(owner->connect(ownerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(firstCandidate->connect(firstCandidateReports, HLA_EVOKED));
  REQUIRE_NOTHROW(secondCandidate->connect(secondCandidateReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      owner->createFederationExecution(federationName, fomModule, standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(owner->joinFederationExecution(
      L"ownership-epoch-owner", L"publisher", federationName));
  REQUIRE_NOTHROW(firstCandidate->joinFederationExecution(
      L"ownership-epoch-first", L"candidate", federationName));
  REQUIRE_NOTHROW(secondCandidate->joinFederationExecution(
      L"ownership-epoch-second", L"candidate", federationName));

  auto const server = owner->getObjectClassHandle(fixture_hla::fom::employee_server);
  auto const efficiency = owner->getAttributeHandle(server, fixture_hla::fixture::efficiency);
  auto const privilegeToDelete = owner->getAttributeHandle(
      server,
      standard_hla::mom::privilege_to_delete_object);
  AttributeHandleSet const publishedAttributes{efficiency};
  AttributeHandleSet const transferredAttributes{efficiency, privilegeToDelete};
  REQUIRE_NOTHROW(owner->publishObjectClassAttributes(server, publishedAttributes));
  REQUIRE_NOTHROW(firstCandidate->subscribeObjectClassAttributes(server, publishedAttributes));
  REQUIRE_NOTHROW(secondCandidate->subscribeObjectClassAttributes(server, publishedAttributes));

  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = owner->registerObjectInstance(server));
  REQUIRE_FALSE(firstCandidate->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE_FALSE(secondCandidate->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(firstCandidateReports.objectDiscoveryReports.size() == 1);
  REQUIRE(secondCandidateReports.objectDiscoveryReports.size() == 1);
  REQUIRE_NOTHROW(firstCandidate->publishObjectClassAttributes(server, publishedAttributes));
  REQUIRE_NOTHROW(secondCandidate->publishObjectClassAttributes(server, publishedAttributes));

  unsigned char const firstDivestitureTagBytes[] = {0xE1, 0x25};
  unsigned char const secondDivestitureTagBytes[] = {0xE2, 0x25};
  unsigned char const acquisitionTagBytes[] = {0xE3, 0x25};
  VariableLengthData const firstDivestitureTag(
      firstDivestitureTagBytes,
      sizeof(firstDivestitureTagBytes));
  VariableLengthData const secondDivestitureTag(
      secondDivestitureTagBytes,
      sizeof(secondDivestitureTagBytes));
  VariableLengthData const acquisitionTag(acquisitionTagBytes, sizeof(acquisitionTagBytes));

  // Both currently eligible candidates receive the first search offer. Only
  // the first candidate accepts it; the second candidate's reservation is
  // intentionally left behind to test the next ownership-transfer epoch.
  REQUIRE_NOTHROW(owner->unconditionalAttributeOwnershipDivestiture(
      objectInstance,
      transferredAttributes,
      firstDivestitureTag));
  while (firstCandidate->evokeMultipleCallbacks(0.0, 0.0)) {
  }
  while (secondCandidate->evokeMultipleCallbacks(0.0, 0.0)) {
  }
  REQUIRE(firstCandidateReports.attributeOwnershipAssumptionReports.size() == 1);
  REQUIRE(secondCandidateReports.attributeOwnershipAssumptionReports.size() == 1);

  REQUIRE_NOTHROW(firstCandidate->attributeOwnershipAcquisition(
      objectInstance,
      transferredAttributes,
      acquisitionTag));
  while (firstCandidate->evokeMultipleCallbacks(0.0, 0.0)) {
  }
  REQUIRE(firstCandidateReports.attributeOwnershipAcquisitionReports.size() == 1);
  REQUIRE(
      firstCandidateReports.attributeOwnershipAcquisitionReports.front().kind ==
      ReportingFederateAmbassador::AttributeOwnershipAcquisitionReport::Kind::notification);
  REQUIRE(firstCandidate->isAttributeOwnedByFederate(objectInstance, efficiency));

  // Remove the old candidate from eligibility without starting another
  // acquisition. The first search has already offered this federate once.
  REQUIRE_NOTHROW(secondCandidate->unpublishObjectClass(server));
  REQUIRE_NOTHROW(owner->resignFederationExecution(NO_ACTION));

  // The first candidate now becomes the owner and starts a new unowned
  // interval. The second candidate must be offered again after it republishes;
  // the old search reservation must not suppress the fresh epoch or its tag.
  REQUIRE_NOTHROW(firstCandidate->unconditionalAttributeOwnershipDivestiture(
      objectInstance,
      transferredAttributes,
      secondDivestitureTag));
  REQUIRE_NOTHROW(secondCandidate->publishObjectClassAttributes(server, publishedAttributes));
  while (secondCandidate->evokeMultipleCallbacks(0.0, 0.0)) {
  }
  REQUIRE(secondCandidateReports.attributeOwnershipAssumptionReports.size() == 2);
  auto const& freshAssumption = secondCandidateReports.attributeOwnershipAssumptionReports.back();
  REQUIRE(freshAssumption.objectInstance == objectInstance);
  REQUIRE(freshAssumption.attributes == transferredAttributes);
  REQUIRE(variableLengthDataBytes(freshAssumption.userSuppliedTag) ==
          std::vector<unsigned char>(
              secondDivestitureTagBytes,
              secondDivestitureTagBytes + sizeof(secondDivestitureTagBytes)));

  REQUIRE_NOTHROW(secondCandidate->unpublishObjectClass(server));
  REQUIRE_NOTHROW(firstCandidate->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(secondCandidate->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(secondCandidate->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(owner->disconnect());
  REQUIRE_NOTHROW(firstCandidate->disconnect());
  REQUIRE_NOTHROW(secondCandidate->disconnect());
}

} // namespace
