#include "ieee1516_2025_federation_management_fixture_support.hpp"

namespace {
void runRegionalRequestAttributeValueUpdateProviderResponseScenario() {
  ReportingFederateAmbassador ownerReports;
  ReportingFederateAmbassador requesterReports;
  auto owner = makeRti();
  auto requester = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  unsigned char const requestTagBytes[] = {0x52, 0x45, 0x51};
  unsigned char const responseTagBytes[] = {0x52, 0x45, 0x53};
  unsigned char const responseValueBytes[] = {0x5A, 0x25, 0x07};
  VariableLengthData const requestTag(requestTagBytes, sizeof(requestTagBytes));

  REQUIRE_NOTHROW(owner->connect(ownerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(requester->connect(requesterReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      owner->createFederationExecution(federationName, fomModule, L"HLAinteger64Time"));
  FederateHandle ownerHandle;
  REQUIRE_NOTHROW(ownerHandle = owner->joinFederationExecution(
      L"regional-response-owner", L"publisher", federationName));
  REQUIRE_NOTHROW(requester->joinFederationExecution(
      L"regional-response-requester", L"subscriber", federationName));

  auto const soda = owner->getObjectClassHandle(L"HLAobjectRoot.Food.Drink.Soda");
  auto const flavor = owner->getAttributeHandle(soda, L"Flavor");
  auto const sodaFlavor = owner->getDimensionHandle(L"SodaFlavor");
  REQUIRE(soda.isValid());
  REQUIRE(flavor.isValid());
  REQUIRE(sodaFlavor.isValid());
  AttributeHandleSet const flavorOnly{flavor};
  REQUIRE_NOTHROW(owner->publishObjectClassAttributes(soda, flavorOnly));

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

  // The Restaurant FOM enables automatic provision for this class.  Drain the
  // resulting setup callback before installing the explicit-response handler,
  // then assert only the callback caused by this regional request.
  while (owner->evokeCallback(0.0)) {
  }
  auto const initialProvideReportCount =
      ownerReports.attributeValueUpdateRequestReports.size();

  std::vector<unsigned char> observedRequestTag;
  std::vector<unsigned char> const responseValue(
      responseValueBytes,
      responseValueBytes + sizeof(responseValueBytes));
  std::vector<unsigned char> const responseTag(
      responseTagBytes,
      responseTagBytes + sizeof(responseTagBytes));
  ownerReports.provideAttributeValueUpdateHandler = [
      &owner,
      &observedRequestTag,
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
    VariableLengthData responseUserTag(responseTag.data(), responseTag.size());
    owner->updateAttributeValues(callbackObject, values, responseUserTag);
  };

  // This class-level regional request is not automatic provision. The owner
  // explicitly answers from the standard Provide Attribute Value Update
  // callback; its pre-existing update-region association is then carried into
  // the receiver's ordinary Reflect Attribute Values callback.
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
  auto const& provide = ownerReports.attributeValueUpdateRequestReports.back();
  REQUIRE(provide.objectInstance == objectInstance);
  REQUIRE(provide.attributes == flavorOnly);
  REQUIRE(observedRequestTag ==
          std::vector<unsigned char>(requestTagBytes, requestTagBytes + sizeof(requestTagBytes)));

  while (requester->evokeCallback(0.0)) {
  }
  REQUIRE(requesterReports.attributeReflectionReports.size() == 1U);
  auto const& reflection = requesterReports.attributeReflectionReports.front();
  REQUIRE(reflection.objectInstance == objectInstance);
  REQUIRE(reflection.attributeValues.size() == 1U);
  REQUIRE(reflection.attributeValues.contains(flavor));
  REQUIRE(variableLengthDataBytes(reflection.attributeValues.at(flavor)) == responseValue);
  REQUIRE(variableLengthDataBytes(reflection.userSuppliedTag) == responseTag);
  REQUIRE(reflection.transportationType == owner->getTransportationTypeHandle(L"HLAreliable"));
  REQUIRE(reflection.producingFederate == ownerHandle);
  REQUIRE(reflection.sentRegionsSupplied);
  REQUIRE(reflection.sentRegions == RegionHandleSet{ownerRegion});

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
}  // namespace

TEST_CASE(
    "Embedded regional Request Attribute Value Update supports a 2025 provider response",
    "[integration][development-profile][object-management][ddm]"
    "[rti.service.request-attribute-value-update-with-regions]"
    "[rti.service.update-attribute-values]"
    "[federate.callback.provide-attribute-value-update]"
    "[federate.callback.reflect-attribute-values]"
    "[regional-attribute-value-update-provider-response]") {
  runRegionalRequestAttributeValueUpdateProviderResponseScenario();
}

TEST_CASE(
    "Embedded regional Request Attribute Value Update supports a 2025 provider response (restored baseline copy)",
    "[integration][development-profile][object-management][ddm]"
    "[rti.service.request-attribute-value-update-with-regions]"
    "[rti.service.update-attribute-values]"
    "[federate.callback.provide-attribute-value-update]"
    "[federate.callback.reflect-attribute-values]"
    "[regional-attribute-value-update-provider-response][restored-baseline-copy]") {
  runRegionalRequestAttributeValueUpdateProviderResponseScenario();
}
