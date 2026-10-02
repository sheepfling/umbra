#include "ieee1516_2025_federation_management_test_support.hpp"
#include "ieee1516_2025_federation_management_fixture_support.hpp"

TEST_CASE(
    "Embedded joins preserve an explicit FOM NoAction automatic-resign directive",
    "[integration][development-profile][federation-management][fom][switches]"
    "[rti.service.join-federation-execution]"
    "[rti.service.get-automatic-resign-directive]"
    "[join-automatic-resign-directive][2025]") {
  TestFederateAmbassador reports;
  auto rti = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule =
      (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data" /
       "switch-explicit-no-action-fom.xml")
          .wstring();

  REQUIRE_NOTHROW(rti->connect(reports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      rti->createFederationExecution(federationName, fomModule, standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(rti->joinFederationExecution(
      L"explicit-no-action", L"observer", federationName));

  // The configured NoAction value is not the omitted FDD default. It must
  // reach the joined federate's per-member support-switch state unchanged.
  REQUIRE(rti->getAutomaticResignDirective() == rti1516_2025::NO_ACTION);

  REQUIRE_NOTHROW(rti->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(rti->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(rti->disconnect());
}
