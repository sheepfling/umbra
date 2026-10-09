#include "ieee1516_2025_federation_management_fixture_support.hpp"

namespace {

TEST_CASE(
    "Embedded regional object attributes filter 2025 no-time updates by overlap",
    "[integration][development-profile][federation-management][ddm]"
    "[rti.service.register-object-instance-with-regions]"
    "[rti.service.associate-regions-for-updates]"
    "[rti.service.unassociate-regions-for-updates]"
    "[rti.service.subscribe-object-class-attributes-with-regions]"
    "[rti.service.unsubscribe-object-class-attributes-with-regions]"
    "[rti.service.update-attribute-values]"
    "[federate.callback.discover-object-instance]"
    "[federate.callback.reflect-attribute-values]") {
  ReportingFederateAmbassador publisherReports;
  ReportingFederateAmbassador subscriberReports;
  auto publisher = makeRti();
  auto subscriber = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();

  REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED));
  REQUIRE_NOTHROW(subscriber->connect(subscriberReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      publisher->createFederationExecution(federationName, fomModule, L"HLAinteger64Time"));
  REQUIRE_NOTHROW(publisher->joinFederationExecution(
      L"regional-object-publisher", L"publisher", federationName));
  REQUIRE_NOTHROW(subscriber->joinFederationExecution(
      L"regional-object-subscriber", L"subscriber", federationName));
  REQUIRE_FALSE(subscriber->getConveyRegionDesignatorSetsSwitch());

  auto const soda = publisher->getObjectClassHandle(L"HLAobjectRoot.Food.Drink.Soda");
  auto const flavor = publisher->getAttributeHandle(soda, L"Flavor");
  auto const sodaFlavor = publisher->getDimensionHandle(L"SodaFlavor");
  REQUIRE(soda.isValid());
  REQUIRE(flavor.isValid());
  REQUIRE(sodaFlavor.isValid());
  AttributeHandleSet const flavorOnly{flavor};
  REQUIRE_NOTHROW(publisher->publishObjectClassAttributes(soda, flavorOnly));

  auto const publisherRegion = publisher->createRegion(DimensionHandleSet{sodaFlavor});
  auto const subscriberRegion = subscriber->createRegion(DimensionHandleSet{sodaFlavor});
  REQUIRE_NOTHROW(publisher->setRangeBounds(
      publisherRegion,
      sodaFlavor,
      RangeBounds(0UL, 1UL)));
  REQUIRE_NOTHROW(publisher->commitRegionModifications(RegionHandleSet{publisherRegion}));
  REQUIRE_NOTHROW(subscriber->setRangeBounds(
      subscriberRegion,
      sodaFlavor,
      RangeBounds(2UL, 3UL)));
  REQUIRE_NOTHROW(subscriber->commitRegionModifications(RegionHandleSet{subscriberRegion}));

  AttributeHandleSetRegionHandleSetPairVector const regionalPair{{
      flavorOnly,
      RegionHandleSet{publisherRegion},
  }};
  AttributeHandleSetRegionHandleSetPairVector const subscriberPair{{
      flavorOnly,
      RegionHandleSet{subscriberRegion},
  }};
  AttributeHandleSetRegionHandleSetPairVector const emptyRegionPair{{
      flavorOnly,
      RegionHandleSet{},
  }};
  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = publisher->registerObjectInstanceWithRegions(
      soda,
      regionalPair));
  REQUIRE(objectInstance.isValid());
  REQUIRE_NOTHROW(subscriber->subscribeObjectClassAttributesWithRegions(
      soda,
      subscriberPair));
  REQUIRE_NOTHROW(subscriber->subscribeObjectClassAttributesWithRegions(
      soda,
      emptyRegionPair));
  REQUIRE_NOTHROW(publisher->associateRegionsForUpdates(
      objectInstance,
      emptyRegionPair));
  REQUIRE(subscriberReports.objectDiscoveryReports.empty());

  unsigned char const firstValueBytes[] = {0x10, 0x25};
  AttributeHandleValueMap firstValue;
  firstValue.emplace(
      flavor,
      VariableLengthData(firstValueBytes, sizeof(firstValueBytes)));
  REQUIRE_NOTHROW(publisher->updateAttributeValues(
      objectInstance,
      firstValue,
      VariableLengthData()));
  REQUIRE_FALSE(subscriber->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(subscriberReports.attributeReflectionReports.empty());

  // Association is additive and idempotent. The disjoint subscriber remains
  // undiscoverable even after the producer repeats the association explicitly.
  REQUIRE_NOTHROW(publisher->associateRegionsForUpdates(objectInstance, regionalPair));
  REQUIRE_NOTHROW(subscriber->setRangeBounds(
      subscriberRegion,
      sodaFlavor,
      RangeBounds(0UL, 1UL)));
  REQUIRE_NOTHROW(subscriber->commitRegionModifications(RegionHandleSet{subscriberRegion}));
  REQUIRE_NOTHROW(subscriber->subscribeObjectClassAttributesWithRegions(
      soda,
      subscriberPair));
  REQUIRE_FALSE(subscriber->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(subscriberReports.objectDiscoveryReports.size() == 1);
  REQUIRE(subscriber->getKnownObjectClassHandle(objectInstance) == soda);

  unsigned char const secondValueBytes[] = {0x20, 0x25};
  AttributeHandleValueMap secondValue;
  secondValue.emplace(
      flavor,
      VariableLengthData(secondValueBytes, sizeof(secondValueBytes)));
  REQUIRE_NOTHROW(publisher->updateAttributeValues(
      objectInstance,
      secondValue,
      VariableLengthData()));
  REQUIRE_FALSE(subscriber->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(subscriberReports.attributeReflectionReports.size() == 1);
  REQUIRE_FALSE(subscriberReports.attributeReflectionReports.front().sentRegionsSupplied);
  REQUIRE(
      subscriberReports.attributeReflectionReports.front().attributeValues.contains(flavor));

  // The switch is recipient-local and may be changed after a federation has
  // joined.  Enabling it exposes the same update-region realization on the
  // next reflection without changing regional overlap routing.
  REQUIRE_NOTHROW(subscriber->setConveyRegionDesignatorSetsSwitch(true));
  unsigned char const conveyedValueBytes[] = {0x25, 0x20};
  AttributeHandleValueMap conveyedValue;
  conveyedValue.emplace(
      flavor,
      VariableLengthData(conveyedValueBytes, sizeof(conveyedValueBytes)));
  REQUIRE_NOTHROW(publisher->updateAttributeValues(
      objectInstance,
      conveyedValue,
      VariableLengthData()));
  REQUIRE_FALSE(subscriber->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(subscriberReports.attributeReflectionReports.size() == 2);
  REQUIRE(subscriberReports.attributeReflectionReports.back().sentRegionsSupplied);
  REQUIRE(subscriberReports.attributeReflectionReports.back().sentRegions.contains(publisherRegion));

  // Changing the committed subscriber region to a disjoint range removes the
  // regional reflection route without changing ordinary known-instance state.
  REQUIRE_NOTHROW(subscriber->setRangeBounds(
      subscriberRegion,
      sodaFlavor,
      RangeBounds(2UL, 3UL)));
  REQUIRE_NOTHROW(subscriber->commitRegionModifications(RegionHandleSet{subscriberRegion}));
  unsigned char const disjointValueBytes[] = {0x30, 0x25};
  AttributeHandleValueMap disjointValue;
  disjointValue.emplace(
      flavor,
      VariableLengthData(disjointValueBytes, sizeof(disjointValueBytes)));
  REQUIRE_NOTHROW(publisher->updateAttributeValues(
      objectInstance,
      disjointValue,
      VariableLengthData()));
  REQUIRE_FALSE(subscriber->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(subscriberReports.attributeReflectionReports.size() == 2);

  REQUIRE_NOTHROW(publisher->unassociateRegionsForUpdates(objectInstance, regionalPair));
  REQUIRE_NOTHROW(publisher->unassociateRegionsForUpdates(objectInstance, emptyRegionPair));
  REQUIRE_NOTHROW(subscriber->unsubscribeObjectClassAttributesWithRegions(
      soda,
      subscriberPair));
  REQUIRE_NOTHROW(subscriber->unsubscribeObjectClassAttributesWithRegions(
      soda,
      emptyRegionPair));
  REQUIRE_NOTHROW(publisher->deleteRegion(publisherRegion));
  REQUIRE_NOTHROW(subscriber->deleteRegion(subscriberRegion));

  REQUIRE_NOTHROW(subscriber->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(publisher->disconnect());
  REQUIRE_NOTHROW(subscriber->disconnect());
}


}  // namespace
