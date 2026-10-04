#include "ieee1516_2025_federation_management_fixture_support.hpp"

namespace {
TEST_CASE(
    "Embedded federation-management preparation failures leave shared federation state unchanged",
    "[integration][development-profile][federation-management][rti.service.create-federation-execution]"
    "[rti.service.join-federation-execution][rti.service.resign-federation-execution]"
    "[rti.service.destroy-federation-execution][rti.service.disconnect][fom-preparation-failure-atomicity]") {
  TestFederateAmbassador ownerFederate;
  TestFederateAmbassador applicantFederate;
  auto owner = makeRti();
  auto applicant = makeRti();
  auto const federationName = nextFederationName();
  auto const baseFom = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  auto const extensionFom = resourcePath("examples/RestaurantExtensionFOMmodule-2025.xml").wstring();

  REQUIRE_NOTHROW(owner->connect(ownerFederate, HLA_EVOKED));
  REQUIRE_NOTHROW(applicant->connect(applicantFederate, HLA_EVOKED));

  // The base module documents HLAinteger64Time, so the no-name Create default
  // must fail without reserving the federation name.
  REQUIRE_THROWS_AS(
      owner->createFederationExecution(federationName, baseFom),
      rti1516_2025::InconsistentFOM);
  REQUIRE_NOTHROW(
      owner->createFederationExecution(federationName, baseFom, standard_hla::mom::integer64_time));

  // The supplied extension cannot be materialized by the official FDD XSD.
  // Its failed addition must leave the valid base definition joinable.
  REQUIRE_THROWS_AS(
      applicant->joinFederationExecution(
          L"applicant",
          L"observer",
          federationName,
          std::vector<std::wstring>{extensionFom}),
      rti1516_2025::InconsistentFOM);
  FederateHandle applicantHandle;
  REQUIRE_NOTHROW(
      applicantHandle = applicant->joinFederationExecution(L"applicant", L"observer", federationName));
  REQUIRE(applicantHandle.isValid());

  REQUIRE_NOTHROW(applicant->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(owner->disconnect());
  REQUIRE_NOTHROW(applicant->disconnect());
}
} // namespace
