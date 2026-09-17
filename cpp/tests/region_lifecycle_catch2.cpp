#include <catch2/catch_test_macros.hpp>

#include "internal/fom/hla_names.hpp"
#include "hla_test_names.hpp"

#include <RTI/NullFederateAmbassador.h>
#include <RTI/RTI1516.h>

#include <atomic>
#include <filesystem>
#include <memory>

#ifndef UMBRA_SOURCE_DIRECTORY
#error "The region-lifecycle test requires the Umbra source directory."
#endif

namespace {

namespace standard_hla = umbra::detail::hla::wide;
namespace fixture_hla = umbra::test::hla::wide;

using rti1516_2025::DimensionHandle;
using rti1516_2025::DimensionHandleSet;
using rti1516_2025::HLA_EVOKED;
using rti1516_2025::NO_ACTION;
using rti1516_2025::RangeBounds;
using rti1516_2025::RegionHandle;
using rti1516_2025::RegionHandleSet;
using rti1516_2025::RTIambassador;
using rti1516_2025::RTIambassadorFactory;
using rti1516_2025::VariableLengthData;

std::wstring nextFederationName() {
  static std::atomic_uint64_t sequence{0U};
  return L"region-lifecycle-" +
      std::to_wstring(sequence.fetch_add(1U, std::memory_order_relaxed));
}

std::filesystem::path resourcePath(std::filesystem::path const& relativePath) {
  return std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "third_party" /
      "ieee1516.2-2025" / "resources" / relativePath;
}

std::unique_ptr<RTIambassador> makeRti() {
  RTIambassadorFactory factory;
  return factory.createRTIambassador();
}

}  // namespace

TEST_CASE(
    "Standalone 2025 region templates preserve pending and committed range state",
    "[integration][development-profile][federation-management][ddm]"
    "[region-lifecycle][range-bounds]"
    "[rti.service.create-region][rti.service.commit-region-modifications]"
    "[rti.service.delete-region][rti.service.get-dimension-handle-set]"
    "[rti.service.get-range-bounds][rti.service.set-range-bounds]"
    "[rti.service.decode-region-handle]") {
  rti1516_2025::NullFederateAmbassador unjoinedFederate;
  rti1516_2025::NullFederateAmbassador ownerFederate;
  rti1516_2025::NullFederateAmbassador foreignFederate;
  auto unjoined = makeRti();
  auto owner = makeRti();
  auto foreign = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule =
      resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  RegionHandle invalidRegion;
  DimensionHandle invalidDimension;

  REQUIRE_THROWS_AS(
      unjoined->createRegion(DimensionHandleSet{}),
      rti1516_2025::NotConnected);
  REQUIRE_THROWS_AS(
      unjoined->decodeRegionHandle(VariableLengthData{}),
      rti1516_2025::NotConnected);
  REQUIRE_NOTHROW(unjoined->connect(unjoinedFederate, HLA_EVOKED));
  REQUIRE_THROWS_AS(
      unjoined->createRegion(DimensionHandleSet{}),
      rti1516_2025::FederateNotExecutionMember);
  REQUIRE_THROWS_AS(
      unjoined->decodeRegionHandle(VariableLengthData{}),
      rti1516_2025::FederateNotExecutionMember);
  REQUIRE_THROWS_AS(
      unjoined->getDimensionHandleSet(invalidRegion),
      rti1516_2025::FederateNotExecutionMember);

  REQUIRE_NOTHROW(owner->connect(ownerFederate, HLA_EVOKED));
  REQUIRE_NOTHROW(foreign->connect(foreignFederate, HLA_EVOKED));
  REQUIRE_NOTHROW(owner->createFederationExecution(
      federationName, fomModule, standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(owner->joinFederationExecution(
      L"region-owner", L"owner", federationName));
  REQUIRE_NOTHROW(foreign->joinFederationExecution(
      L"region-foreign", L"foreign", federationName));

  // The official 2025 C++ createRegion surface accepts an empty set of
  // dimensions. This is a valid zero-dimensional template.
  auto const emptyRegion = owner->createRegion(DimensionHandleSet{});
  REQUIRE(emptyRegion.isValid());
  REQUIRE(owner->getDimensionHandleSet(emptyRegion).empty());
  REQUIRE_NOTHROW(owner->commitRegionModifications(RegionHandleSet{emptyRegion}));
  REQUIRE(owner->getDimensionHandleSet(emptyRegion).empty());
  REQUIRE_NOTHROW(owner->deleteRegion(emptyRegion));

  auto const barQuantity =
      owner->getDimensionHandle(fixture_hla::fixture::bar_quantity);
  auto const sodaFlavor =
      owner->getDimensionHandle(fixture_hla::fixture::soda_flavor);
  REQUIRE(barQuantity.isValid());
  REQUIRE(sodaFlavor.isValid());

  auto const region =
      owner->createRegion(DimensionHandleSet{barQuantity, sodaFlavor});
  REQUIRE(region.isValid());
  REQUIRE(owner->getDimensionHandleSet(region) ==
          DimensionHandleSet{barQuantity, sodaFlavor});
  REQUIRE(owner->decodeRegionHandle(region.encode()) == region);
  REQUIRE_THROWS_AS(
      owner->decodeRegionHandle(VariableLengthData{}),
      rti1516_2025::CouldNotDecode);

  REQUIRE_THROWS_AS(
      owner->getRangeBounds(region, barQuantity),
      rti1516_2025::InvalidRegion);
  REQUIRE_THROWS_AS(
      owner->commitRegionModifications(RegionHandleSet{region}),
      rti1516_2025::InvalidRegion);
  REQUIRE_THROWS_AS(
      owner->setRangeBounds(region, invalidDimension, RangeBounds(0UL, 10UL)),
      rti1516_2025::RegionDoesNotContainSpecifiedDimension);
  REQUIRE_THROWS_AS(
      owner->setRangeBounds(region, barQuantity, RangeBounds(10UL, 10UL)),
      rti1516_2025::InvalidRangeBound);
  REQUIRE_THROWS_AS(
      owner->setRangeBounds(region, barQuantity, RangeBounds(0UL, 26UL)),
      rti1516_2025::InvalidRangeBound);

  REQUIRE_NOTHROW(
      owner->setRangeBounds(region, barQuantity, RangeBounds(0UL, 10UL)));
  auto const pendingBar = owner->getRangeBounds(region, barQuantity);
  REQUIRE(pendingBar.getLowerBound() == 0UL);
  REQUIRE(pendingBar.getUpperBound() == 10UL);
  REQUIRE_THROWS_AS(
      owner->commitRegionModifications(RegionHandleSet{region}),
      rti1516_2025::InvalidRegion);
  REQUIRE(owner->getRangeBounds(region, barQuantity).getUpperBound() == 10UL);
  REQUIRE_THROWS_AS(
      owner->getRangeBounds(region, sodaFlavor),
      rti1516_2025::InvalidRegion);

  REQUIRE_THROWS_AS(
      foreign->commitRegionModifications(RegionHandleSet{region}),
      rti1516_2025::RegionNotCreatedByThisFederate);
  REQUIRE_THROWS_AS(
      foreign->deleteRegion(region),
      rti1516_2025::RegionNotCreatedByThisFederate);
  REQUIRE_THROWS_AS(
      foreign->getDimensionHandleSet(region),
      rti1516_2025::InvalidRegion);
  REQUIRE_THROWS_AS(
      foreign->getRangeBounds(region, barQuantity),
      rti1516_2025::InvalidRegion);
  REQUIRE_THROWS_AS(
      foreign->setRangeBounds(region, barQuantity, RangeBounds(0UL, 5UL)),
      rti1516_2025::RegionNotCreatedByThisFederate);

  REQUIRE_NOTHROW(
      owner->setRangeBounds(region, sodaFlavor, RangeBounds(1UL, 3UL)));
  REQUIRE_NOTHROW(owner->commitRegionModifications(RegionHandleSet{region}));
  auto const committedBar = owner->getRangeBounds(region, barQuantity);
  auto const committedSoda = owner->getRangeBounds(region, sodaFlavor);
  REQUIRE(committedBar.getLowerBound() == 0UL);
  REQUIRE(committedBar.getUpperBound() == 10UL);
  REQUIRE(committedSoda.getLowerBound() == 1UL);
  REQUIRE(committedSoda.getUpperBound() == 3UL);

  REQUIRE_NOTHROW(
      owner->setRangeBounds(region, barQuantity, RangeBounds(5UL, 15UL)));
  auto const replacement = owner->getRangeBounds(region, barQuantity);
  REQUIRE(replacement.getLowerBound() == 5UL);
  REQUIRE(replacement.getUpperBound() == 15UL);
  REQUIRE_NOTHROW(owner->commitRegionModifications(RegionHandleSet{region}));
  auto const recommitted = owner->getRangeBounds(region, barQuantity);
  REQUIRE(recommitted.getLowerBound() == 5UL);
  REQUIRE(recommitted.getUpperBound() == 15UL);

  // A region remains in use while a regional subscription is active.
  auto const interactionClass = owner->getInteractionClassHandle(
      fixture_hla::fom::main_course_served);
  auto const serverId =
      owner->getDimensionHandle(fixture_hla::fixture::server_id);
  REQUIRE(interactionClass.isValid());
  REQUIRE(serverId.isValid());
  auto const subscriptionRegion =
      owner->createRegion(DimensionHandleSet{serverId});
  REQUIRE_NOTHROW(owner->setRangeBounds(
      subscriptionRegion, serverId, RangeBounds(0UL, 10UL)));
  REQUIRE_NOTHROW(owner->commitRegionModifications(
      RegionHandleSet{subscriptionRegion}));
  REQUIRE_NOTHROW(owner->subscribeInteractionClassWithRegions(
      interactionClass, RegionHandleSet{subscriptionRegion}, false));
  REQUIRE_THROWS_AS(
      owner->deleteRegion(subscriptionRegion),
      rti1516_2025::RegionInUseForUpdateOrSubscription);
  REQUIRE_NOTHROW(owner->unsubscribeInteractionClassWithRegions(
      interactionClass, RegionHandleSet{subscriptionRegion}));
  REQUIRE_NOTHROW(owner->deleteRegion(subscriptionRegion));

  REQUIRE_NOTHROW(owner->deleteRegion(region));
  REQUIRE_THROWS_AS(
      owner->getDimensionHandleSet(region),
      rti1516_2025::InvalidRegion);
  REQUIRE_THROWS_AS(
      owner->deleteRegion(region),
      rti1516_2025::InvalidRegion);

  REQUIRE_NOTHROW(owner->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(foreign->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(unjoined->disconnect());
  REQUIRE_NOTHROW(owner->disconnect());
  REQUIRE_NOTHROW(foreign->disconnect());
}
