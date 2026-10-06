#include "ieee1516_2025_federation_management_test_support.hpp"
#include "ieee1516_2025_federation_management_fixture_support.hpp"
#include "ieee1516_2025_public_federation_restore_test_support.hpp"

namespace {
TEST_CASE(
    "Embedded service reporting records Request Federation Restore before its confirmation",
    "[integration][development-profile][federation-management][save-restore]"
    "[mom][service-report-file][service-reporting][request-federation-restore]"
    "[confirm-federation-restoration-request-service-report]"
    "[rti.service.request-federation-save][rti.service.federate-save-begun]"
    "[rti.service.federate-save-complete][rti.service.request-federation-restore]"
    "[federate.callback.initiate-federate-save]"
    "[federate.callback.request-federation-restore-succeeded]") {
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
      rti->joinFederationExecution(L"restore-request-report-subject", L"subject", federationName));
  auto const files = serviceReportFiles(directory.path());
  REQUIRE(files.size() == 1U);
  auto const reportFile = files.front();
  auto const reportDesignator = joinedFederateReportDesignator(reportFile);

  // Produce one real snapshot. The Request, Initiate, Begun, Complete, and
  // Federation Saved records are outside this lane's report assertions and establish serial 5 for the
  // bounded §4.27 request below.
  REQUIRE_NOTHROW(rti->requestFederationSave(L"report-restore-snapshot"));
  REQUIRE_FALSE(rti->evokeCallback(0.0));
  REQUIRE(reports.initiateFederateSaveReports ==
          std::vector<std::wstring>{L"report-restore-snapshot"});
  REQUIRE_NOTHROW(rti->federateSaveBegun());
  REQUIRE_NOTHROW(rti->federateSaveComplete());
  REQUIRE_FALSE(rti->evokeCallback(0.0));
  REQUIRE(reports.federationSavedReportCount == 1U);
  auto const savedText = readTextFile(reportFile);

  REQUIRE_NOTHROW(rti->requestFederationRestore(L"report-restore-snapshot"));
  auto const expectedRequest =
      R"({"HLAserialNumber":5,"HLAreturnedArgument":[null],"HLAservice":"RequestFederationRestore","HLAsuppliedArguments":[{"HLAargumentType":53,"HLAargumentName":"Federation save label","HLAargumentValue":"report-restore-snapshot"}],"HLAsuccessIndicator":true,"HLAexception":null})";
  auto const expectedConfirmation =
      R"({"HLAserialNumber":6,"HLAreturnedArgument":[null],"HLAservice":"ConfirmFederationRestorationRequest","HLAsuppliedArguments":[{"HLAargumentType":53,"HLAargumentName":"Federation save label","HLAargumentValue":"report-restore-snapshot"},{"HLAargumentType":6,"HLAargumentName":"Request-success indicator","HLAargumentValue":true}],"HLAsuccessIndicator":true,"HLAexception":null})";
  auto const expectedRestoreBegun =
      R"({"HLAserialNumber":7,"HLAreturnedArgument":[null],"HLAservice":"FederationRestoreBegun","HLAsuppliedArguments":[],"HLAsuccessIndicator":true,"HLAexception":null})";
  auto const expectedInitiateRestore = initiateFederateRestoreServiceReportRecord(
      8U,
      "report-restore-snapshot",
      reportDesignator,
      "restore-request-report-subject");
  REQUIRE(readTextFile(reportFile) ==
          savedText + expectedRequest + expectedConfirmation + expectedRestoreBegun +
              expectedInitiateRestore);
  REQUIRE(reports.requestFederationRestoreSucceededReports.empty());

  // The accepted §4.27 and RTI-initiated §4.28/§4.29/§4.30 records are
  // already durable before the confirmation and restore-work callbacks.
  // Finish the real restore cycle so teardown remains state-safe.
  while (rti->evokeCallback(0.0)) {
  }
  REQUIRE(reports.requestFederationRestoreSucceededReports ==
          std::vector<std::wstring>{L"report-restore-snapshot"});
  REQUIRE_NOTHROW(rti->federateRestoreComplete());
  while (rti->evokeCallback(0.0)) {
  }
  REQUIRE(reports.federationRestoredReportCount == 1U);

  REQUIRE_NOTHROW(rti->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(rti->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(rti->disconnect());
}
}  // namespace
