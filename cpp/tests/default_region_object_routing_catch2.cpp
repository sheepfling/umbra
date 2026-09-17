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
#error "The default-region object-routing test requires the Umbra source directory."
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
using rti1516_2025::RegionHandle;
using rti1516_2025::RegionHandleSet;
using rti1516_2025::RangeBounds;
using rti1516_2025::RTIambassador;
using rti1516_2025::RTIambassadorFactory;
using rti1516_2025::TransportationTypeHandle;
using rti1516_2025::VariableLengthData;

std::wstring nextFederationName() {
  static std::atomic_uint64_t sequence{0U};
  return L"default-region-object-routing-" +
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
    "Embedded default-region object routing derives 2025 ordinary and regional effectiveness",
    "[integration][development-profile][object-management][ddm][default-region]"
    "[rti.service.register-object-instance-with-regions]"
    "[rti.service.associate-regions-for-updates]"
    "[rti.service.unassociate-regions-for-updates]"
    "[rti.service.subscribe-object-class-attributes]"
    "[rti.service.subscribe-object-class-attributes-with-regions]"
    "[rti.service.unsubscribe-object-class-attributes-with-regions]"
    "[rti.service.update-attribute-values]"
    "[federate.callback.discover-object-instance]"
    "[federate.callback.reflect-attribute-values]") {
  ReportingFederateAmbassador publisherReports;
  ReportingFederateAmbassador regionalReports;
  ReportingFederateAmbassador mixedReports;
  auto publisher = makeRti();
  auto regional = makeRti();
  auto mixed = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath(
      "examples/RestaurantFOMmodule-2025.xml").wstring();

  REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED));
  REQUIRE_NOTHROW(regional->connect(regionalReports, HLA_EVOKED));
  REQUIRE_NOTHROW(mixed->connect(mixedReports, HLA_EVOKED));
  REQUIRE_NOTHROW(publisher->createFederationExecution(
      federationName,
      fomModule,
      standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(publisher->joinFederationExecution(
      L"default-region-object-publisher", L"publisher", federationName));
  REQUIRE_NOTHROW(regional->joinFederationExecution(
      L"default-region-object-regional", L"subscriber", federationName));
  REQUIRE_NOTHROW(mixed->joinFederationExecution(
      L"default-region-object-mixed", L"subscriber", federationName));

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
  REQUIRE_NOTHROW(publisher->publishObjectClassAttributes(
      soda,
      flavorOnly));

  auto const sourceRegion = publisher->createRegion(
      DimensionHandleSet{sodaFlavor});
  auto const regionalRegion = regional->createRegion(
      DimensionHandleSet{sodaFlavor});
  auto const mixedRegion = mixed->createRegion(
      DimensionHandleSet{sodaFlavor});
  REQUIRE_NOTHROW(publisher->setRangeBounds(
      sourceRegion,
      sodaFlavor,
      RangeBounds(0UL, 1UL)));
  REQUIRE_NOTHROW(publisher->commitRegionModifications(
      RegionHandleSet{sourceRegion}));
  REQUIRE_NOTHROW(regional->setRangeBounds(
      regionalRegion,
      sodaFlavor,
      RangeBounds(0UL, 1UL)));
  REQUIRE_NOTHROW(regional->commitRegionModifications(
      RegionHandleSet{regionalRegion}));
  REQUIRE_NOTHROW(mixed->setRangeBounds(
      mixedRegion,
      sodaFlavor,
      RangeBounds(2UL, 3UL)));
  REQUIRE_NOTHROW(mixed->commitRegionModifications(
      RegionHandleSet{mixedRegion}));

  AttributeHandleSetRegionHandleSetPairVector const sourcePair{{
      flavorOnly,
      RegionHandleSet{sourceRegion},
  }};
  AttributeHandleSetRegionHandleSetPairVector const regionalPair{{
      flavorOnly,
      RegionHandleSet{regionalRegion},
  }};
  AttributeHandleSetRegionHandleSetPairVector const mixedPair{{
      flavorOnly,
      RegionHandleSet{mixedRegion},
  }};

  REQUIRE_NOTHROW(regional->subscribeObjectClassAttributesWithRegions(
      soda,
      regionalPair));
  REQUIRE_NOTHROW(mixed->subscribeObjectClassAttributes(
      soda,
      flavorOnly,
      true));
  REQUIRE_NOTHROW(mixed->subscribeObjectClassAttributesWithRegions(
      soda,
      mixedPair));

  // The explicit source overlaps only the regional subscriber. The mixed
  // subscriber's retained ordinary declaration is not allowed to use the
  // default region while its disjoint explicit declaration exists.
  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = publisher->registerObjectInstanceWithRegions(
      soda,
      sourcePair));
  drainCallbacks(*regional);
  drainCallbacks(*mixed);
  REQUIRE(regionalReports.objectDiscoveryReports.size() == 1U);
  REQUIRE(regionalReports.objectDiscoveryReports.back().objectInstance == objectInstance);
  REQUIRE(mixedReports.objectDiscoveryReports.empty());

  // Removing the mixed explicit declaration lets its retained ordinary
  // declaration use the default region again and requests fresh discovery.
  REQUIRE_NOTHROW(mixed->unsubscribeObjectClassAttributesWithRegions(
      soda,
      mixedPair));
  REQUIRE_NOTHROW(mixed->subscribeObjectClassAttributes(
      soda,
      flavorOnly,
      true));
  drainCallbacks(*mixed);
  REQUIRE(mixedReports.objectDiscoveryReports.size() == 1U);
  REQUIRE(mixedReports.objectDiscoveryReports.back().objectInstance == objectInstance);

  // Reapply the disjoint regional declaration and remove the source's only
  // explicit association. Both regional subscribers now overlap the invisible
  // default source region.
  REQUIRE_NOTHROW(mixed->subscribeObjectClassAttributesWithRegions(
      soda,
      mixedPair));
  REQUIRE_NOTHROW(publisher->unassociateRegionsForUpdates(
      objectInstance,
      sourcePair));
  drainCallbacks(*regional);
  drainCallbacks(*mixed);
  REQUIRE_NOTHROW(regional->setConveyRegionDesignatorSetsSwitch(true));
  REQUIRE_NOTHROW(mixed->setConveyRegionDesignatorSetsSwitch(true));

  unsigned char const defaultValueBytes[] = {0xD0, 0x01};
  AttributeHandleValueMap defaultValues;
  defaultValues.emplace(
      flavor,
      VariableLengthData(defaultValueBytes, sizeof(defaultValueBytes)));
  REQUIRE_NOTHROW(publisher->updateAttributeValues(
      objectInstance,
      defaultValues,
      VariableLengthData()));
  drainCallbacks(*regional);
  drainCallbacks(*mixed);
  REQUIRE(regionalReports.attributeReflectionReports.size() == 1U);
  REQUIRE(mixedReports.attributeReflectionReports.size() == 1U);
  REQUIRE(regionalReports.attributeReflectionReports.back().sentRegionsSupplied);
  REQUIRE(regionalReports.attributeReflectionReports.back().sentRegions.empty());
  REQUIRE(mixedReports.attributeReflectionReports.back().sentRegionsSupplied);
  REQUIRE(mixedReports.attributeReflectionReports.back().sentRegions.empty());

  // A later explicit association replaces the default source for this
  // attribute. Only the matching regional subscriber receives the update.
  REQUIRE_NOTHROW(publisher->associateRegionsForUpdates(
      objectInstance,
      sourcePair));
  unsigned char const explicitValueBytes[] = {0xE0, 0x02};
  AttributeHandleValueMap explicitValues;
  explicitValues.emplace(
      flavor,
      VariableLengthData(explicitValueBytes, sizeof(explicitValueBytes)));
  REQUIRE_NOTHROW(publisher->updateAttributeValues(
      objectInstance,
      explicitValues,
      VariableLengthData()));
  drainCallbacks(*regional);
  drainCallbacks(*mixed);
  REQUIRE(regionalReports.attributeReflectionReports.size() == 2U);
  REQUIRE(regionalReports.attributeReflectionReports.back().sentRegionsSupplied);
  REQUIRE(regionalReports.attributeReflectionReports.back().sentRegions.contains(sourceRegion));
  REQUIRE(mixedReports.attributeReflectionReports.size() == 1U);

  REQUIRE_NOTHROW(publisher->unassociateRegionsForUpdates(
      objectInstance,
      sourcePair));
  unsigned char const restoredDefaultBytes[] = {0xD0, 0x03};
  AttributeHandleValueMap restoredDefaultValues;
  restoredDefaultValues.emplace(
      flavor,
      VariableLengthData(restoredDefaultBytes, sizeof(restoredDefaultBytes)));
  REQUIRE_NOTHROW(publisher->updateAttributeValues(
      objectInstance,
      restoredDefaultValues,
      VariableLengthData()));
  drainCallbacks(*regional);
  drainCallbacks(*mixed);
  REQUIRE(regionalReports.attributeReflectionReports.size() == 3U);
  REQUIRE(mixedReports.attributeReflectionReports.size() == 2U);
  REQUIRE(regionalReports.attributeReflectionReports.back().sentRegionsSupplied);
  REQUIRE(regionalReports.attributeReflectionReports.back().sentRegions.empty());
  REQUIRE(mixedReports.attributeReflectionReports.back().sentRegionsSupplied);
  REQUIRE(mixedReports.attributeReflectionReports.back().sentRegions.empty());

  REQUIRE_NOTHROW(mixed->unsubscribeObjectClassAttributesWithRegions(
      soda,
      mixedPair));
  REQUIRE_NOTHROW(mixed->unsubscribeObjectClassAttributes(
      soda,
      flavorOnly));
  REQUIRE_NOTHROW(regional->unsubscribeObjectClassAttributesWithRegions(
      soda,
      regionalPair));
  REQUIRE_NOTHROW(publisher->deleteRegion(sourceRegion));
  REQUIRE_NOTHROW(regional->deleteRegion(regionalRegion));
  REQUIRE_NOTHROW(mixed->deleteRegion(mixedRegion));
  REQUIRE_NOTHROW(mixed->resignFederationExecution(
      rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(regional->resignFederationExecution(
      rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(
      rti1516_2025::CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(mixed->disconnect());
  REQUIRE_NOTHROW(regional->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
}
