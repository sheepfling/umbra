#include "ieee1516_2025_federation_management_test_support.hpp"
#include "ieee1516_2025_federation_management_fixture_support.hpp"
#include "ieee1516_2025_public_federation_restore_test_support.hpp"

namespace {
TEST_CASE(
    "Embedded service reporting records an accepted Federate Save Begun invocation",
    "[integration][development-profile][federation-management][save-restore]"
    "[mom][service-report-file][service-reporting][federate-save-begun]"
    "[rti.service.request-federation-save][rti.service.federate-save-begun]"
    "[rti.service.federate-save-complete][federate.callback.initiate-federate-save]"
    "[federate.callback.federation-saved]") {
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
      rti->joinFederationExecution(L"save-begun-report-subject", L"subject", federationName));
  auto const files = serviceReportFiles(directory.path());
  REQUIRE(files.size() == 1U);
  auto const reportFile = files.front();
  auto const initialText = readTextFile(reportFile);

  // Before §4.20's Initiate Federate Save callback, §4.21 fails and must not
  // reserve a successful-void report serial.
  REQUIRE_THROWS_AS(rti->federateSaveBegun(), rti1516_2025::SaveNotInitiated);
  REQUIRE(readTextFile(reportFile) == initialText);

  REQUIRE_NOTHROW(rti->requestFederationSave(L"report-save"));
  REQUIRE(reports.initiateFederateSaveReports.empty());
  REQUIRE_FALSE(rti->evokeCallback(0.0));
  REQUIRE(reports.initiateFederateSaveReports == std::vector<std::wstring>{L"report-save"});
  auto const requestText = readTextFile(reportFile);

  REQUIRE_NOTHROW(rti->federateSaveBegun());
  auto const expectedRecord =
      R"({"HLAserialNumber":2,"HLAreturnedArgument":[null],"HLAservice":"FederateSaveBegun","HLAsuppliedArguments":[],"HLAsuccessIndicator":true,"HLAexception":null})";
  REQUIRE(readTextFile(reportFile) == requestText + expectedRecord);
  REQUIRE(reports.federationSavedReportCount == 0U);
  REQUIRE(reports.federationNotSavedReasons.empty());

  // Finish the real save cycle so cleanup occurs through the state machine.
  // Federate Save Complete reporting has its own focused regression below.
  REQUIRE_NOTHROW(rti->federateSaveComplete());
  REQUIRE_FALSE(rti->evokeCallback(0.0));
  REQUIRE(reports.federationSavedReportCount == 1U);

  REQUIRE_NOTHROW(rti->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(rti->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(rti->disconnect());
}
}  // namespace
