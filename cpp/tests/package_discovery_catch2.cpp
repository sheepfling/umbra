#include <catch2/catch_test_macros.hpp>

#include <RTI/RTI1516.h>
#include <RTI/NullFederateAmbassador.h>

#include <algorithm>
#include <atomic>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#ifndef UMBRA_PACKAGE_RESTAURANT_FOM
#error "The package-consumer discovery test requires an external FOM fixture path."
#endif

namespace {

using rti1516_2025::AttributeHandleSet;
using rti1516_2025::FederateHandle;
using rti1516_2025::HLA_EVOKED;
using rti1516_2025::NO_ACTION;
using rti1516_2025::ObjectClassHandle;
using rti1516_2025::ObjectInstanceHandle;
using rti1516_2025::RTIambassador;
using rti1516_2025::RTIambassadorFactory;

struct Discovery final {
  ObjectInstanceHandle object;
  ObjectClassHandle objectClass;
  std::wstring name;
  FederateHandle producer;
};

class DiscoveryRecorder final : public rti1516_2025::NullFederateAmbassador {
 public:
  void discoverObjectInstance(
      ObjectInstanceHandle const& object,
      ObjectClassHandle const& objectClass,
      std::wstring const& name,
      FederateHandle const& producer) override {
    discoveries.push_back({object, objectClass, name, producer});
  }

  std::vector<Discovery> discoveries;
};

std::wstring nextFederationName() {
  static std::atomic_uint64_t sequence{0U};
  return L"package-subscribe-discovery-" +
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
    "Installed package discovers existing and later objects after active subscription",
    "[package-consumer][subscribe-object-class-attributes-discovery]"
    "[rti.service.subscribe-object-class-attributes]"
    "[federate.callback.discover-object-instance]") {
  DiscoveryRecorder publisherReports;
  DiscoveryRecorder subscriberReports;
  auto publisher = makeRti();
  auto subscriber = makeRti();
  auto const federationName = nextFederationName();
  std::wstring const fom =
      std::filesystem::path(UMBRA_PACKAGE_RESTAURANT_FOM).wstring();

  publisher->connect(publisherReports, HLA_EVOKED);
  subscriber->connect(subscriberReports, HLA_EVOKED);
  publisher->createFederationExecution(
      federationName, fom, L"HLAinteger64Time");
  auto const publisherHandle = publisher->joinFederationExecution(
      L"package-discovery-publisher", L"publisher", federationName);
  auto const publisherClass = publisher->getObjectClassHandle(
      L"HLAobjectRoot.Employee.Server");
  auto const publisherAttribute = publisher->getAttributeHandle(
      publisherClass, L"Efficiency");
  REQUIRE(publisherClass.isValid());
  REQUIRE(publisherAttribute.isValid());
  publisher->publishObjectClassAttributes(
      publisherClass, AttributeHandleSet{publisherAttribute});
  auto const existingObject = publisher->registerObjectInstance(publisherClass);
  REQUIRE(existingObject.isValid());

  subscriber->joinFederationExecution(
      L"package-discovery-subscriber", L"subscriber", federationName);
  auto const subscriberClass = subscriber->getObjectClassHandle(
      L"HLAobjectRoot.Employee.Server");
  auto const subscriberAttribute = subscriber->getAttributeHandle(
      subscriberClass, L"Efficiency");
  REQUIRE(subscriberClass.isValid());
  REQUIRE(subscriberAttribute.isValid());
  drainCallbacks(*subscriber);
  REQUIRE(subscriberReports.discoveries.empty());

  subscriber->subscribeObjectClassAttributes(
      subscriberClass, AttributeHandleSet{subscriberAttribute}, true);
  drainCallbacks(*subscriber);
  REQUIRE(subscriberReports.discoveries.size() == 1U);
  CHECK(subscriberReports.discoveries.front().object == existingObject);
  CHECK(subscriberReports.discoveries.front().objectClass == subscriberClass);
  CHECK_FALSE(subscriberReports.discoveries.front().name.empty());
  CHECK(subscriberReports.discoveries.front().producer == publisherHandle);

  auto const laterObject = publisher->registerObjectInstance(publisherClass);
  REQUIRE(laterObject.isValid());
  REQUIRE(subscriberReports.discoveries.size() == 1U);
  drainCallbacks(*subscriber);
  REQUIRE(subscriberReports.discoveries.size() == 2U);
  auto const laterDiscovery = std::find_if(
      subscriberReports.discoveries.begin(),
      subscriberReports.discoveries.end(),
      [&](Discovery const& discovery) { return discovery.object == laterObject; });
  REQUIRE(laterDiscovery != subscriberReports.discoveries.end());
  CHECK(laterDiscovery->objectClass == subscriberClass);
  CHECK(laterDiscovery->producer == publisherHandle);
  CHECK(std::count_if(
            subscriberReports.discoveries.begin(),
            subscriberReports.discoveries.end(),
            [&](Discovery const& discovery) { return discovery.object == laterObject; }) ==
        1);

  subscriber->unsubscribeObjectClassAttributes(
      subscriberClass, AttributeHandleSet{subscriberAttribute});
  subscriber->resignFederationExecution(NO_ACTION);
  publisher->resignFederationExecution(rti1516_2025::CANCEL_THEN_DELETE_THEN_DIVEST);
  publisher->destroyFederationExecution(federationName);
  subscriber->disconnect();
  publisher->disconnect();
}
