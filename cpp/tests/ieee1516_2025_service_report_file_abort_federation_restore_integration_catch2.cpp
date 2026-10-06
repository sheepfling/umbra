#include "ieee1516_2025_federation_management_test_support.hpp"
#include "ieee1516_2025_federation_management_fixture_support.hpp"
#include "ieee1516_2025_public_federation_restore_test_support.hpp"

namespace {
TEST_CASE(
    "Embedded service reporting records an accepted Abort Federation Restore invocation",
    "[integration][development-profile][federation-management][save-restore]"
    "[mom][service-report-file][service-reporting][abort-federation-restore]"
    "[rti.service.request-federation-save][rti.service.federate-save-begun]"
    "[rti.service.federate-save-complete][rti.service.request-federation-restore]"
    "[rti.service.abort-federation-restore][federate.callback.initiate-federate-save]"
    "[federate.callback.request-federation-restore-succeeded]"
    "[federate.callback.federation-not-restored]") {
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
      rti->joinFederationExecution(L"abort-restore-report-subject", L"subject", federationName));
  auto const files = serviceReportFiles(directory.path());
  REQUIRE(files.size() == 1U);
  auto const reportFile = files.front();
  auto const initialText = readTextFile(reportFile);
  auto const reportDesignator = joinedFederateReportDesignator(reportFile);

  // A pre-restore abort is rejected and must not reserve a report serial.
  // The accepted request below owns serial five after the real save snapshot.
  REQUIRE_THROWS_AS(rti->abortFederationRestore(), rti1516_2025::RestoreNotInProgress);
  REQUIRE(readTextFile(reportFile) == initialText);

  REQUIRE_NOTHROW(rti->requestFederationSave(L"abort-restore-report-snapshot"));
  REQUIRE_FALSE(rti->evokeCallback(0.0));
  REQUIRE(reports.initiateFederateSaveReports ==
          std::vector<std::wstring>{L"abort-restore-report-snapshot"});
  REQUIRE_NOTHROW(rti->federateSaveBegun());
  REQUIRE_NOTHROW(rti->federateSaveComplete());
  REQUIRE_FALSE(rti->evokeCallback(0.0));
  REQUIRE(reports.federationSavedReportCount == 1U);
  auto const savedText = readTextFile(reportFile);

  REQUIRE_NOTHROW(rti->requestFederationRestore(L"abort-restore-report-snapshot"));
  auto const expectedRequest =
      R"({"HLAserialNumber":5,"HLAreturnedArgument":[null],"HLAservice":"RequestFederationRestore","HLAsuppliedArguments":[{"HLAargumentType":53,"HLAargumentName":"Federation save label","HLAargumentValue":"abort-restore-report-snapshot"}],"HLAsuccessIndicator":true,"HLAexception":null})";
  auto const expectedConfirmation =
      R"({"HLAserialNumber":6,"HLAreturnedArgument":[null],"HLAservice":"ConfirmFederationRestorationRequest","HLAsuppliedArguments":[{"HLAargumentType":53,"HLAargumentName":"Federation save label","HLAargumentValue":"abort-restore-report-snapshot"},{"HLAargumentType":6,"HLAargumentName":"Request-success indicator","HLAargumentValue":true}],"HLAsuccessIndicator":true,"HLAexception":null})";
  auto const expectedRestoreBegun =
      R"({"HLAserialNumber":7,"HLAreturnedArgument":[null],"HLAservice":"FederationRestoreBegun","HLAsuppliedArguments":[],"HLAsuccessIndicator":true,"HLAexception":null})";
  auto const expectedInitiateRestore = initiateFederateRestoreServiceReportRecord(
      8U,
      "abort-restore-report-snapshot",
      reportDesignator,
      "abort-restore-report-subject");
  REQUIRE(readTextFile(reportFile) ==
          savedText + expectedRequest + expectedConfirmation + expectedRestoreBegun +
              expectedInitiateRestore);
  while (rti->evokeCallback(0.0)) {
  }
  REQUIRE(reports.requestFederationRestoreSucceededReports ==
          std::vector<std::wstring>{L"abort-restore-report-snapshot"});

  // §4.33 accepts the no-argument abort before the later restore-result
  // callback. The ordinary incomplete-restore path reports RESTORE_ABORTED.
  REQUIRE_NOTHROW(rti->abortFederationRestore());
  auto const expectedAbort =
      R"({"HLAserialNumber":9,"HLAreturnedArgument":[null],"HLAservice":"AbortFederationRestore","HLAsuppliedArguments":[],"HLAsuccessIndicator":true,"HLAexception":null})";
  REQUIRE(readTextFile(reportFile) ==
          savedText + expectedRequest + expectedConfirmation + expectedRestoreBegun +
              expectedInitiateRestore + expectedAbort);
  REQUIRE(reports.federationNotRestoredReasons.empty());
  REQUIRE_FALSE(rti->evokeCallback(0.0));
  REQUIRE(reports.federationNotRestoredReasons ==
          std::vector<RestoreFailureReason>{rti1516_2025::RESTORE_ABORTED});

  REQUIRE_NOTHROW(rti->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(rti->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(rti->disconnect());
}
}  // namespace
