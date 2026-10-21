#include "ieee1516_2025_federation_management_fixture_support.hpp"

namespace {

TEST_CASE(
    "Embedded asynchronous delivery does not bypass the TSO time gate",
    "[integration][development-profile][time-management]"
    "[asynchronous-delivery-tso-exclusion][callback-immediate]"
    "[rti.service.enable-asynchronous-delivery]"
    "[federate.callback.receive-interaction]") {
  auto runScenario = [](auto const callbackModel) {
    ReportingFederateAmbassador senderReports;
    ReportingFederateAmbassador receiverReports;
    auto sender = makeRti();
    auto receiver = makeRti();
    bool const immediate = callbackModel == rti1516_2025::HLA_IMMEDIATE;
    auto const federationName = nextFederationName();
    auto const fomModule =
        (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" /
         "data" / "parameter-handle-provider-fom.xml")
            .wstring();

    REQUIRE_NOTHROW(sender->connect(senderReports, callbackModel));
    REQUIRE_NOTHROW(receiver->connect(receiverReports, callbackModel));
    REQUIRE_NOTHROW(sender->createFederationExecution(
        federationName, fomModule, standard_hla::mom::integer64_time));
    REQUIRE_NOTHROW(sender->joinFederationExecution(
        L"async-tso-sender", L"publisher", federationName));
    REQUIRE_NOTHROW(receiver->joinFederationExecution(
        L"async-tso-receiver", L"subscriber", federationName));

    auto const tsoClass = sender->getInteractionClassHandle(
        fixture_hla::fom::parameter_fixture_child_interaction);
    auto const roClass = sender->getInteractionClassHandle(
        L"HLAinteractionRoot.UmbraParameterFixtureBase");
    REQUIRE(tsoClass.isValid());
    REQUIRE(roClass.isValid());
    REQUIRE_NOTHROW(sender->publishInteractionClass(tsoClass));
    REQUIRE_NOTHROW(sender->publishInteractionClass(roClass));
    REQUIRE_NOTHROW(sender->changeInteractionOrderType(tsoClass, TIMESTAMP));
    REQUIRE_NOTHROW(receiver->subscribeInteractionClass(tsoClass));
    REQUIRE_NOTHROW(receiver->subscribeInteractionClass(roClass));

    REQUIRE_NOTHROW(receiver->enableTimeConstrained());
    if (!immediate) {
      while (receiver->evokeCallback(0.0)) {
      }
    }
    REQUIRE(receiverReports.timeConstrainedEnabledReports.size() == 1);
    REQUIRE_NOTHROW(sender->enableTimeRegulation(
        rti1516_2025::HLAinteger64Interval(1)));
    if (!immediate) {
      while (sender->evokeCallback(0.0)) {
      }
    }
    REQUIRE(senderReports.timeRegulationEnabledReports.size() == 1);

    REQUIRE_NOTHROW(receiver->enableAsynchronousDelivery());
    auto const retraction = sender->sendInteraction(
        tsoClass, ParameterHandleValueMap{}, VariableLengthData{},
        rti1516_2025::HLAinteger64Time(5));
    REQUIRE(retraction.isValid());
    REQUIRE_NOTHROW(sender->sendInteraction(
        roClass, ParameterHandleValueMap{}, VariableLengthData{}));
    // The sender can advance its regulating bound without putting the
    // constrained receiver into Time Advancing.
    REQUIRE_NOTHROW(sender->timeAdvanceRequest(
        rti1516_2025::HLAinteger64Time(5)));
    if (!immediate) {
      while (sender->evokeCallback(0.0)) {
      }
      while (receiver->evokeCallback(0.0)) {
      }
    }

    // RO progress proves the callback route is live with async enabled;
    // §8.1.5 nevertheless excludes TSO while the receiver is Time Granted.
    REQUIRE(receiverReports.interactionReports.size() == 1);
    REQUIRE(receiverReports.interactionReports.front().interactionClass == roClass);
    REQUIRE(receiverReports.timestampedInteractionReports.empty());
    REQUIRE(receiverReports.timeAdvanceGrantReports.empty());

    REQUIRE_NOTHROW(receiver->timeAdvanceRequest(
        rti1516_2025::HLAinteger64Time(5)));
    if (!immediate) {
      for (int attempt = 0;
           attempt != 8 && receiverReports.timestampedInteractionReports.empty();
           ++attempt) {
        receiver->evokeCallback(0.25);
      }
    }
    REQUIRE(receiverReports.timestampedInteractionReports.size() == 1);
    auto const& delivered = receiverReports.timestampedInteractionReports.front();
    REQUIRE(delivered.interactionClass == tsoClass);
    REQUIRE(delivered.timeValue == L"5");
    REQUIRE(delivered.sentOrderType == TIMESTAMP);
    REQUIRE(delivered.receivedOrderType == TIMESTAMP);

    REQUIRE_NOTHROW(receiver->resignFederationExecution(NO_ACTION));
    REQUIRE_NOTHROW(sender->resignFederationExecution(NO_ACTION));
    REQUIRE_NOTHROW(sender->destroyFederationExecution(federationName));
    REQUIRE_NOTHROW(receiver->disconnect());
    REQUIRE_NOTHROW(sender->disconnect());
  };

  SECTION("HLA_EVOKED") {
    runScenario(HLA_EVOKED);
  }
  SECTION("HLA_IMMEDIATE") {
    runScenario(rti1516_2025::HLA_IMMEDIATE);
  }
}

TEST_CASE(
    "Embedded asynchronous eligibility can release all pending RO messages",
    "[integration][development-profile][time-management]"
    "[asynchronous-delivery-ro-backlog][callback-immediate]"
    "[rti.service.enable-asynchronous-delivery]"
    "[federate.callback.receive-interaction]") {
  auto runScenario = [](auto const callbackModel) {
    ReportingFederateAmbassador senderReports;
    ReportingFederateAmbassador receiverReports;
    auto sender = makeRti();
    auto receiver = makeRti();
    bool const immediate = callbackModel == rti1516_2025::HLA_IMMEDIATE;
    auto const federationName = nextFederationName();
    auto const fomModule =
        (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" /
         "data" / "parameter-handle-provider-fom.xml")
            .wstring();

    REQUIRE_NOTHROW(sender->connect(senderReports, callbackModel));
    REQUIRE_NOTHROW(receiver->connect(receiverReports, callbackModel));
    REQUIRE_NOTHROW(sender->createFederationExecution(
        federationName, fomModule, standard_hla::mom::integer64_time));
    REQUIRE_NOTHROW(sender->joinFederationExecution(
        L"ro-backlog-sender", L"publisher", federationName));
    REQUIRE_NOTHROW(receiver->joinFederationExecution(
        L"ro-backlog-receiver", L"subscriber", federationName));

    auto const roClass = sender->getInteractionClassHandle(
        fixture_hla::fom::parameter_fixture_child_interaction);
    REQUIRE(roClass.isValid());
    REQUIRE_NOTHROW(sender->publishInteractionClass(roClass));
    REQUIRE_NOTHROW(receiver->subscribeInteractionClass(roClass));
    REQUIRE_NOTHROW(receiver->enableTimeConstrained());
    if (!immediate) {
      REQUIRE_FALSE(receiver->evokeCallback(0.0));
    }
    REQUIRE(receiverReports.timeConstrainedEnabledReports.size() == 1);

    // In Time Granted with asynchronous delivery disabled, these three
    // distinct sent RO interactions remain unreceived by this subscriber.
    unsigned char const tags[] = {0xA1, 0xA2, 0xA3};
    for (auto const tag : tags) {
      REQUIRE_NOTHROW(sender->sendInteraction(
          roClass, ParameterHandleValueMap{}, VariableLengthData(&tag, 1)));
    }
    if (!immediate) {
      // A callback pump may report other pending callback work; the relevant
      // oracle is that none of these RO interactions has been received yet.
      receiver->evokeCallback(0.0);
    }
    REQUIRE(receiverReports.interactionReports.empty());

    // After this eligibility transition, the only receiver calls are Evoke
    // for HLA_EVOKED. HLA_IMMEDIATE receives the backlog from the transition.
    REQUIRE_NOTHROW(receiver->enableAsynchronousDelivery());
    if (!immediate) {
      for (int attempt = 0;
           attempt != 8 && receiverReports.interactionReports.size() < 3;
           ++attempt) {
        receiver->evokeCallback(0.25);
      }
    }
    REQUIRE(receiverReports.interactionReports.size() == 3);
    bool receivedA1 = false;
    bool receivedA2 = false;
    bool receivedA3 = false;
    for (auto const& report : receiverReports.interactionReports) {
      REQUIRE(report.interactionClass == roClass);
      REQUIRE(report.userSuppliedTag.size() == 1);
      auto const receivedTag =
          static_cast<unsigned char const*>(report.userSuppliedTag.data())[0];
      REQUIRE((receivedTag == 0xA1 || receivedTag == 0xA2 || receivedTag == 0xA3));
      if (receivedTag == 0xA1) {
        REQUIRE_FALSE(receivedA1);
        receivedA1 = true;
      } else if (receivedTag == 0xA2) {
        REQUIRE_FALSE(receivedA2);
        receivedA2 = true;
      } else {
        REQUIRE_FALSE(receivedA3);
        receivedA3 = true;
      }
    }
    REQUIRE(receivedA1);
    REQUIRE(receivedA2);
    REQUIRE(receivedA3);
    REQUIRE(receiverReports.timestampedInteractionReports.empty());
    REQUIRE(receiverReports.timeAdvanceGrantReports.empty());

    REQUIRE_NOTHROW(receiver->resignFederationExecution(NO_ACTION));
    REQUIRE_NOTHROW(sender->resignFederationExecution(NO_ACTION));
    REQUIRE_NOTHROW(sender->destroyFederationExecution(federationName));
    REQUIRE_NOTHROW(receiver->disconnect());
    REQUIRE_NOTHROW(sender->disconnect());
  };

  SECTION("HLA_EVOKED") {
    runScenario(HLA_EVOKED);
  }
  SECTION("HLA_IMMEDIATE") {
    runScenario(rti1516_2025::HLA_IMMEDIATE);
  }
}

}  // namespace
