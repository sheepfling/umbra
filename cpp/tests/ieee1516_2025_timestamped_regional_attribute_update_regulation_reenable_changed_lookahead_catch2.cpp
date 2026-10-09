#include "ieee1516_2025_federation_management_fixture_support.hpp"

namespace {

TEST_CASE(
    "Embedded queued timestamped regional attribute update survives time-regulation disable and re-enable with changed lookahead",
    "[integration][development-profile][object-management][ddm][time-management]"
    "[timestamped-regional-attribute-update][explicit-source][tso][re-enable][regulation-disable][changed-lookahead]"
    "[timestamped-regional-attribute-update-regulation-reenable-changed-lookahead]"
    "[rti.service.register-object-instance-with-regions]"
    "[rti.service.subscribe-object-class-attributes-with-regions]"
    "[rti.service.update-attribute-values][rti.service.retract]"
    "[rti.service.create-region][rti.service.set-range-bounds]"
    "[rti.service.commit-region-modifications]"
    "[rti.service.set-convey-region-designator-sets-switch]"
    "[rti.service.change-default-attribute-order-type]"
    "[rti.service.enable-time-regulation][rti.service.disable-time-regulation]"
    "[rti.service.query-lookahead][rti.service.enable-time-constrained]"
    "[rti.service.time-advance-request]"
    "[federate.callback.reflect-attribute-values]"
    "[federate.callback.time-regulation-enabled]"
    "[federate.callback.time-advance-grant]") {
  ReportingFederateAmbassador publisherReports;
  ReportingFederateAmbassador receiverReports;
  auto publisher = makeRti();
  auto receiver = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  unsigned char const valueBytes[] = {0x52, 0x47, 0x41, 0x43};
  unsigned char const tagBytes[] = {0x43, 0x48, 0x47, 0x52};
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));

  REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED));
  REQUIRE_NOTHROW(receiver->connect(receiverReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      publisher->createFederationExecution(federationName, fomModule, standard_hla::mom::integer64_time));
  FederateHandle publisherHandle;
  REQUIRE_NOTHROW(publisherHandle = publisher->joinFederationExecution(
      L"timestamped-regional-attribute-changed-lookahead-publisher",
      L"publisher",
      federationName));
  REQUIRE_NOTHROW(receiver->joinFederationExecution(
      L"timestamped-regional-attribute-changed-lookahead-receiver",
      L"subscriber",
      federationName));

  auto const soda = publisher->getObjectClassHandle(fixture_hla::fom::food_drink_soda);
  auto const flavor = publisher->getAttributeHandle(soda, fixture_hla::fixture::flavor);
  auto const sodaFlavor = publisher->getDimensionHandle(fixture_hla::fixture::soda_flavor);
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
  while (receiver->evokeCallback(0.0)) {
  }
  REQUIRE(receiverReports.objectDiscoveryReports.size() == 1U);
  REQUIRE(receiverReports.objectDiscoveryReports.front().objectInstance == objectInstance);

  REQUIRE_NOTHROW(receiver->enableTimeConstrained());
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE(receiverReports.timeConstrainedEnabledReports.size() == 1U);
  REQUIRE_NOTHROW(publisher->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(1)));
  while (publisher->evokeCallback(0.0)) {
  }
  REQUIRE(publisherReports.timeRegulationEnabledReports.size() == 1U);

  AttributeHandleValueMap values;
  values.emplace(flavor, VariableLengthData(valueBytes, sizeof(valueBytes)));
  auto const retraction = publisher->updateAttributeValues(
      objectInstance,
      values,
      tag,
      rti1516_2025::HLAinteger64Time(5));
  REQUIRE(retraction.isValid());
  REQUIRE(receiverReports.attributeReflectionReports.empty());

  // The source realization is part of the accepted passel.  Mutating the
  // live region before the callback/grant boundary must not erase this
  // already-overlap-qualified reflection.
  REQUIRE_NOTHROW(publisher->setRangeBounds(
      publisherRegion,
      sodaFlavor,
      RangeBounds(3UL, 4UL)));
  REQUIRE_NOTHROW(publisher->commitRegionModifications(RegionHandleSet{publisherRegion}));

  // The accepted passel retains its explicit source-region snapshot while
  // the producer changes time-regulation role and lookahead. Re-enabling at
  // three must establish the new GALT boundary without replacing the queued
  // object/update or its retraction identity.
  REQUIRE_NOTHROW(publisher->disableTimeRegulation());
  REQUIRE_NOTHROW(publisher->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(3)));
  while (publisher->evokeCallback(0.0)) {
  }
  REQUIRE(publisherReports.timeRegulationEnabledReports.size() == 2U);
  rti1516_2025::HLAinteger64Interval changedLookahead;
  REQUIRE_NOTHROW(publisher->queryLookahead(changedLookahead));
  REQUIRE(changedLookahead.getInterval() == 3);

  // With current time zero and lookahead three, the producer's advance to two
  // raises GALT to the queued timestamp five. Reflection must precede the
  // receiver's matching grant and preserve the original source region.
  receiverReports.callbackOrder.clear();
  REQUIRE_NOTHROW(receiver->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(5)));
  REQUIRE(receiverReports.attributeReflectionReports.empty());
  REQUIRE_NOTHROW(publisher->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(2)));
  REQUIRE_FALSE(publisher->evokeCallback(0.0));
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE(receiverReports.attributeReflectionReports.size() == 1U);
  REQUIRE(receiverReports.timeAdvanceGrantReports.size() == 1U);
  REQUIRE(receiverReports.callbackOrder == std::vector<std::string>{"reflect", "grant"});

  auto const& reflection = receiverReports.attributeReflectionReports.front();
  REQUIRE(reflection.objectInstance == objectInstance);
  REQUIRE(reflection.attributeValues.size() == 1U);
  REQUIRE(reflection.attributeValues.contains(flavor));
  REQUIRE(variableLengthDataBytes(reflection.attributeValues.at(flavor)) ==
          std::vector<unsigned char>(valueBytes, valueBytes + sizeof(valueBytes)));
  REQUIRE(variableLengthDataBytes(reflection.userSuppliedTag) ==
          std::vector<unsigned char>(tagBytes, tagBytes + sizeof(tagBytes)));
  REQUIRE(reflection.producingFederate == publisherHandle);
  REQUIRE(reflection.sentRegionsSupplied);
  REQUIRE(reflection.sentRegions == RegionHandleSet{publisherRegion});
  REQUIRE(reflection.timeImplementationName == standard_hla::mom::integer64_time);
  REQUIRE(reflection.timeValue == L"5");
  REQUIRE(reflection.sentOrderType == rti1516_2025::TIMESTAMP);
  REQUIRE(reflection.receivedOrderType == rti1516_2025::TIMESTAMP);
  REQUIRE(reflection.retractionSupplied);
  REQUIRE(reflection.retractionValid);
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
