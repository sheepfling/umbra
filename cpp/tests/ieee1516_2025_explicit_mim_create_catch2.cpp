#include "ieee1516_2025_federation_management_fixture_support.hpp"

namespace {
TEST_CASE(
    "Embedded Create Federation Execution accepts a validated explicit MIM path",
    "[integration][development-profile][federation-management][fom][mim]"
    "[rti.service.create-federation-execution-with-mim]"
    "[federation-management][explicit-mim]") {
  FederationEventFederateAmbassador reports;
  auto creator = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  auto const mimModule = resourcePath("mim/HLAstandardMIM-2025.xml").wstring();

  REQUIRE_NOTHROW(creator->connect(reports, HLA_EVOKED));
  // The reserved spelling HLAstandardMIM is rejected by the explicit-MIM
  // overload. A valid local path to the same official 2025 MIM is a distinct
  // caller-supplied designator and must take the normal validation,
  // composition, and atomic-create path.
  REQUIRE_NOTHROW(creator->createFederationExecutionWithMIM(
      federationName,
      std::vector<std::wstring>{fomModule},
      mimModule,
      standard_hla::mom::integer64_time));
  auto const federate = creator->joinFederationExecution(
      L"explicit-mim-subject", L"observer", federationName);
  REQUIRE(federate.isValid());
  REQUIRE_NOTHROW(creator->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(creator->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(creator->disconnect());
}
}  // namespace
