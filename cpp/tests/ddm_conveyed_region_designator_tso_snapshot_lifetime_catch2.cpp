#include <catch2/catch_test_macros.hpp>

#include "hla_test_names.hpp"

#include <RTI/NullFederateAmbassador.h>
#include <RTI/RTI1516.h>
#include <RTI/time/HLAinteger64Interval.h>
#include <RTI/time/HLAinteger64Time.h>

#include <atomic>
#include <exception>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>

#ifndef UMBRA_SOURCE_DIRECTORY
#error "The timestamped conveyed-region test requires the Umbra source directory."
#endif

namespace {

namespace fixture_hla = umbra::test::hla::wide;

using rti1516_2025::DimensionHandle;
using rti1516_2025::DimensionHandleSet;
using rti1516_2025::Exception;
using rti1516_2025::FederateHandle;
using rti1516_2025::HLA_EVOKED;
using rti1516_2025::HLAinteger64Interval;
using rti1516_2025::HLAinteger64Time;
using rti1516_2025::InteractionClassHandle;
using rti1516_2025::MessageRetractionHandle;
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

std::string narrow(std::wstring const& value);

class TimestampedSnapshotCheckingReceiver final : public NullFederateAmbassador {
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
  std::exception_ptr queryFailure;
  std::string queryFailureMessage;
  std::exception_ptr mutationFailure;

  void receiveInteraction(
      InteractionClassHandle const&,
      ParameterHandleValueMap const&,
      VariableLengthData const&,
      TransportationTypeHandle const&,
      FederateHandle const&,
      RegionHandleSet const* optionalSentRegions,
      rti1516_2025::LogicalTime const&,
      rti1516_2025::OrderType,
      rti1516_2025::OrderType,
      MessageRetractionHandle const*) override {
    ++callbacks;
    if (optionalSentRegions == nullptr || optionalSentRegions->size() != 1U) {
      return;
    }

    conveyedRegion = *optionalSentRegions->begin();
    try {
      conveyedDimensions = receiver->getDimensionHandleSet(*conveyedRegion);
      boundsBeforeMutation = receiver->getRangeBounds(
          *conveyedRegion, receiverDimension);
    } catch (...) {
      queryFailure = std::current_exception();
      try {
        std::rethrow_exception(queryFailure);
      } catch (Exception const& error) {
        queryFailureMessage = narrow(error.name()) + ": " + narrow(error.what());
      } catch (std::exception const& error) {
        queryFailureMessage = error.what();
      } catch (...) {
        queryFailureMessage = "non-standard exception during callback region query";
      }
    }
    try {
      publisher->setRangeBounds(
          sourceRegion, publisherDimension, RangeBounds(4UL, 6UL));
      publisher->commitRegionModifications(RegionHandleSet{sourceRegion});
    } catch (...) {
      mutationFailure = std::current_exception();
    }
    try {
      boundsAfterMutation = receiver->getRangeBounds(
          *conveyedRegion, receiverDimension);
    } catch (...) {
      queryFailure = std::current_exception();
      try {
        std::rethrow_exception(queryFailure);
      } catch (Exception const& error) {
        queryFailureMessage = narrow(error.name()) + ": " + narrow(error.what());
      } catch (std::exception const& error) {
        queryFailureMessage = error.what();
      } catch (...) {
        queryFailureMessage = "non-standard exception during callback region query";
      }
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
  return L"ddm-conveyed-region-tso-snapshot-" +
      std::to_wstring(sequence.fetch_add(1U, std::memory_order_relaxed));
}

std::string narrow(std::wstring const& value) {
  std::string result;
  result.reserve(value.size());
  for (wchar_t const character : value) {
    result.push_back(character >= 0 && character <= 0x7f
        ? static_cast<char>(character)
        : '?');
  }
  return result;
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
    "Embedded 2025 timestamped conveyed region designators remain callback snapshots",
    "[integration][development-profile][time-management][ddm]"
    "[ddm-conveyed-region-designator-tso-snapshot-lifetime]"
    "[rti.service.send-interaction-with-regions]"
    "[rti.service.set-convey-region-designator-sets-switch]"
    "[rti.service.get-range-bounds][rti.service.get-dimension-handle-set]"
    "[federate.callback.receive-interaction][callback-evoked]"
    "[rti.service.time-advance-request][2025]") {
  NullFederateAmbassador publisherCallbacks;
  TimestampedSnapshotCheckingReceiver receiverCallbacks;
  auto publisher = makeRti();
  auto receiver = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = regionalFom().wstring();

  REQUIRE_NOTHROW(publisher->connect(publisherCallbacks, HLA_EVOKED));
  REQUIRE_NOTHROW(receiver->connect(receiverCallbacks, HLA_EVOKED));
  REQUIRE_NOTHROW(publisher->createFederationExecution(
      federationName, fomModule, L"HLAinteger64Time"));
  REQUIRE_NOTHROW(publisher->joinFederationExecution(
      L"ddm-tso-snapshot-publisher", L"publisher", federationName));
  REQUIRE_NOTHROW(receiver->joinFederationExecution(
      L"ddm-tso-snapshot-receiver", L"receiver", federationName));

  auto const publisherInteraction = publisher->getInteractionClassHandle(
      fixture_hla::fom::two_dimensional_regional_interaction);
  auto const receiverInteraction = receiver->getInteractionClassHandle(
      fixture_hla::fom::two_dimensional_regional_interaction);
  auto const publisherX = publisher->getDimensionHandle(
      fixture_hla::fixture::umbra_region_x);
  auto const publisherY = publisher->getDimensionHandle(
      fixture_hla::fixture::umbra_region_y);
  auto const receiverX = receiver->getDimensionHandle(
      fixture_hla::fixture::umbra_region_x);
  auto const receiverY = receiver->getDimensionHandle(
      fixture_hla::fixture::umbra_region_y);
  REQUIRE(publisherInteraction.isValid());
  REQUIRE(receiverInteraction.isValid());

  REQUIRE_NOTHROW(publisher->publishInteractionClass(publisherInteraction));
  auto const sourceRegion = publisher->createRegion(
      DimensionHandleSet{publisherX, publisherY});
  auto const subscriptionRegion = receiver->createRegion(
      DimensionHandleSet{receiverX, receiverY});
  REQUIRE(sourceRegion.isValid());
  REQUIRE(subscriptionRegion.isValid());
  REQUIRE_NOTHROW(publisher->setRangeBounds(
      sourceRegion, publisherX, RangeBounds(1UL, 3UL)));
  REQUIRE_NOTHROW(publisher->setRangeBounds(
      sourceRegion, publisherY, RangeBounds(1UL, 3UL)));
  REQUIRE_NOTHROW(publisher->commitRegionModifications(
      RegionHandleSet{sourceRegion}));
  REQUIRE_NOTHROW(receiver->setRangeBounds(
      subscriptionRegion, receiverX, RangeBounds(1UL, 3UL)));
  REQUIRE_NOTHROW(receiver->setRangeBounds(
      subscriptionRegion, receiverY, RangeBounds(1UL, 3UL)));
  REQUIRE_NOTHROW(receiver->commitRegionModifications(
      RegionHandleSet{subscriptionRegion}));
  REQUIRE_NOTHROW(receiver->subscribeInteractionClassWithRegions(
      receiverInteraction, RegionHandleSet{subscriptionRegion}));
  REQUIRE_NOTHROW(receiver->setConveyRegionDesignatorSetsSwitch(true));

  receiverCallbacks.receiver = receiver.get();
  receiverCallbacks.publisher = publisher.get();
  receiverCallbacks.sourceRegion = sourceRegion;
  receiverCallbacks.publisherDimension = publisherX;
  receiverCallbacks.receiverDimension = receiverX;

  REQUIRE_NOTHROW(publisher->enableTimeRegulation(HLAinteger64Interval(0)));
  drainCallbacks(*publisher);
  REQUIRE_NOTHROW(receiver->enableTimeConstrained());
  drainCallbacks(*receiver);

  HLAinteger64Time const timestamp(5);
  REQUIRE_NOTHROW(receiver->timeAdvanceRequest(timestamp));
  drainCallbacks(*receiver);
  REQUIRE_NOTHROW(publisher->sendInteractionWithRegions(
      publisherInteraction,
      ParameterHandleValueMap{},
      RegionHandleSet{sourceRegion},
      VariableLengthData{},
      timestamp));
  REQUIRE_NOTHROW(publisher->timeAdvanceRequest(timestamp));
  drainCallbacks(*publisher);
  drainCallbacks(*receiver);

  REQUIRE(receiverCallbacks.callbacks == 1U);
  REQUIRE(receiverCallbacks.conveyedRegion.has_value());
  INFO(receiverCallbacks.queryFailureMessage);
  REQUIRE(receiverCallbacks.queryFailure == nullptr);
  REQUIRE(receiverCallbacks.mutationFailure == nullptr);
  REQUIRE(receiverCallbacks.conveyedDimensions ==
          DimensionHandleSet{receiverX, receiverY});
  REQUIRE(receiverCallbacks.boundsBeforeMutation.has_value());
  REQUIRE(receiverCallbacks.boundsAfterMutation.has_value());
  REQUIRE(receiverCallbacks.boundsBeforeMutation->getLowerBound() == 1UL);
  REQUIRE(receiverCallbacks.boundsBeforeMutation->getUpperBound() == 3UL);
  REQUIRE(receiverCallbacks.boundsAfterMutation->getLowerBound() == 1UL);
  REQUIRE(receiverCallbacks.boundsAfterMutation->getUpperBound() == 3UL);
  auto const publisherBounds = publisher->getRangeBounds(sourceRegion, publisherX);
  REQUIRE(publisherBounds.getLowerBound() == 4UL);
  REQUIRE(publisherBounds.getUpperBound() == 6UL);

  REQUIRE_NOTHROW(receiver->unsubscribeInteractionClassWithRegions(
      receiverInteraction, RegionHandleSet{subscriptionRegion}));
  REQUIRE_NOTHROW(publisher->deleteRegion(sourceRegion));
  REQUIRE_NOTHROW(receiver->deleteRegion(subscriptionRegion));
  REQUIRE_NOTHROW(receiver->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(receiver->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
}
