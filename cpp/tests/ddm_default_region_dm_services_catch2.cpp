#include <catch2/catch_test_macros.hpp>

#include "hla_test_names.hpp"

#include <RTI/NullFederateAmbassador.h>
#include <RTI/RTI1516.h>

#include <atomic>
#include <filesystem>
#include <memory>

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

std::unique_ptr<RTIambassador> makeRti() {
  RTIambassadorFactory factory;
  return factory.createRTIambassador();
}

std::filesystem::path regionalFom() {
  return std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" /
      "data" / "two-dimensional-regional-interaction-fom.xml";
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

  // §9.1.5 (continued on p. 223) says DM and DDM subscription effects do not
  // interact. The four DM operations use the default region independently of
  // the disjoint explicit regional declarations.
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
