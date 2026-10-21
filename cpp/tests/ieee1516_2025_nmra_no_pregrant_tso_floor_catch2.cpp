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
    "Embedded NMRA without pre-grant TSO enforces request-time lookahead floor",
    "[integration][development-profile][time-management]"
    "[nmra-no-pregrant-tso-floor]"
    "[rti.service.next-message-request-available]"
    "[rti.service.send-interaction]"
    "[federate.callback.time-advance-grant]") {
  ReportingFederateAmbassador requesterReports;
  ReportingFederateAmbassador peerReports;
  auto requester = makeRti();
  auto peer = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule =
      (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" /
       "data" / "parameter-handle-provider-fom.xml")
          .wstring();

  REQUIRE_NOTHROW(requester->connect(requesterReports, HLA_EVOKED));
  REQUIRE_NOTHROW(peer->connect(peerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(requester->createFederationExecution(
      federationName, fomModule, standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(requester->joinFederationExecution(
      L"nmra-no-pregrant-requester", L"publisher", federationName));
  REQUIRE_NOTHROW(peer->joinFederationExecution(
      L"nmra-no-pregrant-peer", L"publisher", federationName));

  auto const interactionClass = requester->getInteractionClassHandle(
      fixture_hla::fom::parameter_fixture_child_interaction);
  auto const peerInteractionClass = peer->getInteractionClassHandle(
      fixture_hla::fom::parameter_fixture_child_interaction);
  REQUIRE(interactionClass.isValid());
  REQUIRE(peerInteractionClass.isValid());
  REQUIRE_NOTHROW(requester->publishInteractionClass(interactionClass));
  REQUIRE_NOTHROW(peer->publishInteractionClass(peerInteractionClass));
  REQUIRE_NOTHROW(requester->changeInteractionOrderType(interactionClass, TIMESTAMP));
  REQUIRE_NOTHROW(peer->changeInteractionOrderType(peerInteractionClass, TIMESTAMP));
  REQUIRE_NOTHROW(requester->enableTimeConstrained());
  drainCallbacks(*requester);
  REQUIRE(requesterReports.timeConstrainedEnabledReports.size() == 1U);
  REQUIRE_NOTHROW(requester->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(1)));
  drainCallbacks(*requester);
  REQUIRE(requesterReports.timeRegulationEnabledReports.size() == 1U);
  REQUIRE_NOTHROW(peer->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(1)));
  drainCallbacks(*peer);
  REQUIRE(peerReports.timeRegulationEnabledReports.size() == 1U);

  // Move the other regulating federate beyond the request without sending
  // any TSO, so the NMRA completes at its supplied time with an empty input
  // set and the no-TSO-before-grant branch is unambiguous.
  REQUIRE_NOTHROW(peer->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(12)));
  drainCallbacks(*peer);
  REQUIRE(peerReports.timeAdvanceGrantReports.size() == 1U);
  REQUIRE(peerReports.timeAdvanceGrantReports.front().value == L"12");
  REQUIRE(requesterReports.timestampedInteractionReports.empty());

  REQUIRE_NOTHROW(requester->nextMessageRequestAvailable(
      rti1516_2025::HLAinteger64Time(10)));
  drainCallbacks(*requester);
  REQUIRE(requesterReports.timestampedInteractionReports.empty());
  REQUIRE(requesterReports.timeAdvanceGrantReports.size() == 1U);
  auto const grantTime = requesterReports.timeAdvanceGrantReports.front().value;

  ParameterHandleValueMap outgoingValues;
  unsigned char const payload[] = {0x4EU, 0x4DU, 0x52U, 0x41U};
  unsigned char const tagBytes[] = {0x4Eu, 0x4F, 0x50, 0x52, 0x45};
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));
  auto const parameter = requester->getParameterHandle(
      interactionClass, fixture_hla::fixture::identifier);
  REQUIRE(parameter.isValid());
  outgoingValues.emplace(parameter, VariableLengthData(payload, sizeof(payload)));

  // With no TSO received before the grant, the normative floor is request 10
  // plus actual lookahead 1. In this valid no-input NMRA path the grant equals
  // the request; the standard specifies that result when no TSO is delivered.
  REQUIRE(grantTime == L"10");
  REQUIRE_THROWS_AS(
      requester->sendInteraction(
          interactionClass,
          outgoingValues,
          tag,
          rti1516_2025::HLAinteger64Time(10)),
      rti1516_2025::InvalidLogicalTime);
  REQUIRE_NOTHROW(requester->sendInteraction(
      interactionClass,
      outgoingValues,
      tag,
      rti1516_2025::HLAinteger64Time(11)));

  REQUIRE_NOTHROW(requester->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(peer->resignFederationExecution(rti1516_2025::CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(requester->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(requester->disconnect());
  REQUIRE_NOTHROW(peer->disconnect());
}

}  // namespace
