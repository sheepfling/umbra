#include "ieee1516_2025_federation_management_fixture_support.hpp"

namespace {
TEST_CASE(
    "Embedded federation restore preserves a terminal timestamped default-region attribute classification",
    "[integration][development-profile][federation-management][save-restore]"
    "[object-management][ddm][time-management][tso]"
    "[timestamped-default-region-attribute-update][default-region][tombstone]"
    "[rti.service.request-federation-save][rti.service.request-federation-restore]"
    "[rti.service.federate-save-begun][rti.service.federate-save-complete]"
    "[rti.service.federate-restore-complete][rti.service.update-attribute-values]"
    "[rti.service.publish-object-class-attributes]"
    "[rti.service.change-default-attribute-order-type]"
    "[rti.service.register-object-instance]"
    "[rti.service.create-region][rti.service.set-range-bounds]"
    "[rti.service.commit-region-modifications]"
    "[rti.service.subscribe-object-class-attributes-with-regions]"
    "[rti.service.unsubscribe-object-class-attributes-with-regions]"
    "[rti.service.delete-region]"
    "[rti.service.set-convey-region-designator-sets-switch]"
    "[rti.service.enable-time-constrained][rti.service.enable-time-regulation]"
    "[rti.service.time-advance-request][rti.service.retract]"
    "[federate.callback.federation-saved][federate.callback.federation-restored]") {
  ReportingFederateAmbassador publisherReports;
  ReportingFederateAmbassador receiverReports;
  auto publisher = makeRti();
  auto receiver = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  unsigned char const valueBytes[] = {0x44, 0x52, 0x2D, 0x41};
  unsigned char const tagBytes[] = {0x44, 0x52, 0x2D, 0x54};
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));
  std::wstring const saveLabel = L"terminal-default-region-attribute-retraction-baseline";

  REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED));
  REQUIRE_NOTHROW(receiver->connect(receiverReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      publisher->createFederationExecution(federationName, fomModule, standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(publisher->joinFederationExecution(
      L"restore-terminal-default-region-attribute-publisher", L"publisher", federationName));
  REQUIRE_NOTHROW(receiver->joinFederationExecution(
      L"restore-terminal-default-region-attribute-receiver", L"subscriber", federationName));

  auto const soda = publisher->getObjectClassHandle(fixture_hla::fom::food_drink_soda);
  auto const flavor = publisher->getAttributeHandle(soda, fixture_hla::fixture::flavor);
  auto const sodaFlavor = publisher->getDimensionHandle(fixture_hla::fixture::soda_flavor);
  REQUIRE(soda.isValid());
  REQUIRE(flavor.isValid());
  REQUIRE(sodaFlavor.isValid());
  AttributeHandleSet const flavorOnly{flavor};
  REQUIRE_NOTHROW(publisher->publishObjectClassAttributes(soda, flavorOnly));
  REQUIRE_NOTHROW(publisher->changeDefaultAttributeOrderType(soda, flavorOnly, TIMESTAMP));

  auto const receiverRegion = receiver->createRegion(DimensionHandleSet{sodaFlavor});
  REQUIRE_NOTHROW(receiver->setRangeBounds(
      receiverRegion,
      sodaFlavor,
      RangeBounds(2UL, 3UL)));
  REQUIRE_NOTHROW(receiver->commitRegionModifications(RegionHandleSet{receiverRegion}));
  AttributeHandleSetRegionHandleSetPairVector const receiverPair{{
      flavorOnly,
      RegionHandleSet{receiverRegion},
  }};
  REQUIRE_NOTHROW(receiver->subscribeObjectClassAttributesWithRegions(soda, receiverPair));
  REQUIRE_NOTHROW(receiver->setConveyRegionDesignatorSetsSwitch(true));

  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = publisher->registerObjectInstance(soda));
  while (receiver->evokeCallback(0.0)) {
  }
  REQUIRE(receiverReports.objectDiscoveryReports.size() == 1U);
  REQUIRE_NOTHROW(receiver->enableTimeConstrained());
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE_NOTHROW(publisher->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(5)));
  while (publisher->evokeCallback(0.0)) {
  }

  AttributeHandleValueMap values;
  values.emplace(flavor, VariableLengthData(valueBytes, sizeof(valueBytes)));
  auto const terminal = publisher->updateAttributeValues(
      objectInstance,
      values,
      tag,
      rti1516_2025::HLAinteger64Time(7));
  REQUIRE(terminal.isValid());
  REQUIRE(receiverReports.attributeReflectionReports.empty());
  REQUIRE_NOTHROW(publisher->retract(terminal));
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE(receiverReports.attributeReflectionReports.empty());
  REQUIRE_THROWS_AS(
      publisher->retract(terminal),
      rti1516_2025::MessageCanNoLongerBeRetracted);

  // Keep a distinct live default-source passel to admit the constrained
  // recipient; the terminal tombstone has no pending recipient state after
  // Retract.
  auto const admission = publisher->updateAttributeValues(
      objectInstance,
      values,
      tag,
      rti1516_2025::HLAinteger64Time(7));
  REQUIRE(admission.isValid());
  REQUIRE_NOTHROW(receiver->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(1)));
  REQUIRE_NOTHROW(publisher->requestFederationSave(saveLabel));
  while (receiver->evokeCallback(0.0)) {
  }
  while (publisher->evokeCallback(0.0)) {
  }
  REQUIRE_NOTHROW(publisher->federateSaveBegun());
  REQUIRE_NOTHROW(receiver->federateSaveBegun());
  REQUIRE_NOTHROW(publisher->federateSaveComplete());
  REQUIRE_NOTHROW(receiver->federateSaveComplete());
  while (publisher->evokeCallback(0.0)) {
  }
  while (receiver->evokeCallback(0.0)) {
  }
  REQUIRE(publisherReports.federationSavedReportCount == 1U);
  REQUIRE(receiverReports.federationSavedReportCount == 1U);

  REQUIRE_NOTHROW(publisher->retract(admission));
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  auto const postSave = publisher->updateAttributeValues(
      objectInstance,
      values,
      tag,
      rti1516_2025::HLAinteger64Time(7));
  REQUIRE(postSave.isValid());
  REQUIRE(variableLengthDataBytes(terminal.encode()) !=
          variableLengthDataBytes(postSave.encode()));

  REQUIRE_NOTHROW(publisher->requestFederationRestore(saveLabel));
  while (publisher->evokeCallback(0.0)) {
  }
  while (receiver->evokeCallback(0.0)) {
  }
  REQUIRE_NOTHROW(publisher->federateRestoreComplete());
  REQUIRE_NOTHROW(receiver->federateRestoreComplete());
  while (publisher->evokeCallback(0.0)) {
  }
  while (receiver->evokeCallback(0.0)) {
  }
  REQUIRE(publisherReports.federationRestoredReportCount == 1U);
  REQUIRE(receiverReports.federationRestoredReportCount == 1U);

  REQUIRE_THROWS_AS(
      publisher->retract(terminal),
      rti1516_2025::MessageCanNoLongerBeRetracted);
  REQUIRE_THROWS_AS(
      publisher->retract(postSave),
      rti1516_2025::InvalidMessageRetractionHandle);

  REQUIRE_NOTHROW(receiver->unsubscribeObjectClassAttributesWithRegions(soda, receiverPair));
  REQUIRE_NOTHROW(receiver->deleteRegion(receiverRegion));
  REQUIRE_NOTHROW(receiver->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(receiver->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
}

}  // namespace
