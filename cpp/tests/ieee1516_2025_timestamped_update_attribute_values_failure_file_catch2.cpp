#include "ieee1516_2025_federation_management_test_support.hpp"
#include "ieee1516_2025_federation_management_fixture_support.hpp"

namespace {
void runTimestampedUpdateAttributeValuesFailureFileScenario(bool verifyFileSwitchReenablement) {
  ReportingFederateAmbassador publisherReports;
  auto publisher = makeRti();
  auto const federationName = nextFederationName();
  auto const updateFom = (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" /
                          "data" / "attribute-update-passel-fom.xml")
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
  unsigned char const valueBytes[] = {0x01};
  VariableLengthData const value(valueBytes, sizeof(valueBytes));

  REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED, configuration));
  REQUIRE_NOTHROW(publisher->createFederationExecution(
      federationName,
      std::vector<std::wstring>{updateFom, switchFom},
      L"HLAinteger64Time"));
  REQUIRE_NOTHROW(publisher->joinFederationExecution(
      L"timestamped-update-failure-publisher", L"publisher", federationName));
  REQUIRE_NOTHROW(publisher->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(publisher->setSendServiceReportsToFileSwitch(false));

  auto const child = publisher->getObjectClassHandle(
      L"HLAobjectRoot.UmbraAttributeFixtureBase.UmbraAttributeFixtureChild");
  auto const reliableBaseA = publisher->getAttributeHandle(child, L"ReliableBaseA");
  REQUIRE(child.isValid());
  REQUIRE(reliableBaseA.isValid());
  AttributeHandleSet const updatedAttributes{reliableBaseA};
  REQUIRE_NOTHROW(publisher->publishObjectClassAttributes(child, updatedAttributes));
  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = publisher->registerObjectInstance(child));

  REQUIRE_NOTHROW(publisher->enableTimeRegulation(rti1516_2025::HLAinteger64Interval(5)));
  while (publisher->evokeCallback(0.0)) {
  }
  REQUIRE_NOTHROW(publisher->setServiceReportingSwitch(true));
  REQUIRE_NOTHROW(publisher->setSendServiceReportsToFileSwitch(true));
  auto const files = serviceReportFiles(directory.path());
  REQUIRE(files.size() == 1U);
  auto const reportFile = files.front();
  auto const initialText = readTextFile(reportFile);

  AttributeHandleValueMap const values{{reliableBaseA, value}};
  AttributeHandleValueMap const invalidValues{{AttributeHandle{}, value}};
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
  auto const objectValue = asAscii(objectInstance.toString());
  auto const invalidObjectValue = asAscii(ObjectInstanceHandle{}.toString());
  auto const attributeValue = asAscii(reliableBaseA.toString());
  auto const invalidAttributeValue = asAscii(AttributeHandle{}.toString());
  auto const validMap = std::string{"{\""} + attributeValue + "\":\"AQ==\"}";
  auto const invalidMap = std::string{"{\""} + invalidAttributeValue + "\":\"AQ==\"}";
  auto const reportRecord = [](std::uint32_t serial,
                               std::string const& objectText,
                               std::string const& mapText,
                               std::string const& timestampText,
                               std::string const& exception) {
    return std::string{"{\"HLAserialNumber\":"} + std::to_string(serial) +
        R"(,"HLAreturnedArgument":[null],"HLAservice":"UpdateAttributeValues","HLAsuppliedArguments":[{"HLAargumentType":37,"HLAargumentName":"Object instance designator","HLAargumentValue":")" +
        objectText + R"("},{"HLAargumentType":2,"HLAargumentName":"Constrained set of attribute designator and value pairs","HLAargumentValue":)" +
        mapText +
        R"(},{"HLAargumentType":63,"HLAargumentName":"User-supplied tag","HLAargumentValue":"dHNv"},{"HLAargumentType":31,"HLAargumentName":"Optional timestamp","HLAargumentValue":)" +
        timestampText + R"(}],"HLAsuccessIndicator":false,"HLAexception":")" +
        exception + "\"}";
  };
  auto const unknownObjectFailure = reportRecord(
      0U,
      invalidObjectValue,
      validMap,
      "\"6\"",
      "ObjectInstanceNotKnown: Timestamped Update Attribute Values requires a known ObjectInstanceHandle.");
  REQUIRE_THROWS_AS(
      publisher->updateAttributeValues(
          ObjectInstanceHandle{},
          values,
          tag,
          rti1516_2025::HLAinteger64Time(6)),
      rti1516_2025::ObjectInstanceNotKnown);
  REQUIRE(readTextFile(reportFile) == initialText + unknownObjectFailure);

  auto const invalidAttributeFailure = reportRecord(
      1U,
      objectValue,
      invalidMap,
      "\"6\"",
      "AttributeNotDefined: Timestamped Update Attribute Values requires defined AttributeHandle values.");
  REQUIRE_THROWS_AS(
      publisher->updateAttributeValues(
          objectInstance,
          invalidValues,
          tag,
          rti1516_2025::HLAinteger64Time(6)),
      rti1516_2025::AttributeNotDefined);
  REQUIRE(readTextFile(reportFile) == initialText + unknownObjectFailure + invalidAttributeFailure);

  auto const invalidTimeFailure = reportRecord(
      2U,
      objectValue,
      validMap,
      "\"4\"",
      "InvalidLogicalTime: A timestamped service is earlier than the sender\\'s current logical time plus lookahead.");
  REQUIRE_THROWS_AS(
      publisher->updateAttributeValues(
          objectInstance,
          values,
          tag,
          rti1516_2025::HLAinteger64Time(4)),
      rti1516_2025::InvalidLogicalTime);
  REQUIRE(readTextFile(reportFile) ==
          initialText + unknownObjectFailure + invalidAttributeFailure + invalidTimeFailure);

  if (verifyFileSwitchReenablement) {
    REQUIRE_NOTHROW(publisher->setSendServiceReportsToFileSwitch(false));
    auto const textWhileFileOutputDisabled = readTextFile(reportFile);
    auto const filesWhileFileOutputDisabled = serviceReportFiles(directory.path());
    REQUIRE(filesWhileFileOutputDisabled.size() == 1U);
    REQUIRE(filesWhileFileOutputDisabled.front() == reportFile);
    REQUIRE_THROWS_AS(
        publisher->updateAttributeValues(
            ObjectInstanceHandle{},
            values,
            tag,
            rti1516_2025::HLAinteger64Time(6)),
        rti1516_2025::ObjectInstanceNotKnown);
    REQUIRE(readTextFile(reportFile) == textWhileFileOutputDisabled);

    REQUIRE_NOTHROW(publisher->setSendServiceReportsToFileSwitch(true));
    auto const filesAfterFileOutputReenabled = serviceReportFiles(directory.path());
    REQUIRE(filesAfterFileOutputReenabled.size() == 1U);
    REQUIRE(filesAfterFileOutputReenabled.front() == reportFile);
    auto const textAfterFileOutputReenabled = readTextFile(reportFile);
    REQUIRE(textAfterFileOutputReenabled.starts_with(textWhileFileOutputDisabled));
    REQUIRE_THROWS_AS(
        publisher->updateAttributeValues(
            ObjectInstanceHandle{},
            values,
            tag,
            rti1516_2025::HLAinteger64Time(6)),
        rti1516_2025::ObjectInstanceNotKnown);
    auto const textAfterFailureReenabled = readTextFile(reportFile);
    REQUIRE(textAfterFailureReenabled.starts_with(textAfterFileOutputReenabled));
    REQUIRE(textAfterFailureReenabled.size() > textAfterFileOutputReenabled.size());
    auto const filesAfterFailureReenabled = serviceReportFiles(directory.path());
    REQUIRE(filesAfterFailureReenabled.size() == 1U);
    REQUIRE(filesAfterFailureReenabled.front() == reportFile);
  }

  REQUIRE_NOTHROW(publisher->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(publisher->setSendServiceReportsToFileSwitch(false));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(publisher->disconnect());
}
}  // namespace

TEST_CASE(
    "Embedded service reporting records failed timestamped Update Attribute Values invocations (restored baseline copy)",
    "[integration][development-profile][federation-management][object-management][time-management]"
    "[mom][service-report-file][service-reporting][service-failure][tso]"
    "[timestamped-attribute-update-failure][rti.service.timestamped-attribute-update-failure-matrix]") {
  runTimestampedUpdateAttributeValuesFailureFileScenario(false);
}

TEST_CASE(
    "Embedded service reporting records failed timestamped Update Attribute Values invocations",
    "[integration][development-profile][federation-management][object-management][time-management]"
    "[mom][service-report-file][service-reporting][service-failure][tso]"
    "[timestamped-attribute-update-failure][timestamped-update-failure-service-report-file]"
    "[rti.service.timestamped-attribute-update-failure-matrix]") {
  runTimestampedUpdateAttributeValuesFailureFileScenario(true);
}
