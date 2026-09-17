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
#error "The object-class lookup tests require the Umbra source directory."
#endif

namespace {

namespace standard_hla = umbra::detail::hla::wide;
namespace fixture_hla = umbra::test::hla::wide;

using rti1516_2025::HLA_EVOKED;
using rti1516_2025::NO_ACTION;
using rti1516_2025::NullFederateAmbassador;
using rti1516_2025::ObjectClassHandle;
using rti1516_2025::RTIambassador;
using rti1516_2025::RTIambassadorFactory;

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
  return L"umbra-object-class-lookup-" +
      std::to_wstring(++counter);
}

}  // namespace

TEST_CASE(
    "Embedded object-class lookup services use stable handles from the joined federation FOM",
    "[integration][development-profile][federation-management][object-class-lookup]"
    "[rti.service.get-object-class-handle][rti.service.get-object-class-name]"
    "[2025]") {
  TestFederateAmbassador unjoinedFederate;
  TestFederateAmbassador ownerFederate;
  TestFederateAmbassador extensionFederate;
  auto unjoined = makeRti();
  auto owner = makeRti();
  auto extensionJoiner = makeRti();
  auto const federationName = nextFederationName();
  auto const baseFom = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  auto const extensionFom = std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
      "cpp" / "tests" / "data" / "reference-data-class-provider-fom.xml";
  ObjectClassHandle invalid;

  REQUIRE_THROWS_AS(
      unjoined->getObjectClassHandle(fixture_hla::fom::employee),
      rti1516_2025::NotConnected);
  REQUIRE_THROWS_AS(unjoined->getObjectClassName(invalid), rti1516_2025::NotConnected);
  REQUIRE_NOTHROW(unjoined->connect(unjoinedFederate, HLA_EVOKED));
  REQUIRE_THROWS_AS(
      unjoined->getObjectClassHandle(fixture_hla::fom::employee),
      rti1516_2025::FederateNotExecutionMember);
  REQUIRE_THROWS_AS(
      unjoined->getObjectClassName(invalid),
      rti1516_2025::FederateNotExecutionMember);

  REQUIRE_NOTHROW(owner->connect(ownerFederate, HLA_EVOKED));
  REQUIRE_NOTHROW(extensionJoiner->connect(extensionFederate, HLA_EVOKED));
  REQUIRE_NOTHROW(
      owner->createFederationExecution(
          federationName, baseFom, standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(owner->joinFederationExecution(
      L"lookup-owner", L"owner", federationName));

  auto const employee = owner->getObjectClassHandle(fixture_hla::fom::employee);
  REQUIRE(employee.isValid());
  REQUIRE(owner->getObjectClassHandle(fixture_hla::fom::employee) == employee);
  REQUIRE(owner->getObjectClassName(employee) == fixture_hla::fom::employee);
  REQUIRE_THROWS_AS(
      owner->getObjectClassHandle(fixture_hla::fom::missing_object),
      rti1516_2025::NameNotFound);
  REQUIRE_THROWS_AS(
      owner->getObjectClassName(invalid),
      rti1516_2025::InvalidObjectClassHandle);

  // A compatible additional-FOM join extends the catalog. The previously
  // returned class handle must remain stable while the new class becomes
  // discoverable from the same joined federation.
  REQUIRE_NOTHROW(extensionJoiner->joinFederationExecution(
      L"lookup-extension",
      L"observer",
      federationName,
      std::vector<std::wstring>{extensionFom.wstring()}));
  REQUIRE(owner->getObjectClassHandle(fixture_hla::fom::employee) == employee);
  auto const extension = owner->getObjectClassHandle(fixture_hla::fom::reference_fixture_class);
  REQUIRE(extension.isValid());
  REQUIRE(extension != employee);
  REQUIRE(owner->getObjectClassName(extension) == fixture_hla::fom::reference_fixture_class);

  REQUIRE_NOTHROW(extensionJoiner->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(unjoined->disconnect());
  REQUIRE_NOTHROW(owner->disconnect());
  REQUIRE_NOTHROW(extensionJoiner->disconnect());
}
