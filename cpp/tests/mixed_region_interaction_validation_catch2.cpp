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
#error "The mixed-region interaction-validation test requires the Umbra source directory."
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
  return L"restaurant-mixed-region-interaction-validation-" +
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

bool sameBytes(VariableLengthData const& left, VariableLengthData const& right) {
  if (left.size() != right.size()) {
    return false;
  }
  auto const* leftBytes = static_cast<unsigned char const*>(left.data());
  auto const* rightBytes = static_cast<unsigned char const*>(right.data());
  if (left.size() == 0U) {
    return true;
  }
  return leftBytes != nullptr && rightBytes != nullptr &&
      std::equal(leftBytes, leftBytes + left.size(), rightBytes);
}

}  // namespace

TEST_CASE(
    "Embedded mixed-dimensional regional interactions reject an invalid region set atomically",
    "[integration][development-profile][federation-management][interaction-management]"
    "[ddm][regional-interaction][mixed-dimensional-region-set][validation]"
    "[positive-dimensional-overlap][rti.service.get-dimension-handle]"
    "[rti.service.get-parameter-handle][rti.service.create-region]"
    "[rti.service.set-range-bounds][rti.service.commit-region-modifications]"
    "[rti.service.publish-interaction-class][rti.service.subscribe-interaction-class]"
    "[rti.service.send-interaction-with-regions]"
    "[rti.service.set-convey-region-designator-sets-switch]"
    "[federate.callback.receive-interaction][callback-evoked][2025]") {
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
      L"mixed-region-interaction-publisher", L"publisher", federationName));
  REQUIRE_NOTHROW(subscriber->joinFederationExecution(
      L"mixed-region-interaction-subscriber", L"subscriber", federationName));

  auto const publisherInteraction = publisher->getInteractionClassHandle(
      fixture_hla::fom::main_course_served);
  auto const subscriberInteraction = subscriber->getInteractionClassHandle(
      fixture_hla::fom::main_course_served);
  auto const parameter = publisher->getParameterHandle(
      publisherInteraction, fixture_hla::fixture::temperature_ok);
  auto const publisherDimension = publisher->getDimensionHandle(
      fixture_hla::fixture::server_id);
  auto const unrelatedDimension = publisher->getDimensionHandle(
      fixture_hla::fixture::soda_flavor);
  REQUIRE(publisherInteraction.isValid());
  REQUIRE(subscriberInteraction.isValid());
  REQUIRE(parameter.isValid());
  REQUIRE(publisherDimension.isValid());
  REQUIRE(unrelatedDimension.isValid());
  REQUIRE_NOTHROW(publisher->publishInteractionClass(publisherInteraction));
  REQUIRE_NOTHROW(subscriber->subscribeInteractionClass(subscriberInteraction));
  REQUIRE_NOTHROW(subscriber->setConveyRegionDesignatorSetsSwitch(true));

  auto const validRegion = publisher->createRegion(
      DimensionHandleSet{publisherDimension});
  auto const invalidRegion = publisher->createRegion(
      DimensionHandleSet{unrelatedDimension});
  REQUIRE(validRegion.isValid());
  REQUIRE(invalidRegion.isValid());
  REQUIRE_NOTHROW(publisher->setRangeBounds(
      validRegion, publisherDimension, RangeBounds(0UL, 10UL)));
  REQUIRE_NOTHROW(publisher->setRangeBounds(
      invalidRegion, unrelatedDimension, RangeBounds(0UL, 4UL)));
  REQUIRE_NOTHROW(publisher->commitRegionModifications(
      RegionHandleSet{validRegion, invalidRegion}));

  unsigned char const parameterBytes[] = {0x01U};
  ParameterHandleValueMap parameters;
  parameters.emplace(
      parameter,
      VariableLengthData(parameterBytes, sizeof(parameterBytes)));
  unsigned char const invalidTagBytes[] = {'m', 'i', 'x', 'e', 'd'};
  VariableLengthData const invalidTag(invalidTagBytes, sizeof(invalidTagBytes));

  // IEEE 1516.1-2025 §9.1.3.3 requires every region associated with a sent
  // interaction to use only dimensions available to that interaction class.
  // A mixed set must fail before any portion of the send is routed.
  REQUIRE_THROWS_AS(
      publisher->sendInteractionWithRegions(
          publisherInteraction,
          parameters,
          RegionHandleSet{validRegion, invalidRegion},
          invalidTag),
      rti1516_2025::InvalidRegionContext);
  drain(*subscriber);
  REQUIRE(subscriberCallbacks.received.empty());

  unsigned char const validTagBytes[] = {'v', 'a', 'l', 'i', 'd'};
  VariableLengthData const validTag(validTagBytes, sizeof(validTagBytes));
  REQUIRE_NOTHROW(publisher->sendInteractionWithRegions(
      publisherInteraction,
      parameters,
      RegionHandleSet{validRegion},
      validTag));
  drain(*subscriber);
  REQUIRE(subscriberCallbacks.received.size() == 1U);
  auto const& received = subscriberCallbacks.received.back();
  REQUIRE(received.interactionClass == subscriberInteraction);
  REQUIRE(received.parameterValues.size() == parameters.size());
  REQUIRE(received.parameterValues.contains(parameter));
  REQUIRE(sameBytes(received.parameterValues.at(parameter), parameters.at(parameter)));
  REQUIRE(received.sentRegionsSupplied);
  REQUIRE(received.sentRegions == RegionHandleSet{validRegion});
  REQUIRE(hasTag(received.userSuppliedTag, "valid"));

  REQUIRE_NOTHROW(subscriber->unsubscribeInteractionClass(subscriberInteraction));
  REQUIRE_NOTHROW(publisher->deleteRegion(invalidRegion));
  REQUIRE_NOTHROW(publisher->deleteRegion(validRegion));
  REQUIRE_NOTHROW(subscriber->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(
      rti1516_2025::CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(subscriber->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
}

TEST_CASE(
    "Embedded regional interaction subscriptions reject unavailable dimensions atomically",
    "[integration][development-profile][federation-management][interaction-management]"
    "[ddm][regional-interaction][subscription-dimension-validation][2025]"
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
      L"subscription-dimension-validation-publisher", L"publisher", federationName));
  REQUIRE_NOTHROW(subscriber->joinFederationExecution(
      L"subscription-dimension-validation-subscriber", L"subscriber", federationName));

  auto const publisherInteraction = publisher->getInteractionClassHandle(
      fixture_hla::fom::main_course_served);
  auto const subscriberInteraction = subscriber->getInteractionClassHandle(
      fixture_hla::fom::main_course_served);
  auto const parameter = publisher->getParameterHandle(
      publisherInteraction, fixture_hla::fixture::temperature_ok);
  auto const publisherDimension = publisher->getDimensionHandle(
      fixture_hla::fixture::server_id);
  auto const subscriberDimension = subscriber->getDimensionHandle(
      fixture_hla::fixture::server_id);
  auto const unavailableDimension = subscriber->getDimensionHandle(
      fixture_hla::fixture::soda_flavor);
  REQUIRE(publisherInteraction.isValid());
  REQUIRE(subscriberInteraction.isValid());
  REQUIRE(parameter.isValid());
  REQUIRE(publisherDimension.isValid());
  REQUIRE(subscriberDimension.isValid());
  REQUIRE(unavailableDimension.isValid());
  REQUIRE_NOTHROW(publisher->publishInteractionClass(publisherInteraction));

  auto const sourceRegion = publisher->createRegion(
      DimensionHandleSet{publisherDimension});
  auto const validSubscriptionRegion = subscriber->createRegion(
      DimensionHandleSet{subscriberDimension});
  auto const invalidSubscriptionRegion = subscriber->createRegion(
      DimensionHandleSet{unavailableDimension});
  REQUIRE(sourceRegion.isValid());
  REQUIRE(validSubscriptionRegion.isValid());
  REQUIRE(invalidSubscriptionRegion.isValid());
  REQUIRE_NOTHROW(publisher->setRangeBounds(
      sourceRegion, publisherDimension, RangeBounds(0UL, 2UL)));
  REQUIRE_NOTHROW(subscriber->setRangeBounds(
      validSubscriptionRegion, subscriberDimension, RangeBounds(0UL, 2UL)));
  REQUIRE_NOTHROW(subscriber->setRangeBounds(
      invalidSubscriptionRegion, unavailableDimension, RangeBounds(0UL, 2UL)));
  REQUIRE_NOTHROW(publisher->commitRegionModifications(
      RegionHandleSet{sourceRegion}));
  REQUIRE_NOTHROW(subscriber->commitRegionModifications(
      RegionHandleSet{validSubscriptionRegion, invalidSubscriptionRegion}));

  ParameterHandleValueMap parameters;
  unsigned char const parameterBytes[] = {0x01U};
  parameters.emplace(
      parameter,
      VariableLengthData(parameterBytes, sizeof(parameterBytes)));
  auto send = [&](std::string const& tag) {
    REQUIRE_NOTHROW(publisher->sendInteractionWithRegions(
        publisherInteraction,
        parameters,
        RegionHandleSet{sourceRegion},
        VariableLengthData(tag.data(), tag.size())));
    drain(*subscriber);
  };

  // §9.1.4 requires every subscription region to use only dimensions
  // available to the interaction class.  A rejected declaration must not
  // leave a partial subscription behind.
  REQUIRE_THROWS_AS(
      subscriber->subscribeInteractionClassWithRegions(
          subscriberInteraction,
          RegionHandleSet{invalidSubscriptionRegion}),
      rti1516_2025::InvalidRegionContext);
  send("after-invalid-subscribe");
  REQUIRE(subscriberCallbacks.received.empty());

  REQUIRE_NOTHROW(subscriber->subscribeInteractionClassWithRegions(
      subscriberInteraction, RegionHandleSet{validSubscriptionRegion}));
  send("valid-subscribe");
  REQUIRE(subscriberCallbacks.received.size() == 1U);
  REQUIRE(hasTag(
      subscriberCallbacks.received.back().userSuppliedTag,
      "valid-subscribe"));

  REQUIRE_NOTHROW(subscriber->unsubscribeInteractionClassWithRegions(
      subscriberInteraction, RegionHandleSet{validSubscriptionRegion}));
  REQUIRE_NOTHROW(subscriber->deleteRegion(invalidSubscriptionRegion));
  REQUIRE_NOTHROW(subscriber->deleteRegion(validSubscriptionRegion));
  REQUIRE_NOTHROW(publisher->deleteRegion(sourceRegion));
  REQUIRE_NOTHROW(subscriber->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(
      rti1516_2025::CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(subscriber->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
}
