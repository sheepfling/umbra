#include "ieee1516_2025_federation_management_fixture_support.hpp"
#include "ieee1516_2025_public_federation_restore_test_support.hpp"

namespace {
TEST_CASE(
    "Embedded service reporting records failed regional object-class subscription invocations",
    "[integration][development-profile][federation-management][object-management][ddm]"
    "[mom][service-report-file][service-reporting][service-failure]"
    "[ddm-regional-object-failure][rti.service.ddm-regional-object-failure-matrix]") {
  using namespace public_federation_restore_test_support;
  TestFederateAmbassador reports;
  auto subscriber = makeRti();
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

  REQUIRE_NOTHROW(subscriber->connect(reports, HLA_EVOKED, configuration));
  REQUIRE_NOTHROW(
      subscriber->createFederationExecution(federationName, fomModules, standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(subscriber->joinFederationExecution(
      L"regional-object-failure-subscriber", L"subscriber", federationName));

  auto const files = serviceReportFiles(directory.path());
  REQUIRE(files.size() == 1U);
  auto const reportFile = files.front();
  auto const initialText = readTextFile(reportFile);

  // Build valid object/attribute/region state while both report switches are
  // disabled so the first selected serial belongs to this failure matrix.
  REQUIRE_NOTHROW(subscriber->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(subscriber->setSendServiceReportsToFileSwitch(false));
  auto const soda = subscriber->getObjectClassHandle(fixture_hla::fom::food_drink_soda);
  auto const flavor = subscriber->getAttributeHandle(soda, fixture_hla::fixture::flavor);
  auto const sodaFlavor = subscriber->getDimensionHandle(fixture_hla::fixture::soda_flavor);
  REQUIRE(soda.isValid());
  REQUIRE(flavor.isValid());
  REQUIRE(sodaFlavor.isValid());
  AttributeHandleSet const flavorOnly{flavor};
  auto const region = subscriber->createRegion(DimensionHandleSet{sodaFlavor});
  REQUIRE_NOTHROW(subscriber->setRangeBounds(region, sodaFlavor, RangeBounds(2UL, 3UL)));
  REQUIRE_NOTHROW(subscriber->commitRegionModifications(RegionHandleSet{region}));
  AttributeHandleSetRegionHandleSetPairVector const validPair{{
      flavorOnly,
      RegionHandleSet{region},
  }};
  AttributeHandleSetRegionHandleSetPairVector const invalidPair{{
      flavorOnly,
      RegionHandleSet{RegionHandle{}},
  }};

  REQUIRE_THROWS_AS(
      subscriber->subscribeObjectClassAttributesWithRegions(
          ObjectClassHandle{}, {}, true, L"High"),
      rti1516_2025::ObjectClassNotDefined);
  REQUIRE_THROWS_AS(
      subscriber->unsubscribeObjectClassAttributesWithRegions(ObjectClassHandle{}, {}),
      rti1516_2025::ObjectClassNotDefined);
  REQUIRE_THROWS_AS(
      subscriber->subscribeObjectClassAttributesWithRegions(soda, invalidPair, true, L"High"),
      rti1516_2025::InvalidRegion);
  REQUIRE_THROWS_AS(
      subscriber->unsubscribeObjectClassAttributesWithRegions(soda, invalidPair),
      rti1516_2025::InvalidRegion);
  REQUIRE(readTextFile(reportFile) == initialText);

  REQUIRE_NOTHROW(subscriber->setServiceReportingSwitch(true));
  REQUIRE_NOTHROW(subscriber->setSendServiceReportsToFileSwitch(true));

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
  auto quote = [](std::string const& value) {
    return std::string{"\""} + value + "\"";
  };
  auto const pairValue = [&](AttributeHandleSetRegionHandleSetPairVector const& pair) {
    if (pair.empty()) {
      return std::string{"[]"};
    }
    auto const& first = pair.front();
    auto const attributeValue = asAscii(first.first.begin()->toString());
    auto const regionValue = asAscii(first.second.begin()->toString());
    return std::string{"[{\"attributeHandleSet\":[\""} + attributeValue +
           "\"],\"regionHandleSet\":[\"" + regionValue + "\"]}]";
  };
  auto const suppliedArgument = [](int type,
                                   std::string const& name,
                                   std::string const& value) {
    return std::string{"{\"HLAargumentType\":"} + std::to_string(type) +
           ",\"HLAargumentName\":\"" + name +
           "\",\"HLAargumentValue\":" + value + "}";
  };
  auto const reportRecord = [&](std::uint32_t serial,
                                std::string const& service,
                                std::vector<std::string> const& arguments,
                                bool success,
                                std::string const& exception) {
    std::string result = std::string{"{\"HLAserialNumber\":"} +
        std::to_string(serial) +
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

  auto const objectClassValue = asAscii(ObjectClassHandle{}.toString());
  auto const sodaValue = asAscii(soda.toString());
  auto const validPairValue = pairValue(validPair);
  auto const invalidPairValue = pairValue(invalidPair);
  auto const emptyPairValue = pairValue({});
  auto const associationName =
      std::string{"Collection of attribute designator set and region designator set pairs"};

  REQUIRE_THROWS_AS(
      subscriber->subscribeObjectClassAttributesWithRegions(
          ObjectClassHandle{}, {}, true, L"High"),
      rti1516_2025::ObjectClassNotDefined);
  auto const failedSubscribeClassRecord = reportRecord(
      0U,
      "SubscribeObjectClassAttributesWithRegions",
      {suppliedArgument(36, "Object class designator", quote(objectClassValue)),
       suppliedArgument(4, associationName, emptyPairValue),
       suppliedArgument(6, "Optional passive subscription indicator", "false"),
       suppliedArgument(53, "Optional update rate designator", quote("High"))},
      false,
      "ObjectClassNotDefined: Subscribe Object Class Attributes With Regions requires a defined ObjectClassHandle.");
  REQUIRE(readTextFile(reportFile) == initialText + failedSubscribeClassRecord);

  REQUIRE_THROWS_AS(
      subscriber->unsubscribeObjectClassAttributesWithRegions(ObjectClassHandle{}, {}),
      rti1516_2025::ObjectClassNotDefined);
  auto const failedUnsubscribeClassRecord = reportRecord(
      1U,
      "UnsubscribeObjectClassAttributesWithRegions",
      {suppliedArgument(36, "Object class designator", quote(objectClassValue)),
       suppliedArgument(4, associationName, emptyPairValue)},
      false,
      "ObjectClassNotDefined: Unsubscribe Object Class Attributes With Regions requires a defined ObjectClassHandle.");
  REQUIRE(readTextFile(reportFile) ==
          initialText + failedSubscribeClassRecord + failedUnsubscribeClassRecord);

  REQUIRE_THROWS_AS(
      subscriber->subscribeObjectClassAttributesWithRegions(soda, invalidPair, true, L"High"),
      rti1516_2025::InvalidRegion);
  auto const failedSubscribeRegionRecord = reportRecord(
      2U,
      "SubscribeObjectClassAttributesWithRegions",
      {suppliedArgument(36, "Object class designator", quote(sodaValue)),
       suppliedArgument(4, associationName, invalidPairValue),
       suppliedArgument(6, "Optional passive subscription indicator", "false"),
       suppliedArgument(53, "Optional update rate designator", quote("High"))},
      false,
      "InvalidRegion: Subscribe Object Class Attributes With Regions requires defined RegionHandle values.");
  REQUIRE(readTextFile(reportFile) ==
          initialText + failedSubscribeClassRecord + failedUnsubscribeClassRecord +
              failedSubscribeRegionRecord);

  REQUIRE_THROWS_AS(
      subscriber->unsubscribeObjectClassAttributesWithRegions(soda, invalidPair),
      rti1516_2025::InvalidRegion);
  auto const failedUnsubscribeRegionRecord = reportRecord(
      3U,
      "UnsubscribeObjectClassAttributesWithRegions",
      {suppliedArgument(36, "Object class designator", quote(sodaValue)),
       suppliedArgument(4, associationName, invalidPairValue)},
      false,
      "InvalidRegion: Unsubscribe Object Class Attributes With Regions requires defined RegionHandle values.");
  REQUIRE(readTextFile(reportFile) ==
          initialText + failedSubscribeClassRecord + failedUnsubscribeClassRecord +
              failedSubscribeRegionRecord + failedUnsubscribeRegionRecord);

  REQUIRE_NOTHROW(
      subscriber->subscribeObjectClassAttributesWithRegions(soda, validPair, true, L"High"));
  auto const successfulSubscribeRecord = reportRecord(
      4U,
      "SubscribeObjectClassAttributesWithRegions",
      {suppliedArgument(36, "Object class designator", quote(sodaValue)),
       suppliedArgument(4, associationName, validPairValue),
       suppliedArgument(6, "Optional passive subscription indicator", "false"),
       suppliedArgument(53, "Optional update rate designator", quote("High"))},
      true,
      {});
  REQUIRE(readTextFile(reportFile) ==
          initialText + failedSubscribeClassRecord + failedUnsubscribeClassRecord +
              failedSubscribeRegionRecord + failedUnsubscribeRegionRecord +
              successfulSubscribeRecord);

  REQUIRE_NOTHROW(subscriber->unsubscribeObjectClassAttributesWithRegions(soda, validPair));
  auto const successfulUnsubscribeRecord = reportRecord(
      5U,
      "UnsubscribeObjectClassAttributesWithRegions",
      {suppliedArgument(36, "Object class designator", quote(sodaValue)),
       suppliedArgument(4, associationName, validPairValue)},
      true,
      {});
  REQUIRE(readTextFile(reportFile) ==
          initialText + failedSubscribeClassRecord + failedUnsubscribeClassRecord +
              failedSubscribeRegionRecord + failedUnsubscribeRegionRecord +
              successfulSubscribeRecord + successfulUnsubscribeRecord);

  REQUIRE_NOTHROW(subscriber->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(subscriber->setSendServiceReportsToFileSwitch(false));
  REQUIRE_NOTHROW(subscriber->deleteRegion(region));
  REQUIRE_NOTHROW(subscriber->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(subscriber->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(subscriber->disconnect());
}

}  // namespace
