#include <catch2/catch_test_macros.hpp>

#include <RTI/NullFederateAmbassador.h>
#include <RTI/RTI1516.h>

#include <atomic>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#ifndef UMBRA_SOURCE_DIRECTORY
#error "The Auto Provide baseline tests require the Umbra source directory."
#endif

namespace {

using rti1516_2025::AttributeHandle;
using rti1516_2025::AttributeHandleSet;
using rti1516_2025::CallbackModel;
using rti1516_2025::FederateHandle;
using rti1516_2025::HLA_EVOKED;
using rti1516_2025::ObjectClassHandle;
using rti1516_2025::ObjectInstanceHandle;
using rti1516_2025::RTIambassador;
using rti1516_2025::RTIambassadorFactory;
using rti1516_2025::VariableLengthData;

class AutoProvideFederateAmbassador final
    : public rti1516_2025::NullFederateAmbassador {
 public:
  struct ProvideReport final {
    ObjectInstanceHandle objectInstance;
    AttributeHandleSet attributes;
    VariableLengthData userSuppliedTag;
  };

  void discoverObjectInstance(
      ObjectInstanceHandle const& objectInstance,
      ObjectClassHandle const&,
      std::wstring const&,
      FederateHandle const&) override {
    discoveredObjects.push_back(objectInstance);
    callbackOrder.push_back("discover");
  }

  void provideAttributeValueUpdate(
      ObjectInstanceHandle const& objectInstance,
      AttributeHandleSet const& attributes,
      VariableLengthData const& userSuppliedTag) override {
    provideReports.push_back({objectInstance, attributes, userSuppliedTag});
    callbackOrder.push_back("provide");
  }

  std::vector<ObjectInstanceHandle> discoveredObjects;
  std::vector<ProvideReport> provideReports;
  std::vector<std::string> callbackOrder;
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
  return L"auto-provide-baseline-" +
      std::to_wstring(sequence.fetch_add(1U, std::memory_order_relaxed));
}

void drainCallbacks(RTIambassador& rti) {
  for (int pass = 0; pass != 64; ++pass) {
    static_cast<void>(rti.evokeCallback(0.0));
  }
}

}  // namespace

TEST_CASE(
    "Embedded Auto Provide solicits in-scope owners after discovery",
    "[integration][development-profile][federation-management][object-management]"
    "[auto-provide][callback-evoked]"
    "[rti.service.get-auto-provide-switch]"
    "[rti.service.get-object-class-handle]"
    "[rti.service.get-attribute-handle]"
    "[rti.service.publish-object-class-attributes]"
    "[rti.service.subscribe-object-class-attributes]"
    "[rti.service.register-object-instance]"
    "[federate.callback.discover-object-instance]"
    "[federate.callback.provide-attribute-value-update]") {
  AutoProvideFederateAmbassador ownerReports;
  AutoProvideFederateAmbassador requesterReports;
  auto owner = makeRti();
  auto requester = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("regional-ownership-fanout-fom.xml").wstring();

  REQUIRE_NOTHROW(owner->connect(ownerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(requester->connect(requesterReports, HLA_EVOKED));
  REQUIRE_NOTHROW(owner->createFederationExecution(
      federationName,
      fomModule,
      L"HLAinteger64Time"));
  REQUIRE_NOTHROW(owner->joinFederationExecution(
      L"auto-provide-baseline-owner",
      L"provider",
      federationName));
  REQUIRE_NOTHROW(requester->joinFederationExecution(
      L"auto-provide-baseline-requester",
      L"subscriber",
      federationName));

  auto const objectClass = owner->getObjectClassHandle(
      L"HLAobjectRoot.UmbraRegionalOwnershipFanout");
  auto const providerA = owner->getAttributeHandle(objectClass, L"ProviderAValue");
  auto const providerB = owner->getAttributeHandle(objectClass, L"ProviderBValue");
  REQUIRE(objectClass.isValid());
  REQUIRE(providerA.isValid());
  REQUIRE(providerB.isValid());
  REQUIRE(owner->getAutoProvideSwitch());
  REQUIRE(requester->getAutoProvideSwitch());

  AttributeHandleSet const attributes{providerA, providerB};
  REQUIRE_NOTHROW(owner->publishObjectClassAttributes(objectClass, attributes));
  REQUIRE_NOTHROW(requester->subscribeObjectClassAttributes(objectClass, attributes));

  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = owner->registerObjectInstance(objectClass));
  REQUIRE(objectInstance.isValid());

  // Discovery and the RTI-invoked solicitation are separate callbacks.  The
  // owner receives exactly one grouped request for every owned in-scope
  // attribute, and the RTI supplies the required empty tag.
  drainCallbacks(*requester);
  drainCallbacks(*owner);
  REQUIRE(requesterReports.discoveredObjects.size() == 1U);
  REQUIRE(requesterReports.discoveredObjects.front() == objectInstance);
  REQUIRE(ownerReports.provideReports.size() == 1U);
  REQUIRE(ownerReports.callbackOrder == std::vector<std::string>{"provide"});
  REQUIRE(requesterReports.callbackOrder == std::vector<std::string>{"discover"});

  auto const& provide = ownerReports.provideReports.front();
  REQUIRE(provide.objectInstance == objectInstance);
  REQUIRE(provide.attributes == attributes);
  REQUIRE(provide.userSuppliedTag.size() == 0U);

  REQUIRE_NOTHROW(requester->unsubscribeObjectClassAttributes(objectClass, attributes));
  REQUIRE_NOTHROW(requester->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(owner->resignFederationExecution(
      rti1516_2025::CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(requester->disconnect());
  REQUIRE_NOTHROW(owner->disconnect());
}
