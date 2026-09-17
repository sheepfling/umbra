#include <catch2/catch_test_macros.hpp>

#include <RTI/NullFederateAmbassador.h>
#include <RTI/RTI1516.h>

#include "hla_test_names.hpp"

#include <atomic>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#ifndef UMBRA_SOURCE_DIRECTORY
#error "The FOM declaration-management tests require the Umbra source directory."
#endif

namespace {

std::unique_ptr<rti1516_2025::RTIambassador> makeRti() {
  rti1516_2025::RTIambassadorFactory factory;
  return factory.createRTIambassador();
}

std::filesystem::path resourcePath(std::filesystem::path const& relativePath) {
  return std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
      "third_party" / "ieee1516.2-2025" / "resources" / relativePath;
}

std::filesystem::path testDataPath(std::filesystem::path const& relativePath) {
  return std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" /
      "data" / relativePath;
}

std::wstring nextFederationName() {
  static std::atomic_uint64_t counter{0};
  return L"umbra-catch2-empty-fom-" + std::to_wstring(++counter);
}

TEST_CASE(
    "Embedded Create Federation Execution rejects an empty FOM module set",
    "[integration][development-profile][federation-management][fom][fom-module-management]"
    "[rti.service.create-federation-execution]"
    "[rti.service.create-federation-execution-with-mim]") {
  rti1516_2025::NullFederateAmbassador federate;
  auto creator = makeRti();
  auto const federationName = nextFederationName();
  auto const validFom = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  auto const mimModule = resourcePath("mim/HLAstandardMIM-2025.xml").wstring();

  REQUIRE_NOTHROW(creator->connect(federate, rti1516_2025::HLA_EVOKED));

  // IEEE 1516.1-2025 §4.5.5 requires at least one FOM module for both
  // Create Federation Execution overloads. Validation must precede any
  // federation-registry mutation, including when an explicit MIM is supplied.
  REQUIRE_THROWS_AS(
      creator->createFederationExecution(
          federationName,
          std::vector<std::wstring>{},
          L"HLAinteger64Time"),
      rti1516_2025::InvalidFOM);
  REQUIRE_THROWS_AS(
      creator->createFederationExecutionWithMIM(
          federationName,
          std::vector<std::wstring>{},
          mimModule,
          L"HLAinteger64Time"),
      rti1516_2025::InvalidFOM);

  // A valid create proves that neither rejected request left a partial
  // federation execution behind.
  REQUIRE_NOTHROW(creator->createFederationExecution(
      federationName,
      validFom,
      L"HLAinteger64Time"));
  REQUIRE_NOTHROW(creator->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(creator->disconnect());
}

TEST_CASE(
    "Embedded Join Federation Execution composes an additional FOM module for all members",
    "[integration][development-profile][federation-management][fom][fom-module-management]"
    "[rti.service.join-federation-execution]"
    "[rti.service.get-object-class-handle][rti.service.get-object-class-name]") {
  rti1516_2025::NullFederateAmbassador creatorFederate;
  rti1516_2025::NullFederateAmbassador ownerFederate;
  rti1516_2025::NullFederateAmbassador extensionFederate;
  auto creator = makeRti();
  auto owner = makeRti();
  auto extensionJoiner = makeRti();
  auto const federationName = nextFederationName();
  auto const baseFom = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  auto const extensionFom = testDataPath("reference-data-class-provider-fom.xml").wstring();
  auto const baseClassName = std::wstring{L"HLAobjectRoot.Employee"};
  auto const extensionClassName = std::wstring{L"HLAobjectRoot.UmbraReferenceFixtureClass"};

  REQUIRE_NOTHROW(creator->connect(creatorFederate, rti1516_2025::HLA_EVOKED));
  REQUIRE_NOTHROW(owner->connect(ownerFederate, rti1516_2025::HLA_EVOKED));
  REQUIRE_NOTHROW(extensionJoiner->connect(extensionFederate, rti1516_2025::HLA_EVOKED));
  REQUIRE_NOTHROW(creator->createFederationExecution(
      federationName,
      baseFom,
      L"HLAinteger64Time"));
  REQUIRE_NOTHROW(owner->joinFederationExecution(
      L"fom-composition-owner",
      L"owner",
      federationName));

  auto const baseClass = owner->getObjectClassHandle(baseClassName);
  REQUIRE(baseClass.isValid());
  REQUIRE_THROWS_AS(
      owner->getObjectClassHandle(extensionClassName),
      rti1516_2025::NameNotFound);

  auto const extensionHandle = extensionJoiner->joinFederationExecution(
      L"fom-composition-extension",
      L"extension",
      federationName,
      std::vector<std::wstring>{extensionFom});
  REQUIRE(extensionHandle.isValid());

  // IEEE 1516.1-2025 requires the successfully supplied modules to become
  // part of the current FOM. The new class must be visible to the joining
  // federate and to an existing member, while the old handle remains stable.
  auto const extensionClass = owner->getObjectClassHandle(extensionClassName);
  REQUIRE(extensionClass.isValid());
  REQUIRE(extensionJoiner->getObjectClassHandle(extensionClassName) == extensionClass);
  REQUIRE(owner->getObjectClassHandle(baseClassName) == baseClass);
  REQUIRE(owner->getObjectClassName(extensionClass) == extensionClassName);

  REQUIRE_NOTHROW(extensionJoiner->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(owner->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(creator->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(creator->disconnect());
  REQUIRE_NOTHROW(owner->disconnect());
  REQUIRE_NOTHROW(extensionJoiner->disconnect());
}

TEST_CASE(
    "Embedded object class attribute declarations retain 2025 FOM and lifecycle boundaries",
    "[integration][development-profile][federation-management][declaration-management]"
    "[publish-object-class-attributes]"
    "[unpublish-object-class-attributes]"
    "[rti.service.publish-object-class-attributes]"
    "[rti.service.unpublish-object-class]"
    "[rti.service.unpublish-object-class-attributes]"
    "[rti.service.subscribe-object-class-attributes]"
    "[rti.service.unsubscribe-object-class]"
    "[rti.service.unsubscribe-object-class-attributes]") {
  rti1516_2025::NullFederateAmbassador unjoinedFederate;
  rti1516_2025::NullFederateAmbassador publisherFederate;
  rti1516_2025::NullFederateAmbassador subscriberFederate;
  auto unjoined = makeRti();
  auto publisher = makeRti();
  auto subscriber = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  rti1516_2025::ObjectClassHandle invalidObjectClass;
  rti1516_2025::AttributeHandle invalidAttribute;
  rti1516_2025::AttributeHandleSet const noAttributes;

  REQUIRE_THROWS_AS(
      publisher->publishObjectClassAttributes(invalidObjectClass, noAttributes),
      rti1516_2025::NotConnected);
  REQUIRE_THROWS_AS(
      publisher->unpublishObjectClass(invalidObjectClass),
      rti1516_2025::NotConnected);

  REQUIRE_NOTHROW(unjoined->connect(unjoinedFederate, rti1516_2025::HLA_EVOKED));
  REQUIRE_NOTHROW(publisher->connect(publisherFederate, rti1516_2025::HLA_EVOKED));
  REQUIRE_NOTHROW(subscriber->connect(subscriberFederate, rti1516_2025::HLA_EVOKED));
  REQUIRE_THROWS_AS(
      unjoined->subscribeObjectClassAttributes(invalidObjectClass, noAttributes),
      rti1516_2025::FederateNotExecutionMember);
  REQUIRE_THROWS_AS(
      unjoined->unsubscribeObjectClass(invalidObjectClass),
      rti1516_2025::FederateNotExecutionMember);

  REQUIRE_NOTHROW(
      publisher->createFederationExecution(
          federationName, fomModule, L"HLAinteger64Time"));
  REQUIRE_NOTHROW(publisher->joinFederationExecution(L"publisher", federationName));
  REQUIRE_NOTHROW(subscriber->joinFederationExecution(L"subscriber", federationName));

  auto const employee = publisher->getObjectClassHandle(
      umbra::test::hla::wide::fom::employee);
  auto const server = publisher->getObjectClassHandle(
      umbra::test::hla::wide::fom::employee_server);
  auto const name = publisher->getAttributeHandle(
      employee, umbra::test::hla::wide::fixture::name);
  auto const inheritedName = publisher->getAttributeHandle(
      server, umbra::test::hla::wide::fixture::name);
  auto const efficiency = publisher->getAttributeHandle(
      server, umbra::test::hla::wide::fixture::efficiency);
  REQUIRE(name == inheritedName);

  rti1516_2025::AttributeHandleSet const nameOnly{name};
  rti1516_2025::AttributeHandleSet const efficiencyOnly{efficiency};
  rti1516_2025::AttributeHandleSet const invalidOnly{invalidAttribute};
  REQUIRE_THROWS_AS(
      publisher->publishObjectClassAttributes(invalidObjectClass, nameOnly),
      rti1516_2025::ObjectClassNotDefined);
  REQUIRE_THROWS_AS(
      publisher->unpublishObjectClass(invalidObjectClass),
      rti1516_2025::ObjectClassNotDefined);
  REQUIRE_THROWS_AS(
      publisher->publishObjectClassAttributes(employee, invalidOnly),
      rti1516_2025::AttributeNotDefined);
  REQUIRE_THROWS_AS(
      publisher->publishObjectClassAttributes(employee, efficiencyOnly),
      rti1516_2025::AttributeNotDefined);

  REQUIRE_NOTHROW(publisher->publishObjectClassAttributes(employee, nameOnly));
  REQUIRE_NOTHROW(publisher->publishObjectClassAttributes(server, efficiencyOnly));
  REQUIRE_NOTHROW(
      subscriber->subscribeObjectClassAttributes(server, nameOnly, false));
  REQUIRE_NOTHROW(
      subscriber->subscribeObjectClassAttributes(server, efficiencyOnly, true));
  // The whole-class form removes ordinary subscriptions at that exact class;
  // regional declarations remain an independent DDM state family.
  REQUIRE_NOTHROW(subscriber->unsubscribeObjectClass(server));
  REQUIRE_NOTHROW(
      subscriber->subscribeObjectClassAttributes(server, nameOnly, false));
  REQUIRE_NOTHROW(
      subscriber->subscribeObjectClassAttributes(server, efficiencyOnly, true));
  // Whole-class unpublication removes every currently published attribute at
  // that class, while the inherited Name publication on Employee remains
  // until its own class is unpublished.
  REQUIRE_NOTHROW(publisher->unpublishObjectClass(server));
  REQUIRE_NOTHROW(publisher->unpublishObjectClass(employee));
  REQUIRE_NOTHROW(publisher->unpublishObjectClass(server));
  REQUIRE_NOTHROW(subscriber->unsubscribeObjectClass(server));
  REQUIRE_NOTHROW(
      subscriber->unsubscribeObjectClassAttributes(server, nameOnly));
  REQUIRE_NOTHROW(
      publisher->unpublishObjectClassAttributes(employee, nameOnly));

  REQUIRE_NOTHROW(
      publisher->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(
      subscriber->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(unjoined->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
  REQUIRE_NOTHROW(subscriber->disconnect());
}

}  // namespace
