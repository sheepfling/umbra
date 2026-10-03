#include "ieee1516_2025_federation_management_test_support.hpp"
#include "ieee1516_2025_federation_management_fixture_support.hpp"

namespace {
TEST_CASE(
    "Embedded service reporting records failed federate lookup invocations",
    "[integration][development-profile][federation-management][support-services]"
    "[mom][service-report-file][service-reporting][service-failure]"
    "[rti.service.get-federate-handle][rti.service.get-federate-name]"
    "[rti.service.federate-lookup-failure-matrix]") {
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
  FederateHandle ownerFederate;
  REQUIRE_NOTHROW(ownerFederate = owner->joinFederationExecution(
                      L"federate-lookup-failure-owner", L"owner", federationName));

  auto const files = serviceReportFiles(directory.path());
  REQUIRE(files.size() == 1U);
  auto const reportFile = files.front();
  auto const initialText = readTextFile(reportFile);

  // Setup and the first disabled pass must not consume service-report serials.
  REQUIRE_NOTHROW(owner->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(owner->setSendServiceReportsToFileSwitch(false));
  REQUIRE_THROWS_AS(
      owner->getFederateHandle(L"federate-lookup-failure-missing"),
      rti1516_2025::NameNotFound);
  REQUIRE_THROWS_AS(
      owner->getFederateName(FederateHandle{}),
      rti1516_2025::InvalidFederateHandle);
  REQUIRE(readTextFile(reportFile) == initialText);

  REQUIRE_NOTHROW(owner->setServiceReportingSwitch(true));
  REQUIRE_NOTHROW(owner->setSendServiceReportsToFileSwitch(true));
  REQUIRE_THROWS_AS(
      owner->getFederateHandle(L"federate-lookup-failure-missing"),
      rti1516_2025::NameNotFound);
  REQUIRE_THROWS_AS(
      owner->getFederateName(FederateHandle{}),
      rti1516_2025::InvalidFederateHandle);
  REQUIRE(owner->getFederateHandle(L"federate-lookup-failure-owner") == ownerFederate);
  REQUIRE(owner->getFederateName(ownerFederate) == L"federate-lookup-failure-owner");

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
  auto const ownerHandle = asAscii(ownerFederate.toString());
  auto const invalidHandle = asAscii(FederateHandle{}.toString());
  auto const expectedRecords =
      std::string{
          R"({"HLAserialNumber":0,"HLAreturnedArgument":[null],"HLAservice":"GetFederateHandle","HLAsuppliedArguments":[{"HLAargumentType":53,"HLAargumentName":"Federate name","HLAargumentValue":"federate-lookup-failure-missing"}],"HLAsuccessIndicator":false,"HLAexception":"NameNotFound: The supplied federate name is not active in this federation execution."})"} +
      R"({"HLAserialNumber":1,"HLAreturnedArgument":[null],"HLAservice":"GetFederateName","HLAsuppliedArguments":[{"HLAargumentType":15,"HLAargumentName":"Federate handle","HLAargumentValue":")" +
      invalidHandle +
      R"("}],"HLAsuccessIndicator":false,"HLAexception":"InvalidFederateHandle: Get Federate Name requires a valid FederateHandle."})" +
      R"({"HLAserialNumber":2,"HLAreturnedArgument":[{"HLAargumentType":15,"HLAargumentName":"Federate handle","HLAargumentValue":")" +
      ownerHandle +
      R"("}],"HLAservice":"GetFederateHandle","HLAsuppliedArguments":[{"HLAargumentType":53,"HLAargumentName":"Federate name","HLAargumentValue":"federate-lookup-failure-owner"}],"HLAsuccessIndicator":true,"HLAexception":null})" +
      R"({"HLAserialNumber":3,"HLAreturnedArgument":[{"HLAargumentType":53,"HLAargumentName":"Federate name","HLAargumentValue":"federate-lookup-failure-owner"}],"HLAservice":"GetFederateName","HLAsuppliedArguments":[{"HLAargumentType":15,"HLAargumentName":"Federate handle","HLAargumentValue":")" +
      ownerHandle +
      R"("}],"HLAsuccessIndicator":true,"HLAexception":null})";
  REQUIRE(readTextFile(reportFile) == initialText + expectedRecords);

  REQUIRE_NOTHROW(owner->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(owner->setSendServiceReportsToFileSwitch(false));
  REQUIRE_NOTHROW(owner->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(owner->disconnect());
}
} // namespace
