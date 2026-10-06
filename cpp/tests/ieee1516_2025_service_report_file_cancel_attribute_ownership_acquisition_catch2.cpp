#include "ieee1516_2025_federation_management_fixture_support.hpp"

namespace {
TEST_CASE(
    "Embedded service reporting preserves Cancel Attribute Ownership Acquisition arguments",
    "[integration][development-profile][federation-management][ownership-management]"
    "[mom][service-report-file][service-reporting]"
    "[cancel-attribute-ownership-acquisition]"
    "[rti.service.attribute-ownership-acquisition]"
    "[rti.service.cancel-attribute-ownership-acquisition]"
    "[federate.callback.confirm-attribute-ownership-acquisition-cancellation]") {
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
      owner->joinFederationExecution(L"cancellation-report-owner", L"publisher", federationName));
  REQUIRE_NOTHROW(requester->joinFederationExecution(
      L"cancellation-report-requester",
      L"publisher",
      federationName));
  // This support FOM enables both reporting switches by default. Keep setup
  // outside the one-record assertion, then select file reporting only for the
  // accepted Section 7.15 cancellation below.
  REQUIRE_NOTHROW(requester->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(requester->setSendServiceReportsToFileSwitch(false));

  auto const child = owner->getObjectClassHandle(
      L"HLAobjectRoot.UmbraAttributeFixtureBase.UmbraAttributeFixtureChild");
  auto const attribute = owner->getAttributeHandle(child, L"ReliableBaseA");
  REQUIRE(child.isValid());
  REQUIRE(attribute.isValid());
  AttributeHandleSet const attributes{attribute};
  REQUIRE_NOTHROW(requester->subscribeObjectClassAttributes(child, attributes));
  REQUIRE_NOTHROW(owner->publishObjectClassAttributes(child, attributes));
  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = owner->registerObjectInstance(child));
  REQUIRE_FALSE(requester->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(requesterReports.objectDiscoveryReports.size() == 1U);
  REQUIRE_NOTHROW(requester->publishObjectClassAttributes(child, attributes));

  auto const files = serviceReportFiles(directory.path());
  REQUIRE(files.size() == 1U);
  auto const reportFile = files.front();
  auto const initialText = readTextFile(reportFile);
  unsigned char const acquisitionTagBytes[] = {0xC0U, 0xDEU, 0x25U};
  VariableLengthData const acquisitionTag(acquisitionTagBytes, sizeof(acquisitionTagBytes));
  REQUIRE_NOTHROW(
      requester->attributeOwnershipAcquisition(objectInstance, attributes, acquisitionTag));
  REQUIRE(readTextFile(reportFile) == initialText);

  REQUIRE_NOTHROW(requester->setServiceReportingSwitch(true));
  REQUIRE_NOTHROW(requester->setSendServiceReportsToFileSwitch(true));
  REQUIRE(readTextFile(reportFile) == initialText);

  // A rejected cancellation does not reserve a serial or append a
  // successful-void record.
  ObjectInstanceHandle const unknownObject;
  REQUIRE_THROWS_AS(
      requester->cancelAttributeOwnershipAcquisition(unknownObject, attributes),
      rti1516_2025::ObjectInstanceNotKnown);
  REQUIRE(readTextFile(reportFile) == initialText);

  REQUIRE_NOTHROW(requester->cancelAttributeOwnershipAcquisition(objectInstance, attributes));

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
  auto const objectInstanceValue = asAscii(objectInstance.toString());
  auto const attributeValue = asAscii(attribute.toString());
  auto const expectedRecord = std::string{
      R"({"HLAserialNumber":0,"HLAreturnedArgument":[null],"HLAservice":"CancelAttributeOwnershipAcquisition","HLAsuppliedArguments":[{"HLAargumentType":37,"HLAargumentName":"Object instance designator","HLAargumentValue":")" +
      objectInstanceValue +
      R"("},{"HLAargumentType":1,"HLAargumentName":"Set of attribute designators","HLAargumentValue":[")" +
      attributeValue +
      R"("]}],"HLAsuccessIndicator":true,"HLAexception":null})"};
  // The accepted §7.15 cancellation is reported before its separately queued
  // confirmation callback.  Its supplied one-element set remains Table 5's
  // type-1 AttributeHandleSet form.
  REQUIRE(readTextFile(reportFile) == initialText + expectedRecord);
  REQUIRE(requesterReports.attributeOwnershipAcquisitionCancellationReports.empty());

  // The cancellation invalidates the pending owner-side acquisition work,
  // then the requester receives the confirmation after the record is durable.
  while (owner->evokeMultipleCallbacks(0.0, 0.0)) {
  }
  REQUIRE(ownerReports.attributeOwnershipReleaseRequestReports.empty());
  while (requester->evokeMultipleCallbacks(0.0, 0.0)) {
  }
  REQUIRE(requesterReports.attributeOwnershipAcquisitionReports.empty());
  REQUIRE(requesterReports.attributeOwnershipAcquisitionCancellationReports.size() == 1U);
  auto const& confirmation =
      requesterReports.attributeOwnershipAcquisitionCancellationReports.front();
  REQUIRE(confirmation.objectInstance == objectInstance);
  REQUIRE(confirmation.attributes == attributes);
  REQUIRE(readTextFile(reportFile) == initialText + expectedRecord);

  REQUIRE_NOTHROW(owner->resignFederationExecution(CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(requester->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(owner->disconnect());
  REQUIRE_NOTHROW(requester->disconnect());
}
}  // namespace
