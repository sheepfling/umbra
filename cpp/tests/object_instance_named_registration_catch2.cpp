#include <catch2/catch_test_macros.hpp>

#include "hla_test_names.hpp"
#include "internal/fom/hla_names.hpp"

#include <RTI/NullFederateAmbassador.h>
#include <RTI/RTI1516.h>

#include <atomic>
#include <filesystem>
#include <memory>
#include <set>
#include <string>
#include <vector>

#ifndef UMBRA_SOURCE_DIRECTORY
#error "The named object-instance registration test requires the Umbra source directory."
#endif

namespace {

namespace standard_hla = umbra::detail::hla::wide;
namespace fixture_hla = umbra::test::hla::wide;

using rti1516_2025::AttributeHandle;
using rti1516_2025::AttributeHandleSet;
using rti1516_2025::AttributeHandleSetRegionHandleSetPairVector;
using rti1516_2025::DimensionHandleSet;
using rti1516_2025::HLA_EVOKED;
using rti1516_2025::NullFederateAmbassador;
using rti1516_2025::ObjectClassHandle;
using rti1516_2025::ObjectInstanceHandle;
using rti1516_2025::RangeBounds;
using rti1516_2025::RegionHandleSet;
using rti1516_2025::RTIambassador;
using rti1516_2025::RTIambassadorFactory;

class NamedRegistrationAmbassador final : public NullFederateAmbassador {
 public:
  struct Discovery final {
    ObjectInstanceHandle objectInstance;
    ObjectClassHandle objectClass;
    std::wstring objectInstanceName;
  };

  struct Reservation final {
    std::wstring objectInstanceName;
  };

  void discoverObjectInstance(
      ObjectInstanceHandle const& objectInstance,
      ObjectClassHandle const& objectClass,
      std::wstring const& objectInstanceName,
      rti1516_2025::FederateHandle const&) override {
    discoveries.push_back({objectInstance, objectClass, objectInstanceName});
  }

  void objectInstanceNameReservationSucceeded(
      std::wstring const& objectInstanceName) override {
    reservationsSucceeded.push_back({objectInstanceName});
  }

  std::vector<Discovery> discoveries;
  std::vector<Reservation> reservationsSucceeded;
};

std::unique_ptr<RTIambassador> makeRti() {
  RTIambassadorFactory factory;
  return factory.createRTIambassador();
}

std::filesystem::path resourcePath(std::filesystem::path const& relativePath) {
  return std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "third_party" /
      "ieee1516.2-2025" / "resources" / relativePath;
}

std::wstring nextFederationName() {
  static std::atomic_uint64_t sequence{0U};
  return L"object-instance-named-registration-" +
      std::to_wstring(sequence.fetch_add(1U, std::memory_order_relaxed));
}

void drainCallbacks(RTIambassador& rti) {
  for (int pass = 0; pass != 64; ++pass) {
    if (!rti.evokeMultipleCallbacks(0.0, 0.0)) {
      break;
    }
  }
}

}  // namespace

TEST_CASE(
    "Embedded 2025 named object-instance registration consumes reservations",
    "[integration][development-profile][federation-management][object-management][callbacks]"
    "[object-instance-named-registration][object-name-reservation]"
    "[rti.service.register-object-instance-named]"
    "[rti.service.register-object-instance-with-regions-named]"
    "[rti.service.reserve-object-instance-name]"
    "[rti.service.release-object-instance-name]"
    "[federate.callback.discover-object-instance]") {
  NamedRegistrationAmbassador unjoinedReports;
  NamedRegistrationAmbassador ownerReports;
  NamedRegistrationAmbassador peerReports;
  auto unjoined = makeRti();
  auto owner = makeRti();
  auto peer = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule =
      resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  ObjectClassHandle invalidObjectClass;

  REQUIRE_THROWS_AS(
      unjoined->registerObjectInstance(invalidObjectClass, L"Named-Not-Connected"),
      rti1516_2025::NotConnected);
  REQUIRE_NOTHROW(unjoined->connect(unjoinedReports, HLA_EVOKED));
  REQUIRE_THROWS_AS(
      unjoined->registerObjectInstance(invalidObjectClass, L"Named-Not-Joined"),
      rti1516_2025::FederateNotExecutionMember);

  REQUIRE_NOTHROW(owner->connect(ownerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(peer->connect(peerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(owner->createFederationExecution(
      federationName, fomModule, standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(owner->joinFederationExecution(
      L"named-owner", L"owner", federationName));
  REQUIRE_NOTHROW(peer->joinFederationExecution(
      L"named-peer", L"peer", federationName));
  REQUIRE_NOTHROW(owner->setObjectClassRelevanceAdvisorySwitch(false));
  REQUIRE_NOTHROW(owner->setInteractionRelevanceAdvisorySwitch(false));

  auto const employee = owner->getObjectClassHandle(fixture_hla::fom::employee);
  auto const server = owner->getObjectClassHandle(fixture_hla::fom::employee_server);
  auto const name = owner->getAttributeHandle(employee, fixture_hla::fixture::name);
  auto const efficiency = owner->getAttributeHandle(server, fixture_hla::fixture::efficiency);
  AttributeHandleSet const nameOnly{name};
  AttributeHandleSet const efficiencyOnly{efficiency};
  AttributeHandleSet const publishedAttributes{name, efficiency};

  REQUIRE_NOTHROW(owner->publishObjectClassAttributes(server, publishedAttributes));
  REQUIRE_NOTHROW(peer->publishObjectClassAttributes(server, efficiencyOnly));
  REQUIRE_NOTHROW(peer->subscribeObjectClassAttributes(server, nameOnly));

  std::wstring const firstName = L"UmbraNamedEmployee-1";
  REQUIRE_NOTHROW(owner->reserveObjectInstanceName(firstName));
  REQUIRE_FALSE(owner->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(ownerReports.reservationsSucceeded.size() == 1U);
  REQUIRE(ownerReports.reservationsSucceeded.front().objectInstanceName == firstName);

  // Reservation ownership is enforced at registration; a different
  // publisher cannot consume the owner's reservation.
  REQUIRE_THROWS_AS(
      peer->registerObjectInstance(server, firstName),
      rti1516_2025::ObjectInstanceNameNotReserved);

  ObjectInstanceHandle firstInstance;
  REQUIRE_NOTHROW(firstInstance = owner->registerObjectInstance(server, firstName));
  REQUIRE(firstInstance.isValid());
  REQUIRE(owner->getObjectInstanceName(firstInstance) == firstName);
  REQUIRE(owner->getObjectInstanceHandle(firstName) == firstInstance);
  REQUIRE_THROWS_AS(
      owner->releaseObjectInstanceName(firstName),
      rti1516_2025::ObjectInstanceNameNotReserved);
  REQUIRE_THROWS_AS(
      peer->registerObjectInstance(server, firstName),
      rti1516_2025::ObjectInstanceNameInUse);

  REQUIRE(peerReports.discoveries.empty());
  REQUIRE_FALSE(peer->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(peerReports.discoveries.size() == 1U);
  REQUIRE(peerReports.discoveries.front().objectInstance == firstInstance);
  REQUIRE(peerReports.discoveries.front().objectClass == server);
  REQUIRE(peerReports.discoveries.front().objectInstanceName == firstName);
  drainCallbacks(*owner);

  // A failed registration does not consume a valid reservation.
  std::wstring const secondName = L"UmbraNamedEmployee-2";
  REQUIRE_NOTHROW(owner->reserveObjectInstanceName(secondName));
  REQUIRE_FALSE(owner->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE_THROWS_AS(
      owner->registerObjectInstance(employee, secondName),
      rti1516_2025::ObjectClassNotPublished);
  REQUIRE_NOTHROW(owner->publishObjectClassAttributes(employee, nameOnly));
  REQUIRE_NOTHROW(peer->subscribeObjectClassAttributes(employee, nameOnly));

  ObjectInstanceHandle secondInstance;
  REQUIRE_NOTHROW(secondInstance = owner->registerObjectInstance(employee, secondName));
  REQUIRE(secondInstance.isValid());
  REQUIRE(owner->getObjectInstanceName(secondInstance) == secondName);
  REQUIRE_FALSE(peer->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(peerReports.discoveries.size() == 2U);
  REQUIRE(peerReports.discoveries.back().objectInstance == secondInstance);
  REQUIRE(peerReports.discoveries.back().objectClass == employee);
  REQUIRE(peerReports.discoveries.back().objectInstanceName == secondName);
  drainCallbacks(*owner);

  // Regional named registration consumes the same reservation state after
  // the accepted regional association has been committed.
  auto const soda = owner->getObjectClassHandle(fixture_hla::fom::food_drink_soda);
  auto const flavor = owner->getAttributeHandle(soda, fixture_hla::fixture::flavor);
  auto const sodaFlavor = owner->getDimensionHandle(fixture_hla::fixture::soda_flavor);
  AttributeHandleSet const flavorOnly{flavor};
  REQUIRE_NOTHROW(owner->publishObjectClassAttributes(soda, flavorOnly));
  auto const ownerRegion = owner->createRegion(DimensionHandleSet{sodaFlavor});
  REQUIRE_NOTHROW(owner->setRangeBounds(ownerRegion, sodaFlavor, RangeBounds(0UL, 1UL)));
  REQUIRE_NOTHROW(owner->commitRegionModifications(RegionHandleSet{ownerRegion}));
  AttributeHandleSetRegionHandleSetPairVector const regionalPair{{
      flavorOnly,
      RegionHandleSet{ownerRegion},
  }};
  REQUIRE_THROWS_AS(
      owner->registerObjectInstanceWithRegions(
          soda, regionalPair, L"UmbraRegionalUnreserved"),
      rti1516_2025::ObjectInstanceNameNotReserved);

  std::wstring const regionalName = L"UmbraRegional-1";
  REQUIRE_NOTHROW(owner->reserveObjectInstanceName(regionalName));
  REQUIRE_FALSE(owner->evokeMultipleCallbacks(0.0, 0.0));
  ObjectInstanceHandle regionalInstance;
  REQUIRE_NOTHROW(regionalInstance = owner->registerObjectInstanceWithRegions(
      soda, regionalPair, regionalName));
  REQUIRE(regionalInstance.isValid());
  REQUIRE(owner->getObjectInstanceName(regionalInstance) == regionalName);
  REQUIRE(owner->getObjectInstanceHandle(regionalName) == regionalInstance);
  REQUIRE_THROWS_AS(
      owner->releaseObjectInstanceName(regionalName),
      rti1516_2025::ObjectInstanceNameNotReserved);
  drainCallbacks(*owner);

  // Release before registration returns the name to the federation-wide pool.
  std::wstring const reusableName = L"UmbraNamedReusable";
  REQUIRE_NOTHROW(owner->reserveObjectInstanceName(reusableName));
  REQUIRE_FALSE(owner->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE_NOTHROW(owner->releaseObjectInstanceName(reusableName));
  REQUIRE_NOTHROW(peer->reserveObjectInstanceName(reusableName));
  REQUIRE_FALSE(peer->evokeMultipleCallbacks(0.0, 0.0));
  ObjectInstanceHandle peerInstance;
  REQUIRE_NOTHROW(peerInstance = peer->registerObjectInstance(server, reusableName));
  REQUIRE(peerInstance.isValid());
  REQUIRE(peer->getObjectInstanceName(peerInstance) == reusableName);
  REQUIRE_THROWS_AS(
      peer->releaseObjectInstanceName(reusableName),
      rti1516_2025::ObjectInstanceNameNotReserved);
  drainCallbacks(*peer);

  REQUIRE_NOTHROW(peer->resignFederationExecution(
      rti1516_2025::CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(owner->resignFederationExecution(
      rti1516_2025::CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(unjoined->disconnect());
  REQUIRE_NOTHROW(owner->disconnect());
  REQUIRE_NOTHROW(peer->disconnect());
}
