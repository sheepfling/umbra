#include "ieee1516_2025_federation_management_fixture_support.hpp"

namespace {

TEST_CASE(
    "Embedded service reporting records Set Range Bounds arguments",
    "[integration][development-profile][federation-management][ddm]"
    "[mom][service-report-file][service-reporting]"
    "[set-range-bounds-service-report]"
    "[rti.service.set-range-bounds]") {
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
      L"set-range-report-owner", L"owner", federationName));

  auto const files = serviceReportFiles(directory.path());
  REQUIRE(files.size() == 1U);
  auto const reportFile = files.front();
  auto const initialText = readTextFile(reportFile);

  REQUIRE_NOTHROW(owner->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(owner->setSendServiceReportsToFileSwitch(false));
  auto const barQuantity = owner->getDimensionHandle(fixture_hla::fixture::bar_quantity);
  REQUIRE(barQuantity.isValid());
  auto const region = owner->createRegion(DimensionHandleSet{barQuantity});
  REQUIRE_NOTHROW(owner->setRangeBounds(region, barQuantity, RangeBounds(0UL, 10UL)));
  REQUIRE(readTextFile(reportFile) == initialText);

  REQUIRE_NOTHROW(owner->setServiceReportingSwitch(true));
  REQUIRE_NOTHROW(owner->setSendServiceReportsToFileSwitch(true));
  REQUIRE_NOTHROW(owner->setRangeBounds(region, barQuantity, RangeBounds(2UL, 8UL)));

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
  auto const regionValue = asAscii(region.toString());
  auto const dimensionValue = asAscii(barQuantity.toString());
  auto const expectedRecord = std::string{
      R"({"HLAserialNumber":0,"HLAreturnedArgument":[null],"HLAservice":"SetRangeBounds","HLAsuppliedArguments":[{"HLAargumentType":42,"HLAargumentName":"Region handle","HLAargumentValue":")" +
      regionValue +
      R"("},{"HLAargumentType":10,"HLAargumentName":"Dimension handle","HLAargumentValue":")" +
      dimensionValue +
      R"("},{"HLAargumentType":35,"HLAargumentName":"Range lower bound","HLAargumentValue":2},{"HLAargumentType":35,"HLAargumentName":"Range upper bound","HLAargumentValue":8}],"HLAsuccessIndicator":true,"HLAexception":null})"};
  REQUIRE(readTextFile(reportFile) == initialText + expectedRecord);

  REQUIRE_THROWS_AS(
      owner->setRangeBounds(region, barQuantity, RangeBounds(8UL, 8UL)),
      rti1516_2025::InvalidRangeBound);
  auto const invalidRangeRecord = std::string{
      R"({"HLAserialNumber":1,"HLAreturnedArgument":[null],"HLAservice":"SetRangeBounds","HLAsuppliedArguments":[{"HLAargumentType":42,"HLAargumentName":"Region handle","HLAargumentValue":")" +
      regionValue +
      R"("},{"HLAargumentType":10,"HLAargumentName":"Dimension handle","HLAargumentValue":")" +
      dimensionValue +
      R"("},{"HLAargumentType":35,"HLAargumentName":"Range lower bound","HLAargumentValue":8},{"HLAargumentType":35,"HLAargumentName":"Range upper bound","HLAargumentValue":8}],"HLAsuccessIndicator":false,"HLAexception":"InvalidRangeBound: Set Range Bounds requires 0 <= lowerBound < upperBound <= dimension upper bound."})"};
  REQUIRE(readTextFile(reportFile) == initialText + expectedRecord + invalidRangeRecord);

  REQUIRE_NOTHROW(owner->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(owner->setSendServiceReportsToFileSwitch(false));
  REQUIRE_NOTHROW(owner->deleteRegion(region));
  REQUIRE_NOTHROW(owner->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(owner->disconnect());
}

}  // namespace
