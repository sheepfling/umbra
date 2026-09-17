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
#error "The dimension lookup tests require the Umbra source directory."
#endif

namespace {

namespace standard_hla = umbra::detail::hla::wide;
namespace fixture_hla = umbra::test::hla::wide;

using rti1516_2025::DimensionHandle;
using rti1516_2025::DimensionHandleSet;
using rti1516_2025::HLA_EVOKED;
using rti1516_2025::InteractionClassHandle;
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
  return L"umbra-dimension-lookup-" + std::to_wstring(++counter);
}

}  // namespace

TEST_CASE(
    "Embedded 2025 dimension lookup follows FOM hierarchy and upper bounds",
    "[integration][development-profile][federation-management][ddm-foundation]"
    "[dimension-lookup][rti.service.get-available-dimensions-for-object-class]"
    "[rti.service.get-available-dimensions-for-interaction-class]"
    "[rti.service.get-dimension-handle][rti.service.get-dimension-name]"
    "[rti.service.get-dimension-upper-bound][2025]") {
  TestFederateAmbassador unjoinedFederate;
  TestFederateAmbassador ownerFederate;
  TestFederateAmbassador extensionFederate;
  auto unjoined = makeRti();
  auto owner = makeRti();
  auto extensionJoiner = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  auto const dimensionConsumerFom = std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
      "cpp" / "tests" / "data" / "dimension-reference-consumer-fom.xml";
  auto const dimensionProviderFom = std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
      "cpp" / "tests" / "data" / "dimension-reference-provider-fom.xml";
  ObjectClassHandle invalidObjectClass;
  InteractionClassHandle invalidInteractionClass;
  DimensionHandle invalidDimension;

  REQUIRE_THROWS_AS(
      unjoined->getDimensionHandle(fixture_hla::fixture::soda_flavor),
      rti1516_2025::NotConnected);
  REQUIRE_THROWS_AS(
      unjoined->getDimensionName(invalidDimension),
      rti1516_2025::NotConnected);
  REQUIRE_NOTHROW(unjoined->connect(unjoinedFederate, HLA_EVOKED));
  REQUIRE_THROWS_AS(
      unjoined->getAvailableDimensionsForObjectClass(invalidObjectClass),
      rti1516_2025::FederateNotExecutionMember);
  REQUIRE_THROWS_AS(
      unjoined->getAvailableDimensionsForInteractionClass(invalidInteractionClass),
      rti1516_2025::FederateNotExecutionMember);

  REQUIRE_NOTHROW(owner->connect(ownerFederate, HLA_EVOKED));
  REQUIRE_NOTHROW(extensionJoiner->connect(extensionFederate, HLA_EVOKED));
  REQUIRE_NOTHROW(
      owner->createFederationExecution(federationName, fomModule, standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(owner->joinFederationExecution(L"dimension-owner", L"owner", federationName));

  auto const barQuantity = owner->getDimensionHandle(fixture_hla::fixture::bar_quantity);
  auto const sodaFlavor = owner->getDimensionHandle(fixture_hla::fixture::soda_flavor);
  auto const sweetener = owner->getDimensionHandle(fixture_hla::fixture::sweetener);
  auto const serverId = owner->getDimensionHandle(fixture_hla::fixture::server_id);
  REQUIRE(barQuantity.isValid());
  REQUIRE(sodaFlavor.isValid());
  REQUIRE(sweetener.isValid());
  REQUIRE(serverId.isValid());
  REQUIRE(owner->getDimensionHandle(fixture_hla::fixture::soda_flavor) == sodaFlavor);
  REQUIRE(owner->getDimensionName(sodaFlavor) == fixture_hla::fixture::soda_flavor);
  REQUIRE(owner->getDimensionUpperBound(barQuantity) == 25UL);
  REQUIRE(owner->getDimensionUpperBound(sodaFlavor) == 4UL);
  REQUIRE(owner->getDimensionUpperBound(sweetener) == 3UL);
  REQUIRE(owner->getDimensionUpperBound(serverId) == 20UL);

  auto const drink = owner->getObjectClassHandle(fixture_hla::fom::food_drink);
  auto const soda = owner->getObjectClassHandle(fixture_hla::fom::food_drink_soda);
  auto const light = owner->getObjectClassHandle(fixture_hla::fom::food_drink_soda_light);
  DimensionHandleSet drinkDimensions = owner->getAvailableDimensionsForObjectClass(drink);
  DimensionHandleSet sodaDimensions = owner->getAvailableDimensionsForObjectClass(soda);
  DimensionHandleSet lightDimensions = owner->getAvailableDimensionsForObjectClass(light);
  REQUIRE(drinkDimensions == DimensionHandleSet{barQuantity});
  REQUIRE(sodaDimensions == DimensionHandleSet{barQuantity, sodaFlavor});
  REQUIRE(lightDimensions == DimensionHandleSet{barQuantity, sodaFlavor, sweetener});

  auto const foodServed = owner->getInteractionClassHandle(fixture_hla::fom::food_served);
  auto const mainCourseServed = owner->getInteractionClassHandle(
      fixture_hla::fom::main_course_served);
  REQUIRE(owner->getAvailableDimensionsForInteractionClass(foodServed).empty());
  REQUIRE(
      owner->getAvailableDimensionsForInteractionClass(mainCourseServed) ==
      DimensionHandleSet{serverId});

  // A later 2025 FOM join contributes a class, interaction, and dimension
  // from separate modules. Existing Restaurant handles stay stable while the
  // newly composed dimension is immediately available to lookup services.
  REQUIRE_NOTHROW(extensionJoiner->joinFederationExecution(
      L"dimension-extension",
      L"observer",
      federationName,
      std::vector<std::wstring>{dimensionConsumerFom.wstring(), dimensionProviderFom.wstring()}));
  REQUIRE(owner->getDimensionHandle(fixture_hla::fixture::soda_flavor) == sodaFlavor);
  auto const fixtureDimension = owner->getDimensionHandle(fixture_hla::fixture::umbra_dimension_fixture);
  REQUIRE(fixtureDimension.isValid());
  REQUIRE(owner->getDimensionUpperBound(fixtureDimension) == 100UL);
  auto const fixtureObjectClass = owner->getObjectClassHandle(
      fixture_hla::fom::dimension_fixture_object);
  REQUIRE(
      owner->getAvailableDimensionsForObjectClass(fixtureObjectClass) ==
      DimensionHandleSet{fixtureDimension});
  auto const fixtureInteractionClass = owner->getInteractionClassHandle(
      fixture_hla::fom::dimension_fixture_interaction);
  REQUIRE(
      owner->getAvailableDimensionsForInteractionClass(fixtureInteractionClass) ==
      DimensionHandleSet{fixtureDimension});

  REQUIRE_THROWS_AS(
      owner->getDimensionHandle(fixture_hla::fixture::missing_dimension),
      rti1516_2025::NameNotFound);
  REQUIRE_THROWS_AS(
      owner->getDimensionName(invalidDimension),
      rti1516_2025::InvalidDimensionHandle);
  REQUIRE_THROWS_AS(
      owner->getDimensionUpperBound(invalidDimension),
      rti1516_2025::InvalidDimensionHandle);
  REQUIRE_THROWS_AS(
      owner->getAvailableDimensionsForObjectClass(invalidObjectClass),
      rti1516_2025::InvalidObjectClassHandle);
  REQUIRE_THROWS_AS(
      owner->getAvailableDimensionsForInteractionClass(invalidInteractionClass),
      rti1516_2025::InvalidInteractionClassHandle);

  REQUIRE_NOTHROW(owner->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(extensionJoiner->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(unjoined->disconnect());
  REQUIRE_NOTHROW(owner->disconnect());
  REQUIRE_NOTHROW(extensionJoiner->disconnect());
}
