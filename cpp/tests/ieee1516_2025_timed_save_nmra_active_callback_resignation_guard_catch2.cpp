#include "ieee1516_2025_federation_management_fixture_support.hpp"

namespace {
TEST_CASE(
    "Embedded timed federation save guards resignation from an NMRA callback",
    "[integration][development-profile][federation-management][save-restore][time-management]"
    "[timed-save][alternate-advance][multi-federate-callback-ordering]"
    "[active-callback-resignation][callbacks]"
    "[timed-save-nmra-active-callback-resignation-guard]"
    "[rti.service.request-federation-save][rti.service.resign-federation-execution]"
    "[rti.service.evoke-callback][rti.service.time-advance-request]"
    "[rti.service.next-message-request-available]"
    "[rti.service.enable-time-constrained][rti.service.enable-time-regulation]"
    "[rti.service.federate-save-begun][rti.service.federate-save-complete]"
    "[federate.callback.initiate-federate-save][federate.callback.time-advance-grant]"
    "[federate.callback.federation-saved]") {
  ReportingFederateAmbassador ownerReports;
  ReportingFederateAmbassador nmraReports;
  auto owner = makeRti();
  auto nmra = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
                          "cpp" /
                          "tests" /
                          "data" /
                          "parameter-handle-provider-fom.xml")
                             .wstring();
  std::wstring const saveLabel = L"timed-nmra-active-callback-resignation-save";
  bool resignationRejectedFromActiveCallback = false;

  REQUIRE_NOTHROW(owner->connect(ownerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(nmra->connect(nmraReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      owner->createFederationExecution(federationName, fomModule, standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(owner->joinFederationExecution(
      L"timed-nmra-active-resignation-owner", L"regulator", federationName));
  REQUIRE_NOTHROW(nmra->joinFederationExecution(
      L"timed-nmra-active-resignation-nmra", L"time-constrained", federationName));
  REQUIRE_NOTHROW(nmra->enableTimeConstrained());
  REQUIRE_FALSE(nmra->evokeCallback(0.0));
  REQUIRE_NOTHROW(owner->enableTimeRegulation(rti1516_2025::HLAinteger64Interval(1)));
  REQUIRE_FALSE(owner->evokeCallback(0.0));

  nmraReports.onInitiateFederateSave = [&] {
    try {
      nmra->resignFederationExecution(NO_ACTION);
    } catch (rti1516_2025::CallNotAllowedFromWithinCallback const&) {
      resignationRejectedFromActiveCallback = true;
    }
  };

  REQUIRE_NOTHROW(owner->requestFederationSave(
      saveLabel,
      rti1516_2025::HLAinteger64Time(5)));
  REQUIRE_NOTHROW(nmra->nextMessageRequestAvailable(
      rti1516_2025::HLAinteger64Time(6)));
  REQUIRE_NOTHROW(owner->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(5)));

  // The regulator's advance establishes the save frontier. The strict NMRA
  // boundary then reaches Initiate Federate Save before its grant.
  REQUIRE_FALSE(owner->evokeCallback(0.0));
  REQUIRE(ownerReports.timeAdvanceGrantReports.size() == 1U);
  REQUIRE(ownerReports.timeAdvanceGrantReports.back().value == L"5");
  REQUIRE(ownerReports.initiateFederateSaveReports.empty());
  REQUIRE_FALSE(nmra->evokeCallback(0.0));
  REQUIRE(resignationRejectedFromActiveCallback);
  REQUIRE(nmraReports.initiateFederateSaveReports ==
          std::vector<std::wstring>{saveLabel});
  REQUIRE(nmraReports.timeAdvanceGrantReports.size() == 1U);
  REQUIRE(nmraReports.timeAdvanceGrantReports.back().value == L"6");
  REQUIRE(nmraReports.callbackOrder ==
          std::vector<std::string>{"save-initiate", "grant"});

  // The rejected callback-time call must not alter the timed save. The owner
  // receives its initiation, both members complete, and the save succeeds.
  REQUIRE_FALSE(owner->evokeCallback(0.0));
  REQUIRE(ownerReports.initiateFederateSaveReports ==
          std::vector<std::wstring>{saveLabel});
  REQUIRE(ownerReports.callbackOrder ==
          std::vector<std::string>{"grant", "save-initiate"});
  REQUIRE_NOTHROW(owner->federateSaveBegun());
  REQUIRE_NOTHROW(nmra->federateSaveBegun());
  REQUIRE_NOTHROW(owner->federateSaveComplete());
  REQUIRE_NOTHROW(nmra->federateSaveComplete());
  while (owner->evokeCallback(0.0)) {
  }
  while (nmra->evokeCallback(0.0)) {
  }
  REQUIRE(ownerReports.federationSavedReportCount == 1U);
  REQUIRE(nmraReports.federationSavedReportCount == 1U);
  REQUIRE(ownerReports.federationNotSavedReasons.empty());
  REQUIRE(nmraReports.federationNotSavedReasons.empty());

  REQUIRE_NOTHROW(nmra->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(nmra->disconnect());
  REQUIRE_NOTHROW(owner->disconnect());
}
} // namespace
