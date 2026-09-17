#include <catch2/catch_test_macros.hpp>

#include <RTI/NullFederateAmbassador.h>
#include <RTI/RTI1516.h>

#include <atomic>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#ifndef UMBRA_SOURCE_DIRECTORY
#error "The Request Attribute Value Update baseline tests require the Umbra source directory."
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

class RequestFederateAmbassador final
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
      FederateHandle const&) override {
    discoveredObjects.push_back({objectInstance, objectClass});
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
  return L"attribute-value-update-request-baseline-" +
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
    "Embedded object-instance Request Attribute Value Update solicits 2025 owners",
    "[integration][development-profile][federation-management][object-management]"
    "[attribute-value-update-request-baseline][callback-evoked]"
    "[rti.service.request-attribute-value-update]"
    "[rti.service.publish-object-class-attributes]"
    "[rti.service.subscribe-object-class-attributes]"
    "[rti.service.register-object-instance]"
    "[federate.callback.discover-object-instance]"
    "[federate.callback.provide-attribute-value-update]") {
  RequestFederateAmbassador ownerReports;
  RequestFederateAmbassador requesterReports;
  auto owner = makeRti();
  auto requester = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("attribute-update-passel-fom.xml").wstring();

  REQUIRE_NOTHROW(owner->connect(ownerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(requester->connect(requesterReports, HLA_EVOKED));
  REQUIRE_NOTHROW(owner->createFederationExecution(
      federationName, fomModule, L"HLAinteger64Time"));
  REQUIRE_NOTHROW(owner->joinFederationExecution(
      L"attribute-value-update-request-owner", L"provider", federationName));
  REQUIRE_NOTHROW(requester->joinFederationExecution(
      L"attribute-value-update-request-requester", L"requester", federationName));

  auto const objectClass = owner->getObjectClassHandle(
      L"HLAobjectRoot.UmbraAttributeFixtureBase.UmbraAttributeFixtureChild");
  auto const reliableBaseA = owner->getAttributeHandle(
      objectClass, L"ReliableBaseA");
  auto const reliableChild = owner->getAttributeHandle(
      objectClass, L"ReliableChild");
  auto const reliableBaseB = owner->getAttributeHandle(
      objectClass, L"ReliableBaseB");
  auto const unownedChild = owner->getAttributeHandle(
      objectClass, L"UnownedChild");
  REQUIRE(objectClass.isValid());
  REQUIRE(reliableBaseA.isValid());
  REQUIRE(reliableChild.isValid());
  REQUIRE(reliableBaseB.isValid());
  REQUIRE(unownedChild.isValid());

  AttributeHandleSet const ownerAttributes{reliableBaseA, reliableChild};
  AttributeHandleSet const requesterAttributes{reliableBaseB};
  AttributeHandleSet const requestAttributes{
      reliableBaseA, reliableChild, reliableBaseB, unownedChild};
  REQUIRE_NOTHROW(owner->publishObjectClassAttributes(
      objectClass, ownerAttributes));
  REQUIRE_NOTHROW(requester->publishObjectClassAttributes(
      objectClass, requesterAttributes));
  REQUIRE_NOTHROW(requester->subscribeObjectClassAttributes(
      objectClass, requestAttributes));

  ObjectInstanceHandle ownerObject;
  REQUIRE_NOTHROW(ownerObject = owner->registerObjectInstance(objectClass));
  REQUIRE(ownerObject.isValid());
  drainCallbacks(*requester);
  REQUIRE(requesterReports.discoveredObjects.size() == 1U);
  REQUIRE(requesterReports.discoveredObjects.front().objectInstance == ownerObject);
  REQUIRE(requesterReports.discoveredObjects.front().objectClass == objectClass);

  std::vector<unsigned char> const requestTagBytes{0x52U, 0x51U, 0x2DU, 0x31U};
  VariableLengthData const requestTag(
      requestTagBytes.data(), requestTagBytes.size());
  REQUIRE_NOTHROW(requester->requestAttributeValueUpdate(
      ownerObject, requestAttributes, requestTag));

  // HLA_EVOKED keeps the provider callback pending until the owner explicitly
  // evicts it; this also proves the request targets one known instance.
  REQUIRE(ownerReports.provideReports.empty());
  drainCallbacks(*owner);
  REQUIRE(ownerReports.provideReports.size() == 1U);
  auto const& ownerProvide = ownerReports.provideReports.front();
  REQUIRE(ownerProvide.objectInstance == ownerObject);
  REQUIRE(ownerProvide.attributes == ownerAttributes);
  REQUIRE(bytes(ownerProvide.userSuppliedTag) == requestTagBytes);

  // A requester-owned attribute is never solicited back to that same
  // federate.  A second object keeps this assertion independent of ownership
  // transfer and the first object's external-owner delivery.
  ObjectInstanceHandle requesterObject;
  REQUIRE_NOTHROW(requesterObject = requester->registerObjectInstance(objectClass));
  REQUIRE(requesterObject.isValid());
  REQUIRE_NOTHROW(requester->requestAttributeValueUpdate(
      requesterObject, requestAttributes, requestTag));
  drainCallbacks(*requester);
  REQUIRE(requesterReports.provideReports.empty());

  REQUIRE_NOTHROW(requester->unsubscribeObjectClassAttributes(
      objectClass, requestAttributes));
  REQUIRE_NOTHROW(requester->unpublishObjectClassAttributes(
      objectClass, requesterAttributes));
  REQUIRE_NOTHROW(owner->unpublishObjectClassAttributes(
      objectClass, ownerAttributes));
  REQUIRE_NOTHROW(requester->resignFederationExecution(
      rti1516_2025::CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(owner->resignFederationExecution(
      rti1516_2025::CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(requester->disconnect());
  REQUIRE_NOTHROW(owner->disconnect());
}
