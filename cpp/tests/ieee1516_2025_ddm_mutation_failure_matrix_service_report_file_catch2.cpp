#include "ieee1516_2025_federation_management_test_support.hpp"
#include "ieee1516_2025_federation_management_fixture_support.hpp"

namespace {
TEST_CASE(
    "Embedded service reporting records failed DDM region mutation invocations",
    "[integration][development-profile][federation-management][ddm]"
    "[mom][service-report-file][service-reporting][service-failure]"
    "[ddm-mutation-failure][rti.service.ddm-mutation-failure-matrix]") {
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
      L"failed-ddm-mutation-report-owner", L"owner", federationName));

  auto const files = serviceReportFiles(directory.path());
  REQUIRE(files.size() == 1U);
  auto const reportFile = files.front();
  auto const initialText = readTextFile(reportFile);

  REQUIRE_NOTHROW(owner->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(owner->setSendServiceReportsToFileSwitch(false));
  auto const barQuantity = owner->getDimensionHandle(fixture_hla::fixture::bar_quantity);
  REQUIRE(barQuantity.isValid());
  auto const existingRegion = owner->createRegion(DimensionHandleSet{barQuantity});
  auto const deletableRegion = owner->createRegion(DimensionHandleSet{barQuantity});
  REQUIRE_THROWS_AS(
      owner->commitRegionModifications(RegionHandleSet{RegionHandle{}}),
      rti1516_2025::InvalidRegion);
  REQUIRE_THROWS_AS(owner->deleteRegion(RegionHandle{}), rti1516_2025::InvalidRegion);
  REQUIRE_THROWS_AS(
      owner->setRangeBounds(existingRegion, DimensionHandle{}, RangeBounds(0UL, 10UL)),
      rti1516_2025::RegionDoesNotContainSpecifiedDimension);
  REQUIRE_THROWS_AS(
      owner->setRangeBounds(existingRegion, barQuantity, RangeBounds(8UL, 8UL)),
      rti1516_2025::InvalidRangeBound);
  REQUIRE(readTextFile(reportFile) == initialText);

  REQUIRE_NOTHROW(owner->setServiceReportingSwitch(true));
  REQUIRE_NOTHROW(owner->setSendServiceReportsToFileSwitch(true));
  REQUIRE_THROWS_AS(
      owner->commitRegionModifications(RegionHandleSet{RegionHandle{}}),
      rti1516_2025::InvalidRegion);
  REQUIRE_THROWS_AS(owner->deleteRegion(RegionHandle{}), rti1516_2025::InvalidRegion);
  REQUIRE_THROWS_AS(
      owner->setRangeBounds(existingRegion, DimensionHandle{}, RangeBounds(0UL, 10UL)),
      rti1516_2025::RegionDoesNotContainSpecifiedDimension);
  REQUIRE_THROWS_AS(
      owner->setRangeBounds(existingRegion, barQuantity, RangeBounds(8UL, 8UL)),
      rti1516_2025::InvalidRangeBound);
  REQUIRE_NOTHROW(owner->setRangeBounds(
      existingRegion, barQuantity, RangeBounds(2UL, 8UL)));
  REQUIRE_NOTHROW(owner->commitRegionModifications(RegionHandleSet{existingRegion}));
  REQUIRE_NOTHROW(owner->deleteRegion(deletableRegion));

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
  auto const invalidRegionValue = asAscii(RegionHandle{}.toString());
  auto const invalidDimensionValue = asAscii(DimensionHandle{}.toString());
  auto const barQuantityValue = asAscii(barQuantity.toString());
  auto const existingRegionValue = asAscii(existingRegion.toString());
  auto const deletableRegionValue = asAscii(deletableRegion.toString());
  auto const expectedRecords =
      std::string{
          R"({"HLAserialNumber":0,"HLAreturnedArgument":[null],"HLAservice":"CommitRegionModifications","HLAsuppliedArguments":[{"HLAargumentType":43,"HLAargumentName":"Set of region designators","HLAargumentValue":[")" +
      invalidRegionValue +
      R"("]}],"HLAsuccessIndicator":false,"HLAexception":"InvalidRegion: Commit Region Modifications requires valid RegionHandle values."})"} +
      std::string{
          R"({"HLAserialNumber":1,"HLAreturnedArgument":[null],"HLAservice":"DeleteRegion","HLAsuppliedArguments":[{"HLAargumentType":42,"HLAargumentName":"Region designator","HLAargumentValue":")" +
      invalidRegionValue +
      R"("}],"HLAsuccessIndicator":false,"HLAexception":"InvalidRegion: Delete Region requires a valid RegionHandle."})"} +
      std::string{
          R"({"HLAserialNumber":2,"HLAreturnedArgument":[null],"HLAservice":"SetRangeBounds","HLAsuppliedArguments":[{"HLAargumentType":42,"HLAargumentName":"Region handle","HLAargumentValue":")" +
      existingRegionValue +
      R"("},{"HLAargumentType":10,"HLAargumentName":"Dimension handle","HLAargumentValue":")" +
      invalidDimensionValue +
      R"("},{"HLAargumentType":35,"HLAargumentName":"Range lower bound","HLAargumentValue":0},{"HLAargumentType":35,"HLAargumentName":"Range upper bound","HLAargumentValue":10}],"HLAsuccessIndicator":false,"HLAexception":"RegionDoesNotContainSpecifiedDimension: Set Range Bounds requires a dimension contained by the region."})"} +
      std::string{
          R"({"HLAserialNumber":3,"HLAreturnedArgument":[null],"HLAservice":"SetRangeBounds","HLAsuppliedArguments":[{"HLAargumentType":42,"HLAargumentName":"Region handle","HLAargumentValue":")" +
      existingRegionValue +
      R"("},{"HLAargumentType":10,"HLAargumentName":"Dimension handle","HLAargumentValue":")" +
      barQuantityValue +
      R"("},{"HLAargumentType":35,"HLAargumentName":"Range lower bound","HLAargumentValue":8},{"HLAargumentType":35,"HLAargumentName":"Range upper bound","HLAargumentValue":8}],"HLAsuccessIndicator":false,"HLAexception":"InvalidRangeBound: Set Range Bounds requires 0 <= lowerBound < upperBound <= dimension upper bound."})"} +
      std::string{
          R"({"HLAserialNumber":4,"HLAreturnedArgument":[null],"HLAservice":"SetRangeBounds","HLAsuppliedArguments":[{"HLAargumentType":42,"HLAargumentName":"Region handle","HLAargumentValue":")" +
      existingRegionValue +
      R"("},{"HLAargumentType":10,"HLAargumentName":"Dimension handle","HLAargumentValue":")" +
      barQuantityValue +
      R"("},{"HLAargumentType":35,"HLAargumentName":"Range lower bound","HLAargumentValue":2},{"HLAargumentType":35,"HLAargumentName":"Range upper bound","HLAargumentValue":8}],"HLAsuccessIndicator":true,"HLAexception":null})"} +
      std::string{
          R"({"HLAserialNumber":5,"HLAreturnedArgument":[null],"HLAservice":"CommitRegionModifications","HLAsuppliedArguments":[{"HLAargumentType":43,"HLAargumentName":"Set of region designators","HLAargumentValue":[")" +
      existingRegionValue +
      R"("]}],"HLAsuccessIndicator":true,"HLAexception":null})"} +
      std::string{
          R"({"HLAserialNumber":6,"HLAreturnedArgument":[null],"HLAservice":"DeleteRegion","HLAsuppliedArguments":[{"HLAargumentType":42,"HLAargumentName":"Region designator","HLAargumentValue":")" +
      deletableRegionValue +
      R"("}],"HLAsuccessIndicator":true,"HLAexception":null})"};
  REQUIRE(readTextFile(reportFile) == initialText + expectedRecords);

  REQUIRE_NOTHROW(owner->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(owner->setSendServiceReportsToFileSwitch(false));
  REQUIRE_NOTHROW(owner->deleteRegion(existingRegion));
  REQUIRE_NOTHROW(owner->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(owner->disconnect());
}
} // namespace
