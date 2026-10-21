#include "ieee1516_2025_federation_management_fixture_support.hpp"

namespace {

TEST_CASE(
    "Embedded class Request Attribute Value Update without regions matches the default-region request",
    "[integration][development-profile][federation-management][ddm]"
    "[default-region-equivalence]"
    "[rti.service.request-attribute-value-update]"
    "[rti.service.request-attribute-value-update-with-regions]"
    "[federate.callback.provide-attribute-value-update]") {
  ReportingFederateAmbassador ownerReports;
  ReportingFederateAmbassador ordinaryRequesterReports;
  ReportingFederateAmbassador regionalRequesterReports;
  auto owner = makeRti();
  auto ordinaryRequester = makeRti();
  auto regionalRequester = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();

  REQUIRE_NOTHROW(owner->connect(ownerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(ordinaryRequester->connect(ordinaryRequesterReports, HLA_EVOKED));
  REQUIRE_NOTHROW(regionalRequester->connect(regionalRequesterReports, HLA_EVOKED));
  REQUIRE_NOTHROW(owner->createFederationExecution(
      federationName,
      fomModule,
      L"HLAinteger64Time"));
  REQUIRE_NOTHROW(owner->joinFederationExecution(
      L"ddm-default-region-request-owner", L"publisher", federationName));
  REQUIRE_NOTHROW(ordinaryRequester->joinFederationExecution(
      L"ddm-default-region-ordinary-requester", L"requester", federationName));
  REQUIRE_NOTHROW(regionalRequester->joinFederationExecution(
      L"ddm-default-region-regional-requester", L"requester", federationName));

  auto const soda = owner->getObjectClassHandle(L"HLAobjectRoot.Food.Drink.Soda");
  auto const flavor = owner->getAttributeHandle(soda, L"Flavor");
  auto const sodaFlavor = owner->getDimensionHandle(L"SodaFlavor");
  REQUIRE(soda.isValid());
  REQUIRE(flavor.isValid());
  REQUIRE(sodaFlavor.isValid());
  AttributeHandleSet const flavorOnly{flavor};
  REQUIRE_NOTHROW(owner->publishObjectClassAttributes(soda, flavorOnly));

  auto const sourceRegion = owner->createRegion(DimensionHandleSet{sodaFlavor});
  auto const broadRequestRegion =
      regionalRequester->createRegion(DimensionHandleSet{sodaFlavor});
  REQUIRE_NOTHROW(owner->setRangeBounds(
      sourceRegion,
      sodaFlavor,
      RangeBounds(0UL, 1UL)));
  REQUIRE_NOTHROW(owner->commitRegionModifications(RegionHandleSet{sourceRegion}));
  REQUIRE_NOTHROW(regionalRequester->setRangeBounds(
      broadRequestRegion,
      sodaFlavor,
      RangeBounds(0UL, 3UL)));
  REQUIRE_NOTHROW(regionalRequester->commitRegionModifications(
      RegionHandleSet{broadRequestRegion}));

  AttributeHandleSetRegionHandleSetPairVector const sourcePair{{
      flavorOnly,
      RegionHandleSet{sourceRegion},
  }};
  AttributeHandleSetRegionHandleSetPairVector const broadRequestPair{{
      flavorOnly,
      RegionHandleSet{broadRequestRegion},
  }};
  ObjectInstanceHandle const regionalObject =
      owner->registerObjectInstanceWithRegions(soda, sourcePair);
  ObjectInstanceHandle const defaultRegionObject = owner->registerObjectInstance(soda);
  REQUIRE(regionalObject.isValid());
  REQUIRE(defaultRegionObject.isValid());

  unsigned char const ordinaryTagBytes[] = {0xA1, 0x01};
  unsigned char const regionalTagBytes[] = {0xB2, 0x02};
  VariableLengthData const ordinaryTag(ordinaryTagBytes, sizeof(ordinaryTagBytes));
  VariableLengthData const regionalTag(regionalTagBytes, sizeof(regionalTagBytes));

  // The omitted region is the default-region request. A broad explicit region
  // supplies the same coverage through the corresponding regional overload.
  REQUIRE_NOTHROW(ordinaryRequester->requestAttributeValueUpdate(
      soda,
      flavorOnly,
      ordinaryTag));
  REQUIRE_NOTHROW(regionalRequester->requestAttributeValueUpdateWithRegions(
      soda,
      broadRequestPair,
      regionalTag));

  for (int pass = 0; pass != 128; ++pass) {
    if (!owner->evokeMultipleCallbacks(0.0, 0.0)) {
      break;
    }
  }

  std::set<ObjectInstanceHandle> ordinaryRequestObjects;
  std::set<ObjectInstanceHandle> regionalRequestObjects;
  for (auto const& report : ownerReports.attributeValueUpdateRequestReports) {
    REQUIRE(report.attributes == flavorOnly);
    auto const reportTag = variableLengthDataBytes(report.userSuppliedTag);
    if (reportTag == std::vector<unsigned char>(
                         ordinaryTagBytes,
                         ordinaryTagBytes + sizeof(ordinaryTagBytes))) {
      ordinaryRequestObjects.insert(report.objectInstance);
    } else if (reportTag == std::vector<unsigned char>(
                                regionalTagBytes,
                                regionalTagBytes + sizeof(regionalTagBytes))) {
      regionalRequestObjects.insert(report.objectInstance);
    } else {
      FAIL("The provider callback retained an unknown request tag.");
    }
  }

  std::set<ObjectInstanceHandle> const expectedObjects{
      regionalObject,
      defaultRegionObject,
  };
  REQUIRE(ordinaryRequestObjects == expectedObjects);
  REQUIRE(regionalRequestObjects == expectedObjects);
  REQUIRE(ordinaryRequestObjects == regionalRequestObjects);

  REQUIRE_NOTHROW(owner->unassociateRegionsForUpdates(regionalObject, sourcePair));
  REQUIRE_NOTHROW(owner->deleteRegion(sourceRegion));
  REQUIRE_NOTHROW(regionalRequester->deleteRegion(broadRequestRegion));
  REQUIRE_NOTHROW(regionalRequester->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(ordinaryRequester->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->resignFederationExecution(CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(regionalRequester->disconnect());
  REQUIRE_NOTHROW(ordinaryRequester->disconnect());
  REQUIRE_NOTHROW(owner->disconnect());
}

}  // namespace
