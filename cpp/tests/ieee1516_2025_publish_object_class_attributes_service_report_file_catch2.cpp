#include "ieee1516_2025_federation_management_test_support.hpp"
#include "ieee1516_2025_federation_management_fixture_support.hpp"

namespace {
TEST_CASE(
    "Embedded service reporting preserves Publish Object Class Attributes arguments",
    "[integration][development-profile][federation-management][declaration-management]"
    "[mom][service-report-file][service-reporting]"
    "[publish-object-class-attributes-service-report-file]"
    "[attribute-handle-set-array-encoding]"
    "[publish-object-class-attributes]"
    "[rti.service.publish-object-class-attributes]") {
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
      rti->createFederationExecution(federationName, fomModules, L"HLAinteger64Time"));
  REQUIRE_NOTHROW(
      rti->joinFederationExecution(L"publish-object-class-report-subject", L"publisher", federationName));

  auto const files = serviceReportFiles(directory.path());
  REQUIRE(files.size() == 1U);
  auto const reportFile = files.front();
  auto const initialText = readTextFile(reportFile);

  // The dedicated object-attribute failure matrix owns failed-service
  // records; keep this argument regression focused on accepted forms.
  REQUIRE_NOTHROW(rti->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(rti->setSendServiceReportsToFileSwitch(false));
  ObjectClassHandle const unknownObjectClass;
  REQUIRE_THROWS_AS(
      rti->publishObjectClassAttributes(unknownObjectClass, AttributeHandleSet{}),
      rti1516_2025::ObjectClassNotDefined);
  REQUIRE(readTextFile(reportFile) == initialText);

  auto const objectClass = rti->getObjectClassHandle(L"HLAobjectRoot.Employee.Server");
  auto const attribute = rti->getAttributeHandle(objectClass, L"Efficiency");
  auto const additionalAttribute = rti->getAttributeHandle(objectClass, L"Cheerfulness");
  REQUIRE(objectClass.isValid());
  REQUIRE(attribute.isValid());
  REQUIRE(additionalAttribute.isValid());
  // Two elements ensure the Table 5 AttributeHandleSet is represented as an
  // array of handles rather than accidentally passing as a scalar.
  AttributeHandleSet const attributes{attribute, additionalAttribute};
  REQUIRE_NOTHROW(rti->setServiceReportingSwitch(true));
  REQUIRE_NOTHROW(rti->setSendServiceReportsToFileSwitch(true));
  REQUIRE_NOTHROW(rti->publishObjectClassAttributes(objectClass, attributes));

  auto const objectClassText = objectClass.toString();
  std::string objectClassValue;
  objectClassValue.reserve(objectClassText.size());
  for (wchar_t const character : objectClassText) {
    REQUIRE(character >= L' ');
    REQUIRE(character <= L'~');
    objectClassValue.push_back(static_cast<char>(character));
  }
  std::string attributeValues{"["};
  bool firstAttribute = true;
  for (auto const& attributeHandle : attributes) {
    if (!firstAttribute) {
      attributeValues.push_back(',');
    }
    firstAttribute = false;
    auto const attributeText = attributeHandle.toString();
    std::string attributeValue;
    attributeValue.reserve(attributeText.size());
    for (wchar_t const character : attributeText) {
      REQUIRE(character >= L' ');
      REQUIRE(character <= L'~');
      attributeValue.push_back(static_cast<char>(character));
    }
    attributeValues.push_back('"');
    attributeValues += attributeValue;
    attributeValues.push_back('"');
  }
  attributeValues.push_back(']');
  auto const expectedRecord = std::string{
      R"({"HLAserialNumber":0,"HLAreturnedArgument":[null],"HLAservice":"PublishObjectClassAttributes","HLAsuppliedArguments":[{"HLAargumentType":36,"HLAargumentName":"Object class designator","HLAargumentValue":")" +
      objectClassValue +
      R"("},{"HLAargumentType":1,"HLAargumentName":"Set of attribute designators","HLAargumentValue":)" +
      attributeValues +
      R"(}],"HLAsuccessIndicator":true,"HLAexception":null})"};
  REQUIRE(readTextFile(reportFile) == initialText + expectedRecord);
  auto const publicationMarker = std::string_view{
      "\"HLAservice\":\"PublishObjectClassAttributes\""};
  auto const countPublicationRecords = [publicationMarker](std::string const& text) {
    std::size_t count = 0U;
    std::size_t offset = 0U;
    while ((offset = text.find(publicationMarker, offset)) != std::string::npos) {
      ++count;
      offset += publicationMarker.size();
    }
    return count;
  };
  REQUIRE(countPublicationRecords(readTextFile(reportFile)) == 1U);

  // The file switch gates writes to this already-published path. Keep the
  // service-report interaction switch enabled so this specifically exercises
  // file-disable/re-enable behavior, not the broader reporting switch.
  REQUIRE_NOTHROW(rti->setSendServiceReportsToFileSwitch(false));
  REQUIRE_NOTHROW(rti->unpublishObjectClassAttributes(objectClass, attributes));
  REQUIRE(serviceReportFiles(directory.path()) == files);
  REQUIRE(readTextFile(reportFile) == initialText + expectedRecord);

  REQUIRE_NOTHROW(rti->setSendServiceReportsToFileSwitch(true));
  REQUIRE_NOTHROW(rti->publishObjectClassAttributes(objectClass, attributes));
  auto const reenabledText = readTextFile(reportFile);
  REQUIRE(serviceReportFiles(directory.path()) == files);
  REQUIRE(reenabledText.size() > initialText.size() + expectedRecord.size());
  REQUIRE(countPublicationRecords(reenabledText) == 2U);
  REQUIRE_FALSE(rti->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(readTextFile(reportFile) == reenabledText);

  REQUIRE_NOTHROW(rti->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(rti->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(rti->disconnect());
}

}  // namespace
