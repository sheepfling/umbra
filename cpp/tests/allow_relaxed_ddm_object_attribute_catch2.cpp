#include <catch2/catch_test_macros.hpp>

#include "internal/fom/hla_names.hpp"

#include <RTI/NullFederateAmbassador.h>
#include <RTI/RTI1516.h>

#include <atomic>
#include <filesystem>
#include <memory>
#include <vector>

#ifndef UMBRA_SOURCE_DIRECTORY
#error "The Allow Relaxed DDM object-attribute test requires the Umbra source directory."
#endif

namespace {

namespace standard_hla = umbra::detail::hla::wide;

using rti1516_2025::AttributeHandle;
using rti1516_2025::AttributeHandleSet;
using rti1516_2025::AttributeHandleSetRegionHandleSetPairVector;
using rti1516_2025::AttributeHandleValueMap;
using rti1516_2025::CallbackModel;
using rti1516_2025::DimensionHandle;
using rti1516_2025::DimensionHandleSet;
using rti1516_2025::HLA_EVOKED;
using rti1516_2025::ObjectClassHandle;
using rti1516_2025::ObjectInstanceHandle;
using rti1516_2025::RTIambassador;
using rti1516_2025::RTIambassadorFactory;
using rti1516_2025::RangeBounds;
using rti1516_2025::RegionHandleSet;
using rti1516_2025::VariableLengthData;

std::wstring nextFederationName() {
  static std::atomic_uint64_t sequence{0U};
  return L"allow-relaxed-ddm-object-attribute-" +
      std::to_wstring(sequence.fetch_add(1U, std::memory_order_relaxed));
}

std::filesystem::path sourcePath(std::filesystem::path const& relativePath) {
  return std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / relativePath;
}

std::filesystem::path resourcePath(std::filesystem::path const& relativePath) {
  return sourcePath(std::filesystem::path("third_party") /
                    "ieee1516.2-2025" / "resources" / relativePath);
}

class ReportingFederateAmbassador final
    : public rti1516_2025::NullFederateAmbassador {
 public:
  struct DiscoveryReport final {
    ObjectInstanceHandle objectInstance;
  };

  struct ReflectionReport final {
    ObjectInstanceHandle objectInstance;
    AttributeHandleValueMap attributeValues;
  };

  void discoverObjectInstance(
      ObjectInstanceHandle const& objectInstance,
      ObjectClassHandle const&,
      std::wstring const&,
      rti1516_2025::FederateHandle const&) override {
    objectDiscoveryReports.push_back({objectInstance});
  }

  void reflectAttributeValues(
      ObjectInstanceHandle const& objectInstance,
      AttributeHandleValueMap const& attributeValues,
      VariableLengthData const&,
      rti1516_2025::TransportationTypeHandle const&,
      rti1516_2025::FederateHandle const&,
      rti1516_2025::RegionHandleSet const*) override {
    attributeReflectionReports.push_back({objectInstance, attributeValues});
  }

  std::vector<DiscoveryReport> objectDiscoveryReports;
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
    "Embedded Allow Relaxed DDM expands only touching regional object-attribute ranges",
    "[integration][development-profile][object-management][ddm][allow-relaxed-ddm]"
    "[rti.service.get-allow-relaxed-ddm-switch]"
    "[rti.service.create-region]"
    "[rti.service.set-range-bounds]"
    "[rti.service.commit-region-modifications]"
    "[rti.service.register-object-instance-with-regions]"
    "[rti.service.subscribe-object-class-attributes-with-regions]"
    "[rti.service.update-attribute-values]"
    "[federate.callback.discover-object-instance]"
    "[federate.callback.reflect-attribute-values]") {
  auto runScenario = [](bool const relaxedDdmEnabled) {
    ReportingFederateAmbassador publisherReports;
    ReportingFederateAmbassador subscriberReports;
    auto publisher = makeRti();
    auto subscriber = makeRti();
    auto const federationName = nextFederationName();
    auto const restaurantFom =
        resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
    auto const relaxedDdmFom =
        sourcePath(std::filesystem::path("cpp") / "tests" / "data" /
                   "allow-relaxed-ddm-enabled-fom.xml")
            .wstring();
    std::vector<std::wstring> fomModules{restaurantFom};
    if (relaxedDdmEnabled) {
      fomModules.push_back(relaxedDdmFom);
    }

    REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED));
    REQUIRE_NOTHROW(subscriber->connect(subscriberReports, HLA_EVOKED));
    REQUIRE_NOTHROW(publisher->createFederationExecution(
        federationName,
        fomModules,
        standard_hla::mom::integer64_time));
    REQUIRE_NOTHROW(publisher->joinFederationExecution(
        L"relaxed-ddm-object-publisher",
        L"publisher",
        federationName));
    REQUIRE_NOTHROW(subscriber->joinFederationExecution(
        L"relaxed-ddm-object-subscriber",
        L"subscriber",
        federationName));
    REQUIRE(publisher->getAllowRelaxedDDMSwitch() == relaxedDdmEnabled);
    REQUIRE(subscriber->getAllowRelaxedDDMSwitch() == relaxedDdmEnabled);

    auto const soda = publisher->getObjectClassHandle(
        L"HLAobjectRoot.Food.Drink.Soda");
    auto const subscriberSoda = subscriber->getObjectClassHandle(
        L"HLAobjectRoot.Food.Drink.Soda");
    auto const flavor = publisher->getAttributeHandle(soda, L"Flavor");
    auto const subscriberFlavor =
        subscriber->getAttributeHandle(subscriberSoda, L"Flavor");
    auto const sodaFlavor = publisher->getDimensionHandle(L"SodaFlavor");
    auto const subscriberSodaFlavor =
        subscriber->getDimensionHandle(L"SodaFlavor");
    REQUIRE(soda.isValid());
    REQUIRE(subscriberSoda.isValid());
    REQUIRE(flavor.isValid());
    REQUIRE(subscriberFlavor.isValid());
    REQUIRE(sodaFlavor.isValid());
    REQUIRE(subscriberSodaFlavor.isValid());
    AttributeHandleSet const flavorOnly{flavor};
    AttributeHandleSet const subscriberFlavorOnly{subscriberFlavor};
    REQUIRE_NOTHROW(publisher->publishObjectClassAttributes(soda, flavorOnly));

    auto const sourceRegion =
        publisher->createRegion(DimensionHandleSet{sodaFlavor});
    auto const subscriptionRegion = subscriber->createRegion(
        DimensionHandleSet{subscriberSodaFlavor});
    REQUIRE_NOTHROW(publisher->setRangeBounds(
        sourceRegion,
        sodaFlavor,
        RangeBounds(0UL, 2UL)));
    REQUIRE_NOTHROW(publisher->commitRegionModifications(
        RegionHandleSet{sourceRegion}));
    REQUIRE_NOTHROW(subscriber->setRangeBounds(
        subscriptionRegion,
        subscriberSodaFlavor,
        RangeBounds(2UL, 3UL)));
    REQUIRE_NOTHROW(subscriber->commitRegionModifications(
        RegionHandleSet{subscriptionRegion}));
    AttributeHandleSetRegionHandleSetPairVector const sourcePair{{
        flavorOnly,
        RegionHandleSet{sourceRegion},
    }};
    AttributeHandleSetRegionHandleSetPairVector const subscriptionPair{{
        subscriberFlavorOnly,
        RegionHandleSet{subscriptionRegion},
    }};
    ObjectInstanceHandle objectInstance;
    REQUIRE_NOTHROW(objectInstance = publisher->registerObjectInstanceWithRegions(
        soda,
        sourcePair));
    REQUIRE(objectInstance.isValid());
    REQUIRE_NOTHROW(subscriber->subscribeObjectClassAttributesWithRegions(
        subscriberSoda,
        subscriptionPair));
    drainCallbacks(*subscriber);

    std::size_t expectedDiscoveries = relaxedDdmEnabled ? 1U : 0U;
    std::size_t expectedReflections = relaxedDdmEnabled ? 1U : 0U;
    REQUIRE(subscriberReports.objectDiscoveryReports.size() == expectedDiscoveries);
    std::vector<unsigned char> const touchingValueBytes{0x31U, 0x42U};
    AttributeHandleValueMap touchingValue;
    touchingValue.emplace(
        flavor,
        VariableLengthData(touchingValueBytes.data(), touchingValueBytes.size()));
    REQUIRE_NOTHROW(publisher->updateAttributeValues(
        objectInstance,
        touchingValue,
        VariableLengthData()));
    drainCallbacks(*subscriber);
    REQUIRE(subscriberReports.attributeReflectionReports.size() == expectedReflections);

    // A positive gap removes the route even when the object is already known
    // from a previous relaxed discovery.
    REQUIRE_NOTHROW(subscriber->setRangeBounds(
        subscriptionRegion,
        subscriberSodaFlavor,
        RangeBounds(3UL, 4UL)));
    REQUIRE_NOTHROW(subscriber->commitRegionModifications(
        RegionHandleSet{subscriptionRegion}));
    std::vector<unsigned char> const gapValueBytes{0x53U, 0x64U};
    AttributeHandleValueMap gapValue;
    gapValue.emplace(
        flavor,
        VariableLengthData(gapValueBytes.data(), gapValueBytes.size()));
    REQUIRE_NOTHROW(publisher->updateAttributeValues(
        objectInstance,
        gapValue,
        VariableLengthData()));
    drainCallbacks(*subscriber);
    REQUIRE(subscriberReports.attributeReflectionReports.size() == expectedReflections);

    // Restoring strict overlap must discover the disabled-profile recipient
    // and preserve delivery in the enabled profile.
    REQUIRE_NOTHROW(subscriber->setRangeBounds(
        subscriptionRegion,
        subscriberSodaFlavor,
        RangeBounds(1UL, 3UL)));
    REQUIRE_NOTHROW(subscriber->commitRegionModifications(
        RegionHandleSet{subscriptionRegion}));
    REQUIRE_NOTHROW(subscriber->subscribeObjectClassAttributesWithRegions(
        subscriberSoda,
        subscriptionPair));
    drainCallbacks(*subscriber);
    if (!relaxedDdmEnabled) {
      ++expectedDiscoveries;
    }
    REQUIRE(subscriberReports.objectDiscoveryReports.size() == expectedDiscoveries);
    std::vector<unsigned char> const strictValueBytes{0x75U, 0x86U};
    AttributeHandleValueMap strictValue;
    strictValue.emplace(
        flavor,
        VariableLengthData(strictValueBytes.data(), strictValueBytes.size()));
    REQUIRE_NOTHROW(publisher->updateAttributeValues(
        objectInstance,
        strictValue,
        VariableLengthData()));
    drainCallbacks(*subscriber);
    ++expectedReflections;
    REQUIRE(subscriberReports.attributeReflectionReports.size() == expectedReflections);

    REQUIRE_NOTHROW(subscriber->unsubscribeObjectClassAttributesWithRegions(
        subscriberSoda,
        subscriptionPair));
    REQUIRE_NOTHROW(publisher->unassociateRegionsForUpdates(
        objectInstance,
        sourcePair));
    REQUIRE_NOTHROW(subscriber->deleteRegion(subscriptionRegion));
    REQUIRE_NOTHROW(publisher->deleteRegion(sourceRegion));
    REQUIRE_NOTHROW(subscriber->resignFederationExecution(
        rti1516_2025::NO_ACTION));
    REQUIRE_NOTHROW(publisher->resignFederationExecution(
        rti1516_2025::CANCEL_THEN_DELETE_THEN_DIVEST));
    REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
    REQUIRE_NOTHROW(subscriber->disconnect());
    REQUIRE_NOTHROW(publisher->disconnect());
  };

  SECTION("the FDD enables Relaxed DDM") {
    runScenario(true);
  }
  SECTION("the FDD leaves Relaxed DDM disabled") {
    runScenario(false);
  }
}
