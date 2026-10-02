#include "ieee1516_2025_federation_management_test_support.hpp"
#include "ieee1516_2025_federation_management_fixture_support.hpp"
#include "ieee1516_2025_public_federation_restore_test_support.hpp"

namespace {
TEST_CASE(
    "Embedded regional Request Attribute Value Update supports a timestamped provider response",
    "[integration][development-profile][object-management][ddm][time-management]"
    "[timestamped-regional-attribute-update][timestamped-regional-request-provider-response][tso]"
    "[rti.service.request-attribute-value-update-with-regions]"
    "[rti.service.update-attribute-values]"
    "[rti.service.change-default-attribute-order-type]"
    "[rti.service.enable-time-regulation][rti.service.enable-time-constrained]"
    "[rti.service.time-advance-request]"
    "[federate.callback.provide-attribute-value-update]"
    "[federate.callback.reflect-attribute-values]"
    "[federate.callback.time-constrained-enabled]"
    "[federate.callback.time-advance-grant]") {
  ReportingFederateAmbassador ownerReports;
  ReportingFederateAmbassador requesterReports;
  auto owner = makeRti();
  auto requester = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  unsigned char const requestTagBytes[] = {0x54, 0x53, 0x4F};
  unsigned char const responseTagBytes[] = {0x52, 0x45, 0x51, 0x54};
  unsigned char const responseValueBytes[] = {0x6A, 0x25, 0x02};
  VariableLengthData const requestTag(requestTagBytes, sizeof(requestTagBytes));
  VariableLengthData const responseTag(responseTagBytes, sizeof(responseTagBytes));
  std::vector<unsigned char> const responseValue(
      responseValueBytes,
      responseValueBytes + sizeof(responseValueBytes));
  rti1516_2025::MessageRetractionHandle responseRetraction;

  REQUIRE_NOTHROW(owner->connect(ownerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(requester->connect(requesterReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      owner->createFederationExecution(federationName, fomModule, L"HLAinteger64Time"));
  FederateHandle ownerHandle;
  REQUIRE_NOTHROW(ownerHandle = owner->joinFederationExecution(
      L"regional-timestamped-response-owner", L"publisher", federationName));
  REQUIRE_NOTHROW(requester->joinFederationExecution(
      L"regional-timestamped-response-requester", L"subscriber", federationName));

  auto const soda = owner->getObjectClassHandle(L"HLAobjectRoot.Food.Drink.Soda");
  auto const flavor = owner->getAttributeHandle(soda, L"Flavor");
  auto const sodaFlavor = owner->getDimensionHandle(L"SodaFlavor");
  REQUIRE(soda.isValid());
  REQUIRE(flavor.isValid());
  REQUIRE(sodaFlavor.isValid());
  AttributeHandleSet const flavorOnly{flavor};
  REQUIRE_NOTHROW(owner->publishObjectClassAttributes(soda, flavorOnly));
  REQUIRE_NOTHROW(owner->changeDefaultAttributeOrderType(soda, flavorOnly, TIMESTAMP));

  auto const ownerRegion = owner->createRegion(DimensionHandleSet{sodaFlavor});
  auto const requesterRegion = requester->createRegion(DimensionHandleSet{sodaFlavor});
  REQUIRE_NOTHROW(owner->setRangeBounds(
      ownerRegion,
      sodaFlavor,
      RangeBounds(0UL, 1UL)));
  REQUIRE_NOTHROW(requester->setRangeBounds(
      requesterRegion,
      sodaFlavor,
      RangeBounds(0UL, 1UL)));
  REQUIRE_NOTHROW(owner->commitRegionModifications(RegionHandleSet{ownerRegion}));
  REQUIRE_NOTHROW(requester->commitRegionModifications(RegionHandleSet{requesterRegion}));
  AttributeHandleSetRegionHandleSetPairVector const ownerPair{{
      flavorOnly,
      RegionHandleSet{ownerRegion},
  }};
  AttributeHandleSetRegionHandleSetPairVector const requesterPair{{
      flavorOnly,
      RegionHandleSet{requesterRegion},
  }};
  REQUIRE_NOTHROW(requester->subscribeObjectClassAttributesWithRegions(
      soda,
      requesterPair));
  REQUIRE_NOTHROW(requester->setConveyRegionDesignatorSetsSwitch(true));

  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = owner->registerObjectInstanceWithRegions(
      soda,
      ownerPair));
  while (requester->evokeCallback(0.0)) {
  }
  REQUIRE(requesterReports.objectDiscoveryReports.size() == 1U);
  REQUIRE(requester->getKnownObjectClassHandle(objectInstance) == soda);

  // Drain Restaurant's automatic-provision setup before installing the
  // explicit provider response used by this test.
  while (owner->evokeCallback(0.0)) {
  }
  auto const initialProvideReportCount =
      ownerReports.attributeValueUpdateRequestReports.size();
  std::vector<unsigned char> observedRequestTag;
  ownerReports.provideAttributeValueUpdateHandler = [
      &owner,
      &observedRequestTag,
      &responseRetraction,
      responseValue,
      responseTag](
      ObjectInstanceHandle const& callbackObject,
      AttributeHandleSet const& callbackAttributes,
      VariableLengthData const& callbackTag) {
    observedRequestTag = variableLengthDataBytes(callbackTag);
    AttributeHandleValueMap values;
    for (AttributeHandle const& attribute : callbackAttributes) {
      values.emplace(
          attribute,
          VariableLengthData(responseValue.data(), responseValue.size()));
    }
    responseRetraction = owner->updateAttributeValues(
        callbackObject,
        values,
        responseTag,
        rti1516_2025::HLAinteger64Time(2));
  };

  REQUIRE_NOTHROW(requester->enableTimeConstrained());
  REQUIRE_FALSE(requester->evokeCallback(0.0));
  REQUIRE(requesterReports.timeConstrainedEnabledReports.size() == 1U);
  REQUIRE_NOTHROW(owner->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(1)));
  while (owner->evokeCallback(0.0)) {
  }
  REQUIRE(ownerReports.timeRegulationEnabledReports.size() == 1U);

  // The request itself only arranges the provider callback. The provider's
  // timestamped response must remain queued until the constrained recipient's
  // matching grant, with the source update association preserved.
  REQUIRE_NOTHROW(requester->requestAttributeValueUpdateWithRegions(
      soda,
      requesterPair,
      requestTag));
  REQUIRE(ownerReports.attributeValueUpdateRequestReports.size() ==
          initialProvideReportCount);
  REQUIRE(requesterReports.attributeReflectionReports.empty());
  while (owner->evokeCallback(0.0)) {
  }
  REQUIRE(ownerReports.attributeValueUpdateRequestReports.size() ==
          initialProvideReportCount + 1U);
  REQUIRE(responseRetraction.isValid());
  REQUIRE(observedRequestTag ==
          std::vector<unsigned char>(
              requestTagBytes,
              requestTagBytes + sizeof(requestTagBytes)));
  REQUIRE(requesterReports.attributeReflectionReports.empty());

  REQUIRE_NOTHROW(requester->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(2)));
  REQUIRE_NOTHROW(owner->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(2)));
  while (owner->evokeCallback(0.0)) {
  }
  while (requester->evokeCallback(0.0)) {
  }
  REQUIRE(requesterReports.attributeReflectionReports.size() == 1U);
  REQUIRE(requesterReports.timeAdvanceGrantReports.size() == 1U);
  REQUIRE(requesterReports.callbackOrder == std::vector<std::string>{"reflect", "grant"});
  auto const& reflection = requesterReports.attributeReflectionReports.front();
  REQUIRE(reflection.objectInstance == objectInstance);
  REQUIRE(reflection.attributeValues.size() == 1U);
  REQUIRE(reflection.attributeValues.contains(flavor));
  REQUIRE(variableLengthDataBytes(reflection.attributeValues.at(flavor)) == responseValue);
  REQUIRE(variableLengthDataBytes(reflection.userSuppliedTag) ==
          std::vector<unsigned char>(
              responseTagBytes,
              responseTagBytes + sizeof(responseTagBytes)));
  REQUIRE(reflection.transportationType == owner->getTransportationTypeHandle(L"HLAreliable"));
  REQUIRE(reflection.producingFederate == ownerHandle);
  REQUIRE(reflection.sentRegionsSupplied);
  REQUIRE(reflection.sentRegions == RegionHandleSet{ownerRegion});
  REQUIRE(reflection.timeValue == L"2");
  REQUIRE(reflection.sentOrderType == TIMESTAMP);
  REQUIRE(reflection.receivedOrderType == TIMESTAMP);
  REQUIRE(reflection.retractionSupplied);
  REQUIRE(reflection.retractionValid);

  REQUIRE_THROWS_AS(
      owner->retract(responseRetraction),
      rti1516_2025::MessageCanNoLongerBeRetracted);

  REQUIRE_NOTHROW(requester->unsubscribeObjectClassAttributesWithRegions(
      soda,
      requesterPair));
  REQUIRE_NOTHROW(owner->unassociateRegionsForUpdates(objectInstance, ownerPair));
  REQUIRE_NOTHROW(owner->deleteRegion(ownerRegion));
  REQUIRE_NOTHROW(requester->deleteRegion(requesterRegion));
  REQUIRE_NOTHROW(requester->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->resignFederationExecution(CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(owner->disconnect());
  REQUIRE_NOTHROW(requester->disconnect());
}

}
