#include "ieee1516_2025_federation_management_test_support.hpp"
#include "ieee1516_2025_federation_management_fixture_support.hpp"
#include "ieee1516_2025_public_federation_restore_test_support.hpp"

namespace {
TEST_CASE(
    "Embedded service reporting preserves Federate Restore Complete success-indicator forms",
    "[integration][development-profile][federation-management][save-restore]"
    "[mom][service-report-file][service-reporting][federate-restore-complete]"
    "[federate-restore-not-complete][rti.service.request-federation-save]"
    "[rti.service.federate-save-begun][rti.service.federate-save-complete]"
    "[rti.service.request-federation-restore][rti.service.federate-restore-complete]"
    "[rti.service.federate-restore-not-complete][federate.callback.initiate-federate-save]"
    "[federate.callback.request-federation-restore-succeeded]"
    "[federate.callback.federation-restored][federate.callback.federation-not-restored]") {
  ReportingFederateAmbassador reports;
  auto rti = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule =
      (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data" /
       "switch-support-enabled-fom.xml")
          .wstring();
  auto directory = temporaryServiceReportDirectory();
  auto configuration = configurationForServiceReportDirectory(directory.path());
  configuration.withRtiAddress(L"in-process");

  REQUIRE_NOTHROW(rti->connect(reports, HLA_EVOKED, configuration));
  REQUIRE_NOTHROW(
      rti->createFederationExecution(federationName, fomModule, standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(
      rti->joinFederationExecution(L"restore-complete-report-subject", L"subject", federationName));
  auto const files = serviceReportFiles(directory.path());
  REQUIRE(files.size() == 1U);
  auto const reportFile = files.front();
  auto const initialText = readTextFile(reportFile);
  auto const reportDesignator = joinedFederateReportDesignator(reportFile);

  // §4.31 has no successful report before the joined federate has been
  // directed to restore.  A rejected selector must not consume serial zero.
  REQUIRE_THROWS_AS(rti->federateRestoreComplete(), rti1516_2025::RestoreNotRequested);
  REQUIRE(readTextFile(reportFile) == initialText);

  // Make one real snapshot. Its Request, Initiate, Begun, Complete, and
  // Federation Saved records establish serial 5 for the first restore request;
  // this lane confines its exact
  // assertions to the §4.31 selectors that follow.
  REQUIRE_NOTHROW(rti->requestFederationSave(L"restore-complete-report-snapshot"));
  REQUIRE_FALSE(rti->evokeCallback(0.0));
  REQUIRE(reports.initiateFederateSaveReports ==
          std::vector<std::wstring>{L"restore-complete-report-snapshot"});
  REQUIRE_NOTHROW(rti->federateSaveBegun());
  REQUIRE_NOTHROW(rti->federateSaveComplete());
  REQUIRE_FALSE(rti->evokeCallback(0.0));
  REQUIRE(reports.federationSavedReportCount == 1U);
  auto const savedText = readTextFile(reportFile);

  REQUIRE_NOTHROW(rti->requestFederationRestore(L"restore-complete-report-snapshot"));
  auto const expectedFirstRequest =
      R"({"HLAserialNumber":5,"HLAreturnedArgument":[null],"HLAservice":"RequestFederationRestore","HLAsuppliedArguments":[{"HLAargumentType":53,"HLAargumentName":"Federation save label","HLAargumentValue":"restore-complete-report-snapshot"}],"HLAsuccessIndicator":true,"HLAexception":null})";
  auto const expectedFirstConfirmation =
      R"({"HLAserialNumber":6,"HLAreturnedArgument":[null],"HLAservice":"ConfirmFederationRestorationRequest","HLAsuppliedArguments":[{"HLAargumentType":53,"HLAargumentName":"Federation save label","HLAargumentValue":"restore-complete-report-snapshot"},{"HLAargumentType":6,"HLAargumentName":"Request-success indicator","HLAargumentValue":true}],"HLAsuccessIndicator":true,"HLAexception":null})";
  auto const expectedFirstRestoreBegun =
      R"({"HLAserialNumber":7,"HLAreturnedArgument":[null],"HLAservice":"FederationRestoreBegun","HLAsuppliedArguments":[],"HLAsuccessIndicator":true,"HLAexception":null})";
  auto const expectedFirstInitiateRestore = initiateFederateRestoreServiceReportRecord(
      8U,
      "restore-complete-report-snapshot",
      reportDesignator,
      "restore-complete-report-subject");
  REQUIRE(readTextFile(reportFile) ==
          savedText + expectedFirstRequest + expectedFirstConfirmation + expectedFirstRestoreBegun +
              expectedFirstInitiateRestore);
  while (rti->evokeCallback(0.0)) {
  }
  REQUIRE(reports.requestFederationRestoreSucceededReports ==
          std::vector<std::wstring>{L"restore-complete-report-snapshot"});

  REQUIRE_NOTHROW(rti->federateRestoreComplete());
  auto const expectedSuccess =
      R"({"HLAserialNumber":9,"HLAreturnedArgument":[null],"HLAservice":"FederateRestoreComplete","HLAsuppliedArguments":[{"HLAargumentType":6,"HLAargumentName":"Federate restore-success indicator","HLAargumentValue":true}],"HLAsuccessIndicator":true,"HLAexception":null})";
  REQUIRE(readTextFile(reportFile) ==
          savedText + expectedFirstRequest + expectedFirstConfirmation + expectedFirstRestoreBegun +
              expectedFirstInitiateRestore + expectedSuccess);
  REQUIRE(reports.federationRestoredReportCount == 0U);
  REQUIRE_FALSE(rti->evokeCallback(0.0));
  REQUIRE(reports.federationRestoredReportCount == 1U);

  // A later request is a new service invocation, while the original joined
  // federate's report serial keeps advancing across the restored snapshot.
  REQUIRE_NOTHROW(rti->requestFederationRestore(L"restore-complete-report-snapshot"));
  auto const expectedSecondRequest =
      R"({"HLAserialNumber":10,"HLAreturnedArgument":[null],"HLAservice":"RequestFederationRestore","HLAsuppliedArguments":[{"HLAargumentType":53,"HLAargumentName":"Federation save label","HLAargumentValue":"restore-complete-report-snapshot"}],"HLAsuccessIndicator":true,"HLAexception":null})";
  auto const expectedSecondConfirmation =
      R"({"HLAserialNumber":11,"HLAreturnedArgument":[null],"HLAservice":"ConfirmFederationRestorationRequest","HLAsuppliedArguments":[{"HLAargumentType":53,"HLAargumentName":"Federation save label","HLAargumentValue":"restore-complete-report-snapshot"},{"HLAargumentType":6,"HLAargumentName":"Request-success indicator","HLAargumentValue":true}],"HLAsuccessIndicator":true,"HLAexception":null})";
  auto const expectedSecondRestoreBegun =
      R"({"HLAserialNumber":12,"HLAreturnedArgument":[null],"HLAservice":"FederationRestoreBegun","HLAsuppliedArguments":[],"HLAsuccessIndicator":true,"HLAexception":null})";
  auto const expectedSecondInitiateRestore = initiateFederateRestoreServiceReportRecord(
      13U,
      "restore-complete-report-snapshot",
      reportDesignator,
      "restore-complete-report-subject");
  REQUIRE(readTextFile(reportFile) ==
          savedText + expectedFirstRequest + expectedFirstConfirmation + expectedFirstRestoreBegun +
              expectedFirstInitiateRestore + expectedSuccess + expectedSecondRequest +
              expectedSecondConfirmation + expectedSecondRestoreBegun + expectedSecondInitiateRestore);
  while (rti->evokeCallback(0.0)) {
  }

  REQUIRE_NOTHROW(rti->federateRestoreNotComplete());
  auto const expectedFailure =
      R"({"HLAserialNumber":14,"HLAreturnedArgument":[null],"HLAservice":"FederateRestoreComplete","HLAsuppliedArguments":[{"HLAargumentType":6,"HLAargumentName":"Federate restore-success indicator","HLAargumentValue":false}],"HLAsuccessIndicator":true,"HLAexception":null})";
  REQUIRE(readTextFile(reportFile) ==
          savedText + expectedFirstRequest + expectedFirstConfirmation + expectedFirstRestoreBegun +
              expectedFirstInitiateRestore + expectedSuccess + expectedSecondRequest +
              expectedSecondConfirmation + expectedSecondRestoreBegun + expectedSecondInitiateRestore +
              expectedFailure);
  REQUIRE(reports.federationNotRestoredReasons.empty());
  REQUIRE_FALSE(rti->evokeCallback(0.0));
  REQUIRE(reports.federationNotRestoredReasons ==
          std::vector<RestoreFailureReason>{
              rti1516_2025::FEDERATE_REPORTED_FAILURE_DURING_RESTORE});

  REQUIRE_NOTHROW(rti->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(rti->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(rti->disconnect());
}
}  // namespace
