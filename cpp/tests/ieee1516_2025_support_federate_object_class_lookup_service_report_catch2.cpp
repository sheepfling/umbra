#include "ieee1516_2025_federation_management_test_support.hpp"
#include "ieee1516_2025_federation_management_fixture_support.hpp"

namespace {
TEST_CASE(
    "Embedded service reporting records federate and object-class lookup return arguments",
    "[integration][development-profile][federation-management][support-services]"
    "[mom][service-report-file][service-reporting][federation-object-lookup-service-reports]"
    "[rti.service.get-federate-handle][rti.service.get-federate-name]"
    "[rti.service.get-object-class-handle][rti.service.get-object-class-name]") {
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
                      L"federation-object-lookup-report-owner", L"owner", federationName));

  auto const files = serviceReportFiles(directory.path());
  REQUIRE(files.size() == 1U);
  auto const reportFile = files.front();
  auto const initialText = readTextFile(reportFile);

  FederateHandle federate;
  REQUIRE_NOTHROW(
      federate = owner->getFederateHandle(L"federation-object-lookup-report-owner"));
  std::wstring federateName;
  REQUIRE_NOTHROW(federateName = owner->getFederateName(ownerFederate));
  ObjectClassHandle server;
  REQUIRE_NOTHROW(server = owner->getObjectClassHandle(fixture_hla::fom::employee_server));
  std::wstring serverName;
  REQUIRE_NOTHROW(serverName = owner->getObjectClassName(server));
  REQUIRE(federate == ownerFederate);
  REQUIRE(federateName == L"federation-object-lookup-report-owner");
  REQUIRE(serverName == fixture_hla::fom::employee_server);

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
  auto const federateValue = asAscii(federate.toString());
  auto const serverValue = asAscii(server.toString());
  auto const expectedRecords =
      std::string{
          R"({"HLAserialNumber":0,"HLAreturnedArgument":[{"HLAargumentType":15,"HLAargumentName":"Federate handle","HLAargumentValue":")" +
      federateValue +
      R"("}],"HLAservice":"GetFederateHandle","HLAsuppliedArguments":[{"HLAargumentType":53,"HLAargumentName":"Federate name","HLAargumentValue":"federation-object-lookup-report-owner"}],"HLAsuccessIndicator":true,"HLAexception":null})" +
      R"({"HLAserialNumber":1,"HLAreturnedArgument":[{"HLAargumentType":53,"HLAargumentName":"Federate name","HLAargumentValue":"federation-object-lookup-report-owner"}],"HLAservice":"GetFederateName","HLAsuppliedArguments":[{"HLAargumentType":15,"HLAargumentName":"Federate handle","HLAargumentValue":")" +
      federateValue +
      R"("}],"HLAsuccessIndicator":true,"HLAexception":null})" +
      R"({"HLAserialNumber":2,"HLAreturnedArgument":[{"HLAargumentType":36,"HLAargumentName":"Object class handle","HLAargumentValue":")" +
      serverValue +
      R"("}],"HLAservice":"GetObjectClassHandle","HLAsuppliedArguments":[{"HLAargumentType":53,"HLAargumentName":"Object class name","HLAargumentValue":"HLAobjectRoot.Employee.Server"}],"HLAsuccessIndicator":true,"HLAexception":null})" +
      R"({"HLAserialNumber":3,"HLAreturnedArgument":[{"HLAargumentType":53,"HLAargumentName":"Object class name","HLAargumentValue":"HLAobjectRoot.Employee.Server"}],"HLAservice":"GetObjectClassName","HLAsuppliedArguments":[{"HLAargumentType":36,"HLAargumentName":"Object class handle","HLAargumentValue":")" +
      serverValue +
      R"("}],"HLAsuccessIndicator":true,"HLAexception":null})"};
  REQUIRE(readTextFile(reportFile) == initialText + expectedRecords);

  REQUIRE_NOTHROW(owner->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(owner->disconnect());
}
} // namespace
