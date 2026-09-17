#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include "internal/fom/hla_names.hpp"
#include "hla_test_names.hpp"

#include <RTI/NullFederateAmbassador.h>
#include <RTI/RTI1516.h>

#include <atomic>
#include <filesystem>
#include <memory>

#ifndef UMBRA_SOURCE_DIRECTORY
#error "The update-rate value test requires the Umbra source directory."
#endif

namespace {

namespace standard_hla = umbra::detail::hla::wide;
namespace fixture_hla = umbra::test::hla::wide;

using rti1516_2025::AttributeHandle;
using rti1516_2025::AttributeHandleSet;
using rti1516_2025::HLA_EVOKED;
using rti1516_2025::ObjectInstanceHandle;
using rti1516_2025::RTIambassador;
using rti1516_2025::RTIambassadorFactory;

std::wstring nextFederationName() {
  static std::atomic_uint64_t sequence{0U};
  return L"update-rate-value-" +
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
    "Standalone update-rate lookup exposes FDD values and the default attribute boundary",
    "[integration][development-profile][federation-management][object-management]"
    "[fom][update-rate-value][standalone][2025]"
    "[rti.service.get-update-rate-value]"
    "[rti.service.get-update-rate-value-for-attribute]"
    "[rti.service.subscribe-object-class-attributes]"
    "[rti.service.unsubscribe-object-class-attributes]") {
  rti1516_2025::NullFederateAmbassador unjoinedCallbacks;
  rti1516_2025::NullFederateAmbassador memberCallbacks;
  auto unjoined = makeRti();
  auto member = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule =
      resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  ObjectInstanceHandle invalidObjectInstance;
  AttributeHandle invalidAttribute;

  REQUIRE_THROWS_AS(
      unjoined->getUpdateRateValue(L"High"),
      rti1516_2025::NotConnected);
  REQUIRE_THROWS_AS(
      unjoined->getUpdateRateValueForAttribute(invalidObjectInstance, invalidAttribute),
      rti1516_2025::NotConnected);
  REQUIRE_NOTHROW(unjoined->connect(unjoinedCallbacks, HLA_EVOKED));
  REQUIRE_THROWS_AS(
      unjoined->getUpdateRateValue(L"High"),
      rti1516_2025::FederateNotExecutionMember);
  REQUIRE_THROWS_AS(
      unjoined->getUpdateRateValueForAttribute(invalidObjectInstance, invalidAttribute),
      rti1516_2025::FederateNotExecutionMember);

  REQUIRE_NOTHROW(member->connect(memberCallbacks, HLA_EVOKED));
  REQUIRE_NOTHROW(member->createFederationExecution(
      federationName,
      fomModule,
      standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(member->joinFederationExecution(
      L"update-rate-member", L"member", federationName));

  REQUIRE(member->getUpdateRateValue(L"High") == Catch::Approx(30.0));
  REQUIRE(member->getUpdateRateValue(L"Medium") == Catch::Approx(5.0));
  REQUIRE(member->getUpdateRateValue(L"Low") == Catch::Approx(0.2));
  REQUIRE(member->getUpdateRateValue(standard_hla::mom::default_update_rate) ==
          Catch::Approx(0.0));
  REQUIRE(member->getUpdateRateValue(L"default") == Catch::Approx(0.0));
  REQUIRE(member->getUpdateRateValue(
              standard_hla::mom::default_update_rate_attribute) ==
          Catch::Approx(0.0));
  REQUIRE(member->getUpdateRateValue(L"") == Catch::Approx(0.0));
  REQUIRE_THROWS_AS(
      member->getUpdateRateValue(L"MissingRate"),
      rti1516_2025::InvalidUpdateRateDesignator);

  auto const server = member->getObjectClassHandle(
      fixture_hla::fom::employee_server);
  auto const name = member->getAttributeHandle(
      server, fixture_hla::fixture::name);
  auto const payRate = member->getAttributeHandle(
      server, fixture_hla::fixture::pay_rate);
  AttributeHandleSet const nameOnly{name};
  AttributeHandleSet const payRateOnly{payRate};
  REQUIRE_NOTHROW(member->publishObjectClassAttributes(
      server, AttributeHandleSet{name, payRate}));
  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = member->registerObjectInstance(server));

  REQUIRE_THROWS_AS(
      member->subscribeObjectClassAttributes(server, nameOnly, true, L"MissingRate"),
      rti1516_2025::InvalidUpdateRateDesignator);
  REQUIRE_NOTHROW(member->subscribeObjectClassAttributes(
      server, nameOnly, true, L"High"));
  REQUIRE_NOTHROW(member->subscribeObjectClassAttributes(
      server, payRateOnly, true, L"Medium"));
  REQUIRE(member->getUpdateRateValueForAttribute(objectInstance, name) ==
          Catch::Approx(30.0));
  REQUIRE(member->getUpdateRateValueForAttribute(objectInstance, payRate) ==
          Catch::Approx(5.0));

  // Passive declarations remain in the ledger but do not contribute to this
  // active update-rate query until replaced by an active declaration.
  REQUIRE_NOTHROW(member->subscribeObjectClassAttributes(
      server, nameOnly, false, L"High"));
  REQUIRE(member->getUpdateRateValueForAttribute(objectInstance, name) ==
          Catch::Approx(0.0));
  REQUIRE_NOTHROW(member->subscribeObjectClassAttributes(
      server, nameOnly, true, L"High"));
  REQUIRE(member->getUpdateRateValueForAttribute(objectInstance, name) ==
          Catch::Approx(30.0));

  REQUIRE_NOTHROW(member->unsubscribeObjectClassAttributes(server, nameOnly));
  REQUIRE(member->getUpdateRateValueForAttribute(objectInstance, name) ==
          Catch::Approx(0.0));
  REQUIRE(member->getUpdateRateValueForAttribute(objectInstance, payRate) ==
          Catch::Approx(5.0));
  REQUIRE_NOTHROW(member->unsubscribeObjectClassAttributes(server, payRateOnly));
  REQUIRE(member->getUpdateRateValueForAttribute(objectInstance, name) ==
          Catch::Approx(0.0));
  REQUIRE(member->getUpdateRateValueForAttribute(objectInstance, payRate) ==
          Catch::Approx(0.0));
  REQUIRE_THROWS_AS(
      member->getUpdateRateValueForAttribute(invalidObjectInstance, name),
      rti1516_2025::ObjectInstanceNotKnown);
  REQUIRE_THROWS_AS(
      member->getUpdateRateValueForAttribute(objectInstance, invalidAttribute),
      rti1516_2025::AttributeNotDefined);

  REQUIRE_NOTHROW(member->unpublishObjectClassAttributes(
      server, AttributeHandleSet{name, payRate}));
  REQUIRE_NOTHROW(member->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(member->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(unjoined->disconnect());
  REQUIRE_NOTHROW(member->disconnect());
}
