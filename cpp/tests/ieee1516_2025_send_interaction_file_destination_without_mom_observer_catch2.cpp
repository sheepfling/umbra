#include "ieee1516_2025_federation_management_fixture_support.hpp"

namespace {
TEST_CASE(
    "Embedded Send Interaction reports to file when no MOM observer subscribes",
    "[integration][development-profile][interaction-management][mom]"
    "[service-report-file][service-report-destination-selection]"
    "[send-service-reports-file-switch-on-without-mom-observer]"
    "[rti.service.send-interaction][rti.service.set-service-reporting-switch]"
    "[rti.service.set-send-service-reports-to-file-switch]"
    "[federate.callback.receive-interaction]") {
  ReportingFederateAmbassador publisherReports;
  ReportingFederateAmbassador receiverReports;
  ReportingFederateAmbassador nonObserverReports;
  auto publisher = makeRti();
  auto receiver = makeRti();
  auto nonObserver = makeRti();
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
  auto reportDirectory = temporaryServiceReportDirectory();
  auto configuration = configurationForServiceReportDirectory(reportDirectory.path());
  configuration.withRtiAddress(L"in-process");
  unsigned char const valueBytes[] = {0x41, 0x42};
  unsigned char const tagBytes[] = {'f', 'i', 'l', 'e'};
  VariableLengthData const value(valueBytes, sizeof(valueBytes));
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));

  REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED, configuration));
  REQUIRE_NOTHROW(receiver->connect(receiverReports, HLA_EVOKED, configuration));
  REQUIRE_NOTHROW(nonObserver->connect(
      nonObserverReports, rti1516_2025::HLA_IMMEDIATE, configuration));
  REQUIRE_NOTHROW(publisher->createFederationExecution(
      federationName, fomModules, standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(publisher->joinFederationExecution(
      L"file-destination-publisher", L"publisher", federationName));
  REQUIRE_NOTHROW(receiver->joinFederationExecution(
      L"file-destination-receiver", L"receiver", federationName));
  REQUIRE_NOTHROW(nonObserver->joinFederationExecution(
      L"file-destination-non-observer", L"observer", federationName));

  // Keep join/setup records out of the accepted Send Interaction report.
  REQUIRE_NOTHROW(publisher->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(publisher->setSendServiceReportsToFileSwitch(false));
  REQUIRE_NOTHROW(receiver->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(receiver->setSendServiceReportsToFileSwitch(false));
  REQUIRE_NOTHROW(nonObserver->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(nonObserver->setSendServiceReportsToFileSwitch(false));

  auto const interactionClass = publisher->getInteractionClassHandle(
      fixture_hla::fom::parameter_fixture_child_interaction);
  auto const receiverInteractionClass = receiver->getInteractionClassHandle(
      fixture_hla::fom::parameter_fixture_child_interaction);
  auto const valueParameter = publisher->getParameterHandle(
      interactionClass, fixture_hla::fixture::identifier);
  REQUIRE(interactionClass.isValid());
  REQUIRE(receiverInteractionClass.isValid());
  REQUIRE(valueParameter.isValid());
  REQUIRE_NOTHROW(publisher->publishInteractionClass(interactionClass));
  REQUIRE_NOTHROW(receiver->subscribeInteractionClass(receiverInteractionClass));

  // The third federate deliberately does not subscribe to
  // HLAreportServiceInvocation. Its presence makes the absent MOM route
  // explicit while the publisher selects file reporting.
  auto const reportClass = nonObserver->getInteractionClassHandle(
      standard_hla::mom::report_service_invocation);
  REQUIRE(reportClass.isValid());
  auto const reportFiles = serviceReportFiles(reportDirectory.path());
  REQUIRE(reportFiles.size() == 3U);
  auto const publisherFile = std::find_if(
      reportFiles.begin(), reportFiles.end(), [](std::filesystem::path const& path) {
        return path.filename().string().find("file-destination-publisher") !=
            std::string::npos;
      });
  REQUIRE(publisherFile != reportFiles.end());
  auto const initialContents = readTextFile(*publisherFile);

  REQUIRE_NOTHROW(publisher->setServiceReportingSwitch(true));
  REQUIRE_NOTHROW(publisher->setSendServiceReportsToFileSwitch(true));
  REQUIRE_NOTHROW(publisher->sendInteraction(
      interactionClass,
      ParameterHandleValueMap{{valueParameter, value}},
      tag));

  auto const fileContents = readTextFile(*publisherFile);
  REQUIRE(fileContents != initialContents);
  REQUIRE(fileContents.find("\"HLAservice\":\"SendInteraction\"") !=
          std::string::npos);
  REQUIRE(nonObserverReports.interactionReports.empty());

  while (receiver->evokeCallback(0.0)) {
  }
  REQUIRE(receiverReports.interactionReports.size() == 1U);
  REQUIRE(receiverReports.interactionReports.front().interactionClass ==
          receiverInteractionClass);
  REQUIRE(nonObserverReports.interactionReports.empty());

  REQUIRE_NOTHROW(receiver->unsubscribeInteractionClass(receiverInteractionClass));
  REQUIRE_NOTHROW(receiver->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(nonObserver->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(nonObserver->disconnect());
  REQUIRE_NOTHROW(receiver->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
}
}  // namespace
