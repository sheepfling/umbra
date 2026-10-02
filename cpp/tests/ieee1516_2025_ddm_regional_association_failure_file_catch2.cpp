#include "ieee1516_2025_federation_management_test_support.hpp"
#include "ieee1516_2025_federation_management_fixture_support.hpp"

namespace {
TEST_CASE(
    "Embedded service reporting records failed regional object-attribute association invocations",
    "[integration][development-profile][federation-management][object-management][ddm]"
    "[mom][service-report-file][service-reporting][service-failure]"
    "[ddm-regional-association-failure][rti.service.ddm-regional-association-failure-matrix]"
    "[2025]") {
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
      L"regional-association-failure-owner", L"owner", federationName));

  auto const files = serviceReportFiles(directory.path());
  REQUIRE(files.size() == 1U);
  auto const reportFile = files.front();
  auto const initialText = readTextFile(reportFile);

  REQUIRE_NOTHROW(owner->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(owner->setSendServiceReportsToFileSwitch(false));
  auto const soda = owner->getObjectClassHandle(fixture_hla::fom::food_drink_soda);
  auto const flavor = owner->getAttributeHandle(soda, fixture_hla::fixture::flavor);
  auto const sodaFlavor = owner->getDimensionHandle(fixture_hla::fixture::soda_flavor);
  REQUIRE(soda.isValid());
  REQUIRE(flavor.isValid());
  REQUIRE(sodaFlavor.isValid());
  AttributeHandleSet const flavorOnly{flavor};
  REQUIRE_NOTHROW(owner->publishObjectClassAttributes(soda, flavorOnly));
  auto const ownerRegion = owner->createRegion(DimensionHandleSet{sodaFlavor});
  REQUIRE_NOTHROW(owner->setRangeBounds(ownerRegion, sodaFlavor, RangeBounds(0UL, 1UL)));
  REQUIRE_NOTHROW(owner->commitRegionModifications(RegionHandleSet{ownerRegion}));
  AttributeHandleSetRegionHandleSetPairVector const ownerPair{{
      flavorOnly,
      RegionHandleSet{ownerRegion},
  }};
  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = owner->registerObjectInstanceWithRegions(soda, ownerPair));
  static_cast<void>(owner->evokeMultipleCallbacks(0.0, 0.0));
  auto const disjointRegion = owner->createRegion(DimensionHandleSet{sodaFlavor});
  REQUIRE_NOTHROW(owner->setRangeBounds(disjointRegion, sodaFlavor, RangeBounds(2UL, 3UL)));
  REQUIRE_NOTHROW(owner->commitRegionModifications(RegionHandleSet{disjointRegion}));
  AttributeHandleSetRegionHandleSetPairVector const validPair{{
      flavorOnly,
      RegionHandleSet{disjointRegion},
  }};
  AttributeHandleSetRegionHandleSetPairVector const invalidPair{{
      flavorOnly,
      RegionHandleSet{RegionHandle{}},
  }};

  REQUIRE_THROWS_AS(
      owner->associateRegionsForUpdates(ObjectInstanceHandle{}, {}),
      rti1516_2025::ObjectInstanceNotKnown);
  REQUIRE_THROWS_AS(
      owner->unassociateRegionsForUpdates(ObjectInstanceHandle{}, {}),
      rti1516_2025::ObjectInstanceNotKnown);
  REQUIRE_THROWS_AS(
      owner->associateRegionsForUpdates(objectInstance, invalidPair),
      rti1516_2025::InvalidRegion);
  REQUIRE_THROWS_AS(
      owner->unassociateRegionsForUpdates(objectInstance, invalidPair),
      rti1516_2025::InvalidRegion);
  REQUIRE(readTextFile(reportFile) == initialText);

  REQUIRE_NOTHROW(owner->setServiceReportingSwitch(true));
  REQUIRE_NOTHROW(owner->setSendServiceReportsToFileSwitch(true));

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

  auto const objectValue = asAscii(ObjectInstanceHandle{}.toString());
  auto const validObjectValue = asAscii(objectInstance.toString());
  auto const validPairValue = pairValue(validPair);
  auto const invalidPairValue = pairValue(invalidPair);
  auto const emptyPairValue = pairValue({});
  auto const associationName =
      std::string{"Collection of attribute designator set and region designator set pairs"};

  REQUIRE_THROWS_AS(
      owner->associateRegionsForUpdates(ObjectInstanceHandle{}, {}),
      rti1516_2025::ObjectInstanceNotKnown);
  auto const failedAssociateObjectRecord = reportRecord(
      0U,
      "AssociateRegionsForUpdates",
      {suppliedArgument(37, "Object instance designator", quote(objectValue)),
       suppliedArgument(4, associationName, emptyPairValue)},
      false,
      "ObjectInstanceNotKnown: Associate Regions For Updates requires a known ObjectInstanceHandle.");
  REQUIRE(readTextFile(reportFile) == initialText + failedAssociateObjectRecord);

  REQUIRE_THROWS_AS(
      owner->unassociateRegionsForUpdates(ObjectInstanceHandle{}, {}),
      rti1516_2025::ObjectInstanceNotKnown);
  auto const failedUnassociateObjectRecord = reportRecord(
      1U,
      "UnassociateRegionsForUpdates",
      {suppliedArgument(37, "Object instance designator", quote(objectValue)),
       suppliedArgument(4, associationName, emptyPairValue)},
      false,
      "ObjectInstanceNotKnown: Unassociate Regions For Updates requires a known ObjectInstanceHandle.");
  REQUIRE(readTextFile(reportFile) ==
          initialText + failedAssociateObjectRecord + failedUnassociateObjectRecord);

  REQUIRE_THROWS_AS(owner->associateRegionsForUpdates(objectInstance, invalidPair),
                    rti1516_2025::InvalidRegion);
  auto const failedAssociateRegionRecord = reportRecord(
      2U,
      "AssociateRegionsForUpdates",
      {suppliedArgument(37, "Object instance designator", quote(validObjectValue)),
       suppliedArgument(4, associationName, invalidPairValue)},
      false,
      "InvalidRegion: Associate Regions For Updates requires defined RegionHandle values.");
  REQUIRE(readTextFile(reportFile) ==
          initialText + failedAssociateObjectRecord + failedUnassociateObjectRecord +
              failedAssociateRegionRecord);

  REQUIRE_THROWS_AS(owner->unassociateRegionsForUpdates(objectInstance, invalidPair),
                    rti1516_2025::InvalidRegion);
  auto const failedUnassociateRegionRecord = reportRecord(
      3U,
      "UnassociateRegionsForUpdates",
      {suppliedArgument(37, "Object instance designator", quote(validObjectValue)),
       suppliedArgument(4, associationName, invalidPairValue)},
      false,
      "InvalidRegion: Unassociate Regions For Updates requires defined RegionHandle values.");
  REQUIRE(readTextFile(reportFile) ==
          initialText + failedAssociateObjectRecord + failedUnassociateObjectRecord +
              failedAssociateRegionRecord + failedUnassociateRegionRecord);

  REQUIRE_NOTHROW(owner->associateRegionsForUpdates(objectInstance, validPair));
  auto const successfulAssociateRecord = reportRecord(
      4U,
      "AssociateRegionsForUpdates",
      {suppliedArgument(37, "Object instance designator", quote(validObjectValue)),
       suppliedArgument(4, associationName, validPairValue)},
      true,
      {});
  REQUIRE(readTextFile(reportFile) ==
          initialText + failedAssociateObjectRecord + failedUnassociateObjectRecord +
              failedAssociateRegionRecord + failedUnassociateRegionRecord +
              successfulAssociateRecord);

  REQUIRE_NOTHROW(owner->unassociateRegionsForUpdates(objectInstance, validPair));
  auto const successfulUnassociateRecord = reportRecord(
      5U,
      "UnassociateRegionsForUpdates",
      {suppliedArgument(37, "Object instance designator", quote(validObjectValue)),
       suppliedArgument(4, associationName, validPairValue)},
      true,
      {});
  REQUIRE(readTextFile(reportFile) ==
          initialText + failedAssociateObjectRecord + failedUnassociateObjectRecord +
              failedAssociateRegionRecord + failedUnassociateRegionRecord +
              successfulAssociateRecord + successfulUnassociateRecord);

  REQUIRE_NOTHROW(owner->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(owner->setSendServiceReportsToFileSwitch(false));
  REQUIRE_NOTHROW(owner->unassociateRegionsForUpdates(objectInstance, ownerPair));
  REQUIRE_NOTHROW(owner->deleteRegion(disjointRegion));
  REQUIRE_NOTHROW(owner->deleteRegion(ownerRegion));
  REQUIRE_NOTHROW(owner->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(owner->disconnect());
}
} // namespace
