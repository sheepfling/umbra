#include "ieee1516_2025_federation_management_test_support.hpp"
#include "ieee1516_2025_federation_management_fixture_support.hpp"
#include "ieee1516_2025_public_federation_restore_test_support.hpp"

namespace {
TEST_CASE(
    "Embedded service reporting records an accepted Abort Federation Save invocation",
    "[integration][development-profile][federation-management][save-restore]"
    "[mom][service-report-file][service-reporting][abort-federation-save]"
    "[rti.service.request-federation-save][rti.service.abort-federation-save]"
    "[federate.callback.initiate-federate-save][federate.callback.federation-not-saved]") {
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
      rti->joinFederationExecution(L"abort-save-report-subject", L"subject", federationName));
  auto const files = serviceReportFiles(directory.path());
  REQUIRE(files.size() == 1U);
  auto const reportFile = files.front();
  auto const initialText = readTextFile(reportFile);

  // An absent save operation rejects §4.24 and must not reserve a report
  // serial. The accepted request below owns serial zero; this slice verifies
  // that the later abort advances it rather than replacing that record.
  REQUIRE_THROWS_AS(rti->abortFederationSave(), rti1516_2025::SaveNotInProgress);
  REQUIRE(readTextFile(reportFile) == initialText);
  REQUIRE_NOTHROW(rti->requestFederationSave(L"report-save-abort"));
  auto const requestText = readTextFile(reportFile);
  REQUIRE_FALSE(rti->evokeCallback(0.0));
  REQUIRE(reports.initiateFederateSaveReports == std::vector<std::wstring>{L"report-save-abort"});

  REQUIRE_NOTHROW(rti->abortFederationSave());
  auto const expectedRecord =
      R"({"HLAserialNumber":2,"HLAreturnedArgument":[null],"HLAservice":"AbortFederationSave","HLAsuppliedArguments":[],"HLAsuccessIndicator":true,"HLAexception":null})";
  auto const expectedFederationSaved =
      R"({"HLAserialNumber":3,"HLAreturnedArgument":[null],"HLAservice":"FederationSaved","HLAsuppliedArguments":[{"HLAargumentType":6,"HLAargumentName":"Federation save-success indicator","HLAargumentValue":false},{"HLAargumentType":48,"HLAargumentName":"Optional failure reason","HLAargumentValue":"SAVE_ABORTED"}],"HLAsuccessIndicator":true,"HLAexception":null})";
  REQUIRE(readTextFile(reportFile) == requestText + expectedRecord + expectedFederationSaved);
  REQUIRE(reports.federationNotSavedReasons.empty());
  REQUIRE_FALSE(rti->evokeCallback(0.0));
  REQUIRE(reports.federationNotSavedReasons ==
          std::vector<SaveFailureReason>{rti1516_2025::SAVE_ABORTED});

  REQUIRE_NOTHROW(rti->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(rti->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(rti->disconnect());
}
}  // namespace
