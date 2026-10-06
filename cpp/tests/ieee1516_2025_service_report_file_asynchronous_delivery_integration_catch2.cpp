#include "ieee1516_2025_federation_management_test_support.hpp"
#include "ieee1516_2025_federation_management_fixture_support.hpp"
#include "ieee1516_2025_public_federation_restore_test_support.hpp"

namespace {
TEST_CASE(
    "Embedded service reporting appends Table 5 void records for no-argument asynchronous delivery services",
    "[integration][development-profile][federation-management][mom][service-report-file]"
    "[service-reporting][time-management][asynchronous-delivery]"
    "[rti.service.enable-asynchronous-delivery]"
    "[rti.service.disable-asynchronous-delivery]") {
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
      L"asynchronous-delivery-report-subject", L"subject", federationName));
  auto const files = public_federation_restore_test_support::serviceReportFiles(directory.path());
  REQUIRE(files.size() == 1U);
  auto const initialText = public_federation_restore_test_support::readTextFile(files.front());

  // Both service narratives declare no supplied or returned arguments.  The
  // §11.5.1 supplied-argument rule therefore requires [], while Table 5's
  // explicit successful-void record still uses [null] for the return form.
  REQUIRE_NOTHROW(rti->enableAsynchronousDelivery());
  REQUIRE_NOTHROW(rti->disableAsynchronousDelivery());
  auto const expectedEnable =
      R"({"HLAserialNumber":0,"HLAreturnedArgument":[null],"HLAservice":"EnableAsynchronousDelivery","HLAsuppliedArguments":[],"HLAsuccessIndicator":true,"HLAexception":null})";
  auto const expectedDisable =
      R"({"HLAserialNumber":1,"HLAreturnedArgument":[null],"HLAservice":"DisableAsynchronousDelivery","HLAsuppliedArguments":[],"HLAsuccessIndicator":true,"HLAexception":null})";
  REQUIRE(public_federation_restore_test_support::readTextFile(files.front()) ==
          initialText + expectedEnable + expectedDisable);

  REQUIRE_NOTHROW(rti->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(rti->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(rti->disconnect());
}
}  // namespace
