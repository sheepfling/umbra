#include <catch2/catch_test_macros.hpp>

#include "internal/fom/hla_names.hpp"

#include <RTI/NullFederateAmbassador.h>
#include <RTI/RTI1516.h>
#include <RTI/encoding/BasicDataElements.h>

#include <algorithm>
#include <atomic>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#ifndef UMBRA_SOURCE_DIRECTORY
#error "The Subscribe Object Class Attributes tests require the Umbra source directory."
#endif

namespace {

namespace standard_hla = umbra::detail::hla::wide;

using rti1516_2025::AttributeHandleSet;
using rti1516_2025::AttributeHandleValueMap;
using rti1516_2025::CANCEL_THEN_DELETE_THEN_DIVEST;
using rti1516_2025::FederateHandle;
using rti1516_2025::HLA_EVOKED;
using rti1516_2025::NO_ACTION;
using rti1516_2025::ObjectClassHandle;
using rti1516_2025::ObjectInstanceHandle;
using rti1516_2025::RTIambassador;
using rti1516_2025::RTIambassadorFactory;
using rti1516_2025::TransportationTypeHandle;
using rti1516_2025::VariableLengthData;

struct Discovery final {
  ObjectInstanceHandle objectInstance;
  ObjectClassHandle objectClass;
  std::wstring objectInstanceName;
  FederateHandle producingFederate;
};

struct AttributeReflection final {
  ObjectInstanceHandle objectInstance;
  AttributeHandleValueMap attributeValues;
};

class DiscoveryFederateAmbassador final : public rti1516_2025::NullFederateAmbassador {
 public:
  void discoverObjectInstance(
      ObjectInstanceHandle const& objectInstance,
      ObjectClassHandle const& objectClass,
      std::wstring const& objectInstanceName,
      FederateHandle const& producingFederate) override {
    discoveries.push_back({objectInstance, objectClass, objectInstanceName, producingFederate});
  }

  void reflectAttributeValues(
      ObjectInstanceHandle const& objectInstance,
      AttributeHandleValueMap const& attributeValues,
      VariableLengthData const&,
      TransportationTypeHandle const&,
      FederateHandle const&,
      rti1516_2025::RegionHandleSet const*) override {
    reflections.push_back({objectInstance, attributeValues});
  }

  std::vector<Discovery> discoveries;
  std::vector<AttributeReflection> reflections;
};

std::wstring nextFederationName() {
  static std::atomic_uint64_t sequence{0U};
  return L"federation-subscribe-discovery-" +
      std::to_wstring(sequence.fetch_add(1U, std::memory_order_relaxed));
}

std::unique_ptr<RTIambassador> makeRti() {
  RTIambassadorFactory factory;
  return factory.createRTIambassador();
}

std::filesystem::path restaurantFom() {
  return std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "third_party" /
      "ieee1516.2-2025" / "resources" / "examples" /
      "RestaurantFOMmodule-2025.xml";
}

void drainCallbacks(RTIambassador& rti) {
  while (rti.evokeCallback(0.0)) {
  }
}

}  // namespace

TEST_CASE(
    "Embedded Subscribe Object Class Attributes discovers existing and subsequently registered objects",
    "[integration][development-profile][declaration-management][object-management]"
    "[subscribe-object-class-attributes-discovery]"
    "[rti.service.connect][rti.service.create-federation-execution]"
    "[rti.service.join-federation-execution][rti.service.get-object-class-handle]"
    "[rti.service.get-attribute-handle][rti.service.publish-object-class-attributes]"
    "[rti.service.register-object-instance][rti.service.subscribe-object-class-attributes]"
    "[rti.service.evoke-callback][rti.service.unsubscribe-object-class-attributes]"
    "[rti.service.resign-federation-execution][rti.service.destroy-federation-execution]"
    "[rti.service.disconnect][federate.callback.discover-object-instance]") {
  DiscoveryFederateAmbassador publisherReports;
  DiscoveryFederateAmbassador subscriberReports;
  auto publisher = makeRti();
  auto subscriber = makeRti();
  auto const federationName = nextFederationName();
  auto const fom = restaurantFom().wstring();

  REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED));
  REQUIRE_NOTHROW(subscriber->connect(subscriberReports, HLA_EVOKED));
  REQUIRE_NOTHROW(publisher->createFederationExecution(
      federationName, fom, standard_hla::mom::integer64_time));
  FederateHandle publisherHandle;
  REQUIRE_NOTHROW(publisherHandle = publisher->joinFederationExecution(
      L"subscribe-discovery-publisher", L"publisher", federationName));

  auto const publisherClass = publisher->getObjectClassHandle(L"HLAobjectRoot.Employee.Server");
  auto const publisherAttribute = publisher->getAttributeHandle(publisherClass, L"Efficiency");
  REQUIRE(publisherClass.isValid());
  REQUIRE(publisherAttribute.isValid());
  REQUIRE_NOTHROW(publisher->publishObjectClassAttributes(
      publisherClass, AttributeHandleSet{publisherAttribute}));

  ObjectInstanceHandle existingObject;
  REQUIRE_NOTHROW(existingObject = publisher->registerObjectInstance(publisherClass));
  REQUIRE(existingObject.isValid());

  REQUIRE_NOTHROW(subscriber->joinFederationExecution(
      L"subscribe-discovery-subscriber", L"subscriber", federationName));
  auto const subscriberClass = subscriber->getObjectClassHandle(L"HLAobjectRoot.Employee.Server");
  auto const subscriberAttribute = subscriber->getAttributeHandle(subscriberClass, L"Efficiency");
  REQUIRE(subscriberClass.isValid());
  REQUIRE(subscriberAttribute.isValid());
  REQUIRE(subscriberReports.discoveries.empty());
  drainCallbacks(*subscriber);
  REQUIRE(subscriberReports.discoveries.empty());

  AttributeHandleSet const subscribedAttributes{subscriberAttribute};
  REQUIRE_NOTHROW(subscriber->subscribeObjectClassAttributes(
      subscriberClass, subscribedAttributes, true));
  drainCallbacks(*subscriber);
  REQUIRE(subscriberReports.discoveries.size() == 1U);
  REQUIRE(subscriberReports.discoveries.front().objectInstance == existingObject);
  REQUIRE(subscriberReports.discoveries.front().objectClass == subscriberClass);
  REQUIRE_FALSE(subscriberReports.discoveries.front().objectInstanceName.empty());
  REQUIRE(subscriberReports.discoveries.front().producingFederate == publisherHandle);

  ObjectInstanceHandle laterObject;
  REQUIRE_NOTHROW(laterObject = publisher->registerObjectInstance(publisherClass));
  REQUIRE(laterObject.isValid());
  REQUIRE(subscriberReports.discoveries.size() == 1U);
  drainCallbacks(*subscriber);
  REQUIRE(subscriberReports.discoveries.size() == 2U);
  auto const laterDiscovery = std::find_if(
      subscriberReports.discoveries.begin(),
      subscriberReports.discoveries.end(),
      [&](Discovery const& discovery) {
        return discovery.objectInstance == laterObject;
      });
  REQUIRE(laterDiscovery != subscriberReports.discoveries.end());
  REQUIRE(laterDiscovery->objectClass == subscriberClass);
  REQUIRE(laterDiscovery->producingFederate == publisherHandle);

  REQUIRE_NOTHROW(subscriber->unsubscribeObjectClassAttributes(
      subscriberClass, subscribedAttributes));
  REQUIRE_NOTHROW(subscriber->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(subscriber->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
}

TEST_CASE(
    "Embedded Subscribe Object Class Attributes reflects only values in its subscribed set",
    "[integration][development-profile][declaration-management][object-management]"
    "[subscribe-object-class-attributes-value-filter]"
    "[rti.service.connect][rti.service.create-federation-execution]"
    "[rti.service.join-federation-execution][rti.service.get-object-class-handle]"
    "[rti.service.get-attribute-handle][rti.service.publish-object-class-attributes]"
    "[rti.service.register-object-instance][rti.service.update-attribute-values]"
    "[rti.service.subscribe-object-class-attributes][rti.service.evoke-callback]"
    "[rti.service.unsubscribe-object-class-attributes]"
    "[rti.service.resign-federation-execution][rti.service.destroy-federation-execution]"
    "[rti.service.disconnect][federate.callback.discover-object-instance]"
    "[federate.callback.reflect-attribute-values]") {
  DiscoveryFederateAmbassador publisherReports;
  DiscoveryFederateAmbassador subscriberReports;
  auto publisher = makeRti();
  auto subscriber = makeRti();
  auto const federationName = nextFederationName();
  auto const fom = (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data" /
                    "attribute-update-passel-fom.xml")
                       .wstring();

  REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED));
  REQUIRE_NOTHROW(subscriber->connect(subscriberReports, HLA_EVOKED));
  REQUIRE_NOTHROW(publisher->createFederationExecution(
      federationName, fom, standard_hla::mom::integer64_time));
  FederateHandle publisherHandle;
  REQUIRE_NOTHROW(publisherHandle = publisher->joinFederationExecution(
      L"attribute-filter-publisher", L"publisher", federationName));

  auto const publisherClass = publisher->getObjectClassHandle(
      L"HLAobjectRoot.UmbraAttributeFixtureBase");
  auto const publisherFirstAttribute = publisher->getAttributeHandle(
      publisherClass, L"ReliableBaseA");
  auto const publisherSecondAttribute = publisher->getAttributeHandle(
      publisherClass, L"ReliableBaseB");
  REQUIRE(publisherClass.isValid());
  REQUIRE(publisherFirstAttribute.isValid());
  REQUIRE(publisherSecondAttribute.isValid());
  REQUIRE_NOTHROW(publisher->publishObjectClassAttributes(
      publisherClass,
      AttributeHandleSet{publisherFirstAttribute, publisherSecondAttribute}));

  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = publisher->registerObjectInstance(publisherClass));
  REQUIRE(objectInstance.isValid());
  rti1516_2025::HLAunicodeString initialFirstValue{L"initial-first"};
  rti1516_2025::HLAunicodeString initialSecondValue{L"initial-second"};
  REQUIRE_NOTHROW(publisher->updateAttributeValues(
      objectInstance,
      AttributeHandleValueMap{
          {publisherFirstAttribute, initialFirstValue.encode()},
          {publisherSecondAttribute, initialSecondValue.encode()},
      },
      VariableLengthData{}));

  REQUIRE_NOTHROW(subscriber->joinFederationExecution(
      L"attribute-filter-subscriber", L"subscriber", federationName));
  auto const subscriberClass = subscriber->getObjectClassHandle(
      L"HLAobjectRoot.UmbraAttributeFixtureBase");
  auto const subscriberFirstAttribute = subscriber->getAttributeHandle(
      subscriberClass, L"ReliableBaseA");
  auto const subscriberSecondAttribute = subscriber->getAttributeHandle(
      subscriberClass, L"ReliableBaseB");
  REQUIRE(subscriberClass.isValid());
  REQUIRE(subscriberFirstAttribute.isValid());
  REQUIRE(subscriberSecondAttribute.isValid());
  REQUIRE(subscriberReports.discoveries.empty());
  REQUIRE(subscriberReports.reflections.empty());

  AttributeHandleSet const subscribedAttributes{subscriberFirstAttribute};
  REQUIRE_NOTHROW(subscriber->subscribeObjectClassAttributes(
      subscriberClass, subscribedAttributes, true));
  drainCallbacks(*subscriber);
  REQUIRE(subscriberReports.discoveries.size() == 1U);
  REQUIRE(subscriberReports.discoveries.front().objectInstance == objectInstance);
  REQUIRE(subscriberReports.discoveries.front().producingFederate == publisherHandle);
  REQUIRE(subscriberReports.reflections.size() == 1U);
  REQUIRE(subscriberReports.reflections.front().objectInstance == objectInstance);
  REQUIRE(subscriberReports.reflections.front().attributeValues.size() == 1U);
  REQUIRE(subscriberReports.reflections.front().attributeValues.contains(subscriberFirstAttribute));
  REQUIRE_FALSE(subscriberReports.reflections.front().attributeValues.contains(subscriberSecondAttribute));
  rti1516_2025::HLAunicodeString decodedInitialFirstValue;
  REQUIRE_NOTHROW(decodedInitialFirstValue.decode(
      subscriberReports.reflections.front().attributeValues.at(subscriberFirstAttribute)));
  REQUIRE(decodedInitialFirstValue.get() == L"initial-first");

  rti1516_2025::HLAunicodeString updatedFirstValue{L"updated-first"};
  rti1516_2025::HLAunicodeString updatedSecondValue{L"updated-second"};
  auto const reflectionsBeforeUpdate = subscriberReports.reflections.size();
  REQUIRE_NOTHROW(publisher->updateAttributeValues(
      objectInstance,
      AttributeHandleValueMap{
          {publisherFirstAttribute, updatedFirstValue.encode()},
          {publisherSecondAttribute, updatedSecondValue.encode()},
      },
      VariableLengthData{}));
  REQUIRE(subscriberReports.reflections.size() == reflectionsBeforeUpdate);
  drainCallbacks(*subscriber);
  REQUIRE(subscriberReports.reflections.size() == reflectionsBeforeUpdate + 1U);
  REQUIRE(subscriberReports.reflections.back().attributeValues.size() == 1U);
  REQUIRE(subscriberReports.reflections.back().attributeValues.contains(subscriberFirstAttribute));
  REQUIRE_FALSE(subscriberReports.reflections.back().attributeValues.contains(subscriberSecondAttribute));
  rti1516_2025::HLAunicodeString decodedFirstValue;
  REQUIRE_NOTHROW(decodedFirstValue.decode(
      subscriberReports.reflections.back().attributeValues.at(subscriberFirstAttribute)));
  REQUIRE(decodedFirstValue.get() == L"updated-first");

  REQUIRE_NOTHROW(subscriber->unsubscribeObjectClassAttributes(
      subscriberClass, subscribedAttributes));
  REQUIRE_NOTHROW(subscriber->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(subscriber->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
}

TEST_CASE(
    "Embedded Subscribe Object Class Attributes rejects out-of-class attributes and reports the closest subscribed class",
    "[integration][development-profile][declaration-management][object-management]"
    "[subscribe-object-class-attributes-attribute-set-validity]"
    "[rti.service.connect][rti.service.create-federation-execution]"
    "[rti.service.join-federation-execution][rti.service.get-object-class-handle]"
    "[rti.service.get-attribute-handle][rti.service.publish-object-class-attributes]"
    "[rti.service.register-object-instance][rti.service.update-attribute-values]"
    "[rti.service.subscribe-object-class-attributes][rti.service.evoke-callback]"
    "[rti.service.resign-federation-execution][rti.service.destroy-federation-execution]"
    "[rti.service.disconnect][federate.callback.discover-object-instance]"
    "[federate.callback.reflect-attribute-values]") {
  DiscoveryFederateAmbassador publisherReports;
  DiscoveryFederateAmbassador subscriberReports;
  auto publisher = makeRti();
  auto subscriber = makeRti();
  auto const federationName = nextFederationName();
  auto const fom = (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data" /
                    "attribute-update-passel-fom.xml")
                       .wstring();

  REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED));
  REQUIRE_NOTHROW(subscriber->connect(subscriberReports, HLA_EVOKED));
  REQUIRE_NOTHROW(publisher->createFederationExecution(
      federationName, fom, standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(publisher->joinFederationExecution(
      L"attribute-set-validity-publisher", L"publisher", federationName));

  auto const publisherBaseClass = publisher->getObjectClassHandle(
      L"HLAobjectRoot.UmbraAttributeFixtureBase");
  auto const publisherChildClass = publisher->getObjectClassHandle(
      L"HLAobjectRoot.UmbraAttributeFixtureBase.UmbraAttributeFixtureChild");
  auto const publisherBaseAttribute = publisher->getAttributeHandle(
      publisherBaseClass, L"ReliableBaseA");
  auto const publisherChildAttribute = publisher->getAttributeHandle(
      publisherChildClass, L"ReliableChild");
  REQUIRE(publisherBaseClass.isValid());
  REQUIRE(publisherChildClass.isValid());
  REQUIRE(publisherBaseAttribute.isValid());
  REQUIRE(publisherChildAttribute.isValid());
  REQUIRE_NOTHROW(publisher->publishObjectClassAttributes(
      publisherChildClass,
      AttributeHandleSet{publisherBaseAttribute, publisherChildAttribute}));

  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = publisher->registerObjectInstance(publisherChildClass));
  REQUIRE(objectInstance.isValid());
  rti1516_2025::HLAunicodeString initialBaseValue{L"base-value"};
  rti1516_2025::HLAunicodeString initialChildValue{L"child-value"};
  REQUIRE_NOTHROW(publisher->updateAttributeValues(
      objectInstance,
      AttributeHandleValueMap{
          {publisherBaseAttribute, initialBaseValue.encode()},
          {publisherChildAttribute, initialChildValue.encode()},
      },
      VariableLengthData{}));

  REQUIRE_NOTHROW(subscriber->joinFederationExecution(
      L"attribute-set-validity-subscriber", L"subscriber", federationName));
  auto const subscriberBaseClass = subscriber->getObjectClassHandle(
      L"HLAobjectRoot.UmbraAttributeFixtureBase");
  auto const subscriberChildClass = subscriber->getObjectClassHandle(
      L"HLAobjectRoot.UmbraAttributeFixtureBase.UmbraAttributeFixtureChild");
  auto const subscriberBaseAttribute = subscriber->getAttributeHandle(
      subscriberBaseClass, L"ReliableBaseA");
  auto const subscriberChildAttribute = subscriber->getAttributeHandle(
      subscriberChildClass, L"ReliableChild");
  REQUIRE(subscriberBaseClass.isValid());
  REQUIRE(subscriberChildClass.isValid());
  REQUIRE(subscriberBaseAttribute.isValid());
  REQUIRE(subscriberChildAttribute.isValid());
  REQUIRE(subscriberReports.discoveries.empty());
  REQUIRE(subscriberReports.reflections.empty());

  AttributeHandleSet const emptyAttributeSet;
  REQUIRE_NOTHROW(subscriber->subscribeObjectClassAttributes(
      subscriberBaseClass, emptyAttributeSet, true));
  drainCallbacks(*subscriber);
  REQUIRE(subscriberReports.discoveries.empty());
  REQUIRE(subscriberReports.reflections.empty());

  REQUIRE_THROWS_AS(
      subscriber->subscribeObjectClassAttributes(
          subscriberBaseClass, AttributeHandleSet{subscriberChildAttribute}, true),
      rti1516_2025::AttributeNotDefined);
  REQUIRE(subscriberReports.discoveries.empty());
  REQUIRE(subscriberReports.reflections.empty());

  AttributeHandleSet const validBaseSubscription{subscriberBaseAttribute};
  REQUIRE_NOTHROW(subscriber->subscribeObjectClassAttributes(
      subscriberBaseClass, validBaseSubscription, true));
  drainCallbacks(*subscriber);
  REQUIRE(subscriberReports.discoveries.size() == 1U);
  REQUIRE(subscriberReports.discoveries.front().objectInstance == objectInstance);
  REQUIRE(subscriberReports.discoveries.front().objectClass == subscriberBaseClass);
  REQUIRE(subscriberReports.reflections.size() == 1U);
  REQUIRE(subscriberReports.reflections.front().attributeValues.size() == 1U);
  REQUIRE(subscriberReports.reflections.front().attributeValues.contains(subscriberBaseAttribute));
  REQUIRE_FALSE(subscriberReports.reflections.front().attributeValues.contains(subscriberChildAttribute));
  rti1516_2025::HLAunicodeString decodedBaseValue;
  REQUIRE_NOTHROW(decodedBaseValue.decode(
      subscriberReports.reflections.front().attributeValues.at(subscriberBaseAttribute)));
  REQUIRE(decodedBaseValue.get() == L"base-value");

  REQUIRE_NOTHROW(subscriber->unsubscribeObjectClassAttributes(
      subscriberBaseClass, validBaseSubscription));
  REQUIRE_NOTHROW(subscriber->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(subscriber->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
}
