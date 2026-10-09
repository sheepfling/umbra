#include "ieee1516_2025_federation_management_fixture_support.hpp"

namespace {

TEST_CASE(
    "Embedded timestamped regional Update Attribute Values returns a retraction designator without overlap-qualified recipients",
    "[integration][development-profile][object-management][ddm][time-management][tso]"
    "[rti.service.update-attribute-values][rti.service.retract]"
    "[rti.service.register-object-instance-with-regions]"
    "[rti.service.subscribe-object-class-attributes-with-regions]"
    "[rti.service.time-advance-request]") {
  ReportingFederateAmbassador publisherReports;
  ReportingFederateAmbassador receiverReports;
  auto publisher = makeRti();
  auto receiver = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  unsigned char const valueBytes[] = {0x5A, 0x25};
  unsigned char const tagBytes[] = {0x4F, 0x56, 0x45, 0x52};
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));

  REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED));
  REQUIRE_NOTHROW(receiver->connect(receiverReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      publisher->createFederationExecution(federationName, fomModule, L"HLAinteger64Time"));
  REQUIRE_NOTHROW(publisher->joinFederationExecution(
      L"regional-attribute-no-overlap-publisher", L"publisher", federationName));
  REQUIRE_NOTHROW(receiver->joinFederationExecution(
      L"regional-attribute-no-overlap-receiver", L"subscriber", federationName));

  auto const soda = publisher->getObjectClassHandle(L"HLAobjectRoot.Food.Drink.Soda");
  auto const flavor = publisher->getAttributeHandle(soda, L"Flavor");
  auto const sodaFlavor = publisher->getDimensionHandle(L"SodaFlavor");
  REQUIRE(soda.isValid());
  REQUIRE(flavor.isValid());
  REQUIRE(sodaFlavor.isValid());
  AttributeHandleSet const flavorOnly{flavor};
  REQUIRE_NOTHROW(publisher->publishObjectClassAttributes(soda, flavorOnly));
  REQUIRE_NOTHROW(publisher->changeDefaultAttributeOrderType(soda, flavorOnly, TIMESTAMP));

  auto const publisherRegion = publisher->createRegion(DimensionHandleSet{sodaFlavor});
  auto const receiverRegion = receiver->createRegion(DimensionHandleSet{sodaFlavor});
  REQUIRE_NOTHROW(publisher->setRangeBounds(
      publisherRegion, sodaFlavor, RangeBounds(0UL, 1UL)));
  REQUIRE_NOTHROW(receiver->setRangeBounds(
      receiverRegion, sodaFlavor, RangeBounds(2UL, 3UL)));
  REQUIRE_NOTHROW(publisher->commitRegionModifications(RegionHandleSet{publisherRegion}));
  REQUIRE_NOTHROW(receiver->commitRegionModifications(RegionHandleSet{receiverRegion}));
  AttributeHandleSetRegionHandleSetPairVector const publisherPair{{
      flavorOnly,
      RegionHandleSet{publisherRegion},
  }};
  AttributeHandleSetRegionHandleSetPairVector const receiverPair{{
      flavorOnly,
      RegionHandleSet{receiverRegion},
  }};
  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = publisher->registerObjectInstanceWithRegions(
      soda,
      publisherPair));
  REQUIRE_NOTHROW(receiver->subscribeObjectClassAttributesWithRegions(
      soda,
      receiverPair));
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE(receiverReports.objectDiscoveryReports.empty());

  REQUIRE_NOTHROW(receiver->enableTimeConstrained());
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE_NOTHROW(publisher->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(5)));
  // Time regulation may report an advisory callback through the evoked
  // ambassador. Drain setup traffic before asserting the TSO/DDM behavior.
  while (publisher->evokeCallback(0.0)) {
  }
  AttributeHandleValueMap values;
  values.emplace(flavor, VariableLengthData(valueBytes, sizeof(valueBytes)));

  // Clause 6.10's TSO-preferred-attribute condition holds despite the
  // disjoint subscription. The public result therefore has a designator
  // while the regional planner suppresses all callback fanout.
  auto const retractable = publisher->updateAttributeValues(
      objectInstance,
      values,
      tag,
      rti1516_2025::HLAinteger64Time(6));
  REQUIRE(retractable.isValid());
  REQUIRE(receiverReports.attributeReflectionReports.empty());
  REQUIRE_NOTHROW(publisher->retract(retractable));
  REQUIRE_THROWS_AS(
      publisher->retract(retractable),
      rti1516_2025::MessageCanNoLongerBeRetracted);

  auto const expired = publisher->updateAttributeValues(
      objectInstance,
      values,
      tag,
      rti1516_2025::HLAinteger64Time(6));
  REQUIRE(expired.isValid());
  REQUIRE_NOTHROW(publisher->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(1)));
  REQUIRE_THROWS_AS(
      publisher->retract(expired),
      rti1516_2025::MessageCanNoLongerBeRetracted);
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE(receiverReports.attributeReflectionReports.empty());

  REQUIRE_NOTHROW(receiver->unsubscribeObjectClassAttributesWithRegions(
      soda,
      receiverPair));
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
