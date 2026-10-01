#include "ieee1516_2025_federation_management_fixture_support.hpp"

TEST_CASE(
    "Embedded federation restore rebinds pending time-role callbacks through public callback models",
    "[integration][development-profile][federation-management][save-restore]"
    "[durable-save][restore][time-management][time-role][callbacks]"
    "[restore-pending-time-role-callbacks]"
    "[pending-application-request-state]"
    "[rti.service.enable-time-regulation][rti.service.request-federation-save]"
    "[rti.service.federate-save-begun][rti.service.federate-save-complete]"
    "[rti.service.request-federation-restore][rti.service.federate-restore-complete]"
    "[federate.callback.time-regulation-enabled]"
    "[federate.callback.federation-restored]"
    "[callback-immediate]") {
  auto runScenario = [](CallbackModel callbackModel) {
    ReportingFederateAmbassador reports;
    auto rti = makeRti();
    auto const federationName = nextFederationName();
    auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
    auto const saveLabel = callbackModel == HLA_EVOKED
        ? std::wstring{L"public-pending-role-evoked"}
        : std::wstring{L"public-pending-role-immediate"};

    REQUIRE_NOTHROW(rti->connect(reports, callbackModel));
    REQUIRE_NOTHROW(
        rti->createFederationExecution(federationName, fomModule, standard_hla::mom::integer64_time));
    REQUIRE_NOTHROW(rti->joinFederationExecution(
        L"public-pending-role-member", L"time-managed", federationName));

    // HLA_EVOKED leaves the role callback in the caller-gated queue.  The
    // immediate model is held at the same boundary by disabling callbacks
    // before submitting the request; both paths therefore save the pending
    // request rather than its post-callback role state.
    if (callbackModel == rti1516_2025::HLA_IMMEDIATE) {
      REQUIRE_NOTHROW(rti->disableCallbacks());
    }
    REQUIRE_NOTHROW(rti->enableTimeRegulation(
        rti1516_2025::HLAinteger64Interval(2)));
    REQUIRE(reports.timeRegulationEnabledReports.empty());

    // The public save controls are intentionally completed directly while
    // their initiation callback is still queued.  This is the same accepted
    // state transition for both callback models and keeps the role callback
    // ahead of the restore lifecycle work in the pre-restore FIFO.
    REQUIRE_NOTHROW(rti->requestFederationSave(saveLabel));
    REQUIRE_NOTHROW(rti->federateSaveBegun());
    REQUIRE_NOTHROW(rti->federateSaveComplete());
    REQUIRE(reports.federationSavedReportCount == 0U);

    REQUIRE_NOTHROW(rti->requestFederationRestore(saveLabel));
    REQUIRE_NOTHROW(rti->federateRestoreComplete());

    if (callbackModel == rti1516_2025::HLA_IMMEDIATE) {
      REQUIRE_NOTHROW(rti->enableCallbacks());
    } else {
      while (rti->evokeCallback(0.0)) {
      }
    }

    // The queued pre-restore closure is fenced by the restored callback epoch;
    // exactly one public callback comes from the freshly rebound dispatch.
    REQUIRE(reports.timeRegulationEnabledReports.size() == 1U);
    REQUIRE(reports.timeRegulationEnabledReports.front().value == L"0");
    REQUIRE(reports.federationSavedReportCount == 1U);
    REQUIRE(reports.federationRestoredReportCount == 1U);

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
    "Embedded federation restore rebinds pending time-constrained callbacks through public callback models",
    "[integration][development-profile][federation-management][save-restore]"
    "[durable-save][restore][time-management][time-role][callbacks]"
    "[restore-pending-time-role-callbacks]"
    "[pending-application-request-state]"
    "[rti.service.enable-time-constrained][rti.service.request-federation-save]"
    "[rti.service.federate-save-begun][rti.service.federate-save-complete]"
    "[rti.service.request-federation-restore][rti.service.federate-restore-complete]"
    "[federate.callback.time-constrained-enabled]"
    "[federate.callback.federation-restored]"
    "[callback-immediate]") {
  auto runScenario = [](CallbackModel callbackModel) {
    ReportingFederateAmbassador reports;
    auto rti = makeRti();
    auto const federationName = nextFederationName();
    auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
    auto const saveLabel = callbackModel == HLA_EVOKED
        ? std::wstring{L"public-pending-constrained-evoked"}
        : std::wstring{L"public-pending-constrained-immediate"};

    REQUIRE_NOTHROW(rti->connect(reports, callbackModel));
    REQUIRE_NOTHROW(
        rti->createFederationExecution(federationName, fomModule, standard_hla::mom::integer64_time));
    REQUIRE_NOTHROW(rti->joinFederationExecution(
        L"public-pending-constrained-member", L"time-managed", federationName));

    if (callbackModel == rti1516_2025::HLA_IMMEDIATE) {
      REQUIRE_NOTHROW(rti->disableCallbacks());
    }
    REQUIRE_NOTHROW(rti->enableTimeConstrained());
    REQUIRE(reports.timeConstrainedEnabledReports.empty());

    // Complete the public save while the role-enable callback remains queued.
    // The saved image therefore contains the pending constrained request, not
    // the post-callback role state.
    REQUIRE_NOTHROW(rti->requestFederationSave(saveLabel));
    REQUIRE_NOTHROW(rti->federateSaveBegun());
    REQUIRE_NOTHROW(rti->federateSaveComplete());
    REQUIRE(reports.federationSavedReportCount == 0U);

    REQUIRE_NOTHROW(rti->requestFederationRestore(saveLabel));
    REQUIRE_NOTHROW(rti->federateRestoreComplete());

    if (callbackModel == rti1516_2025::HLA_IMMEDIATE) {
      REQUIRE_NOTHROW(rti->enableCallbacks());
    } else {
      while (rti->evokeCallback(0.0)) {
      }
    }

    REQUIRE(reports.timeConstrainedEnabledReports.size() == 1U);
    REQUIRE(reports.timeConstrainedEnabledReports.front().value == L"0");
    REQUIRE(reports.federationSavedReportCount == 1U);
    REQUIRE(reports.federationRestoredReportCount == 1U);

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
    "Embedded federation restore rebinds mixed pending time-role callbacks per member",
    "[integration][development-profile][federation-management][save-restore]"
    "[durable-save][restore][time-management][time-role][callbacks]"
    "[restore-pending-time-role-callbacks]"
    "[pending-application-request-state][multi-federate-callback-ordering]"
    "[rti.service.enable-time-regulation][rti.service.enable-time-constrained]"
    "[rti.service.request-federation-save][rti.service.federate-save-begun]"
    "[rti.service.federate-save-complete][rti.service.request-federation-restore]"
    "[rti.service.federate-restore-complete]"
    "[federate.callback.time-regulation-enabled]"
    "[federate.callback.time-constrained-enabled]"
    "[federate.callback.federation-saved][federate.callback.federation-restored]"
    "[callback-immediate]") {
  auto runScenario = [](CallbackModel callbackModel) {
    ReportingFederateAmbassador regulationReports;
    ReportingFederateAmbassador constrainedReports;
    auto regulation = makeRti();
    auto constrained = makeRti();
    auto const federationName = nextFederationName();
    auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
    auto const saveLabel = callbackModel == HLA_EVOKED
        ? std::wstring{L"public-mixed-pending-role-evoked"}
        : std::wstring{L"public-mixed-pending-role-immediate"};

    REQUIRE_NOTHROW(regulation->connect(regulationReports, callbackModel));
    REQUIRE_NOTHROW(constrained->connect(constrainedReports, callbackModel));
    REQUIRE_NOTHROW(regulation->createFederationExecution(
        federationName,
        fomModule,
        standard_hla::mom::integer64_time));
    REQUIRE_NOTHROW(regulation->joinFederationExecution(
        L"public-mixed-regulation-member", L"time-managed", federationName));
    REQUIRE_NOTHROW(constrained->joinFederationExecution(
        L"public-mixed-constrained-member", L"time-managed", federationName));

    if (callbackModel == rti1516_2025::HLA_IMMEDIATE) {
      REQUIRE_NOTHROW(regulation->disableCallbacks());
      REQUIRE_NOTHROW(constrained->disableCallbacks());
    }
    REQUIRE_NOTHROW(regulation->enableTimeRegulation(
        rti1516_2025::HLAinteger64Interval(2)));
    REQUIRE_NOTHROW(constrained->enableTimeConstrained());
    REQUIRE(regulationReports.timeRegulationEnabledReports.empty());
    REQUIRE(constrainedReports.timeConstrainedEnabledReports.empty());

    // Complete one public save across both members without draining either
    // pending role callback.  The image therefore contains two distinct role
    // generations owned by two distinct live callback routes.
    REQUIRE_NOTHROW(regulation->requestFederationSave(saveLabel));
    REQUIRE_NOTHROW(regulation->federateSaveBegun());
    REQUIRE_NOTHROW(regulation->federateSaveComplete());
    REQUIRE_NOTHROW(constrained->federateSaveBegun());
    REQUIRE_NOTHROW(constrained->federateSaveComplete());
    REQUIRE(regulationReports.federationSavedReportCount == 0U);
    REQUIRE(constrainedReports.federationSavedReportCount == 0U);

    REQUIRE_NOTHROW(regulation->requestFederationRestore(saveLabel));
    REQUIRE_NOTHROW(regulation->federateRestoreComplete());
    REQUIRE_NOTHROW(constrained->federateRestoreComplete());

    auto const drain = [](RTIambassador& ambassador) {
      while (ambassador.evokeCallback(0.0)) {
      }
    };
    if (callbackModel == rti1516_2025::HLA_IMMEDIATE) {
      REQUIRE_NOTHROW(regulation->enableCallbacks());
    } else {
      drain(*regulation);
    }

    // The final completion call schedules both dispatches, but delivering one
    // route cannot enter the other member's ambassador.
    REQUIRE(regulationReports.timeRegulationEnabledReports.size() == 1U);
    REQUIRE(constrainedReports.timeConstrainedEnabledReports.empty());
    REQUIRE(regulationReports.federationRestoredReportCount == 1U);
    REQUIRE(constrainedReports.federationRestoredReportCount == 0U);

    if (callbackModel == rti1516_2025::HLA_IMMEDIATE) {
      REQUIRE_NOTHROW(constrained->enableCallbacks());
    } else {
      drain(*constrained);
    }
    REQUIRE(constrainedReports.timeConstrainedEnabledReports.size() == 1U);
    REQUIRE(constrainedReports.federationRestoredReportCount == 1U);
    REQUIRE(regulationReports.federationSavedReportCount == 1U);
    REQUIRE(constrainedReports.federationSavedReportCount == 1U);

    REQUIRE_NOTHROW(regulation->resignFederationExecution(NO_ACTION));
    REQUIRE_NOTHROW(constrained->resignFederationExecution(NO_ACTION));
    REQUIRE_NOTHROW(regulation->destroyFederationExecution(federationName));
    REQUIRE_NOTHROW(constrained->disconnect());
    REQUIRE_NOTHROW(regulation->disconnect());
  };

  SECTION("HLA_EVOKED") {
    runScenario(HLA_EVOKED);
  }
  SECTION("HLA_IMMEDIATE") {
    runScenario(rti1516_2025::HLA_IMMEDIATE);
  }
}

TEST_CASE(
    "Embedded fresh-registry restore rebinds pending TAR and deferred lookahead through public callback models",
    "[integration][development-profile][federation-management][save-restore]"
    "[durable-save][filesystem][process-restart][restore][time-management][callbacks]"
    "[pending-application-request-state][callback-immediate]"
    "[restore-pending-time-role-callbacks]"
    "[rti.service.enable-time-regulation][rti.service.enable-time-constrained]"
    "[rti.service.modify-lookahead][rti.service.time-advance-request]"
    "[rti.service.request-federation-save][rti.service.federate-save-begun]"
    "[rti.service.federate-save-complete][rti.service.request-federation-restore]"
    "[rti.service.federate-restore-complete]"
    "[federate.callback.time-advance-grant][federate.callback.federation-saved]"
    "[federate.callback.federation-restored]") {
  auto runScenario = [](CallbackModel callbackModel) {
    auto const saveDirectory = temporaryFederationSaveDirectory();
    auto const saveStore = std::make_shared<umbra::detail::FilesystemFederationSaveCommitStore>(
        saveDirectory.path());
    auto const sourceRegistry = std::make_shared<umbra::detail::EmbeddedFederationRegistry>(
        nullptr,
        saveStore);
    ScopedEmbeddedFederationRegistry sourceScope(sourceRegistry);

    auto const federationName = nextFederationName();
    auto const fomModule = (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
                            "cpp" / "tests" / "data" /
                            "switch-nrg-disabled-fom.xml")
                               .wstring();
    auto const saveLabel = callbackModel == HLA_EVOKED
        ? std::wstring{L"public-fresh-pending-evoked"}
        : std::wstring{L"public-fresh-pending-immediate"};
    auto const drainBoth = [](RTIambassador& first, RTIambassador& second) {
      bool progressed = false;
      do {
        progressed = false;
        if (first.evokeCallback(0.0)) {
          progressed = true;
        }
        if (second.evokeCallback(0.0)) {
          progressed = true;
        }
      } while (progressed);
    };

    ReportingFederateAmbassador sourceTargetReports;
    ReportingFederateAmbassador sourceRegulatorReports;
    auto sourceTarget = makeRti();
    auto sourceRegulator = makeRti();
    FederateHandle sourceTargetHandle;
    FederateHandle sourceRegulatorHandle;

    REQUIRE_NOTHROW(sourceTarget->connect(sourceTargetReports, callbackModel));
    REQUIRE_NOTHROW(sourceRegulator->connect(sourceRegulatorReports, callbackModel));
    REQUIRE_NOTHROW(sourceTarget->createFederationExecution(
        federationName,
        fomModule,
        standard_hla::mom::integer64_time));
    REQUIRE_NOTHROW(sourceTargetHandle = sourceTarget->joinFederationExecution(
        L"fresh-pending-target",
        L"time-managed",
        federationName));
    REQUIRE_NOTHROW(sourceRegulatorHandle = sourceRegulator->joinFederationExecution(
        L"fresh-pending-regulator",
        L"time-managed",
        federationName));

    // Both members participate in the direct constrained-save boundary and
    // both regulate.  The second request sees the first member's pending
    // regulating boundary, so the two callback routes can be rebound without
    // introducing a non-constrained save participant.
    REQUIRE_NOTHROW(sourceTarget->enableTimeConstrained());
    REQUIRE_NOTHROW(sourceTarget->enableTimeRegulation(
        rti1516_2025::HLAinteger64Interval(10)));
    REQUIRE_NOTHROW(sourceRegulator->enableTimeConstrained());
    REQUIRE_NOTHROW(sourceRegulator->enableTimeRegulation(
        rti1516_2025::HLAinteger64Interval(10)));
    if (callbackModel == HLA_EVOKED) {
      drainBoth(*sourceTarget, *sourceRegulator);
    }
    REQUIRE(sourceTargetReports.timeConstrainedEnabledReports.size() == 1U);
    REQUIRE(sourceTargetReports.timeRegulationEnabledReports.size() == 1U);
    REQUIRE(sourceRegulatorReports.timeConstrainedEnabledReports.size() == 1U);
    REQUIRE(sourceRegulatorReports.timeRegulationEnabledReports.size() == 1U);

    // Keep the visible regulator lookahead at ten while retaining a deferred
    // decreasing Modify Lookahead request in the saved application ledger.
    REQUIRE_NOTHROW(sourceRegulator->modifyLookahead(
        rti1516_2025::HLAinteger64Interval(1)));
    rti1516_2025::HLAinteger64Interval sourceLookahead;
    REQUIRE_NOTHROW(sourceRegulator->queryLookahead(sourceLookahead));
    REQUIRE(sourceLookahead.getInterval() == 10);

    sourceTargetReports.onInitiateFederateSave = [&] {
      REQUIRE_NOTHROW(sourceTarget->federateSaveBegun());
      REQUIRE_NOTHROW(sourceTarget->federateSaveComplete());
    };
    sourceRegulatorReports.onInitiateFederateSave = [&] {
      REQUIRE_NOTHROW(sourceRegulator->federateSaveBegun());
      REQUIRE_NOTHROW(sourceRegulator->federateSaveComplete());
    };
    REQUIRE_NOTHROW(sourceTarget->requestFederationSave(saveLabel));

    // Each constrained member requests the same future time after the save
    // is pending.  The first request waits at the other regulator's GALT; the
    // second request makes both grants eligible, and the direct save callback
    // runs before either matching Time Advance Grant mutates its time state.
    REQUIRE_NOTHROW(sourceTarget->timeAdvanceRequest(
        rti1516_2025::HLAinteger64Time(20)));
    REQUIRE_NOTHROW(sourceRegulator->timeAdvanceRequest(
        rti1516_2025::HLAinteger64Time(20)));
    if (callbackModel == HLA_EVOKED) {
      drainBoth(*sourceTarget, *sourceRegulator);
    }
    REQUIRE(sourceTargetReports.federationSavedReportCount == 1U);
    REQUIRE(sourceRegulatorReports.federationSavedReportCount == 1U);

    auto const durable = saveStore->load(federationName, saveLabel);
    REQUIRE(durable.has_value());
    REQUIRE_FALSE(durable->stateImage.empty());
    auto const durableImage = umbra::detail::FederationStateImageCodec::decode(
        durable->stateImage);
    auto const sourceTargetId =
        rti1516_2025::umbra_binding_detail::federateHandleValue(sourceTargetHandle);
    auto const sourceRegulatorId =
        rti1516_2025::umbra_binding_detail::federateHandleValue(sourceRegulatorHandle);
    REQUIRE(sourceTargetId.has_value());
    REQUIRE(sourceRegulatorId.has_value());
    auto const targetState = std::find_if(
        durableImage.timeStates.begin(),
        durableImage.timeStates.end(),
        [sourceTargetId](umbra::detail::FederationStateImageTimeState const& state) {
          return state.federateId == *sourceTargetId;
        });
    auto const regulatorState = std::find_if(
        durableImage.timeStates.begin(),
        durableImage.timeStates.end(),
        [sourceRegulatorId](umbra::detail::FederationStateImageTimeState const& state) {
          return state.federateId == *sourceRegulatorId;
        });
    REQUIRE(targetState != durableImage.timeStates.end());
    REQUIRE(regulatorState != durableImage.timeStates.end());
    REQUIRE((regulatorState->flags & (1U << 4U)) != 0U);
    REQUIRE(regulatorState->requestedTimeEncoding.has_value());
    REQUIRE(regulatorState->pendingModifiedLookaheadEncoding.has_value());

    // The source-side callbacks have now completed the save and both source
    // ambassadors are back at their post-grant boundary.  Tear them down so
    // the fresh registry below can load only the durable image, not any
    // process-local snapshot or callback closure.
    sourceTargetReports.onInitiateFederateSave = {};
    sourceRegulatorReports.onInitiateFederateSave = {};
    REQUIRE_NOTHROW(sourceTarget->resignFederationExecution(NO_ACTION));
    REQUIRE_NOTHROW(sourceRegulator->resignFederationExecution(NO_ACTION));
    REQUIRE_NOTHROW(sourceTarget->destroyFederationExecution(federationName));
    REQUIRE_NOTHROW(sourceTarget->disconnect());
    REQUIRE_NOTHROW(sourceRegulator->disconnect());

    auto const freshRegistry = std::make_shared<umbra::detail::EmbeddedFederationRegistry>(
        nullptr,
        saveStore);
    ScopedEmbeddedFederationRegistry freshScope(freshRegistry);
    ReportingFederateAmbassador freshTargetReports;
    ReportingFederateAmbassador freshRegulatorReports;
    auto freshTarget = makeRti();
    auto freshRegulator = makeRti();
    FederateHandle freshTargetHandle;
    FederateHandle freshRegulatorHandle;

    REQUIRE_NOTHROW(freshTarget->connect(freshTargetReports, callbackModel));
    REQUIRE_NOTHROW(freshRegulator->connect(freshRegulatorReports, callbackModel));
    REQUIRE_NOTHROW(freshTarget->createFederationExecution(
        federationName,
        fomModule,
        standard_hla::mom::integer64_time));
    REQUIRE_NOTHROW(freshTargetHandle = freshTarget->joinFederationExecution(
        L"fresh-pending-target",
        L"time-managed",
        federationName));
    REQUIRE_NOTHROW(freshRegulatorHandle = freshRegulator->joinFederationExecution(
        L"fresh-pending-regulator",
        L"time-managed",
        federationName));
    REQUIRE(freshTargetHandle == sourceTargetHandle);
    REQUIRE(freshRegulatorHandle == sourceRegulatorHandle);

    REQUIRE_NOTHROW(freshTarget->requestFederationRestore(saveLabel));
    if (callbackModel == HLA_EVOKED) {
      drainBoth(*freshTarget, *freshRegulator);
    }
    REQUIRE_NOTHROW(freshTarget->federateRestoreComplete());
    REQUIRE_NOTHROW(freshRegulator->federateRestoreComplete());
    if (callbackModel == HLA_EVOKED) {
      drainBoth(*freshTarget, *freshRegulator);
    }
    REQUIRE(freshTargetReports.federationRestoredReportCount == 1U);
    REQUIRE(freshRegulatorReports.federationRestoredReportCount == 1U);
    // The durable image contains the second member's still-pending TAR.  Its
    // fresh live route is eligible immediately because the first member was
    // saved at time twenty, so restore completion must deliver exactly one
    // newly rebound grant to that member.
    REQUIRE(freshTargetReports.timeAdvanceGrantReports.empty());
    REQUIRE(freshRegulatorReports.timeAdvanceGrantReports.size() == 1U);
    REQUIRE(freshRegulatorReports.timeAdvanceGrantReports.back().value == L"20");
    rti1516_2025::HLAinteger64Time freshTargetTime;
    rti1516_2025::HLAinteger64Time freshRegulatorTime;
    rti1516_2025::HLAinteger64Interval freshLookahead;
    REQUIRE_NOTHROW(freshRegulator->queryLogicalTime(freshRegulatorTime));
    REQUIRE_NOTHROW(freshRegulator->queryLookahead(freshLookahead));
    REQUIRE_NOTHROW(freshTarget->queryLogicalTime(freshTargetTime));
    REQUIRE(freshTargetTime.getTime() == 20);
    REQUIRE(freshRegulatorTime.getTime() == 20);
    REQUIRE(freshLookahead.getInterval() == 1);

    REQUIRE_NOTHROW(freshTarget->resignFederationExecution(NO_ACTION));
    REQUIRE_NOTHROW(freshRegulator->resignFederationExecution(NO_ACTION));
    REQUIRE_NOTHROW(freshTarget->destroyFederationExecution(federationName));
    REQUIRE_NOTHROW(freshTarget->disconnect());
    REQUIRE_NOTHROW(freshRegulator->disconnect());
  };

  SECTION("HLA_EVOKED") {
    runScenario(HLA_EVOKED);
  }
  SECTION("HLA_IMMEDIATE") {
    runScenario(rti1516_2025::HLA_IMMEDIATE);
  }
}
