#include <catch2/catch_test_macros.hpp>

#include <RTI/NullFederateAmbassador.h>
#include <RTI/RTI1516.h>

#include "internal/fom/hla_names.hpp"

#include <atomic>
#include <filesystem>
#include <memory>
#include <string>

#ifndef UMBRA_SOURCE_DIRECTORY
#error "The order-type lookup tests require the Umbra source directory."
#endif

namespace {

namespace standard_hla = umbra::detail::hla::wide;

using rti1516_2025::HLA_EVOKED;
using rti1516_2025::NO_ACTION;
using rti1516_2025::NullFederateAmbassador;
using rti1516_2025::OrderType;
using rti1516_2025::RECEIVE;
using rti1516_2025::RTIambassador;
using rti1516_2025::RTIambassadorFactory;
using rti1516_2025::TIMESTAMP;

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
  return L"umbra-order-type-lookup-" + std::to_wstring(++counter);
}

}  // namespace

TEST_CASE(
    "Embedded order type lookup exposes the mandatory 2025 Receive and TimeStamp pair",
    "[integration][development-profile][federation-management][time-management]"
    "[order-type-lookup][rti.service.get-order-type]"
    "[rti.service.get-order-name][2025]") {
  TestFederateAmbassador unjoinedFederate;
  TestFederateAmbassador memberFederate;
  auto unjoined = makeRti();
  auto member = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  OrderType invalid = static_cast<OrderType>(0x7f);

  REQUIRE_THROWS_AS(
      unjoined->getOrderType(L"Receive"),
      rti1516_2025::NotConnected);
  REQUIRE_THROWS_AS(
      unjoined->getOrderName(RECEIVE),
      rti1516_2025::NotConnected);
  REQUIRE_NOTHROW(unjoined->connect(unjoinedFederate, HLA_EVOKED));
  REQUIRE_THROWS_AS(
      unjoined->getOrderType(L"Receive"),
      rti1516_2025::FederateNotExecutionMember);
  REQUIRE_THROWS_AS(
      unjoined->getOrderName(RECEIVE),
      rti1516_2025::FederateNotExecutionMember);

  REQUIRE_NOTHROW(member->connect(memberFederate, HLA_EVOKED));
  REQUIRE_NOTHROW(
      member->createFederationExecution(
          federationName,
          fomModule,
          standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(
      member->joinFederationExecution(L"order-member", L"member", federationName));

  auto const receive = member->getOrderType(L"Receive");
  auto const timestamp = member->getOrderType(L"TimeStamp");
  REQUIRE(receive == RECEIVE);
  REQUIRE(timestamp == TIMESTAMP);
  REQUIRE(receive != timestamp);
  REQUIRE(member->getOrderType(L"Receive") == receive);
  REQUIRE(member->getOrderName(receive) == L"Receive");
  REQUIRE(member->getOrderName(timestamp) == L"TimeStamp");
  REQUIRE_THROWS_AS(
      member->getOrderType(standard_hla::mom::custom_order),
      rti1516_2025::InvalidOrderName);
  REQUIRE_THROWS_AS(
      member->getOrderName(invalid),
      rti1516_2025::InvalidOrderType);

  REQUIRE_NOTHROW(member->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(member->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(unjoined->disconnect());
  REQUIRE_NOTHROW(member->disconnect());
}
