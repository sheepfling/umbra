#include "ieee1516_2025_federation_management_test_support.hpp"
#include "ieee1516_2025_federation_management_fixture_support.hpp"

namespace {
TEST_CASE(
    "Embedded service reporting preserves Unsubscribe Object Class Directed Interactions arguments",
    "[integration][development-profile][federation-management][declaration-management]"
    "[mom][service-report-file][service-reporting]"
    "[unsubscribe-object-class-directed-interactions]"
    "[rti.service.unsubscribe-object-class-directed-interactions]") {
  ReportingFederateAmbassador reports;
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
  std::vector<std::wstring> const fomModules{
      objectConsumer, interactionProvider, switchFom};
  auto directory = temporaryServiceReportDirectory();
  auto configuration = configurationForServiceReportDirectory(directory.path());
  configuration.withRtiAddress(L"in-process");

  REQUIRE_NOTHROW(rti->connect(reports, HLA_EVOKED, configuration));
  REQUIRE_NOTHROW(
      rti->createFederationExecution(federationName, fomModules, standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(rti->joinFederationExecution(
      L"unsubscribe-directed-interactions-report-subject", L"subscriber", federationName));

  auto const files = serviceReportFiles(directory.path());
  REQUIRE(files.size() == 1U);
  auto const reportFile = files.front();
  auto const initialText = readTextFile(reportFile);

  // The dedicated directed-subscription failure matrix owns failed-service
  // records; keep this argument regression focused on accepted forms.
  REQUIRE_NOTHROW(rti->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(rti->setSendServiceReportsToFileSwitch(false));
  ObjectClassHandle const unknownObjectClass;
  REQUIRE_THROWS_AS(
      rti->unsubscribeObjectClassDirectedInteractions(
          unknownObjectClass,
          InteractionClassHandleSet{}),
      rti1516_2025::ObjectClassNotDefined);
  REQUIRE(readTextFile(reportFile) == initialText);

  auto const objectClass = rti->getObjectClassHandle(
      fixture_hla::fom::directed_fixture_object);
  REQUIRE(objectClass.isValid());
  InteractionClassHandle const unknownInteractionClass;
  REQUIRE_THROWS_AS(
      rti->unsubscribeObjectClassDirectedInteractions(
          objectClass,
          InteractionClassHandleSet{unknownInteractionClass}),
      rti1516_2025::InteractionClassNotDefined);
  REQUIRE(readTextFile(reportFile) == initialText);

  auto const interactionClass = rti->getInteractionClassHandle(
      fixture_hla::fom::directed_fixture_interaction);
  REQUIRE(interactionClass.isValid());
  REQUIRE_NOTHROW(rti->setServiceReportingSwitch(true));
  REQUIRE_NOTHROW(rti->setSendServiceReportsToFileSwitch(true));
  InteractionClassHandleSet const interactionClasses{interactionClass};
  // §5.13 permits the supplied set form independently of whether an earlier
  // subscription made a material state change, letting this isolate type-28.
  REQUIRE_NOTHROW(rti->unsubscribeObjectClassDirectedInteractions(
      objectClass,
      interactionClasses));

  auto const objectClassText = objectClass.toString();
  std::string objectClassValue;
  objectClassValue.reserve(objectClassText.size());
  for (wchar_t const character : objectClassText) {
    REQUIRE(character >= L' ');
    REQUIRE(character <= L'~');
    objectClassValue.push_back(static_cast<char>(character));
  }
  auto const interactionClassText = interactionClass.toString();
  std::string interactionClassValue;
  interactionClassValue.reserve(interactionClassText.size());
  for (wchar_t const character : interactionClassText) {
    REQUIRE(character >= L' ');
    REQUIRE(character <= L'~');
    interactionClassValue.push_back(static_cast<char>(character));
  }
  auto const subsetRecord = std::string{
      R"({"HLAserialNumber":0,"HLAreturnedArgument":[null],"HLAservice":"UnsubscribeObjectClassDirectedInteractions","HLAsuppliedArguments":[{"HLAargumentType":36,"HLAargumentName":"Object class designator","HLAargumentValue":")" +
      objectClassValue +
      R"("},{"HLAargumentType":28,"HLAargumentName":"Optional set of directed interaction designators","HLAargumentValue":[")" +
      interactionClassValue +
      R"("]}],"HLAsuccessIndicator":true,"HLAexception":null})"};
  REQUIRE(readTextFile(reportFile) == initialText + subsetRecord);

  // A supplied empty set remains type-28 with an empty array. It is not the
  // whole-class overload's absent optional argument.
  REQUIRE_NOTHROW(rti->unsubscribeObjectClassDirectedInteractions(
      objectClass,
      InteractionClassHandleSet{}));
  auto const emptySetRecord = std::string{
      R"({"HLAserialNumber":1,"HLAreturnedArgument":[null],"HLAservice":"UnsubscribeObjectClassDirectedInteractions","HLAsuppliedArguments":[{"HLAargumentType":36,"HLAargumentName":"Object class designator","HLAargumentValue":")" +
      objectClassValue +
      R"("},{"HLAargumentType":28,"HLAargumentName":"Optional set of directed interaction designators","HLAargumentValue":[]}],"HLAsuccessIndicator":true,"HLAexception":null})"};
  REQUIRE(readTextFile(reportFile) == initialText + subsetRecord + emptySetRecord);

  // The official C++ whole-class overload omits the optional set, so §11.5.1
  // keeps the second supplied-argument slot as Null rather than as [].
  REQUIRE_NOTHROW(rti->unsubscribeObjectClassDirectedInteractions(objectClass));
  auto const wholeClassRecord = std::string{
      R"({"HLAserialNumber":2,"HLAreturnedArgument":[null],"HLAservice":"UnsubscribeObjectClassDirectedInteractions","HLAsuppliedArguments":[{"HLAargumentType":36,"HLAargumentName":"Object class designator","HLAargumentValue":")" +
      objectClassValue +
      R"("},{"HLAargumentType":34,"HLAargumentName":"Optional set of directed interaction designators","HLAargumentValue":null}],"HLAsuccessIndicator":true,"HLAexception":null})"};
  REQUIRE(readTextFile(reportFile) ==
          initialText + subsetRecord + emptySetRecord + wholeClassRecord);
  REQUIRE_FALSE(rti->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(readTextFile(reportFile) ==
          initialText + subsetRecord + emptySetRecord + wholeClassRecord);

  REQUIRE_NOTHROW(rti->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(rti->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(rti->disconnect());
}

}  // namespace
