#include <catch2/catch_test_macros.hpp>

#include "hla_test_names.hpp"

#include <RTI/NullFederateAmbassador.h>
#include <RTI/RTI1516.h>

#include <atomic>
#include <filesystem>
#include <memory>
#include <vector>

#ifndef UMBRA_SOURCE_DIRECTORY
#error "The resign-action test requires the Umbra source directory."
#endif

namespace {

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
  return L"resign-action-unconditional-divestiture-" +
      std::to_wstring(sequence.fetch_add(1U, std::memory_order_relaxed));
}

std::filesystem::path resourcePath(std::filesystem::path const& relativePath) {
  return std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "third_party" /
      "ieee1516.2-2025" / "resources" / relativePath;
}

class ReportingFederateAmbassador final
    : public rti1516_2025::NullFederateAmbassador {
 public:
  struct ObjectDiscoveryReport final {
    ObjectInstanceHandle objectInstance;
    ObjectClassHandle objectClass;
  };

  struct OwnershipAssumptionReport final {
    ObjectInstanceHandle objectInstance;
    AttributeHandleSet attributes;
    VariableLengthData userSuppliedTag;
  };

  struct AcquisitionReport final {
    ObjectInstanceHandle objectInstance;
    AttributeHandleSet attributes;
    VariableLengthData userSuppliedTag;
  };

  void discoverObjectInstance(
      ObjectInstanceHandle const& objectInstance,
      ObjectClassHandle const& objectClass,
      std::wstring const&,
      FederateHandle const&) override {
    objectDiscoveryReports.push_back({objectInstance, objectClass});
  }

  void requestAttributeOwnershipAssumption(
      ObjectInstanceHandle const& objectInstance,
      AttributeHandleSet const& offeredAttributes,
      VariableLengthData const& userSuppliedTag) override {
    ownershipAssumptionReports.push_back({
        objectInstance,
        offeredAttributes,
        userSuppliedTag,
    });
  }

  void attributeOwnershipAcquisitionNotification(
      ObjectInstanceHandle const& objectInstance,
      AttributeHandleSet const& securedAttributes,
      VariableLengthData const& userSuppliedTag) override {
    acquisitionReports.push_back({
        objectInstance,
        securedAttributes,
        userSuppliedTag,
    });
  }

  std::vector<ObjectDiscoveryReport> objectDiscoveryReports;
  std::vector<OwnershipAssumptionReport> ownershipAssumptionReports;
  std::vector<AcquisitionReport> acquisitionReports;
};

std::unique_ptr<RTIambassador> makeRti() {
  RTIambassadorFactory factory;
  return factory.createRTIambassador();
}

}  // namespace

TEST_CASE(
    "Standalone resign action unconditionally divests attributes for the 2025 ownership model",
    "[integration][development-profile][federation-management][ownership-management]"
    "[resign-action-unconditional-divestiture][standalone]"
    "[rti.service.resign-federation-execution]"
    "[federate.callback.request-attribute-ownership-assumption]"
    "[rti.service.attribute-ownership-acquisition]") {
  rti1516_2025::NullFederateAmbassador ownerFederate;
  ReportingFederateAmbassador peerReports;
  auto owner = makeRti();
  auto peer = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();

  REQUIRE_NOTHROW(owner->connect(ownerFederate, HLA_EVOKED));
  REQUIRE_NOTHROW(peer->connect(peerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(owner->createFederationExecution(
      federationName, fomModule, L"HLAinteger64Time"));
  REQUIRE_NOTHROW(owner->joinFederationExecution(
      L"resigning-owner", L"publisher", federationName));
  REQUIRE_NOTHROW(peer->joinFederationExecution(
      L"resigning-peer", L"subscriber", federationName));

  auto const server = owner->getObjectClassHandle(fixture_hla::fom::employee_server);
  auto const efficiency = owner->getAttributeHandle(
      server, fixture_hla::fixture::efficiency);
  auto const privilegeToDelete = owner->getAttributeHandle(
      server, L"HLAprivilegeToDeleteObject");
  AttributeHandleSet const efficiencyOnly{efficiency};
  AttributeHandleSet const divestedAttributes{efficiency, privilegeToDelete};
  REQUIRE(server.isValid());
  REQUIRE(efficiency.isValid());
  REQUIRE(privilegeToDelete.isValid());
  REQUIRE_NOTHROW(owner->publishObjectClassAttributes(server, efficiencyOnly));
  REQUIRE_NOTHROW(peer->subscribeObjectClassAttributes(server, efficiencyOnly));
  REQUIRE_NOTHROW(peer->publishObjectClassAttributes(server, efficiencyOnly));

  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = owner->registerObjectInstance(server));
  REQUIRE_FALSE(peer->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(peerReports.objectDiscoveryReports.size() == 1U);

  REQUIRE_THROWS_AS(
      owner->resignFederationExecution(rti1516_2025::NO_ACTION),
      rti1516_2025::FederateOwnsAttributes);
  REQUIRE_NOTHROW(owner->resignFederationExecution(
      rti1516_2025::UNCONDITIONALLY_DIVEST_ATTRIBUTES));
  REQUIRE_FALSE(peer->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(peerReports.ownershipAssumptionReports.size() == 1U);
  auto const& assumption = peerReports.ownershipAssumptionReports.front();
  REQUIRE(assumption.objectInstance == objectInstance);
  REQUIRE(assumption.attributes == divestedAttributes);
  REQUIRE(assumption.userSuppliedTag.size() == 0U);
  REQUIRE_FALSE(peer->isAttributeOwnedByFederate(objectInstance, efficiency));

  unsigned char const acquisitionTagBytes[] = {0xD1, 0x25};
  VariableLengthData const acquisitionTag(
      acquisitionTagBytes, sizeof(acquisitionTagBytes));
  REQUIRE_NOTHROW(peer->attributeOwnershipAcquisition(
      objectInstance, efficiencyOnly, acquisitionTag));
  REQUIRE_FALSE(peer->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(peerReports.acquisitionReports.size() == 1U);
  auto const& notification = peerReports.acquisitionReports.front();
  REQUIRE(notification.objectInstance == objectInstance);
  REQUIRE(notification.attributes == efficiencyOnly);
  REQUIRE(peer->isAttributeOwnedByFederate(objectInstance, efficiency));

  REQUIRE_NOTHROW(peer->resignFederationExecution(
      rti1516_2025::UNCONDITIONALLY_DIVEST_ATTRIBUTES));
  REQUIRE_NOTHROW(peer->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(owner->disconnect());
  REQUIRE_NOTHROW(peer->disconnect());
}
