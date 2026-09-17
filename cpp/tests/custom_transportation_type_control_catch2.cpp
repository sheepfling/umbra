#include <catch2/catch_test_macros.hpp>

#include "internal/fom/hla_names.hpp"
#include "hla_test_names.hpp"

#include <RTI/NullFederateAmbassador.h>
#include <RTI/RTI1516.h>

#include <atomic>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#ifndef UMBRA_SOURCE_DIRECTORY
#error "The focused transportation-type control test requires the Umbra source directory."
#endif

namespace {

namespace standard_hla = umbra::detail::hla::wide;
namespace fixture_hla = umbra::test::hla::wide;

using rti1516_2025::AttributeHandle;
using rti1516_2025::AttributeHandleSet;
using rti1516_2025::FederateHandle;
using rti1516_2025::HLA_EVOKED;
using rti1516_2025::InteractionClassHandle;
using rti1516_2025::ObjectClassHandle;
using rti1516_2025::ObjectInstanceHandle;
using rti1516_2025::RTIambassador;
using rti1516_2025::RTIambassadorFactory;
using rti1516_2025::TransportationTypeHandle;

class TransportationFederateAmbassador final
    : public rti1516_2025::NullFederateAmbassador {
 public:
  struct Discovery final {
    ObjectInstanceHandle objectInstance;
    ObjectClassHandle objectClass;
    std::wstring objectInstanceName;
    FederateHandle producingFederate;
  };

  struct AttributeChange final {
    ObjectInstanceHandle objectInstance;
    AttributeHandleSet attributes;
    TransportationTypeHandle transportationType;
  };

  struct AttributeQuery final {
    ObjectInstanceHandle objectInstance;
    AttributeHandle attribute;
    TransportationTypeHandle transportationType;
  };

  void discoverObjectInstance(
      ObjectInstanceHandle const& objectInstance,
      ObjectClassHandle const& objectClass,
      std::wstring const& objectInstanceName,
      FederateHandle const& producingFederate) override {
    discoveries.push_back({
        objectInstance,
        objectClass,
        objectInstanceName,
        producingFederate,
    });
  }

  void confirmAttributeTransportationTypeChange(
      ObjectInstanceHandle const& objectInstance,
      AttributeHandleSet const& attributes,
      TransportationTypeHandle const& transportationType) override {
    attributeChanges.push_back({objectInstance, attributes, transportationType});
  }

  void reportAttributeTransportationType(
      ObjectInstanceHandle const& objectInstance,
      AttributeHandle const& attribute,
      TransportationTypeHandle const& transportationType) override {
    attributeQueries.push_back({objectInstance, attribute, transportationType});
  }

  std::vector<Discovery> discoveries;
  std::vector<AttributeChange> attributeChanges;
  std::vector<AttributeQuery> attributeQueries;
};

std::unique_ptr<RTIambassador> makeRti() {
  RTIambassadorFactory factory;
  return factory.createRTIambassador();
}

std::wstring nextFederationName() {
  static std::atomic_uint64_t sequence{0U};
  return L"focused-attribute-transportation-control-" +
      std::to_wstring(sequence.fetch_add(1U, std::memory_order_relaxed));
}

}  // namespace

TEST_CASE(
    "Embedded focused attribute transportation control preserves declared FOM defaults",
    "[integration][development-profile][federation-management][transportation]"
    "[transportation-management][custom-transportation]"
    "[custom-transportation-type-control]"
    "[rti.service.get-object-class-handle][rti.service.get-attribute-handle]"
    "[rti.service.get-transportation-type-handle]"
    "[rti.service.publish-object-class-attributes]"
    "[rti.service.subscribe-object-class-attributes]"
    "[rti.service.change-default-attribute-transportation-type]"
    "[rti.service.register-object-instance]"
    "[rti.service.request-attribute-transportation-type-change]"
    "[rti.service.query-attribute-transportation-type]"
    "[rti.service.evoke-callback]"
    "[federate.callback.discover-object-instance]"
    "[federate.callback.confirm-attribute-transportation-type-change]"
    "[federate.callback.report-attribute-transportation-type]") {
  TransportationFederateAmbassador ownerReports;
  TransportationFederateAmbassador peerReports;
  auto owner = makeRti();
  auto peer = makeRti();
  auto const federationName = nextFederationName();
  auto const testData =
      std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data";
  std::vector<std::wstring> const fomModules{
      (testData / "transportation-reference-consumer-fom.xml").wstring(),
      (testData / "transportation-reference-provider-fom.xml").wstring(),
  };

  REQUIRE_NOTHROW(owner->connect(ownerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(peer->connect(peerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(owner->createFederationExecution(
      federationName,
      fomModules,
      standard_hla::mom::integer64_time));
  FederateHandle ownerHandle;
  REQUIRE_NOTHROW(ownerHandle = owner->joinFederationExecution(
      L"focused-attribute-transportation-owner",
      L"owner",
      federationName));
  REQUIRE_NOTHROW(peer->joinFederationExecution(
      L"focused-attribute-transportation-peer",
      L"peer",
      federationName));
  auto const drainOwnerCallbacks = [&] {
    bool delivered = false;
    while (owner->evokeCallback(0.0)) {
      delivered = true;
    }
    return delivered;
  };

  auto const ownerObjectClass = owner->getObjectClassHandle(
      fixture_hla::fom::transportation_fixture_object);
  auto const peerObjectClass = peer->getObjectClassHandle(
      fixture_hla::fom::transportation_fixture_object);
  auto const ownerAttribute = owner->getAttributeHandle(
      ownerObjectClass,
      fixture_hla::fixture::umbra_transportation_fixture_attribute);
  auto const peerAttribute = peer->getAttributeHandle(
      peerObjectClass,
      fixture_hla::fixture::umbra_transportation_fixture_attribute);
  auto const reliable = owner->getTransportationTypeHandle(
      standard_hla::mom::reliable);
  auto const custom = owner->getTransportationTypeHandle(
      fixture_hla::fixture::umbra_transportation_fixture);
  REQUIRE(ownerObjectClass.isValid());
  REQUIRE(peerObjectClass.isValid());
  REQUIRE(ownerAttribute.isValid());
  REQUIRE(peerAttribute.isValid());
  REQUIRE(reliable.isValid());
  REQUIRE(custom.isValid());
  REQUIRE(custom != reliable);

  REQUIRE_NOTHROW(owner->publishObjectClassAttributes(
      ownerObjectClass,
      AttributeHandleSet{ownerAttribute}));
  REQUIRE_NOTHROW(peer->subscribeObjectClassAttributes(
      peerObjectClass,
      AttributeHandleSet{peerAttribute}));

  // The first object captures the standard override; the second object
  // captures the later custom default. Existing instances are not rewritten.
  REQUIRE_NOTHROW(owner->changeDefaultAttributeTransportationType(
      ownerObjectClass,
      AttributeHandleSet{ownerAttribute},
      reliable));
  ObjectInstanceHandle firstObject;
  REQUIRE_NOTHROW(firstObject = owner->registerObjectInstance(ownerObjectClass));
  while (peer->evokeCallback(0.0)) {
  }
  REQUIRE(peerReports.discoveries.size() == 1U);
  REQUIRE(peerReports.discoveries.front().objectClass == peerObjectClass);

  REQUIRE_NOTHROW(owner->changeDefaultAttributeTransportationType(
      ownerObjectClass,
      AttributeHandleSet{ownerAttribute},
      custom));
  ObjectInstanceHandle secondObject;
  REQUIRE_NOTHROW(secondObject = owner->registerObjectInstance(ownerObjectClass));
  while (peer->evokeCallback(0.0)) {
  }
  REQUIRE(peerReports.discoveries.size() == 2U);

  REQUIRE_NOTHROW(owner->queryAttributeTransportationType(
      secondObject,
      ownerAttribute));
  static_cast<void>(drainOwnerCallbacks());
  REQUIRE(ownerReports.attributeQueries.size() == 1U);
  REQUIRE(ownerReports.attributeQueries.back().objectInstance == secondObject);
  REQUIRE(ownerReports.attributeQueries.back().attribute == ownerAttribute);
  REQUIRE(ownerReports.attributeQueries.back().transportationType == custom);

  // The instance-level request commits at the confirmation callback boundary.
  REQUIRE_NOTHROW(owner->requestAttributeTransportationTypeChange(
      firstObject,
      AttributeHandleSet{ownerAttribute},
      custom));
  REQUIRE(ownerReports.attributeChanges.empty());
  static_cast<void>(drainOwnerCallbacks());
  REQUIRE(ownerReports.attributeChanges.size() == 1U);
  REQUIRE(ownerReports.attributeChanges.front().objectInstance == firstObject);
  REQUIRE(ownerReports.attributeChanges.front().attributes ==
          AttributeHandleSet{ownerAttribute});
  REQUIRE(ownerReports.attributeChanges.front().transportationType == custom);

  REQUIRE_NOTHROW(owner->queryAttributeTransportationType(
      firstObject,
      ownerAttribute));
  static_cast<void>(drainOwnerCallbacks());
  REQUIRE(ownerReports.attributeQueries.size() == 2U);
  REQUIRE(ownerReports.attributeQueries.back().objectInstance == firstObject);
  REQUIRE(ownerReports.attributeQueries.back().transportationType == custom);

  // A peer cannot mutate the owner's per-instance transportation state merely
  // by observing the same FOM; its callback ledger remains discovery-only.
  REQUIRE(peerReports.attributeChanges.empty());
  REQUIRE(peerReports.attributeQueries.empty());
  REQUIRE_NOTHROW(peer->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(owner->resignFederationExecution(
      rti1516_2025::CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(peer->disconnect());
  REQUIRE_NOTHROW(owner->disconnect());
}
