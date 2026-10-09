#include "ieee1516_2025_federation_management_fixture_support.hpp"

namespace {

TEST_CASE(
    "Embedded transport loss delivers timestamped interactions before the lost federate's last-known time",
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
  unsigned char const tagBytes[] = {0x4C, 0x45, 0x53, 0x53};
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));

  REQUIRE_NOTHROW(lost->connect(lostReports, HLA_EVOKED));
  REQUIRE_NOTHROW(surviving->connect(survivingReports, HLA_EVOKED));
  REQUIRE_NOTHROW(lost->createFederationExecution(
      federationName,
      fomModule,
      standard_hla::mom::integer64_time));
  FederateHandle lostFederate;
  REQUIRE_NOTHROW(lostFederate = lost->joinFederationExecution(
      L"less-cutoff-lost-publisher", L"publisher", federationName));
  REQUIRE_NOTHROW(surviving->joinFederationExecution(
      L"less-cutoff-surviving-subscriber", L"subscriber", federationName));

  auto const interactionClass = lost->getInteractionClassHandle(
      fixture_hla::fom::parameter_fixture_child_interaction);
  auto const identifier = lost->getParameterHandle(interactionClass, fixture_hla::fixture::identifier);
  REQUIRE(interactionClass.isValid());
  REQUIRE(identifier.isValid());
  ParameterHandleValueMap parameterValues;
  unsigned char const identifierBytes[] = {0x5A, 0x11};
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
  REQUIRE(lostReports.timeRegulationEnabledReports.size() == 1U);

  // The TSO payload is strictly before the eventual loss boundary.  The
  // recipient nevertheless requests time 6, so it cannot receive the time-5
  // interaction before the regulating publisher reaches the boundary.
  auto const retraction = lost->sendInteraction(
      interactionClass,
      parameterValues,
      tag,
      rti1516_2025::HLAinteger64Time(5));
  REQUIRE(retraction.isValid());
  REQUIRE_NOTHROW(surviving->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(6)));
  REQUIRE_NOTHROW(lost->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(6)));
  REQUIRE_FALSE(lost->evokeCallback(0.0));
  REQUIRE(lostReports.timeAdvanceGrantReports.size() == 1U);
  REQUIRE(lostReports.timeAdvanceGrantReports.front().value == L"6");
  REQUIRE(survivingReports.timestampedInteractionReports.empty());
  REQUIRE(survivingReports.timeAdvanceGrantReports.empty());

  std::wstring const faultDescription = L"strictly-before cutoff transport fault";
  REQUIRE(umbra::detail::failEmbeddedTransportConnectionForTesting(
      *lost,
      faultDescription));
  REQUIRE(survivingReports.timestampedInteractionReports.empty());

  REQUIRE_FALSE(surviving->evokeCallback(0.0));
  REQUIRE(survivingReports.timestampedInteractionReports.size() == 1U);
  REQUIRE(survivingReports.timeAdvanceGrantReports.size() == 1U);
  REQUIRE(
      survivingReports.callbackOrder == std::vector<std::string>{"interaction", "grant"});
  auto const& report = survivingReports.timestampedInteractionReports.front();
  REQUIRE(report.interactionClass == interactionClass);
  REQUIRE(report.producingFederate == lostFederate);
  REQUIRE(variableLengthDataBytes(report.userSuppliedTag) ==
          variableLengthDataBytes(tag));
  REQUIRE(report.timeImplementationName == standard_hla::mom::integer64_time);
  REQUIRE(report.timeValue == L"5");
  REQUIRE(report.sentOrderType == TIMESTAMP);
  REQUIRE(report.receivedOrderType == TIMESTAMP);
  REQUIRE(report.retractionSupplied);
  REQUIRE(report.retractionValid);
  REQUIRE(report.parameterValues.size() == 1U);
  REQUIRE(variableLengthDataBytes(report.parameterValues.at(identifier)) ==
          variableLengthDataBytes(parameterValues.at(identifier)));
  REQUIRE(survivingReports.timeAdvanceGrantReports.front().value == L"6");

  REQUIRE_FALSE(lost->evokeCallback(0.0));
  REQUIRE(lostReports.faultDescriptions == std::vector<std::wstring>{faultDescription});
  REQUIRE_THROWS_AS(lost->disconnect(), rti1516_2025::NotConnected);

  REQUIRE_NOTHROW(lost->connect(lostReports, HLA_EVOKED));
  REQUIRE_NOTHROW(lost->disconnect());
  REQUIRE_NOTHROW(surviving->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(surviving->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(surviving->disconnect());
}

}
