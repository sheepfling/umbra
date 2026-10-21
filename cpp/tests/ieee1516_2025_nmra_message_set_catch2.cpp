#include "ieee1516_2025_federation_management_fixture_support.hpp"

#include <algorithm>
#include <vector>

namespace {

void drainCallbacks(rti1516_2025::RTIambassador& rti) {
  for (int pass = 0; pass != 128; ++pass) {
    if (!rti.evokeCallback(0.0)) {
      break;
    }
  }
}

TEST_CASE(
    "Embedded NMRA delivers queued RO messages and the earliest equal-time TSO cohort",
    "[integration][development-profile][time-management]"
    "[next-message-request-available][nmra-message-set]"
    "[rti.service.next-message-request-available]"
    "[federate.callback.receive-interaction]"
    "[federate.callback.time-advance-grant]") {
  ReportingFederateAmbassador requesterReports;
  ReportingFederateAmbassador publisherReports;
  auto requester = makeRti();
  auto publisher = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule =
      (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" /
       "data" / "parameter-handle-provider-fom.xml")
          .wstring();

  REQUIRE_NOTHROW(requester->connect(requesterReports, HLA_EVOKED));
  REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED));
  REQUIRE_NOTHROW(requester->createFederationExecution(
      federationName, fomModule, standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(requester->joinFederationExecution(
      L"nmra-message-set-requester", L"subscriber", federationName));
  REQUIRE_NOTHROW(publisher->joinFederationExecution(
      L"nmra-message-set-publisher", L"publisher", federationName));

  auto const interactionClass = publisher->getInteractionClassHandle(
      fixture_hla::fom::parameter_fixture_child_interaction);
  auto const requesterInteractionClass = requester->getInteractionClassHandle(
      fixture_hla::fom::parameter_fixture_child_interaction);
  REQUIRE(interactionClass.isValid());
  REQUIRE(requesterInteractionClass.isValid());
  auto const parameter = publisher->getParameterHandle(
      interactionClass, fixture_hla::fixture::identifier);
  REQUIRE(parameter.isValid());
  REQUIRE_NOTHROW(publisher->publishInteractionClass(interactionClass));
  REQUIRE_NOTHROW(requester->subscribeInteractionClass(requesterInteractionClass));
  REQUIRE_NOTHROW(publisher->changeInteractionOrderType(interactionClass, TIMESTAMP));

  REQUIRE_NOTHROW(requester->enableTimeConstrained());
  drainCallbacks(*requester);
  REQUIRE(requesterReports.timeConstrainedEnabledReports.size() == 1U);
  REQUIRE_NOTHROW(publisher->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(1)));
  drainCallbacks(*publisher);
  REQUIRE(publisherReports.timeRegulationEnabledReports.size() == 1U);

  unsigned char const payloadBytes[] = {0x4EU, 0x4DU, 0x52U, 0x41U};
  ParameterHandleValueMap outgoingValues;
  outgoingValues.emplace(parameter, VariableLengthData(payloadBytes, sizeof(payloadBytes)));

  unsigned char const receiveOrderTagOneBytes[] = {0x11U};
  unsigned char const receiveOrderTagTwoBytes[] = {0x12U};
  VariableLengthData const receiveOrderTagOne(
      receiveOrderTagOneBytes, sizeof(receiveOrderTagOneBytes));
  VariableLengthData const receiveOrderTagTwo(
      receiveOrderTagTwoBytes, sizeof(receiveOrderTagTwoBytes));
  REQUIRE_NOTHROW(publisher->sendInteraction(
      interactionClass, outgoingValues, receiveOrderTagOne));
  REQUIRE_NOTHROW(publisher->sendInteraction(
      interactionClass, outgoingValues, receiveOrderTagTwo));

  unsigned char const firstTsoTagBytes[] = {0x21U};
  unsigned char const secondTsoTagBytes[] = {0x22U};
  unsigned char const laterTsoTagBytes[] = {0x30U};
  VariableLengthData const firstTsoTag(firstTsoTagBytes, sizeof(firstTsoTagBytes));
  VariableLengthData const secondTsoTag(secondTsoTagBytes, sizeof(secondTsoTagBytes));
  VariableLengthData const laterTsoTag(laterTsoTagBytes, sizeof(laterTsoTagBytes));
  REQUIRE(publisher->sendInteraction(
              interactionClass,
              outgoingValues,
              firstTsoTag,
              rti1516_2025::HLAinteger64Time(5))
              .isValid());
  REQUIRE(publisher->sendInteraction(
              interactionClass,
              outgoingValues,
              secondTsoTag,
              rti1516_2025::HLAinteger64Time(5))
              .isValid());
  REQUIRE(publisher->sendInteraction(
              interactionClass,
              outgoingValues,
              laterTsoTag,
              rti1516_2025::HLAinteger64Time(8))
              .isValid());

  REQUIRE(requesterReports.interactionReports.empty());
  REQUIRE(requesterReports.timestampedInteractionReports.empty());
  REQUIRE_NOTHROW(requester->nextMessageRequestAvailable(
      rti1516_2025::HLAinteger64Time(10)));
  REQUIRE(requesterReports.timeAdvanceGrantReports.empty());
  drainCallbacks(*requester);

  // All receive-order messages queued at invocation belong to the NMRA set,
  // independently of the timestamped message that determines the grant.
  REQUIRE(requesterReports.interactionReports.size() == 2U);
  REQUIRE(requesterReports.timestampedInteractionReports.empty());
  REQUIRE(requesterReports.timeAdvanceGrantReports.empty());

  // Advancing the publisher makes the queued TSO messages deliverable. NMRA
  // selects the smallest eligible timestamp (5), including its equal-time
  // cohort, but not the later timestamp (8), even though 8 is <= request 10.
  REQUIRE_NOTHROW(publisher->timeAdvanceRequest(
      rti1516_2025::HLAinteger64Time(12)));
  drainCallbacks(*publisher);
  drainCallbacks(*requester);

  REQUIRE(requesterReports.timestampedInteractionReports.size() == 2U);
  std::vector<unsigned char> receiveOrderTags;
  for (auto const& report : requesterReports.interactionReports) {
    auto const tagBytes = variableLengthDataBytes(report.userSuppliedTag);
    REQUIRE(tagBytes.size() == 1U);
    receiveOrderTags.push_back(tagBytes.front());
  }
  std::sort(receiveOrderTags.begin(), receiveOrderTags.end());
  REQUIRE(receiveOrderTags == std::vector<unsigned char>{0x11U, 0x12U});

  std::vector<unsigned char> deliveredTsoTags;
  for (auto const& report : requesterReports.timestampedInteractionReports) {
    REQUIRE(report.timeValue == L"5");
    deliveredTsoTags.push_back(variableLengthDataBytes(report.userSuppliedTag).front());
  }
  std::sort(deliveredTsoTags.begin(), deliveredTsoTags.end());
  REQUIRE(deliveredTsoTags == std::vector<unsigned char>{0x21U, 0x22U});

  REQUIRE(requesterReports.timeAdvanceGrantReports.size() == 1U);
  REQUIRE(requesterReports.timeAdvanceGrantReports.front().value == L"5");
  rti1516_2025::HLAinteger64Time requesterTime;
  REQUIRE_NOTHROW(requester->queryLogicalTime(requesterTime));
  REQUIRE(requesterTime.getTime() == 5);

  REQUIRE_NOTHROW(requester->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(
      rti1516_2025::CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(requester->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(requester->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
}

}  // namespace
