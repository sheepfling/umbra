#include <catch2/catch_test_macros.hpp>

#include "hla_test_names.hpp"

#include <RTI/NullFederateAmbassador.h>
#include <RTI/RTI1516.h>

#include <atomic>
#include <exception>
#include <filesystem>
#include <memory>
#include <optional>
#include <vector>

#ifndef UMBRA_SOURCE_DIRECTORY
#error "The reflected conveyed-region-designator test requires the Umbra source directory."
#endif

namespace {

namespace fixture_hla = umbra::test::hla::wide;

using rti1516_2025::AttributeHandleSet;
using rti1516_2025::AttributeHandleSetRegionHandleSetPairVector;
using rti1516_2025::AttributeHandleValueMap;
using rti1516_2025::DimensionHandle;
using rti1516_2025::DimensionHandleSet;
using rti1516_2025::FederateHandle;
using rti1516_2025::HLA_EVOKED;
using rti1516_2025::NO_ACTION;
using rti1516_2025::NullFederateAmbassador;
using rti1516_2025::ObjectClassHandle;
using rti1516_2025::ObjectInstanceHandle;
using rti1516_2025::RangeBounds;
using rti1516_2025::RegionHandle;
using rti1516_2025::RegionHandleSet;
using rti1516_2025::RTIambassador;
using rti1516_2025::RTIambassadorFactory;
using rti1516_2025::TransportationTypeHandle;
using rti1516_2025::VariableLengthData;

class ReflectionSnapshotReceiver final : public NullFederateAmbassador {
 public:
  RTIambassador* receiver = nullptr;
  RTIambassador* publisher = nullptr;
  RegionHandle sourceRegion;
  DimensionHandle sourceDimension;
  DimensionHandle receiverDimension;
  std::size_t callbacks = 0U;
  std::optional<RegionHandle> conveyedRegion;
  DimensionHandleSet conveyedDimensions;
  std::optional<RangeBounds> boundsBeforeMutation;
  std::optional<RangeBounds> boundsAfterMutation;
  std::exception_ptr dimensionQueryFailure;
  std::exception_ptr beforeBoundsQueryFailure;
  std::exception_ptr sourceMutationFailure;
  std::exception_ptr afterBoundsQueryFailure;

  void reflectAttributeValues(
      ObjectInstanceHandle const&,
      AttributeHandleValueMap const&,
      VariableLengthData const&,
      TransportationTypeHandle const&,
      FederateHandle const&,
      RegionHandleSet const* optionalSentRegions) override {
    ++callbacks;
    if (optionalSentRegions == nullptr || optionalSentRegions->size() != 1U) {
      return;
    }

    conveyedRegion = *optionalSentRegions->begin();
    try {
      conveyedDimensions = receiver->getDimensionHandleSet(*conveyedRegion);
    } catch (...) {
      dimensionQueryFailure = std::current_exception();
    }
    try {
      boundsBeforeMutation = receiver->getRangeBounds(
          *conveyedRegion,
          receiverDimension);
    } catch (...) {
      beforeBoundsQueryFailure = std::current_exception();
    }

    // §9.1.8 requires the conveyed realization copy to remain stable through
    // Reflect Attribute Values even if the publisher changes its source region.
    try {
      publisher->setRangeBounds(
          sourceRegion,
          sourceDimension,
          RangeBounds(0UL, 2UL));
      publisher->commitRegionModifications(RegionHandleSet{sourceRegion});
    } catch (...) {
      sourceMutationFailure = std::current_exception();
    }
    try {
      boundsAfterMutation = receiver->getRangeBounds(
          *conveyedRegion,
          receiverDimension);
    } catch (...) {
      afterBoundsQueryFailure = std::current_exception();
    }
  }
};

std::unique_ptr<RTIambassador> makeRti() {
  RTIambassadorFactory factory;
  return factory.createRTIambassador();
}

std::filesystem::path restaurantFom() {
  return std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "third_party" /
      "ieee1516.2-2025" / "resources" / "examples" /
      "RestaurantFOMmodule-2025.xml";
}

std::wstring nextFederationName() {
  static std::atomic_uint64_t sequence{0U};
  return L"ddm-reflect-region-snapshot-" +
      std::to_wstring(sequence.fetch_add(1U, std::memory_order_relaxed));
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
    "Embedded 2025 conveyed region designators remain snapshots during Reflect Attribute Values",
    "[integration][development-profile][object-management][ddm]"
    "[ddm-conveyed-region-designator-snapshot-lifetime]"
    "[ddm-conveyed-region-designator-reflect-snapshot]"
    "[rti.service.register-object-instance-with-regions]"
    "[rti.service.subscribe-object-class-attributes-with-regions]"
    "[rti.service.update-attribute-values]"
    "[rti.service.set-range-bounds][rti.service.commit-region-modifications]"
    "[rti.service.get-range-bounds][rti.service.get-dimension-handle-set]"
    "[rti.service.set-convey-region-designator-sets-switch]"
    "[federate.callback.reflect-attribute-values][callback-evoked][2025]") {
  NullFederateAmbassador publisherCallbacks;
  ReflectionSnapshotReceiver subscriberCallbacks;
  auto publisher = makeRti();
  auto subscriber = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = restaurantFom().wstring();

  REQUIRE_NOTHROW(publisher->connect(publisherCallbacks, HLA_EVOKED));
  REQUIRE_NOTHROW(subscriber->connect(subscriberCallbacks, HLA_EVOKED));
  REQUIRE_NOTHROW(publisher->createFederationExecution(
      federationName,
      fomModule,
      L"HLAinteger64Time"));
  REQUIRE_NOTHROW(publisher->joinFederationExecution(
      L"ddm-reflect-snapshot-publisher", L"publisher", federationName));
  REQUIRE_NOTHROW(subscriber->joinFederationExecution(
      L"ddm-reflect-snapshot-subscriber", L"subscriber", federationName));

  auto const publisherSoda = publisher->getObjectClassHandle(
      L"HLAobjectRoot.Food.Drink.Soda");
  auto const subscriberSoda = subscriber->getObjectClassHandle(
      L"HLAobjectRoot.Food.Drink.Soda");
  auto const publisherFlavor = publisher->getAttributeHandle(
      publisherSoda, L"Flavor");
  auto const subscriberFlavor = subscriber->getAttributeHandle(
      subscriberSoda, L"Flavor");
  auto const publisherDimension = publisher->getDimensionHandle(L"SodaFlavor");
  auto const subscriberDimension = subscriber->getDimensionHandle(L"SodaFlavor");
  REQUIRE(publisherSoda.isValid());
  REQUIRE(subscriberSoda.isValid());
  REQUIRE(publisherFlavor.isValid());
  REQUIRE(subscriberFlavor.isValid());
  REQUIRE(publisherDimension.isValid());
  REQUIRE(subscriberDimension.isValid());

  REQUIRE_NOTHROW(publisher->publishObjectClassAttributes(
      publisherSoda,
      AttributeHandleSet{publisherFlavor}));
  auto const sourceRegion = publisher->createRegion(
      DimensionHandleSet{publisherDimension});
  auto const subscriptionRegion = subscriber->createRegion(
      DimensionHandleSet{subscriberDimension});
  REQUIRE(sourceRegion.isValid());
  REQUIRE(subscriptionRegion.isValid());
  REQUIRE_NOTHROW(publisher->setRangeBounds(
      sourceRegion, publisherDimension, RangeBounds(1UL, 3UL)));
  REQUIRE_NOTHROW(publisher->commitRegionModifications(
      RegionHandleSet{sourceRegion}));
  REQUIRE_NOTHROW(subscriber->setRangeBounds(
      subscriptionRegion, subscriberDimension, RangeBounds(1UL, 3UL)));
  REQUIRE_NOTHROW(subscriber->commitRegionModifications(
      RegionHandleSet{subscriptionRegion}));

  AttributeHandleSetRegionHandleSetPairVector const sourcePair{{
      AttributeHandleSet{publisherFlavor},
      RegionHandleSet{sourceRegion},
  }};
  AttributeHandleSetRegionHandleSetPairVector const subscriptionPair{{
      AttributeHandleSet{subscriberFlavor},
      RegionHandleSet{subscriptionRegion},
  }};
  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = publisher->registerObjectInstanceWithRegions(
      publisherSoda,
      sourcePair));
  REQUIRE(objectInstance.isValid());
  REQUIRE_NOTHROW(subscriber->subscribeObjectClassAttributesWithRegions(
      subscriberSoda,
      subscriptionPair));
  REQUIRE_NOTHROW(subscriber->setConveyRegionDesignatorSetsSwitch(true));
  drainCallbacks(*subscriber);

  subscriberCallbacks.receiver = subscriber.get();
  subscriberCallbacks.publisher = publisher.get();
  subscriberCallbacks.sourceRegion = sourceRegion;
  subscriberCallbacks.sourceDimension = publisherDimension;
  subscriberCallbacks.receiverDimension = subscriberDimension;

  std::vector<unsigned char> const valueBytes{0x31U, 0x42U};
  AttributeHandleValueMap values;
  values.emplace(
      publisherFlavor,
      VariableLengthData(valueBytes.data(), valueBytes.size()));
  REQUIRE_NOTHROW(publisher->updateAttributeValues(
      objectInstance,
      values,
      VariableLengthData()));
  drainCallbacks(*subscriber);

  REQUIRE(subscriberCallbacks.callbacks == 1U);
  REQUIRE(subscriberCallbacks.conveyedRegion.has_value());
  REQUIRE(subscriberCallbacks.dimensionQueryFailure == nullptr);
  REQUIRE(subscriberCallbacks.beforeBoundsQueryFailure == nullptr);
  REQUIRE(subscriberCallbacks.sourceMutationFailure == nullptr);
  REQUIRE(subscriberCallbacks.afterBoundsQueryFailure == nullptr);
  REQUIRE(subscriberCallbacks.conveyedDimensions ==
          DimensionHandleSet{subscriberDimension});
  REQUIRE(subscriberCallbacks.boundsBeforeMutation.has_value());
  REQUIRE(subscriberCallbacks.boundsAfterMutation.has_value());
  REQUIRE(subscriberCallbacks.boundsBeforeMutation->getLowerBound() == 1UL);
  REQUIRE(subscriberCallbacks.boundsBeforeMutation->getUpperBound() == 3UL);
  REQUIRE(subscriberCallbacks.boundsAfterMutation->getLowerBound() == 1UL);
  REQUIRE(subscriberCallbacks.boundsAfterMutation->getUpperBound() == 3UL);
  auto const senderBoundsAfterCallback = publisher->getRangeBounds(
      sourceRegion,
      publisherDimension);
  REQUIRE(senderBoundsAfterCallback.getLowerBound() == 0UL);
  REQUIRE(senderBoundsAfterCallback.getUpperBound() == 2UL);

  REQUIRE_NOTHROW(subscriber->unsubscribeObjectClassAttributesWithRegions(
      subscriberSoda,
      subscriptionPair));
  REQUIRE_NOTHROW(publisher->unassociateRegionsForUpdates(
      objectInstance,
      sourcePair));
  REQUIRE_NOTHROW(subscriber->deleteRegion(subscriptionRegion));
  REQUIRE_NOTHROW(publisher->deleteRegion(sourceRegion));
  REQUIRE_NOTHROW(subscriber->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(
      rti1516_2025::CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(subscriber->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
}
