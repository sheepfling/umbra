#include "ieee1516_2025_federation_management_test_support.hpp"
#include "ieee1516_2025_federation_management_fixture_support.hpp"
#include "ieee1516_2025_public_federation_restore_test_support.hpp"

namespace {
TEST_CASE(
    "Embedded service reporting records a negative Confirm Federation Restoration Request before its callback",
    "[integration][development-profile][federation-management][save-restore]"
    "[mom][service-report-file][service-reporting][request-federation-restore]"
    "[confirm-federation-restoration-request-service-report]"
    "[rti.service.request-federation-restore]"
    "[federate.callback.request-federation-restore-failed]") {
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
      rti->joinFederationExecution(L"restore-request-negative-report-subject", L"subject", federationName));
  auto const files = serviceReportFiles(directory.path());
  REQUIRE(files.size() == 1U);
  auto const reportFile = files.front();
  auto const initialText = readTextFile(reportFile);

  // A missing snapshot is still a normally returned §4.27 request. The
  // negative §4.28 record owns the next serial and precedes the failure
  // callback, without starting a restore operation.
  REQUIRE_NOTHROW(rti->requestFederationRestore(L"missing-report-restore-snapshot"));
  auto const expectedRequest =
      R"({"HLAserialNumber":0,"HLAreturnedArgument":[null],"HLAservice":"RequestFederationRestore","HLAsuppliedArguments":[{"HLAargumentType":53,"HLAargumentName":"Federation save label","HLAargumentValue":"missing-report-restore-snapshot"}],"HLAsuccessIndicator":true,"HLAexception":null})";
  auto const expectedConfirmation =
      R"({"HLAserialNumber":1,"HLAreturnedArgument":[null],"HLAservice":"ConfirmFederationRestorationRequest","HLAsuppliedArguments":[{"HLAargumentType":53,"HLAargumentName":"Federation save label","HLAargumentValue":"missing-report-restore-snapshot"},{"HLAargumentType":6,"HLAargumentName":"Request-success indicator","HLAargumentValue":false}],"HLAsuccessIndicator":true,"HLAexception":null})";
  REQUIRE(readTextFile(reportFile) == initialText + expectedRequest + expectedConfirmation);
  REQUIRE(reports.requestFederationRestoreFailedReports.empty());

  REQUIRE_FALSE(rti->evokeCallback(0.0));
  REQUIRE(reports.requestFederationRestoreFailedReports ==
          std::vector<std::wstring>{L"missing-report-restore-snapshot"});
  REQUIRE_THROWS_AS(rti->federateRestoreComplete(), rti1516_2025::RestoreNotRequested);
  REQUIRE(readTextFile(reportFile) == initialText + expectedRequest + expectedConfirmation);

  REQUIRE_NOTHROW(rti->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(rti->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(rti->disconnect());
}
}  // namespace
