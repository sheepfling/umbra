#include "ieee1516_2025_federation_management_fixture_support.hpp"
#include "ieee1516_2025_public_federation_restore_test_support.hpp"

namespace {
TEST_CASE(
    "Embedded joined-federate MOM report-file identity survives save and restore",
    "[integration][development-profile][federation-management][mom][save-restore]"
    "[service-report-file][service-report-file-lifecycle]"
    "[rti.service.request-federation-save][rti.service.federate-save-begun]"
    "[rti.service.federate-save-complete][rti.service.request-federation-restore]"
    "[rti.service.federate-restore-complete]"
    "[rti.service.request-attribute-value-update]"
    "[federate.callback.reflect-attribute-values]") {
  using rti1516_2025::umbra_binding_detail::UmbraRtiAmbassador;
  using namespace public_federation_restore_test_support;

  ReportingFederateAmbassador subjectReports;
  ReportingFederateAmbassador observerReports;
  auto subject = std::make_unique<UmbraRtiAmbassador>();
  auto observer = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  auto subjectDirectory = temporaryServiceReportDirectory();
  auto observerDirectory = temporaryServiceReportDirectory();
  auto subjectConfiguration = configurationForServiceReportDirectory(subjectDirectory.path());
  auto observerConfiguration = configurationForServiceReportDirectory(observerDirectory.path());
  subjectConfiguration.withRtiAddress(L"in-process");
  observerConfiguration.withRtiAddress(L"in-process");

  REQUIRE_NOTHROW(subject->connect(
      subjectReports, rti1516_2025::HLA_IMMEDIATE, subjectConfiguration));
  REQUIRE_NOTHROW(observer->connect(
      observerReports, rti1516_2025::HLA_IMMEDIATE, observerConfiguration));
  REQUIRE_NOTHROW(subject->createFederationExecution(
      federationName, fomModule, standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(subject->joinFederationExecution(
      L"mom-save-file-subject", L"subject", federationName));
  REQUIRE_NOTHROW(observer->joinFederationExecution(
      L"mom-save-file-observer", L"observer", federationName));

  auto const momClass = observer->getObjectClassHandle(
      standard_hla::mom::federate_object_class);
  auto const reportFileAttribute = observer->getAttributeHandle(
      momClass, standard_hla::mom::report_service_file);
  auto const federateHandleAttribute = observer->getAttributeHandle(
      momClass, standard_hla::mom::federate_handle);
  REQUIRE(momClass.isValid());
  REQUIRE(reportFileAttribute.isValid());
  REQUIRE(federateHandleAttribute.isValid());
  REQUIRE_NOTHROW(observer->subscribeObjectClassAttributes(
      momClass,
      AttributeHandleSet{reportFileAttribute, federateHandleAttribute},
      true));

  auto const subjectFederate = subject->getFederateHandle(L"mom-save-file-subject");
  auto initialSnapshot = subject->joinedFederateMomObjectSnapshotForTesting();
  REQUIRE(initialSnapshot);
  REQUIRE_FALSE(initialSnapshot->reportServiceFile.empty());
  auto const reportFiles = serviceReportFiles(subjectDirectory.path());
  REQUIRE(reportFiles.size() == 1U);
  REQUIRE(initialSnapshot->reportServiceFile == reportFiles.front().wstring());

  auto const reflectedSubject = std::find_if(
      observerReports.attributeReflectionReports.begin(),
      observerReports.attributeReflectionReports.end(),
      [&](ReportingFederateAmbassador::AttributeReflectionReport const& report) {
        auto const handle = report.attributeValues.find(federateHandleAttribute);
        auto const path = report.attributeValues.find(reportFileAttribute);
        return handle != report.attributeValues.end() &&
            path != report.attributeValues.end() &&
            variableLengthDataBytes(handle->second) ==
                variableLengthDataBytes(subjectFederate.encode());
      });
  REQUIRE(reflectedSubject != observerReports.attributeReflectionReports.end());
  auto const subjectObjectInstance = reflectedSubject->objectInstance;
  auto const expectedReportFile =
      rti1516_2025::HLAunicodeString{reportFiles.front().wstring()}.encode();
  REQUIRE(variableLengthDataBytes(
              reflectedSubject->attributeValues.at(reportFileAttribute)) ==
          variableLengthDataBytes(expectedReportFile));

  REQUIRE_NOTHROW(subject->setServiceReportingSwitch(true));
  REQUIRE_NOTHROW(subject->setSendServiceReportsToFileSwitch(true));
  REQUIRE_NOTHROW(subject->getObjectClassHandle(standard_hla::fom::object_root));
  auto const textBeforeSave = readTextFile(reportFiles.front());

  REQUIRE_NOTHROW(subject->requestFederationSave(L"mom-save-file-identity"));
  REQUIRE_NOTHROW(subject->federateSaveBegun());
  REQUIRE_NOTHROW(observer->federateSaveBegun());
  REQUIRE_NOTHROW(subject->federateSaveComplete());
  REQUIRE_NOTHROW(observer->federateSaveComplete());
  REQUIRE_NOTHROW(subject->getObjectClassHandle(standard_hla::fom::object_root));
  auto const textAfterSave = readTextFile(reportFiles.front());
  REQUIRE(textAfterSave.size() > textBeforeSave.size());

  REQUIRE_NOTHROW(subject->requestFederationRestore(L"mom-save-file-identity"));
  REQUIRE_NOTHROW(subject->federateRestoreComplete());
  REQUIRE_NOTHROW(observer->federateRestoreComplete());

  auto const restoredSnapshot = subject->joinedFederateMomObjectSnapshotForTesting();
  REQUIRE(restoredSnapshot);
  REQUIRE(restoredSnapshot->objectInstanceHandle == initialSnapshot->objectInstanceHandle);
  REQUIRE(restoredSnapshot->joinedFederateId == initialSnapshot->joinedFederateId);
  REQUIRE(restoredSnapshot->reportServiceFile == initialSnapshot->reportServiceFile);
  REQUIRE(restoredSnapshot->initialAttributeValues.size() ==
          initialSnapshot->initialAttributeValues.size());
  auto const textAfterRestore = readTextFile(reportFiles.front());
  REQUIRE(textAfterRestore.size() > textAfterSave.size());

  observerReports.attributeReflectionReports.clear();
  REQUIRE_NOTHROW(observer->requestAttributeValueUpdate(
      subjectObjectInstance,
      AttributeHandleSet{reportFileAttribute},
      VariableLengthData{}));
  REQUIRE(observerReports.attributeReflectionReports.size() == 1U);
  auto const& restoredReflection = observerReports.attributeReflectionReports.front();
  REQUIRE(restoredReflection.objectInstance == subjectObjectInstance);
  REQUIRE(variableLengthDataBytes(
              restoredReflection.attributeValues.at(reportFileAttribute)) ==
          variableLengthDataBytes(expectedReportFile));

  REQUIRE_NOTHROW(subject->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(observer->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(subject->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(subject->disconnect());
  REQUIRE_NOTHROW(observer->disconnect());
}

}  // namespace
