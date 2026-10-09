#include "ieee1516_2025_federation_management_fixture_support.hpp"

namespace {

TEST_CASE(
    "Embedded timestamped regional attribute update survives time-constrained re-enable",
    "[integration][development-profile][object-management][ddm][time-management]"
    "[timestamped-regional-attribute-update][explicit-source][tso][re-enable]"
    "[timestamped-regional-attribute-reenable]"
    "[rti.service.register-object-instance-with-regions]"
    "[rti.service.subscribe-object-class-attributes-with-regions]"
    "[rti.service.update-attribute-values]"
    "[rti.service.create-region][rti.service.set-range-bounds]"
    "[rti.service.commit-region-modifications]"
    "[rti.service.set-convey-region-designator-sets-switch]"
    "[rti.service.enable-time-regulation][rti.service.enable-time-constrained]"
    "[rti.service.disable-time-constrained][rti.service.time-advance-request]"
    "[federate.callback.reflect-attribute-values]"
    "[federate.callback.time-constrained-enabled]"
    "[federate.callback.time-advance-grant]") {
  ReportingFederateAmbassador publisherReports;
  ReportingFederateAmbassador receiverReports;
  auto publisher = makeRti();
  auto receiver = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  unsigned char const valueBytes[] = {0x52, 0x45, 0x47, 0x32};
  unsigned char const tagBytes[] = {0x52, 0x45, 0x45, 0x4E};
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));

  REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED));
  REQUIRE_NOTHROW(receiver->connect(receiverReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      publisher->createFederationExecution(federationName, fomModule, L"HLAinteger64Time"));
  FederateHandle publisherHandle;
  REQUIRE_NOTHROW(publisherHandle = publisher->joinFederationExecution(
      L"timestamped-regional-attribute-reenable-publisher",
      L"publisher",
      federationName));
  REQUIRE_NOTHROW(receiver->joinFederationExecution(
      L"timestamped-regional-attribute-reenable-receiver",
      L"subscriber",
      federationName));

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

  auto const publisherRegion = publisher->createRegion(DimensionHandleSet{sodaFlavor});
  auto const receiverRegion = receiver->createRegion(DimensionHandleSet{sodaFlavor});
  REQUIRE_NOTHROW(publisher->setRangeBounds(
      publisherRegion,
      sodaFlavor,
      RangeBounds(0UL, 2UL)));
  REQUIRE_NOTHROW(publisher->commitRegionModifications(RegionHandleSet{publisherRegion}));
  REQUIRE_NOTHROW(receiver->setRangeBounds(
      receiverRegion,
      sodaFlavor,
      RangeBounds(1UL, 3UL)));
  REQUIRE_NOTHROW(receiver->commitRegionModifications(RegionHandleSet{receiverRegion}));
  AttributeHandleSetRegionHandleSetPairVector const publisherPair{{
      flavorOnly,
      RegionHandleSet{publisherRegion},
  }};
  AttributeHandleSetRegionHandleSetPairVector const receiverPair{{
      flavorOnly,
      RegionHandleSet{receiverRegion},
  }};
  REQUIRE_NOTHROW(receiver->subscribeObjectClassAttributesWithRegions(soda, receiverPair));
  REQUIRE_FALSE(receiver->getConveyRegionDesignatorSetsSwitch());
  REQUIRE_NOTHROW(receiver->setConveyRegionDesignatorSetsSwitch(true));

  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = publisher->registerObjectInstanceWithRegions(
      soda,
      publisherPair));
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE(receiverReports.objectDiscoveryReports.size() == 1U);
  REQUIRE(receiverReports.objectDiscoveryReports.front().objectInstance == objectInstance);

  REQUIRE_NOTHROW(receiver->enableTimeConstrained());
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE(receiverReports.timeConstrainedEnabledReports.size() == 1U);
  REQUIRE_NOTHROW(publisher->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(5)));
  while (publisher->evokeCallback(0.0)) {
  }

  AttributeHandleValueMap values;
  values.emplace(flavor, VariableLengthData(valueBytes, sizeof(valueBytes)));
  auto const retraction = publisher->updateAttributeValues(
      objectInstance,
      values,
      tag,
      rti1516_2025::HLAinteger64Time(7));
  REQUIRE(retraction.isValid());
  REQUIRE(receiverReports.attributeReflectionReports.empty());

  // The source association is explicit and overlap-qualified. Its queued
  // callback must retain the original RegionHandle through the role change.
  REQUIRE_NOTHROW(receiver->disableTimeConstrained());
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE(receiverReports.attributeReflectionReports.empty());
  REQUIRE_NOTHROW(receiver->enableTimeConstrained());
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE(receiverReports.timeConstrainedEnabledReports.size() == 2U);

  REQUIRE_NOTHROW(receiver->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(7)));
  REQUIRE_NOTHROW(publisher->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(2)));
  REQUIRE_FALSE(publisher->evokeCallback(0.0));
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE(receiverReports.attributeReflectionReports.size() == 1U);
  REQUIRE(receiverReports.timeAdvanceGrantReports.size() == 1U);
  REQUIRE(receiverReports.callbackOrder == std::vector<std::string>{"reflect", "grant"});

  auto const& report = receiverReports.attributeReflectionReports.front();
  REQUIRE(report.objectInstance == objectInstance);
  REQUIRE(report.attributeValues.size() == 1U);
  REQUIRE(report.attributeValues.contains(flavor));
  REQUIRE(variableLengthDataBytes(report.attributeValues.at(flavor)) ==
          std::vector<unsigned char>(valueBytes, valueBytes + sizeof(valueBytes)));
  REQUIRE(variableLengthDataBytes(report.userSuppliedTag) ==
          std::vector<unsigned char>(tagBytes, tagBytes + sizeof(tagBytes)));
  REQUIRE(report.producingFederate == publisherHandle);
  REQUIRE(report.timeImplementationName == L"HLAinteger64Time");
  REQUIRE(report.timeValue == L"7");
  REQUIRE(report.sentOrderType == rti1516_2025::TIMESTAMP);
  REQUIRE(report.receivedOrderType == rti1516_2025::TIMESTAMP);
  REQUIRE(report.retractionSupplied);
  REQUIRE(report.retractionValid);
  REQUIRE(report.sentRegionsSupplied);
  REQUIRE(report.sentRegions == RegionHandleSet{publisherRegion});
  REQUIRE_THROWS_AS(
      publisher->retract(retraction),
      rti1516_2025::MessageCanNoLongerBeRetracted);

  REQUIRE_NOTHROW(receiver->unsubscribeObjectClassAttributesWithRegions(soda, receiverPair));
  REQUIRE_NOTHROW(publisher->unassociateRegionsForUpdates(objectInstance, publisherPair));
  REQUIRE_NOTHROW(receiver->deleteRegion(receiverRegion));
  REQUIRE_NOTHROW(publisher->deleteRegion(publisherRegion));
  REQUIRE_NOTHROW(receiver->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(receiver->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
}


}  // namespace
