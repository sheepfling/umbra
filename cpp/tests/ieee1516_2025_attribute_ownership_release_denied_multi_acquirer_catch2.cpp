#include "ieee1516_2025_federation_management_fixture_support.hpp"

namespace {
TEST_CASE(
    "Embedded Attribute Ownership Release Denied reaches all 2025 regular acquirers",
    "[integration][development-profile][ownership-management]"
    "[attribute-ownership-release-denied-multi-acquirer]"
    "[rti.service.attribute-ownership-acquisition]"
    "[federate.callback.request-attribute-ownership-release]"
    "[rti.service.attribute-ownership-release-denied]"
    "[federate.callback.attribute-ownership-unavailable]") {
  ReportingFederateAmbassador ownerReports;
  ReportingFederateAmbassador firstRequesterReports;
  ReportingFederateAmbassador secondRequesterReports;
  auto owner = makeRti();
  auto firstRequester = makeRti();
  auto secondRequester = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
                          "cpp" /
                          "tests" /
                          "data" /
                          "attribute-update-passel-fom.xml")
                             .wstring();
  unsigned char const firstAcquisitionTagBytes[] = {0x01, 0x02, 0x03};
  unsigned char const secondAcquisitionTagBytes[] = {0x04, 0x05, 0x06};
  unsigned char const denialTagBytes[] = {0xD3, 0x1E, 0xD0};
  VariableLengthData const firstAcquisitionTag(
      firstAcquisitionTagBytes,
      sizeof(firstAcquisitionTagBytes));
  VariableLengthData const secondAcquisitionTag(
      secondAcquisitionTagBytes,
      sizeof(secondAcquisitionTagBytes));
  VariableLengthData const denialTag(denialTagBytes, sizeof(denialTagBytes));

  REQUIRE_NOTHROW(owner->connect(ownerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(firstRequester->connect(firstRequesterReports, HLA_EVOKED));
  REQUIRE_NOTHROW(secondRequester->connect(secondRequesterReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      owner->createFederationExecution(federationName, fomModule, L"HLAinteger64Time"));
  REQUIRE_NOTHROW(
      owner->joinFederationExecution(L"denial-owner", L"publisher", federationName));
  REQUIRE_NOTHROW(firstRequester->joinFederationExecution(
      L"denial-first-requester",
      L"publisher",
      federationName));
  REQUIRE_NOTHROW(secondRequester->joinFederationExecution(
      L"denial-second-requester",
      L"publisher",
      federationName));

  auto const child = owner->getObjectClassHandle(
      L"HLAobjectRoot.UmbraAttributeFixtureBase.UmbraAttributeFixtureChild");
  auto const reliableBaseA = owner->getAttributeHandle(child, L"ReliableBaseA");
  REQUIRE(child.isValid());
  REQUIRE(reliableBaseA.isValid());
  AttributeHandleSet const ownedAttributes{reliableBaseA};
  REQUIRE_NOTHROW(firstRequester->subscribeObjectClassAttributes(child, ownedAttributes));
  REQUIRE_NOTHROW(secondRequester->subscribeObjectClassAttributes(child, ownedAttributes));
  REQUIRE_NOTHROW(owner->publishObjectClassAttributes(child, ownedAttributes));

  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = owner->registerObjectInstance(child));
  REQUIRE_FALSE(firstRequester->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE_FALSE(secondRequester->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(firstRequesterReports.objectDiscoveryReports.size() == 1);
  REQUIRE(secondRequesterReports.objectDiscoveryReports.size() == 1);
  REQUIRE_NOTHROW(firstRequester->publishObjectClassAttributes(child, ownedAttributes));
  REQUIRE_NOTHROW(secondRequester->publishObjectClassAttributes(child, ownedAttributes));

  REQUIRE_NOTHROW(firstRequester->attributeOwnershipAcquisition(
      objectInstance,
      ownedAttributes,
      firstAcquisitionTag));
  REQUIRE_NOTHROW(secondRequester->attributeOwnershipAcquisition(
      objectInstance,
      ownedAttributes,
      secondAcquisitionTag));
  REQUIRE(owner->evokeMultipleCallbacks(0.0, 0.0));
  while (owner->evokeMultipleCallbacks(0.0, 0.0)) {
  }
  REQUIRE_FALSE(owner->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(ownerReports.attributeOwnershipReleaseRequestReports.size() == 2);
  auto const firstRelease = std::find_if(
      ownerReports.attributeOwnershipReleaseRequestReports.begin(),
      ownerReports.attributeOwnershipReleaseRequestReports.end(),
      [&firstAcquisitionTagBytes](
          ReportingFederateAmbassador::AttributeOwnershipReleaseRequestReport const& report) {
        return variableLengthDataBytes(report.userSuppliedTag) ==
            std::vector<unsigned char>(
                firstAcquisitionTagBytes,
                firstAcquisitionTagBytes + sizeof(firstAcquisitionTagBytes));
      });
  auto const secondRelease = std::find_if(
      ownerReports.attributeOwnershipReleaseRequestReports.begin(),
      ownerReports.attributeOwnershipReleaseRequestReports.end(),
      [&secondAcquisitionTagBytes](
          ReportingFederateAmbassador::AttributeOwnershipReleaseRequestReport const& report) {
        return variableLengthDataBytes(report.userSuppliedTag) ==
            std::vector<unsigned char>(
                secondAcquisitionTagBytes,
                secondAcquisitionTagBytes + sizeof(secondAcquisitionTagBytes));
      });
  REQUIRE(firstRelease != ownerReports.attributeOwnershipReleaseRequestReports.end());
  REQUIRE(secondRelease != ownerReports.attributeOwnershipReleaseRequestReports.end());
  REQUIRE(firstRelease->objectInstance == objectInstance);
  REQUIRE(secondRelease->objectInstance == objectInstance);
  REQUIRE(firstRelease->attributes == ownedAttributes);
  REQUIRE(secondRelease->attributes == ownedAttributes);

  REQUIRE_NOTHROW(owner->attributeOwnershipReleaseDenied(
      objectInstance,
      ownedAttributes,
      denialTag));
  REQUIRE_FALSE(firstRequester->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE_FALSE(secondRequester->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(firstRequesterReports.attributeOwnershipAcquisitionReports.size() == 1);
  REQUIRE(secondRequesterReports.attributeOwnershipAcquisitionReports.size() == 1);
  auto const& firstUnavailable = firstRequesterReports.attributeOwnershipAcquisitionReports.front();
  auto const& secondUnavailable = secondRequesterReports.attributeOwnershipAcquisitionReports.front();
  REQUIRE(firstUnavailable.kind ==
          ReportingFederateAmbassador::AttributeOwnershipAcquisitionReport::Kind::unavailable);
  REQUIRE(secondUnavailable.kind ==
          ReportingFederateAmbassador::AttributeOwnershipAcquisitionReport::Kind::unavailable);
  REQUIRE(firstUnavailable.attributes == ownedAttributes);
  REQUIRE(secondUnavailable.attributes == ownedAttributes);
  REQUIRE(variableLengthDataBytes(firstUnavailable.userSuppliedTag) ==
          std::vector<unsigned char>(denialTagBytes, denialTagBytes + sizeof(denialTagBytes)));
  REQUIRE(variableLengthDataBytes(secondUnavailable.userSuppliedTag) ==
          std::vector<unsigned char>(denialTagBytes, denialTagBytes + sizeof(denialTagBytes)));
  REQUIRE(owner->isAttributeOwnedByFederate(objectInstance, reliableBaseA));
  REQUIRE_FALSE(firstRequester->isAttributeOwnedByFederate(objectInstance, reliableBaseA));
  REQUIRE_FALSE(secondRequester->isAttributeOwnedByFederate(objectInstance, reliableBaseA));
  REQUIRE_NOTHROW(firstRequester->unpublishObjectClassAttributes(child, ownedAttributes));
  REQUIRE_NOTHROW(secondRequester->unpublishObjectClassAttributes(child, ownedAttributes));

  REQUIRE_NOTHROW(owner->resignFederationExecution(CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(firstRequester->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(secondRequester->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(owner->disconnect());
  REQUIRE_NOTHROW(firstRequester->disconnect());
  REQUIRE_NOTHROW(secondRequester->disconnect());
}
}
