#include "ieee1516_2025_federation_management_test_support.hpp"
#include "ieee1516_2025_federation_management_fixture_support.hpp"

namespace {
TEST_CASE(
    "Embedded service reporting preserves Unsubscribe Interaction Class arguments",
    "[integration][development-profile][federation-management][declaration-management]"
    "[mom][service-report-file][service-reporting]"
    "[unsubscribe-interaction-class]"
    "[rti.service.unsubscribe-interaction-class]") {
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
      rti->joinFederationExecution(L"unsubscribe-interaction-report-subject", L"subscriber", federationName));

  auto const files = serviceReportFiles(directory.path());
  REQUIRE(files.size() == 1U);
  auto const reportFile = files.front();
  auto const initialText = readTextFile(reportFile);

  // The failed-service matrix is covered below; keep this argument regression
  // focused on the accepted call.
  REQUIRE_NOTHROW(rti->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(rti->setSendServiceReportsToFileSwitch(false));
  InteractionClassHandle const unknownInteraction;
  REQUIRE_THROWS_AS(
      rti->unsubscribeInteractionClass(unknownInteraction),
      rti1516_2025::InteractionClassNotDefined);
  REQUIRE(readTextFile(reportFile) == initialText);

  auto const interactionClass = rti->getInteractionClassHandle(
      fixture_hla::fom::server_take_order);
  REQUIRE(interactionClass.isValid());
  REQUIRE_NOTHROW(rti->setServiceReportingSwitch(true));
  REQUIRE_NOTHROW(rti->setSendServiceReportsToFileSwitch(true));
  // §5.11 has no precondition that the ordinary subscription already exists;
  // this accepted idempotent removal isolates the service-report boundary.
  REQUIRE_NOTHROW(rti->unsubscribeInteractionClass(interactionClass));
  auto const interactionClassText = interactionClass.toString();
  std::string interactionClassValue;
  interactionClassValue.reserve(interactionClassText.size());
  for (wchar_t const character : interactionClassText) {
    REQUIRE(character >= L' ');
    REQUIRE(character <= L'~');
    interactionClassValue.push_back(static_cast<char>(character));
  }
  auto const expectedRecord = std::string{
      R"({"HLAserialNumber":0,"HLAreturnedArgument":[null],"HLAservice":"UnsubscribeInteractionClass","HLAsuppliedArguments":[{"HLAargumentType":27,"HLAargumentName":"Interaction class designator","HLAargumentValue":")" +
      interactionClassValue +
      R"("}],"HLAsuccessIndicator":true,"HLAexception":null})"};
  REQUIRE(readTextFile(reportFile) == initialText + expectedRecord);
  REQUIRE_FALSE(rti->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(readTextFile(reportFile) == initialText + expectedRecord);

  REQUIRE_NOTHROW(rti->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(rti->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(rti->disconnect());
}

TEST_CASE(
    "Embedded service reporting records failed interaction-class subscription invocations",
    "[integration][development-profile][federation-management][declaration-management]"
    "[mom][service-report-file][service-reporting][service-failure]"
    "[declaration-subscription-failure][rti.service.declaration-subscription-failure-matrix]") {
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
      L"declaration-subscription-failure-subject", L"subscriber", federationName));
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
  auto const argument = [](int type, std::string const& name, std::string const& value) {
    return std::string{"{\"HLAargumentType\":"} + std::to_string(type) +
        ",\"HLAargumentName\":\"" + name + "\",\"HLAargumentValue\":" + value + "}";
  };
  auto const reportRecord = [&quote](std::uint32_t serial,
                                     std::string const& service,
                                     std::vector<std::string> const& arguments,
                                     bool success,
                                     std::string const& exception) {
    std::string result = std::string{"{\"HLAserialNumber\":"} + std::to_string(serial) +
        ",\"HLAreturnedArgument\":[null],\"HLAservice\":\"" + service +
        "\",\"HLAsuppliedArguments\":[";
    for (std::size_t index = 0U; index < arguments.size(); ++index) {
      if (index != 0U) {
        result += ',';
      }
      result += arguments[index];
    }
    result += std::string{
        "],\"HLAsuccessIndicator\":"} + (success ? "true" : "false") +
        ",\"HLAexception\":" +
        (exception.empty() ? std::string{"null"} : quote(exception)) + "}";
    return result;
  };
  auto const invalidValue = asAscii(InteractionClassHandle{}.toString());
  auto const validValue = asAscii(interactionClass.toString());
  auto const failedSubscribeArguments = std::vector<std::string>{
      argument(27, "Interaction class designator", quote(invalidValue)),
      argument(6, "Optional passive subscription indicator", "true")};
  auto const failedUnsubscribeArguments = std::vector<std::string>{
      argument(27, "Interaction class designator", quote(invalidValue))};
  auto const successfulSubscribeArguments = std::vector<std::string>{
      argument(27, "Interaction class designator", quote(validValue)),
      argument(6, "Optional passive subscription indicator", "false")};
  auto const successfulUnsubscribeArguments = std::vector<std::string>{
      argument(27, "Interaction class designator", quote(validValue))};

  REQUIRE_THROWS_AS(
      rti->subscribeInteractionClass(InteractionClassHandle{}, false),
      rti1516_2025::InteractionClassNotDefined);
  auto const failedSubscribe = reportRecord(
      0U,
      "SubscribeInteractionClass",
      failedSubscribeArguments,
      false,
      "InteractionClassNotDefined: Subscribe Interaction Class requires a defined InteractionClassHandle.");
  REQUIRE(readTextFile(reportFile) == initialText + failedSubscribe);

  REQUIRE_THROWS_AS(
      rti->unsubscribeInteractionClass(InteractionClassHandle{}),
      rti1516_2025::InteractionClassNotDefined);
  auto const failedUnsubscribe = reportRecord(
      1U,
      "UnsubscribeInteractionClass",
      failedUnsubscribeArguments,
      false,
      "InteractionClassNotDefined: Unsubscribe Interaction Class requires a defined InteractionClassHandle.");
  REQUIRE(readTextFile(reportFile) == initialText + failedSubscribe + failedUnsubscribe);

  REQUIRE_NOTHROW(rti->subscribeInteractionClass(interactionClass, true));
  auto const successfulSubscribe = reportRecord(
      2U,
      "SubscribeInteractionClass",
      successfulSubscribeArguments,
      true,
      {});
  REQUIRE(readTextFile(reportFile) ==
          initialText + failedSubscribe + failedUnsubscribe + successfulSubscribe);

  REQUIRE_NOTHROW(rti->unsubscribeInteractionClass(interactionClass));
  auto const successfulUnsubscribe = reportRecord(
      3U,
      "UnsubscribeInteractionClass",
      successfulUnsubscribeArguments,
      true,
      {});
  REQUIRE(readTextFile(reportFile) ==
          initialText + failedSubscribe + failedUnsubscribe + successfulSubscribe +
              successfulUnsubscribe);

  REQUIRE_NOTHROW(rti->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(rti->setSendServiceReportsToFileSwitch(false));
  REQUIRE_NOTHROW(rti->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(rti->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(rti->disconnect());
}
}  // namespace
