#include "ieee1516_2025_federation_management_test_support.hpp"
#include "ieee1516_2025_federation_management_fixture_support.hpp"

namespace {
TEST_CASE(
    "Embedded service reporting records object, attribute, and update-rate lookup return arguments",
    "[integration][development-profile][federation-management][support-services]"
    "[mom][service-report-file][service-reporting][federate-object-attribute-update-rate-lookup-service-reports]"
    "[rti.service.get-known-object-class-handle][rti.service.get-object-instance-handle]"
    "[rti.service.get-object-instance-name][rti.service.get-attribute-handle]"
    "[rti.service.get-attribute-name][rti.service.get-update-rate-value]"
    "[rti.service.get-update-rate-value-for-attribute]") {
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
                      L"lookup-report-owner", L"owner", federationName));

  auto const files = serviceReportFiles(directory.path());
  REQUIRE(files.size() == 1U);
  auto const reportFile = files.front();
  auto const initialText = readTextFile(reportFile);

  REQUIRE_NOTHROW(owner->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(owner->setSendServiceReportsToFileSwitch(false));
  ObjectClassHandle server;
  REQUIRE_NOTHROW(server = owner->getObjectClassHandle(fixture_hla::fom::employee_server));
  AttributeHandle name;
  REQUIRE_NOTHROW(name = owner->getAttributeHandle(server, fixture_hla::fixture::efficiency));
  REQUIRE_NOTHROW(owner->publishObjectClassAttributes(server, AttributeHandleSet{name}));
  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(owner->reserveObjectInstanceName(L"lookup-service-object"));
  REQUIRE_FALSE(owner->evokeCallback(0.0));
  REQUIRE_NOTHROW(
      objectInstance = owner->registerObjectInstance(server, L"lookup-service-object"));
  REQUIRE(readTextFile(reportFile) == initialText);

  REQUIRE_NOTHROW(owner->setServiceReportingSwitch(true));
  REQUIRE_NOTHROW(owner->setSendServiceReportsToFileSwitch(true));
  ObjectClassHandle knownObjectClass;
  REQUIRE_NOTHROW(knownObjectClass = owner->getKnownObjectClassHandle(objectInstance));
  ObjectInstanceHandle objectInstanceByName;
  REQUIRE_NOTHROW(
      objectInstanceByName = owner->getObjectInstanceHandle(L"lookup-service-object"));
  std::wstring objectName;
  REQUIRE_NOTHROW(objectName = owner->getObjectInstanceName(objectInstance));
  AttributeHandle nameByClass;
  REQUIRE_NOTHROW(nameByClass = owner->getAttributeHandle(server, fixture_hla::fixture::efficiency));
  std::wstring classAttributeName;
  REQUIRE_NOTHROW(classAttributeName = owner->getAttributeName(server, name));
  double maximumRate = 0.0;
  REQUIRE_NOTHROW(maximumRate = owner->getUpdateRateValue(L"High"));
  double attributeMaximumRate = 0.0;
  REQUIRE_NOTHROW(
      attributeMaximumRate = owner->getUpdateRateValueForAttribute(objectInstance, name));
  REQUIRE(knownObjectClass == server);
  REQUIRE(objectInstanceByName == objectInstance);
  REQUIRE(objectName == L"lookup-service-object");
  REQUIRE(nameByClass == name);
  REQUIRE(classAttributeName == fixture_hla::fixture::efficiency);
  REQUIRE(maximumRate == Catch::Approx(30.0));
  REQUIRE(attributeMaximumRate == Catch::Approx(0.0));

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
  auto const objectClassValue = asAscii(knownObjectClass.toString());
  auto const objectInstanceValue = asAscii(objectInstance.toString());
  auto const attributeValue = asAscii(name.toString());
  auto const maximumRateValue = asAscii(std::to_wstring(maximumRate));
  auto const attributeMaximumRateValue = asAscii(std::to_wstring(attributeMaximumRate));
  auto const expectedRecords =
      std::string{
          R"({"HLAserialNumber":0,"HLAreturnedArgument":[{"HLAargumentType":36,"HLAargumentName":"Object class handle","HLAargumentValue":")" +
      objectClassValue +
      R"("}],"HLAservice":"GetKnownObjectClassHandle","HLAsuppliedArguments":[{"HLAargumentType":37,"HLAargumentName":"Object instance handle","HLAargumentValue":")" +
      objectInstanceValue +
      R"("}],"HLAsuccessIndicator":true,"HLAexception":null})" +
      R"({"HLAserialNumber":1,"HLAreturnedArgument":[{"HLAargumentType":37,"HLAargumentName":"Object instance handle","HLAargumentValue":")" +
      objectInstanceValue +
      R"("}],"HLAservice":"GetObjectInstanceHandle","HLAsuppliedArguments":[{"HLAargumentType":53,"HLAargumentName":"Object instance name","HLAargumentValue":"lookup-service-object"}],"HLAsuccessIndicator":true,"HLAexception":null})" +
      R"({"HLAserialNumber":2,"HLAreturnedArgument":[{"HLAargumentType":53,"HLAargumentName":"Object instance name","HLAargumentValue":"lookup-service-object"}],"HLAservice":"GetObjectInstanceName","HLAsuppliedArguments":[{"HLAargumentType":37,"HLAargumentName":"Object instance handle","HLAargumentValue":")" +
      objectInstanceValue +
      R"("}],"HLAsuccessIndicator":true,"HLAexception":null})" +
      R"({"HLAserialNumber":3,"HLAreturnedArgument":[{"HLAargumentType":0,"HLAargumentName":"Class attribute handle","HLAargumentValue":")" +
      attributeValue +
      R"("}],"HLAservice":"GetAttributeHandle","HLAsuppliedArguments":[{"HLAargumentType":36,"HLAargumentName":"Object class handle","HLAargumentValue":")" +
      objectClassValue +
      R"("},{"HLAargumentType":53,"HLAargumentName":"Class attribute name","HLAargumentValue":"Efficiency"}],"HLAsuccessIndicator":true,"HLAexception":null})" +
      R"({"HLAserialNumber":4,"HLAreturnedArgument":[{"HLAargumentType":53,"HLAargumentName":"Class attribute name","HLAargumentValue":"Efficiency"}],"HLAservice":"GetAttributeName","HLAsuppliedArguments":[{"HLAargumentType":36,"HLAargumentName":"Object class handle","HLAargumentValue":")" +
      objectClassValue +
      R"("},{"HLAargumentType":0,"HLAargumentName":"Class attribute handle","HLAargumentValue":")" +
      attributeValue +
      R"("}],"HLAsuccessIndicator":true,"HLAexception":null})" +
      R"({"HLAserialNumber":5,"HLAreturnedArgument":[{"HLAargumentType":35,"HLAargumentName":"Maximum update rate value","HLAargumentValue":)" +
      maximumRateValue +
      R"(}],"HLAservice":"GetUpdateRateValue","HLAsuppliedArguments":[{"HLAargumentType":53,"HLAargumentName":"Update rate name","HLAargumentValue":"High"}],"HLAsuccessIndicator":true,"HLAexception":null})" +
      R"({"HLAserialNumber":6,"HLAreturnedArgument":[{"HLAargumentType":35,"HLAargumentName":"Maximum update rate value","HLAargumentValue":)" +
      attributeMaximumRateValue +
      R"(}],"HLAservice":"GetUpdateRateValueForAttribute","HLAsuppliedArguments":[{"HLAargumentType":37,"HLAargumentName":"Object instance handle","HLAargumentValue":")" +
      objectInstanceValue +
      R"("},{"HLAargumentType":0,"HLAargumentName":"Attribute handle","HLAargumentValue":")" +
      attributeValue +
      R"("}],"HLAsuccessIndicator":true,"HLAexception":null})"};
  REQUIRE(readTextFile(reportFile) == initialText + expectedRecords);

  REQUIRE_NOTHROW(owner->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(owner->disconnect());
}
} // namespace
