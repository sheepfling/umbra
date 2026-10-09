#include "ieee1516_2025_federation_management_fixture_support.hpp"

namespace {

TEST_CASE(
    "Embedded timestamped default-region attribute update survives time-regulation disable and re-enable at the new lookahead",
    "[integration][development-profile][object-management][ddm][time-management]"
    "[timestamped-default-region-attribute-update][timestamped-default-region-attribute-regulation-reenable]"
    "[timestamped-default-region-attribute-regulation-reenable-changed-lookahead]"
    "[default-region][tso][re-enable][regulation-disable][changed-lookahead]"
    "[rti.service.register-object-instance]"
    "[rti.service.subscribe-object-class-attributes-with-regions]"
    "[rti.service.update-attribute-values]"
    "[rti.service.create-region][rti.service.set-range-bounds]"
    "[rti.service.commit-region-modifications]"
    "[rti.service.set-convey-region-designator-sets-switch]"
    "[rti.service.enable-time-regulation][rti.service.disable-time-regulation]"
    "[rti.service.enable-time-constrained][rti.service.time-advance-request]"
    "[rti.service.retract]"
    "[federate.callback.reflect-attribute-values]"
    "[federate.callback.time-regulation-enabled]"
    "[federate.callback.time-advance-grant]") {
  auto runScenario = [](std::int64_t reenabledLookahead,
                        std::int64_t producerAdvance) {
  ReportingFederateAmbassador publisherReports;
  ReportingFederateAmbassador receiverReports;
  auto publisher = makeRti();
  auto receiver = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  unsigned char const valueBytes[] = {0x52, 0x45, 0x47, 0x52};
  unsigned char const tagBytes[] = {0x52, 0x45, 0x47, 0x41};
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));

  REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED));
  REQUIRE_NOTHROW(receiver->connect(receiverReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      publisher->createFederationExecution(federationName, fomModule, standard_hla::mom::integer64_time));
  FederateHandle publisherHandle;
  REQUIRE_NOTHROW(publisherHandle = publisher->joinFederationExecution(
      L"timestamped-default-region-attribute-regulation-publisher",
      L"publisher",
      federationName));
  REQUIRE_NOTHROW(receiver->joinFederationExecution(
      L"timestamped-default-region-attribute-regulation-receiver",
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
  REQUIRE_FALSE(receiver->getConveyRegionDesignatorSetsSwitch());
  REQUIRE_NOTHROW(receiver->setConveyRegionDesignatorSetsSwitch(true));

  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = publisher->registerObjectInstance(soda));
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
  REQUIRE(publisherReports.timeRegulationEnabledReports.size() == 1U);

  AttributeHandleValueMap values;
  values.emplace(flavor, VariableLengthData(valueBytes, sizeof(valueBytes)));
  auto const retraction = publisher->updateAttributeValues(
      objectInstance,
      values,
      tag,
      rti1516_2025::HLAinteger64Time(6));
  REQUIRE(retraction.isValid());
  REQUIRE(receiverReports.attributeReflectionReports.empty());

  // Producer role changes must not discard or retarget an already accepted
  // default-region passel. Re-enabling at either the same or a changed
  // lookahead establishes a new producer boundary while the recipient's
  // queued copy remains tied to this joined-federate lifetime.
  REQUIRE_NOTHROW(publisher->disableTimeRegulation());
  REQUIRE(publisherReports.timeRegulationEnabledReports.size() == 1U);
  REQUIRE_NOTHROW(publisher->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(reenabledLookahead)));
  REQUIRE_FALSE(publisher->evokeCallback(0.0));
  REQUIRE(publisherReports.timeRegulationEnabledReports.size() == 2U);
  rti1516_2025::HLAinteger64Interval changedLookahead;
  REQUIRE_NOTHROW(publisher->queryLookahead(changedLookahead));
  REQUIRE(changedLookahead.getInterval() == reenabledLookahead);

  REQUIRE_NOTHROW(receiver->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(6)));
  REQUIRE_NOTHROW(publisher->timeAdvanceRequest(
      rti1516_2025::HLAinteger64Time(producerAdvance)));
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
  REQUIRE(report.timeImplementationName == standard_hla::mom::integer64_time);
  REQUIRE(report.timeValue == L"6");
  REQUIRE(report.sentOrderType == rti1516_2025::TIMESTAMP);
  REQUIRE(report.receivedOrderType == rti1516_2025::TIMESTAMP);
  REQUIRE(report.retractionSupplied);
  REQUIRE(report.retractionValid);
  REQUIRE(report.sentRegionsSupplied);
  REQUIRE(report.sentRegions.empty());
  REQUIRE_THROWS_AS(
      publisher->retract(retraction),
      rti1516_2025::MessageCanNoLongerBeRetracted);

  REQUIRE_NOTHROW(receiver->unsubscribeObjectClassAttributesWithRegions(soda, receiverPair));
  REQUIRE_NOTHROW(receiver->deleteRegion(receiverRegion));
  REQUIRE_NOTHROW(receiver->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(receiver->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
  };

  // Exercise both the unchanged-lookahead lifecycle and the changed-lookahead
  // boundary.  With lookahead three, TAR(3) is the first producer position
  // that raises GALT to the queued timestamp six.
  runScenario(5, 2);
  runScenario(3, 3);
}

} // namespace
