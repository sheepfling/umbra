#include <catch2/catch_test_macros.hpp>

#include <RTI/NullFederateAmbassador.h>
#include <RTI/RTI1516.h>
#include <RTI/encoding/BasicDataElements.h>

#include <atomic>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#ifndef UMBRA_PACKAGE_ATTRIBUTE_FILTER_FOM
#error "The package-consumer subscription-validity test requires its FOM fixture path."
#endif

namespace {

using namespace rti1516_2025;

struct Discovery final {
  ObjectInstanceHandle object;
  ObjectClassHandle objectClass;
};

struct Reflection final {
  ObjectInstanceHandle object;
  AttributeHandleValueMap values;
};

class Reports final : public NullFederateAmbassador {
 public:
  void discoverObjectInstance(
      ObjectInstanceHandle const& object,
      ObjectClassHandle const& objectClass,
      std::wstring const&,
      FederateHandle const&) override {
    discoveries.push_back({object, objectClass});
  }

  void reflectAttributeValues(
      ObjectInstanceHandle const& object,
      AttributeHandleValueMap const& values,
      VariableLengthData const&,
      TransportationTypeHandle const&,
      FederateHandle const&,
      RegionHandleSet const*) override {
    reflections.push_back({object, values});
  }

  std::vector<Discovery> discoveries;
  std::vector<Reflection> reflections;
};

std::wstring nextFederationName() {
  static std::atomic_uint64_t sequence{0U};
  return L"package-subscription-validity-" +
      std::to_wstring(sequence.fetch_add(1U, std::memory_order_relaxed));
}

std::unique_ptr<RTIambassador> makeRti() {
  RTIambassadorFactory factory;
  return factory.createRTIambassador();
}

void drainCallbacks(RTIambassador& rti) {
  while (rti.evokeCallback(0.0)) {
  }
}

}  // namespace

TEST_CASE(
    "Installed package validates subscription sets and reports closest subscribed superclass",
    "[package-consumer][subscribe-object-class-attributes-attribute-set-validity]"
    "[rti.service.subscribe-object-class-attributes]"
    "[federate.callback.discover-object-instance]"
    "[federate.callback.reflect-attribute-values]") {
  Reports publisherReports;
  Reports subscriberReports;
  auto publisher = makeRti();
  auto subscriber = makeRti();
  auto const federationName = nextFederationName();
  std::wstring const fom =
      std::filesystem::path(UMBRA_PACKAGE_ATTRIBUTE_FILTER_FOM).wstring();

  publisher->connect(publisherReports, HLA_EVOKED);
  subscriber->connect(subscriberReports, HLA_EVOKED);
  publisher->createFederationExecution(federationName, fom, L"HLAinteger64Time");
  publisher->joinFederationExecution(L"package-validity-publisher", L"publisher", federationName);
  auto const publisherBase = publisher->getObjectClassHandle(
      L"HLAobjectRoot.UmbraAttributeFixtureBase");
  auto const publisherChild = publisher->getObjectClassHandle(
      L"HLAobjectRoot.UmbraAttributeFixtureBase.UmbraAttributeFixtureChild");
  auto const publisherBaseAttribute = publisher->getAttributeHandle(publisherBase, L"ReliableBaseA");
  auto const publisherChildAttribute = publisher->getAttributeHandle(publisherChild, L"ReliableChild");
  REQUIRE(publisherBase.isValid());
  REQUIRE(publisherChild.isValid());
  REQUIRE(publisherBaseAttribute.isValid());
  REQUIRE(publisherChildAttribute.isValid());
  publisher->publishObjectClassAttributes(
      publisherChild, AttributeHandleSet{publisherBaseAttribute, publisherChildAttribute});
  auto const object = publisher->registerObjectInstance(publisherChild);
  REQUIRE(object.isValid());
  HLAunicodeString baseValue{L"base-value"};
  HLAunicodeString childValue{L"child-value"};
  publisher->updateAttributeValues(
      object,
      AttributeHandleValueMap{
          {publisherBaseAttribute, baseValue.encode()},
          {publisherChildAttribute, childValue.encode()},
      },
      VariableLengthData{});

  subscriber->joinFederationExecution(L"package-validity-subscriber", L"subscriber", federationName);
  auto const subscriberBase = subscriber->getObjectClassHandle(
      L"HLAobjectRoot.UmbraAttributeFixtureBase");
  auto const subscriberChild = subscriber->getObjectClassHandle(
      L"HLAobjectRoot.UmbraAttributeFixtureBase.UmbraAttributeFixtureChild");
  auto const subscriberBaseAttribute = subscriber->getAttributeHandle(subscriberBase, L"ReliableBaseA");
  auto const subscriberChildAttribute = subscriber->getAttributeHandle(subscriberChild, L"ReliableChild");
  REQUIRE(subscriberBase.isValid());
  REQUIRE(subscriberChild.isValid());
  REQUIRE(subscriberBaseAttribute.isValid());
  REQUIRE(subscriberChildAttribute.isValid());
  drainCallbacks(*subscriber);
  REQUIRE(subscriberReports.discoveries.empty());
  REQUIRE(subscriberReports.reflections.empty());

  subscriber->subscribeObjectClassAttributes(subscriberBase, AttributeHandleSet{}, true);
  drainCallbacks(*subscriber);
  CHECK(subscriberReports.discoveries.empty());
  CHECK(subscriberReports.reflections.empty());
  CHECK_THROWS_AS(
      subscriber->subscribeObjectClassAttributes(
          subscriberBase, AttributeHandleSet{subscriberChildAttribute}, true),
      AttributeNotDefined);
  CHECK(subscriberReports.discoveries.empty());
  CHECK(subscriberReports.reflections.empty());

  subscriber->subscribeObjectClassAttributes(
      subscriberBase, AttributeHandleSet{subscriberBaseAttribute}, true);
  drainCallbacks(*subscriber);
  REQUIRE(subscriberReports.discoveries.size() == 1U);
  CHECK(subscriberReports.discoveries.front().object == object);
  CHECK(subscriberReports.discoveries.front().objectClass == subscriberBase);
  REQUIRE(subscriberReports.reflections.size() == 1U);
  auto const& reflection = subscriberReports.reflections.front();
  CHECK(reflection.object == object);
  REQUIRE(reflection.values.size() == 1U);
  REQUIRE(reflection.values.contains(subscriberBaseAttribute));
  CHECK_FALSE(reflection.values.contains(subscriberChildAttribute));
  HLAunicodeString decodedValue;
  decodedValue.decode(reflection.values.at(subscriberBaseAttribute));
  CHECK(decodedValue.get() == L"base-value");

  subscriber->unsubscribeObjectClassAttributes(
      subscriberBase, AttributeHandleSet{subscriberBaseAttribute});
  subscriber->resignFederationExecution(NO_ACTION);
  publisher->resignFederationExecution(CANCEL_THEN_DELETE_THEN_DIVEST);
  publisher->destroyFederationExecution(federationName);
  subscriber->disconnect();
  publisher->disconnect();
}
