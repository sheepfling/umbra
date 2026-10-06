#include "ieee1516_2025_federation_management_fixture_support.hpp"

TEST_CASE(
    "Embedded Delay Subscription Evaluation defers ordinary receive-order interaction eligibility",
    "[integration][development-profile][interaction-management][delay-subscription-evaluation]"
    "[delay-subscription-evaluation-receive-order-interaction]"
    "[rti.service.send-interaction]"
    "[rti.service.get-delay-subscription-evaluation-switch]"
    "[rti.service.subscribe-interaction-class]"
    "[rti.service.unsubscribe-interaction-class]"
    "[rti.service.evoke-callback]"
    "[rti.service.disable-callbacks]"
    "[rti.service.enable-callbacks]"
    "[callback-immediate]"
    "[federate.callback.receive-interaction]") {
  auto runScenario = [](bool const delaySubscriptionEvaluationEnabled,
                        auto const callbackModel) {
    ReportingFederateAmbassador publisherReports;
    ReportingFederateAmbassador receiverReports;
    auto publisher = makeRti();
    auto receiver = makeRti();
    bool const immediate = callbackModel == rti1516_2025::HLA_IMMEDIATE;
    auto const federationName = nextFederationName();
    auto const interactionFom = (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
                                 "cpp" / "tests" / "data" /
                                 "parameter-handle-provider-fom.xml")
                                    .wstring();
    auto const switchesFom = (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
                              "cpp" / "tests" / "data" /
                              "switch-support-enabled-fom.xml")
                                 .wstring();
    std::vector<std::wstring> fomModules{interactionFom};
    if (delaySubscriptionEvaluationEnabled) {
      fomModules.push_back(switchesFom);
    }

    REQUIRE_NOTHROW(publisher->connect(publisherReports, callbackModel));
    REQUIRE_NOTHROW(receiver->connect(receiverReports, callbackModel));
    REQUIRE_NOTHROW(publisher->createFederationExecution(
        federationName, fomModules, standard_hla::mom::integer64_time));
    REQUIRE_NOTHROW(publisher->joinFederationExecution(
        L"delay-ro-publisher", L"publisher", federationName));
    REQUIRE_NOTHROW(receiver->joinFederationExecution(
        L"delay-ro-receiver", L"subscriber", federationName));

    auto const interactionClass = publisher->getInteractionClassHandle(
        fixture_hla::fom::parameter_fixture_child_interaction);
    REQUIRE(interactionClass.isValid());
    REQUIRE(publisher->getDelaySubscriptionEvaluationSwitch() ==
            delaySubscriptionEvaluationEnabled);
    REQUIRE(receiver->getDelaySubscriptionEvaluationSwitch() ==
            delaySubscriptionEvaluationEnabled);
    REQUIRE_NOTHROW(publisher->publishInteractionClass(interactionClass));

    // The receiver is joined but has no subscription when the RO message is
    // generated.  Only the Enabled federation keeps it as a candidate until
    // the receiver's actual callback boundary. HLA_IMMEDIATE establishes the
    // same boundary by temporarily suspending callback dispatch.
    if (immediate) {
      REQUIRE_NOTHROW(receiver->disableCallbacks());
    }
    REQUIRE_NOTHROW(publisher->sendInteraction(
        interactionClass, ParameterHandleValueMap{}, VariableLengthData()));
    REQUIRE_NOTHROW(receiver->subscribeInteractionClass(interactionClass));
    if (immediate) {
      REQUIRE_NOTHROW(receiver->enableCallbacks());
    } else {
      static_cast<void>(receiver->evokeCallback(0.0));
    }
    REQUIRE(receiverReports.interactionReports.size() ==
            (delaySubscriptionEvaluationEnabled ? 1U : 0U));
    if (delaySubscriptionEvaluationEnabled) {
      REQUIRE(receiverReports.interactionReports.front().interactionClass == interactionClass);
    }

    // Clause 8.1.8 requires an actual-delivery decision from the receiver's
    // current subscriptions in both modes. An accepted recipient that
    // unsubscribes before its callback boundary must therefore be suppressed.
    if (immediate) {
      REQUIRE_NOTHROW(receiver->disableCallbacks());
    }
    REQUIRE_NOTHROW(publisher->sendInteraction(
        interactionClass, ParameterHandleValueMap{}, VariableLengthData()));
    REQUIRE_NOTHROW(receiver->unsubscribeInteractionClass(interactionClass));
    if (immediate) {
      REQUIRE_NOTHROW(receiver->enableCallbacks());
    } else {
      static_cast<void>(receiver->evokeCallback(0.0));
    }
    REQUIRE(receiverReports.interactionReports.size() ==
            (delaySubscriptionEvaluationEnabled ? 1U : 0U));

    REQUIRE_NOTHROW(receiver->resignFederationExecution(NO_ACTION));
    REQUIRE_NOTHROW(publisher->resignFederationExecution(NO_ACTION));
    REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
    REQUIRE_NOTHROW(receiver->disconnect());
    REQUIRE_NOTHROW(publisher->disconnect());
  };

  SECTION("the creation-time switch is Enabled under HLA_EVOKED") {
    runScenario(true, HLA_EVOKED);
  }
  SECTION("the omitted switch uses the Disabled default under HLA_EVOKED") {
    runScenario(false, HLA_EVOKED);
  }
  SECTION("the creation-time switch is Enabled under HLA_IMMEDIATE") {
    runScenario(true, rti1516_2025::HLA_IMMEDIATE);
  }
  SECTION("the omitted switch uses the Disabled default under HLA_IMMEDIATE") {
    runScenario(false, rti1516_2025::HLA_IMMEDIATE);
  }
}
