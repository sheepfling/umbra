#include "ieee1516_2025_federation_management_fixture_support.hpp"

namespace {
TEST_CASE(
    "Embedded service reporting delivers timestamped Send Interaction through MOM interaction",
    "[integration][development-profile][federation-management][interaction-management]"
    "[time-management][tso][mom][service-reporting][service-report-interaction]"
    "[timestamped-send-interaction-service-report-interaction]"
    "[rti.service.timestamped-send-interaction-service-report-interaction]"
    "[rti.service.send-interaction][rti.service.set-service-reporting-switch]"
    "[rti.service.set-send-service-reports-to-file-switch]"
    "[rti.service.subscribe-interaction-class]"
    "[federate.callback.receive-interaction]") {
  ReportingFederateAmbassador publisherReports;
  ReportingFederateAmbassador receiverReports;
  ReportingFederateAmbassador observerReports;
  auto publisher = makeRti();
  auto receiver = makeRti();
  auto observer = makeRti();
  auto const federationName = nextFederationName();
  auto const interactionFom =
      (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data" /
       "parameter-handle-provider-fom.xml")
          .wstring();
  auto const switchFom =
      (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data" /
       "switch-support-enabled-fom.xml")
          .wstring();
  std::vector<std::wstring> const fomModules{interactionFom, switchFom};
  unsigned char const tagBytes[] = {'t', 's', 'o'};
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));
  unsigned char const identifierBytes[] = {0x01, 0x02};
  VariableLengthData const identifierValue(identifierBytes, sizeof(identifierBytes));
  rti1516_2025::HLAinteger64Time const timestamp(6);

  REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED));
  REQUIRE_NOTHROW(receiver->connect(receiverReports, HLA_EVOKED));
  REQUIRE_NOTHROW(observer->connect(observerReports, rti1516_2025::HLA_IMMEDIATE));
  REQUIRE_NOTHROW(publisher->createFederationExecution(
      federationName,
      fomModules,
      standard_hla::mom::integer64_time));
  FederateHandle publisherHandle;
  REQUIRE_NOTHROW(publisherHandle = publisher->joinFederationExecution(
      L"timestamped-send-mom-publisher", L"publisher", federationName));
  REQUIRE_NOTHROW(receiver->joinFederationExecution(
      L"timestamped-send-mom-receiver", L"receiver", federationName));
  REQUIRE_NOTHROW(observer->joinFederationExecution(
      L"timestamped-send-mom-observer", L"observer", federationName));

  // Keep setup calls out of the accepted timestamped service-report record.
  REQUIRE_NOTHROW(publisher->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(publisher->setSendServiceReportsToFileSwitch(false));
  REQUIRE_NOTHROW(receiver->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(receiver->setSendServiceReportsToFileSwitch(false));
  REQUIRE_NOTHROW(observer->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(observer->setSendServiceReportsToFileSwitch(false));

  auto const reportClass = observer->getInteractionClassHandle(
      standard_hla::mom::report_service_invocation);
  REQUIRE(reportClass.isValid());
  std::vector<ParameterHandle> reportParameters;
  for (auto const& name : {
           standard_hla::mom::service,
           standard_hla::mom::service_type,
           standard_hla::mom::success_indicator,
           standard_hla::mom::supplied_arguments,
           standard_hla::mom::returned_argument,
           standard_hla::mom::exception,
           standard_hla::mom::serial_number}) {
    auto const parameter = observer->getParameterHandle(reportClass, name);
    REQUIRE(parameter.isValid());
    reportParameters.push_back(parameter);
  }
  REQUIRE_NOTHROW(observer->subscribeInteractionClass(reportClass));

  auto const interactionClass = publisher->getInteractionClassHandle(
      fixture_hla::fom::parameter_fixture_child_interaction);
  auto const identifier = publisher->getParameterHandle(
      interactionClass,
      fixture_hla::fixture::identifier);
  REQUIRE(interactionClass.isValid());
  REQUIRE(identifier.isValid());
  REQUIRE_NOTHROW(publisher->publishInteractionClass(interactionClass));
  REQUIRE_NOTHROW(receiver->subscribeInteractionClass(interactionClass));

  REQUIRE_NOTHROW(publisher->setServiceReportingSwitch(true));
  REQUIRE_FALSE(publisher->getSendServiceReportsToFileSwitch());
  ParameterHandleValueMap const values{{identifier, identifierValue}};
  auto const retraction = publisher->sendInteraction(
      interactionClass,
      values,
      tag,
      timestamp);
  REQUIRE_FALSE(retraction.isValid());
  REQUIRE(observerReports.interactionReports.size() == 1U);
  REQUIRE(receiverReports.timestampedInteractionReports.empty());
  auto const& report = observerReports.interactionReports.front();
  REQUIRE(report.interactionClass == reportClass);
  REQUIRE(report.parameterValues.size() == 8U);
  REQUIRE(report.userSuppliedTag.size() == 0U);
  REQUIRE(report.transportationType ==
          observer->getTransportationTypeHandle(standard_hla::mom::reliable));
  REQUIRE_FALSE(report.producingFederate.isValid());
  REQUIRE_FALSE(report.sentRegionsSupplied);

  rti1516_2025::HLAunicodeString decodedService;
  REQUIRE_NOTHROW(decodedService.decode(report.parameterValues.at(reportParameters[0])));
  REQUIRE(decodedService.get() == L"SendInteraction");
  rti1516_2025::HLAinteger16BE decodedServiceType;
  REQUIRE_NOTHROW(decodedServiceType.decode(report.parameterValues.at(reportParameters[1])));
  REQUIRE(decodedServiceType.get() == 2);
  rti1516_2025::HLAboolean decodedSuccess;
  REQUIRE_NOTHROW(decodedSuccess.decode(report.parameterValues.at(reportParameters[2])));
  REQUIRE(decodedSuccess.get());

  rti1516_2025::HLAfixedRecord argumentPrototype;
  argumentPrototype.appendElement(rti1516_2025::HLAinteger32BE{})
      .appendElement(rti1516_2025::HLAunicodeString{})
      .appendElement(rti1516_2025::HLAunicodeString{});
  rti1516_2025::HLAvariableArray suppliedArguments{argumentPrototype};
  REQUIRE_NOTHROW(
      suppliedArguments.decode(report.parameterValues.at(reportParameters[3])));
  REQUIRE(suppliedArguments.size() == 4U);
  auto const verifyArgument = [&](std::size_t argumentIndex,
                                  std::int32_t type,
                                  std::wstring const& name,
                                  std::wstring const& value) {
    auto const& supplied = dynamic_cast<rti1516_2025::HLAfixedRecord const&>(
        suppliedArguments.get(argumentIndex));
    REQUIRE(dynamic_cast<rti1516_2025::HLAinteger32BE const&>(supplied.get(0U)).get() == type);
    REQUIRE(dynamic_cast<rti1516_2025::HLAunicodeString const&>(supplied.get(1U)).get() == name);
    REQUIRE(dynamic_cast<rti1516_2025::HLAunicodeString const&>(supplied.get(2U)).get() == value);
  };
  verifyArgument(
      0U,
      27,
      L"Interaction class designator",
      L"\"" + interactionClass.toString() + L"\"");
  verifyArgument(
      1U,
      40,
      L"Constrained set of interaction parameter designator and value pairs",
      L"{\"" + identifier.toString() + L"\":\"AQI=\"}");
  verifyArgument(2U, 60, L"User-supplied tag", L"\"dHNv\"");
  verifyArgument(3U, 31, L"Optional timestamp", L"\"" + timestamp.toString() + L"\"");

  rti1516_2025::HLAfixedRecord returnedArgument;
  returnedArgument.appendElement(rti1516_2025::HLAinteger32BE{})
      .appendElement(rti1516_2025::HLAunicodeString{})
      .appendElement(rti1516_2025::HLAunicodeString{});
  REQUIRE_NOTHROW(
      returnedArgument.decode(report.parameterValues.at(reportParameters[4])));
  REQUIRE(dynamic_cast<rti1516_2025::HLAinteger32BE const&>(returnedArgument.get(0U)).get() ==
          34);
  REQUIRE(dynamic_cast<rti1516_2025::HLAunicodeString const&>(returnedArgument.get(1U)).get().empty());
  REQUIRE(dynamic_cast<rti1516_2025::HLAunicodeString const&>(returnedArgument.get(2U)).get() ==
          L"null");
  rti1516_2025::HLAunicodeString decodedException;
  REQUIRE_NOTHROW(decodedException.decode(report.parameterValues.at(reportParameters[5])));
  REQUIRE(decodedException.get().empty());
  rti1516_2025::HLAinteger32BE decodedSerial;
  REQUIRE_NOTHROW(decodedSerial.decode(report.parameterValues.at(reportParameters[6])));
  REQUIRE(decodedSerial.get() == 0);

  while (receiver->evokeCallback(0.0)) {
  }
  REQUIRE(receiverReports.timestampedInteractionReports.size() == 1U);
  auto const& interaction = receiverReports.timestampedInteractionReports.front();
  REQUIRE(interaction.interactionClass == interactionClass);
  REQUIRE(interaction.parameterValues.size() == 1U);
  REQUIRE(interaction.parameterValues.contains(identifier));
  REQUIRE(variableLengthDataBytes(interaction.parameterValues.at(identifier)) ==
          std::vector<unsigned char>(identifierBytes, identifierBytes + sizeof(identifierBytes)));
  REQUIRE(variableLengthDataBytes(interaction.userSuppliedTag) ==
          std::vector<unsigned char>(tagBytes, tagBytes + sizeof(tagBytes)));
  REQUIRE(interaction.transportationType ==
          receiver->getTransportationTypeHandle(standard_hla::mom::reliable));
  REQUIRE(interaction.producingFederate == publisherHandle);
  REQUIRE_FALSE(interaction.sentRegionsSupplied);
  REQUIRE(interaction.sentRegions.empty());
  REQUIRE(interaction.timeImplementationName == standard_hla::mom::integer64_time);
  REQUIRE(interaction.timeValue == timestamp.toString());
  REQUIRE(interaction.sentOrderType == RECEIVE);
  REQUIRE(interaction.receivedOrderType == RECEIVE);
  REQUIRE_FALSE(interaction.retractionSupplied);
  REQUIRE_FALSE(interaction.retractionValid);

  REQUIRE_NOTHROW(observer->unsubscribeInteractionClass(reportClass));
  REQUIRE_NOTHROW(receiver->unsubscribeInteractionClass(interactionClass));
  REQUIRE_NOTHROW(publisher->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(receiver->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(observer->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(observer->disconnect());
  REQUIRE_NOTHROW(receiver->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
}
}
