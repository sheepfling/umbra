#include <catch2/catch_test_macros.hpp>

#include "internal/fom/hla_names.hpp"
#include "hla_test_names.hpp"

#include <RTI/NullFederateAmbassador.h>
#include <RTI/RTI1516.h>

#include <atomic>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#ifndef UMBRA_SOURCE_DIRECTORY
#error "The passive regional interaction transition test requires the Umbra source directory."
#endif

namespace {

namespace standard_hla = umbra::detail::hla::wide;
namespace fixture_hla = umbra::test::hla::wide;

using rti1516_2025::CANCEL_THEN_DELETE_THEN_DIVEST;
using rti1516_2025::FederateHandle;
using rti1516_2025::HLA_EVOKED;
using rti1516_2025::InteractionClassHandle;
using rti1516_2025::ParameterHandle;
using rti1516_2025::ParameterHandleValueMap;
using rti1516_2025::NO_ACTION;
using rti1516_2025::NullFederateAmbassador;
using rti1516_2025::RTIambassador;
using rti1516_2025::RTIambassadorFactory;
using rti1516_2025::RangeBounds;
using rti1516_2025::RegionHandleSet;
using rti1516_2025::TransportationTypeHandle;
using rti1516_2025::VariableLengthData;

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
      TransportationTypeHandle const& transportationType,
      FederateHandle const& producingFederate,
      RegionHandleSet const* optionalSentRegions) override {
    static_cast<void>(transportationType);
    static_cast<void>(producingFederate);
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
  return L"passive-regional-interaction-transition-" +
      std::to_wstring(sequence.fetch_add(1U, std::memory_order_relaxed));
}

void drain(RTIambassador& ambassador) {
  for (int pass = 0; pass != 64; ++pass) {
    static_cast<void>(ambassador.evokeCallback(0.0));
  }
}

}  // namespace

TEST_CASE(
    "Embedded passive regional interaction subscriptions preserve delivery and transition state",
    "[integration][development-profile][federation-management][interaction-management]"
    "[ddm][passive-subscription][regional-interaction][active-passive-transition]"
    "[passive-regional-interaction-transition]"
    "[rti.service.subscribe-interaction-class-with-regions]"
    "[rti.service.unsubscribe-interaction-class-with-regions]"
    "[rti.service.send-interaction-with-regions]"
    "[federate.callback.receive-interaction][callback-evoked][2025]") {
  NullFederateAmbassador publisherCallbacks;
  InteractionReceiver activeCallbacks;
  InteractionReceiver passiveCallbacks;
  auto publisher = makeRti();
  auto active = makeRti();
  auto passive = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = restaurantFom().wstring();
  unsigned char const parameterBytes[] = {0x2A, 0x15};
  unsigned char const tagBytes[] = {'p', 'r', 'g'};
  VariableLengthData const parameterValue(parameterBytes, sizeof(parameterBytes));
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));

  REQUIRE_NOTHROW(publisher->connect(publisherCallbacks, HLA_EVOKED));
  REQUIRE_NOTHROW(active->connect(activeCallbacks, HLA_EVOKED));
  REQUIRE_NOTHROW(passive->connect(passiveCallbacks, HLA_EVOKED));
  REQUIRE_NOTHROW(publisher->createFederationExecution(
      federationName,
      fomModule,
      standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(publisher->joinFederationExecution(
      L"passive-transition-publisher",
      L"publisher",
      federationName));
  REQUIRE_NOTHROW(active->joinFederationExecution(
      L"passive-transition-active",
      L"active-subscriber",
      federationName));
  REQUIRE_NOTHROW(passive->joinFederationExecution(
      L"passive-transition-passive",
      L"passive-subscriber",
      federationName));

  auto const publisherInteraction = publisher->getInteractionClassHandle(
      fixture_hla::fom::main_course_served);
  auto const activeInteraction = active->getInteractionClassHandle(
      fixture_hla::fom::main_course_served);
  auto const passiveInteraction = passive->getInteractionClassHandle(
      fixture_hla::fom::main_course_served);
  auto const publisherParameter = publisher->getParameterHandle(
      publisherInteraction,
      fixture_hla::fixture::temperature_ok);
  auto const activeParameter = active->getParameterHandle(
      activeInteraction,
      fixture_hla::fixture::temperature_ok);
  auto const passiveParameter = passive->getParameterHandle(
      passiveInteraction,
      fixture_hla::fixture::temperature_ok);
  auto const publisherDimension = publisher->getDimensionHandle(
      fixture_hla::fixture::server_id);
  auto const activeDimension = active->getDimensionHandle(
      fixture_hla::fixture::server_id);
  auto const passiveDimension = passive->getDimensionHandle(
      fixture_hla::fixture::server_id);
  REQUIRE(publisherInteraction.isValid());
  REQUIRE(activeInteraction.isValid());
  REQUIRE(passiveInteraction.isValid());
  REQUIRE(publisherParameter.isValid());
  REQUIRE(activeParameter.isValid());
  REQUIRE(passiveParameter.isValid());
  REQUIRE(publisherDimension.isValid());
  REQUIRE(activeDimension.isValid());
  REQUIRE(passiveDimension.isValid());

  auto const publisherRegion = publisher->createRegion(
      rti1516_2025::DimensionHandleSet{publisherDimension});
  auto const activeRegion = active->createRegion(
      rti1516_2025::DimensionHandleSet{activeDimension});
  auto const passiveRegion = passive->createRegion(
      rti1516_2025::DimensionHandleSet{passiveDimension});
  REQUIRE_NOTHROW(publisher->setRangeBounds(
      publisherRegion,
      publisherDimension,
      RangeBounds(0UL, 10UL)));
  REQUIRE_NOTHROW(active->setRangeBounds(
      activeRegion,
      activeDimension,
      RangeBounds(5UL, 15UL)));
  REQUIRE_NOTHROW(passive->setRangeBounds(
      passiveRegion,
      passiveDimension,
      RangeBounds(5UL, 15UL)));
  REQUIRE_NOTHROW(publisher->commitRegionModifications(
      RegionHandleSet{publisherRegion}));
  REQUIRE_NOTHROW(active->commitRegionModifications(
      RegionHandleSet{activeRegion}));
  REQUIRE_NOTHROW(passive->commitRegionModifications(
      RegionHandleSet{passiveRegion}));
  REQUIRE_NOTHROW(publisher->publishInteractionClass(publisherInteraction));

  REQUIRE_NOTHROW(active->subscribeInteractionClassWithRegions(
      activeInteraction,
      RegionHandleSet{activeRegion},
      true));
  // A passive subscription is retained for delivery but does not establish
  // interaction relevance on its own.  The active subscriber above makes the
  // interaction relevant for both overlapping regional recipients.
  REQUIRE_NOTHROW(passive->subscribeInteractionClassWithRegions(
      passiveInteraction,
      RegionHandleSet{passiveRegion},
      true));

  auto const send = [&] {
    REQUIRE_NOTHROW(publisher->sendInteractionWithRegions(
        publisherInteraction,
        ParameterHandleValueMap{{publisherParameter, parameterValue}},
        RegionHandleSet{publisherRegion},
        tag));
    drain(*active);
    drain(*passive);
  };
  auto const requireReceived = [&](
      InteractionReceiver const& receiver,
      InteractionClassHandle const& interactionClass,
      ParameterHandle const& parameter) {
    REQUIRE(receiver.received.size() == 1U);
    auto const& report = receiver.received.front();
    REQUIRE(report.interactionClass == interactionClass);
    REQUIRE(report.parameterValues.size() == 1U);
    REQUIRE(report.parameterValues.contains(parameter));
    REQUIRE(report.parameterValues.at(parameter).size() == sizeof(parameterBytes));
    auto const* observed = static_cast<unsigned char const*>(
        report.parameterValues.at(parameter).data());
    REQUIRE(observed != nullptr);
    REQUIRE(observed[0] == parameterBytes[0]);
    REQUIRE(observed[1] == parameterBytes[1]);
    REQUIRE(report.userSuppliedTag.size() == sizeof(tagBytes));
  };

  // Both active and passive regional recipients receive a send that is
  // already relevant because of the active subscription.
  send();
  requireReceived(activeCallbacks, activeInteraction, activeParameter);
  requireReceived(passiveCallbacks, passiveInteraction, passiveParameter);

  activeCallbacks.received.clear();
  passiveCallbacks.received.clear();
  // Replacing an active (passive=false) regional pair with passive=true must
  // preserve delivery while the separate active subscriber keeps relevance.
  REQUIRE_NOTHROW(passive->subscribeInteractionClassWithRegions(
      passiveInteraction,
      RegionHandleSet{passiveRegion},
      false));
  send();
  requireReceived(activeCallbacks, activeInteraction, activeParameter);
  requireReceived(passiveCallbacks, passiveInteraction, passiveParameter);

  activeCallbacks.received.clear();
  passiveCallbacks.received.clear();
  // With the only active regional declaration removed, the passive declaration
  // remains stored but no longer establishes relevance or delivery.
  REQUIRE_NOTHROW(active->unsubscribeInteractionClassWithRegions(
      activeInteraction,
      RegionHandleSet{activeRegion}));
  send();
  REQUIRE(activeCallbacks.received.empty());
  REQUIRE(passiveCallbacks.received.empty());

  // An empty region set is a no-op for active/passive state.  This must not
  // turn the retained passive pair back into an active subscription.
  REQUIRE_NOTHROW(passive->subscribeInteractionClassWithRegions(
      passiveInteraction,
      RegionHandleSet{},
      true));
  send();
  REQUIRE(passiveCallbacks.received.empty());

  // Supplying the existing region with active=true changes that pair back to
  // active and restores delivery/relevance.
  REQUIRE_NOTHROW(passive->subscribeInteractionClassWithRegions(
      passiveInteraction,
      RegionHandleSet{passiveRegion},
      true));
  send();
  requireReceived(passiveCallbacks, passiveInteraction, passiveParameter);

  REQUIRE_NOTHROW(passive->unsubscribeInteractionClassWithRegions(
      passiveInteraction,
      RegionHandleSet{passiveRegion}));
  REQUIRE_NOTHROW(passive->deleteRegion(passiveRegion));
  REQUIRE_NOTHROW(active->deleteRegion(activeRegion));
  REQUIRE_NOTHROW(publisher->deleteRegion(publisherRegion));
  REQUIRE_NOTHROW(publisher->unpublishInteractionClass(publisherInteraction));
  REQUIRE_NOTHROW(passive->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(active->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(
      CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(passive->disconnect());
  REQUIRE_NOTHROW(active->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
}
