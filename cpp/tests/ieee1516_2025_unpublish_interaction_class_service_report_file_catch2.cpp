#include "ieee1516_2025_federation_management_test_support.hpp"
#include "ieee1516_2025_federation_management_fixture_support.hpp"

namespace {
TEST_CASE(
    "Embedded service reporting preserves Unpublish Interaction Class arguments",
    "[integration][development-profile][federation-management][declaration-management]"
    "[mom][service-report-file][service-reporting]"
    "[unpublish-interaction-class]"
    "[rti.service.publish-interaction-class]"
    "[rti.service.unpublish-interaction-class]") {
  ReportingFederateAmbassador reports;
  auto rti = makeRti();
  auto const federationName = nextFederationName();
  auto const restaurantFom = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  auto const switchFom =
      (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data" /
       "switch-support-enabled-fom.xml")
          .wstring();
  std::vector<std::wstring> const fomModules{restaurantFom, switchFom};
  auto directory = temporaryServiceReportDirectory();
  auto configuration = configurationForServiceReportDirectory(directory.path());
  configuration.withRtiAddress(L"in-process");

  REQUIRE_NOTHROW(rti->connect(reports, HLA_EVOKED, configuration));
  REQUIRE_NOTHROW(
      rti->createFederationExecution(federationName, fomModules, standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(
      rti->joinFederationExecution(L"unpublish-interaction-report-subject", L"publisher", federationName));

  auto const files = serviceReportFiles(directory.path());
  REQUIRE(files.size() == 1U);
  auto const reportFile = files.front();
  auto const initialText = readTextFile(reportFile);

  // The failed-service matrix is covered below; keep this success-argument
  // regression focused on the accepted calls.
  REQUIRE_NOTHROW(rti->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(rti->setSendServiceReportsToFileSwitch(false));
  InteractionClassHandle const unknownInteraction;
  REQUIRE_THROWS_AS(
      rti->unpublishInteractionClass(unknownInteraction),
      rti1516_2025::InteractionClassNotDefined);
  REQUIRE(readTextFile(reportFile) == initialText);

  auto const interactionClass = rti->getInteractionClassHandle(
      fixture_hla::fom::server_take_order);
  REQUIRE(interactionClass.isValid());
  REQUIRE_NOTHROW(rti->setServiceReportingSwitch(true));
  REQUIRE_NOTHROW(rti->setSendServiceReportsToFileSwitch(true));
  auto const interactionClassText = interactionClass.toString();
  std::string interactionClassValue;
  interactionClassValue.reserve(interactionClassText.size());
  for (wchar_t const character : interactionClassText) {
    REQUIRE(character >= L' ');
    REQUIRE(character <= L'~');
    interactionClassValue.push_back(static_cast<char>(character));
  }
  auto const expectedPublishRecord = std::string{
      R"({"HLAserialNumber":0,"HLAreturnedArgument":[null],"HLAservice":"PublishInteractionClass","HLAsuppliedArguments":[{"HLAargumentType":27,"HLAargumentName":"Interaction class designator","HLAargumentValue":")" +
      interactionClassValue +
      R"("}],"HLAsuccessIndicator":true,"HLAexception":null})"};
  REQUIRE_NOTHROW(rti->publishInteractionClass(interactionClass));
  REQUIRE(readTextFile(reportFile) == initialText + expectedPublishRecord);

  REQUIRE_NOTHROW(rti->unpublishInteractionClass(interactionClass));
  auto const expectedRecord = std::string{
      R"({"HLAserialNumber":1,"HLAreturnedArgument":[null],"HLAservice":"UnpublishInteractionClass","HLAsuppliedArguments":[{"HLAargumentType":27,"HLAargumentName":"Interaction class designator","HLAargumentValue":")" +
      interactionClassValue +
      R"("}],"HLAsuccessIndicator":true,"HLAexception":null})"};
  REQUIRE(readTextFile(reportFile) == initialText + expectedPublishRecord + expectedRecord);
  REQUIRE_FALSE(rti->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(readTextFile(reportFile) == initialText + expectedPublishRecord + expectedRecord);

  REQUIRE_NOTHROW(rti->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(rti->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(rti->disconnect());
}


TEST_CASE(
    "Embedded service reporting records failed interaction-class declaration invocations",
    "[integration][development-profile][federation-management][declaration-management]"
    "[mom][service-report-file][service-reporting][service-failure]"
    "[declaration-interaction-failure][rti.service.declaration-interaction-failure-matrix]") {
  TestFederateAmbassador reports;
  auto rti = makeRti();
  auto const federationName = nextFederationName();
  auto const restaurantFom = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  auto const switchFom =
      (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data" /
       "switch-support-enabled-fom.xml")
          .wstring();
  auto directory = temporaryServiceReportDirectory();
  auto configuration = configurationForServiceReportDirectory(directory.path());
  configuration.withRtiAddress(L"in-process");

  REQUIRE_NOTHROW(rti->connect(reports, HLA_EVOKED, configuration));
  REQUIRE_NOTHROW(rti->createFederationExecution(
      federationName,
      std::vector<std::wstring>{restaurantFom, switchFom},
      standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(rti->joinFederationExecution(
      L"declaration-interaction-failure-subject", L"publisher", federationName));
  auto const files = serviceReportFiles(directory.path());
  REQUIRE(files.size() == 1U);
  auto const reportFile = files.front();
  auto const initialText = readTextFile(reportFile);

  REQUIRE_NOTHROW(rti->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(rti->setSendServiceReportsToFileSwitch(false));
  auto const interactionClass = rti->getInteractionClassHandle(
      fixture_hla::fom::server_take_order);
  REQUIRE(interactionClass.isValid());
  REQUIRE_NOTHROW(rti->setServiceReportingSwitch(true));
  REQUIRE_NOTHROW(rti->setSendServiceReportsToFileSwitch(true));

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
  auto const quote = [](std::string const& value) {
    return std::string{"\""} + value + "\"";
  };
  auto const argument = [](std::string const& value) {
    return std::string{
               "{\"HLAargumentType\":27,\"HLAargumentName\":\"Interaction class designator\",\"HLAargumentValue\":"} +
        value + "}";
  };
  auto const reportRecord = [&quote](std::uint32_t serial,
                               std::string const& service,
                               std::string const& suppliedArgument,
                               bool success,
                               std::string const& exception) {
    return std::string{"{\"HLAserialNumber\":"} + std::to_string(serial) +
        ",\"HLAreturnedArgument\":[null],\"HLAservice\":\"" + service +
        "\",\"HLAsuppliedArguments\":[" + suppliedArgument +
        "],\"HLAsuccessIndicator\":" + (success ? "true" : "false") +
        ",\"HLAexception\":" +
        (exception.empty() ? std::string{"null"} : quote(exception)) + "}";
  };
  auto const invalidArgument = argument(quote(asAscii(InteractionClassHandle{}.toString())));
  auto const validArgument = argument(quote(asAscii(interactionClass.toString())));

  REQUIRE_THROWS_AS(
      rti->publishInteractionClass(InteractionClassHandle{}),
      rti1516_2025::InteractionClassNotDefined);
  auto const failedPublish = reportRecord(
      0U,
      "PublishInteractionClass",
      invalidArgument,
      false,
      "InteractionClassNotDefined: Publish Interaction Class requires a defined InteractionClassHandle.");
  REQUIRE(readTextFile(reportFile) == initialText + failedPublish);

  REQUIRE_THROWS_AS(
      rti->unpublishInteractionClass(InteractionClassHandle{}),
      rti1516_2025::InteractionClassNotDefined);
  auto const failedUnpublish = reportRecord(
      1U,
      "UnpublishInteractionClass",
      invalidArgument,
      false,
      "InteractionClassNotDefined: Unpublish Interaction Class requires a defined InteractionClassHandle.");
  REQUIRE(readTextFile(reportFile) == initialText + failedPublish + failedUnpublish);

  REQUIRE_NOTHROW(rti->publishInteractionClass(interactionClass));
  auto const successfulPublish = reportRecord(
      2U, "PublishInteractionClass", validArgument, true, {});
  REQUIRE(readTextFile(reportFile) ==
          initialText + failedPublish + failedUnpublish + successfulPublish);
  REQUIRE_NOTHROW(rti->unpublishInteractionClass(interactionClass));
  auto const successfulUnpublish = reportRecord(
      3U, "UnpublishInteractionClass", validArgument, true, {});
  REQUIRE(readTextFile(reportFile) ==
          initialText + failedPublish + failedUnpublish + successfulPublish +
              successfulUnpublish);

  REQUIRE_NOTHROW(rti->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(rti->setSendServiceReportsToFileSwitch(false));
  REQUIRE_NOTHROW(rti->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(rti->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(rti->disconnect());
}
}  // namespace
