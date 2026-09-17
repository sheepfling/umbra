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
#error "The Local Delete Object Instance test requires the Umbra source directory."
#endif

namespace {

namespace standard_hla = umbra::detail::hla::wide;
namespace fixture_hla = umbra::test::hla::wide;

using rti1516_2025::AttributeHandle;
using rti1516_2025::AttributeHandleSet;
using rti1516_2025::AttributeHandleValueMap;
using rti1516_2025::FederateHandle;
using rti1516_2025::HLA_EVOKED;
using rti1516_2025::ObjectClassHandle;
using rti1516_2025::ObjectInstanceHandle;
using rti1516_2025::RTIambassador;
using rti1516_2025::RTIambassadorFactory;
using rti1516_2025::RegionHandleSet;
using rti1516_2025::TransportationTypeHandle;
using rti1516_2025::VariableLengthData;

std::wstring nextFederationName() {
  static std::atomic_uint64_t sequence{0U};
  return L"local-delete-object-instance-" +
      std::to_wstring(sequence.fetch_add(1U, std::memory_order_relaxed));
}

std::filesystem::path resourcePath(std::filesystem::path const& relativePath) {
  return std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "third_party" /
      "ieee1516.2-2025" / "resources" / relativePath;
}

using TestFederateAmbassador = rti1516_2025::NullFederateAmbassador;

class ReportingFederateAmbassador final
    : public rti1516_2025::NullFederateAmbassador {
 public:
  struct ObjectDiscoveryReport final {
    ObjectInstanceHandle objectInstance;
    ObjectClassHandle objectClass;
    std::wstring objectInstanceName;
    FederateHandle producingFederate;
  };

  struct ReflectionReport final {
    ObjectInstanceHandle objectInstance;
    AttributeHandleValueMap attributeValues;
  };

  void discoverObjectInstance(
      ObjectInstanceHandle const& objectInstance,
      ObjectClassHandle const& objectClass,
      std::wstring const& objectInstanceName,
      FederateHandle const& producingFederate) override {
    objectDiscoveryReports.push_back({
        objectInstance,
        objectClass,
        objectInstanceName,
        producingFederate,
    });
  }

  void reflectAttributeValues(
      ObjectInstanceHandle const& objectInstance,
      AttributeHandleValueMap const& attributeValues,
      VariableLengthData const& userSuppliedTag,
      TransportationTypeHandle const& transportationType,
      FederateHandle const& producingFederate,
      rti1516_2025::RegionHandleSet const* optionalSentRegions) override {
    static_cast<void>(userSuppliedTag);
    static_cast<void>(transportationType);
    static_cast<void>(producingFederate);
    static_cast<void>(optionalSentRegions);
    attributeReflectionReports.push_back({objectInstance, attributeValues});
  }

  std::vector<ObjectDiscoveryReport> objectDiscoveryReports;
  std::vector<ReflectionReport> attributeReflectionReports;
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
    "Embedded Local Delete Object Instance preserves 2025 federation state",
    "[integration][development-profile][object-management]"
    "[local-delete-object-instance]"
    "[rti.service.local-delete-object-instance]") {
  TestFederateAmbassador unjoinedFederate;
  ReportingFederateAmbassador ownerReports;
  ReportingFederateAmbassador requesterReports;
  auto unjoined = makeRti();
  auto owner = makeRti();
  auto requester = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  ObjectInstanceHandle invalidObjectInstance;

  REQUIRE_THROWS_AS(
      owner->localDeleteObjectInstance(invalidObjectInstance),
      rti1516_2025::NotConnected);
  REQUIRE_NOTHROW(unjoined->connect(unjoinedFederate, HLA_EVOKED));
  REQUIRE_THROWS_AS(
      unjoined->localDeleteObjectInstance(invalidObjectInstance),
      rti1516_2025::FederateNotExecutionMember);

  REQUIRE_NOTHROW(owner->connect(ownerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(requester->connect(requesterReports, HLA_EVOKED));
  REQUIRE_NOTHROW(owner->createFederationExecution(
      federationName,
      fomModule,
      standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(owner->joinFederationExecution(
      L"local-delete-owner", L"owner", federationName));
  REQUIRE_NOTHROW(requester->joinFederationExecution(
      L"local-delete-requester", L"requester", federationName));

  auto const server = owner->getObjectClassHandle(
      fixture_hla::fom::employee_server);
  auto const efficiency = owner->getAttributeHandle(
      server,
      fixture_hla::fixture::efficiency);
  AttributeHandleSet const efficiencyOnly{efficiency};
  REQUIRE(server.isValid());
  REQUIRE(efficiency.isValid());
  REQUIRE_NOTHROW(owner->publishObjectClassAttributes(server, efficiencyOnly));
  REQUIRE_NOTHROW(requester->subscribeObjectClassAttributes(server, efficiencyOnly));

  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = owner->registerObjectInstance(server));
  REQUIRE(requesterReports.objectDiscoveryReports.empty());
  drainCallbacks(*requester);
  REQUIRE(requesterReports.objectDiscoveryReports.size() == 1U);
  REQUIRE(requester->getKnownObjectClassHandle(objectInstance) == server);

  // The producer owns the registration-established attribute and therefore
  // cannot use Local Delete Object Instance until ownership has moved away.
  REQUIRE_THROWS_AS(
      owner->localDeleteObjectInstance(objectInstance),
      rti1516_2025::FederateOwnsAttributes);

  unsigned char const tagBytes[] = {0x51, 0x18};
  VariableLengthData const acquisitionTag(tagBytes, sizeof(tagBytes));
  REQUIRE_NOTHROW(requester->publishObjectClassAttributes(server, efficiencyOnly));
  REQUIRE_NOTHROW(requester->attributeOwnershipAcquisition(
      objectInstance,
      efficiencyOnly,
      acquisitionTag));
  REQUIRE_THROWS_AS(
      requester->localDeleteObjectInstance(objectInstance),
      rti1516_2025::OwnershipAcquisitionPending);
  REQUIRE_NOTHROW(requester->resignFederationExecution(
      rti1516_2025::CANCEL_THEN_DELETE_THEN_DIVEST));

  // Rejoin a fresh requester so the local-delete transition itself is tested
  // without carrying an acquisition reservation across the teardown boundary.
  requester = makeRti();
  requesterReports = ReportingFederateAmbassador{};
  REQUIRE_NOTHROW(requester->connect(requesterReports, HLA_EVOKED));
  REQUIRE_NOTHROW(requester->joinFederationExecution(
      L"local-delete-requester-2", L"requester", federationName));
  REQUIRE_NOTHROW(requester->subscribeObjectClassAttributes(server, efficiencyOnly));
  drainCallbacks(*requester);
  REQUIRE(requesterReports.objectDiscoveryReports.size() == 1U);
  REQUIRE_NOTHROW(requester->localDeleteObjectInstance(objectInstance));
  REQUIRE_THROWS_AS(
      requester->getKnownObjectClassHandle(objectInstance),
      rti1516_2025::ObjectInstanceNotKnown);

  // Local deletion does not remove the federation-wide object. Repeating the
  // eligible subscription permits a new discovery, and the owner remains able
  // to update the object for the rediscovered requester.
  REQUIRE_NOTHROW(requester->subscribeObjectClassAttributes(server, efficiencyOnly));
  drainCallbacks(*requester);
  REQUIRE(requesterReports.objectDiscoveryReports.size() == 2U);
  REQUIRE(requester->getKnownObjectClassHandle(objectInstance) == server);

  unsigned char const valueBytes[] = {0x04, 0x20};
  unsigned char const updateTagBytes[] = {0x7A};
  AttributeHandleValueMap values;
  values.emplace(efficiency, VariableLengthData(valueBytes, sizeof(valueBytes)));
  VariableLengthData const updateTag(updateTagBytes, sizeof(updateTagBytes));
  REQUIRE_NOTHROW(owner->updateAttributeValues(objectInstance, values, updateTag));
  drainCallbacks(*requester);
  REQUIRE(requesterReports.attributeReflectionReports.size() == 1U);
  REQUIRE(requesterReports.attributeReflectionReports.front().objectInstance == objectInstance);
  REQUIRE(requesterReports.attributeReflectionReports.front().attributeValues.contains(efficiency));

  REQUIRE_NOTHROW(requester->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(owner->resignFederationExecution(
      rti1516_2025::CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(unjoined->disconnect());
  REQUIRE_NOTHROW(owner->disconnect());
  REQUIRE_NOTHROW(requester->disconnect());
}
