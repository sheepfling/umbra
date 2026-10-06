#include "ieee1516_2025_federation_management_fixture_support.hpp"

TEST_CASE(
    "Embedded Request Retraction notifies delivered interaction recipients and suppresses queued fanout",
    "[integration][development-profile][interaction-management][time-management]"
    "[timestamped-interaction-request-retraction-fanout]"
    "[retract]"
    "[rti.service.send-interaction][rti.service.retract]"
    "[rti.service.time-advance-request][federate.callback.receive-interaction]"
    "[federate.callback.request-retraction]") {
  ReportingFederateAmbassador publisherReports;
  ReportingFederateAmbassador immediateReports;
  ReportingFederateAmbassador constrainedReports;
  auto publisher = makeRti();
  auto immediate = makeRti();
  auto constrained = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
                          "cpp" /
                          "tests" /
                          "data" /
                          "parameter-handle-provider-fom.xml")
                             .wstring();
  unsigned char const tagBytes[] = {0x52, 0x54, 0x49};
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));

  REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED));
  REQUIRE_NOTHROW(immediate->connect(immediateReports, HLA_EVOKED));
  REQUIRE_NOTHROW(constrained->connect(constrainedReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      publisher->createFederationExecution(federationName, fomModule, standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(publisher->joinFederationExecution(
      L"retraction-publisher", L"publisher", federationName));
  REQUIRE_NOTHROW(immediate->joinFederationExecution(
      L"retraction-immediate", L"subscriber", federationName));
  REQUIRE_NOTHROW(constrained->joinFederationExecution(
      L"retraction-constrained", L"subscriber", federationName));

  auto const interactionClass = publisher->getInteractionClassHandle(
      fixture_hla::fom::parameter_fixture_child_interaction);
  REQUIRE(interactionClass.isValid());
  REQUIRE_NOTHROW(publisher->publishInteractionClass(interactionClass));
  REQUIRE_NOTHROW(publisher->changeInteractionOrderType(interactionClass, TIMESTAMP));
  REQUIRE_NOTHROW(immediate->subscribeInteractionClass(interactionClass));
  REQUIRE_NOTHROW(constrained->subscribeInteractionClass(interactionClass));
  REQUIRE_NOTHROW(constrained->enableTimeConstrained());
  REQUIRE_FALSE(constrained->evokeCallback(0.0));
  REQUIRE_NOTHROW(publisher->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(1)));
  REQUIRE_FALSE(publisher->evokeCallback(0.0));

  auto const retraction = publisher->sendInteraction(
      interactionClass,
      ParameterHandleValueMap{},
      tag,
      rti1516_2025::HLAinteger64Time(2));
  REQUIRE(retraction.isValid());

  // The nonconstrained recipient sees the original timestamped interaction
  // immediately; the constrained recipient remains in the federation-owned
  // TSO queue until a later grant.
  REQUIRE_FALSE(immediate->evokeCallback(0.0));
  REQUIRE(immediateReports.timestampedInteractionReports.size() == 1);
  REQUIRE(immediateReports.timestampedInteractionReports.front().retractionValid);
  REQUIRE(constrainedReports.timestampedInteractionReports.empty());

  REQUIRE_NOTHROW(publisher->retract(retraction));
  REQUIRE_FALSE(immediate->evokeCallback(0.0));
  REQUIRE(immediateReports.requestRetractionReports.size() == 1);
  REQUIRE(immediateReports.requestRetractionReports.front().retractionValid);
  REQUIRE(variableLengthDataBytes(
              immediateReports.requestRetractionReports.front().encodedRetraction) ==
          variableLengthDataBytes(retraction.encode()));
  REQUIRE(immediateReports.callbackOrder ==
          std::vector<std::string>{"interaction", "request-retraction"});

  // The still-pending fanout is removed before it can cross the recipient's
  // grant boundary. Advancing the regulator past its initial position releases
  // the recipient's TAR to 2 without a stale Receive Interaction callback.
  REQUIRE_NOTHROW(constrained->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(2)));
  REQUIRE_NOTHROW(publisher->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(2)));
  REQUIRE_FALSE(publisher->evokeCallback(0.0));
  REQUIRE_FALSE(constrained->evokeCallback(0.0));
  REQUIRE(constrainedReports.timeAdvanceGrantReports.size() == 1);
  REQUIRE(constrainedReports.timestampedInteractionReports.empty());
  REQUIRE(constrainedReports.requestRetractionReports.empty());

  // Once the constrained recipient resigns, the next timestamped interaction
  // has no temporal-queue fanout at all. It must still retain a federation
  // ledger record so its already-delivered nonconstrained recipient can receive
  // Request Retraction.
  REQUIRE_NOTHROW(constrained->resignFederationExecution(NO_ACTION));

  auto const immediateOnlyRetraction = publisher->sendInteraction(
      interactionClass,
      ParameterHandleValueMap{},
      tag,
      rti1516_2025::HLAinteger64Time(4));
  REQUIRE(immediateOnlyRetraction.isValid());
  REQUIRE_FALSE(immediate->evokeCallback(0.0));
  REQUIRE(immediateReports.timestampedInteractionReports.size() == 2);
  REQUIRE(immediateReports.timestampedInteractionReports.back().retractionValid);

  REQUIRE_NOTHROW(publisher->retract(immediateOnlyRetraction));
  REQUIRE_FALSE(immediate->evokeCallback(0.0));
  REQUIRE(immediateReports.requestRetractionReports.size() == 2);
  REQUIRE(immediateReports.requestRetractionReports.back().retractionValid);
  REQUIRE(variableLengthDataBytes(
              immediateReports.requestRetractionReports.back().encodedRetraction) ==
          variableLengthDataBytes(immediateOnlyRetraction.encode()));

  REQUIRE_NOTHROW(immediate->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(constrained->disconnect());
  REQUIRE_NOTHROW(immediate->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
}
