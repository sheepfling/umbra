#include "ieee1516_2025_federation_management_fixture_support.hpp"

TEST_CASE(
    "Embedded federation restore restores saved logical time and actual lookahead",
    "[integration][development-profile][federation-management][save-restore]"
    "[time-management][lookahead][modify-lookahead]"
    "[restore-time-window-state]"
    "[rti.service.request-federation-save][rti.service.request-federation-restore]"
    "[rti.service.federate-restore-complete][rti.service.query-logical-time]"
    "[rti.service.query-lookahead][rti.service.modify-lookahead]"
    "[rti.service.time-advance-request]") {
  ReportingFederateAmbassador reports;
  auto rti = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  std::wstring const saveLabel = L"time-window-baseline";
  rti1516_2025::HLAinteger64Time logicalTime;
  rti1516_2025::HLAinteger64Interval lookahead;

  REQUIRE_NOTHROW(rti->connect(reports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      rti->createFederationExecution(federationName, fomModule, standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(rti->joinFederationExecution(
      L"restore-time-window-member", L"publisher", federationName));
  REQUIRE_NOTHROW(rti->enableTimeRegulation(rti1516_2025::HLAinteger64Interval(2)));
  while (rti->evokeCallback(0.0)) {
  }
  REQUIRE_NOTHROW(rti->queryLogicalTime(logicalTime));
  REQUIRE(logicalTime.getTime() == 0);
  REQUIRE_NOTHROW(rti->queryLookahead(lookahead));
  REQUIRE(lookahead.getInterval() == 2);

  REQUIRE_NOTHROW(rti->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(3)));
  while (rti->evokeCallback(0.0)) {
  }
  REQUIRE_NOTHROW(rti->queryLogicalTime(logicalTime));
  REQUIRE(logicalTime.getTime() == 3);

  // The completed image includes the federation's authoritative time
  // coordinator state.  Mutate both parts after saving so restore has to
  // replace, rather than merely retain, the post-save time window.
  REQUIRE_NOTHROW(rti->requestFederationSave(saveLabel));
  while (rti->evokeCallback(0.0)) {
  }
  REQUIRE_NOTHROW(rti->federateSaveBegun());
  REQUIRE_NOTHROW(rti->federateSaveComplete());
  while (rti->evokeCallback(0.0)) {
  }
  REQUIRE(reports.federationSavedReportCount == 1);

  REQUIRE_NOTHROW(rti->modifyLookahead(rti1516_2025::HLAinteger64Interval(5)));
  REQUIRE_NOTHROW(rti->queryLookahead(lookahead));
  REQUIRE(lookahead.getInterval() == 5);
  REQUIRE_NOTHROW(rti->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(5)));
  while (rti->evokeCallback(0.0)) {
  }
  REQUIRE_NOTHROW(rti->queryLogicalTime(logicalTime));
  REQUIRE(logicalTime.getTime() == 5);

  REQUIRE_NOTHROW(rti->requestFederationRestore(saveLabel));
  while (rti->evokeCallback(0.0)) {
  }
  REQUIRE_NOTHROW(rti->federateRestoreComplete());
  while (rti->evokeCallback(0.0)) {
  }
  REQUIRE(reports.federationRestoredReportCount == 1);

  // The post-save time advance and lookahead increase are discarded.  This is
  // a narrow process-local rollback proof, not a claim of timed or durable
  // restore semantics.
  REQUIRE_NOTHROW(rti->queryLogicalTime(logicalTime));
  REQUIRE(logicalTime.getTime() == 3);
  REQUIRE_NOTHROW(rti->queryLookahead(lookahead));
  REQUIRE(lookahead.getInterval() == 2);

  REQUIRE_NOTHROW(rti->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(rti->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(rti->disconnect());
}

TEST_CASE(
    "Embedded federation restore preserves a deferred lookahead decrease",
    "[integration][development-profile][federation-management][save-restore]"
    "[time-management][lookahead][modify-lookahead]"
    "[restore-time-window-state]"
    "[rti.service.request-federation-save][rti.service.request-federation-restore]"
    "[rti.service.federate-restore-complete][rti.service.query-logical-time]"
    "[rti.service.query-lookahead][rti.service.modify-lookahead]"
    "[rti.service.time-advance-request]") {
  ReportingFederateAmbassador reports;
  auto rti = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  std::wstring const saveLabel = L"deferred-lookahead-baseline";
  rti1516_2025::HLAinteger64Time logicalTime;
  rti1516_2025::HLAinteger64Interval lookahead;

  REQUIRE_NOTHROW(rti->connect(reports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      rti->createFederationExecution(federationName, fomModule, standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(rti->joinFederationExecution(
      L"restore-deferred-lookahead-member", L"publisher", federationName));
  REQUIRE_NOTHROW(rti->enableTimeRegulation(rti1516_2025::HLAinteger64Interval(5)));
  while (rti->evokeCallback(0.0)) {
  }

  // Clause 8.20 makes a decreasing request prospective: the actual
  // lookahead is still five before the next grant, while the requested value
  // is private future state that a completed save must retain.
  REQUIRE_NOTHROW(rti->modifyLookahead(rti1516_2025::HLAinteger64Interval(1)));
  REQUIRE_NOTHROW(rti->queryLookahead(lookahead));
  REQUIRE(lookahead.getInterval() == 5);

  REQUIRE_NOTHROW(rti->requestFederationSave(saveLabel));
  while (rti->evokeCallback(0.0)) {
  }
  REQUIRE_NOTHROW(rti->federateSaveBegun());
  REQUIRE_NOTHROW(rti->federateSaveComplete());
  while (rti->evokeCallback(0.0)) {
  }
  REQUIRE(reports.federationSavedReportCount == 1);

  // Consume the post-save decrease. Restore must put both the actual value
  // and its deferred target back, rather than just rewind the current time.
  REQUIRE_NOTHROW(rti->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(3)));
  while (rti->evokeCallback(0.0)) {
  }
  REQUIRE_NOTHROW(rti->queryLogicalTime(logicalTime));
  REQUIRE(logicalTime.getTime() == 3);
  REQUIRE_NOTHROW(rti->queryLookahead(lookahead));
  REQUIRE(lookahead.getInterval() == 2);

  REQUIRE_NOTHROW(rti->requestFederationRestore(saveLabel));
  while (rti->evokeCallback(0.0)) {
  }
  REQUIRE_NOTHROW(rti->federateRestoreComplete());
  while (rti->evokeCallback(0.0)) {
  }
  REQUIRE(reports.federationRestoredReportCount == 1);
  REQUIRE_NOTHROW(rti->queryLogicalTime(logicalTime));
  REQUIRE(logicalTime.getTime() == 0);
  REQUIRE_NOTHROW(rti->queryLookahead(lookahead));
  REQUIRE(lookahead.getInterval() == 5);

  // Advance the restored member through the same elapsed interval.  The
  // second result proves that the saved deferred target, not only the visible
  // actual lookahead, returned with the snapshot.
  REQUIRE_NOTHROW(rti->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(3)));
  while (rti->evokeCallback(0.0)) {
  }
  REQUIRE_NOTHROW(rti->queryLogicalTime(logicalTime));
  REQUIRE(logicalTime.getTime() == 3);
  REQUIRE_NOTHROW(rti->queryLookahead(lookahead));
  REQUIRE(lookahead.getInterval() == 2);

  REQUIRE_NOTHROW(rti->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(rti->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(rti->disconnect());
}

TEST_CASE(
    "Embedded federation restore reschedules a saved pending time advance",
    "[integration][development-profile][federation-management][save-restore]"
    "[time-management][callbacks][time-advance-request]"
    "[restore-pending-time-advance]"
    "[restore-time-window-state]"
    "[rti.service.request-federation-save][rti.service.federate-save-begun]"
    "[rti.service.federate-save-complete][rti.service.request-federation-restore]"
    "[rti.service.federate-restore-complete][rti.service.time-advance-request]"
    "[rti.service.enable-time-constrained]"
    "[rti.service.query-logical-time][federate.callback.initiate-federate-save]"
    "[federate.callback.time-constrained-enabled][federate.callback.time-advance-grant]"
    "[federate.callback.federation-saved]"
    "[federate.callback.federation-restored]") {
  ReportingFederateAmbassador reports;
  auto rti = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
                          "cpp" /
                          "tests" /
                          "data" /
                          "switch-nrg-enabled-fom.xml")
                             .wstring();
  std::wstring const saveLabel = L"pending-time-advance-baseline";
  rti1516_2025::HLAinteger64Time logicalTime;

  REQUIRE_NOTHROW(rti->connect(reports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      rti->createFederationExecution(federationName, fomModule, standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(rti->joinFederationExecution(
      L"restore-pending-time-member", L"time-managed", federationName));
  REQUIRE_NOTHROW(rti->enableTimeConstrained());
  REQUIRE(reports.timeConstrainedEnabledReports.empty());
  while (rti->evokeCallback(0.0)) {
  }
  REQUIRE(reports.timeConstrainedEnabledReports.size() == 1U);

  // With the FOM's Non-Regulated Grant switch enabled, this one constrained
  // member reaches Initiate Federate Save directly before its Time Advance
  // Grant. Completing the save inside that callback captures an execution
  // image whose private time state is still pending.
  reports.callbackOrder.clear();
  reports.onInitiateFederateSave = [&] {
    rti->federateSaveBegun();
    rti->federateSaveComplete();
  };
  REQUIRE_NOTHROW(rti->requestFederationSave(saveLabel));
  REQUIRE(reports.initiateFederateSaveReports.empty());
  REQUIRE_NOTHROW(rti->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(5)));
  REQUIRE_NOTHROW(static_cast<void>(rti->evokeCallback(0.0)));
  REQUIRE(reports.initiateFederateSaveReports == std::vector<std::wstring>{saveLabel});
  REQUIRE(reports.timeAdvanceGrantReports.size() == 1);
  REQUIRE(reports.timeAdvanceGrantReports.back().value == L"5");
  while (rti->evokeCallback(0.0)) {
  }
  REQUIRE(reports.federationSavedReportCount == 1);

  // Advance after the completed save so a later restore must replace both the
  // post-save time window and the completed callback work with the saved,
  // still-pending request.
  reports.onInitiateFederateSave = {};
  REQUIRE_NOTHROW(rti->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(9)));
  while (rti->evokeCallback(0.0)) {
  }
  REQUIRE(reports.timeAdvanceGrantReports.size() == 2);
  REQUIRE(reports.timeAdvanceGrantReports.back().value == L"9");
  REQUIRE_NOTHROW(rti->queryLogicalTime(logicalTime));
  REQUIRE(logicalTime.getTime() == 9);

  reports.callbackOrder.clear();
  REQUIRE_NOTHROW(rti->requestFederationRestore(saveLabel));
  while (rti->evokeCallback(0.0)) {
  }
  REQUIRE_NOTHROW(rti->federateRestoreComplete());

  // Before the reconstructed callback is delivered, the restored time state
  // is intentionally still Time Advancing at the saved current time.
  REQUIRE_NOTHROW(rti->queryLogicalTime(logicalTime));
  REQUIRE(logicalTime.getTime() == 0);
  REQUIRE_THROWS_AS(
      rti->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(6)),
      rti1516_2025::InTimeAdvancingState);

  // The completion callback is submitted before the fresh grant dispatch;
  // the original pre-restore work cannot consume the restored request. The
  // earlier restore-control callbacks may still be queued under HLA_EVOKED,
  // so assert the complete lifecycle order rather than assuming their delivery
  // happened while requestFederationRestore was on the stack.
  REQUIRE_NOTHROW(static_cast<void>(rti->evokeCallback(0.0)));
  REQUIRE(reports.federationRestoredReportCount == 1);
  REQUIRE(reports.timeAdvanceGrantReports.size() == 2);
  REQUIRE_NOTHROW(static_cast<void>(rti->evokeCallback(0.0)));
  REQUIRE(reports.timeAdvanceGrantReports.size() == 3);
  REQUIRE(reports.timeAdvanceGrantReports.back().value == L"5");
  REQUIRE(reports.callbackOrder ==
          std::vector<std::string>{"restore-request-succeeded",
                                   "restore-begun",
                                   "restore-initiate",
                                   "restore-complete",
                                   "grant"});
  REQUIRE_NOTHROW(rti->queryLogicalTime(logicalTime));
  REQUIRE(logicalTime.getTime() == 5);

  REQUIRE_NOTHROW(rti->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(rti->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(rti->disconnect());
}

TEST_CASE(
    "Embedded immediate federation restore reschedules a saved pending time advance",
    "[integration][development-profile][federation-management][save-restore]"
    "[time-management][callbacks][callback-immediate]"
    "[restore-time-window-state]"
    "[rti.service.request-federation-save][rti.service.federate-save-begun]"
    "[rti.service.federate-save-complete][rti.service.request-federation-restore]"
    "[rti.service.federate-restore-complete][rti.service.time-advance-request]"
    "[rti.service.enable-time-constrained]"
    "[rti.service.query-logical-time][federate.callback.initiate-federate-save]"
    "[federate.callback.time-constrained-enabled][federate.callback.time-advance-grant]"
    "[federate.callback.federation-saved]"
    "[federate.callback.federation-restored]") {
  ReportingFederateAmbassador reports;
  auto rti = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
                          "cpp" /
                          "tests" /
                          "data" /
                          "switch-nrg-enabled-fom.xml")
                             .wstring();
  std::wstring const saveLabel = L"pending-time-advance-immediate";
  rti1516_2025::HLAinteger64Time logicalTime;

  REQUIRE_NOTHROW(rti->connect(reports, rti1516_2025::HLA_IMMEDIATE));
  REQUIRE_NOTHROW(
      rti->createFederationExecution(federationName, fomModule, standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(rti->joinFederationExecution(
      L"restore-pending-time-immediate-member", L"time-managed", federationName));
  REQUIRE_NOTHROW(rti->enableTimeConstrained());
  REQUIRE(reports.timeConstrainedEnabledReports.size() == 1U);

  // This uses the same NRG save boundary as the evoked regression, but each
  // callback occurs on the originating service stack.  Save completion is
  // deliberately re-entrant from Initiate Federate Save, while the private
  // time state is still Time Advancing; the subsequent direct Time Advance
  // Grant must not prevent that state from being captured.
  reports.callbackOrder.clear();
  reports.onInitiateFederateSave = [&] {
    rti->federateSaveBegun();
    rti->federateSaveComplete();
  };
  REQUIRE_NOTHROW(rti->requestFederationSave(saveLabel));
  REQUIRE_NOTHROW(rti->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(5)));
  REQUIRE(reports.initiateFederateSaveReports == std::vector<std::wstring>{saveLabel});
  REQUIRE(reports.federationSavedReportCount == 1U);
  REQUIRE(reports.timeAdvanceGrantReports.size() == 1U);
  REQUIRE(reports.timeAdvanceGrantReports.back().value == L"5");
  REQUIRE(reports.callbackOrder ==
          std::vector<std::string>{"save-initiate", "save-complete", "grant"});

  reports.onInitiateFederateSave = {};
  REQUIRE_NOTHROW(rti->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(9)));
  REQUIRE(reports.timeAdvanceGrantReports.size() == 2U);
  REQUIRE(reports.timeAdvanceGrantReports.back().value == L"9");
  REQUIRE_NOTHROW(rti->queryLogicalTime(logicalTime));
  REQUIRE(logicalTime.getTime() == 9);

  reports.callbackOrder.clear();
  REQUIRE_NOTHROW(rti->requestFederationRestore(saveLabel));
  REQUIRE_NOTHROW(rti->federateRestoreComplete());

  // HLA_IMMEDIATE has no Evoke boundary after the completion service.  The
  // restore notification must still precede the fresh callback-gated grant;
  // an old execution's captured callback cannot consume the restored state.
  REQUIRE(reports.federationRestoredReportCount == 1U);
  REQUIRE(reports.timeAdvanceGrantReports.size() == 3U);
  REQUIRE(reports.timeAdvanceGrantReports.back().value == L"5");
  REQUIRE(reports.callbackOrder ==
          std::vector<std::string>{"restore-request-succeeded",
                                   "restore-begun",
                                   "restore-initiate",
                                   "restore-complete",
                                   "grant"});
  REQUIRE_NOTHROW(rti->queryLogicalTime(logicalTime));
  REQUIRE(logicalTime.getTime() == 5);

  // The direct restored grant releases the time-advancing state before the
  // completion service returns, so the next request can be accepted normally.
  REQUIRE_NOTHROW(rti->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(6)));
  REQUIRE(reports.timeAdvanceGrantReports.size() == 4U);
  REQUIRE(reports.timeAdvanceGrantReports.back().value == L"6");

  REQUIRE_NOTHROW(rti->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(rti->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(rti->disconnect());
}
