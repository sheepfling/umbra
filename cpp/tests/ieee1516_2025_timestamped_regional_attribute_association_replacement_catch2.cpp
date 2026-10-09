#include "ieee1516_2025_federation_management_fixture_support.hpp"

namespace {

TEST_CASE(
    "Embedded timestamped regional attribute association replacement does not retarget a queued passel",
    "[integration][development-profile][object-management][ddm][time-management][tso]"
    "[timestamped-regional-attribute-update][explicit-source][association-replacement]"
    "[rti.service.create-region][rti.service.set-range-bounds]"
    "[rti.service.commit-region-modifications]"
    "[rti.service.register-object-instance-with-regions]"
    "[rti.service.associate-regions-for-updates]"
    "[rti.service.unassociate-regions-for-updates]"
    "[rti.service.subscribe-object-class-attributes-with-regions]"
    "[rti.service.unsubscribe-object-class-attributes-with-regions]"
    "[rti.service.update-attribute-values]"
    "[rti.service.set-convey-region-designator-sets-switch]"
    "[rti.service.enable-time-constrained][rti.service.enable-time-regulation]"
    "[rti.service.time-advance-request]"
    "[federate.callback.discover-object-instance]"
    "[federate.callback.reflect-attribute-values]"
    "[federate.callback.time-advance-grant]"
    "[federate.callback.request-retraction]") {
  ReportingFederateAmbassador publisherReports;
  ReportingFederateAmbassador receiverReports;
  auto publisher = makeRti();
  auto receiver = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  unsigned char const firstValueBytes[] = {0x53, 0x52, 0x31};
  unsigned char const secondValueBytes[] = {0x53, 0x52, 0x32};
  unsigned char const firstTagBytes[] = {0x52, 0x45, 0x50, 0x31};
  unsigned char const secondTagBytes[] = {0x52, 0x45, 0x50, 0x32};
  VariableLengthData const firstTag(firstTagBytes, sizeof(firstTagBytes));
  VariableLengthData const secondTag(secondTagBytes, sizeof(secondTagBytes));

  REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED));
  REQUIRE_NOTHROW(receiver->connect(receiverReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      publisher->createFederationExecution(federationName, fomModule, L"HLAinteger64Time"));
  REQUIRE_NOTHROW(publisher->joinFederationExecution(
      L"timestamped-regional-replacement-publisher", L"publisher", federationName));
  REQUIRE_NOTHROW(receiver->joinFederationExecution(
      L"timestamped-regional-replacement-receiver", L"subscriber", federationName));

  auto const soda = publisher->getObjectClassHandle(L"HLAobjectRoot.Food.Drink.Soda");
  auto const flavor = publisher->getAttributeHandle(soda, L"Flavor");
  auto const sodaFlavor = publisher->getDimensionHandle(L"SodaFlavor");
  REQUIRE(soda.isValid());
  REQUIRE(flavor.isValid());
  REQUIRE(sodaFlavor.isValid());
  AttributeHandleSet const flavorOnly{flavor};
  REQUIRE_NOTHROW(publisher->publishObjectClassAttributes(soda, flavorOnly));
  REQUIRE_NOTHROW(publisher->changeDefaultAttributeOrderType(
      soda,
      flavorOnly,
      TIMESTAMP));

  auto const sourceRegionA = publisher->createRegion(DimensionHandleSet{sodaFlavor});
  auto const sourceRegionB = publisher->createRegion(DimensionHandleSet{sodaFlavor});
  auto const receiverRegion = receiver->createRegion(DimensionHandleSet{sodaFlavor});
  for (auto const sourceRegion : {sourceRegionA, sourceRegionB}) {
    REQUIRE_NOTHROW(publisher->setRangeBounds(
        sourceRegion,
        sodaFlavor,
        RangeBounds(0UL, 1UL)));
    REQUIRE_NOTHROW(publisher->commitRegionModifications(RegionHandleSet{sourceRegion}));
  }
  REQUIRE_NOTHROW(receiver->setRangeBounds(
      receiverRegion,
      sodaFlavor,
      RangeBounds(0UL, 1UL)));
  REQUIRE_NOTHROW(receiver->commitRegionModifications(RegionHandleSet{receiverRegion}));

  AttributeHandleSetRegionHandleSetPairVector const sourcePairA{{
      flavorOnly,
      RegionHandleSet{sourceRegionA},
  }};
  AttributeHandleSetRegionHandleSetPairVector const sourcePairB{{
      flavorOnly,
      RegionHandleSet{sourceRegionB},
  }};
  AttributeHandleSetRegionHandleSetPairVector const receiverPair{{
      flavorOnly,
      RegionHandleSet{receiverRegion},
  }};
  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = publisher->registerObjectInstanceWithRegions(
      soda,
      sourcePairA));
  REQUIRE_NOTHROW(receiver->subscribeObjectClassAttributesWithRegions(soda, receiverPair));
  while (receiver->evokeCallback(0.0)) {
  }
  REQUIRE(receiverReports.objectDiscoveryReports.size() == 1U);
  REQUIRE_NOTHROW(receiver->setConveyRegionDesignatorSetsSwitch(true));
  REQUIRE_NOTHROW(receiver->enableTimeConstrained());
  while (receiver->evokeCallback(0.0)) {
  }
  REQUIRE_NOTHROW(publisher->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(5)));
  while (publisher->evokeCallback(0.0)) {
  }

  AttributeHandleValueMap firstValues;
  firstValues.emplace(
      flavor,
      VariableLengthData(firstValueBytes, sizeof(firstValueBytes)));
  auto const firstRetraction = publisher->updateAttributeValues(
      objectInstance,
      firstValues,
      firstTag,
      rti1516_2025::HLAinteger64Time(6));
  REQUIRE(firstRetraction.isValid());
  REQUIRE(receiverReports.attributeReflectionReports.empty());

  // The queued passel captured sourceRegionA. Replacing the association before
  // its callback boundary must not retarget that old payload to sourceRegionB.
  REQUIRE_NOTHROW(publisher->unassociateRegionsForUpdates(objectInstance, sourcePairA));
  REQUIRE_NOTHROW(publisher->associateRegionsForUpdates(objectInstance, sourcePairB));
  REQUIRE_NOTHROW(receiver->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(6)));
  REQUIRE_NOTHROW(publisher->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(2)));
  while (publisher->evokeCallback(0.0)) {
  }
  while (receiver->evokeCallback(0.0)) {
  }
  REQUIRE(receiverReports.timeAdvanceGrantReports.size() == 1U);
  REQUIRE(receiverReports.attributeReflectionReports.empty());
  REQUIRE(receiverReports.requestRetractionReports.empty());

  // The replacement applies to later sends. The new passel carries only
  // sourceRegionB and is delivered after the receiver crosses timestamp 7.
  AttributeHandleValueMap secondValues;
  secondValues.emplace(
      flavor,
      VariableLengthData(secondValueBytes, sizeof(secondValueBytes)));
  auto const secondRetraction = publisher->updateAttributeValues(
      objectInstance,
      secondValues,
      secondTag,
      rti1516_2025::HLAinteger64Time(7));
  REQUIRE(secondRetraction.isValid());
  REQUIRE_NOTHROW(receiver->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(7)));
  REQUIRE_NOTHROW(publisher->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(3)));
  while (publisher->evokeCallback(0.0)) {
  }
  while (receiver->evokeCallback(0.0)) {
  }
  REQUIRE(receiverReports.timeAdvanceGrantReports.size() == 2U);
  REQUIRE(receiverReports.attributeReflectionReports.size() == 1U);
  auto const& report = receiverReports.attributeReflectionReports.front();
  REQUIRE(report.objectInstance == objectInstance);
  REQUIRE(report.attributeValues.size() == 1U);
  REQUIRE(report.attributeValues.contains(flavor));
  REQUIRE(variableLengthDataBytes(report.attributeValues.at(flavor)) ==
          std::vector<unsigned char>(secondValueBytes, secondValueBytes + sizeof(secondValueBytes)));
  REQUIRE(report.timeValue == L"7");
  REQUIRE(report.sentOrderType == rti1516_2025::TIMESTAMP);
  REQUIRE(report.receivedOrderType == rti1516_2025::TIMESTAMP);
  REQUIRE(report.sentRegionsSupplied);
  REQUIRE(report.sentRegions.size() == 1U);
  REQUIRE(report.sentRegions.contains(sourceRegionB));
  REQUIRE(variableLengthDataBytes(report.userSuppliedTag) ==
          std::vector<unsigned char>(secondTagBytes, secondTagBytes + sizeof(secondTagBytes)));
  REQUIRE(receiverReports.requestRetractionReports.empty());

  REQUIRE_NOTHROW(receiver->unsubscribeObjectClassAttributesWithRegions(soda, receiverPair));
  REQUIRE_NOTHROW(publisher->unassociateRegionsForUpdates(objectInstance, sourcePairB));
  REQUIRE_NOTHROW(receiver->deleteRegion(receiverRegion));
  REQUIRE_NOTHROW(publisher->deleteRegion(sourceRegionB));
  REQUIRE_NOTHROW(publisher->deleteRegion(sourceRegionA));
  REQUIRE_NOTHROW(receiver->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(receiver->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
}


}  // namespace
