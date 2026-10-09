#include "ieee1516_2025_federation_management_fixture_support.hpp"

namespace {

TEST_CASE(
    "Embedded transport loss reports the TSO cutoff through HLAreportFederateLost",
    "[integration][development-profile][federation-management][transport][mom]"
    "[interaction-management][time-management][asynchronous-delivery]"
    "[connection-lost-tso-cutoff]"
    "[rti.service.connection-lost][rti.service.send-interaction]"
    "[rti.service.time-advance-request][rti.service.enable-asynchronous-delivery]"
    "[federate.callback.connection-lost][federate.callback.receive-interaction]"
    "[federate.callback.time-advance-grant]") {
  ReportingFederateAmbassador lostReports;
  ReportingFederateAmbassador survivingReports;
  auto lost = makeRti();
  auto surviving = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  std::wstring const reportClassName =
      standard_hla::mom::report_federate_lost;
  std::wstring const tsoInteractionClassName =
      fixture_hla::fom::customer_seated;

  REQUIRE_NOTHROW(lost->connect(lostReports, HLA_EVOKED));
  REQUIRE_NOTHROW(surviving->connect(survivingReports, HLA_EVOKED));
  REQUIRE_NOTHROW(lost->createFederationExecution(
      federationName,
      fomModule,
      standard_hla::mom::integer64_time));
  FederateHandle lostFederate;
  REQUIRE_NOTHROW(lostFederate = lost->joinFederationExecution(
      L"report-cutoff-lost-publisher", L"publisher", federationName));
  REQUIRE_NOTHROW(surviving->joinFederationExecution(
      L"report-cutoff-surviving-subscriber", L"subscriber", federationName));

  auto const reportClass = surviving->getInteractionClassHandle(reportClassName);
  auto const reportTimestamp = surviving->getParameterHandle(reportClass, standard_hla::mom::time_stamp);
  auto const tsoInteractionClass = lost->getInteractionClassHandle(tsoInteractionClassName);
  REQUIRE(reportClass.isValid());
  REQUIRE(reportTimestamp.isValid());
  REQUIRE(tsoInteractionClass.isValid());
  REQUIRE_NOTHROW(lost->publishInteractionClass(tsoInteractionClass));
  REQUIRE_NOTHROW(lost->changeInteractionOrderType(tsoInteractionClass, TIMESTAMP));
  REQUIRE_NOTHROW(surviving->subscribeInteractionClass(reportClass));
  REQUIRE_NOTHROW(surviving->subscribeInteractionClass(tsoInteractionClass));
  REQUIRE_NOTHROW(surviving->enableTimeConstrained());
  REQUIRE_FALSE(surviving->evokeCallback(0.0));
  REQUIRE(survivingReports.timeConstrainedEnabledReports.size() == 1U);
  // HLAreportFederateLost is receive order.  §8.15 therefore makes this
  // explicitly enabled survivor able to receive it both while advancing and
  // after its matching grant; the test does not manufacture a MOM-specific
  // exception to the ordinary RO delivery rule.
  REQUIRE_NOTHROW(surviving->enableAsynchronousDelivery());
  REQUIRE_NOTHROW(lost->enableTimeRegulation(rti1516_2025::HLAinteger64Interval(5)));
  // The official Restaurant FOM also carries an interaction-relevance
  // advisory for this class.  Drain that unrelated setup callback alongside
  // Time Regulation Enabled rather than making it part of this fault test.
  while (lost->evokeCallback(0.0)) {
  }
  REQUIRE(lostReports.timeRegulationEnabledReports.size() == 1U);

  // Advance the time-regulating publisher to the timestamped interaction's
  // exact time before the fault.  The loss report's HLAtimeStamp and the
  // required queued TSO delivery must therefore describe one same boundary.
  auto const retraction = lost->sendInteraction(
      tsoInteractionClass,
      ParameterHandleValueMap{},
      VariableLengthData(),
      rti1516_2025::HLAinteger64Time(6));
  REQUIRE(retraction.isValid());
  REQUIRE_NOTHROW(surviving->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(6)));
  REQUIRE_NOTHROW(lost->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(6)));
  REQUIRE_FALSE(lost->evokeCallback(0.0));
  REQUIRE(lostReports.timeAdvanceGrantReports.size() == 1U);
  REQUIRE(lostReports.timeAdvanceGrantReports.front().value == L"6");
  REQUIRE(survivingReports.interactionReports.empty());
  REQUIRE(survivingReports.timestampedInteractionReports.empty());
  REQUIRE(survivingReports.timeAdvanceGrantReports.empty());

  std::wstring const faultDescription = L"report and cutoff transport fault";
  REQUIRE(umbra::detail::failEmbeddedTransportConnectionForTesting(
      *lost,
      faultDescription));
  REQUIRE(survivingReports.interactionReports.empty());
  REQUIRE(survivingReports.timestampedInteractionReports.empty());

  // The three independent callbacks (RO report, cutoff TSO interaction, and
  // grant) are permitted to require more than one zero-duration Evoke call.
  // Drain the callback queue without introducing an ordering claim between
  // the RO report and the timestamped interaction.
  while (surviving->evokeCallback(0.0)) {
  }
  REQUIRE(survivingReports.interactionReports.size() == 1U);
  REQUIRE(survivingReports.timestampedInteractionReports.size() == 1U);
  REQUIRE(survivingReports.timeAdvanceGrantReports.size() == 1U);
  REQUIRE(
      survivingReports.callbackOrder == std::vector<std::string>{"interaction", "grant"});

  auto const& lossReport = survivingReports.interactionReports.front();
  REQUIRE(lossReport.interactionClass == reportClass);
  REQUIRE(lossReport.producingFederate.isValid() == false);
  auto const reportTimestampValue = lossReport.parameterValues.find(reportTimestamp);
  REQUIRE(reportTimestampValue != lossReport.parameterValues.end());
  rti1516_2025::HLAinteger64Time decodedReportTimestamp;
  REQUIRE_NOTHROW(decodedReportTimestamp.decode(reportTimestampValue->second));
  REQUIRE(decodedReportTimestamp.getTime() == 6);

  auto const& tsoReport = survivingReports.timestampedInteractionReports.front();
  REQUIRE(tsoReport.interactionClass == tsoInteractionClass);
  REQUIRE(tsoReport.producingFederate == lostFederate);
  REQUIRE(tsoReport.timeImplementationName == standard_hla::mom::integer64_time);
  REQUIRE(tsoReport.timeValue == L"6");
  REQUIRE(tsoReport.sentOrderType == TIMESTAMP);
  REQUIRE(tsoReport.receivedOrderType == TIMESTAMP);
  REQUIRE(tsoReport.retractionSupplied);
  REQUIRE(tsoReport.retractionValid);
  REQUIRE(survivingReports.timeAdvanceGrantReports.front().value == L"6");

  // This case intentionally does not prescribe a callback order between the
  // receive-order MOM report and the timestamped application interaction; it
  // pins their common cutoff value, while the existing TSO cases pin delivery
  // before the matching grant.
  REQUIRE_FALSE(lost->evokeCallback(0.0));
  REQUIRE(lostReports.faultDescriptions == std::vector<std::wstring>{faultDescription});
  REQUIRE_NOTHROW(lost->connect(lostReports, HLA_EVOKED));
  REQUIRE_NOTHROW(lost->disconnect());
  REQUIRE_NOTHROW(surviving->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(surviving->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(surviving->disconnect());
}

}
