#include "ieee1516_2025_federation_management_test_support.hpp"
#include "ieee1516_2025_federation_management_fixture_support.hpp"

TEST_CASE(
    "Embedded time-role services keep enable requests callback-gated before TSO support",
    "[integration][development-profile][time-management][time-role][callbacks]"
    "[rti.service.enable-time-regulation][rti.service.disable-time-regulation]"
    "[rti.service.enable-time-constrained][rti.service.disable-time-constrained]"
    "[rti.service.query-lookahead]"
    "[federate.callback.time-regulation-enabled]"
    "[federate.callback.time-constrained-enabled]"
    "[time-role-enable-callback-gating][2025]") {
  ReportingFederateAmbassador evokedReports;
  ReportingFederateAmbassador immediateReports;
  auto evoked = makeRti();
  auto immediate = makeRti();
  auto const evokedFederationName = nextFederationName();
  auto const immediateFederationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();

  rti1516_2025::HLAinteger64Interval queriedLookahead;
  REQUIRE_THROWS_AS(
      evoked->enableTimeRegulation(rti1516_2025::HLAinteger64Interval(2)),
      rti1516_2025::NotConnected);
  REQUIRE_THROWS_AS(evoked->disableTimeRegulation(), rti1516_2025::NotConnected);
  REQUIRE_THROWS_AS(evoked->enableTimeConstrained(), rti1516_2025::NotConnected);
  REQUIRE_THROWS_AS(evoked->disableTimeConstrained(), rti1516_2025::NotConnected);
  REQUIRE_THROWS_AS(evoked->queryLookahead(queriedLookahead), rti1516_2025::NotConnected);

  REQUIRE_NOTHROW(evoked->connect(evokedReports, HLA_EVOKED));
  REQUIRE_THROWS_AS(
      evoked->enableTimeRegulation(rti1516_2025::HLAinteger64Interval(2)),
      rti1516_2025::FederateNotExecutionMember);
  REQUIRE_THROWS_AS(evoked->enableTimeConstrained(), rti1516_2025::FederateNotExecutionMember);
  REQUIRE_THROWS_AS(
      evoked->queryLookahead(queriedLookahead),
      rti1516_2025::FederateNotExecutionMember);

  REQUIRE_NOTHROW(
      evoked->createFederationExecution(
          evokedFederationName,
          fomModule,
          standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(
      evoked->joinFederationExecution(L"evoked-time-role-client", evokedFederationName));
  REQUIRE_THROWS_AS(
      evoked->queryLookahead(queriedLookahead),
      rti1516_2025::TimeRegulationIsNotEnabled);
  REQUIRE_THROWS_AS(
      evoked->enableTimeRegulation(rti1516_2025::HLAfloat64Interval(2.0)),
      rti1516_2025::InvalidLookahead);
  // The official reference interval constructors reject a negative value
  // before a caller can invoke the service; the mismatched reference type
  // above exercises Umbra's service-boundary InvalidLookahead mapping.

  REQUIRE_NOTHROW(evoked->enableTimeRegulation(rti1516_2025::HLAinteger64Interval(2)));
  REQUIRE(evokedReports.timeRegulationEnabledReports.empty());
  REQUIRE_THROWS_AS(
      evoked->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(1)),
      rti1516_2025::RequestForTimeRegulationPending);
  REQUIRE_THROWS_AS(
      evoked->enableTimeRegulation(rti1516_2025::HLAinteger64Interval(3)),
      rti1516_2025::RequestForTimeRegulationPending);
  REQUIRE_FALSE(evoked->evokeCallback(0.0));
  REQUIRE(evokedReports.timeRegulationEnabledReports.size() == 1);
  REQUIRE(evokedReports.timeRegulationEnabledReports.front().implementationName == standard_hla::mom::integer64_time);
  REQUIRE(evokedReports.timeRegulationEnabledReports.front().value == L"0");
  REQUIRE_NOTHROW(evoked->queryLookahead(queriedLookahead));
  REQUIRE(queriedLookahead.getInterval() == 2);
  rti1516_2025::HLAfloat64Interval mismatchedQueryLookahead;
  REQUIRE_THROWS_AS(
      evoked->queryLookahead(mismatchedQueryLookahead),
      rti1516_2025::RTIinternalError);
  REQUIRE_NOTHROW(evoked->disableTimeRegulation());
  REQUIRE_THROWS_AS(
      evoked->queryLookahead(queriedLookahead),
      rti1516_2025::TimeRegulationIsNotEnabled);
  REQUIRE_THROWS_AS(
      evoked->disableTimeRegulation(),
      rti1516_2025::TimeRegulationIsNotEnabled);

  REQUIRE_NOTHROW(evoked->enableTimeConstrained());
  REQUIRE(evokedReports.timeConstrainedEnabledReports.empty());
  REQUIRE_THROWS_AS(
      evoked->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(1)),
      rti1516_2025::RequestForTimeConstrainedPending);
  REQUIRE_THROWS_AS(
      evoked->enableTimeConstrained(),
      rti1516_2025::RequestForTimeConstrainedPending);
  REQUIRE_FALSE(evoked->evokeCallback(0.0));
  REQUIRE(evokedReports.timeConstrainedEnabledReports.size() == 1);
  REQUIRE(evokedReports.timeConstrainedEnabledReports.front().value == L"0");
  REQUIRE_NOTHROW(evoked->disableTimeConstrained());
  REQUIRE_THROWS_AS(
      evoked->disableTimeConstrained(),
      rti1516_2025::TimeConstrainedIsNotEnabled);

  REQUIRE_NOTHROW(immediate->connect(immediateReports, rti1516_2025::HLA_IMMEDIATE));
  REQUIRE_NOTHROW(
      immediate->createFederationExecution(
          immediateFederationName,
          fomModule,
          standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(
      immediate->joinFederationExecution(L"immediate-time-role-client", immediateFederationName));
  REQUIRE_NOTHROW(immediate->enableTimeRegulation(rti1516_2025::HLAinteger64Interval(5)));
  REQUIRE(immediateReports.timeRegulationEnabledReports.size() == 1);
  rti1516_2025::HLAinteger64Interval immediateLookahead;
  REQUIRE_NOTHROW(immediate->queryLookahead(immediateLookahead));
  REQUIRE(immediateLookahead.getInterval() == 5);
  REQUIRE_NOTHROW(immediate->enableTimeConstrained());
  REQUIRE(immediateReports.timeConstrainedEnabledReports.size() == 1);

  REQUIRE_NOTHROW(evoked->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(evoked->destroyFederationExecution(evokedFederationName));
  REQUIRE_NOTHROW(immediate->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(immediate->destroyFederationExecution(immediateFederationName));
  REQUIRE_NOTHROW(evoked->disconnect());
  REQUIRE_NOTHROW(immediate->disconnect());
}

TEST_CASE(
    "Enable Time Regulation applies its actual lookahead to timestamped sends",
    "[integration][development-profile][time-management][time-role][lookahead]"
    "[enable-time-regulation-actual-lookahead-tso-send]"
    "[rti.service.enable-time-regulation][rti.service.query-lookahead]"
    "[rti.service.send-interaction][federate.callback.time-regulation-enabled]"
    "[2025]") {
  ReportingFederateAmbassador reports;
  auto rti = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();

  REQUIRE_NOTHROW(rti->connect(reports, HLA_EVOKED));
  REQUIRE_NOTHROW(rti->createFederationExecution(
      federationName, fomModule, standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(rti->joinFederationExecution(
      L"actual-lookahead-sender", L"sender", federationName));

  auto const interactionClass = rti->getInteractionClassHandle(
      fixture_hla::fom::main_course_served);
  REQUIRE(interactionClass.isValid());
  REQUIRE_NOTHROW(rti->publishInteractionClass(interactionClass));
  REQUIRE_NOTHROW(rti->changeInteractionOrderType(interactionClass, TIMESTAMP));

  rti1516_2025::HLAinteger64Interval const requestedLookahead(2);
  REQUIRE_NOTHROW(rti->enableTimeRegulation(requestedLookahead));
  REQUIRE(reports.timeRegulationEnabledReports.empty());
  REQUIRE_FALSE(rti->evokeCallback(0.0));
  REQUIRE(reports.timeRegulationEnabledReports.size() == 1U);
  REQUIRE(reports.timeRegulationEnabledReports.front().value == L"0");

  rti1516_2025::HLAinteger64Interval actualLookahead;
  REQUIRE_NOTHROW(rti->queryLookahead(actualLookahead));
  REQUIRE(actualLookahead.getInterval() == 2);

  ParameterHandleValueMap const values;
  VariableLengthData const tag;
  REQUIRE_THROWS_AS(
      rti->sendInteraction(
          interactionClass,
          values,
          tag,
          rti1516_2025::HLAinteger64Time(1)),
      rti1516_2025::InvalidLogicalTime);
  auto const message = rti->sendInteraction(
      interactionClass,
      values,
      tag,
      rti1516_2025::HLAinteger64Time(2));
  REQUIRE(message.isValid());

  REQUIRE_NOTHROW(rti->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(rti->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(rti->disconnect());
}

TEST_CASE(
    "Disable Time Regulation converts timestamped sends to receive order",
    "[integration][development-profile][time-management][time-role]"
    "[disable-time-regulation-send-order-transition][2025]"
    "[rti.service.disable-time-regulation][rti.service.send-interaction]"
    "[federate.callback.receive-interaction]") {
  ReportingFederateAmbassador senderReports;
  ReportingFederateAmbassador receiverReports;
  auto sender = makeRti();
  auto receiver = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();

  REQUIRE_NOTHROW(sender->connect(senderReports, rti1516_2025::HLA_IMMEDIATE));
  REQUIRE_NOTHROW(receiver->connect(receiverReports, rti1516_2025::HLA_IMMEDIATE));
  REQUIRE_NOTHROW(sender->createFederationExecution(
      federationName, fomModule, standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(sender->joinFederationExecution(
      L"disable-regulation-send-order-publisher", L"publisher", federationName));
  REQUIRE_NOTHROW(receiver->joinFederationExecution(
      L"disable-regulation-send-order-subscriber", L"subscriber", federationName));

  auto const interactionClass = sender->getInteractionClassHandle(
      fixture_hla::fom::main_course_served);
  REQUIRE(interactionClass.isValid());
  REQUIRE_NOTHROW(sender->publishInteractionClass(interactionClass));
  REQUIRE_NOTHROW(sender->changeInteractionOrderType(interactionClass, TIMESTAMP));
  REQUIRE_NOTHROW(receiver->subscribeInteractionClass(interactionClass));
  REQUIRE_NOTHROW(sender->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(1)));
  REQUIRE(senderReports.timeRegulationEnabledReports.size() == 1U);

  REQUIRE_NOTHROW(sender->disableTimeRegulation());

  // This profile exposes the post-disable conversion through the timestamped
  // callback shape, while the public callback's explicit order fields record
  // the converted receive-order semantics. This case checks those order
  // fields only; it does not establish the normative callback-shape rule.
  REQUIRE_NOTHROW(sender->sendInteraction(
      interactionClass,
      ParameterHandleValueMap{},
      VariableLengthData{},
      rti1516_2025::HLAinteger64Time(5)));
  REQUIRE(receiverReports.interactionReports.empty());
  REQUIRE(receiverReports.timestampedInteractionReports.size() == 1U);
  auto const& delivered = receiverReports.timestampedInteractionReports.front();
  REQUIRE(delivered.interactionClass == interactionClass);
  REQUIRE(delivered.sentOrderType == rti1516_2025::RECEIVE);
  REQUIRE(delivered.receivedOrderType == rti1516_2025::RECEIVE);

  REQUIRE_NOTHROW(receiver->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(sender->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(sender->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(receiver->disconnect());
  REQUIRE_NOTHROW(sender->disconnect());
}
