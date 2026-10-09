#include "ieee1516_2025_federation_management_fixture_support.hpp"

namespace {

TEST_CASE(
    "Embedded service reporting records Create Region and Get Range Bounds return arguments",
    "[integration][development-profile][federation-management][ddm]"
    "[mom][service-report-file][service-reporting]"
    "[create-region-service-report][get-range-bounds-service-report]"
    "[rti.service.create-region][rti.service.get-range-bounds]") {
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
      L"create-get-range-report-owner", L"owner", federationName));

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
  REQUIRE(readTextFile(reportFile) == initialText);

  REQUIRE_NOTHROW(owner->setServiceReportingSwitch(true));
  REQUIRE_NOTHROW(owner->setSendServiceReportsToFileSwitch(true));
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
  auto const barQuantityValue = asAscii(barQuantity.toString());
  auto const existingRegionValue = asAscii(existingRegion.toString());
  auto const createdRegionValue = asAscii(createdRegion.toString());
  auto const createRegionRecord = std::string{
      R"({"HLAserialNumber":0,"HLAreturnedArgument":[{"HLAargumentType":42,"HLAargumentName":"Region designator","HLAargumentValue":")" +
      createdRegionValue +
      R"("}],"HLAservice":"CreateRegion","HLAsuppliedArguments":[{"HLAargumentType":11,"HLAargumentName":"Set of dimension designators","HLAargumentValue":[")" +
      barQuantityValue +
      R"("]}],"HLAsuccessIndicator":true,"HLAexception":null})"};
  auto const getRangeBoundsRecord = std::string{
      R"({"HLAserialNumber":1,"HLAreturnedArgument":[{"HLAargumentType":41,"HLAargumentName":"Range bounds","HLAargumentValue":{"lower":2,"upper":8}}],"HLAservice":"GetRangeBounds","HLAsuppliedArguments":[{"HLAargumentType":42,"HLAargumentName":"Region handle","HLAargumentValue":")" +
      existingRegionValue +
      R"("},{"HLAargumentType":10,"HLAargumentName":"Dimension handle","HLAargumentValue":")" +
      barQuantityValue +
      R"("}],"HLAsuccessIndicator":true,"HLAexception":null})"};
  REQUIRE(readTextFile(reportFile) == initialText + createRegionRecord + getRangeBoundsRecord);

  REQUIRE_NOTHROW(owner->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(owner->setSendServiceReportsToFileSwitch(false));
  REQUIRE_NOTHROW(owner->deleteRegion(createdRegion));
  REQUIRE_NOTHROW(owner->deleteRegion(existingRegion));
  REQUIRE_NOTHROW(owner->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(owner->disconnect());
}

} // namespace
