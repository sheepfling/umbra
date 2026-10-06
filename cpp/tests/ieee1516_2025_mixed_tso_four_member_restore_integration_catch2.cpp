#include "ieee1516_2025_federation_management_test_support.hpp"
#include "ieee1516_2025_federation_management_fixture_support.hpp"
#include "ieee1516_2025_public_federation_restore_test_support.hpp"

namespace {
TEST_CASE(
    "Embedded federation restore composes a saved timestamped object update and interaction across four members",
    "[integration][development-profile][federation-management][save-restore]"
    "[object-management][interaction-management][time-management][tso]"
    "[mixed-fanout][multi-federate-callback-ordering]"
    "[timestamped-object-update-restore][timestamped-interaction-restore]"
    "[timestamped-object-deletion-restore][tso-object-deletion-state]"
    "[tso-retraction-ledger-state]"
    "[rti.service.request-federation-save][rti.service.request-federation-restore]"
    "[rti.service.federate-save-begun][rti.service.federate-save-complete]"
    "[rti.service.federate-restore-complete][rti.service.publish-object-class-attributes]"
    "[rti.service.subscribe-object-class-attributes][rti.service.publish-interaction-class]"
    "[rti.service.subscribe-interaction-class][rti.service.enable-time-regulation]"
    "[rti.service.enable-time-constrained][rti.service.update-attribute-values]"
    "[rti.service.send-interaction][rti.service.delete-object-instance]"
    "[rti.service.retract][rti.service.flush-queue-request]"
    "[federate.callback.reflect-attribute-values][federate.callback.receive-interaction]"
    "[federate.callback.remove-object-instance]"
    "[federate.callback.request-retraction][federate.callback.flush-queue-grant]"
    "[federate.callback.federation-saved][federate.callback.federation-restored]") {
  ReportingFederateAmbassador ownerReports;
  ReportingFederateAmbassador mirrorReports;
  ReportingFederateAmbassador senderReports;
  ReportingFederateAmbassador observerReports;
  auto owner = makeRti();
  auto mirror = makeRti();
  auto sender = makeRti();
  auto observer = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  std::wstring const objectInstanceName = L"mixed-restore-server";
  std::wstring const saveLabel = L"mixed-object-interaction-baseline";
  unsigned char const attributeBytes[] = {0x4D, 0x49, 0x58, 0x2D, 0x41};
  unsigned char const interactionBytes[] = {0x4D, 0x49, 0x58, 0x2D, 0x49};
  unsigned char const tagBytes[] = {0x4D, 0x49, 0x58, 0x2D, 0x54};
  VariableLengthData const attributeTag(tagBytes, sizeof(tagBytes));
  VariableLengthData const interactionTag(tagBytes, sizeof(tagBytes));

  auto const drain = [](RTIambassador& ambassador) {
    while (ambassador.evokeCallback(0.0)) {
    }
  };

  REQUIRE_NOTHROW(owner->connect(ownerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(mirror->connect(mirrorReports, HLA_EVOKED));
  REQUIRE_NOTHROW(sender->connect(senderReports, HLA_EVOKED));
  REQUIRE_NOTHROW(observer->connect(observerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(owner->createFederationExecution(
      federationName, fomModule, standard_hla::mom::integer64_time));
  FederateHandle ownerHandle;
  FederateHandle senderHandle;
  REQUIRE_NOTHROW(ownerHandle = owner->joinFederationExecution(
                      L"mixed-restore-owner", L"owner", federationName));
  REQUIRE_NOTHROW(mirror->joinFederationExecution(
      L"mixed-restore-mirror", L"mirror", federationName));
  REQUIRE_NOTHROW(senderHandle = sender->joinFederationExecution(
                      L"mixed-restore-sender", L"sender", federationName));
  REQUIRE_NOTHROW(observer->joinFederationExecution(
      L"mixed-restore-observer", L"observer", federationName));

  auto const ownerServer = owner->getObjectClassHandle(fixture_hla::fom::employee_server);
  auto const mirrorServer = mirror->getObjectClassHandle(fixture_hla::fom::employee_server);
  auto const ownerEfficiency = owner->getAttributeHandle(
      ownerServer, fixture_hla::fixture::efficiency);
  auto const ownerDeletePrivilege = owner->getAttributeHandle(
      ownerServer, L"HLAprivilegeToDeleteObject");
  auto const mirrorEfficiency = mirror->getAttributeHandle(
      mirrorServer, fixture_hla::fixture::efficiency);
  auto const senderInteraction = sender->getInteractionClassHandle(
      fixture_hla::fom::main_course_served);
  auto const observerInteraction = observer->getInteractionClassHandle(
      fixture_hla::fom::main_course_served);
  auto const senderTemperature = sender->getParameterHandle(
      senderInteraction, fixture_hla::fixture::temperature_ok);
  auto const observerTemperature = observer->getParameterHandle(
      observerInteraction, fixture_hla::fixture::temperature_ok);
  REQUIRE(ownerServer.isValid());
  REQUIRE(ownerEfficiency.isValid());
  REQUIRE(ownerDeletePrivilege.isValid());
  REQUIRE(senderInteraction.isValid());
  REQUIRE(senderTemperature.isValid());

  REQUIRE_NOTHROW(owner->publishObjectClassAttributes(
      ownerServer, AttributeHandleSet{ownerEfficiency, ownerDeletePrivilege}));
  REQUIRE_NOTHROW(mirror->subscribeObjectClassAttributes(
      mirrorServer, AttributeHandleSet{mirrorEfficiency}));
  REQUIRE_NOTHROW(sender->publishInteractionClass(senderInteraction));
  REQUIRE_NOTHROW(observer->subscribeInteractionClass(observerInteraction));
  REQUIRE_NOTHROW(mirror->enableTimeConstrained());
  REQUIRE_NOTHROW(observer->enableTimeConstrained());
  REQUIRE_FALSE(mirror->evokeCallback(0.0));
  REQUIRE_FALSE(observer->evokeCallback(0.0));
  REQUIRE_NOTHROW(owner->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(1)));
  REQUIRE_NOTHROW(sender->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(1)));
  drain(*owner);
  drain(*sender);

  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(owner->reserveObjectInstanceName(objectInstanceName));
  drain(*owner);
  REQUIRE_NOTHROW(objectInstance = owner->registerObjectInstance(
                      ownerServer, objectInstanceName));
  drain(*mirror);
  REQUIRE(mirrorReports.objectDiscoveryReports.size() == 1U);
  ObjectInstanceHandle mirrorObject;
  REQUIRE_NOTHROW(mirrorObject = mirror->getObjectInstanceHandle(objectInstanceName));
  REQUIRE(mirrorObject == objectInstance);

  AttributeHandleValueMap attributeValues;
  attributeValues.emplace(
      ownerEfficiency,
      VariableLengthData(attributeBytes, sizeof(attributeBytes)));
  ParameterHandleValueMap parameterValues;
  parameterValues.emplace(
      senderTemperature,
      VariableLengthData(interactionBytes, sizeof(interactionBytes)));
  auto const attributeRetraction = owner->updateAttributeValues(
      objectInstance,
      attributeValues,
      attributeTag,
      rti1516_2025::HLAinteger64Time(7));
  auto const interactionRetraction = sender->sendInteraction(
      senderInteraction,
      parameterValues,
      interactionTag,
      rti1516_2025::HLAinteger64Time(7));
  unsigned char const deletionTagBytes[] = {'M', 'I', 'X', '-', 'D'};
  VariableLengthData const deletionTag(deletionTagBytes, sizeof(deletionTagBytes));
  auto const deletionRetraction = owner->deleteObjectInstance(
      objectInstance,
      deletionTag,
      rti1516_2025::HLAinteger64Time(8));
  REQUIRE(attributeRetraction.isValid());
  REQUIRE(interactionRetraction.isValid());
  REQUIRE(deletionRetraction.isValid());
  REQUIRE(mirrorReports.attributeReflectionReports.empty());
  REQUIRE(observerReports.timestampedInteractionReports.empty());

  // Keep both constrained recipients below the saved timestamp while the
  // four-member save captures the object/update and interaction ledgers.
  REQUIRE_NOTHROW(owner->requestFederationSave(saveLabel));
  REQUIRE_NOTHROW(owner->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(1)));
  REQUIRE_NOTHROW(mirror->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(1)));
  REQUIRE_NOTHROW(sender->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(1)));
  REQUIRE_NOTHROW(observer->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(1)));
  drain(*mirror);
  drain(*observer);
  drain(*owner);
  drain(*sender);
  REQUIRE(ownerReports.initiateFederateSaveReports == std::vector<std::wstring>{saveLabel});
  REQUIRE(mirrorReports.initiateFederateSaveReports == std::vector<std::wstring>{saveLabel});
  REQUIRE(senderReports.initiateFederateSaveReports == std::vector<std::wstring>{saveLabel});
  REQUIRE(observerReports.initiateFederateSaveReports == std::vector<std::wstring>{saveLabel});
  REQUIRE_NOTHROW(owner->federateSaveBegun());
  REQUIRE_NOTHROW(mirror->federateSaveBegun());
  REQUIRE_NOTHROW(sender->federateSaveBegun());
  REQUIRE_NOTHROW(observer->federateSaveBegun());
  REQUIRE_NOTHROW(owner->federateSaveComplete());
  REQUIRE_NOTHROW(mirror->federateSaveComplete());
  REQUIRE_NOTHROW(sender->federateSaveComplete());
  REQUIRE_NOTHROW(observer->federateSaveComplete());
  drain(*owner);
  drain(*mirror);
  drain(*sender);
  drain(*observer);
  REQUIRE(ownerReports.federationSavedReportCount == 1U);
  REQUIRE(mirrorReports.federationSavedReportCount == 1U);
  REQUIRE(senderReports.federationSavedReportCount == 1U);
  REQUIRE(observerReports.federationSavedReportCount == 1U);

  // Terminalize all live post-save records. Restore must reconstitute the
  // saved queue entries and their independent retraction ledgers.
  REQUIRE_NOTHROW(owner->retract(attributeRetraction));
  REQUIRE_NOTHROW(sender->retract(interactionRetraction));
  REQUIRE_NOTHROW(owner->retract(deletionRetraction));
  drain(*mirror);
  drain(*observer);
  REQUIRE(mirrorReports.attributeReflectionReports.empty());
  REQUIRE(observerReports.timestampedInteractionReports.empty());
  REQUIRE_THROWS_AS(owner->retract(attributeRetraction), rti1516_2025::MessageCanNoLongerBeRetracted);
  REQUIRE_THROWS_AS(sender->retract(interactionRetraction), rti1516_2025::MessageCanNoLongerBeRetracted);
  REQUIRE_THROWS_AS(owner->retract(deletionRetraction), rti1516_2025::MessageCanNoLongerBeRetracted);

  REQUIRE_NOTHROW(owner->requestFederationRestore(saveLabel));
  drain(*owner);
  drain(*mirror);
  drain(*sender);
  drain(*observer);
  REQUIRE(ownerReports.requestFederationRestoreSucceededReports ==
          std::vector<std::wstring>{saveLabel});
  REQUIRE(mirrorReports.initiateFederateRestoreReports.size() == 1U);
  REQUIRE(observerReports.initiateFederateRestoreReports.size() == 1U);
  REQUIRE_NOTHROW(owner->federateRestoreComplete());
  REQUIRE_NOTHROW(mirror->federateRestoreComplete());
  REQUIRE_NOTHROW(sender->federateRestoreComplete());
  REQUIRE_NOTHROW(observer->federateRestoreComplete());
  drain(*owner);
  drain(*mirror);
  drain(*sender);
  drain(*observer);
  REQUIRE(ownerReports.federationRestoredReportCount == 1U);
  REQUIRE(mirrorReports.federationRestoredReportCount == 1U);
  REQUIRE(senderReports.federationRestoredReportCount == 1U);
  REQUIRE(observerReports.federationRestoredReportCount == 1U);

  mirrorReports.callbackOrder.clear();
  observerReports.callbackOrder.clear();
  REQUIRE_NOTHROW(mirror->flushQueueRequest(rti1516_2025::HLAinteger64Time(10)));
  drain(*mirror);
  REQUIRE_NOTHROW(observer->flushQueueRequest(rti1516_2025::HLAinteger64Time(10)));
  drain(*observer);
  REQUIRE(mirrorReports.attributeReflectionReports.size() == 1U);
  REQUIRE(mirrorReports.objectRemovalReports.size() == 1U);
  REQUIRE(observerReports.timestampedInteractionReports.size() == 1U);
  auto const& reflection = mirrorReports.attributeReflectionReports.front();
  REQUIRE(reflection.objectInstance == mirrorObject);
  REQUIRE(reflection.attributeValues.size() == 1U);
  REQUIRE(reflection.attributeValues.contains(mirrorEfficiency));
  REQUIRE(variableLengthDataBytes(reflection.attributeValues.at(mirrorEfficiency)) ==
          std::vector<unsigned char>(attributeBytes, attributeBytes + sizeof(attributeBytes)));
  REQUIRE(variableLengthDataBytes(reflection.userSuppliedTag) ==
          variableLengthDataBytes(attributeTag));
  REQUIRE(reflection.producingFederate == ownerHandle);
  REQUIRE(reflection.timeValue == L"7");
  REQUIRE(reflection.retractionSupplied);
  REQUIRE(reflection.retractionValid);
  auto const& removal = mirrorReports.objectRemovalReports.front();
  REQUIRE(removal.objectInstance == mirrorObject);
  REQUIRE(variableLengthDataBytes(removal.userSuppliedTag) ==
          variableLengthDataBytes(deletionTag));
  REQUIRE(removal.producingFederate == ownerHandle);
  REQUIRE(removal.timeValue == L"8");
  REQUIRE(removal.sentOrderType == rti1516_2025::TIMESTAMP);
  REQUIRE(removal.receivedOrderType == rti1516_2025::TIMESTAMP);
  REQUIRE(removal.retractionSupplied);
  REQUIRE(removal.retractionValid);
  auto const& interaction = observerReports.timestampedInteractionReports.front();
  REQUIRE(interaction.interactionClass == observerInteraction);
  REQUIRE(interaction.parameterValues.size() == 1U);
  REQUIRE(interaction.parameterValues.contains(observerTemperature));
  REQUIRE(variableLengthDataBytes(interaction.parameterValues.at(observerTemperature)) ==
          std::vector<unsigned char>(interactionBytes, interactionBytes + sizeof(interactionBytes)));
  REQUIRE(variableLengthDataBytes(interaction.userSuppliedTag) ==
          variableLengthDataBytes(interactionTag));
  REQUIRE(interaction.producingFederate == senderHandle);
  REQUIRE(interaction.timeValue == L"7");
  REQUIRE(interaction.retractionSupplied);
  REQUIRE(interaction.retractionValid);
  REQUIRE(mirrorReports.callbackOrder ==
          std::vector<std::string>{"reflect", "remove", "flush-grant"});
  REQUIRE(observerReports.callbackOrder == std::vector<std::string>{"interaction", "flush-grant"});

  REQUIRE_NOTHROW(owner->retract(attributeRetraction));
  REQUIRE_NOTHROW(sender->retract(interactionRetraction));
  REQUIRE_NOTHROW(owner->retract(deletionRetraction));
  drain(*mirror);
  drain(*observer);
  REQUIRE(mirrorReports.requestRetractionReports.size() == 2U);
  REQUIRE(observerReports.requestRetractionReports.size() == 1U);
  REQUIRE(mirrorReports.requestRetractionReports.front().retractionValid);
  REQUIRE(std::any_of(
      mirrorReports.requestRetractionReports.begin(),
      mirrorReports.requestRetractionReports.end(),
      [](auto const& report) { return report.retractionValid; }));
  REQUIRE(observerReports.requestRetractionReports.front().retractionValid);
  REQUIRE_THROWS_AS(owner->retract(attributeRetraction), rti1516_2025::MessageCanNoLongerBeRetracted);
  REQUIRE_THROWS_AS(sender->retract(interactionRetraction), rti1516_2025::MessageCanNoLongerBeRetracted);
  REQUIRE_THROWS_AS(owner->retract(deletionRetraction), rti1516_2025::MessageCanNoLongerBeRetracted);

  REQUIRE_NOTHROW(mirror->unsubscribeObjectClassAttributes(
      mirrorServer, AttributeHandleSet{mirrorEfficiency}));
  REQUIRE_NOTHROW(observer->unsubscribeInteractionClass(observerInteraction));
  REQUIRE_NOTHROW(mirror->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(observer->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->resignFederationExecution(CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(sender->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(mirror->disconnect());
  REQUIRE_NOTHROW(observer->disconnect());
  REQUIRE_NOTHROW(sender->disconnect());
  REQUIRE_NOTHROW(owner->disconnect());
}
} // namespace
