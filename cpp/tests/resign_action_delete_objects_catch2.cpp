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
#error "The resign-action delete test requires the Umbra source directory."
#endif

namespace {

namespace standard_hla = umbra::detail::hla::wide;
namespace fixture_hla = umbra::test::hla::wide;

using rti1516_2025::AttributeHandle;
using rti1516_2025::AttributeHandleSet;
using rti1516_2025::FederateHandle;
using rti1516_2025::HLA_EVOKED;
using rti1516_2025::ObjectClassHandle;
using rti1516_2025::ObjectInstanceHandle;
using rti1516_2025::RTIambassador;
using rti1516_2025::RTIambassadorFactory;
using rti1516_2025::VariableLengthData;

std::wstring nextFederationName() {
  static std::atomic_uint64_t sequence{0U};
  return L"resign-action-delete-objects-" +
      std::to_wstring(sequence.fetch_add(1U, std::memory_order_relaxed));
}

std::filesystem::path resourcePath(std::filesystem::path const& relativePath) {
  return std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "third_party" /
      "ieee1516.2-2025" / "resources" / relativePath;
}

class ReportingFederateAmbassador final
    : public rti1516_2025::NullFederateAmbassador {
 public:
  struct Discovery final {
    ObjectInstanceHandle objectInstance;
    ObjectClassHandle objectClass;
  };

  struct Removal final {
    ObjectInstanceHandle objectInstance;
    VariableLengthData userSuppliedTag;
    FederateHandle producingFederate;
  };

  void discoverObjectInstance(
      ObjectInstanceHandle const& objectInstance,
      ObjectClassHandle const& objectClass,
      std::wstring const&,
      FederateHandle const&) override {
    discoveries.push_back({objectInstance, objectClass});
  }

  void removeObjectInstance(
      ObjectInstanceHandle const& objectInstance,
      VariableLengthData const& userSuppliedTag,
      FederateHandle const& producingFederate) override {
    removals.push_back({
        objectInstance,
        userSuppliedTag,
        producingFederate,
    });
  }

  std::vector<Discovery> discoveries;
  std::vector<Removal> removals;
};

std::unique_ptr<RTIambassador> makeRti() {
  RTIambassadorFactory factory;
  return factory.createRTIambassador();
}

void drainCallbacks(RTIambassador& rti) {
  for (int pass = 0; pass != 128; ++pass) {
    if (!rti.evokeCallback(0.0)) {
      break;
    }
  }
}

}  // namespace

TEST_CASE(
    "Standalone resign action deletes delete-privileged objects and reports removal",
    "[integration][development-profile][federation-management][object-management]"
    "[resign-action-delete-objects][standalone][2025]"
    "[rti.service.resign-federation-execution]"
    "[federate.callback.remove-object-instance]") {
  rti1516_2025::NullFederateAmbassador ownerCallbacks;
  ReportingFederateAmbassador peerReports;
  auto owner = makeRti();
  auto peer = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();

  REQUIRE_NOTHROW(owner->connect(ownerCallbacks, HLA_EVOKED));
  REQUIRE_NOTHROW(peer->connect(peerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(owner->createFederationExecution(
      federationName,
      fomModule,
      standard_hla::mom::integer64_time));
  FederateHandle ownerHandle;
  REQUIRE_NOTHROW(ownerHandle = owner->joinFederationExecution(
      L"deleting-owner", L"publisher", federationName));
  REQUIRE_NOTHROW(peer->joinFederationExecution(
      L"deleting-peer", L"subscriber", federationName));

  auto const server = owner->getObjectClassHandle(
      fixture_hla::fom::employee_server);
  auto const efficiency = owner->getAttributeHandle(
      server, fixture_hla::fixture::efficiency);
  AttributeHandleSet const efficiencyOnly{efficiency};
  REQUIRE(server.isValid());
  REQUIRE(efficiency.isValid());
  REQUIRE_NOTHROW(owner->publishObjectClassAttributes(server, efficiencyOnly));
  REQUIRE_NOTHROW(peer->subscribeObjectClassAttributes(server, efficiencyOnly));

  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = owner->registerObjectInstance(server));
  auto const objectInstanceName = owner->getObjectInstanceName(objectInstance);
  drainCallbacks(*peer);
  REQUIRE(peerReports.discoveries.size() == 1U);
  REQUIRE(peerReports.discoveries.front().objectInstance == objectInstance);

  // The owner still owns the implicit HLAprivilegeToDeleteObject attribute;
  // NO_ACTION therefore hits the official FederateOwnsAttributes guard.
  REQUIRE_THROWS_AS(
      owner->resignFederationExecution(rti1516_2025::NO_ACTION),
      rti1516_2025::FederateOwnsAttributes);
  REQUIRE(owner->getFederateHandle(L"deleting-owner") == ownerHandle);

  REQUIRE_NOTHROW(owner->resignFederationExecution(rti1516_2025::DELETE_OBJECTS));
  REQUIRE(peerReports.removals.empty());
  drainCallbacks(*peer);
  REQUIRE(peerReports.removals.size() == 1U);
  auto const& removal = peerReports.removals.front();
  REQUIRE(removal.objectInstance == objectInstance);
  REQUIRE(removal.userSuppliedTag.size() == 0U);
  REQUIRE(removal.producingFederate == ownerHandle);
  REQUIRE_THROWS_AS(
      peer->getObjectInstanceHandle(objectInstanceName),
      rti1516_2025::ObjectInstanceNotKnown);

  REQUIRE_NOTHROW(peer->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(peer->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(owner->disconnect());
  REQUIRE_NOTHROW(peer->disconnect());
}
