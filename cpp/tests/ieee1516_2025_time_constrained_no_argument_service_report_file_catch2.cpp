#ifndef UMBRA_SOURCE_DIRECTORY
#error "The 2025 federation-management service-report tests require the Umbra source directory."
#endif

#include "ieee1516_2025_federation_management_fixture_support.hpp"

namespace {

using namespace rti1516_2025;

TEST_CASE(
    "Embedded service reporting records callback-gated no-argument time-constrained services",
    "[integration][development-profile][federation-management][mom][service-report-file]"
    "[service-reporting][time-management][time-role][service-report-time-constrained-no-argument-services][2025]"
    "[rti.service.enable-time-constrained]"
    "[rti.service.disable-time-constrained]"
    "[federate.callback.time-constrained-enabled]") {
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
  REQUIRE_NOTHROW(rti->joinFederationExecution(
      L"time-constrained-report-subject", L"subject", federationName));
  auto const files = serviceReportFiles(directory.path());
  REQUIRE(files.size() == 1U);
  auto const initialText = readTextFile(files.front());

  // The request is reportable immediately, but its semantic completion stays
  // callback-gated.  The later disable therefore follows the actual Time
  // Constrained Enabled callback rather than making the pending request look
  // like enabled state.
  REQUIRE_NOTHROW(rti->enableTimeConstrained());
  auto const expectedEnable =
      R"({"HLAserialNumber":0,"HLAreturnedArgument":[null],"HLAservice":"EnableTimeConstrained","HLAsuppliedArguments":[],"HLAsuccessIndicator":true,"HLAexception":null})";
  REQUIRE(readTextFile(files.front()) == initialText + expectedEnable);
  REQUIRE(reports.timeConstrainedEnabledReports.empty());
  REQUIRE_FALSE(rti->evokeCallback(0.0));
  REQUIRE(reports.timeConstrainedEnabledReports.size() == 1U);

  REQUIRE_NOTHROW(rti->disableTimeConstrained());
  auto const expectedDisable =
      R"({"HLAserialNumber":1,"HLAreturnedArgument":[null],"HLAservice":"DisableTimeConstrained","HLAsuppliedArguments":[],"HLAsuccessIndicator":true,"HLAexception":null})";
  REQUIRE(readTextFile(files.front()) == initialText + expectedEnable + expectedDisable);

  REQUIRE_NOTHROW(rti->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(rti->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(rti->disconnect());
}

}  // namespace
