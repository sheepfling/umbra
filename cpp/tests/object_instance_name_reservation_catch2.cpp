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
#error "The object-instance-name reservation test requires the Umbra source directory."
#endif

namespace {

namespace standard_hla = umbra::detail::hla::wide;
namespace fixture_hla = umbra::test::hla::wide;

using rti1516_2025::AttributeHandle;
using rti1516_2025::AttributeHandleSet;
using rti1516_2025::HLA_EVOKED;
using rti1516_2025::NullFederateAmbassador;
using rti1516_2025::ObjectClassHandle;
using rti1516_2025::RTIambassador;
using rti1516_2025::RTIambassadorFactory;

class ReservationAmbassador final : public NullFederateAmbassador {
 public:
  struct SingleReport final {
    std::wstring objectInstanceName;
  };

  struct MultipleReport final {
    std::set<std::wstring> objectInstanceNames;
  };

  void objectInstanceNameReservationSucceeded(
      std::wstring const& objectInstanceName) override {
    singleSucceeded.push_back({objectInstanceName});
  }

  void objectInstanceNameReservationFailed(
      std::wstring const& objectInstanceName) override {
    singleFailed.push_back({objectInstanceName});
  }

  void multipleObjectInstanceNameReservationSucceeded(
      std::set<std::wstring> const& objectInstanceNames) override {
    multipleSucceeded.push_back({objectInstanceNames});
  }

  void multipleObjectInstanceNameReservationFailed(
      std::set<std::wstring> const& objectInstanceNames) override {
    multipleFailed.push_back({objectInstanceNames});
  }

  std::vector<SingleReport> singleSucceeded;
  std::vector<SingleReport> singleFailed;
  std::vector<MultipleReport> multipleSucceeded;
  std::vector<MultipleReport> multipleFailed;
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
  return L"object-instance-name-reservation-" +
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
    "Embedded 2025 object-instance name reservation commits and reports asynchronously",
    "[integration][development-profile][federation-management][object-management][callbacks]"
    "[object-instance-name-reservation][object-name-reservation]"
    "[rti.service.reserve-object-instance-name][rti.service.release-object-instance-name]"
    "[rti.service.reserve-multiple-object-instance-names]"
    "[rti.service.release-multiple-object-instance-names]"
    "[federate.callback.object-instance-name-reservation-succeeded]"
    "[federate.callback.object-instance-name-reservation-failed]"
    "[federate.callback.multiple-object-instance-name-reservation-succeeded]"
    "[federate.callback.multiple-object-instance-name-reservation-failed]") {
  ReservationAmbassador unjoinedReports;
  ReservationAmbassador ownerReports;
  ReservationAmbassador peerReports;
  auto unjoined = makeRti();
  auto owner = makeRti();
  auto peer = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule =
      resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();

  REQUIRE_THROWS_AS(
      unjoined->reserveObjectInstanceName(L"Umbra.NotConnected"),
      rti1516_2025::NotConnected);
  REQUIRE_THROWS_AS(
      unjoined->reserveMultipleObjectInstanceNames({L"Umbra.NotConnected"}),
      rti1516_2025::NotConnected);
  REQUIRE_NOTHROW(unjoined->connect(unjoinedReports, HLA_EVOKED));
  REQUIRE_THROWS_AS(
      unjoined->reserveObjectInstanceName(L"Umbra.NotJoined"),
      rti1516_2025::FederateNotExecutionMember);

  REQUIRE_NOTHROW(owner->connect(ownerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(peer->connect(peerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(owner->createFederationExecution(
      federationName, fomModule, standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(owner->joinFederationExecution(
      L"reservation-owner", L"owner", federationName));
  REQUIRE_NOTHROW(peer->joinFederationExecution(
      L"reservation-peer", L"peer", federationName));

  REQUIRE_THROWS_AS(
      owner->reserveObjectInstanceName(L""), rti1516_2025::IllegalName);
  REQUIRE_THROWS_AS(
      owner->reserveObjectInstanceName(L"HLA.ReservedByTheRTI"),
      rti1516_2025::IllegalName);
  REQUIRE_THROWS_AS(
      owner->reserveMultipleObjectInstanceNames({}),
      rti1516_2025::NameSetWasEmpty);
  REQUIRE_THROWS_AS(
      owner->reserveMultipleObjectInstanceNames({L"Umbra.Valid", L"HLA.Invalid"}),
      rti1516_2025::IllegalName);

  auto const singleName = std::wstring{L"Umbra.SingleReservation"};
  REQUIRE_NOTHROW(owner->reserveObjectInstanceName(singleName));
  REQUIRE(ownerReports.singleSucceeded.empty());
  REQUIRE(ownerReports.singleFailed.empty());
  REQUIRE_FALSE(owner->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(ownerReports.singleSucceeded.size() == 1U);
  REQUIRE(ownerReports.singleSucceeded.front().objectInstanceName == singleName);

  // Contention is an asynchronous failure, not ObjectInstanceNameInUse at
  // the service call boundary.
  REQUIRE_NOTHROW(peer->reserveObjectInstanceName(singleName));
  REQUIRE(peerReports.singleFailed.empty());
  REQUIRE_FALSE(peer->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(peerReports.singleFailed.size() == 1U);
  REQUIRE(peerReports.singleFailed.front().objectInstanceName == singleName);

  REQUIRE_NOTHROW(owner->releaseObjectInstanceName(singleName));
  REQUIRE_THROWS_AS(
      owner->releaseObjectInstanceName(singleName),
      rti1516_2025::ObjectInstanceNameNotReserved);
  REQUIRE_NOTHROW(peer->reserveObjectInstanceName(singleName));
  REQUIRE_FALSE(peer->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(peerReports.singleSucceeded.size() == 1U);
  REQUIRE(peerReports.singleSucceeded.front().objectInstanceName == singleName);
  REQUIRE_NOTHROW(peer->releaseObjectInstanceName(singleName));

  auto const multipleA = std::wstring{L"Umbra.MultipleA"};
  auto const multipleB = std::wstring{L"Umbra.MultipleB"};
  auto const multipleC = std::wstring{L"Umbra.MultipleC"};
  REQUIRE_NOTHROW(owner->reserveMultipleObjectInstanceNames({multipleA, multipleB}));
  REQUIRE(ownerReports.multipleSucceeded.empty());
  REQUIRE_FALSE(owner->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(ownerReports.multipleSucceeded.size() == 1U);
  REQUIRE(ownerReports.multipleSucceeded.front().objectInstanceNames ==
          std::set<std::wstring>{multipleA, multipleB});

  REQUIRE_NOTHROW(peer->reserveMultipleObjectInstanceNames({multipleB, multipleC}));
  REQUIRE(peerReports.multipleSucceeded.empty());
  REQUIRE(peerReports.multipleFailed.empty());
  REQUIRE(peer->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE_FALSE(peer->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(peerReports.multipleSucceeded.size() == 1U);
  REQUIRE(peerReports.multipleFailed.size() == 1U);
  REQUIRE(peerReports.multipleSucceeded.front().objectInstanceNames ==
          std::set<std::wstring>{multipleC});
  REQUIRE(peerReports.multipleFailed.front().objectInstanceNames ==
          std::set<std::wstring>{multipleB});

  // Multiple release validates the entire set before mutating any name.
  REQUIRE_THROWS_AS(
      owner->releaseMultipleObjectInstanceNames({multipleA, multipleC}),
      rti1516_2025::ObjectInstanceNameNotReserved);
  REQUIRE_NOTHROW(peer->releaseObjectInstanceName(multipleC));
  REQUIRE_NOTHROW(owner->releaseMultipleObjectInstanceNames({multipleA, multipleB}));
  REQUIRE_NOTHROW(owner->releaseMultipleObjectInstanceNames({}));

  // A reserved RTI-generated spelling cannot shadow an unnamed registration.
  auto const generatedName = std::wstring{L"UmbraObjectInstance-1"};
  REQUIRE_NOTHROW(owner->reserveObjectInstanceName(generatedName));
  REQUIRE_FALSE(owner->evokeMultipleCallbacks(0.0, 0.0));
  auto const soda = owner->getObjectClassHandle(fixture_hla::fom::food_drink_soda);
  auto const flavor = owner->getAttributeHandle(soda, fixture_hla::fixture::flavor);
  REQUIRE_NOTHROW(owner->publishObjectClassAttributes(soda, AttributeHandleSet{flavor}));
  auto const generatedObject = owner->registerObjectInstance(soda);
  auto const actualGeneratedName = owner->getObjectInstanceName(generatedObject);
  REQUIRE(actualGeneratedName != generatedName);
  REQUIRE(actualGeneratedName.rfind(L"UmbraObjectInstance-", 0U) == 0U);
  REQUIRE_NOTHROW(owner->releaseObjectInstanceName(generatedName));

  // Resignation returns reservations to the federation-wide pool.
  auto const resignedName = std::wstring{L"Umbra.ReleasedOnResign"};
  REQUIRE_NOTHROW(owner->reserveObjectInstanceName(resignedName));
  REQUIRE_FALSE(owner->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE_NOTHROW(owner->resignFederationExecution(
      rti1516_2025::CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(peer->reserveObjectInstanceName(resignedName));
  REQUIRE_FALSE(peer->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(peerReports.singleSucceeded.size() == 2U);
  REQUIRE(peerReports.singleSucceeded.back().objectInstanceName == resignedName);
  REQUIRE_NOTHROW(peer->releaseObjectInstanceName(resignedName));

  REQUIRE_NOTHROW(peer->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(peer->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(unjoined->disconnect());
  REQUIRE_NOTHROW(owner->disconnect());
  REQUIRE_NOTHROW(peer->disconnect());
}
