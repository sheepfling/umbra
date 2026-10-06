#include "ieee1516_2025_federation_management_test_support.hpp"
#include "ieee1516_2025_federation_management_fixture_support.hpp"
#include "ieee1516_2025_public_federation_restore_test_support.hpp"

namespace {
TEST_CASE(
    "Embedded service reporting appends a Table 5 void-service record to the selected filesystem sink",
    "[integration][development-profile][federation-management][mom][service-report-file]"
    "[service-reporting][support-switches]"
    "[rti.service.set-exception-reporting-switch]"
    "[rti.service.set-service-reporting-switch]"
    "[rti.service.subscribe-interaction-class][federate.callback.receive-interaction]") {
  TestFederateAmbassador subjectReports;
  ReportingFederateAmbassador observerReports;
  auto subject = makeRti();
  auto observer = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule =
      (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data" /
       "switch-support-enabled-fom.xml")
          .wstring();
  auto directory = public_federation_restore_test_support::temporaryServiceReportDirectory();
  auto configuration =
      public_federation_restore_test_support::configurationForServiceReportDirectory(directory.path());
  configuration.withRtiAddress(L"in-process");

  REQUIRE_NOTHROW(subject->connect(subjectReports, HLA_EVOKED, configuration));
  REQUIRE_NOTHROW(observer->connect(observerReports, HLA_EVOKED, configuration));
  REQUIRE_NOTHROW(
      subject->createFederationExecution(federationName, fomModule, standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(subject->joinFederationExecution(
      L"void-report-subject", L"subject", federationName));
  REQUIRE_NOTHROW(observer->joinFederationExecution(
      L"void-report-observer", L"observer", federationName));

  // The observer is permitted to subscribe only after turning off its own
  // service reporting.  The subject retains the FOM's service/file switches,
  // so its following successful void service must select its unique file,
  // not this otherwise matching subscriber.
  REQUIRE_NOTHROW(observer->setServiceReportingSwitch(false));
  auto const reportClass = observer->getInteractionClassHandle(
      standard_hla::mom::report_service_invocation);
  REQUIRE_NOTHROW(observer->subscribeInteractionClass(reportClass, false));

  auto const files = public_federation_restore_test_support::serviceReportFiles(directory.path());
  REQUIRE(files.size() == 2U);
  auto const subjectFile = std::find_if(
      files.begin(),
      files.end(),
      [](std::filesystem::path const& candidate) {
        return public_federation_restore_test_support::readTextFile(candidate).find(
                   "\"HLAfederateName\":\"void-report-subject\"") != std::string::npos;
      });
  REQUIRE(subjectFile != files.end());
  auto const initialText = public_federation_restore_test_support::readTextFile(*subjectFile);

  REQUIRE_NOTHROW(subject->setExceptionReportingSwitch(false));

  // Table 5's explicit successful-void shape uses [null] for the returned
  // argument.  This test is intentionally not evidence for generic returned
  // values, failures, or the deferred interaction-delivery branch.
  auto const expectedRecord =
      R"({"HLAserialNumber":0,"HLAreturnedArgument":[null],"HLAservice":"SetExceptionReportingSwitch","HLAsuppliedArguments":[{"HLAargumentType":6,"HLAargumentName":"SwitchValue","HLAargumentValue":false}],"HLAsuccessIndicator":true,"HLAexception":null})";
  REQUIRE(public_federation_restore_test_support::readTextFile(*subjectFile) ==
          initialText + expectedRecord);
  REQUIRE_FALSE(observer->evokeCallback(0.0));
  REQUIRE(observerReports.interactionReports.empty());

  REQUIRE_NOTHROW(observer->unsubscribeInteractionClass(reportClass));
  REQUIRE_NOTHROW(observer->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(subject->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(subject->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(observer->disconnect());
  REQUIRE_NOTHROW(subject->disconnect());
}
}  // namespace
