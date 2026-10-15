#include <algorithm>
#include <chrono>
#include <condition_variable>
#include <cstring>
#include <exception>
#include <mutex>
#include <thread>
#include <vector>

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
    "Embedded untimed save initiates a queued Flush Queue Request before grant",
    "[integration][development-profile][federation-management][save-restore]"
    "[time-management][callbacks][untimed-save-fqr-pregrant-admission]"
    "[rti.service.request-federation-save][rti.service.federate-save-begun]"
    "[rti.service.federate-save-complete][rti.service.flush-queue-request]"
    "[rti.service.enable-time-constrained][rti.service.enable-time-regulation]"
    "[rti.service.query-logical-time][federate.callback.initiate-federate-save]"
    "[federate.callback.flush-queue-grant][federate.callback.federation-saved]") {
  auto runScenario = [](CallbackModel callbackModel) {
    ReportingFederateAmbassador reports;
    auto rti = makeRti();
    auto const federationName = nextFederationName();
    auto const fomModule = (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
                            "cpp" / "tests" / "data" / "switch-nrg-enabled-fom.xml")
                               .wstring();
    std::wstring const saveLabel = L"untimed-queued-flush";
    rti1516_2025::HLAinteger64Time logicalTime;

    REQUIRE_NOTHROW(rti->connect(reports, callbackModel));
    REQUIRE_NOTHROW(rti->createFederationExecution(
        federationName, fomModule, standard_hla::mom::integer64_time));
    REQUIRE_NOTHROW(rti->joinFederationExecution(
        L"untimed-queued-flush-member", L"time-managed", federationName));
    REQUIRE_NOTHROW(rti->enableTimeConstrained());
    REQUIRE_NOTHROW(rti->enableTimeRegulation(rti1516_2025::HLAinteger64Interval(1)));
    while (rti->evokeCallback(0.0)) {
    }

    // The ordinary-grant path admits an untimed save itself, but FQG does not.
    // Even though this FQR is already queued for grant, its separate direct
    // initiation must remain before FQG and save the pending request.
    reports.callbackOrder.clear();
    reports.onInitiateFederateSave = [&] {
      rti->federateSaveBegun();
      rti->federateSaveComplete();
    };
    REQUIRE_NOTHROW(rti->requestFederationSave(saveLabel));
    REQUIRE_NOTHROW(rti->flushQueueRequest(rti1516_2025::HLAinteger64Time(6)));
    while (rti->evokeCallback(0.0)) {
    }
    REQUIRE(reports.initiateFederateSaveReports == std::vector<std::wstring>{saveLabel});
    REQUIRE(reports.federationSavedReportCount == 1U);
    REQUIRE(reports.flushQueueGrantReports.size() == 1U);
    REQUIRE(reports.flushQueueGrantReports.front().value == L"6");
    auto const saveCallback = std::find(
        reports.callbackOrder.begin(), reports.callbackOrder.end(), "save-initiate");
    auto const flushGrant = std::find(
        reports.callbackOrder.begin(), reports.callbackOrder.end(), "flush-grant");
    REQUIRE(saveCallback != reports.callbackOrder.end());
    REQUIRE(flushGrant != reports.callbackOrder.end());
    REQUIRE(saveCallback < flushGrant);
    REQUIRE_NOTHROW(rti->queryLogicalTime(logicalTime));
    REQUIRE(logicalTime.getTime() == 6);

    REQUIRE_NOTHROW(rti->resignFederationExecution(NO_ACTION));
    REQUIRE_NOTHROW(rti->destroyFederationExecution(federationName));
    REQUIRE_NOTHROW(rti->disconnect());
  };

  SECTION("HLA_EVOKED") {
    runScenario(HLA_EVOKED);
  }
  SECTION("HLA_IMMEDIATE") {
    runScenario(rti1516_2025::HLA_IMMEDIATE);
  }
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
      // Initiate Federate Save and Time Advance Grant are separate callbacks.
      while (rti->evokeCallback(0.0)) {
      }
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

TEST_CASE(
    "Embedded restore replays a timestamped interaction queued for pending NMR",
    "[integration][development-profile][federation-management][interaction-management]"
    "[save-restore][restore-pending-tso-nmr][time-management][tso]"
    "[next-message-request][multi-federate-callback-ordering]"
    "[rti.service.send-interaction][rti.service.subscribe-interaction-class]"
    "[rti.service.enable-time-regulation][rti.service.enable-time-constrained]"
    "[rti.service.next-message-request][rti.service.time-advance-request]"
    "[rti.service.request-federation-save][rti.service.federate-save-begun]"
    "[rti.service.federate-save-complete][rti.service.request-federation-restore]"
    "[rti.service.federate-restore-complete][rti.service.query-logical-time]"
    "[federate.callback.initiate-federate-save][federate.callback.receive-interaction]"
    "[federate.callback.time-advance-grant][federate.callback.federation-saved]"
    "[federate.callback.federation-restored]") {
  auto runScenario = [](auto const callbackModel) {
    ReportingFederateAmbassador publisherReports;
    ReportingFederateAmbassador independentRegulatorReports;
    ReportingFederateAmbassador receiverReports;
    auto publisher = makeRti();
    auto independentRegulator = makeRti();
    auto receiver = makeRti();
    auto const federationName = nextFederationName();
    auto const fomModule = (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
                            "cpp" /
                            "tests" /
                            "data" /
                            "parameter-handle-provider-fom.xml")
                               .wstring();
    std::wstring const saveLabel = L"pending-nmr-tso-replay";
    rti1516_2025::FederateHandle publisherHandle;
    rti1516_2025::HLAinteger64Time publisherLogicalTime;
    rti1516_2025::HLAinteger64Time independentRegulatorLogicalTime;
    rti1516_2025::HLAinteger64Time receiverLogicalTime;
    unsigned char const identifierBytes[] = {0x4E, 0x4D, 0x52, 0x2D, 0x54, 0x53, 0x4F};
    unsigned char const tagBytes[] = {0x52, 0x45, 0x53, 0x54, 0x4F, 0x52, 0x45};
    rti1516_2025::VariableLengthData const tag(tagBytes, sizeof(tagBytes));

    auto const drain = [](rti1516_2025::RTIambassador& ambassador) {
      while (ambassador.evokeCallback(0.0)) {
      }
    };
    auto const drainAll = [&] {
      bool invokedCallback = false;
      do {
        auto const publisherInvoked = publisher->evokeCallback(0.0);
        auto const regulatorInvoked = independentRegulator->evokeCallback(0.0);
        auto const receiverInvoked = receiver->evokeCallback(0.0);
        invokedCallback = publisherInvoked || regulatorInvoked || receiverInvoked;
      } while (invokedCallback);
    };
    auto const matchesBytes = [](rti1516_2025::VariableLengthData const& value,
                                 unsigned char const* expected,
                                 std::size_t const expectedSize) {
      return value.size() == expectedSize && value.data() != nullptr &&
          std::memcmp(value.data(), expected, expectedSize) == 0;
    };

    REQUIRE_NOTHROW(publisher->connect(publisherReports, callbackModel));
    REQUIRE_NOTHROW(independentRegulator->connect(independentRegulatorReports, callbackModel));
    REQUIRE_NOTHROW(receiver->connect(receiverReports, callbackModel));
    REQUIRE_NOTHROW(publisher->createFederationExecution(
        federationName,
        fomModule,
        standard_hla::mom::integer64_time));
    REQUIRE_NOTHROW(publisherHandle = publisher->joinFederationExecution(
        L"restore-pending-nmr-tso-publisher", L"publisher", federationName));
    REQUIRE_NOTHROW(independentRegulator->joinFederationExecution(
        L"restore-pending-nmr-tso-independent-regulator", L"regulator", federationName));
    REQUIRE_NOTHROW(receiver->joinFederationExecution(
        L"restore-pending-nmr-tso-receiver", L"subscriber", federationName));

    auto const interactionClass = publisher->getInteractionClassHandle(
        fixture_hla::fom::parameter_fixture_child_interaction);
    auto const identifier = publisher->getParameterHandle(
        interactionClass,
        fixture_hla::fixture::identifier);
    rti1516_2025::ParameterHandleValueMap parameterValues;
    parameterValues.emplace(
        identifier,
        rti1516_2025::VariableLengthData(identifierBytes, sizeof(identifierBytes)));
    REQUIRE(interactionClass.isValid());
    REQUIRE(identifier.isValid());
    REQUIRE_NOTHROW(publisher->publishInteractionClass(interactionClass));
    REQUIRE_NOTHROW(publisher->changeInteractionOrderType(interactionClass, TIMESTAMP));
    REQUIRE_NOTHROW(receiver->subscribeInteractionClass(interactionClass));
    REQUIRE_NOTHROW(publisher->enableTimeConstrained());
    REQUIRE_NOTHROW(independentRegulator->enableTimeConstrained());
    REQUIRE_NOTHROW(receiver->enableTimeConstrained());
    if (callbackModel == HLA_EVOKED) {
      drainAll();
    }
    REQUIRE(publisherReports.timeConstrainedEnabledReports.size() == 1U);
    REQUIRE(independentRegulatorReports.timeConstrainedEnabledReports.size() == 1U);
    REQUIRE(receiverReports.timeConstrainedEnabledReports.size() == 1U);
    REQUIRE_NOTHROW(publisher->enableTimeRegulation(
        rti1516_2025::HLAinteger64Interval(1)));
    REQUIRE_NOTHROW(independentRegulator->enableTimeRegulation(
        rti1516_2025::HLAinteger64Interval(1)));
    if (callbackModel == HLA_EVOKED) {
      drainAll();
    }
    REQUIRE(publisherReports.timeRegulationEnabledReports.size() == 1U);
    REQUIRE(independentRegulatorReports.timeRegulationEnabledReports.size() == 1U);

    // Establish NMR(10) first.  The accepted timestamp-5 interaction then
    // retargets its pending grant, but the regulator's current GALT keeps the
    // message queued until all three constrained members reach the save boundary.
    REQUIRE_NOTHROW(receiver->nextMessageRequest(rti1516_2025::HLAinteger64Time(10)));
    REQUIRE(receiverReports.timestampedInteractionReports.empty());
    auto const retraction = publisher->sendInteraction(
        interactionClass,
        parameterValues,
        tag,
        rti1516_2025::HLAinteger64Time(5));
    REQUIRE(retraction.isValid());
    REQUIRE(receiverReports.timestampedInteractionReports.empty());
    REQUIRE(receiverReports.timeAdvanceGrantReports.empty());

    // Saving is requested only after the queued message has been accepted.
    // Initiate Federate Save runs at the pre-grant boundary, before NMR's TSO
    // callback drains the saved queue entry.
    REQUIRE_NOTHROW(receiver->requestFederationSave(saveLabel));
    REQUIRE(publisherReports.initiateFederateSaveReports.empty());
    REQUIRE(receiverReports.initiateFederateSaveReports.empty());
    std::mutex saveBoundaryMutex;
    std::condition_variable saveBoundary;
    std::size_t completedSaveParticipants = 0;
    std::vector<std::exception_ptr> grantThreadExceptions(3U);
    auto const grantThreadFailed = [&] {
      return std::any_of(
          grantThreadExceptions.begin(),
          grantThreadExceptions.end(),
          [](std::exception_ptr const& exception) { return static_cast<bool>(exception); });
    };
    auto const completeSaveAndWaitForFederation = [&](rti1516_2025::RTIambassador& ambassador) {
      ambassador.federateSaveBegun();
      ambassador.federateSaveComplete();
      {
        std::scoped_lock lock(saveBoundaryMutex);
        ++completedSaveParticipants;
      }
      saveBoundary.notify_all();
      if (callbackModel == HLA_EVOKED) {
        std::unique_lock lock(saveBoundaryMutex);
        saveBoundary.wait_for(
            lock,
            std::chrono::seconds(5),
            [&] { return completedSaveParticipants == 3U || grantThreadFailed(); });
      }
    };
    publisherReports.onInitiateFederateSave = [&] {
      completeSaveAndWaitForFederation(*publisher);
    };
    independentRegulatorReports.onInitiateFederateSave = [&] {
      completeSaveAndWaitForFederation(*independentRegulator);
    };
    receiverReports.onInitiateFederateSave = [&] {
      completeSaveAndWaitForFederation(*receiver);
    };
    publisherReports.callbackOrder.clear();
    independentRegulatorReports.callbackOrder.clear();
    receiverReports.callbackOrder.clear();
    if (callbackModel == rti1516_2025::HLA_IMMEDIATE) {
      REQUIRE_NOTHROW(publisher->disableCallbacks());
      REQUIRE_NOTHROW(independentRegulator->disableCallbacks());
      REQUIRE_NOTHROW(receiver->disableCallbacks());
    }
    REQUIRE_NOTHROW(publisher->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(5)));
    REQUIRE_NOTHROW(
        independentRegulator->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(5)));
    if (callbackModel == rti1516_2025::HLA_IMMEDIATE) {
      REQUIRE_NOTHROW(publisher->enableCallbacks());
      REQUIRE_NOTHROW(independentRegulator->enableCallbacks());
      REQUIRE_NOTHROW(receiver->enableCallbacks());
    } else {
      auto const drainOnThread = [&](std::size_t const exceptionIndex,
                                     rti1516_2025::RTIambassador& ambassador) {
        try {
          drain(ambassador);
        } catch (...) {
          {
            std::scoped_lock lock(saveBoundaryMutex);
            grantThreadExceptions[exceptionIndex] = std::current_exception();
          }
          saveBoundary.notify_all();
        }
      };
      std::thread publisherGrantThread([&] { drainOnThread(0U, *publisher); });
      std::thread regulatorGrantThread(
          [&] { drainOnThread(1U, *independentRegulator); });
      std::thread receiverGrantThread([&] { drainOnThread(2U, *receiver); });
      publisherGrantThread.join();
      regulatorGrantThread.join();
      receiverGrantThread.join();
      for (auto const& exception : grantThreadExceptions) {
        if (exception) {
          std::rethrow_exception(exception);
        }
      }
    }

    REQUIRE(publisherReports.initiateFederateSaveReports ==
            std::vector<std::wstring>{saveLabel});
    REQUIRE(independentRegulatorReports.initiateFederateSaveReports ==
            std::vector<std::wstring>{saveLabel});
    REQUIRE(receiverReports.initiateFederateSaveReports ==
            std::vector<std::wstring>{saveLabel});
    REQUIRE(publisherReports.federationSavedReportCount == 1U);
    REQUIRE(independentRegulatorReports.federationSavedReportCount == 1U);
    REQUIRE(receiverReports.federationSavedReportCount == 1U);
    REQUIRE(publisherReports.timeAdvanceGrantReports.size() == 1U);
    REQUIRE(publisherReports.timeAdvanceGrantReports.back().value == L"5");
    REQUIRE(independentRegulatorReports.timeAdvanceGrantReports.size() == 1U);
    REQUIRE(independentRegulatorReports.timeAdvanceGrantReports.back().value == L"5");
    REQUIRE(receiverReports.timestampedInteractionReports.size() == 1U);
    REQUIRE(receiverReports.timeAdvanceGrantReports.size() == 1U);
    REQUIRE(receiverReports.timeAdvanceGrantReports.back().value == L"5");
    auto const originalDelivery = receiverReports.timestampedInteractionReports.front();
    REQUIRE(originalDelivery.interactionClass == interactionClass);
    REQUIRE(originalDelivery.producingFederate == publisherHandle);
    REQUIRE(originalDelivery.timeValue == L"5");
    REQUIRE(originalDelivery.sentOrderType == TIMESTAMP);
    REQUIRE(originalDelivery.receivedOrderType == TIMESTAMP);
    REQUIRE(originalDelivery.retractionValid);
    REQUIRE(matchesBytes(originalDelivery.userSuppliedTag, tagBytes, sizeof(tagBytes)));
    REQUIRE(originalDelivery.parameterValues.size() == 1U);
    REQUIRE(matchesBytes(
        originalDelivery.parameterValues.at(identifier),
        identifierBytes,
        sizeof(identifierBytes)));
    auto const originalInteractionCallback = std::find(
        receiverReports.callbackOrder.begin(), receiverReports.callbackOrder.end(), "interaction");
    auto const originalGrantCallback = std::find(
        receiverReports.callbackOrder.begin(), receiverReports.callbackOrder.end(), "grant");
    REQUIRE(originalInteractionCallback != receiverReports.callbackOrder.end());
    REQUIRE(originalGrantCallback != receiverReports.callbackOrder.end());
    REQUIRE(originalInteractionCallback < originalGrantCallback);

    publisherReports.onInitiateFederateSave = {};
    independentRegulatorReports.onInitiateFederateSave = {};
    receiverReports.onInitiateFederateSave = {};
    REQUIRE_NOTHROW(publisher->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(9)));
    REQUIRE_NOTHROW(
        independentRegulator->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(9)));
    REQUIRE_NOTHROW(receiver->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(9)));
    if (callbackModel == HLA_EVOKED) {
      drainAll();
    }
    REQUIRE(publisherReports.timeAdvanceGrantReports.size() == 2U);
    REQUIRE(independentRegulatorReports.timeAdvanceGrantReports.size() == 2U);
    REQUIRE(receiverReports.timeAdvanceGrantReports.size() == 2U);
    REQUIRE(receiverReports.timeAdvanceGrantReports.back().value == L"9");
    REQUIRE_NOTHROW(publisher->queryLogicalTime(publisherLogicalTime));
    REQUIRE_NOTHROW(independentRegulator->queryLogicalTime(independentRegulatorLogicalTime));
    REQUIRE_NOTHROW(receiver->queryLogicalTime(receiverLogicalTime));
    REQUIRE(publisherLogicalTime.getTime() == 9);
    REQUIRE(independentRegulatorLogicalTime.getTime() == 9);
    REQUIRE(receiverLogicalTime.getTime() == 9);

    publisherReports.callbackOrder.clear();
    independentRegulatorReports.callbackOrder.clear();
    receiverReports.callbackOrder.clear();
    REQUIRE_NOTHROW(receiver->requestFederationRestore(saveLabel));
    if (callbackModel == HLA_EVOKED) {
      drainAll();
    }
    REQUIRE_NOTHROW(publisher->federateRestoreComplete());
    REQUIRE_NOTHROW(independentRegulator->federateRestoreComplete());
    REQUIRE_NOTHROW(receiver->federateRestoreComplete());
    if (callbackModel == HLA_EVOKED) {
      drainAll();
    }

    REQUIRE(publisherReports.federationRestoredReportCount == 1U);
    REQUIRE(independentRegulatorReports.federationRestoredReportCount == 1U);
    REQUIRE(receiverReports.federationRestoredReportCount == 1U);
    REQUIRE(receiverReports.timestampedInteractionReports.size() == 2U);
    REQUIRE(receiverReports.timeAdvanceGrantReports.size() == 3U);
    REQUIRE(receiverReports.timeAdvanceGrantReports.back().value == L"5");
    auto const& replayedDelivery = receiverReports.timestampedInteractionReports.back();
    REQUIRE(replayedDelivery.interactionClass == originalDelivery.interactionClass);
    REQUIRE(replayedDelivery.producingFederate == originalDelivery.producingFederate);
    REQUIRE(replayedDelivery.timeValue == originalDelivery.timeValue);
    REQUIRE(replayedDelivery.sentOrderType == originalDelivery.sentOrderType);
    REQUIRE(replayedDelivery.receivedOrderType == originalDelivery.receivedOrderType);
    REQUIRE(replayedDelivery.retractionValid == originalDelivery.retractionValid);
    REQUIRE(matchesBytes(replayedDelivery.userSuppliedTag, tagBytes, sizeof(tagBytes)));
    REQUIRE(replayedDelivery.parameterValues.size() == 1U);
    REQUIRE(matchesBytes(
        replayedDelivery.parameterValues.at(identifier),
        identifierBytes,
        sizeof(identifierBytes)));
    auto const restoreCompleteCallback = std::find(
        receiverReports.callbackOrder.begin(), receiverReports.callbackOrder.end(), "restore-complete");
    auto const replayCallback = std::find(
        receiverReports.callbackOrder.begin(), receiverReports.callbackOrder.end(), "interaction");
    auto const restoredGrantCallback = std::find(
        receiverReports.callbackOrder.begin(), receiverReports.callbackOrder.end(), "grant");
    REQUIRE(restoreCompleteCallback != receiverReports.callbackOrder.end());
    REQUIRE(replayCallback != receiverReports.callbackOrder.end());
    REQUIRE(restoredGrantCallback != receiverReports.callbackOrder.end());
    REQUIRE(restoreCompleteCallback < replayCallback);
    REQUIRE(replayCallback < restoredGrantCallback);
    REQUIRE_NOTHROW(publisher->queryLogicalTime(publisherLogicalTime));
    REQUIRE_NOTHROW(independentRegulator->queryLogicalTime(independentRegulatorLogicalTime));
    REQUIRE_NOTHROW(receiver->queryLogicalTime(receiverLogicalTime));
    REQUIRE(publisherLogicalTime.getTime() == 5);
    REQUIRE(independentRegulatorLogicalTime.getTime() == 5);
    REQUIRE(receiverLogicalTime.getTime() == 5);

    REQUIRE_NOTHROW(publisher->resignFederationExecution(NO_ACTION));
    REQUIRE_NOTHROW(independentRegulator->resignFederationExecution(NO_ACTION));
    REQUIRE_NOTHROW(receiver->resignFederationExecution(NO_ACTION));
    REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
    REQUIRE_NOTHROW(publisher->disconnect());
    REQUIRE_NOTHROW(independentRegulator->disconnect());
    REQUIRE_NOTHROW(receiver->disconnect());
  };

  SECTION("HLA_EVOKED") {
    runScenario(HLA_EVOKED);
  }
  SECTION("HLA_IMMEDIATE") {
    runScenario(rti1516_2025::HLA_IMMEDIATE);
  }
}

TEST_CASE(
    "Embedded restore replays a timestamped attribute update queued for pending NMRA at GALT",
    "[integration][development-profile][federation-management][object-management]"
    "[save-restore][restore-pending-tso-nmra-attribute-update][time-management][tso]"
    "[next-message-request-available][timestamped-attribute-update]"
    "[multi-federate-callback-ordering]"
    "[rti.service.update-attribute-values][rti.service.subscribe-object-class-attributes]"
    "[rti.service.publish-object-class-attributes][rti.service.register-object-instance]"
    "[rti.service.enable-time-regulation][rti.service.enable-time-constrained]"
    "[rti.service.next-message-request-available][rti.service.time-advance-request]"
    "[rti.service.request-federation-save][rti.service.federate-save-begun]"
    "[rti.service.federate-save-complete][rti.service.request-federation-restore]"
    "[rti.service.federate-restore-complete][rti.service.query-logical-time]"
    "[federate.callback.initiate-federate-save][federate.callback.reflect-attribute-values]"
    "[federate.callback.time-advance-grant][federate.callback.federation-saved]"
    "[federate.callback.federation-restored]") {
  auto runScenario = [](auto const callbackModel) {
    ReportingFederateAmbassador publisherReports;
    ReportingFederateAmbassador independentRegulatorReports;
    ReportingFederateAmbassador receiverReports;
    auto publisher = makeRti();
    auto independentRegulator = makeRti();
    auto receiver = makeRti();
    auto const federationName = nextFederationName();
    auto const fomModule = (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
                            "cpp" /
                            "tests" /
                            "data" /
                            "attribute-update-passel-fom.xml")
                               .wstring();
    std::wstring const saveLabel = L"pending-nmra-attribute-update-galt-snapshot";
    rti1516_2025::FederateHandle publisherHandle;
    rti1516_2025::ObjectClassHandle objectClass;
    rti1516_2025::AttributeHandle reliableAttribute;
    rti1516_2025::ObjectInstanceHandle objectInstance;
    rti1516_2025::HLAinteger64Time publisherLogicalTime;
    rti1516_2025::HLAinteger64Time independentRegulatorLogicalTime;
    rti1516_2025::HLAinteger64Time receiverLogicalTime;
    unsigned char const tagBytes[] = {0x4E, 0x4D, 0x52, 0x41, 0x2D, 0x55, 0x50, 0x44};
    unsigned char const attributeBytes[] = {0x51, 0x22, 0x73};
    rti1516_2025::VariableLengthData const tag(tagBytes, sizeof(tagBytes));

    auto const drain = [](rti1516_2025::RTIambassador& ambassador) {
      while (ambassador.evokeCallback(0.0)) {
      }
    };
    auto const drainAll = [&] {
      bool invokedCallback = false;
      do {
        auto const publisherInvoked = publisher->evokeCallback(0.0);
        auto const regulatorInvoked = independentRegulator->evokeCallback(0.0);
        auto const receiverInvoked = receiver->evokeCallback(0.0);
        invokedCallback = publisherInvoked || regulatorInvoked || receiverInvoked;
      } while (invokedCallback);
    };
    auto const matchesBytes = [](rti1516_2025::VariableLengthData const& value,
                                 unsigned char const* expected,
                                 std::size_t const expectedSize) {
      return value.size() == expectedSize && value.data() != nullptr &&
          std::memcmp(value.data(), expected, expectedSize) == 0;
    };

    REQUIRE_NOTHROW(publisher->connect(publisherReports, callbackModel));
    REQUIRE_NOTHROW(independentRegulator->connect(independentRegulatorReports, callbackModel));
    REQUIRE_NOTHROW(receiver->connect(receiverReports, callbackModel));
    REQUIRE_NOTHROW(publisher->createFederationExecution(
        federationName,
        fomModule,
        standard_hla::mom::integer64_time));
    REQUIRE_NOTHROW(publisherHandle = publisher->joinFederationExecution(
        L"restore-pending-nmra-attribute-publisher", L"publisher", federationName));
    REQUIRE_NOTHROW(independentRegulator->joinFederationExecution(
        L"restore-pending-nmra-attribute-regulator", L"regulator", federationName));
    REQUIRE_NOTHROW(receiver->joinFederationExecution(
        L"restore-pending-nmra-attribute-receiver", L"subscriber", federationName));

    objectClass = publisher->getObjectClassHandle(
        fixture_hla::fom::attribute_fixture_child);
    reliableAttribute = publisher->getAttributeHandle(
        objectClass, fixture_hla::fixture::reliable_base_a);
    rti1516_2025::AttributeHandleSet const attributes{reliableAttribute};
    rti1516_2025::AttributeHandleValueMap const attributeValues{
        {reliableAttribute,
         rti1516_2025::VariableLengthData(attributeBytes, sizeof(attributeBytes))}};
    REQUIRE(objectClass.isValid());
    REQUIRE(reliableAttribute.isValid());
    REQUIRE_NOTHROW(publisher->publishObjectClassAttributes(objectClass, attributes));
    REQUIRE_NOTHROW(receiver->subscribeObjectClassAttributes(objectClass, attributes));
    REQUIRE_NOTHROW(publisher->changeDefaultAttributeOrderType(
        objectClass, attributes, rti1516_2025::TIMESTAMP));
    REQUIRE_NOTHROW(objectInstance = publisher->registerObjectInstance(objectClass));
    if (callbackModel == rti1516_2025::HLA_EVOKED) {
      drainAll();
    }
    REQUIRE(publisherReports.objectDiscoveryReports.empty());
    REQUIRE(receiverReports.objectDiscoveryReports.size() == 1U);
    REQUIRE(receiverReports.objectDiscoveryReports.front().objectInstance == objectInstance);
    REQUIRE(receiverReports.objectDiscoveryReports.front().objectClass == objectClass);

    REQUIRE_NOTHROW(publisher->enableTimeConstrained());
    REQUIRE_NOTHROW(independentRegulator->enableTimeConstrained());
    REQUIRE_NOTHROW(receiver->enableTimeConstrained());
    if (callbackModel == rti1516_2025::HLA_EVOKED) {
      drainAll();
    }
    REQUIRE(publisherReports.timeConstrainedEnabledReports.size() == 1U);
    REQUIRE(independentRegulatorReports.timeConstrainedEnabledReports.size() == 1U);
    REQUIRE(receiverReports.timeConstrainedEnabledReports.size() == 1U);
    REQUIRE_NOTHROW(publisher->enableTimeRegulation(
        rti1516_2025::HLAinteger64Interval(1)));
    REQUIRE_NOTHROW(independentRegulator->enableTimeRegulation(
        rti1516_2025::HLAinteger64Interval(1)));
    if (callbackModel == rti1516_2025::HLA_EVOKED) {
      drainAll();
    }
    REQUIRE(publisherReports.timeRegulationEnabledReports.size() == 1U);
    REQUIRE(independentRegulatorReports.timeRegulationEnabledReports.size() == 1U);

    // NMRA(10) is pending before the accepted timestamp-5 update. Advancing
    // both regulators to 4 makes GALT exactly 5 with lookahead 1, preserving
    // the Available equality boundary while the save captures the queued TSO.
    REQUIRE_NOTHROW(
        receiver->nextMessageRequestAvailable(rti1516_2025::HLAinteger64Time(10)));
    REQUIRE(receiverReports.attributeReflectionReports.empty());
    auto const retraction = publisher->updateAttributeValues(
        objectInstance,
        attributeValues,
        tag,
        rti1516_2025::HLAinteger64Time(5));
    REQUIRE(retraction.isValid());
    REQUIRE(receiverReports.attributeReflectionReports.empty());
    REQUIRE(receiverReports.timeAdvanceGrantReports.empty());

    REQUIRE_NOTHROW(receiver->requestFederationSave(saveLabel));
    std::mutex saveBoundaryMutex;
    std::condition_variable saveBoundary;
    std::size_t completedSaveParticipants = 0;
    std::vector<std::exception_ptr> grantThreadExceptions(3U);
    auto const grantThreadFailed = [&] {
      return std::any_of(
          grantThreadExceptions.begin(),
          grantThreadExceptions.end(),
          [](std::exception_ptr const& exception) { return static_cast<bool>(exception); });
    };
    auto const completeSaveAndWaitForFederation = [&](rti1516_2025::RTIambassador& ambassador) {
      ambassador.federateSaveBegun();
      ambassador.federateSaveComplete();
      {
        std::scoped_lock lock(saveBoundaryMutex);
        ++completedSaveParticipants;
      }
      saveBoundary.notify_all();
      if (callbackModel == rti1516_2025::HLA_EVOKED) {
        std::unique_lock lock(saveBoundaryMutex);
        saveBoundary.wait_for(
            lock,
            std::chrono::seconds(5),
            [&] { return completedSaveParticipants == 3U || grantThreadFailed(); });
      }
    };
    publisherReports.onInitiateFederateSave = [&] {
      completeSaveAndWaitForFederation(*publisher);
    };
    independentRegulatorReports.onInitiateFederateSave = [&] {
      completeSaveAndWaitForFederation(*independentRegulator);
    };
    receiverReports.onInitiateFederateSave = [&] {
      completeSaveAndWaitForFederation(*receiver);
    };
    publisherReports.callbackOrder.clear();
    independentRegulatorReports.callbackOrder.clear();
    receiverReports.callbackOrder.clear();
    if (callbackModel == rti1516_2025::HLA_IMMEDIATE) {
      REQUIRE_NOTHROW(publisher->disableCallbacks());
      REQUIRE_NOTHROW(independentRegulator->disableCallbacks());
      REQUIRE_NOTHROW(receiver->disableCallbacks());
    }
    REQUIRE_NOTHROW(publisher->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(4)));
    REQUIRE_NOTHROW(
        independentRegulator->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(4)));
    if (callbackModel == rti1516_2025::HLA_IMMEDIATE) {
      REQUIRE_NOTHROW(publisher->enableCallbacks());
      REQUIRE_NOTHROW(independentRegulator->enableCallbacks());
      REQUIRE_NOTHROW(receiver->enableCallbacks());
    } else {
      auto const drainOnThread = [&](std::size_t const exceptionIndex,
                                     rti1516_2025::RTIambassador& ambassador) {
        try {
          drain(ambassador);
        } catch (...) {
          {
            std::scoped_lock lock(saveBoundaryMutex);
            grantThreadExceptions[exceptionIndex] = std::current_exception();
          }
          saveBoundary.notify_all();
        }
      };
      std::thread publisherGrantThread([&] { drainOnThread(0U, *publisher); });
      std::thread regulatorGrantThread(
          [&] { drainOnThread(1U, *independentRegulator); });
      std::thread receiverGrantThread([&] { drainOnThread(2U, *receiver); });
      publisherGrantThread.join();
      regulatorGrantThread.join();
      receiverGrantThread.join();
      for (auto const& exception : grantThreadExceptions) {
        if (exception) {
          std::rethrow_exception(exception);
        }
      }
    }

    REQUIRE(publisherReports.initiateFederateSaveReports ==
            std::vector<std::wstring>{saveLabel});
    REQUIRE(independentRegulatorReports.initiateFederateSaveReports ==
            std::vector<std::wstring>{saveLabel});
    REQUIRE(receiverReports.initiateFederateSaveReports == std::vector<std::wstring>{saveLabel});
    REQUIRE(publisherReports.federationSavedReportCount == 1U);
    REQUIRE(independentRegulatorReports.federationSavedReportCount == 1U);
    REQUIRE(receiverReports.federationSavedReportCount == 1U);
    REQUIRE(publisherReports.timeAdvanceGrantReports.size() == 1U);
    REQUIRE(publisherReports.timeAdvanceGrantReports.back().value == L"4");
    REQUIRE(independentRegulatorReports.timeAdvanceGrantReports.size() == 1U);
    REQUIRE(independentRegulatorReports.timeAdvanceGrantReports.back().value == L"4");
    REQUIRE(receiverReports.attributeReflectionReports.size() == 1U);
    REQUIRE(receiverReports.timeAdvanceGrantReports.size() == 1U);
    REQUIRE(receiverReports.timeAdvanceGrantReports.back().value == L"5");
    auto const originalReflection = receiverReports.attributeReflectionReports.front();
    REQUIRE(originalReflection.objectInstance == objectInstance);
    REQUIRE(originalReflection.producingFederate == publisherHandle);
    REQUIRE(originalReflection.timeImplementationName == L"HLAinteger64Time");
    REQUIRE(originalReflection.timeValue == L"5");
    REQUIRE(originalReflection.sentOrderType == rti1516_2025::TIMESTAMP);
    REQUIRE(originalReflection.receivedOrderType == rti1516_2025::TIMESTAMP);
    REQUIRE(originalReflection.retractionSupplied);
    REQUIRE(originalReflection.retractionValid);
    REQUIRE(matchesBytes(originalReflection.userSuppliedTag, tagBytes, sizeof(tagBytes)));
    REQUIRE(originalReflection.attributeValues.size() == 1U);
    REQUIRE(matchesBytes(
        originalReflection.attributeValues.at(reliableAttribute),
        attributeBytes,
        sizeof(attributeBytes)));
    auto const saveCallback = std::find(
        receiverReports.callbackOrder.begin(),
        receiverReports.callbackOrder.end(),
        "save-initiate");
    auto const originalReflectionCallback = std::find(
        receiverReports.callbackOrder.begin(),
        receiverReports.callbackOrder.end(),
        "reflect");
    auto const originalGrantCallback = std::find(
        receiverReports.callbackOrder.begin(), receiverReports.callbackOrder.end(), "grant");
    REQUIRE(saveCallback != receiverReports.callbackOrder.end());
    REQUIRE(originalReflectionCallback != receiverReports.callbackOrder.end());
    REQUIRE(originalGrantCallback != receiverReports.callbackOrder.end());
    REQUIRE(saveCallback < originalReflectionCallback);
    REQUIRE(originalReflectionCallback < originalGrantCallback);

    publisherReports.onInitiateFederateSave = {};
    independentRegulatorReports.onInitiateFederateSave = {};
    receiverReports.onInitiateFederateSave = {};
    REQUIRE_NOTHROW(publisher->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(9)));
    REQUIRE_NOTHROW(
        independentRegulator->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(9)));
    REQUIRE_NOTHROW(receiver->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(9)));
    if (callbackModel == rti1516_2025::HLA_EVOKED) {
      drainAll();
    }
    REQUIRE(publisherReports.timeAdvanceGrantReports.size() == 2U);
    REQUIRE(independentRegulatorReports.timeAdvanceGrantReports.size() == 2U);
    REQUIRE(receiverReports.timeAdvanceGrantReports.size() == 2U);
    REQUIRE(receiverReports.timeAdvanceGrantReports.back().value == L"9");

    publisherReports.callbackOrder.clear();
    independentRegulatorReports.callbackOrder.clear();
    receiverReports.callbackOrder.clear();
    REQUIRE_NOTHROW(receiver->requestFederationRestore(saveLabel));
    if (callbackModel == rti1516_2025::HLA_EVOKED) {
      drainAll();
    }
    REQUIRE_NOTHROW(publisher->federateRestoreComplete());
    REQUIRE_NOTHROW(independentRegulator->federateRestoreComplete());
    REQUIRE_NOTHROW(receiver->federateRestoreComplete());
    if (callbackModel == rti1516_2025::HLA_EVOKED) {
      drainAll();
    }

    REQUIRE(publisherReports.federationRestoredReportCount == 1U);
    REQUIRE(independentRegulatorReports.federationRestoredReportCount == 1U);
    REQUIRE(receiverReports.federationRestoredReportCount == 1U);
    REQUIRE(receiverReports.attributeReflectionReports.size() == 2U);
    REQUIRE(receiverReports.timeAdvanceGrantReports.size() == 3U);
    REQUIRE(receiverReports.timeAdvanceGrantReports.back().value == L"5");
    auto const& replayedReflection = receiverReports.attributeReflectionReports.back();
    REQUIRE(replayedReflection.objectInstance == originalReflection.objectInstance);
    REQUIRE(replayedReflection.producingFederate == originalReflection.producingFederate);
    REQUIRE(replayedReflection.timeImplementationName == originalReflection.timeImplementationName);
    REQUIRE(replayedReflection.timeValue == originalReflection.timeValue);
    REQUIRE(replayedReflection.sentOrderType == originalReflection.sentOrderType);
    REQUIRE(replayedReflection.receivedOrderType == originalReflection.receivedOrderType);
    REQUIRE(replayedReflection.retractionSupplied == originalReflection.retractionSupplied);
    REQUIRE(replayedReflection.retractionValid == originalReflection.retractionValid);
    REQUIRE(matchesBytes(replayedReflection.userSuppliedTag, tagBytes, sizeof(tagBytes)));
    REQUIRE(replayedReflection.attributeValues.size() == 1U);
    REQUIRE(matchesBytes(
        replayedReflection.attributeValues.at(reliableAttribute),
        attributeBytes,
        sizeof(attributeBytes)));
    auto const restoreCompleteCallback = std::find(
        receiverReports.callbackOrder.begin(),
        receiverReports.callbackOrder.end(),
        "restore-complete");
    auto const replayCallback = std::find(
        receiverReports.callbackOrder.begin(), receiverReports.callbackOrder.end(), "reflect");
    auto const restoredGrantCallback = std::find(
        receiverReports.callbackOrder.begin(), receiverReports.callbackOrder.end(), "grant");
    REQUIRE(restoreCompleteCallback != receiverReports.callbackOrder.end());
    REQUIRE(replayCallback != receiverReports.callbackOrder.end());
    REQUIRE(restoredGrantCallback != receiverReports.callbackOrder.end());
    REQUIRE(restoreCompleteCallback < replayCallback);
    REQUIRE(replayCallback < restoredGrantCallback);
    REQUIRE_NOTHROW(publisher->queryLogicalTime(publisherLogicalTime));
    REQUIRE_NOTHROW(independentRegulator->queryLogicalTime(independentRegulatorLogicalTime));
    REQUIRE_NOTHROW(receiver->queryLogicalTime(receiverLogicalTime));
    REQUIRE(publisherLogicalTime.getTime() == 4);
    REQUIRE(independentRegulatorLogicalTime.getTime() == 4);
    REQUIRE(receiverLogicalTime.getTime() == 5);

    REQUIRE_NOTHROW(publisher->resignFederationExecution(CANCEL_THEN_DELETE_THEN_DIVEST));
    REQUIRE_NOTHROW(independentRegulator->resignFederationExecution(NO_ACTION));
    REQUIRE_NOTHROW(receiver->resignFederationExecution(NO_ACTION));
    REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
    REQUIRE_NOTHROW(publisher->disconnect());
    REQUIRE_NOTHROW(independentRegulator->disconnect());
    REQUIRE_NOTHROW(receiver->disconnect());
  };

  SECTION("HLA_EVOKED") {
    runScenario(HLA_EVOKED);
  }
  SECTION("HLA_IMMEDIATE") {
    runScenario(rti1516_2025::HLA_IMMEDIATE);
  }
}

TEST_CASE(
    "Embedded restore replays a timestamped object deletion queued for pending NMRA at GALT",
    "[integration][development-profile][federation-management][object-management]"
    "[save-restore][restore-pending-tso-nmra-delete][time-management][tso]"
    "[next-message-request-available][timestamped-object-deletion]"
    "[multi-federate-callback-ordering]"
    "[rti.service.delete-object-instance][rti.service.subscribe-object-class-attributes]"
    "[rti.service.publish-object-class-attributes][rti.service.register-object-instance]"
    "[rti.service.enable-time-regulation][rti.service.enable-time-constrained]"
    "[rti.service.next-message-request-available][rti.service.time-advance-request]"
    "[rti.service.request-federation-save][rti.service.federate-save-begun]"
    "[rti.service.federate-save-complete][rti.service.request-federation-restore]"
    "[rti.service.federate-restore-complete][rti.service.query-logical-time]"
    "[federate.callback.initiate-federate-save][federate.callback.remove-object-instance]"
    "[federate.callback.time-advance-grant][federate.callback.federation-saved]"
    "[federate.callback.federation-restored]") {
  auto runScenario = [](auto const callbackModel) {
    ReportingFederateAmbassador publisherReports;
    ReportingFederateAmbassador independentRegulatorReports;
    ReportingFederateAmbassador receiverReports;
    auto publisher = makeRti();
    auto independentRegulator = makeRti();
    auto receiver = makeRti();
    auto const federationName = nextFederationName();
    auto const fomModule = (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
                            "cpp" /
                            "tests" /
                            "data" /
                            "attribute-update-passel-fom.xml")
                               .wstring();
    std::wstring const saveLabel = L"pending-nmra-delete-galt-snapshot";
    rti1516_2025::FederateHandle publisherHandle;
    rti1516_2025::ObjectInstanceHandle objectInstance;
    rti1516_2025::HLAinteger64Time publisherLogicalTime;
    rti1516_2025::HLAinteger64Time independentRegulatorLogicalTime;
    rti1516_2025::HLAinteger64Time receiverLogicalTime;
    unsigned char const tagBytes[] = {0x4E, 0x4D, 0x52, 0x41, 0x2D, 0x44, 0x45, 0x4C};
    rti1516_2025::VariableLengthData const tag(tagBytes, sizeof(tagBytes));

    auto const drain = [](rti1516_2025::RTIambassador& ambassador) {
      while (ambassador.evokeCallback(0.0)) {
      }
    };
    auto const drainAll = [&] {
      bool invokedCallback = false;
      do {
        auto const publisherInvoked = publisher->evokeCallback(0.0);
        auto const regulatorInvoked = independentRegulator->evokeCallback(0.0);
        auto const receiverInvoked = receiver->evokeCallback(0.0);
        invokedCallback = publisherInvoked || regulatorInvoked || receiverInvoked;
      } while (invokedCallback);
    };
    auto const matchesBytes = [](rti1516_2025::VariableLengthData const& value,
                                 unsigned char const* expected,
                                 std::size_t const expectedSize) {
      return value.size() == expectedSize && value.data() != nullptr &&
          std::memcmp(value.data(), expected, expectedSize) == 0;
    };

    REQUIRE_NOTHROW(publisher->connect(publisherReports, callbackModel));
    REQUIRE_NOTHROW(independentRegulator->connect(independentRegulatorReports, callbackModel));
    REQUIRE_NOTHROW(receiver->connect(receiverReports, callbackModel));
    REQUIRE_NOTHROW(publisher->createFederationExecution(
        federationName,
        fomModule,
        standard_hla::mom::integer64_time));
    REQUIRE_NOTHROW(publisherHandle = publisher->joinFederationExecution(
        L"restore-pending-nmra-delete-publisher", L"publisher", federationName));
    REQUIRE_NOTHROW(independentRegulator->joinFederationExecution(
        L"restore-pending-nmra-delete-independent-regulator", L"regulator", federationName));
    REQUIRE_NOTHROW(receiver->joinFederationExecution(
        L"restore-pending-nmra-delete-receiver", L"subscriber", federationName));

    auto const objectClass = publisher->getObjectClassHandle(
        fixture_hla::fom::attribute_fixture_child);
    auto const reliableAttribute = publisher->getAttributeHandle(
        objectClass, fixture_hla::fixture::reliable_base_a);
    auto const bestEffortAttribute = publisher->getAttributeHandle(
        objectClass, fixture_hla::fixture::best_effort_base);
    rti1516_2025::AttributeHandleSet const attributes{
        reliableAttribute,
        bestEffortAttribute};
    REQUIRE(objectClass.isValid());
    REQUIRE(reliableAttribute.isValid());
    REQUIRE(bestEffortAttribute.isValid());
    REQUIRE_NOTHROW(publisher->publishObjectClassAttributes(objectClass, attributes));
    REQUIRE_NOTHROW(receiver->subscribeObjectClassAttributes(objectClass, attributes));
    REQUIRE_NOTHROW(objectInstance = publisher->registerObjectInstance(objectClass));
    if (callbackModel == HLA_EVOKED) {
      drainAll();
    }
    REQUIRE(publisherReports.objectDiscoveryReports.empty());
    REQUIRE(receiverReports.objectDiscoveryReports.size() == 1U);
    REQUIRE(receiverReports.objectDiscoveryReports.front().objectInstance == objectInstance);
    REQUIRE(receiverReports.objectDiscoveryReports.front().objectClass == objectClass);

    REQUIRE_NOTHROW(publisher->enableTimeConstrained());
    REQUIRE_NOTHROW(independentRegulator->enableTimeConstrained());
    REQUIRE_NOTHROW(receiver->enableTimeConstrained());
    if (callbackModel == HLA_EVOKED) {
      drainAll();
    }
    REQUIRE(publisherReports.timeConstrainedEnabledReports.size() == 1U);
    REQUIRE(independentRegulatorReports.timeConstrainedEnabledReports.size() == 1U);
    REQUIRE(receiverReports.timeConstrainedEnabledReports.size() == 1U);
    REQUIRE_NOTHROW(publisher->enableTimeRegulation(
        rti1516_2025::HLAinteger64Interval(1)));
    REQUIRE_NOTHROW(independentRegulator->enableTimeRegulation(
        rti1516_2025::HLAinteger64Interval(1)));
    if (callbackModel == HLA_EVOKED) {
      drainAll();
    }
    REQUIRE(publisherReports.timeRegulationEnabledReports.size() == 1U);
    REQUIRE(independentRegulatorReports.timeRegulationEnabledReports.size() == 1U);

    // NMRA(10) is pending before the accepted timestamp-5 deletion. Advancing
    // both regulators to 4 makes GALT exactly 5 with lookahead 1, preserving
    // the Available equality boundary while the save captures the queued TSO.
    REQUIRE_NOTHROW(
        receiver->nextMessageRequestAvailable(rti1516_2025::HLAinteger64Time(10)));
    REQUIRE(receiverReports.objectRemovalReports.empty());
    auto const retraction = publisher->deleteObjectInstance(
        objectInstance,
        tag,
        rti1516_2025::HLAinteger64Time(5));
    REQUIRE(retraction.isValid());
    REQUIRE(receiverReports.objectRemovalReports.empty());
    REQUIRE(receiverReports.timeAdvanceGrantReports.empty());

    REQUIRE_NOTHROW(receiver->requestFederationSave(saveLabel));
    std::mutex saveBoundaryMutex;
    std::condition_variable saveBoundary;
    std::size_t completedSaveParticipants = 0;
    std::vector<std::exception_ptr> grantThreadExceptions(3U);
    auto const grantThreadFailed = [&] {
      return std::any_of(
          grantThreadExceptions.begin(),
          grantThreadExceptions.end(),
          [](std::exception_ptr const& exception) { return static_cast<bool>(exception); });
    };
    auto const completeSaveAndWaitForFederation = [&](rti1516_2025::RTIambassador& ambassador) {
      ambassador.federateSaveBegun();
      ambassador.federateSaveComplete();
      {
        std::scoped_lock lock(saveBoundaryMutex);
        ++completedSaveParticipants;
      }
      saveBoundary.notify_all();
      if (callbackModel == HLA_EVOKED) {
        std::unique_lock lock(saveBoundaryMutex);
        saveBoundary.wait_for(
            lock,
            std::chrono::seconds(5),
            [&] { return completedSaveParticipants == 3U || grantThreadFailed(); });
      }
    };
    publisherReports.onInitiateFederateSave = [&] {
      completeSaveAndWaitForFederation(*publisher);
    };
    independentRegulatorReports.onInitiateFederateSave = [&] {
      completeSaveAndWaitForFederation(*independentRegulator);
    };
    receiverReports.onInitiateFederateSave = [&] {
      completeSaveAndWaitForFederation(*receiver);
    };
    publisherReports.callbackOrder.clear();
    independentRegulatorReports.callbackOrder.clear();
    receiverReports.callbackOrder.clear();
    if (callbackModel == rti1516_2025::HLA_IMMEDIATE) {
      REQUIRE_NOTHROW(publisher->disableCallbacks());
      REQUIRE_NOTHROW(independentRegulator->disableCallbacks());
      REQUIRE_NOTHROW(receiver->disableCallbacks());
    }
    REQUIRE_NOTHROW(publisher->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(4)));
    REQUIRE_NOTHROW(
        independentRegulator->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(4)));
    if (callbackModel == rti1516_2025::HLA_IMMEDIATE) {
      REQUIRE_NOTHROW(publisher->enableCallbacks());
      REQUIRE_NOTHROW(independentRegulator->enableCallbacks());
      REQUIRE_NOTHROW(receiver->enableCallbacks());
    } else {
      auto const drainOnThread = [&](std::size_t const exceptionIndex,
                                     rti1516_2025::RTIambassador& ambassador) {
        try {
          drain(ambassador);
        } catch (...) {
          {
            std::scoped_lock lock(saveBoundaryMutex);
            grantThreadExceptions[exceptionIndex] = std::current_exception();
          }
          saveBoundary.notify_all();
        }
      };
      std::thread publisherGrantThread([&] { drainOnThread(0U, *publisher); });
      std::thread regulatorGrantThread(
          [&] { drainOnThread(1U, *independentRegulator); });
      std::thread receiverGrantThread([&] { drainOnThread(2U, *receiver); });
      publisherGrantThread.join();
      regulatorGrantThread.join();
      receiverGrantThread.join();
      for (auto const& exception : grantThreadExceptions) {
        if (exception) {
          std::rethrow_exception(exception);
        }
      }
    }

    REQUIRE(publisherReports.initiateFederateSaveReports ==
            std::vector<std::wstring>{saveLabel});
    REQUIRE(independentRegulatorReports.initiateFederateSaveReports ==
            std::vector<std::wstring>{saveLabel});
    REQUIRE(receiverReports.initiateFederateSaveReports == std::vector<std::wstring>{saveLabel});
    REQUIRE(publisherReports.federationSavedReportCount == 1U);
    REQUIRE(independentRegulatorReports.federationSavedReportCount == 1U);
    REQUIRE(receiverReports.federationSavedReportCount == 1U);
    REQUIRE(publisherReports.timeAdvanceGrantReports.size() == 1U);
    REQUIRE(publisherReports.timeAdvanceGrantReports.back().value == L"4");
    REQUIRE(independentRegulatorReports.timeAdvanceGrantReports.size() == 1U);
    REQUIRE(independentRegulatorReports.timeAdvanceGrantReports.back().value == L"4");
    REQUIRE(receiverReports.objectRemovalReports.size() == 1U);
    REQUIRE(receiverReports.timeAdvanceGrantReports.size() == 1U);
    REQUIRE(receiverReports.timeAdvanceGrantReports.back().value == L"5");
    auto const originalRemoval = receiverReports.objectRemovalReports.front();
    REQUIRE(originalRemoval.objectInstance == objectInstance);
    REQUIRE(originalRemoval.producingFederate == publisherHandle);
    REQUIRE(originalRemoval.timeImplementationName == L"HLAinteger64Time");
    REQUIRE(originalRemoval.timeValue == L"5");
    REQUIRE(originalRemoval.sentOrderType == TIMESTAMP);
    REQUIRE(originalRemoval.receivedOrderType == TIMESTAMP);
    REQUIRE(originalRemoval.retractionSupplied);
    REQUIRE(originalRemoval.retractionValid);
    REQUIRE(matchesBytes(originalRemoval.userSuppliedTag, tagBytes, sizeof(tagBytes)));
    auto const saveCallback = std::find(
        receiverReports.callbackOrder.begin(),
        receiverReports.callbackOrder.end(),
        "save-initiate");
    auto const originalRemovalCallback = std::find(
        receiverReports.callbackOrder.begin(), receiverReports.callbackOrder.end(), "remove");
    auto const originalGrantCallback = std::find(
        receiverReports.callbackOrder.begin(), receiverReports.callbackOrder.end(), "grant");
    REQUIRE(saveCallback != receiverReports.callbackOrder.end());
    REQUIRE(originalRemovalCallback != receiverReports.callbackOrder.end());
    REQUIRE(originalGrantCallback != receiverReports.callbackOrder.end());
    REQUIRE(saveCallback < originalRemovalCallback);
    REQUIRE(originalRemovalCallback < originalGrantCallback);

    publisherReports.onInitiateFederateSave = {};
    independentRegulatorReports.onInitiateFederateSave = {};
    receiverReports.onInitiateFederateSave = {};
    REQUIRE_NOTHROW(publisher->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(9)));
    REQUIRE_NOTHROW(
        independentRegulator->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(9)));
    REQUIRE_NOTHROW(receiver->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(9)));
    if (callbackModel == HLA_EVOKED) {
      drainAll();
    }
    REQUIRE(publisherReports.timeAdvanceGrantReports.size() == 2U);
    REQUIRE(independentRegulatorReports.timeAdvanceGrantReports.size() == 2U);
    REQUIRE(receiverReports.timeAdvanceGrantReports.size() == 2U);
    REQUIRE(receiverReports.timeAdvanceGrantReports.back().value == L"9");

    publisherReports.callbackOrder.clear();
    independentRegulatorReports.callbackOrder.clear();
    receiverReports.callbackOrder.clear();
    REQUIRE_NOTHROW(receiver->requestFederationRestore(saveLabel));
    if (callbackModel == HLA_EVOKED) {
      drainAll();
    }
    REQUIRE_NOTHROW(publisher->federateRestoreComplete());
    REQUIRE_NOTHROW(independentRegulator->federateRestoreComplete());
    REQUIRE_NOTHROW(receiver->federateRestoreComplete());
    if (callbackModel == HLA_EVOKED) {
      drainAll();
    }

    REQUIRE(publisherReports.federationRestoredReportCount == 1U);
    REQUIRE(independentRegulatorReports.federationRestoredReportCount == 1U);
    REQUIRE(receiverReports.federationRestoredReportCount == 1U);
    REQUIRE(receiverReports.objectRemovalReports.size() == 2U);
    REQUIRE(receiverReports.timeAdvanceGrantReports.size() == 3U);
    REQUIRE(receiverReports.timeAdvanceGrantReports.back().value == L"5");
    auto const& replayedRemoval = receiverReports.objectRemovalReports.back();
    REQUIRE(replayedRemoval.objectInstance == originalRemoval.objectInstance);
    REQUIRE(replayedRemoval.producingFederate == originalRemoval.producingFederate);
    REQUIRE(replayedRemoval.timeImplementationName == originalRemoval.timeImplementationName);
    REQUIRE(replayedRemoval.timeValue == originalRemoval.timeValue);
    REQUIRE(replayedRemoval.sentOrderType == originalRemoval.sentOrderType);
    REQUIRE(replayedRemoval.receivedOrderType == originalRemoval.receivedOrderType);
    REQUIRE(replayedRemoval.retractionSupplied == originalRemoval.retractionSupplied);
    REQUIRE(replayedRemoval.retractionValid == originalRemoval.retractionValid);
    REQUIRE(matchesBytes(replayedRemoval.userSuppliedTag, tagBytes, sizeof(tagBytes)));
    auto const restoreCompleteCallback = std::find(
        receiverReports.callbackOrder.begin(),
        receiverReports.callbackOrder.end(),
        "restore-complete");
    auto const replayCallback = std::find(
        receiverReports.callbackOrder.begin(), receiverReports.callbackOrder.end(), "remove");
    auto const restoredGrantCallback = std::find(
        receiverReports.callbackOrder.begin(), receiverReports.callbackOrder.end(), "grant");
    REQUIRE(restoreCompleteCallback != receiverReports.callbackOrder.end());
    REQUIRE(replayCallback != receiverReports.callbackOrder.end());
    REQUIRE(restoredGrantCallback != receiverReports.callbackOrder.end());
    REQUIRE(restoreCompleteCallback < replayCallback);
    REQUIRE(replayCallback < restoredGrantCallback);
    REQUIRE_NOTHROW(publisher->queryLogicalTime(publisherLogicalTime));
    REQUIRE_NOTHROW(independentRegulator->queryLogicalTime(independentRegulatorLogicalTime));
    REQUIRE_NOTHROW(receiver->queryLogicalTime(receiverLogicalTime));
    REQUIRE(publisherLogicalTime.getTime() == 4);
    REQUIRE(independentRegulatorLogicalTime.getTime() == 4);
    REQUIRE(receiverLogicalTime.getTime() == 5);

    REQUIRE_NOTHROW(publisher->resignFederationExecution(NO_ACTION));
    REQUIRE_NOTHROW(independentRegulator->resignFederationExecution(NO_ACTION));
    REQUIRE_NOTHROW(receiver->resignFederationExecution(NO_ACTION));
    REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
    REQUIRE_NOTHROW(publisher->disconnect());
    REQUIRE_NOTHROW(independentRegulator->disconnect());
    REQUIRE_NOTHROW(receiver->disconnect());
  };

  SECTION("HLA_EVOKED") {
    runScenario(HLA_EVOKED);
  }
  SECTION("HLA_IMMEDIATE") {
    runScenario(rti1516_2025::HLA_IMMEDIATE);
  }
}

TEST_CASE(
    "Embedded restore replays a timestamped interaction queued for pending NMRA at GALT",
    "[integration][development-profile][federation-management][interaction-management]"
    "[save-restore][restore-pending-tso-nmra][time-management][tso]"
    "[next-message-request-available][multi-federate-callback-ordering]"
    "[rti.service.send-interaction][rti.service.subscribe-interaction-class]"
    "[rti.service.enable-time-regulation][rti.service.enable-time-constrained]"
    "[rti.service.next-message-request-available][rti.service.time-advance-request]"
    "[rti.service.request-federation-save][rti.service.federate-save-begun]"
    "[rti.service.federate-save-complete][rti.service.request-federation-restore]"
    "[rti.service.federate-restore-complete][rti.service.query-logical-time]"
    "[federate.callback.initiate-federate-save][federate.callback.receive-interaction]"
    "[federate.callback.time-advance-grant][federate.callback.federation-saved]"
    "[federate.callback.federation-restored]") {
  auto runScenario = [](auto const callbackModel) {
    ReportingFederateAmbassador publisherReports;
    ReportingFederateAmbassador independentRegulatorReports;
    ReportingFederateAmbassador receiverReports;
    auto publisher = makeRti();
    auto independentRegulator = makeRti();
    auto receiver = makeRti();
    auto const federationName = nextFederationName();
    auto const fomModule = (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
                            "cpp" /
                            "tests" /
                            "data" /
                            "parameter-handle-provider-fom.xml")
                               .wstring();
    std::wstring const saveLabel = L"pending-nmra-tso-available-boundary";
    rti1516_2025::FederateHandle publisherHandle;
    rti1516_2025::HLAinteger64Time publisherLogicalTime;
    rti1516_2025::HLAinteger64Time independentRegulatorLogicalTime;
    rti1516_2025::HLAinteger64Time receiverLogicalTime;
    unsigned char const identifierBytes[] = {0x4E, 0x4D, 0x52, 0x41, 0x2D, 0x54, 0x53, 0x4F};
    unsigned char const tagBytes[] = {0x4E, 0x4D, 0x52, 0x41, 0x2D, 0x52, 0x45, 0x50, 0x4C, 0x41, 0x59};
    rti1516_2025::VariableLengthData const tag(tagBytes, sizeof(tagBytes));

    auto const drain = [](rti1516_2025::RTIambassador& ambassador) {
      while (ambassador.evokeCallback(0.0)) {
      }
    };
    auto const drainAll = [&] {
      bool invokedCallback = false;
      do {
        auto const publisherInvoked = publisher->evokeCallback(0.0);
        auto const regulatorInvoked = independentRegulator->evokeCallback(0.0);
        auto const receiverInvoked = receiver->evokeCallback(0.0);
        invokedCallback = publisherInvoked || regulatorInvoked || receiverInvoked;
      } while (invokedCallback);
    };
    auto const matchesBytes = [](rti1516_2025::VariableLengthData const& value,
                                 unsigned char const* expected,
                                 std::size_t const expectedSize) {
      return value.size() == expectedSize && value.data() != nullptr &&
          std::memcmp(value.data(), expected, expectedSize) == 0;
    };

    REQUIRE_NOTHROW(publisher->connect(publisherReports, callbackModel));
    REQUIRE_NOTHROW(independentRegulator->connect(independentRegulatorReports, callbackModel));
    REQUIRE_NOTHROW(receiver->connect(receiverReports, callbackModel));
    REQUIRE_NOTHROW(publisher->createFederationExecution(
        federationName,
        fomModule,
        standard_hla::mom::integer64_time));
    REQUIRE_NOTHROW(publisherHandle = publisher->joinFederationExecution(
        L"restore-pending-nmra-tso-publisher", L"publisher", federationName));
    REQUIRE_NOTHROW(independentRegulator->joinFederationExecution(
        L"restore-pending-nmra-tso-independent-regulator", L"regulator", federationName));
    REQUIRE_NOTHROW(receiver->joinFederationExecution(
        L"restore-pending-nmra-tso-receiver", L"subscriber", federationName));

    auto const interactionClass = publisher->getInteractionClassHandle(
        fixture_hla::fom::parameter_fixture_child_interaction);
    auto const identifier = publisher->getParameterHandle(
        interactionClass,
        fixture_hla::fixture::identifier);
    rti1516_2025::ParameterHandleValueMap parameterValues;
    parameterValues.emplace(
        identifier,
        rti1516_2025::VariableLengthData(identifierBytes, sizeof(identifierBytes)));
    REQUIRE(interactionClass.isValid());
    REQUIRE(identifier.isValid());
    REQUIRE_NOTHROW(publisher->publishInteractionClass(interactionClass));
    REQUIRE_NOTHROW(publisher->changeInteractionOrderType(interactionClass, TIMESTAMP));
    REQUIRE_NOTHROW(receiver->subscribeInteractionClass(interactionClass));
    REQUIRE_NOTHROW(publisher->enableTimeConstrained());
    REQUIRE_NOTHROW(independentRegulator->enableTimeConstrained());
    REQUIRE_NOTHROW(receiver->enableTimeConstrained());
    if (callbackModel == HLA_EVOKED) {
      drainAll();
    }
    REQUIRE(publisherReports.timeConstrainedEnabledReports.size() == 1U);
    REQUIRE(independentRegulatorReports.timeConstrainedEnabledReports.size() == 1U);
    REQUIRE(receiverReports.timeConstrainedEnabledReports.size() == 1U);
    REQUIRE_NOTHROW(publisher->enableTimeRegulation(
        rti1516_2025::HLAinteger64Interval(1)));
    REQUIRE_NOTHROW(independentRegulator->enableTimeRegulation(
        rti1516_2025::HLAinteger64Interval(1)));
    if (callbackModel == HLA_EVOKED) {
      drainAll();
    }
    REQUIRE(publisherReports.timeRegulationEnabledReports.size() == 1U);
    REQUIRE(independentRegulatorReports.timeRegulationEnabledReports.size() == 1U);

    // NMRA(10) is pending before the accepted TSO at 5. Advancing both
    // regulators to 4 makes GALT exactly 5 (lookahead 1), so this exercises
    // the Available-boundary equality while the save captures the queued TSO.
    REQUIRE_NOTHROW(
        receiver->nextMessageRequestAvailable(rti1516_2025::HLAinteger64Time(10)));
    REQUIRE(receiverReports.timestampedInteractionReports.empty());
    auto const retraction = publisher->sendInteraction(
        interactionClass,
        parameterValues,
        tag,
        rti1516_2025::HLAinteger64Time(5));
    REQUIRE(retraction.isValid());
    REQUIRE(receiverReports.timestampedInteractionReports.empty());
    REQUIRE(receiverReports.timeAdvanceGrantReports.empty());

    REQUIRE_NOTHROW(receiver->requestFederationSave(saveLabel));
    REQUIRE(publisherReports.initiateFederateSaveReports.empty());
    REQUIRE(receiverReports.initiateFederateSaveReports.empty());
    std::mutex saveBoundaryMutex;
    std::condition_variable saveBoundary;
    std::size_t completedSaveParticipants = 0;
    std::vector<std::exception_ptr> grantThreadExceptions(3U);
    auto const grantThreadFailed = [&] {
      return std::any_of(
          grantThreadExceptions.begin(),
          grantThreadExceptions.end(),
          [](std::exception_ptr const& exception) { return static_cast<bool>(exception); });
    };
    auto const completeSaveAndWaitForFederation = [&](rti1516_2025::RTIambassador& ambassador) {
      ambassador.federateSaveBegun();
      ambassador.federateSaveComplete();
      {
        std::scoped_lock lock(saveBoundaryMutex);
        ++completedSaveParticipants;
      }
      saveBoundary.notify_all();
      if (callbackModel == HLA_EVOKED) {
        std::unique_lock lock(saveBoundaryMutex);
        saveBoundary.wait_for(
            lock,
            std::chrono::seconds(5),
            [&] { return completedSaveParticipants == 3U || grantThreadFailed(); });
      }
    };
    publisherReports.onInitiateFederateSave = [&] {
      completeSaveAndWaitForFederation(*publisher);
    };
    independentRegulatorReports.onInitiateFederateSave = [&] {
      completeSaveAndWaitForFederation(*independentRegulator);
    };
    receiverReports.onInitiateFederateSave = [&] {
      completeSaveAndWaitForFederation(*receiver);
    };
    publisherReports.callbackOrder.clear();
    independentRegulatorReports.callbackOrder.clear();
    receiverReports.callbackOrder.clear();
    if (callbackModel == rti1516_2025::HLA_IMMEDIATE) {
      REQUIRE_NOTHROW(publisher->disableCallbacks());
      REQUIRE_NOTHROW(independentRegulator->disableCallbacks());
      REQUIRE_NOTHROW(receiver->disableCallbacks());
    }
    REQUIRE_NOTHROW(publisher->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(4)));
    REQUIRE_NOTHROW(
        independentRegulator->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(4)));
    if (callbackModel == rti1516_2025::HLA_IMMEDIATE) {
      REQUIRE_NOTHROW(publisher->enableCallbacks());
      REQUIRE_NOTHROW(independentRegulator->enableCallbacks());
      REQUIRE_NOTHROW(receiver->enableCallbacks());
    } else {
      auto const drainOnThread = [&](std::size_t const exceptionIndex,
                                     rti1516_2025::RTIambassador& ambassador) {
        try {
          drain(ambassador);
        } catch (...) {
          {
            std::scoped_lock lock(saveBoundaryMutex);
            grantThreadExceptions[exceptionIndex] = std::current_exception();
          }
          saveBoundary.notify_all();
        }
      };
      std::thread publisherGrantThread([&] { drainOnThread(0U, *publisher); });
      std::thread regulatorGrantThread(
          [&] { drainOnThread(1U, *independentRegulator); });
      std::thread receiverGrantThread([&] { drainOnThread(2U, *receiver); });
      publisherGrantThread.join();
      regulatorGrantThread.join();
      receiverGrantThread.join();
      for (auto const& exception : grantThreadExceptions) {
        if (exception) {
          std::rethrow_exception(exception);
        }
      }
    }

    REQUIRE(publisherReports.initiateFederateSaveReports == std::vector<std::wstring>{saveLabel});
    REQUIRE(independentRegulatorReports.initiateFederateSaveReports ==
            std::vector<std::wstring>{saveLabel});
    REQUIRE(receiverReports.initiateFederateSaveReports == std::vector<std::wstring>{saveLabel});
    REQUIRE(publisherReports.federationSavedReportCount == 1U);
    REQUIRE(independentRegulatorReports.federationSavedReportCount == 1U);
    REQUIRE(receiverReports.federationSavedReportCount == 1U);
    REQUIRE(publisherReports.timeAdvanceGrantReports.size() == 1U);
    REQUIRE(publisherReports.timeAdvanceGrantReports.back().value == L"4");
    REQUIRE(independentRegulatorReports.timeAdvanceGrantReports.size() == 1U);
    REQUIRE(independentRegulatorReports.timeAdvanceGrantReports.back().value == L"4");
    REQUIRE(receiverReports.timestampedInteractionReports.size() == 1U);
    REQUIRE(receiverReports.timeAdvanceGrantReports.size() == 1U);
    REQUIRE(receiverReports.timeAdvanceGrantReports.back().value == L"5");
    auto const originalDelivery = receiverReports.timestampedInteractionReports.front();
    REQUIRE(originalDelivery.interactionClass == interactionClass);
    REQUIRE(originalDelivery.producingFederate == publisherHandle);
    REQUIRE(originalDelivery.timeValue == L"5");
    REQUIRE(originalDelivery.sentOrderType == TIMESTAMP);
    REQUIRE(originalDelivery.receivedOrderType == TIMESTAMP);
    REQUIRE(originalDelivery.retractionValid);
    REQUIRE(matchesBytes(originalDelivery.userSuppliedTag, tagBytes, sizeof(tagBytes)));
    REQUIRE(originalDelivery.parameterValues.size() == 1U);
    REQUIRE(matchesBytes(
        originalDelivery.parameterValues.at(identifier),
        identifierBytes,
        sizeof(identifierBytes)));
    auto const saveCallback = std::find(
        receiverReports.callbackOrder.begin(),
        receiverReports.callbackOrder.end(),
        "save-initiate");
    auto const originalInteractionCallback = std::find(
        receiverReports.callbackOrder.begin(), receiverReports.callbackOrder.end(), "interaction");
    auto const originalGrantCallback = std::find(
        receiverReports.callbackOrder.begin(), receiverReports.callbackOrder.end(), "grant");
    REQUIRE(saveCallback != receiverReports.callbackOrder.end());
    REQUIRE(originalInteractionCallback != receiverReports.callbackOrder.end());
    REQUIRE(originalGrantCallback != receiverReports.callbackOrder.end());
    REQUIRE(saveCallback < originalInteractionCallback);
    REQUIRE(originalInteractionCallback < originalGrantCallback);
    REQUIRE_NOTHROW(publisher->queryLogicalTime(publisherLogicalTime));
    REQUIRE_NOTHROW(independentRegulator->queryLogicalTime(independentRegulatorLogicalTime));
    REQUIRE_NOTHROW(receiver->queryLogicalTime(receiverLogicalTime));
    REQUIRE(publisherLogicalTime.getTime() == 4);
    REQUIRE(independentRegulatorLogicalTime.getTime() == 4);
    REQUIRE(receiverLogicalTime.getTime() == 5);

    publisherReports.onInitiateFederateSave = {};
    independentRegulatorReports.onInitiateFederateSave = {};
    receiverReports.onInitiateFederateSave = {};
    REQUIRE_NOTHROW(publisher->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(9)));
    REQUIRE_NOTHROW(
        independentRegulator->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(9)));
    REQUIRE_NOTHROW(receiver->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(9)));
    if (callbackModel == HLA_EVOKED) {
      drainAll();
    }
    REQUIRE(publisherReports.timeAdvanceGrantReports.size() == 2U);
    REQUIRE(independentRegulatorReports.timeAdvanceGrantReports.size() == 2U);
    REQUIRE(receiverReports.timeAdvanceGrantReports.size() == 2U);
    REQUIRE(receiverReports.timeAdvanceGrantReports.back().value == L"9");

    publisherReports.callbackOrder.clear();
    independentRegulatorReports.callbackOrder.clear();
    receiverReports.callbackOrder.clear();
    REQUIRE_NOTHROW(receiver->requestFederationRestore(saveLabel));
    if (callbackModel == HLA_EVOKED) {
      drainAll();
    }
    REQUIRE_NOTHROW(publisher->federateRestoreComplete());
    REQUIRE_NOTHROW(independentRegulator->federateRestoreComplete());
    REQUIRE_NOTHROW(receiver->federateRestoreComplete());
    if (callbackModel == HLA_EVOKED) {
      drainAll();
    }

    REQUIRE(publisherReports.federationRestoredReportCount == 1U);
    REQUIRE(independentRegulatorReports.federationRestoredReportCount == 1U);
    REQUIRE(receiverReports.federationRestoredReportCount == 1U);
    REQUIRE(receiverReports.timestampedInteractionReports.size() == 2U);
    REQUIRE(receiverReports.timeAdvanceGrantReports.size() == 3U);
    REQUIRE(receiverReports.timeAdvanceGrantReports.back().value == L"5");
    auto const& replayedDelivery = receiverReports.timestampedInteractionReports.back();
    REQUIRE(replayedDelivery.interactionClass == originalDelivery.interactionClass);
    REQUIRE(replayedDelivery.producingFederate == originalDelivery.producingFederate);
    REQUIRE(replayedDelivery.timeValue == originalDelivery.timeValue);
    REQUIRE(replayedDelivery.sentOrderType == originalDelivery.sentOrderType);
    REQUIRE(replayedDelivery.receivedOrderType == originalDelivery.receivedOrderType);
    REQUIRE(replayedDelivery.retractionValid == originalDelivery.retractionValid);
    REQUIRE(matchesBytes(replayedDelivery.userSuppliedTag, tagBytes, sizeof(tagBytes)));
    REQUIRE(replayedDelivery.parameterValues.size() == 1U);
    REQUIRE(matchesBytes(
        replayedDelivery.parameterValues.at(identifier),
        identifierBytes,
        sizeof(identifierBytes)));
    auto const restoreCompleteCallback = std::find(
        receiverReports.callbackOrder.begin(),
        receiverReports.callbackOrder.end(),
        "restore-complete");
    auto const replayCallback = std::find(
        receiverReports.callbackOrder.begin(), receiverReports.callbackOrder.end(), "interaction");
    auto const restoredGrantCallback = std::find(
        receiverReports.callbackOrder.begin(), receiverReports.callbackOrder.end(), "grant");
    REQUIRE(restoreCompleteCallback != receiverReports.callbackOrder.end());
    REQUIRE(replayCallback != receiverReports.callbackOrder.end());
    REQUIRE(restoredGrantCallback != receiverReports.callbackOrder.end());
    REQUIRE(restoreCompleteCallback < replayCallback);
    REQUIRE(replayCallback < restoredGrantCallback);
    REQUIRE_NOTHROW(publisher->queryLogicalTime(publisherLogicalTime));
    REQUIRE_NOTHROW(independentRegulator->queryLogicalTime(independentRegulatorLogicalTime));
    REQUIRE_NOTHROW(receiver->queryLogicalTime(receiverLogicalTime));
    REQUIRE(publisherLogicalTime.getTime() == 4);
    REQUIRE(independentRegulatorLogicalTime.getTime() == 4);
    REQUIRE(receiverLogicalTime.getTime() == 5);

    REQUIRE_NOTHROW(publisher->resignFederationExecution(NO_ACTION));
    REQUIRE_NOTHROW(independentRegulator->resignFederationExecution(NO_ACTION));
    REQUIRE_NOTHROW(receiver->resignFederationExecution(NO_ACTION));
    REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
    REQUIRE_NOTHROW(publisher->disconnect());
    REQUIRE_NOTHROW(independentRegulator->disconnect());
    REQUIRE_NOTHROW(receiver->disconnect());
  };

  SECTION("HLA_EVOKED") {
    runScenario(HLA_EVOKED);
  }
  SECTION("HLA_IMMEDIATE") {
    runScenario(rti1516_2025::HLA_IMMEDIATE);
  }
}
