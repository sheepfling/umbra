#include "ieee1516_2025_federation_management_fixture_support.hpp"

namespace {

TEST_CASE(
    "Embedded transport loss applies the configured automatic delete-then-divest directive",
    "[integration][development-profile][federation-management][transport]"
    "[ownership-management][object-management][rti.service.connection-lost]"
    "[rti.service.get-automatic-resign-directive]"
    "[rti.service.set-automatic-resign-directive]"
    "[federate.callback.connection-lost]"
    "[federate.callback.remove-object-instance]"
    "[federate.callback.request-attribute-ownership-assumption]"
    "[multi-federate-callback-ordering]") {
  ReportingFederateAmbassador lostReports;
  ReportingFederateAmbassador survivingReports;
  auto lost = makeRti();
  auto surviving = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();

  REQUIRE_NOTHROW(lost->connect(lostReports, HLA_EVOKED));
  REQUIRE_NOTHROW(surviving->connect(survivingReports, HLA_EVOKED));
  REQUIRE_NOTHROW(lost->createFederationExecution(
      federationName,
      fomModule,
      standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(lost->joinFederationExecution(
      L"automatic-delete-then-divest-lost",
      L"publisher",
      federationName));
  REQUIRE_NOTHROW(surviving->joinFederationExecution(
      L"automatic-delete-then-divest-survivor",
      L"subscriber",
      federationName));

  auto const server = lost->getObjectClassHandle(fixture_hla::fom::employee_server);
  auto const efficiency = lost->getAttributeHandle(server, fixture_hla::fixture::efficiency);
  AttributeHandleSet const efficiencyOnly{efficiency};
  REQUIRE_NOTHROW(lost->publishObjectClassAttributes(server, efficiencyOnly));
  REQUIRE_NOTHROW(lost->subscribeObjectClassAttributes(server, efficiencyOnly));
  REQUIRE_NOTHROW(surviving->publishObjectClassAttributes(server, efficiencyOnly));
  REQUIRE_NOTHROW(surviving->subscribeObjectClassAttributes(server, efficiencyOnly));
  survivingReports.recordReceiveOrderObjectRemovalInCallbackOrder = true;
  survivingReports.onRequestAttributeOwnershipAssumption = [&survivingReports]() {
    survivingReports.callbackOrder.push_back("assumption");
  };

  // The survivor owns this first object and deliberately transfers only its
  // Efficiency attribute to the federate that will be lost. It stays alive
  // after directive 4 because the lost federate never owns its delete
  // privilege.
  ObjectInstanceHandle retainedObject;
  REQUIRE_NOTHROW(retainedObject = surviving->registerObjectInstance(server));
  auto const retainedObjectName = surviving->getObjectInstanceName(retainedObject);
  while (lost->evokeCallback(0.0)) {
  }
  REQUIRE(lostReports.objectDiscoveryReports.size() == 1);
  unsigned char const manualDivestitureTagBytes[] = {0xD4, 0x25};
  VariableLengthData const manualDivestitureTag(
      manualDivestitureTagBytes,
      sizeof(manualDivestitureTagBytes));
  REQUIRE_NOTHROW(surviving->unconditionalAttributeOwnershipDivestiture(
      retainedObject,
      efficiencyOnly,
      manualDivestitureTag));
  while (lost->evokeCallback(0.0)) {
  }
  REQUIRE(lostReports.attributeOwnershipAssumptionReports.size() == 1);
  REQUIRE(lostReports.attributeOwnershipAssumptionReports.front().objectInstance == retainedObject);
  REQUIRE(lostReports.attributeOwnershipAssumptionReports.front().attributes == efficiencyOnly);

  unsigned char const acquisitionTagBytes[] = {0xD5, 0x25};
  VariableLengthData const acquisitionTag(acquisitionTagBytes, sizeof(acquisitionTagBytes));
  REQUIRE_NOTHROW(lost->attributeOwnershipAcquisitionIfAvailable(
      retainedObject,
      efficiencyOnly,
      acquisitionTag));
  while (lost->evokeCallback(0.0)) {
  }
  REQUIRE(lostReports.attributeOwnershipAcquisitionReports.size() == 1);
  REQUIRE(lost->isAttributeOwnedByFederate(retainedObject, efficiency));

  // The lost federate owns delete privilege for this separately registered
  // object. Directive 4 must delete it before divesting the transferred
  // attribute on retainedObject.
  ObjectInstanceHandle deletedObject;
  REQUIRE_NOTHROW(deletedObject = lost->registerObjectInstance(server));
  auto const deletedObjectName = lost->getObjectInstanceName(deletedObject);
  while (surviving->evokeCallback(0.0)) {
  }
  REQUIRE(survivingReports.objectDiscoveryReports.size() == 1);
  REQUIRE(survivingReports.objectDiscoveryReports.front().objectInstance == deletedObject);

  REQUIRE_NOTHROW(lost->setAutomaticResignDirective(
      rti1516_2025::DELETE_OBJECTS_THEN_DIVEST));
  REQUIRE(
      lost->getAutomaticResignDirective() ==
      rti1516_2025::DELETE_OBJECTS_THEN_DIVEST);

  survivingReports.callbackOrder.clear();
  REQUIRE(survivingReports.callbackOrder.empty());
  REQUIRE(umbra::detail::failEmbeddedTransportConnectionForTesting(
      *lost,
      L"automatic delete-then-divest transport fault"));
  REQUIRE(lostReports.faultDescriptions.empty());
  REQUIRE(survivingReports.objectRemovalReports.empty());
  REQUIRE(survivingReports.attributeOwnershipAssumptionReports.empty());

  REQUIRE_FALSE(lost->evokeCallback(0.0));
  REQUIRE(lostReports.faultDescriptions == std::vector<std::wstring>{
      L"automatic delete-then-divest transport fault"});
  while (surviving->evokeCallback(0.0)) {
  }
  REQUIRE(survivingReports.objectRemovalReports.size() == 1);
  REQUIRE(survivingReports.objectRemovalReports.front().objectInstance == deletedObject);
  REQUIRE(survivingReports.attributeOwnershipAssumptionReports.size() == 1);
  auto const& assumption = survivingReports.attributeOwnershipAssumptionReports.front();
  REQUIRE(assumption.objectInstance == retainedObject);
  REQUIRE(assumption.attributes == efficiencyOnly);
  // Keep the embedded recipient-local dispatch sequence explicit as
  // implementation regression coverage, not as a cross-service ordering
  // claim from the standard.
  REQUIRE(survivingReports.callbackOrder ==
          std::vector<std::string>{"assumption", "remove"});
  REQUIRE(surviving->getObjectInstanceHandle(retainedObjectName) == retainedObject);
  REQUIRE_THROWS_AS(
      surviving->getObjectInstanceHandle(deletedObjectName),
      rti1516_2025::ObjectInstanceNotKnown);
  REQUIRE_FALSE(surviving->isAttributeOwnedByFederate(retainedObject, efficiency));

  REQUIRE_NOTHROW(surviving->resignFederationExecution(rti1516_2025::DELETE_OBJECTS));
  REQUIRE_NOTHROW(surviving->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(lost->connect(lostReports, HLA_EVOKED));
  REQUIRE_NOTHROW(lost->disconnect());
  REQUIRE_NOTHROW(surviving->disconnect());
}

} // namespace
