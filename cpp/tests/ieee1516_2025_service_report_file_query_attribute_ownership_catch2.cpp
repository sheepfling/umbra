#include "ieee1516_2025_federation_management_fixture_support.hpp"

namespace {
TEST_CASE(
    "Embedded service reporting preserves Query Attribute Ownership arguments",
    "[integration][development-profile][federation-management][object-management]"
    "[ownership-management][mom][service-report-file][service-reporting]"
    "[query-attribute-ownership]"
    "[rti.service.publish-object-class-attributes]"
    "[rti.service.subscribe-object-class-attributes]"
    "[rti.service.register-object-instance]"
    "[rti.service.query-attribute-ownership]"
    "[federate.callback.inform-attribute-ownership]"
    "[federate.callback.attribute-is-not-owned]") {
  ReportingFederateAmbassador ownerReports;
  ReportingFederateAmbassador requesterReports;
  auto owner = makeRti();
  auto requester = makeRti();
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

  REQUIRE_NOTHROW(owner->connect(ownerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(requester->connect(requesterReports, HLA_EVOKED, configuration));
  REQUIRE_NOTHROW(
      owner->createFederationExecution(federationName, fomModules, L"HLAinteger64Time"));
  REQUIRE_NOTHROW(
      owner->joinFederationExecution(L"query-ownership-report-owner", L"publisher", federationName));
  REQUIRE_NOTHROW(requester->joinFederationExecution(
      L"query-ownership-report-requester", L"subscriber", federationName));

  auto const files = serviceReportFiles(directory.path());
  REQUIRE(files.size() == 1U);
  auto const reportFile = files.front();

  REQUIRE_NOTHROW(owner->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(owner->setSendServiceReportsToFileSwitch(false));
  auto const child = owner->getObjectClassHandle(
      L"HLAobjectRoot.UmbraAttributeFixtureBase.UmbraAttributeFixtureChild");
  auto const ownedAttribute = owner->getAttributeHandle(child, L"ReliableBaseA");
  auto const unownedAttribute = owner->getAttributeHandle(child, L"UnownedChild");
  REQUIRE(child.isValid());
  REQUIRE(ownedAttribute.isValid());
  REQUIRE(unownedAttribute.isValid());

  AttributeHandleSet const ownerAttributes{ownedAttribute};
  AttributeHandleSet const requesterSubscriptions{ownedAttribute, unownedAttribute};
  AttributeHandleSet const queriedAttributes{ownedAttribute, unownedAttribute};
  REQUIRE_NOTHROW(requester->subscribeObjectClassAttributes(child, requesterSubscriptions));
  REQUIRE_NOTHROW(owner->publishObjectClassAttributes(child, ownerAttributes));
  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = owner->registerObjectInstance(child));
  REQUIRE_FALSE(requester->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(requesterReports.objectDiscoveryReports.size() == 1U);

  // The prior ordinary subscription is itself a reportable §5.8 service.
  // Capture the report boundary after it, so this test remains focused on the
  // later §7.17 accepted-query record.
  auto const initialText = readTextFile(reportFile);

  // A rejected §7.17 request does not create a successful-void report record.
  ObjectInstanceHandle const unknownObject;
  REQUIRE_THROWS_AS(
      requester->queryAttributeOwnership(unknownObject, queriedAttributes),
      rti1516_2025::ObjectInstanceNotKnown);
  REQUIRE(readTextFile(reportFile) == initialText);

  REQUIRE_NOTHROW(requester->queryAttributeOwnership(objectInstance, queriedAttributes));

  auto const objectInstanceText = objectInstance.toString();
  std::string objectInstanceValue;
  objectInstanceValue.reserve(objectInstanceText.size());
  for (wchar_t const character : objectInstanceText) {
    REQUIRE(character >= L' ');
    REQUIRE(character <= L'~');
    objectInstanceValue.push_back(static_cast<char>(character));
  }
  std::string attributeValues;
  for (auto const& attribute : queriedAttributes) {
    auto const attributeText = attribute.toString();
    std::string attributeValue;
    attributeValue.reserve(attributeText.size());
    for (wchar_t const character : attributeText) {
      REQUIRE(character >= L' ');
      REQUIRE(character <= L'~');
      attributeValue.push_back(static_cast<char>(character));
    }
    if (!attributeValues.empty()) {
      attributeValues += ',';
    }
    attributeValues += '"';
    attributeValues += attributeValue;
    attributeValues += '"';
  }
  auto const expectedRecord = std::string{
      R"({"HLAserialNumber":2,"HLAreturnedArgument":[null],"HLAservice":"QueryAttributeOwnership","HLAsuppliedArguments":[{"HLAargumentType":37,"HLAargumentName":"Object instance designator","HLAargumentValue":")" +
      objectInstanceValue +
      R"("},{"HLAargumentType":1,"HLAargumentName":"Set of attribute designators","HLAargumentValue":[)" +
      attributeValues +
      R"(]}],"HLAsuccessIndicator":true,"HLAexception":null})"};
  // The service record is emitted on accepted §7.17 invocation; the grouped
  // owner and unowned responses remain later §7.18 callbacks.
  REQUIRE(readTextFile(reportFile) == initialText + expectedRecord);
  REQUIRE(requesterReports.attributeOwnershipReports.empty());
  REQUIRE(requester->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE_FALSE(requester->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(requesterReports.attributeOwnershipReports.size() == 2U);
  REQUIRE(readTextFile(reportFile) == initialText + expectedRecord);

  REQUIRE_NOTHROW(owner->resignFederationExecution(CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(requester->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(owner->disconnect());
  REQUIRE_NOTHROW(requester->disconnect());
}
} // namespace
