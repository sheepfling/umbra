#include "ieee1516_2025_federation_management_fixture_support.hpp"

namespace {

TEST_CASE(
    "Embedded transport loss schedules cutoff TSO delivery requested after the loss",
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
  unsigned char const tagBytes[] = {0x46, 0x55, 0x54, 0x55, 0x52, 0x45};
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));

  REQUIRE_NOTHROW(lost->connect(lostReports, HLA_EVOKED));
  REQUIRE_NOTHROW(surviving->connect(survivingReports, HLA_EVOKED));
  REQUIRE_NOTHROW(lost->createFederationExecution(
      federationName,
      fomModule,
      standard_hla::mom::integer64_time));
  FederateHandle lostFederate;
  REQUIRE_NOTHROW(lostFederate = lost->joinFederationExecution(
      L"later-cutoff-lost-publisher", L"publisher", federationName));
  REQUIRE_NOTHROW(surviving->joinFederationExecution(
      L"later-cutoff-surviving-subscriber", L"subscriber", federationName));

  auto const interactionClass = lost->getInteractionClassHandle(
      fixture_hla::fom::parameter_fixture_child_interaction);
  auto const identifier = lost->getParameterHandle(interactionClass, fixture_hla::fixture::identifier);
  REQUIRE(interactionClass.isValid());
  REQUIRE(identifier.isValid());
  ParameterHandleValueMap parameterValues;
  unsigned char const identifierBytes[] = {0xA6, 0xA7};
  parameterValues.emplace(
      identifier,
      VariableLengthData(identifierBytes, sizeof(identifierBytes)));

  REQUIRE_NOTHROW(lost->publishInteractionClass(interactionClass));
  REQUIRE_NOTHROW(lost->changeInteractionOrderType(interactionClass, TIMESTAMP));
  REQUIRE_NOTHROW(surviving->subscribeInteractionClass(interactionClass));
  REQUIRE_NOTHROW(surviving->enableTimeConstrained());
  REQUIRE_FALSE(surviving->evokeCallback(0.0));
  REQUIRE_NOTHROW(lost->enableTimeRegulation(rti1516_2025::HLAinteger64Interval(5)));
  while (lost->evokeCallback(0.0)) {
  }

  auto const retraction = lost->sendInteraction(
      interactionClass,
      parameterValues,
      tag,
      rti1516_2025::HLAinteger64Time(6));
  REQUIRE(retraction.isValid());
  REQUIRE_NOTHROW(lost->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(6)));
  REQUIRE_FALSE(lost->evokeCallback(0.0));
  REQUIRE(lostReports.timeAdvanceGrantReports.size() == 1U);
  REQUIRE(lostReports.timeAdvanceGrantReports.front().value == L"6");

  // No subscriber advance exists when the publisher leaves. The same
  // at-or-before-loss obligation must remain until a later TAR establishes
  // the recipient's callback boundary.
  std::wstring const faultDescription = L"later timestamped cutoff transport fault";
  REQUIRE(umbra::detail::failEmbeddedTransportConnectionForTesting(
      *lost,
      faultDescription));
  REQUIRE(survivingReports.timestampedInteractionReports.empty());
  REQUIRE_NOTHROW(surviving->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(6)));
  REQUIRE_FALSE(surviving->evokeCallback(0.0));

  REQUIRE(survivingReports.timestampedInteractionReports.size() == 1U);
  REQUIRE(survivingReports.timeAdvanceGrantReports.size() == 1U);
  REQUIRE(
      survivingReports.callbackOrder == std::vector<std::string>{"interaction", "grant"});
  auto const& report = survivingReports.timestampedInteractionReports.front();
  REQUIRE(report.interactionClass == interactionClass);
  REQUIRE(report.producingFederate == lostFederate);
  REQUIRE(report.timeImplementationName == standard_hla::mom::integer64_time);
  REQUIRE(report.timeValue == L"6");
  REQUIRE(report.sentOrderType == rti1516_2025::TIMESTAMP);
  REQUIRE(report.receivedOrderType == rti1516_2025::TIMESTAMP);
  REQUIRE(report.retractionSupplied);
  REQUIRE(report.retractionValid);
  REQUIRE(variableLengthDataBytes(report.userSuppliedTag) ==
          variableLengthDataBytes(tag));
  REQUIRE(report.parameterValues.size() == 1U);
  REQUIRE(variableLengthDataBytes(report.parameterValues.at(identifier)) ==
          variableLengthDataBytes(parameterValues.at(identifier)));
  REQUIRE(survivingReports.timeAdvanceGrantReports.front().value == L"6");

  REQUIRE_FALSE(lost->evokeCallback(0.0));
  REQUIRE(lostReports.faultDescriptions == std::vector<std::wstring>{faultDescription});
  REQUIRE_NOTHROW(lost->connect(lostReports, HLA_EVOKED));
  REQUIRE_NOTHROW(lost->disconnect());
  REQUIRE_NOTHROW(surviving->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(surviving->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(surviving->disconnect());
}

}
