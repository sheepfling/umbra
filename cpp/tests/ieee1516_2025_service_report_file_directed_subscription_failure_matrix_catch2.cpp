#include "ieee1516_2025_federation_management_fixture_support.hpp"
#include "ieee1516_2025_public_federation_restore_test_support.hpp"

namespace {
TEST_CASE(
    "Embedded service reporting records failed directed subscription invocations",
    "[integration][development-profile][federation-management][declaration-management]"
    "[mom][service-report-file][service-reporting][service-failure]"
    "[directed-subscription-failure][rti.service.directed-subscription-failure-matrix]") {
  using namespace public_federation_restore_test_support;
  TestFederateAmbassador reports;
  auto rti = makeRti();
  auto const federationName = nextFederationName();
  auto const objectConsumer =
      (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data" /
       "directed-interaction-object-consumer-fom.xml")
          .wstring();
  auto const interactionProvider =
      (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data" /
       "directed-interaction-interaction-provider-fom.xml")
          .wstring();
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
      std::vector<std::wstring>{objectConsumer, interactionProvider, switchFom},
      standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(rti->joinFederationExecution(
      L"directed-subscription-failure-subject", L"subscriber", federationName));
  auto const files = serviceReportFiles(directory.path());
  REQUIRE(files.size() == 1U);
  auto const reportFile = files.front();
  auto const initialText = readTextFile(reportFile);

  REQUIRE_NOTHROW(rti->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(rti->setSendServiceReportsToFileSwitch(false));
  auto const objectClass = rti->getObjectClassHandle(
      fixture_hla::fom::directed_fixture_object);
  auto const interactionClass = rti->getInteractionClassHandle(
      fixture_hla::fom::directed_fixture_interaction);
  REQUIRE(objectClass.isValid());
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
  auto const invalidObjectValue = asAscii(ObjectClassHandle{}.toString());
  auto const validObjectValue = asAscii(objectClass.toString());
  auto const invalidInteractionValue = asAscii(InteractionClassHandle{}.toString());
  auto const validInteractionValue = asAscii(interactionClass.toString());
  auto const invalidSet = std::string{"[\""} + invalidInteractionValue + "\"]";
  auto const validSet = std::string{"[\""} + validInteractionValue + "\"]";
  auto const subscribeArguments = [&](std::string const& objectValue,
                                      std::string const& setValue,
                                      bool universally) {
    return std::vector<std::string>{
        argument(36, "Object class designator", quote(objectValue)),
        argument(28, "Set of directed interaction designators", setValue),
        argument(6, "Optional universal subscription indicator",
                 universally ? "true" : "false")};
  };
  auto const unsubscribeArguments = [&](std::string const& objectValue,
                                        std::string const& setValue) {
    return std::vector<std::string>{
        argument(36, "Object class designator", quote(objectValue)),
        argument(28, "Optional set of directed interaction designators", setValue)};
  };
  auto const unsubscribeWholeArguments = [&](std::string const& objectValue) {
    return std::vector<std::string>{
        argument(36, "Object class designator", quote(objectValue)),
        argument(34, "Optional set of directed interaction designators", "null")};
  };

  REQUIRE_THROWS_AS(
      rti->subscribeObjectClassDirectedInteractions(
          ObjectClassHandle{}, InteractionClassHandleSet{}, false),
      rti1516_2025::ObjectClassNotDefined);
  auto const failedSubscribeObject = reportRecord(
      0U,
      "SubscribeObjectClassDirectedInteractions",
      subscribeArguments(invalidObjectValue, "[]", false),
      false,
      "ObjectClassNotDefined: Subscribe Object Class Directed Interactions requires a defined ObjectClassHandle.");
  REQUIRE(readTextFile(reportFile) == initialText + failedSubscribeObject);

  REQUIRE_THROWS_AS(
      rti->subscribeObjectClassDirectedInteractions(
          objectClass, InteractionClassHandleSet{InteractionClassHandle{}}, true),
      rti1516_2025::InteractionClassNotDefined);
  auto const failedSubscribeInteraction = reportRecord(
      1U,
      "SubscribeObjectClassDirectedInteractions",
      subscribeArguments(validObjectValue, invalidSet, true),
      false,
      "InteractionClassNotDefined: Subscribe Object Class Directed Interactions requires defined InteractionClassHandle values.");
  REQUIRE(readTextFile(reportFile) == initialText + failedSubscribeObject + failedSubscribeInteraction);

  REQUIRE_THROWS_AS(
      rti->unsubscribeObjectClassDirectedInteractions(
          ObjectClassHandle{}, InteractionClassHandleSet{}),
      rti1516_2025::ObjectClassNotDefined);
  auto const failedUnsubscribeObject = reportRecord(
      2U,
      "UnsubscribeObjectClassDirectedInteractions",
      unsubscribeArguments(invalidObjectValue, "[]"),
      false,
      "ObjectClassNotDefined: Unsubscribe Object Class Directed Interactions requires a defined ObjectClassHandle.");
  REQUIRE(readTextFile(reportFile) ==
          initialText + failedSubscribeObject + failedSubscribeInteraction + failedUnsubscribeObject);

  REQUIRE_THROWS_AS(
      rti->unsubscribeObjectClassDirectedInteractions(
          objectClass, InteractionClassHandleSet{InteractionClassHandle{}}),
      rti1516_2025::InteractionClassNotDefined);
  auto const failedUnsubscribeInteraction = reportRecord(
      3U,
      "UnsubscribeObjectClassDirectedInteractions",
      unsubscribeArguments(validObjectValue, invalidSet),
      false,
      "InteractionClassNotDefined: Unsubscribe Object Class Directed Interactions requires defined InteractionClassHandle values.");
  REQUIRE(readTextFile(reportFile) ==
          initialText + failedSubscribeObject + failedSubscribeInteraction + failedUnsubscribeObject +
              failedUnsubscribeInteraction);

  InteractionClassHandleSet const interactionClasses{interactionClass};
  REQUIRE_NOTHROW(rti->subscribeObjectClassDirectedInteractions(
      objectClass, interactionClasses, false));
  auto const successfulSubscribe = reportRecord(
      4U,
      "SubscribeObjectClassDirectedInteractions",
      subscribeArguments(validObjectValue, validSet, false),
      true,
      {});
  REQUIRE(readTextFile(reportFile) ==
          initialText + failedSubscribeObject + failedSubscribeInteraction + failedUnsubscribeObject +
              failedUnsubscribeInteraction + successfulSubscribe);

  REQUIRE_NOTHROW(rti->unsubscribeObjectClassDirectedInteractions(
      objectClass, interactionClasses));
  auto const successfulUnsubscribe = reportRecord(
      5U,
      "UnsubscribeObjectClassDirectedInteractions",
      unsubscribeArguments(validObjectValue, validSet),
      true,
      {});
  REQUIRE(readTextFile(reportFile) ==
          initialText + failedSubscribeObject + failedSubscribeInteraction + failedUnsubscribeObject +
              failedUnsubscribeInteraction + successfulSubscribe + successfulUnsubscribe);

  REQUIRE_NOTHROW(rti->unsubscribeObjectClassDirectedInteractions(objectClass));
  auto const successfulWholeUnsubscribe = reportRecord(
      6U,
      "UnsubscribeObjectClassDirectedInteractions",
      unsubscribeWholeArguments(validObjectValue),
      true,
      {});
  REQUIRE(readTextFile(reportFile) ==
          initialText + failedSubscribeObject + failedSubscribeInteraction + failedUnsubscribeObject +
              failedUnsubscribeInteraction + successfulSubscribe + successfulUnsubscribe +
              successfulWholeUnsubscribe);

  REQUIRE_NOTHROW(rti->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(rti->setSendServiceReportsToFileSwitch(false));
  REQUIRE_NOTHROW(rti->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(rti->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(rti->disconnect());
}

}  // namespace
