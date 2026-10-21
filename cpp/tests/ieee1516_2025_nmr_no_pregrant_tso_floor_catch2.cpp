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
    "Embedded NMR without pre-grant TSO input enforces the requested-time lookahead floor",
    "[integration][development-profile][time-management]"
    "[nmr-no-pregrant-tso-floor]"
    "[rti.service.next-message-request]"
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
      L"nmr-no-pregrant-floor-requester", L"publisher", federationName));
  REQUIRE_NOTHROW(peer->joinFederationExecution(
      L"nmr-no-pregrant-floor-peer", L"regulator", federationName));

  auto const interactionClass = requester->getInteractionClassHandle(
      fixture_hla::fom::parameter_fixture_child_interaction);
  REQUIRE(interactionClass.isValid());
  REQUIRE_NOTHROW(requester->publishInteractionClass(interactionClass));
  REQUIRE_NOTHROW(requester->changeInteractionOrderType(interactionClass, TIMESTAMP));

  REQUIRE_NOTHROW(requester->enableTimeConstrained());
  drainCallbacks(*requester);
  REQUIRE_NOTHROW(requester->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(1)));
  drainCallbacks(*requester);
  REQUIRE(requesterReports.timeRegulationEnabledReports.size() == 1U);
  REQUIRE_NOTHROW(peer->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(1)));
  drainCallbacks(*peer);

  // No TSO message is sent to the requester before its grant. Since NMR has
  // no message timestamp to select, it retains the specified target 10; the
  // peer's advance raises GALT above that target so the grant can complete.
  REQUIRE_NOTHROW(requester->nextMessageRequest(
      rti1516_2025::HLAinteger64Time(10)));
  REQUIRE(requesterReports.timestampedInteractionReports.empty());
  REQUIRE(requesterReports.timeAdvanceGrantReports.empty());
  REQUIRE_NOTHROW(peer->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(10)));
  drainCallbacks(*peer);
  drainCallbacks(*requester);
  REQUIRE(requesterReports.timestampedInteractionReports.empty());
  REQUIRE(requesterReports.timeAdvanceGrantReports.size() == 1U);
  REQUIRE(requesterReports.timeAdvanceGrantReports.front().value == L"10");

  // With no pre-grant TSO input, §8.10.2 anchors the future-message floor to
  // the specified NMR time 10 plus actual lookahead 1.
  auto const parameter = requester->getParameterHandle(
      interactionClass, fixture_hla::fixture::identifier);
  REQUIRE(parameter.isValid());
  unsigned char const payload[] = {0x4EU, 0x4DU, 0x52U};
  unsigned char const tagBytes[] = {0x46U, 0x4CU, 0x4FU, 0x4FU, 0x52U};
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));
  ParameterHandleValueMap values;
  values.emplace(parameter, VariableLengthData(payload, sizeof(payload)));
  REQUIRE_THROWS_AS(
      requester->sendInteraction(
          interactionClass,
          values,
          tag,
          rti1516_2025::HLAinteger64Time(10)),
      rti1516_2025::InvalidLogicalTime);
  REQUIRE_NOTHROW(requester->sendInteraction(
      interactionClass,
      values,
      tag,
      rti1516_2025::HLAinteger64Time(11)));

  REQUIRE_NOTHROW(requester->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(peer->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(requester->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(requester->disconnect());
  REQUIRE_NOTHROW(peer->disconnect());
}

}  // namespace
