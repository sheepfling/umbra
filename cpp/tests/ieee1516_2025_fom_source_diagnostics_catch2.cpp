#include "ieee1516_2025_federation_management_fixture_support.hpp"

namespace {
TEST_CASE(
    "Embedded Create Federation Execution maps an existing non-file FOM source to ErrorReadingFOM",
    "[integration][development-profile][federation-management][fom][fom-source-diagnostics]"
    "[rti.service.create-federation-execution]") {
  FederationEventFederateAmbassador reports;
  auto creator = makeRti();
  auto const federationName = nextFederationName();
  auto const directorySource = resourcePath("examples").wstring();
  auto const validFom = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();

  REQUIRE_NOTHROW(creator->connect(reports, HLA_EVOKED));
  // The designator exists, but it names a directory rather than an XML file.
  // That is a determined read failure, not a missing path, and must not leave
  // a partially-created federation behind.
  REQUIRE_THROWS_AS(
      creator->createFederationExecution(
          federationName,
          directorySource,
          standard_hla::mom::integer64_time),
      rti1516_2025::ErrorReadingFOM);

  REQUIRE_NOTHROW(creator->createFederationExecution(
      federationName,
      validFom,
      standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(creator->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(creator->disconnect());
}
}  // namespace
