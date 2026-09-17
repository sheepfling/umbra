#include <catch2/catch_test_macros.hpp>

#include <RTI/NullFederateAmbassador.h>
#include <RTI/RTI1516.h>

#include <atomic>
#include <filesystem>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#ifndef UMBRA_SOURCE_DIRECTORY
#error "The regional provider-response recheck test requires the Umbra source directory."
#endif

namespace {

using rti1516_2025::AttributeHandle;
using rti1516_2025::AttributeHandleSet;
using rti1516_2025::AttributeHandleValueMap;
using rti1516_2025::AttributeHandleSetRegionHandleSetPairVector;
using rti1516_2025::DimensionHandleSet;
using rti1516_2025::FederateHandle;
using rti1516_2025::HLA_EVOKED;
using rti1516_2025::CANCEL_THEN_DELETE_THEN_DIVEST;
using rti1516_2025::NO_ACTION;
using rti1516_2025::ObjectClassHandle;
using rti1516_2025::ObjectInstanceHandle;
using rti1516_2025::RangeBounds;
using rti1516_2025::RegionHandleSet;
using rti1516_2025::RTIambassador;
using rti1516_2025::RTIambassadorFactory;
using rti1516_2025::TransportationTypeHandle;
using rti1516_2025::VariableLengthData;

class RegionalResponseAmbassador final
    : public rti1516_2025::NullFederateAmbassador {
 public:
  struct Reflection final {
    ObjectInstanceHandle objectInstance;
    AttributeHandleValueMap attributeValues;
    VariableLengthData userSuppliedTag;
    TransportationTypeHandle transportationType;
    FederateHandle producingFederate;
    bool sentRegionsSupplied = false;
    RegionHandleSet sentRegions;
  };

  void discoverObjectInstance(
      ObjectInstanceHandle const& objectInstance,
      ObjectClassHandle const& objectClass,
      std::wstring const& objectInstanceName,
      FederateHandle const& producingFederate) override {
    static_cast<void>(objectClass);
    static_cast<void>(objectInstanceName);
    static_cast<void>(producingFederate);
    discoveredObjects.push_back(objectInstance);
  }

  void provideAttributeValueUpdate(
      ObjectInstanceHandle const& objectInstance,
      AttributeHandleSet const& attributes,
      VariableLengthData const& userSuppliedTag) override {
    provideObjects.push_back(objectInstance);
    provideAttributes.push_back(attributes);
    provideTags.push_back(userSuppliedTag);
    if (provideHandler) {
      provideHandler(objectInstance, attributes, userSuppliedTag);
    }
  }

  void reflectAttributeValues(
      ObjectInstanceHandle const& objectInstance,
      AttributeHandleValueMap const& attributeValues,
      VariableLengthData const& userSuppliedTag,
      TransportationTypeHandle const& transportationType,
      FederateHandle const& producingFederate,
      RegionHandleSet const* optionalSentRegions) override {
    reflections.push_back({
        objectInstance,
        attributeValues,
        userSuppliedTag,
        transportationType,
        producingFederate,
        optionalSentRegions != nullptr,
        optionalSentRegions == nullptr ? RegionHandleSet{} : *optionalSentRegions,
    });
  }

  std::vector<ObjectInstanceHandle> discoveredObjects;
  std::vector<ObjectInstanceHandle> provideObjects;
  std::vector<AttributeHandleSet> provideAttributes;
  std::vector<VariableLengthData> provideTags;
  std::vector<Reflection> reflections;
  std::function<void(
      ObjectInstanceHandle const&,
      AttributeHandleSet const&,
      VariableLengthData const&)> provideHandler;
};

std::unique_ptr<RTIambassador> makeRti() {
  RTIambassadorFactory factory;
  return factory.createRTIambassador();
}

std::wstring nextFederationName() {
  static std::atomic_uint64_t sequence{0U};
  return L"regional-provider-response-recheck-" +
      std::to_wstring(sequence.fetch_add(1U, std::memory_order_relaxed));
}

std::wstring restaurantFom() {
  return (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
          "third_party" / "ieee1516.2-2025" / "resources" /
          "examples" / "RestaurantFOMmodule-2025.xml")
      .wstring();
}

void drain(RTIambassador& ambassador) {
  for (int pass = 0; pass != 64; ++pass) {
    static_cast<void>(ambassador.evokeCallback(0.0));
  }
}

std::vector<unsigned char> bytes(VariableLengthData const& value) {
  auto const* data = static_cast<unsigned char const*>(value.data());
  if (data == nullptr || value.size() == 0U) {
    return {};
  }
  return {data, data + value.size()};
}

}  // namespace

TEST_CASE(
    "Embedded regional provider response rechecks DDM eligibility at reflection delivery",
    "[integration][development-profile][federation-management][object-management][ddm]"
    "[regional-provider-response-ddm-recheck]"
    "[callback-evoked][receive-order]"
    "[rti.service.request-attribute-value-update-with-regions]"
    "[rti.service.set-range-bounds]"
    "[rti.service.commit-region-modifications]"
    "[rti.service.register-object-instance-with-regions]"
    "[rti.service.subscribe-object-class-attributes-with-regions]"
    "[rti.service.update-attribute-values]"
    "[federate.callback.provide-attribute-value-update]"
    "[federate.callback.reflect-attribute-values]") {
  RegionalResponseAmbassador ownerReports;
  RegionalResponseAmbassador requesterReports;
  auto owner = makeRti();
  auto requester = makeRti();
  auto const federationName = nextFederationName();
  unsigned char const requestTagBytes[] = {0x52, 0x45, 0x51};
  unsigned char const responseTagBytes[] = {0x52, 0x45, 0x53};
  unsigned char const responseValueBytes[] = {0x5A, 0x25, 0x07};
  VariableLengthData const requestTag(requestTagBytes, sizeof(requestTagBytes));
  std::vector<unsigned char> const responseTag(
      responseTagBytes,
      responseTagBytes + sizeof(responseTagBytes));
  std::vector<unsigned char> const responseValue(
      responseValueBytes,
      responseValueBytes + sizeof(responseValueBytes));

  REQUIRE_NOTHROW(owner->connect(ownerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(requester->connect(requesterReports, HLA_EVOKED));
  REQUIRE_NOTHROW(owner->createFederationExecution(
      federationName,
      restaurantFom(),
      L"HLAinteger64Time"));
  FederateHandle ownerHandle;
  REQUIRE_NOTHROW(ownerHandle = owner->joinFederationExecution(
      L"regional-response-recheck-owner",
      L"publisher",
      federationName));
  REQUIRE_NOTHROW(requester->joinFederationExecution(
      L"regional-response-recheck-requester",
      L"subscriber",
      federationName));

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
  REQUIRE(objectInstance.isValid());
  drain(*requester);
  REQUIRE(requesterReports.discoveredObjects ==
          std::vector<ObjectInstanceHandle>{objectInstance});

  // Drain the Restaurant FOM's automatic-provision setup callback before
  // installing the explicit response handler for this request.
  drain(*owner);
  auto const initialProvideCount = ownerReports.provideObjects.size();

  std::vector<unsigned char> observedRequestTag;
  ownerReports.provideHandler = [
      &owner,
      &observedRequestTag,
      responseValue,
      responseTag](ObjectInstanceHandle const& callbackObject,
                   AttributeHandleSet const& callbackAttributes,
                   VariableLengthData const& callbackTag) {
    observedRequestTag = bytes(callbackTag);
    AttributeHandleValueMap values;
    for (auto const& attribute : callbackAttributes) {
      values.emplace(
          attribute,
          VariableLengthData(responseValue.data(), responseValue.size()));
    }
    owner->updateAttributeValues(
        callbackObject,
        values,
        VariableLengthData(responseTag.data(), responseTag.size()));
  };

  REQUIRE_NOTHROW(requester->requestAttributeValueUpdateWithRegions(
      soda,
      requesterPair,
      requestTag));
  REQUIRE(ownerReports.provideObjects.size() == initialProvideCount);
  REQUIRE(requesterReports.reflections.empty());

  // The provider callback is queued under HLA_EVOKED. Its no-time response
  // is consequently queued for the requester and has not crossed the
  // recipient callback boundary yet.
  drain(*owner);
  REQUIRE(ownerReports.provideObjects.size() == initialProvideCount + 1U);
  REQUIRE(ownerReports.provideObjects.back() == objectInstance);
  REQUIRE(ownerReports.provideAttributes.back() == flavorOnly);
  REQUIRE(bytes(ownerReports.provideTags.back()) ==
          std::vector<unsigned char>(requestTagBytes,
                                     requestTagBytes + sizeof(requestTagBytes)));
  REQUIRE(observedRequestTag ==
          std::vector<unsigned char>(requestTagBytes,
                                     requestTagBytes + sizeof(requestTagBytes)));
  REQUIRE(requesterReports.reflections.empty());

  // Move the requester's committed subscription away after the response has
  // been queued but before its callback is evoked. Reflection delivery must
  // recheck the current DDM overlap and suppress the stale response.
  REQUIRE_NOTHROW(requester->setRangeBounds(
      requesterRegion,
      sodaFlavor,
      RangeBounds(2UL, 3UL)));
  REQUIRE_NOTHROW(requester->commitRegionModifications(
      RegionHandleSet{requesterRegion}));
  drain(*requester);
  REQUIRE(requesterReports.reflections.empty());

  REQUIRE_NOTHROW(requester->unsubscribeObjectClassAttributesWithRegions(
      soda,
      requesterPair));
  REQUIRE_NOTHROW(owner->unassociateRegionsForUpdates(objectInstance, ownerPair));
  REQUIRE_NOTHROW(requester->deleteRegion(requesterRegion));
  REQUIRE_NOTHROW(owner->deleteRegion(ownerRegion));
  REQUIRE_NOTHROW(requester->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->resignFederationExecution(CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(requester->disconnect());
  REQUIRE_NOTHROW(owner->disconnect());
}
