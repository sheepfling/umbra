#include "ieee1516_2025_federation_management_fixture_support.hpp"

namespace {
TEST_CASE(
    "Embedded Flush Queue Request reports Flush Queue Grant instead of Time Advance Grant",
    "[integration][development-profile][time-management][callbacks]"
    "[flush-queue-request-grant-callback-kind][rti.service.flush-queue-request]"
    "[federate.callback.flush-queue-grant][federate.callback.time-advance-grant]") {
  ReportingFederateAmbassador reports;
  auto rti = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();

  REQUIRE_NOTHROW(rti->connect(reports, HLA_EVOKED));
  REQUIRE_NOTHROW(rti->createFederationExecution(
      federationName,
      fomModule,
      standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(rti->joinFederationExecution(L"fqr-grant-kind-client", federationName));

  REQUIRE_NOTHROW(rti->enableTimeConstrained());
  REQUIRE_FALSE(rti->evokeCallback(0.0));
  REQUIRE(reports.timeConstrainedEnabledReports.size() == 1U);
  REQUIRE(reports.timeAdvanceGrantReports.empty());

  REQUIRE_NOTHROW(rti->flushQueueRequest(rti1516_2025::HLAinteger64Time(7)));
  REQUIRE(reports.flushQueueGrantReports.empty());
  REQUIRE(reports.timeAdvanceGrantReports.empty());

  REQUIRE_FALSE(rti->evokeCallback(0.0));
  REQUIRE(reports.flushQueueGrantReports.size() == 1U);
  REQUIRE(reports.flushQueueGrantReports.front().value == L"7");
  REQUIRE(reports.timeAdvanceGrantReports.empty());

  REQUIRE_NOTHROW(rti->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(rti->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(rti->disconnect());
}
} // namespace
