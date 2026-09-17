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
#error "The resign-action cancellation test requires the Umbra source directory."
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
  return L"resign-action-cancel-pending-acquisition-" +
      std::to_wstring(sequence.fetch_add(1U, std::memory_order_relaxed));
}

std::filesystem::path resourcePath(std::filesystem::path const& relativePath) {
  return std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "third_party" /
      "ieee1516.2-2025" / "resources" / relativePath;
}

class ReportingFederateAmbassador final
    : public rti1516_2025::NullFederateAmbassador {
 public:
  struct ReleaseRequest final {
    ObjectInstanceHandle objectInstance;
    AttributeHandleSet attributes;
    VariableLengthData userSuppliedTag;
  };

  void discoverObjectInstance(
      ObjectInstanceHandle const& objectInstance,
      ObjectClassHandle const& objectClass,
      std::wstring const&,
      FederateHandle const&) override {
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

  struct Discovery final {
    ObjectInstanceHandle objectInstance;
    ObjectClassHandle objectClass;
  };

  std::vector<Discovery> discoveries;
  std::vector<ReleaseRequest> releaseRequests;
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
    "Standalone resign action cancels pending ownership acquisition work",
    "[integration][development-profile][federation-management][ownership-management]"
    "[resign-action-cancel-pending-acquisition][standalone][2025]"
    "[rti.service.resign-federation-execution]"
    "[rti.service.attribute-ownership-acquisition]"
    "[federate.callback.request-attribute-ownership-release]") {
  ReportingFederateAmbassador requesterReports;
  ReportingFederateAmbassador ownerReports;
  auto requester = makeRti();
  auto owner = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();

  REQUIRE_NOTHROW(requester->connect(requesterReports, HLA_EVOKED));
  REQUIRE_NOTHROW(owner->connect(ownerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(requester->createFederationExecution(
      federationName,
      fomModule,
      standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(requester->joinFederationExecution(
      L"canceling-requester", L"subscriber", federationName));
  REQUIRE_NOTHROW(owner->joinFederationExecution(
      L"canceling-owner", L"publisher", federationName));

  auto const server = requester->getObjectClassHandle(
      fixture_hla::fom::employee_server);
  auto const efficiency = requester->getAttributeHandle(
      server, fixture_hla::fixture::efficiency);
  AttributeHandleSet const efficiencyOnly{efficiency};
  REQUIRE(server.isValid());
  REQUIRE(efficiency.isValid());
  REQUIRE_NOTHROW(requester->subscribeObjectClassAttributes(server, efficiencyOnly));
  REQUIRE_NOTHROW(owner->publishObjectClassAttributes(server, efficiencyOnly));

  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = owner->registerObjectInstance(server));
  drainCallbacks(*requester);
  REQUIRE(requesterReports.discoveries.size() == 1U);
  REQUIRE(requesterReports.discoveries.front().objectInstance == objectInstance);

  // Publication after discovery makes the requester eligible for a regular
  // acquisition while the remote federate remains the current owner.
  REQUIRE_NOTHROW(requester->publishObjectClassAttributes(server, efficiencyOnly));
  unsigned char const acquisitionTagBytes[] = {0xD8, 0x25};
  VariableLengthData const acquisitionTag(
      acquisitionTagBytes, sizeof(acquisitionTagBytes));
  REQUIRE_NOTHROW(requester->attributeOwnershipAcquisition(
      objectInstance, efficiencyOnly, acquisitionTag));
  REQUIRE(ownerReports.releaseRequests.empty());
  REQUIRE(owner->isAttributeOwnedByFederate(objectInstance, efficiency));
  REQUIRE_FALSE(requester->isAttributeOwnedByFederate(objectInstance, efficiency));

  // Directive 3 cancels only the resigning requester's pending acquisition.
  // The already queued owner callback must become stale and never be delivered.
  REQUIRE_NOTHROW(requester->resignFederationExecution(
      rti1516_2025::CANCEL_PENDING_OWNERSHIP_ACQUISITIONS));
  drainCallbacks(*owner);
  REQUIRE(ownerReports.releaseRequests.empty());
  REQUIRE(owner->isAttributeOwnedByFederate(objectInstance, efficiency));

  REQUIRE_NOTHROW(owner->resignFederationExecution(rti1516_2025::DELETE_OBJECTS));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(requester->disconnect());
  REQUIRE_NOTHROW(owner->disconnect());
}
