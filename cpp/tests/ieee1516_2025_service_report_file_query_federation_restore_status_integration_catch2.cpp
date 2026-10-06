#include "ieee1516_2025_federation_management_test_support.hpp"
#include "ieee1516_2025_federation_management_fixture_support.hpp"
#include "ieee1516_2025_public_federation_restore_test_support.hpp"

namespace {
TEST_CASE(
    "Embedded service reporting records Query Federation Restore Status before its response",
    "[integration][development-profile][federation-management][save-restore]"
    "[mom][service-report-file][service-reporting][query-federation-restore-status]"
    "[federation-restore-status-response-service-report]"
    "[rti.service.query-federation-restore-status][rti.service.request-federation-save]"
    "[rti.service.abort-federation-save][federate.callback.initiate-federate-save]"
    "[federate.callback.federation-restore-status-response]") {
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
      rti->joinFederationExecution(L"restore-status-report-subject", L"subject", federationName));
  auto const files = serviceReportFiles(directory.path());
  REQUIRE(files.size() == 1U);
  auto const reportFile = files.front();
  auto const initialText = readTextFile(reportFile);
  auto const reportDesignator = joinedFederateReportDesignator(reportFile);

  // §4.34 accepts a no-argument status query before the separately queued
  // §4.35 response. With no restore running, that response still describes
  // the joined federate as NO_RESTORE_IN_PROGRESS.
  REQUIRE_NOTHROW(rti->queryFederationRestoreStatus());
  auto const expectedQuery =
      R"({"HLAserialNumber":0,"HLAreturnedArgument":[null],"HLAservice":"QueryFederationRestoreStatus","HLAsuppliedArguments":[],"HLAsuccessIndicator":true,"HLAexception":null})";
  auto const expectedStatusResponse =
      std::string{R"umbra({"HLAserialNumber":1,"HLAreturnedArgument":[null],"HLAservice":"FederationRestoreStatusResponse","HLAsuppliedArguments":[{"HLAargumentType":20,"HLAargumentName":"List of joined federates and restore status for each","HLAargumentValue":[{"preRestoreHandle":")umbra"} +
      reportDesignator +
      R"umbra(","postRestoreHandle":"FederateHandle(invalid)","status":"NO_RESTORE_IN_PROGRESS"}]}],"HLAsuccessIndicator":true,"HLAexception":null})umbra";
  REQUIRE(readTextFile(reportFile) == initialText + expectedQuery + expectedStatusResponse);
  REQUIRE(reports.federationRestoreStatusReports.empty());
  REQUIRE_FALSE(rti->evokeCallback(0.0));
  REQUIRE(reports.federationRestoreStatusReports.size() == 1U);
  auto const& idleStatuses = reports.federationRestoreStatusReports.back().statuses;
  REQUIRE(idleStatuses.size() == 1U);
  REQUIRE(idleStatuses.front().status == rti1516_2025::NO_RESTORE_IN_PROGRESS);

  // A save in progress rejects §4.34. The request's separate record follows
  // the query/response pair, but the rejected query must not append a record.
  REQUIRE_NOTHROW(rti->requestFederationSave(L"restore-status-report-save"));
  auto const savingText = readTextFile(reportFile);
  REQUIRE_FALSE(rti->evokeCallback(0.0));
  REQUIRE(reports.initiateFederateSaveReports ==
          std::vector<std::wstring>{L"restore-status-report-save"});
  REQUIRE_THROWS_AS(rti->queryFederationRestoreStatus(), rti1516_2025::SaveInProgress);
  REQUIRE(readTextFile(reportFile) == savingText);

  REQUIRE_NOTHROW(rti->abortFederationSave());
  REQUIRE_FALSE(rti->evokeCallback(0.0));
  REQUIRE_NOTHROW(rti->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(rti->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(rti->disconnect());
}
}  // namespace
