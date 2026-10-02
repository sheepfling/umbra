#include "ieee1516_2025_federation_management_test_support.hpp"
#include "ieee1516_2025_federation_management_fixture_support.hpp"

namespace {
TEST_CASE(
    "Embedded service reporting records timestamped Update Attribute Values before reflection callback",
    "[integration][development-profile][federation-management][object-management][time-management]"
    "[mom][service-report-file][service-reporting][timestamped-attribute-update][tso]"
    "[timestamped-attribute-update-service-report]"
    "[rti.service.update-attribute-values][federate.callback.reflect-attribute-values]") {
  ReportingFederateAmbassador publisherReports;
  ReportingFederateAmbassador receiverReports;
  auto publisher = makeRti();
  auto receiver = makeRti();
  auto const federationName = nextFederationName();
  auto const updateFom = (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" /
                          "data" / "attribute-update-passel-fom.xml")
                             .wstring();
  auto const switchFom =
      (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data" /
       "switch-support-enabled-fom.xml")
          .wstring();
  std::vector<std::wstring> const fomModules{updateFom, switchFom};
  auto directory = temporaryServiceReportDirectory();
  auto configuration = configurationForServiceReportDirectory(directory.path());
  configuration.withRtiAddress(L"in-process");
  unsigned char const tagBytes[] = {'t', 's', 'o'};
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));
  unsigned char const valueBytes[] = {0x01, 0x02};
  VariableLengthData const value(valueBytes, sizeof(valueBytes));
  rti1516_2025::HLAinteger64Time const timestamp(1);

  REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED, configuration));
  REQUIRE_NOTHROW(receiver->connect(receiverReports, HLA_EVOKED));
  REQUIRE_NOTHROW(publisher->createFederationExecution(
      federationName, fomModules, L"HLAinteger64Time"));
  REQUIRE_NOTHROW(publisher->joinFederationExecution(
      L"timestamped-update-report-publisher", L"publisher", federationName));
  REQUIRE_NOTHROW(receiver->joinFederationExecution(
      L"timestamped-update-report-receiver", L"subscriber", federationName));

  // Keep setup out of the accepted timestamped sender record. The support FOM
  // seeds both switches, so explicit selection below isolates queue admission.
  REQUIRE_NOTHROW(publisher->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(publisher->setSendServiceReportsToFileSwitch(false));

  auto const child = publisher->getObjectClassHandle(
      L"HLAobjectRoot.UmbraAttributeFixtureBase.UmbraAttributeFixtureChild");
  auto const reliableAttribute = publisher->getAttributeHandle(child, L"ReliableBaseA");
  auto const reliableTransportation = publisher->getTransportationTypeHandle(L"HLAreliable");
  REQUIRE(child.isValid());
  REQUIRE(reliableAttribute.isValid());
  REQUIRE(reliableTransportation.isValid());
  AttributeHandleSet const attributes{reliableAttribute};
  REQUIRE_NOTHROW(publisher->publishObjectClassAttributes(child, attributes));
  REQUIRE_NOTHROW(receiver->subscribeObjectClassAttributes(child, attributes));
  REQUIRE_NOTHROW(publisher->changeDefaultAttributeOrderType(child, attributes, TIMESTAMP));

  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = publisher->registerObjectInstance(child));
  REQUIRE_FALSE(receiver->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(receiverReports.objectDiscoveryReports.size() == 1U);

  auto const files = serviceReportFiles(directory.path());
  REQUIRE(files.size() == 1U);
  auto const reportFile = files.front();
  auto const initialText = readTextFile(reportFile);
  REQUIRE_NOTHROW(publisher->setServiceReportingSwitch(true));
  REQUIRE_NOTHROW(publisher->setSendServiceReportsToFileSwitch(true));
  REQUIRE(readTextFile(reportFile) == initialText);

  AttributeHandleValueMap const values{{reliableAttribute, value}};
  REQUIRE_NOTHROW(publisher->updateAttributeValues(objectInstance, values, tag, timestamp));

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
  auto const attributeValue = asAscii(reliableAttribute.toString());
  auto const timestampValue = asAscii(timestamp.toString());
  auto const expectedRecord = std::string{
      R"({"HLAserialNumber":0,"HLAreturnedArgument":[{"HLAargumentType":34,"HLAargumentName":"","HLAargumentValue":null}],"HLAservice":"UpdateAttributeValues","HLAsuppliedArguments":[{"HLAargumentType":37,"HLAargumentName":"Object instance designator","HLAargumentValue":")" +
      objectValue +
      R"("},{"HLAargumentType":2,"HLAargumentName":"Constrained set of attribute designator and value pairs","HLAargumentValue":{")" +
      attributeValue +
      R"(":"AQI="}},{"HLAargumentType":63,"HLAargumentName":"User-supplied tag","HLAargumentValue":"dHNv"},{"HLAargumentType":31,"HLAargumentName":"Optional timestamp","HLAargumentValue":")" +
      timestampValue + R"("}],"HLAsuccessIndicator":true,"HLAexception":null})"};

  // The shared selector must make the accepted sender record durable before
  // HLA_EVOKED enters the recipient's timestamped Reflect Attribute Values
  // callback. A non-time-regulating sender has the official type-34 Null
  // returned argument.
  REQUIRE(readTextFile(reportFile) == initialText + expectedRecord);
  REQUIRE(receiverReports.attributeReflectionReports.empty());
  bool reportPresentAtCallbackEntry = false;
  receiverReports.onAttributeReflection = [&] {
    reportPresentAtCallbackEntry =
        readTextFile(reportFile) == initialText + expectedRecord;
  };
  REQUIRE_FALSE(receiver->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(reportPresentAtCallbackEntry);
  REQUIRE(receiverReports.attributeReflectionReports.size() == 1U);
  auto const& reflection = receiverReports.attributeReflectionReports.front();
  REQUIRE(reflection.objectInstance == objectInstance);
  REQUIRE(reflection.attributeValues.size() == 1U);
  REQUIRE(variableLengthDataBytes(reflection.attributeValues.at(reliableAttribute)) ==
          std::vector<unsigned char>(valueBytes, valueBytes + sizeof(valueBytes)));
  REQUIRE(variableLengthDataBytes(reflection.userSuppliedTag) ==
          std::vector<unsigned char>(tagBytes, tagBytes + sizeof(tagBytes)));
  REQUIRE(reflection.transportationType == reliableTransportation);
  REQUIRE(reflection.timeImplementationName == L"HLAinteger64Time");
  REQUIRE(reflection.timeValue == timestamp.toString());
  REQUIRE(reflection.sentOrderType == RECEIVE);
  REQUIRE(reflection.receivedOrderType == RECEIVE);
  REQUIRE_FALSE(reflection.retractionSupplied);
  REQUIRE_FALSE(reflection.retractionValid);
  REQUIRE(readTextFile(reportFile) == initialText + expectedRecord);

  REQUIRE_NOTHROW(receiver->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(publisher->disconnect());
  REQUIRE_NOTHROW(receiver->disconnect());
}
} // namespace
