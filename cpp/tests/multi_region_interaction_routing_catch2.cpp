#include <catch2/catch_test_macros.hpp>

#include "hla_test_names.hpp"

#include <RTI/NullFederateAmbassador.h>
#include <RTI/RTI1516.h>

#include <algorithm>
#include <atomic>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#ifndef UMBRA_SOURCE_DIRECTORY
#error "The multi-region interaction-routing test requires the Umbra source directory."
#endif

namespace {

namespace fixture_hla = umbra::test::hla::wide;

using rti1516_2025::DimensionHandle;
using rti1516_2025::DimensionHandleSet;
using rti1516_2025::FederateHandle;
using rti1516_2025::HLA_EVOKED;
using rti1516_2025::InteractionClassHandle;
using rti1516_2025::NullFederateAmbassador;
using rti1516_2025::ParameterHandle;
using rti1516_2025::ParameterHandleValueMap;
using rti1516_2025::RangeBounds;
using rti1516_2025::RegionHandle;
using rti1516_2025::RegionHandleSet;
using rti1516_2025::RTIambassador;
using rti1516_2025::RTIambassadorFactory;
using rti1516_2025::TransportationTypeHandle;
using rti1516_2025::VariableLengthData;

class InteractionReceiver final : public NullFederateAmbassador {
 public:
  struct Received final {
    InteractionClassHandle interactionClass;
    ParameterHandleValueMap parameterValues;
    VariableLengthData userSuppliedTag;
    bool sentRegionsSupplied = false;
    RegionHandleSet sentRegions;
  };

  void receiveInteraction(
      InteractionClassHandle const& interactionClass,
      ParameterHandleValueMap const& parameterValues,
      VariableLengthData const& userSuppliedTag,
      TransportationTypeHandle const&,
      FederateHandle const&,
      RegionHandleSet const* optionalSentRegions) override {
    Received report{interactionClass, parameterValues, userSuppliedTag};
    if (optionalSentRegions != nullptr) {
      report.sentRegionsSupplied = true;
      report.sentRegions = *optionalSentRegions;
    }
    received.push_back(std::move(report));
  }

  std::vector<Received> received;
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
  return L"restaurant-multi-region-interaction-routing-" +
      std::to_wstring(sequence.fetch_add(1U, std::memory_order_relaxed));
}

void drain(RTIambassador& ambassador) {
  for (int pass = 0; pass != 128; ++pass) {
    if (!ambassador.evokeCallback(0.0)) {
      break;
    }
  }
}

bool hasTag(VariableLengthData const& value, std::string const& expected) {
  if (value.size() != expected.size()) {
    return false;
  }
  auto const* bytes = static_cast<unsigned char const*>(value.data());
  return bytes != nullptr && std::equal(
      expected.begin(), expected.end(), bytes,
      [](char expectedByte, unsigned char actualByte) {
        return static_cast<unsigned char>(expectedByte) == actualByte;
      });
}

}  // namespace

TEST_CASE(
    "Embedded multi-region interactions route each overlapping source realization once",
    "[integration][development-profile][federation-management][interaction-management]"
    "[ddm][regional-interaction][multi-region][positive-dimensional-overlap]"
    "[rti.service.create-region][rti.service.set-range-bounds]"
    "[rti.service.commit-region-modifications][rti.service.publish-interaction-class]"
    "[rti.service.subscribe-interaction-class-with-regions]"
    "[rti.service.send-interaction-with-regions]"
    "[rti.service.set-convey-region-designator-sets-switch]"
    "[federate.callback.receive-interaction][callback-evoked][2025]") {
  NullFederateAmbassador publisherCallbacks;
  InteractionReceiver xCallbacks;
  InteractionReceiver yCallbacks;
  InteractionReceiver combinedCallbacks;
  InteractionReceiver disjointCallbacks;
  auto publisher = makeRti();
  auto xSubscriber = makeRti();
  auto ySubscriber = makeRti();
  auto combinedSubscriber = makeRti();
  auto disjointSubscriber = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = restaurantFom().wstring();

  REQUIRE_NOTHROW(publisher->connect(publisherCallbacks, HLA_EVOKED));
  REQUIRE_NOTHROW(xSubscriber->connect(xCallbacks, HLA_EVOKED));
  REQUIRE_NOTHROW(ySubscriber->connect(yCallbacks, HLA_EVOKED));
  REQUIRE_NOTHROW(combinedSubscriber->connect(combinedCallbacks, HLA_EVOKED));
  REQUIRE_NOTHROW(disjointSubscriber->connect(disjointCallbacks, HLA_EVOKED));
  REQUIRE_NOTHROW(publisher->createFederationExecution(
      federationName, fomModule, L"HLAinteger64Time"));
  REQUIRE_NOTHROW(publisher->joinFederationExecution(
      L"multi-region-interaction-publisher", L"publisher", federationName));
  REQUIRE_NOTHROW(xSubscriber->joinFederationExecution(
      L"multi-region-interaction-x", L"subscriber", federationName));
  REQUIRE_NOTHROW(ySubscriber->joinFederationExecution(
      L"multi-region-interaction-y", L"subscriber", federationName));
  REQUIRE_NOTHROW(combinedSubscriber->joinFederationExecution(
      L"multi-region-interaction-combined", L"subscriber", federationName));
  REQUIRE_NOTHROW(disjointSubscriber->joinFederationExecution(
      L"multi-region-interaction-disjoint", L"subscriber", federationName));

  auto const publisherInteraction = publisher->getInteractionClassHandle(
      fixture_hla::fom::main_course_served);
  auto const xInteraction = xSubscriber->getInteractionClassHandle(
      fixture_hla::fom::main_course_served);
  auto const yInteraction = ySubscriber->getInteractionClassHandle(
      fixture_hla::fom::main_course_served);
  auto const combinedInteraction = combinedSubscriber->getInteractionClassHandle(
      fixture_hla::fom::main_course_served);
  auto const disjointInteraction = disjointSubscriber->getInteractionClassHandle(
      fixture_hla::fom::main_course_served);
  auto const temperatureOk = publisher->getParameterHandle(
      publisherInteraction, fixture_hla::fixture::temperature_ok);
  REQUIRE(publisherInteraction.isValid());
  REQUIRE(xInteraction.isValid());
  REQUIRE(yInteraction.isValid());
  REQUIRE(combinedInteraction.isValid());
  REQUIRE(disjointInteraction.isValid());
  REQUIRE(temperatureOk.isValid());
  REQUIRE_NOTHROW(publisher->publishInteractionClass(publisherInteraction));

  auto const publisherX = publisher->getDimensionHandle(fixture_hla::fixture::server_id);
  auto const xDimension = xSubscriber->getDimensionHandle(fixture_hla::fixture::server_id);
  auto const yDimension = ySubscriber->getDimensionHandle(fixture_hla::fixture::server_id);
  auto const combinedX = combinedSubscriber->getDimensionHandle(fixture_hla::fixture::server_id);
  auto const disjointX = disjointSubscriber->getDimensionHandle(fixture_hla::fixture::server_id);
  REQUIRE(publisherX.isValid());
  REQUIRE(xDimension.isValid());
  REQUIRE(yDimension.isValid());
  REQUIRE(combinedX.isValid());
  REQUIRE(disjointX.isValid());

  DimensionHandleSet const publisherDimensions{publisherX};
  auto const sourceX = publisher->createRegion(publisherDimensions);
  auto const sourceY = publisher->createRegion(publisherDimensions);
  auto const xRegion = xSubscriber->createRegion(DimensionHandleSet{xDimension});
  auto const yRegion = ySubscriber->createRegion(DimensionHandleSet{yDimension});
  auto const combinedXRegion = combinedSubscriber->createRegion(DimensionHandleSet{combinedX});
  auto const combinedYRegion = combinedSubscriber->createRegion(DimensionHandleSet{combinedX});
  auto const disjointRegion = disjointSubscriber->createRegion(DimensionHandleSet{disjointX});
  REQUIRE(sourceX.isValid());
  REQUIRE(sourceY.isValid());
  REQUIRE(xRegion.isValid());
  REQUIRE(yRegion.isValid());
  REQUIRE(combinedXRegion.isValid());
  REQUIRE(combinedYRegion.isValid());
  REQUIRE(disjointRegion.isValid());

  REQUIRE_NOTHROW(publisher->setRangeBounds(sourceX, publisherX, RangeBounds(0UL, 2UL)));
  REQUIRE_NOTHROW(publisher->setRangeBounds(sourceY, publisherX, RangeBounds(5UL, 7UL)));
  REQUIRE_NOTHROW(xSubscriber->setRangeBounds(xRegion, xDimension, RangeBounds(0UL, 2UL)));
  REQUIRE_NOTHROW(ySubscriber->setRangeBounds(yRegion, yDimension, RangeBounds(5UL, 7UL)));
  REQUIRE_NOTHROW(combinedSubscriber->setRangeBounds(combinedXRegion, combinedX, RangeBounds(0UL, 2UL)));
  REQUIRE_NOTHROW(combinedSubscriber->setRangeBounds(combinedYRegion, combinedX, RangeBounds(5UL, 7UL)));
  REQUIRE_NOTHROW(disjointSubscriber->setRangeBounds(disjointRegion, disjointX, RangeBounds(8UL, 9UL)));
  REQUIRE_NOTHROW(publisher->commitRegionModifications(
      RegionHandleSet{sourceX, sourceY}));
  REQUIRE_NOTHROW(xSubscriber->commitRegionModifications(RegionHandleSet{xRegion}));
  REQUIRE_NOTHROW(ySubscriber->commitRegionModifications(RegionHandleSet{yRegion}));
  REQUIRE_NOTHROW(combinedSubscriber->commitRegionModifications(
      RegionHandleSet{combinedXRegion, combinedYRegion}));
  REQUIRE_NOTHROW(disjointSubscriber->commitRegionModifications(
      RegionHandleSet{disjointRegion}));
  REQUIRE_NOTHROW(xSubscriber->subscribeInteractionClassWithRegions(
      xInteraction, RegionHandleSet{xRegion}));
  REQUIRE_NOTHROW(ySubscriber->subscribeInteractionClassWithRegions(
      yInteraction, RegionHandleSet{yRegion}));
  REQUIRE_NOTHROW(combinedSubscriber->subscribeInteractionClassWithRegions(
      combinedInteraction, RegionHandleSet{combinedXRegion, combinedYRegion}));
  REQUIRE_NOTHROW(disjointSubscriber->subscribeInteractionClassWithRegions(
      disjointInteraction, RegionHandleSet{disjointRegion}));
  REQUIRE_NOTHROW(xSubscriber->setConveyRegionDesignatorSetsSwitch(true));
  REQUIRE_NOTHROW(ySubscriber->setConveyRegionDesignatorSetsSwitch(true));
  REQUIRE_NOTHROW(combinedSubscriber->setConveyRegionDesignatorSetsSwitch(true));
  REQUIRE_NOTHROW(disjointSubscriber->setConveyRegionDesignatorSetsSwitch(true));

  unsigned char const temperatureBytes[] = {0x01U};
  ParameterHandleValueMap parameters;
  parameters.emplace(
      temperatureOk,
      VariableLengthData(temperatureBytes, sizeof(temperatureBytes)));

  auto send = [&](RegionHandleSet const& regions, std::string const& tag) {
    REQUIRE_NOTHROW(publisher->sendInteractionWithRegions(
        publisherInteraction,
        parameters,
        regions,
        VariableLengthData(tag.data(), tag.size())));
    drain(*xSubscriber);
    drain(*ySubscriber);
    drain(*combinedSubscriber);
    drain(*disjointSubscriber);
  };

  send(RegionHandleSet{sourceX, sourceY}, "both");
  REQUIRE(xCallbacks.received.size() == 1U);
  REQUIRE(yCallbacks.received.size() == 1U);
  REQUIRE(combinedCallbacks.received.size() == 1U);
  REQUIRE(disjointCallbacks.received.empty());
  REQUIRE(xCallbacks.received.back().sentRegionsSupplied);
  REQUIRE(yCallbacks.received.back().sentRegionsSupplied);
  REQUIRE(combinedCallbacks.received.back().sentRegionsSupplied);
  REQUIRE(xCallbacks.received.back().parameterValues.size() == 1U);
  REQUIRE(yCallbacks.received.back().parameterValues.size() == 1U);
  REQUIRE(combinedCallbacks.received.back().parameterValues.size() == 1U);
  REQUIRE(xCallbacks.received.back().sentRegions == RegionHandleSet{sourceX, sourceY});
  REQUIRE(yCallbacks.received.back().sentRegions == RegionHandleSet{sourceX, sourceY});
  REQUIRE(combinedCallbacks.received.back().sentRegions == RegionHandleSet{sourceX, sourceY});
  REQUIRE(hasTag(xCallbacks.received.back().userSuppliedTag, "both"));
  REQUIRE(hasTag(yCallbacks.received.back().userSuppliedTag, "both"));
  REQUIRE(hasTag(combinedCallbacks.received.back().userSuppliedTag, "both"));

  send(RegionHandleSet{sourceY}, "y-only");
  REQUIRE(xCallbacks.received.size() == 1U);
  REQUIRE(yCallbacks.received.size() == 2U);
  REQUIRE(combinedCallbacks.received.size() == 2U);
  REQUIRE(disjointCallbacks.received.empty());
  REQUIRE(yCallbacks.received.back().sentRegions == RegionHandleSet{sourceY});
  REQUIRE(combinedCallbacks.received.back().sentRegions == RegionHandleSet{sourceY});
  REQUIRE(hasTag(yCallbacks.received.back().userSuppliedTag, "y-only"));
  REQUIRE(hasTag(combinedCallbacks.received.back().userSuppliedTag, "y-only"));

  send(RegionHandleSet{sourceX}, "x-only");
  REQUIRE(xCallbacks.received.size() == 2U);
  REQUIRE(yCallbacks.received.size() == 2U);
  REQUIRE(combinedCallbacks.received.size() == 3U);
  REQUIRE(disjointCallbacks.received.empty());
  REQUIRE(xCallbacks.received.back().sentRegions == RegionHandleSet{sourceX});
  REQUIRE(combinedCallbacks.received.back().sentRegions == RegionHandleSet{sourceX});
  REQUIRE(hasTag(xCallbacks.received.back().userSuppliedTag, "x-only"));
  REQUIRE(hasTag(combinedCallbacks.received.back().userSuppliedTag, "x-only"));

  REQUIRE_NOTHROW(xSubscriber->unsubscribeInteractionClassWithRegions(
      xInteraction, RegionHandleSet{xRegion}));
  REQUIRE_NOTHROW(ySubscriber->unsubscribeInteractionClassWithRegions(
      yInteraction, RegionHandleSet{yRegion}));
  REQUIRE_NOTHROW(combinedSubscriber->unsubscribeInteractionClassWithRegions(
      combinedInteraction, RegionHandleSet{combinedXRegion, combinedYRegion}));
  REQUIRE_NOTHROW(disjointSubscriber->unsubscribeInteractionClassWithRegions(
      disjointInteraction, RegionHandleSet{disjointRegion}));
  REQUIRE_NOTHROW(xSubscriber->deleteRegion(xRegion));
  REQUIRE_NOTHROW(ySubscriber->deleteRegion(yRegion));
  REQUIRE_NOTHROW(combinedSubscriber->deleteRegion(combinedXRegion));
  REQUIRE_NOTHROW(combinedSubscriber->deleteRegion(combinedYRegion));
  REQUIRE_NOTHROW(disjointSubscriber->deleteRegion(disjointRegion));
  REQUIRE_NOTHROW(publisher->deleteRegion(sourceX));
  REQUIRE_NOTHROW(publisher->deleteRegion(sourceY));
  REQUIRE_NOTHROW(disjointSubscriber->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(combinedSubscriber->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(ySubscriber->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(xSubscriber->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(
      rti1516_2025::CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(disjointSubscriber->disconnect());
  REQUIRE_NOTHROW(combinedSubscriber->disconnect());
  REQUIRE_NOTHROW(ySubscriber->disconnect());
  REQUIRE_NOTHROW(xSubscriber->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
}

TEST_CASE(
    "Embedded regional interaction subscription empty sets are no-ops",
    "[integration][development-profile][federation-management][interaction-management]"
    "[ddm][regional-interaction][subscription-empty-set][2025]"
    "[rti.service.create-region][rti.service.set-range-bounds]"
    "[rti.service.commit-region-modifications]"
    "[rti.service.publish-interaction-class]"
    "[rti.service.subscribe-interaction-class-with-regions]"
    "[rti.service.unsubscribe-interaction-class-with-regions]"
    "[rti.service.send-interaction-with-regions]"
    "[federate.callback.receive-interaction][callback-evoked]") {
  NullFederateAmbassador publisherCallbacks;
  InteractionReceiver subscriberCallbacks;
  auto publisher = makeRti();
  auto subscriber = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = restaurantFom().wstring();

  REQUIRE_NOTHROW(publisher->connect(publisherCallbacks, HLA_EVOKED));
  REQUIRE_NOTHROW(subscriber->connect(subscriberCallbacks, HLA_EVOKED));
  REQUIRE_NOTHROW(publisher->createFederationExecution(
      federationName, fomModule, L"HLAinteger64Time"));
  REQUIRE_NOTHROW(publisher->joinFederationExecution(
      L"regional-subscription-empty-publisher", L"publisher", federationName));
  REQUIRE_NOTHROW(subscriber->joinFederationExecution(
      L"regional-subscription-empty-subscriber", L"subscriber", federationName));

  auto const publisherInteraction = publisher->getInteractionClassHandle(
      fixture_hla::fom::main_course_served);
  auto const subscriberInteraction = subscriber->getInteractionClassHandle(
      fixture_hla::fom::main_course_served);
  auto const temperatureOk = publisher->getParameterHandle(
      publisherInteraction, fixture_hla::fixture::temperature_ok);
  auto const publisherDimension = publisher->getDimensionHandle(
      fixture_hla::fixture::server_id);
  auto const subscriberDimension = subscriber->getDimensionHandle(
      fixture_hla::fixture::server_id);
  REQUIRE(publisherInteraction.isValid());
  REQUIRE(subscriberInteraction.isValid());
  REQUIRE(temperatureOk.isValid());
  REQUIRE(publisherDimension.isValid());
  REQUIRE(subscriberDimension.isValid());
  REQUIRE_NOTHROW(publisher->publishInteractionClass(publisherInteraction));

  auto const sourceRegion = publisher->createRegion(
      DimensionHandleSet{publisherDimension});
  auto const subscriptionRegion = subscriber->createRegion(
      DimensionHandleSet{subscriberDimension});
  REQUIRE_NOTHROW(publisher->setRangeBounds(
      sourceRegion, publisherDimension, RangeBounds(0UL, 2UL)));
  REQUIRE_NOTHROW(subscriber->setRangeBounds(
      subscriptionRegion, subscriberDimension, RangeBounds(0UL, 2UL)));
  REQUIRE_NOTHROW(publisher->commitRegionModifications(
      RegionHandleSet{sourceRegion}));
  REQUIRE_NOTHROW(subscriber->commitRegionModifications(
      RegionHandleSet{subscriptionRegion}));

  ParameterHandleValueMap parameters;
  unsigned char const valueBytes[] = {0x52U};
  parameters.emplace(
      temperatureOk,
      VariableLengthData(valueBytes, sizeof(valueBytes)));

  auto send = [&](std::string const& tag) {
    REQUIRE_NOTHROW(publisher->sendInteractionWithRegions(
        publisherInteraction,
        parameters,
        RegionHandleSet{sourceRegion},
        VariableLengthData(tag.data(), tag.size())));
    drain(*subscriber);
  };

  // An empty regional subscription must not create a subscription. The
  // overlapping source therefore has no eligible recipient yet.
  REQUIRE_NOTHROW(subscriber->subscribeInteractionClassWithRegions(
      subscriberInteraction, RegionHandleSet{}));
  send("empty-subscribe");
  REQUIRE(subscriberCallbacks.received.empty());

  // A non-empty subscription establishes the route, proving that the first
  // empty invocation did not silently broaden the subscription.
  REQUIRE_NOTHROW(subscriber->subscribeInteractionClassWithRegions(
      subscriberInteraction, RegionHandleSet{subscriptionRegion}));
  send("subscribed");
  REQUIRE(subscriberCallbacks.received.size() == 1U);
  REQUIRE(hasTag(subscriberCallbacks.received.back().userSuppliedTag, "subscribed"));

  // An empty unsubscription must not remove the established route.
  REQUIRE_NOTHROW(subscriber->unsubscribeInteractionClassWithRegions(
      subscriberInteraction, RegionHandleSet{}));
  send("empty-unsubscribe");
  REQUIRE(subscriberCallbacks.received.size() == 2U);
  REQUIRE(hasTag(
      subscriberCallbacks.received.back().userSuppliedTag,
      "empty-unsubscribe"));

  // The corresponding non-empty unsubscription still removes the route.
  REQUIRE_NOTHROW(subscriber->unsubscribeInteractionClassWithRegions(
      subscriberInteraction, RegionHandleSet{subscriptionRegion}));
  send("unsubscribed");
  REQUIRE(subscriberCallbacks.received.size() == 2U);

  REQUIRE_NOTHROW(subscriber->deleteRegion(subscriptionRegion));
  REQUIRE_NOTHROW(publisher->deleteRegion(sourceRegion));
  REQUIRE_NOTHROW(subscriber->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(
      rti1516_2025::CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(subscriber->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
}
