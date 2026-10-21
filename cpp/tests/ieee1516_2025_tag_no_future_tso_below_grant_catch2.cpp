#include "ieee1516_2025_federation_management_fixture_support.hpp"

namespace {

void drainCallbacks(rti1516_2025::RTIambassador& rti) {
  for (int pass = 0; pass != 128; ++pass) {
    if (!rti.evokeCallback(0.0)) {
      break;
    }
  }
}

}  // namespace

TEST_CASE(
    "Embedded time advance grant excludes later TSO below its grant time",
    "[integration][development-profile][time-management][tso]"
    "[tag-no-future-tso-below-grant]"
    "[rti.service.send-interaction][rti.service.time-advance-request]"
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
      L"tag-no-future-tso-sender", L"publisher", federationName));
  REQUIRE_NOTHROW(receiver->joinFederationExecution(
      L"tag-no-future-tso-receiver", L"subscriber", federationName));

  auto const interactionClass = sender->getInteractionClassHandle(
      fixture_hla::fom::parameter_fixture_child_interaction);
  auto const parameter = sender->getParameterHandle(
      interactionClass, fixture_hla::fixture::identifier);
  REQUIRE(interactionClass.isValid());
  REQUIRE(parameter.isValid());
  REQUIRE_NOTHROW(sender->publishInteractionClass(interactionClass));
  REQUIRE_NOTHROW(sender->changeInteractionOrderType(interactionClass, TIMESTAMP));
  REQUIRE_NOTHROW(receiver->subscribeInteractionClass(interactionClass));
  REQUIRE_NOTHROW(receiver->enableTimeConstrained());
  drainCallbacks(*receiver);
  REQUIRE(receiverReports.timeConstrainedEnabledReports.size() == 1U);
  REQUIRE_NOTHROW(sender->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(5)));
  drainCallbacks(*sender);
  REQUIRE(senderReports.timeRegulationEnabledReports.size() == 1U);

  unsigned char const tagBytes[] = {0x47U, 0x52U};
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));
  auto sendAt = [&](std::int64_t timestamp, unsigned char payloadByte) {
    ParameterHandleValueMap values;
    values.emplace(parameter, VariableLengthData(&payloadByte, sizeof(payloadByte)));
    return sender->sendInteraction(
        interactionClass,
        values,
        tag,
        rti1516_2025::HLAinteger64Time(timestamp));
  };

  auto const earlierThanGrant = sendAt(6, 0x36U);
  REQUIRE(earlierThanGrant.isValid());
  REQUIRE(receiverReports.timestampedInteractionReports.empty());

  // The queued TSO(6) is delivered before TAG(6). Advancing the producer to
  // time 2 with lookahead 5 establishes a legal-send floor of 7, so no later
  // send from this producer can introduce another TSO below the receiver's
  // grant time.
  REQUIRE_NOTHROW(receiver->timeAdvanceRequest(
      rti1516_2025::HLAinteger64Time(6)));
  REQUIRE_NOTHROW(sender->timeAdvanceRequest(
      rti1516_2025::HLAinteger64Time(2)));
  drainCallbacks(*sender);
  REQUIRE(senderReports.timeAdvanceGrantReports.size() == 1U);
  REQUIRE(senderReports.timeAdvanceGrantReports.front().value == L"2");
  drainCallbacks(*receiver);
  REQUIRE(receiverReports.timestampedInteractionReports.size() == 1U);
  REQUIRE(receiverReports.timestampedInteractionReports.front().timeValue == L"6");
  REQUIRE(receiverReports.timeAdvanceGrantReports.size() == 1U);
  REQUIRE(receiverReports.timeAdvanceGrantReports.front().value == L"6");
  REQUIRE(receiverReports.callbackOrder ==
          std::vector<std::string>{"interaction", "grant"});

  REQUIRE_THROWS_AS(sendAt(6, 0x3FU), rti1516_2025::InvalidLogicalTime);
  REQUIRE(receiverReports.timestampedInteractionReports.size() == 1U);
  REQUIRE_NOTHROW(sendAt(7, 0x37U));
  REQUIRE_NOTHROW(receiver->timeAdvanceRequest(
      rti1516_2025::HLAinteger64Time(7)));
  REQUIRE_NOTHROW(sender->timeAdvanceRequest(
      rti1516_2025::HLAinteger64Time(3)));
  drainCallbacks(*sender);
  REQUIRE(senderReports.timeAdvanceGrantReports.size() == 2U);
  REQUIRE(senderReports.timeAdvanceGrantReports.back().value == L"3");
  drainCallbacks(*receiver);
  REQUIRE(receiverReports.timestampedInteractionReports.size() == 2U);
  REQUIRE(receiverReports.timestampedInteractionReports.back().timeValue == L"7");
  REQUIRE(receiverReports.timeAdvanceGrantReports.size() == 2U);
  REQUIRE(receiverReports.timeAdvanceGrantReports.back().value == L"7");
  REQUIRE(receiverReports.callbackOrder ==
          std::vector<std::string>{"interaction", "grant", "interaction", "grant"});

  REQUIRE_NOTHROW(receiver->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(sender->resignFederationExecution(
      rti1516_2025::CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(sender->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(receiver->disconnect());
  REQUIRE_NOTHROW(sender->disconnect());
}
