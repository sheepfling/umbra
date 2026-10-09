#include "ieee1516_2025_federation_management_fixture_support.hpp"

namespace {

TEST_CASE(
    "Embedded suppressed timestamped regional attribute callback does not request retraction",
    "[integration][development-profile][object-management][ddm][time-management][tso][retract]"
    "[rti.service.update-attribute-values][rti.service.set-range-bounds]"
    "[rti.service.commit-region-modifications]"
    "[rti.service.register-object-instance-with-regions]"
    "[rti.service.subscribe-object-class-attributes-with-regions]"
    "[rti.service.retract][federate.callback.reflect-attribute-values]"
    "[federate.callback.request-retraction][suppressed-timestamped-regional-retraction]") {
  ReportingFederateAmbassador publisherReports;
  ReportingFederateAmbassador receiverReports;
  auto publisher = makeRti();
  auto receiver = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  unsigned char const valueBytes[] = {0x53, 0x55, 0x50, 0x41, 0x54, 0x54};
  AttributeHandleValueMap values;

  REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED));
  REQUIRE_NOTHROW(receiver->connect(receiverReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      publisher->createFederationExecution(federationName, fomModule, L"HLAinteger64Time"));
  REQUIRE_NOTHROW(publisher->joinFederationExecution(
      L"suppressed-regional-attribute-retraction-publisher",
      L"publisher",
      federationName));
  REQUIRE_NOTHROW(receiver->joinFederationExecution(
      L"suppressed-regional-attribute-retraction-receiver",
      L"subscriber",
      federationName));

  auto const soda = publisher->getObjectClassHandle(L"HLAobjectRoot.Food.Drink.Soda");
  auto const flavor = publisher->getAttributeHandle(soda, L"Flavor");
  auto const sodaFlavor = publisher->getDimensionHandle(L"SodaFlavor");
  REQUIRE(soda.isValid());
  REQUIRE(flavor.isValid());
  REQUIRE(sodaFlavor.isValid());
  AttributeHandleSet const flavorOnly{flavor};
  values.emplace(flavor, VariableLengthData(valueBytes, sizeof(valueBytes)));
  REQUIRE_NOTHROW(publisher->publishObjectClassAttributes(soda, flavorOnly));
  REQUIRE_NOTHROW(publisher->changeDefaultAttributeOrderType(
      soda,
      flavorOnly,
      TIMESTAMP));

  auto const publisherRegion = publisher->createRegion(DimensionHandleSet{sodaFlavor});
  auto const receiverRegion = receiver->createRegion(DimensionHandleSet{sodaFlavor});
  REQUIRE_NOTHROW(publisher->setRangeBounds(
      publisherRegion,
      sodaFlavor,
      RangeBounds(0UL, 1UL)));
  REQUIRE_NOTHROW(receiver->setRangeBounds(
      receiverRegion,
      sodaFlavor,
      RangeBounds(0UL, 1UL)));
  REQUIRE_NOTHROW(publisher->commitRegionModifications(RegionHandleSet{publisherRegion}));
  REQUIRE_NOTHROW(receiver->commitRegionModifications(RegionHandleSet{receiverRegion}));
  AttributeHandleSetRegionHandleSetPairVector const publisherPair{{
      flavorOnly,
      RegionHandleSet{publisherRegion},
  }};
  AttributeHandleSetRegionHandleSetPairVector const receiverPair{{
      flavorOnly,
      RegionHandleSet{receiverRegion},
  }};
  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = publisher->registerObjectInstanceWithRegions(
      soda,
      publisherPair));
  REQUIRE_NOTHROW(receiver->subscribeObjectClassAttributesWithRegions(
      soda,
      receiverPair));
  while (receiver->evokeCallback(0.0)) {
  }
  REQUIRE(receiverReports.objectDiscoveryReports.size() == 1U);
  REQUIRE_NOTHROW(publisher->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(5)));
  while (publisher->evokeCallback(0.0)) {
  }

  auto const retraction = publisher->updateAttributeValues(
      objectInstance,
      values,
      VariableLengthData(),
      rti1516_2025::HLAinteger64Time(6));
  REQUIRE(retraction.isValid());

  // The instance/update region overlapped when the passel was accepted. A
  // later committed disjoint region suppresses the callback at its boundary;
  // it must consume the recipient ledger without classifying it delivered.
  REQUIRE_NOTHROW(receiver->setRangeBounds(
      receiverRegion,
      sodaFlavor,
      RangeBounds(2UL, 3UL)));
  REQUIRE_NOTHROW(receiver->commitRegionModifications(RegionHandleSet{receiverRegion}));
  while (receiver->evokeCallback(0.0)) {
  }
  REQUIRE(receiverReports.attributeReflectionReports.empty());
  REQUIRE_NOTHROW(publisher->retract(retraction));
  while (receiver->evokeCallback(0.0)) {
  }
  REQUIRE(receiverReports.requestRetractionReports.empty());

  REQUIRE_NOTHROW(receiver->unsubscribeObjectClassAttributesWithRegions(
      soda,
      receiverPair));
  REQUIRE_NOTHROW(publisher->unassociateRegionsForUpdates(objectInstance, publisherPair));
  REQUIRE_NOTHROW(receiver->deleteRegion(receiverRegion));
  REQUIRE_NOTHROW(publisher->deleteRegion(publisherRegion));
  REQUIRE_NOTHROW(receiver->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(receiver->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
}


} // namespace
