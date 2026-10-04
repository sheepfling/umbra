#include "ieee1516_2025_federation_management_fixture_support.hpp"

namespace {
TEST_CASE(
    "Embedded federation save cancels when a member resigns from Initiate Federate Save",
    "[integration][development-profile][federation-management][save-restore][callbacks]"
    "[multi-federate-callback-ordering][active-callback-resignation]"
    "[save-active-callback-resignation-guard]"
    "[rti.service.request-federation-save][rti.service.resign-federation-execution]"
    "[rti.service.evoke-callback][rti.service.federate-save-begun]"
    "[rti.service.federate-save-complete]"
    "[federate.callback.initiate-federate-save]"
    "[federate.callback.federation-not-saved]"
    "[federate.callback.federation-saved]") {
  ReportingFederateAmbassador ownerReports;
  ReportingFederateAmbassador heldReports;
  ReportingFederateAmbassador departingReports;
  auto owner = makeRti();
  auto held = makeRti();
  auto departing = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  std::wstring const saveLabel = L"active-callback-resignation-save";

  REQUIRE_NOTHROW(owner->connect(ownerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(held->connect(heldReports, HLA_EVOKED));
  REQUIRE_NOTHROW(departing->connect(departingReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      owner->createFederationExecution(federationName, fomModule, standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(owner->joinFederationExecution(
      L"active-resignation-owner", L"owner", federationName));
  REQUIRE_NOTHROW(held->joinFederationExecution(
      L"active-resignation-held", L"held", federationName));
  REQUIRE_NOTHROW(departing->joinFederationExecution(
      L"active-resignation-departing", L"departing", federationName));

  heldReports.onInitiateFederateSave = [&] { held->federateSaveBegun(); };
  bool resignationRejectedFromActiveCallback = false;
  departingReports.onInitiateFederateSave = [&] {
    departing->federateSaveBegun();
    try {
      departing->resignFederationExecution(NO_ACTION);
    } catch (rti1516_2025::CallNotAllowedFromWithinCallback const&) {
      resignationRejectedFromActiveCallback = true;
    }
  };

  REQUIRE_NOTHROW(owner->requestFederationSave(saveLabel));

  // Deliver one participant's Initiate Federate Save first so its begun state
  // is active when the second participant resigns from its own callback.
  REQUIRE_FALSE(held->evokeCallback(0.0));
  REQUIRE(heldReports.callbackOrder == std::vector<std::string>{"save-initiate"});
  REQUIRE_NOTHROW(departing->evokeCallback(0.0));
  REQUIRE(resignationRejectedFromActiveCallback);
  REQUIRE(departingReports.callbackOrder == std::vector<std::string>{"save-initiate"});

  // Once the callback returns, the same resignation is legal. The in-flight
  // save then fails for the remaining participants, while the departing
  // callback route is removed without receiving a stale failure.
  REQUIRE_NOTHROW(departing->resignFederationExecution(NO_ACTION));
  while (owner->evokeCallback(0.0)) {
  }
  while (held->evokeCallback(0.0)) {
  }
  REQUIRE(ownerReports.federationNotSavedReasons ==
          std::vector<SaveFailureReason>{rti1516_2025::FEDERATE_RESIGNED_DURING_SAVE});
  REQUIRE(heldReports.federationNotSavedReasons ==
          std::vector<SaveFailureReason>{rti1516_2025::FEDERATE_RESIGNED_DURING_SAVE});
  REQUIRE(departingReports.federationNotSavedReasons.empty());
  REQUIRE(ownerReports.callbackOrder ==
          std::vector<std::string>{"save-initiate", "save-failed"});
  REQUIRE(heldReports.callbackOrder ==
          std::vector<std::string>{"save-initiate", "save-failed"});

  // Failure cleanup must leave the surviving members able to start a fresh
  // save rather than retaining the canceled operation's callback state.
  heldReports.onInitiateFederateSave = {};
  ownerReports.callbackOrder.clear();
  heldReports.callbackOrder.clear();
  REQUIRE_NOTHROW(owner->requestFederationSave(L"post-active-resignation-save"));
  while (owner->evokeCallback(0.0)) {
  }
  while (held->evokeCallback(0.0)) {
  }
  REQUIRE(ownerReports.initiateFederateSaveReports ==
          std::vector<std::wstring>{saveLabel, L"post-active-resignation-save"});
  REQUIRE(heldReports.initiateFederateSaveReports ==
          std::vector<std::wstring>{saveLabel, L"post-active-resignation-save"});
  REQUIRE_NOTHROW(owner->federateSaveBegun());
  REQUIRE_NOTHROW(held->federateSaveBegun());
  REQUIRE_NOTHROW(owner->federateSaveComplete());
  REQUIRE_NOTHROW(held->federateSaveComplete());
  while (owner->evokeCallback(0.0)) {
  }
  while (held->evokeCallback(0.0)) {
  }
  REQUIRE(ownerReports.federationSavedReportCount == 1U);
  REQUIRE(heldReports.federationSavedReportCount == 1U);

  REQUIRE_NOTHROW(held->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(departing->disconnect());
  REQUIRE_NOTHROW(held->disconnect());
  REQUIRE_NOTHROW(owner->disconnect());
}
} // namespace
