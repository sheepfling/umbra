#ifndef UMBRA_SOURCE_DIRECTORY
#error "The 2025 federation-management service-report tests require the Umbra source directory."
#endif

#include "ieee1516_2025_federation_management_fixture_support.hpp"

namespace {

using namespace rti1516_2025;

TEST_CASE(
    "Embedded service reporting preserves the supplied Next Message Request Available boundary",
    "[integration][development-profile][federation-management][interaction-management][mom]"
    "[service-report-file][service-reporting][time-management]"
    "[service-report-next-message-request-available-boundary]"
    "[next-message-request-available]"
    "[rti.service.next-message-request-available][rti.service.send-interaction]"
    "[rti.service.time-advance-request][rti.service.query-logical-time]"
    "[federate.callback.receive-interaction][federate.callback.time-advance-grant]") {
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
  auto receiverConfiguration = configurationForServiceReportDirectory(directory.path());
  receiverConfiguration.withRtiAddress(L"in-process");
  unsigned char const tagBytes[] = {0x4E, 0x4D, 0x52, 0x41};
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));

  REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED));
  REQUIRE_NOTHROW(receiver->connect(receiverReports, HLA_EVOKED, receiverConfiguration));
  REQUIRE_NOTHROW(
      publisher->createFederationExecution(federationName, fomModules, standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(publisher->joinFederationExecution(
      L"nmra-report-publisher", L"publisher", federationName));
  REQUIRE_NOTHROW(receiver->joinFederationExecution(
      L"nmra-report-receiver", L"subscriber", federationName));
  auto const files = serviceReportFiles(directory.path());
  REQUIRE(files.size() == 1U);
  auto const receiverReportFile = files.front();

  auto const interactionClass = publisher->getInteractionClassHandle(
      fixture_hla::fom::parameter_fixture_child_interaction);
  auto const identifier = publisher->getParameterHandle(interactionClass, fixture_hla::fixture::identifier);
  REQUIRE(interactionClass.isValid());
  REQUIRE(identifier.isValid());
  ParameterHandleValueMap parameterValues;
  unsigned char const identifierBytes[] = {0xE1, 0xF2};
  parameterValues.emplace(
      identifier,
      VariableLengthData(identifierBytes, sizeof(identifierBytes)));

  REQUIRE_NOTHROW(publisher->publishInteractionClass(interactionClass));
  REQUIRE_NOTHROW(publisher->changeInteractionOrderType(interactionClass, TIMESTAMP));
  REQUIRE_NOTHROW(receiver->subscribeInteractionClass(interactionClass));
  REQUIRE_NOTHROW(receiver->enableTimeConstrained());
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE_NOTHROW(publisher->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(5)));
  while (publisher->evokeCallback(0.0)) {
  }

  auto const handle = publisher->sendInteraction(
      interactionClass,
      parameterValues,
      tag,
      rti1516_2025::HLAinteger64Time(7));
  REQUIRE(handle.isValid());
  auto const textBeforeRequest = readTextFile(receiverReportFile);

  // §8.11 supplies 10 even though the Available-form request will later grant
  // at queued timestamp 7. The report preserves the requested boundary, not
  // the scheduler's effective target.
  REQUIRE_NOTHROW(
      receiver->nextMessageRequestAvailable(rti1516_2025::HLAinteger64Time(10)));
  auto const expectedRequest =
      R"({"HLAserialNumber":2,"HLAreturnedArgument":[null],"HLAservice":"NextMessageRequestAvailable","HLAsuppliedArguments":[{"HLAargumentType":31,"HLAargumentName":"Logical time","HLAargumentValue":"10"}],"HLAsuccessIndicator":true,"HLAexception":null})";
  REQUIRE(readTextFile(receiverReportFile) == textBeforeRequest + expectedRequest);
  rti1516_2025::HLAinteger64Time queriedTime;
  REQUIRE_NOTHROW(receiver->queryLogicalTime(queriedTime));
  REQUIRE(queriedTime.isInitial());
  REQUIRE(receiverReports.timeAdvanceGrantReports.empty());

  // The Available form accepts the queued timestamp at the inclusive GALT
  // boundary once the regulator advances to 2 with lookahead 5.
  REQUIRE_NOTHROW(publisher->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(2)));
  REQUIRE_FALSE(publisher->evokeCallback(0.0));
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE(receiverReports.timestampedInteractionReports.size() == 1U);
  REQUIRE(receiverReports.timeAdvanceGrantReports.size() == 1U);
  REQUIRE(receiverReports.timeAdvanceGrantReports.front().value == L"7");
  REQUIRE_NOTHROW(receiver->queryLogicalTime(queriedTime));
  REQUIRE(queriedTime.getTime() == 7);
  REQUIRE(readTextFile(receiverReportFile) == textBeforeRequest + expectedRequest);

  REQUIRE_NOTHROW(receiver->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(receiver->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
}

TEST_CASE(
    "Embedded service reporting records a rejected backward Next Message Request Available",
    "[integration][development-profile][time-management][mom][service-report-file]"
    "[service-reporting][service-failure][service-report-next-message-request-available-failure]"
    "[next-message-request-available][rti.service.next-message-request-available]"
    "[federate.callback.time-advance-grant][2025]") {
  ReportingFederateAmbassador reports;
  auto rti = makeRti();
  auto const federationName = nextFederationName();
  auto const interactionFom =
      (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data" /
       "parameter-handle-provider-fom.xml")
          .wstring();
  auto const switchFom =
      (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data" /
       "switch-support-enabled-fom.xml")
          .wstring();
  auto directory = temporaryServiceReportDirectory();
  auto configuration = configurationForServiceReportDirectory(directory.path());
  configuration.withRtiAddress(L"in-process");

  REQUIRE_NOTHROW(rti->connect(reports, HLA_EVOKED, configuration));
  REQUIRE_NOTHROW(rti->createFederationExecution(
      federationName,
      std::vector<std::wstring>{interactionFom, switchFom},
      standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(rti->joinFederationExecution(
      L"nmra-failure-report-member", L"subscriber", federationName));
  auto const files = serviceReportFiles(directory.path());
  REQUIRE(files.size() == 1U);
  auto const reportFile = files.front();

  REQUIRE_NOTHROW(rti->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(rti->setSendServiceReportsToFileSwitch(false));
  REQUIRE_NOTHROW(rti->timeAdvanceRequest(HLAinteger64Time(5)));
  REQUIRE_FALSE(rti->evokeCallback(0.0));
  REQUIRE(reports.timeAdvanceGrantReports.size() == 1U);
  REQUIRE(reports.timeAdvanceGrantReports.front().value == L"5");
  auto const textBeforeFailure = readTextFile(reportFile);
  REQUIRE_NOTHROW(rti->setServiceReportingSwitch(true));
  REQUIRE_NOTHROW(rti->setSendServiceReportsToFileSwitch(true));

  REQUIRE_THROWS_AS(
      rti->nextMessageRequestAvailable(HLAinteger64Time(4)),
      LogicalTimeAlreadyPassed);
  auto const expectedFailure =
      R"({"HLAserialNumber":0,"HLAreturnedArgument":[null],"HLAservice":"NextMessageRequestAvailable","HLAsuppliedArguments":[{"HLAargumentType":31,"HLAargumentName":"Logical time","HLAargumentValue":"4"}],"HLAsuccessIndicator":false,"HLAexception":"LogicalTimeAlreadyPassed: Next Message Request Available cannot move a joined federate backward in logical time."})";
  REQUIRE(readTextFile(reportFile) == textBeforeFailure + expectedFailure);
  REQUIRE(reports.timeAdvanceGrantReports.size() == 1U);
  HLAinteger64Time queriedTime;
  REQUIRE_NOTHROW(rti->queryLogicalTime(queriedTime));
  REQUIRE(queriedTime.getTime() == 5);

  REQUIRE_NOTHROW(rti->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(rti->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(rti->disconnect());
}

TEST_CASE(
    "Embedded service reporting records a pending-advance Next Message Request Available failure",
    "[integration][development-profile][time-management][mom][service-report-file]"
    "[service-reporting][service-failure][service-report-next-message-request-available-pending-failure]"
    "[next-message-request-available][rti.service.next-message-request-available]"
    "[federate.callback.time-advance-grant][2025]") {
  ReportingFederateAmbassador reports;
  auto rti = makeRti();
  auto const federationName = nextFederationName();
  auto const interactionFom =
      (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data" /
       "parameter-handle-provider-fom.xml")
          .wstring();
  auto const switchFom =
      (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data" /
       "switch-support-enabled-fom.xml")
          .wstring();
  auto directory = temporaryServiceReportDirectory();
  auto configuration = configurationForServiceReportDirectory(directory.path());
  configuration.withRtiAddress(L"in-process");

  REQUIRE_NOTHROW(rti->connect(reports, HLA_EVOKED, configuration));
  REQUIRE_NOTHROW(rti->createFederationExecution(
      federationName,
      std::vector<std::wstring>{interactionFom, switchFom},
      standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(rti->joinFederationExecution(
      L"nmra-pending-report-member", L"subscriber", federationName));
  auto const files = serviceReportFiles(directory.path());
  REQUIRE(files.size() == 1U);
  auto const reportFile = files.front();

  REQUIRE_NOTHROW(rti->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(rti->setSendServiceReportsToFileSwitch(false));
  REQUIRE_NOTHROW(rti->timeAdvanceRequest(HLAinteger64Time(5)));
  REQUIRE(reports.timeAdvanceGrantReports.empty());
  auto const textBeforeFailure = readTextFile(reportFile);
  REQUIRE_NOTHROW(rti->setServiceReportingSwitch(true));
  REQUIRE_NOTHROW(rti->setSendServiceReportsToFileSwitch(true));

  REQUIRE_THROWS_AS(
      rti->nextMessageRequestAvailable(HLAinteger64Time(6)),
      InTimeAdvancingState);
  auto const expectedFailure =
      R"({"HLAserialNumber":0,"HLAreturnedArgument":[null],"HLAservice":"NextMessageRequestAvailable","HLAsuppliedArguments":[{"HLAargumentType":31,"HLAargumentName":"Logical time","HLAargumentValue":"6"}],"HLAsuccessIndicator":false,"HLAexception":"InTimeAdvancingState: Next Message Request Available cannot run while a time-advance request is awaiting a grant."})";
  REQUIRE(readTextFile(reportFile) == textBeforeFailure + expectedFailure);
  REQUIRE(reports.timeAdvanceGrantReports.empty());
  HLAinteger64Time queriedTime;
  REQUIRE_NOTHROW(rti->queryLogicalTime(queriedTime));
  REQUIRE(queriedTime.getTime() == 0);
  REQUIRE_FALSE(rti->evokeCallback(0.0));
  REQUIRE(reports.timeAdvanceGrantReports.size() == 1U);
  REQUIRE(reports.timeAdvanceGrantReports.front().value == L"5");

  REQUIRE_NOTHROW(rti->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(rti->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(rti->disconnect());
}

}  // namespace
