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
#error "The parameter lookup tests require the Umbra source directory."
#endif

namespace {

namespace standard_hla = umbra::detail::hla::wide;
namespace fixture_hla = umbra::test::hla::wide;

using rti1516_2025::HLA_EVOKED;
using rti1516_2025::InteractionClassHandle;
using rti1516_2025::NO_ACTION;
using rti1516_2025::NullFederateAmbassador;
using rti1516_2025::ParameterHandle;
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
  return L"umbra-parameter-lookup-" + std::to_wstring(++counter);
}

}  // namespace

TEST_CASE(
    "Embedded parameter lookup resolves inherited definitions in the joined federation FOM",
    "[integration][development-profile][federation-management][parameter-lookup]"
    "[rti.service.get-parameter-handle][rti.service.get-parameter-name]"
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
      "cpp" / "tests" / "data" / "parameter-handle-provider-fom.xml";
  InteractionClassHandle invalidInteractionClass;
  ParameterHandle invalidParameter;

  REQUIRE_THROWS_AS(
      unjoined->getParameterHandle(invalidInteractionClass, fixture_hla::fixture::temperature_ok),
      rti1516_2025::NotConnected);
  REQUIRE_THROWS_AS(
      unjoined->getParameterName(invalidInteractionClass, invalidParameter),
      rti1516_2025::NotConnected);
  REQUIRE_NOTHROW(unjoined->connect(unjoinedFederate, HLA_EVOKED));
  REQUIRE_THROWS_AS(
      unjoined->getParameterHandle(invalidInteractionClass, fixture_hla::fixture::temperature_ok),
      rti1516_2025::FederateNotExecutionMember);
  REQUIRE_THROWS_AS(
      unjoined->getParameterName(invalidInteractionClass, invalidParameter),
      rti1516_2025::FederateNotExecutionMember);

  REQUIRE_NOTHROW(owner->connect(ownerFederate, HLA_EVOKED));
  REQUIRE_NOTHROW(extensionJoiner->connect(extensionFederate, HLA_EVOKED));
  REQUIRE_NOTHROW(
      owner->createFederationExecution(
          federationName, baseFom, standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(owner->joinFederationExecution(
      L"parameter-owner", L"owner", federationName));

  auto const mainCourseServed = owner->getInteractionClassHandle(
      fixture_hla::fom::main_course_served);
  auto const takeOrder = owner->getInteractionClassHandle(
      fixture_hla::fom::server_take_order);
  auto const temperatureOk = owner->getParameterHandle(
      mainCourseServed, fixture_hla::fixture::temperature_ok);
  REQUIRE(temperatureOk.isValid());
  REQUIRE(owner->getParameterHandle(
      mainCourseServed, fixture_hla::fixture::temperature_ok) == temperatureOk);
  REQUIRE(owner->getParameterName(
      mainCourseServed, temperatureOk) == fixture_hla::fixture::temperature_ok);
  REQUIRE_THROWS_AS(
      owner->getParameterHandle(mainCourseServed, fixture_hla::fixture::missing),
      rti1516_2025::NameNotFound);
  REQUIRE_THROWS_AS(
      owner->getParameterHandle(
          invalidInteractionClass, fixture_hla::fixture::temperature_ok),
      rti1516_2025::InvalidInteractionClassHandle);
  REQUIRE_THROWS_AS(
      owner->getParameterName(invalidInteractionClass, temperatureOk),
      rti1516_2025::InvalidInteractionClassHandle);
  REQUIRE_THROWS_AS(
      owner->getParameterName(mainCourseServed, invalidParameter),
      rti1516_2025::InvalidParameterHandle);
  REQUIRE_THROWS_AS(
      owner->getParameterName(takeOrder, temperatureOk),
      rti1516_2025::InteractionParameterNotDefined);

  // A compatible additional-FOM join adds a new interaction inheritance
  // chain. The original Restaurant parameter handle remains stable, while the
  // extension's defining parameter has the same handle through its child.
  REQUIRE_NOTHROW(extensionJoiner->joinFederationExecution(
      L"parameter-extension",
      L"observer",
      federationName,
      std::vector<std::wstring>{extensionFom.wstring()}));
  REQUIRE(owner->getParameterHandle(
      mainCourseServed, fixture_hla::fixture::temperature_ok) == temperatureOk);
  auto const extensionBase = owner->getInteractionClassHandle(
      fixture_hla::fom::parameter_fixture_base);
  auto const extensionChild = owner->getInteractionClassHandle(
      fixture_hla::fom::parameter_fixture_child_interaction);
  auto const identifier = owner->getParameterHandle(
      extensionBase, fixture_hla::fixture::identifier);
  REQUIRE(identifier.isValid());
  REQUIRE(owner->getParameterHandle(
      extensionChild, fixture_hla::fixture::identifier) == identifier);
  REQUIRE(owner->getParameterName(
      extensionChild, identifier) == fixture_hla::fixture::identifier);

  REQUIRE_NOTHROW(extensionJoiner->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(unjoined->disconnect());
  REQUIRE_NOTHROW(owner->disconnect());
  REQUIRE_NOTHROW(extensionJoiner->disconnect());
}
