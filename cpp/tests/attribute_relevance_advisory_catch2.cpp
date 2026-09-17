#include "internal/fom/hla_names.hpp"
#include "hla_test_names.hpp"

#include <atomic>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#include <catch2/catch_test_macros.hpp>
#include <RTI/NullFederateAmbassador.h>
#include <RTI/RTI1516.h>

#ifndef UMBRA_SOURCE_DIRECTORY
#error "The regional advisory tests require the Umbra source directory."
#endif

namespace {

namespace standard_hla = umbra::detail::hla::wide;
namespace fixture_hla = umbra::test::hla::wide;

using rti1516_2025::AttributeHandle;
using rti1516_2025::AttributeHandleSet;
using rti1516_2025::AttributeHandleSetRegionHandleSetPairVector;
using rti1516_2025::CallbackModel;
using rti1516_2025::DimensionHandleSet;
using rti1516_2025::FederateAmbassador;
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

class ReportingFederateAmbassador final : public NullFederateAmbassador {
 public:
  struct DiscoveryReport {
    ObjectInstanceHandle objectInstance;
  };

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
    objectDiscoveryReports.push_back({objectInstance});
  }

  void turnUpdatesOnForObjectInstance(
      ObjectInstanceHandle const& objectInstance,
      AttributeHandleSet const& attributes) override {
    turnUpdatesOnForObjectInstanceReports.push_back({objectInstance, attributes});
  }

  void turnUpdatesOnForObjectInstance(
      ObjectInstanceHandle const& objectInstance,
      AttributeHandleSet const& attributes,
      std::wstring const& updateRateDesignator) override {
    turnUpdatesOnForObjectInstanceRateReports.push_back({
        objectInstance,
        attributes,
        updateRateDesignator,
    });
  }

  void turnUpdatesOffForObjectInstance(
      ObjectInstanceHandle const& objectInstance,
      AttributeHandleSet const& attributes) override {
    turnUpdatesOffForObjectInstanceReports.push_back({objectInstance, attributes});
  }

  std::vector<DiscoveryReport> objectDiscoveryReports;
  std::vector<AttributeReport> turnUpdatesOnForObjectInstanceReports;
  std::vector<RateReport> turnUpdatesOnForObjectInstanceRateReports;
  std::vector<AttributeReport> turnUpdatesOffForObjectInstanceReports;
};

std::unique_ptr<RTIambassador> makeRti() {
  RTIambassadorFactory factory;
  return factory.createRTIambassador();
}

std::wstring nextFederationName() {
  static std::atomic_uint64_t sequence{0U};
  return L"regional-queued-callback-" +
      std::to_wstring(sequence.fetch_add(1U, std::memory_order_relaxed));
}

void suppressDeclarationRelevanceAdvisories(RTIambassador& rti) {
  if (rti.getObjectClassRelevanceAdvisorySwitch()) {
    rti.setObjectClassRelevanceAdvisorySwitch(false);
  }
  if (rti.getInteractionRelevanceAdvisorySwitch()) {
    rti.setInteractionRelevanceAdvisorySwitch(false);
  }
}

std::filesystem::path resourcePath(std::filesystem::path const& relativePath) {
  return std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
      "third_party" / "ieee1516.2-2025" / "resources" / relativePath;
}

TEST_CASE(
    "Embedded regional attribute relevance advisories recheck callbacks after an active-rate and scope transition under HLA_EVOKED and HLA_IMMEDIATE",
    "[integration][development-profile][federation-management][object-management][ddm]"
    "[callbacks][callback-immediate][callback-suppression][regional-attribute-relevance][update-rate-reissue]"
    "[rti.service.get-attribute-relevance-advisory-switch]"
    "[rti.service.subscribe-object-class-attributes-with-regions]"
    "[rti.service.register-object-instance-with-regions]"
    "[rti.service.associate-regions-for-updates]"
    "[rti.service.unassociate-regions-for-updates]"
    "[federate.callback.turn-updates-on-for-object-instance]"
    "[federate.callback.turn-updates-off-for-object-instance]") {
  auto runScenario = [](CallbackModel callbackModel) {
  ReportingFederateAmbassador ownerReports;
  ReportingFederateAmbassador subscriberReports;
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
      L"regional-queued-owner", L"regional-queued-owner", federationName));
  REQUIRE_NOTHROW(subscriber->joinFederationExecution(
      L"regional-queued-subscriber", L"regional-queued-subscriber", federationName));
  suppressDeclarationRelevanceAdvisories(*owner);

  auto const soda = owner->getObjectClassHandle(fixture_hla::fom::food_drink_soda);
  auto const flavor = owner->getAttributeHandle(soda, fixture_hla::fixture::flavor);
  auto const sodaFlavor = owner->getDimensionHandle(fixture_hla::fixture::soda_flavor);
  REQUIRE(soda.isValid());
  REQUIRE(flavor.isValid());
  REQUIRE(sodaFlavor.isValid());
  REQUIRE(owner->getAttributeRelevanceAdvisorySwitch());

  AttributeHandleSet const flavorOnly{flavor};
  REQUIRE_NOTHROW(owner->publishObjectClassAttributes(soda, flavorOnly));
  auto const ownerRegion = owner->createRegion(DimensionHandleSet{sodaFlavor});
  auto const disjointOwnerRegion = owner->createRegion(DimensionHandleSet{sodaFlavor});
  auto const subscriberRegion = subscriber->createRegion(DimensionHandleSet{sodaFlavor});
  REQUIRE_NOTHROW(owner->setRangeBounds(ownerRegion, sodaFlavor, RangeBounds(0UL, 1UL)));
  REQUIRE_NOTHROW(owner->commitRegionModifications(RegionHandleSet{ownerRegion}));
  REQUIRE_NOTHROW(owner->setRangeBounds(
      disjointOwnerRegion, sodaFlavor, RangeBounds(2UL, 3UL)));
  REQUIRE_NOTHROW(
      owner->commitRegionModifications(RegionHandleSet{disjointOwnerRegion}));
  REQUIRE_NOTHROW(
      subscriber->setRangeBounds(subscriberRegion, sodaFlavor, RangeBounds(0UL, 1UL)));
  REQUIRE_NOTHROW(
      subscriber->commitRegionModifications(RegionHandleSet{subscriberRegion}));

  AttributeHandleSetRegionHandleSetPairVector const ownerPair{{
      flavorOnly,
      RegionHandleSet{ownerRegion},
  }};
  AttributeHandleSetRegionHandleSetPairVector const disjointOwnerPair{{
      flavorOnly,
      RegionHandleSet{disjointOwnerRegion},
  }};
  AttributeHandleSetRegionHandleSetPairVector const subscriberPair{{
      flavorOnly,
      RegionHandleSet{subscriberRegion},
  }};
  REQUIRE_NOTHROW(subscriber->subscribeObjectClassAttributesWithRegions(
      soda,
      subscriberPair,
      true,
      L"High"));

  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = owner->registerObjectInstanceWithRegions(soda, ownerPair));
  if (callbackModel == HLA_EVOKED) {
    REQUIRE_FALSE(subscriber->evokeMultipleCallbacks(0.0, 0.0));
  }
  REQUIRE(subscriberReports.objectDiscoveryReports.size() == 1U);
  while (callbackModel == HLA_EVOKED && owner->evokeMultipleCallbacks(0.0, 0.0)) {
  }
  REQUIRE(ownerReports.turnUpdatesOnForObjectInstanceRateReports.size() == 1U);
  REQUIRE(ownerReports.turnUpdatesOnForObjectInstanceRateReports.front().objectInstance ==
          objectInstance);
  REQUIRE(ownerReports.turnUpdatesOnForObjectInstanceRateReports.front().attributes ==
          flavorOnly);
  REQUIRE(ownerReports.turnUpdatesOnForObjectInstanceRateReports.front().updateRateDesignator ==
          L"High");
  ownerReports.turnUpdatesOnForObjectInstanceRateReports.clear();
  ownerReports.turnUpdatesOffForObjectInstanceReports.clear();

  // Changing the active designator reissues a rate-bearing On callback.  Under
  // HLA_EVOKED that work is queued and must be rechecked after the source
  // region is moved out of overlap; HLA_IMMEDIATE delivers the reissue before
  // the transition completes.
  REQUIRE_NOTHROW(subscriber->subscribeObjectClassAttributesWithRegions(
      soda,
      subscriberPair,
      true,
      L"Medium"));
  REQUIRE_NOTHROW(owner->associateRegionsForUpdates(objectInstance, disjointOwnerPair));
  REQUIRE_NOTHROW(owner->unassociateRegionsForUpdates(objectInstance, ownerPair));

  while (callbackModel == HLA_EVOKED && owner->evokeMultipleCallbacks(0.0, 0.0)) {
  }
  if (callbackModel == HLA_EVOKED) {
    REQUIRE(ownerReports.turnUpdatesOnForObjectInstanceRateReports.empty());
  } else {
    REQUIRE(ownerReports.turnUpdatesOnForObjectInstanceRateReports.size() == 1U);
    REQUIRE(ownerReports.turnUpdatesOnForObjectInstanceRateReports.front().objectInstance ==
            objectInstance);
    REQUIRE(ownerReports.turnUpdatesOnForObjectInstanceRateReports.front().attributes ==
            flavorOnly);
    REQUIRE(ownerReports.turnUpdatesOnForObjectInstanceRateReports.front().updateRateDesignator ==
            L"Medium");
  }
  REQUIRE(ownerReports.turnUpdatesOffForObjectInstanceReports.size() == 1U);
  REQUIRE(ownerReports.turnUpdatesOffForObjectInstanceReports.front().objectInstance ==
          objectInstance);
  REQUIRE(ownerReports.turnUpdatesOffForObjectInstanceReports.front().attributes ==
          flavorOnly);

  REQUIRE_NOTHROW(owner->unassociateRegionsForUpdates(objectInstance, disjointOwnerPair));
  REQUIRE_NOTHROW(subscriber->unsubscribeObjectClassAttributesWithRegions(
      soda,
      subscriberPair));
  REQUIRE_NOTHROW(owner->deleteRegion(ownerRegion));
  REQUIRE_NOTHROW(owner->deleteRegion(disjointOwnerRegion));
  REQUIRE_NOTHROW(subscriber->deleteRegion(subscriberRegion));
  REQUIRE_NOTHROW(subscriber->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->resignFederationExecution(CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(owner->disconnect());
  REQUIRE_NOTHROW(subscriber->disconnect());
  };

  SECTION("HLA_EVOKED") {
    runScenario(HLA_EVOKED);
  }
  SECTION("HLA_IMMEDIATE") {
    runScenario(HLA_IMMEDIATE);
  }
}

TEST_CASE(
    "Embedded attribute relevance advisories follow scope transitions",
    "[integration][development-profile][federation-management][object-management][ddm]"
    "[callbacks][callback-immediate][callback-suppression][attribute-relevance-advisory]"
    "[attribute-relevance-scope-transition][update-rate-reissue][2025]"
    "[rti.service.get-attribute-relevance-advisory-switch]"
    "[rti.service.set-attribute-relevance-advisory-switch]"
    "[rti.service.subscribe-object-class-attributes]"
    "[rti.service.unsubscribe-object-class-attributes]"
    "[rti.service.publish-object-class-attributes]"
    "[rti.service.register-object-instance]"
    "[federate.callback.turn-updates-on-for-object-instance]"
    "[federate.callback.turn-updates-off-for-object-instance]") {
  auto runScenario = [](CallbackModel callbackModel) {
    ReportingFederateAmbassador ownerReports;
    ReportingFederateAmbassador subscriberReports;
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
        L"ordinary-advisory-owner", L"ordinary-advisory-owner", federationName));
    REQUIRE_NOTHROW(subscriber->joinFederationExecution(
        L"ordinary-advisory-subscriber",
        L"ordinary-advisory-subscriber",
        federationName));
    suppressDeclarationRelevanceAdvisories(*owner);

    auto const soda = owner->getObjectClassHandle(fixture_hla::fom::food_drink_soda);
    auto const flavor = owner->getAttributeHandle(soda, fixture_hla::fixture::flavor);
    REQUIRE(soda.isValid());
    REQUIRE(flavor.isValid());
    REQUIRE(owner->getAttributeRelevanceAdvisorySwitch());
    REQUIRE(subscriber->getAttributeRelevanceAdvisorySwitch());

    AttributeHandleSet const flavorOnly{flavor};
    REQUIRE_NOTHROW(owner->publishObjectClassAttributes(soda, flavorOnly));

    auto drainOwnerCallbacks = [&] {
      if (callbackModel == HLA_EVOKED) {
        while (owner->evokeMultipleCallbacks(0.0, 0.0)) {
        }
      }
    };
    auto drainSubscriberCallbacks = [&] {
      if (callbackModel == HLA_EVOKED) {
        while (subscriber->evokeMultipleCallbacks(0.0, 0.0)) {
        }
      }
    };

    // Register before changing the subscription so the owner-directed
    // advisory is the transition under test (registration itself only plans
    // discovery callbacks).
    ObjectInstanceHandle objectInstance;
    REQUIRE_NOTHROW(objectInstance = owner->registerObjectInstance(soda));

    // A default (unspecified) subscription uses the no-rate callback form.
    REQUIRE_NOTHROW(subscriber->subscribeObjectClassAttributes(
        soda,
        flavorOnly,
        true));
    // The subscription also discovers an already-registered object.  Under
    // HLA_EVOKED the initial owner-directed advisory is planned only after
    // that discovery callback commits the receiver's known-instance state.
    drainSubscriberCallbacks();
    drainOwnerCallbacks();
    REQUIRE(ownerReports.turnUpdatesOnForObjectInstanceReports.size() == 1U);
    REQUIRE(ownerReports.turnUpdatesOnForObjectInstanceReports.front().objectInstance ==
            objectInstance);
    REQUIRE(ownerReports.turnUpdatesOnForObjectInstanceReports.front().attributes ==
            flavorOnly);
    REQUIRE(ownerReports.turnUpdatesOnForObjectInstanceRateReports.empty());

    REQUIRE_NOTHROW(subscriber->unsubscribeObjectClassAttributes(soda, flavorOnly));
    drainOwnerCallbacks();
    REQUIRE(ownerReports.turnUpdatesOffForObjectInstanceReports.size() == 1U);
    REQUIRE(ownerReports.turnUpdatesOffForObjectInstanceReports.front().objectInstance ==
            objectInstance);
    REQUIRE(ownerReports.turnUpdatesOffForObjectInstanceReports.front().attributes ==
            flavorOnly);
    ownerReports.turnUpdatesOnForObjectInstanceReports.clear();
    ownerReports.turnUpdatesOffForObjectInstanceReports.clear();

    // The switch gates both callback overloads.  A disabled switch must not
    // leak a queued or immediate advisory while the subscription changes.
    REQUIRE_NOTHROW(owner->setAttributeRelevanceAdvisorySwitch(false));
    REQUIRE_FALSE(owner->getAttributeRelevanceAdvisorySwitch());
    REQUIRE_NOTHROW(subscriber->subscribeObjectClassAttributes(
        soda,
        flavorOnly,
        true,
        L"High"));
    drainOwnerCallbacks();
    REQUIRE(ownerReports.turnUpdatesOnForObjectInstanceReports.empty());
    REQUIRE(ownerReports.turnUpdatesOnForObjectInstanceRateReports.empty());
    REQUIRE(ownerReports.turnUpdatesOffForObjectInstanceReports.empty());
    REQUIRE_NOTHROW(subscriber->unsubscribeObjectClassAttributes(soda, flavorOnly));
    drainOwnerCallbacks();
    REQUIRE(ownerReports.turnUpdatesOnForObjectInstanceReports.empty());
    REQUIRE(ownerReports.turnUpdatesOnForObjectInstanceRateReports.empty());
    REQUIRE(ownerReports.turnUpdatesOffForObjectInstanceReports.empty());

    REQUIRE_NOTHROW(owner->setAttributeRelevanceAdvisorySwitch(true));
    REQUIRE(owner->getAttributeRelevanceAdvisorySwitch());

    // An explicit subscription uses the rate-bearing overload and a later
    // designator change reissues the active maximum rate.
    REQUIRE_NOTHROW(subscriber->subscribeObjectClassAttributes(
        soda,
        flavorOnly,
        true,
        L"High"));
    drainOwnerCallbacks();
    REQUIRE(ownerReports.turnUpdatesOnForObjectInstanceReports.empty());
    REQUIRE(ownerReports.turnUpdatesOnForObjectInstanceRateReports.size() == 1U);
    REQUIRE(ownerReports.turnUpdatesOnForObjectInstanceRateReports.front().objectInstance ==
            objectInstance);
    REQUIRE(ownerReports.turnUpdatesOnForObjectInstanceRateReports.front().attributes ==
            flavorOnly);
    REQUIRE(ownerReports.turnUpdatesOnForObjectInstanceRateReports.front().updateRateDesignator ==
            L"High");
    ownerReports.turnUpdatesOnForObjectInstanceRateReports.clear();

    REQUIRE_NOTHROW(subscriber->subscribeObjectClassAttributes(
        soda,
        flavorOnly,
        true,
        L"Medium"));
    drainOwnerCallbacks();
    REQUIRE(ownerReports.turnUpdatesOnForObjectInstanceReports.empty());
    REQUIRE(ownerReports.turnUpdatesOnForObjectInstanceRateReports.size() == 1U);
    REQUIRE(ownerReports.turnUpdatesOnForObjectInstanceRateReports.front().objectInstance ==
            objectInstance);
    REQUIRE(ownerReports.turnUpdatesOnForObjectInstanceRateReports.front().attributes ==
            flavorOnly);
    REQUIRE(ownerReports.turnUpdatesOnForObjectInstanceRateReports.front().updateRateDesignator ==
            L"Medium");

    REQUIRE_NOTHROW(subscriber->unsubscribeObjectClassAttributes(soda, flavorOnly));
    drainOwnerCallbacks();
    REQUIRE(ownerReports.turnUpdatesOffForObjectInstanceReports.size() == 1U);
    REQUIRE(ownerReports.turnUpdatesOffForObjectInstanceReports.front().objectInstance ==
            objectInstance);
    REQUIRE(ownerReports.turnUpdatesOffForObjectInstanceReports.front().attributes ==
            flavorOnly);

    REQUIRE_NOTHROW(subscriber->resignFederationExecution(NO_ACTION));
    REQUIRE_NOTHROW(owner->resignFederationExecution(CANCEL_THEN_DELETE_THEN_DIVEST));
    REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
    REQUIRE_NOTHROW(owner->disconnect());
    REQUIRE_NOTHROW(subscriber->disconnect());
  };

  SECTION("HLA_EVOKED") {
    runScenario(HLA_EVOKED);
  }
  SECTION("HLA_IMMEDIATE") {
    runScenario(HLA_IMMEDIATE);
  }
}

}  // namespace
