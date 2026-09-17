#include <catch2/catch_test_macros.hpp>

#include "internal/fom/hla_names.hpp"
#include "hla_test_names.hpp"

#include <RTI/NullFederateAmbassador.h>
#include <RTI/RTI1516.h>

#include <atomic>
#include <filesystem>
#include <memory>
#include <vector>

#ifndef UMBRA_SOURCE_DIRECTORY
#error "The passive object-attribute subscription test requires the Umbra source directory."
#endif

namespace {

namespace standard_hla = umbra::detail::hla::wide;
namespace fixture_hla = umbra::test::hla::wide;

using rti1516_2025::AttributeHandle;
using rti1516_2025::AttributeHandleSet;
using rti1516_2025::AttributeHandleSetRegionHandleSetPairVector;
using rti1516_2025::AttributeHandleValueMap;
using rti1516_2025::DimensionHandleSet;
using rti1516_2025::FederateHandle;
using rti1516_2025::HLA_EVOKED;
using rti1516_2025::ObjectClassHandle;
using rti1516_2025::ObjectInstanceHandle;
using rti1516_2025::RTIambassador;
using rti1516_2025::RTIambassadorFactory;
using rti1516_2025::RegionHandle;
using rti1516_2025::RegionHandleSet;
using rti1516_2025::RangeBounds;
using rti1516_2025::TransportationTypeHandle;
using rti1516_2025::VariableLengthData;

std::wstring nextFederationName() {
  static std::atomic_uint64_t sequence{0U};
  return L"passive-object-attribute-subscription-" +
      std::to_wstring(sequence.fetch_add(1U, std::memory_order_relaxed));
}

std::filesystem::path resourcePath(std::filesystem::path const& relativePath) {
  return std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "third_party" /
      "ieee1516.2-2025" / "resources" / relativePath;
}

class ReportingFederateAmbassador final
    : public rti1516_2025::NullFederateAmbassador {
 public:
  struct ObjectDiscoveryReport final {
    ObjectInstanceHandle objectInstance;
    ObjectClassHandle objectClass;
    std::wstring objectInstanceName;
    FederateHandle producingFederate;
  };

  struct ReflectionReport final {
    ObjectInstanceHandle objectInstance;
    AttributeHandleValueMap attributeValues;
    VariableLengthData userSuppliedTag;
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
      VariableLengthData const& userSuppliedTag,
      TransportationTypeHandle const& transportationType,
      FederateHandle const& producingFederate,
      RegionHandleSet const* optionalSentRegions) override {
    static_cast<void>(transportationType);
    static_cast<void>(producingFederate);
    attributeReflectionReports.push_back({
        objectInstance,
        attributeValues,
        userSuppliedTag,
        optionalSentRegions != nullptr,
        optionalSentRegions == nullptr ? RegionHandleSet{} : *optionalSentRegions,
    });
  }

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
    "Embedded passive object-attribute subscriptions do not arrange ordinary or regional delivery",
    "[integration][development-profile][object-management][ddm][passive-subscription]"
    "[rti.service.subscribe-object-class-attributes]"
    "[rti.service.subscribe-object-class-attributes-with-regions]"
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
      L"passive-attribute-publisher", L"publisher", federationName));
  REQUIRE_NOTHROW(subscriber->joinFederationExecution(
      L"passive-attribute-subscriber", L"subscriber", federationName));

  auto const soda = publisher->getObjectClassHandle(
      fixture_hla::fom::food_drink_soda);
  auto const flavor = publisher->getAttributeHandle(
      soda,
      fixture_hla::fixture::flavor);
  auto const sodaFlavor = publisher->getDimensionHandle(
      fixture_hla::fixture::soda_flavor);
  REQUIRE(soda.isValid());
  REQUIRE(flavor.isValid());
  REQUIRE(sodaFlavor.isValid());
  AttributeHandleSet const flavorOnly{flavor};
  REQUIRE_NOTHROW(publisher->publishObjectClassAttributes(soda, flavorOnly));

  auto const publisherRegion = publisher->createRegion(
      DimensionHandleSet{sodaFlavor});
  auto const subscriberRegion = subscriber->createRegion(
      DimensionHandleSet{sodaFlavor});
  REQUIRE_NOTHROW(publisher->setRangeBounds(
      publisherRegion,
      sodaFlavor,
      RangeBounds(0UL, 1UL)));
  REQUIRE_NOTHROW(publisher->commitRegionModifications(
      RegionHandleSet{publisherRegion}));
  REQUIRE_NOTHROW(subscriber->setRangeBounds(
      subscriberRegion,
      sodaFlavor,
      RangeBounds(0UL, 1UL)));
  REQUIRE_NOTHROW(subscriber->commitRegionModifications(
      RegionHandleSet{subscriberRegion}));

  AttributeHandleSetRegionHandleSetPairVector const publisherPair{{
      flavorOnly,
      RegionHandleSet{publisherRegion},
  }};
  AttributeHandleSetRegionHandleSetPairVector const subscriberPair{{
      flavorOnly,
      RegionHandleSet{subscriberRegion},
  }};

  // Passive ordinary declarations are retained, but do not arrange instance
  // discovery. Replacing the same declaration with an active one does.
  REQUIRE_NOTHROW(subscriber->subscribeObjectClassAttributes(soda, flavorOnly, false));
  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = publisher->registerObjectInstanceWithRegions(
      soda,
      publisherPair));
  drainCallbacks(*subscriber);
  REQUIRE(subscriberReports.objectDiscoveryReports.empty());
  REQUIRE_THROWS_AS(
      subscriber->getKnownObjectClassHandle(objectInstance),
      rti1516_2025::ObjectInstanceNotKnown);

  REQUIRE_NOTHROW(subscriber->subscribeObjectClassAttributes(soda, flavorOnly, true));
  drainCallbacks(*subscriber);
  REQUIRE(subscriberReports.objectDiscoveryReports.size() == 1U);
  REQUIRE(subscriberReports.objectDiscoveryReports.front().objectInstance == objectInstance);
  REQUIRE(subscriber->getKnownObjectClassHandle(objectInstance) == soda);

  unsigned char const passiveOrdinaryBytes[] = {0x10, 0x20};
  AttributeHandleValueMap passiveOrdinaryValues;
  passiveOrdinaryValues.emplace(
      flavor,
      VariableLengthData(passiveOrdinaryBytes, sizeof(passiveOrdinaryBytes)));
  REQUIRE_NOTHROW(subscriber->subscribeObjectClassAttributes(soda, flavorOnly, false));
  drainCallbacks(*subscriber);
  REQUIRE_NOTHROW(publisher->updateAttributeValues(
      objectInstance,
      passiveOrdinaryValues,
      VariableLengthData()));
  drainCallbacks(*subscriber);
  REQUIRE(subscriberReports.attributeReflectionReports.empty());

  unsigned char const activeOrdinaryBytes[] = {0x30, 0x40};
  AttributeHandleValueMap activeOrdinaryValues;
  activeOrdinaryValues.emplace(
      flavor,
      VariableLengthData(activeOrdinaryBytes, sizeof(activeOrdinaryBytes)));
  REQUIRE_NOTHROW(subscriber->subscribeObjectClassAttributes(soda, flavorOnly, true));
  drainCallbacks(*subscriber);
  REQUIRE_NOTHROW(publisher->updateAttributeValues(
      objectInstance,
      activeOrdinaryValues,
      VariableLengthData()));
  drainCallbacks(*subscriber);
  REQUIRE(subscriberReports.attributeReflectionReports.size() == 1U);
  REQUIRE(subscriberReports.attributeReflectionReports.front().objectInstance == objectInstance);
  REQUIRE(
      subscriberReports.attributeReflectionReports.front().attributeValues.contains(flavor));

  // The same active/passive rule applies to an overlapping regional pair.
  // Ordinary and regional declarations are independent, so remove the
  // ordinary declaration before exercising the regional state.
  REQUIRE_NOTHROW(subscriber->unsubscribeObjectClassAttributes(soda, flavorOnly));
  drainCallbacks(*subscriber);
  REQUIRE_NOTHROW(subscriber->subscribeObjectClassAttributesWithRegions(
      soda,
      subscriberPair,
      false));
  drainCallbacks(*subscriber);

  ObjectInstanceHandle passiveRegionalObject;
  REQUIRE_NOTHROW(passiveRegionalObject = publisher->registerObjectInstanceWithRegions(
      soda,
      publisherPair));
  drainCallbacks(*subscriber);
  REQUIRE(subscriberReports.objectDiscoveryReports.size() == 1U);
  REQUIRE_THROWS_AS(
      subscriber->getKnownObjectClassHandle(passiveRegionalObject),
      rti1516_2025::ObjectInstanceNotKnown);

  unsigned char const passiveRegionalBytes[] = {0x50, 0x60};
  AttributeHandleValueMap passiveRegionalValues;
  passiveRegionalValues.emplace(
      flavor,
      VariableLengthData(passiveRegionalBytes, sizeof(passiveRegionalBytes)));
  REQUIRE_NOTHROW(publisher->updateAttributeValues(
      objectInstance,
      passiveRegionalValues,
      VariableLengthData()));
  drainCallbacks(*subscriber);
  REQUIRE(subscriberReports.attributeReflectionReports.size() == 1U);

  unsigned char const activeRegionalBytes[] = {0x70, 0x80};
  AttributeHandleValueMap activeRegionalValues;
  activeRegionalValues.emplace(
      flavor,
      VariableLengthData(activeRegionalBytes, sizeof(activeRegionalBytes)));
  REQUIRE_NOTHROW(subscriber->subscribeObjectClassAttributesWithRegions(
      soda,
      subscriberPair,
      true));
  drainCallbacks(*subscriber);
  REQUIRE(subscriberReports.objectDiscoveryReports.size() == 2U);
  REQUIRE(subscriberReports.objectDiscoveryReports.back().objectInstance == passiveRegionalObject);
  REQUIRE(subscriber->getKnownObjectClassHandle(passiveRegionalObject) == soda);
  REQUIRE_NOTHROW(publisher->updateAttributeValues(
      objectInstance,
      activeRegionalValues,
      VariableLengthData()));
  drainCallbacks(*subscriber);
  REQUIRE(subscriberReports.attributeReflectionReports.size() == 2U);
  REQUIRE(subscriberReports.attributeReflectionReports.back().objectInstance == objectInstance);
  REQUIRE(subscriberReports.attributeReflectionReports.back().attributeValues.contains(flavor));

  REQUIRE_NOTHROW(publisher->unassociateRegionsForUpdates(objectInstance, publisherPair));
  REQUIRE_NOTHROW(publisher->unassociateRegionsForUpdates(passiveRegionalObject, publisherPair));
  REQUIRE_NOTHROW(subscriber->unsubscribeObjectClassAttributesWithRegions(
      soda,
      subscriberPair));
  REQUIRE_NOTHROW(publisher->deleteRegion(publisherRegion));
  REQUIRE_NOTHROW(subscriber->deleteRegion(subscriberRegion));
  REQUIRE_NOTHROW(subscriber->resignFederationExecution(
      rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(
      rti1516_2025::CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(publisher->disconnect());
  REQUIRE_NOTHROW(subscriber->disconnect());
}
