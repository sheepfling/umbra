#include "ieee1516_2025_federation_management_fixture_support.hpp"
#include "ieee1516_2025_public_federation_restore_test_support.hpp"

namespace {
TEST_CASE(
    "Embedded service reporting records failed directed declaration invocations",
    "[integration][development-profile][federation-management][declaration-management]"
    "[mom][service-report-file][service-reporting][service-failure]"
    "[directed-declaration-failure][rti.service.directed-declaration-failure-matrix]") {
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
      L"directed-declaration-failure-subject", L"publisher", federationName));
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
  auto const publishSetName = std::string{"Set of interaction class designators"};
  auto const unpublishSetName = std::string{"Optional set of interaction class designators"};
  auto const publishArguments = [&](std::string const& objectValue,
                                    std::string const& setValue) {
    return std::vector<std::string>{
        argument(36, "Object class designator", quote(objectValue)),
        argument(28, publishSetName, setValue)};
  };
  auto const unpublishArguments = [&](std::string const& objectValue,
                                      std::string const& setValue) {
    return std::vector<std::string>{
        argument(36, "Object class designator", quote(objectValue)),
        argument(28, unpublishSetName, setValue)};
  };
  auto const unpublishWholeArguments = [&](std::string const& objectValue) {
    return std::vector<std::string>{
        argument(36, "Object class designator", quote(objectValue)),
        argument(34, unpublishSetName, "null")};
  };
  auto const invalidSet = std::string{"[\""} + invalidInteractionValue + "\"]";
  auto const validSet = std::string{"[\""} + validInteractionValue + "\"]";

  REQUIRE_THROWS_AS(
      rti->publishObjectClassDirectedInteractions(
          ObjectClassHandle{}, InteractionClassHandleSet{}),
      rti1516_2025::ObjectClassNotDefined);
  auto const failedPublishObject = reportRecord(
      0U,
      "PublishObjectClassDirectedInteractions",
      publishArguments(invalidObjectValue, "[]"),
      false,
      "ObjectClassNotDefined: Publish Object Class Directed Interactions requires a defined ObjectClassHandle.");
  REQUIRE(readTextFile(reportFile) == initialText + failedPublishObject);

  REQUIRE_THROWS_AS(
      rti->publishObjectClassDirectedInteractions(
          objectClass, InteractionClassHandleSet{InteractionClassHandle{}}),
      rti1516_2025::InteractionClassNotDefined);
  auto const failedPublishInteraction = reportRecord(
      1U,
      "PublishObjectClassDirectedInteractions",
      publishArguments(validObjectValue, invalidSet),
      false,
      "InteractionClassNotDefined: Publish Object Class Directed Interactions requires defined InteractionClassHandle values.");
  REQUIRE(readTextFile(reportFile) == initialText + failedPublishObject + failedPublishInteraction);

  REQUIRE_THROWS_AS(
      rti->unpublishObjectClassDirectedInteractions(
          ObjectClassHandle{}, InteractionClassHandleSet{}),
      rti1516_2025::ObjectClassNotDefined);
  auto const failedUnpublishObject = reportRecord(
      2U,
      "UnpublishObjectClassDirectedInteractions",
      unpublishArguments(invalidObjectValue, "[]"),
      false,
      "ObjectClassNotDefined: Unpublish Object Class Directed Interactions requires a defined ObjectClassHandle.");
  REQUIRE(readTextFile(reportFile) ==
          initialText + failedPublishObject + failedPublishInteraction + failedUnpublishObject);

  REQUIRE_THROWS_AS(
      rti->unpublishObjectClassDirectedInteractions(
          objectClass, InteractionClassHandleSet{InteractionClassHandle{}}),
      rti1516_2025::InteractionClassNotDefined);
  auto const failedUnpublishInteraction = reportRecord(
      3U,
      "UnpublishObjectClassDirectedInteractions",
      unpublishArguments(validObjectValue, invalidSet),
      false,
      "InteractionClassNotDefined: Unpublish Object Class Directed Interactions requires defined InteractionClassHandle values.");
  REQUIRE(readTextFile(reportFile) ==
          initialText + failedPublishObject + failedPublishInteraction + failedUnpublishObject +
              failedUnpublishInteraction);

  InteractionClassHandleSet const interactionClasses{interactionClass};
  REQUIRE_NOTHROW(
      rti->publishObjectClassDirectedInteractions(objectClass, interactionClasses));
  auto const successfulPublish = reportRecord(
      4U,
      "PublishObjectClassDirectedInteractions",
      publishArguments(validObjectValue, validSet),
      true,
      {});
  REQUIRE(readTextFile(reportFile) ==
          initialText + failedPublishObject + failedPublishInteraction + failedUnpublishObject +
              failedUnpublishInteraction + successfulPublish);

  REQUIRE_NOTHROW(
      rti->unpublishObjectClassDirectedInteractions(objectClass, interactionClasses));
  auto const successfulUnpublish = reportRecord(
      5U,
      "UnpublishObjectClassDirectedInteractions",
      unpublishArguments(validObjectValue, validSet),
      true,
      {});
  REQUIRE(readTextFile(reportFile) ==
          initialText + failedPublishObject + failedPublishInteraction + failedUnpublishObject +
              failedUnpublishInteraction + successfulPublish + successfulUnpublish);

  REQUIRE_NOTHROW(rti->unpublishObjectClassDirectedInteractions(objectClass));
  auto const successfulWholeUnpublish = reportRecord(
      6U,
      "UnpublishObjectClassDirectedInteractions",
      unpublishWholeArguments(validObjectValue),
      true,
      {});
  REQUIRE(readTextFile(reportFile) ==
          initialText + failedPublishObject + failedPublishInteraction + failedUnpublishObject +
              failedUnpublishInteraction + successfulPublish + successfulUnpublish +
              successfulWholeUnpublish);

  REQUIRE_NOTHROW(rti->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(rti->setSendServiceReportsToFileSwitch(false));
  REQUIRE_NOTHROW(rti->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(rti->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(rti->disconnect());
}

}  // namespace
