#include "ieee1516_2025_federation_management_test_support.hpp"
#include "ieee1516_2025_public_federation_restore_test_support.hpp"
#include "ieee1516_2025_federation_management_fixture_support.hpp"
#include "internal/runtime/ambassador_connection_support.hpp"

TEST_CASE(
    "Embedded timed federation save waits for constrained TSO delivery and replaces pending requests",
    "[integration][development-profile][federation-management][save-restore][time-management]"
    "[timed-save][multi-federate-callback-ordering]"
    "[rti.service.request-federation-save][rti.service.time-advance-request]"
    "[federate.callback.initiate-federate-save][federate.callback.time-advance-grant]"
    "[federate.callback.receive-interaction]") {
  ReportingFederateAmbassador ownerReports;
  ReportingFederateAmbassador receiverReports;
  auto owner = makeRti();
  auto receiver = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
                          "cpp" /
                          "tests" /
                          "data" /
                          "parameter-handle-provider-fom.xml")
                             .wstring();
  unsigned char const tagBytes[] = {0x54, 0x53, 0x4F};
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));

  REQUIRE_NOTHROW(owner->connect(ownerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(receiver->connect(receiverReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      owner->createFederationExecution(federationName, fomModule, standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(owner->joinFederationExecution(
      L"timed-save-owner", L"publisher", federationName));
  REQUIRE_NOTHROW(receiver->joinFederationExecution(
      L"timed-save-receiver", L"subscriber", federationName));

  auto const interactionClass = owner->getInteractionClassHandle(
      fixture_hla::fom::parameter_fixture_child_interaction);
  auto const identifier = owner->getParameterHandle(interactionClass, fixture_hla::fixture::identifier);
  ParameterHandleValueMap parameters;
  unsigned char const identifierBytes[] = {0xA5, 0x25};
  parameters.emplace(identifier, VariableLengthData(identifierBytes, sizeof(identifierBytes)));
  REQUIRE_NOTHROW(owner->publishInteractionClass(interactionClass));
  REQUIRE_NOTHROW(owner->changeInteractionOrderType(interactionClass, TIMESTAMP));
  REQUIRE_NOTHROW(receiver->subscribeInteractionClass(interactionClass));

  REQUIRE_NOTHROW(receiver->enableTimeConstrained());
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE_NOTHROW(owner->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(1)));
  REQUIRE_FALSE(owner->evokeCallback(0.0));
  ownerReports.callbackOrder.clear();
  receiverReports.callbackOrder.clear();
  std::size_t receiverTsoReportsAtSaveInitiate = 0;
  receiverReports.onInitiateFederateSave = [&receiverReports, &receiverTsoReportsAtSaveInitiate] {
    receiverTsoReportsAtSaveInitiate = receiverReports.timestampedInteractionReports.size();
  };

  REQUIRE_THROWS_AS(
      owner->requestFederationSave(L"timed-save-too-early", rti1516_2025::HLAinteger64Time(0)),
      rti1516_2025::InvalidLogicalTime);
  REQUIRE_THROWS_AS(
      receiver->requestFederationSave(L"timed-save-at-galt", rti1516_2025::HLAinteger64Time(1)),
      rti1516_2025::LogicalTimeAlreadyPassed);

  // The second request replaces the first while neither has reached the
  // Initiate Federate Save boundary.
  REQUIRE_NOTHROW(owner->requestFederationSave(
      L"timed-save-replaced",
      rti1516_2025::HLAinteger64Time(5)));
  REQUIRE_NOTHROW(owner->requestFederationSave(
      L"timed-save-final",
      rti1516_2025::HLAinteger64Time(7)));
  REQUIRE(ownerReports.initiateFederateSaveReports.empty());
  REQUIRE(receiverReports.initiateFederateSaveReports.empty());
  REQUIRE(ownerReports.callbackOrder.empty());
  REQUIRE(receiverReports.callbackOrder.empty());

  // A timestamped interaction at the save boundary must be received before
  // the constrained federate can be instructed to save.
  auto const message = owner->sendInteraction(
      interactionClass,
      parameters,
      tag,
      rti1516_2025::HLAinteger64Time(7));
  REQUIRE(message.isValid());
  REQUIRE_NOTHROW(receiver->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(7)));
  REQUIRE_NOTHROW(owner->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(7)));
  REQUIRE_FALSE(owner->evokeCallback(0.0));
  // The receiver first receives the TSO payload at the scheduled-save
  // boundary, then receives Initiate Federate Save while still Time
  // Advancing, and only then receives its grant.
  while (receiver->evokeCallback(0.0)) {
  }
  REQUIRE(receiverReports.timestampedInteractionReports.size() == 1);
  REQUIRE(receiverReports.timeAdvanceGrantReports.size() == 1);
  REQUIRE(receiverReports.initiateFederateSaveReports ==
          std::vector<std::wstring>{L"timed-save-final"});
  REQUIRE(receiverReports.timestampedSaveInitiationReports.size() == 1);
  REQUIRE(receiverReports.timestampedSaveInitiationReports.back().label == L"timed-save-final");
  REQUIRE(receiverReports.timestampedSaveInitiationReports.back().timeImplementationName ==
          standard_hla::mom::integer64_time);
  REQUIRE(receiverReports.timestampedSaveInitiationReports.back().timeValue == L"7");
  REQUIRE(receiverTsoReportsAtSaveInitiate == 1);
  REQUIRE(receiverReports.callbackOrder ==
          std::vector<std::string>{"interaction", "save-initiate", "grant"});
  REQUIRE(ownerReports.initiateFederateSaveReports.empty());
  REQUIRE(ownerReports.callbackOrder == std::vector<std::string>{"grant"});

  // The non-time-constrained regulator is queued only after the constrained
  // recipient has been admitted at its own pre-grant boundary.
  static_cast<void>(owner->evokeCallback(0.0));
  REQUIRE(ownerReports.initiateFederateSaveReports ==
          std::vector<std::wstring>{L"timed-save-final"});
  REQUIRE(ownerReports.callbackOrder ==
          std::vector<std::string>{"grant", "save-initiate"});

  REQUIRE_NOTHROW(owner->federateSaveBegun());
  REQUIRE_NOTHROW(receiver->federateSaveBegun());
  REQUIRE_NOTHROW(owner->federateSaveComplete());
  REQUIRE_NOTHROW(receiver->federateSaveComplete());
  static_cast<void>(owner->evokeCallback(0.0));
  static_cast<void>(receiver->evokeCallback(0.0));
  REQUIRE(ownerReports.federationSavedReportCount == 1);
  REQUIRE(receiverReports.federationSavedReportCount == 1);

  REQUIRE_NOTHROW(receiver->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->resignFederationExecution(CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(receiver->disconnect());
  REQUIRE_NOTHROW(owner->disconnect());
}

TEST_CASE(
    "Embedded timed save after distinct FQR grant is delivered before the crossing time grant",
    "[integration][development-profile][2025][federation-management][time-management]"
    "[fqr-timed-save-crossed-boundary-callback-ordering]"
    "[rti.service.request-federation-save][rti.service.time-advance-request]"
    "[rti.service.flush-queue-request]"
    "[federate.callback.initiate-federate-save][federate.callback.time-advance-grant]"
    "[federate.callback.receive-interaction]") {
  ReportingFederateAmbassador regulatingReports;
  ReportingFederateAmbassador peerReports;
  auto regulating = makeRti();
  auto peer = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
                          "cpp" /
                          "tests" /
                          "data" /
                          "parameter-handle-provider-fom.xml")
                             .wstring();
  unsigned char const valueBytes[] = {0x46U, 0x51U, 0x52U, 0x53U};
  unsigned char const tagBytes[] = {0x53U, 0x41U, 0x56U, 0x45U};
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));
  auto const drain = [](RTIambassador& ambassador) {
    while (ambassador.evokeCallback(0.0)) {
    }
  };

  REQUIRE_NOTHROW(regulating->connect(regulatingReports, HLA_EVOKED));
  REQUIRE_NOTHROW(peer->connect(peerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(regulating->createFederationExecution(
      federationName, fomModule, standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(regulating->joinFederationExecution(
      L"fqr-timed-save-regulating", L"publisher", federationName));
  REQUIRE_NOTHROW(peer->joinFederationExecution(
      L"fqr-timed-save-peer", L"publisher", federationName));

  auto const interactionClass = regulating->getInteractionClassHandle(
      fixture_hla::fom::parameter_fixture_child_interaction);
  auto const identifier = regulating->getParameterHandle(
      interactionClass, fixture_hla::fixture::identifier);
  REQUIRE_NOTHROW(regulating->publishInteractionClass(interactionClass));
  REQUIRE_NOTHROW(peer->publishInteractionClass(interactionClass));
  REQUIRE_NOTHROW(regulating->changeInteractionOrderType(interactionClass, TIMESTAMP));
  REQUIRE_NOTHROW(peer->changeInteractionOrderType(interactionClass, TIMESTAMP));
  REQUIRE_NOTHROW(regulating->subscribeInteractionClass(interactionClass));
  REQUIRE_NOTHROW(regulating->enableTimeConstrained());
  drain(*regulating);
  REQUIRE_NOTHROW(regulating->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(1)));
  drain(*regulating);
  REQUIRE_NOTHROW(peer->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(1)));
  drain(*peer);

  REQUIRE_NOTHROW(regulating->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(4)));
  REQUIRE_NOTHROW(peer->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(4)));
  drain(*regulating);
  drain(*peer);

  ParameterHandleValueMap incomingValues;
  incomingValues.emplace(identifier, VariableLengthData(valueBytes, sizeof(valueBytes)));
  REQUIRE(peer->sendInteraction(
              interactionClass,
              incomingValues,
              tag,
              rti1516_2025::HLAinteger64Time(7))
              .isValid());
  REQUIRE_NOTHROW(regulating->flushQueueRequest(
      rti1516_2025::HLAinteger64Time(10)));
  drain(*regulating);
  REQUIRE(regulatingReports.flushQueueGrantReports.size() == 1U);
  REQUIRE(regulatingReports.flushQueueGrantReports.front().value == L"5");
  REQUIRE(regulatingReports.flushQueueGrantReports.front().optimisticValue == L"7");

  rti1516_2025::HLAinteger64Time logicalTime;
  REQUIRE_NOTHROW(regulating->queryLogicalTime(logicalTime));
  REQUIRE(logicalTime.getTime() == 5);
  REQUIRE_NOTHROW(regulating->requestFederationSave(
      L"fqr-save-crossed-at-six", rti1516_2025::HLAinteger64Time(6)));
  REQUIRE(regulatingReports.initiateFederateSaveReports.empty());
  REQUIRE(regulatingReports.timeAdvanceGrantReports.size() == 1U);

  // The request is legal at actual time 5 although the distinct FQR grant
  // reported optimistic time 7.  Advancing to 7 crosses the save timestamp;
  // require initiation to be observed before the matching grant callback.
  REQUIRE_NOTHROW(regulating->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(7)));
  REQUIRE_NOTHROW(peer->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(7)));
  drain(*peer);
  drain(*regulating);
  REQUIRE(regulatingReports.timestampedSaveInitiationReports.size() == 1U);
  REQUIRE(regulatingReports.timestampedSaveInitiationReports.front().label ==
          L"fqr-save-crossed-at-six");
  REQUIRE(regulatingReports.timestampedSaveInitiationReports.front().timeValue == L"6");
  REQUIRE(regulatingReports.timeAdvanceGrantReports.size() == 2U);
  REQUIRE(regulatingReports.callbackOrder ==
          std::vector<std::string>{
              "grant", "interaction", "flush-grant", "save-initiate", "grant"});

  // Keep the outstanding save bounded and cleanly terminate both members;
  // this case asserts initiation ordering, not save-image completion.
  REQUIRE_NOTHROW(regulating->federateSaveBegun());
  REQUIRE_NOTHROW(peer->federateSaveBegun());
  REQUIRE_NOTHROW(regulating->federateSaveComplete());
  REQUIRE_NOTHROW(peer->federateSaveComplete());
  drain(*regulating);
  drain(*peer);
  REQUIRE(regulatingReports.federationSavedReportCount == 1U);
  REQUIRE(peerReports.federationSavedReportCount == 1U);

  REQUIRE_NOTHROW(regulating->resignFederationExecution(CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(peer->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(regulating->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(regulating->disconnect());
  REQUIRE_NOTHROW(peer->disconnect());
}

TEST_CASE(
    "Embedded float64 timed federation save preserves provider time scheduling",
    "[integration][development-profile][federation-management][save-restore][time-management]"
    "[float-time][timed-save]"
    "[rti.service.request-federation-save][rti.service.time-advance-request]"
    "[federate.callback.initiate-federate-save][federate.callback.time-advance-grant]"
    "[federate.callback.federation-saved]") {
  ReportingFederateAmbassador ownerReports;
  ReportingFederateAmbassador receiverReports;
  auto owner = makeRti();
  auto receiver = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
                          "cpp" /
                          "tests" /
                          "data" /
                          "time-representation-float64-fom.xml")
                             .wstring();
  rti1516_2025::HLAfloat64Time const saveTime(5.0);
  auto const saveLabel = std::wstring(L"float64-timed-save-boundary");
  auto const drain = [](RTIambassador& ambassador) {
    while (ambassador.evokeCallback(0.0)) {
    }
  };

  REQUIRE_NOTHROW(owner->connect(ownerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(receiver->connect(receiverReports, HLA_EVOKED));
  REQUIRE_NOTHROW(owner->createFederationExecution(
      federationName,
      fomModule,
      standard_hla::mom::float64_time));
  REQUIRE_NOTHROW(owner->joinFederationExecution(
      L"float64-timed-save-owner", L"publisher", federationName));
  REQUIRE_NOTHROW(receiver->joinFederationExecution(
      L"float64-timed-save-receiver", L"subscriber", federationName));

  REQUIRE_NOTHROW(owner->enableTimeRegulation(
      rti1516_2025::HLAfloat64Interval(1.0)));
  REQUIRE_NOTHROW(receiver->enableTimeConstrained());
  drain(*owner);
  drain(*receiver);
  REQUIRE(ownerReports.timeRegulationEnabledReports.size() == 1U);
  REQUIRE(receiverReports.timeConstrainedEnabledReports.size() == 1U);
  REQUIRE(ownerReports.timeRegulationEnabledReports.front().implementationName ==
          standard_hla::mom::float64_time);
  REQUIRE(receiverReports.timeConstrainedEnabledReports.front().implementationName ==
          standard_hla::mom::float64_time);

  REQUIRE_NOTHROW(owner->requestFederationSave(saveLabel, saveTime));
  REQUIRE(ownerReports.timestampedSaveInitiationReports.empty());
  REQUIRE(receiverReports.timestampedSaveInitiationReports.empty());

  // The regulator reaches the requested boundary first, but the timed save
  // remains held until the constrained Java/C++ member reaches the same
  // floating boundary.  This is the native scheduling authority for the
  // external IEEE-JAR float64 save vectors.
  REQUIRE_NOTHROW(owner->timeAdvanceRequest(saveTime));
  drain(*owner);
  REQUIRE(ownerReports.timestampedSaveInitiationReports.empty());
  REQUIRE(receiverReports.timestampedSaveInitiationReports.empty());

  REQUIRE_NOTHROW(receiver->timeAdvanceRequest(saveTime));
  drain(*receiver);
  drain(*owner);
  REQUIRE(ownerReports.timestampedSaveInitiationReports.size() == 1U);
  REQUIRE(receiverReports.timestampedSaveInitiationReports.size() == 1U);
  for (auto const* reports : {&ownerReports, &receiverReports}) {
    REQUIRE(reports->timestampedSaveInitiationReports.front().label == saveLabel);
    REQUIRE(
        reports->timestampedSaveInitiationReports.front().timeImplementationName ==
        standard_hla::mom::float64_time);
    REQUIRE(
        reports->timestampedSaveInitiationReports.front().timeValue ==
        saveTime.toString());
  }

  REQUIRE_NOTHROW(owner->federateSaveBegun());
  REQUIRE_NOTHROW(receiver->federateSaveBegun());
  REQUIRE_NOTHROW(owner->federateSaveComplete());
  REQUIRE_NOTHROW(receiver->federateSaveComplete());
  drain(*owner);
  drain(*receiver);
  REQUIRE(ownerReports.federationSavedReportCount == 1U);
  REQUIRE(receiverReports.federationSavedReportCount == 1U);
  REQUIRE(ownerReports.timeAdvanceGrantReports.size() == 1U);
  REQUIRE(receiverReports.timeAdvanceGrantReports.size() == 1U);
  REQUIRE(ownerReports.timeAdvanceGrantReports.front().implementationName ==
          standard_hla::mom::float64_time);
  REQUIRE(receiverReports.timeAdvanceGrantReports.front().implementationName ==
          standard_hla::mom::float64_time);

  REQUIRE_NOTHROW(receiver->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(receiver->disconnect());
  REQUIRE_NOTHROW(owner->disconnect());
}

TEST_CASE(
    "Embedded federation resignation fails an outstanding save for remaining participants",
    "[integration][development-profile][federation-management][save-restore]"
    "[rti.service.resign-federation-execution][federate.callback.federation-not-saved]") {
  ReportingFederateAmbassador ownerReports;
  ReportingFederateAmbassador resigningReports;
  auto owner = makeRti();
  auto resigning = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();

  REQUIRE_NOTHROW(owner->connect(ownerReports, rti1516_2025::HLA_IMMEDIATE));
  REQUIRE_NOTHROW(resigning->connect(resigningReports, rti1516_2025::HLA_IMMEDIATE));
  REQUIRE_NOTHROW(
      owner->createFederationExecution(federationName, fomModule, standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(owner->joinFederationExecution(L"save-owner-resign", L"owner", federationName));
  REQUIRE_NOTHROW(
      resigning->joinFederationExecution(L"save-resigning", L"observer", federationName));

  REQUIRE_NOTHROW(owner->requestFederationSave(L"resignation-save"));
  REQUIRE_NOTHROW(resigning->resignFederationExecution(NO_ACTION));
  REQUIRE(ownerReports.federationNotSavedReasons.size() == 1);
  REQUIRE(ownerReports.federationNotSavedReasons.back() ==
          rti1516_2025::FEDERATE_RESIGNED_DURING_SAVE);
  REQUIRE_THROWS_AS(owner->federateSaveBegun(), rti1516_2025::SaveNotInitiated);

  // The failed operation is cleared, so the remaining member can start a
  // subsequent control-plane save rather than being left permanently stuck.
  REQUIRE_NOTHROW(owner->requestFederationSave(L"post-resignation-save"));
  REQUIRE_NOTHROW(owner->federateSaveBegun());
  REQUIRE_NOTHROW(owner->federateSaveComplete());
  REQUIRE(ownerReports.federationSavedReportCount == 1);

  REQUIRE_NOTHROW(owner->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(owner->disconnect());
  REQUIRE_NOTHROW(resigning->disconnect());
}

TEST_CASE(
    "Embedded federation restore rolls back a saved object-management snapshot",
    "[integration][development-profile][federation-management][save-restore]"
    "[rti.service.request-federation-restore][rti.service.federate-restore-complete]"
    "[rti.service.federate-restore-not-complete][rti.service.abort-federation-restore]"
    "[rti.service.query-federation-restore-status]"
    "[federate.callback.request-federation-restore-succeeded]"
    "[federate.callback.federation-restore-begun]"
    "[federate.callback.initiate-federate-restore]"
    "[federate.callback.federation-restored]"
    "[federate.callback.federation-restore-status-response]") {
  ReportingFederateAmbassador ownerReports;
  ReportingFederateAmbassador peerReports;
  auto owner = makeRti();
  auto peer = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();

  REQUIRE_NOTHROW(owner->connect(ownerReports, rti1516_2025::HLA_IMMEDIATE));
  REQUIRE_NOTHROW(peer->connect(peerReports, rti1516_2025::HLA_IMMEDIATE));
  REQUIRE_NOTHROW(
      owner->createFederationExecution(federationName, fomModule, standard_hla::mom::integer64_time));
  FederateHandle ownerHandle;
  FederateHandle peerHandle;
  REQUIRE_NOTHROW(
      ownerHandle = owner->joinFederationExecution(L"restore-owner", L"owner", federationName));
  REQUIRE_NOTHROW(
      peerHandle = peer->joinFederationExecution(L"restore-peer", L"peer", federationName));
  auto const requireRestoreStatusFor = [](
      rti1516_2025::FederateRestoreStatusVector const& statuses,
      FederateHandle const& preRestoreHandle,
      FederateHandle const& postRestoreHandle,
      rti1516_2025::RestoreStatus expectedStatus) {
    bool found = false;
    for (rti1516_2025::FederateRestoreStatus const& descriptor : statuses) {
      if (descriptor.preRestoreHandle != preRestoreHandle) {
        continue;
      }
      REQUIRE_FALSE(found);
      found = true;
      REQUIRE(descriptor.postRestoreHandle == postRestoreHandle);
      REQUIRE(descriptor.status == expectedStatus);
    }
    REQUIRE(found);
  };

  // Establish a completed, restorable image before adding any object state.
  REQUIRE_NOTHROW(owner->requestFederationSave(L"object-baseline"));
  REQUIRE_NOTHROW(owner->federateSaveBegun());
  REQUIRE_NOTHROW(peer->federateSaveBegun());
  REQUIRE_NOTHROW(owner->federateSaveComplete());
  REQUIRE_NOTHROW(peer->federateSaveComplete());
  REQUIRE(ownerReports.federationSavedReportCount == 1);
  REQUIRE(peerReports.federationSavedReportCount == 1);

  auto const server = owner->getObjectClassHandle(fixture_hla::fom::employee_server);
  auto const efficiency = owner->getAttributeHandle(server, fixture_hla::fixture::efficiency);
  AttributeHandleSet const efficiencyOnly{efficiency};
  REQUIRE_NOTHROW(owner->publishObjectClassAttributes(server, efficiencyOnly));
  REQUIRE_NOTHROW(peer->subscribeObjectClassAttributes(server, efficiencyOnly));
  ObjectInstanceHandle mutatedObject;
  REQUIRE_NOTHROW(mutatedObject = owner->registerObjectInstance(server));
  REQUIRE(mutatedObject.isValid());

  REQUIRE_NOTHROW(owner->requestFederationRestore(L"object-baseline"));
  REQUIRE(ownerReports.requestFederationRestoreSucceededReports ==
          std::vector<std::wstring>{L"object-baseline"});
  REQUIRE(ownerReports.federationRestoreBegunReportCount == 1);
  REQUIRE(peerReports.federationRestoreBegunReportCount == 1);
  REQUIRE(ownerReports.initiateFederateRestoreReports.size() == 1);
  REQUIRE(peerReports.initiateFederateRestoreReports.size() == 1);
  REQUIRE(ownerReports.initiateFederateRestoreReports.front().federateName == L"restore-owner");
  REQUIRE(peerReports.initiateFederateRestoreReports.front().federateName == L"restore-peer");
  REQUIRE(ownerReports.initiateFederateRestoreReports.front().postRestoreFederateHandle ==
          ownerHandle);
  REQUIRE(peerReports.initiateFederateRestoreReports.front().postRestoreFederateHandle ==
          peerHandle);

  REQUIRE_NOTHROW(owner->queryFederationRestoreStatus());
  REQUIRE(ownerReports.federationRestoreStatusReports.size() == 1);
  REQUIRE(ownerReports.federationRestoreStatusReports.back().statuses.size() == 2);
  REQUIRE(ownerReports.federationRestoreStatusReports.back().statuses[0].status ==
          rti1516_2025::FEDERATE_RESTORING);
  auto const& restoringStatuses = ownerReports.federationRestoreStatusReports.back().statuses;
  requireRestoreStatusFor(
      restoringStatuses,
      ownerHandle,
      ownerHandle,
      rti1516_2025::FEDERATE_RESTORING);
  requireRestoreStatusFor(
      restoringStatuses,
      peerHandle,
      peerHandle,
      rti1516_2025::FEDERATE_RESTORING);

  REQUIRE_NOTHROW(owner->federateRestoreComplete());
  REQUIRE(ownerReports.federationRestoredReportCount == 0);
  REQUIRE(peerReports.federationRestoredReportCount == 0);
  REQUIRE_NOTHROW(peer->federateRestoreComplete());
  REQUIRE(ownerReports.federationRestoredReportCount == 1);
  REQUIRE(peerReports.federationRestoredReportCount == 1);
  REQUIRE(ownerReports.federationNotRestoredReasons.empty());
  REQUIRE(peerReports.federationNotRestoredReasons.empty());

  // The public ambassador still owns the same joined time state and callback
  // route, but the saved federation image no longer contains the post-save
  // object or its declarations.
  REQUIRE_THROWS_AS(
      owner->getKnownObjectClassHandle(mutatedObject),
      rti1516_2025::ObjectInstanceNotKnown);
  REQUIRE_NOTHROW(owner->queryFederationRestoreStatus());
  auto const& idleStatuses = ownerReports.federationRestoreStatusReports.back().statuses;
  REQUIRE(idleStatuses.size() == 2);
  FederateHandle const noPostRestoreHandle;
  requireRestoreStatusFor(
      idleStatuses,
      ownerHandle,
      noPostRestoreHandle,
      rti1516_2025::NO_RESTORE_IN_PROGRESS);
  requireRestoreStatusFor(
      idleStatuses,
      peerHandle,
      noPostRestoreHandle,
      rti1516_2025::NO_RESTORE_IN_PROGRESS);

  REQUIRE_NOTHROW(owner->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(peer->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(owner->disconnect());
  REQUIRE_NOTHROW(peer->disconnect());
}

TEST_CASE(
    "Embedded federation restore reports missing labels and participant failures",
    "[integration][development-profile][federation-management][save-restore]"
    "[federate.callback.request-federation-restore-failed]"
    "[federate.callback.federation-not-restored]") {
  ReportingFederateAmbassador ownerReports;
  ReportingFederateAmbassador peerReports;
  auto owner = makeRti();
  auto peer = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();

  REQUIRE_NOTHROW(owner->connect(ownerReports, rti1516_2025::HLA_IMMEDIATE));
  REQUIRE_NOTHROW(peer->connect(peerReports, rti1516_2025::HLA_IMMEDIATE));
  REQUIRE_NOTHROW(
      owner->createFederationExecution(federationName, fomModule, standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(owner->joinFederationExecution(L"restore-failure-owner", L"owner", federationName));
  REQUIRE_NOTHROW(peer->joinFederationExecution(L"restore-failure-peer", L"peer", federationName));

  REQUIRE_NOTHROW(owner->requestFederationRestore(L"missing-label"));
  REQUIRE(ownerReports.requestFederationRestoreFailedReports ==
          std::vector<std::wstring>{L"missing-label"});

  REQUIRE_NOTHROW(owner->requestFederationSave(L"failure-baseline"));
  REQUIRE_NOTHROW(owner->federateSaveBegun());
  REQUIRE_NOTHROW(peer->federateSaveBegun());
  REQUIRE_NOTHROW(owner->federateSaveComplete());
  REQUIRE_NOTHROW(peer->federateSaveComplete());

  REQUIRE_NOTHROW(owner->requestFederationRestore(L"failure-baseline"));
  REQUIRE_NOTHROW(owner->federateRestoreNotComplete());
  REQUIRE(ownerReports.federationNotRestoredReasons.size() == 1);
  REQUIRE(ownerReports.federationNotRestoredReasons.back() ==
          rti1516_2025::FEDERATE_REPORTED_FAILURE_DURING_RESTORE);
  REQUIRE(peerReports.federationNotRestoredReasons.size() == 1);
  REQUIRE(peerReports.federationNotRestoredReasons.back() ==
          rti1516_2025::FEDERATE_REPORTED_FAILURE_DURING_RESTORE);

  REQUIRE_NOTHROW(owner->requestFederationRestore(L"failure-baseline"));
  REQUIRE_NOTHROW(owner->abortFederationRestore());
  REQUIRE(ownerReports.federationNotRestoredReasons.back() ==
          rti1516_2025::RESTORE_ABORTED);
  REQUIRE(peerReports.federationNotRestoredReasons.back() ==
          rti1516_2025::RESTORE_ABORTED);

  REQUIRE_NOTHROW(owner->requestFederationRestore(L"failure-baseline"));
  REQUIRE_NOTHROW(peer->resignFederationExecution(NO_ACTION));
  REQUIRE(ownerReports.federationNotRestoredReasons.back() ==
          rti1516_2025::FEDERATE_RESIGNED_DURING_RESTORE);

  REQUIRE_NOTHROW(owner->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(owner->disconnect());
    REQUIRE_NOTHROW(peer->disconnect());
}

TEST_CASE(
    "Embedded federation restore preserves synchronization-point state from the saved image",
    "[integration][development-profile][federation-management][save-restore][synchronization]"
    "[synchronization-state][region-state]"
    "[rti.service.create-region][rti.service.set-range-bounds]"
    "[rti.service.commit-region-modifications][rti.service.get-dimension-handle-set]"
    "[rti.service.get-range-bounds][rti.service.delete-region]"
    "[rti.service.register-federation-synchronization-point]"
    "[rti.service.synchronization-point-achieved]"
    "[rti.service.request-federation-save][rti.service.federate-save-complete]"
    "[rti.service.request-federation-restore][rti.service.federate-restore-complete]"
    "[federate.callback.federation-synchronized][federate.callback.federation-restored]") {
  ReportingFederateAmbassador reports;
  auto rti = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  unsigned char const tagBytes[] = {0x53, 0x41, 0x56, 0x45};
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));

  REQUIRE_NOTHROW(rti->connect(reports, rti1516_2025::HLA_IMMEDIATE));
  REQUIRE_NOTHROW(
      rti->createFederationExecution(federationName, fomModule, standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(
      rti->joinFederationExecution(L"synchronization-restore", L"sync", federationName));

  // Save while the point is announced but not yet achieved.  The saved image
  // must retain that point even if the live execution completes it afterward.
  REQUIRE_NOTHROW(rti->registerFederationSynchronizationPoint(L"restore-sync", tag));
  REQUIRE(reports.synchronizationPointAnnouncementReports.size() == 1U);
  REQUIRE(reports.synchronizationPointAnnouncementReports.front().label == L"restore-sync");

  auto const serverId = rti->getDimensionHandle(fixture_hla::fixture::server_id);
  REQUIRE(serverId.isValid());
  auto const savedRegion = rti->createRegion(DimensionHandleSet{serverId});
  REQUIRE_NOTHROW(rti->setRangeBounds(savedRegion, serverId, RangeBounds(2UL, 5UL)));
  REQUIRE_NOTHROW(rti->commitRegionModifications(RegionHandleSet{savedRegion}));
  REQUIRE_NOTHROW(rti->requestFederationSave(L"synchronization-baseline"));
  REQUIRE_NOTHROW(rti->federateSaveBegun());
  REQUIRE_NOTHROW(rti->federateSaveComplete());

  // Complete and remove the live point so the restore has to reconstitute it
  // from the saved federation image rather than observing the current state.
  REQUIRE_NOTHROW(rti->synchronizationPointAchieved(L"restore-sync"));
  REQUIRE(reports.federationSynchronizedReports.size() == 1U);
  REQUIRE_NOTHROW(rti->deleteRegion(savedRegion));

  REQUIRE_NOTHROW(rti->requestFederationRestore(L"synchronization-baseline"));
  REQUIRE_NOTHROW(rti->federateRestoreComplete());
  REQUIRE(reports.federationRestoredReportCount == 1U);

  // A second achievement is legal only if restore brought back the
  // announced, unachieved synchronization point from the saved image.
  REQUIRE_NOTHROW(rti->synchronizationPointAchieved(L"restore-sync"));
  REQUIRE(reports.federationSynchronizedReports.size() == 2U);
  REQUIRE(rti->getDimensionHandleSet(savedRegion) == DimensionHandleSet{serverId});
  auto const restoredBounds = rti->getRangeBounds(savedRegion, serverId);
  REQUIRE(restoredBounds.getLowerBound() == 2UL);
  REQUIRE(restoredBounds.getUpperBound() == 5UL);
  REQUIRE_NOTHROW(rti->deleteRegion(savedRegion));

  REQUIRE_NOTHROW(rti->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(rti->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(rti->disconnect());
}

TEST_CASE(
    "Embedded public multi-member save commits only after the final completion and restores from a new filesystem store",
    "[integration][development-profile][federation-management][save-restore]"
    "[durable-save][filesystem][process-restart][multi-federate]"
    "[public-multi-member-save-commit-barrier][callback-immediate]"
    "[rti.service.request-federation-save][rti.service.federate-save-begun]"
    "[rti.service.federate-save-complete][rti.service.request-federation-restore]"
    "[rti.service.federate-restore-complete]"
    "[federate.callback.initiate-federate-save][federate.callback.federation-saved]"
    "[federate.callback.request-federation-restore-succeeded]"
    "[federate.callback.federation-restore-begun]"
    "[federate.callback.initiate-federate-restore]"
    "[federate.callback.federation-restored]") {
  auto runScenario = [](CallbackModel callbackModel) {
    auto const saveDirectory = temporaryFederationSaveDirectory();
    auto const reportDirectory = temporaryServiceReportDirectory();
    auto configuration =
        configurationForServiceReportDirectory(reportDirectory.path());
    configuration.withRtiAddress(L"in-process");
    auto const federationName = nextFederationName();
    auto const fomModule =
        resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
    auto const saveLabel = callbackModel == rti1516_2025::HLA_EVOKED
        ? std::wstring{L"multi-member-barrier-evoked"}
        : std::wstring{L"multi-member-barrier-immediate"};

    auto const drainBoth = [](RTIambassador& first, RTIambassador& second) {
      for (int pass = 0; pass != 32; ++pass) {
        static_cast<void>(first.evokeCallback(0.0));
        static_cast<void>(second.evokeCallback(0.0));
      }
    };

    {
      auto const sourceStore =
          std::make_shared<umbra::detail::FilesystemFederationSaveCommitStore>(
              saveDirectory.path());
      auto const sourceRegistry =
          std::make_shared<umbra::detail::EmbeddedFederationRegistry>(
              nullptr, sourceStore);
      ScopedEmbeddedFederationRegistry sourceScope(sourceRegistry);
      ReportingFederateAmbassador firstReports;
      ReportingFederateAmbassador secondReports;
      auto first = makeRti();
      auto second = makeRti();

      REQUIRE_NOTHROW(first->connect(firstReports, callbackModel, configuration));
      REQUIRE_NOTHROW(second->connect(secondReports, callbackModel, configuration));
      REQUIRE_NOTHROW(first->createFederationExecution(
          federationName, fomModule, standard_hla::mom::integer64_time));
      REQUIRE_NOTHROW(first->joinFederationExecution(
          L"barrier-first", L"participant", federationName));
      REQUIRE_NOTHROW(second->joinFederationExecution(
          L"barrier-second", L"participant", federationName));

      REQUIRE_NOTHROW(first->requestFederationSave(saveLabel));
      drainBoth(*first, *second);
      REQUIRE(firstReports.initiateFederateSaveReports.size() == 1U);
      REQUIRE(secondReports.initiateFederateSaveReports.size() == 1U);
      REQUIRE_NOTHROW(first->federateSaveBegun());
      REQUIRE_NOTHROW(second->federateSaveBegun());
      REQUIRE_NOTHROW(first->federateSaveComplete());
      drainBoth(*first, *second);
      REQUIRE_FALSE(sourceStore->load(federationName, saveLabel).has_value());
      REQUIRE(firstReports.federationSavedReportCount == 0U);
      REQUIRE(secondReports.federationSavedReportCount == 0U);

      REQUIRE_NOTHROW(second->federateSaveComplete());
      drainBoth(*first, *second);
      auto const durable = sourceStore->load(federationName, saveLabel);
      REQUIRE(durable.has_value());
      REQUIRE(durable->memberFederateIds.size() == 2U);
      REQUIRE(firstReports.federationSavedReportCount == 1U);
      REQUIRE(secondReports.federationSavedReportCount == 1U);

      REQUIRE_NOTHROW(second->resignFederationExecution(NO_ACTION));
      REQUIRE_NOTHROW(first->resignFederationExecution(NO_ACTION));
      REQUIRE_NOTHROW(first->destroyFederationExecution(federationName));
      REQUIRE_NOTHROW(second->disconnect());
      REQUIRE_NOTHROW(first->disconnect());
    }

    {
      auto const freshStore =
          std::make_shared<umbra::detail::FilesystemFederationSaveCommitStore>(
              saveDirectory.path());
      auto const freshRegistry =
          std::make_shared<umbra::detail::EmbeddedFederationRegistry>(
              nullptr, freshStore);
      ScopedEmbeddedFederationRegistry freshScope(freshRegistry);
      ReportingFederateAmbassador firstReports;
      ReportingFederateAmbassador secondReports;
      auto first = makeRti();
      auto second = makeRti();

      REQUIRE_NOTHROW(first->connect(firstReports, callbackModel, configuration));
      REQUIRE_NOTHROW(second->connect(secondReports, callbackModel, configuration));
      REQUIRE_NOTHROW(first->createFederationExecution(
          federationName, fomModule, standard_hla::mom::integer64_time));
      REQUIRE_NOTHROW(first->joinFederationExecution(
          L"barrier-first", L"participant", federationName));
      REQUIRE_NOTHROW(second->joinFederationExecution(
          L"barrier-second", L"participant", federationName));

      REQUIRE_NOTHROW(first->requestFederationRestore(saveLabel));
      drainBoth(*first, *second);
      REQUIRE(firstReports.requestFederationRestoreSucceededReports ==
              std::vector<std::wstring>{saveLabel});
      REQUIRE(firstReports.federationRestoreBegunReportCount == 1U);
      REQUIRE(secondReports.federationRestoreBegunReportCount == 1U);
      REQUIRE(firstReports.initiateFederateRestoreReports.size() == 1U);
      REQUIRE(secondReports.initiateFederateRestoreReports.size() == 1U);
      REQUIRE_NOTHROW(first->federateRestoreComplete());
      REQUIRE_NOTHROW(second->federateRestoreComplete());
      drainBoth(*first, *second);
      REQUIRE(firstReports.federationRestoredReportCount == 1U);
      REQUIRE(secondReports.federationRestoredReportCount == 1U);

      REQUIRE_NOTHROW(second->resignFederationExecution(NO_ACTION));
      REQUIRE_NOTHROW(first->resignFederationExecution(NO_ACTION));
      REQUIRE_NOTHROW(first->destroyFederationExecution(federationName));
      REQUIRE_NOTHROW(second->disconnect());
      REQUIRE_NOTHROW(first->disconnect());
    }
  };

  runScenario(rti1516_2025::HLA_EVOKED);
  runScenario(rti1516_2025::HLA_IMMEDIATE);
}

TEST_CASE(
    "Embedded evoked federation restore drops queued callbacks for a resigning participant",
    "[integration][development-profile][federation-management][save-restore][callbacks]"
    "[restore-resignation-callback-lifetime]"
    "[rti.service.request-federation-save][rti.service.federate-save-begun]"
    "[rti.service.federate-save-complete][rti.service.request-federation-restore]"
    "[rti.service.resign-federation-execution]"
    "[federate.callback.request-federation-restore-succeeded]"
    "[federate.callback.federation-restore-begun]"
    "[federate.callback.initiate-federate-restore]"
    "[federate.callback.federation-not-restored]") {
  ReportingFederateAmbassador survivorReports;
  ReportingFederateAmbassador resigningReports;
  auto survivor = makeRti();
  auto resigning = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();

  REQUIRE_NOTHROW(survivor->connect(survivorReports, rti1516_2025::HLA_EVOKED));
  REQUIRE_NOTHROW(resigning->connect(resigningReports, rti1516_2025::HLA_EVOKED));
  REQUIRE_NOTHROW(
      survivor->createFederationExecution(federationName, fomModule, standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(
      survivor->joinFederationExecution(L"restore-silence-survivor", L"survivor", federationName));
  REQUIRE_NOTHROW(
      resigning->joinFederationExecution(L"restore-silence-resigning", L"resigning", federationName));

  // Establish a completed image.  Drain the evoked save callbacks so the
  // restore case below starts with an empty callback queue on both members.
  REQUIRE_NOTHROW(survivor->requestFederationSave(L"restore-silence-baseline"));
  while (survivor->evokeCallback(0.0)) {
  }
  while (resigning->evokeCallback(0.0)) {
  }
  REQUIRE_NOTHROW(survivor->federateSaveBegun());
  REQUIRE_NOTHROW(resigning->federateSaveBegun());
  REQUIRE_NOTHROW(survivor->federateSaveComplete());
  REQUIRE_NOTHROW(resigning->federateSaveComplete());
  while (survivor->evokeCallback(0.0)) {
  }
  while (resigning->evokeCallback(0.0)) {
  }
  REQUIRE(survivorReports.federationSavedReportCount == 1U);
  REQUIRE(resigningReports.federationSavedReportCount == 1U);

  survivorReports.callbackOrder.clear();
  resigningReports.callbackOrder.clear();
  REQUIRE_NOTHROW(survivor->requestFederationRestore(L"restore-silence-baseline"));
  REQUIRE(survivorReports.callbackOrder.empty());
  REQUIRE(resigningReports.callbackOrder.empty());

  // The resigning member has restore-request-succeeded, restore-begun, and
  // initiate-federate-restore callbacks queued but not yet evoked.  Resign
  // must invalidate that session's queue; only surviving members receive the
  // Federation Restored failure caused by the resignation.
  REQUIRE_NOTHROW(resigning->resignFederationExecution(NO_ACTION));
  while (survivor->evokeCallback(0.0)) {
  }
  REQUIRE(survivorReports.callbackOrder ==
          std::vector<std::string>{"restore-request-succeeded",
                                   "restore-begun",
                                   "restore-initiate",
                                   "restore-failed"});
  REQUIRE(survivorReports.federationNotRestoredReasons ==
          std::vector<RestoreFailureReason>{
              rti1516_2025::FEDERATE_RESIGNED_DURING_RESTORE});
  // Resignation may leave a private no-op cleanup task (for example, a MOM
  // conditional update that rechecks membership at callback time).  Drain
  // that task, but require that no public restore callback reaches the
  // departed participant.
  while (resigning->evokeCallback(0.0)) {
  }
  REQUIRE(resigningReports.callbackOrder.empty());
  REQUIRE(resigningReports.requestFederationRestoreSucceededReports.empty());
  REQUIRE(resigningReports.federationRestoreBegunReportCount == 0U);
  REQUIRE(resigningReports.initiateFederateRestoreReports.empty());
  REQUIRE(resigningReports.federationNotRestoredReasons.empty());

  REQUIRE_NOTHROW(survivor->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(survivor->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(survivor->disconnect());
  REQUIRE_NOTHROW(resigning->disconnect());
}

TEST_CASE(
    "Embedded evoked federation restore reconstitutes a saved synchronization point",
    "[integration][development-profile][federation-management][save-restore][synchronization][callback-evoked]"
    "[rti.service.register-federation-synchronization-point]"
    "[rti.service.synchronization-point-achieved]"
    "[rti.service.request-federation-save][rti.service.federate-save-begun]"
    "[rti.service.federate-save-complete][rti.service.request-federation-restore]"
    "[rti.service.federate-restore-complete]"
    "[federate.callback.synchronization-point-registration-succeeded]"
    "[federate.callback.announce-synchronization-point]"
    "[federate.callback.initiate-federate-save][federate.callback.federation-saved]"
    "[federate.callback.request-federation-restore-succeeded]"
    "[federate.callback.federation-restore-begun]"
    "[federate.callback.initiate-federate-restore]"
    "[federate.callback.federation-restored]"
    "[federate.callback.federation-synchronized]") {
  bool saveDirectorySettingApplied = false;
  auto const defaultSaveDirectory =
      rti1516_2025::umbra_binding_detail::configuredAmbassadorFederationSaveDirectory(
          nullptr,
          saveDirectorySettingApplied);
  REQUIRE_FALSE(saveDirectorySettingApplied);
  ReportingFederateAmbassador reports;
  auto rti = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  unsigned char const tagBytes[] = {0x45, 0x56, 0x4F, 0x4B};
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));
  std::wstring const synchronizationLabel = L"evoked-restore-sync";
  std::wstring const saveLabel = L"evoked-restore-sync-baseline";
  auto const drain = [&] {
    while (rti->evokeCallback(0.0)) {
    }
  };

  REQUIRE_NOTHROW(rti->connect(reports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      rti->createFederationExecution(federationName, fomModule, standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(rti->joinFederationExecution(
      L"evoked-restore-sync-federate", L"sync", federationName));

  // The point is announced before the save.  The save image must retain this
  // unachieved state even though the live execution completes the point later.
  REQUIRE_NOTHROW(rti->registerFederationSynchronizationPoint(
      synchronizationLabel, tag));
  REQUIRE(reports.synchronizationPointRegistrationReports.empty());
  REQUIRE(reports.synchronizationPointAnnouncementReports.empty());
  drain();
  REQUIRE(reports.synchronizationPointRegistrationReports.size() == 1U);
  REQUIRE(reports.synchronizationPointRegistrationReports.front().label ==
          synchronizationLabel);
  REQUIRE(reports.synchronizationPointRegistrationReports.front().succeeded);
  REQUIRE(reports.synchronizationPointAnnouncementReports.size() == 1U);
  REQUIRE(reports.synchronizationPointAnnouncementReports.front().label ==
          synchronizationLabel);
  REQUIRE(variableLengthDataBytes(
              reports.synchronizationPointAnnouncementReports.front()
                  .userSuppliedTag) ==
          std::vector<unsigned char>{tagBytes, tagBytes + sizeof(tagBytes)});

  REQUIRE_NOTHROW(rti->requestFederationSave(saveLabel));
  REQUIRE(reports.initiateFederateSaveReports.empty());
  drain();
  REQUIRE(reports.initiateFederateSaveReports ==
          std::vector<std::wstring>{saveLabel});
  REQUIRE_NOTHROW(rti->federateSaveBegun());
  REQUIRE_NOTHROW(rti->federateSaveComplete());
  REQUIRE(reports.federationSavedReportCount == 0U);
  drain();
  REQUIRE(reports.federationSavedReportCount == 1U);
  auto const persisted =
      umbra::detail::FilesystemFederationSaveCommitStore(defaultSaveDirectory)
          .load(federationName, saveLabel);
  REQUIRE(persisted.has_value());
  REQUIRE(persisted->federationName == federationName);
  REQUIRE(persisted->label == saveLabel);
  REQUIRE_FALSE(persisted->stateImage.empty());

  // Complete the live point after the save.  Restore must replace this live
  // completion with the announced-but-unachieved state from the image.
  REQUIRE_NOTHROW(rti->synchronizationPointAchieved(synchronizationLabel));
  REQUIRE(reports.federationSynchronizedReports.empty());
  drain();
  REQUIRE(reports.federationSynchronizedReports.size() == 1U);

  REQUIRE_NOTHROW(rti->requestFederationRestore(saveLabel));
  REQUIRE(reports.requestFederationRestoreSucceededReports.empty());
  drain();
  REQUIRE(reports.requestFederationRestoreSucceededReports ==
          std::vector<std::wstring>{saveLabel});
  REQUIRE(reports.federationRestoreBegunReportCount == 1U);
  REQUIRE(reports.initiateFederateRestoreReports.size() == 1U);
  REQUIRE(reports.initiateFederateRestoreReports.front().federateName ==
          L"evoked-restore-sync-federate");
  REQUIRE_NOTHROW(rti->federateRestoreComplete());
  REQUIRE(reports.federationRestoredReportCount == 0U);
  drain();
  REQUIRE(reports.federationRestoredReportCount == 1U);

  // A second achievement is legal only if restore reconstituted the saved
  // announced, unachieved synchronization point.
  REQUIRE_NOTHROW(rti->synchronizationPointAchieved(synchronizationLabel));
  drain();
  REQUIRE(reports.federationSynchronizedReports.size() == 2U);
  REQUIRE(reports.federationSynchronizedReports.back().label ==
          synchronizationLabel);
  REQUIRE(reports.federationSynchronizedReports.back().failedToSyncSet.empty());
  REQUIRE(reports.synchronizationPointAnnouncementReports.size() == 1U);

  REQUIRE_NOTHROW(rti->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(rti->destroyFederationExecution(federationName));
  // The save is required to outlive the federation execution itself, not
  // merely the live registry entry used to create it.
  auto const persistedAfterDestroy =
      umbra::detail::FilesystemFederationSaveCommitStore(defaultSaveDirectory)
          .load(federationName, saveLabel);
  REQUIRE(persistedAfterDestroy.has_value());
  REQUIRE(persistedAfterDestroy->federationName == federationName);
  REQUIRE(persistedAfterDestroy->label == saveLabel);
  REQUIRE(persistedAfterDestroy->stateImage == persisted->stateImage);
  REQUIRE_NOTHROW(rti->disconnect());
}
