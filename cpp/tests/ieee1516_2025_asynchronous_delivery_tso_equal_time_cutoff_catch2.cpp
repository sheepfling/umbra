#include "ieee1516_2025_federation_management_fixture_support.hpp"

namespace {

TEST_CASE(
    "Embedded asynchronous delivery observes the TAR equal-time TSO cutoff",
    "[integration][development-profile][time-management]"
    "[asynchronous-delivery-tso-equal-time-cutoff]"
    "[rti.service.enable-asynchronous-delivery]"
    "[rti.service.time-advance-request]"
    "[federate.callback.receive-interaction]"
    "[federate.callback.time-advance-grant]") {
  ReportingFederateAmbassador senderReports;
  ReportingFederateAmbassador receiverReports;
  auto sender = makeRti();
  auto receiver = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule =
      (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" /
       "data" / "parameter-handle-provider-fom.xml")
          .wstring();

  REQUIRE_NOTHROW(sender->connect(senderReports, HLA_EVOKED));
  REQUIRE_NOTHROW(receiver->connect(receiverReports, HLA_EVOKED));
  REQUIRE_NOTHROW(sender->createFederationExecution(
      federationName, fomModule, standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(sender->joinFederationExecution(
      L"async-cutoff-sender", L"publisher", federationName));
  REQUIRE_NOTHROW(receiver->joinFederationExecution(
      L"async-cutoff-receiver", L"subscriber", federationName));

  auto const interactionClass = sender->getInteractionClassHandle(
      fixture_hla::fom::parameter_fixture_child_interaction);
  REQUIRE(interactionClass.isValid());
  REQUIRE_NOTHROW(sender->publishInteractionClass(interactionClass));
  REQUIRE_NOTHROW(sender->changeInteractionOrderType(interactionClass, TIMESTAMP));
  REQUIRE_NOTHROW(receiver->subscribeInteractionClass(interactionClass));
  REQUIRE_NOTHROW(receiver->enableTimeConstrained());
  while (receiver->evokeCallback(0.0)) {
  }
  REQUIRE(receiverReports.timeConstrainedEnabledReports.size() == 1U);
  REQUIRE_NOTHROW(sender->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(1)));
  while (sender->evokeCallback(0.0)) {
  }
  REQUIRE(senderReports.timeRegulationEnabledReports.size() == 1U);
  REQUIRE_NOTHROW(receiver->enableAsynchronousDelivery());

  auto const sendAt = [&](std::int64_t const time) {
    REQUIRE_NOTHROW(sender->sendInteraction(
        interactionClass,
        ParameterHandleValueMap{},
        VariableLengthData{},
        rti1516_2025::HLAinteger64Time(time)));
  };
  sendAt(5);
  sendAt(5);
  sendAt(6);
  REQUIRE_NOTHROW(sender->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(6)));
  while (sender->evokeCallback(0.0)) {
  }
  while (receiver->evokeCallback(0.0)) {
  }
  REQUIRE(receiverReports.timestampedInteractionReports.empty());
  REQUIRE(receiverReports.timeAdvanceGrantReports.empty());

  // The supplied logical time is inclusive: both timestamp-5 messages must
  // be drained for TAR(5), and the timestamp-6 control must remain pending.
  REQUIRE_NOTHROW(receiver->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(5)));
  for (int attempt = 0;
       attempt != 12 && receiverReports.timeAdvanceGrantReports.empty();
       ++attempt) {
    receiver->evokeCallback(0.25);
  }
  REQUIRE(receiverReports.timeAdvanceGrantReports.size() == 1U);
  REQUIRE(receiverReports.timeAdvanceGrantReports.front().value == L"5");
  REQUIRE(receiverReports.timestampedInteractionReports.size() == 2U);
  REQUIRE(receiverReports.timestampedInteractionReports[0].timeValue == L"5");
  REQUIRE(receiverReports.timestampedInteractionReports[1].timeValue == L"5");

  // Once TAR(5)'s grant has been reported, no additional TSO callback at or
  // below that supplied logical time may appear on subsequent callback pumps.
  for (int attempt = 0; attempt != 8; ++attempt) {
    receiver->evokeCallback(0.0);
  }
  REQUIRE(receiverReports.timeAdvanceGrantReports.size() == 1U);
  REQUIRE(receiverReports.timestampedInteractionReports.size() == 2U);
  REQUIRE(receiverReports.timestampedInteractionReports[0].timeValue == L"5");
  REQUIRE(receiverReports.timestampedInteractionReports[1].timeValue == L"5");

  REQUIRE_NOTHROW(receiver->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(6)));
  for (int attempt = 0;
       attempt != 12 && receiverReports.timeAdvanceGrantReports.size() != 2U;
       ++attempt) {
    receiver->evokeCallback(0.25);
  }
  REQUIRE(receiverReports.timeAdvanceGrantReports.size() == 2U);
  REQUIRE(receiverReports.timeAdvanceGrantReports.back().value == L"6");
  REQUIRE(receiverReports.timestampedInteractionReports.size() == 3U);
  REQUIRE(receiverReports.timestampedInteractionReports.back().timeValue == L"6");

  REQUIRE_NOTHROW(receiver->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(sender->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(sender->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(receiver->disconnect());
  REQUIRE_NOTHROW(sender->disconnect());
}

}  // namespace
