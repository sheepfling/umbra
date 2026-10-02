#include "ieee1516_2025_federation_management_test_support.hpp"
#include "ieee1516_2025_federation_management_fixture_support.hpp"

TEST_CASE(
    "Embedded joins seed advisory switches from the current composed FDD",
    "[integration][development-profile][federation-management][fom][switches]"
    "[rti.service.join-federation-execution]"
    "[rti.service.get-object-class-relevance-advisory-switch]"
    "[rti.service.get-interaction-relevance-advisory-switch][2025]") {
  TestFederateAmbassador baseReports;
  TestFederateAmbassador extensionReports;
  auto base = makeRti();
  auto extension = makeRti();
  auto const federationName = nextFederationName();
  auto const baseFom =
      (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data" /
       "switch-nrg-disabled-fom.xml").wstring();
  auto const advisoryFom = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();

  REQUIRE_NOTHROW(base->connect(baseReports, HLA_EVOKED));
  REQUIRE_NOTHROW(extension->connect(extensionReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      base->createFederationExecution(federationName, baseFom, standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(
      base->joinFederationExecution(L"switch-base", L"base", federationName));

  // The base FOM omits both advisory entries, so the standard Disabled
  // default is applied to the first member.
  REQUIRE_FALSE(base->getObjectClassRelevanceAdvisorySwitch());
  REQUIRE_FALSE(base->getInteractionRelevanceAdvisorySwitch());

  // The additional module contributes explicit Enabled settings. It seeds
  // only the new member; the existing member's independently owned values are
  // not reset by the composed-definition replacement.
  REQUIRE_NOTHROW(extension->joinFederationExecution(
      L"switch-extension",
      L"extension",
      federationName,
      std::vector<std::wstring>{advisoryFom}));
  REQUIRE(extension->getObjectClassRelevanceAdvisorySwitch());
  REQUIRE(extension->getInteractionRelevanceAdvisorySwitch());
  REQUIRE_FALSE(base->getObjectClassRelevanceAdvisorySwitch());
  REQUIRE_FALSE(base->getInteractionRelevanceAdvisorySwitch());

  REQUIRE_NOTHROW(extension->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(base->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(base->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(base->disconnect());
  REQUIRE_NOTHROW(extension->disconnect());
}
