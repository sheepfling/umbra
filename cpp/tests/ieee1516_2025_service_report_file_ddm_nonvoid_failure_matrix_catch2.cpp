#include "ieee1516_2025_federation_management_fixture_support.hpp"

namespace {

TEST_CASE(
    "Embedded service reporting records failed Create Region and Get Range Bounds invocations",
    "[integration][development-profile][federation-management][ddm]"
    "[mom][service-report-file][service-reporting][service-failure]"
    "[ddm-nonvoid-failure][rti.service.ddm-nonvoid-failure-matrix]") {
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
      L"failed-ddm-nonvoid-report-owner", L"owner", federationName));

  auto const files = serviceReportFiles(directory.path());
  REQUIRE(files.size() == 1U);
  auto const reportFile = files.front();
  auto const initialText = readTextFile(reportFile);

  REQUIRE_NOTHROW(owner->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(owner->setSendServiceReportsToFileSwitch(false));
  auto const barQuantity = owner->getDimensionHandle(fixture_hla::fixture::bar_quantity);
  REQUIRE(barQuantity.isValid());
  auto const existingRegion = owner->createRegion(DimensionHandleSet{barQuantity});
  REQUIRE_NOTHROW(
      owner->setRangeBounds(existingRegion, barQuantity, RangeBounds(2UL, 8UL)));
  REQUIRE_THROWS_AS(
      owner->createRegion(DimensionHandleSet{DimensionHandle{}}),
      rti1516_2025::InvalidDimensionHandle);
  REQUIRE_THROWS_AS(
      owner->getRangeBounds(existingRegion, DimensionHandle{}),
      rti1516_2025::RegionDoesNotContainSpecifiedDimension);
  REQUIRE(readTextFile(reportFile) == initialText);

  REQUIRE_NOTHROW(owner->setServiceReportingSwitch(true));
  REQUIRE_NOTHROW(owner->setSendServiceReportsToFileSwitch(true));
  REQUIRE_THROWS_AS(
      owner->createRegion(DimensionHandleSet{DimensionHandle{}}),
      rti1516_2025::InvalidDimensionHandle);
  REQUIRE_THROWS_AS(
      owner->getRangeBounds(existingRegion, DimensionHandle{}),
      rti1516_2025::RegionDoesNotContainSpecifiedDimension);
  auto const createdRegion = owner->createRegion(DimensionHandleSet{barQuantity});
  auto const rangeBounds = owner->getRangeBounds(existingRegion, barQuantity);
  REQUIRE(rangeBounds.getLowerBound() == 2UL);
  REQUIRE(rangeBounds.getUpperBound() == 8UL);

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
  auto const invalidDimensionValue = asAscii(DimensionHandle{}.toString());
  auto const barQuantityValue = asAscii(barQuantity.toString());
  auto const existingRegionValue = asAscii(existingRegion.toString());
  auto const createdRegionValue = asAscii(createdRegion.toString());
  auto const expectedRecords =
      std::string{
          R"({"HLAserialNumber":0,"HLAreturnedArgument":[null],"HLAservice":"CreateRegion","HLAsuppliedArguments":[{"HLAargumentType":11,"HLAargumentName":"Set of dimension designators","HLAargumentValue":[")" +
      invalidDimensionValue +
      R"("]}],"HLAsuccessIndicator":false,"HLAexception":"InvalidDimensionHandle: Create Region requires valid DimensionHandle values."})"} +
      std::string{
          R"({"HLAserialNumber":1,"HLAreturnedArgument":[null],"HLAservice":"GetRangeBounds","HLAsuppliedArguments":[{"HLAargumentType":42,"HLAargumentName":"Region handle","HLAargumentValue":")" +
      existingRegionValue +
      R"("},{"HLAargumentType":10,"HLAargumentName":"Dimension handle","HLAargumentValue":")" +
      invalidDimensionValue +
      R"("}],"HLAsuccessIndicator":false,"HLAexception":"RegionDoesNotContainSpecifiedDimension: Get Range Bounds requires a dimension contained by the region."})"} +
      std::string{
          R"({"HLAserialNumber":2,"HLAreturnedArgument":[{"HLAargumentType":42,"HLAargumentName":"Region designator","HLAargumentValue":")" +
      createdRegionValue +
      R"("}],"HLAservice":"CreateRegion","HLAsuppliedArguments":[{"HLAargumentType":11,"HLAargumentName":"Set of dimension designators","HLAargumentValue":[")" +
      barQuantityValue +
      R"("]}],"HLAsuccessIndicator":true,"HLAexception":null})"} +
      std::string{
          R"({"HLAserialNumber":3,"HLAreturnedArgument":[{"HLAargumentType":41,"HLAargumentName":"Range bounds","HLAargumentValue":{"lower":2,"upper":8}}],"HLAservice":"GetRangeBounds","HLAsuppliedArguments":[{"HLAargumentType":42,"HLAargumentName":"Region handle","HLAargumentValue":")" +
      existingRegionValue +
      R"("},{"HLAargumentType":10,"HLAargumentName":"Dimension handle","HLAargumentValue":")" +
      barQuantityValue +
      R"("}],"HLAsuccessIndicator":true,"HLAexception":null})"};
  REQUIRE(readTextFile(reportFile) == initialText + expectedRecords);

  REQUIRE_NOTHROW(owner->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(owner->setSendServiceReportsToFileSwitch(false));
  REQUIRE_NOTHROW(owner->deleteRegion(createdRegion));
  REQUIRE_NOTHROW(owner->deleteRegion(existingRegion));
  REQUIRE_NOTHROW(owner->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(owner->disconnect());
}

} // namespace
