#include "ieee1516_2025_federation_management_fixture_support.hpp"
#include "hla_test_names.hpp"

namespace {

void runScenario(rti1516_2025::CallbackModel const callbackModel) {
  using rti1516_2025::HLA_EVOKED;
  using rti1516_2025::NO_ACTION;
  using rti1516_2025::RTIambassador;

  ReportingFederateAmbassador senderReports;
  ReportingFederateAmbassador receiverReports;
  auto sender = makeRti();
  auto receiver = makeRti();
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
      L"async-reenable-sender", L"publisher", federationName));
  REQUIRE_NOTHROW(receiver->joinFederationExecution(
      L"async-reenable-receiver", L"subscriber", federationName));

  auto drain = [callbackModel](RTIambassador& rti) {
    if (callbackModel == HLA_EVOKED) {
      while (rti.evokeCallback(0.0)) {
      }
    }
  };
  auto pumpUntil = [callbackModel](RTIambassador& rti, auto const& predicate) {
    for (int attempt = 0; attempt != 8 && !predicate(); ++attempt) {
      if (callbackModel == HLA_EVOKED) {
        rti.evokeCallback(0.25);
      }
    }
  };

  auto const tsoClass = sender->getInteractionClassHandle(
      fixture_hla::fom::parameter_fixture_child_interaction);
  auto const roClass = sender->getInteractionClassHandle(
      umbra::test::hla::wide::fom::parameter_fixture_base);
  REQUIRE(tsoClass.isValid());
  REQUIRE(roClass.isValid());
  REQUIRE_NOTHROW(sender->publishInteractionClass(tsoClass));
  REQUIRE_NOTHROW(sender->publishInteractionClass(roClass));
  REQUIRE_NOTHROW(sender->changeInteractionOrderType(tsoClass, TIMESTAMP));
  REQUIRE_NOTHROW(receiver->subscribeInteractionClass(tsoClass));
  REQUIRE_NOTHROW(receiver->subscribeInteractionClass(roClass));
  REQUIRE_NOTHROW(receiver->enableTimeConstrained());
  drain(*receiver);
  REQUIRE(receiverReports.timeConstrainedEnabledReports.size() == 1U);
  REQUIRE_NOTHROW(sender->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(1)));
  drain(*sender);
  REQUIRE(senderReports.timeRegulationEnabledReports.size() == 1U);

  REQUIRE_NOTHROW(receiver->enableAsynchronousDelivery());
  REQUIRE_NOTHROW(receiver->disableAsynchronousDelivery());
  REQUIRE_NOTHROW(sender->sendInteraction(
      tsoClass,
      rti1516_2025::ParameterHandleValueMap{},
      rti1516_2025::VariableLengthData{},
      rti1516_2025::HLAinteger64Time(5)));
  REQUIRE_NOTHROW(sender->sendInteraction(
      roClass,
      rti1516_2025::ParameterHandleValueMap{},
      rti1516_2025::VariableLengthData{}));
  REQUIRE_NOTHROW(sender->timeAdvanceRequest(
      rti1516_2025::HLAinteger64Time(5)));
  drain(*sender);
  drain(*receiver);
  REQUIRE(receiverReports.interactionReports.empty());
  REQUIRE(receiverReports.timestampedInteractionReports.empty());

  // Re-enabling makes the pending RO callback eligible without putting this
  // receiver into Time Advancing; it must not release the queued TSO callback.
  REQUIRE_NOTHROW(receiver->enableAsynchronousDelivery());
  pumpUntil(*receiver, [&] { return !receiverReports.interactionReports.empty(); });
  REQUIRE(receiverReports.interactionReports.size() == 1U);
  REQUIRE(receiverReports.interactionReports.front().interactionClass == roClass);
  REQUIRE(receiverReports.timestampedInteractionReports.empty());
  REQUIRE(receiverReports.timeAdvanceGrantReports.empty());

  REQUIRE_NOTHROW(receiver->timeAdvanceRequest(
      rti1516_2025::HLAinteger64Time(5)));
  pumpUntil(*receiver, [&] {
    return !receiverReports.timestampedInteractionReports.empty();
  });
  REQUIRE(receiverReports.timestampedInteractionReports.size() == 1U);
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
}

void runTimeAdvancingScenario(rti1516_2025::CallbackModel const callbackModel,
                             bool const disableWhileAdvancing = false) {
  using rti1516_2025::HLA_EVOKED;
  using rti1516_2025::NO_ACTION;
  using rti1516_2025::RTIambassador;

  ReportingFederateAmbassador senderReports;
  ReportingFederateAmbassador receiverReports;
  auto sender = makeRti();
  auto receiver = makeRti();
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
      L"async-time-advancing-sender", L"publisher", federationName));
  REQUIRE_NOTHROW(receiver->joinFederationExecution(
      L"async-time-advancing-receiver", L"subscriber", federationName));

  auto drain = [callbackModel](RTIambassador& rti) {
    if (callbackModel == HLA_EVOKED) {
      while (rti.evokeCallback(0.0)) {
      }
    }
  };
  auto pumpUntil = [callbackModel](RTIambassador& rti, auto const& predicate) {
    for (int attempt = 0; attempt != 8 && !predicate(); ++attempt) {
      if (callbackModel == HLA_EVOKED) {
        rti.evokeCallback(0.25);
      }
    }
  };

  auto const tsoClass = sender->getInteractionClassHandle(
      fixture_hla::fom::parameter_fixture_child_interaction);
  auto const roClass = sender->getInteractionClassHandle(
      umbra::test::hla::wide::fom::parameter_fixture_base);
  REQUIRE(tsoClass.isValid());
  REQUIRE(roClass.isValid());
  REQUIRE_NOTHROW(sender->publishInteractionClass(tsoClass));
  REQUIRE_NOTHROW(sender->publishInteractionClass(roClass));
  REQUIRE_NOTHROW(sender->changeInteractionOrderType(tsoClass, TIMESTAMP));
  REQUIRE_NOTHROW(receiver->subscribeInteractionClass(tsoClass));
  REQUIRE_NOTHROW(receiver->subscribeInteractionClass(roClass));
  REQUIRE_NOTHROW(receiver->enableTimeConstrained());
  drain(*receiver);
  REQUIRE(receiverReports.timeConstrainedEnabledReports.size() == 1U);
  REQUIRE_NOTHROW(sender->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(1)));
  drain(*sender);
  REQUIRE(senderReports.timeRegulationEnabledReports.size() == 1U);

  // Keep the receiver in Time Advancing while the sender's lookahead prevents
  // an immediate grant. Exercise the corresponding enable or disable rule in
  // that state, then prove a newly received RO callback is eligible there.
  if (disableWhileAdvancing) {
    REQUIRE_NOTHROW(receiver->enableAsynchronousDelivery());
  }
  REQUIRE_NOTHROW(receiver->timeAdvanceRequest(
      rti1516_2025::HLAinteger64Time(6)));
  REQUIRE(receiverReports.timeAdvanceGrantReports.empty());
  if (disableWhileAdvancing) {
    REQUIRE_NOTHROW(receiver->disableAsynchronousDelivery());
  } else {
    REQUIRE_NOTHROW(receiver->enableAsynchronousDelivery());
  }
  REQUIRE_NOTHROW(sender->sendInteraction(
      roClass,
      rti1516_2025::ParameterHandleValueMap{},
      rti1516_2025::VariableLengthData{}));
  pumpUntil(*receiver, [&] { return !receiverReports.interactionReports.empty(); });
  REQUIRE(receiverReports.interactionReports.size() == 1U);
  REQUIRE(receiverReports.interactionReports.front().interactionClass == roClass);
  REQUIRE(receiverReports.timeAdvanceGrantReports.empty());
  REQUIRE(receiverReports.timestampedInteractionReports.empty());

  // Advance the sender's lower bound before sending the timestamped event at
  // the receiver's requested time. This resolves the pending TAR; it is not
  // an assertion about asynchronous TSO delivery.
  REQUIRE_NOTHROW(sender->timeAdvanceRequest(
      rti1516_2025::HLAinteger64Time(5)));
  pumpUntil(*sender, [&] { return !senderReports.timeAdvanceGrantReports.empty(); });
  REQUIRE(senderReports.timeAdvanceGrantReports.size() == 1U);
  REQUIRE_NOTHROW(sender->sendInteraction(
      tsoClass,
      rti1516_2025::ParameterHandleValueMap{},
      rti1516_2025::VariableLengthData{},
      rti1516_2025::HLAinteger64Time(6)));
  pumpUntil(*receiver, [&] {
    return !receiverReports.timeAdvanceGrantReports.empty();
  });
  REQUIRE(receiverReports.timeAdvanceGrantReports.size() == 1U);
  REQUIRE(receiverReports.timestampedInteractionReports.size() == 1U);
  auto const& delivered = receiverReports.timestampedInteractionReports.front();
  REQUIRE(delivered.interactionClass == tsoClass);
  REQUIRE(delivered.timeValue == L"6");

  REQUIRE_NOTHROW(receiver->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(sender->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(sender->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(receiver->disconnect());
  REQUIRE_NOTHROW(sender->disconnect());
}

}  // namespace

TEST_CASE(
    "Embedded asynchronous re-enable releases RO but keeps TSO gated until advance",
    "[integration][development-profile][time-management]"
    "[asynchronous-delivery-reenable-tso-gate]"
    "[rti.service.enable-asynchronous-delivery]"
    "[rti.service.disable-asynchronous-delivery]"
    "[federate.callback.receive-interaction]") {
  SECTION("HLA_EVOKED") {
    runScenario(rti1516_2025::HLA_EVOKED);
  }
  SECTION("HLA_IMMEDIATE") {
    runScenario(rti1516_2025::HLA_IMMEDIATE);
  }
}

TEST_CASE(
    "Embedded asynchronous delivery enables RO while Time Advancing",
    "[integration][development-profile][time-management]"
    "[asynchronous-delivery-time-advancing-ro]"
    "[rti.service.enable-asynchronous-delivery]"
    "[federate.callback.receive-interaction]") {
  SECTION("HLA_EVOKED") {
    runTimeAdvancingScenario(rti1516_2025::HLA_EVOKED);
  }
  SECTION("HLA_IMMEDIATE") {
    runTimeAdvancingScenario(rti1516_2025::HLA_IMMEDIATE);
  }
}

TEST_CASE(
    "Embedded asynchronous delivery keeps RO eligible after disable while Time Advancing",
    "[integration][development-profile][time-management]"
    "[asynchronous-delivery-time-advancing-ro]"
    "[rti.service.disable-asynchronous-delivery]"
    "[federate.callback.receive-interaction]") {
  SECTION("HLA_EVOKED") {
    runTimeAdvancingScenario(rti1516_2025::HLA_EVOKED, true);
  }
  SECTION("HLA_IMMEDIATE") {
    runTimeAdvancingScenario(rti1516_2025::HLA_IMMEDIATE, true);
  }
}
