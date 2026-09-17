#include "internal/fom/hla_names.hpp"
#include "hla_test_names.hpp"

#include <atomic>
#include <filesystem>
#include <memory>
#include <optional>
#include <vector>

#include <catch2/catch_test_macros.hpp>
#include <RTI/NullFederateAmbassador.h>
#include <RTI/RTI1516.h>

#ifndef UMBRA_SOURCE_DIRECTORY
#error "The regional rate-designator tests require the Umbra source directory."
#endif

namespace {

namespace standard_hla = umbra::detail::hla::wide;
namespace fixture_hla = umbra::test::hla::wide;

using rti1516_2025::AttributeHandle;
using rti1516_2025::AttributeHandleSet;
using rti1516_2025::AttributeHandleSetRegionHandleSetPairVector;
using rti1516_2025::CallbackModel;
using rti1516_2025::DimensionHandleSet;
using rti1516_2025::HLA_EVOKED;
using rti1516_2025::HLA_IMMEDIATE;
using rti1516_2025::NO_ACTION;
using rti1516_2025::NullFederateAmbassador;
using rti1516_2025::ObjectClassHandle;
using rti1516_2025::ObjectInstanceHandle;
using rti1516_2025::RTIambassador;
using rti1516_2025::RTIambassadorFactory;
using rti1516_2025::RangeBounds;
using rti1516_2025::RegionHandleSet;
using rti1516_2025::CANCEL_THEN_DELETE_THEN_DIVEST;

class RecordingFederateAmbassador final : public NullFederateAmbassador {
 public:
  struct RateReport {
    ObjectInstanceHandle objectInstance;
    AttributeHandleSet attributes;
    std::wstring updateRateDesignator;
  };

  struct AttributeReport {
    ObjectInstanceHandle objectInstance;
    AttributeHandleSet attributes;
  };

  void discoverObjectInstance(
      ObjectInstanceHandle const& objectInstance,
      ObjectClassHandle const&,
      std::wstring const&,
      rti1516_2025::FederateHandle const&) override {
    discoveredObjectInstances.push_back(objectInstance);
  }

  void turnUpdatesOnForObjectInstance(
      ObjectInstanceHandle const& objectInstance,
      AttributeHandleSet const& attributes,
      std::wstring const& updateRateDesignator) override {
    turnUpdatesOnForObjectInstanceReports.push_back(
        {objectInstance, attributes, updateRateDesignator});
  }

  void turnUpdatesOffForObjectInstance(
      ObjectInstanceHandle const& objectInstance,
      AttributeHandleSet const& attributes) override {
    turnUpdatesOffForObjectInstanceReports.push_back({objectInstance, attributes});
  }

  std::vector<ObjectInstanceHandle> discoveredObjectInstances;
  std::vector<RateReport> turnUpdatesOnForObjectInstanceReports;
  std::vector<AttributeReport> turnUpdatesOffForObjectInstanceReports;
};

std::unique_ptr<RTIambassador> makeRti() {
  RTIambassadorFactory factory;
  return factory.createRTIambassador();
}

std::wstring nextFederationName() {
  static std::atomic_uint64_t sequence{0U};
  return L"embedded-regional-rate-designator-" +
      std::to_wstring(sequence.fetch_add(1U, std::memory_order_relaxed));
}

std::filesystem::path resourcePath(std::filesystem::path const& relativePath) {
  return std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
      "third_party" / "ieee1516.2-2025" / "resources" / relativePath;
}

void suppressDeclarationRelevanceAdvisories(RTIambassador& rti) {
  if (rti.getObjectClassRelevanceAdvisorySwitch()) {
    rti.setObjectClassRelevanceAdvisorySwitch(false);
  }
  if (rti.getInteractionRelevanceAdvisorySwitch()) {
    rti.setInteractionRelevanceAdvisorySwitch(false);
  }
}

TEST_CASE(
    "Embedded regional attribute relevance advisories retain explicit update-rate designators",
    "[integration][development-profile][federation-management][object-management][ddm]"
    "[callbacks][regional-attribute-relevance][regional-update-rate-designator]"
    "[regional-attribute-relevance-rate-designator]"
    "[2025][rti.service.get-attribute-relevance-advisory-switch]"
    "[rti.service.subscribe-object-class-attributes-with-regions]"
    "[rti.service.register-object-instance-with-regions]"
    "[rti.service.associate-regions-for-updates]"
    "[rti.service.unassociate-regions-for-updates]"
    "[federate.callback.turn-updates-on-for-object-instance]"
    "[federate.callback.turn-updates-off-for-object-instance]") {
  auto runScenario = [](CallbackModel callbackModel) {
    RecordingFederateAmbassador ownerReports;
    RecordingFederateAmbassador subscriberReports;
    auto owner = makeRti();
    auto subscriber = makeRti();
    auto const federationName = nextFederationName();
    auto const fomModule =
        resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();

    REQUIRE_NOTHROW(owner->connect(ownerReports, callbackModel));
    REQUIRE_NOTHROW(subscriber->connect(subscriberReports, callbackModel));
    REQUIRE_NOTHROW(owner->createFederationExecution(
        federationName,
        fomModule,
        standard_hla::mom::integer64_time));
    REQUIRE_NOTHROW(owner->joinFederationExecution(
        L"regional-rate-owner", L"regional-rate-owner", federationName));
    REQUIRE_NOTHROW(subscriber->joinFederationExecution(
        L"regional-rate-subscriber", L"regional-rate-subscriber", federationName));
    suppressDeclarationRelevanceAdvisories(*owner);

    auto const objectClass =
        owner->getObjectClassHandle(fixture_hla::fom::food_drink_soda);
    auto const attribute =
        owner->getAttributeHandle(objectClass, fixture_hla::fixture::flavor);
    auto const dimension =
        owner->getDimensionHandle(fixture_hla::fixture::soda_flavor);
    REQUIRE(objectClass.isValid());
    REQUIRE(attribute.isValid());
    REQUIRE(dimension.isValid());
    REQUIRE(owner->getAttributeRelevanceAdvisorySwitch());
    REQUIRE(subscriber->getAttributeRelevanceAdvisorySwitch());

    AttributeHandleSet const attributeSet{attribute};
    REQUIRE_NOTHROW(owner->publishObjectClassAttributes(objectClass, attributeSet));

    auto const ownerRegion = owner->createRegion(DimensionHandleSet{dimension});
    auto const disjointOwnerRegion =
        owner->createRegion(DimensionHandleSet{dimension});
    auto const subscriberRegion =
        subscriber->createRegion(DimensionHandleSet{dimension});
    REQUIRE_NOTHROW(owner->setRangeBounds(ownerRegion, dimension, RangeBounds(0UL, 1UL)));
    REQUIRE_NOTHROW(owner->commitRegionModifications(RegionHandleSet{ownerRegion}));
    REQUIRE_NOTHROW(owner->setRangeBounds(
        disjointOwnerRegion, dimension, RangeBounds(2UL, 3UL)));
    REQUIRE_NOTHROW(owner->commitRegionModifications(
        RegionHandleSet{disjointOwnerRegion}));
    REQUIRE_NOTHROW(subscriber->setRangeBounds(
        subscriberRegion, dimension, RangeBounds(0UL, 1UL)));
    REQUIRE_NOTHROW(subscriber->commitRegionModifications(
        RegionHandleSet{subscriberRegion}));

    AttributeHandleSetRegionHandleSetPairVector const ownerPair{{
        attributeSet,
        RegionHandleSet{ownerRegion},
    }};
    AttributeHandleSetRegionHandleSetPairVector const disjointOwnerPair{{
        attributeSet,
        RegionHandleSet{disjointOwnerRegion},
    }};
    AttributeHandleSetRegionHandleSetPairVector const subscriberPair{{
        attributeSet,
        RegionHandleSet{subscriberRegion},
    }};
    REQUIRE_NOTHROW(subscriber->subscribeObjectClassAttributesWithRegions(
        objectClass,
        subscriberPair,
        true,
        L"High"));

    auto drain = [](RTIambassador& rti, CallbackModel model) {
      if (model == HLA_EVOKED) {
        while (rti.evokeMultipleCallbacks(0.0, 0.0)) {
        }
      }
    };
    auto drainSubscriber = [&] { drain(*subscriber, callbackModel); };
    auto drainOwner = [&] { drain(*owner, callbackModel); };

    ObjectInstanceHandle objectInstance;
    REQUIRE_NOTHROW(objectInstance = owner->registerObjectInstanceWithRegions(
        objectClass,
        ownerPair));
    drainSubscriber();
    REQUIRE(subscriberReports.discoveredObjectInstances.size() == 1U);
    drainOwner();
    REQUIRE(ownerReports.turnUpdatesOnForObjectInstanceReports.size() == 1U);
    REQUIRE(ownerReports.turnUpdatesOnForObjectInstanceReports.front().objectInstance ==
            objectInstance);
    REQUIRE(ownerReports.turnUpdatesOnForObjectInstanceReports.front().attributes ==
            attributeSet);
    REQUIRE(ownerReports.turnUpdatesOnForObjectInstanceReports.front().updateRateDesignator ==
            L"High");
    REQUIRE(ownerReports.turnUpdatesOffForObjectInstanceReports.empty());

    // Add a disjoint source association while the original overlap remains;
    // the active overlap and explicit High designator are unchanged.
    REQUIRE_NOTHROW(owner->associateRegionsForUpdates(objectInstance, disjointOwnerPair));
    drainOwner();
    REQUIRE(ownerReports.turnUpdatesOnForObjectInstanceReports.size() == 1U);
    REQUIRE(ownerReports.turnUpdatesOffForObjectInstanceReports.empty());

    // Removing the overlapping source association leaves only the disjoint
    // range, makes the attribute irrelevant, and emits one owner-directed Off.
    REQUIRE_NOTHROW(owner->unassociateRegionsForUpdates(objectInstance, ownerPair));
    drainOwner();
    REQUIRE(ownerReports.turnUpdatesOnForObjectInstanceReports.size() == 1U);
    REQUIRE(ownerReports.turnUpdatesOffForObjectInstanceReports.size() == 1U);
    REQUIRE(ownerReports.turnUpdatesOffForObjectInstanceReports.front().objectInstance ==
            objectInstance);
    REQUIRE(ownerReports.turnUpdatesOffForObjectInstanceReports.front().attributes ==
            attributeSet);

    // Removing the disjoint association exposes the private default source
    // realization. Restore the original overlap before the callback boundary
    // so both callback models observe one re-entry with the same High rate.
    REQUIRE_NOTHROW(owner->unassociateRegionsForUpdates(
        objectInstance,
        disjointOwnerPair));
    REQUIRE_NOTHROW(owner->associateRegionsForUpdates(objectInstance, ownerPair));
    drainOwner();
    REQUIRE(ownerReports.turnUpdatesOnForObjectInstanceReports.size() == 2U);
    REQUIRE(ownerReports.turnUpdatesOnForObjectInstanceReports.back().objectInstance ==
            objectInstance);
    REQUIRE(ownerReports.turnUpdatesOnForObjectInstanceReports.back().attributes ==
            attributeSet);
    REQUIRE(ownerReports.turnUpdatesOnForObjectInstanceReports.back().updateRateDesignator ==
            L"High");
    REQUIRE(ownerReports.turnUpdatesOffForObjectInstanceReports.size() == 1U);

    REQUIRE_NOTHROW(owner->unassociateRegionsForUpdates(objectInstance, ownerPair));
    REQUIRE_NOTHROW(subscriber->unsubscribeObjectClassAttributesWithRegions(
        objectClass,
        subscriberPair));
    drainOwner();
    REQUIRE_NOTHROW(owner->deleteRegion(ownerRegion));
    REQUIRE_NOTHROW(owner->deleteRegion(disjointOwnerRegion));
    REQUIRE_NOTHROW(subscriber->deleteRegion(subscriberRegion));
    REQUIRE_NOTHROW(subscriber->resignFederationExecution(NO_ACTION));
    REQUIRE_NOTHROW(owner->resignFederationExecution(CANCEL_THEN_DELETE_THEN_DIVEST));
    REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
    REQUIRE_NOTHROW(subscriber->disconnect());
    REQUIRE_NOTHROW(owner->disconnect());
  };

  SECTION("HLA_EVOKED") {
    runScenario(HLA_EVOKED);
  }
  SECTION("HLA_IMMEDIATE") {
    runScenario(HLA_IMMEDIATE);
  }
}

}  // namespace
