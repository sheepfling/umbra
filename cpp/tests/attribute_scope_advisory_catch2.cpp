#include <atomic>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#include <catch2/catch_test_macros.hpp>
#include <RTI/NullFederateAmbassador.h>
#include <RTI/RTI1516.h>

#ifndef UMBRA_SOURCE_DIRECTORY
#error "The regional scope-advisory tests require the Umbra source directory."
#endif

namespace {

using rti1516_2025::AttributeHandle;
using rti1516_2025::AttributeHandleSet;
using rti1516_2025::AttributeHandleSetRegionHandleSetPairVector;
using rti1516_2025::CallbackModel;
using rti1516_2025::DimensionHandleSet;
using rti1516_2025::FederateAmbassador;
using rti1516_2025::FederateHandle;
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

class ScopeFederateAmbassador final : public NullFederateAmbassador {
 public:
  struct DiscoveryReport {
    ObjectInstanceHandle objectInstance;
  };

  struct ScopeReport {
    ObjectInstanceHandle objectInstance;
    AttributeHandleSet attributes;
  };

  void discoverObjectInstance(
      ObjectInstanceHandle const& objectInstance,
      ObjectClassHandle const&,
      std::wstring const&,
      FederateHandle const&) override {
    discoveries.push_back({objectInstance});
  }

  void attributesInScope(
      ObjectInstanceHandle const& objectInstance,
      AttributeHandleSet const& attributes) override {
    inScope.push_back({objectInstance, attributes});
  }

  void attributesOutOfScope(
      ObjectInstanceHandle const& objectInstance,
      AttributeHandleSet const& attributes) override {
    outOfScope.push_back({objectInstance, attributes});
  }

  std::vector<DiscoveryReport> discoveries;
  std::vector<ScopeReport> inScope;
  std::vector<ScopeReport> outOfScope;
};

std::unique_ptr<RTIambassador> makeRti() {
  RTIambassadorFactory factory;
  return factory.createRTIambassador();
}

std::filesystem::path resourcePath(std::filesystem::path const& relativePath) {
  return std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data" /
      relativePath;
}

std::wstring nextFederationName() {
  static std::atomic_uint64_t sequence{0U};
  return L"attribute-scope-advisory-" +
      std::to_wstring(sequence.fetch_add(1U, std::memory_order_relaxed));
}

void setRegionBounds(
    RTIambassador& rti,
    rti1516_2025::RegionHandle const& region,
    rti1516_2025::DimensionHandle const& dimension,
    unsigned long lower,
    unsigned long upper) {
  REQUIRE_NOTHROW(rti.setRangeBounds(region, dimension, RangeBounds(lower, upper)));
  REQUIRE_NOTHROW(rti.commitRegionModifications(RegionHandleSet{region}));
}

TEST_CASE(
    "Embedded regional object scope callbacks follow 2025 region, association, and subscription changes",
    "[integration][development-profile][federation-management][object-management][ddm]"
    "[callbacks][attribute-scope-advisory][callback-immediate][callback-suppression]"
    "[rti.service.get-attribute-scope-advisory-switch]"
    "[rti.service.set-attribute-scope-advisory-switch]"
    "[rti.service.commit-region-modifications]"
    "[rti.service.associate-regions-for-updates]"
    "[rti.service.unassociate-regions-for-updates]"
    "[rti.service.subscribe-object-class-attributes]"
    "[rti.service.unsubscribe-object-class-attributes]"
    "[rti.service.subscribe-object-class-attributes-with-regions]"
    "[rti.service.unsubscribe-object-class-attributes-with-regions]"
    "[federate.callback.attributes-in-scope]"
    "[federate.callback.attributes-out-of-scope]") {
  auto runScenario = [](CallbackModel const callbackModel) {
    ScopeFederateAmbassador ownerReports;
    ScopeFederateAmbassador subscriberReports;
    auto owner = makeRti();
    auto subscriber = makeRti();
    auto const federationName = nextFederationName();
    auto const fomModule = resourcePath("regional-ownership-fanout-fom.xml").wstring();

    REQUIRE_NOTHROW(owner->connect(ownerReports, callbackModel));
    REQUIRE_NOTHROW(subscriber->connect(subscriberReports, callbackModel));
    REQUIRE_NOTHROW(owner->createFederationExecution(
        federationName,
        fomModule,
        L"HLAinteger64Time"));
    REQUIRE_NOTHROW(owner->joinFederationExecution(
        L"scope-owner", L"scope-owner", federationName));
    REQUIRE_NOTHROW(subscriber->joinFederationExecution(
        L"scope-subscriber", L"scope-subscriber", federationName));

    auto const objectClass = owner->getObjectClassHandle(
        L"HLAobjectRoot.UmbraRegionalOwnershipFanout");
    auto const providerA = owner->getAttributeHandle(objectClass, L"ProviderAValue");
    auto const providerB = owner->getAttributeHandle(objectClass, L"ProviderBValue");
    auto const dimension = owner->getDimensionHandle(L"UmbraRegionX");
    REQUIRE(objectClass.isValid());
    REQUIRE(providerA.isValid());
    REQUIRE(providerB.isValid());
    REQUIRE(dimension.isValid());

    AttributeHandleSet const attributes{providerA, providerB};
    REQUIRE_NOTHROW(owner->publishObjectClassAttributes(objectClass, attributes));

    auto const ownerOverlap = owner->createRegion(DimensionHandleSet{dimension});
    auto const ownerDisjoint = owner->createRegion(DimensionHandleSet{dimension});
    auto const subscriberRegion = subscriber->createRegion(DimensionHandleSet{dimension});
    REQUIRE_NOTHROW(setRegionBounds(*owner, ownerOverlap, dimension, 0UL, 2UL));
    REQUIRE_NOTHROW(setRegionBounds(*owner, ownerDisjoint, dimension, 5UL, 7UL));
    REQUIRE_NOTHROW(setRegionBounds(*subscriber, subscriberRegion, dimension, 0UL, 2UL));

    AttributeHandleSetRegionHandleSetPairVector const ownerOverlapPair{{
        attributes,
        RegionHandleSet{ownerOverlap},
    }};
    AttributeHandleSetRegionHandleSetPairVector const ownerDisjointPair{{
        attributes,
        RegionHandleSet{ownerDisjoint},
    }};
    AttributeHandleSetRegionHandleSetPairVector const subscriberPair{{
        attributes,
        RegionHandleSet{subscriberRegion},
    }};

    // Scope advisories are per-federate and independent of Attribute Relevance.
    REQUIRE_FALSE(subscriber->getAttributeScopeAdvisorySwitch());
    REQUIRE_NOTHROW(subscriber->setAttributeScopeAdvisorySwitch(true));
    REQUIRE(subscriber->getAttributeScopeAdvisorySwitch());
    REQUIRE_NOTHROW(subscriber->subscribeObjectClassAttributesWithRegions(
        objectClass,
        subscriberPair,
        true));

    ObjectInstanceHandle objectInstance;
    REQUIRE_NOTHROW(objectInstance = owner->registerObjectInstanceWithRegions(
        objectClass,
        ownerOverlapPair));
    if (callbackModel == HLA_EVOKED) {
      static_cast<void>(subscriber->evokeMultipleCallbacks(0.0, 0.0));
    }
    REQUIRE(subscriberReports.discoveries.size() == 1U);
    REQUIRE(subscriberReports.discoveries.front().objectInstance == objectInstance);
    subscriberReports.discoveries.clear();
    subscriberReports.inScope.clear();
    subscriberReports.outOfScope.clear();

    auto drainSubscriber = [&] {
      if (callbackModel == HLA_EVOKED) {
        while (subscriber->evokeMultipleCallbacks(0.0, 0.0)) {
        }
      }
    };
    auto requireSingleScopeReport = [&](std::vector<ScopeFederateAmbassador::ScopeReport>& reports) {
      drainSubscriber();
      REQUIRE(reports.size() == 1U);
      REQUIRE(reports.front().objectInstance == objectInstance);
      REQUIRE(reports.front().attributes == attributes);
      reports.clear();
    };

    // Add a second, disjoint source region, then remove the overlapping one.
    // The resulting grouped Attributes Out Of Scope callback exercises the
    // update-region association path for both attributes at once.
    REQUIRE_NOTHROW(owner->associateRegionsForUpdates(objectInstance, ownerDisjointPair));
    REQUIRE_NOTHROW(owner->unassociateRegionsForUpdates(objectInstance, ownerOverlapPair));
    requireSingleScopeReport(subscriberReports.outOfScope);
    REQUIRE(subscriberReports.inScope.empty());

    // Reintroduce overlap and then change the committed receiver region before
    // an evoked callback is entered.  The queued In callback is stale and must
    // be suppressed; the valid Out callback from the committed mutation remains.
    REQUIRE_NOTHROW(owner->associateRegionsForUpdates(objectInstance, ownerOverlapPair));
    if (callbackModel == HLA_IMMEDIATE) {
      requireSingleScopeReport(subscriberReports.inScope);
    }
    REQUIRE_NOTHROW(subscriber->setRangeBounds(
        subscriberRegion,
        dimension,
        RangeBounds(8UL, 9UL)));
    REQUIRE_NOTHROW(subscriber->commitRegionModifications(RegionHandleSet{subscriberRegion}));
    drainSubscriber();
    REQUIRE(subscriberReports.inScope.empty());
    REQUIRE(subscriberReports.outOfScope.size() == 1U);
    REQUIRE(subscriberReports.outOfScope.front().objectInstance == objectInstance);
    REQUIRE(subscriberReports.outOfScope.front().attributes == attributes);
    subscriberReports.outOfScope.clear();

    // Moving the committed subscription region back into overlap is a distinct
    // region-modification transition and must group both changed attributes.
    REQUIRE_NOTHROW(subscriber->setRangeBounds(
        subscriberRegion,
        dimension,
        RangeBounds(0UL, 2UL)));
    REQUIRE_NOTHROW(subscriber->commitRegionModifications(RegionHandleSet{subscriberRegion}));
    requireSingleScopeReport(subscriberReports.inScope);
    REQUIRE(subscriberReports.outOfScope.empty());

    // Replace the regional declaration with ordinary scope, then remove it.
    // These two transitions exercise the ordinary/regional subscription forms
    // without changing the source object's region association.
    REQUIRE_NOTHROW(subscriber->unsubscribeObjectClassAttributesWithRegions(
        objectClass,
        subscriberPair));
    requireSingleScopeReport(subscriberReports.outOfScope);
    REQUIRE_NOTHROW(subscriber->subscribeObjectClassAttributes(
        objectClass,
        attributes,
        true));
    requireSingleScopeReport(subscriberReports.inScope);
    REQUIRE_NOTHROW(subscriber->unsubscribeObjectClassAttributes(objectClass, attributes));
    requireSingleScopeReport(subscriberReports.outOfScope);

    // Restoring the regional declaration creates one grouped In callback.  A
    // later move to the disjoint source range is intentionally separated from
    // the default-region boundary below.
    REQUIRE_NOTHROW(subscriber->subscribeObjectClassAttributesWithRegions(
        objectClass,
        subscriberPair,
        true));
    requireSingleScopeReport(subscriberReports.inScope);

    // The source is currently associated with both owner regions and the
    // receiver overlaps the first. Move the receiver to the second overlap,
    // then remove the first source association. Both states stay in scope.
    REQUIRE_NOTHROW(subscriber->setRangeBounds(
        subscriberRegion,
        dimension,
        RangeBounds(5UL, 7UL)));
    REQUIRE_NOTHROW(subscriber->commitRegionModifications(RegionHandleSet{subscriberRegion}));
    REQUIRE(subscriberReports.inScope.empty());
    REQUIRE(subscriberReports.outOfScope.empty());
    REQUIRE_NOTHROW(owner->unassociateRegionsForUpdates(objectInstance, ownerOverlapPair));
    REQUIRE(subscriberReports.inScope.empty());
    REQUIRE(subscriberReports.outOfScope.empty());

    // Removing the final explicit source association replaces it with the
    // default realization, which overlaps the existing regional subscription;
    // that replacement is not a scope transition and emits no callback.
    REQUIRE_NOTHROW(owner->unassociateRegionsForUpdates(objectInstance, ownerDisjointPair));
    REQUIRE(subscriberReports.inScope.empty());
    REQUIRE(subscriberReports.outOfScope.empty());

    // The switch gate suppresses a queued transition. Re-enable it and the
    // same regional declaration produces one fresh In callback.
    REQUIRE_NOTHROW(subscriber->setAttributeScopeAdvisorySwitch(false));
    REQUIRE_FALSE(subscriber->getAttributeScopeAdvisorySwitch());
    REQUIRE_NOTHROW(subscriber->unsubscribeObjectClassAttributesWithRegions(
        objectClass,
        subscriberPair));
    drainSubscriber();
    REQUIRE(subscriberReports.inScope.empty());
    REQUIRE(subscriberReports.outOfScope.empty());
    REQUIRE_NOTHROW(subscriber->setAttributeScopeAdvisorySwitch(true));
    REQUIRE(subscriber->getAttributeScopeAdvisorySwitch());
    REQUIRE_NOTHROW(subscriber->subscribeObjectClassAttributesWithRegions(
        objectClass,
        subscriberPair,
        true));
    requireSingleScopeReport(subscriberReports.inScope);

    REQUIRE_NOTHROW(subscriber->setAttributeScopeAdvisorySwitch(false));
    REQUIRE_NOTHROW(subscriber->unsubscribeObjectClassAttributesWithRegions(
        objectClass,
        subscriberPair));
    REQUIRE_NOTHROW(owner->deleteRegion(ownerOverlap));
    REQUIRE_NOTHROW(owner->deleteRegion(ownerDisjoint));
    REQUIRE_NOTHROW(subscriber->deleteRegion(subscriberRegion));
    REQUIRE_NOTHROW(subscriber->resignFederationExecution(NO_ACTION));
    REQUIRE_NOTHROW(owner->resignFederationExecution(NO_ACTION));
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
