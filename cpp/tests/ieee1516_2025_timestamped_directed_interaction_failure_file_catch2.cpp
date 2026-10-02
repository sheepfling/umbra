#include "ieee1516_2025_federation_management_test_support.hpp"
#include "ieee1516_2025_federation_management_fixture_support.hpp"

namespace {
TEST_CASE(
    "Embedded service reporting records failed timestamped Send Directed Interaction invocations",
    "[integration][development-profile][federation-management][interaction-management][directed][time-management]"
    "[mom][service-report-file][service-reporting][service-failure][tso]"
    "[timestamped-directed-interaction-failure][timestamped-directed-interaction-failure-file]"
    "[rti.service.timestamped-directed-interaction-failure-matrix]") {
  ReportingFederateAmbassador publisherReports;
  auto publisher = makeRti();
  auto const federationName = nextFederationName();
  auto const objectConsumer = (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" /
                               "data" / "directed-interaction-object-consumer-fom.xml")
                                  .wstring();
  auto const interactionProvider = (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" /
                                    "data" / "directed-interaction-interaction-provider-fom.xml")
                                       .wstring();
  auto const switchFom =
      (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data" /
       "switch-support-enabled-fom.xml")
          .wstring();
  auto directory = temporaryServiceReportDirectory();
  auto configuration = configurationForServiceReportDirectory(directory.path());
  configuration.withRtiAddress(L"in-process");
  unsigned char const tagBytes[] = {'t', 's', 'o'};
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));
  unsigned char const parameterBytes[] = {0x01};
  VariableLengthData const parameterValue(parameterBytes, sizeof(parameterBytes));

  REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED, configuration));
  REQUIRE_NOTHROW(publisher->createFederationExecution(
      federationName,
      std::vector<std::wstring>{objectConsumer, interactionProvider, switchFom},
      L"HLAinteger64Time"));
  REQUIRE_NOTHROW(publisher->joinFederationExecution(
      L"timestamped-directed-failure-publisher", L"publisher", federationName));
  REQUIRE_NOTHROW(publisher->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(publisher->setSendServiceReportsToFileSwitch(false));

  auto const objectClass = publisher->getObjectClassHandle(
      L"HLAobjectRoot.UmbraDirectedFixtureObject");
  auto const marker = publisher->getAttributeHandle(objectClass, L"DirectedTargetMarker");
  auto const interactionClass = publisher->getInteractionClassHandle(
      L"HLAinteractionRoot.UmbraDirectedFixtureInteraction");
  REQUIRE(objectClass.isValid());
  REQUIRE(marker.isValid());
  REQUIRE(interactionClass.isValid());
  InteractionClassHandleSet const directedClasses{interactionClass};
  REQUIRE_NOTHROW(publisher->publishObjectClassAttributes(objectClass, {marker}));
  REQUIRE_NOTHROW(publisher->publishObjectClassDirectedInteractions(
      objectClass, directedClasses));
  REQUIRE_NOTHROW(publisher->changeInteractionOrderType(interactionClass, TIMESTAMP));
  ObjectInstanceHandle target;
  REQUIRE_NOTHROW(target = publisher->registerObjectInstance(objectClass));
  REQUIRE(target.isValid());
  REQUIRE_NOTHROW(publisher->enableTimeRegulation(rti1516_2025::HLAinteger64Interval(5)));
  while (publisher->evokeCallback(0.0)) {
  }
  REQUIRE_NOTHROW(publisher->setServiceReportingSwitch(true));
  REQUIRE_NOTHROW(publisher->setSendServiceReportsToFileSwitch(true));
  auto const files = serviceReportFiles(directory.path());
  REQUIRE(files.size() == 1U);
  auto const reportFile = files.front();
  auto const initialText = readTextFile(reportFile);

  ParameterHandleValueMap const values;
  ParameterHandleValueMap const invalidValues{{ParameterHandle{}, parameterValue}};
  auto asAscii = [](std::wstring const& text) {
    std::string result;
    result.reserve(text.size());
    for (wchar_t const character : text) {
      REQUIRE(character >= L' ');
      REQUIRE(character <= L'~');
      result.push_back(static_cast<char>(character));
    }
    return result;
  };
  auto const interactionValue = asAscii(interactionClass.toString());
  auto const invalidInteractionValue = asAscii(InteractionClassHandle{}.toString());
  auto const targetValue = asAscii(target.toString());
  auto const invalidTargetValue = asAscii(ObjectInstanceHandle{}.toString());
  auto const invalidParameterValue = asAscii(ParameterHandle{}.toString());
  auto const invalidMap = std::string{"{\""} + invalidParameterValue + "\":\"AQ==\"}";
  auto const reportRecord = [](std::uint32_t serial,
                               std::string const& interactionText,
                               std::string const& targetText,
                               std::string const& mapText,
                               std::string const& timestampText,
                               std::string const& exception) {
    return std::string{"{\"HLAserialNumber\":"} + std::to_string(serial) +
        R"(,"HLAreturnedArgument":[null],"HLAservice":"SendDirectedInteraction","HLAsuppliedArguments":[{"HLAargumentType":27,"HLAargumentName":"Interaction class designator","HLAargumentValue":")" +
        interactionText + R"("},{"HLAargumentType":37,"HLAargumentName":"Object instance designator","HLAargumentValue":")" +
        targetText + R"("},{"HLAargumentType":40,"HLAargumentName":"Constrained set of interaction parameter designator and value pairs","HLAargumentValue":)" +
        mapText +
        R"(},{"HLAargumentType":63,"HLAargumentName":"User-supplied tag","HLAargumentValue":"dHNv"},{"HLAargumentType":31,"HLAargumentName":"Optional timestamp","HLAargumentValue":)" +
        timestampText + R"(}],"HLAsuccessIndicator":false,"HLAexception":")" +
        exception + "\"}";
  };
  auto const invalidInteractionFailure = reportRecord(
      0U,
      invalidInteractionValue,
      targetValue,
      "{}",
      "\"6\"",
      "InteractionClassNotDefined: Timestamped Send Directed Interaction requires a defined InteractionClassHandle.");
  REQUIRE_THROWS_AS(
      publisher->sendDirectedInteraction(
          InteractionClassHandle{}, target, values, tag, rti1516_2025::HLAinteger64Time(6)),
      rti1516_2025::InteractionClassNotDefined);
  REQUIRE(readTextFile(reportFile) == initialText + invalidInteractionFailure);

  REQUIRE_NOTHROW(publisher->setSendServiceReportsToFileSwitch(false));
  auto const textWhileFileOutputDisabled = readTextFile(reportFile);
  auto const filesWhileFileOutputDisabled = serviceReportFiles(directory.path());
  REQUIRE(filesWhileFileOutputDisabled.size() == 1U);
  REQUIRE(filesWhileFileOutputDisabled.front() == reportFile);
  REQUIRE_THROWS_AS(
      publisher->sendDirectedInteraction(
          interactionClass, ObjectInstanceHandle{}, values, tag, rti1516_2025::HLAinteger64Time(6)),
      rti1516_2025::ObjectInstanceNotKnown);
  REQUIRE(readTextFile(reportFile) == textWhileFileOutputDisabled);

  REQUIRE_NOTHROW(publisher->setSendServiceReportsToFileSwitch(true));
  auto const filesAfterFileOutputReenabled = serviceReportFiles(directory.path());
  REQUIRE(filesAfterFileOutputReenabled.size() == 1U);
  REQUIRE(filesAfterFileOutputReenabled.front() == reportFile);
  REQUIRE(readTextFile(reportFile) == textWhileFileOutputDisabled);

  auto const invalidTargetFailure = reportRecord(
      1U,
      interactionValue,
      invalidTargetValue,
      "{}",
      "\"6\"",
      "ObjectInstanceNotKnown: Timestamped Send Directed Interaction requires a valid target ObjectInstanceHandle.");
  REQUIRE_THROWS_AS(
      publisher->sendDirectedInteraction(
          interactionClass, ObjectInstanceHandle{}, values, tag, rti1516_2025::HLAinteger64Time(6)),
      rti1516_2025::ObjectInstanceNotKnown);
  REQUIRE(readTextFile(reportFile) == initialText + invalidInteractionFailure + invalidTargetFailure);

  auto const invalidParameterFailure = reportRecord(
      2U,
      interactionValue,
      targetValue,
      invalidMap,
      "\"6\"",
      "InteractionParameterNotDefined: Timestamped Send Directed Interaction requires defined ParameterHandle values.");
  REQUIRE_THROWS_AS(
      publisher->sendDirectedInteraction(
          interactionClass, target, invalidValues, tag, rti1516_2025::HLAinteger64Time(6)),
      rti1516_2025::InteractionParameterNotDefined);
  REQUIRE(readTextFile(reportFile) ==
          initialText + invalidInteractionFailure + invalidTargetFailure + invalidParameterFailure);

  auto const invalidTimeFailure = reportRecord(
      3U,
      interactionValue,
      targetValue,
      "{}",
      "\"4\"",
      "InvalidLogicalTime: A timestamped service is earlier than the sender\\'s current logical time plus lookahead.");
  REQUIRE_THROWS_AS(
      publisher->sendDirectedInteraction(
          interactionClass, target, values, tag, rti1516_2025::HLAinteger64Time(4)),
      rti1516_2025::InvalidLogicalTime);
  REQUIRE(readTextFile(reportFile) ==
          initialText + invalidInteractionFailure + invalidTargetFailure + invalidParameterFailure +
              invalidTimeFailure);

  REQUIRE_NOTHROW(publisher->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(publisher->setSendServiceReportsToFileSwitch(false));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(publisher->disconnect());
}
}  // namespace
