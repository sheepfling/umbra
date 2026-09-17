#include <catch2/catch_test_macros.hpp>

#include "internal/fom/hla_names.hpp"
#include "hla_test_names.hpp"

#include <RTI/NullFederateAmbassador.h>
#include <RTI/RTI1516.h>
#include <RTI/time/HLAinteger64Interval.h>
#include <RTI/time/HLAinteger64Time.h>

#include <atomic>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#ifndef UMBRA_SOURCE_DIRECTORY
#error "The Delay Subscription Evaluation test requires the Umbra source directory."
#endif

namespace {

namespace standard_hla = umbra::detail::hla::wide;
namespace fixture_hla = umbra::test::hla::wide;

using rti1516_2025::CallbackModel;
using rti1516_2025::HLA_EVOKED;
using rti1516_2025::HLA_IMMEDIATE;
using rti1516_2025::InteractionClassHandle;
using rti1516_2025::InteractionClassHandleSet;
using rti1516_2025::LogicalTime;
using rti1516_2025::ObjectClassHandle;
using rti1516_2025::ObjectInstanceHandle;
using rti1516_2025::OrderType;
using rti1516_2025::ParameterHandleValueMap;
using rti1516_2025::RegionHandleSet;
using rti1516_2025::RTIambassador;
using rti1516_2025::RTIambassadorFactory;
using rti1516_2025::TIMESTAMP;
using rti1516_2025::TransportationTypeHandle;
using rti1516_2025::VariableLengthData;

std::wstring nextFederationName() {
  static std::atomic_uint64_t sequence{0U};
  return L"delay-subscription-evaluation-timestamped-directed-interaction-" +
      std::to_wstring(sequence.fetch_add(1U, std::memory_order_relaxed));
}

std::filesystem::path testDataPath(std::filesystem::path const& relativePath) {
  return std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" /
      "data" / relativePath;
}

class ReportingFederateAmbassador final
    : public rti1516_2025::NullFederateAmbassador {
 public:
  struct DirectedInteractionReport final {
    InteractionClassHandle interactionClass;
    ObjectInstanceHandle objectInstance;
    std::wstring timeImplementationName;
    std::wstring timeValue;
    OrderType sentOrderType = rti1516_2025::RECEIVE;
    OrderType receivedOrderType = rti1516_2025::RECEIVE;
  };

  void discoverObjectInstance(
      ObjectInstanceHandle const& objectInstance,
      ObjectClassHandle const&,
      std::wstring const&,
      rti1516_2025::FederateHandle const&) override {
    objectDiscoveryReports.push_back(objectInstance);
  }

  void timeConstrainedEnabled(LogicalTime const& time) override {
    timeConstrainedEnabledReports.push_back(time.toString());
  }

  void timeRegulationEnabled(LogicalTime const& time) override {
    timeRegulationEnabledReports.push_back(time.toString());
  }

  void receiveDirectedInteraction(
      InteractionClassHandle const& interactionClass,
      ObjectInstanceHandle const& objectInstance,
      ParameterHandleValueMap const&,
      VariableLengthData const&,
      TransportationTypeHandle const&,
      rti1516_2025::FederateHandle const&,
      LogicalTime const& time,
      OrderType sentOrderType,
      OrderType receivedOrderType,
      rti1516_2025::MessageRetractionHandle const*) override {
    directedInteractionReports.push_back({
        interactionClass,
        objectInstance,
        time.implementationName(),
        time.toString(),
        sentOrderType,
        receivedOrderType,
    });
  }

  void timeAdvanceGrant(LogicalTime const& time) override {
    timeAdvanceGrantReports.push_back(time.toString());
  }

  std::vector<ObjectInstanceHandle> objectDiscoveryReports;
  std::vector<DirectedInteractionReport> directedInteractionReports;
  std::vector<std::wstring> timeConstrainedEnabledReports;
  std::vector<std::wstring> timeRegulationEnabledReports;
  std::vector<std::wstring> timeAdvanceGrantReports;
};

std::unique_ptr<RTIambassador> makeRti() {
  RTIambassadorFactory factory;
  return factory.createRTIambassador();
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
    "Embedded Delay Subscription Evaluation defers timestamped directed interaction eligibility",
    "[integration][development-profile][interaction-management][directed][time-management]"
    "[delay-subscription-evaluation][tso]"
    "[rti.service.send-directed-interaction]"
    "[rti.service.get-delay-subscription-evaluation-switch]"
    "[rti.service.publish-object-class-directed-interactions]"
    "[rti.service.subscribe-object-class-directed-interactions]"
    "[rti.service.unsubscribe-object-class-directed-interactions]"
    "[rti.service.enable-time-regulation][rti.service.enable-time-constrained]"
    "[rti.service.time-advance-request][federate.callback.receive-directed-interaction]"
    "[federate.callback.time-advance-grant]") {
  auto runScenario = [](bool const delaySubscriptionEvaluationEnabled,
                        CallbackModel const callbackModel) {
    ReportingFederateAmbassador ownerReports;
    ReportingFederateAmbassador receiverReports;
    auto owner = makeRti();
    auto receiver = makeRti();
    bool const immediate = callbackModel == HLA_IMMEDIATE;
    auto const federationName = nextFederationName();
    auto const objectConsumer = testDataPath(
        "directed-interaction-object-consumer-fom.xml").wstring();
    auto const interactionProvider = testDataPath(
        "directed-interaction-interaction-provider-fom.xml").wstring();
    auto const switchesFom = testDataPath(
        "switch-support-enabled-fom.xml").wstring();
    std::vector<std::wstring> fomModules{objectConsumer, interactionProvider};
    if (delaySubscriptionEvaluationEnabled) {
      fomModules.push_back(switchesFom);
    }

    REQUIRE_NOTHROW(owner->connect(ownerReports, callbackModel));
    REQUIRE_NOTHROW(receiver->connect(receiverReports, callbackModel));
    REQUIRE_NOTHROW(owner->createFederationExecution(
        federationName,
        fomModules,
        standard_hla::mom::integer64_time));
    REQUIRE_NOTHROW(owner->joinFederationExecution(
        L"delay-tso-directed-owner", L"owner", federationName));
    REQUIRE_NOTHROW(receiver->joinFederationExecution(
        L"delay-tso-directed-receiver", L"subscriber", federationName));

    auto const objectClass = owner->getObjectClassHandle(
        fixture_hla::fom::directed_fixture_object);
    auto const marker = owner->getAttributeHandle(
        objectClass,
        fixture_hla::fixture::directed_target_marker);
    auto const interactionClass = owner->getInteractionClassHandle(
        fixture_hla::fom::directed_fixture_interaction);
    InteractionClassHandleSet const directedClasses{interactionClass};
    REQUIRE(objectClass.isValid());
    REQUIRE(marker.isValid());
    REQUIRE(interactionClass.isValid());
    REQUIRE(owner->getDelaySubscriptionEvaluationSwitch() ==
            delaySubscriptionEvaluationEnabled);
    REQUIRE(receiver->getDelaySubscriptionEvaluationSwitch() ==
            delaySubscriptionEvaluationEnabled);

    REQUIRE_NOTHROW(owner->publishObjectClassAttributes(objectClass, {marker}));
    REQUIRE_NOTHROW(receiver->subscribeObjectClassAttributes(objectClass, {marker}));
    REQUIRE_NOTHROW(owner->publishObjectClassDirectedInteractions(
        objectClass,
        directedClasses));

    ObjectInstanceHandle target;
    REQUIRE_NOTHROW(target = owner->registerObjectInstance(objectClass));
    REQUIRE(target.isValid());
    if (!immediate) {
      drainCallbacks(*receiver);
    }
    REQUIRE(receiverReports.objectDiscoveryReports.size() == 1U);

    REQUIRE_NOTHROW(receiver->enableTimeConstrained());
    if (!immediate) {
      drainCallbacks(*receiver);
    }
    REQUIRE(receiverReports.timeConstrainedEnabledReports.size() == 1U);
    REQUIRE_NOTHROW(owner->enableTimeRegulation(
        rti1516_2025::HLAinteger64Interval(1)));
    if (!immediate) {
      drainCallbacks(*owner);
    }
    REQUIRE(ownerReports.timeRegulationEnabledReports.size() == 1U);
    REQUIRE_NOTHROW(owner->changeInteractionOrderType(
        interactionClass,
        TIMESTAMP));

    // With Delay Subscription Evaluation enabled, this timestamped send
    // retains a route-only candidate even though no directed selector exists
    // yet. The default-disabled profile has no eligible recipient at send.
    auto const firstRetraction = owner->sendDirectedInteraction(
        interactionClass,
        target,
        ParameterHandleValueMap{},
        VariableLengthData{},
        rti1516_2025::HLAinteger64Time(2));
    REQUIRE(firstRetraction.isValid() == delaySubscriptionEvaluationEnabled);
    REQUIRE(receiverReports.directedInteractionReports.empty());
    REQUIRE_NOTHROW(receiver->subscribeObjectClassDirectedInteractions(
        objectClass,
        directedClasses,
        true));

    REQUIRE_NOTHROW(receiver->timeAdvanceRequest(
        rti1516_2025::HLAinteger64Time(2)));
    REQUIRE_NOTHROW(owner->timeAdvanceRequest(
        rti1516_2025::HLAinteger64Time(2)));
    if (!immediate) {
      drainCallbacks(*owner);
      drainCallbacks(*receiver);
    }
    REQUIRE(receiverReports.directedInteractionReports.size() ==
            (delaySubscriptionEvaluationEnabled ? 1U : 0U));
    REQUIRE(receiverReports.timeAdvanceGrantReports.size() == 1U);
    if (delaySubscriptionEvaluationEnabled) {
      auto const& report = receiverReports.directedInteractionReports.front();
      REQUIRE(report.interactionClass == interactionClass);
      REQUIRE(report.objectInstance == target);
      REQUIRE(report.timeValue == L"2");
      REQUIRE(report.sentOrderType == TIMESTAMP);
      REQUIRE(report.receivedOrderType == TIMESTAMP);
    }

    // The second queued candidate is evaluated against the live selector at
    // the grant boundary. Removing the selector before delivery suppresses it
    // in both profiles.
    auto const secondRetraction = owner->sendDirectedInteraction(
        interactionClass,
        target,
        ParameterHandleValueMap{},
        VariableLengthData{},
        rti1516_2025::HLAinteger64Time(3));
    REQUIRE(secondRetraction.isValid());
    REQUIRE_NOTHROW(receiver->unsubscribeObjectClassDirectedInteractions(
        objectClass,
        directedClasses));
    REQUIRE_NOTHROW(receiver->timeAdvanceRequest(
        rti1516_2025::HLAinteger64Time(3)));
    REQUIRE_NOTHROW(owner->timeAdvanceRequest(
        rti1516_2025::HLAinteger64Time(3)));
    if (!immediate) {
      drainCallbacks(*owner);
      drainCallbacks(*receiver);
    }
    REQUIRE(receiverReports.directedInteractionReports.size() ==
            (delaySubscriptionEvaluationEnabled ? 1U : 0U));
    REQUIRE(receiverReports.timeAdvanceGrantReports.size() == 2U);

    REQUIRE_NOTHROW(receiver->resignFederationExecution(
        rti1516_2025::NO_ACTION));
    REQUIRE_NOTHROW(owner->resignFederationExecution(
        rti1516_2025::CANCEL_THEN_DELETE_THEN_DIVEST));
    REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
    REQUIRE_NOTHROW(receiver->disconnect());
    REQUIRE_NOTHROW(owner->disconnect());
  };

  SECTION("the creation-time switch is Enabled under HLA_EVOKED") {
    runScenario(true, HLA_EVOKED);
  }
  SECTION("the omitted switch uses the Disabled default under HLA_EVOKED") {
    runScenario(false, HLA_EVOKED);
  }
  SECTION("the creation-time switch is Enabled under HLA_IMMEDIATE") {
    runScenario(true, HLA_IMMEDIATE);
  }
  SECTION("the omitted switch uses the Disabled default under HLA_IMMEDIATE") {
    runScenario(false, HLA_IMMEDIATE);
  }
}
