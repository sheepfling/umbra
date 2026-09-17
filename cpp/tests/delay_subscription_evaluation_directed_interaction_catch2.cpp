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
#error "The Delay Subscription Evaluation test requires the Umbra source directory."
#endif

namespace {

namespace standard_hla = umbra::detail::hla::wide;
namespace fixture_hla = umbra::test::hla::wide;

using rti1516_2025::CallbackModel;
using rti1516_2025::FederateHandle;
using rti1516_2025::HLA_EVOKED;
using rti1516_2025::HLA_IMMEDIATE;
using rti1516_2025::InteractionClassHandle;
using rti1516_2025::InteractionClassHandleSet;
using rti1516_2025::ObjectClassHandle;
using rti1516_2025::ObjectInstanceHandle;
using rti1516_2025::ParameterHandleValueMap;
using rti1516_2025::RegionHandleSet;
using rti1516_2025::RTIambassador;
using rti1516_2025::RTIambassadorFactory;
using rti1516_2025::TransportationTypeHandle;
using rti1516_2025::VariableLengthData;

std::wstring nextFederationName() {
  static std::atomic_uint64_t sequence{0U};
  return L"delay-subscription-evaluation-directed-interaction-" +
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
  };

  void discoverObjectInstance(
      ObjectInstanceHandle const& objectInstance,
      ObjectClassHandle const&,
      std::wstring const&,
      FederateHandle const&) override {
    objectDiscoveryReports.push_back(objectInstance);
  }

  void receiveDirectedInteraction(
      InteractionClassHandle const& interactionClass,
      ObjectInstanceHandle const& objectInstance,
      ParameterHandleValueMap const&,
      VariableLengthData const&,
      TransportationTypeHandle const&,
      FederateHandle const&) override {
    directedInteractionReports.push_back({interactionClass, objectInstance});
  }

  std::vector<ObjectInstanceHandle> objectDiscoveryReports;
  std::vector<DirectedInteractionReport> directedInteractionReports;
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
    "Embedded Delay Subscription Evaluation defers directed interaction eligibility",
    "[integration][development-profile][federation-management][object-management]"
    "[interaction-management][directed][delay-subscription-evaluation][callbacks]"
    "[callback-immediate]"
    "[rti.service.publish-object-class-attributes]"
    "[rti.service.subscribe-object-class-attributes]"
    "[rti.service.publish-object-class-directed-interactions]"
    "[rti.service.subscribe-object-class-directed-interactions]"
    "[rti.service.unsubscribe-object-class-directed-interactions]"
    "[rti.service.register-object-instance]"
    "[rti.service.send-directed-interaction]"
    "[rti.service.get-delay-subscription-evaluation-switch]"
    "[rti.service.disable-callbacks][rti.service.enable-callbacks]"
    "[rti.service.evoke-callback]"
    "[federate.callback.discover-object-instance]"
    "[federate.callback.receive-directed-interaction]") {
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
        L"delay-directed-owner", L"owner", federationName));
    REQUIRE_NOTHROW(receiver->joinFederationExecution(
        L"delay-directed-receiver", L"subscriber", federationName));

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
    drainCallbacks(*receiver);
    REQUIRE(receiverReports.objectDiscoveryReports.size() == 1U);

    // The receiver has no directed selector when the send is admitted. With
    // §8.1.10 delayed evaluation enabled, the route-only candidate survives
    // until this callback boundary and the selector added below makes it
    // deliverable. The disabled profile has no recipient to retain.
    if (immediate) {
      REQUIRE_NOTHROW(receiver->disableCallbacks());
    }
    REQUIRE_NOTHROW(owner->sendDirectedInteraction(
        interactionClass,
        target,
        ParameterHandleValueMap{},
        VariableLengthData{}));
    REQUIRE_NOTHROW(receiver->subscribeObjectClassDirectedInteractions(
        objectClass,
        directedClasses,
        true));
    if (immediate) {
      REQUIRE_NOTHROW(receiver->enableCallbacks());
    } else {
      static_cast<void>(receiver->evokeCallback(0.0));
    }
    REQUIRE(receiverReports.directedInteractionReports.size() ==
            (delaySubscriptionEvaluationEnabled ? 1U : 0U));
    if (delaySubscriptionEvaluationEnabled) {
      REQUIRE(receiverReports.directedInteractionReports.front().interactionClass ==
              interactionClass);
      REQUIRE(receiverReports.directedInteractionReports.front().objectInstance ==
              target);
    }

    // A delayed candidate is still subject to the live callback-time fence.
    // Removing the selector before delivery must suppress the accepted work
    // in both switch modes.
    if (immediate) {
      REQUIRE_NOTHROW(receiver->disableCallbacks());
    }
    REQUIRE_NOTHROW(owner->sendDirectedInteraction(
        interactionClass,
        target,
        ParameterHandleValueMap{},
        VariableLengthData{}));
    REQUIRE_NOTHROW(receiver->unsubscribeObjectClassDirectedInteractions(
        objectClass,
        directedClasses));
    if (immediate) {
      REQUIRE_NOTHROW(receiver->enableCallbacks());
    } else {
      static_cast<void>(receiver->evokeCallback(0.0));
    }
    REQUIRE(receiverReports.directedInteractionReports.size() ==
            (delaySubscriptionEvaluationEnabled ? 1U : 0U));

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
