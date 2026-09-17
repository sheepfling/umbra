#include <catch2/catch_test_macros.hpp>

#include "internal/fom/hla_names.hpp"
#include "hla_test_names.hpp"

#include <RTI/NullFederateAmbassador.h>
#include <RTI/RTI1516.h>

#include <atomic>
#include <filesystem>
#include <memory>
#include <vector>

#ifndef UMBRA_SOURCE_DIRECTORY
#error "The disabled Auto Provide test requires the Umbra source directory."
#endif

namespace {

namespace standard_hla = umbra::detail::hla::wide;
namespace fixture_hla = umbra::test::hla::wide;

using rti1516_2025::AttributeHandle;
using rti1516_2025::AttributeHandleSet;
using rti1516_2025::AttributeHandleValueMap;
using rti1516_2025::HLA_EVOKED;
using rti1516_2025::ObjectClassHandle;
using rti1516_2025::ObjectInstanceHandle;
using rti1516_2025::RTIambassador;
using rti1516_2025::RTIambassadorFactory;
using rti1516_2025::VariableLengthData;

std::wstring nextFederationName() {
  static std::atomic_uint64_t sequence{0U};
  return L"auto-provide-disabled-discovery-only-" +
      std::to_wstring(sequence.fetch_add(1U, std::memory_order_relaxed));
}

std::filesystem::path testDataPath(std::filesystem::path const& relativePath) {
  return std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" /
      "data" / relativePath;
}

class ReportingFederateAmbassador final
    : public rti1516_2025::NullFederateAmbassador {
 public:
  void discoverObjectInstance(
      ObjectInstanceHandle const& objectInstance,
      ObjectClassHandle const&,
      std::wstring const&,
      rti1516_2025::FederateHandle const&) override {
    discoveredObjectInstances.push_back(objectInstance);
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

  struct AttributeValueUpdateRequestReport final {
    ObjectInstanceHandle objectInstance;
    AttributeHandleSet attributes;
    VariableLengthData userSuppliedTag;
  };

  std::vector<ObjectInstanceHandle> discoveredObjectInstances;
  std::vector<AttributeValueUpdateRequestReport>
      attributeValueUpdateRequestReports;
};

std::unique_ptr<RTIambassador> makeRti() {
  RTIambassadorFactory factory;
  return factory.createRTIambassador();
}

}  // namespace

TEST_CASE(
    "Embedded disabled Auto Provide leaves discovery without a provider callback",
    "[integration][development-profile][object-management][auto-provide]"
    "[rti.service.get-auto-provide-switch]"
    "[federate.callback.discover-object-instance]"
    "[federate.callback.provide-attribute-value-update]") {
  ReportingFederateAmbassador ownerReports;
  ReportingFederateAmbassador requesterReports;
  auto owner = makeRti();
  auto requester = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = testDataPath("attribute-update-passel-fom.xml").wstring();

  REQUIRE_NOTHROW(owner->connect(ownerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(requester->connect(requesterReports, HLA_EVOKED));
  REQUIRE_NOTHROW(owner->createFederationExecution(
      federationName,
      fomModule,
      standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(owner->joinFederationExecution(
      L"auto-provide-disabled-owner", L"publisher", federationName));
  REQUIRE_NOTHROW(requester->joinFederationExecution(
      L"auto-provide-disabled-requester", L"subscriber", federationName));

  REQUIRE_FALSE(owner->getAutoProvideSwitch());
  REQUIRE_FALSE(requester->getAutoProvideSwitch());
  auto const child = owner->getObjectClassHandle(
      fixture_hla::fom::attribute_fixture_child);
  auto const reliableBaseA = owner->getAttributeHandle(
      child,
      fixture_hla::fixture::reliable_base_a);
  REQUIRE(child.isValid());
  REQUIRE(reliableBaseA.isValid());
  AttributeHandleSet const ownedAttributes{reliableBaseA};
  REQUIRE_NOTHROW(owner->publishObjectClassAttributes(child, ownedAttributes));
  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = owner->registerObjectInstance(child));

  REQUIRE_NOTHROW(requester->subscribeObjectClassAttributes(
      child,
      ownedAttributes));
  REQUIRE_FALSE(requester->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(requesterReports.discoveredObjectInstances.size() == 1U);
  REQUIRE_FALSE(owner->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(ownerReports.attributeValueUpdateRequestReports.empty());

  REQUIRE_NOTHROW(requester->resignFederationExecution(
      rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(owner->resignFederationExecution(
      rti1516_2025::CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(requester->disconnect());
  REQUIRE_NOTHROW(owner->disconnect());
}
