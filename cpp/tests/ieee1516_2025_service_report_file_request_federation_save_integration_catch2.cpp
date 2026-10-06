#include "ieee1516_2025_federation_management_test_support.hpp"
#include "ieee1516_2025_federation_management_fixture_support.hpp"
#include "ieee1516_2025_public_federation_restore_test_support.hpp"

namespace {
TEST_CASE(
    "Embedded service reporting preserves Request Federation Save timestamp overloads",
    "[integration][development-profile][federation-management][save-restore]"
    "[mom][service-report-file][service-reporting][request-federation-save]"
    "[rti.service.request-federation-save][rti.service.enable-time-constrained]"
    "[rti.service.enable-time-regulation][federate.callback.time-constrained-enabled]"
    "[federate.callback.time-regulation-enabled][federate.callback.initiate-federate-save]") {
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
      rti->joinFederationExecution(L"save-request-report-subject", L"subject", federationName));
  auto const files = serviceReportFiles(directory.path());
  REQUIRE(files.size() == 1U);
  auto const reportFile = files.front();

  // Establish a member that is both time-constrained and time-regulating
  // before taking the report baseline.  This keeps an untimed save request
  // pending (rather than immediately starting it) while making the supplied
  // timestamp form valid.  The setup records belong to the two time-role
  // services, not the Request Federation Save records proved below.
  REQUIRE_NOTHROW(rti->enableTimeConstrained());
  REQUIRE_FALSE(rti->evokeCallback(0.0));
  REQUIRE(reports.timeConstrainedEnabledReports.size() == 1U);
  REQUIRE_NOTHROW(rti->enableTimeRegulation(rti1516_2025::HLAinteger64Interval(1)));
  REQUIRE_FALSE(rti->evokeCallback(0.0));
  REQUIRE(reports.timeRegulationEnabledReports.size() == 1U);
  auto const setupText = readTextFile(reportFile);

  // The first request leaves its Initiate Federate Save callback queued.  A
  // valid timestamped request may replace that outstanding request, proving
  // both official C++ overloads without crossing into save-completion work.
  REQUIRE_NOTHROW(rti->requestFederationSave(L"report-save-untimed"));
  auto const expectedUntimed =
      R"({"HLAserialNumber":2,"HLAreturnedArgument":[null],"HLAservice":"RequestFederationSave","HLAsuppliedArguments":[{"HLAargumentType":53,"HLAargumentName":"Federation save label","HLAargumentValue":"report-save-untimed"},{"HLAargumentType":34,"HLAargumentName":"Optional timestamp","HLAargumentValue":null}],"HLAsuccessIndicator":true,"HLAexception":null})";
  REQUIRE(readTextFile(reportFile) == setupText + expectedUntimed);
  REQUIRE(reports.initiateFederateSaveReports.empty());

  REQUIRE_NOTHROW(rti->requestFederationSave(
      L"report-save-timed",
      rti1516_2025::HLAinteger64Time(5)));
  auto const expectedTimed =
      R"({"HLAserialNumber":3,"HLAreturnedArgument":[null],"HLAservice":"RequestFederationSave","HLAsuppliedArguments":[{"HLAargumentType":53,"HLAargumentName":"Federation save label","HLAargumentValue":"report-save-timed"},{"HLAargumentType":31,"HLAargumentName":"Optional timestamp","HLAargumentValue":"5"}],"HLAsuccessIndicator":true,"HLAexception":null})";
  REQUIRE(readTextFile(reportFile) == setupText + expectedUntimed + expectedTimed);
  REQUIRE(reports.initiateFederateSaveReports.empty());

  // The replacement request remains pending and is intentionally outside this
  // report-shape slice.  Resignation discards its undelivered callback work.
  REQUIRE_NOTHROW(rti->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(rti->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(rti->disconnect());
}
}  // namespace
