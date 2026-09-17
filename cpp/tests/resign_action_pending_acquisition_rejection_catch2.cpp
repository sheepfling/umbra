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
#error "The resign-action pending-acquisition test requires the Umbra source directory."
#endif

namespace {

namespace standard_hla = umbra::detail::hla::wide;
namespace fixture_hla = umbra::test::hla::wide;

using rti1516_2025::AttributeHandle;
using rti1516_2025::AttributeHandleSet;
using rti1516_2025::HLA_EVOKED;
using rti1516_2025::ObjectClassHandle;
using rti1516_2025::ObjectInstanceHandle;
using rti1516_2025::RTIambassador;
using rti1516_2025::RTIambassadorFactory;
using rti1516_2025::VariableLengthData;

std::wstring nextFederationName() {
  static std::atomic_uint64_t sequence{0U};
  return L"resign-action-pending-acquisition-rejection-" +
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

  struct ReleaseRequest final {
    ObjectInstanceHandle objectInstance;
    AttributeHandleSet attributes;
    VariableLengthData userSuppliedTag;
  };

  void discoverObjectInstance(
      ObjectInstanceHandle const& objectInstance,
      ObjectClassHandle const& objectClass,
      std::wstring const&,
      rti1516_2025::FederateHandle const&) override {
    discoveries.push_back({objectInstance, objectClass});
  }

  void requestAttributeOwnershipRelease(
      ObjectInstanceHandle const& objectInstance,
      AttributeHandleSet const& candidateAttributes,
      VariableLengthData const& userSuppliedTag) override {
    releaseRequests.push_back({
        objectInstance,
        candidateAttributes,
        userSuppliedTag,
    });
  }

  std::vector<Discovery> discoveries;
  std::vector<ReleaseRequest> releaseRequests;
};

std::unique_ptr<RTIambassador> makeRti() {
  RTIambassadorFactory factory;
  return factory.createRTIambassador();
}

}  // namespace

TEST_CASE(
    "Standalone resign action rejects pending ownership acquisition work",
    "[integration][development-profile][federation-management][ownership-management]"
    "[resign-action-pending-acquisition-rejection][standalone][2025]"
    "[rti.service.resign-federation-execution]"
    "[rti.service.attribute-ownership-acquisition]"
    "[federate.callback.request-attribute-ownership-release]") {
  rti1516_2025::NullFederateAmbassador ownerCallbacks;
  ReportingFederateAmbassador requesterReports;
  auto owner = makeRti();
  auto requester = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();

  REQUIRE_NOTHROW(owner->connect(ownerCallbacks, HLA_EVOKED));
  REQUIRE_NOTHROW(requester->connect(requesterReports, HLA_EVOKED));
  REQUIRE_NOTHROW(owner->createFederationExecution(
      federationName,
      fomModule,
      standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(owner->joinFederationExecution(
      L"pending-owner", L"subscriber", federationName));
  REQUIRE_NOTHROW(requester->joinFederationExecution(
      L"pending-requester", L"publisher", federationName));

  auto const server = owner->getObjectClassHandle(fixture_hla::fom::employee_server);
  auto const efficiency = owner->getAttributeHandle(
      server, fixture_hla::fixture::efficiency);
  AttributeHandleSet const efficiencyOnly{efficiency};
  REQUIRE(server.isValid());
  REQUIRE(efficiency.isValid());
  REQUIRE_NOTHROW(owner->subscribeObjectClassAttributes(server, efficiencyOnly));
  REQUIRE_NOTHROW(requester->publishObjectClassAttributes(server, efficiencyOnly));

  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = requester->registerObjectInstance(server));
  while (owner->evokeCallback(0.0)) {
  }
  REQUIRE(requesterReports.discoveries.size() == 0U);
  REQUIRE(owner->getKnownObjectClassHandle(objectInstance) == server);

  // Publication after discovery makes the owner eligible to request the
  // regular acquisition while the producer remains the current owner.
  REQUIRE_NOTHROW(owner->publishObjectClassAttributes(server, efficiencyOnly));
  unsigned char const acquisitionTagBytes[] = {0xD2, 0x25};
  VariableLengthData const acquisitionTag(
      acquisitionTagBytes, sizeof(acquisitionTagBytes));
  REQUIRE_NOTHROW(owner->attributeOwnershipAcquisition(
      objectInstance, efficiencyOnly, acquisitionTag));
  REQUIRE(requesterReports.releaseRequests.empty());

  REQUIRE_THROWS_AS(
      owner->resignFederationExecution(
          rti1516_2025::UNCONDITIONALLY_DIVEST_ATTRIBUTES),
      rti1516_2025::OwnershipAcquisitionPending);
  REQUIRE(owner->getFederateHandle(L"pending-owner").isValid());

  // Directive 5 resolves the pending request before resignation; the queued
  // owner-release callback must be stale and must not be delivered.
  REQUIRE_NOTHROW(owner->resignFederationExecution(
      rti1516_2025::CANCEL_THEN_DELETE_THEN_DIVEST));
  while (requester->evokeCallback(0.0)) {
  }
  REQUIRE(requesterReports.releaseRequests.empty());

  REQUIRE_NOTHROW(requester->resignFederationExecution(
      rti1516_2025::DELETE_OBJECTS));
  REQUIRE_NOTHROW(requester->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(owner->disconnect());
  REQUIRE_NOTHROW(requester->disconnect());
}
