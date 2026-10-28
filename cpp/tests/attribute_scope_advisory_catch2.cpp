#include <atomic>
#include "hla_test_names.hpp"
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

class ScopeFederateAmbassador : public NullFederateAmbassador {
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
    "[callbacks][attribute-scope-advisory][implicit-self-unsubscribe-no-out-of-scope-advisory]"
    "[callback-immediate][callback-suppression]"
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
        umbra::test::hla::wide::fom::regional_ownership_fanout);
    auto const providerA = owner->getAttributeHandle(objectClass, umbra::test::hla::wide::fixture::provider_a_value);
    auto const providerB = owner->getAttributeHandle(objectClass, umbra::test::hla::wide::fixture::provider_b_value);
    auto const dimension = owner->getDimensionHandle(umbra::test::hla::wide::fixture::umbra_region_x);
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

    // Clear the explicit source associations before replacing the regional
    // declaration with ordinary scope.  An ordinary subscription uses the
    // implicit default source realization; it does not match an explicitly
    // associated source region.
    // §10.1.3 treats a federate's own removal of its final subscription as
    // implicit out-of-scope, so it must not receive an Out advisory for that
    // transition even while its Attribute Scope Advisory Switch is enabled.
    REQUIRE_NOTHROW(subscriber->unsubscribeObjectClassAttributesWithRegions(
        objectClass,
        subscriberPair));
    drainSubscriber();
    REQUIRE(subscriberReports.outOfScope.empty());
    REQUIRE_NOTHROW(owner->unassociateRegionsForUpdates(
        objectInstance,
        ownerOverlapPair));
    REQUIRE_NOTHROW(owner->unassociateRegionsForUpdates(
        objectInstance,
        ownerDisjointPair));
    REQUIRE_NOTHROW(subscriber->subscribeObjectClassAttributes(
        objectClass,
        attributes,
        true));
    requireSingleScopeReport(subscriberReports.inScope);
    REQUIRE_NOTHROW(subscriber->unsubscribeObjectClassAttributes(objectClass, attributes));
    drainSubscriber();
    REQUIRE(subscriberReports.outOfScope.empty());

    // Restoring the regional declaration creates one grouped In callback.  A
    // later move to the disjoint source range is intentionally separated from
    // the default-region boundary below.
    REQUIRE_NOTHROW(subscriber->subscribeObjectClassAttributesWithRegions(
        objectClass,
        subscriberPair,
        true));
    requireSingleScopeReport(subscriberReports.inScope);

    REQUIRE_NOTHROW(owner->associateRegionsForUpdates(
        objectInstance,
        ownerOverlapPair));
    REQUIRE_NOTHROW(owner->associateRegionsForUpdates(
        objectInstance,
        ownerDisjointPair));

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

class TransferScopeFederateAmbassador final : public ScopeFederateAmbassador {
 public:
  struct ReflectionReport {
    ObjectInstanceHandle objectInstance;
    rti1516_2025::AttributeHandleValueMap attributeValues;
  };

  void reflectAttributeValues(
      ObjectInstanceHandle const& objectInstance,
      rti1516_2025::AttributeHandleValueMap const& attributeValues,
      rti1516_2025::VariableLengthData const&,
      rti1516_2025::TransportationTypeHandle const&,
      FederateHandle const&,
      RegionHandleSet const*) override {
    reflections.push_back({objectInstance, attributeValues});
  }

  std::vector<ReflectionReport> reflections;
};

TEST_CASE(
    "Embedded ownership transfer keeps a regionally subscribed attribute continuously in scope",
    "[integration][development-profile][2025][ownership-management][ddm]"
    "[ownership-continuous-scope-transfer][attribute-scope-advisory]"
    "[rti.service.negotiated-attribute-ownership-divestiture]"
    "[rti.service.attribute-ownership-divestiture-if-wanted]"
    "[federate.callback.attributes-in-scope]"
    "[federate.callback.attributes-out-of-scope]") {
  auto runTransfer = [](CallbackModel callbackModel, bool negotiated) {
    ScopeFederateAmbassador ownerReports;
    ScopeFederateAmbassador successorReports;
    TransferScopeFederateAmbassador subscriberReports;
    auto owner = makeRti();
    auto successor = makeRti();
    auto subscriber = makeRti();
    auto const federationName = nextFederationName();
    auto const fomModule = resourcePath("regional-ownership-fanout-fom.xml").wstring();

    REQUIRE_NOTHROW(owner->connect(ownerReports, callbackModel));
    REQUIRE_NOTHROW(successor->connect(successorReports, callbackModel));
    REQUIRE_NOTHROW(subscriber->connect(subscriberReports, callbackModel));
    REQUIRE_NOTHROW(owner->createFederationExecution(
        federationName, fomModule, L"HLAinteger64Time"));
    REQUIRE_NOTHROW(owner->joinFederationExecution(
        L"continuous-scope-owner", L"scope-owner", federationName));
    REQUIRE_NOTHROW(successor->joinFederationExecution(
        L"continuous-scope-successor", L"scope-owner", federationName));
    REQUIRE_NOTHROW(subscriber->joinFederationExecution(
        L"continuous-scope-subscriber", L"scope-subscriber", federationName));

    auto const objectClass = owner->getObjectClassHandle(
        umbra::test::hla::wide::fom::regional_ownership_fanout);
    auto const attribute = owner->getAttributeHandle(objectClass, umbra::test::hla::wide::fixture::provider_a_value);
    auto const dimension = owner->getDimensionHandle(umbra::test::hla::wide::fixture::umbra_region_x);
    REQUIRE(objectClass.isValid());
    REQUIRE(attribute.isValid());
    REQUIRE(dimension.isValid());
    AttributeHandleSet const attributes{attribute};
    REQUIRE_NOTHROW(owner->publishObjectClassAttributes(objectClass, attributes));
    REQUIRE_NOTHROW(successor->publishObjectClassAttributes(objectClass, attributes));
    REQUIRE_NOTHROW(successor->subscribeObjectClassAttributes(objectClass, attributes));

    auto const ownerRegion = owner->createRegion(DimensionHandleSet{dimension});
    auto const successorRegion = successor->createRegion(DimensionHandleSet{dimension});
    auto const subscriberRegion = subscriber->createRegion(DimensionHandleSet{dimension});
    REQUIRE_NOTHROW(setRegionBounds(*owner, ownerRegion, dimension, 0UL, 2UL));
    REQUIRE_NOTHROW(setRegionBounds(*successor, successorRegion, dimension, 0UL, 2UL));
    REQUIRE_NOTHROW(setRegionBounds(*subscriber, subscriberRegion, dimension, 0UL, 2UL));
    AttributeHandleSetRegionHandleSetPairVector const ownerPair{{
        attributes, RegionHandleSet{ownerRegion},
    }};
    AttributeHandleSetRegionHandleSetPairVector const successorPair{{
        attributes, RegionHandleSet{successorRegion},
    }};
    AttributeHandleSetRegionHandleSetPairVector const subscriberPair{{
        attributes, RegionHandleSet{subscriberRegion},
    }};
    REQUIRE_NOTHROW(subscriber->setAttributeScopeAdvisorySwitch(true));
    REQUIRE_NOTHROW(subscriber->subscribeObjectClassAttributesWithRegions(
        objectClass, subscriberPair, true));

    ObjectInstanceHandle objectInstance;
    REQUIRE_NOTHROW(objectInstance = owner->registerObjectInstanceWithRegions(
        objectClass, ownerPair));
    auto drain = [&](RTIambassador& rti) {
      if (callbackModel == HLA_EVOKED) {
        while (rti.evokeMultipleCallbacks(0.0, 0.0)) {
        }
      }
    };
    drain(*successor);
    drain(*subscriber);
    REQUIRE(successorReports.discoveries.size() == 1U);
    REQUIRE(subscriberReports.discoveries.size() == 1U);
    REQUIRE(successorReports.discoveries.front().objectInstance == objectInstance);
    REQUIRE(subscriberReports.discoveries.front().objectInstance == objectInstance);
    REQUIRE(owner->isAttributeOwnedByFederate(objectInstance, attribute));
    REQUIRE_FALSE(successor->isAttributeOwnedByFederate(objectInstance, attribute));
    subscriberReports.inScope.clear();
    subscriberReports.outOfScope.clear();

    rti1516_2025::VariableLengthData const tag;
    unsigned char const originalValue[] = {0x11};
    rti1516_2025::AttributeHandleValueMap originalValues;
    originalValues.emplace(
        attribute,
        rti1516_2025::VariableLengthData(originalValue, sizeof(originalValue)));
    REQUIRE_NOTHROW(owner->updateAttributeValues(
        objectInstance, originalValues, tag));
    drain(*subscriber);
    REQUIRE(subscriberReports.reflections.size() == 1U);
    REQUIRE(subscriberReports.reflections.front().objectInstance == objectInstance);
    REQUIRE(subscriberReports.reflections.front().attributeValues.contains(attribute));
    subscriberReports.reflections.clear();

    if (negotiated) {
      REQUIRE_NOTHROW(successor->attributeOwnershipAcquisition(
          objectInstance, attributes, tag));
      REQUIRE_NOTHROW(owner->negotiatedAttributeOwnershipDivestiture(
          objectInstance, attributes, tag));
      drain(*owner);
      drain(*subscriber);
      REQUIRE(subscriberReports.outOfScope.empty());
      REQUIRE_NOTHROW(owner->confirmDivestiture(
          objectInstance, attributes, tag));
    } else {
      REQUIRE_NOTHROW(successor->attributeOwnershipAcquisition(
          objectInstance, attributes, tag));
      AttributeHandleSet divested;
      REQUIRE_NOTHROW(owner->attributeOwnershipDivestitureIfWanted(
          objectInstance, attributes, tag, divested));
      REQUIRE(divested == attributes);
    }
    drain(*owner);
    drain(*successor);
    drain(*subscriber);
    REQUIRE_FALSE(owner->isAttributeOwnedByFederate(objectInstance, attribute));
    REQUIRE(successor->isAttributeOwnedByFederate(objectInstance, attribute));
    REQUIRE(subscriberReports.outOfScope.empty());
    REQUIRE(subscriberReports.inScope.empty());

    unsigned char const successorValue[] = {0x22};
    rti1516_2025::AttributeHandleValueMap successorValues;
    successorValues.emplace(
        attribute,
        rti1516_2025::VariableLengthData(successorValue, sizeof(successorValue)));
    REQUIRE_NOTHROW(successor->updateAttributeValues(
        objectInstance, successorValues, tag));
    drain(*subscriber);
    REQUIRE(subscriberReports.reflections.size() == 1U);
    REQUIRE(subscriberReports.reflections.front().objectInstance == objectInstance);
    REQUIRE(subscriberReports.reflections.front().attributeValues.contains(attribute));
    REQUIRE(subscriberReports.outOfScope.empty());
    REQUIRE(subscriberReports.inScope.empty());

    // A subsequent real regional scope transition proves that the observer's
    // advisory switch and callback path were live during the transfer.
    REQUIRE_NOTHROW(successor->associateRegionsForUpdates(
        objectInstance, successorPair));
    drain(*subscriber);
    REQUIRE(subscriberReports.outOfScope.empty());
    REQUIRE(subscriberReports.inScope.empty());
    REQUIRE_NOTHROW(setRegionBounds(
        *subscriber, subscriberRegion, dimension, 5UL, 7UL));
    drain(*subscriber);
    REQUIRE(subscriberReports.outOfScope.size() == 1U);
    REQUIRE(subscriberReports.outOfScope.front().objectInstance == objectInstance);
    REQUIRE(subscriberReports.outOfScope.front().attributes == attributes);
    subscriberReports.outOfScope.clear();
    REQUIRE_NOTHROW(setRegionBounds(
        *subscriber, subscriberRegion, dimension, 0UL, 2UL));
    drain(*subscriber);
    REQUIRE(subscriberReports.inScope.size() == 1U);
    REQUIRE(subscriberReports.inScope.front().objectInstance == objectInstance);
    REQUIRE(subscriberReports.inScope.front().attributes == attributes);

    REQUIRE_NOTHROW(subscriber->setAttributeScopeAdvisorySwitch(false));
    REQUIRE_NOTHROW(subscriber->unsubscribeObjectClassAttributesWithRegions(
        objectClass, subscriberPair));
    REQUIRE_NOTHROW(successor->unsubscribeObjectClassAttributes(
        objectClass, attributes));
    REQUIRE_NOTHROW(successor->unassociateRegionsForUpdates(
        objectInstance, successorPair));
    REQUIRE_NOTHROW(owner->deleteObjectInstance(objectInstance, tag));
    REQUIRE_NOTHROW(subscriber->deleteRegion(subscriberRegion));
    REQUIRE_NOTHROW(successor->deleteRegion(successorRegion));
    REQUIRE_NOTHROW(owner->deleteRegion(ownerRegion));
    REQUIRE_NOTHROW(subscriber->resignFederationExecution(NO_ACTION));
    REQUIRE_NOTHROW(successor->resignFederationExecution(NO_ACTION));
    REQUIRE_NOTHROW(owner->resignFederationExecution(NO_ACTION));
    REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
    REQUIRE_NOTHROW(subscriber->disconnect());
    REQUIRE_NOTHROW(successor->disconnect());
    REQUIRE_NOTHROW(owner->disconnect());
  };

  SECTION("Negotiated Divestiture under HLA_EVOKED") {
    runTransfer(HLA_EVOKED, true);
  }
  SECTION("Negotiated Divestiture under HLA_IMMEDIATE") {
    runTransfer(HLA_IMMEDIATE, true);
  }
  SECTION("Divestiture If Wanted under HLA_EVOKED") {
    runTransfer(HLA_EVOKED, false);
  }
  SECTION("Divestiture If Wanted under HLA_IMMEDIATE") {
    runTransfer(HLA_IMMEDIATE, false);
  }
}

TEST_CASE(
    "Associate Regions For Updates rejects a stale region without changing existing associations",
    "[integration][development-profile][2025][federation-management][object-management][ddm]"
    "[ddm-associate-regions-update-invalid-region-2025]"
    "[rti.service.associate-regions-for-updates]"
    "[rti.service.commit-region-modifications]"
    "[rti.service.subscribe-object-class-attributes-with-regions]"
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
        federationName, fomModule, L"HLAinteger64Time"));
    REQUIRE_NOTHROW(owner->joinFederationExecution(
        L"stale-association-owner", L"stale-association-owner", federationName));
    REQUIRE_NOTHROW(subscriber->joinFederationExecution(
        L"stale-association-subscriber", L"stale-association-subscriber", federationName));

    auto const objectClass = owner->getObjectClassHandle(
        umbra::test::hla::wide::fom::regional_ownership_fanout);
    auto const attribute = owner->getAttributeHandle(objectClass, umbra::test::hla::wide::fixture::provider_a_value);
    auto const dimension = owner->getDimensionHandle(umbra::test::hla::wide::fixture::umbra_region_x);
    REQUIRE(objectClass.isValid());
    REQUIRE(attribute.isValid());
    REQUIRE(dimension.isValid());
    AttributeHandleSet const attributes{attribute};
    REQUIRE_NOTHROW(owner->publishObjectClassAttributes(objectClass, attributes));

    auto const ownerRegion = owner->createRegion(DimensionHandleSet{dimension});
    auto const staleRegion = owner->createRegion(DimensionHandleSet{dimension});
    auto const subscriberRegion = subscriber->createRegion(DimensionHandleSet{dimension});
    REQUIRE_NOTHROW(setRegionBounds(*owner, ownerRegion, dimension, 0UL, 2UL));
    REQUIRE_NOTHROW(setRegionBounds(*owner, staleRegion, dimension, 0UL, 2UL));
    REQUIRE_NOTHROW(setRegionBounds(*subscriber, subscriberRegion, dimension, 0UL, 2UL));
    AttributeHandleSetRegionHandleSetPairVector const ownerPair{{
        attributes, RegionHandleSet{ownerRegion},
    }};
    AttributeHandleSetRegionHandleSetPairVector const stalePair{{
        attributes, RegionHandleSet{staleRegion},
    }};
    AttributeHandleSetRegionHandleSetPairVector const subscriberPair{{
        attributes, RegionHandleSet{subscriberRegion},
    }};

    // Keep a live association on the object, but make the second designator a
    // stale handle so the failed call must leave that live association intact.
    REQUIRE_NOTHROW(owner->deleteRegion(staleRegion));
    REQUIRE_NOTHROW(subscriber->setAttributeScopeAdvisorySwitch(true));
    REQUIRE_NOTHROW(subscriber->subscribeObjectClassAttributesWithRegions(
        objectClass, subscriberPair, true));
    ObjectInstanceHandle objectInstance;
    REQUIRE_NOTHROW(objectInstance = owner->registerObjectInstanceWithRegions(
        objectClass, ownerPair));

    auto drain = [&](RTIambassador& rti) {
      if (callbackModel == HLA_EVOKED) {
        while (rti.evokeMultipleCallbacks(0.0, 0.0)) {
        }
      }
    };
    drain(*subscriber);
    REQUIRE(subscriberReports.discoveries.size() == 1U);
    REQUIRE(subscriberReports.discoveries.front().objectInstance == objectInstance);
    subscriberReports.inScope.clear();
    subscriberReports.outOfScope.clear();

    REQUIRE_THROWS_AS(
        owner->associateRegionsForUpdates(objectInstance, stalePair),
        rti1516_2025::InvalidRegion);

    // If the failed call disturbed the original association, moving its live
    // region away from the subscription would not take this attribute out of
    // scope.
    REQUIRE_NOTHROW(setRegionBounds(*owner, ownerRegion, dimension, 5UL, 7UL));
    drain(*subscriber);
    REQUIRE(subscriberReports.inScope.empty());
    REQUIRE(subscriberReports.outOfScope.size() == 1U);
    REQUIRE(subscriberReports.outOfScope.front().objectInstance == objectInstance);
    REQUIRE(subscriberReports.outOfScope.front().attributes == attributes);

    REQUIRE_NOTHROW(subscriber->setAttributeScopeAdvisorySwitch(false));
    REQUIRE_NOTHROW(subscriber->unsubscribeObjectClassAttributesWithRegions(
        objectClass, subscriberPair));
    REQUIRE_NOTHROW(owner->unassociateRegionsForUpdates(objectInstance, ownerPair));
    REQUIRE_NOTHROW(owner->deleteObjectInstance(objectInstance, rti1516_2025::VariableLengthData{}));
    REQUIRE_NOTHROW(subscriber->deleteRegion(subscriberRegion));
    REQUIRE_NOTHROW(owner->deleteRegion(ownerRegion));
    REQUIRE_NOTHROW(subscriber->resignFederationExecution(NO_ACTION));
    REQUIRE_NOTHROW(owner->resignFederationExecution(NO_ACTION));
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

TEST_CASE(
    "Associate Regions For Updates leaves an existing region association unchanged when repeated",
    "[integration][development-profile][2025][federation-management][object-management][ddm]"
    "[ddm-associate-regions-duplicate-association-idempotent-2025]"
    "[rti.service.associate-regions-for-updates]"
    "[rti.service.unassociate-regions-for-updates]"
    "[rti.service.subscribe-object-class-attributes-with-regions]"
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
        federationName, fomModule, L"HLAinteger64Time"));
    REQUIRE_NOTHROW(owner->joinFederationExecution(
        L"duplicate-association-owner", L"duplicate-association-owner", federationName));
    REQUIRE_NOTHROW(subscriber->joinFederationExecution(
        L"duplicate-association-subscriber", L"duplicate-association-subscriber", federationName));

    auto const objectClass = owner->getObjectClassHandle(
        umbra::test::hla::wide::fom::regional_ownership_fanout);
    auto const attribute = owner->getAttributeHandle(objectClass, umbra::test::hla::wide::fixture::provider_a_value);
    auto const dimension = owner->getDimensionHandle(umbra::test::hla::wide::fixture::umbra_region_x);
    REQUIRE(objectClass.isValid());
    REQUIRE(attribute.isValid());
    REQUIRE(dimension.isValid());
    AttributeHandleSet const attributes{attribute};
    REQUIRE_NOTHROW(owner->publishObjectClassAttributes(objectClass, attributes));

    auto const overlappingRegion = owner->createRegion(DimensionHandleSet{dimension});
    auto const disjointRegion = owner->createRegion(DimensionHandleSet{dimension});
    auto const subscriberRegion = subscriber->createRegion(DimensionHandleSet{dimension});
    REQUIRE_NOTHROW(setRegionBounds(*owner, overlappingRegion, dimension, 0UL, 2UL));
    REQUIRE_NOTHROW(setRegionBounds(*owner, disjointRegion, dimension, 5UL, 7UL));
    REQUIRE_NOTHROW(setRegionBounds(*subscriber, subscriberRegion, dimension, 0UL, 2UL));
    AttributeHandleSetRegionHandleSetPairVector const overlappingPair{{
        attributes, RegionHandleSet{overlappingRegion},
    }};
    AttributeHandleSetRegionHandleSetPairVector const disjointPair{{
        attributes, RegionHandleSet{disjointRegion},
    }};
    AttributeHandleSetRegionHandleSetPairVector const subscriberPair{{
        attributes, RegionHandleSet{subscriberRegion},
    }};

    REQUIRE_NOTHROW(subscriber->setAttributeScopeAdvisorySwitch(true));
    REQUIRE_NOTHROW(subscriber->subscribeObjectClassAttributesWithRegions(
        objectClass, subscriberPair, true));
    ObjectInstanceHandle objectInstance;
    REQUIRE_NOTHROW(objectInstance = owner->registerObjectInstanceWithRegions(
        objectClass, overlappingPair));

    auto drainSubscriber = [&] {
      if (callbackModel == HLA_EVOKED) {
        while (subscriber->evokeMultipleCallbacks(0.0, 0.0)) {
        }
      }
    };
    drainSubscriber();
    REQUIRE(subscriberReports.discoveries.size() == 1U);
    REQUIRE(subscriberReports.discoveries.front().objectInstance == objectInstance);
    subscriberReports.inScope.clear();
    subscriberReports.outOfScope.clear();

    // Registration already associates the overlapping region. Repeating that
    // association must not leave an extra association behind when removed once.
    REQUIRE_NOTHROW(owner->associateRegionsForUpdates(objectInstance, overlappingPair));
    REQUIRE_NOTHROW(owner->associateRegionsForUpdates(objectInstance, disjointPair));
    REQUIRE_NOTHROW(owner->unassociateRegionsForUpdates(objectInstance, overlappingPair));
    drainSubscriber();
    REQUIRE(subscriberReports.inScope.empty());
    REQUIRE(subscriberReports.outOfScope.size() == 1U);
    REQUIRE(subscriberReports.outOfScope.front().objectInstance == objectInstance);
    REQUIRE(subscriberReports.outOfScope.front().attributes == attributes);

    REQUIRE_NOTHROW(subscriber->setAttributeScopeAdvisorySwitch(false));
    REQUIRE_NOTHROW(subscriber->unsubscribeObjectClassAttributesWithRegions(
        objectClass, subscriberPair));
    REQUIRE_NOTHROW(owner->unassociateRegionsForUpdates(objectInstance, disjointPair));
    REQUIRE_NOTHROW(owner->deleteObjectInstance(
        objectInstance, rti1516_2025::VariableLengthData{}));
    REQUIRE_NOTHROW(subscriber->deleteRegion(subscriberRegion));
    REQUIRE_NOTHROW(owner->deleteRegion(disjointRegion));
    REQUIRE_NOTHROW(owner->deleteRegion(overlappingRegion));
    REQUIRE_NOTHROW(subscriber->resignFederationExecution(NO_ACTION));
    REQUIRE_NOTHROW(owner->resignFederationExecution(NO_ACTION));
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
