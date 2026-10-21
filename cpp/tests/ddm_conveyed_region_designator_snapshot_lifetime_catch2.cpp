#include <catch2/catch_test_macros.hpp>

#include "hla_test_names.hpp"

#include <RTI/NullFederateAmbassador.h>
#include <RTI/RTI1516.h>

#include <atomic>
#include <exception>
#include <filesystem>
#include <memory>
#include <optional>

#ifndef UMBRA_SOURCE_DIRECTORY
#error "The conveyed-region-designator test requires the Umbra source directory."
#endif

namespace {

namespace fixture_hla = umbra::test::hla::wide;

using rti1516_2025::DimensionHandle;
using rti1516_2025::DimensionHandleSet;
using rti1516_2025::FederateHandle;
using rti1516_2025::HLA_EVOKED;
using rti1516_2025::InteractionClassHandle;
using rti1516_2025::NO_ACTION;
using rti1516_2025::NullFederateAmbassador;
using rti1516_2025::ParameterHandleValueMap;
using rti1516_2025::RangeBounds;
using rti1516_2025::RegionHandle;
using rti1516_2025::RegionHandleSet;
using rti1516_2025::RTIambassador;
using rti1516_2025::RTIambassadorFactory;
using rti1516_2025::TransportationTypeHandle;
using rti1516_2025::VariableLengthData;

class SnapshotCheckingReceiver final : public NullFederateAmbassador {
 public:
  RTIambassador* receiver = nullptr;
  RTIambassador* publisher = nullptr;
  RegionHandle sourceRegion;
  DimensionHandle publisherDimension;
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

  void receiveInteraction(
      InteractionClassHandle const&,
      ParameterHandleValueMap const&,
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
    // this callback even when its source realization changes in the meantime.
    try {
      publisher->setRangeBounds(
          sourceRegion,
          publisherDimension,
          RangeBounds(4UL, 6UL));
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

std::filesystem::path regionalFom() {
  return std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" /
      "data" / "two-dimensional-regional-interaction-fom.xml";
}

std::wstring nextFederationName() {
  static std::atomic_uint64_t sequence{0U};
  return L"ddm-conveyed-region-snapshot-" +
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
    "Embedded 2025 conveyed region designators remain snapshots during Receive Interaction",
    "[integration][development-profile][interaction-management][ddm]"
    "[ddm-conveyed-region-designator-snapshot-lifetime]"
    "[rti.service.send-interaction-with-regions]"
    "[rti.service.set-convey-region-designator-sets-switch]"
    "[rti.service.get-range-bounds][rti.service.get-dimension-handle-set]"
    "[federate.callback.receive-interaction][callback-evoked][2025]") {
  NullFederateAmbassador publisherCallbacks;
  SnapshotCheckingReceiver subscriberCallbacks;
  auto publisher = makeRti();
  auto subscriber = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = regionalFom().wstring();

  REQUIRE_NOTHROW(publisher->connect(publisherCallbacks, HLA_EVOKED));
  REQUIRE_NOTHROW(subscriber->connect(subscriberCallbacks, HLA_EVOKED));
  REQUIRE_NOTHROW(publisher->createFederationExecution(
      federationName,
      fomModule,
      L"HLAinteger64Time"));
  REQUIRE_NOTHROW(publisher->joinFederationExecution(
      L"ddm-snapshot-publisher", L"publisher", federationName));
  REQUIRE_NOTHROW(subscriber->joinFederationExecution(
      L"ddm-snapshot-subscriber", L"subscriber", federationName));

  auto const publisherInteraction = publisher->getInteractionClassHandle(
      fixture_hla::fom::two_dimensional_regional_interaction);
  auto const subscriberInteraction = subscriber->getInteractionClassHandle(
      fixture_hla::fom::two_dimensional_regional_interaction);
  auto const publisherX = publisher->getDimensionHandle(
      fixture_hla::fixture::umbra_region_x);
  auto const publisherY = publisher->getDimensionHandle(
      fixture_hla::fixture::umbra_region_y);
  auto const subscriberX = subscriber->getDimensionHandle(
      fixture_hla::fixture::umbra_region_x);
  auto const subscriberY = subscriber->getDimensionHandle(
      fixture_hla::fixture::umbra_region_y);
  REQUIRE(publisherInteraction.isValid());
  REQUIRE(subscriberInteraction.isValid());
  REQUIRE(publisherX.isValid());
  REQUIRE(publisherY.isValid());
  REQUIRE(subscriberX.isValid());
  REQUIRE(subscriberY.isValid());

  REQUIRE_NOTHROW(publisher->publishInteractionClass(publisherInteraction));
  auto const sourceRegion = publisher->createRegion(
      DimensionHandleSet{publisherX, publisherY});
  auto const subscriptionRegion = subscriber->createRegion(
      DimensionHandleSet{subscriberX, subscriberY});
  REQUIRE(sourceRegion.isValid());
  REQUIRE(subscriptionRegion.isValid());
  REQUIRE_NOTHROW(publisher->setRangeBounds(
      sourceRegion, publisherX, RangeBounds(1UL, 3UL)));
  REQUIRE_NOTHROW(publisher->setRangeBounds(
      sourceRegion, publisherY, RangeBounds(1UL, 3UL)));
  REQUIRE_NOTHROW(publisher->commitRegionModifications(RegionHandleSet{sourceRegion}));
  REQUIRE_NOTHROW(subscriber->setRangeBounds(
      subscriptionRegion, subscriberX, RangeBounds(1UL, 3UL)));
  REQUIRE_NOTHROW(subscriber->setRangeBounds(
      subscriptionRegion, subscriberY, RangeBounds(1UL, 3UL)));
  REQUIRE_NOTHROW(subscriber->commitRegionModifications(
      RegionHandleSet{subscriptionRegion}));
  REQUIRE_NOTHROW(subscriber->subscribeInteractionClassWithRegions(
      subscriberInteraction,
      RegionHandleSet{subscriptionRegion}));
  REQUIRE_NOTHROW(subscriber->setConveyRegionDesignatorSetsSwitch(true));

  subscriberCallbacks.receiver = subscriber.get();
  subscriberCallbacks.publisher = publisher.get();
  subscriberCallbacks.sourceRegion = sourceRegion;
  subscriberCallbacks.publisherDimension = publisherX;
  subscriberCallbacks.receiverDimension = subscriberX;
  VariableLengthData const tag;
  REQUIRE_NOTHROW(publisher->sendInteractionWithRegions(
      publisherInteraction,
      ParameterHandleValueMap{},
      RegionHandleSet{sourceRegion},
      tag));
  drainCallbacks(*subscriber);

  REQUIRE(subscriberCallbacks.callbacks == 1U);
  REQUIRE(subscriberCallbacks.conveyedRegion.has_value());
  REQUIRE(subscriberCallbacks.dimensionQueryFailure == nullptr);
  REQUIRE(subscriberCallbacks.beforeBoundsQueryFailure == nullptr);
  REQUIRE(subscriberCallbacks.sourceMutationFailure == nullptr);
  REQUIRE(subscriberCallbacks.afterBoundsQueryFailure == nullptr);
  REQUIRE(subscriberCallbacks.conveyedDimensions == DimensionHandleSet{subscriberX, subscriberY});
  REQUIRE(subscriberCallbacks.boundsBeforeMutation.has_value());
  REQUIRE(subscriberCallbacks.boundsAfterMutation.has_value());
  REQUIRE(subscriberCallbacks.boundsBeforeMutation->getLowerBound() == 1UL);
  REQUIRE(subscriberCallbacks.boundsBeforeMutation->getUpperBound() == 3UL);
  REQUIRE(subscriberCallbacks.boundsAfterMutation->getLowerBound() == 1UL);
  REQUIRE(subscriberCallbacks.boundsAfterMutation->getUpperBound() == 3UL);
  auto const senderBoundsAfterCallback = publisher->getRangeBounds(sourceRegion, publisherX);
  REQUIRE(senderBoundsAfterCallback.getLowerBound() == 4UL);
  REQUIRE(senderBoundsAfterCallback.getUpperBound() == 6UL);

  REQUIRE_NOTHROW(subscriber->unsubscribeInteractionClassWithRegions(
      subscriberInteraction,
      RegionHandleSet{subscriptionRegion}));
  REQUIRE_NOTHROW(publisher->deleteRegion(sourceRegion));
  REQUIRE_NOTHROW(subscriber->deleteRegion(subscriptionRegion));
  REQUIRE_NOTHROW(subscriber->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(subscriber->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
}
