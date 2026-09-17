#include <catch2/catch_test_macros.hpp>

#include "internal/federation/embedded_transport.hpp"
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
#error "The connection-loss cancellation test requires the Umbra source directory."
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
  return L"connection-loss-automatic-cancel-pending-acquisition-" +
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

  struct OwnershipAssumptionReport final {
    ObjectInstanceHandle objectInstance;
    AttributeHandleSet attributes;
    VariableLengthData userSuppliedTag;
  };

  struct ReleaseRequestReport final {
    ObjectInstanceHandle objectInstance;
    AttributeHandleSet attributes;
    VariableLengthData userSuppliedTag;
  };

  void connectionLost(std::wstring const& description) override {
    faultDescriptions.push_back(description);
  }

  void discoverObjectInstance(
      ObjectInstanceHandle const& objectInstance,
      ObjectClassHandle const& objectClass,
      std::wstring const&,
      FederateHandle const&) override {
    discoveries.push_back({objectInstance, objectClass});
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

  std::vector<std::wstring> faultDescriptions;
  std::vector<Discovery> discoveries;
  std::vector<OwnershipAssumptionReport> ownershipAssumptionReports;
  std::vector<ReleaseRequestReport> releaseRequests;
};

std::unique_ptr<RTIambassador> makeRti() {
  RTIambassadorFactory factory;
  return factory.createRTIambassador();
}

}  // namespace

TEST_CASE(
    "Standalone transport loss cancels the lost federate's pending ownership acquisition",
    "[integration][development-profile][federation-management][transport]"
    "[ownership-management][connection-lost-automatic-cancel-pending-acquisition]"
    "[standalone][2025]"
    "[rti.service.connection-lost]"
    "[rti.service.get-automatic-resign-directive]"
    "[rti.service.set-automatic-resign-directive]"
    "[rti.service.attribute-ownership-acquisition]"
    "[federate.callback.connection-lost]"
    "[federate.callback.request-attribute-ownership-assumption]"
    "[federate.callback.request-attribute-ownership-release]") {
  ReportingFederateAmbassador ownerReports;
  ReportingFederateAmbassador lostReports;
  ReportingFederateAmbassador survivorReports;
  auto owner = makeRti();
  auto lost = makeRti();
  auto survivor = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();

  REQUIRE_NOTHROW(owner->connect(ownerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(lost->connect(lostReports, HLA_EVOKED));
  REQUIRE_NOTHROW(survivor->connect(survivorReports, HLA_EVOKED));
  REQUIRE_NOTHROW(owner->createFederationExecution(
      federationName,
      fomModule,
      standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(owner->joinFederationExecution(
      L"automatic-cancel-owner", L"publisher", federationName));
  REQUIRE_NOTHROW(lost->joinFederationExecution(
      L"automatic-cancel-lost", L"candidate", federationName));
  REQUIRE_NOTHROW(survivor->joinFederationExecution(
      L"automatic-cancel-survivor", L"candidate", federationName));

  auto const server = owner->getObjectClassHandle(fixture_hla::fom::employee_server);
  auto const efficiency = owner->getAttributeHandle(
      server, fixture_hla::fixture::efficiency);
  AttributeHandleSet const efficiencyOnly{efficiency};
  REQUIRE(server.isValid());
  REQUIRE(efficiency.isValid());
  REQUIRE_NOTHROW(owner->publishObjectClassAttributes(server, efficiencyOnly));
  REQUIRE_NOTHROW(lost->publishObjectClassAttributes(server, efficiencyOnly));
  REQUIRE_NOTHROW(lost->subscribeObjectClassAttributes(server, efficiencyOnly));
  REQUIRE_NOTHROW(survivor->publishObjectClassAttributes(server, efficiencyOnly));
  REQUIRE_NOTHROW(survivor->subscribeObjectClassAttributes(server, efficiencyOnly));

  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = owner->registerObjectInstance(server));
  REQUIRE(objectInstance.isValid());
  REQUIRE(lost->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE_FALSE(lost->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(survivor->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE_FALSE(survivor->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(lostReports.discoveries.size() == 1U);
  REQUIRE(survivorReports.discoveries.size() == 1U);
  REQUIRE(lostReports.discoveries.front().objectInstance == objectInstance);
  REQUIRE(survivorReports.discoveries.front().objectInstance == objectInstance);

  unsigned char const acquisitionTagBytes[] = {0xD6, 0x25};
  VariableLengthData const acquisitionTag(
      acquisitionTagBytes, sizeof(acquisitionTagBytes));
  REQUIRE_NOTHROW(lost->attributeOwnershipAcquisition(
      objectInstance, efficiencyOnly, acquisitionTag));
  REQUIRE(ownerReports.releaseRequests.empty());
  REQUIRE_NOTHROW(lost->setAutomaticResignDirective(
      rti1516_2025::CANCEL_PENDING_OWNERSHIP_ACQUISITIONS));
  REQUIRE(
      lost->getAutomaticResignDirective() ==
      rti1516_2025::CANCEL_PENDING_OWNERSHIP_ACQUISITIONS);

  std::wstring const faultDescription =
      L"automatic pending-acquisition cancellation transport fault";
  REQUIRE(umbra::detail::failEmbeddedTransportConnectionForTesting(
      *lost, faultDescription));
  REQUIRE(lostReports.faultDescriptions.empty());
  REQUIRE(ownerReports.releaseRequests.empty());

  REQUIRE_FALSE(lost->evokeCallback(0.0));
  REQUIRE(lostReports.faultDescriptions ==
          std::vector<std::wstring>{faultDescription});
  while (owner->evokeCallback(0.0)) {
  }
  REQUIRE(ownerReports.releaseRequests.empty());
  REQUIRE(survivorReports.ownershipAssumptionReports.empty());

  // The stale release request from the departed requester must not reach the
  // owner. A later divestiture can therefore discover the remaining survivor.
  unsigned char const divestitureTagBytes[] = {0xD7, 0x25};
  VariableLengthData const divestitureTag(
      divestitureTagBytes, sizeof(divestitureTagBytes));
  REQUIRE_NOTHROW(owner->unconditionalAttributeOwnershipDivestiture(
      objectInstance, efficiencyOnly, divestitureTag));
  while (survivor->evokeCallback(0.0)) {
  }
  REQUIRE(survivorReports.ownershipAssumptionReports.size() == 1U);
  auto const& assumption = survivorReports.ownershipAssumptionReports.front();
  REQUIRE(assumption.objectInstance == objectInstance);
  REQUIRE(assumption.attributes == efficiencyOnly);
  REQUIRE(assumption.userSuppliedTag.size() == sizeof(divestitureTagBytes));
  REQUIRE_FALSE(survivor->isAttributeOwnedByFederate(objectInstance, efficiency));

  REQUIRE_NOTHROW(survivor->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(owner->resignFederationExecution(rti1516_2025::DELETE_OBJECTS));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(lost->connect(lostReports, HLA_EVOKED));
  REQUIRE_NOTHROW(lost->disconnect());
  REQUIRE_NOTHROW(survivor->disconnect());
  REQUIRE_NOTHROW(owner->disconnect());
}
