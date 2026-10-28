#include "ieee1516_2025_federation_management_test_support.hpp"
#include "ieee1516_2025_federation_management_fixture_support.hpp"

#include <RTI/time/HLAinteger64Time.h>
#include <RTI/time/HLAinteger64Interval.h>

namespace {
TEST_CASE(
    "Embedded public time management preserves the official HLAinteger64Time representation",
    "[integration][development-profile][time-management][integer-time]"
    "[rti.service.enable-time-regulation][rti.service.time-advance-request]"
    "[rti.service.query-logical-time][rti.service.query-lookahead]"
    "[federate.callback.time-regulation-enabled][federate.callback.time-advance-grant]") {
  ReportingFederateAmbassador reports;
  auto rti = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule =
      (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data" /
       "time-representation-integer64-fom.xml")
          .wstring();

  REQUIRE_NOTHROW(rti->connect(reports, HLA_EVOKED));
  REQUIRE_NOTHROW(rti->createFederationExecution(
      federationName,
      fomModule,
      standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(rti->joinFederationExecution(L"integer-time-client", federationName));

  auto factory = rti->getTimeFactory();
  REQUIRE(factory);
  REQUIRE(factory->getName() == standard_hla::mom::integer64_time);
  auto initial = factory->makeInitial();
  auto* integerInitial = dynamic_cast<rti1516_2025::HLAinteger64Time*>(initial.get());
  REQUIRE(integerInitial != nullptr);
  REQUIRE(integerInitial->isInitial());

  rti1516_2025::HLAinteger64Time queriedTime;
  REQUIRE_NOTHROW(rti->queryLogicalTime(queriedTime));
  REQUIRE(queriedTime.getTime() == 0);
  REQUIRE(queriedTime.implementationName() == standard_hla::mom::integer64_time);

  REQUIRE_NOTHROW(rti->enableTimeRegulation(rti1516_2025::HLAinteger64Interval(2)));
  REQUIRE(reports.timeRegulationEnabledReports.empty());
  REQUIRE_FALSE(rti->evokeCallback(0.0));
  REQUIRE(reports.timeRegulationEnabledReports.size() == 1U);
  REQUIRE(reports.timeRegulationEnabledReports.front().implementationName == standard_hla::mom::integer64_time);
  REQUIRE(reports.timeRegulationEnabledReports.front().value == L"0");

  rti1516_2025::HLAinteger64Interval queriedLookahead;
  REQUIRE_NOTHROW(rti->queryLookahead(queriedLookahead));
  REQUIRE(queriedLookahead.getInterval() == 2);

  REQUIRE_NOTHROW(rti->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(15)));
  REQUIRE_NOTHROW(rti->queryLogicalTime(queriedTime));
  REQUIRE(queriedTime.getTime() == 0);
  REQUIRE(reports.timeAdvanceGrantReports.empty());

  REQUIRE_FALSE(rti->evokeCallback(0.0));
  REQUIRE(reports.timeAdvanceGrantReports.size() == 1U);
  REQUIRE(reports.timeAdvanceGrantReports.front().implementationName == standard_hla::mom::integer64_time);
  REQUIRE(reports.timeAdvanceGrantReports.front().value == L"15");
  REQUIRE_NOTHROW(rti->queryLogicalTime(queriedTime));
  REQUIRE(queriedTime.getTime() == 15);

  REQUIRE_NOTHROW(rti->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(rti->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(rti->disconnect());
}
} // namespace
