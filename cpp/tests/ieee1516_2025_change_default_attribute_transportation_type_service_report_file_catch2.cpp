#ifndef UMBRA_SOURCE_DIRECTORY
#error "The 2025 federation-management service-report tests require the Umbra source directory."
#endif

#include "ieee1516_2025_federation_management_fixture_support.hpp"

namespace {

using namespace rti1516_2025;

TEST_CASE(
    "Embedded service reporting preserves Change Default Attribute Transportation Type arguments",
    "[integration][development-profile][federation-management][object-management]"
    "[transportation-management][mom][service-report-file][service-reporting]"
    "[service-report-change-default-attribute-transportation-type]"
    "[change-default-attribute-transportation-type]"
    "[rti.service.change-default-attribute-transportation-type]") {
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
      L"default-attribute-transport-report-subject", L"subject", federationName));
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

  // An unsuccessful §6.27 invocation has no successful-void report record.
  ObjectClassHandle const unknownObjectClass;
  REQUIRE_THROWS_AS(
      rti->changeDefaultAttributeTransportationType(
          unknownObjectClass, attributes, bestEffort),
      rti1516_2025::ObjectClassNotDefined);
  REQUIRE(readTextFile(reportFile) == initialText);

  REQUIRE_NOTHROW(
      rti->changeDefaultAttributeTransportationType(objectClass, attributes, bestEffort));

  auto const objectClassText = objectClass.toString();
  auto const attributeText = efficiency.toString();
  auto const transportationText = bestEffort.toString();
  std::string objectClassValue;
  objectClassValue.reserve(objectClassText.size());
  for (wchar_t const character : objectClassText) {
    REQUIRE(character >= L' ');
    REQUIRE(character <= L'~');
    objectClassValue.push_back(static_cast<char>(character));
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
      R"({"HLAserialNumber":0,"HLAreturnedArgument":[null],"HLAservice":"ChangeDefaultAttributeTransportationType","HLAsuppliedArguments":[{"HLAargumentType":36,"HLAargumentName":"Object class designator","HLAargumentValue":")" +
      objectClassValue +
      R"("},{"HLAargumentType":1,"HLAargumentName":"Set of attribute designators","HLAargumentValue":[")" +
      attributeValue +
      R"("]},{"HLAargumentType":59,"HLAargumentName":"Transportation type","HLAargumentValue":")" +
      transportationValue +
      R"("}],"HLAsuccessIndicator":true,"HLAexception":null})"};
  REQUIRE(readTextFile(reportFile) == initialText + expectedRecord);

  REQUIRE_NOTHROW(rti->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(rti->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(rti->disconnect());
}

}  // namespace
