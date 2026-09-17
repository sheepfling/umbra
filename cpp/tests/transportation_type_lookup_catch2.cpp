#include <catch2/catch_test_macros.hpp>

#include <RTI/NullFederateAmbassador.h>
#include <RTI/RTI1516.h>

#include "hla_test_names.hpp"
#include "internal/fom/hla_names.hpp"

#include <atomic>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#ifndef UMBRA_SOURCE_DIRECTORY
#error "The transportation-type lookup tests require the Umbra source directory."
#endif

namespace {

namespace standard_hla = umbra::detail::hla::wide;
namespace fixture_hla = umbra::test::hla::wide;

using rti1516_2025::HLA_EVOKED;
using rti1516_2025::NO_ACTION;
using rti1516_2025::NullFederateAmbassador;
using rti1516_2025::RTIambassador;
using rti1516_2025::RTIambassadorFactory;
using rti1516_2025::TransportationTypeHandle;

using TestFederateAmbassador = NullFederateAmbassador;

std::unique_ptr<RTIambassador> makeRti() {
  RTIambassadorFactory factory;
  return factory.createRTIambassador();
}

std::filesystem::path resourcePath(std::filesystem::path const& relativePath) {
  return std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
      "third_party" / "ieee1516.2-2025" / "resources" / relativePath;
}

std::wstring nextFederationName() {
  static std::atomic_uint64_t counter{0};
  return L"umbra-transportation-type-lookup-" + std::to_wstring(++counter);
}

}  // namespace

TEST_CASE(
    "Embedded transportation type lookup exposes the mandatory 2025 support pair",
    "[integration][development-profile][federation-management]"
    "[transportation][transportation-type-lookup]"
    "[rti.service.get-transportation-type-handle]"
    "[rti.service.get-transportation-type-name][2025]") {
  TestFederateAmbassador unjoinedFederate;
  TestFederateAmbassador memberFederate;
  auto unjoined = makeRti();
  auto member = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  TransportationTypeHandle invalid;

  REQUIRE_THROWS_AS(
      unjoined->getTransportationTypeHandle(standard_hla::mom::reliable),
      rti1516_2025::NotConnected);
  REQUIRE_THROWS_AS(
      unjoined->getTransportationTypeName(invalid),
      rti1516_2025::NotConnected);
  REQUIRE_NOTHROW(unjoined->connect(unjoinedFederate, HLA_EVOKED));
  REQUIRE_THROWS_AS(
      unjoined->getTransportationTypeHandle(standard_hla::mom::reliable),
      rti1516_2025::FederateNotExecutionMember);
  REQUIRE_THROWS_AS(
      unjoined->getTransportationTypeName(invalid),
      rti1516_2025::FederateNotExecutionMember);

  REQUIRE_NOTHROW(member->connect(memberFederate, HLA_EVOKED));
  REQUIRE_NOTHROW(
      member->createFederationExecution(federationName, fomModule, standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(
      member->joinFederationExecution(L"transport-member", L"member", federationName));

  auto const reliable = member->getTransportationTypeHandle(standard_hla::mom::reliable);
  auto const bestEffort = member->getTransportationTypeHandle(standard_hla::mom::best_effort);
  REQUIRE(reliable.isValid());
  REQUIRE(bestEffort.isValid());
  REQUIRE(reliable != bestEffort);
  REQUIRE(member->getTransportationTypeHandle(standard_hla::mom::reliable) == reliable);
  REQUIRE(member->getTransportationTypeName(reliable) == standard_hla::mom::reliable);
  REQUIRE(member->getTransportationTypeName(bestEffort) == standard_hla::mom::best_effort);
  REQUIRE_THROWS_AS(
      member->getTransportationTypeHandle(fixture_hla::fixture::umbra_custom_transport),
      rti1516_2025::InvalidTransportationName);
  REQUIRE_THROWS_AS(
      member->getTransportationTypeName(invalid),
      rti1516_2025::InvalidTransportationTypeHandle);

  REQUIRE_NOTHROW(member->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(member->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(unjoined->disconnect());
  REQUIRE_NOTHROW(member->disconnect());
}

TEST_CASE(
    "Embedded transportation type lookup resolves a declared FOM transportation per execution",
    "[integration][development-profile][federation-management][transportation]"
    "[fom][transportation-type-lookup]"
    "[rti.service.get-transportation-type-handle]"
    "[rti.service.get-transportation-type-name]") {
  TestFederateAmbassador memberFederate;
  TestFederateAmbassador peerFederate;
  auto member = makeRti();
  auto peer = makeRti();
  auto const federationName = nextFederationName();
  auto const testData = std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
      "cpp" / "tests" / "data";
  std::vector<std::wstring> const fomModules{
      (testData / "transportation-reference-consumer-fom.xml").wstring(),
      (testData / "transportation-reference-provider-fom.xml").wstring(),
  };

  REQUIRE_NOTHROW(member->connect(memberFederate, HLA_EVOKED));
  REQUIRE_NOTHROW(peer->connect(peerFederate, HLA_EVOKED));
  REQUIRE_NOTHROW(member->createFederationExecution(
      federationName, fomModules, standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(member->joinFederationExecution(
      L"transport-custom-member", L"member", federationName));

  auto const reliable = member->getTransportationTypeHandle(
      standard_hla::mom::reliable);
  auto const custom = member->getTransportationTypeHandle(
      fixture_hla::fixture::umbra_transportation_fixture);
  REQUIRE(reliable.isValid());
  REQUIRE(custom.isValid());
  REQUIRE(custom != reliable);
  REQUIRE(member->getTransportationTypeHandle(
      fixture_hla::fixture::umbra_transportation_fixture) == custom);
  REQUIRE(member->getTransportationTypeName(custom) ==
      fixture_hla::fixture::umbra_transportation_fixture);

  REQUIRE_NOTHROW(peer->joinFederationExecution(
      L"transport-custom-peer", L"peer", federationName));
  REQUIRE(peer->getTransportationTypeHandle(
      fixture_hla::fixture::umbra_transportation_fixture) == custom);
  REQUIRE(peer->getTransportationTypeName(custom) ==
      fixture_hla::fixture::umbra_transportation_fixture);

  REQUIRE_NOTHROW(peer->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(member->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(member->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(peer->disconnect());
  REQUIRE_NOTHROW(member->disconnect());
}
