#include "ieee1516_2025_federation_management_test_support.hpp"
#include "ieee1516_2025_federation_management_fixture_support.hpp"

namespace {
TEST_CASE(
    "Embedded service reporting records failed interaction-class and parameter lookups",
    "[integration][development-profile][federation-management][support-services]"
    "[mom][service-report-file][service-reporting][service-failure]"
    "[rti.service.interaction-lookup-failure-matrix]") {
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
      L"failed-interaction-lookup-report-owner", L"owner", federationName));

  auto const files = serviceReportFiles(directory.path());
  REQUIRE(files.size() == 1U);
  auto const reportFile = files.front();
  auto const initialText = readTextFile(reportFile);
  auto const interactionClassName =
      fixture_hla::fom::main_course_served;
  auto const parameterName = fixture_hla::fixture::temperature_ok;

  REQUIRE_NOTHROW(owner->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(owner->setSendServiceReportsToFileSwitch(false));
  auto const interactionClass = owner->getInteractionClassHandle(interactionClassName);
  auto const parameter = owner->getParameterHandle(interactionClass, parameterName);
  REQUIRE(interactionClass.isValid());
  REQUIRE(parameter.isValid());
  REQUIRE_NOTHROW(owner->setServiceReportingSwitch(true));
  REQUIRE_NOTHROW(owner->setSendServiceReportsToFileSwitch(true));

  REQUIRE_THROWS_AS(
      owner->getInteractionClassHandle(fixture_hla::fom::missing_interaction),
      rti1516_2025::NameNotFound);
  REQUIRE_THROWS_AS(
      owner->getInteractionClassName(InteractionClassHandle{}),
      rti1516_2025::InvalidInteractionClassHandle);
  REQUIRE_THROWS_AS(
      owner->getParameterHandle(interactionClass, fixture_hla::fixture::missing_parameter),
      rti1516_2025::NameNotFound);
  REQUIRE_THROWS_AS(
      owner->getParameterName(interactionClass, ParameterHandle{}),
      rti1516_2025::InvalidParameterHandle);
  REQUIRE(owner->getInteractionClassHandle(interactionClassName) == interactionClass);

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
  auto const interactionClassValue = asAscii(interactionClass.toString());
  auto const invalidInteractionClassValue = asAscii(InteractionClassHandle{}.toString());
  auto const parameterValue = asAscii(parameter.toString());
  auto const expectedRecords =
      std::string{
          R"({"HLAserialNumber":0,"HLAreturnedArgument":[null],"HLAservice":"GetInteractionClassHandle","HLAsuppliedArguments":[{"HLAargumentType":53,"HLAargumentName":"Interaction class name","HLAargumentValue":"HLAinteractionRoot.MissingInteraction"}],"HLAsuccessIndicator":false,"HLAexception":"NameNotFound: The supplied interaction class name is not defined in this federation execution."})"} +
      R"({"HLAserialNumber":1,"HLAreturnedArgument":[null],"HLAservice":"GetInteractionClassName","HLAsuppliedArguments":[{"HLAargumentType":27,"HLAargumentName":"Interaction class handle","HLAargumentValue":")" +
      invalidInteractionClassValue +
      R"("}],"HLAsuccessIndicator":false,"HLAexception":"InvalidInteractionClassHandle: Get Interaction Class Name requires a valid InteractionClassHandle."})" +
      R"({"HLAserialNumber":2,"HLAreturnedArgument":[null],"HLAservice":"GetParameterHandle","HLAsuppliedArguments":[{"HLAargumentType":27,"HLAargumentName":"Interaction class handle","HLAargumentValue":")" +
      interactionClassValue +
      R"("},{"HLAargumentType":53,"HLAargumentName":"Parameter name","HLAargumentValue":"MissingParameter"}],"HLAsuccessIndicator":false,"HLAexception":"NameNotFound: The supplied parameter name is not defined for this interaction class."})" +
      R"({"HLAserialNumber":3,"HLAreturnedArgument":[null],"HLAservice":"GetParameterName","HLAsuppliedArguments":[{"HLAargumentType":27,"HLAargumentName":"Interaction class handle","HLAargumentValue":")" +
      interactionClassValue +
      R"("},{"HLAargumentType":39,"HLAargumentName":"Parameter handle","HLAargumentValue":")" +
      asAscii(ParameterHandle{}.toString()) +
      R"("}],"HLAsuccessIndicator":false,"HLAexception":"InvalidParameterHandle: Get Parameter Name requires a valid ParameterHandle."})" +
      R"({"HLAserialNumber":4,"HLAreturnedArgument":[{"HLAargumentType":27,"HLAargumentName":"Interaction class handle","HLAargumentValue":")" +
      interactionClassValue +
      R"("}],"HLAservice":"GetInteractionClassHandle","HLAsuppliedArguments":[{"HLAargumentType":53,"HLAargumentName":"Interaction class name","HLAargumentValue":"HLAinteractionRoot.CustomerTransactions.FoodServed.MainCourseServed"}],"HLAsuccessIndicator":true,"HLAexception":null})";
  static_cast<void>(parameterValue);
  REQUIRE(readTextFile(reportFile) == initialText + expectedRecords);

  REQUIRE_NOTHROW(owner->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(owner->setSendServiceReportsToFileSwitch(false));
  REQUIRE_NOTHROW(owner->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(owner->disconnect());
}
} // namespace
