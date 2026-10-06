#include "ieee1516_2025_federation_management_fixture_support.hpp"

TEST_CASE(
    "Embedded Delay Subscription Evaluation defers ordinary timestamped interaction eligibility",
    "[integration][development-profile][interaction-management][time-management]"
    "[delay-subscription-evaluation][tso]"
    "[rti.service.send-interaction]"
    "[rti.service.get-delay-subscription-evaluation-switch]"
    "[rti.service.subscribe-interaction-class]"
    "[rti.service.unsubscribe-interaction-class]"
    "[rti.service.enable-time-regulation]"
    "[rti.service.enable-time-constrained]"
    "[rti.service.time-advance-request]"
    "[callback-immediate]"
    "[federate.callback.receive-interaction]"
    "[federate.callback.time-advance-grant]") {
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
        L"delay-tso-publisher", L"publisher", federationName));
    REQUIRE_NOTHROW(receiver->joinFederationExecution(
        L"delay-tso-receiver", L"subscriber", federationName));

    auto const interactionClass = publisher->getInteractionClassHandle(
        fixture_hla::fom::parameter_fixture_child_interaction);
    REQUIRE(interactionClass.isValid());
    REQUIRE(publisher->getDelaySubscriptionEvaluationSwitch() ==
            delaySubscriptionEvaluationEnabled);
    REQUIRE_NOTHROW(publisher->publishInteractionClass(interactionClass));
    REQUIRE_NOTHROW(publisher->changeInteractionOrderType(interactionClass, TIMESTAMP));
    REQUIRE_NOTHROW(receiver->enableTimeConstrained());
    if (!immediate) {
      static_cast<void>(receiver->evokeCallback(0.0));
    }
    REQUIRE(receiverReports.timeConstrainedEnabledReports.size() == 1);
    REQUIRE_NOTHROW(publisher->enableTimeRegulation(
        rti1516_2025::HLAinteger64Interval(1)));
    if (!immediate) {
      static_cast<void>(publisher->evokeCallback(0.0));
    }
    REQUIRE(publisherReports.timeRegulationEnabledReports.size() == 1);

    // The timestamped message is generated while the receiver is unsubscribed.
    // In the Enabled case it waits in the federation-owned TSO queue and is
    // projected only when the recipient becomes eligible at its grant.
    auto const firstRetraction = publisher->sendInteraction(
        interactionClass,
        ParameterHandleValueMap{},
        VariableLengthData(),
        rti1516_2025::HLAinteger64Time(2));
    REQUIRE(firstRetraction.isValid());
    REQUIRE_NOTHROW(receiver->subscribeInteractionClass(interactionClass));
    REQUIRE_NOTHROW(receiver->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(2)));
    REQUIRE_NOTHROW(publisher->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(2)));
    if (!immediate) {
      static_cast<void>(publisher->evokeCallback(0.0));
      static_cast<void>(receiver->evokeCallback(0.0));
    }
    REQUIRE(receiverReports.timestampedInteractionReports.size() ==
            (delaySubscriptionEvaluationEnabled ? 1U : 0U));
    REQUIRE(receiverReports.timeAdvanceGrantReports.size() == 1);
    if (delaySubscriptionEvaluationEnabled) {
      auto const& report = receiverReports.timestampedInteractionReports.front();
      REQUIRE(report.interactionClass == interactionClass);
      REQUIRE(report.timeValue == L"2");
      REQUIRE(report.sentOrderType == TIMESTAMP);
      REQUIRE(report.receivedOrderType == TIMESTAMP);
    }

    // A subscription existing at generation is not a promise of delivery:
    // current state at the second grant suppresses this otherwise queued TSO
    // interaction under both switch settings.
    auto const secondRetraction = publisher->sendInteraction(
        interactionClass,
        ParameterHandleValueMap{},
        VariableLengthData(),
        rti1516_2025::HLAinteger64Time(3));
    REQUIRE(secondRetraction.isValid());
    REQUIRE_NOTHROW(receiver->unsubscribeInteractionClass(interactionClass));
    REQUIRE_NOTHROW(receiver->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(3)));
    REQUIRE_NOTHROW(publisher->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(3)));
    if (!immediate) {
      static_cast<void>(publisher->evokeCallback(0.0));
      static_cast<void>(receiver->evokeCallback(0.0));
    }
    REQUIRE(receiverReports.timestampedInteractionReports.size() ==
            (delaySubscriptionEvaluationEnabled ? 1U : 0U));
    REQUIRE(receiverReports.timeAdvanceGrantReports.size() == 2);

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
