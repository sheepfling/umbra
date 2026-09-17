#include <catch2/catch_test_macros.hpp>

#include "internal/fom/hla_names.hpp"

#include <RTI/NullFederateAmbassador.h>
#include <RTI/RTI1516.h>
#include <RTI/encoding/BasicDataElements.h>

#include <atomic>
#include <cstddef>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#ifndef UMBRA_SOURCE_DIRECTORY
#error "The federation-wide MOM Auto Provide test requires the Umbra source directory."
#endif

namespace {

namespace standard_hla = umbra::detail::hla::wide;

using rti1516_2025::AttributeHandle;
using rti1516_2025::AttributeHandleSet;
using rti1516_2025::AttributeHandleValueMap;
using rti1516_2025::FederateHandle;
using rti1516_2025::HLA_EVOKED;
using rti1516_2025::ObjectClassHandle;
using rti1516_2025::ObjectInstanceHandle;
using rti1516_2025::ParameterHandle;
using rti1516_2025::ParameterHandleValueMap;
using rti1516_2025::RTIambassador;
using rti1516_2025::RTIambassadorFactory;
using rti1516_2025::VariableLengthData;

std::vector<unsigned char> variableLengthDataBytes(
    VariableLengthData const& value) {
  auto const* bytes = static_cast<unsigned char const*>(value.data());
  if (bytes == nullptr) {
    return {};
  }
  return {bytes, bytes + value.size()};
}

std::wstring nextFederationName() {
  static std::atomic_uint64_t sequence{0U};
  return L"auto-provide-mom-" +
      std::to_wstring(sequence.fetch_add(1U, std::memory_order_relaxed));
}

std::filesystem::path resourcePath(std::filesystem::path const& relativePath) {
  return std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" /
      "data" / relativePath;
}

class ReportingFederateAmbassador final
    : public rti1516_2025::NullFederateAmbassador {
 public:
  struct DiscoveryReport final {
    ObjectInstanceHandle objectInstance;
    ObjectClassHandle objectClass;
    std::wstring objectInstanceName;
  };

  struct ProvideReport final {
    ObjectInstanceHandle objectInstance;
    AttributeHandleSet attributes;
    VariableLengthData userSuppliedTag;
  };

  void discoverObjectInstance(
      ObjectInstanceHandle const& objectInstance,
      ObjectClassHandle const& objectClass,
      std::wstring const& objectInstanceName,
      FederateHandle const&) override {
    objectDiscoveryReports.push_back({
        objectInstance,
        objectClass,
        objectInstanceName,
    });
  }

  void provideAttributeValueUpdate(
      ObjectInstanceHandle const& objectInstance,
      AttributeHandleSet const& attributes,
      VariableLengthData const& userSuppliedTag) override {
    attributeValueUpdateRequestReports.push_back({
        objectInstance,
        attributes,
        userSuppliedTag,
    });
  }

  std::vector<DiscoveryReport> objectDiscoveryReports;
  std::vector<ProvideReport> attributeValueUpdateRequestReports;
};

std::unique_ptr<RTIambassador> makeRti() {
  RTIambassadorFactory factory;
  return factory.createRTIambassador();
}

void drainCallbacks(RTIambassador& rti) {
  for (int pass = 0; pass != 64; ++pass) {
    if (!rti.evokeMultipleCallbacks(0.0, 0.0)) {
      break;
    }
  }
}

VariableLengthData encodeSwitch(bool const enabled) {
  auto const encoded = rti1516_2025::HLAinteger32BE(
      enabled ? 1 : 0).encode();
  return encoded;
}

}  // namespace

TEST_CASE(
    "Embedded MOM HLAsetSwitches adjusts federation-wide Auto Provide",
    "[integration][development-profile][federation-management][mom][auto-provide]"
    "[mom-auto-provide-switch-mutation][2025]"
    "[rti.service.send-interaction]"
    "[rti.service.get-auto-provide-switch]"
    "[federate.callback.discover-object-instance]"
    "[federate.callback.provide-attribute-value-update]") {
  ReportingFederateAmbassador ownerReports;
  ReportingFederateAmbassador requesterReports;
  auto owner = makeRti();
  auto requester = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule =
      resourcePath("attribute-update-passel-fom.xml").wstring();

  REQUIRE_NOTHROW(owner->connect(ownerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(requester->connect(requesterReports, HLA_EVOKED));
  REQUIRE_NOTHROW(owner->createFederationExecution(
      federationName,
      fomModule,
      standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(owner->joinFederationExecution(
      L"auto-provide-mom-owner",
      L"publisher",
      federationName));
  REQUIRE_NOTHROW(requester->joinFederationExecution(
      L"auto-provide-mom-requester",
      L"subscriber",
      federationName));

  REQUIRE_FALSE(owner->getAutoProvideSwitch());
  REQUIRE_FALSE(requester->getAutoProvideSwitch());

  auto const setSwitches = owner->getInteractionClassHandle(
      standard_hla::mom::set_switches_federation);
  auto const autoProvideParameter = owner->getParameterHandle(
      setSwitches,
      standard_hla::mom::auto_provide);
  REQUIRE(setSwitches.isValid());
  REQUIRE(autoProvideParameter.isValid());

  std::vector<unsigned char> const invalidSwitchBytes{0U, 0U, 0U, 2U};
  ParameterHandleValueMap invalidSwitchValues{
      {autoProvideParameter,
       VariableLengthData(
           invalidSwitchBytes.data(),
           invalidSwitchBytes.size())}};
  REQUIRE_THROWS_AS(
      owner->sendInteraction(
          setSwitches,
          invalidSwitchValues,
          VariableLengthData()),
      rti1516_2025::RTIinternalError);
  REQUIRE_FALSE(owner->getAutoProvideSwitch());
  REQUIRE_FALSE(requester->getAutoProvideSwitch());

  auto const child = owner->getObjectClassHandle(
      L"HLAobjectRoot.UmbraAttributeFixtureBase.UmbraAttributeFixtureChild");
  auto const reliableBaseA = owner->getAttributeHandle(child, L"ReliableBaseA");
  auto const reliableChild = owner->getAttributeHandle(child, L"ReliableChild");
  REQUIRE(child.isValid());
  REQUIRE(reliableBaseA.isValid());
  REQUIRE(reliableChild.isValid());
  AttributeHandleSet const ownedAttributes{reliableBaseA, reliableChild};
  REQUIRE_NOTHROW(owner->publishObjectClassAttributes(child, ownedAttributes));
  REQUIRE_NOTHROW(requester->subscribeObjectClassAttributes(child, ownedAttributes));

  ObjectInstanceHandle firstObject;
  REQUIRE_NOTHROW(firstObject = owner->registerObjectInstance(child));
  drainCallbacks(*requester);
  REQUIRE(requesterReports.objectDiscoveryReports.size() == 1U);
  drainCallbacks(*owner);
  REQUIRE(ownerReports.attributeValueUpdateRequestReports.empty());

  ParameterHandleValueMap const enabledSwitchValues{
      {autoProvideParameter, encodeSwitch(true)}};
  REQUIRE_NOTHROW(owner->sendInteraction(
      setSwitches,
      enabledSwitchValues,
      VariableLengthData()));
  REQUIRE(owner->getAutoProvideSwitch());
  REQUIRE(requester->getAutoProvideSwitch());

  ObjectInstanceHandle secondObject;
  REQUIRE_NOTHROW(secondObject = owner->registerObjectInstance(child));
  drainCallbacks(*requester);
  REQUIRE(requesterReports.objectDiscoveryReports.size() == 2U);
  REQUIRE(ownerReports.attributeValueUpdateRequestReports.empty());
  drainCallbacks(*owner);
  REQUIRE(ownerReports.attributeValueUpdateRequestReports.size() == 1U);
  auto const& enabledRequest =
      ownerReports.attributeValueUpdateRequestReports.front();
  REQUIRE(enabledRequest.objectInstance == secondObject);
  REQUIRE(enabledRequest.attributes == ownedAttributes);
  REQUIRE(variableLengthDataBytes(enabledRequest.userSuppliedTag).empty());

  ParameterHandleValueMap const disabledSwitchValues{
      {autoProvideParameter, encodeSwitch(false)}};
  REQUIRE_NOTHROW(requester->sendInteraction(
      setSwitches,
      disabledSwitchValues,
      VariableLengthData()));
  REQUIRE_FALSE(owner->getAutoProvideSwitch());
  REQUIRE_FALSE(requester->getAutoProvideSwitch());

  ObjectInstanceHandle thirdObject;
  REQUIRE_NOTHROW(thirdObject = owner->registerObjectInstance(child));
  drainCallbacks(*requester);
  REQUIRE(requesterReports.objectDiscoveryReports.size() == 3U);
  drainCallbacks(*owner);
  REQUIRE(ownerReports.attributeValueUpdateRequestReports.size() == 1U);

  REQUIRE_NOTHROW(requester->resignFederationExecution(
      rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(owner->resignFederationExecution(
      rti1516_2025::CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(requester->disconnect());
  REQUIRE_NOTHROW(owner->disconnect());
}
