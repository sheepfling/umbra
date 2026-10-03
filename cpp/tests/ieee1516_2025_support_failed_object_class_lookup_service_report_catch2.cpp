#include "ieee1516_2025_federation_management_test_support.hpp"
#include "ieee1516_2025_federation_management_fixture_support.hpp"

namespace {
TEST_CASE(
    "Embedded service reporting records failed non-void lookup invocations",
    "[integration][development-profile][federation-management][support-services]"
    "[mom][service-report-file][service-reporting][service-failure]"
    "[rti.service.get-object-class-handle][rti.service.failure]") {
  TestFederateAmbassador reports;
  auto owner = makeRti();
  auto const federationName = nextFederationName();
  auto const restaurantFom = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  auto const serviceReportFom =
      (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data" /
       "switch-support-enabled-fom.xml")
          .wstring();
  std::vector<std::wstring> const fomModules{restaurantFom, serviceReportFom};
  auto directory = temporaryServiceReportDirectory();
  auto configuration = configurationForServiceReportDirectory(directory.path());
  configuration.withRtiAddress(L"in-process");

  REQUIRE_NOTHROW(owner->connect(reports, HLA_EVOKED, configuration));
  REQUIRE_NOTHROW(
      owner->createFederationExecution(federationName, fomModules, standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(owner->joinFederationExecution(
      L"lookup-failure-report-owner", L"owner", federationName));

  auto const files = serviceReportFiles(directory.path());
  REQUIRE(files.size() == 1U);
  auto const reportFile = files.front();
  auto const initialText = readTextFile(reportFile);

  // Keep setup lookups out of the service-report sequence. Re-enable both
  // switches only for the failed and successful calls under test.
  REQUIRE_NOTHROW(owner->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(owner->setSendServiceReportsToFileSwitch(false));
  REQUIRE_NOTHROW(owner->setServiceReportingSwitch(true));
  REQUIRE_NOTHROW(owner->setSendServiceReportsToFileSwitch(true));

  REQUIRE_THROWS_AS(
      owner->getObjectClassHandle(fixture_hla::fom::missing_object),
      rti1516_2025::NameNotFound);
  auto const server = owner->getObjectClassHandle(fixture_hla::fom::employee_server);

  auto asAscii = [](std::wstring const& value) {
    std::string result;
    result.reserve(value.size());
    for (wchar_t const character : value) {
      REQUIRE(character >= L' ');
      REQUIRE(character <= L'~');
      result.push_back(static_cast<char>(character));
    }
    return result;
  };
  auto const serverValue = asAscii(server.toString());
  auto const expectedRecords =
      std::string{
          R"({"HLAserialNumber":0,"HLAreturnedArgument":[null],"HLAservice":"GetObjectClassHandle","HLAsuppliedArguments":[{"HLAargumentType":53,"HLAargumentName":"Object class name","HLAargumentValue":"HLAobjectRoot.Missing"}],"HLAsuccessIndicator":false,"HLAexception":"NameNotFound: The supplied object class name is not defined in this federation execution."})"} +
      R"({"HLAserialNumber":1,"HLAreturnedArgument":[{"HLAargumentType":36,"HLAargumentName":"Object class handle","HLAargumentValue":")" +
      serverValue +
      R"("}],"HLAservice":"GetObjectClassHandle","HLAsuppliedArguments":[{"HLAargumentType":53,"HLAargumentName":"Object class name","HLAargumentValue":"HLAobjectRoot.Employee.Server"}],"HLAsuccessIndicator":true,"HLAexception":null})";
  REQUIRE(readTextFile(reportFile) == initialText + expectedRecords);

  REQUIRE_NOTHROW(owner->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(owner->setSendServiceReportsToFileSwitch(false));
  REQUIRE_NOTHROW(owner->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(owner->disconnect());
}
} // namespace
