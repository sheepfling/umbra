#include "ieee1516_2025_federation_management_test_support.hpp"
#include "ieee1516_2025_federation_management_fixture_support.hpp"

namespace {
TEST_CASE(
    "Embedded additional FOM NRG metadata cannot change the static switch or TAR",
    "[integration][development-profile][time-management][fom][non-regulated-grant][callbacks]"
    "[rti.service.join-federation-execution][rti.service.time-advance-request]"
    "[federate.callback.time-advance-grant]") {
  auto const nrgFom = nrgEnabledRestaurantModule();
  ReportingFederateAmbassador receiverReports;
  ReportingFederateAmbassador applicantReports;
  auto receiver = makeRti();
  auto applicant = makeRti();
  auto const federationName = nextFederationName();
  auto const baseFom = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();

  REQUIRE_NOTHROW(receiver->connect(receiverReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      receiver->createFederationExecution(federationName, baseFom, standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(receiver->joinFederationExecution(L"receiver", federationName));
  REQUIRE_NOTHROW(receiver->enableTimeConstrained());
  REQUIRE_FALSE(receiver->evokeCallback(0.0));

  // The official Restaurant FOM omits NRG, which means Disabled. With no
  // regulator, the constrained TAR must remain pending before the additional
  // module joins.
  REQUIRE_NOTHROW(receiver->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(1)));
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE(receiverReports.timeAdvanceGrantReports.empty());

  REQUIRE_NOTHROW(applicant->connect(applicantReports, HLA_EVOKED));
  REQUIRE_NOTHROW(applicant->joinFederationExecution(
      L"metadata-applicant",
      L"observer",
      federationName,
      std::vector<std::wstring>{nrgFom.path().wstring()}));

  // NRG is a static federation-wide switch established at creation. The
  // additional module cannot change it or release the existing TAR.
  REQUIRE_FALSE(receiver->getNonRegulatedGrantSwitch());
  REQUIRE_FALSE(applicant->getNonRegulatedGrantSwitch());
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE(receiverReports.timeAdvanceGrantReports.empty());

  REQUIRE_NOTHROW(applicant->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(receiver->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(receiver->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(applicant->disconnect());
  REQUIRE_NOTHROW(receiver->disconnect());
}
}  // namespace
