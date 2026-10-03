#include "ieee1516_2025_federation_management_test_support.hpp"

namespace {
std::wstring nextFederationName() {
  static std::atomic_uint64_t sequence{0U};
  return L"connection-lost-cancel-delete-divest-" +
      std::to_wstring(sequence.fetch_add(1U, std::memory_order_relaxed));
}

std::filesystem::path resourcePath(std::filesystem::path const& relativePath) {
  return std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "third_party" /
      "ieee1516.2-2025" / "resources" / relativePath;
}

TEST_CASE(
    "Embedded transport loss applies the configured automatic cancel-then-delete-then-divest directive",
    "[integration][development-profile][federation-management][transport]"
    "[ownership-management][object-management][rti.service.connection-lost]"
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

  REQUIRE_NOTHROW(lost->connect(lostReports, HLA_EVOKED));
  REQUIRE_NOTHROW(surviving->connect(survivingReports, HLA_EVOKED));
  REQUIRE_NOTHROW(lost->createFederationExecution(
      federationName,
      fomModule,
      standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(lost->joinFederationExecution(
      L"automatic-combined-lost",
      L"publisher",
      federationName));
  REQUIRE_NOTHROW(surviving->joinFederationExecution(
      L"automatic-combined-survivor",
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

  // The survivor transfers one non-delete attribute to the federate that will
  // be lost. Directive 5 must later offer it back only after cancellation and
  // deletion have been handled.
  ObjectInstanceHandle retainedObject;
  REQUIRE_NOTHROW(retainedObject = surviving->registerObjectInstance(server));
  auto const retainedObjectName = surviving->getObjectInstanceName(retainedObject);
  while (lost->evokeCallback(0.0)) {
  }
  REQUIRE(lostReports.objectDiscoveryReports.size() == 1);
  unsigned char const divestitureTagBytes[] = {0xD8, 0x25};
  VariableLengthData const divestitureTag(divestitureTagBytes, sizeof(divestitureTagBytes));
  REQUIRE_NOTHROW(surviving->unconditionalAttributeOwnershipDivestiture(
      retainedObject,
      efficiencyOnly,
      divestitureTag));
  while (lost->evokeCallback(0.0)) {
  }
  REQUIRE(lostReports.attributeOwnershipAssumptionReports.size() == 1);
  unsigned char const retainedAcquisitionTagBytes[] = {0xD9, 0x25};
  VariableLengthData const retainedAcquisitionTag(
      retainedAcquisitionTagBytes,
      sizeof(retainedAcquisitionTagBytes));
  REQUIRE_NOTHROW(lost->attributeOwnershipAcquisitionIfAvailable(
      retainedObject,
      efficiencyOnly,
      retainedAcquisitionTag));
  while (lost->evokeCallback(0.0)) {
  }
  REQUIRE(lostReports.attributeOwnershipAcquisitionReports.size() == 1);
  REQUIRE(lost->isAttributeOwnedByFederate(retainedObject, efficiency));

  // The lost federate owns delete privilege for this final object, so the
  // deletion phase has an observable Remove Object Instance callback.
  ObjectInstanceHandle deletedObject;
  REQUIRE_NOTHROW(deletedObject = lost->registerObjectInstance(server));
  auto const deletedObjectName = lost->getObjectInstanceName(deletedObject);
  while (surviving->evokeCallback(0.0)) {
  }
  REQUIRE(survivingReports.objectDiscoveryReports.size() == 1);
  REQUIRE(survivingReports.objectDiscoveryReports.front().objectInstance == deletedObject);

  // The pending regular acquisition queues an owner-release callback at the
  // survivor. Directive 5's cancellation phase must make that queued work
  // stale before delivery, rather than leave it aimed at the departed member.
  ObjectInstanceHandle acquisitionTarget;
  REQUIRE_NOTHROW(acquisitionTarget = surviving->registerObjectInstance(server));
  while (lost->evokeCallback(0.0)) {
  }
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
  REQUIRE(
      lost->getAutomaticResignDirective() ==
      rti1516_2025::CANCEL_THEN_DELETE_THEN_DIVEST);

  survivingReports.callbackOrder.clear();
  REQUIRE(survivingReports.callbackOrder.empty());
  REQUIRE(umbra::detail::failEmbeddedTransportConnectionForTesting(
      *lost,
      L"automatic cancel-delete-divest transport fault"));
  REQUIRE(lostReports.faultDescriptions.empty());
  REQUIRE(survivingReports.objectRemovalReports.empty());
  REQUIRE(survivingReports.attributeOwnershipAssumptionReports.empty());

  REQUIRE_FALSE(lost->evokeCallback(0.0));
  REQUIRE(lostReports.faultDescriptions == std::vector<std::wstring>{
      L"automatic cancel-delete-divest transport fault"});
  while (surviving->evokeCallback(0.0)) {
  }
  REQUIRE(survivingReports.attributeOwnershipReleaseRequestReports.empty());
  REQUIRE(survivingReports.objectRemovalReports.size() == 1);
  REQUIRE(survivingReports.objectRemovalReports.front().objectInstance == deletedObject);
  REQUIRE(survivingReports.attributeOwnershipAssumptionReports.size() == 1);
  auto const& assumption = survivingReports.attributeOwnershipAssumptionReports.front();
  REQUIRE(assumption.objectInstance == retainedObject);
  REQUIRE(assumption.attributes == efficiencyOnly);
  REQUIRE(survivingReports.callbackOrder ==
          std::vector<std::string>{"assumption", "remove"});
  REQUIRE(surviving->getObjectInstanceHandle(retainedObjectName) == retainedObject);
  REQUIRE_THROWS_AS(
      surviving->getObjectInstanceHandle(deletedObjectName),
      rti1516_2025::ObjectInstanceNotKnown);
  REQUIRE_FALSE(surviving->isAttributeOwnedByFederate(retainedObject, efficiency));

  REQUIRE_NOTHROW(
      surviving->resignFederationExecution(rti1516_2025::CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(surviving->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(lost->connect(lostReports, HLA_EVOKED));
  REQUIRE_NOTHROW(lost->disconnect());
  REQUIRE_NOTHROW(surviving->disconnect());
}
}  // namespace
