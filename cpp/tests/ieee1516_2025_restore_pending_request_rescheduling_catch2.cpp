#include "ieee1516_2025_federation_management_fixture_support.hpp"

TEST_CASE(
    "Embedded federation restore reschedules saved pending time advances for multiple constrained members",
    "[integration][development-profile][federation-management][save-restore]"
    "[restore-pending-request-rescheduling]"
    "[time-management][callbacks][multi-federate-callback-ordering]"
    "[rti.service.request-federation-save][rti.service.federate-save-begun]"
    "[rti.service.federate-save-complete][rti.service.request-federation-restore]"
    "[rti.service.federate-restore-complete][rti.service.time-advance-request]"
    "[rti.service.enable-time-constrained]"
    "[federate.callback.initiate-federate-save][federate.callback.time-advance-grant]"
    "[federate.callback.federation-saved][federate.callback.federation-restored]") {
  ReportingFederateAmbassador firstReports;
  ReportingFederateAmbassador secondReports;
  auto first = makeRti();
  auto second = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
                          "cpp" /
                          "tests" /
                          "data" /
                          "switch-nrg-enabled-fom.xml")
                             .wstring();
  std::wstring const saveLabel = L"multi-member-pending-time-advance";
  rti1516_2025::HLAinteger64Time firstLogicalTime;
  rti1516_2025::HLAinteger64Time secondLogicalTime;

  auto const drain = [](RTIambassador& ambassador) {
    while (ambassador.evokeCallback(0.0)) {
    }
  };

  REQUIRE_NOTHROW(first->connect(firstReports, HLA_EVOKED));
  REQUIRE_NOTHROW(second->connect(secondReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      first->createFederationExecution(federationName, fomModule, standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(first->joinFederationExecution(
      L"restore-multi-pending-first", L"time-managed", federationName));
  REQUIRE_NOTHROW(second->joinFederationExecution(
      L"restore-multi-pending-second", L"time-managed", federationName));
  REQUIRE_NOTHROW(first->enableTimeConstrained());
  REQUIRE_NOTHROW(second->enableTimeConstrained());
  drain(*first);
  drain(*second);
  REQUIRE(firstReports.timeConstrainedEnabledReports.size() == 1U);
  REQUIRE(secondReports.timeConstrainedEnabledReports.size() == 1U);

  // The two constrained members must become Time Advancing together before
  // the untimed save is admitted.  Each Initiate Federate Save callback
  // completes its own participation, but the snapshot is not materialized
  // until both callback paths have reported Save Complete.
  firstReports.callbackOrder.clear();
  secondReports.callbackOrder.clear();
  std::mutex saveBoundaryMutex;
  std::condition_variable saveBoundary;
  bool firstSaveComplete = false;
  bool secondSaveComplete = false;
  std::exception_ptr secondGrantThreadException;
  firstReports.onInitiateFederateSave = [&] {
    first->federateSaveBegun();
    first->federateSaveComplete();
    {
      std::scoped_lock lock(saveBoundaryMutex);
      firstSaveComplete = true;
    }
    saveBoundary.notify_all();
    std::unique_lock lock(saveBoundaryMutex);
    saveBoundary.wait_for(
        lock,
        std::chrono::seconds(5),
        [&] { return secondSaveComplete || secondGrantThreadException; });
  };
  secondReports.onInitiateFederateSave = [&] {
    second->federateSaveBegun();
    second->federateSaveComplete();
    {
      std::scoped_lock lock(saveBoundaryMutex);
      secondSaveComplete = true;
    }
    saveBoundary.notify_all();
  };
  REQUIRE_NOTHROW(first->requestFederationSave(saveLabel));
  REQUIRE_NOTHROW(first->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(5)));
  REQUIRE_NOTHROW(second->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(5)));
  // Hold the first grant in its pre-grant Initiate Federate Save callback
  // while a separate evoked route reaches the second callback boundary. The
  // save snapshot is then materialized with both private requests pending,
  // which is the multi-member image this restore proof needs.
  std::thread secondGrantThread([&] {
    {
      std::unique_lock lock(saveBoundaryMutex);
      saveBoundary.wait_for(
          lock,
          std::chrono::seconds(5),
          [&] { return firstSaveComplete || secondGrantThreadException; });
    }
    try {
      static_cast<void>(second->evokeCallback(0.0));
    } catch (...) {
      std::scoped_lock lock(saveBoundaryMutex);
      secondGrantThreadException = std::current_exception();
      saveBoundary.notify_all();
    }
  });
  std::exception_ptr firstGrantThreadException;
  try {
    static_cast<void>(first->evokeCallback(0.0));
  } catch (...) {
    firstGrantThreadException = std::current_exception();
    saveBoundary.notify_all();
  }
  secondGrantThread.join();
  if (firstGrantThreadException) {
    std::rethrow_exception(firstGrantThreadException);
  }
  if (secondGrantThreadException) {
    std::rethrow_exception(secondGrantThreadException);
  }
  REQUIRE(firstReports.initiateFederateSaveReports == std::vector<std::wstring>{saveLabel});
  REQUIRE(secondReports.initiateFederateSaveReports == std::vector<std::wstring>{saveLabel});
  drain(*first);
  drain(*second);
  REQUIRE(firstReports.initiateFederateSaveReports == std::vector<std::wstring>{saveLabel});
  REQUIRE(secondReports.initiateFederateSaveReports == std::vector<std::wstring>{saveLabel});
  REQUIRE(firstReports.federationSavedReportCount == 1U);
  REQUIRE(secondReports.federationSavedReportCount == 1U);
  REQUIRE(firstReports.timeAdvanceGrantReports.size() == 1U);
  REQUIRE(secondReports.timeAdvanceGrantReports.size() == 1U);
  REQUIRE(firstReports.timeAdvanceGrantReports.back().value == L"5");
  REQUIRE(secondReports.timeAdvanceGrantReports.back().value == L"5");

  // Advance both members after the image.  Restore must replace these two
  // post-save grants with two fresh grants from the independently saved
  // pending requests rather than replaying one member's closure twice.
  firstReports.onInitiateFederateSave = {};
  secondReports.onInitiateFederateSave = {};
  REQUIRE_NOTHROW(first->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(9)));
  REQUIRE_NOTHROW(second->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(9)));
  drain(*first);
  drain(*second);
  REQUIRE(firstReports.timeAdvanceGrantReports.size() == 2U);
  REQUIRE(secondReports.timeAdvanceGrantReports.size() == 2U);
  REQUIRE(firstReports.timeAdvanceGrantReports.back().value == L"9");
  REQUIRE(secondReports.timeAdvanceGrantReports.back().value == L"9");
  REQUIRE_NOTHROW(first->queryLogicalTime(firstLogicalTime));
  REQUIRE_NOTHROW(second->queryLogicalTime(secondLogicalTime));
  REQUIRE(firstLogicalTime.getTime() == 9);
  REQUIRE(secondLogicalTime.getTime() == 9);

  firstReports.callbackOrder.clear();
  secondReports.callbackOrder.clear();
  REQUIRE_NOTHROW(first->requestFederationRestore(saveLabel));
  drain(*first);
  drain(*second);
  // Isolate the completion/grant ledger from the restore-initiation callbacks.
  // Completing one member must not release the other member's reconstructed
  // pending request.
  firstReports.callbackOrder.clear();
  secondReports.callbackOrder.clear();
  REQUIRE_NOTHROW(first->federateRestoreComplete());
  REQUIRE_NOTHROW(second->federateRestoreComplete());

  drain(*first);
  REQUIRE(firstReports.federationRestoredReportCount == 1U);
  REQUIRE(firstReports.timeAdvanceGrantReports.size() == 3U);
  REQUIRE(secondReports.federationRestoredReportCount == 0U);
  REQUIRE(secondReports.timeAdvanceGrantReports.size() == 2U);
  REQUIRE(secondReports.callbackOrder.empty());
  drain(*second);
  REQUIRE(secondReports.federationRestoredReportCount == 1U);
  REQUIRE(secondReports.timeAdvanceGrantReports.size() == 3U);

  // Restore completion is submitted before each reconstructed grant.  Drain
  // the two routes independently to prove one restored pending advance does
  // not release or consume the other member's saved request.
  for (int pass = 0; pass < 3; ++pass) {
    drain(*first);
    drain(*second);
  }
  REQUIRE(firstReports.federationRestoredReportCount == 1U);
  REQUIRE(secondReports.federationRestoredReportCount == 1U);
  REQUIRE(firstReports.timeAdvanceGrantReports.size() == 3U);
  REQUIRE(secondReports.timeAdvanceGrantReports.size() == 3U);
  REQUIRE(firstReports.timeAdvanceGrantReports.back().value == L"5");
  REQUIRE(secondReports.timeAdvanceGrantReports.back().value == L"5");
  REQUIRE_NOTHROW(first->queryLogicalTime(firstLogicalTime));
  REQUIRE_NOTHROW(second->queryLogicalTime(secondLogicalTime));
  REQUIRE(firstLogicalTime.getTime() == 5);
  REQUIRE(secondLogicalTime.getTime() == 5);
  REQUIRE(firstReports.callbackOrder.size() >= 2U);
  REQUIRE(secondReports.callbackOrder.size() >= 2U);
  REQUIRE(std::vector<std::string>(
              firstReports.callbackOrder.end() - 2,
              firstReports.callbackOrder.end()) ==
          std::vector<std::string>{"restore-complete", "grant"});
  REQUIRE(std::vector<std::string>(
              secondReports.callbackOrder.end() - 2,
              secondReports.callbackOrder.end()) ==
          std::vector<std::string>{"restore-complete", "grant"});

  REQUIRE_NOTHROW(first->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(second->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(first->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(first->disconnect());
  REQUIRE_NOTHROW(second->disconnect());
}

TEST_CASE(
    "Embedded immediate federation restore reschedules a saved pending Flush Queue Request",
    "[integration][development-profile][federation-management][save-restore]"
    "[restore-pending-request-rescheduling]"
    "[time-management][callbacks][callback-immediate]"
    "[rti.service.request-federation-save][rti.service.federate-save-begun]"
    "[rti.service.federate-save-complete][rti.service.request-federation-restore]"
    "[rti.service.federate-restore-complete][rti.service.flush-queue-request]"
    "[rti.service.time-advance-request][rti.service.enable-time-constrained]"
    "[rti.service.enable-time-regulation][rti.service.query-logical-time]"
    "[federate.callback.initiate-federate-save][federate.callback.flush-queue-grant]"
    "[federate.callback.time-constrained-enabled][federate.callback.time-regulation-enabled]"
    "[federate.callback.time-advance-grant][federate.callback.federation-saved]"
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
  std::wstring const saveLabel = L"pending-flush-queue-immediate";
  rti1516_2025::HLAinteger64Time logicalTime;

  REQUIRE_NOTHROW(rti->connect(reports, rti1516_2025::HLA_IMMEDIATE));
  REQUIRE_NOTHROW(
      rti->createFederationExecution(federationName, fomModule, standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(rti->joinFederationExecution(
      L"restore-pending-flush-immediate-member", L"time-managed", federationName));
  REQUIRE_NOTHROW(rti->enableTimeConstrained());
  REQUIRE(reports.timeConstrainedEnabledReports.size() == 1U);
  REQUIRE_NOTHROW(rti->enableTimeRegulation(rti1516_2025::HLAinteger64Interval(1)));
  REQUIRE(reports.timeRegulationEnabledReports.size() == 1U);

  // The direct Initiate Federate Save runs while the special FQR state is
  // still Time Advancing. Completing the save re-entrantly captures that
  // exact special mode before its direct Flush Queue Grant clears it.
  reports.callbackOrder.clear();
  reports.onInitiateFederateSave = [&] {
    rti->federateSaveBegun();
    rti->federateSaveComplete();
  };
  REQUIRE_NOTHROW(rti->requestFederationSave(
      saveLabel,
      rti1516_2025::HLAinteger64Time(5)));
  REQUIRE_NOTHROW(rti->flushQueueRequest(rti1516_2025::HLAinteger64Time(6)));
  REQUIRE(reports.initiateFederateSaveReports == std::vector<std::wstring>{saveLabel});
  REQUIRE(reports.federationSavedReportCount == 1U);
  REQUIRE(reports.flushQueueGrantReports.size() == 1U);
  REQUIRE(reports.flushQueueGrantReports.back().value == L"6");
  REQUIRE(reports.callbackOrder ==
          std::vector<std::string>{"save-initiate", "save-complete", "flush-grant"});

  // Replace the saved FQR with ordinary post-save work. Restore must discard
  // that work and issue a fresh special FQG from the saved temporal image.
  reports.onInitiateFederateSave = {};
  REQUIRE_NOTHROW(rti->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(9)));
  REQUIRE(reports.timeAdvanceGrantReports.size() == 1U);
  REQUIRE(reports.timeAdvanceGrantReports.back().value == L"9");
  REQUIRE_NOTHROW(rti->queryLogicalTime(logicalTime));
  REQUIRE(logicalTime.getTime() == 9);

  reports.callbackOrder.clear();
  REQUIRE_NOTHROW(rti->requestFederationRestore(saveLabel));
  REQUIRE_NOTHROW(rti->federateRestoreComplete());

  // There is no Evoke boundary in HLA_IMMEDIATE, but the restored notification
  // must remain ahead of the new FQG and the stale pre-restore closure must
  // not consume the saved FQR state.
  REQUIRE(reports.federationRestoredReportCount == 1U);
  REQUIRE(reports.flushQueueGrantReports.size() == 2U);
  REQUIRE(reports.flushQueueGrantReports.back().value == L"6");
  REQUIRE(reports.callbackOrder ==
          std::vector<std::string>{"restore-request-succeeded",
                                   "restore-begun",
                                   "restore-initiate",
                                   "restore-complete",
                                   "flush-grant"});
  REQUIRE_NOTHROW(rti->queryLogicalTime(logicalTime));
  REQUIRE(logicalTime.getTime() == 6);

  // The direct restored FQG has released Time Advancing before Restore
  // Complete returns, so a new FQR is legal and receives a fresh FQG.
  REQUIRE_NOTHROW(rti->flushQueueRequest(rti1516_2025::HLAinteger64Time(7)));
  REQUIRE(reports.flushQueueGrantReports.size() == 3U);
  REQUIRE(reports.flushQueueGrantReports.back().value == L"7");

  REQUIRE_NOTHROW(rti->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(rti->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(rti->disconnect());
}

TEST_CASE(
    "Embedded federation restore reschedules a saved pending Flush Queue Request",
    "[integration][development-profile][federation-management][save-restore]"
    "[restore-pending-request-rescheduling]"
    "[time-management][callbacks][restore-pending-flush-queue-request]"
    "[rti.service.connect][rti.service.create-federation-execution]"
    "[rti.service.join-federation-execution][rti.service.evoke-callback]"
    "[rti.service.resign-federation-execution]"
    "[rti.service.destroy-federation-execution][rti.service.disconnect]"
    "[rti.service.request-federation-save][rti.service.federate-save-begun]"
    "[rti.service.federate-save-complete][rti.service.request-federation-restore]"
    "[rti.service.federate-restore-complete][rti.service.flush-queue-request]"
    "[rti.service.time-advance-request][rti.service.enable-time-constrained]"
    "[rti.service.enable-time-regulation][rti.service.query-logical-time]"
    "[federate.callback.initiate-federate-save][federate.callback.flush-queue-grant]"
    "[federate.callback.time-advance-grant][federate.callback.federation-saved]"
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
  std::wstring const saveLabel = L"pending-flush-queue";
  rti1516_2025::HLAinteger64Time logicalTime;

  REQUIRE_NOTHROW(rti->connect(reports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      rti->createFederationExecution(federationName, fomModule, standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(rti->joinFederationExecution(
      L"restore-pending-flush-member", L"time-managed", federationName));
  REQUIRE_NOTHROW(rti->enableTimeConstrained());
  while (rti->evokeCallback(0.0)) {
  }
  REQUIRE_NOTHROW(rti->enableTimeRegulation(rti1516_2025::HLAinteger64Interval(1)));
  while (rti->evokeCallback(0.0)) {
  }

  // A single member may be both time-constrained and regulating in this
  // bounded profile. Its FQR actual grant is 6, strictly beyond the timed
  // save boundary at 5. Complete the save inside the direct pre-FQG callback
  // so the saved temporal image still contains a Flush Queue Request.
  reports.callbackOrder.clear();
  reports.onInitiateFederateSave = [&] {
    rti->federateSaveBegun();
    rti->federateSaveComplete();
  };
  REQUIRE_NOTHROW(rti->requestFederationSave(
      saveLabel,
      rti1516_2025::HLAinteger64Time(5)));
  REQUIRE_NOTHROW(rti->flushQueueRequest(rti1516_2025::HLAinteger64Time(6)));
  REQUIRE_NOTHROW(static_cast<void>(rti->evokeCallback(0.0)));
  REQUIRE(reports.initiateFederateSaveReports == std::vector<std::wstring>{saveLabel});
  REQUIRE(reports.flushQueueGrantReports.size() == 1U);
  REQUIRE(reports.flushQueueGrantReports.back().value == L"6");
  REQUIRE(reports.callbackOrder == std::vector<std::string>{"save-initiate", "flush-grant"});
  while (rti->evokeCallback(0.0)) {
  }
  REQUIRE(reports.federationSavedReportCount == 1U);

  // Post-save time work is deliberately ordinary TAR work. Restore must
  // discard it and rebuild the saved FQR's special grant branch instead.
  reports.onInitiateFederateSave = {};
  REQUIRE_NOTHROW(rti->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(9)));
  while (rti->evokeCallback(0.0)) {
  }
  REQUIRE(reports.timeAdvanceGrantReports.size() == 1U);
  REQUIRE(reports.timeAdvanceGrantReports.back().value == L"9");
  REQUIRE_NOTHROW(rti->queryLogicalTime(logicalTime));
  REQUIRE(logicalTime.getTime() == 9);

  reports.callbackOrder.clear();
  REQUIRE_NOTHROW(rti->requestFederationRestore(saveLabel));
  while (rti->evokeCallback(0.0)) {
  }
  REQUIRE_NOTHROW(rti->federateRestoreComplete());

  // The reconstructed FQR remains pending at the saved time until its fresh
  // dispatch follows Federation Restored.  A normal advance cannot overwrite
  // the restored special-mode state while that callback is queued.
  REQUIRE_NOTHROW(rti->queryLogicalTime(logicalTime));
  REQUIRE(logicalTime.getTime() == 0);
  REQUIRE_THROWS_AS(
      rti->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(7)),
      rti1516_2025::InTimeAdvancingState);
  REQUIRE_NOTHROW(static_cast<void>(rti->evokeCallback(0.0)));
  REQUIRE(reports.federationRestoredReportCount == 1U);
  REQUIRE(reports.flushQueueGrantReports.size() == 1U);
  REQUIRE_NOTHROW(static_cast<void>(rti->evokeCallback(0.0)));
  REQUIRE(reports.flushQueueGrantReports.size() == 2U);
  REQUIRE(reports.flushQueueGrantReports.back().value == L"6");
  REQUIRE(reports.callbackOrder ==
          std::vector<std::string>{"restore-request-succeeded",
                                   "restore-begun",
                                   "restore-initiate",
                                   "restore-complete",
                                   "flush-grant"});
  REQUIRE_NOTHROW(rti->queryLogicalTime(logicalTime));
  REQUIRE(logicalTime.getTime() == 6);

  REQUIRE_NOTHROW(rti->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(rti->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(rti->disconnect());
}

TEST_CASE(
    "Embedded federation restore reschedules saved pending TARA, NMR, and NMRA requests",
    "[integration][development-profile][federation-management][save-restore]"
    "[restore-pending-request-rescheduling]"
    "[restore-pending-time-advance-requests]"
    "[time-management][callbacks][callback-immediate][time-advance-request]"
    "[time-advance-request-available][next-message-request]"
    "[next-message-request-available]"
    "[rti.service.request-federation-save][rti.service.federate-save-begun]"
    "[rti.service.federate-save-complete][rti.service.request-federation-restore]"
    "[rti.service.federate-restore-complete][rti.service.time-advance-request]"
    "[rti.service.time-advance-request-available][rti.service.next-message-request]"
    "[rti.service.next-message-request-available][rti.service.enable-time-constrained]"
    "[rti.service.query-logical-time][federate.callback.initiate-federate-save]"
    "[federate.callback.time-constrained-enabled][federate.callback.time-advance-grant]"
    "[federate.callback.federation-saved][federate.callback.federation-restored]") {
  auto runScenario = [](std::wstring const& memberName,
                        std::wstring const& saveLabel,
                        auto const callbackModel,
                        auto const& submitAdvance) {
    ReportingFederateAmbassador reports;
    auto rti = makeRti();
    auto const federationName = nextFederationName();
    auto const fomModule = (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
                            "cpp" /
                            "tests" /
                            "data" /
                            "switch-nrg-enabled-fom.xml")
                               .wstring();
    bool const immediate = callbackModel == rti1516_2025::HLA_IMMEDIATE;
    rti1516_2025::HLAinteger64Time logicalTime;

    REQUIRE_NOTHROW(rti->connect(reports, callbackModel));
    REQUIRE_NOTHROW(
        rti->createFederationExecution(federationName, fomModule, standard_hla::mom::integer64_time));
    REQUIRE_NOTHROW(rti->joinFederationExecution(memberName, L"time-managed", federationName));
    REQUIRE_NOTHROW(rti->enableTimeConstrained());
    if (!immediate) {
      REQUIRE_NOTHROW(static_cast<void>(rti->evokeCallback(0.0)));
    }
    REQUIRE(reports.timeConstrainedEnabledReports.size() == 1U);

    // The NRG FOM places each ordinary advance form at the direct untimed
    // save boundary. Saving from Initiate Federate Save captures the exact
    // still-pending mode and generation, before its matching grant occurs.
    reports.callbackOrder.clear();
    reports.onInitiateFederateSave = [&] {
      rti->federateSaveBegun();
      rti->federateSaveComplete();
    };
    REQUIRE_NOTHROW(rti->requestFederationSave(saveLabel));
    REQUIRE(reports.initiateFederateSaveReports.empty());
    REQUIRE_NOTHROW(submitAdvance(*rti, 5));
    if (!immediate) {
      REQUIRE_NOTHROW(static_cast<void>(rti->evokeCallback(0.0)));
    }
    REQUIRE(reports.initiateFederateSaveReports == std::vector<std::wstring>{saveLabel});
    REQUIRE(reports.timeAdvanceGrantReports.size() == 1U);
    REQUIRE(reports.timeAdvanceGrantReports.back().value == L"5");
    if (!immediate) {
      while (rti->evokeCallback(0.0)) {
      }
    }
    REQUIRE(reports.federationSavedReportCount == 1U);

    // Complete equivalent post-save work, then prove restore discards it in
    // favor of a freshly scheduled version of the saved request.
    reports.onInitiateFederateSave = {};
    REQUIRE_NOTHROW(submitAdvance(*rti, 9));
    if (!immediate) {
      while (rti->evokeCallback(0.0)) {
      }
    }
    REQUIRE(reports.timeAdvanceGrantReports.size() == 2U);
    REQUIRE(reports.timeAdvanceGrantReports.back().value == L"9");
    REQUIRE_NOTHROW(rti->queryLogicalTime(logicalTime));
    REQUIRE(logicalTime.getTime() == 9);

    reports.callbackOrder.clear();
    REQUIRE_NOTHROW(rti->requestFederationRestore(saveLabel));
    if (!immediate) {
      while (rti->evokeCallback(0.0)) {
      }
    }

    REQUIRE_NOTHROW(rti->federateRestoreComplete());
    if (!immediate) {
      // HLA_EVOKED exposes the restored Time Advancing state between the
      // completion service and its separately queued callback work. A
      // competing TAR cannot overwrite the restored TARA/NMR/NMRA request.
      REQUIRE_NOTHROW(rti->queryLogicalTime(logicalTime));
      REQUIRE(logicalTime.getTime() == 0);
      REQUIRE_THROWS_AS(
          rti->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(6)),
          rti1516_2025::InTimeAdvancingState);
      REQUIRE_NOTHROW(static_cast<void>(rti->evokeCallback(0.0)));
      REQUIRE(reports.federationRestoredReportCount == 1U);
      REQUIRE(reports.timeAdvanceGrantReports.size() == 2U);
      REQUIRE_NOTHROW(static_cast<void>(rti->evokeCallback(0.0)));
    }
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

    // The same public advance family remains usable after the reconstructed
    // callback releases the saved state.
    REQUIRE_NOTHROW(submitAdvance(*rti, 6));
    if (!immediate) {
      while (rti->evokeCallback(0.0)) {
      }
    }
    REQUIRE(reports.timeAdvanceGrantReports.size() == 4U);
    REQUIRE(reports.timeAdvanceGrantReports.back().value == L"6");

    REQUIRE_NOTHROW(rti->resignFederationExecution(NO_ACTION));
    REQUIRE_NOTHROW(rti->destroyFederationExecution(federationName));
    REQUIRE_NOTHROW(rti->disconnect());
  };

  SECTION("TARA under HLA_EVOKED") {
    runScenario(
        L"restore-pending-tara-evoked",
        L"pending-tara-evoked",
        HLA_EVOKED,
        [](rti1516_2025::RTIambassador& rti, std::int64_t const value) {
          rti.timeAdvanceRequestAvailable(rti1516_2025::HLAinteger64Time(value));
        });
  }
  SECTION("NMR under HLA_EVOKED") {
    runScenario(
        L"restore-pending-nmr-evoked",
        L"pending-nmr-evoked",
        HLA_EVOKED,
        [](rti1516_2025::RTIambassador& rti, std::int64_t const value) {
          rti.nextMessageRequest(rti1516_2025::HLAinteger64Time(value));
        });
  }
  SECTION("NMRA under HLA_EVOKED") {
    runScenario(
        L"restore-pending-nmra-evoked",
        L"pending-nmra-evoked",
        HLA_EVOKED,
        [](rti1516_2025::RTIambassador& rti, std::int64_t const value) {
          rti.nextMessageRequestAvailable(rti1516_2025::HLAinteger64Time(value));
        });
  }
  SECTION("TARA under HLA_IMMEDIATE") {
    runScenario(
        L"restore-pending-tara-immediate",
        L"pending-tara-immediate",
        rti1516_2025::HLA_IMMEDIATE,
        [](rti1516_2025::RTIambassador& rti, std::int64_t const value) {
          rti.timeAdvanceRequestAvailable(rti1516_2025::HLAinteger64Time(value));
        });
  }
  SECTION("NMR under HLA_IMMEDIATE") {
    runScenario(
        L"restore-pending-nmr-immediate",
        L"pending-nmr-immediate",
        rti1516_2025::HLA_IMMEDIATE,
        [](rti1516_2025::RTIambassador& rti, std::int64_t const value) {
          rti.nextMessageRequest(rti1516_2025::HLAinteger64Time(value));
        });
  }
  SECTION("NMRA under HLA_IMMEDIATE") {
    runScenario(
        L"restore-pending-nmra-immediate",
        L"pending-nmra-immediate",
        rti1516_2025::HLA_IMMEDIATE,
        [](rti1516_2025::RTIambassador& rti, std::int64_t const value) {
          rti.nextMessageRequestAvailable(rti1516_2025::HLAinteger64Time(value));
        });
  }
}
