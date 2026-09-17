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
#error "The attribute lookup tests require the Umbra source directory."
#endif

namespace {

namespace standard_hla = umbra::detail::hla::wide;
namespace fixture_hla = umbra::test::hla::wide;

using rti1516_2025::AttributeHandle;
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
  return L"umbra-attribute-lookup-" + std::to_wstring(++counter);
}

}  // namespace

TEST_CASE(
    "Embedded attribute lookup resolves inherited definitions in the joined federation FOM",
    "[integration][development-profile][federation-management][attribute-lookup]"
    "[rti.service.get-attribute-handle][rti.service.get-attribute-name]"
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
      "cpp" / "tests" / "data" / "reference-data-attribute-provider-fom.xml";
  ObjectClassHandle invalidObjectClass;
  AttributeHandle invalidAttribute;

  REQUIRE_THROWS_AS(
      unjoined->getAttributeHandle(invalidObjectClass, fixture_hla::fixture::name),
      rti1516_2025::NotConnected);
  REQUIRE_THROWS_AS(
      unjoined->getAttributeName(invalidObjectClass, invalidAttribute),
      rti1516_2025::NotConnected);
  REQUIRE_NOTHROW(unjoined->connect(unjoinedFederate, HLA_EVOKED));
  REQUIRE_THROWS_AS(
      unjoined->getAttributeHandle(invalidObjectClass, fixture_hla::fixture::name),
      rti1516_2025::FederateNotExecutionMember);
  REQUIRE_THROWS_AS(
      unjoined->getAttributeName(invalidObjectClass, invalidAttribute),
      rti1516_2025::FederateNotExecutionMember);

  REQUIRE_NOTHROW(owner->connect(ownerFederate, HLA_EVOKED));
  REQUIRE_NOTHROW(extensionJoiner->connect(extensionFederate, HLA_EVOKED));
  REQUIRE_NOTHROW(
      owner->createFederationExecution(
          federationName, baseFom, standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(owner->joinFederationExecution(
      L"attribute-owner", L"owner", federationName));

  auto const employee = owner->getObjectClassHandle(fixture_hla::fom::employee);
  auto const server = owner->getObjectClassHandle(fixture_hla::fom::employee_server);
  auto const customer = owner->getObjectClassHandle(fixture_hla::fom::customer);
  auto const employeeName = owner->getAttributeHandle(employee, fixture_hla::fixture::name);
  REQUIRE(employeeName.isValid());
  REQUIRE(owner->getAttributeHandle(employee, fixture_hla::fixture::name) == employeeName);
  REQUIRE(owner->getAttributeHandle(server, fixture_hla::fixture::name) == employeeName);
  REQUIRE(owner->getAttributeName(employee, employeeName) == fixture_hla::fixture::name);
  REQUIRE(owner->getAttributeName(server, employeeName) == fixture_hla::fixture::name);
  REQUIRE_THROWS_AS(
      owner->getAttributeHandle(server, fixture_hla::fixture::missing),
      rti1516_2025::NameNotFound);
  REQUIRE_THROWS_AS(
      owner->getAttributeHandle(invalidObjectClass, fixture_hla::fixture::name),
      rti1516_2025::InvalidObjectClassHandle);
  REQUIRE_THROWS_AS(
      owner->getAttributeName(invalidObjectClass, employeeName),
      rti1516_2025::InvalidObjectClassHandle);
  REQUIRE_THROWS_AS(
      owner->getAttributeName(employee, invalidAttribute),
      rti1516_2025::InvalidAttributeHandle);
  REQUIRE_THROWS_AS(
      owner->getAttributeName(customer, employeeName),
      rti1516_2025::AttributeNotDefined);

  // A compatible additional-FOM join adds a new inheritance chain. The
  // original Employee::Name handle stays stable, while the extension's
  // defining attribute has the same handle through its child class.
  REQUIRE_NOTHROW(extensionJoiner->joinFederationExecution(
      L"attribute-extension",
      L"observer",
      federationName,
      std::vector<std::wstring>{extensionFom.wstring()}));
  REQUIRE(owner->getAttributeHandle(employee, fixture_hla::fixture::name) == employeeName);
  auto const extensionBase = owner->getObjectClassHandle(
      fixture_hla::fom::attribute_fixture_base);
  auto const extensionChild = owner->getObjectClassHandle(
      fixture_hla::fom::attribute_fixture_class);
  auto const identifier = owner->getAttributeHandle(
      extensionBase, fixture_hla::fixture::identifier);
  REQUIRE(identifier.isValid());
  REQUIRE(owner->getAttributeHandle(extensionChild, fixture_hla::fixture::identifier) == identifier);
  REQUIRE(owner->getAttributeName(extensionChild, identifier) == fixture_hla::fixture::identifier);

  REQUIRE_NOTHROW(extensionJoiner->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(unjoined->disconnect());
  REQUIRE_NOTHROW(owner->disconnect());
  REQUIRE_NOTHROW(extensionJoiner->disconnect());
}
