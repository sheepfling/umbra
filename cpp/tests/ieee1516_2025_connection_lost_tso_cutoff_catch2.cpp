#include "ieee1516_2025_federation_management_fixture_support.hpp"

namespace {

TEST_CASE(
    "Embedded transport loss delivers timestamped interactions through the lost federate's last-known time",
    "[integration][development-profile][federation-management][transport]"
    "[interaction-management][time-management][connection-lost-tso-cutoff]"
    "[rti.service.connection-lost][rti.service.send-interaction]"
    "[rti.service.time-advance-request]"
    "[federate.callback.connection-lost][federate.callback.receive-interaction]"
    "[federate.callback.time-advance-grant]") {
  ReportingFederateAmbassador lostReports;
  ReportingFederateAmbassador survivingReports;
  auto lost = makeRti();
  auto surviving = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
                          "cpp" /
                          "tests" /
                          "data" /
                          "parameter-handle-provider-fom.xml")
                             .wstring();
  unsigned char const tagBytes[] = {0x4C, 0x4F, 0x53, 0x54};
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));

  REQUIRE_NOTHROW(lost->connect(lostReports, HLA_EVOKED));
  REQUIRE_NOTHROW(surviving->connect(survivingReports, HLA_EVOKED));
  REQUIRE_NOTHROW(lost->createFederationExecution(
      federationName,
      fomModule,
      standard_hla::mom::integer64_time));
  FederateHandle lostFederate;
  REQUIRE_NOTHROW(lostFederate = lost->joinFederationExecution(
      L"cutoff-lost-publisher", L"publisher", federationName));
  REQUIRE_NOTHROW(surviving->joinFederationExecution(
      L"cutoff-surviving-subscriber", L"subscriber", federationName));

  auto const interactionClass = lost->getInteractionClassHandle(
      fixture_hla::fom::parameter_fixture_child_interaction);
  auto const identifier = lost->getParameterHandle(interactionClass, fixture_hla::fixture::identifier);
  REQUIRE(interactionClass.isValid());
  REQUIRE(identifier.isValid());
  ParameterHandleValueMap parameterValues;
  unsigned char const identifierBytes[] = {0xC0, 0xFF, 0xEE};
  parameterValues.emplace(
      identifier,
      VariableLengthData(identifierBytes, sizeof(identifierBytes)));

  REQUIRE_NOTHROW(lost->publishInteractionClass(interactionClass));
  REQUIRE_NOTHROW(lost->changeInteractionOrderType(interactionClass, TIMESTAMP));
  REQUIRE_NOTHROW(surviving->subscribeInteractionClass(interactionClass));
  REQUIRE_NOTHROW(surviving->enableTimeConstrained());
  REQUIRE_FALSE(surviving->evokeCallback(0.0));
  REQUIRE(survivingReports.timeConstrainedEnabledReports.size() == 1U);
  REQUIRE_NOTHROW(lost->enableTimeRegulation(rti1516_2025::HLAinteger64Interval(5)));
  REQUIRE_FALSE(lost->evokeCallback(0.0));
  REQUIRE(lostReports.faultDescriptions.empty());

  // Queue a message at time 6, then advance the time-regulating publisher to
  // that same time.  The subscriber's matching TAR remains pending until the
  // producer grant establishes its last-known time position.
  auto const retraction = lost->sendInteraction(
      interactionClass,
      parameterValues,
      tag,
      rti1516_2025::HLAinteger64Time(6));
  REQUIRE(retraction.isValid());
  REQUIRE_NOTHROW(surviving->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(6)));
  REQUIRE_NOTHROW(lost->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(6)));
  REQUIRE_FALSE(lost->evokeCallback(0.0));
  REQUIRE(lostReports.faultDescriptions.empty());
  REQUIRE(lostReports.timeAdvanceGrantReports.size() == 1U);
  REQUIRE(lostReports.timeAdvanceGrantReports.front().value == L"6");
  REQUIRE(survivingReports.timestampedInteractionReports.empty());
  REQUIRE(survivingReports.timeAdvanceGrantReports.empty());

  // IEEE 1516.1-2025 4.4 requires delivery to remaining subscribers for all
  // TSO messages at or before the lost time-regulating federate's last known
  // time.  A message after that cutoff is deliberately not asserted because
  // the standard permits either outcome there.
  std::wstring const faultDescription = L"timestamped cutoff transport fault";
  REQUIRE(umbra::detail::failEmbeddedTransportConnectionForTesting(
      *lost,
      faultDescription));
  REQUIRE(survivingReports.timestampedInteractionReports.empty());

  REQUIRE_FALSE(surviving->evokeCallback(0.0));
  REQUIRE(survivingReports.timeAdvanceGrantReports.size() == 1U);
  REQUIRE(survivingReports.timestampedInteractionReports.size() == 1U);
  REQUIRE(
      survivingReports.callbackOrder == std::vector<std::string>{"interaction", "grant"});
  auto const& report = survivingReports.timestampedInteractionReports.front();
  REQUIRE(report.interactionClass == interactionClass);
  REQUIRE(report.producingFederate == lostFederate);
  REQUIRE(variableLengthDataBytes(report.userSuppliedTag) ==
          variableLengthDataBytes(tag));
  REQUIRE(report.timeImplementationName == standard_hla::mom::integer64_time);
  REQUIRE(report.timeValue == L"6");
  REQUIRE(report.sentOrderType == rti1516_2025::TIMESTAMP);
  REQUIRE(report.receivedOrderType == rti1516_2025::TIMESTAMP);
  REQUIRE(report.retractionSupplied);
  REQUIRE(report.retractionValid);
  REQUIRE(report.parameterValues.size() == 1U);
  REQUIRE(variableLengthDataBytes(report.parameterValues.at(identifier)) ==
          variableLengthDataBytes(parameterValues.at(identifier)));
  REQUIRE(survivingReports.timeAdvanceGrantReports.front().implementationName ==
          standard_hla::mom::integer64_time);
  REQUIRE(survivingReports.timeAdvanceGrantReports.front().value == L"6");

  // The queued message is consumed exactly once; the faulted endpoint itself
  // receives the official Connection Lost callback before it can reconnect.
  REQUIRE_FALSE(surviving->evokeCallback(0.0));
  REQUIRE(survivingReports.timestampedInteractionReports.size() == 1U);
  REQUIRE_FALSE(lost->evokeCallback(0.0));
  REQUIRE(lostReports.faultDescriptions == std::vector<std::wstring>{faultDescription});
  REQUIRE_THROWS_AS(lost->disconnect(), rti1516_2025::NotConnected);

  REQUIRE_NOTHROW(lost->connect(lostReports, HLA_EVOKED));
  REQUIRE_NOTHROW(lost->disconnect());
  REQUIRE_NOTHROW(surviving->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(surviving->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(surviving->disconnect());
}

} // namespace
