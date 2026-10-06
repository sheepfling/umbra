#include "ieee1516_2025_federation_management_fixture_support.hpp"
#include "ieee1516_2025_public_federation_restore_test_support.hpp"

namespace {
TEST_CASE(
    "Embedded service reporting records failed object-attribute subscription invocations",
    "[integration][development-profile][federation-management][declaration-management]"
    "[mom][service-report-file][service-reporting][service-failure]"
    "[object-attribute-subscription-failure][rti.service.object-attribute-subscription-failure-matrix]") {
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
      L"object-attribute-subscription-failure-subject", L"subscriber", federationName));
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
  auto const subscribeArguments = [&](std::string const& objectValue,
                                      std::string const& attributeSetValue,
                                      bool passive,
                                      std::string const& rate) {
    return std::vector<std::string>{
        argument(36, "Object class designator", quote(objectValue)),
        argument(1, "Set of attribute designators", attributeSetValue),
        argument(6, "Optional passive subscription indicator", passive ? "true" : "false"),
        argument(53, "Optional update rate designator", quote(rate))};
  };
  auto const unsubscribeArguments = [&](std::string const& objectValue,
                                        std::string const& attributeSetValue) {
    return std::vector<std::string>{
        argument(36, "Object class designator", quote(objectValue)),
        argument(1, "Optional set of attribute designators", attributeSetValue)};
  };
  auto const unsubscribeWholeArguments = [&](std::string const& objectValue) {
    return std::vector<std::string>{
        argument(36, "Object class designator", quote(objectValue)),
        argument(34, "Optional set of attribute designators", "null")};
  };
  auto const invalidSet = std::string{"[\""} + invalidAttributeValue + "\"]";
  auto const validSet = std::string{"[\""} + validAttributeValue + "\"]";

  REQUIRE_THROWS_AS(
      rti->subscribeObjectClassAttributes(
          ObjectClassHandle{}, AttributeHandleSet{}, false, L"High"),
      rti1516_2025::ObjectClassNotDefined);
  auto const failedSubscribeObject = reportRecord(
      0U,
      "SubscribeObjectClassAttributes",
      subscribeArguments(invalidObjectValue, "[]", true, "High"),
      false,
      "ObjectClassNotDefined: Subscribe Object Class Attributes requires a defined ObjectClassHandle.");
  REQUIRE(readTextFile(reportFile) == initialText + failedSubscribeObject);

  REQUIRE_THROWS_AS(
      rti->subscribeObjectClassAttributes(
          objectClass, AttributeHandleSet{AttributeHandle{}}, false, L"High"),
      rti1516_2025::AttributeNotDefined);
  auto const failedSubscribeAttribute = reportRecord(
      1U,
      "SubscribeObjectClassAttributes",
      subscribeArguments(validObjectValue, invalidSet, true, "High"),
      false,
      "AttributeNotDefined: Subscribe Object Class Attributes requires defined AttributeHandle values.");
  REQUIRE(readTextFile(reportFile) == initialText + failedSubscribeObject + failedSubscribeAttribute);

  REQUIRE_THROWS_AS(
      rti->unsubscribeObjectClassAttributes(ObjectClassHandle{}, AttributeHandleSet{}),
      rti1516_2025::ObjectClassNotDefined);
  auto const failedUnsubscribeObject = reportRecord(
      2U,
      "UnsubscribeObjectClassAttributes",
      unsubscribeArguments(invalidObjectValue, "[]"),
      false,
      "ObjectClassNotDefined: Unsubscribe Object Class Attributes requires a defined ObjectClassHandle.");
  REQUIRE(readTextFile(reportFile) ==
          initialText + failedSubscribeObject + failedSubscribeAttribute + failedUnsubscribeObject);

  REQUIRE_THROWS_AS(
      rti->unsubscribeObjectClassAttributes(
          objectClass, AttributeHandleSet{AttributeHandle{}}),
      rti1516_2025::AttributeNotDefined);
  auto const failedUnsubscribeAttribute = reportRecord(
      3U,
      "UnsubscribeObjectClassAttributes",
      unsubscribeArguments(validObjectValue, invalidSet),
      false,
      "AttributeNotDefined: Unsubscribe Object Class Attributes requires defined AttributeHandle values.");
  REQUIRE(readTextFile(reportFile) ==
          initialText + failedSubscribeObject + failedSubscribeAttribute + failedUnsubscribeObject +
              failedUnsubscribeAttribute);

  REQUIRE_THROWS_AS(
      rti->unsubscribeObjectClass(ObjectClassHandle{}),
      rti1516_2025::ObjectClassNotDefined);
  auto const failedUnsubscribeWholeObject = reportRecord(
      4U,
      "UnsubscribeObjectClassAttributes",
      unsubscribeWholeArguments(invalidObjectValue),
      false,
      "ObjectClassNotDefined: Unsubscribe Object Class requires a defined ObjectClassHandle.");
  REQUIRE(readTextFile(reportFile) ==
          initialText + failedSubscribeObject + failedSubscribeAttribute + failedUnsubscribeObject +
              failedUnsubscribeAttribute + failedUnsubscribeWholeObject);

  AttributeHandleSet const attributes{attribute};
  REQUIRE_NOTHROW(rti->subscribeObjectClassAttributes(objectClass, attributes, false, L"High"));
  auto const successfulSubscribe = reportRecord(
      5U,
      "SubscribeObjectClassAttributes",
      subscribeArguments(validObjectValue, validSet, true, "High"),
      true,
      {});
  REQUIRE(readTextFile(reportFile) ==
          initialText + failedSubscribeObject + failedSubscribeAttribute + failedUnsubscribeObject +
              failedUnsubscribeAttribute + failedUnsubscribeWholeObject + successfulSubscribe);

  REQUIRE_NOTHROW(rti->unsubscribeObjectClassAttributes(objectClass, attributes));
  auto const successfulUnsubscribe = reportRecord(
      6U,
      "UnsubscribeObjectClassAttributes",
      unsubscribeArguments(validObjectValue, validSet),
      true,
      {});
  REQUIRE(readTextFile(reportFile) ==
          initialText + failedSubscribeObject + failedSubscribeAttribute + failedUnsubscribeObject +
              failedUnsubscribeAttribute + failedUnsubscribeWholeObject + successfulSubscribe +
              successfulUnsubscribe);

  REQUIRE_NOTHROW(rti->unsubscribeObjectClass(objectClass));
  auto const successfulWholeUnsubscribe = reportRecord(
      7U,
      "UnsubscribeObjectClassAttributes",
      unsubscribeWholeArguments(validObjectValue),
      true,
      {});
  REQUIRE(readTextFile(reportFile) ==
          initialText + failedSubscribeObject + failedSubscribeAttribute + failedUnsubscribeObject +
              failedUnsubscribeAttribute + failedUnsubscribeWholeObject + successfulSubscribe +
              successfulUnsubscribe + successfulWholeUnsubscribe);

  REQUIRE_NOTHROW(rti->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(rti->setSendServiceReportsToFileSwitch(false));
  REQUIRE_NOTHROW(rti->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(rti->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(rti->disconnect());
}

}  // namespace
