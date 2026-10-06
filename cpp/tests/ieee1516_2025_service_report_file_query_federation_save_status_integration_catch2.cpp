#include "ieee1516_2025_federation_management_test_support.hpp"
#include "ieee1516_2025_federation_management_fixture_support.hpp"
#include "ieee1516_2025_public_federation_restore_test_support.hpp"

namespace {
TEST_CASE(
    "Embedded service reporting records Query Federation Save Status before its response",
    "[integration][development-profile][federation-management][save-restore]"
    "[mom][service-report-file][service-reporting][query-federation-save-status]"
    "[federation-save-status-response-service-report]"
    "[rti.service.request-federation-save][rti.service.query-federation-save-status]"
    "[rti.service.abort-federation-save][federate.callback.initiate-federate-save]"
    "[federate.callback.federation-save-status-response]") {
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
  FederateHandle federateHandle;
  REQUIRE_NOTHROW(federateHandle = rti->joinFederationExecution(
                      L"save-status-report-subject", L"subject", federationName));
  auto const files = serviceReportFiles(directory.path());
  REQUIRE(files.size() == 1U);
  auto const reportFile = files.front();

  // Start a real save so the §4.26 response carries an in-progress status.
  // Its request and initiation records own serials zero and one; the query and
  // its distinct response record are durable before the callback is evoked.
  REQUIRE_NOTHROW(rti->requestFederationSave(L"report-save-status"));
  auto const requestText = readTextFile(reportFile);
  REQUIRE_FALSE(rti->evokeCallback(0.0));
  REQUIRE(reports.initiateFederateSaveReports == std::vector<std::wstring>{L"report-save-status"});

  REQUIRE_NOTHROW(rti->queryFederationSaveStatus());
  auto const expectedRecord =
      R"({"HLAserialNumber":2,"HLAreturnedArgument":[null],"HLAservice":"QueryFederationSaveStatus","HLAsuppliedArguments":[],"HLAsuccessIndicator":true,"HLAexception":null})";
  auto const handleText = umbra::detail::utf8FromWide(federateHandle.toString());
  REQUIRE(handleText.has_value());
  auto const expectedStatusResponseRecord =
      std::string{R"({"HLAserialNumber":3,"HLAreturnedArgument":[null],"HLAservice":"FederationSaveStatusResponse","HLAsuppliedArguments":[{"HLAargumentType":17,"HLAargumentName":"List of joined federates and save status for each","HLAargumentValue":[{"handle":")"} +
      *handleText +
      R"(","status":"FEDERATE_INSTRUCTED_TO_SAVE"}]}],"HLAsuccessIndicator":true,"HLAexception":null})";
  REQUIRE(readTextFile(reportFile) ==
          requestText + expectedRecord + expectedStatusResponseRecord);
  REQUIRE(reports.federationSaveStatusReports.empty());
  REQUIRE_FALSE(rti->evokeCallback(0.0));
  REQUIRE(reports.federationSaveStatusReports.size() == 1U);
  auto const& statusResponse = reports.federationSaveStatusReports.back().statuses;
  REQUIRE(statusResponse.size() == 1U);
  REQUIRE(statusResponse.front().first == federateHandle);
  REQUIRE(statusResponse.front().second == rti1516_2025::FEDERATE_INSTRUCTED_TO_SAVE);

  // Close the real save control operation; Abort Federation Save reporting is
  // covered by its own focused regression.
  REQUIRE_NOTHROW(rti->abortFederationSave());
  REQUIRE_FALSE(rti->evokeCallback(0.0));
  REQUIRE_NOTHROW(rti->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(rti->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(rti->disconnect());
}
}  // namespace
