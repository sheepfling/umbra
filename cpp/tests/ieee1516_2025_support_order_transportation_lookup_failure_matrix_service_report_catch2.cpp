#include "ieee1516_2025_federation_management_test_support.hpp"
#include "ieee1516_2025_federation_management_fixture_support.hpp"

namespace {
TEST_CASE(
    "Embedded service reporting records failed order and transportation lookups",
    "[integration][development-profile][federation-management][support-services]"
    "[mom][service-report-file][service-reporting][service-failure]"
    "[rti.service.order-transportation-lookup-failure-matrix]") {
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
      L"failed-order-transportation-lookup-report-owner", L"owner", federationName));

  auto const files = serviceReportFiles(directory.path());
  REQUIRE(files.size() == 1U);
  auto const reportFile = files.front();
  auto const initialText = readTextFile(reportFile);

  REQUIRE_NOTHROW(owner->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(owner->setSendServiceReportsToFileSwitch(false));
  REQUIRE_THROWS_AS(
      owner->getOrderType(L"UmbraCustomOrder"),
      rti1516_2025::InvalidOrderName);
  REQUIRE_THROWS_AS(
      owner->getOrderName(static_cast<OrderType>(0x7f)),
      rti1516_2025::InvalidOrderType);
  REQUIRE_THROWS_AS(
      owner->getTransportationTypeHandle(fixture_hla::fixture::umbra_custom_transport),
      rti1516_2025::InvalidTransportationName);
  REQUIRE_THROWS_AS(
      owner->getTransportationTypeName(TransportationTypeHandle{}),
      rti1516_2025::InvalidTransportationTypeHandle);
  REQUIRE(readTextFile(reportFile) == initialText);

  REQUIRE_NOTHROW(owner->setServiceReportingSwitch(true));
  REQUIRE_NOTHROW(owner->setSendServiceReportsToFileSwitch(true));
  REQUIRE_THROWS_AS(
      owner->getOrderType(L"UmbraCustomOrder"),
      rti1516_2025::InvalidOrderName);
  REQUIRE_THROWS_AS(
      owner->getOrderName(static_cast<OrderType>(0x7f)),
      rti1516_2025::InvalidOrderType);
  REQUIRE_THROWS_AS(
      owner->getTransportationTypeHandle(fixture_hla::fixture::umbra_custom_transport),
      rti1516_2025::InvalidTransportationName);
  auto const invalidTransportation = TransportationTypeHandle{};
  REQUIRE_THROWS_AS(
      owner->getTransportationTypeName(invalidTransportation),
      rti1516_2025::InvalidTransportationTypeHandle);
  REQUIRE(owner->getOrderType(L"Receive") == RECEIVE);

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
  auto const invalidTransportationValue = asAscii(invalidTransportation.toString());
  auto const expectedRecords =
      std::string{
          R"({"HLAserialNumber":0,"HLAreturnedArgument":[null],"HLAservice":"GetOrderType","HLAsuppliedArguments":[{"HLAargumentType":53,"HLAargumentName":"Order name","HLAargumentValue":"UmbraCustomOrder"}],"HLAsuccessIndicator":false,"HLAexception":"InvalidOrderName: The embedded profile supports only the Receive and TimeStamp order names."})"} +
      R"({"HLAserialNumber":1,"HLAreturnedArgument":[null],"HLAservice":"GetOrderName","HLAsuppliedArguments":[{"HLAargumentType":38,"HLAargumentName":"Order type","HLAargumentValue":"UNSUPPORTED"}],"HLAsuccessIndicator":false,"HLAexception":"InvalidOrderType: The supplied OrderType is not supported by this embedded profile."})" +
      R"({"HLAserialNumber":2,"HLAreturnedArgument":[null],"HLAservice":"GetTransportationTypeHandle","HLAsuppliedArguments":[{"HLAargumentType":53,"HLAargumentName":"Transportation type name","HLAargumentValue":"UmbraCustomTransport"}],"HLAsuccessIndicator":false,"HLAexception":"InvalidTransportationName: The supplied transportation type name is not declared in this federation execution."})" +
      R"({"HLAserialNumber":3,"HLAreturnedArgument":[null],"HLAservice":"GetTransportationTypeName","HLAsuppliedArguments":[{"HLAargumentType":59,"HLAargumentName":"Transportation type handle","HLAargumentValue":")" +
      invalidTransportationValue +
      R"("}],"HLAsuccessIndicator":false,"HLAexception":"InvalidTransportationTypeHandle: The supplied TransportationTypeHandle is not declared in this federation execution."})" +
      R"({"HLAserialNumber":4,"HLAreturnedArgument":[{"HLAargumentType":38,"HLAargumentName":"Order type","HLAargumentValue":"RECEIVE"}],"HLAservice":"GetOrderType","HLAsuppliedArguments":[{"HLAargumentType":53,"HLAargumentName":"Order name","HLAargumentValue":"Receive"}],"HLAsuccessIndicator":true,"HLAexception":null})";
  REQUIRE(readTextFile(reportFile) == initialText + expectedRecords);

  REQUIRE_NOTHROW(owner->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(owner->setSendServiceReportsToFileSwitch(false));
  REQUIRE_NOTHROW(owner->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(owner->disconnect());
}
} // namespace
