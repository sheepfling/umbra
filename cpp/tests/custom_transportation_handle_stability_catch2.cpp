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
#error "The transportation-handle stability tests require the Umbra source directory."
#endif

namespace {

namespace standard_hla = umbra::detail::hla::wide;
namespace fixture_hla = umbra::test::hla::wide;

using rti1516_2025::HLA_EVOKED;
using rti1516_2025::NO_ACTION;
using rti1516_2025::NullFederateAmbassador;
using rti1516_2025::RTIambassador;
using rti1516_2025::RTIambassadorFactory;

using TestFederateAmbassador = NullFederateAmbassador;

std::unique_ptr<RTIambassador> makeRti() {
  RTIambassadorFactory factory;
  return factory.createRTIambassador();
}

std::wstring nextFederationName() {
  static std::atomic_uint64_t counter{0};
  return L"umbra-transportation-handle-stability-" + std::to_wstring(++counter);
}

}  // namespace

TEST_CASE(
    "Embedded custom transportation handles remain stable across an additional FOM join",
    "[integration][development-profile][federation-management][transportation]"
    "[fom][transportation-type-lookup][additional-fom][transportation-handle-stability]"
    "[fom-composition][2025]"
    "[rti.service.connect][rti.service.create-federation-execution]"
    "[rti.service.join-federation-execution]"
    "[rti.service.get-transportation-type-handle]"
    "[rti.service.get-transportation-type-name]"
    "[rti.service.resign-federation-execution][rti.service.destroy-federation-execution]"
    "[rti.service.disconnect]") {
  TestFederateAmbassador memberFederate;
  TestFederateAmbassador extensionFederate;
  auto member = makeRti();
  auto extension = makeRti();
  auto const federationName = nextFederationName();
  auto const testData = std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
      "cpp" / "tests" / "data";
  std::vector<std::wstring> const initialFomModules{
      (testData / "transportation-reference-consumer-fom.xml").wstring(),
      (testData / "transportation-reference-provider-fom.xml").wstring(),
  };
  auto const extensionFom =
      (testData / "transportation-extension-earlier-fom.xml").wstring();
  constexpr wchar_t earlierTransportation[] = L"AaaTransportationFixture";

  REQUIRE_NOTHROW(member->connect(memberFederate, HLA_EVOKED));
  REQUIRE_NOTHROW(extension->connect(extensionFederate, HLA_EVOKED));
  REQUIRE_NOTHROW(member->createFederationExecution(
      federationName,
      initialFomModules,
      standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(member->joinFederationExecution(
      L"transport-stable-member",
      L"member",
      federationName));

  auto const existing = member->getTransportationTypeHandle(
      fixture_hla::fixture::umbra_transportation_fixture);
  REQUIRE(existing.isValid());
  REQUIRE(member->getTransportationTypeName(existing) ==
      fixture_hla::fixture::umbra_transportation_fixture);

  REQUIRE_NOTHROW(extension->joinFederationExecution(
      L"transport-stable-extension",
      L"extension",
      federationName,
      std::vector<std::wstring>{extensionFom}));

  // The extension name sorts before the existing declaration. A catalog-order
  // allocator would renumber the existing handle; the execution directory
  // must preserve it for every already-joined and newly-joined federate.
  REQUIRE(member->getTransportationTypeHandle(
      fixture_hla::fixture::umbra_transportation_fixture) == existing);
  REQUIRE(member->getTransportationTypeName(existing) ==
      fixture_hla::fixture::umbra_transportation_fixture);
  REQUIRE(extension->getTransportationTypeHandle(
      fixture_hla::fixture::umbra_transportation_fixture) == existing);
  REQUIRE(extension->getTransportationTypeName(existing) ==
      fixture_hla::fixture::umbra_transportation_fixture);

  auto const earlier = extension->getTransportationTypeHandle(earlierTransportation);
  REQUIRE(earlier.isValid());
  REQUIRE(earlier != existing);
  REQUIRE(extension->getTransportationTypeName(earlier) == earlierTransportation);
  REQUIRE(member->getTransportationTypeHandle(earlierTransportation) == earlier);
  REQUIRE(member->getTransportationTypeName(earlier) == earlierTransportation);

  REQUIRE_NOTHROW(extension->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(member->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(member->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(extension->disconnect());
  REQUIRE_NOTHROW(member->disconnect());
}
