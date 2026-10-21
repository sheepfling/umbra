#include "ieee1516_2025_federation_management_fixture_support.hpp"
#include "ieee1516_2025_public_federation_restore_test_support.hpp"

namespace {
TEST_CASE(
    "Embedded service reporting records Unpublish blocked by pending ownership acquisition",
    "[integration][development-profile][federation-management][declaration-management]"
    "[ownership-management][mom][service-report-file][service-reporting][service-failure]"
    "[object-attribute-declaration-failure][ownership-acquisition-pending][2025]"
    "[rti.service.object-attribute-declaration-failure-matrix]"
    "[rti.service.unpublish-object-class-attributes-ownership-pending]") {
  using rti1516_2025::HLA_EVOKED;
  using rti1516_2025::CANCEL_THEN_DELETE_THEN_DIVEST;
  using rti1516_2025::DELETE_OBJECTS;

  rti1516_2025::NullFederateAmbassador ownerFederate;
  ReportingFederateAmbassador requesterReports;
  auto owner = makeRti();
  auto requester = makeRti();
  auto const federationName = nextFederationName();
  auto const restaurantFom = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  auto ownerDirectory = temporaryServiceReportDirectory();
  auto requesterDirectory = temporaryServiceReportDirectory();
  auto ownerConfiguration = configurationForServiceReportDirectory(ownerDirectory.path());
  auto requesterConfiguration =
      configurationForServiceReportDirectory(requesterDirectory.path());
  ownerConfiguration.withRtiAddress(L"in-process");
  requesterConfiguration.withRtiAddress(L"in-process");

  REQUIRE_NOTHROW(owner->connect(ownerFederate, HLA_EVOKED, ownerConfiguration));
  REQUIRE_NOTHROW(requester->connect(requesterReports, HLA_EVOKED, requesterConfiguration));
  REQUIRE_NOTHROW(owner->createFederationExecution(
      federationName,
      restaurantFom,
      standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(owner->joinFederationExecution(
      L"pending-unpublish-owner", L"publisher", federationName));
  REQUIRE_NOTHROW(requester->joinFederationExecution(
      L"pending-unpublish-requester", L"subscriber", federationName));

  auto const objectClass = owner->getObjectClassHandle(fixture_hla::fom::employee_server);
  auto const attribute = owner->getAttributeHandle(objectClass, fixture_hla::fixture::efficiency);
  REQUIRE(objectClass.isValid());
  REQUIRE(attribute.isValid());
  AttributeHandleSet const attributes{attribute};
  REQUIRE_NOTHROW(requester->subscribeObjectClassAttributes(objectClass, attributes));
  REQUIRE_NOTHROW(owner->publishObjectClassAttributes(objectClass, attributes));
  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = owner->registerObjectInstance(objectClass));
  REQUIRE_FALSE(requester->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(requester->getKnownObjectClassHandle(objectInstance) == objectClass);

  // The requester must publish the attribute before acquiring it. Keep its
  // report switches disabled through setup so the failure under test is the
  // first service record appended after the joined-federate initial record.
  REQUIRE_NOTHROW(requester->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(requester->setSendServiceReportsToFileSwitch(false));
  REQUIRE_NOTHROW(requester->publishObjectClassAttributes(objectClass, attributes));
  REQUIRE_NOTHROW(requester->attributeOwnershipAcquisition(
      objectInstance,
      attributes,
      VariableLengthData{}));

  auto const requesterFiles = serviceReportFiles(requesterDirectory.path());
  REQUIRE(requesterFiles.size() == 1U);
  auto const reportFile = requesterFiles.front();
  auto const initialText = readTextFile(reportFile);
  REQUIRE_NOTHROW(requester->setServiceReportingSwitch(true));
  REQUIRE_NOTHROW(requester->setSendServiceReportsToFileSwitch(true));
  REQUIRE_THROWS_AS(
      requester->unpublishObjectClassAttributes(objectClass, attributes),
      rti1516_2025::OwnershipAcquisitionPending);

  auto const reportText = readTextFile(reportFile);
  REQUIRE(serviceReportFiles(requesterDirectory.path()) == requesterFiles);
  REQUIRE(reportText.size() > initialText.size());
  auto const failedRecord = reportText.substr(initialText.size());
  REQUIRE(failedRecord.find("\"HLAserialNumber\":0") != std::string::npos);
  REQUIRE(failedRecord.find("\"HLAreturnedArgument\":[null]") != std::string::npos);
  REQUIRE(failedRecord.find("\"HLAservice\":\"UnpublishObjectClassAttributes\"") !=
          std::string::npos);
  REQUIRE(failedRecord.find("\"HLAargumentType\":36") != std::string::npos);
  REQUIRE(failedRecord.find("\"HLAargumentType\":1") != std::string::npos);
  REQUIRE(failedRecord.find("\"HLAsuccessIndicator\":false") != std::string::npos);
  REQUIRE(failedRecord.find(
              "\"HLAexception\":\"OwnershipAcquisitionPending: Unpublish Object Class Attributes cannot remove a publication required by a pending ownership acquisition.\"") !=
          std::string::npos);

  REQUIRE_NOTHROW(requester->setServiceReportingSwitch(false));
  REQUIRE_NOTHROW(requester->setSendServiceReportsToFileSwitch(false));
  REQUIRE_NOTHROW(requester->resignFederationExecution(CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(owner->resignFederationExecution(DELETE_OBJECTS));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(requester->disconnect());
  REQUIRE_NOTHROW(owner->disconnect());
}

TEST_CASE(
    "Embedded partial unpublication excludes an unrelated pending acquisition",
    "[integration][development-profile][declaration-management]"
    "[ownership-management][selective-unpublication-pending-acquisition][2025]"
    "[rti.service.unpublish-object-class-attributes]"
    "[rti.service.attribute-ownership-acquisition]") {
  rti1516_2025::NullFederateAmbassador ownerFederate;
  ReportingFederateAmbassador requesterReports;
  auto owner = makeRti();
  auto requester = makeRti();
  auto const federationName = nextFederationName();
  auto const restaurantFom =
      resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  auto ownerDirectory = temporaryServiceReportDirectory();
  auto requesterDirectory = temporaryServiceReportDirectory();
  auto ownerConfiguration =
      configurationForServiceReportDirectory(ownerDirectory.path());
  auto requesterConfiguration =
      configurationForServiceReportDirectory(requesterDirectory.path());
  ownerConfiguration.withRtiAddress(L"in-process");
  requesterConfiguration.withRtiAddress(L"in-process");

  REQUIRE_NOTHROW(owner->connect(
      ownerFederate, rti1516_2025::HLA_EVOKED, ownerConfiguration));
  REQUIRE_NOTHROW(requester->connect(
      requesterReports, rti1516_2025::HLA_EVOKED, requesterConfiguration));
  REQUIRE_NOTHROW(owner->createFederationExecution(
      federationName, restaurantFom, standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(owner->joinFederationExecution(
      L"selective-unpublish-owner", L"publisher", federationName));
  REQUIRE_NOTHROW(requester->joinFederationExecution(
      L"selective-unpublish-requester", L"subscriber", federationName));

  auto const objectClass =
      owner->getObjectClassHandle(fixture_hla::fom::employee_server);
  auto const pendingAttribute = owner->getAttributeHandle(
      objectClass, fixture_hla::fixture::efficiency);
  auto const unrelatedAttribute = owner->getAttributeHandle(
      objectClass, fixture_hla::fixture::name);
  REQUIRE(objectClass.isValid());
  REQUIRE(pendingAttribute.isValid());
  REQUIRE(unrelatedAttribute.isValid());
  REQUIRE(pendingAttribute != unrelatedAttribute);
  AttributeHandleSet const pendingOnly{pendingAttribute};
  AttributeHandleSet const unrelatedOnly{unrelatedAttribute};
  AttributeHandleSet const both{pendingAttribute, unrelatedAttribute};
  REQUIRE_NOTHROW(requester->subscribeObjectClassAttributes(objectClass, both));
  REQUIRE_NOTHROW(owner->publishObjectClassAttributes(objectClass, both));
  auto const objectInstance = owner->registerObjectInstance(objectClass);
  REQUIRE(objectInstance.isValid());
  REQUIRE_FALSE(requester->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(requesterReports.objectDiscoveryReports.size() == 1U);
  REQUIRE(requester->getKnownObjectClassHandle(objectInstance) == objectClass);

  REQUIRE_NOTHROW(requester->publishObjectClassAttributes(objectClass, both));
  REQUIRE_NOTHROW(requester->attributeOwnershipAcquisition(
      objectInstance, pendingOnly, VariableLengthData{}));
  REQUIRE_FALSE(requester->isAttributeOwnedByFederate(
      objectInstance, pendingAttribute));
  REQUIRE(owner->isAttributeOwnedByFederate(
      objectInstance, pendingAttribute));
  REQUIRE(owner->isAttributeOwnedByFederate(
      objectInstance, unrelatedAttribute));

  // The acquisition protects only its own class attribute publication.
  REQUIRE_NOTHROW(requester->unpublishObjectClassAttributes(
      objectClass, unrelatedOnly));
  REQUIRE_THROWS_AS(
      requester->attributeOwnershipAcquisition(
          objectInstance, unrelatedOnly, VariableLengthData{}),
      rti1516_2025::AttributeNotPublished);
  REQUIRE_THROWS_AS(
      requester->unpublishObjectClassAttributes(objectClass, pendingOnly),
      rti1516_2025::OwnershipAcquisitionPending);
  REQUIRE_FALSE(requester->isAttributeOwnedByFederate(
      objectInstance, pendingAttribute));
  REQUIRE(owner->isAttributeOwnedByFederate(
      objectInstance, pendingAttribute));

  REQUIRE_NOTHROW(requester->resignFederationExecution(
      rti1516_2025::CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(owner->resignFederationExecution(
      rti1516_2025::DELETE_OBJECTS));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(requester->disconnect());
  REQUIRE_NOTHROW(owner->disconnect());
}

}  // namespace
