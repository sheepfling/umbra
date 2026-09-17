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
#error "The zero-dimensional regional-interaction test requires the Umbra source directory."
#endif

namespace {

using rti1516_2025::FederateHandle;
using rti1516_2025::HLA_EVOKED;
using rti1516_2025::InteractionClassHandle;
using rti1516_2025::NO_ACTION;
using rti1516_2025::NullFederateAmbassador;
using rti1516_2025::ParameterHandle;
using rti1516_2025::ParameterHandleValueMap;
using rti1516_2025::DimensionHandle;
using rti1516_2025::DimensionHandleSet;
using rti1516_2025::RTIambassador;
using rti1516_2025::RTIambassadorFactory;
using rti1516_2025::RegionHandle;
using rti1516_2025::RegionHandleSet;
using rti1516_2025::VariableLengthData;

namespace fixture_hla = umbra::test::hla::wide;

class InteractionReceiver final : public NullFederateAmbassador {
 public:
  struct Received final {
    InteractionClassHandle interactionClass;
    ParameterHandleValueMap parameterValues;
    VariableLengthData userSuppliedTag;
    bool sentRegionsSupplied = false;
  };

  void receiveInteraction(
      InteractionClassHandle const& interactionClass,
      ParameterHandleValueMap const& parameterValues,
      VariableLengthData const& userSuppliedTag,
      rti1516_2025::TransportationTypeHandle const&,
      FederateHandle const&,
      RegionHandleSet const* optionalSentRegions) override {
    received.push_back({
        interactionClass,
        parameterValues,
        userSuppliedTag,
        optionalSentRegions != nullptr,
    });
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
  return L"zero-dimensional-regional-interaction-" +
      std::to_wstring(sequence.fetch_add(1U, std::memory_order_relaxed));
}

void drain(RTIambassador& ambassador) {
  for (int pass = 0; pass != 64; ++pass) {
    if (!ambassador.evokeCallback(0.0)) {
      break;
    }
  }
}

}  // namespace

TEST_CASE(
    "Embedded zero-dimensional regional interactions do not overlap the default region",
    "[integration][development-profile][federation-management][interaction-management]"
    "[ddm][zero-dimensional-region][regional-interaction]"
    "[zero-dimensional-regional-interaction]"
    "[rti.service.create-region][rti.service.commit-region-modifications]"
    "[rti.service.publish-interaction-class][rti.service.subscribe-interaction-class]"
    "[rti.service.send-interaction][rti.service.send-interaction-with-regions]"
    "[federate.callback.receive-interaction][callback-evoked][2025]") {
  NullFederateAmbassador publisherCallbacks;
  InteractionReceiver subscriberCallbacks;
  auto publisher = makeRti();
  auto subscriber = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = restaurantFom().wstring();
  unsigned char const parameterBytes[] = {0x2A, 0x15};
  unsigned char const tagBytes[] = {'z', 'd', 'm'};
  VariableLengthData const parameterValue(parameterBytes, sizeof(parameterBytes));
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));

  REQUIRE_NOTHROW(publisher->connect(publisherCallbacks, HLA_EVOKED));
  REQUIRE_NOTHROW(subscriber->connect(subscriberCallbacks, HLA_EVOKED));
  REQUIRE_NOTHROW(publisher->createFederationExecution(
      federationName,
      fomModule,
      L"HLAinteger64Time"));
  REQUIRE_NOTHROW(publisher->joinFederationExecution(
      L"zero-dimensional-interaction-publisher",
      L"publisher",
      federationName));
  REQUIRE_NOTHROW(subscriber->joinFederationExecution(
      L"zero-dimensional-interaction-subscriber",
      L"subscriber",
      federationName));

  auto const publisherInteraction = publisher->getInteractionClassHandle(
      fixture_hla::fom::main_course_served);
  auto const subscriberInteraction = subscriber->getInteractionClassHandle(
      fixture_hla::fom::main_course_served);
  auto const publisherParameter = publisher->getParameterHandle(
      publisherInteraction,
      fixture_hla::fixture::temperature_ok);
  auto const subscriberParameter = subscriber->getParameterHandle(
      subscriberInteraction,
      fixture_hla::fixture::temperature_ok);
  auto const publisherDimension = publisher->getDimensionHandle(
      fixture_hla::fixture::server_id);
  REQUIRE(publisherInteraction.isValid());
  REQUIRE(subscriberInteraction.isValid());
  REQUIRE(publisherParameter.isValid());
  REQUIRE(subscriberParameter.isValid());
  REQUIRE(publisherDimension.isValid());

  REQUIRE_NOTHROW(publisher->publishInteractionClass(publisherInteraction));
  REQUIRE_NOTHROW(subscriber->subscribeInteractionClass(subscriberInteraction, true));

  // Establish the ordinary/default-region baseline before testing an explicit
  // zero-dimensional source region.  The subscriber must be genuinely
  // eligible; the later empty realization is the only changed variable.
  REQUIRE_NOTHROW(publisher->sendInteraction(
      publisherInteraction,
      ParameterHandleValueMap{{publisherParameter, parameterValue}},
      tag));
  drain(*subscriber);
  REQUIRE(subscriberCallbacks.received.size() == 1U);
  REQUIRE(subscriberCallbacks.received.front().interactionClass == subscriberInteraction);
  REQUIRE(subscriberCallbacks.received.front().parameterValues.size() == 1U);
  REQUIRE(subscriberCallbacks.received.front().parameterValues.contains(subscriberParameter));
  REQUIRE(subscriberCallbacks.received.front().userSuppliedTag.size() == sizeof(tagBytes));
  REQUIRE_FALSE(subscriberCallbacks.received.front().sentRegionsSupplied);

  auto const zeroRegion = publisher->createRegion(DimensionHandleSet{});
  REQUIRE(zeroRegion.isValid());
  REQUIRE(publisher->getDimensionHandleSet(zeroRegion).empty());
  REQUIRE_NOTHROW(publisher->commitRegionModifications(RegionHandleSet{zeroRegion}));
  REQUIRE(publisher->getDimensionHandleSet(zeroRegion).empty());

  subscriberCallbacks.received.clear();
  // An explicit zero-dimensional realization is not the ordinary send path;
  // §9.1.3.2 requires it to overlap no realization, including the default.
  REQUIRE_NOTHROW(publisher->sendInteractionWithRegions(
      publisherInteraction,
      ParameterHandleValueMap{{publisherParameter, parameterValue}},
      RegionHandleSet{zeroRegion},
      tag));
  drain(*subscriber);
  REQUIRE(subscriberCallbacks.received.empty());

  REQUIRE_NOTHROW(publisher->deleteRegion(zeroRegion));
  REQUIRE_NOTHROW(subscriber->unsubscribeInteractionClass(subscriberInteraction));
  REQUIRE_NOTHROW(publisher->unpublishInteractionClass(publisherInteraction));
  REQUIRE_NOTHROW(subscriber->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(subscriber->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
}
