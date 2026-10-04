#include "ieee1516_2025_federation_management_fixture_support.hpp"

namespace {
TEST_CASE(
    "Embedded timed federation save guards resignation from an FQR callback",
    "[integration][development-profile][federation-management][save-restore][time-management]"
    "[timed-save][multi-federate-callback-ordering][active-callback-resignation]"
    "[timed-save-fqr-active-callback-resignation-guard]"
    "[rti.service.request-federation-save][rti.service.resign-federation-execution]"
    "[rti.service.time-advance-request][rti.service.flush-queue-request]"
    "[rti.service.enable-time-constrained][rti.service.enable-time-regulation]"
    "[rti.service.federate-save-begun][rti.service.federate-save-complete]"
    "[federate.callback.initiate-federate-save][federate.callback.flush-queue-grant]"
    "[federate.callback.federation-saved]") {
  ReportingFederateAmbassador ownerReports;
  ReportingFederateAmbassador fqrReports;
  auto owner = makeRti();
  auto fqr = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
                          "cpp" /
                          "tests" /
                          "data" /
                          "parameter-handle-provider-fom.xml")
                             .wstring();
  std::wstring const saveLabel = L"timed-fqr-active-callback-resignation-save";
  bool resignationRejectedFromActiveCallback = false;

  REQUIRE_NOTHROW(owner->connect(ownerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(fqr->connect(fqrReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      owner->createFederationExecution(federationName, fomModule, standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(owner->joinFederationExecution(
      L"timed-fqr-active-resignation-owner", L"regulator", federationName));
  REQUIRE_NOTHROW(fqr->joinFederationExecution(
      L"timed-fqr-active-resignation-fqr", L"time-constrained", federationName));
  REQUIRE_NOTHROW(fqr->enableTimeConstrained());
  REQUIRE_FALSE(fqr->evokeCallback(0.0));
  REQUIRE_NOTHROW(owner->enableTimeRegulation(rti1516_2025::HLAinteger64Interval(1)));
  REQUIRE_FALSE(owner->evokeCallback(0.0));

  fqrReports.onInitiateFederateSave = [&] {
    try {
      fqr->resignFederationExecution(NO_ACTION);
    } catch (rti1516_2025::CallNotAllowedFromWithinCallback const&) {
      resignationRejectedFromActiveCallback = true;
    }
  };

  REQUIRE_NOTHROW(owner->requestFederationSave(
      saveLabel,
      rti1516_2025::HLAinteger64Time(5)));
  REQUIRE_NOTHROW(fqr->flushQueueRequest(rti1516_2025::HLAinteger64Time(6)));
  REQUIRE_NOTHROW(owner->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(5)));

  // The regulator's advance establishes the save frontier. The strict FQR
  // grant then reaches Initiate Federate Save before its Flush Queue Grant.
  REQUIRE_FALSE(owner->evokeCallback(0.0));
  REQUIRE(ownerReports.timeAdvanceGrantReports.size() == 1U);
  REQUIRE(ownerReports.timeAdvanceGrantReports.back().value == L"5");
  REQUIRE(ownerReports.initiateFederateSaveReports.empty());
  REQUIRE_FALSE(fqr->evokeCallback(0.0));
  REQUIRE(resignationRejectedFromActiveCallback);
  REQUIRE(fqrReports.initiateFederateSaveReports ==
          std::vector<std::wstring>{saveLabel});
  REQUIRE(fqrReports.flushQueueGrantReports.size() == 1U);
  REQUIRE(fqrReports.flushQueueGrantReports.back().value == L"6");
  REQUIRE(fqrReports.callbackOrder ==
          std::vector<std::string>{"save-initiate", "flush-grant"});

  REQUIRE_FALSE(owner->evokeCallback(0.0));
  REQUIRE(ownerReports.initiateFederateSaveReports ==
          std::vector<std::wstring>{saveLabel});
  REQUIRE(ownerReports.callbackOrder ==
          std::vector<std::string>{"grant", "save-initiate"});
  REQUIRE_NOTHROW(owner->federateSaveBegun());
  REQUIRE_NOTHROW(fqr->federateSaveBegun());
  REQUIRE_NOTHROW(owner->federateSaveComplete());
  REQUIRE_NOTHROW(fqr->federateSaveComplete());
  while (owner->evokeCallback(0.0)) {
  }
  while (fqr->evokeCallback(0.0)) {
  }
  REQUIRE(ownerReports.federationSavedReportCount == 1U);
  REQUIRE(fqrReports.federationSavedReportCount == 1U);
  REQUIRE(ownerReports.federationNotSavedReasons.empty());
  REQUIRE(fqrReports.federationNotSavedReasons.empty());

  REQUIRE_NOTHROW(fqr->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(fqr->disconnect());
  REQUIRE_NOTHROW(owner->disconnect());
}
} // namespace
