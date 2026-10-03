#include "ieee1516_2025_federation_management_test_support.hpp"
#include "ieee1516_2025_federation_management_fixture_support.hpp"

namespace {
TEST_CASE(
    "Embedded service reporting records order and transportation lookup return arguments",
    "[integration][development-profile][federation-management][support-services]"
    "[mom][service-report-file][service-reporting][transportation-management][time-management]"
    "[order-transportation-lookup-service-reports]"
    "[rti.service.get-order-type][rti.service.get-order-name]"
    "[rti.service.get-transportation-type-handle]"
    "[rti.service.get-transportation-type-name]") {
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
      L"order-transportation-lookup-report-owner", L"owner", federationName));

  auto const files = serviceReportFiles(directory.path());
  REQUIRE(files.size() == 1U);
  auto const reportFile = files.front();
  auto const initialText = readTextFile(reportFile);

  REQUIRE_NOTHROW(owner->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(owner->setSendServiceReportsToFileSwitch(false));
  auto const reliable = owner->getTransportationTypeHandle(standard_hla::mom::reliable);
  auto const receive = owner->getOrderType(L"Receive");
  REQUIRE(reliable.isValid());
  REQUIRE(receive == RECEIVE);
  REQUIRE(readTextFile(reportFile) == initialText);

  REQUIRE_NOTHROW(owner->setServiceReportingSwitch(true));
  REQUIRE_NOTHROW(owner->setSendServiceReportsToFileSwitch(true));
  REQUIRE(owner->getOrderType(L"TimeStamp") == TIMESTAMP);
  REQUIRE(owner->getOrderName(RECEIVE) == L"Receive");
  REQUIRE(owner->getTransportationTypeHandle(standard_hla::mom::reliable) == reliable);
  REQUIRE(owner->getTransportationTypeName(reliable) == standard_hla::mom::reliable);

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
  auto const reliableValue = asAscii(reliable.toString());
  auto const expectedRecords =
      std::string{R"({"HLAserialNumber":0,"HLAreturnedArgument":[{"HLAargumentType":38,"HLAargumentName":"Order type","HLAargumentValue":"TIMESTAMP"}],"HLAservice":"GetOrderType","HLAsuppliedArguments":[{"HLAargumentType":53,"HLAargumentName":"Order name","HLAargumentValue":"TimeStamp"}],"HLAsuccessIndicator":true,"HLAexception":null})"} +
      R"({"HLAserialNumber":1,"HLAreturnedArgument":[{"HLAargumentType":53,"HLAargumentName":"Order name","HLAargumentValue":"Receive"}],"HLAservice":"GetOrderName","HLAsuppliedArguments":[{"HLAargumentType":38,"HLAargumentName":"Order type","HLAargumentValue":"RECEIVE"}],"HLAsuccessIndicator":true,"HLAexception":null})" +
      R"({"HLAserialNumber":2,"HLAreturnedArgument":[{"HLAargumentType":59,"HLAargumentName":"Transportation type handle","HLAargumentValue":")" +
      reliableValue +
      R"("}],"HLAservice":"GetTransportationTypeHandle","HLAsuppliedArguments":[{"HLAargumentType":53,"HLAargumentName":"Transportation type name","HLAargumentValue":"HLAreliable"}],"HLAsuccessIndicator":true,"HLAexception":null})" +
      R"({"HLAserialNumber":3,"HLAreturnedArgument":[{"HLAargumentType":53,"HLAargumentName":"Transportation type name","HLAargumentValue":"HLAreliable"}],"HLAservice":"GetTransportationTypeName","HLAsuppliedArguments":[{"HLAargumentType":59,"HLAargumentName":"Transportation type handle","HLAargumentValue":")" +
      reliableValue +
      R"("}],"HLAsuccessIndicator":true,"HLAexception":null})";
  REQUIRE(readTextFile(reportFile) == initialText + expectedRecords);

  REQUIRE_NOTHROW(owner->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(owner->setSendServiceReportsToFileSwitch(false));
  REQUIRE_NOTHROW(owner->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(owner->disconnect());
}
} // namespace
