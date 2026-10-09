#include "ieee1516_2025_federation_management_fixture_support.hpp"

namespace {

TEST_CASE(
    "Embedded service reporting records support dimension lookup return arguments",
    "[integration][development-profile][federation-management][support-services][ddm]"
    "[mom][service-report-file][service-reporting][dimension-lookup-service-reports]"
    "[rti.service.get-available-dimensions-for-object-class]"
    "[rti.service.get-available-dimensions-for-interaction-class]"
    "[rti.service.get-dimension-handle][rti.service.get-dimension-name]"
    "[rti.service.get-dimension-upper-bound][rti.service.get-dimension-handle-set]") {
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
      L"dimension-lookup-report-owner", L"owner", federationName));

  auto const files = serviceReportFiles(directory.path());
  REQUIRE(files.size() == 1U);
  auto const reportFile = files.front();
  auto const initialText = readTextFile(reportFile);

  REQUIRE_NOTHROW(owner->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(owner->setSendServiceReportsToFileSwitch(false));
  auto const barQuantity = owner->getDimensionHandle(fixture_hla::fixture::bar_quantity);
  auto const serverId = owner->getDimensionHandle(fixture_hla::fixture::server_id);
  auto const drink = owner->getObjectClassHandle(fixture_hla::fom::food_drink);
  auto const mainCourseServed = owner->getInteractionClassHandle(
      fixture_hla::fom::main_course_served);
  auto const region = owner->createRegion(DimensionHandleSet{barQuantity});
  REQUIRE_NOTHROW(owner->setRangeBounds(region, barQuantity, RangeBounds(0UL, 10UL)));
  REQUIRE(readTextFile(reportFile) == initialText);

  REQUIRE_NOTHROW(owner->setServiceReportingSwitch(true));
  REQUIRE_NOTHROW(owner->setSendServiceReportsToFileSwitch(true));
  REQUIRE(owner->getAvailableDimensionsForObjectClass(drink) ==
          DimensionHandleSet{barQuantity});
  REQUIRE(owner->getAvailableDimensionsForInteractionClass(mainCourseServed) ==
          DimensionHandleSet{serverId});
  REQUIRE(owner->getDimensionHandle(fixture_hla::fixture::bar_quantity) == barQuantity);
  REQUIRE(owner->getDimensionName(barQuantity) == fixture_hla::fixture::bar_quantity);
  REQUIRE(owner->getDimensionUpperBound(barQuantity) == 25UL);
  REQUIRE(owner->getDimensionHandleSet(region) == DimensionHandleSet{barQuantity});

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
  auto const serverIdValue = asAscii(serverId.toString());
  auto const drinkValue = asAscii(drink.toString());
  auto const mainCourseServedValue = asAscii(mainCourseServed.toString());
  auto const regionValue = asAscii(region.toString());
  auto const expectedRecords =
      std::string{
          R"({"HLAserialNumber":0,"HLAreturnedArgument":[{"HLAargumentType":11,"HLAargumentName":"A set of dimension handles","HLAargumentValue":[")" +
      barQuantityValue +
      R"("]}],"HLAservice":"GetAvailableDimensionsForObjectClass","HLAsuppliedArguments":[{"HLAargumentType":36,"HLAargumentName":"Object class handle","HLAargumentValue":")" +
      drinkValue +
      R"("}],"HLAsuccessIndicator":true,"HLAexception":null})" +
      R"({"HLAserialNumber":1,"HLAreturnedArgument":[{"HLAargumentType":11,"HLAargumentName":"A set of dimension handles","HLAargumentValue":[")" +
      serverIdValue +
      R"("]}],"HLAservice":"GetAvailableDimensionsForInteractionClass","HLAsuppliedArguments":[{"HLAargumentType":27,"HLAargumentName":"Interaction class handle","HLAargumentValue":")" +
      mainCourseServedValue +
      R"("}],"HLAsuccessIndicator":true,"HLAexception":null})" +
      R"({"HLAserialNumber":2,"HLAreturnedArgument":[{"HLAargumentType":10,"HLAargumentName":"Dimension handle","HLAargumentValue":")" +
      barQuantityValue +
      R"("}],"HLAservice":"GetDimensionHandle","HLAsuppliedArguments":[{"HLAargumentType":53,"HLAargumentName":"Dimension name","HLAargumentValue":"BarQuantity"}],"HLAsuccessIndicator":true,"HLAexception":null})" +
      R"({"HLAserialNumber":3,"HLAreturnedArgument":[{"HLAargumentType":53,"HLAargumentName":"Dimension name","HLAargumentValue":"BarQuantity"}],"HLAservice":"GetDimensionName","HLAsuppliedArguments":[{"HLAargumentType":10,"HLAargumentName":"Dimension handle","HLAargumentValue":")" +
      barQuantityValue +
      R"("}],"HLAsuccessIndicator":true,"HLAexception":null})" +
      R"({"HLAserialNumber":4,"HLAreturnedArgument":[{"HLAargumentType":35,"HLAargumentName":"Dimension upper bound","HLAargumentValue":25}],"HLAservice":"GetDimensionUpperBound","HLAsuppliedArguments":[{"HLAargumentType":10,"HLAargumentName":"Dimension handle","HLAargumentValue":")" +
      barQuantityValue +
      R"("}],"HLAsuccessIndicator":true,"HLAexception":null})" +
      R"({"HLAserialNumber":5,"HLAreturnedArgument":[{"HLAargumentType":11,"HLAargumentName":"A set of dimensions","HLAargumentValue":[")" +
      barQuantityValue +
      R"("]}],"HLAservice":"GetDimensionHandleSet","HLAsuppliedArguments":[{"HLAargumentType":42,"HLAargumentName":"Region handle","HLAargumentValue":")" +
      regionValue +
      R"("}],"HLAsuccessIndicator":true,"HLAexception":null})"};
  REQUIRE(readTextFile(reportFile) == initialText + expectedRecords);

  REQUIRE_NOTHROW(owner->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(owner->setSendServiceReportsToFileSwitch(false));
  REQUIRE_NOTHROW(owner->deleteRegion(region));
  REQUIRE_NOTHROW(owner->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(owner->disconnect());
}

} // namespace
