#include <catch2/catch_test_macros.hpp>

#include "hla_test_names.hpp"

#include <RTI/NullFederateAmbassador.h>
#include <RTI/RTI1516.h>

#include <atomic>
#include <filesystem>
#include <memory>
#include <vector>

#ifndef UMBRA_SOURCE_DIRECTORY
#error "The default-region DM services test requires the Umbra source directory."
#endif

namespace {

namespace fixture_hla = umbra::test::hla::wide;

using rti1516_2025::AttributeHandle;
using rti1516_2025::AttributeHandleSet;
using rti1516_2025::AttributeHandleSetRegionHandleSetPairVector;
using rti1516_2025::AttributeHandleValueMap;
using rti1516_2025::DimensionHandle;
using rti1516_2025::DimensionHandleSet;
using rti1516_2025::FederateHandle;
using rti1516_2025::HLA_EVOKED;
using rti1516_2025::InteractionClassHandle;
using rti1516_2025::NO_ACTION;
using rti1516_2025::NullFederateAmbassador;
using rti1516_2025::ObjectClassHandle;
using rti1516_2025::ObjectInstanceHandle;
using rti1516_2025::ParameterHandleValueMap;
using rti1516_2025::RangeBounds;
using rti1516_2025::RegionHandle;
using rti1516_2025::RegionHandleSet;
using rti1516_2025::RTIambassador;
using rti1516_2025::RTIambassadorFactory;
using rti1516_2025::TransportationTypeHandle;
using rti1516_2025::VariableLengthData;

class RoutingReceiver final : public NullFederateAmbassador {
 public:
  void discoverObjectInstance(
      ObjectInstanceHandle const&,
      ObjectClassHandle const&,
      std::wstring const&,
      FederateHandle const&) override {
    ++discoveries;
  }

  void reflectAttributeValues(
      ObjectInstanceHandle const&,
      AttributeHandleValueMap const&,
      VariableLengthData const&,
      TransportationTypeHandle const&,
      FederateHandle const&,
      RegionHandleSet const*) override {
    ++reflections;
  }

  void receiveInteraction(
      InteractionClassHandle const&,
      ParameterHandleValueMap const&,
      VariableLengthData const&,
      TransportationTypeHandle const&,
      FederateHandle const&,
      RegionHandleSet const*) override {
    ++interactions;
  }

  std::size_t discoveries = 0U;
  std::size_t reflections = 0U;
  std::size_t interactions = 0U;
};

class ClassIsolationReceiver final : public NullFederateAmbassador {
 public:
  struct Reflection final {
    ObjectInstanceHandle objectInstance;
    AttributeHandleValueMap values;
  };

  void discoverObjectInstance(
      ObjectInstanceHandle const& objectInstance,
      ObjectClassHandle const& objectClass,
      std::wstring const&,
      FederateHandle const&) override {
    discoveries.emplace_back(objectInstance, objectClass);
  }

  void reflectAttributeValues(
      ObjectInstanceHandle const& objectInstance,
      AttributeHandleValueMap const& values,
      VariableLengthData const&,
      TransportationTypeHandle const&,
      FederateHandle const&,
      RegionHandleSet const*) override {
    reflections.push_back({objectInstance, values});
  }

  std::vector<std::pair<ObjectInstanceHandle, ObjectClassHandle>> discoveries;
  std::vector<Reflection> reflections;
};

std::unique_ptr<RTIambassador> makeRti() {
  RTIambassadorFactory factory;
  return factory.createRTIambassador();
}

std::filesystem::path regionalFom() {
  return std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" /
      "data" / "two-dimensional-regional-interaction-fom.xml";
}

std::filesystem::path classIsolationFom() {
  return std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" /
      "data" / "two-dimensional-regional-class-isolation-fom.xml";
}

std::wstring nextFederationName() {
  static std::atomic_uint64_t sequence{0U};
  return L"ddm-default-region-dm-services-" +
      std::to_wstring(sequence.fetch_add(1U, std::memory_order_relaxed));
}

void drainCallbacks(RTIambassador& rti) {
  for (int pass = 0; pass != 128; ++pass) {
    if (!rti.evokeCallback(0.0)) {
      break;
    }
  }
}

void setBounds(
    RTIambassador& rti,
    RegionHandle const& region,
    DimensionHandle const& x,
    DimensionHandle const& y,
    RangeBounds const& xBounds,
    RangeBounds const& yBounds) {
  rti.setRangeBounds(region, x, xBounds);
  rti.setRangeBounds(region, y, yBounds);
  rti.commitRegionModifications(RegionHandleSet{region});
}

}  // namespace

TEST_CASE(
    "Embedded 2025 DM subscribe and unsubscribe services remain independent of regional declarations",
    "[integration][development-profile][object-management][interaction-management]"
    "[ddm][ddm-default-region-dm-reinterpretation]"
    "[rti.service.subscribe-object-class-attributes]"
    "[rti.service.unsubscribe-object-class-attributes]"
    "[rti.service.subscribe-object-class-attributes-with-regions]"
    "[rti.service.unsubscribe-object-class-attributes-with-regions]"
    "[rti.service.subscribe-interaction-class]"
    "[rti.service.unsubscribe-interaction-class]"
    "[rti.service.subscribe-interaction-class-with-regions]"
    "[rti.service.unsubscribe-interaction-class-with-regions]"
    "[rti.service.register-object-instance-with-regions]"
    "[rti.service.update-attribute-values]"
    "[rti.service.send-interaction-with-regions]"
    "[federate.callback.discover-object-instance]"
    "[federate.callback.reflect-attribute-values]"
    "[federate.callback.receive-interaction][callback-evoked][2025]") {
  NullFederateAmbassador publisherCallbacks;
  RoutingReceiver subscriberCallbacks;
  auto publisher = makeRti();
  auto subscriber = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = regionalFom().wstring();

  REQUIRE_NOTHROW(publisher->connect(publisherCallbacks, HLA_EVOKED));
  REQUIRE_NOTHROW(subscriber->connect(subscriberCallbacks, HLA_EVOKED));
  REQUIRE_NOTHROW(publisher->createFederationExecution(
      federationName,
      fomModule,
      L"HLAinteger64Time"));
  REQUIRE_NOTHROW(publisher->joinFederationExecution(
      L"ddm-default-region-dm-publisher", L"publisher", federationName));
  REQUIRE_NOTHROW(subscriber->joinFederationExecution(
      L"ddm-default-region-dm-subscriber", L"subscriber", federationName));

  auto const objectClass = publisher->getObjectClassHandle(
      fixture_hla::fom::two_dimensional_regional_object);
  auto const attribute = publisher->getAttributeHandle(
      objectClass,
      fixture_hla::fixture::value);
  auto const interactionClass = publisher->getInteractionClassHandle(
      fixture_hla::fom::two_dimensional_regional_interaction);
  auto const publisherX = publisher->getDimensionHandle(fixture_hla::fixture::umbra_region_x);
  auto const publisherY = publisher->getDimensionHandle(fixture_hla::fixture::umbra_region_y);
  auto const subscriberObjectClass = subscriber->getObjectClassHandle(
      fixture_hla::fom::two_dimensional_regional_object);
  auto const subscriberAttribute = subscriber->getAttributeHandle(
      subscriberObjectClass,
      fixture_hla::fixture::value);
  auto const subscriberInteractionClass = subscriber->getInteractionClassHandle(
      fixture_hla::fom::two_dimensional_regional_interaction);
  auto const subscriberX = subscriber->getDimensionHandle(fixture_hla::fixture::umbra_region_x);
  auto const subscriberY = subscriber->getDimensionHandle(fixture_hla::fixture::umbra_region_y);
  REQUIRE(objectClass.isValid());
  REQUIRE(attribute.isValid());
  REQUIRE(interactionClass.isValid());
  REQUIRE(publisherX.isValid());
  REQUIRE(publisherY.isValid());
  REQUIRE(subscriberObjectClass.isValid());
  REQUIRE(subscriberAttribute.isValid());
  REQUIRE(subscriberInteractionClass.isValid());
  REQUIRE(subscriberX.isValid());
  REQUIRE(subscriberY.isValid());

  AttributeHandleSet const publishedAttributes{attribute};
  REQUIRE_NOTHROW(publisher->publishObjectClassAttributes(
      objectClass,
      publishedAttributes));
  REQUIRE_NOTHROW(publisher->publishInteractionClass(interactionClass));

  auto const sourceRegion = publisher->createRegion(
      DimensionHandleSet{publisherX, publisherY});
  auto const subscriptionRegion = subscriber->createRegion(
      DimensionHandleSet{subscriberX, subscriberY});
  REQUIRE(sourceRegion.isValid());
  REQUIRE(subscriptionRegion.isValid());
  setBounds(
      *publisher,
      sourceRegion,
      publisherX,
      publisherY,
      RangeBounds(0UL, 2UL),
      RangeBounds(0UL, 2UL));
  // The regional declaration overlaps the explicit source on X but is
  // disjoint on Y. The implicit default region still overlaps this
  // positive-dimensional source.
  setBounds(
      *subscriber,
      subscriptionRegion,
      subscriberX,
      subscriberY,
      RangeBounds(1UL, 3UL),
      RangeBounds(4UL, 6UL));

  AttributeHandleSetRegionHandleSetPairVector const sourcePair{{
      publishedAttributes,
      RegionHandleSet{sourceRegion},
  }};
  AttributeHandleSetRegionHandleSetPairVector const subscriptionPair{{
      AttributeHandleSet{subscriberAttribute},
      RegionHandleSet{subscriptionRegion},
  }};
  REQUIRE_NOTHROW(subscriber->subscribeObjectClassAttributesWithRegions(
      subscriberObjectClass,
      subscriptionPair));
  REQUIRE_NOTHROW(subscriber->subscribeInteractionClassWithRegions(
      subscriberInteractionClass,
      RegionHandleSet{subscriptionRegion}));

  // §9.1.5 p. 222 l. 17 calls for expanded DM interpretations when used with
  // DDM services. This case is one concrete subscription/routing example, not
  // evidence for every Clause 6 definition, constraint, or service.
  // §9.1.5 p. 222 l. 89 says a DDM service cannot substitute for the
  // corresponding DM service through the default region. These no-region DM
  // calls remain distinct from the disjoint explicit regional declarations;
  // removing the DM subscriptions below leaves those declarations ineffective
  // until their ranges explicitly overlap the source.
  REQUIRE_NOTHROW(subscriber->subscribeObjectClassAttributes(
      subscriberObjectClass,
      AttributeHandleSet{subscriberAttribute},
      true));
  REQUIRE_NOTHROW(subscriber->subscribeInteractionClass(subscriberInteractionClass));

  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = publisher->registerObjectInstanceWithRegions(
      objectClass,
      sourcePair));
  drainCallbacks(*subscriber);
  REQUIRE(subscriberCallbacks.discoveries == 1U);

  unsigned char const valueBytes[] = {0xA1, 0x03};
  AttributeHandleValueMap const values{{
      attribute,
      VariableLengthData(valueBytes, sizeof(valueBytes)),
  }};
  VariableLengthData const tag;
  REQUIRE_NOTHROW(publisher->updateAttributeValues(
      objectInstance,
      values,
      tag));
  REQUIRE_NOTHROW(publisher->sendInteractionWithRegions(
      interactionClass,
      ParameterHandleValueMap{},
      RegionHandleSet{sourceRegion},
      tag));
  drainCallbacks(*subscriber);
  REQUIRE(subscriberCallbacks.reflections == 1U);
  REQUIRE(subscriberCallbacks.interactions == 1U);

  REQUIRE_NOTHROW(subscriber->unsubscribeObjectClassAttributes(
      subscriberObjectClass,
      AttributeHandleSet{subscriberAttribute}));
  REQUIRE_NOTHROW(subscriber->unsubscribeInteractionClass(subscriberInteractionClass));
  REQUIRE_NOTHROW(publisher->updateAttributeValues(objectInstance, values, tag));
  REQUIRE_NOTHROW(publisher->sendInteractionWithRegions(
      interactionClass,
      ParameterHandleValueMap{},
      RegionHandleSet{sourceRegion},
      tag));
  drainCallbacks(*subscriber);
  REQUIRE(subscriberCallbacks.reflections == 1U);
  REQUIRE(subscriberCallbacks.interactions == 1U);

  // Ordinary unsubscribe leaves the With Regions declarations intact: moving
  // the same source into their explicit overlap makes both callbacks eligible.
  setBounds(
      *publisher,
      sourceRegion,
      publisherX,
      publisherY,
      RangeBounds(1UL, 3UL),
      RangeBounds(4UL, 6UL));
  REQUIRE_NOTHROW(publisher->updateAttributeValues(objectInstance, values, tag));
  REQUIRE_NOTHROW(publisher->sendInteractionWithRegions(
      interactionClass,
      ParameterHandleValueMap{},
      RegionHandleSet{sourceRegion},
      tag));
  drainCallbacks(*subscriber);
  REQUIRE(subscriberCallbacks.reflections == 2U);
  REQUIRE(subscriberCallbacks.interactions == 2U);

  REQUIRE_NOTHROW(publisher->unassociateRegionsForUpdates(objectInstance, sourcePair));
  REQUIRE_NOTHROW(subscriber->unsubscribeObjectClassAttributesWithRegions(
      subscriberObjectClass,
      subscriptionPair));
  REQUIRE_NOTHROW(subscriber->unsubscribeInteractionClassWithRegions(
      subscriberInteractionClass,
      RegionHandleSet{subscriptionRegion}));
  REQUIRE_NOTHROW(publisher->deleteRegion(sourceRegion));
  REQUIRE_NOTHROW(subscriber->deleteRegion(subscriptionRegion));
  REQUIRE_NOTHROW(subscriber->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(subscriber->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
}

TEST_CASE(
    "Embedded regional unsubscribe for one class-region-attribute triple preserves another triple",
    "[integration][development-profile][object-management][ddm]"
    "[ddm-region-triple-non-interference][2025]"
    "[rti.service.publish-object-class-attributes]"
    "[rti.service.subscribe-object-class-attributes-with-regions]"
    "[rti.service.unsubscribe-object-class-attributes-with-regions]"
    "[rti.service.register-object-instance-with-regions]"
    "[rti.service.update-attribute-values]"
    "[federate.callback.reflect-attribute-values]") {
  ClassIsolationReceiver publisherCallbacks;
  ClassIsolationReceiver subscriberCallbacks;
  auto publisher = makeRti();
  auto subscriber = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = classIsolationFom().wstring();

  REQUIRE_NOTHROW(publisher->connect(publisherCallbacks, HLA_EVOKED));
  REQUIRE_NOTHROW(subscriber->connect(subscriberCallbacks, HLA_EVOKED));
  REQUIRE_NOTHROW(publisher->createFederationExecution(
      federationName,
      fomModule,
      L"HLAinteger64Time"));
  REQUIRE_NOTHROW(publisher->joinFederationExecution(
      L"ddm-triple-isolation-publisher", L"publisher", federationName));
  REQUIRE_NOTHROW(subscriber->joinFederationExecution(
      L"ddm-triple-isolation-subscriber", L"subscriber", federationName));

  auto const publisherObjectClass = publisher->getObjectClassHandle(
      L"HLAobjectRoot.UmbraTwoDimensionalRegionObjectPeer");
  auto const subscriberObjectClass = subscriber->getObjectClassHandle(
      L"HLAobjectRoot.UmbraTwoDimensionalRegionObjectPeer");
  auto const publisherValue = publisher->getAttributeHandle(
      publisherObjectClass, L"Value");
  auto const publisherAuxiliaryValue = publisher->getAttributeHandle(
      publisherObjectClass, L"AuxiliaryValue");
  auto const subscriberValue = subscriber->getAttributeHandle(
      subscriberObjectClass, L"Value");
  auto const subscriberAuxiliaryValue = subscriber->getAttributeHandle(
      subscriberObjectClass, L"AuxiliaryValue");
  auto const publisherX = publisher->getDimensionHandle(L"UmbraRegionX");
  auto const publisherY = publisher->getDimensionHandle(L"UmbraRegionY");
  auto const subscriberX = subscriber->getDimensionHandle(L"UmbraRegionX");
  auto const subscriberY = subscriber->getDimensionHandle(L"UmbraRegionY");
  REQUIRE(publisherObjectClass.isValid());
  REQUIRE(subscriberObjectClass.isValid());
  REQUIRE(publisherValue.isValid());
  REQUIRE(publisherAuxiliaryValue.isValid());
  REQUIRE(subscriberValue.isValid());
  REQUIRE(subscriberAuxiliaryValue.isValid());
  REQUIRE(publisherX.isValid());
  REQUIRE(publisherY.isValid());
  REQUIRE(subscriberX.isValid());
  REQUIRE(subscriberY.isValid());

  AttributeHandleSet const publishedAttributes{
      publisherValue,
      publisherAuxiliaryValue,
  };
  REQUIRE_NOTHROW(publisher->publishObjectClassAttributes(
      publisherObjectClass,
      publishedAttributes));

  auto const sourceRegion = publisher->createRegion(
      DimensionHandleSet{publisherX, publisherY});
  auto const subscriptionRegion = subscriber->createRegion(
      DimensionHandleSet{subscriberX, subscriberY});
  REQUIRE(sourceRegion.isValid());
  REQUIRE(subscriptionRegion.isValid());
  setBounds(
      *publisher,
      sourceRegion,
      publisherX,
      publisherY,
      RangeBounds(1UL, 3UL),
      RangeBounds(1UL, 3UL));
  setBounds(
      *subscriber,
      subscriptionRegion,
      subscriberX,
      subscriberY,
      RangeBounds(1UL, 3UL),
      RangeBounds(1UL, 3UL));

  auto const sourcePair = AttributeHandleSetRegionHandleSetPairVector{
      {AttributeHandleSet{publisherValue}, RegionHandleSet{sourceRegion}},
      {AttributeHandleSet{publisherAuxiliaryValue}, RegionHandleSet{sourceRegion}},
  };
  auto const subscriptionPair = AttributeHandleSetRegionHandleSetPairVector{
      {AttributeHandleSet{subscriberValue}, RegionHandleSet{subscriptionRegion}},
      {AttributeHandleSet{subscriberAuxiliaryValue}, RegionHandleSet{subscriptionRegion}},
  };
  auto const auxiliaryPair = AttributeHandleSetRegionHandleSetPairVector{{
      AttributeHandleSet{subscriberAuxiliaryValue},
      RegionHandleSet{subscriptionRegion},
  }};
  REQUIRE_NOTHROW(subscriber->subscribeObjectClassAttributesWithRegions(
      subscriberObjectClass,
      subscriptionPair,
      true));

  ObjectInstanceHandle const objectInstance =
      publisher->registerObjectInstanceWithRegions(publisherObjectClass, sourcePair);
  REQUIRE(objectInstance.isValid());
  drainCallbacks(*subscriber);
  REQUIRE(subscriberCallbacks.discoveries.size() == 1U);
  REQUIRE(subscriberCallbacks.discoveries.front().first == objectInstance);
  REQUIRE(subscriberCallbacks.discoveries.front().second == subscriberObjectClass);

  unsigned char const valueBytes[] = {0xC3, 0x01};
  unsigned char const auxiliaryBytes[] = {0xD4, 0x02};
  auto updateBothAttributes = [&] {
    REQUIRE_NOTHROW(publisher->updateAttributeValues(
        objectInstance,
        AttributeHandleValueMap{
            {publisherValue, VariableLengthData(valueBytes, sizeof(valueBytes))},
            {publisherAuxiliaryValue, VariableLengthData(auxiliaryBytes, sizeof(auxiliaryBytes))},
        },
        VariableLengthData{}));
    drainCallbacks(*subscriber);
  };
  auto hasReflection = [](
      std::vector<ClassIsolationReceiver::Reflection> const& reports,
      ObjectInstanceHandle const& reflectedObject,
      AttributeHandle const& attribute) {
    for (auto const& report : reports) {
      if (report.objectInstance == reflectedObject &&
          report.values.contains(attribute)) {
        return true;
      }
    }
    return false;
  };

  updateBothAttributes();
  REQUIRE(hasReflection(
      subscriberCallbacks.reflections, objectInstance, subscriberValue));
  REQUIRE(hasReflection(
      subscriberCallbacks.reflections, objectInstance, subscriberAuxiliaryValue));

  subscriberCallbacks.reflections.clear();
  // §9.1.5 p. 223 l. 76: removing the (class, region, Value) triple must not
  // alter the distinct (class, region, AuxiliaryValue) subscription.
  REQUIRE_NOTHROW(subscriber->unsubscribeObjectClassAttributesWithRegions(
      subscriberObjectClass,
      AttributeHandleSetRegionHandleSetPairVector{{
          AttributeHandleSet{subscriberValue},
          RegionHandleSet{subscriptionRegion},
      }}));
  updateBothAttributes();
  REQUIRE_FALSE(hasReflection(
      subscriberCallbacks.reflections, objectInstance, subscriberValue));
  REQUIRE(hasReflection(
      subscriberCallbacks.reflections, objectInstance, subscriberAuxiliaryValue));

  REQUIRE_NOTHROW(publisher->unassociateRegionsForUpdates(objectInstance, sourcePair));
  REQUIRE_NOTHROW(subscriber->unsubscribeObjectClassAttributesWithRegions(
      subscriberObjectClass,
      auxiliaryPair));
  REQUIRE_NOTHROW(publisher->unpublishObjectClassAttributes(
      publisherObjectClass,
      publishedAttributes));
  REQUIRE_NOTHROW(publisher->deleteRegion(sourceRegion));
  REQUIRE_NOTHROW(subscriber->deleteRegion(subscriptionRegion));
  REQUIRE_NOTHROW(subscriber->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(subscriber->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
}

TEST_CASE(
    "Embedded DDM unsubscribe for one object class preserves another class regional subscription",
    "[integration][development-profile][object-management][ddm]"
    "[ddm-unsubscribe-object-class-isolation][2025]"
    "[rti.service.publish-object-class-attributes]"
    "[rti.service.subscribe-object-class-attributes]"
    "[rti.service.subscribe-object-class-attributes-with-regions]"
    "[rti.service.unsubscribe-object-class-attributes]"
    "[rti.service.unsubscribe-object-class-attributes-with-regions]"
    "[rti.service.register-object-instance]"
    "[rti.service.register-object-instance-with-regions]"
    "[rti.service.update-attribute-values]"
    "[rti.service.reflect-attribute-values]") {
  ClassIsolationReceiver publisherCallbacks;
  ClassIsolationReceiver subscriberCallbacks;
  auto publisher = makeRti();
  auto subscriber = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = classIsolationFom().wstring();

  REQUIRE_NOTHROW(publisher->connect(publisherCallbacks, HLA_EVOKED));
  REQUIRE_NOTHROW(subscriber->connect(subscriberCallbacks, HLA_EVOKED));
  REQUIRE_NOTHROW(publisher->createFederationExecution(
      federationName,
      fomModule,
      L"HLAinteger64Time"));
  REQUIRE_NOTHROW(publisher->joinFederationExecution(
      L"ddm-class-isolation-publisher", L"publisher", federationName));
  REQUIRE_NOTHROW(subscriber->joinFederationExecution(
      L"ddm-class-isolation-subscriber", L"subscriber", federationName));

  auto const publisherDmClass = publisher->getObjectClassHandle(
      L"HLAobjectRoot.UmbraTwoDimensionalRegionObject");
  auto const publisherDdmClass = publisher->getObjectClassHandle(
      L"HLAobjectRoot.UmbraTwoDimensionalRegionObjectPeer");
  auto const subscriberDmClass = subscriber->getObjectClassHandle(
      L"HLAobjectRoot.UmbraTwoDimensionalRegionObject");
  auto const subscriberDdmClass = subscriber->getObjectClassHandle(
      L"HLAobjectRoot.UmbraTwoDimensionalRegionObjectPeer");
  auto const publisherDmAttribute = publisher->getAttributeHandle(
      publisherDmClass, L"Value");
  auto const publisherDdmAttribute = publisher->getAttributeHandle(
      publisherDdmClass, L"Value");
  auto const subscriberDmAttribute = subscriber->getAttributeHandle(
      subscriberDmClass, L"Value");
  auto const subscriberDdmAttribute = subscriber->getAttributeHandle(
      subscriberDdmClass, L"Value");
  auto const publisherX = publisher->getDimensionHandle(L"UmbraRegionX");
  auto const publisherY = publisher->getDimensionHandle(L"UmbraRegionY");
  auto const subscriberX = subscriber->getDimensionHandle(L"UmbraRegionX");
  auto const subscriberY = subscriber->getDimensionHandle(L"UmbraRegionY");
  REQUIRE(publisherDmClass.isValid());
  REQUIRE(publisherDdmClass.isValid());
  REQUIRE(subscriberDmClass.isValid());
  REQUIRE(subscriberDdmClass.isValid());
  REQUIRE(publisherDmAttribute.isValid());
  REQUIRE(publisherDdmAttribute.isValid());
  REQUIRE(subscriberDmAttribute.isValid());
  REQUIRE(subscriberDdmAttribute.isValid());
  REQUIRE(publisherX.isValid());
  REQUIRE(publisherY.isValid());
  REQUIRE(subscriberX.isValid());
  REQUIRE(subscriberY.isValid());

  REQUIRE_NOTHROW(publisher->publishObjectClassAttributes(
      publisherDmClass,
      AttributeHandleSet{publisherDmAttribute}));
  REQUIRE_NOTHROW(publisher->publishObjectClassAttributes(
      publisherDdmClass,
      AttributeHandleSet{publisherDdmAttribute}));

  auto const sourceRegion = publisher->createRegion(
      DimensionHandleSet{publisherX, publisherY});
  auto const subscriptionRegion = subscriber->createRegion(
      DimensionHandleSet{subscriberX, subscriberY});
  REQUIRE(sourceRegion.isValid());
  REQUIRE(subscriptionRegion.isValid());
  setBounds(
      *publisher,
      sourceRegion,
      publisherX,
      publisherY,
      RangeBounds(1UL, 3UL),
      RangeBounds(1UL, 3UL));
  setBounds(
      *subscriber,
      subscriptionRegion,
      subscriberX,
      subscriberY,
      RangeBounds(1UL, 3UL),
      RangeBounds(1UL, 3UL));

  AttributeHandleSetRegionHandleSetPairVector const sourcePair{{
      AttributeHandleSet{publisherDdmAttribute},
      RegionHandleSet{sourceRegion},
  }};
  AttributeHandleSetRegionHandleSetPairVector const subscriptionPair{{
      AttributeHandleSet{subscriberDdmAttribute},
      RegionHandleSet{subscriptionRegion},
  }};

  ObjectInstanceHandle const dmObject =
      publisher->registerObjectInstance(publisherDmClass);
  ObjectInstanceHandle const ddmObject =
      publisher->registerObjectInstanceWithRegions(publisherDdmClass, sourcePair);
  REQUIRE(dmObject.isValid());
  REQUIRE(ddmObject.isValid());
  REQUIRE(dmObject != ddmObject);
  drainCallbacks(*subscriber);
  // §9.1.5 p. 223 l. 118: neither class is subscribed until at least one of
  // its attributes is subscribed, whether the declaration will be ordinary
  // or regional.
  REQUIRE(subscriberCallbacks.discoveries.empty());

  REQUIRE_NOTHROW(subscriber->subscribeObjectClassAttributes(
      subscriberDmClass,
      AttributeHandleSet{subscriberDmAttribute},
      true));
  REQUIRE_NOTHROW(subscriber->subscribeObjectClassAttributesWithRegions(
      subscriberDdmClass,
      subscriptionPair,
      true));
  drainCallbacks(*subscriber);
  REQUIRE(subscriberCallbacks.discoveries.size() == 2U);
  auto hasDiscoveryFor = [&](ObjectInstanceHandle const& instance,
                             ObjectClassHandle const& objectClass) {
    for (auto const& discovery : subscriberCallbacks.discoveries) {
      if (discovery.first == instance && discovery.second == objectClass) {
        return true;
      }
    }
    return false;
  };
  // §9.1.5 p. 223 l. 145: discovery preserves the class supplied to each
  // registration form, including Register Object Instance With Regions.
  REQUIRE(hasDiscoveryFor(dmObject, subscriberDmClass));
  REQUIRE(hasDiscoveryFor(ddmObject, subscriberDdmClass));

  unsigned char const dmBytes[] = {0xA1, 0x01};
  unsigned char const ddmBytes[] = {0xB2, 0x02};
  VariableLengthData const tag;
  auto sendBoth = [&] {
    REQUIRE_NOTHROW(publisher->updateAttributeValues(
        dmObject,
        AttributeHandleValueMap{{
            publisherDmAttribute,
            VariableLengthData(dmBytes, sizeof(dmBytes)),
        }},
        tag));
    // The object was registered with the DDM region association, so this
    // ordinary update follows that association while using the public DM API.
    REQUIRE_NOTHROW(publisher->updateAttributeValues(
        ddmObject,
        AttributeHandleValueMap{{
            publisherDdmAttribute,
            VariableLengthData(ddmBytes, sizeof(ddmBytes)),
        }},
        tag));
    drainCallbacks(*subscriber);
  };
  auto hasReflection = [](
      std::vector<ClassIsolationReceiver::Reflection> const& reports,
      ObjectInstanceHandle const& objectInstance,
      AttributeHandle const& attribute) {
    for (auto const& report : reports) {
      if (report.objectInstance == objectInstance &&
          report.values.contains(attribute)) {
        return true;
      }
    }
    return false;
  };

  sendBoth();
  REQUIRE(subscriberCallbacks.reflections.size() == 2U);
  REQUIRE(hasReflection(
      subscriberCallbacks.reflections, dmObject, subscriberDmAttribute));
  REQUIRE(hasReflection(
      subscriberCallbacks.reflections, ddmObject, subscriberDdmAttribute));

  subscriberCallbacks.reflections.clear();
  // §9.1.5 p. 223 l. 52: unsubscribing the ordinary DM class removes only
  // that class's attribute subscription; the other class's regional DDM
  // subscription continues to receive its matching update.
  REQUIRE_NOTHROW(subscriber->unsubscribeObjectClassAttributes(
      subscriberDmClass,
      AttributeHandleSet{subscriberDmAttribute}));
  sendBoth();
  REQUIRE(subscriberCallbacks.reflections.size() == 1U);
  REQUIRE_FALSE(hasReflection(
      subscriberCallbacks.reflections, dmObject, subscriberDmAttribute));
  REQUIRE(hasReflection(
      subscriberCallbacks.reflections, ddmObject, subscriberDdmAttribute));

  REQUIRE_NOTHROW(publisher->unassociateRegionsForUpdates(ddmObject, sourcePair));
  REQUIRE_NOTHROW(subscriber->unsubscribeObjectClassAttributesWithRegions(
      subscriberDdmClass,
      subscriptionPair));
  REQUIRE_NOTHROW(publisher->unpublishObjectClassAttributes(
      publisherDmClass,
      AttributeHandleSet{publisherDmAttribute}));
  REQUIRE_NOTHROW(publisher->unpublishObjectClassAttributes(
      publisherDdmClass,
      AttributeHandleSet{publisherDdmAttribute}));
  REQUIRE_NOTHROW(publisher->deleteRegion(sourceRegion));
  REQUIRE_NOTHROW(subscriber->deleteRegion(subscriptionRegion));
  REQUIRE_NOTHROW(subscriber->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(subscriber->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
}
