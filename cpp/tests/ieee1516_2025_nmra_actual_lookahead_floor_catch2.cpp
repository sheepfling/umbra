#include "ieee1516_2025_federation_management_fixture_support.hpp"

namespace {

void drainCallbacks(rti1516_2025::RTIambassador& rti) {
  for (int pass = 0; pass != 128; ++pass) {
    if (!rti.evokeCallback(0.0)) {
      break;
    }
  }
}

TEST_CASE(
    "Embedded NMRA grant enforces the actual-lookahead TSO floor",
    "[integration][development-profile][time-management]"
    "[nmra-actual-lookahead-tso-floor]"
    "[rti.service.next-message-request-available]"
    "[rti.service.send-interaction]"
    "[federate.callback.time-advance-grant]") {
  ReportingFederateAmbassador regulatingReports;
  ReportingFederateAmbassador peerReports;
  auto regulating = makeRti();
  auto peer = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule =
      (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" /
       "data" / "parameter-handle-provider-fom.xml")
          .wstring();

  REQUIRE_NOTHROW(regulating->connect(regulatingReports, HLA_EVOKED));
  REQUIRE_NOTHROW(peer->connect(peerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(regulating->createFederationExecution(
      federationName, fomModule, standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(regulating->joinFederationExecution(
      L"nmra-lookahead-floor-regulating", L"publisher", federationName));
  REQUIRE_NOTHROW(peer->joinFederationExecution(
      L"nmra-lookahead-floor-peer", L"publisher", federationName));

  auto const interactionClass = regulating->getInteractionClassHandle(
      fixture_hla::fom::parameter_fixture_child_interaction);
  auto const peerInteractionClass = peer->getInteractionClassHandle(
      fixture_hla::fom::parameter_fixture_child_interaction);
  REQUIRE(interactionClass.isValid());
  REQUIRE(peerInteractionClass.isValid());
  REQUIRE_NOTHROW(regulating->publishInteractionClass(interactionClass));
  REQUIRE_NOTHROW(peer->publishInteractionClass(peerInteractionClass));
  REQUIRE_NOTHROW(regulating->changeInteractionOrderType(interactionClass, TIMESTAMP));
  REQUIRE_NOTHROW(peer->changeInteractionOrderType(peerInteractionClass, TIMESTAMP));
  REQUIRE_NOTHROW(regulating->subscribeInteractionClass(interactionClass));

  REQUIRE_NOTHROW(regulating->enableTimeConstrained());
  drainCallbacks(*regulating);
  REQUIRE(regulatingReports.timeConstrainedEnabledReports.size() == 1U);
  REQUIRE_NOTHROW(regulating->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(1)));
  drainCallbacks(*regulating);
  REQUIRE(regulatingReports.timeRegulationEnabledReports.size() == 1U);
  REQUIRE_NOTHROW(peer->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(1)));
  drainCallbacks(*peer);
  REQUIRE(peerReports.timeRegulationEnabledReports.size() == 1U);

  auto const peerParameter = peer->getParameterHandle(
      peerInteractionClass, fixture_hla::fixture::identifier);
  REQUIRE(peerParameter.isValid());
  unsigned char const payload[] = {0x4EU, 0x4DU, 0x52U, 0x41U};
  unsigned char const tagBytes[] = {0x46U, 0x4CU, 0x4FU, 0x4FU, 0x52U};
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));
  ParameterHandleValueMap incomingValues;
  incomingValues.emplace(peerParameter, VariableLengthData(payload, sizeof(payload)));
  REQUIRE(peer->sendInteraction(
      peerInteractionClass,
      incomingValues,
      tag,
      rti1516_2025::HLAinteger64Time(5)).isValid());

  REQUIRE_NOTHROW(regulating->nextMessageRequestAvailable(
      rti1516_2025::HLAinteger64Time(10)));
  REQUIRE(regulatingReports.timeAdvanceGrantReports.empty());
  REQUIRE_NOTHROW(peer->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(4)));
  drainCallbacks(*peer);
  drainCallbacks(*regulating);
  REQUIRE(regulatingReports.timestampedInteractionReports.size() == 1U);
  REQUIRE(regulatingReports.timeAdvanceGrantReports.size() == 1U);
  REQUIRE(regulatingReports.timeAdvanceGrantReports.front().value == L"5");

  // NMRA grants at logical time 5 although 10 was requested. With actual
  // lookahead 1, timestamp 5 is below the required floor of 6 and must fail;
  // timestamp 6 is the inclusive floor and remains admissible.
  ParameterHandleValueMap outgoingValues;
  outgoingValues.emplace(peerParameter, VariableLengthData(payload, sizeof(payload)));
  REQUIRE_THROWS_AS(
      regulating->sendInteraction(
          interactionClass,
          outgoingValues,
          tag,
          rti1516_2025::HLAinteger64Time(5)),
      rti1516_2025::InvalidLogicalTime);
  REQUIRE_NOTHROW(regulating->sendInteraction(
      interactionClass,
      outgoingValues,
      tag,
      rti1516_2025::HLAinteger64Time(6)));

  REQUIRE_NOTHROW(regulating->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(peer->resignFederationExecution(rti1516_2025::CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(regulating->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(regulating->disconnect());
  REQUIRE_NOTHROW(peer->disconnect());
}

}  // namespace
