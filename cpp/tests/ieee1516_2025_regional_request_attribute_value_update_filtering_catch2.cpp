#include "ieee1516_2025_federation_management_fixture_support.hpp"

namespace {
enum class RegionalRequestFilteringVariant {
  original,
  restoredBaselineCopy,
};

void runRegionalRequestAttributeValueUpdateFilteringScenario(
    RegionalRequestFilteringVariant variant) {
  bool const isRestoredBaselineCopy =
      variant == RegionalRequestFilteringVariant::restoredBaselineCopy;
  ReportingFederateAmbassador ownerReports;
  ReportingFederateAmbassador requesterReports;
  auto owner = makeRti();
  auto requester = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  unsigned char const tagBytes[] = {0x91, 0x25};
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));

  REQUIRE_NOTHROW(owner->connect(ownerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(requester->connect(requesterReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      owner->createFederationExecution(federationName, fomModule, L"HLAinteger64Time"));
  REQUIRE_NOTHROW(owner->joinFederationExecution(
      L"regional-request-owner", L"publisher", federationName));
  REQUIRE_NOTHROW(requester->joinFederationExecution(
      L"regional-requester", L"requester", federationName));

  auto const soda = owner->getObjectClassHandle(L"HLAobjectRoot.Food.Drink.Soda");
  auto const flavor = owner->getAttributeHandle(soda, L"Flavor");
  auto const sodaFlavor = owner->getDimensionHandle(L"SodaFlavor");
  REQUIRE(soda.isValid());
  REQUIRE(flavor.isValid());
  REQUIRE(sodaFlavor.isValid());
  AttributeHandleSet const flavorOnly{flavor};
  REQUIRE_NOTHROW(owner->publishObjectClassAttributes(soda, flavorOnly));
  if (isRestoredBaselineCopy) {
    REQUIRE_NOTHROW(requester->publishObjectClassAttributes(soda, flavorOnly));
  }

  auto const ownerRegion = owner->createRegion(DimensionHandleSet{sodaFlavor});
  auto const requestRegion = requester->createRegion(DimensionHandleSet{sodaFlavor});
  REQUIRE_NOTHROW(owner->setRangeBounds(ownerRegion, sodaFlavor, RangeBounds(0UL, 1UL)));
  REQUIRE_NOTHROW(owner->commitRegionModifications(RegionHandleSet{ownerRegion}));
  REQUIRE_NOTHROW(requester->setRangeBounds(requestRegion, sodaFlavor, RangeBounds(2UL, 3UL)));
  REQUIRE_NOTHROW(requester->commitRegionModifications(RegionHandleSet{requestRegion}));

  AttributeHandleSetRegionHandleSetPairVector const ownerPair{{
      flavorOnly,
      RegionHandleSet{ownerRegion},
  }};
  AttributeHandleSetRegionHandleSetPairVector const disjointRequestPair{{
      flavorOnly,
      RegionHandleSet{requestRegion},
  }};
  AttributeHandleSetRegionHandleSetPairVector const emptyRequestPair{{
      flavorOnly,
      RegionHandleSet{},
  }};
  auto const serverId = requester->getDimensionHandle(L"ServerId");
  auto const wrongContextRegion = requester->createRegion(DimensionHandleSet{serverId});
  auto const uncommittedRegion = requester->createRegion(DimensionHandleSet{sodaFlavor});
  REQUIRE_NOTHROW(requester->setRangeBounds(
      wrongContextRegion,
      serverId,
      RangeBounds(0UL, 1UL)));
  REQUIRE_NOTHROW(requester->commitRegionModifications(RegionHandleSet{wrongContextRegion}));
  AttributeHandleSetRegionHandleSetPairVector const foreignRequestPair{{
      flavorOnly,
      RegionHandleSet{ownerRegion},
  }};
  AttributeHandleSetRegionHandleSetPairVector const wrongContextRequestPair{{
      flavorOnly,
      RegionHandleSet{wrongContextRegion},
  }};
  AttributeHandleSetRegionHandleSetPairVector const uncommittedRequestPair{{
      flavorOnly,
      RegionHandleSet{uncommittedRegion},
  }};

  ObjectInstanceHandle regionalObject;
  ObjectInstanceHandle defaultObject;
  REQUIRE_NOTHROW(regionalObject = owner->registerObjectInstanceWithRegions(soda, ownerPair));
  REQUIRE_NOTHROW(defaultObject = owner->registerObjectInstance(soda));
  REQUIRE(regionalObject.isValid());
  REQUIRE(defaultObject.isValid());

  REQUIRE_THROWS_AS(
      requester->requestAttributeValueUpdateWithRegions(soda, foreignRequestPair, tag),
      rti1516_2025::RegionNotCreatedByThisFederate);
  REQUIRE_THROWS_AS(
      requester->requestAttributeValueUpdateWithRegions(soda, wrongContextRequestPair, tag),
      rti1516_2025::InvalidRegionContext);
  REQUIRE_THROWS_AS(
      requester->requestAttributeValueUpdateWithRegions(soda, uncommittedRequestPair, tag),
      rti1516_2025::InvalidRegion);

  // An empty request-region set is a no-op for that class attribute.
  REQUIRE_NOTHROW(requester->requestAttributeValueUpdateWithRegions(
      soda,
      emptyRequestPair,
      tag));
  REQUIRE_FALSE(owner->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(ownerReports.attributeValueUpdateRequestReports.empty());
  if (isRestoredBaselineCopy) {
    REQUIRE(requesterReports.attributeValueUpdateRequestReports.empty());
  }

  // The explicit [0,1) update association is disjoint from [2,3), while the
  // ordinary/default-region object remains eligible for a regional request.
  REQUIRE_NOTHROW(requester->requestAttributeValueUpdateWithRegions(
      soda,
      disjointRequestPair,
      tag));
  REQUIRE_FALSE(owner->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(ownerReports.attributeValueUpdateRequestReports.size() == 1);
  REQUIRE(ownerReports.attributeValueUpdateRequestReports.front().objectInstance == defaultObject);
  REQUIRE(ownerReports.attributeValueUpdateRequestReports.front().attributes == flavorOnly);
  REQUIRE(variableLengthDataBytes(
              ownerReports.attributeValueUpdateRequestReports.front().userSuppliedTag) ==
          std::vector<unsigned char>(tagBytes, tagBytes + sizeof(tagBytes)));
  if (isRestoredBaselineCopy) {
    REQUIRE(requesterReports.attributeValueUpdateRequestReports.empty());
  }

  // Recommitting the requester region into [0,1) makes both the explicit and
  // default-region instances eligible.
  REQUIRE_NOTHROW(requester->setRangeBounds(requestRegion, sodaFlavor, RangeBounds(0UL, 1UL)));
  REQUIRE_NOTHROW(requester->commitRegionModifications(RegionHandleSet{requestRegion}));
  REQUIRE_NOTHROW(requester->requestAttributeValueUpdateWithRegions(
      soda,
      disjointRequestPair,
      tag));
  REQUIRE(owner->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE_FALSE(owner->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(ownerReports.attributeValueUpdateRequestReports.size() == 3);
  if (isRestoredBaselineCopy) {
    REQUIRE(requesterReports.attributeValueUpdateRequestReports.empty());
  }

  // Queue the same overlapping request, then move the committed request
  // region away before delivery. The explicit provider callback is suppressed
  // at callback entry while the default-region callback remains eligible.
  REQUIRE_NOTHROW(requester->requestAttributeValueUpdateWithRegions(
      soda,
      disjointRequestPair,
      tag));
  REQUIRE_NOTHROW(requester->setRangeBounds(requestRegion, sodaFlavor, RangeBounds(2UL, 3UL)));
  REQUIRE_NOTHROW(requester->commitRegionModifications(RegionHandleSet{requestRegion}));
  REQUIRE(owner->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE_FALSE(owner->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(ownerReports.attributeValueUpdateRequestReports.size() == 4);
  std::set<ObjectInstanceHandle> reportedObjects;
  for (auto const& report : ownerReports.attributeValueUpdateRequestReports) {
    REQUIRE(report.attributes == flavorOnly);
    reportedObjects.insert(report.objectInstance);
  }
  REQUIRE(reportedObjects == std::set<ObjectInstanceHandle>{regionalObject, defaultObject});
  if (isRestoredBaselineCopy) {
    REQUIRE(requesterReports.attributeValueUpdateRequestReports.empty());
  }

  REQUIRE_NOTHROW(owner->unassociateRegionsForUpdates(regionalObject, ownerPair));
  REQUIRE_NOTHROW(owner->deleteRegion(ownerRegion));
  REQUIRE_NOTHROW(requester->deleteRegion(wrongContextRegion));
  REQUIRE_NOTHROW(requester->deleteRegion(uncommittedRegion));
  REQUIRE_NOTHROW(requester->deleteRegion(requestRegion));
  REQUIRE_NOTHROW(requester->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->resignFederationExecution(CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(owner->disconnect());
  REQUIRE_NOTHROW(requester->disconnect());
}
}  // namespace

TEST_CASE(
    "Embedded regional Request Attribute Value Update filters 2025 owner solicitations",
    "[integration][development-profile][federation-management][ddm]"
    "[ddm-clause6-service-expansion]"
    "[rti.service.request-attribute-value-update-with-regions]"
    "[federate.callback.provide-attribute-value-update]"
    "[regional-request-filtering]") {
  runRegionalRequestAttributeValueUpdateFilteringScenario(
      RegionalRequestFilteringVariant::original);
}

TEST_CASE(
    "Embedded regional Request Attribute Value Update filters 2025 owner solicitations (restored baseline copy)",
    "[integration][development-profile][federation-management][ddm]"
    "[rti.service.request-attribute-value-update-with-regions]"
    "[federate.callback.provide-attribute-value-update]"
    "[regional-request-filtering][restored-baseline-copy]") {
  runRegionalRequestAttributeValueUpdateFilteringScenario(
      RegionalRequestFilteringVariant::restoredBaselineCopy);
}
