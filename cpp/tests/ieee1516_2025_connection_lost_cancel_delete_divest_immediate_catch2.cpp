#include "ieee1516_2025_connection_loss_test_support.hpp"

namespace {
TEST_CASE(
    "Immediate callbacks apply the configured automatic cancel-then-delete-then-divest directive synchronously",
    "[integration][development-profile][federation-management][transport]"
    "[ownership-management][object-management]"
    "[connection-lost-automatic-cancel-delete-divest]"
    "[connection-lost-automatic-cancel-delete-divest-immediate]"
    "[standalone][2025][callback-model][immediate]"
    "[rti.service.connection-lost]"
    "[rti.service.get-automatic-resign-directive]"
    "[rti.service.set-automatic-resign-directive]"
    "[rti.service.attribute-ownership-acquisition]"
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

  // Keep the current owner evoked so the pending acquisition release remains
  // queued when the HLA_IMMEDIATE lost member fails.
  REQUIRE_NOTHROW(lost->connect(lostReports, rti1516_2025::HLA_IMMEDIATE));
  REQUIRE_NOTHROW(surviving->connect(survivingReports, HLA_EVOKED));
  REQUIRE_NOTHROW(lost->createFederationExecution(
      federationName,
      fomModule,
      standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(lost->joinFederationExecution(
      L"automatic-combined-immediate-lost",
      L"publisher",
      federationName));
  REQUIRE_NOTHROW(surviving->joinFederationExecution(
      L"automatic-combined-immediate-survivor",
      L"subscriber",
      federationName));

  auto const server = lost->getObjectClassHandle(fixture_hla::fom::employee_server);
  auto const efficiency =
      lost->getAttributeHandle(server, fixture_hla::fixture::efficiency);
  AttributeHandleSet const efficiencyOnly{efficiency};
  REQUIRE_NOTHROW(lost->publishObjectClassAttributes(server, efficiencyOnly));
  REQUIRE_NOTHROW(lost->subscribeObjectClassAttributes(server, efficiencyOnly));
  REQUIRE_NOTHROW(surviving->publishObjectClassAttributes(server, efficiencyOnly));
  REQUIRE_NOTHROW(surviving->subscribeObjectClassAttributes(server, efficiencyOnly));
  survivingReports.recordReceiveOrderObjectRemovalInCallbackOrder = true;
  survivingReports.onRequestAttributeOwnershipAssumption = [&survivingReports]() {
    survivingReports.callbackOrder.push_back("assumption");
  };

  ObjectInstanceHandle retainedObject;
  REQUIRE_NOTHROW(retainedObject = surviving->registerObjectInstance(server));
  auto const retainedObjectName = surviving->getObjectInstanceName(retainedObject);
  REQUIRE(lostReports.objectDiscoveryReports.size() == 1);
  unsigned char const divestitureTagBytes[] = {0xD8, 0x25};
  VariableLengthData const divestitureTag(divestitureTagBytes, sizeof(divestitureTagBytes));
  REQUIRE_NOTHROW(surviving->unconditionalAttributeOwnershipDivestiture(
      retainedObject,
      efficiencyOnly,
      divestitureTag));
  REQUIRE(lostReports.attributeOwnershipAssumptionReports.size() == 1);
  unsigned char const retainedAcquisitionTagBytes[] = {0xD9, 0x25};
  VariableLengthData const retainedAcquisitionTag(
      retainedAcquisitionTagBytes,
      sizeof(retainedAcquisitionTagBytes));
  REQUIRE_NOTHROW(lost->attributeOwnershipAcquisitionIfAvailable(
      retainedObject,
      efficiencyOnly,
      retainedAcquisitionTag));
  REQUIRE(lostReports.attributeOwnershipAcquisitionReports.size() == 1);
  REQUIRE(lost->isAttributeOwnedByFederate(retainedObject, efficiency));

  ObjectInstanceHandle deletedObject;
  REQUIRE_NOTHROW(deletedObject = lost->registerObjectInstance(server));
  auto const deletedObjectName = lost->getObjectInstanceName(deletedObject);
  while (surviving->evokeCallback(0.0)) {
  }
  REQUIRE(survivingReports.objectDiscoveryReports.size() == 1);
  REQUIRE(survivingReports.objectDiscoveryReports.front().objectInstance ==
          deletedObject);

  // Leave a regular acquisition pending at the evoked owner. Directive 5
  // must cancel its stale release before the owner services that queue.
  ObjectInstanceHandle acquisitionTarget;
  REQUIRE_NOTHROW(acquisitionTarget = surviving->registerObjectInstance(server));
  REQUIRE(lostReports.objectDiscoveryReports.size() == 2);
  unsigned char const pendingAcquisitionTagBytes[] = {0xDA, 0x25};
  VariableLengthData const pendingAcquisitionTag(
      pendingAcquisitionTagBytes,
      sizeof(pendingAcquisitionTagBytes));
  REQUIRE_NOTHROW(lost->attributeOwnershipAcquisition(
      acquisitionTarget,
      efficiencyOnly,
      pendingAcquisitionTag));
  REQUIRE(survivingReports.attributeOwnershipReleaseRequestReports.empty());

  REQUIRE_NOTHROW(lost->setAutomaticResignDirective(
      rti1516_2025::CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE(lost->getAutomaticResignDirective() ==
          rti1516_2025::CANCEL_THEN_DELETE_THEN_DIVEST);

  survivingReports.callbackOrder.clear();
  survivingReports.attributeOwnershipAssumptionReports.clear();
  survivingReports.objectRemovalReports.clear();
  REQUIRE(survivingReports.callbackOrder.empty());
  REQUIRE(umbra::detail::failEmbeddedTransportConnectionForTesting(
      *lost,
      L"automatic cancel-delete-divest immediate transport fault"));
  REQUIRE(lostReports.faultDescriptions == std::vector<std::wstring>{
      L"automatic cancel-delete-divest immediate transport fault"});
  REQUIRE(survivingReports.objectRemovalReports.empty());
  REQUIRE(survivingReports.attributeOwnershipAssumptionReports.empty());

  while (surviving->evokeCallback(0.0)) {
  }
  REQUIRE(survivingReports.attributeOwnershipReleaseRequestReports.empty());
  REQUIRE(survivingReports.objectRemovalReports.size() == 1);
  REQUIRE(survivingReports.objectRemovalReports.front().objectInstance ==
          deletedObject);
  REQUIRE(survivingReports.attributeOwnershipAssumptionReports.size() == 1);
  auto const& assumption =
      survivingReports.attributeOwnershipAssumptionReports.front();
  REQUIRE(assumption.objectInstance == retainedObject);
  REQUIRE(assumption.attributes == efficiencyOnly);
  REQUIRE(survivingReports.callbackOrder ==
          std::vector<std::string>{"assumption", "remove"});
  REQUIRE(surviving->getObjectInstanceHandle(retainedObjectName) == retainedObject);
  REQUIRE_THROWS_AS(
      surviving->getObjectInstanceHandle(deletedObjectName),
      rti1516_2025::ObjectInstanceNotKnown);
  REQUIRE_FALSE(surviving->isAttributeOwnedByFederate(retainedObject, efficiency));

  REQUIRE_NOTHROW(surviving->resignFederationExecution(
      rti1516_2025::CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(surviving->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(lost->connect(lostReports, rti1516_2025::HLA_IMMEDIATE));
  REQUIRE_NOTHROW(lost->disconnect());
  REQUIRE_NOTHROW(surviving->disconnect());
}
}  // namespace
