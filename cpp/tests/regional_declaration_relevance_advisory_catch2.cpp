#include <catch2/catch_test_macros.hpp>

#include "internal/fom/hla_names.hpp"
#include "hla_test_names.hpp"

#include <RTI/NullFederateAmbassador.h>
#include <RTI/RTI1516.h>
#include <RTI/time/HLAinteger64Time.h>

#include <atomic>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#ifndef UMBRA_SOURCE_DIRECTORY
#error "The regional declaration-relevance focused test requires the Umbra source directory."
#endif

namespace {

namespace standard_hla = umbra::detail::hla::wide;
namespace fixture_hla = umbra::test::hla::wide;

using rti1516_2025::AttributeHandleSet;
using rti1516_2025::AttributeHandleSetRegionHandleSetPairVector;
using rti1516_2025::CallbackModel;
using rti1516_2025::DimensionHandleSet;
using rti1516_2025::HLA_EVOKED;
using rti1516_2025::InteractionClassHandle;
using rti1516_2025::NO_ACTION;
using rti1516_2025::NullFederateAmbassador;
using rti1516_2025::ObjectClassHandle;
using rti1516_2025::ObjectInstanceHandle;
using rti1516_2025::RTIambassador;
using rti1516_2025::RTIambassadorFactory;
using rti1516_2025::RangeBounds;
using rti1516_2025::RegionHandleSet;

struct ObjectClassRelevanceReport final {
  ObjectClassHandle objectClass;
};

struct InteractionRelevanceReport final {
  InteractionClassHandle interactionClass;
};

class RegionalDeclarationFederateAmbassador final : public NullFederateAmbassador {
 public:
  void discoverObjectInstance(
      ObjectInstanceHandle const& objectInstance,
      ObjectClassHandle const&,
      std::wstring const&,
      rti1516_2025::FederateHandle const&) override {
    discoveredObjectInstances.push_back(objectInstance);
  }

  void turnUpdatesOnForObjectInstance(
      ObjectInstanceHandle const& objectInstance,
      AttributeHandleSet const& attributes) override {
    turnUpdatesOnReports.push_back({objectInstance, attributes});
  }

  void turnUpdatesOffForObjectInstance(
      ObjectInstanceHandle const& objectInstance,
      AttributeHandleSet const& attributes) override {
    turnUpdatesOffReports.push_back({objectInstance, attributes});
  }

  struct AttributeReport final {
    ObjectInstanceHandle objectInstance;
    AttributeHandleSet attributes;
  };

  void startRegistrationForObjectClass(
      ObjectClassHandle const& objectClass) override {
    startRegistrationForObjectClassReports.push_back({objectClass});
  }

  void stopRegistrationForObjectClass(
      ObjectClassHandle const& objectClass) override {
    stopRegistrationForObjectClassReports.push_back({objectClass});
  }

  void turnInteractionsOn(
      InteractionClassHandle const& interactionClass) override {
    turnInteractionsOnReports.push_back({interactionClass});
  }

  void turnInteractionsOff(
      InteractionClassHandle const& interactionClass) override {
    turnInteractionsOffReports.push_back({interactionClass});
  }

  std::vector<ObjectInstanceHandle> discoveredObjectInstances;
  std::vector<AttributeReport> turnUpdatesOnReports;
  std::vector<AttributeReport> turnUpdatesOffReports;
  std::vector<ObjectClassRelevanceReport> startRegistrationForObjectClassReports;
  std::vector<ObjectClassRelevanceReport> stopRegistrationForObjectClassReports;
  std::vector<InteractionRelevanceReport> turnInteractionsOnReports;
  std::vector<InteractionRelevanceReport> turnInteractionsOffReports;
};

std::unique_ptr<RTIambassador> makeRti() {
  RTIambassadorFactory factory;
  return factory.createRTIambassador();
}

std::filesystem::path resourcePath(std::filesystem::path const& relativePath) {
  return std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "third_party" /
      "ieee1516.2-2025" / "resources" / relativePath;
}

std::wstring nextFederationName() {
  static std::atomic_uint64_t counter{0};
  return L"umbra-regional-declaration-relevance-" +
      std::to_wstring(++counter);
}

}  // namespace

TEST_CASE(
    "Embedded regional declaration relevance advisories follow active subscriptions",
    "[integration][development-profile][federation-management][ddm]"
    "[rti.service.subscribe-object-class-attributes-with-regions]"
    "[rti.service.unsubscribe-object-class-attributes-with-regions]"
    "[rti.service.subscribe-interaction-class-with-regions]"
    "[rti.service.unsubscribe-interaction-class-with-regions]"
    "[rti.service.start-registration-for-object-class]"
    "[rti.service.stop-registration-for-object-class]"
    "[rti.service.turn-interactions-on][rti.service.turn-interactions-off]"
    "[federate.callback.start-registration-for-object-class]"
    "[federate.callback.stop-registration-for-object-class]"
    "[federate.callback.turn-interactions-on][federate.callback.turn-interactions-off]"
    "[declaration-relevance-advisory-regional][2025]") {
  RegionalDeclarationFederateAmbassador publisherReports;
  RegionalDeclarationFederateAmbassador subscriberReports;
  auto publisher = makeRti();
  auto subscriber = makeRti();
  auto const federationName = nextFederationName();
  auto const restaurantFom =
      resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  auto const switchFom =
      (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" /
       "data" / "switch-support-enabled-fom.xml")
          .wstring();

  REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED));
  REQUIRE_NOTHROW(subscriber->connect(subscriberReports, HLA_EVOKED));
  REQUIRE_NOTHROW(publisher->createFederationExecution(
      federationName,
      std::vector<std::wstring>{restaurantFom, switchFom},
      standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(publisher->joinFederationExecution(
      L"regional-relevance-publisher", L"publisher", federationName));
  REQUIRE_NOTHROW(subscriber->joinFederationExecution(
      L"regional-relevance-subscriber", L"subscriber", federationName));

  auto const objectClass = publisher->getObjectClassHandle(
      fixture_hla::fom::food_drink_soda);
  auto const attribute = publisher->getAttributeHandle(
      objectClass, fixture_hla::fixture::flavor);
  auto const objectDimension =
      publisher->getDimensionHandle(fixture_hla::fixture::soda_flavor);
  auto const interactionClass = publisher->getInteractionClassHandle(
      fixture_hla::fom::main_course_served);
  auto const interactionDimension =
      publisher->getDimensionHandle(fixture_hla::fixture::server_id);
  REQUIRE(objectClass.isValid());
  REQUIRE(attribute.isValid());
  REQUIRE(objectDimension.isValid());
  REQUIRE(interactionClass.isValid());
  REQUIRE(interactionDimension.isValid());
  REQUIRE(publisher->getObjectClassRelevanceAdvisorySwitch());
  REQUIRE(publisher->getInteractionRelevanceAdvisorySwitch());

  REQUIRE_NOTHROW(
      publisher->publishObjectClassAttributes(objectClass, {attribute}));
  REQUIRE_NOTHROW(publisher->publishInteractionClass(interactionClass));

  auto const objectRegion =
      subscriber->createRegion(DimensionHandleSet{objectDimension});
  auto const secondObjectRegion =
      subscriber->createRegion(DimensionHandleSet{objectDimension});
  REQUIRE_NOTHROW(subscriber->setRangeBounds(
      objectRegion, objectDimension, RangeBounds(0UL, 1UL)));
  REQUIRE_NOTHROW(
      subscriber->commitRegionModifications(RegionHandleSet{objectRegion}));
  REQUIRE_NOTHROW(subscriber->setRangeBounds(
      secondObjectRegion, objectDimension, RangeBounds(2UL, 3UL)));
  REQUIRE_NOTHROW(subscriber->commitRegionModifications(
      RegionHandleSet{secondObjectRegion}));
  AttributeHandleSetRegionHandleSetPairVector const objectSubscription{{
      AttributeHandleSet{attribute},
      RegionHandleSet{objectRegion},
  }};
  AttributeHandleSetRegionHandleSetPairVector const secondObjectSubscription{{
      AttributeHandleSet{attribute},
      RegionHandleSet{secondObjectRegion},
  }};

  // A passive regional declaration is retained but does not establish
  // relevance. It must not cause Start Registration at the publisher.
  REQUIRE_NOTHROW(subscriber->subscribeObjectClassAttributesWithRegions(
      objectClass, objectSubscription, false));
  REQUIRE_FALSE(publisher->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(publisherReports.startRegistrationForObjectClassReports.empty());

  // Active/passive state is per (class, attribute, region) triple. Making a
  // different region active establishes relevance without changing the first
  // region's passive state.
  REQUIRE_NOTHROW(subscriber->subscribeObjectClassAttributesWithRegions(
      objectClass, secondObjectSubscription, true));
  REQUIRE_FALSE(publisher->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(publisherReports.startRegistrationForObjectClassReports.size() == 1U);

  // Replacing that active regional triple with passive changes the previous
  // subscription and removes the last active route, so Stop is permitted.
  REQUIRE_NOTHROW(subscriber->subscribeObjectClassAttributesWithRegions(
      objectClass, secondObjectSubscription, false));
  REQUIRE_FALSE(publisher->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(publisherReports.stopRegistrationForObjectClassReports.size() == 1U);

  // Activating the first region independently establishes relevance again.
  REQUIRE_NOTHROW(subscriber->subscribeObjectClassAttributesWithRegions(
      objectClass, objectSubscription, true));
  REQUIRE_FALSE(publisher->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(publisherReports.startRegistrationForObjectClassReports.size() == 2U);
  REQUIRE(publisherReports.startRegistrationForObjectClassReports.front().objectClass ==
          objectClass);

  REQUIRE_NOTHROW(subscriber->unsubscribeObjectClassAttributesWithRegions(
      objectClass, objectSubscription));
  REQUIRE_FALSE(publisher->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(publisherReports.stopRegistrationForObjectClassReports.size() == 2U);
  REQUIRE(publisherReports.stopRegistrationForObjectClassReports.front().objectClass ==
          objectClass);
  REQUIRE_NOTHROW(subscriber->unsubscribeObjectClassAttributesWithRegions(
      objectClass, secondObjectSubscription));
  REQUIRE_FALSE(publisher->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(publisherReports.stopRegistrationForObjectClassReports.size() == 2U);

  auto const interactionRegion =
      subscriber->createRegion(DimensionHandleSet{interactionDimension});
  REQUIRE_NOTHROW(subscriber->setRangeBounds(
      interactionRegion, interactionDimension, RangeBounds(0UL, 1UL)));
  REQUIRE_NOTHROW(subscriber->commitRegionModifications(
      RegionHandleSet{interactionRegion}));

  // Regional interaction declarations use the same active/passive boundary.
  REQUIRE_NOTHROW(subscriber->subscribeInteractionClassWithRegions(
      interactionClass, RegionHandleSet{interactionRegion}, false));
  REQUIRE_FALSE(publisher->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(publisherReports.turnInteractionsOnReports.empty());
  REQUIRE_NOTHROW(subscriber->subscribeInteractionClassWithRegions(
      interactionClass, RegionHandleSet{interactionRegion}, true));
  REQUIRE_FALSE(publisher->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(publisherReports.turnInteractionsOnReports.size() == 1U);
  REQUIRE(publisherReports.turnInteractionsOnReports.front().interactionClass ==
          interactionClass);

  REQUIRE_NOTHROW(subscriber->unsubscribeInteractionClassWithRegions(
      interactionClass, RegionHandleSet{interactionRegion}));
  REQUIRE_FALSE(publisher->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(publisherReports.turnInteractionsOffReports.size() == 1U);
  REQUIRE(publisherReports.turnInteractionsOffReports.front().interactionClass ==
          interactionClass);

  REQUIRE_NOTHROW(subscriber->deleteRegion(objectRegion));
  REQUIRE_NOTHROW(subscriber->deleteRegion(secondObjectRegion));
  REQUIRE_NOTHROW(subscriber->deleteRegion(interactionRegion));
  REQUIRE_NOTHROW(publisher->unpublishInteractionClass(interactionClass));
  REQUIRE_NOTHROW(publisher->unpublishObjectClassAttributes(
      objectClass, {attribute}));
  REQUIRE_NOTHROW(subscriber->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(subscriber->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
}

TEST_CASE(
    "Embedded regional passive subscription changes gate registered-object turn updates per region",
    "[integration][development-profile][federation-management][object-management][ddm]"
    "[callbacks][regional-attribute-relevance][regional-passive-subscription-triple]"
    "[rti.service.subscribe-object-class-attributes-with-regions]"
    "[rti.service.register-object-instance-with-regions]"
    "[rti.service.discover-object-instance]"
    "[federate.callback.turn-updates-on-for-object-instance]"
    "[federate.callback.turn-updates-off-for-object-instance]"
    "[regional-passive-subscription-turn-updates]"
    "[2025]") {
  RegionalDeclarationFederateAmbassador publisherReports;
  RegionalDeclarationFederateAmbassador subscriberReports;
  auto publisher = makeRti();
  auto subscriber = makeRti();
  auto const federationName = nextFederationName();
  auto const restaurantFom =
      resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();

  REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED));
  REQUIRE_NOTHROW(subscriber->connect(subscriberReports, HLA_EVOKED));
  REQUIRE_NOTHROW(publisher->createFederationExecution(
      federationName,
      restaurantFom,
      standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(publisher->joinFederationExecution(
      L"regional-passive-publisher", L"publisher", federationName));
  REQUIRE_NOTHROW(subscriber->joinFederationExecution(
      L"regional-passive-subscriber", L"subscriber", federationName));

  auto const objectClass = publisher->getObjectClassHandle(
      fixture_hla::fom::food_drink_soda);
  auto const attribute = publisher->getAttributeHandle(
      objectClass, fixture_hla::fixture::flavor);
  auto const objectDimension =
      publisher->getDimensionHandle(fixture_hla::fixture::soda_flavor);
  REQUIRE(objectClass.isValid());
  REQUIRE(attribute.isValid());
  REQUIRE(objectDimension.isValid());

  AttributeHandleSet const attributeOnly{attribute};
  REQUIRE_NOTHROW(publisher->publishObjectClassAttributes(
      objectClass, attributeOnly));

  auto const publisherRegion =
      publisher->createRegion(DimensionHandleSet{objectDimension});
  auto const subscriberRegion =
      subscriber->createRegion(DimensionHandleSet{objectDimension});
  auto const disjointSubscriberRegion =
      subscriber->createRegion(DimensionHandleSet{objectDimension});
  REQUIRE_NOTHROW(publisher->setRangeBounds(
      publisherRegion, objectDimension, RangeBounds(0UL, 1UL)));
  REQUIRE_NOTHROW(publisher->commitRegionModifications(
      RegionHandleSet{publisherRegion}));
  REQUIRE_NOTHROW(subscriber->setRangeBounds(
      subscriberRegion, objectDimension, RangeBounds(0UL, 1UL)));
  REQUIRE_NOTHROW(subscriber->commitRegionModifications(
      RegionHandleSet{subscriberRegion}));
  REQUIRE_NOTHROW(subscriber->setRangeBounds(
      disjointSubscriberRegion, objectDimension, RangeBounds(2UL, 3UL)));
  REQUIRE_NOTHROW(subscriber->commitRegionModifications(
      RegionHandleSet{disjointSubscriberRegion}));

  AttributeHandleSetRegionHandleSetPairVector const sourcePair{{
      attributeOnly,
      RegionHandleSet{publisherRegion},
  }};
  AttributeHandleSetRegionHandleSetPairVector const overlappingSubscription{{
      attributeOnly,
      RegionHandleSet{subscriberRegion},
  }};
  AttributeHandleSetRegionHandleSetPairVector const disjointSubscription{{
      attributeOnly,
      RegionHandleSet{disjointSubscriberRegion},
  }};
  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = publisher->registerObjectInstanceWithRegions(
      objectClass, sourcePair));
  REQUIRE(objectInstance.isValid());

  auto drainCallbacks = [&] {
    while (subscriber->evokeMultipleCallbacks(0.0, 0.0)) {
    }
    while (publisher->evokeMultipleCallbacks(0.0, 0.0)) {
    }
  };

  // A passive declaration is retained but does not discover the already
  // registered object or turn updates on for its attribute.
  REQUIRE_NOTHROW(subscriber->subscribeObjectClassAttributesWithRegions(
      objectClass, overlappingSubscription, false));
  drainCallbacks();
  REQUIRE(subscriberReports.discoveredObjectInstances.empty());
  REQUIRE(publisherReports.turnUpdatesOnReports.empty());
  REQUIRE(publisherReports.turnUpdatesOffReports.empty());

  // A separate active region does not make the first (passive) triple
  // relevant.  It is intentionally disjoint from the registered object.
  REQUIRE_NOTHROW(subscriber->subscribeObjectClassAttributesWithRegions(
      objectClass, disjointSubscription, true));
  drainCallbacks();
  REQUIRE(subscriberReports.discoveredObjectInstances.empty());
  REQUIRE(publisherReports.turnUpdatesOnReports.empty());
  REQUIRE(publisherReports.turnUpdatesOffReports.empty());

  // Activating the overlapping triple discovers the object and enables its
  // attribute updates without changing the disjoint subscription.
  REQUIRE_NOTHROW(subscriber->subscribeObjectClassAttributesWithRegions(
      objectClass, overlappingSubscription, true));
  drainCallbacks();
  REQUIRE(subscriberReports.discoveredObjectInstances.size() == 1U);
  REQUIRE(subscriberReports.discoveredObjectInstances.front() == objectInstance);
  REQUIRE(publisherReports.turnUpdatesOnReports.size() == 1U);
  REQUIRE(publisherReports.turnUpdatesOnReports.front().objectInstance ==
          objectInstance);
  REQUIRE(publisherReports.turnUpdatesOnReports.front().attributes ==
          attributeOnly);
  publisherReports.turnUpdatesOnReports.clear();

  // Making only the overlapping triple passive turns updates off, even while
  // the independent disjoint triple remains active.
  REQUIRE_NOTHROW(subscriber->subscribeObjectClassAttributesWithRegions(
      objectClass, overlappingSubscription, false));
  drainCallbacks();
  REQUIRE(publisherReports.turnUpdatesOffReports.size() == 1U);
  REQUIRE(publisherReports.turnUpdatesOffReports.front().objectInstance ==
          objectInstance);
  REQUIRE(publisherReports.turnUpdatesOffReports.front().attributes ==
          attributeOnly);
  publisherReports.turnUpdatesOffReports.clear();

  // Re-enabling the same triple reuses the same registered object and emits a
  // fresh On callback; no new object discovery is required.
  REQUIRE_NOTHROW(subscriber->subscribeObjectClassAttributesWithRegions(
      objectClass, overlappingSubscription, true));
  drainCallbacks();
  REQUIRE(subscriberReports.discoveredObjectInstances.size() == 1U);
  REQUIRE(publisherReports.turnUpdatesOnReports.size() == 1U);
  REQUIRE(publisherReports.turnUpdatesOnReports.front().objectInstance ==
          objectInstance);
  REQUIRE(publisherReports.turnUpdatesOnReports.front().attributes ==
          attributeOnly);
  publisherReports.turnUpdatesOnReports.clear();

  REQUIRE_NOTHROW(subscriber->unsubscribeObjectClassAttributesWithRegions(
      objectClass, overlappingSubscription));
  drainCallbacks();
  REQUIRE(publisherReports.turnUpdatesOffReports.size() == 1U);
  REQUIRE(publisherReports.turnUpdatesOffReports.front().objectInstance ==
          objectInstance);
  REQUIRE(publisherReports.turnUpdatesOffReports.front().attributes ==
          attributeOnly);

  REQUIRE_NOTHROW(subscriber->unsubscribeObjectClassAttributesWithRegions(
      objectClass, disjointSubscription));
  REQUIRE_NOTHROW(publisher->unpublishObjectClassAttributes(
      objectClass, attributeOnly));
  REQUIRE_NOTHROW(subscriber->deleteRegion(subscriberRegion));
  REQUIRE_NOTHROW(subscriber->deleteRegion(disjointSubscriberRegion));
  REQUIRE_NOTHROW(publisher->deleteRegion(publisherRegion));
  REQUIRE_NOTHROW(subscriber->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(subscriber->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
}
