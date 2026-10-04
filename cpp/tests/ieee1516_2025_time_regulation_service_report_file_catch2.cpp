#ifndef UMBRA_SOURCE_DIRECTORY
#error "The 2025 federation-management service-report tests require the Umbra source directory."
#endif

#include "ieee1516_2025_federation_management_fixture_support.hpp"

namespace {

using namespace rti1516_2025;

TEST_CASE(
    "Embedded service reporting records callback-gated time-regulation services",
    "[integration][development-profile][federation-management][mom][service-report-file]"
    "[service-reporting][time-management][time-role]"
    "[service-report-time-regulation-services]"
    "[rti.service.enable-time-regulation]"
    "[rti.service.disable-time-regulation]"
    "[federate.callback.time-regulation-enabled]") {
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
      L"time-regulation-report-subject", L"subject", federationName));
  auto const files = serviceReportFiles(directory.path());
  REQUIRE(files.size() == 1U);
  auto const initialText = readTextFile(files.front());

  // Enable Time Regulation has exactly one supplied Lookahead argument. Its
  // report uses Table 5's LogicalTimeInterval (32) / interval.toString()
  // form at request acceptance; role establishment remains callback-gated.
  REQUIRE_NOTHROW(rti->enableTimeRegulation(rti1516_2025::HLAinteger64Interval(2)));
  auto const expectedEnable =
      R"({"HLAserialNumber":0,"HLAreturnedArgument":[null],"HLAservice":"EnableTimeRegulation","HLAsuppliedArguments":[{"HLAargumentType":32,"HLAargumentName":"Lookahead","HLAargumentValue":"2"}],"HLAsuccessIndicator":true,"HLAexception":null})";
  REQUIRE(readTextFile(files.front()) == initialText + expectedEnable);
  REQUIRE(reports.timeRegulationEnabledReports.empty());
  REQUIRE_FALSE(rti->evokeCallback(0.0));
  REQUIRE(reports.timeRegulationEnabledReports.size() == 1U);

  REQUIRE_NOTHROW(rti->disableTimeRegulation());
  auto const expectedDisable =
      R"({"HLAserialNumber":1,"HLAreturnedArgument":[null],"HLAservice":"DisableTimeRegulation","HLAsuppliedArguments":[],"HLAsuccessIndicator":true,"HLAexception":null})";
  REQUIRE(readTextFile(files.front()) == initialText + expectedEnable + expectedDisable);

  REQUIRE_NOTHROW(rti->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(rti->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(rti->disconnect());
}

}  // namespace
