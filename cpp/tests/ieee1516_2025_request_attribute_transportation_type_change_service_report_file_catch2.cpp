#ifndef UMBRA_SOURCE_DIRECTORY
#error "The 2025 federation-management service-report tests require the Umbra source directory."
#endif

#include "ieee1516_2025_federation_management_fixture_support.hpp"

namespace {

using namespace rti1516_2025;

TEST_CASE(
    "Embedded service reporting preserves Request Attribute Transportation Type Change arguments",
    "[integration][development-profile][federation-management][object-management]"
    "[transportation-management][mom][service-report-file][service-reporting]"
    "[request-attribute-transportation-type-change]"
    "[rti.service.publish-object-class-attributes]"
    "[rti.service.register-object-instance]"
    "[rti.service.request-attribute-transportation-type-change]"
    "[federate.callback.confirm-attribute-transportation-type-change]") {
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
      L"attribute-transport-report-subject", L"subject", federationName));
  auto const files = serviceReportFiles(directory.path());
  REQUIRE(files.size() == 1U);
  auto const reportFile = files.front();
  auto const initialText = readTextFile(reportFile);

  REQUIRE_NOTHROW(rti->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(rti->setSendServiceReportsToFileSwitch(false));
  auto const objectClass = rti->getObjectClassHandle(fixture_hla::fom::employee_server);
  auto const efficiency = rti->getAttributeHandle(objectClass, fixture_hla::fixture::efficiency);
  AttributeHandleSet const attributes{efficiency};
  auto const bestEffort = rti->getTransportationTypeHandle(standard_hla::mom::best_effort);
  REQUIRE(objectClass.isValid());
  REQUIRE(efficiency.isValid());
  REQUIRE(bestEffort.isValid());
  REQUIRE_NOTHROW(rti->setServiceReportingSwitch(true));
  REQUIRE_NOTHROW(rti->setSendServiceReportsToFileSwitch(true));

  // An unsuccessful §6.25 invocation has no successful-void report record.
  ObjectInstanceHandle const unknownObject;
  REQUIRE_THROWS_AS(
      rti->requestAttributeTransportationTypeChange(unknownObject, attributes, bestEffort),
      rti1516_2025::ObjectInstanceNotKnown);
  REQUIRE(readTextFile(reportFile) == initialText);

  REQUIRE_NOTHROW(rti->publishObjectClassAttributes(objectClass, attributes));
  auto const textBeforeRequest = readTextFile(reportFile);
  ObjectInstanceHandle objectInstance;
  // Keep setup registration outside this focused request-service report lane.
  REQUIRE_NOTHROW(rti->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(rti->setSendServiceReportsToFileSwitch(false));
  REQUIRE_NOTHROW(objectInstance = rti->registerObjectInstance(objectClass));
  REQUIRE_NOTHROW(rti->setServiceReportingSwitch(true));
  REQUIRE_NOTHROW(rti->setSendServiceReportsToFileSwitch(true));
  REQUIRE_NOTHROW(
      rti->requestAttributeTransportationTypeChange(objectInstance, attributes, bestEffort));

  auto const objectInstanceText = objectInstance.toString();
  auto const attributeText = efficiency.toString();
  auto const transportationText = bestEffort.toString();
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
  std::string transportationValue;
  transportationValue.reserve(transportationText.size());
  for (wchar_t const character : transportationText) {
    REQUIRE(character >= L' ');
    REQUIRE(character <= L'~');
    transportationValue.push_back(static_cast<char>(character));
  }
  auto const expectedRecord = std::string{
      R"({"HLAserialNumber":1,"HLAreturnedArgument":[null],"HLAservice":"RequestAttributeTransportationTypeChange","HLAsuppliedArguments":[{"HLAargumentType":37,"HLAargumentName":"Object instance designator","HLAargumentValue":")" +
      objectInstanceValue +
      R"("},{"HLAargumentType":1,"HLAargumentName":"Set of attribute designators","HLAargumentValue":[")" +
      attributeValue +
      R"("]},{"HLAargumentType":59,"HLAargumentName":"Transportation type","HLAargumentValue":")" +
      transportationValue +
      R"("}],"HLAsuccessIndicator":true,"HLAexception":null})"};
  // The report describes an accepted request. The preferred transportation
  // changes only with the separately queued confirmation callback.
  REQUIRE(readTextFile(reportFile) == textBeforeRequest + expectedRecord);
  REQUIRE(reports.attributeTransportationTypeChangeReports.empty());
  REQUIRE_FALSE(rti->evokeCallback(0.0));
  REQUIRE(reports.attributeTransportationTypeChangeReports.size() == 1U);
  REQUIRE(reports.attributeTransportationTypeChangeReports.front().objectInstance ==
          objectInstance);
  REQUIRE(reports.attributeTransportationTypeChangeReports.front().attributes == attributes);
  REQUIRE(reports.attributeTransportationTypeChangeReports.front().transportationType ==
          bestEffort);
  REQUIRE(readTextFile(reportFile) == textBeforeRequest + expectedRecord);

  REQUIRE_NOTHROW(rti->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(rti->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(rti->disconnect());
}

}  // namespace
