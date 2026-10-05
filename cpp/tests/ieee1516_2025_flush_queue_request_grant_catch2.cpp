#include "ieee1516_2025_federation_management_test_support.hpp"
#include "ieee1516_2025_federation_management_fixture_support.hpp"

namespace {
TEST_CASE(
    "Embedded immediate Flush Queue Request delivers the Flush Queue Grant before returning",
    "[integration][development-profile][time-management][callbacks][flush-queue-request]"
    "[flush-queue-grant-logical-time-gating][rti.service.flush-queue-request]"
    "[rti.service.query-logical-time][rti.service.enable-time-constrained]"
    "[federate.callback.time-constrained-enabled][federate.callback.flush-queue-grant]") {
  ReportingFederateAmbassador reports;
  auto rti = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  rti1516_2025::HLAinteger64Time logicalTime;

  REQUIRE_NOTHROW(rti->connect(reports, rti1516_2025::HLA_IMMEDIATE));
  REQUIRE_NOTHROW(rti->createFederationExecution(
      federationName,
      fomModule,
      standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(rti->joinFederationExecution(L"immediate-flush-queue-client", federationName));
  REQUIRE_NOTHROW(rti->queryLogicalTime(logicalTime));
  REQUIRE(logicalTime.isInitial());

  REQUIRE_NOTHROW(rti->enableTimeConstrained());
  REQUIRE(reports.timeConstrainedEnabledReports.size() == 1U);
  REQUIRE_NOTHROW(rti->queryLogicalTime(logicalTime));
  REQUIRE(logicalTime.isInitial());

  REQUIRE_NOTHROW(rti->flushQueueRequest(rti1516_2025::HLAinteger64Time(7)));
  // The callback is observed immediately after the service returns; unlike
  // the HLA_EVOKED companion, this case does not dispatch it with evokeCallback.
  REQUIRE(reports.flushQueueGrantReports.size() == 1U);
  REQUIRE(reports.flushQueueGrantReports.front().value == L"7");
  REQUIRE_NOTHROW(rti->queryLogicalTime(logicalTime));
  REQUIRE(logicalTime.getTime() == 7);

  REQUIRE_NOTHROW(rti->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(rti->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(rti->disconnect());
}
} // namespace
