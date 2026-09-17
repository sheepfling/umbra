#include <catch2/catch_test_macros.hpp>

#include "hla_test_names.hpp"
#include "internal/fom/hla_names.hpp"

#include <RTI/NullFederateAmbassador.h>
#include <RTI/RTI1516.h>

#include <atomic>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#ifndef UMBRA_SOURCE_DIRECTORY
#error "The regional object-attribute routing test requires the Umbra source directory."
#endif

namespace {

using rti1516_2025::AttributeHandle;
using rti1516_2025::AttributeHandleSet;
using rti1516_2025::AttributeHandleSetRegionHandleSetPairVector;
using rti1516_2025::AttributeHandleValueMap;
using rti1516_2025::DimensionHandle;
using rti1516_2025::DimensionHandleSet;
using rti1516_2025::FederateHandle;
using rti1516_2025::HLA_EVOKED;
using rti1516_2025::NO_ACTION;
using rti1516_2025::CANCEL_THEN_DELETE_THEN_DIVEST;
using rti1516_2025::ObjectClassHandle;
using rti1516_2025::ObjectInstanceHandle;
using rti1516_2025::RTIambassador;
using rti1516_2025::RTIambassadorFactory;
using rti1516_2025::RangeBounds;
using rti1516_2025::RegionHandle;
using rti1516_2025::RegionHandleSet;
using rti1516_2025::TransportationTypeHandle;
using rti1516_2025::VariableLengthData;

namespace fixture_hla = umbra::test::hla::wide;

std::wstring nextFederationName() {
  static std::atomic_uint64_t sequence{0U};
  return L"regional-object-attribute-routing-" +
      std::to_wstring(sequence.fetch_add(1U, std::memory_order_relaxed));
}

std::filesystem::path resourcePath(std::filesystem::path const& relativePath) {
  return std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "third_party" /
      "ieee1516.2-2025" / "resources" / relativePath;
}

class ReportingFederateAmbassador final
    : public rti1516_2025::NullFederateAmbassador {
 public:
  struct ReflectionReport final {
    ObjectInstanceHandle objectInstance;
    AttributeHandleValueMap attributeValues;
    bool sentRegionsSupplied = false;
    RegionHandleSet sentRegions;
  };

  void discoverObjectInstance(
      ObjectInstanceHandle const& objectInstance,
      ObjectClassHandle const& objectClass,
      std::wstring const& objectInstanceName,
      FederateHandle const& producingFederate) override {
    objectDiscoveryReports.push_back({
        objectInstance,
        objectClass,
        objectInstanceName,
        producingFederate,
    });
  }

  void reflectAttributeValues(
      ObjectInstanceHandle const& objectInstance,
      AttributeHandleValueMap const& attributeValues,
      VariableLengthData const&,
      TransportationTypeHandle const&,
      FederateHandle const&,
      RegionHandleSet const* optionalSentRegions) override {
    attributeReflectionReports.push_back({
        objectInstance,
        attributeValues,
        optionalSentRegions != nullptr,
        optionalSentRegions == nullptr ? RegionHandleSet{} : *optionalSentRegions,
    });
  }

  struct ObjectDiscoveryReport final {
    ObjectInstanceHandle objectInstance;
    ObjectClassHandle objectClass;
    std::wstring objectInstanceName;
    FederateHandle producingFederate;
  };

  std::vector<ObjectDiscoveryReport> objectDiscoveryReports;
  std::vector<ReflectionReport> attributeReflectionReports;
};

std::unique_ptr<RTIambassador> makeRti() {
  RTIambassadorFactory factory;
  return factory.createRTIambassador();
}

void drainCallbacks(RTIambassador& rti) {
  for (int pass = 0; pass != 128; ++pass) {
    if (!rti.evokeCallback(0.0)) {
      break;
    }
  }
}

}  // namespace

TEST_CASE(
    "Embedded regional object attributes filter 2025 no-time updates by overlap",
    "[integration][development-profile][federation-management][ddm]"
    "[regional-object-attribute-routing]"
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
  drainCallbacks(*subscriber);
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
  drainCallbacks(*subscriber);
  REQUIRE(subscriberReports.attributeReflectionReports.size() == 1);
  REQUIRE_FALSE(subscriberReports.attributeReflectionReports.front().sentRegionsSupplied);
  REQUIRE(
      subscriberReports.attributeReflectionReports.front().attributeValues.contains(flavor));

  // The switch is recipient-local and may be changed after a federation has
  // joined. Enabling it exposes the same update-region realization on the
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
  drainCallbacks(*subscriber);
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
  drainCallbacks(*subscriber);
  REQUIRE(subscriberReports.attributeReflectionReports.size() == 2);

  REQUIRE_NOTHROW(publisher->unassociateRegionsForUpdates(objectInstance, regionalPair));

  // Keep the regional subscription active while removing the source-side
  // association.  This isolates the §9.1.3.2 rule that the (region,
  // attribute) triple is no longer used for subsequent updates; the default
  // region may still route the update, and the later unsubscribe is a
  // separate teardown operation.
  REQUIRE_NOTHROW(subscriber->setRangeBounds(
      subscriberRegion,
      sodaFlavor,
      RangeBounds(0UL, 1UL)));
  REQUIRE_NOTHROW(subscriber->commitRegionModifications(RegionHandleSet{subscriberRegion}));
  drainCallbacks(*subscriber);
  auto const reflectionCountAfterRegionReactivation =
      subscriberReports.attributeReflectionReports.size();
  unsigned char const unassociatedValueBytes[] = {0x35, 0x25};
  AttributeHandleValueMap unassociatedValue;
  unassociatedValue.emplace(
      flavor,
      VariableLengthData(unassociatedValueBytes, sizeof(unassociatedValueBytes)));
  REQUIRE_NOTHROW(publisher->updateAttributeValues(
      objectInstance,
      unassociatedValue,
      VariableLengthData()));
  drainCallbacks(*subscriber);
  REQUIRE(
      subscriberReports.attributeReflectionReports.size() ==
      reflectionCountAfterRegionReactivation + 1);
  REQUIRE(subscriberReports.attributeReflectionReports.back().sentRegionsSupplied);
  REQUIRE_FALSE(
      subscriberReports.attributeReflectionReports.back().sentRegions.contains(
          publisherRegion));

  REQUIRE_NOTHROW(publisher->unassociateRegionsForUpdates(objectInstance, emptyRegionPair));
  REQUIRE_NOTHROW(subscriber->unsubscribeObjectClassAttributesWithRegions(
      soda,
      subscriberPair));
  REQUIRE_NOTHROW(subscriber->unsubscribeObjectClassAttributesWithRegions(
      soda,
      emptyRegionPair));

  // Unsubscription removes the same (object class, region, attribute) route;
  // a later update must not be reflected through the retired subscription.
  unsigned char const unsubscribedValueBytes[] = {0x40, 0x25};
  AttributeHandleValueMap unsubscribedValue;
  unsubscribedValue.emplace(
      flavor,
      VariableLengthData(unsubscribedValueBytes, sizeof(unsubscribedValueBytes)));
  REQUIRE_NOTHROW(publisher->updateAttributeValues(
      objectInstance,
      unsubscribedValue,
      VariableLengthData()));
  drainCallbacks(*subscriber);
  REQUIRE(
      subscriberReports.attributeReflectionReports.size() ==
      reflectionCountAfterRegionReactivation + 1);

  REQUIRE_NOTHROW(publisher->deleteRegion(publisherRegion));
  REQUIRE_NOTHROW(subscriber->deleteRegion(subscriberRegion));

  REQUIRE_NOTHROW(subscriber->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(publisher->disconnect());
  REQUIRE_NOTHROW(subscriber->disconnect());
}

TEST_CASE(
    "Embedded regional object attributes with no common dimensions never overlap",
    "[integration][development-profile][federation-management][object-management][ddm]"
    "[regional-object-attribute-routing][no-common-dimension]"
    "[rti.service.get-available-dimensions-for-object-class]"
    "[rti.service.register-object-instance-with-regions]"
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
  auto const fomModule = std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
      "cpp" / "tests" / "data" / "two-dimensional-regional-interaction-fom.xml";

  REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED));
  REQUIRE_NOTHROW(subscriber->connect(subscriberReports, HLA_EVOKED));
  REQUIRE_NOTHROW(publisher->createFederationExecution(
      federationName, fomModule.wstring(), L"HLAinteger64Time"));
  REQUIRE_NOTHROW(publisher->joinFederationExecution(
      L"no-common-dimension-publisher", L"publisher", federationName));
  REQUIRE_NOTHROW(subscriber->joinFederationExecution(
      L"no-common-dimension-subscriber", L"subscriber", federationName));

  auto const publisherObjectClass = publisher->getObjectClassHandle(
      fixture_hla::fom::two_dimensional_regional_object);
  auto const subscriberObjectClass = subscriber->getObjectClassHandle(
      fixture_hla::fom::two_dimensional_regional_object);
  auto const publisherAttribute = publisher->getAttributeHandle(
      publisherObjectClass, fixture_hla::fixture::value);
  auto const subscriberAttribute = subscriber->getAttributeHandle(
      subscriberObjectClass, fixture_hla::fixture::value);
  auto const publisherX = publisher->getDimensionHandle(
      fixture_hla::fixture::umbra_region_x);
  auto const publisherY = publisher->getDimensionHandle(
      fixture_hla::fixture::umbra_region_y);
  auto const subscriberX = subscriber->getDimensionHandle(
      fixture_hla::fixture::umbra_region_x);
  auto const subscriberY = subscriber->getDimensionHandle(
      fixture_hla::fixture::umbra_region_y);
  REQUIRE(publisherObjectClass.isValid());
  REQUIRE(subscriberObjectClass.isValid());
  REQUIRE(publisherAttribute.isValid());
  REQUIRE(subscriberAttribute.isValid());
  REQUIRE(publisherX.isValid());
  REQUIRE(publisherY.isValid());
  REQUIRE(subscriberX.isValid());
  REQUIRE(subscriberY.isValid());
  REQUIRE(publisher->getAvailableDimensionsForObjectClass(publisherObjectClass) ==
          DimensionHandleSet{publisherX, publisherY});
  REQUIRE(subscriber->getAvailableDimensionsForObjectClass(subscriberObjectClass) ==
          DimensionHandleSet{subscriberX, subscriberY});

  AttributeHandleSet const publisherAttributes{publisherAttribute};
  AttributeHandleSet const subscriberAttributes{subscriberAttribute};
  REQUIRE_NOTHROW(publisher->publishObjectClassAttributes(
      publisherObjectClass, publisherAttributes));

  // The source uses X while the subscription uses Y.  Both dimensions are
  // valid for the class, but the region realizations have no dimension in
  // common and therefore cannot overlap under §9.1.4.
  auto const sourceRegion = publisher->createRegion(DimensionHandleSet{publisherX});
  auto const disjointSubscriptionRegion =
      subscriber->createRegion(DimensionHandleSet{subscriberY});
  REQUIRE_NOTHROW(publisher->setRangeBounds(
      sourceRegion, publisherX, RangeBounds(0UL, 2UL)));
  REQUIRE_NOTHROW(subscriber->setRangeBounds(
      disjointSubscriptionRegion, subscriberY, RangeBounds(0UL, 2UL)));
  REQUIRE_NOTHROW(publisher->commitRegionModifications(RegionHandleSet{sourceRegion}));
  REQUIRE_NOTHROW(subscriber->commitRegionModifications(
      RegionHandleSet{disjointSubscriptionRegion}));
  AttributeHandleSetRegionHandleSetPairVector const sourcePair{{
      publisherAttributes,
      RegionHandleSet{sourceRegion},
  }};
  AttributeHandleSetRegionHandleSetPairVector const disjointSubscriptionPair{{
      subscriberAttributes,
      RegionHandleSet{disjointSubscriptionRegion},
  }};
  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = publisher->registerObjectInstanceWithRegions(
      publisherObjectClass, sourcePair));
  REQUIRE(objectInstance.isValid());
  REQUIRE_NOTHROW(subscriber->subscribeObjectClassAttributesWithRegions(
      subscriberObjectClass, disjointSubscriptionPair));
  drainCallbacks(*subscriber);
  REQUIRE(subscriberReports.objectDiscoveryReports.empty());

  unsigned char const firstValueBytes[] = {'n', 'o', '-', 'c', 'o', 'm', 'm', 'o', 'n'};
  AttributeHandleValueMap firstValue;
  firstValue.emplace(
      publisherAttribute,
      VariableLengthData(firstValueBytes, sizeof(firstValueBytes)));
  REQUIRE_NOTHROW(publisher->updateAttributeValues(
      objectInstance, firstValue, VariableLengthData()));
  drainCallbacks(*subscriber);
  REQUIRE(subscriberReports.attributeReflectionReports.empty());

  // Replace the subscription with a region on X.  This control establishes
  // that the object and update association remain usable after the no-common
  // dimension was correctly suppressed.
  REQUIRE_NOTHROW(subscriber->unsubscribeObjectClassAttributesWithRegions(
      subscriberObjectClass, disjointSubscriptionPair));
  REQUIRE_NOTHROW(subscriber->deleteRegion(disjointSubscriptionRegion));
  auto const matchingSubscriptionRegion =
      subscriber->createRegion(DimensionHandleSet{subscriberX});
  REQUIRE_NOTHROW(subscriber->setRangeBounds(
      matchingSubscriptionRegion, subscriberX, RangeBounds(0UL, 2UL)));
  REQUIRE_NOTHROW(subscriber->commitRegionModifications(
      RegionHandleSet{matchingSubscriptionRegion}));
  AttributeHandleSetRegionHandleSetPairVector const matchingSubscriptionPair{{
      subscriberAttributes,
      RegionHandleSet{matchingSubscriptionRegion},
  }};
  REQUIRE_NOTHROW(subscriber->subscribeObjectClassAttributesWithRegions(
      subscriberObjectClass, matchingSubscriptionPair));
  drainCallbacks(*subscriber);
  REQUIRE(subscriberReports.objectDiscoveryReports.size() == 1U);
  REQUIRE(subscriberReports.objectDiscoveryReports.front().objectInstance == objectInstance);

  unsigned char const secondValueBytes[] = {'m', 'a', 't', 'c', 'h'};
  AttributeHandleValueMap secondValue;
  secondValue.emplace(
      publisherAttribute,
      VariableLengthData(secondValueBytes, sizeof(secondValueBytes)));
  REQUIRE_NOTHROW(publisher->updateAttributeValues(
      objectInstance, secondValue, VariableLengthData()));
  drainCallbacks(*subscriber);
  REQUIRE(subscriberReports.attributeReflectionReports.size() == 1U);
  REQUIRE(subscriberReports.attributeReflectionReports.front().attributeValues.contains(
      subscriberAttribute));

  REQUIRE_NOTHROW(publisher->unassociateRegionsForUpdates(objectInstance, sourcePair));
  REQUIRE_NOTHROW(subscriber->unsubscribeObjectClassAttributesWithRegions(
      subscriberObjectClass, matchingSubscriptionPair));
  REQUIRE_NOTHROW(subscriber->deleteRegion(matchingSubscriptionRegion));
  REQUIRE_NOTHROW(publisher->deleteRegion(sourceRegion));
  REQUIRE_NOTHROW(subscriber->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(subscriber->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
}

TEST_CASE(
    "Embedded regional object services reject region dimensions outside available object dimensions",
    "[integration][development-profile][object-management][ddm]"
    "[regional-object-attribute-region-context]"
    "[rti.service.register-object-instance-with-regions]"
    "[rti.service.associate-regions-for-updates]"
    "[rti.service.subscribe-object-class-attributes-with-regions]"
    "[rti.service.request-attribute-value-update-with-regions]") {
  ReportingFederateAmbassador publisherReports;
  ReportingFederateAmbassador subscriberReports;
  auto publisher = makeRti();
  auto subscriber = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule =
      resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();

  REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED));
  REQUIRE_NOTHROW(subscriber->connect(subscriberReports, HLA_EVOKED));
  REQUIRE_NOTHROW(publisher->createFederationExecution(
      federationName, fomModule, L"HLAinteger64Time"));
  REQUIRE_NOTHROW(publisher->joinFederationExecution(
      L"regional-object-context-publisher", L"publisher", federationName));
  REQUIRE_NOTHROW(subscriber->joinFederationExecution(
      L"regional-object-context-subscriber", L"subscriber", federationName));

  auto const soda = publisher->getObjectClassHandle(L"HLAobjectRoot.Food.Drink.Soda");
  auto const flavor = publisher->getAttributeHandle(soda, L"Flavor");
  auto const sodaFlavor = publisher->getDimensionHandle(L"SodaFlavor");
  auto const serverId = publisher->getDimensionHandle(L"ServerId");
  REQUIRE(soda.isValid());
  REQUIRE(flavor.isValid());
  REQUIRE(sodaFlavor.isValid());
  REQUIRE(serverId.isValid());
  AttributeHandleSet const flavorOnly{flavor};
  REQUIRE_NOTHROW(publisher->publishObjectClassAttributes(soda, flavorOnly));

  auto const validRegion = publisher->createRegion(DimensionHandleSet{sodaFlavor});
  auto const wrongPublisherRegion =
      publisher->createRegion(DimensionHandleSet{serverId});
  auto const wrongSubscriberRegion =
      subscriber->createRegion(DimensionHandleSet{serverId});
  REQUIRE_NOTHROW(publisher->setRangeBounds(
      validRegion, sodaFlavor, RangeBounds(0UL, 1UL)));
  REQUIRE_NOTHROW(publisher->commitRegionModifications(
      RegionHandleSet{validRegion}));
  REQUIRE_NOTHROW(publisher->setRangeBounds(
      wrongPublisherRegion, serverId, RangeBounds(0UL, 1UL)));
  REQUIRE_NOTHROW(publisher->commitRegionModifications(
      RegionHandleSet{wrongPublisherRegion}));
  REQUIRE_NOTHROW(subscriber->setRangeBounds(
      wrongSubscriberRegion, serverId, RangeBounds(0UL, 1UL)));
  REQUIRE_NOTHROW(subscriber->commitRegionModifications(
      RegionHandleSet{wrongSubscriberRegion}));

  AttributeHandleSetRegionHandleSetPairVector const validPair{{
      flavorOnly,
      RegionHandleSet{validRegion},
  }};
  AttributeHandleSetRegionHandleSetPairVector const wrongPublisherPair{{
      flavorOnly,
      RegionHandleSet{wrongPublisherRegion},
  }};
  AttributeHandleSetRegionHandleSetPairVector const wrongSubscriberPair{{
      flavorOnly,
      RegionHandleSet{wrongSubscriberRegion},
  }};

  REQUIRE_THROWS_AS(
      publisher->registerObjectInstanceWithRegions(soda, wrongPublisherPair),
      rti1516_2025::InvalidRegionContext);
  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = publisher->registerObjectInstanceWithRegions(
      soda, validPair));
  REQUIRE(objectInstance.isValid());
  REQUIRE_THROWS_AS(
      publisher->associateRegionsForUpdates(objectInstance, wrongPublisherPair),
      rti1516_2025::InvalidRegionContext);
  REQUIRE_THROWS_AS(
      subscriber->subscribeObjectClassAttributesWithRegions(
          soda, wrongSubscriberPair),
      rti1516_2025::InvalidRegionContext);
  unsigned char const tagBytes[] = {0x90, 0x25};
  REQUIRE_THROWS_AS(
      subscriber->requestAttributeValueUpdateWithRegions(
          soda,
          wrongSubscriberPair,
          VariableLengthData(tagBytes, sizeof(tagBytes))),
      rti1516_2025::InvalidRegionContext);

  REQUIRE_NOTHROW(publisher->unassociateRegionsForUpdates(objectInstance, validPair));
  REQUIRE_NOTHROW(publisher->deleteRegion(validRegion));
  REQUIRE_NOTHROW(publisher->deleteRegion(wrongPublisherRegion));
  REQUIRE_NOTHROW(subscriber->deleteRegion(wrongSubscriberRegion));
  REQUIRE_NOTHROW(subscriber->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(
      CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(publisher->disconnect());
  REQUIRE_NOTHROW(subscriber->disconnect());
}

TEST_CASE(
    "Embedded zero-dimensional region realizations do not overlap the default region",
    "[integration][development-profile][federation-management][object-management][ddm]"
    "[zero-dimensional-region][regional-object-attribute-routing]"
    "[rti.service.create-region][rti.service.commit-region-modifications]"
    "[rti.service.register-object-instance-with-regions]"
    "[rti.service.subscribe-object-class-attributes]"
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
      L"zero-dimensional-publisher", L"publisher", federationName));
  REQUIRE_NOTHROW(subscriber->joinFederationExecution(
      L"zero-dimensional-subscriber", L"subscriber", federationName));

  auto const soda = publisher->getObjectClassHandle(L"HLAobjectRoot.Food.Drink.Soda");
  auto const flavor = publisher->getAttributeHandle(soda, L"Flavor");
  REQUIRE(soda.isValid());
  REQUIRE(flavor.isValid());
  AttributeHandleSet const flavorOnly{flavor};
  REQUIRE_NOTHROW(publisher->publishObjectClassAttributes(soda, flavorOnly));
  REQUIRE_NOTHROW(subscriber->subscribeObjectClassAttributes(soda, flavorOnly));

  // IEEE 1516.1-2025 §9.1.3.2 permits a zero-dimensional realization, but
  // explicitly says it does not overlap any other realization, including the
  // invisible default region.
  auto const zeroRegion = publisher->createRegion(DimensionHandleSet{});
  REQUIRE(zeroRegion.isValid());
  REQUIRE(publisher->getDimensionHandleSet(zeroRegion).empty());
  REQUIRE_NOTHROW(publisher->commitRegionModifications(RegionHandleSet{zeroRegion}));
  AttributeHandleSetRegionHandleSetPairVector const zeroRegionPair{{
      flavorOnly,
      RegionHandleSet{zeroRegion},
  }};

  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = publisher->registerObjectInstanceWithRegions(
      soda,
      zeroRegionPair));
  REQUIRE(objectInstance.isValid());
  REQUIRE_FALSE(subscriber->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(subscriberReports.objectDiscoveryReports.empty());

  unsigned char const valueBytes[] = {0x51, 0x25};
  AttributeHandleValueMap value;
  value.emplace(flavor, VariableLengthData(valueBytes, sizeof(valueBytes)));
  REQUIRE_NOTHROW(publisher->updateAttributeValues(
      objectInstance,
      value,
      VariableLengthData()));
  REQUIRE_FALSE(subscriber->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(subscriberReports.attributeReflectionReports.empty());

  REQUIRE_NOTHROW(publisher->unassociateRegionsForUpdates(objectInstance, zeroRegionPair));
  REQUIRE_NOTHROW(publisher->deleteRegion(zeroRegion));
  REQUIRE_NOTHROW(subscriber->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(
      CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(publisher->disconnect());
  REQUIRE_NOTHROW(subscriber->disconnect());
}
