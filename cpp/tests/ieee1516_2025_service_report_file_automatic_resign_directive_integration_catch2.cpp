#include "ieee1516_2025_federation_management_test_support.hpp"
#include "ieee1516_2025_federation_management_fixture_support.hpp"
#include "ieee1516_2025_public_federation_restore_test_support.hpp"

namespace {
TEST_CASE(
    "Embedded service reporting appends a Table 5 void record for Set Automatic Resign Directive",
    "[integration][development-profile][federation-management][mom][service-report-file]"
    "[service-reporting][support-switches][resign-action]"
    "[rti.service.set-automatic-resign-directive]") {
  TestFederateAmbassador reports;
  auto rti = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule =
      (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data" /
       "switch-support-enabled-fom.xml")
          .wstring();
  auto directory = public_federation_restore_test_support::temporaryServiceReportDirectory();
  auto configuration =
      public_federation_restore_test_support::configurationForServiceReportDirectory(directory.path());
  configuration.withRtiAddress(L"in-process");

  REQUIRE_NOTHROW(rti->connect(reports, HLA_EVOKED, configuration));
  REQUIRE_NOTHROW(
      rti->createFederationExecution(federationName, fomModule, standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(rti->joinFederationExecution(
      L"automatic-resign-report-subject", L"subject", federationName));
  auto const files = public_federation_restore_test_support::serviceReportFiles(directory.path());
  REQUIRE(files.size() == 1U);
  auto const initialText = public_federation_restore_test_support::readTextFile(files.front());

  REQUIRE_NOTHROW(rti->setAutomaticResignDirective(rti1516_2025::DELETE_OBJECTS));
  auto const expectedRecord =
      R"({"HLAserialNumber":0,"HLAreturnedArgument":[null],"HLAservice":"SetAutomaticResignDirective","HLAsuppliedArguments":[{"HLAargumentType":44,"HLAargumentName":"AutomaticResignDirective","HLAargumentValue":"DELETE_OBJECTS"}],"HLAsuccessIndicator":true,"HLAexception":null})";
  REQUIRE(public_federation_restore_test_support::readTextFile(files.front()) ==
          initialText + expectedRecord);

  REQUIRE_NOTHROW(rti->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(rti->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(rti->disconnect());
}
}  // namespace
