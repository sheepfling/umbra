#include "ieee1516_2025_federation_management_fixture_support.hpp"

namespace {

TEST_CASE(
    "Embedded asynchronous delivery gates receive-order callbacks by temporal state",
    "[integration][development-profile][time-management][asynchronous-delivery]"
    "[rti.service.enable-asynchronous-delivery][rti.service.disable-asynchronous-delivery]"
    "[callback-immediate]"
    "[rti.service.time-advance-request][federate.callback.receive-interaction]") {
  auto runScenario = [](auto const callbackModel) {
    ReportingFederateAmbassador publisherReports;
    ReportingFederateAmbassador receiverReports;
    auto publisher = makeRti();
    auto receiver = makeRti();
    bool const immediate = callbackModel == rti1516_2025::HLA_IMMEDIATE;
    auto const federationName = nextFederationName();
    auto const fomModule = (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
                            "cpp" /
                            "tests" /
                            "data" /
                            "parameter-handle-provider-fom.xml")
                               .wstring();

    REQUIRE_THROWS_AS(
        receiver->enableAsynchronousDelivery(),
        rti1516_2025::NotConnected);
    REQUIRE_THROWS_AS(
        receiver->disableAsynchronousDelivery(),
        rti1516_2025::NotConnected);
    REQUIRE_NOTHROW(publisher->connect(publisherReports, callbackModel));
    REQUIRE_NOTHROW(receiver->connect(receiverReports, callbackModel));
    REQUIRE_THROWS_AS(
        receiver->enableAsynchronousDelivery(),
        rti1516_2025::FederateNotExecutionMember);
    REQUIRE_THROWS_AS(
        receiver->disableAsynchronousDelivery(),
        rti1516_2025::FederateNotExecutionMember);

    REQUIRE_NOTHROW(
        publisher->createFederationExecution(
            federationName,
            fomModule,
            standard_hla::mom::integer64_time));
    REQUIRE_NOTHROW(publisher->joinFederationExecution(
        L"async-publisher",
        L"publisher",
        federationName));
    REQUIRE_NOTHROW(receiver->joinFederationExecution(
        L"async-receiver",
        L"subscriber",
        federationName));

    auto const interactionClass = publisher->getInteractionClassHandle(
        fixture_hla::fom::parameter_fixture_child_interaction);
    REQUIRE_NOTHROW(publisher->publishInteractionClass(interactionClass));
    REQUIRE_NOTHROW(receiver->subscribeInteractionClass(interactionClass));

    REQUIRE_NOTHROW(receiver->enableTimeConstrained());
    if (!immediate) {
      REQUIRE_FALSE(receiver->evokeCallback(0.0));
    }
    REQUIRE(receiverReports.timeConstrainedEnabledReports.size() == 1);

    // The default switch is disabled. A receive-order message submitted while
    // the constrained federate is Time Granted is retained, not discarded.
    REQUIRE_NOTHROW(
        publisher->sendInteraction(interactionClass, ParameterHandleValueMap{}, VariableLengthData()));
    if (!immediate) {
      REQUIRE_FALSE(receiver->evokeCallback(0.0));
    }
    REQUIRE(receiverReports.interactionReports.empty());

    // Under HLA_IMMEDIATE this service flushes the deferred callback directly;
    // under HLA_EVOKED it makes the same callback available to Evoke.
    REQUIRE_NOTHROW(receiver->enableAsynchronousDelivery());
    REQUIRE_THROWS_AS(
        receiver->enableAsynchronousDelivery(),
        rti1516_2025::AsynchronousDeliveryAlreadyEnabled);
    if (!immediate) {
      REQUIRE_FALSE(receiver->evokeCallback(0.0));
    }
    REQUIRE(receiverReports.interactionReports.size() == 1);

    REQUIRE_NOTHROW(receiver->disableAsynchronousDelivery());
    REQUIRE_THROWS_AS(
        receiver->disableAsynchronousDelivery(),
        rti1516_2025::AsynchronousDeliveryAlreadyDisabled);

    REQUIRE_NOTHROW(
        publisher->sendInteraction(interactionClass, ParameterHandleValueMap{}, VariableLengthData()));
    if (!immediate) {
      REQUIRE_FALSE(receiver->evokeCallback(0.0));
    }
    REQUIRE(receiverReports.interactionReports.size() == 1);

    // Entering Time Advancing makes the retained RO message eligible. The
    // direct model observes it during this service call; the evoked model
    // observes it during the following Evoke. Neither path needs a grant from
    // another federate in this isolated scenario.
    REQUIRE_NOTHROW(receiver->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(1)));
    if (!immediate) {
      REQUIRE_FALSE(receiver->evokeCallback(0.0));
    }
    REQUIRE(receiverReports.interactionReports.size() == 2);
    REQUIRE(receiverReports.timeAdvanceGrantReports.empty());

    REQUIRE_NOTHROW(receiver->resignFederationExecution(NO_ACTION));
    REQUIRE_NOTHROW(publisher->resignFederationExecution(NO_ACTION));
    REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
    REQUIRE_NOTHROW(receiver->disconnect());
    REQUIRE_NOTHROW(publisher->disconnect());
  };

  SECTION("HLA_EVOKED") {
    runScenario(HLA_EVOKED);
  }
  SECTION("HLA_IMMEDIATE") {
    runScenario(rti1516_2025::HLA_IMMEDIATE);
  }
}
}  // namespace
