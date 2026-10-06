#include "ieee1516_2025_federation_management_fixture_support.hpp"
#include "ieee1516_2025_public_federation_restore_test_support.hpp"

namespace {
TEST_CASE(
    "Embedded service reporting records failed object-attribute declaration invocations",
    "[integration][development-profile][federation-management][declaration-management]"
    "[mom][service-report-file][service-reporting][service-failure]"
    "[object-attribute-declaration-failure][rti.service.object-attribute-declaration-failure-matrix]") {
  using namespace public_federation_restore_test_support;
  TestFederateAmbassador reports;
  auto rti = makeRti();
  auto const federationName = nextFederationName();
  auto const restaurantFom = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  auto const switchFom =
      (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data" /
       "switch-support-enabled-fom.xml")
          .wstring();
  auto directory = temporaryServiceReportDirectory();
  auto configuration = configurationForServiceReportDirectory(directory.path());
  configuration.withRtiAddress(L"in-process");

  REQUIRE_NOTHROW(rti->connect(reports, HLA_EVOKED, configuration));
  REQUIRE_NOTHROW(rti->createFederationExecution(
      federationName,
      std::vector<std::wstring>{restaurantFom, switchFom},
      standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(rti->joinFederationExecution(
      L"object-attribute-declaration-failure-subject", L"publisher", federationName));
  auto const files = serviceReportFiles(directory.path());
  REQUIRE(files.size() == 1U);
  auto const reportFile = files.front();
  auto const initialText = readTextFile(reportFile);

  REQUIRE_NOTHROW(rti->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(rti->setSendServiceReportsToFileSwitch(false));
  auto const objectClass = rti->getObjectClassHandle(fixture_hla::fom::employee_server);
  auto const attribute = rti->getAttributeHandle(objectClass, fixture_hla::fixture::efficiency);
  REQUIRE(objectClass.isValid());
  REQUIRE(attribute.isValid());
  REQUIRE_NOTHROW(rti->setServiceReportingSwitch(true));
  REQUIRE_NOTHROW(rti->setSendServiceReportsToFileSwitch(true));

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
  auto const quote = [](std::string const& value) {
    return std::string{"\""} + value + "\"";
  };
  auto const argument = [](int type, std::string const& name, std::string const& value) {
    return std::string{"{\"HLAargumentType\":"} + std::to_string(type) +
        ",\"HLAargumentName\":\"" + name + "\",\"HLAargumentValue\":" + value + "}";
  };
  auto const reportRecord = [&quote](std::uint32_t serial,
                                     std::string const& service,
                                     std::vector<std::string> const& arguments,
                                     bool success,
                                     std::string const& exception) {
    std::string result = std::string{"{\"HLAserialNumber\":"} + std::to_string(serial) +
        ",\"HLAreturnedArgument\":[null],\"HLAservice\":\"" + service +
        "\",\"HLAsuppliedArguments\":[";
    for (std::size_t index = 0U; index < arguments.size(); ++index) {
      if (index != 0U) {
        result += ',';
      }
      result += arguments[index];
    }
    result += std::string{
        "],\"HLAsuccessIndicator\":"} + (success ? "true" : "false") +
        ",\"HLAexception\":" +
        (exception.empty() ? std::string{"null"} : quote(exception)) + "}";
    return result;
  };
  auto const invalidObjectValue = asAscii(ObjectClassHandle{}.toString());
  auto const validObjectValue = asAscii(objectClass.toString());
  auto const invalidAttributeValue = asAscii(AttributeHandle{}.toString());
  auto const validAttributeValue = asAscii(attribute.toString());
  auto const publishArguments = [&](std::string const& objectValue,
                                    std::string const& attributeSetValue) {
    return std::vector<std::string>{
        argument(36, "Object class designator", quote(objectValue)),
        argument(1, "Set of attribute designators", attributeSetValue)};
  };
  auto const unpublishArguments = [&](std::string const& objectValue,
                                      std::string const& attributeSetValue) {
    return std::vector<std::string>{
        argument(36, "Object class designator", quote(objectValue)),
        argument(1, "Optional set of attribute designators", attributeSetValue)};
  };
  auto const invalidSet = std::string{"[\""} + invalidAttributeValue + "\"]";
  auto const validSet = std::string{"[\""} + validAttributeValue + "\"]";

  REQUIRE_THROWS_AS(
      rti->publishObjectClassAttributes(ObjectClassHandle{}, AttributeHandleSet{}),
      rti1516_2025::ObjectClassNotDefined);
  auto const failedPublishObject = reportRecord(
      0U,
      "PublishObjectClassAttributes",
      publishArguments(invalidObjectValue, "[]"),
      false,
      "ObjectClassNotDefined: Publish Object Class Attributes requires a defined ObjectClassHandle.");
  REQUIRE(readTextFile(reportFile) == initialText + failedPublishObject);

  REQUIRE_THROWS_AS(
      rti->publishObjectClassAttributes(objectClass, AttributeHandleSet{AttributeHandle{}}),
      rti1516_2025::AttributeNotDefined);
  auto const failedPublishAttribute = reportRecord(
      1U,
      "PublishObjectClassAttributes",
      publishArguments(validObjectValue, invalidSet),
      false,
      "AttributeNotDefined: Publish Object Class Attributes requires defined AttributeHandle values.");
  REQUIRE(readTextFile(reportFile) == initialText + failedPublishObject + failedPublishAttribute);

  REQUIRE_THROWS_AS(
      rti->unpublishObjectClassAttributes(ObjectClassHandle{}, AttributeHandleSet{}),
      rti1516_2025::ObjectClassNotDefined);
  auto const failedUnpublishObject = reportRecord(
      2U,
      "UnpublishObjectClassAttributes",
      unpublishArguments(invalidObjectValue, "[]"),
      false,
      "ObjectClassNotDefined: Unpublish Object Class Attributes requires a defined ObjectClassHandle.");
  REQUIRE(readTextFile(reportFile) ==
          initialText + failedPublishObject + failedPublishAttribute + failedUnpublishObject);

  REQUIRE_THROWS_AS(
      rti->unpublishObjectClassAttributes(
          objectClass, AttributeHandleSet{AttributeHandle{}}),
      rti1516_2025::AttributeNotDefined);
  auto const failedUnpublishAttribute = reportRecord(
      3U,
      "UnpublishObjectClassAttributes",
      unpublishArguments(validObjectValue, invalidSet),
      false,
      "AttributeNotDefined: Unpublish Object Class Attributes requires defined AttributeHandle values.");
  REQUIRE(readTextFile(reportFile) ==
          initialText + failedPublishObject + failedPublishAttribute + failedUnpublishObject +
              failedUnpublishAttribute);

  AttributeHandleSet const attributes{attribute};
  REQUIRE_NOTHROW(rti->publishObjectClassAttributes(objectClass, attributes));
  auto const successfulPublish = reportRecord(
      4U,
      "PublishObjectClassAttributes",
      publishArguments(validObjectValue, validSet),
      true,
      {});
  REQUIRE(readTextFile(reportFile) ==
          initialText + failedPublishObject + failedPublishAttribute + failedUnpublishObject +
              failedUnpublishAttribute + successfulPublish);

  REQUIRE_NOTHROW(rti->unpublishObjectClassAttributes(objectClass, attributes));
  auto const successfulUnpublish = reportRecord(
      5U,
      "UnpublishObjectClassAttributes",
      unpublishArguments(validObjectValue, validSet),
      true,
      {});
  REQUIRE(readTextFile(reportFile) ==
          initialText + failedPublishObject + failedPublishAttribute + failedUnpublishObject +
              failedUnpublishAttribute + successfulPublish + successfulUnpublish);

  REQUIRE_NOTHROW(rti->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(rti->setSendServiceReportsToFileSwitch(false));
  REQUIRE_NOTHROW(rti->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(rti->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(rti->disconnect());
}

}  // namespace
