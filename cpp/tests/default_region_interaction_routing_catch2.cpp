#include <catch2/catch_test_macros.hpp>

#include "hla_test_names.hpp"

#include <RTI/NullFederateAmbassador.h>
#include <RTI/RTI1516.h>

#include <atomic>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#ifndef UMBRA_SOURCE_DIRECTORY
#error "The default-region interaction-routing test requires the Umbra source directory."
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
using rti1516_2025::ParameterHandle;
using rti1516_2025::ParameterHandleValueMap;
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
  return L"default-region-interaction-routing-" +
      std::to_wstring(sequence.fetch_add(1U, std::memory_order_relaxed));
}

void drain(RTIambassador& ambassador) {
  static_cast<void>(ambassador.evokeMultipleCallbacks(0.0, 0.0));
}

}  // namespace

TEST_CASE(
    "Embedded focused default-region interaction routing derives 2025 ordinary and regional effectiveness",
    "[integration][development-profile][federation-management][interaction-management]"
    "[ddm][regional-interaction][default-region][positive-dimensional-overlap]"
    "[rti.service.create-region][rti.service.set-range-bounds]"
    "[rti.service.commit-region-modifications][rti.service.publish-interaction-class]"
    "[rti.service.subscribe-interaction-class][rti.service.subscribe-interaction-class-with-regions]"
    "[rti.service.unsubscribe-interaction-class-with-regions]"
    "[rti.service.send-interaction][rti.service.send-interaction-with-regions]"
    "[rti.service.set-convey-region-designator-sets-switch]"
    "[federate.callback.receive-interaction][callback-evoked][2025]") {
  NullFederateAmbassador publisherCallbacks;
  InteractionReceiver regionalCallbacks;
  InteractionReceiver mixedCallbacks;
  auto publisher = makeRti();
  auto regional = makeRti();
  auto mixed = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = restaurantFom().wstring();
  unsigned char const parameterBytes[] = {0xD1, 0x04};
  ParameterHandleValueMap parameters;

  REQUIRE_NOTHROW(publisher->connect(publisherCallbacks, HLA_EVOKED));
  REQUIRE_NOTHROW(regional->connect(regionalCallbacks, HLA_EVOKED));
  REQUIRE_NOTHROW(mixed->connect(mixedCallbacks, HLA_EVOKED));
  REQUIRE_NOTHROW(publisher->createFederationExecution(
      federationName,
      fomModule,
      L"HLAinteger64Time"));
  REQUIRE_NOTHROW(publisher->joinFederationExecution(
      L"default-region-interaction-publisher",
      L"publisher",
      federationName));
  REQUIRE_NOTHROW(regional->joinFederationExecution(
      L"default-region-interaction-regional",
      L"subscriber",
      federationName));
  REQUIRE_NOTHROW(mixed->joinFederationExecution(
      L"default-region-interaction-mixed",
      L"subscriber",
      federationName));

  auto const interactionClass = publisher->getInteractionClassHandle(
      fixture_hla::fom::main_course_served);
  auto const temperatureOk = publisher->getParameterHandle(
      interactionClass,
      fixture_hla::fixture::temperature_ok);
  auto const serverId = publisher->getDimensionHandle(
      fixture_hla::fixture::server_id);
  REQUIRE(interactionClass.isValid());
  REQUIRE(temperatureOk.isValid());
  REQUIRE(serverId.isValid());
  parameters.emplace(
      temperatureOk,
      VariableLengthData(parameterBytes, sizeof(parameterBytes)));
  REQUIRE_NOTHROW(publisher->publishInteractionClass(interactionClass));

  auto const sourceRegion = publisher->createRegion(DimensionHandleSet{serverId});
  auto const regionalRegion = regional->createRegion(DimensionHandleSet{serverId});
  auto const mixedRegion = mixed->createRegion(DimensionHandleSet{serverId});
  REQUIRE_NOTHROW(publisher->setRangeBounds(
      sourceRegion,
      serverId,
      rti1516_2025::RangeBounds(0UL, 1UL)));
  REQUIRE_NOTHROW(publisher->commitRegionModifications(RegionHandleSet{sourceRegion}));
  REQUIRE_NOTHROW(regional->setRangeBounds(
      regionalRegion,
      serverId,
      rti1516_2025::RangeBounds(0UL, 1UL)));
  REQUIRE_NOTHROW(regional->commitRegionModifications(RegionHandleSet{regionalRegion}));
  REQUIRE_NOTHROW(mixed->setRangeBounds(
      mixedRegion,
      serverId,
      rti1516_2025::RangeBounds(2UL, 3UL)));
  REQUIRE_NOTHROW(mixed->commitRegionModifications(RegionHandleSet{mixedRegion}));

  REQUIRE_NOTHROW(regional->subscribeInteractionClassWithRegions(
      interactionClass,
      RegionHandleSet{regionalRegion}));
  REQUIRE_NOTHROW(mixed->subscribeInteractionClass(interactionClass));
  REQUIRE_NOTHROW(mixed->subscribeInteractionClassWithRegions(
      interactionClass,
      RegionHandleSet{mixedRegion}));

  VariableLengthData const tag;
  // A matching explicit source region reaches the regional subscriber.  The
  // mixed subscriber's disjoint regional realization suppresses its retained
  // ordinary/default declaration.
  REQUIRE_NOTHROW(publisher->sendInteractionWithRegions(
      interactionClass,
      parameters,
      RegionHandleSet{sourceRegion},
      tag));
  drain(*regional);
  drain(*mixed);
  REQUIRE(regionalCallbacks.received.size() == 1U);
  REQUIRE(mixedCallbacks.received.empty());
  REQUIRE(regionalCallbacks.received.front().interactionClass == interactionClass);
  REQUIRE(regionalCallbacks.received.front().parameterValues.contains(temperatureOk));

  // Removing the disjoint explicit declaration restores the mixed federate's
  // ordinary/default realization for a valid explicit source-region send.
  REQUIRE_NOTHROW(mixed->unsubscribeInteractionClassWithRegions(
      interactionClass,
      RegionHandleSet{mixedRegion}));
  REQUIRE_NOTHROW(publisher->sendInteractionWithRegions(
      interactionClass,
      parameters,
      RegionHandleSet{sourceRegion},
      tag));
  drain(*regional);
  drain(*mixed);
  REQUIRE(regionalCallbacks.received.size() == 2U);
  REQUIRE(mixedCallbacks.received.size() == 1U);

  // An ordinary send uses the invisible default region.  The regional
  // subscribers receive it, and the enabled convey switch exposes the
  // supplied-but-empty source RegionHandleSet in each callback.
  REQUIRE_NOTHROW(mixed->subscribeInteractionClassWithRegions(
      interactionClass,
      RegionHandleSet{mixedRegion}));
  REQUIRE_NOTHROW(regional->setConveyRegionDesignatorSetsSwitch(true));
  REQUIRE_NOTHROW(mixed->setConveyRegionDesignatorSetsSwitch(true));
  REQUIRE_NOTHROW(publisher->sendInteraction(interactionClass, parameters, tag));
  drain(*regional);
  drain(*mixed);
  REQUIRE(regionalCallbacks.received.size() == 3U);
  REQUIRE(mixedCallbacks.received.size() == 2U);
  REQUIRE(regionalCallbacks.received.back().sentRegionsSupplied);
  REQUIRE(regionalCallbacks.received.back().sentRegions.empty());
  REQUIRE(mixedCallbacks.received.back().sentRegionsSupplied);
  REQUIRE(mixedCallbacks.received.back().sentRegions.empty());

  REQUIRE_NOTHROW(mixed->unsubscribeInteractionClassWithRegions(
      interactionClass,
      RegionHandleSet{mixedRegion}));
  REQUIRE_NOTHROW(mixed->unsubscribeInteractionClass(interactionClass));
  REQUIRE_NOTHROW(regional->unsubscribeInteractionClassWithRegions(
      interactionClass,
      RegionHandleSet{regionalRegion}));
  REQUIRE_NOTHROW(publisher->deleteRegion(sourceRegion));
  REQUIRE_NOTHROW(regional->deleteRegion(regionalRegion));
  REQUIRE_NOTHROW(mixed->deleteRegion(mixedRegion));
  REQUIRE_NOTHROW(mixed->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(regional->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(mixed->disconnect());
  REQUIRE_NOTHROW(regional->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
}

TEST_CASE(
    "Embedded whole-class interaction unsubscription removes default-region subscription",
    "[integration][development-profile][federation-management][interaction-management]"
    "[ddm][regional-interaction][default-region][whole-class-unsubscribe][2025]"
    "[rti.service.publish-interaction-class]"
    "[rti.service.subscribe-interaction-class]"
    "[rti.service.unsubscribe-interaction-class]"
    "[rti.service.send-interaction]"
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
      L"whole-class-unsubscribe-publisher", L"publisher", federationName));
  REQUIRE_NOTHROW(subscriber->joinFederationExecution(
      L"whole-class-unsubscribe-subscriber", L"subscriber", federationName));

  auto const publisherInteraction = publisher->getInteractionClassHandle(
      fixture_hla::fom::main_course_served);
  auto const subscriberInteraction = subscriber->getInteractionClassHandle(
      fixture_hla::fom::main_course_served);
  auto const parameter = publisher->getParameterHandle(
      publisherInteraction, fixture_hla::fixture::temperature_ok);
  REQUIRE(publisherInteraction.isValid());
  REQUIRE(subscriberInteraction.isValid());
  REQUIRE(parameter.isValid());
  REQUIRE_NOTHROW(publisher->publishInteractionClass(publisherInteraction));

  ParameterHandleValueMap parameters;
  unsigned char const parameterBytes[] = {0x2AU};
  parameters.emplace(
      parameter,
      VariableLengthData(parameterBytes, sizeof(parameterBytes)));
  REQUIRE_NOTHROW(subscriber->subscribeInteractionClass(subscriberInteraction));
  auto send = [&](std::string const& tag) {
    REQUIRE_NOTHROW(publisher->sendInteraction(
        publisherInteraction,
        parameters,
        VariableLengthData(tag.data(), tag.size())));
    drain(*subscriber);
  };

  // The ordinary subscription uses the implicit default region.
  send("before-unsubscribe");
  REQUIRE(subscriberCallbacks.received.size() == 1U);

  // §9.1.4 requires whole-class unsubscription to remove that default-region
  // subscription; a later ordinary send must not be delivered.
  REQUIRE_NOTHROW(subscriber->unsubscribeInteractionClass(subscriberInteraction));
  send("after-unsubscribe");
  REQUIRE(subscriberCallbacks.received.size() == 1U);

  REQUIRE_NOTHROW(subscriber->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(subscriber->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
}
