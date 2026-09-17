#include <catch2/catch_test_macros.hpp>

#include "hla_test_names.hpp"
#include "internal/fom/hla_names.hpp"

#include <RTI/NullFederateAmbassador.h>
#include <RTI/RTI1516.h>

#include <atomic>
#include <filesystem>
#include <limits>
#include <memory>
#include <string>

#ifndef UMBRA_SOURCE_DIRECTORY
#error "The handle-normalization tests require the Umbra source directory."
#endif

namespace {

namespace standard_hla = umbra::detail::hla::wide;
namespace fixture_hla = umbra::test::hla::wide;

using rti1516_2025::AttributeHandle;
using rti1516_2025::AttributeHandleSet;
using rti1516_2025::FederateHandle;
using rti1516_2025::HLA_EVOKED;
using rti1516_2025::NO_ACTION;
using rti1516_2025::NullFederateAmbassador;
using rti1516_2025::ObjectClassHandle;
using rti1516_2025::ObjectInstanceHandle;
using rti1516_2025::RTIambassador;
using rti1516_2025::RTIambassadorFactory;
using rti1516_2025::ServiceGroup;
using rti1516_2025::InteractionClassHandle;
using rti1516_2025::VariableLengthData;

std::unique_ptr<RTIambassador> makeRti() {
  RTIambassadorFactory factory;
  return factory.createRTIambassador();
}

std::filesystem::path resourcePath(std::filesystem::path const& relativePath) {
  return std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
      "third_party" / "ieee1516.2-2025" / "resources" / relativePath;
}

std::wstring nextFederationName() {
  static std::atomic_uint64_t sequence{0U};
  return L"umbra-handle-normalization-" +
      std::to_wstring(sequence.fetch_add(1U, std::memory_order_relaxed));
}

void requirePointCoordinate(unsigned long value) {
  REQUIRE(value < std::numeric_limits<unsigned long>::max());
}

}  // namespace

TEST_CASE(
    "Embedded handle normalization supplies stable DDM point-range coordinates",
    "[integration][development-profile][federation-management][support-services]"
    "[ddm][handle-normalization][2025]"
    "[rti.service.normalize-service-group]"
    "[rti.service.normalize-federate-handle]"
    "[rti.service.normalize-object-class-handle]"
    "[rti.service.normalize-interaction-class-handle]"
    "[rti.service.normalize-object-instance-handle]") {
  NullFederateAmbassador unjoinedReports;
  NullFederateAmbassador ownerReports;
  NullFederateAmbassador peerReports;
  auto unjoined = makeRti();
  auto owner = makeRti();
  auto peer = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();

  // The official services fence both connection and joined membership before
  // inspecting any caller-supplied designator.
  REQUIRE_THROWS_AS(
      unjoined->normalizeServiceGroup(rti1516_2025::SUPPORT_SERVICES),
      rti1516_2025::NotConnected);
  REQUIRE_THROWS_AS(
      unjoined->normalizeFederateHandle(FederateHandle{}),
      rti1516_2025::NotConnected);
  REQUIRE_THROWS_AS(
      unjoined->normalizeObjectClassHandle(ObjectClassHandle{}),
      rti1516_2025::NotConnected);
  REQUIRE_THROWS_AS(
      unjoined->normalizeInteractionClassHandle(InteractionClassHandle{}),
      rti1516_2025::NotConnected);
  REQUIRE_THROWS_AS(
      unjoined->normalizeObjectInstanceHandle(ObjectInstanceHandle{}),
      rti1516_2025::NotConnected);

  REQUIRE_NOTHROW(unjoined->connect(unjoinedReports, HLA_EVOKED));
  REQUIRE_THROWS_AS(
      unjoined->normalizeServiceGroup(rti1516_2025::SUPPORT_SERVICES),
      rti1516_2025::FederateNotExecutionMember);
  REQUIRE_THROWS_AS(
      unjoined->normalizeFederateHandle(FederateHandle{}),
      rti1516_2025::FederateNotExecutionMember);
  REQUIRE_THROWS_AS(
      unjoined->normalizeObjectClassHandle(ObjectClassHandle{}),
      rti1516_2025::FederateNotExecutionMember);
  REQUIRE_THROWS_AS(
      unjoined->normalizeInteractionClassHandle(InteractionClassHandle{}),
      rti1516_2025::FederateNotExecutionMember);
  REQUIRE_THROWS_AS(
      unjoined->normalizeObjectInstanceHandle(ObjectInstanceHandle{}),
      rti1516_2025::FederateNotExecutionMember);

  REQUIRE_NOTHROW(owner->connect(ownerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(peer->connect(peerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(owner->createFederationExecution(
      federationName, fomModule, standard_hla::mom::integer64_time));

  FederateHandle ownerFederate;
  REQUIRE_NOTHROW(ownerFederate = owner->joinFederationExecution(
      L"handle-normalization-owner", L"owner", federationName));
  REQUIRE_NOTHROW(peer->joinFederationExecution(
      L"handle-normalization-peer", L"peer", federationName));

  auto const server = owner->getObjectClassHandle(fixture_hla::fom::employee_server);
  auto const peerServer = peer->getObjectClassHandle(fixture_hla::fom::employee_server);
  auto const efficiency = owner->getAttributeHandle(
      server, fixture_hla::fixture::efficiency);
  auto const takeOrder = owner->getInteractionClassHandle(
      fixture_hla::fom::server_take_order);
  auto const peerTakeOrder = peer->getInteractionClassHandle(
      fixture_hla::fom::server_take_order);
  REQUIRE(server.isValid());
  REQUIRE(peerServer.isValid());
  REQUIRE(efficiency.isValid());
  REQUIRE(takeOrder.isValid());
  REQUIRE(peerTakeOrder.isValid());
  REQUIRE_NOTHROW(owner->publishObjectClassAttributes(
      server, AttributeHandleSet{efficiency}));

  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = owner->registerObjectInstance(server));
  REQUIRE(objectInstance.isValid());

  // Every successful result is an in-domain point coordinate. The same
  // execution-scoped designator yields the same value through both joined
  // ambassadors, while the service-group value stays in HLAserviceGroup's
  // seven-value domain.
  auto const serviceGroupValue = owner->normalizeServiceGroup(
      rti1516_2025::SUPPORT_SERVICES);
  auto const federateValue = owner->normalizeFederateHandle(ownerFederate);
  auto const objectClassValue = owner->normalizeObjectClassHandle(server);
  auto const interactionClassValue = owner->normalizeInteractionClassHandle(takeOrder);
  auto const objectInstanceValue = owner->normalizeObjectInstanceHandle(objectInstance);
  REQUIRE(serviceGroupValue == static_cast<unsigned long>(
      rti1516_2025::SUPPORT_SERVICES));
  for (auto const value : {
           serviceGroupValue,
           federateValue,
           objectClassValue,
           interactionClassValue,
           objectInstanceValue}) {
    requirePointCoordinate(value);
  }
  REQUIRE(owner->normalizeServiceGroup(rti1516_2025::SUPPORT_SERVICES) ==
          serviceGroupValue);
  REQUIRE(owner->normalizeFederateHandle(ownerFederate) == federateValue);
  REQUIRE(owner->normalizeObjectClassHandle(server) == objectClassValue);
  REQUIRE(owner->normalizeInteractionClassHandle(takeOrder) == interactionClassValue);
  REQUIRE(owner->normalizeObjectInstanceHandle(objectInstance) == objectInstanceValue);
  REQUIRE(peer->normalizeFederateHandle(ownerFederate) == federateValue);
  REQUIRE(peer->normalizeObjectClassHandle(peerServer) == objectClassValue);
  REQUIRE(peer->normalizeInteractionClassHandle(peerTakeOrder) == interactionClassValue);
  REQUIRE(peer->normalizeObjectInstanceHandle(objectInstance) == objectInstanceValue);

  // Invalid designators are rejected by their official typed exceptions.
  REQUIRE_THROWS_AS(
      owner->normalizeServiceGroup(static_cast<ServiceGroup>(99)),
      rti1516_2025::InvalidServiceGroup);
  REQUIRE_THROWS_AS(
      owner->normalizeFederateHandle(FederateHandle{}),
      rti1516_2025::InvalidFederateHandle);
  REQUIRE_THROWS_AS(
      owner->normalizeObjectClassHandle(ObjectClassHandle{}),
      rti1516_2025::InvalidObjectClassHandle);
  REQUIRE_THROWS_AS(
      owner->normalizeInteractionClassHandle(InteractionClassHandle{}),
      rti1516_2025::InvalidInteractionClassHandle);
  REQUIRE_THROWS_AS(
      owner->normalizeObjectInstanceHandle(ObjectInstanceHandle{}),
      rti1516_2025::InvalidObjectInstanceHandle);

  // The federate designator remains normalizable for the still-joined peer
  // after the owner's active membership record has been removed.
  REQUIRE_NOTHROW(owner->deleteObjectInstance(objectInstance, VariableLengthData{}));
  REQUIRE_NOTHROW(owner->resignFederationExecution(NO_ACTION));
  REQUIRE(peer->normalizeFederateHandle(ownerFederate) == federateValue);

  REQUIRE_NOTHROW(peer->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(peer->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(unjoined->disconnect());
  REQUIRE_NOTHROW(owner->disconnect());
  REQUIRE_NOTHROW(peer->disconnect());
}
