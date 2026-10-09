#include "ieee1516_2025_federation_management_fixture_support.hpp"

namespace {

TEST_CASE(
    "Embedded service reporting records failed dimension and region lookups",
    "[integration][development-profile][federation-management][support-services][ddm]"
    "[mom][service-report-file][service-reporting][service-failure]"
    "[rti.service.dimension-lookup-failure-matrix]") {
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
      L"failed-dimension-lookup-report-owner", L"owner", federationName));

  auto const files = serviceReportFiles(directory.path());
  REQUIRE(files.size() == 1U);
  auto const reportFile = files.front();
  auto const initialText = readTextFile(reportFile);

  REQUIRE_NOTHROW(owner->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(owner->setSendServiceReportsToFileSwitch(false));
  REQUIRE_THROWS_AS(
      owner->getAvailableDimensionsForObjectClass(ObjectClassHandle{}),
      rti1516_2025::InvalidObjectClassHandle);
  REQUIRE_THROWS_AS(
      owner->getAvailableDimensionsForInteractionClass(InteractionClassHandle{}),
      rti1516_2025::InvalidInteractionClassHandle);
  REQUIRE_THROWS_AS(
      owner->getDimensionHandle(fixture_hla::fixture::missing_dimension),
      rti1516_2025::NameNotFound);
  REQUIRE_THROWS_AS(
      owner->getDimensionName(DimensionHandle{}),
      rti1516_2025::InvalidDimensionHandle);
  REQUIRE_THROWS_AS(
      owner->getDimensionUpperBound(DimensionHandle{}),
      rti1516_2025::InvalidDimensionHandle);
  REQUIRE_THROWS_AS(
      owner->getDimensionHandleSet(RegionHandle{}),
      rti1516_2025::InvalidRegion);
  REQUIRE(readTextFile(reportFile) == initialText);

  REQUIRE_NOTHROW(owner->setServiceReportingSwitch(true));
  REQUIRE_NOTHROW(owner->setSendServiceReportsToFileSwitch(true));
  REQUIRE_THROWS_AS(
      owner->getAvailableDimensionsForObjectClass(ObjectClassHandle{}),
      rti1516_2025::InvalidObjectClassHandle);
  REQUIRE_THROWS_AS(
      owner->getAvailableDimensionsForInteractionClass(InteractionClassHandle{}),
      rti1516_2025::InvalidInteractionClassHandle);
  REQUIRE_THROWS_AS(
      owner->getDimensionHandle(fixture_hla::fixture::missing_dimension),
      rti1516_2025::NameNotFound);
  REQUIRE_THROWS_AS(
      owner->getDimensionName(DimensionHandle{}),
      rti1516_2025::InvalidDimensionHandle);
  REQUIRE_THROWS_AS(
      owner->getDimensionUpperBound(DimensionHandle{}),
      rti1516_2025::InvalidDimensionHandle);
  REQUIRE_THROWS_AS(
      owner->getDimensionHandleSet(RegionHandle{}),
      rti1516_2025::InvalidRegion);
  auto const barQuantity = owner->getDimensionHandle(fixture_hla::fixture::bar_quantity);
  REQUIRE(barQuantity.isValid());

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
  auto const invalidObjectClassValue = asAscii(ObjectClassHandle{}.toString());
  auto const invalidInteractionClassValue = asAscii(InteractionClassHandle{}.toString());
  auto const invalidDimensionValue = asAscii(DimensionHandle{}.toString());
  auto const invalidRegionValue = asAscii(RegionHandle{}.toString());
  auto const barQuantityValue = asAscii(barQuantity.toString());
  auto expectedRecords = std::string{
      R"({"HLAserialNumber":0,"HLAreturnedArgument":[null],"HLAservice":"GetAvailableDimensionsForObjectClass","HLAsuppliedArguments":[{"HLAargumentType":36,"HLAargumentName":"Object class handle","HLAargumentValue":")" +
      invalidObjectClassValue +
      R"("}],"HLAsuccessIndicator":false,"HLAexception":"InvalidObjectClassHandle: Get Available Dimensions for Object Class requires a valid ObjectClassHandle."})"};
  expectedRecords +=
      std::string{
          R"({"HLAserialNumber":1,"HLAreturnedArgument":[null],"HLAservice":"GetAvailableDimensionsForInteractionClass","HLAsuppliedArguments":[{"HLAargumentType":27,"HLAargumentName":"Interaction class handle","HLAargumentValue":")" +
      invalidInteractionClassValue +
      R"("}],"HLAsuccessIndicator":false,"HLAexception":"InvalidInteractionClassHandle: Get Available Dimensions for Interaction Class requires a valid InteractionClassHandle."})"};
  expectedRecords +=
      R"({"HLAserialNumber":2,"HLAreturnedArgument":[null],"HLAservice":"GetDimensionHandle","HLAsuppliedArguments":[{"HLAargumentType":53,"HLAargumentName":"Dimension name","HLAargumentValue":"MissingDimension"}],"HLAsuccessIndicator":false,"HLAexception":"NameNotFound: The supplied dimension name is not defined in this federation execution."})";
  expectedRecords +=
      std::string{
          R"({"HLAserialNumber":3,"HLAreturnedArgument":[null],"HLAservice":"GetDimensionName","HLAsuppliedArguments":[{"HLAargumentType":10,"HLAargumentName":"Dimension handle","HLAargumentValue":")" +
      invalidDimensionValue +
      R"("}],"HLAsuccessIndicator":false,"HLAexception":"InvalidDimensionHandle: Get Dimension Name requires a valid DimensionHandle."})"};
  expectedRecords +=
      std::string{
          R"({"HLAserialNumber":4,"HLAreturnedArgument":[null],"HLAservice":"GetDimensionUpperBound","HLAsuppliedArguments":[{"HLAargumentType":10,"HLAargumentName":"Dimension handle","HLAargumentValue":")" +
      invalidDimensionValue +
      R"("}],"HLAsuccessIndicator":false,"HLAexception":"InvalidDimensionHandle: Get Dimension Upper Bound requires a valid DimensionHandle."})"};
  expectedRecords +=
      std::string{
          R"({"HLAserialNumber":5,"HLAreturnedArgument":[null],"HLAservice":"GetDimensionHandleSet","HLAsuppliedArguments":[{"HLAargumentType":42,"HLAargumentName":"Region handle","HLAargumentValue":")" +
      invalidRegionValue +
      R"("}],"HLAsuccessIndicator":false,"HLAexception":"InvalidRegion: Get Dimension Handle Set requires a valid RegionHandle."})"};
  expectedRecords +=
      std::string{
          R"({"HLAserialNumber":6,"HLAreturnedArgument":[{"HLAargumentType":10,"HLAargumentName":"Dimension handle","HLAargumentValue":")" +
      barQuantityValue +
      R"("}],"HLAservice":"GetDimensionHandle","HLAsuppliedArguments":[{"HLAargumentType":53,"HLAargumentName":"Dimension name","HLAargumentValue":"BarQuantity"}],"HLAsuccessIndicator":true,"HLAexception":null})"};
  REQUIRE(readTextFile(reportFile) == initialText + expectedRecords);

  REQUIRE_NOTHROW(owner->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(owner->setSendServiceReportsToFileSwitch(false));
  REQUIRE_NOTHROW(owner->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(owner->disconnect());
}

} // namespace
