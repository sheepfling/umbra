#include "ieee1516_2025_federation_management_fixture_support.hpp"

namespace {
TEST_CASE(
    "Embedded service reporting records timestamped Send Interaction before interaction callback",
    "[integration][development-profile][federation-management][interaction-management]"
    "[time-management][tso][mom][service-report-file][service-reporting]"
    "[timestamped-interaction][timestamped-send-interaction-service-report]"
    "[rti.service.send-interaction][rti.service.set-service-reporting-switch]"
    "[rti.service.set-send-service-reports-to-file-switch]"
    "[federate.callback.receive-interaction]") {
  ReportingFederateAmbassador publisherReports;
  ReportingFederateAmbassador receiverReports;
  auto publisher = makeRti();
  auto receiver = makeRti();
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
  auto directory = temporaryServiceReportDirectory();
  auto configuration = configurationForServiceReportDirectory(directory.path());
  configuration.withRtiAddress(L"in-process");
  unsigned char const tagBytes[] = {'t', 's', 'o'};
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));
  unsigned char const identifierBytes[] = {0x01, 0x02};
  VariableLengthData const identifierValue(identifierBytes, sizeof(identifierBytes));
  rti1516_2025::HLAinteger64Time const timestamp(6);

  REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED, configuration));
  REQUIRE_NOTHROW(receiver->connect(receiverReports, HLA_EVOKED));
  REQUIRE_NOTHROW(publisher->createFederationExecution(
      federationName,
      fomModules,
      standard_hla::mom::integer64_time));
  FederateHandle publisherHandle;
  REQUIRE_NOTHROW(publisherHandle = publisher->joinFederationExecution(
      L"timestamped-send-file-publisher", L"publisher", federationName));
  REQUIRE_NOTHROW(receiver->joinFederationExecution(
      L"timestamped-send-file-receiver", L"receiver", federationName));

  // Keep setup calls out of the accepted timestamped service-report record.
  REQUIRE_NOTHROW(publisher->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(publisher->setSendServiceReportsToFileSwitch(false));

  auto const interactionClass = publisher->getInteractionClassHandle(
      fixture_hla::fom::parameter_fixture_child_interaction);
  auto const identifier = publisher->getParameterHandle(
      interactionClass,
      fixture_hla::fixture::identifier);
  REQUIRE(interactionClass.isValid());
  REQUIRE(identifier.isValid());
  REQUIRE_NOTHROW(publisher->publishInteractionClass(interactionClass));
  REQUIRE_NOTHROW(receiver->subscribeInteractionClass(interactionClass));

  auto const files = serviceReportFiles(directory.path());
  REQUIRE(files.size() == 1U);
  auto const reportFile = files.front();
  auto const initialText = readTextFile(reportFile);
  REQUIRE_NOTHROW(publisher->setServiceReportingSwitch(true));
  REQUIRE_NOTHROW(publisher->setSendServiceReportsToFileSwitch(true));
  REQUIRE(readTextFile(reportFile) == initialText);

  ParameterHandleValueMap const values{{identifier, identifierValue}};
  auto const retraction = publisher->sendInteraction(
      interactionClass,
      values,
      tag,
      timestamp);
  REQUIRE_FALSE(retraction.isValid());
  REQUIRE(receiverReports.timestampedInteractionReports.empty());

  auto asAscii = [](std::wstring const& text) {
    std::string result;
    result.reserve(text.size());
    for (wchar_t const character : text) {
      REQUIRE(character >= L' ');
      REQUIRE(character <= L'~');
      result.push_back(static_cast<char>(character));
    }
    return result;
  };
  auto const interactionValue = asAscii(interactionClass.toString());
  auto const parameterValue = asAscii(identifier.toString());
  auto const timestampValue = asAscii(timestamp.toString());
  auto const expectedRecord = std::string{
      R"({"HLAserialNumber":0,"HLAreturnedArgument":[{"HLAargumentType":34,"HLAargumentName":"","HLAargumentValue":null}],"HLAservice":"SendInteraction","HLAsuppliedArguments":[{"HLAargumentType":27,"HLAargumentName":"Interaction class designator","HLAargumentValue":")" +
      interactionValue +
      R"("},{"HLAargumentType":40,"HLAargumentName":"Constrained set of interaction parameter designator and value pairs","HLAargumentValue":{")" +
      parameterValue +
      R"(":"AQI="}},{"HLAargumentType":63,"HLAargumentName":"User-supplied tag","HLAargumentValue":"dHNv"},{"HLAargumentType":31,"HLAargumentName":"Optional timestamp","HLAargumentValue":")" +
      timestampValue + R"("}],"HLAsuccessIndicator":true,"HLAexception":null})"};

  // The accepted type-2 record is durable before HLA_EVOKED enters the
  // recipient's timestamped Receive Interaction callback.
  REQUIRE(readTextFile(reportFile) == initialText + expectedRecord);
  REQUIRE(serviceReportFiles(directory.path()) == files);

  // Switches gate future appends but never replace the joined federate's
  // report file or alter its existing record.
  REQUIRE_NOTHROW(publisher->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(publisher->setSendServiceReportsToFileSwitch(false));
  REQUIRE(serviceReportFiles(directory.path()) == files);
  REQUIRE(readTextFile(reportFile) == initialText + expectedRecord);
  REQUIRE_NOTHROW(publisher->setServiceReportingSwitch(true));
  REQUIRE_NOTHROW(publisher->setSendServiceReportsToFileSwitch(true));
  REQUIRE(serviceReportFiles(directory.path()) == files);
  REQUIRE(readTextFile(reportFile) == initialText + expectedRecord);

  bool reportPresentAtCallbackEntry = false;
  receiverReports.onTimestampedInteraction = [&] {
    reportPresentAtCallbackEntry =
        readTextFile(reportFile) == initialText + expectedRecord;
  };
  while (receiver->evokeCallback(0.0)) {
  }
  REQUIRE(reportPresentAtCallbackEntry);
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
  REQUIRE(readTextFile(reportFile) == initialText + expectedRecord);

  REQUIRE_NOTHROW(receiver->unsubscribeInteractionClass(interactionClass));
  REQUIRE_NOTHROW(publisher->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(receiver->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(
      CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(receiver->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
}
}
