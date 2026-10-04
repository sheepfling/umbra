#include "ieee1516_2025_federation_management_fixture_support.hpp"

namespace {
TEST_CASE(
    "Embedded timed federation save guards resignation from Initiate Federate Save callback",
    "[integration][development-profile][federation-management][save-restore][time-management]"
    "[timed-save][multi-federate-callback-ordering][active-callback-resignation]"
    "[timed-save-active-callback-resignation-guard]"
    "[rti.service.request-federation-save][rti.service.resign-federation-execution]"
    "[rti.service.time-advance-request][rti.service.enable-time-constrained]"
    "[rti.service.enable-time-regulation][rti.service.federate-save-begun]"
    "[rti.service.federate-save-complete][federate.callback.initiate-federate-save]"
    "[federate.callback.time-advance-grant][federate.callback.federation-saved]") {
  ReportingFederateAmbassador ownerReports;
  ReportingFederateAmbassador constrainedReports;
  auto owner = makeRti();
  auto constrained = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
                          "cpp" /
                          "tests" /
                          "data" /
                          "parameter-handle-provider-fom.xml")
                             .wstring();
  std::wstring const saveLabel = L"timed-active-callback-resignation-save";
  bool resignationRejectedFromActiveCallback = false;

  REQUIRE_NOTHROW(owner->connect(ownerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(constrained->connect(constrainedReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      owner->createFederationExecution(federationName, fomModule, standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(owner->joinFederationExecution(
      L"timed-active-resignation-owner", L"regulator", federationName));
  REQUIRE_NOTHROW(constrained->joinFederationExecution(
      L"timed-active-resignation-constrained", L"time-constrained", federationName));
  REQUIRE_NOTHROW(constrained->enableTimeConstrained());
  REQUIRE_FALSE(constrained->evokeCallback(0.0));
  REQUIRE_NOTHROW(owner->enableTimeRegulation(rti1516_2025::HLAinteger64Interval(1)));
  REQUIRE_FALSE(owner->evokeCallback(0.0));

  constrainedReports.onInitiateFederateSave = [&] {
    try {
      constrained->resignFederationExecution(NO_ACTION);
    } catch (rti1516_2025::CallNotAllowedFromWithinCallback const&) {
      resignationRejectedFromActiveCallback = true;
    }
  };

  REQUIRE_NOTHROW(owner->requestFederationSave(
      saveLabel,
      rti1516_2025::HLAinteger64Time(5)));
  REQUIRE_NOTHROW(constrained->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(5)));
  REQUIRE_NOTHROW(owner->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(5)));

  // The regulator's grant is queued first; the constrained route then reaches
  // the timed save's direct pre-grant callback while still Time Advancing.
  REQUIRE_FALSE(owner->evokeCallback(0.0));
  REQUIRE(ownerReports.timeAdvanceGrantReports.size() == 1U);
  REQUIRE(ownerReports.timeAdvanceGrantReports.back().value == L"5");
  REQUIRE(ownerReports.initiateFederateSaveReports.empty());
  REQUIRE_FALSE(constrained->evokeCallback(0.0));
  REQUIRE(resignationRejectedFromActiveCallback);
  REQUIRE(constrainedReports.initiateFederateSaveReports ==
          std::vector<std::wstring>{saveLabel});
  REQUIRE(constrainedReports.timeAdvanceGrantReports.size() == 1U);
  REQUIRE(constrainedReports.timeAdvanceGrantReports.back().value == L"5");
  REQUIRE(constrainedReports.callbackOrder ==
          std::vector<std::string>{"save-initiate", "grant"});

  // The rejected callback-time call must not alter the timed save. The owner
  // receives its initiation, both members complete, and the save succeeds.
  REQUIRE_FALSE(owner->evokeCallback(0.0));
  REQUIRE(ownerReports.initiateFederateSaveReports ==
          std::vector<std::wstring>{saveLabel});
  REQUIRE(ownerReports.callbackOrder ==
          std::vector<std::string>{"grant", "save-initiate"});
  REQUIRE_NOTHROW(owner->federateSaveBegun());
  REQUIRE_NOTHROW(constrained->federateSaveBegun());
  REQUIRE_NOTHROW(owner->federateSaveComplete());
  REQUIRE_NOTHROW(constrained->federateSaveComplete());
  while (owner->evokeCallback(0.0)) {
  }
  while (constrained->evokeCallback(0.0)) {
  }
  REQUIRE(ownerReports.federationSavedReportCount == 1U);
  REQUIRE(constrainedReports.federationSavedReportCount == 1U);
  REQUIRE(ownerReports.federationNotSavedReasons.empty());
  REQUIRE(constrainedReports.federationNotSavedReasons.empty());

  REQUIRE_NOTHROW(constrained->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(constrained->disconnect());
  REQUIRE_NOTHROW(owner->disconnect());
}
} // namespace
