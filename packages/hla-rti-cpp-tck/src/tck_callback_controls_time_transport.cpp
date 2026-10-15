#include "tck_callback_controls_scenarios.hpp"
#include "tck_main_scenarios.hpp"

namespace hla_rti_cpp_tck::detail {

void scenarioCallbackControlsTimeAdvance(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.logicalTimeImplementationName.empty(),
      "Callback-control time-advance testing requires an adapter-supplied logical-time implementation");

  Session regulator(options, model, "regulator");
  Session constrained(options, model, "constrained");
  auto const federation = federationName(
      options,
      "callback-controls-time-advance");
  connectAndJoin(regulator, constrained, options, federation, options.fom);

  auto regulatorTime = makeTimeContext(regulator);
  auto constrainedTime = makeTimeContext(constrained);
  regulator.rtiAmbassador().enableTimeRegulation(*regulatorTime.epsilon);
  waitFor(
      regulator,
      [&] { return regulator.recorder().timeRegulationEnabled().size() == 1U; },
      options,
      "callback-control time-regulation enablement");
  constrained.rtiAmbassador().enableTimeConstrained();
  waitFor(
      constrained,
      [&] { return constrained.recorder().timeConstrainedEnabled().size() == 1U; },
      options,
      "callback-control time-constrained enablement");

  auto const regulatorTarget = timeAfter(
      *regulatorTime.factory,
      *regulatorTime.initial,
      *regulatorTime.epsilon,
      4U);
  auto const constrainedTarget = timeAfter(
      *constrainedTime.factory,
      *constrainedTime.initial,
      *constrainedTime.epsilon,
      4U);
  constrained.rtiAmbassador().disableCallbacks();
  regulator.rtiAmbassador().timeAdvanceRequest(*regulatorTarget);
  constrained.rtiAmbassador().timeAdvanceRequest(*constrainedTarget);
  waitFor(
      regulator,
      [&] { return regulator.recorder().timeAdvanceGrants().size() == 1U; },
      options,
      "callback-control regulator time-advance grant");

  if (model == rti::HLA_EVOKED) {
    require(
        constrained.evokeMultipleCallbacks(0.0, 1.0),
        "evoked callback control did not admit the pending time-advance grant");
  } else {
    static_cast<void>(constrained.rtiAmbassador().getObjectClassHandle(
        options.objectClassName));
  }
  require(
      constrained.recorder().timeAdvanceGrants().empty(),
      "disabled callbacks exposed a time-advance grant");

  constrained.rtiAmbassador().enableCallbacks();
  if (model == rti::HLA_EVOKED) {
    static_cast<void>(constrained.evokeMultipleCallbacks(0.0, 1.0));
  } else {
    static_cast<void>(constrained.rtiAmbassador().getObjectClassHandle(
        options.objectClassName));
  }
  waitFor(
      constrained,
      [&] { return constrained.recorder().timeAdvanceGrants().size() == 1U; },
      options,
      "re-enabled callback-control time-advance grant");
  auto const regulatorGrants = regulator.recorder().timeAdvanceGrants();
  auto const constrainedGrants = constrained.recorder().timeAdvanceGrants();
  require(
      regulatorGrants.front().encoded == encodeTime(*regulatorTarget) &&
          constrainedGrants.front().encoded == encodeTime(*constrainedTarget),
      "re-enabled time-advance callbacks returned the wrong target time");

  constrained.rtiAmbassador().disableTimeConstrained();
  regulator.rtiAmbassador().disableTimeRegulation();
  constrained.resign(rti::NO_ACTION);
  regulator.resign(rti::NO_ACTION);
  regulator.rtiAmbassador().destroyFederationExecution(federation);
  constrained.disconnect();
  regulator.disconnect();
}

void scenarioCallbackControlsTimeAdvanceContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioCallbackControlsTimeAdvance(options, model);
}

void scenarioCallbackControlsSaveRestore(
    Options const& options,
    rti::CallbackModel model) {
  Session owner(options, model, "owner");
  Session member(options, model, "member");
  auto const federation = federationName(
      options,
      "callback-controls-save-restore");
  connectAndJoin(owner, member, options, federation, options.fom);

  auto serviceCallbacks = [&](Session& session, std::string const& description) {
    if (model == rti::HLA_EVOKED) {
      static_cast<void>(session.evokeMultipleCallbacks(0.0, 1.0));
    } else {
      static_cast<void>(session.rtiAmbassador().getObjectClassHandle(
          options.objectClassName));
    }
    static_cast<void>(description);
  };

  auto const ownerHandle = owner.federateHandle();
  auto const memberHandle = member.federateHandle();
  std::wstring const saveLabel = L"tck-callback-controls-save-restore";

  auto const ownerSaveBefore = owner.federateSaveInitiations().size();
  auto const memberSaveBefore = member.federateSaveInitiations().size();
  member.rtiAmbassador().disableCallbacks();
  owner.rtiAmbassador().requestFederationSave(saveLabel);
  waitFor(
      owner,
      [&] {
        return owner.federateSaveInitiations().size() > ownerSaveBefore;
      },
      options,
      "callback-control save initiation for owner");
  auto const ownerSaveInitiations = owner.federateSaveInitiations();
  require(
      ownerSaveInitiations.back() == saveLabel &&
          owner.federateSaveTimes().empty(),
      "callback-control save initiation returned the wrong untimed metadata");

  serviceCallbacks(member, "callback-control save initiation while disabled");
  require(
      member.federateSaveInitiations().size() == memberSaveBefore,
      "disabled callbacks exposed a federate-save initiation");
  member.rtiAmbassador().enableCallbacks();
  serviceCallbacks(member, "callback-control save initiation after enable");
  waitFor(
      member,
      [&] {
        return member.federateSaveInitiations().size() > memberSaveBefore;
      },
      options,
      "callback-control save initiation for member");
  require(
      member.federateSaveInitiations().back() == saveLabel &&
          member.federateSaveTimes().empty(),
      "re-enabled callback-control save initiation returned the wrong metadata");

  owner.rtiAmbassador().federateSaveBegun();
  member.rtiAmbassador().federateSaveBegun();

  auto const saveStatusBefore = owner.federationSaveStatusResponses().size();
  owner.rtiAmbassador().disableCallbacks();
  owner.rtiAmbassador().queryFederationSaveStatus();
  serviceCallbacks(owner, "callback-control save status while disabled");
  require(
      owner.federationSaveStatusResponses().size() == saveStatusBefore,
      "disabled callbacks exposed a federation-save status response");
  owner.rtiAmbassador().enableCallbacks();
  serviceCallbacks(owner, "callback-control save status after enable");
  waitFor(
      owner,
      [&] {
        return owner.federationSaveStatusResponses().size() > saveStatusBefore;
      },
      options,
      "re-enabled callback-control save status response");
  auto const saveStatus = owner.federationSaveStatusResponses().back();
  require(
      saveStatus.size() == 2U,
      "callback-control save status omitted a joined federate");
  auto requireSaveStatus = [&](rti::FederateHandle const& handle,
                               rti::SaveStatus expected,
                               std::string const& description) {
    auto const found = std::find_if(
        saveStatus.begin(),
        saveStatus.end(),
        [&](auto const& entry) { return entry.first == handle; });
    require(found != saveStatus.end(), description + " omitted a federate");
    require(found->second == expected, description + " returned the wrong status");
  };
  requireSaveStatus(
      ownerHandle,
      rti::FEDERATE_SAVING,
      "owner callback-control save status");
  requireSaveStatus(
      memberHandle,
      rti::FEDERATE_SAVING,
      "member callback-control save status");

  auto const ownerSavedBefore = owner.federationSavedCount();
  auto const memberSavedBefore = member.federationSavedCount();
  member.rtiAmbassador().disableCallbacks();
  owner.rtiAmbassador().federateSaveComplete();
  member.rtiAmbassador().federateSaveComplete();
  waitFor(
      owner,
      [&] { return owner.federationSavedCount() > ownerSavedBefore; },
      options,
      "callback-control federation-saved callback for owner");
  serviceCallbacks(member, "callback-control federation-saved while disabled");
  require(
      member.federationSavedCount() == memberSavedBefore,
      "disabled callbacks exposed a federation-saved callback");
  member.rtiAmbassador().enableCallbacks();
  serviceCallbacks(member, "callback-control federation-saved after enable");
  waitFor(
      member,
      [&] { return member.federationSavedCount() > memberSavedBefore; },
      options,
      "re-enabled callback-control federation-saved callback");
  require(
      owner.federationNotSavedReasons().empty() &&
          member.federationNotSavedReasons().empty(),
      "successful callback-control save reported a failure");

  auto const ownerRestoreSucceededBefore =
      owner.federationRestoreRequestsSucceeded().size();
  auto const ownerRestoreBegunBefore = owner.federationRestoreBegunCount();
  auto const ownerRestoreInitiationBefore = owner.federateRestoreInitiations().size();
  auto const memberRestoreBegunBefore = member.federationRestoreBegunCount();
  auto const memberRestoreInitiationBefore = member.federateRestoreInitiations().size();
  member.rtiAmbassador().disableCallbacks();
  owner.rtiAmbassador().requestFederationRestore(saveLabel);
  waitFor(
      owner,
      [&] {
        return owner.federationRestoreRequestsSucceeded().size() >
                ownerRestoreSucceededBefore &&
            owner.federationRestoreBegunCount() > ownerRestoreBegunBefore &&
            owner.federateRestoreInitiations().size() >
                ownerRestoreInitiationBefore;
      },
      options,
      "callback-control restore initiation for owner");
  require(
      owner.federationRestoreRequestsSucceeded().back() == saveLabel &&
          owner.federationRestoreRequestsFailed().empty(),
      "callback-control restore request returned the wrong owner result");
  serviceCallbacks(member, "callback-control restore initiation while disabled");
  require(
      member.federationRestoreBegunCount() == memberRestoreBegunBefore &&
          member.federateRestoreInitiations().size() == memberRestoreInitiationBefore &&
          member.federationRestoreRequestsSucceeded().empty(),
      "disabled callbacks exposed a restore initiation callback");
  member.rtiAmbassador().enableCallbacks();
  serviceCallbacks(member, "callback-control restore initiation after enable");
  waitFor(
      member,
      [&] {
        return member.federationRestoreBegunCount() > memberRestoreBegunBefore &&
            member.federateRestoreInitiations().size() > memberRestoreInitiationBefore;
      },
      options,
      "re-enabled callback-control restore initiation");

  auto const ownerRestore = owner.federateRestoreInitiations().back();
  auto const memberRestore = member.federateRestoreInitiations().back();
  require(
      ownerRestore.label == saveLabel && memberRestore.label == saveLabel &&
          ownerRestore.federateName == options.ownerFederateName &&
          memberRestore.federateName == options.memberFederateName &&
          ownerRestore.postRestoreFederateHandle.isValid() &&
          memberRestore.postRestoreFederateHandle.isValid(),
      "callback-control restore initiation returned the wrong identity metadata");
  require(
      member.federationRestoreRequestsFailed().empty(),
      "callback-control member restore reported a request failure");

  auto const restoreStatusBefore = owner.federationRestoreStatusResponses().size();
  owner.rtiAmbassador().disableCallbacks();
  owner.rtiAmbassador().queryFederationRestoreStatus();
  serviceCallbacks(owner, "callback-control restore status while disabled");
  require(
      owner.federationRestoreStatusResponses().size() == restoreStatusBefore,
      "disabled callbacks exposed a federation-restore status response");
  owner.rtiAmbassador().enableCallbacks();
  serviceCallbacks(owner, "callback-control restore status after enable");
  waitFor(
      owner,
      [&] {
        return owner.federationRestoreStatusResponses().size() > restoreStatusBefore;
      },
      options,
      "re-enabled callback-control restore status response");
  auto const restoreStatus = owner.federationRestoreStatusResponses().back();
  require(
      restoreStatus.size() == 2U,
      "callback-control restore status omitted a joined federate");
  for (auto const& entry : restoreStatus) {
    require(
        entry.status == rti::FEDERATE_RESTORING,
        "callback-control restore status returned the wrong state");
  }

  auto const ownerRestoredBefore = owner.federationRestoredCount();
  auto const memberRestoredBefore = member.federationRestoredCount();
  member.rtiAmbassador().disableCallbacks();
  owner.rtiAmbassador().federateRestoreComplete();
  member.rtiAmbassador().federateRestoreComplete();
  waitFor(
      owner,
      [&] { return owner.federationRestoredCount() > ownerRestoredBefore; },
      options,
      "callback-control federation-restored callback for owner");
  serviceCallbacks(member, "callback-control federation-restored while disabled");
  require(
      member.federationRestoredCount() == memberRestoredBefore,
      "disabled callbacks exposed a federation-restored callback");
  member.rtiAmbassador().enableCallbacks();
  serviceCallbacks(member, "callback-control federation-restored after enable");
  waitFor(
      member,
      [&] { return member.federationRestoredCount() > memberRestoredBefore; },
      options,
      "re-enabled callback-control federation-restored callback");
  require(
      owner.federationNotRestoredReasons().empty() &&
          member.federationNotRestoredReasons().empty(),
      "successful callback-control restore reported a failure");

  member.resign(rti::NO_ACTION);
  owner.resign(rti::NO_ACTION);
  owner.rtiAmbassador().destroyFederationExecution(federation);
  member.disconnect();
  owner.disconnect();
}

void scenarioCallbackControlsSaveRestoreContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioCallbackControlsSaveRestore(options, model);
}

void scenarioCallbackControlsSaveRestoreFailures(
    Options const& options,
    rti::CallbackModel model) {
  Session owner(options, model, "owner");
  Session peer(options, model, "member");
  auto const federation = federationName(
      options,
      "callback-controls-save-restore-failures");
  connectAndJoin(owner, peer, options, federation, options.fom);

  auto serviceCallbacks = [&](Session& session) {
    if (model == rti::HLA_EVOKED) {
      static_cast<void>(session.evokeMultipleCallbacks(0.0, 1.0));
    } else {
      static_cast<void>(session.rtiAmbassador().getObjectClassHandle(
          options.objectClassName));
    }
  };

  auto const failedSaveLabel = L"tck-callback-controls-failed-save";
  auto const ownerSaveBefore = owner.federateSaveInitiations().size();
  auto const peerSaveBefore = peer.federateSaveInitiations().size();
  peer.rtiAmbassador().disableCallbacks();
  owner.rtiAmbassador().requestFederationSave(failedSaveLabel);
  waitFor(
      owner,
      [&] {
        return owner.federateSaveInitiations().size() > ownerSaveBefore;
      },
      options,
      "callback-control failed-save initiation for owner");
  serviceCallbacks(peer);
  require(
      peer.federateSaveInitiations().size() == peerSaveBefore,
      "disabled callbacks exposed a failed-save initiation");
  peer.rtiAmbassador().enableCallbacks();
  serviceCallbacks(peer);
  waitFor(
      peer,
      [&] {
        return peer.federateSaveInitiations().size() > peerSaveBefore;
      },
      options,
      "re-enabled callback-control failed-save initiation");

  owner.rtiAmbassador().federateSaveBegun();
  peer.rtiAmbassador().federateSaveBegun();
  auto const ownerSaveFailureBefore = owner.federationNotSavedReasons().size();
  auto const peerSaveFailureBefore = peer.federationNotSavedReasons().size();
  peer.rtiAmbassador().disableCallbacks();
  peer.rtiAmbassador().federateSaveNotComplete();
  waitFor(
      owner,
      [&] {
        return owner.federationNotSavedReasons().size() > ownerSaveFailureBefore;
      },
      options,
      "callback-control federation-not-saved owner callback");
  serviceCallbacks(peer);
  require(
      peer.federationNotSavedReasons().size() == peerSaveFailureBefore,
      "disabled callbacks exposed federation-not-saved");
  peer.rtiAmbassador().enableCallbacks();
  serviceCallbacks(peer);
  waitFor(
      peer,
      [&] {
        return peer.federationNotSavedReasons().size() > peerSaveFailureBefore;
      },
      options,
      "re-enabled callback-control federation-not-saved callback");
  require(
      owner.federationNotSavedReasons().back() ==
              rti::FEDERATE_REPORTED_FAILURE_DURING_SAVE &&
          peer.federationNotSavedReasons().back() ==
              rti::FEDERATE_REPORTED_FAILURE_DURING_SAVE,
      "federation-not-saved returned the wrong failure reason");

  auto const successfulSaveLabel = L"tck-callback-controls-restore-source";
  auto const ownerSuccessfulSaveBefore = owner.federateSaveInitiations().size();
  auto const peerSuccessfulSaveBefore = peer.federateSaveInitiations().size();
  owner.rtiAmbassador().requestFederationSave(successfulSaveLabel);
  waitForSessions(
      {&owner, &peer},
      [&] {
        return owner.federateSaveInitiations().size() > ownerSuccessfulSaveBefore &&
            peer.federateSaveInitiations().size() > peerSuccessfulSaveBefore;
      },
      options,
      "callback-control restore-source save initiation");
  owner.rtiAmbassador().federateSaveBegun();
  peer.rtiAmbassador().federateSaveBegun();
  auto const ownerSavedBefore = owner.federationSavedCount();
  auto const peerSavedBefore = peer.federationSavedCount();
  owner.rtiAmbassador().federateSaveComplete();
  peer.rtiAmbassador().federateSaveComplete();
  waitForSessions(
      {&owner, &peer},
      [&] {
        return owner.federationSavedCount() > ownerSavedBefore &&
            peer.federationSavedCount() > peerSavedBefore;
      },
      options,
      "callback-control restore-source save completion");

  auto const ownerRestoreRequestBefore =
      owner.federationRestoreRequestsSucceeded().size();
  auto const ownerRestoreBegunBefore = owner.federationRestoreBegunCount();
  auto const peerRestoreBegunBefore = peer.federationRestoreBegunCount();
  auto const ownerRestoreInitiationBefore = owner.federateRestoreInitiations().size();
  auto const peerRestoreInitiationBefore = peer.federateRestoreInitiations().size();
  owner.rtiAmbassador().requestFederationRestore(successfulSaveLabel);
  waitForSessions(
      {&owner, &peer},
      [&] {
        return owner.federationRestoreRequestsSucceeded().size() >
                ownerRestoreRequestBefore &&
            owner.federationRestoreBegunCount() > ownerRestoreBegunBefore &&
            peer.federationRestoreBegunCount() > peerRestoreBegunBefore &&
            owner.federateRestoreInitiations().size() >
                ownerRestoreInitiationBefore &&
            peer.federateRestoreInitiations().size() > peerRestoreInitiationBefore;
      },
      options,
      "callback-control restore-failure initiation");

  owner.rtiAmbassador().federateRestoreComplete();
  auto const ownerRestoreFailureBefore = owner.federationNotRestoredReasons().size();
  auto const peerRestoreFailureBefore = peer.federationNotRestoredReasons().size();
  peer.rtiAmbassador().disableCallbacks();
  peer.rtiAmbassador().federateRestoreNotComplete();
  waitFor(
      owner,
      [&] {
        return owner.federationNotRestoredReasons().size() > ownerRestoreFailureBefore;
      },
      options,
      "callback-control federation-not-restored owner callback");
  serviceCallbacks(peer);
  require(
      peer.federationNotRestoredReasons().size() == peerRestoreFailureBefore,
      "disabled callbacks exposed federation-not-restored");
  peer.rtiAmbassador().enableCallbacks();
  serviceCallbacks(peer);
  waitFor(
      peer,
      [&] {
        return peer.federationNotRestoredReasons().size() > peerRestoreFailureBefore;
      },
      options,
      "re-enabled callback-control federation-not-restored callback");
  require(
      owner.federationNotRestoredReasons().back() ==
              rti::FEDERATE_REPORTED_FAILURE_DURING_RESTORE &&
          peer.federationNotRestoredReasons().back() ==
              rti::FEDERATE_REPORTED_FAILURE_DURING_RESTORE,
      "federation-not-restored returned the wrong failure reason");

  peer.resign(rti::NO_ACTION);
  owner.resign(rti::NO_ACTION);
  owner.rtiAmbassador().destroyFederationExecution(federation);
  peer.disconnect();
  owner.disconnect();
}

void scenarioCallbackControlsSaveRestoreFailuresContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioCallbackControlsSaveRestoreFailures(options, model);
}

void scenarioCallbackControlsRestoreRequestFailure(
    Options const& options,
    rti::CallbackModel model) {
  Session requester(options, model, "owner");
  Session observer(options, model, "member");
  auto const federation = federationName(
      options,
      "callback-controls-restore-request-failure");
  connectAndJoin(requester, observer, options, federation, options.fom);

  auto serviceCallbacks = [&](Session& session) {
    if (model == rti::HLA_EVOKED) {
      static_cast<void>(session.evokeMultipleCallbacks(0.0, 1.0));
    } else {
      static_cast<void>(session.rtiAmbassador().getObjectClassHandle(
          options.objectClassName));
    }
  };

  std::wstring const missingSaveLabel =
      L"tck-callback-controls-missing-restore";
  auto const requesterFailureBefore =
      requester.federationRestoreRequestsFailed().size();
  auto const observerFailureBefore =
      observer.federationRestoreRequestsFailed().size();
  auto const requesterRestoreBegunBefore = requester.federationRestoreBegunCount();
  auto const observerRestoreBegunBefore = observer.federationRestoreBegunCount();
  auto const requesterRestoreInitiationBefore =
      requester.federateRestoreInitiations().size();
  auto const observerRestoreInitiationBefore =
      observer.federateRestoreInitiations().size();

  requester.rtiAmbassador().disableCallbacks();
  requester.rtiAmbassador().requestFederationRestore(missingSaveLabel);
  serviceCallbacks(requester);
  require(
      requester.federationRestoreRequestsFailed().size() == requesterFailureBefore,
      "disabled callbacks exposed a restore-request failure");
  require(
      observer.federationRestoreRequestsFailed().size() == observerFailureBefore,
      "restore-request failure was delivered to a non-requesting federate");
  requester.rtiAmbassador().enableCallbacks();
  serviceCallbacks(requester);
  waitFor(
      requester,
      [&] {
        return requester.federationRestoreRequestsFailed().size() >
            requesterFailureBefore;
      },
      options,
      "re-enabled callback-control restore-request failure");

  require(
      requester.federationRestoreRequestsFailed().back() == missingSaveLabel,
      "restore-request failure returned the wrong save label");
  require(
      requester.federationRestoreRequestsSucceeded().empty() &&
          observer.federationRestoreRequestsSucceeded().empty() &&
          requester.federationRestoreBegunCount() == requesterRestoreBegunBefore &&
          observer.federationRestoreBegunCount() == observerRestoreBegunBefore &&
          requester.federateRestoreInitiations().size() ==
              requesterRestoreInitiationBefore &&
          observer.federateRestoreInitiations().size() ==
              observerRestoreInitiationBefore,
      "failed restore request unexpectedly started a federation restore");

  requester.resign(rti::NO_ACTION);
  observer.resign(rti::NO_ACTION);
  requester.rtiAmbassador().destroyFederationExecution(federation);
  observer.disconnect();
  requester.disconnect();
}

void scenarioCallbackControlsRestoreRequestFailureContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioCallbackControlsRestoreRequestFailure(options, model);
}

void scenarioCallbackControlsTimestampedAttributeUpdate(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.logicalTimeImplementationName.empty(),
      "Callback-control timestamped attribute testing requires an adapter-supplied logical-time implementation");

  Session owner(options, model, "owner");
  Session constrained(options, model, "member");
  auto const federation = federationName(
      options,
      "callback-controls-timestamped-attribute-update");
  connectAndJoin(owner, constrained, options, federation, options.fom);

  auto const ownerClass = owner.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const constrainedClass = constrained.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const ownerAttribute = owner.rtiAmbassador().getAttributeHandle(
      ownerClass,
      options.attributeName);
  auto const constrainedAttribute = constrained.rtiAmbassador().getAttributeHandle(
      constrainedClass,
      options.attributeName);
  require(
      ownerClass.isValid() && constrainedClass.isValid() &&
          ownerAttribute.isValid() && constrainedAttribute.isValid(),
      "callback-control timestamped attribute lookup returned an invalid handle");

  rti::AttributeHandleSet const ownerAttributes{ownerAttribute};
  rti::AttributeHandleSet const constrainedAttributes{constrainedAttribute};
  owner.rtiAmbassador().publishObjectClassAttributes(ownerClass, ownerAttributes);
  owner.rtiAmbassador().changeDefaultAttributeOrderType(
      ownerClass,
      ownerAttributes,
      rti::TIMESTAMP);
  constrained.rtiAmbassador().subscribeObjectClassAttributes(
      constrainedClass,
      constrainedAttributes,
      true,
      L"");

  auto const object = owner.rtiAmbassador().registerObjectInstance(ownerClass);
  require(
      object.isValid(),
      "callback-control timestamped attribute registration returned an invalid object");
  waitFor(
      constrained,
      [&] { return constrained.recorder().discovery().present; },
      options,
      "callback-control timestamped attribute discovery");
  require(
      constrained.recorder().discovery().object == object,
      "callback-control timestamped attribute discovery returned the wrong object");

  auto ownerTime = makeTimeContext(owner);
  auto constrainedTime = makeTimeContext(constrained);
  enableTimestampedRoles(
      owner,
      constrained,
      ownerTime,
      constrainedTime,
      options,
      "callback-control timestamped attribute");
  auto const target = timeAfter(
      *ownerTime.factory,
      *ownerTime.initial,
      *ownerTime.epsilon,
      4U);
  auto const constrainedTarget = timeAfter(
      *constrainedTime.factory,
      *constrainedTime.initial,
      *constrainedTime.epsilon,
      4U);
  std::vector<std::uint8_t> const valueBytes{0x41U, 0x54U, 0x54U};
  std::vector<std::uint8_t> const tagBytes{0x54U, 0x53U, 0x4FU};
  rti::AttributeHandleValueMap values;
  values.emplace(
      ownerAttribute,
      rti::VariableLengthData(valueBytes.data(), valueBytes.size()));
  rti::VariableLengthData tag(tagBytes.data(), tagBytes.size());

  constrained.rtiAmbassador().disableCallbacks();
  auto const retraction = owner.rtiAmbassador().updateAttributeValues(
      object,
      values,
      tag,
      *target);
  require(
      retraction.isValid(),
      "callback-control timestamped attribute update returned an invalid retraction handle");
  constrained.rtiAmbassador().timeAdvanceRequest(*constrainedTarget);
  owner.rtiAmbassador().timeAdvanceRequest(*target);
  waitFor(
      owner,
      [&] { return owner.recorder().timeAdvanceGrants().size() == 1U; },
      options,
      "callback-control timestamped attribute owner grant");

  if (model == rti::HLA_EVOKED) {
    static_cast<void>(constrained.evokeMultipleCallbacks(0.0, 1.0));
  } else {
    static_cast<void>(constrained.rtiAmbassador().getObjectClassHandle(
        options.objectClassName));
  }
  require(
      constrained.recorder().timedReflections().empty() &&
          constrained.recorder().timeAdvanceGrants().empty(),
      "disabled callbacks exposed timestamped attribute delivery or its grant");

  constrained.rtiAmbassador().enableCallbacks();
  if (model == rti::HLA_EVOKED) {
    static_cast<void>(constrained.evokeMultipleCallbacks(0.0, 1.0));
  } else {
    static_cast<void>(constrained.rtiAmbassador().getObjectClassHandle(
        options.objectClassName));
  }
  waitFor(
      constrained,
      [&] {
        return constrained.recorder().timedReflections().size() == 1U &&
            constrained.recorder().timeAdvanceGrants().size() == 1U;
      },
      options,
      "re-enabled callback-control timestamped attribute delivery");
  auto const reflections = constrained.recorder().timedReflections();
  require(
      reflections.size() == 1U,
      "re-enabled callback-control timestamped attribute delivered duplicates");
  auto const& reflection = reflections.front();
  require(
      reflection.object == object &&
          reflection.values.size() == 1U &&
          reflection.values.count(constrainedAttribute) == 1U &&
          copyBytes(reflection.values.at(constrainedAttribute)) == valueBytes &&
          reflection.tag == tagBytes &&
          reflection.producer == owner.federateHandle() &&
          reflection.time == encodeTime(*target) &&
          reflection.sentOrder == rti::TIMESTAMP &&
          reflection.receivedOrder == rti::TIMESTAMP &&
          reflection.transportation.isValid() &&
          !constrained.rtiAmbassador().getTransportationTypeName(
              reflection.transportation).empty() &&
          !reflection.regions.has_value() &&
          reflection.retractionPresent &&
          reflection.retraction == copyBytes(retraction.encode()),
      "re-enabled timestamped attribute reflection returned the wrong metadata");
  require(
      constrained.recorder().callbackOrder() ==
          std::vector<std::string>{"reflect", "grant"},
      "re-enabled timestamped attribute reflection crossed its grant boundary");

  constrained.rtiAmbassador().disableTimeConstrained();
  owner.rtiAmbassador().disableTimeRegulation();
  constrained.rtiAmbassador().unsubscribeObjectClassAttributes(
      constrainedClass,
      constrainedAttributes);
  owner.rtiAmbassador().unpublishObjectClassAttributes(
      ownerClass,
      ownerAttributes);
  constrained.resign(rti::NO_ACTION);
  owner.resign(rti::NO_ACTION);
  owner.rtiAmbassador().destroyFederationExecution(federation);
  constrained.disconnect();
  owner.disconnect();
}

void scenarioCallbackControlsTimestampedAttributeUpdateContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioCallbackControlsTimestampedAttributeUpdate(options, model);
}

void scenarioCallbackControlsTimestampedRegionalInteraction(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.logicalTimeImplementationName.empty(),
      "Callback-control timestamped regional interaction testing requires an adapter-supplied logical-time implementation");
  require(
      !options.ddmFom.empty(),
      "Callback-control timestamped regional interaction testing requires an adapter-supplied dimensional FOM");

  Session publisher(options, model, "owner");
  Session receiver(options, model, "member");
  auto const federation = federationName(
      options,
      "callback-controls-timestamped-regional-interaction");
  connectAndJoin(publisher, receiver, options, federation, options.ddmFom);

  auto const publisherHandles = ddmHandles(publisher, options);
  auto const receiverHandles = ddmHandles(receiver, options);
  verifyDdmClassDimensions(
      publisher,
      options,
      publisherHandles,
      "callback-control timestamped regional interaction");

  require(
      publisherHandles.interactionClass.isValid() &&
          receiverHandles.interactionClass.isValid() &&
          publisherHandles.parameter.isValid() &&
          receiverHandles.parameter.isValid(),
      "callback-control timestamped regional interaction lookup returned an invalid handle");

  publisher.rtiAmbassador().publishInteractionClass(
      publisherHandles.interactionClass);
  publisher.rtiAmbassador().changeInteractionOrderType(
      publisherHandles.interactionClass,
      rti::TIMESTAMP);

  auto const sourceRegion = createDdmRegion(
      publisher,
      publisherHandles,
      1UL,
      5UL,
      1UL,
      5UL);
  auto const receiverRegion = createDdmRegion(
      receiver,
      receiverHandles,
      2UL,
      6UL,
      2UL,
      6UL);
  auto const sourceRegionSet = rti::RegionHandleSet{sourceRegion};
  auto const receiverRegionSet = rti::RegionHandleSet{receiverRegion};
  receiver.rtiAmbassador().setConveyRegionDesignatorSetsSwitch(true);
  receiver.rtiAmbassador().subscribeInteractionClassWithRegions(
      receiverHandles.interactionClass,
      receiverRegionSet,
      true);

  auto publisherTime = makeTimeContext(publisher);
  auto receiverTime = makeTimeContext(receiver);
  enableTimestampedRoles(
      publisher,
      receiver,
      publisherTime,
      receiverTime,
      options,
      "callback-control timestamped regional interaction");

  auto const interactionTime = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      4U);
  auto const receiverTarget = timeAfter(
      *receiverTime.factory,
      *receiverTime.initial,
      *receiverTime.epsilon,
      4U);

  std::vector<std::uint8_t> const parameterBytes{0x52U, 0x49U, 0x47U};
  std::vector<std::uint8_t> const tagBytes{0x52U, 0x47U, 0x43U};
  rti::ParameterHandleValueMap parameters;
  parameters.emplace(
      publisherHandles.parameter,
      rti::VariableLengthData(
          parameterBytes.data(),
          parameterBytes.size()));
  rti::VariableLengthData tag(tagBytes.data(), tagBytes.size());

  receiver.rtiAmbassador().disableCallbacks();
  auto const retraction = publisher.rtiAmbassador().sendInteractionWithRegions(
      publisherHandles.interactionClass,
      parameters,
      sourceRegionSet,
      tag,
      *interactionTime);
  require(
      retraction.isValid(),
      "callback-control timestamped regional interaction returned an invalid retraction handle");

  receiver.rtiAmbassador().timeAdvanceRequest(*receiverTarget);
  publisher.rtiAmbassador().timeAdvanceRequest(*interactionTime);
  waitFor(
      publisher,
      [&] { return publisher.recorder().timeAdvanceGrants().size() == 1U; },
      options,
      "callback-control timestamped regional interaction publisher grant");

  if (model == rti::HLA_EVOKED) {
    static_cast<void>(receiver.evokeMultipleCallbacks(0.0, 1.0));
  } else {
    static_cast<void>(receiver.rtiAmbassador().getInteractionClassHandle(
        options.interactionClassName));
  }
  require(
      receiver.recorder().timedInteractions().empty() &&
          receiver.recorder().timeAdvanceGrants().empty(),
      "disabled callbacks exposed timestamped regional interaction or its grant");

  receiver.rtiAmbassador().enableCallbacks();
  if (model == rti::HLA_EVOKED) {
    static_cast<void>(receiver.evokeMultipleCallbacks(0.0, 1.0));
  } else {
    static_cast<void>(receiver.rtiAmbassador().getInteractionClassHandle(
        options.interactionClassName));
  }
  waitFor(
      receiver,
      [&] {
        return receiver.recorder().timedInteractions().size() == 1U &&
            receiver.recorder().timeAdvanceGrants().size() == 1U;
      },
      options,
      "re-enabled callback-control timestamped regional interaction");

  auto const interactions = receiver.recorder().timedInteractions();
  require(
      interactions.size() == 1U,
      "re-enabled callback-control timestamped regional interaction delivered duplicates");
  auto const& interaction = interactions.front();
  require(
      interaction.interaction == receiverHandles.interactionClass &&
          interaction.parameters.size() == 1U &&
          interaction.parameters.count(receiverHandles.parameter) == 1U &&
          copyBytes(interaction.parameters.at(receiverHandles.parameter)) ==
              parameterBytes &&
          interaction.tag == tagBytes &&
          interaction.producer == publisher.federateHandle() &&
          interaction.time == encodeTime(*interactionTime) &&
          !interaction.timeText.empty() &&
          interaction.sentOrder == rti::TIMESTAMP &&
          interaction.receivedOrder == rti::TIMESTAMP &&
          interaction.transportation.isValid() &&
          !receiver.rtiAmbassador().getTransportationTypeName(
              interaction.transportation).empty() &&
          interaction.retractionPresent &&
          interaction.retraction == copyBytes(retraction.encode()) &&
          interaction.regions.has_value() &&
          interaction.regions->size() == 1U &&
          interaction.regions->count(sourceRegion) == 1U,
      "re-enabled timestamped regional interaction returned the wrong metadata");
  require(
      receiver.recorder().callbackOrder() ==
          std::vector<std::string>{"interaction", "grant"},
      "re-enabled timestamped regional interaction crossed its grant boundary");

  receiver.rtiAmbassador().disableTimeConstrained();
  publisher.rtiAmbassador().disableTimeRegulation();
  receiver.rtiAmbassador().unsubscribeInteractionClassWithRegions(
      receiverHandles.interactionClass,
      receiverRegionSet);
  publisher.rtiAmbassador().unpublishInteractionClass(
      publisherHandles.interactionClass);
  receiver.rtiAmbassador().deleteRegion(receiverRegion);
  publisher.rtiAmbassador().deleteRegion(sourceRegion);
  receiver.resign(rti::NO_ACTION);
  publisher.resign(rti::NO_ACTION);
  publisher.rtiAmbassador().destroyFederationExecution(federation);
  receiver.disconnect();
  publisher.disconnect();
}

void scenarioCallbackControlsTimestampedRegionalInteractionContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioCallbackControlsTimestampedRegionalInteraction(options, model);
}

void scenarioCallbackControlsTimestampedObjectRemoval(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.logicalTimeImplementationName.empty(),
      "Callback-control timestamped object-removal testing requires an adapter-supplied logical-time implementation");

  Session owner(options, model, "owner");
  Session constrained(options, model, "member");
  auto const federation = federationName(
      options,
      "callback-controls-timestamped-object-removal");
  connectAndJoin(owner, constrained, options, federation, options.fom);

  auto const ownerClass = owner.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const constrainedClass = constrained.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const ownerAttribute = owner.rtiAmbassador().getAttributeHandle(
      ownerClass,
      options.attributeName);
  auto const constrainedAttribute = constrained.rtiAmbassador().getAttributeHandle(
      constrainedClass,
      options.attributeName);
  require(
      ownerClass.isValid() && constrainedClass.isValid() &&
          ownerAttribute.isValid() && constrainedAttribute.isValid(),
      "callback-control timestamped object-removal lookup returned an invalid handle");

  rti::AttributeHandleSet const ownerAttributes{ownerAttribute};
  rti::AttributeHandleSet const constrainedAttributes{constrainedAttribute};
  owner.rtiAmbassador().publishObjectClassAttributes(ownerClass, ownerAttributes);
  constrained.rtiAmbassador().subscribeObjectClassAttributes(
      constrainedClass,
      constrainedAttributes,
      true,
      L"");

  auto const object = owner.rtiAmbassador().registerObjectInstance(ownerClass);
  require(
      object.isValid(),
      "callback-control timestamped object-removal registration returned an invalid object");
  auto const objectName = owner.rtiAmbassador().getObjectInstanceName(object);
  require(
      !objectName.empty(),
      "callback-control timestamped object-removal registration returned an empty name");
  waitFor(
      constrained,
      [&] { return constrained.recorder().discovery().present; },
      options,
      "callback-control timestamped object-removal discovery");
  require(
      constrained.recorder().discovery().object == object,
      "callback-control timestamped object-removal discovery returned the wrong object");

  auto ownerTime = makeTimeContext(owner);
  auto constrainedTime = makeTimeContext(constrained);
  enableTimestampedRoles(
      owner,
      constrained,
      ownerTime,
      constrainedTime,
      options,
      "callback-control timestamped object-removal");
  auto const target = timeAfter(
      *ownerTime.factory,
      *ownerTime.initial,
      *ownerTime.epsilon,
      4U);
  auto const constrainedTarget = timeAfter(
      *constrainedTime.factory,
      *constrainedTime.initial,
      *constrainedTime.epsilon,
      4U);
  std::vector<std::uint8_t> const tagBytes{0x52U, 0x4DU, 0x56U};
  rti::VariableLengthData tag(tagBytes.data(), tagBytes.size());

  constrained.rtiAmbassador().disableCallbacks();
  auto const retraction = owner.rtiAmbassador().deleteObjectInstance(
      object,
      tag,
      *target);
  require(
      retraction.isValid(),
      "callback-control timestamped object removal returned an invalid retraction handle");
  constrained.rtiAmbassador().timeAdvanceRequest(*constrainedTarget);
  owner.rtiAmbassador().timeAdvanceRequest(*target);
  waitFor(
      owner,
      [&] { return owner.recorder().timeAdvanceGrants().size() == 1U; },
      options,
      "callback-control timestamped object-removal owner grant");

  if (model == rti::HLA_EVOKED) {
    static_cast<void>(constrained.evokeMultipleCallbacks(0.0, 1.0));
  } else {
    static_cast<void>(constrained.rtiAmbassador().getObjectClassHandle(
        options.objectClassName));
  }
  require(
      constrained.recorder().timedRemovals().empty() &&
          constrained.recorder().timeAdvanceGrants().empty(),
      "disabled callbacks exposed timestamped object removal or its grant");
  require(
      constrained.rtiAmbassador().getObjectInstanceHandle(objectName) == object,
      "disabled timestamped object removal changed object identity before delivery");

  constrained.rtiAmbassador().enableCallbacks();
  if (model == rti::HLA_EVOKED) {
    static_cast<void>(constrained.evokeMultipleCallbacks(0.0, 1.0));
  } else {
    static_cast<void>(constrained.rtiAmbassador().getObjectClassHandle(
        options.objectClassName));
  }
  waitFor(
      constrained,
      [&] {
        return constrained.recorder().timedRemovals().size() == 1U &&
            constrained.recorder().timeAdvanceGrants().size() == 1U;
      },
      options,
      "re-enabled callback-control timestamped object removal");
  auto const removals = constrained.recorder().timedRemovals();
  require(
      removals.size() == 1U,
      "re-enabled callback-control timestamped object removal delivered duplicates");
  auto const& removal = removals.front();
  require(
      removal.object == object &&
          removal.tag == tagBytes &&
          removal.producer == owner.federateHandle() &&
          removal.time == encodeTime(*target) &&
          removal.sentOrder == rti::TIMESTAMP &&
          removal.receivedOrder == rti::TIMESTAMP &&
          removal.retractionPresent &&
          removal.retraction == copyBytes(retraction.encode()),
      "re-enabled timestamped object removal returned the wrong metadata");
  require(
      constrained.recorder().callbackOrder() ==
          std::vector<std::string>{"remove", "grant"},
      "re-enabled timestamped object removal crossed its grant boundary");
  requireException(
      [&] {
        static_cast<void>(constrained.rtiAmbassador().getObjectInstanceHandle(objectName));
      },
      L"ObjectInstanceNotKnown",
      "timestamped object removal did not retire the object name after delivery");

  constrained.rtiAmbassador().disableTimeConstrained();
  owner.rtiAmbassador().disableTimeRegulation();
  constrained.rtiAmbassador().unsubscribeObjectClassAttributes(
      constrainedClass,
      constrainedAttributes);
  owner.rtiAmbassador().unpublishObjectClassAttributes(
      ownerClass,
      ownerAttributes);
  constrained.resign(rti::NO_ACTION);
  owner.resign(rti::NO_ACTION);
  owner.rtiAmbassador().destroyFederationExecution(federation);
  constrained.disconnect();
  owner.disconnect();
}

void scenarioCallbackControlsTimestampedObjectRemovalContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioCallbackControlsTimestampedObjectRemoval(options, model);
}

void scenarioCallbackControlsTimestampedRetraction(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.logicalTimeImplementationName.empty(),
      "Callback-control timestamped retraction testing requires an adapter-supplied logical-time implementation");

  Session publisher(options, model, "owner");
  Session receiver(options, model, "member");
  auto const federation = federationName(
      options,
      "callback-controls-timestamped-retraction");
  connectAndJoin(publisher, receiver, options, federation, options.fom);

  auto const publisherInteraction = publisher.rtiAmbassador().getInteractionClassHandle(
      options.interactionClassName);
  auto const receiverInteraction = receiver.rtiAmbassador().getInteractionClassHandle(
      options.interactionClassName);
  auto const publisherParameter = publisher.rtiAmbassador().getParameterHandle(
      publisherInteraction,
      options.parameterName);
  auto const receiverParameter = receiver.rtiAmbassador().getParameterHandle(
      receiverInteraction,
      options.parameterName);
  require(
      publisherInteraction.isValid() && receiverInteraction.isValid() &&
          publisherParameter.isValid() && receiverParameter.isValid() &&
          publisherInteraction == receiverInteraction &&
          publisherParameter == receiverParameter,
      "callback-control timestamped retraction lookup returned mismatched handles");

  publisher.rtiAmbassador().publishInteractionClass(publisherInteraction);
  publisher.rtiAmbassador().changeInteractionOrderType(
      publisherInteraction,
      rti::TIMESTAMP);
  receiver.rtiAmbassador().subscribeInteractionClass(receiverInteraction, true);

  auto publisherTime = makeTimeContext(publisher);
  publisher.rtiAmbassador().enableTimeRegulation(*publisherTime.epsilon);
  waitFor(
      publisher,
      [&] { return publisher.recorder().timeRegulationEnabled().size() == 1U; },
      options,
      "callback-control timestamped retraction time-regulation callback");
  auto const target = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      3U);
  std::vector<std::uint8_t> const valueBytes{0x52U, 0x45U, 0x54U};
  std::vector<std::uint8_t> const tagBytes{0x52U, 0x45U, 0x54U};
  rti::ParameterHandleValueMap parameters;
  parameters.emplace(
      publisherParameter,
      rti::VariableLengthData(valueBytes.data(), valueBytes.size()));
  rti::VariableLengthData tag(tagBytes.data(), tagBytes.size());
  auto const retraction = publisher.rtiAmbassador().sendInteraction(
      publisherInteraction,
      parameters,
      tag,
      *target);
  require(
      retraction.isValid(),
      "callback-control timestamped retraction send returned an invalid handle");
  waitFor(
      receiver,
      [&] { return receiver.recorder().timedInteractions().size() == 1U; },
      options,
      "callback-control timestamped retraction delivery");
  auto const interactions = receiver.recorder().timedInteractions();
  require(
      interactions.size() == 1U,
      "callback-control timestamped retraction delivered duplicate interactions");
  auto const& interaction = interactions.front();
  require(
      interaction.interaction == receiverInteraction &&
          interaction.parameters.size() == 1U &&
          interaction.parameters.count(receiverParameter) == 1U &&
          copyBytes(interaction.parameters.at(receiverParameter)) == valueBytes &&
          interaction.tag == tagBytes &&
          interaction.producer == publisher.federateHandle() &&
          interaction.time == encodeTime(*target) &&
          interaction.sentOrder == rti::TIMESTAMP &&
          interaction.receivedOrder == rti::RECEIVE &&
          interaction.transportation.isValid() &&
          !receiver.rtiAmbassador().getTransportationTypeName(
              interaction.transportation).empty() &&
          interaction.retractionPresent &&
          interaction.retraction == copyBytes(retraction.encode()),
      "callback-control timestamped retraction delivery returned the wrong metadata");

  receiver.rtiAmbassador().disableCallbacks();
  publisher.rtiAmbassador().retract(retraction);
  if (model == rti::HLA_EVOKED) {
    static_cast<void>(receiver.evokeMultipleCallbacks(0.0, 1.0));
  } else {
    static_cast<void>(receiver.rtiAmbassador().getObjectClassHandle(
        options.objectClassName));
  }
  require(
      receiver.recorder().retractions().empty(),
      "disabled callbacks exposed a request-retraction callback");

  receiver.rtiAmbassador().enableCallbacks();
  if (model == rti::HLA_EVOKED) {
    static_cast<void>(receiver.evokeMultipleCallbacks(0.0, 1.0));
  } else {
    static_cast<void>(receiver.rtiAmbassador().getObjectClassHandle(
        options.objectClassName));
  }
  waitFor(
      receiver,
      [&] { return receiver.recorder().retractions().size() == 1U; },
      options,
      "re-enabled callback-control request-retraction delivery");
  auto const callbacks = receiver.recorder().retractions();
  require(
      callbacks.size() == 1U && callbacks.front().valid &&
          callbacks.front().encoded == copyBytes(retraction.encode()),
      "re-enabled request-retraction callback returned the wrong handle");
  require(
      receiver.recorder().callbackOrder() ==
          std::vector<std::string>{"interaction", "request-retraction"},
      "request-retraction callback was delivered in the wrong order");

  publisher.rtiAmbassador().disableTimeRegulation();
  receiver.rtiAmbassador().unsubscribeInteractionClass(receiverInteraction);
  publisher.rtiAmbassador().unpublishInteractionClass(publisherInteraction);
  receiver.resign(rti::NO_ACTION);
  publisher.resign(rti::NO_ACTION);
  publisher.rtiAmbassador().destroyFederationExecution(federation);
  receiver.disconnect();
  publisher.disconnect();
}

void scenarioCallbackControlsTimestampedRetractionContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioCallbackControlsTimestampedRetraction(options, model);
}

void scenarioCallbackControlsTransportation(
    Options const& options,
    rti::CallbackModel model) {
  Session owner(options, model, "owner");
  Session observer(options, model, "member");
  auto const federation = federationName(
      options,
      "callback-controls-transportation");
  connectAndJoin(owner, observer, options, federation, options.fom);

  rti::ObjectClassHandle ownerClass;
  rti::AttributeHandle ownerAttribute;
  rti::InteractionClassHandle ownerInteraction;
  rti::ParameterHandle ownerParameter;
  handles(owner, options, ownerClass, ownerAttribute, ownerInteraction, ownerParameter);
  auto const observerClass = observer.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const observerAttribute = observer.rtiAmbassador().getAttributeHandle(
      observerClass,
      options.attributeName);
  auto const observerInteraction = observer.rtiAmbassador().getInteractionClassHandle(
      options.interactionClassName);
  require(
      ownerClass.isValid() && ownerAttribute.isValid() &&
          ownerInteraction.isValid() && ownerParameter.isValid() &&
          observerClass.isValid() && observerAttribute.isValid() &&
          observerInteraction.isValid() && ownerClass == observerClass &&
          ownerAttribute == observerAttribute &&
          ownerInteraction == observerInteraction,
      "callback-control transportation lookup returned mismatched handles");

  auto const reliable = owner.rtiAmbassador().getTransportationTypeHandle(
      L"HLAreliable");
  auto const bestEffort = owner.rtiAmbassador().getTransportationTypeHandle(
      L"HLAbestEffort");
  require(
      reliable.isValid() && bestEffort.isValid() && reliable != bestEffort,
      "callback-control transportation lookup did not return distinct standard handles");
  auto sameTransport = [](rti::RTIambassador& inspector,
                          rti::TransportationTypeHandle const& actual,
                          std::wstring const& expectedName) {
    return actual.isValid() &&
        inspector.getTransportationTypeName(actual) == expectedName;
  };

  rti::AttributeHandleSet const ownerAttributes{ownerAttribute};
  rti::AttributeHandleSet const observerAttributes{observerAttribute};
  owner.rtiAmbassador().publishObjectClassAttributes(ownerClass, ownerAttributes);
  observer.rtiAmbassador().subscribeObjectClassAttributes(
      observerClass,
      observerAttributes,
      true,
      L"");
  owner.rtiAmbassador().publishInteractionClass(ownerInteraction);
  observer.rtiAmbassador().subscribeInteractionClass(observerInteraction, true);
  auto const object = owner.rtiAmbassador().registerObjectInstance(ownerClass);
  require(
      object.isValid(),
      "callback-control transportation registration returned an invalid object");
  waitFor(
      observer,
      [&] { return observer.recorder().hasDiscovery(object); },
      options,
      "callback-control transportation object discovery");

  auto serviceOwnerCallbacks = [&] {
    if (model == rti::HLA_EVOKED) {
      static_cast<void>(owner.evokeMultipleCallbacks(0.0, 1.0));
    } else {
      static_cast<void>(owner.rtiAmbassador().getObjectClassHandle(
          options.objectClassName));
    }
  };

  owner.recorder().clearTransportationRecords();
  owner.rtiAmbassador().queryAttributeTransportationType(object, ownerAttribute);
  serviceOwnerCallbacks();
  waitFor(
      owner,
      [&] { return owner.recorder().attributeTransportationReports().size() == 1U; },
      options,
      "baseline attribute transportation report");
  auto const baselineAttributeReports = owner.recorder().attributeTransportationReports();
  require(
      baselineAttributeReports.size() == 1U &&
          baselineAttributeReports.front().object == object &&
          baselineAttributeReports.front().attribute == ownerAttribute &&
          sameTransport(
              owner.rtiAmbassador(),
              baselineAttributeReports.front().transportation,
              L"HLAreliable"),
      "baseline attribute transportation report returned the wrong metadata");

  owner.recorder().clearTransportationRecords();
  owner.rtiAmbassador().disableCallbacks();
  owner.rtiAmbassador().queryAttributeTransportationType(object, ownerAttribute);
  serviceOwnerCallbacks();
  require(
      owner.recorder().attributeTransportationReports().empty(),
      "disabled callbacks exposed an attribute transportation report");
  owner.rtiAmbassador().enableCallbacks();
  serviceOwnerCallbacks();
  waitFor(
      owner,
      [&] { return owner.recorder().attributeTransportationReports().size() == 1U; },
      options,
      "re-enabled attribute transportation report");
  auto const releasedAttributeReports = owner.recorder().attributeTransportationReports();
  require(
      releasedAttributeReports.size() == 1U &&
          releasedAttributeReports.front().object == object &&
          releasedAttributeReports.front().attribute == ownerAttribute &&
          sameTransport(
              owner.rtiAmbassador(),
              releasedAttributeReports.front().transportation,
              L"HLAreliable"),
      "re-enabled attribute transportation report returned the wrong metadata");

  owner.recorder().clearTransportationRecords();
  owner.rtiAmbassador().disableCallbacks();
  owner.rtiAmbassador().requestAttributeTransportationTypeChange(
      object,
      ownerAttributes,
      bestEffort);
  serviceOwnerCallbacks();
  require(
      owner.recorder().attributeTransportationConfirmations().empty(),
      "disabled callbacks exposed an attribute transportation confirmation");
  owner.rtiAmbassador().enableCallbacks();
  serviceOwnerCallbacks();
  waitFor(
      owner,
      [&] {
        return owner.recorder().attributeTransportationConfirmations().size() == 1U;
      },
      options,
      "re-enabled attribute transportation confirmation");
  auto const attributeConfirmations = owner.recorder().attributeTransportationConfirmations();
  require(
      attributeConfirmations.size() == 1U &&
          attributeConfirmations.front().object == object &&
          attributeConfirmations.front().attributes == ownerAttributes &&
          sameTransport(
              owner.rtiAmbassador(),
              attributeConfirmations.front().transportation,
              L"HLAbestEffort"),
      "re-enabled attribute transportation confirmation returned the wrong metadata");

  owner.recorder().clearTransportationRecords();
  owner.rtiAmbassador().queryAttributeTransportationType(object, ownerAttribute);
  serviceOwnerCallbacks();
  waitFor(
      owner,
      [&] { return owner.recorder().attributeTransportationReports().size() == 1U; },
      options,
      "committed attribute transportation report");
  require(
      sameTransport(
          owner.rtiAmbassador(),
          owner.recorder().attributeTransportationReports().front().transportation,
          L"HLAbestEffort"),
      "attribute transportation confirmation did not commit the requested type");

  owner.recorder().clearTransportationRecords();
  owner.rtiAmbassador().queryInteractionTransportationType(
      owner.federateHandle(),
      ownerInteraction);
  serviceOwnerCallbacks();
  waitFor(
      owner,
      [&] { return owner.recorder().interactionTransportationReports().size() == 1U; },
      options,
      "baseline interaction transportation report");
  auto const baselineInteractionReports = owner.recorder().interactionTransportationReports();
  require(
      baselineInteractionReports.size() == 1U &&
          baselineInteractionReports.front().federate == owner.federateHandle() &&
          baselineInteractionReports.front().interaction == ownerInteraction &&
          sameTransport(
              owner.rtiAmbassador(),
              baselineInteractionReports.front().transportation,
              L"HLAreliable"),
      "baseline interaction transportation report returned the wrong metadata");

  owner.recorder().clearTransportationRecords();
  owner.rtiAmbassador().disableCallbacks();
  owner.rtiAmbassador().queryInteractionTransportationType(
      owner.federateHandle(),
      ownerInteraction);
  serviceOwnerCallbacks();
  require(
      owner.recorder().interactionTransportationReports().empty(),
      "disabled callbacks exposed an interaction transportation report");
  owner.rtiAmbassador().enableCallbacks();
  serviceOwnerCallbacks();
  waitFor(
      owner,
      [&] { return owner.recorder().interactionTransportationReports().size() == 1U; },
      options,
      "re-enabled interaction transportation report");
  auto const releasedInteractionReports = owner.recorder().interactionTransportationReports();
  require(
      releasedInteractionReports.size() == 1U &&
          releasedInteractionReports.front().federate == owner.federateHandle() &&
          releasedInteractionReports.front().interaction == ownerInteraction &&
          sameTransport(
              owner.rtiAmbassador(),
              releasedInteractionReports.front().transportation,
              L"HLAreliable"),
      "re-enabled interaction transportation report returned the wrong metadata");

  owner.recorder().clearTransportationRecords();
  owner.rtiAmbassador().disableCallbacks();
  owner.rtiAmbassador().requestInteractionTransportationTypeChange(
      ownerInteraction,
      bestEffort);
  serviceOwnerCallbacks();
  require(
      owner.recorder().interactionTransportationConfirmations().empty(),
      "disabled callbacks exposed an interaction transportation confirmation");
  owner.rtiAmbassador().enableCallbacks();
  serviceOwnerCallbacks();
  waitFor(
      owner,
      [&] {
        return owner.recorder().interactionTransportationConfirmations().size() == 1U;
      },
      options,
      "re-enabled interaction transportation confirmation");
  auto const interactionConfirmations = owner.recorder().interactionTransportationConfirmations();
  require(
      interactionConfirmations.size() == 1U &&
          interactionConfirmations.front().interaction == ownerInteraction &&
          sameTransport(
              owner.rtiAmbassador(),
              interactionConfirmations.front().transportation,
              L"HLAbestEffort"),
      "re-enabled interaction transportation confirmation returned the wrong metadata");

  owner.recorder().clearTransportationRecords();
  owner.rtiAmbassador().queryInteractionTransportationType(
      owner.federateHandle(),
      ownerInteraction);
  serviceOwnerCallbacks();
  waitFor(
      owner,
      [&] { return owner.recorder().interactionTransportationReports().size() == 1U; },
      options,
      "committed interaction transportation report");
  auto const committedInteractionReports = owner.recorder().interactionTransportationReports();
  require(
      committedInteractionReports.size() == 1U &&
          sameTransport(
              owner.rtiAmbassador(),
              committedInteractionReports.front().transportation,
              L"HLAbestEffort"),
      "interaction transportation confirmation did not commit the requested type");

  observer.rtiAmbassador().unsubscribeInteractionClass(observerInteraction);
  observer.rtiAmbassador().unsubscribeObjectClassAttributes(
      observerClass,
      observerAttributes);
  owner.rtiAmbassador().unpublishInteractionClass(ownerInteraction);
  owner.rtiAmbassador().unpublishObjectClassAttributes(ownerClass, ownerAttributes);
  observer.resign(rti::NO_ACTION);
  owner.resign(rti::NO_ACTION);
  owner.rtiAmbassador().destroyFederationExecution(federation);
  observer.disconnect();
  owner.disconnect();
}

void scenarioCallbackControlsTransportationContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioCallbackControlsTransportation(options, model);
}

void scenarioCallbackControlsDirectedInteraction(
    Options const& options,
    rti::CallbackModel model) {
  Session publisher(options, model, "owner");
  Session target(options, model, "member");
  auto const federation = federationName(
      options,
      "callback-controls-directed-interaction");
  connectAndJoin(publisher, target, options, federation, options.fom);

  auto const publisherClass = publisher.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const targetClass = target.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const publisherAttribute = publisher.rtiAmbassador().getAttributeHandle(
      publisherClass,
      options.attributeName);
  auto const targetAttribute = target.rtiAmbassador().getAttributeHandle(
      targetClass,
      options.attributeName);
  auto const publisherInteraction = publisher.rtiAmbassador().getInteractionClassHandle(
      options.interactionClassName);
  auto const targetInteraction = target.rtiAmbassador().getInteractionClassHandle(
      options.interactionClassName);
  auto const publisherParameter = publisher.rtiAmbassador().getParameterHandle(
      publisherInteraction,
      options.parameterName);
  auto const targetParameter = target.rtiAmbassador().getParameterHandle(
      targetInteraction,
      options.parameterName);
  require(
      publisherClass.isValid() && targetClass.isValid() &&
          publisherAttribute.isValid() && targetAttribute.isValid() &&
          publisherInteraction.isValid() && targetInteraction.isValid() &&
          publisherParameter.isValid() && targetParameter.isValid() &&
          publisherClass == targetClass &&
          publisherAttribute == targetAttribute &&
          publisherInteraction == targetInteraction &&
          publisherParameter == targetParameter,
      "callback-control directed-interaction lookup returned mismatched handles");

  rti::AttributeHandleSet const publisherAttributes{publisherAttribute};
  rti::AttributeHandleSet const targetAttributes{targetAttribute};
  rti::InteractionClassHandleSet const directedInteractions{publisherInteraction};
  publisher.rtiAmbassador().publishObjectClassAttributes(
      publisherClass,
      publisherAttributes);
  target.rtiAmbassador().publishObjectClassAttributes(
      targetClass,
      targetAttributes);
  publisher.rtiAmbassador().subscribeObjectClassAttributes(
      publisherClass,
      publisherAttributes,
      true,
      L"");
  target.rtiAmbassador().subscribeObjectClassAttributes(
      targetClass,
      targetAttributes,
      true,
      L"");
  publisher.rtiAmbassador().publishObjectClassDirectedInteractions(
      publisherClass,
      directedInteractions);
  target.rtiAmbassador().subscribeObjectClassDirectedInteractions(
      targetClass,
      rti::InteractionClassHandleSet{targetInteraction},
      false);

  auto const targetObject = target.rtiAmbassador().registerObjectInstance(targetClass);
  require(
      targetObject.isValid(),
      "callback-control directed-interaction registration returned an invalid object");
  waitFor(
      publisher,
      [&] { return publisher.recorder().hasDiscovery(targetObject); },
      options,
      "callback-control directed-interaction target discovery");

  auto assertDirected = [&](DirectedInteractionRecord const& received,
                            std::vector<std::uint8_t> const& expectedValue,
                            std::vector<std::uint8_t> const& expectedTag,
                            std::string const& description) {
    require(
        received.interaction == targetInteraction &&
            received.object == targetObject &&
            received.parameters.size() == 1U &&
            received.parameters.count(targetParameter) == 1U &&
            copyBytes(received.parameters.at(targetParameter)) == expectedValue &&
            received.tag == expectedTag &&
            received.transportation.isValid() &&
            target.rtiAmbassador().getTransportationTypeName(
                received.transportation) == L"HLAreliable" &&
            received.producer == publisher.federateHandle(),
        description + " returned the wrong metadata");
  };

  std::vector<std::uint8_t> const firstValue{0x44U, 0x49U, 0x31U};
  std::vector<std::uint8_t> const firstTag{0x44U, 0x54U, 0x31U};
  rti::ParameterHandleValueMap firstParameters;
  firstParameters.emplace(
      publisherParameter,
      rti::VariableLengthData(firstValue.data(), firstValue.size()));
  rti::VariableLengthData firstTagData(firstTag.data(), firstTag.size());
  publisher.rtiAmbassador().sendDirectedInteraction(
      publisherInteraction,
      targetObject,
      firstParameters,
      firstTagData);
  waitFor(
      target,
      [&] { return target.recorder().directedInteractions().size() == 1U; },
      options,
      "baseline callback-control directed interaction");
  auto const baseline = target.recorder().directedInteractions();
  require(
      baseline.size() == 1U,
      "baseline callback-control directed interaction delivered duplicates");
  assertDirected(
      baseline.front(),
      firstValue,
      firstTag,
      "baseline callback-control directed interaction");

  target.rtiAmbassador().disableCallbacks();
  std::vector<std::uint8_t> const secondValue{0x44U, 0x49U, 0x32U};
  std::vector<std::uint8_t> const secondTag{0x44U, 0x54U, 0x32U};
  rti::ParameterHandleValueMap secondParameters;
  secondParameters.emplace(
      publisherParameter,
      rti::VariableLengthData(secondValue.data(), secondValue.size()));
  rti::VariableLengthData secondTagData(secondTag.data(), secondTag.size());
  publisher.rtiAmbassador().sendDirectedInteraction(
      publisherInteraction,
      targetObject,
      secondParameters,
      secondTagData);
  if (model == rti::HLA_EVOKED) {
    static_cast<void>(target.evokeMultipleCallbacks(0.0, 1.0));
  } else {
    static_cast<void>(target.rtiAmbassador().getObjectClassHandle(
        options.objectClassName));
  }
  require(
      target.recorder().directedInteractions().size() == 1U,
      "disabled callbacks exposed a directed-interaction callback");

  target.rtiAmbassador().enableCallbacks();
  if (model == rti::HLA_EVOKED) {
    static_cast<void>(target.evokeMultipleCallbacks(0.0, 1.0));
  } else {
    static_cast<void>(target.rtiAmbassador().getObjectClassHandle(
        options.objectClassName));
  }
  waitFor(
      target,
      [&] { return target.recorder().directedInteractions().size() == 2U; },
      options,
      "re-enabled callback-control directed interaction");
  auto const released = target.recorder().directedInteractions();
  require(
      released.size() == 2U,
      "re-enabled callback-control directed interaction delivered duplicates");
  assertDirected(
      released.back(),
      secondValue,
      secondTag,
      "re-enabled callback-control directed interaction");

  target.rtiAmbassador().unsubscribeObjectClassDirectedInteractions(
      targetClass,
      rti::InteractionClassHandleSet{targetInteraction});
  publisher.rtiAmbassador().unpublishObjectClassDirectedInteractions(
      publisherClass,
      directedInteractions);
  target.rtiAmbassador().unsubscribeObjectClassAttributes(
      targetClass,
      targetAttributes);
  publisher.rtiAmbassador().unsubscribeObjectClassAttributes(
      publisherClass,
      publisherAttributes);
  publisher.rtiAmbassador().unpublishObjectClassAttributes(
      publisherClass,
      publisherAttributes);
  target.rtiAmbassador().unpublishObjectClassAttributes(
      targetClass,
      targetAttributes);
  target.resign(rti::DELETE_OBJECTS);
  publisher.resign(rti::NO_ACTION);
  publisher.rtiAmbassador().destroyFederationExecution(federation);
  target.disconnect();
  publisher.disconnect();
}

void scenarioCallbackControlsDirectedInteractionContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioCallbackControlsDirectedInteraction(options, model);
}

void scenarioCallbackControlsFederationReports(
    Options const& options,
    rti::CallbackModel model) {
  Session owner(options, model, "owner");
  Session observer(options, model, "observer");
  auto const federation = federationName(
      options,
      "callback-controls-federation-reports");
  auto const missingFederation = federationName(
      options,
      "callback-controls-federation-reports-missing");
  owner.connect();
  observer.connect();
  owner.rtiAmbassador().createFederationExecution(
      federation,
      options.fom.wstring(),
      options.logicalTimeImplementationName);
  owner.join(options.ownerFederateName, options.federateType, federation);
  observer.join(options.memberFederateName, options.federateType, federation);

  auto serviceObserverCallbacks = [&] {
    if (model == rti::HLA_EVOKED) {
      static_cast<void>(observer.evokeMultipleCallbacks(0.0, 1.0));
    } else {
      static_cast<void>(observer.rtiAmbassador().getObjectClassHandle(
          options.objectClassName));
    }
  };
  auto requireFederationReport = [&](std::string const& description) {
    waitFor(
        observer,
        [&] {
          return observer.recorder().listsFederation(
              federation,
              options.logicalTimeImplementationName);
        },
        options,
        description);
  };
  auto requireMemberReport = [&](std::set<std::wstring> const& names,
                                 std::size_t exactCount,
                                 std::string const& description) {
    waitFor(
        observer,
        [&] {
          return observer.recorder().listsMembers(
              federation,
              names,
              exactCount) &&
              observer.recorder().listsMember(
                  federation,
                  options.memberFederateName,
                  options.federateType);
        },
        options,
        description);
  };

  observer.rtiAmbassador().disableCallbacks();
  observer.rtiAmbassador().listFederationExecutions();
  serviceObserverCallbacks();
  require(
      !observer.recorder().listsFederation(
          federation,
          options.logicalTimeImplementationName),
      "disabled callbacks exposed a federation execution report");
  observer.rtiAmbassador().enableCallbacks();
  serviceObserverCallbacks();
  requireFederationReport("re-enabled federation execution report");

  observer.rtiAmbassador().disableCallbacks();
  observer.rtiAmbassador().listFederationExecutionMembers(federation);
  serviceObserverCallbacks();
  require(
      !observer.recorder().listsMembers(
          federation,
          {options.ownerFederateName, options.memberFederateName},
          2U),
      "disabled callbacks exposed a federation member report");
  observer.rtiAmbassador().enableCallbacks();
  serviceObserverCallbacks();
  requireMemberReport(
      {options.ownerFederateName, options.memberFederateName},
      2U,
      "re-enabled federation member report");

  observer.rtiAmbassador().disableCallbacks();
  observer.rtiAmbassador().listFederationExecutionMembers(missingFederation);
  serviceObserverCallbacks();
  require(
      !observer.recorder().reportsMissing(missingFederation),
      "disabled callbacks exposed a federation-does-not-exist report");
  observer.rtiAmbassador().enableCallbacks();
  serviceObserverCallbacks();
  waitFor(
      observer,
      [&] { return observer.recorder().reportsMissing(missingFederation); },
      options,
      "re-enabled federation-does-not-exist report");

  observer.resign(rti::NO_ACTION);
  owner.resign(rti::NO_ACTION);
  owner.rtiAmbassador().destroyFederationExecution(federation);
  observer.disconnect();
  owner.disconnect();
}

void scenarioCallbackControlsFederationReportsContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioCallbackControlsFederationReports(options, model);
}

void scenarioCallbackControlsAttributeScopeAdvisories(
    Options const& options,
    rti::CallbackModel model) {
  Session owner(options, model, "owner");
  Session member(options, model, "member");
  auto const federation = federationName(
      options,
      "callback-controls-attribute-scope-advisories");
  connectAndJoin(owner, member, options, federation, options.ddmFom);

  auto const ownerHandles = ddmHandles(owner, options);
  auto const memberHandles = ddmHandles(member, options);
  verifyDdmClassDimensions(
      owner,
      options,
      ownerHandles,
      "callback-control attribute scope advisories");
  verifyDdmClassDimensions(
      member,
      options,
      memberHandles,
      "callback-control attribute scope advisories member");

  rti::AttributeHandleSet const ownerAttributes{ownerHandles.attribute};
  rti::AttributeHandleSet const memberAttributes{memberHandles.attribute};
  owner.rtiAmbassador().publishObjectClassAttributes(
      ownerHandles.objectClass,
      ownerAttributes);

  auto const sourceRegion = createDdmRegion(
      owner,
      ownerHandles,
      1UL,
      5UL,
      1UL,
      5UL);
  auto const alternateSourceRegion = createDdmRegion(
      owner,
      ownerHandles,
      7UL,
      9UL,
      7UL,
      9UL);
  auto const memberRegion = createDdmRegion(
      member,
      memberHandles,
      2UL,
      6UL,
      2UL,
      6UL);
  rti::AttributeHandleSetRegionHandleSetPairVector const sourcePair{{
      ownerAttributes,
      rti::RegionHandleSet{sourceRegion},
  }};
  rti::AttributeHandleSetRegionHandleSetPairVector const alternateSourcePair{{
      ownerAttributes,
      rti::RegionHandleSet{alternateSourceRegion},
  }};
  rti::AttributeHandleSetRegionHandleSetPairVector const memberPair{{
      memberAttributes,
      rti::RegionHandleSet{memberRegion},
  }};

  member.rtiAmbassador().setAttributeScopeAdvisorySwitch(false);
  require(
      !member.rtiAmbassador().getAttributeScopeAdvisorySwitch(),
      "attribute scope advisory switch did not turn off");
  member.rtiAmbassador().setAttributeScopeAdvisorySwitch(true);
  require(
      member.rtiAmbassador().getAttributeScopeAdvisorySwitch(),
      "attribute scope advisory switch did not turn on");
  member.rtiAmbassador().subscribeObjectClassAttributesWithRegions(
      memberHandles.objectClass,
      memberPair,
      true,
      L"");
  auto const object = owner.rtiAmbassador().registerObjectInstanceWithRegions(
      ownerHandles.objectClass,
      sourcePair);
  require(
      object.isValid(),
      "callback-control attribute scope registration returned an invalid object");
  waitFor(
      member,
      [&] { return member.recorder().hasDiscovery(object); },
      options,
      "callback-control attribute scope discovery");

  auto serviceMemberCallbacks = [&] {
    if (model == rti::HLA_EVOKED) {
      static_cast<void>(member.evokeMultipleCallbacks(0.0, 0.0));
    } else {
      static_cast<void>(member.rtiAmbassador().getObjectClassHandle(
          options.objectClassName));
    }
  };
  auto drainMemberCallbacks = [&] {
    if (model == rti::HLA_EVOKED) {
      while (member.evokeMultipleCallbacks(0.0, 0.0)) {
      }
    }
  };
  auto requireSingleScopeAdvisory = [&](std::vector<ScopeAdvisoryRecord> const& reports,
                                        std::string const& description) {
    require(
        reports.size() == 1U,
        description + " returned an unexpected callback count");
    require(
        reports.front().object == object &&
            reports.front().attributes == memberAttributes,
        description + " returned the wrong object or attributes");
  };
  auto moveOutOfScope = [&] {
    owner.rtiAmbassador().associateRegionsForUpdates(
        object,
        alternateSourcePair);
    owner.rtiAmbassador().unassociateRegionsForUpdates(
        object,
        sourcePair);
  };
  auto moveIntoScope = [&] {
    owner.rtiAmbassador().associateRegionsForUpdates(
        object,
        sourcePair);
    owner.rtiAmbassador().unassociateRegionsForUpdates(
        object,
        alternateSourcePair);
  };

  drainMemberCallbacks();
  member.recorder().clearScopeAdvisories();

  moveOutOfScope();
  waitFor(
      member,
      [&] { return !member.recorder().attributesOutOfScope().empty(); },
      options,
      "baseline attribute out-of-scope advisory");
  requireSingleScopeAdvisory(
      member.recorder().attributesOutOfScope(),
      "baseline attribute out-of-scope advisory");
  require(
      member.recorder().attributesInScope().empty(),
      "baseline out-of-scope transition also produced an in-scope advisory");
  member.recorder().clearScopeAdvisories();

  moveIntoScope();
  waitFor(
      member,
      [&] { return !member.recorder().attributesInScope().empty(); },
      options,
      "baseline attribute in-scope advisory");
  requireSingleScopeAdvisory(
      member.recorder().attributesInScope(),
      "baseline attribute in-scope advisory");
  require(
      member.recorder().attributesOutOfScope().empty(),
      "baseline in-scope transition also produced an out-of-scope advisory");
  member.recorder().clearScopeAdvisories();

  member.rtiAmbassador().disableCallbacks();
  moveOutOfScope();
  serviceMemberCallbacks();
  require(
      member.recorder().attributesInScope().empty() &&
          member.recorder().attributesOutOfScope().empty(),
      "disabled callbacks exposed an out-of-scope advisory");
  member.rtiAmbassador().enableCallbacks();
  serviceMemberCallbacks();
  waitFor(
      member,
      [&] { return !member.recorder().attributesOutOfScope().empty(); },
      options,
      "re-enabled attribute out-of-scope advisory");
  requireSingleScopeAdvisory(
      member.recorder().attributesOutOfScope(),
      "re-enabled attribute out-of-scope advisory");
  require(
      member.recorder().attributesInScope().empty(),
      "re-enabled out-of-scope transition also produced an in-scope advisory");
  member.recorder().clearScopeAdvisories();

  member.rtiAmbassador().disableCallbacks();
  moveIntoScope();
  serviceMemberCallbacks();
  require(
      member.recorder().attributesInScope().empty() &&
          member.recorder().attributesOutOfScope().empty(),
      "disabled callbacks exposed an in-scope advisory");
  member.rtiAmbassador().enableCallbacks();
  serviceMemberCallbacks();
  waitFor(
      member,
      [&] { return !member.recorder().attributesInScope().empty(); },
      options,
      "re-enabled attribute in-scope advisory");
  requireSingleScopeAdvisory(
      member.recorder().attributesInScope(),
      "re-enabled attribute in-scope advisory");
  require(
      member.recorder().attributesOutOfScope().empty(),
      "re-enabled in-scope transition also produced an out-of-scope advisory");

  member.rtiAmbassador().unsubscribeObjectClassAttributesWithRegions(
      memberHandles.objectClass,
      memberPair);
  owner.rtiAmbassador().unassociateRegionsForUpdates(object, sourcePair);
  owner.rtiAmbassador().unpublishObjectClassAttributes(
      ownerHandles.objectClass,
      ownerAttributes);
  owner.rtiAmbassador().deleteRegion(sourceRegion);
  owner.rtiAmbassador().deleteRegion(alternateSourceRegion);
  member.rtiAmbassador().deleteRegion(memberRegion);
  owner.resign(rti::DELETE_OBJECTS);
  member.resign(rti::NO_ACTION);
  owner.rtiAmbassador().destroyFederationExecution(federation);
  owner.disconnect();
  member.disconnect();
}

void scenarioCallbackControlsAttributeScopeAdvisoriesContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioCallbackControlsAttributeScopeAdvisories(options, model);
}

void scenarioCallbackControlsFlushQueueGrant(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.logicalTimeImplementationName.empty(),
      "Callback-control Flush Queue grant test requires an adapter-supplied logical-time implementation");

  Session publisher(options, model, "owner");
  Session receiver(options, model, "member");
  auto const federation = federationName(
      options,
      "callback-controls-flush-queue-grant");
  publisher.connect();
  receiver.connect();
  publisher.rtiAmbassador().createFederationExecution(
      federation,
      options.fom.wstring(),
      options.logicalTimeImplementationName);
  publisher.join(options.ownerFederateName, options.federateType, federation);
  receiver.join(options.memberFederateName, options.federateType, federation);

  auto const publisherInteraction = publisher.rtiAmbassador().getInteractionClassHandle(
      options.interactionClassName);
  auto const receiverInteraction = receiver.rtiAmbassador().getInteractionClassHandle(
      options.interactionClassName);
  auto const publisherParameter = publisher.rtiAmbassador().getParameterHandle(
      publisherInteraction,
      options.parameterName);
  auto const receiverParameter = receiver.rtiAmbassador().getParameterHandle(
      receiverInteraction,
      options.parameterName);
  require(
      publisherInteraction.isValid() && receiverInteraction.isValid() &&
          publisherParameter.isValid() && receiverParameter.isValid() &&
          publisherInteraction == receiverInteraction &&
          publisherParameter == receiverParameter,
      "Callback-control Flush Queue grant lookup returned mismatched handles");

  publisher.rtiAmbassador().publishInteractionClass(publisherInteraction);
  publisher.rtiAmbassador().changeInteractionOrderType(
      publisherInteraction,
      rti::TIMESTAMP);
  receiver.rtiAmbassador().subscribeInteractionClass(receiverInteraction, true);

  auto publisherTime = makeTimeContext(publisher);
  auto receiverTime = makeTimeContext(receiver);
  require(
      publisherTime.factory->getName() == receiverTime.factory->getName(),
      "Callback-control Flush Queue grant members selected different logical-time factories");
  receiver.rtiAmbassador().enableTimeConstrained();
  waitFor(
      receiver,
      [&] { return receiver.recorder().timeConstrainedEnabled().size() >= 1U; },
      options,
      "Callback-control Flush Queue grant time-constrained callback");
  publisher.rtiAmbassador().enableTimeRegulation(*publisherTime.epsilon);
  waitFor(
      publisher,
      [&] { return publisher.recorder().timeRegulationEnabled().size() >= 1U; },
      options,
      "Callback-control Flush Queue grant time-regulation callback");
  publisher.recorder().clearTimeCallbacks();
  receiver.recorder().clearTimeCallbacks();
  receiver.recorder().clearTimedInteractions();
  receiver.recorder().clearFlushQueueGrants();
  publisher.recorder().clearCallbackOrder();
  receiver.recorder().clearCallbackOrder();

  auto const messageTime = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      9U);
  auto const publisherTarget = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      4U);
  auto const requestBoundary = timeAfter(
      *receiverTime.factory,
      *receiverTime.initial,
      *receiverTime.epsilon,
      10U);
  std::vector<std::uint8_t> const parameterBytes{
      0x46U, 0x51U, 0x2DU, 0x43U, 0x54U, 0x52U, 0x4CU};
  std::vector<std::uint8_t> const tagBytes{
      0x46U, 0x51U, 0x2DU, 0x54U, 0x41U, 0x47U};
  rti::ParameterHandleValueMap parameters;
  parameters.emplace(
      publisherParameter,
      rti::VariableLengthData(parameterBytes.data(), parameterBytes.size()));
  rti::VariableLengthData tag(tagBytes.data(), tagBytes.size());
  auto const retraction = publisher.rtiAmbassador().sendInteraction(
      publisherInteraction,
      parameters,
      tag,
      *messageTime);
  require(
      retraction.isValid(),
      "Callback-control Flush Queue grant send returned an invalid retraction handle");

  receiver.rtiAmbassador().disableCallbacks();
  receiver.rtiAmbassador().flushQueueRequest(*requestBoundary);
  if (model == rti::HLA_EVOKED) {
    require(
        receiver.recorder().timedInteractions().empty() &&
            receiver.recorder().flushQueueGrants().empty(),
        "Callback-control Flush Queue grant delivered before callback servicing");
  }
  publisher.rtiAmbassador().timeAdvanceRequest(*publisherTarget);
  waitFor(
      publisher,
      [&] { return publisher.recorder().timeAdvanceGrants().size() >= 1U; },
      options,
      "Callback-control Flush Queue grant publisher advance");
  if (model == rti::HLA_EVOKED) {
    static_cast<void>(receiver.evokeMultipleCallbacks(0.0, 0.0));
  } else {
    static_cast<void>(receiver.rtiAmbassador().getObjectClassHandle(
        options.objectClassName));
  }
  require(
      receiver.recorder().timedInteractions().empty() &&
          receiver.recorder().flushQueueGrants().empty(),
      "disabled callbacks exposed a Flush Queue grant or queued interaction");

  receiver.rtiAmbassador().enableCallbacks();
  if (model == rti::HLA_EVOKED) {
    static_cast<void>(receiver.evokeMultipleCallbacks(0.0, 1.0));
  } else {
    static_cast<void>(receiver.rtiAmbassador().getObjectClassHandle(
        options.objectClassName));
  }
  waitFor(
      receiver,
      [&] {
        return receiver.recorder().timedInteractions().size() == 1U &&
            receiver.recorder().flushQueueGrants().size() == 1U;
      },
      options,
      "re-enabled Flush Queue grant and queued interaction");
  require(
      receiver.recorder().callbackOrder() ==
          std::vector<std::string>{"interaction", "flush-grant"},
      "re-enabled Flush Queue grant was delivered before queued interaction");

  auto const interactions = receiver.recorder().timedInteractions();
  require(
      interactions.front().interaction == receiverInteraction &&
          interactions.front().parameters.size() == 1U &&
          interactions.front().parameters.count(receiverParameter) == 1U &&
          copyBytes(interactions.front().parameters.at(receiverParameter)) == parameterBytes &&
          interactions.front().tag == tagBytes &&
          interactions.front().producer == publisher.federateHandle() &&
          interactions.front().time == encodeTime(*messageTime) &&
          interactions.front().sentOrder == rti::TIMESTAMP &&
          interactions.front().receivedOrder == rti::TIMESTAMP &&
          interactions.front().transportation.isValid() &&
          !interactions.front().regions.has_value() &&
          interactions.front().retractionPresent &&
          interactions.front().retraction == copyBytes(retraction.encode()),
      "re-enabled Flush Queue interaction returned the wrong metadata");
  auto const flushGrants = receiver.recorder().flushQueueGrants();
  require(
      !flushGrants.front().time.empty() &&
          !flushGrants.front().timeText.empty() &&
          !flushGrants.front().optimisticTime.empty() &&
          !flushGrants.front().optimisticTimeText.empty() &&
          flushGrants.front().optimisticTime == encodeTime(*messageTime),
      "re-enabled Flush Queue grant returned incomplete or incorrect time metadata");
  auto actualFlushTime = decodeTime(*receiverTime.factory, flushGrants.front().time);
  require(
      actualFlushTime != nullptr &&
          !(*actualFlushTime < *receiverTime.initial) &&
          !(*requestBoundary < *actualFlushTime),
      "re-enabled Flush Queue grant returned a time outside standard bounds");

  receiver.rtiAmbassador().disableTimeConstrained();
  publisher.rtiAmbassador().disableTimeRegulation();
  receiver.rtiAmbassador().unsubscribeInteractionClass(receiverInteraction);
  publisher.rtiAmbassador().unpublishInteractionClass(publisherInteraction);
  receiver.resign(rti::NO_ACTION);
  publisher.resign(rti::NO_ACTION);
  publisher.rtiAmbassador().destroyFederationExecution(federation);
  receiver.disconnect();
  publisher.disconnect();
}

void scenarioCallbackControlsFlushQueueGrantContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioCallbackControlsFlushQueueGrant(options, model);
}

void scenarioCallbackControlsAvailableTimeAdvanceCallbacks(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.logicalTimeImplementationName.empty(),
      "Callback-control available time-advance test requires an adapter-supplied logical-time implementation");

  Session publisher(options, model, "owner");
  Session receiver(options, model, "member");
  auto const federation = federationName(
      options,
      "callback-controls-available-time-advance-callbacks");
  publisher.connect();
  receiver.connect();
  publisher.rtiAmbassador().createFederationExecution(
      federation,
      options.fom.wstring(),
      options.logicalTimeImplementationName);
  publisher.join(options.ownerFederateName, options.federateType, federation);
  receiver.join(options.memberFederateName, options.federateType, federation);

  auto const publisherInteraction = publisher.rtiAmbassador().getInteractionClassHandle(
      options.interactionClassName);
  auto const receiverInteraction = receiver.rtiAmbassador().getInteractionClassHandle(
      options.interactionClassName);
  auto const publisherParameter = publisher.rtiAmbassador().getParameterHandle(
      publisherInteraction,
      options.parameterName);
  auto const receiverParameter = receiver.rtiAmbassador().getParameterHandle(
      receiverInteraction,
      options.parameterName);
  require(
      publisherInteraction.isValid() && receiverInteraction.isValid() &&
          publisherParameter.isValid() && receiverParameter.isValid() &&
          publisherInteraction == receiverInteraction &&
          publisherParameter == receiverParameter,
      "Callback-control available time-advance lookup returned mismatched handles");

  publisher.rtiAmbassador().publishInteractionClass(publisherInteraction);
  publisher.rtiAmbassador().changeInteractionOrderType(
      publisherInteraction,
      rti::TIMESTAMP);
  receiver.rtiAmbassador().subscribeInteractionClass(receiverInteraction, true);

  auto publisherTime = makeTimeContext(publisher);
  auto receiverTime = makeTimeContext(receiver);
  require(
      publisherTime.factory->getName() == receiverTime.factory->getName(),
      "Callback-control available time-advance members selected different logical-time factories");
  auto const lookaheadTime = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      5U);
  auto lookahead = publisherTime.factory->makeZero();
  require(
      lookahead != nullptr,
      "Callback-control available time-advance test could not allocate a lookahead interval");
  lookahead->setToDifference(*lookaheadTime, *publisherTime.initial);
  receiver.rtiAmbassador().enableTimeConstrained();
  waitFor(
      receiver,
      [&] { return receiver.recorder().timeConstrainedEnabled().size() >= 1U; },
      options,
      "Callback-control available time-advance time-constrained callback");
  publisher.rtiAmbassador().enableTimeRegulation(*lookahead);
  waitFor(
      publisher,
      [&] { return publisher.recorder().timeRegulationEnabled().size() >= 1U; },
      options,
      "Callback-control available time-advance time-regulation callback");
  publisher.recorder().clearTimeCallbacks();
  receiver.recorder().clearTimeCallbacks();
  publisher.recorder().clearCallbackOrder();
  receiver.recorder().clearCallbackOrder();

  auto serviceReceiverCallbacks = [&] {
    if (model == rti::HLA_EVOKED) {
      static_cast<void>(receiver.evokeMultipleCallbacks(0.0, 0.0));
    } else {
      static_cast<void>(receiver.rtiAmbassador().getObjectClassHandle(
          options.objectClassName));
    }
  };
  auto verifyAvailableAdvance = [&](bool useNextMessageRequest,
                                    rti::LogicalTime const& messageTime,
                                    rti::LogicalTime const& availableRequest,
                                    rti::LogicalTime const& publisherTarget,
                                    std::vector<std::uint8_t> const& expectedParameter,
                                    std::vector<std::uint8_t> const& expectedTag,
                                    std::string const& description) {
    rti::ParameterHandleValueMap parameters;
    parameters.emplace(
        publisherParameter,
        rti::VariableLengthData(
            expectedParameter.data(),
            expectedParameter.size()));
    rti::VariableLengthData tag(expectedTag.data(), expectedTag.size());
    auto const retraction = publisher.rtiAmbassador().sendInteraction(
        publisherInteraction,
        parameters,
        tag,
        messageTime);
    require(
        retraction.isValid(),
        description + " send returned an invalid retraction handle");

    receiver.rtiAmbassador().disableCallbacks();
    if (useNextMessageRequest) {
      receiver.rtiAmbassador().nextMessageRequestAvailable(availableRequest);
    } else {
      receiver.rtiAmbassador().timeAdvanceRequestAvailable(availableRequest);
    }
    require(
        receiver.recorder().timedInteractions().empty() &&
        receiver.recorder().timeAdvanceGrants().empty(),
        description + " completed before the regulating federate advanced");
    publisher.rtiAmbassador().timeAdvanceRequest(publisherTarget);
    waitFor(
        publisher,
        [&] { return publisher.recorder().timeAdvanceGrants().size() >= 1U; },
        options,
        description + " publisher grant");
    serviceReceiverCallbacks();
    require(
        receiver.recorder().timedInteractions().empty() &&
            receiver.recorder().timeAdvanceGrants().empty(),
        description + " exposed callbacks while callbacks were disabled");

    receiver.rtiAmbassador().enableCallbacks();
    if (model == rti::HLA_EVOKED) {
      static_cast<void>(receiver.evokeMultipleCallbacks(0.0, 1.0));
    } else {
      static_cast<void>(receiver.rtiAmbassador().getObjectClassHandle(
          options.objectClassName));
    }
    waitFor(
        receiver,
        [&] {
          return receiver.recorder().timedInteractions().size() == 1U &&
              receiver.recorder().timeAdvanceGrants().size() == 1U;
        },
        options,
        description + " re-enabled callbacks");
    require(
        receiver.recorder().callbackOrder() ==
            std::vector<std::string>{"interaction", "grant"},
        description + " delivered its grant before the interaction");

    auto const interactions = receiver.recorder().timedInteractions();
    require(
        interactions.front().interaction == receiverInteraction &&
            interactions.front().parameters.size() == 1U &&
            interactions.front().parameters.count(receiverParameter) == 1U &&
            copyBytes(interactions.front().parameters.at(receiverParameter)) ==
                expectedParameter &&
            interactions.front().tag == expectedTag &&
            interactions.front().producer == publisher.federateHandle() &&
            interactions.front().time == encodeTime(messageTime) &&
            interactions.front().sentOrder == rti::TIMESTAMP &&
            interactions.front().receivedOrder == rti::TIMESTAMP &&
            interactions.front().transportation.isValid() &&
            !interactions.front().regions.has_value() &&
            interactions.front().retractionPresent &&
            interactions.front().retraction == copyBytes(retraction.encode()),
        description + " returned the wrong interaction metadata");
    require(
        receiver.recorder().flushQueueGrants().empty(),
        description + " returned an unexpected Flush Queue grant");
    auto const grants = receiver.recorder().timeAdvanceGrants();
    require(
        grants.front().encoded == encodeTime(messageTime),
        description + " returned the wrong time-advance grant time");
  };

  auto const firstMessageTime = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      7U);
  auto const firstPublisherTarget = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      2U);
  std::vector<std::uint8_t> const firstParameter{
      0x41U, 0x56U, 0x41U, 0x2DU, 0x54U, 0x41U, 0x52U};
  std::vector<std::uint8_t> const firstTag{
      0x41U, 0x56U, 0x41U, 0x2DU, 0x31U};
  verifyAvailableAdvance(
      false,
      *firstMessageTime,
      *firstMessageTime,
      *firstPublisherTarget,
      firstParameter,
      firstTag,
      "time-advance-request-available callback-control delivery");

  publisher.recorder().clearTimeCallbacks();
  receiver.recorder().clearTimeCallbacks();
  receiver.recorder().clearTimedInteractions();
  receiver.recorder().clearFlushQueueGrants();
  publisher.recorder().clearCallbackOrder();
  receiver.recorder().clearCallbackOrder();
  auto const secondMessageTime = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      9U);
  auto const secondRequestBoundary = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      10U);
  auto const secondPublisherTarget = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      4U);
  std::vector<std::uint8_t> const secondParameter{
      0x41U, 0x56U, 0x41U, 0x2DU, 0x4EU, 0x4DU, 0x52U};
  std::vector<std::uint8_t> const secondTag{
      0x41U, 0x56U, 0x41U, 0x2DU, 0x32U};
  verifyAvailableAdvance(
      true,
      *secondMessageTime,
      *secondRequestBoundary,
      *secondPublisherTarget,
      secondParameter,
      secondTag,
      "next-message-request-available callback-control delivery");

  receiver.rtiAmbassador().disableTimeConstrained();
  publisher.rtiAmbassador().disableTimeRegulation();
  receiver.rtiAmbassador().unsubscribeInteractionClass(receiverInteraction);
  publisher.rtiAmbassador().unpublishInteractionClass(publisherInteraction);
  receiver.resign(rti::NO_ACTION);
  publisher.resign(rti::NO_ACTION);
  publisher.rtiAmbassador().destroyFederationExecution(federation);
  receiver.disconnect();
  publisher.disconnect();
}

void scenarioCallbackControlsAvailableTimeAdvanceCallbacksContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioCallbackControlsAvailableTimeAdvanceCallbacks(options, model);
}

void scenarioCallbackControlsTimeRoleEnablement(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.logicalTimeImplementationName.empty(),
      "Callback-control time-role enablement requires an adapter-supplied logical-time implementation");

  Session regulator(options, model, "regulator");
  Session constrained(options, model, "constrained");
  auto const federation = federationName(
      options,
      "callback-controls-time-role-enablement");
  connectAndJoin(regulator, constrained, options, federation, options.fom);

  auto regulatorTime = makeTimeContext(regulator);
  auto constrainedTime = makeTimeContext(constrained);
  require(
      regulatorTime.factory->getName() == constrainedTime.factory->getName(),
      "Callback-control time-role enablement members selected different logical-time factories");
  regulator.recorder().clearTimeCallbacks();
  constrained.recorder().clearTimeCallbacks();
  regulator.recorder().clearCallbackOrder();
  constrained.recorder().clearCallbackOrder();

  regulator.rtiAmbassador().disableCallbacks();
  constrained.rtiAmbassador().disableCallbacks();
  regulator.rtiAmbassador().enableTimeRegulation(*regulatorTime.epsilon);
  constrained.rtiAmbassador().enableTimeConstrained();

  auto serviceCallbacks = [&](Session& session) {
    if (model == rti::HLA_EVOKED) {
      static_cast<void>(session.evokeMultipleCallbacks(0.0, 0.0));
    } else {
      static_cast<void>(session.rtiAmbassador().getObjectClassHandle(
          options.objectClassName));
    }
  };
  serviceCallbacks(regulator);
  serviceCallbacks(constrained);
  require(
      regulator.recorder().timeRegulationEnabled().empty() &&
          constrained.recorder().timeConstrainedEnabled().empty(),
      "disabled callbacks exposed a time-role enablement callback");

  regulator.rtiAmbassador().enableCallbacks();
  constrained.rtiAmbassador().enableCallbacks();
  serviceCallbacks(regulator);
  serviceCallbacks(constrained);
  waitForSessions(
      {&regulator, &constrained},
      [&] {
        return regulator.recorder().timeRegulationEnabled().size() == 1U &&
            constrained.recorder().timeConstrainedEnabled().size() == 1U;
      },
      options,
      "re-enabled time-role enablement callbacks");

  auto const regulationCallbacks = regulator.recorder().timeRegulationEnabled();
  auto const constrainedCallbacks = constrained.recorder().timeConstrainedEnabled();
  require(
      regulationCallbacks.size() == 1U && constrainedCallbacks.size() == 1U &&
          regulationCallbacks.front().encoded == encodeTime(*regulatorTime.initial) &&
          constrainedCallbacks.front().encoded == encodeTime(*constrainedTime.initial) &&
          !regulationCallbacks.front().text.empty() &&
          !constrainedCallbacks.front().text.empty(),
      "re-enabled time-role callbacks returned incomplete or incorrect initial-time metadata");

  constrained.rtiAmbassador().disableTimeConstrained();
  regulator.rtiAmbassador().disableTimeRegulation();
  constrained.resign(rti::NO_ACTION);
  regulator.resign(rti::NO_ACTION);
  regulator.rtiAmbassador().destroyFederationExecution(federation);
  constrained.disconnect();
  regulator.disconnect();
}

void scenarioCallbackControlsTimeRoleEnablementContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioCallbackControlsTimeRoleEnablement(options, model);
}

}  // namespace hla_rti_cpp_tck::detail
