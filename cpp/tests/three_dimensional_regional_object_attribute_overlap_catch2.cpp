#include <catch2/catch_test_macros.hpp>

#include "internal/fom/hla_names.hpp"
#include "hla_test_names.hpp"

#include <RTI/NullFederateAmbassador.h>
#include <RTI/RTI1516.h>

#include <atomic>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#ifndef UMBRA_SOURCE_DIRECTORY
#error "The three-dimensional DDM test requires the Umbra source directory."
#endif

namespace {

namespace standard_hla = umbra::detail::hla::wide;
namespace fixture_hla = umbra::test::hla::wide;

using rti1516_2025::AttributeHandle;
using rti1516_2025::AttributeHandleSet;
using rti1516_2025::AttributeHandleSetRegionHandleSetPairVector;
using rti1516_2025::AttributeHandleValueMap;
using rti1516_2025::DimensionHandle;
using rti1516_2025::DimensionHandleSet;
using rti1516_2025::FederateHandle;
using rti1516_2025::HLA_EVOKED;
using rti1516_2025::ObjectClassHandle;
using rti1516_2025::ObjectInstanceHandle;
using rti1516_2025::RTIambassador;
using rti1516_2025::RTIambassadorFactory;
using rti1516_2025::RangeBounds;
using rti1516_2025::RegionHandle;
using rti1516_2025::RegionHandleSet;
using rti1516_2025::TransportationTypeHandle;
using rti1516_2025::VariableLengthData;

std::wstring nextFederationName() {
  static std::atomic_uint64_t sequence{0U};
  return L"three-dimensional-regional-object-attribute-overlap-" +
      std::to_wstring(sequence.fetch_add(1U, std::memory_order_relaxed));
}

std::filesystem::path restaurantFomPath() {
  return std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "third_party" /
      "ieee1516.2-2025" / "resources" / "examples" /
      "RestaurantFOMmodule-2025.xml";
}

class ReportingFederateAmbassador final
    : public rti1516_2025::NullFederateAmbassador {
 public:
  struct AttributeReflectionReport final {
    ObjectInstanceHandle objectInstance;
    AttributeHandleValueMap attributeValues;
    TransportationTypeHandle transportationType;
    FederateHandle producingFederate;
    bool sentRegionsSupplied = false;
    RegionHandleSet sentRegions;
  };

  void discoverObjectInstance(
      ObjectInstanceHandle const& objectInstance,
      ObjectClassHandle const&,
      std::wstring const&,
      FederateHandle const&) override {
    objectDiscoveryReports.push_back(objectInstance);
  }

  void reflectAttributeValues(
      ObjectInstanceHandle const& objectInstance,
      AttributeHandleValueMap const& attributeValues,
      VariableLengthData const&,
      TransportationTypeHandle const& transportationType,
      FederateHandle const& producingFederate,
      RegionHandleSet const* optionalSentRegions) override {
    attributeReflectionReports.push_back({
        objectInstance,
        attributeValues,
        transportationType,
        producingFederate,
        optionalSentRegions != nullptr,
        optionalSentRegions == nullptr ? RegionHandleSet{} : *optionalSentRegions,
    });
  }

  std::vector<ObjectInstanceHandle> objectDiscoveryReports;
  std::vector<AttributeReflectionReport> attributeReflectionReports;
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
    "Embedded three-dimensional regional object attributes require complete overlap",
    "[integration][development-profile][federation-management][ddm]"
    "[rti.service.register-object-instance-with-regions]"
    "[rti.service.subscribe-object-class-attributes-with-regions]"
    "[rti.service.update-attribute-values]"
    "[federate.callback.discover-object-instance]"
    "[federate.callback.reflect-attribute-values]") {
  ReportingFederateAmbassador publisherReports;
  ReportingFederateAmbassador subscriberReports;
  auto publisher = makeRti();
  auto subscriber = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = restaurantFomPath().wstring();

  REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED));
  REQUIRE_NOTHROW(subscriber->connect(subscriberReports, HLA_EVOKED));
  REQUIRE_NOTHROW(publisher->createFederationExecution(
      federationName,
      fomModule,
      standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(publisher->joinFederationExecution(
      L"regional-object-3d-publisher",
      L"publisher",
      federationName));
  REQUIRE_NOTHROW(subscriber->joinFederationExecution(
      L"regional-object-3d-subscriber",
      L"subscriber",
      federationName));

  auto const light = publisher->getObjectClassHandle(
      fixture_hla::fom::food_drink_soda_light);
  auto const flavor = publisher->getAttributeHandle(
      light,
      fixture_hla::fixture::flavor);
  auto const barQuantity = publisher->getDimensionHandle(
      fixture_hla::fixture::bar_quantity);
  auto const sodaFlavor = publisher->getDimensionHandle(
      fixture_hla::fixture::soda_flavor);
  auto const sweetener = publisher->getDimensionHandle(
      fixture_hla::fixture::sweetener);
  REQUIRE(light.isValid());
  REQUIRE(flavor.isValid());
  REQUIRE(barQuantity.isValid());
  REQUIRE(sodaFlavor.isValid());
  REQUIRE(sweetener.isValid());
  REQUIRE(publisher->getAvailableDimensionsForObjectClass(light) ==
          DimensionHandleSet{barQuantity, sodaFlavor, sweetener});
  AttributeHandleSet const flavorOnly{flavor};
  REQUIRE_NOTHROW(publisher->publishObjectClassAttributes(light, flavorOnly));

  auto const publisherRegion = publisher->createRegion(
      DimensionHandleSet{barQuantity, sodaFlavor, sweetener});
  auto const subscriberBarQuantity = subscriber->getDimensionHandle(
      fixture_hla::fixture::bar_quantity);
  auto const subscriberSodaFlavor = subscriber->getDimensionHandle(
      fixture_hla::fixture::soda_flavor);
  auto const subscriberSweetener = subscriber->getDimensionHandle(
      fixture_hla::fixture::sweetener);
  auto const subscriberRegion = subscriber->createRegion(
      DimensionHandleSet{
          subscriberBarQuantity,
          subscriberSodaFlavor,
          subscriberSweetener});
  REQUIRE_NOTHROW(publisher->setRangeBounds(
      publisherRegion,
      barQuantity,
      RangeBounds(0UL, 5UL)));
  REQUIRE_NOTHROW(publisher->setRangeBounds(
      publisherRegion,
      sodaFlavor,
      RangeBounds(0UL, 2UL)));
  REQUIRE_NOTHROW(publisher->setRangeBounds(
      publisherRegion,
      sweetener,
      RangeBounds(0UL, 1UL)));
  REQUIRE_NOTHROW(subscriber->setRangeBounds(
      subscriberRegion,
      subscriberBarQuantity,
      RangeBounds(0UL, 5UL)));
  REQUIRE_NOTHROW(subscriber->setRangeBounds(
      subscriberRegion,
      subscriberSodaFlavor,
      RangeBounds(0UL, 2UL)));
  REQUIRE_NOTHROW(subscriber->setRangeBounds(
      subscriberRegion,
      subscriberSweetener,
      RangeBounds(0UL, 1UL)));
  REQUIRE_NOTHROW(publisher->commitRegionModifications(
      RegionHandleSet{publisherRegion}));
  REQUIRE_NOTHROW(subscriber->commitRegionModifications(
      RegionHandleSet{subscriberRegion}));

  AttributeHandleSetRegionHandleSetPairVector const publisherPair{{
      flavorOnly,
      RegionHandleSet{publisherRegion},
  }};
  AttributeHandleSetRegionHandleSetPairVector const subscriberPair{{
      AttributeHandleSet{subscriber->getAttributeHandle(
          light,
          fixture_hla::fixture::flavor)},
      RegionHandleSet{subscriberRegion},
  }};
  REQUIRE_NOTHROW(subscriber->setConveyRegionDesignatorSetsSwitch(true));
  REQUIRE_NOTHROW(subscriber->subscribeObjectClassAttributesWithRegions(
      light,
      subscriberPair));
  ObjectInstanceHandle const objectInstance =
      publisher->registerObjectInstanceWithRegions(light, publisherPair);
  REQUIRE(objectInstance.isValid());
  REQUIRE_FALSE(subscriber->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(subscriberReports.objectDiscoveryReports.size() == 1U);

  auto update = [&](unsigned char value) {
    AttributeHandleValueMap values;
    values.emplace(flavor, VariableLengthData(&value, 1));
    REQUIRE_NOTHROW(publisher->updateAttributeValues(
        objectInstance,
        values,
        VariableLengthData()));
    REQUIRE_FALSE(subscriber->evokeMultipleCallbacks(0.0, 0.0));
  };
  update(0x10);
  REQUIRE(subscriberReports.attributeReflectionReports.size() == 1U);
  REQUIRE(subscriberReports.attributeReflectionReports.back().sentRegionsSupplied);
  REQUIRE(subscriberReports.attributeReflectionReports.back().sentRegions.contains(
      publisherRegion));

  auto expectSuppressed = [&](DimensionHandle const& dimension,
                              RangeBounds const& disjoint,
                              DimensionHandle const& restoreDimension) {
    REQUIRE_NOTHROW(publisher->setRangeBounds(
        publisherRegion,
        dimension,
        disjoint));
    REQUIRE_NOTHROW(publisher->commitRegionModifications(
        RegionHandleSet{publisherRegion}));
    update(0x20);
    REQUIRE(subscriberReports.attributeReflectionReports.size() == 1U);
    REQUIRE_NOTHROW(publisher->setRangeBounds(
        publisherRegion,
        restoreDimension,
        restoreDimension == barQuantity
            ? RangeBounds(0UL, 5UL)
            : restoreDimension == sodaFlavor
                ? RangeBounds(0UL, 2UL)
                : RangeBounds(0UL, 1UL)));
    REQUIRE_NOTHROW(publisher->commitRegionModifications(
        RegionHandleSet{publisherRegion}));
  };
  expectSuppressed(barQuantity, RangeBounds(6UL, 8UL), barQuantity);
  expectSuppressed(sodaFlavor, RangeBounds(3UL, 4UL), sodaFlavor);
  expectSuppressed(sweetener, RangeBounds(2UL, 3UL), sweetener);
  update(0x30);
  REQUIRE(subscriberReports.attributeReflectionReports.size() == 2U);

  REQUIRE_NOTHROW(publisher->unassociateRegionsForUpdates(
      objectInstance,
      publisherPair));
  REQUIRE_NOTHROW(subscriber->unsubscribeObjectClassAttributesWithRegions(
      light,
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
