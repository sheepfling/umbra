#include "ieee1516_2025_federation_management_test_support.hpp"
#include "ieee1516_2025_federation_management_fixture_support.hpp"

namespace {
TEST_CASE(
    "Embedded public time management preserves the official HLAfloat64Time representation",
    "[integration][development-profile][time-management][float-time]"
    "[rti.service.enable-time-regulation][rti.service.time-advance-request]"
    "[rti.service.query-logical-time][rti.service.query-lookahead]"
    "[federate.callback.time-regulation-enabled][federate.callback.time-advance-grant]") {
  ReportingFederateAmbassador reports;
  auto rti = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule =
      (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data" /
       "time-representation-float64-fom.xml")
          .wstring();

  REQUIRE_NOTHROW(rti->connect(reports, HLA_EVOKED));
  REQUIRE_NOTHROW(rti->createFederationExecution(
      federationName,
      fomModule,
      standard_hla::mom::float64_time));
  REQUIRE_NOTHROW(rti->joinFederationExecution(L"float-time-client", federationName));

  auto factory = rti->getTimeFactory();
  REQUIRE(factory);
  REQUIRE(factory->getName() == standard_hla::mom::float64_time);
  auto initial = factory->makeInitial();
  auto* floatInitial = dynamic_cast<rti1516_2025::HLAfloat64Time*>(initial.get());
  REQUIRE(floatInitial != nullptr);
  REQUIRE(floatInitial->isInitial());

  rti1516_2025::HLAfloat64Time queriedTime;
  REQUIRE_NOTHROW(rti->queryLogicalTime(queriedTime));
  REQUIRE(queriedTime.getTime() == 0.0);
  REQUIRE(queriedTime.implementationName() == standard_hla::mom::float64_time);

  REQUIRE_NOTHROW(rti->enableTimeRegulation(rti1516_2025::HLAfloat64Interval(0.25)));
  REQUIRE(reports.timeRegulationEnabledReports.empty());
  REQUIRE_FALSE(rti->evokeCallback(0.0));
  REQUIRE(reports.timeRegulationEnabledReports.size() == 1U);
  REQUIRE(reports.timeRegulationEnabledReports.front().implementationName == standard_hla::mom::float64_time);
  REQUIRE(reports.timeRegulationEnabledReports.front().value == L"0");

  rti1516_2025::HLAfloat64Interval queriedLookahead;
  REQUIRE_NOTHROW(rti->queryLookahead(queriedLookahead));
  REQUIRE(queriedLookahead.getInterval() == 0.25);

  REQUIRE_NOTHROW(rti->timeAdvanceRequest(rti1516_2025::HLAfloat64Time(1.5)));
  REQUIRE_NOTHROW(rti->queryLogicalTime(queriedTime));
  REQUIRE(queriedTime.getTime() == 0.0);
  REQUIRE(reports.timeAdvanceGrantReports.empty());

  REQUIRE_FALSE(rti->evokeCallback(0.0));
  REQUIRE(reports.timeAdvanceGrantReports.size() == 1U);
  REQUIRE(reports.timeAdvanceGrantReports.front().implementationName == standard_hla::mom::float64_time);
  REQUIRE(reports.timeAdvanceGrantReports.front().value == L"1.5");
  REQUIRE_NOTHROW(rti->queryLogicalTime(queriedTime));
  REQUIRE(queriedTime.getTime() == 1.5);

  REQUIRE_NOTHROW(rti->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(rti->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(rti->disconnect());
}
} // namespace
