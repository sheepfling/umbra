#include "ieee1516_2025_federation_management_fixture_support.hpp"

#include <RTI/time/HLAfloat64Time.h>

namespace {

using namespace rti1516_2025;

TEST_CASE(
    "Embedded service reporting records an invalid-time Next Message Request Available failure",
    "[integration][development-profile][2025][time-management][mom]"
    "[service-report-file][service-reporting][service-failure]"
    "[service-report-next-message-request-available-invalid-time-failure]"
    "[rti.service.next-message-request-available]") {
  ReportingFederateAmbassador reports;
  auto rti = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  auto directory = temporaryServiceReportDirectory();
  auto configuration = configurationForServiceReportDirectory(directory.path());
  configuration.withRtiAddress(L"in-process");

  REQUIRE_NOTHROW(rti->connect(reports, HLA_EVOKED, configuration));
  REQUIRE_NOTHROW(rti->createFederationExecution(
      federationName, fomModule, standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(rti->joinFederationExecution(
      L"nmra-invalid-time-report-member", L"subscriber", federationName));
  auto const files = serviceReportFiles(directory.path());
  REQUIRE(files.size() == 1U);
  auto const reportFile = files.front();
  REQUIRE_NOTHROW(rti->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(rti->setSendServiceReportsToFileSwitch(false));
  REQUIRE_NOTHROW(rti->setServiceReportingSwitch(true));
  REQUIRE_NOTHROW(rti->setSendServiceReportsToFileSwitch(true));
  auto const textBeforeFailure = readTextFile(reportFile);

  // The execution uses HLAinteger64Time. A valid HLAfloat64Time value is an
  // invalid logical-time argument for this execution and must not be accepted.
  REQUIRE_THROWS_AS(
      rti->nextMessageRequestAvailable(HLAfloat64Time(4.5)), InvalidLogicalTime);
  auto const report = readTextFile(reportFile);
  REQUIRE(report.starts_with(textBeforeFailure));
  auto const failure = report.substr(textBeforeFailure.size());
  REQUIRE(failure.find(R"("HLAservice":"NextMessageRequestAvailable")") !=
          std::string::npos);
  REQUIRE(failure.find(R"("HLAsuccessIndicator":false)") != std::string::npos);
  REQUIRE(failure.find(R"("HLAexception":"InvalidLogicalTime:)" ) !=
          std::string::npos);
  REQUIRE(failure.find(R"("HLAargumentName":"Logical time")") !=
          std::string::npos);
  REQUIRE(failure.find(R"("HLAargumentValue":"4.5")") !=
          std::string::npos);
  HLAinteger64Time queriedTime;
  REQUIRE_NOTHROW(rti->queryLogicalTime(queriedTime));
  REQUIRE(queriedTime.isInitial());
  REQUIRE(reports.timeAdvanceGrantReports.empty());

  REQUIRE_NOTHROW(rti->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(rti->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(rti->disconnect());
}

}  // namespace
