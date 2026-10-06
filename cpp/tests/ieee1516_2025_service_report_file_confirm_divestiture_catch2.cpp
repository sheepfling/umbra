#include "ieee1516_2025_federation_management_fixture_support.hpp"

namespace {
TEST_CASE(
    "Embedded service reporting preserves Confirm Divestiture arguments",
    "[integration][development-profile][federation-management][ownership-management]"
    "[mom][service-report-file][service-reporting][confirm-divestiture]"
    "[confirm-divestiture-mixed-set-atomicity][2025]"
    "[rti.service.negotiated-attribute-ownership-divestiture]"
    "[rti.service.confirm-divestiture]"
    "[rti.service.attribute-ownership-acquisition]"
    "[federate.callback.request-divestiture-confirmation]"
    "[federate.callback.attribute-ownership-acquisition-notification]") {
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
  unsigned char const acquisitionTagBytes[] = {0x21U, 0x22U, 0x23U};
  unsigned char const divestitureTagBytes[] = {0x31U, 0x32U, 0x33U};
  unsigned char const confirmationTagBytes[] = {0x00U, 0xffU, 0x10U, 0xa5U};
  VariableLengthData const acquisitionTag(acquisitionTagBytes, sizeof(acquisitionTagBytes));
  VariableLengthData const divestitureTag(divestitureTagBytes, sizeof(divestitureTagBytes));
  VariableLengthData const confirmationTag(confirmationTagBytes, sizeof(confirmationTagBytes));

  REQUIRE_NOTHROW(owner->connect(ownerReports, HLA_EVOKED, configuration));
  REQUIRE_NOTHROW(requester->connect(requesterReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      owner->createFederationExecution(federationName, fomModules, L"HLAinteger64Time"));
  REQUIRE_NOTHROW(owner->joinFederationExecution(L"confirm-report-owner", L"publisher", federationName));
  REQUIRE_NOTHROW(requester->joinFederationExecution(
      L"confirm-report-requester", L"publisher", federationName));
  REQUIRE_NOTHROW(owner->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(owner->setSendServiceReportsToFileSwitch(false));

  auto const child = owner->getObjectClassHandle(
      L"HLAobjectRoot.UmbraAttributeFixtureBase.UmbraAttributeFixtureChild");
  auto const attribute = owner->getAttributeHandle(child, L"ReliableBaseA");
  auto const unrequestedAttribute = owner->getAttributeHandle(child, L"ReliableBaseB");
  REQUIRE(child.isValid());
  REQUIRE(attribute.isValid());
  REQUIRE(unrequestedAttribute.isValid());
  AttributeHandleSet const attributes{attribute};
  AttributeHandleSet const publishedAttributes{attribute, unrequestedAttribute};
  AttributeHandleSet const mixedConfirmationAttributes{
      attribute,
      unrequestedAttribute,
  };
  REQUIRE_NOTHROW(requester->subscribeObjectClassAttributes(child, attributes));
  REQUIRE_NOTHROW(owner->publishObjectClassAttributes(child, publishedAttributes));
  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = owner->registerObjectInstance(child));
  REQUIRE_FALSE(requester->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(requesterReports.objectDiscoveryReports.size() == 1U);
  REQUIRE_NOTHROW(requester->publishObjectClassAttributes(child, attributes));
  REQUIRE_NOTHROW(requester->attributeOwnershipAcquisition(
      objectInstance,
      attributes,
      acquisitionTag));
  REQUIRE_NOTHROW(owner->negotiatedAttributeOwnershipDivestiture(
      objectInstance,
      attributes,
      divestitureTag));
  while (owner->evokeMultipleCallbacks(0.0, 0.0)) {
  }
  REQUIRE(ownerReports.divestitureConfirmationReports.size() == 1U);

  auto const files = serviceReportFiles(directory.path());
  REQUIRE(files.size() == 1U);
  auto const reportFile = files.front();
  auto const initialText = readTextFile(reportFile);
  REQUIRE_NOTHROW(owner->setServiceReportingSwitch(true));
  REQUIRE_NOTHROW(owner->setSendServiceReportsToFileSwitch(true));
  REQUIRE(readTextFile(reportFile) == initialText);

  ObjectInstanceHandle const unknownObject;
  REQUIRE_THROWS_AS(
      owner->confirmDivestiture(unknownObject, attributes, confirmationTag),
      rti1516_2025::ObjectInstanceNotKnown);
  REQUIRE(readTextFile(reportFile) == initialText);

  // Confirming one valid pending attribute together with a second attribute
  // that was never included in negotiated divestiture must reject the set as
  // a whole; neither ownership nor the successful-void report may partially
  // change.
  REQUIRE_THROWS_AS(
      owner->confirmDivestiture(
          objectInstance,
          mixedConfirmationAttributes,
          confirmationTag),
      rti1516_2025::AttributeDivestitureWasNotRequested);
  REQUIRE(owner->isAttributeOwnedByFederate(objectInstance, attribute));
  REQUIRE_FALSE(requester->isAttributeOwnedByFederate(objectInstance, attribute));
  REQUIRE(owner->isAttributeOwnedByFederate(objectInstance, unrequestedAttribute));
  REQUIRE_FALSE(requester->isAttributeOwnedByFederate(
      objectInstance,
      unrequestedAttribute));
  REQUIRE(readTextFile(reportFile) == initialText);

  REQUIRE_NOTHROW(owner->confirmDivestiture(objectInstance, attributes, confirmationTag));
  REQUIRE_FALSE(owner->isAttributeOwnedByFederate(objectInstance, attribute));
  REQUIRE(requester->isAttributeOwnedByFederate(objectInstance, attribute));

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
  auto const expectedRecord = std::string{
      R"({"HLAserialNumber":0,"HLAreturnedArgument":[null],"HLAservice":"ConfirmDivestiture","HLAsuppliedArguments":[{"HLAargumentType":37,"HLAargumentName":"Object instance designator","HLAargumentValue":")" +
      asAscii(objectInstance.toString()) +
      R"("},{"HLAargumentType":1,"HLAargumentName":"Set of attribute designators","HLAargumentValue":[")" +
      asAscii(attribute.toString()) +
      R"("]},{"HLAargumentType":63,"HLAargumentName":"User-supplied tag","HLAargumentValue":"AP8QpQ=="}],"HLAsuccessIndicator":true,"HLAexception":null})"};
  REQUIRE(readTextFile(reportFile) == initialText + expectedRecord);
  REQUIRE(requesterReports.attributeOwnershipAcquisitionReports.empty());

  while (requester->evokeMultipleCallbacks(0.0, 0.0)) {
  }
  REQUIRE(requesterReports.attributeOwnershipAcquisitionReports.size() == 1U);
  auto const& notification = requesterReports.attributeOwnershipAcquisitionReports.front();
  REQUIRE(notification.kind ==
          ReportingFederateAmbassador::AttributeOwnershipAcquisitionReport::Kind::notification);
  REQUIRE(notification.objectInstance == objectInstance);
  REQUIRE(notification.attributes == attributes);
  REQUIRE(variableLengthDataBytes(notification.userSuppliedTag) ==
          std::vector<unsigned char>(confirmationTagBytes,
                                     confirmationTagBytes + sizeof(confirmationTagBytes)));
  REQUIRE(readTextFile(reportFile) == initialText + expectedRecord);

  REQUIRE_NOTHROW(requester->unpublishObjectClassAttributes(child, attributes));
  REQUIRE_NOTHROW(owner->resignFederationExecution(CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(requester->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(owner->disconnect());
  REQUIRE_NOTHROW(requester->disconnect());
}
}
