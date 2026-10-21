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
#error "The package-consumer attribute-filter test requires its FOM fixture path."
#endif

namespace {

using namespace rti1516_2025;

struct Reflection final {
  ObjectInstanceHandle object;
  AttributeHandleValueMap values;
};

class Reports final : public NullFederateAmbassador {
 public:
  void reflectAttributeValues(
      ObjectInstanceHandle const& object,
      AttributeHandleValueMap const& values,
      VariableLengthData const&,
      TransportationTypeHandle const&,
      FederateHandle const&,
      RegionHandleSet const*) override {
    reflections.push_back({object, values});
  }

  std::vector<Reflection> reflections;
};

std::wstring nextFederationName() {
  static std::atomic_uint64_t sequence{0U};
  return L"package-attribute-filter-" +
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
    "Installed package reflects only subscribed attribute values",
    "[package-consumer][subscribe-object-class-attributes-value-filter]"
    "[rti.service.subscribe-object-class-attributes]"
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
  auto const publisherHandle = publisher->joinFederationExecution(
      L"package-filter-publisher", L"publisher", federationName);
  auto const publisherClass = publisher->getObjectClassHandle(
      L"HLAobjectRoot.UmbraAttributeFixtureBase");
  auto const publisherFirst = publisher->getAttributeHandle(publisherClass, L"ReliableBaseA");
  auto const publisherSecond = publisher->getAttributeHandle(publisherClass, L"ReliableBaseB");
  REQUIRE(publisherClass.isValid());
  REQUIRE(publisherFirst.isValid());
  REQUIRE(publisherSecond.isValid());
  publisher->publishObjectClassAttributes(
      publisherClass, AttributeHandleSet{publisherFirst, publisherSecond});

  auto const object = publisher->registerObjectInstance(publisherClass);
  REQUIRE(object.isValid());
  HLAunicodeString initialFirst{L"initial-first"};
  HLAunicodeString initialSecond{L"initial-second"};
  publisher->updateAttributeValues(
      object,
      AttributeHandleValueMap{
          {publisherFirst, initialFirst.encode()},
          {publisherSecond, initialSecond.encode()},
      },
      VariableLengthData{});

  subscriber->joinFederationExecution(
      L"package-filter-subscriber", L"subscriber", federationName);
  auto const subscriberClass = subscriber->getObjectClassHandle(
      L"HLAobjectRoot.UmbraAttributeFixtureBase");
  auto const subscriberFirst = subscriber->getAttributeHandle(subscriberClass, L"ReliableBaseA");
  auto const subscriberSecond = subscriber->getAttributeHandle(subscriberClass, L"ReliableBaseB");
  REQUIRE(subscriberClass.isValid());
  REQUIRE(subscriberFirst.isValid());
  REQUIRE(subscriberSecond.isValid());
  drainCallbacks(*subscriber);
  REQUIRE(subscriberReports.reflections.empty());

  subscriber->subscribeObjectClassAttributes(
      subscriberClass, AttributeHandleSet{subscriberFirst}, true);
  drainCallbacks(*subscriber);
  REQUIRE(subscriberReports.reflections.size() == 1U);
  auto const& initialReflection = subscriberReports.reflections.front();
  CHECK(initialReflection.object == object);
  REQUIRE(initialReflection.values.size() == 1U);
  REQUIRE(initialReflection.values.contains(subscriberFirst));
  CHECK_FALSE(initialReflection.values.contains(subscriberSecond));
  HLAunicodeString decodedInitial;
  decodedInitial.decode(initialReflection.values.at(subscriberFirst));
  CHECK(decodedInitial.get() == L"initial-first");

  HLAunicodeString updatedFirst{L"updated-first"};
  HLAunicodeString updatedSecond{L"updated-second"};
  publisher->updateAttributeValues(
      object,
      AttributeHandleValueMap{
          {publisherFirst, updatedFirst.encode()},
          {publisherSecond, updatedSecond.encode()},
      },
      VariableLengthData{});
  REQUIRE(subscriberReports.reflections.size() == 1U);
  drainCallbacks(*subscriber);
  REQUIRE(subscriberReports.reflections.size() == 2U);
  auto const& updateReflection = subscriberReports.reflections.back();
  CHECK(updateReflection.object == object);
  REQUIRE(updateReflection.values.size() == 1U);
  REQUIRE(updateReflection.values.contains(subscriberFirst));
  CHECK_FALSE(updateReflection.values.contains(subscriberSecond));
  HLAunicodeString decodedUpdate;
  decodedUpdate.decode(updateReflection.values.at(subscriberFirst));
  CHECK(decodedUpdate.get() == L"updated-first");

  subscriber->unsubscribeObjectClassAttributes(
      subscriberClass, AttributeHandleSet{subscriberFirst});
  subscriber->resignFederationExecution(NO_ACTION);
  publisher->resignFederationExecution(CANCEL_THEN_DELETE_THEN_DIVEST);
  publisher->destroyFederationExecution(federationName);
  subscriber->disconnect();
  publisher->disconnect();
  CHECK(publisherHandle.isValid());
}
