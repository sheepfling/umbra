#include "ieee1516_2025_federation_management_test_support.hpp"
#include "ieee1516_2025_federation_management_fixture_support.hpp"

namespace {
TEST_CASE(
    "Embedded Time Advance Request changes logical time only at Time Advance Grant dispatch",
    "[integration][development-profile][time-management][callbacks][time-advance-request]"
    "[time-advance-request-logical-time-gating]"
    "[rti.service.time-advance-request][rti.service.query-logical-time]"
    "[federate.callback.time-advance-grant]") {
  ReportingFederateAmbassador evokedReports;
  ReportingFederateAmbassador immediateReports;
  auto evoked = makeRti();
  auto immediate = makeRti();
  auto const evokedFederationName = nextFederationName();
  auto const immediateFederationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();

  rti1516_2025::HLAinteger64Time queriedTime;
  REQUIRE_THROWS_AS(evoked->queryLogicalTime(queriedTime), rti1516_2025::NotConnected);
  REQUIRE_THROWS_AS(
      evoked->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(1)),
      rti1516_2025::NotConnected);

  REQUIRE_NOTHROW(evoked->connect(evokedReports, HLA_EVOKED));
  REQUIRE_THROWS_AS(
      evoked->queryLogicalTime(queriedTime),
      rti1516_2025::FederateNotExecutionMember);
  REQUIRE_THROWS_AS(
      evoked->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(1)),
      rti1516_2025::FederateNotExecutionMember);

  REQUIRE_NOTHROW(
      evoked->createFederationExecution(
          evokedFederationName,
          fomModule,
          standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(evoked->joinFederationExecution(L"evoked-time-client", evokedFederationName));
  REQUIRE_NOTHROW(evoked->queryLogicalTime(queriedTime));
  REQUIRE(queriedTime.isInitial());
  rti1516_2025::HLAfloat64Time mismatchedQueryTime;
  REQUIRE_THROWS_AS(
      evoked->queryLogicalTime(mismatchedQueryTime),
      rti1516_2025::RTIinternalError);

  REQUIRE_THROWS_AS(
      evoked->timeAdvanceRequest(rti1516_2025::HLAfloat64Time(7.0)),
      rti1516_2025::InvalidLogicalTime);
  REQUIRE_NOTHROW(evoked->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(7)));
  REQUIRE_NOTHROW(evoked->queryLogicalTime(queriedTime));
  REQUIRE(queriedTime.isInitial());
  REQUIRE(evokedReports.timeAdvanceGrantReports.empty());
  REQUIRE_THROWS_AS(
      evoked->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(8)),
      rti1516_2025::InTimeAdvancingState);

  REQUIRE_FALSE(evoked->evokeCallback(0.0));
  REQUIRE(evokedReports.timeAdvanceGrantReports.size() == 1);
  REQUIRE(evokedReports.timeAdvanceGrantReports.front().implementationName == standard_hla::mom::integer64_time);
  REQUIRE(evokedReports.timeAdvanceGrantReports.front().value == L"7");
  REQUIRE_NOTHROW(evoked->queryLogicalTime(queriedTime));
  REQUIRE(queriedTime.getTime() == 7);
  REQUIRE_THROWS_AS(
      evoked->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(6)),
      rti1516_2025::LogicalTimeAlreadyPassed);

  // Resignation deactivates the per-federate state. Its already-queued grant
  // is harmlessly consumed without advancing state or invoking a callback.
  REQUIRE_NOTHROW(evoked->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(10)));
  REQUIRE_NOTHROW(evoked->resignFederationExecution(NO_ACTION));
  REQUIRE_FALSE(evoked->evokeCallback(0.0));
  REQUIRE(evokedReports.timeAdvanceGrantReports.size() == 1);

  REQUIRE_NOTHROW(immediate->connect(immediateReports, rti1516_2025::HLA_IMMEDIATE));
  REQUIRE_NOTHROW(
      immediate->createFederationExecution(
          immediateFederationName,
          fomModule,
          standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(
      immediate->joinFederationExecution(L"immediate-time-client", immediateFederationName));
  REQUIRE_NOTHROW(immediate->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(5)));
  REQUIRE(immediateReports.timeAdvanceGrantReports.size() == 1);
  REQUIRE(immediateReports.timeAdvanceGrantReports.front().value == L"5");
  rti1516_2025::HLAinteger64Time immediateTime;
  REQUIRE_NOTHROW(immediate->queryLogicalTime(immediateTime));
  REQUIRE(immediateTime.getTime() == 5);

  REQUIRE_NOTHROW(evoked->destroyFederationExecution(evokedFederationName));
  REQUIRE_NOTHROW(immediate->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(immediate->destroyFederationExecution(immediateFederationName));
  REQUIRE_NOTHROW(evoked->disconnect());
  REQUIRE_NOTHROW(immediate->disconnect());
}


TEST_CASE(
    "Embedded three-federate GALT tracks the minimum regulator and resignation",
    "[integration][development-profile][time-management][galt][lits][callbacks]"
    "[multi-federate-callback-ordering]"
    "[rti.service.enable-time-regulation][rti.service.enable-time-constrained]"
    "[rti.service.time-advance-request][rti.service.query-galt]"
    "[rti.service.query-lits][rti.service.resign-federation-execution]") {
  ReportingFederateAmbassador observerReports;
  ReportingFederateAmbassador leftRegulatorReports;
  ReportingFederateAmbassador rightRegulatorReports;
  auto observer = makeRti();
  auto leftRegulator = makeRti();
  auto rightRegulator = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  rti1516_2025::HLAinteger64Time galt;
  rti1516_2025::HLAinteger64Time lits;
  rti1516_2025::HLAinteger64Time leftTime;

  REQUIRE_NOTHROW(observer->connect(observerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      observer->createFederationExecution(federationName, fomModule, standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(observer->joinFederationExecution(L"observer", federationName));
  REQUIRE_NOTHROW(leftRegulator->connect(leftRegulatorReports, HLA_EVOKED));
  REQUIRE_NOTHROW(leftRegulator->joinFederationExecution(L"left-regulator", federationName));
  REQUIRE_NOTHROW(rightRegulator->connect(rightRegulatorReports, HLA_EVOKED));
  REQUIRE_NOTHROW(rightRegulator->joinFederationExecution(L"right-regulator", federationName));

  REQUIRE_NOTHROW(observer->enableTimeConstrained());
  REQUIRE_FALSE(observer->evokeCallback(0.0));
  REQUIRE(observerReports.timeConstrainedEnabledReports.size() == 1);
  REQUIRE_NOTHROW(leftRegulator->enableTimeRegulation(rti1516_2025::HLAinteger64Interval(1)));
  REQUIRE_FALSE(leftRegulator->evokeCallback(0.0));
  REQUIRE(leftRegulatorReports.timeRegulationEnabledReports.size() == 1);
  REQUIRE_NOTHROW(rightRegulator->enableTimeRegulation(rti1516_2025::HLAinteger64Interval(3)));
  REQUIRE_FALSE(rightRegulator->evokeCallback(0.0));
  REQUIRE(rightRegulatorReports.timeRegulationEnabledReports.size() == 1);

  // At the initial logical time, the left regulator (0 + 1) supplies the
  // minimum GALT/LITS candidate rather than the right regulator (0 + 3).
  REQUIRE(observer->queryGALT(galt));
  REQUIRE(galt.getTime() == 1);
  REQUIRE(observer->queryLITS(lits));
  REQUIRE(lits.getTime() == 1);

  // A pending and then granted TAR changes the left regulator's candidate to
  // 5 + 1.  A separately pending right-regulator TAR at 1 + 3 becomes the
  // federation-wide minimum without sharing either regulator's callback queue.
  REQUIRE_NOTHROW(leftRegulator->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(5)));
  REQUIRE_NOTHROW(rightRegulator->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(1)));
  REQUIRE(observer->queryGALT(galt));
  REQUIRE(galt.getTime() == 4);
  REQUIRE(observer->queryLITS(lits));
  REQUIRE(lits.getTime() == 4);
  REQUIRE(leftRegulatorReports.timeAdvanceGrantReports.empty());
  REQUIRE(rightRegulatorReports.timeAdvanceGrantReports.empty());

  // Granting the left request must not drain or synthesize the right
  // regulator's independently pending grant.
  REQUIRE_FALSE(leftRegulator->evokeCallback(0.0));
  REQUIRE(leftRegulatorReports.timeAdvanceGrantReports.size() == 1);
  REQUIRE(rightRegulatorReports.timeAdvanceGrantReports.empty());
  REQUIRE_NOTHROW(leftRegulator->queryLogicalTime(leftTime));
  REQUIRE(leftTime.getTime() == 5);
  REQUIRE(observer->queryGALT(galt));
  REQUIRE(galt.getTime() == 4);

  REQUIRE_FALSE(rightRegulator->evokeCallback(0.0));
  REQUIRE(rightRegulatorReports.timeAdvanceGrantReports.size() == 1);
  REQUIRE(observer->queryGALT(galt));
  REQUIRE(galt.getTime() == 4);

  // Resigning the regulator that supplied the minimum removes it from the
  // federation-owned snapshot, leaving the left regulator's 5 + 1 candidate.
  REQUIRE_NOTHROW(rightRegulator->resignFederationExecution(NO_ACTION));
  REQUIRE(observer->queryGALT(galt));
  REQUIRE(galt.getTime() == 6);
  REQUIRE(observer->queryLITS(lits));
  REQUIRE(lits.getTime() == 6);

  REQUIRE_NOTHROW(leftRegulator->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(observer->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(observer->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(rightRegulator->disconnect());
  REQUIRE_NOTHROW(leftRegulator->disconnect());
  REQUIRE_NOTHROW(observer->disconnect());
}

TEST_CASE(
    "Embedded Query LITS remains defined for a queued TSO after source resignation",
    "[integration][development-profile][time-management][galt][lits][tso]"
    "[rti.service.query-galt][rti.service.query-lits]"
    "[rti.service.send-interaction][rti.service.enable-time-regulation]"
    "[rti.service.enable-time-constrained][rti.service.resign-federation-execution]") {
  ReportingFederateAmbassador publisherReports;
  ReportingFederateAmbassador receiverReports;
  auto publisher = makeRti();
  auto receiver = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  rti1516_2025::HLAinteger64Time galt;
  rti1516_2025::HLAinteger64Time lits;

  REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED));
  REQUIRE_NOTHROW(receiver->connect(receiverReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      publisher->createFederationExecution(federationName, fomModule, standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(publisher->joinFederationExecution(
      L"queued-lits-source", L"publisher", federationName));
  REQUIRE_NOTHROW(receiver->joinFederationExecution(
      L"queued-lits-receiver", L"subscriber", federationName));

  auto const interactionClass = publisher->getInteractionClassHandle(
      fixture_hla::fom::main_course_served);
  auto const temperatureOk = publisher->getParameterHandle(
      interactionClass,
      fixture_hla::fixture::temperature_ok);
  REQUIRE(interactionClass.isValid());
  REQUIRE(temperatureOk.isValid());

  unsigned char const parameterBytes[] = {0x51, 0x4C, 0x49, 0x54, 0x53};
  unsigned char const tagBytes[] = {0x51, 0x55, 0x45, 0x55, 0x45};
  ParameterHandleValueMap parameterValues;
  parameterValues.emplace(
      temperatureOk,
      VariableLengthData(parameterBytes, sizeof(parameterBytes)));
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));

  REQUIRE_NOTHROW(publisher->publishInteractionClass(interactionClass));
  REQUIRE_NOTHROW(publisher->changeInteractionOrderType(interactionClass, TIMESTAMP));
  REQUIRE_NOTHROW(receiver->subscribeInteractionClass(interactionClass));
  REQUIRE_NOTHROW(receiver->enableTimeConstrained());
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE(receiverReports.timeConstrainedEnabledReports.size() == 1U);
  REQUIRE_NOTHROW(publisher->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(1)));
  while (publisher->evokeCallback(0.0)) {
  }
  REQUIRE(publisherReports.timeRegulationEnabledReports.size() == 1U);

  auto const retraction = publisher->sendInteraction(
      interactionClass,
      parameterValues,
      tag,
      rti1516_2025::HLAinteger64Time(5));
  REQUIRE(retraction.isValid());
  REQUIRE(receiverReports.timestampedInteractionReports.empty());

  // While the source regulator is still joined, it supplies GALT=1.  The
  // queued message at 5 is future TSO input but does not lower that bound.
  REQUIRE(receiver->queryGALT(galt));
  REQUIRE(galt.getTime() == 1);
  REQUIRE(receiver->queryLITS(lits));
  REQUIRE(lits.getTime() == 1);

  // Voluntary NO_ACTION resignation removes the source regulator but retains
  // the accepted recipient-local queue entry.  GALT is therefore undefined,
  // while LITS remains the future queued timestamp.
  REQUIRE_NOTHROW(publisher->resignFederationExecution(NO_ACTION));
  REQUIRE_FALSE(receiver->queryGALT(galt));
  REQUIRE(receiver->queryLITS(lits));
  REQUIRE(lits.getTime() == 5);
  REQUIRE(receiverReports.timestampedInteractionReports.empty());

  REQUIRE_NOTHROW(receiver->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(receiver->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(publisher->disconnect());
  REQUIRE_NOTHROW(receiver->disconnect());
}

TEST_CASE(
    "Embedded constrained TAR waits for GALT and is released by a regulator advance",
    "[integration][development-profile][time-management][galt][callbacks]"
    "[rti.service.time-advance-request][federate.callback.time-advance-grant]") {
  ReportingFederateAmbassador receiverReports;
  ReportingFederateAmbassador regulatorReports;
  auto receiver = makeRti();
  auto regulator = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();

  REQUIRE_NOTHROW(receiver->connect(receiverReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      receiver->createFederationExecution(federationName, fomModule, standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(receiver->joinFederationExecution(L"receiver", federationName));
  REQUIRE_NOTHROW(regulator->connect(regulatorReports, HLA_EVOKED));
  REQUIRE_NOTHROW(regulator->joinFederationExecution(L"regulator", federationName));

  REQUIRE_NOTHROW(receiver->enableTimeConstrained());
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE(receiverReports.timeConstrainedEnabledReports.size() == 1);
  REQUIRE_NOTHROW(regulator->enableTimeRegulation(rti1516_2025::HLAinteger64Interval(2)));
  REQUIRE_FALSE(regulator->evokeCallback(0.0));
  REQUIRE(regulatorReports.timeRegulationEnabledReports.size() == 1);

  // GALT is 2, so TAR is strict and TAR(2) must remain pending.
  REQUIRE_NOTHROW(receiver->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(2)));
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE(receiverReports.timeAdvanceGrantReports.empty());
  rti1516_2025::HLAinteger64Time receiverTime;
  REQUIRE_NOTHROW(receiver->queryLogicalTime(receiverTime));
  REQUIRE(receiverTime.isInitial());

  // While the regulator is Time Advancing to 1, its pending time plus
  // lookahead raises GALT to 3 and releases the receiver's TAR(2), before the
  // regulator receives its own grant.
  REQUIRE_NOTHROW(regulator->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(1)));
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE(receiverReports.timeAdvanceGrantReports.size() == 1);
  REQUIRE(receiverReports.timeAdvanceGrantReports.front().value == L"2");
  REQUIRE_NOTHROW(receiver->queryLogicalTime(receiverTime));
  REQUIRE(receiverTime.getTime() == 2);

  REQUIRE_FALSE(regulator->evokeCallback(0.0));
  REQUIRE(regulatorReports.timeAdvanceGrantReports.size() == 1);
  REQUIRE(regulatorReports.timeAdvanceGrantReports.front().value == L"1");

  REQUIRE_NOTHROW(regulator->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(receiver->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(receiver->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(regulator->disconnect());
  REQUIRE_NOTHROW(receiver->disconnect());
}

TEST_CASE(
    "Embedded NRG-disabled constrained TAR waits until a regulator becomes active",
    "[integration][development-profile][time-management][galt][callbacks]"
    "[rti.service.enable-time-regulation][rti.service.time-advance-request]"
    "[federate.callback.time-advance-grant]") {
  ReportingFederateAmbassador receiverReports;
  ReportingFederateAmbassador regulatorReports;
  auto receiver = makeRti();
  auto regulator = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();

  REQUIRE_NOTHROW(receiver->connect(receiverReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      receiver->createFederationExecution(federationName, fomModule, standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(receiver->joinFederationExecution(L"receiver", federationName));
  REQUIRE_NOTHROW(regulator->connect(regulatorReports, HLA_EVOKED));
  REQUIRE_NOTHROW(regulator->joinFederationExecution(L"regulator", federationName));

  REQUIRE_NOTHROW(receiver->enableTimeConstrained());
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE_NOTHROW(receiver->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(1)));
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE(receiverReports.timeAdvanceGrantReports.empty());

  // The supplied FOM omits NRG, so its standard default is Disabled. Once a
  // regulator becomes active at time 0 with lookahead 2, TAR(1) is below the
  // newly defined GALT and the role callback wakes the deferred receiver.
  REQUIRE_NOTHROW(regulator->enableTimeRegulation(rti1516_2025::HLAinteger64Interval(2)));
  REQUIRE_FALSE(regulator->evokeCallback(0.0));
  REQUIRE(regulatorReports.timeRegulationEnabledReports.size() == 1);
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE(receiverReports.timeAdvanceGrantReports.size() == 1);
  REQUIRE(receiverReports.timeAdvanceGrantReports.front().value == L"1");

  REQUIRE_NOTHROW(regulator->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(receiver->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(receiver->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(regulator->disconnect());
  REQUIRE_NOTHROW(receiver->disconnect());
}

TEST_CASE(
    "Embedded constrained TAR is released when time-constrained mode is disabled",
    "[integration][development-profile][time-management][galt][callbacks]"
    "[rti.service.disable-time-constrained][rti.service.time-advance-request]"
    "[federate.callback.time-advance-grant]") {
  ReportingFederateAmbassador receiverReports;
  auto receiver = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();

  REQUIRE_NOTHROW(receiver->connect(receiverReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      receiver->createFederationExecution(federationName, fomModule, standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(receiver->joinFederationExecution(L"receiver", federationName));
  REQUIRE_NOTHROW(receiver->enableTimeConstrained());
  REQUIRE_FALSE(receiver->evokeCallback(0.0));

  // No other regulator means GALT is undefined, and the supplied FOM's NRG
  // switch defaults to Disabled. The constrained TAR therefore remains
  // pending until the role transition removes its GALT restriction.
  REQUIRE_NOTHROW(receiver->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(1)));
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE(receiverReports.timeAdvanceGrantReports.empty());
  REQUIRE_NOTHROW(receiver->disableTimeConstrained());
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE(receiverReports.timeAdvanceGrantReports.size() == 1);
  REQUIRE(receiverReports.timeAdvanceGrantReports.front().value == L"1");

  REQUIRE_NOTHROW(receiver->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(receiver->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(receiver->disconnect());
}

TEST_CASE(
    "Embedded NRG-enabled constrained TAR is released when its only regulator disables",
    "[integration][development-profile][time-management][galt][non-regulated-grant][callbacks]"
    "[rti.service.disable-time-regulation][rti.service.time-advance-request]"
    "[federate.callback.time-advance-grant]") {
  auto const nrgFom = nrgEnabledRestaurantModule();
  ReportingFederateAmbassador receiverReports;
  ReportingFederateAmbassador regulatorReports;
  auto receiver = makeRti();
  auto regulator = makeRti();
  auto const federationName = nextFederationName();

  REQUIRE_NOTHROW(receiver->connect(receiverReports, HLA_EVOKED));
  REQUIRE_NOTHROW(receiver->createFederationExecution(
      federationName,
      nrgFom.path().wstring(),
      standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(receiver->joinFederationExecution(L"receiver", federationName));
  REQUIRE(receiver->getNonRegulatedGrantSwitch());
  REQUIRE_NOTHROW(regulator->connect(regulatorReports, HLA_EVOKED));
  REQUIRE_NOTHROW(regulator->joinFederationExecution(L"regulator", federationName));
  REQUIRE(regulator->getNonRegulatedGrantSwitch());

  REQUIRE_NOTHROW(receiver->enableTimeConstrained());
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE_NOTHROW(regulator->enableTimeRegulation(rti1516_2025::HLAinteger64Interval(2)));
  REQUIRE_FALSE(regulator->evokeCallback(0.0));

  // Defined GALT is 2, so the strict TAR(2) waits while the regulator is
  // active. Disabling the sole regulator makes GALT undefined; the FDD's
  // enabled NRG switch then lets the receiver advance without waiting.
  REQUIRE_NOTHROW(receiver->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(2)));
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE(receiverReports.timeAdvanceGrantReports.empty());
  REQUIRE_NOTHROW(regulator->disableTimeRegulation());
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE(receiverReports.timeAdvanceGrantReports.size() == 1);
  REQUIRE(receiverReports.timeAdvanceGrantReports.front().value == L"2");

  REQUIRE_NOTHROW(regulator->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(receiver->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(receiver->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(regulator->disconnect());
  REQUIRE_NOTHROW(receiver->disconnect());
}

TEST_CASE(
    "Embedded NRG-enabled constrained TAR is released when its only regulator resigns",
    "[integration][development-profile][time-management][galt][non-regulated-grant][callbacks]"
    "[rti.service.resign-federation-execution][rti.service.time-advance-request]"
    "[federate.callback.time-advance-grant]") {
  auto const nrgFom = nrgEnabledRestaurantModule();
  ReportingFederateAmbassador receiverReports;
  ReportingFederateAmbassador regulatorReports;
  auto receiver = makeRti();
  auto regulator = makeRti();
  auto const federationName = nextFederationName();

  REQUIRE_NOTHROW(receiver->connect(receiverReports, HLA_EVOKED));
  REQUIRE_NOTHROW(receiver->createFederationExecution(
      federationName,
      nrgFom.path().wstring(),
      standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(receiver->joinFederationExecution(L"receiver", federationName));
  REQUIRE_NOTHROW(regulator->connect(regulatorReports, HLA_EVOKED));
  REQUIRE_NOTHROW(regulator->joinFederationExecution(L"regulator", federationName));

  REQUIRE_NOTHROW(receiver->enableTimeConstrained());
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE_NOTHROW(regulator->enableTimeRegulation(rti1516_2025::HLAinteger64Interval(2)));
  REQUIRE_FALSE(regulator->evokeCallback(0.0));

  // The defined GALT is 2, so strict TAR(2) waits. Resigning the sole
  // regulator removes the bound; enabled NRG then releases the pending TAR.
  REQUIRE_NOTHROW(receiver->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(2)));
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE(receiverReports.timeAdvanceGrantReports.empty());
  REQUIRE_NOTHROW(regulator->resignFederationExecution(NO_ACTION));
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE(receiverReports.timeAdvanceGrantReports.size() == 1);
  REQUIRE(receiverReports.timeAdvanceGrantReports.front().value == L"2");

  REQUIRE_NOTHROW(receiver->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(receiver->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(regulator->disconnect());
  REQUIRE_NOTHROW(receiver->disconnect());
}

}  // namespace
