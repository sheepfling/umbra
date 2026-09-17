#include <catch2/catch_test_macros.hpp>

#include <RTI/NullFederateAmbassador.h>
#include <RTI/RTI1516.h>

#include <atomic>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#ifndef UMBRA_SOURCE_DIRECTORY
#error "The class-designator Request Attribute Value Update baseline tests require the Umbra source directory."
#endif

namespace {

using rti1516_2025::AttributeHandle;
using rti1516_2025::AttributeHandleSet;
using rti1516_2025::FederateHandle;
using rti1516_2025::HLA_EVOKED;
using rti1516_2025::ObjectClassHandle;
using rti1516_2025::ObjectInstanceHandle;
using rti1516_2025::RTIambassador;
using rti1516_2025::RTIambassadorFactory;
using rti1516_2025::VariableLengthData;

class ClassRequestFederateAmbassador final
    : public rti1516_2025::NullFederateAmbassador {
 public:
  struct ProvideReport final {
    ObjectInstanceHandle objectInstance;
    AttributeHandleSet attributes;
    VariableLengthData userSuppliedTag;
  };

  void discoverObjectInstance(
      ObjectInstanceHandle const& objectInstance,
      ObjectClassHandle const& objectClass,
      std::wstring const&,
      FederateHandle const& producingFederate) override {
    discoveredObjects.push_back({objectInstance, objectClass, producingFederate});
  }

  void provideAttributeValueUpdate(
      ObjectInstanceHandle const& objectInstance,
      AttributeHandleSet const& attributes,
      VariableLengthData const& userSuppliedTag) override {
    provideReports.push_back({objectInstance, attributes, userSuppliedTag});
  }

  struct DiscoveryReport final {
    ObjectInstanceHandle objectInstance;
    ObjectClassHandle objectClass;
    FederateHandle producingFederate;
  };

  std::vector<DiscoveryReport> discoveredObjects;
  std::vector<ProvideReport> provideReports;
};

std::unique_ptr<RTIambassador> makeRti() {
  RTIambassadorFactory factory;
  return factory.createRTIambassador();
}

std::filesystem::path resourcePath(std::filesystem::path const& relativePath) {
  return std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" /
      "data" / relativePath;
}

std::wstring nextFederationName() {
  static std::atomic_uint64_t sequence{0U};
  return L"object-class-attribute-value-update-request-baseline-" +
      std::to_wstring(sequence.fetch_add(1U, std::memory_order_relaxed));
}

void drainCallbacks(RTIambassador& rti) {
  for (int pass = 0; pass != 64; ++pass) {
    static_cast<void>(rti.evokeCallback(0.0));
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
    "Embedded object-class Request Attribute Value Update solicits 2025 subclass owners",
    "[integration][development-profile][federation-management][object-management]"
    "[object-class-attribute-value-update-request-baseline][callback-evoked]"
    "[rti.service.request-attribute-value-update]"
    "[rti.service.publish-object-class-attributes]"
    "[rti.service.subscribe-object-class-attributes]"
    "[rti.service.register-object-instance]"
    "[federate.callback.discover-object-instance]"
    "[federate.callback.provide-attribute-value-update]") {
  ClassRequestFederateAmbassador ownerReports;
  ClassRequestFederateAmbassador requesterReports;
  auto owner = makeRti();
  auto requester = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("attribute-update-passel-fom.xml").wstring();

  REQUIRE_NOTHROW(owner->connect(ownerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(requester->connect(requesterReports, HLA_EVOKED));
  REQUIRE_NOTHROW(owner->createFederationExecution(
      federationName, fomModule, L"HLAinteger64Time"));
  FederateHandle ownerFederate;
  REQUIRE_NOTHROW(ownerFederate = owner->joinFederationExecution(
      L"object-class-attribute-value-update-owner", L"provider", federationName));
  REQUIRE_NOTHROW(requester->joinFederationExecution(
      L"object-class-attribute-value-update-requester", L"requester", federationName));

  auto const baseClass = owner->getObjectClassHandle(
      L"HLAobjectRoot.UmbraAttributeFixtureBase");
  auto const childClass = owner->getObjectClassHandle(
      L"HLAobjectRoot.UmbraAttributeFixtureBase.UmbraAttributeFixtureChild");
  auto const reliableBaseA = owner->getAttributeHandle(
      baseClass, L"ReliableBaseA");
  auto const reliableBaseB = owner->getAttributeHandle(
      baseClass, L"ReliableBaseB");
  auto const reliableChild = owner->getAttributeHandle(
      childClass, L"ReliableChild");
  REQUIRE(baseClass.isValid());
  REQUIRE(childClass.isValid());
  REQUIRE(reliableBaseA.isValid());
  REQUIRE(reliableBaseB.isValid());
  REQUIRE(reliableChild.isValid());

  AttributeHandleSet const ownerAttributes{reliableBaseA, reliableChild};
  AttributeHandleSet const requesterAttributes{reliableBaseB};
  AttributeHandleSet const requestAttributes{reliableBaseA, reliableBaseB};
  REQUIRE_NOTHROW(owner->publishObjectClassAttributes(
      childClass, ownerAttributes));
  REQUIRE_NOTHROW(requester->publishObjectClassAttributes(
      baseClass, requesterAttributes));
  REQUIRE_NOTHROW(requester->subscribeObjectClassAttributes(
      baseClass, requestAttributes));

  std::vector<ObjectInstanceHandle> ownerObjects;
  REQUIRE_NOTHROW(ownerObjects.push_back(owner->registerObjectInstance(childClass)));
  REQUIRE_NOTHROW(ownerObjects.push_back(owner->registerObjectInstance(childClass)));
  REQUIRE(ownerObjects.size() == 2U);
  REQUIRE(ownerObjects.front().isValid());
  REQUIRE(ownerObjects.back().isValid());

  ObjectInstanceHandle requesterObject;
  REQUIRE_NOTHROW(requesterObject = requester->registerObjectInstance(baseClass));
  REQUIRE(requesterObject.isValid());
  drainCallbacks(*requester);
  std::size_t ownerDiscoveryCount = 0U;
  for (auto const& discovery : requesterReports.discoveredObjects) {
    if (discovery.producingFederate != ownerFederate) {
      continue;
    }
    ++ownerDiscoveryCount;
    // The active subscription is at the requested base class, so discovery
    // carries that closest subscribed class while the request later expands
    // across the concrete child instances.
    REQUIRE(discovery.objectClass == baseClass);
    REQUIRE((discovery.objectInstance == ownerObjects.front() ||
             discovery.objectInstance == ownerObjects.back()));
  }
  REQUIRE(ownerDiscoveryCount == ownerObjects.size());

  std::vector<unsigned char> const requestTagBytes{0x43U, 0x4CU, 0x53U, 0x2DU, 0x31U};
  VariableLengthData const requestTag(
      requestTagBytes.data(), requestTagBytes.size());
  REQUIRE_NOTHROW(requester->requestAttributeValueUpdate(
      baseClass, requestAttributes, requestTag));

  // The class designator expands to both registered child instances, but the
  // requester-owned base object is not solicited back to its own federate.
  REQUIRE(ownerReports.provideReports.empty());
  drainCallbacks(*owner);
  REQUIRE(ownerReports.provideReports.size() == ownerObjects.size());
  for (auto const& provide : ownerReports.provideReports) {
    REQUIRE((provide.objectInstance == ownerObjects.front() ||
             provide.objectInstance == ownerObjects.back()));
    REQUIRE(provide.attributes == AttributeHandleSet{reliableBaseA});
    REQUIRE(bytes(provide.userSuppliedTag) == requestTagBytes);
  }
  REQUIRE(requesterReports.provideReports.empty());

  REQUIRE_NOTHROW(requester->unsubscribeObjectClassAttributes(
      baseClass, requestAttributes));
  REQUIRE_NOTHROW(requester->unpublishObjectClassAttributes(
      baseClass, requesterAttributes));
  REQUIRE_NOTHROW(owner->unpublishObjectClassAttributes(
      childClass, ownerAttributes));
  REQUIRE_NOTHROW(requester->resignFederationExecution(
      rti1516_2025::CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(owner->resignFederationExecution(
      rti1516_2025::CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(requester->disconnect());
  REQUIRE_NOTHROW(owner->disconnect());
}
