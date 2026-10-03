#include "ieee1516_2025_federation_management_test_support.hpp"
#include "ieee1516_2025_federation_management_fixture_support.hpp"

namespace {
TEST_CASE(
    "Embedded service reporting records interaction-class and parameter lookup return arguments",
    "[integration][development-profile][federation-management][support-services]"
    "[mom][service-report-file][service-reporting][federation-interaction-parameter-lookup-service-reports]"
    "[rti.service.get-interaction-class-handle][rti.service.get-interaction-class-name]"
    "[rti.service.get-parameter-handle][rti.service.get-parameter-name]") {
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
      L"federation-interaction-parameter-report-owner", L"owner", federationName));

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

  REQUIRE(owner->getInteractionClassHandle(interactionClassName) == interactionClass);
  REQUIRE(owner->getInteractionClassName(interactionClass) == interactionClassName);
  REQUIRE(owner->getParameterHandle(interactionClass, parameterName) == parameter);
  REQUIRE(owner->getParameterName(interactionClass, parameter) == parameterName);

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
  auto const parameterValue = asAscii(parameter.toString());
  auto const expectedRecords =
      std::string{
          R"({"HLAserialNumber":0,"HLAreturnedArgument":[{"HLAargumentType":27,"HLAargumentName":"Interaction class handle","HLAargumentValue":")" +
      interactionClassValue +
      R"("}],"HLAservice":"GetInteractionClassHandle","HLAsuppliedArguments":[{"HLAargumentType":53,"HLAargumentName":"Interaction class name","HLAargumentValue":"HLAinteractionRoot.CustomerTransactions.FoodServed.MainCourseServed"}],"HLAsuccessIndicator":true,"HLAexception":null})" +
      R"({"HLAserialNumber":1,"HLAreturnedArgument":[{"HLAargumentType":53,"HLAargumentName":"Interaction class name","HLAargumentValue":"HLAinteractionRoot.CustomerTransactions.FoodServed.MainCourseServed"}],"HLAservice":"GetInteractionClassName","HLAsuppliedArguments":[{"HLAargumentType":27,"HLAargumentName":"Interaction class handle","HLAargumentValue":")" +
      interactionClassValue +
      R"("}],"HLAsuccessIndicator":true,"HLAexception":null})" +
      R"({"HLAserialNumber":2,"HLAreturnedArgument":[{"HLAargumentType":39,"HLAargumentName":"Parameter handle","HLAargumentValue":")" +
      parameterValue +
      R"("}],"HLAservice":"GetParameterHandle","HLAsuppliedArguments":[{"HLAargumentType":27,"HLAargumentName":"Interaction class handle","HLAargumentValue":")" +
      interactionClassValue +
      R"("},{"HLAargumentType":53,"HLAargumentName":"Parameter name","HLAargumentValue":"TemperatureOk"}],"HLAsuccessIndicator":true,"HLAexception":null})" +
      R"({"HLAserialNumber":3,"HLAreturnedArgument":[{"HLAargumentType":53,"HLAargumentName":"Parameter name","HLAargumentValue":"TemperatureOk"}],"HLAservice":"GetParameterName","HLAsuppliedArguments":[{"HLAargumentType":27,"HLAargumentName":"Interaction class handle","HLAargumentValue":")" +
      interactionClassValue +
      R"("},{"HLAargumentType":39,"HLAargumentName":"Parameter handle","HLAargumentValue":")" +
      parameterValue +
      R"("}],"HLAsuccessIndicator":true,"HLAexception":null})"};
  REQUIRE(readTextFile(reportFile) == initialText + expectedRecords);

  REQUIRE_NOTHROW(owner->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(owner->disconnect());
}
} // namespace
