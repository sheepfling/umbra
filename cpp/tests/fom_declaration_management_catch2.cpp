#include <catch2/catch_test_macros.hpp>

#include <RTI/NullFederateAmbassador.h>
#include <RTI/RTI1516.h>

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

}  // namespace
