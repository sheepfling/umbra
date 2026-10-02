#include "ieee1516_2025_federation_management_test_support.hpp"
#include "ieee1516_2025_federation_management_fixture_support.hpp"

TEST_CASE(
    "Embedded federation shares the static Advisories Use Known Class switch",
    "[integration][development-profile][federation-management][fom][switches]"
    "[rti.service.join-federation-execution]"
    "[rti.service.get-advisories-use-known-class-switch]"
    "[advisories-use-known-class-switch][2025]") {
  TestFederateAmbassador baseReports;
  TestFederateAmbassador knownClassReports;
  auto base = makeRti();
  auto knownClass = makeRti();
  auto const federationName = nextFederationName();
  auto const knownClassFom =
      (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data" /
       "switch-known-class-enabled-fom.xml").wstring();
  auto const disabledFom =
      (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data" /
       "switch-nrg-disabled-fom.xml").wstring();

  REQUIRE_NOTHROW(base->connect(baseReports, HLA_EVOKED));
  REQUIRE_NOTHROW(knownClass->connect(knownClassReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      base->createFederationExecution(federationName, knownClassFom, standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(
      base->joinFederationExecution(L"known-class-base", L"base", federationName));
  REQUIRE(base->getAdvisoriesUseKnownClassSwitch());

  // The additional FOM contributes an explicit Disabled value, but the
  // creation-time static switch remains shared by both federates.
  REQUIRE_NOTHROW(knownClass->joinFederationExecution(
      L"known-class-enabled",
      L"known-class",
      federationName,
      std::vector<std::wstring>{disabledFom}));
  REQUIRE(knownClass->getAdvisoriesUseKnownClassSwitch());
  REQUIRE(base->getAdvisoriesUseKnownClassSwitch());

  REQUIRE_NOTHROW(knownClass->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(base->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(base->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(base->disconnect());
  REQUIRE_NOTHROW(knownClass->disconnect());
}
