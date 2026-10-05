#ifndef UMBRA_SOURCE_DIRECTORY
#error "The 2025 federation-management service-report tests require the Umbra source directory."
#endif

#include "ieee1516_2025_federation_management_fixture_support.hpp"

namespace {

using namespace rti1516_2025;

TEST_CASE(
    "Embedded service reporting preserves Query Attribute Transportation Type arguments",
    "[integration][development-profile][federation-management][object-management]"
    "[transportation-management][mom][service-report-file][service-reporting]"
    "[service-report-query-attribute-transportation-type]"
    "[query-attribute-transportation-type]"
    "[rti.service.publish-object-class-attributes]"
    "[rti.service.register-object-instance]"
    "[rti.service.query-attribute-transportation-type]"
    "[federate.callback.report-attribute-transportation-type]") {
  ReportingFederateAmbassador reports;
  auto rti = makeRti();
  auto const federationName = nextFederationName();
  auto const restaurantFom = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  auto const switchFom =
      (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data" /
       "switch-support-enabled-fom.xml")
          .wstring();
  std::vector<std::wstring> const fomModules{restaurantFom, switchFom};
  auto directory = temporaryServiceReportDirectory();
  auto configuration = configurationForServiceReportDirectory(directory.path());
  configuration.withRtiAddress(L"in-process");

  REQUIRE_NOTHROW(rti->connect(reports, HLA_EVOKED, configuration));
  REQUIRE_NOTHROW(
      rti->createFederationExecution(federationName, fomModules, standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(rti->joinFederationExecution(
      L"query-attribute-transport-report-subject", L"subject", federationName));
  auto const files = serviceReportFiles(directory.path());
  REQUIRE(files.size() == 1U);
  auto const reportFile = files.front();
  auto const initialText = readTextFile(reportFile);

  REQUIRE_NOTHROW(rti->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(rti->setSendServiceReportsToFileSwitch(false));
  auto const objectClass = rti->getObjectClassHandle(fixture_hla::fom::employee_server);
  auto const efficiency = rti->getAttributeHandle(objectClass, fixture_hla::fixture::efficiency);
  AttributeHandleSet const attributes{efficiency};
  auto const reliable = rti->getTransportationTypeHandle(standard_hla::mom::reliable);
  REQUIRE(objectClass.isValid());
  REQUIRE(efficiency.isValid());
  REQUIRE(reliable.isValid());
  REQUIRE_NOTHROW(rti->setServiceReportingSwitch(true));
  REQUIRE_NOTHROW(rti->setSendServiceReportsToFileSwitch(true));

  // An unsuccessful §6.28 invocation has no successful-void report record.
  ObjectInstanceHandle const unknownObject;
  REQUIRE_THROWS_AS(
      rti->queryAttributeTransportationType(unknownObject, efficiency),
      rti1516_2025::ObjectInstanceNotKnown);
  REQUIRE(readTextFile(reportFile) == initialText);

  REQUIRE_NOTHROW(rti->publishObjectClassAttributes(objectClass, attributes));
  auto const textBeforeQuery = readTextFile(reportFile);
  ObjectInstanceHandle objectInstance;
  // Keep setup registration outside this focused query-service report lane.
  REQUIRE_NOTHROW(rti->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(rti->setSendServiceReportsToFileSwitch(false));
  REQUIRE_NOTHROW(objectInstance = rti->registerObjectInstance(objectClass));
  REQUIRE_NOTHROW(rti->setServiceReportingSwitch(true));
  REQUIRE_NOTHROW(rti->setSendServiceReportsToFileSwitch(true));
  REQUIRE_NOTHROW(rti->queryAttributeTransportationType(objectInstance, efficiency));

  auto const objectInstanceText = objectInstance.toString();
  auto const attributeText = efficiency.toString();
  std::string objectInstanceValue;
  objectInstanceValue.reserve(objectInstanceText.size());
  for (wchar_t const character : objectInstanceText) {
    REQUIRE(character >= L' ');
    REQUIRE(character <= L'~');
    objectInstanceValue.push_back(static_cast<char>(character));
  }
  std::string attributeValue;
  attributeValue.reserve(attributeText.size());
  for (wchar_t const character : attributeText) {
    REQUIRE(character >= L' ');
    REQUIRE(character <= L'~');
    attributeValue.push_back(static_cast<char>(character));
  }
  auto const expectedRecord = std::string{
      R"({"HLAserialNumber":1,"HLAreturnedArgument":[null],"HLAservice":"QueryAttributeTransportationType","HLAsuppliedArguments":[{"HLAargumentType":37,"HLAargumentName":"Object instance designator","HLAargumentValue":")" +
      objectInstanceValue +
      R"("},{"HLAargumentType":0,"HLAargumentName":"Attribute designator","HLAargumentValue":")" +
      attributeValue +
      R"("}],"HLAsuccessIndicator":true,"HLAexception":null})"};
  // The service report records accepted query invocation; the separate
  // Report Attribute Transportation Type callback remains later work.
  REQUIRE(readTextFile(reportFile) == textBeforeQuery + expectedRecord);
  REQUIRE(reports.attributeTransportationTypeReports.empty());
  REQUIRE_FALSE(rti->evokeCallback(0.0));
  REQUIRE(reports.attributeTransportationTypeReports.size() == 1U);
  REQUIRE(reports.attributeTransportationTypeReports.front().objectInstance == objectInstance);
  REQUIRE(reports.attributeTransportationTypeReports.front().attribute == efficiency);
  REQUIRE(reports.attributeTransportationTypeReports.front().transportationType == reliable);
  REQUIRE(readTextFile(reportFile) == textBeforeQuery + expectedRecord);

  REQUIRE_NOTHROW(rti->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(rti->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(rti->disconnect());
}

}  // namespace
