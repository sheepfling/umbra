#include "ieee1516_2025_federation_management_fixture_support.hpp"

namespace {
TEST_CASE(
    "Embedded service reporting preserves Unconditional Attribute Ownership Divestiture arguments",
    "[integration][development-profile][federation-management][object-management]"
    "[ownership-management][mom][service-report-file][service-reporting]"
    "[unconditional-attribute-ownership-divestiture]"
    "[rti.service.publish-object-class-attributes]"
    "[rti.service.subscribe-object-class-attributes]"
    "[rti.service.register-object-instance]"
    "[rti.service.unconditional-attribute-ownership-divestiture]"
    "[federate.callback.request-attribute-ownership-assumption]") {
  ReportingFederateAmbassador ownerReports;
  ReportingFederateAmbassador candidateReports;
  auto owner = makeRti();
  auto candidate = makeRti();
  auto const federationName = nextFederationName();
  auto const ownershipFom = (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
                             "cpp" / "tests" / "data" /
                             "attribute-update-passel-fom.xml")
                                .wstring();
  auto const switchFom =
      (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data" /
       "switch-support-enabled-fom.xml")
          .wstring();
  std::vector<std::wstring> const fomModules{ownershipFom, switchFom};
  auto directory = temporaryServiceReportDirectory();
  auto configuration = configurationForServiceReportDirectory(directory.path());
  configuration.withRtiAddress(L"in-process");

  REQUIRE_NOTHROW(owner->connect(ownerReports, HLA_EVOKED, configuration));
  REQUIRE_NOTHROW(candidate->connect(candidateReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      owner->createFederationExecution(federationName, fomModules, L"HLAinteger64Time"));
  REQUIRE_NOTHROW(
      owner->joinFederationExecution(L"divestiture-report-owner", L"publisher", federationName));
  REQUIRE_NOTHROW(candidate->joinFederationExecution(
      L"divestiture-report-candidate", L"publisher", federationName));

  auto const files = serviceReportFiles(directory.path());
  REQUIRE(files.size() == 1U);
  auto const reportFile = files.front();

  REQUIRE_NOTHROW(owner->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(owner->setSendServiceReportsToFileSwitch(false));
  auto const child = owner->getObjectClassHandle(
      L"HLAobjectRoot.UmbraAttributeFixtureBase.UmbraAttributeFixtureChild");
  auto const attribute = owner->getAttributeHandle(child, L"ReliableBaseA");
  REQUIRE(child.isValid());
  REQUIRE(attribute.isValid());
  AttributeHandleSet const attributes{attribute};
  REQUIRE_NOTHROW(owner->setServiceReportingSwitch(true));
  REQUIRE_NOTHROW(owner->setSendServiceReportsToFileSwitch(true));
  REQUIRE_NOTHROW(candidate->subscribeObjectClassAttributes(child, attributes));
  REQUIRE_NOTHROW(owner->publishObjectClassAttributes(child, attributes));
  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = owner->registerObjectInstance(child));
  REQUIRE_FALSE(candidate->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(candidateReports.objectDiscoveryReports.size() == 1U);
  REQUIRE_NOTHROW(candidate->publishObjectClassAttributes(child, attributes));

  auto const initialText = readTextFile(reportFile);
  unsigned char const tagBytes[] = {0x00U, 0xffU, 0x10U, 0xa5U};
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));

  // A rejected §7.2 invocation has no successful-void service record.
  ObjectInstanceHandle const unknownObject;
  REQUIRE_THROWS_AS(
      owner->unconditionalAttributeOwnershipDivestiture(unknownObject, attributes, tag),
      rti1516_2025::ObjectInstanceNotKnown);
  REQUIRE(readTextFile(reportFile) == initialText);

  REQUIRE_NOTHROW(
      owner->unconditionalAttributeOwnershipDivestiture(objectInstance, attributes, tag));
  REQUIRE_FALSE(owner->isAttributeOwnedByFederate(objectInstance, attribute));

  auto const objectInstanceText = objectInstance.toString();
  std::string objectInstanceValue;
  objectInstanceValue.reserve(objectInstanceText.size());
  for (wchar_t const character : objectInstanceText) {
    REQUIRE(character >= L' ');
    REQUIRE(character <= L'~');
    objectInstanceValue.push_back(static_cast<char>(character));
  }
  auto const attributeText = attribute.toString();
  std::string attributeValue;
  attributeValue.reserve(attributeText.size());
  for (wchar_t const character : attributeText) {
    REQUIRE(character >= L' ');
    REQUIRE(character <= L'~');
    attributeValue.push_back(static_cast<char>(character));
  }
  auto const expectedRecord = std::string{
      R"({"HLAserialNumber":2,"HLAreturnedArgument":[null],"HLAservice":"UnconditionalAttributeOwnershipDivestiture","HLAsuppliedArguments":[{"HLAargumentType":37,"HLAargumentName":"Object instance designator","HLAargumentValue":")" +
      objectInstanceValue +
      R"("},{"HLAargumentType":1,"HLAargumentName":"Set of attribute designators","HLAargumentValue":[)" +
      '"' + attributeValue +
      R"("]},{"HLAargumentType":63,"HLAargumentName":"User-supplied tag","HLAargumentValue":"AP8QpQ=="}],"HLAsuccessIndicator":true,"HLAexception":null})"};
  // The §7.2 accepted invocation is recorded before its separately queued
  // §7.4 ownership-assumption callback. The source's Table 5 literal type 63
  // is intentionally confined to this file-record assertion (RL-077).
  REQUIRE(readTextFile(reportFile) == initialText + expectedRecord);
  REQUIRE(candidateReports.attributeOwnershipAssumptionReports.empty());
  REQUIRE_FALSE(candidate->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(candidateReports.attributeOwnershipAssumptionReports.size() == 1U);
  auto const& assumption = candidateReports.attributeOwnershipAssumptionReports.front();
  REQUIRE(assumption.objectInstance == objectInstance);
  REQUIRE(assumption.attributes == attributes);
  REQUIRE(variableLengthDataBytes(assumption.userSuppliedTag) ==
          std::vector<unsigned char>(tagBytes, tagBytes + sizeof(tagBytes)));
  REQUIRE(readTextFile(reportFile) == initialText + expectedRecord);

  REQUIRE_NOTHROW(owner->resignFederationExecution(CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(candidate->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(owner->disconnect());
  REQUIRE_NOTHROW(candidate->disconnect());
}
}  // namespace
