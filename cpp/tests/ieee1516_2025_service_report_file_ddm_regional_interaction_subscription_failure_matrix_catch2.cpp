#include "ieee1516_2025_federation_management_fixture_support.hpp"
#include "ieee1516_2025_public_federation_restore_test_support.hpp"

namespace {
using namespace public_federation_restore_test_support;

TEST_CASE(
    "Embedded service reporting records failed regional interaction subscription invocations",
    "[integration][development-profile][interaction-management][ddm]"
    "[mom][service-report-file][service-reporting][service-failure]"
    "[ddm-regional-failure][rti.service.ddm-regional-failure-matrix]") {
  TestFederateAmbassador reports;
  auto owner = makeRti();
  auto const federationName = nextFederationName();
  auto const restaurantFom = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  auto const switchFom =
      (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data" /
       "switch-support-enabled-fom.xml")
          .wstring();
  auto const directory = temporaryServiceReportDirectory();
  auto configuration = configurationForServiceReportDirectory(directory.path());
  configuration.withRtiAddress(L"in-process");

  REQUIRE_NOTHROW(owner->connect(reports, HLA_EVOKED, configuration));
  REQUIRE_NOTHROW(owner->createFederationExecution(
      federationName,
      std::vector<std::wstring>{restaurantFom, switchFom},
      standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(owner->joinFederationExecution(
      L"regional-subscription-failure-owner", L"owner", federationName));
  REQUIRE_NOTHROW(owner->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(owner->setSendServiceReportsToFileSwitch(false));

  auto const interactionClass = owner->getInteractionClassHandle(
      fixture_hla::fom::main_course_served);
  auto const serverId = owner->getDimensionHandle(fixture_hla::fixture::server_id);
  REQUIRE(interactionClass.isValid());
  REQUIRE(serverId.isValid());
  auto const region = owner->createRegion(DimensionHandleSet{serverId});
  REQUIRE_NOTHROW(owner->setRangeBounds(region, serverId, RangeBounds(0UL, 10UL)));
  REQUIRE_NOTHROW(owner->commitRegionModifications(RegionHandleSet{region}));

  auto const files = serviceReportFiles(directory.path());
  REQUIRE(files.size() == 1U);
  auto const reportFile = files.front();
  auto const initialText = readTextFile(reportFile);
  auto const asAscii = [](std::wstring const& value) {
    std::string result;
    result.reserve(value.size());
    for (wchar_t const character : value) {
      REQUIRE(character >= L' ');
      REQUIRE(character <= L'~');
      result.push_back(static_cast<char>(character));
    }
    return result;
  };
  auto const interactionClassValue = asAscii(interactionClass.toString());
  auto const regionValue = asAscii(region.toString());
  auto const invalidInteractionClassValue = asAscii(InteractionClassHandle{}.toString());
  auto const invalidRegionValue = asAscii(RegionHandle{}.toString());
  auto const regionSet = std::string{"[\""} + regionValue + "\"]";
  auto const invalidRegionSet = std::string{"[\""} + invalidRegionValue + "\"]";
  auto const subscribeArguments = [](std::string const& classValue,
                                     std::string const& regionsValue) {
    return std::string{
               "[{\"HLAargumentType\":27,\"HLAargumentName\":\"Interaction class designator\",\"HLAargumentValue\":\""} +
           classValue +
           "\"},{\"HLAargumentType\":43,\"HLAargumentName\":\"Set of region designators\",\"HLAargumentValue\":" +
           regionsValue +
           "},{\"HLAargumentType\":6,\"HLAargumentName\":\"Optional passive subscription indicator\",\"HLAargumentValue\":false}]";
  };
  auto const unsubscribeArguments = [](std::string const& classValue,
                                       std::string const& regionsValue) {
    return std::string{
               "[{\"HLAargumentType\":27,\"HLAargumentName\":\"Interaction class designator\",\"HLAargumentValue\":\""} +
           classValue +
           "\"},{\"HLAargumentType\":43,\"HLAargumentName\":\"Set of region designators\",\"HLAargumentValue\":" +
           regionsValue + "}]";
  };
  auto const reportRecord = [](std::size_t serial,
                               std::string const& service,
                               std::string const& suppliedArguments,
                               bool success,
                               std::string const& exception) {
    return std::string{"{\"HLAserialNumber\":"} + std::to_string(serial) +
           R"(,"HLAreturnedArgument":[null],"HLAservice":")" + service +
           R"(","HLAsuppliedArguments":)" + suppliedArguments +
           R"(,"HLAsuccessIndicator":)" + (success ? "true" : "false") +
           R"(,"HLAexception":)" +
           (success ? "null" : std::string{"\""} + exception + "\"") + "}";
  };

  REQUIRE_NOTHROW(owner->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(owner->setSendServiceReportsToFileSwitch(false));
  REQUIRE_THROWS_AS(
      owner->subscribeInteractionClassWithRegions(InteractionClassHandle{}, {}),
      rti1516_2025::InteractionClassNotDefined);
  REQUIRE_THROWS_AS(
      owner->unsubscribeInteractionClassWithRegions(InteractionClassHandle{}, {}),
      rti1516_2025::InteractionClassNotDefined);
  REQUIRE_THROWS_AS(
      owner->subscribeInteractionClassWithRegions(
          interactionClass,
          RegionHandleSet{RegionHandle{}}),
      rti1516_2025::InvalidRegion);
  REQUIRE(readTextFile(reportFile) == initialText);

  REQUIRE_NOTHROW(owner->setServiceReportingSwitch(true));
  REQUIRE_NOTHROW(owner->setSendServiceReportsToFileSwitch(true));
  REQUIRE_THROWS_AS(
      owner->subscribeInteractionClassWithRegions(InteractionClassHandle{}, {}),
      rti1516_2025::InteractionClassNotDefined);
  REQUIRE_THROWS_AS(
      owner->unsubscribeInteractionClassWithRegions(InteractionClassHandle{}, {}),
      rti1516_2025::InteractionClassNotDefined);
  REQUIRE_NOTHROW(owner->subscribeInteractionClassWithRegions(
      interactionClass,
      RegionHandleSet{region}));
  REQUIRE_THROWS_AS(
      owner->unsubscribeInteractionClassWithRegions(
          interactionClass,
          RegionHandleSet{RegionHandle{}}),
      rti1516_2025::InvalidRegion);
  REQUIRE_NOTHROW(owner->unsubscribeInteractionClassWithRegions(
      interactionClass,
      RegionHandleSet{region}));

  auto const expectedText =
      initialText +
      reportRecord(
          0U,
          "SubscribeInteractionClassWithRegions",
          subscribeArguments(invalidInteractionClassValue, "[]"),
          false,
          "InteractionClassNotDefined: Subscribe Interaction Class With Regions requires a defined InteractionClassHandle.") +
      reportRecord(
          1U,
          "UnsubscribeInteractionClassWithRegions",
          unsubscribeArguments(invalidInteractionClassValue, "[]"),
          false,
          "InteractionClassNotDefined: Unsubscribe Interaction Class With Regions requires a defined InteractionClassHandle.") +
      reportRecord(
          2U,
          "SubscribeInteractionClassWithRegions",
          subscribeArguments(interactionClassValue, regionSet),
          true,
          "") +
      reportRecord(
          3U,
          "UnsubscribeInteractionClassWithRegions",
          unsubscribeArguments(interactionClassValue, invalidRegionSet),
          false,
          "InvalidRegion: Unsubscribe Interaction Class With Regions requires valid RegionHandle values.") +
      reportRecord(
          4U,
          "UnsubscribeInteractionClassWithRegions",
          unsubscribeArguments(interactionClassValue, regionSet),
          true,
          "");
  REQUIRE(readTextFile(reportFile) == expectedText);

  REQUIRE_NOTHROW(owner->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(owner->setSendServiceReportsToFileSwitch(false));
  REQUIRE_NOTHROW(owner->deleteRegion(region));
  REQUIRE_NOTHROW(owner->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(owner->disconnect());
}
}
