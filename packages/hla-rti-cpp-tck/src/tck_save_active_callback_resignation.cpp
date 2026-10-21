#include "tck_save_active_callback_resignation.hpp"

namespace hla_rti_cpp_tck::detail {
namespace {

void runSaveActiveCallbackResignation(
    Options const& options, rti::CallbackModel model) {
  require(model == rti::HLA_EVOKED,
          "Active save-callback resignation is scoped to evoked callbacks");
  require(!options.fom.empty(),
          "Active save-callback resignation requires an adapter-supplied FOM");

  Session owner(options, model, "owner");
  Session held(options, model, "member");
  Session departing(options, model, "member");
  auto const federation = federationName(options, "save-active-callback-resignation");
  std::wstring const firstLabel = L"tck-active-callback-resignation-save";
  std::wstring const secondLabel = L"tck-after-active-callback-resignation-save";

  owner.connect();
  held.connect();
  departing.connect();
  owner.rtiAmbassador().createFederationExecution(
      federation, options.fom.wstring());
  owner.join(options.ownerFederateName, options.federateType, federation);
  held.join(options.memberFederateName, options.federateType, federation);
  departing.join(L"tck-save-departing", options.federateType, federation);

  held.recorder().setFederateSaveInitiateHook(
      [&] { held.rtiAmbassador().federateSaveBegun(); });
  bool rejectedInsideCallback = false;
  departing.recorder().setFederateSaveInitiateHook([&] {
    departing.rtiAmbassador().federateSaveBegun();
    try {
      departing.rtiAmbassador().resignFederationExecution(rti::NO_ACTION);
    } catch (rti::CallNotAllowedFromWithinCallback const&) {
      rejectedInsideCallback = true;
    }
  });

  owner.rtiAmbassador().requestFederationSave(firstLabel);
  waitFor(held, [&] { return held.federateSaveInitiations().size() == 1U; },
          options, "held member save initiation");
  require(held.recorder().callbackOrder() ==
              std::vector<std::string>{"federate-save-initiate"},
          "Held member did not enter its first save callback");
  waitFor(departing,
          [&] { return departing.federateSaveInitiations().size() == 1U; },
          options, "departing member save initiation");
  require(rejectedInsideCallback,
          "Resignation inside Initiate Federate Save was not rejected");
  require(departing.recorder().callbackOrder() ==
              std::vector<std::string>{"federate-save-initiate"},
          "Departing member received an unexpected callback before resignation");

  departing.resign(rti::NO_ACTION);
  waitForSessions(
      {&owner, &held},
      [&] {
        return owner.federationNotSavedReasons().size() == 1U &&
            held.federationNotSavedReasons().size() == 1U;
      }, options, "save failure after member resignation");
  require(owner.federationNotSavedReasons() ==
              std::vector<rti::SaveFailureReason>{
                  rti::FEDERATE_RESIGNED_DURING_SAVE} &&
              held.federationNotSavedReasons() ==
                  std::vector<rti::SaveFailureReason>{
                      rti::FEDERATE_RESIGNED_DURING_SAVE},
          "Remaining members did not receive the resignation save-failure reason");
  while (departing.rtiAmbassador().evokeCallback(0.0)) {
  }
  require(departing.federationNotSavedReasons().empty(),
          "Resigned member received a stale save-failure callback");
  require(owner.recorder().callbackOrder() ==
              std::vector<std::string>{"federate-save-initiate",
                                       "federation-not-saved"} &&
              held.recorder().callbackOrder() ==
                  std::vector<std::string>{"federate-save-initiate",
                                           "federation-not-saved"},
          "Save initiation and failure callbacks arrived in the wrong order");

  held.recorder().setFederateSaveInitiateHook({});
  owner.recorder().clearCallbackOrder();
  held.recorder().clearCallbackOrder();
  owner.rtiAmbassador().requestFederationSave(secondLabel);
  waitForSessions(
      {&owner, &held},
      [&] {
        return owner.federateSaveInitiations().size() == 2U &&
            held.federateSaveInitiations().size() == 2U;
      }, options, "fresh save initiation after resignation failure");
  require(owner.federateSaveInitiations() ==
              std::vector<std::wstring>{firstLabel, secondLabel} &&
              held.federateSaveInitiations() ==
                  std::vector<std::wstring>{firstLabel, secondLabel},
          "Fresh save did not initiate with the expected labels");
  owner.rtiAmbassador().federateSaveBegun();
  held.rtiAmbassador().federateSaveBegun();
  owner.rtiAmbassador().federateSaveComplete();
  held.rtiAmbassador().federateSaveComplete();
  waitForSessions(
      {&owner, &held},
      [&] {
        return owner.federationSavedCount() == 1U &&
            held.federationSavedCount() == 1U;
      }, options, "fresh save completion after resignation failure");
  require(owner.recorder().callbackOrder() ==
              std::vector<std::string>{"federate-save-initiate",
                                       "federation-saved"} &&
              held.recorder().callbackOrder() ==
                  std::vector<std::string>{"federate-save-initiate",
                                           "federation-saved"},
          "Fresh save callbacks did not complete in order");

  held.resign(rti::NO_ACTION);
  owner.resign(rti::NO_ACTION);
  owner.rtiAmbassador().destroyFederationExecution(federation);
  departing.disconnect();
  held.disconnect();
  owner.disconnect();
}

void runTimedSaveActiveCallbackResignation(
    Options const& options, rti::CallbackModel model) {
  require(model == rti::HLA_EVOKED,
          "Timed active save-callback resignation is scoped to evoked callbacks");
  require(!options.fom.empty(),
          "Timed active save-callback resignation requires an adapter-supplied FOM");
  require(!options.logicalTimeImplementationName.empty(),
          "Timed active save-callback resignation requires an adapter-supplied logical-time implementation");

  Session owner(options, model, "timed-save-owner");
  Session constrained(options, model, "timed-save-constrained");
  auto const federation = federationName(
      options, "timed-save-active-callback-resignation");
  std::wstring const saveLabel = L"tck-timed-active-callback-resignation-save";
  bool rejectedInsideCallback = false;

  owner.connect();
  constrained.connect();
  owner.rtiAmbassador().createFederationExecution(
      federation,
      options.fom.wstring(),
      options.logicalTimeImplementationName);
  owner.join(options.ownerFederateName, options.federateType, federation);
  constrained.join(
      options.memberFederateName, options.federateType, federation);

  auto const ownerTime = makeTimeContext(owner);
  auto const constrainedTime = makeTimeContext(constrained);
  require(
      ownerTime.factory->getName() == constrainedTime.factory->getName(),
      "Timed active save-callback members selected different logical-time factories");

  constrained.rtiAmbassador().enableTimeConstrained();
  waitFor(
      constrained,
      [&] { return constrained.recorder().timeConstrainedEnabled().size() == 1U; },
      options,
      "timed active save-callback constrained-enable callback");
  owner.rtiAmbassador().enableTimeRegulation(*ownerTime.epsilon);
  waitFor(
      owner,
      [&] { return owner.recorder().timeRegulationEnabled().size() == 1U; },
      options,
      "timed active save-callback regulation-enable callback");
  owner.recorder().clearCallbackOrder();
  constrained.recorder().clearCallbackOrder();

  auto const saveTime = timeAfter(
      *ownerTime.factory,
      *ownerTime.initial,
      *ownerTime.epsilon,
      5U);
  constrained.recorder().setFederateSaveInitiateHook([&] {
    try {
      constrained.rtiAmbassador().resignFederationExecution(rti::NO_ACTION);
    } catch (rti::CallNotAllowedFromWithinCallback const&) {
      rejectedInsideCallback = true;
    }
  });

  owner.rtiAmbassador().requestFederationSave(saveLabel, *saveTime);
  constrained.rtiAmbassador().timeAdvanceRequest(*saveTime);
  owner.rtiAmbassador().timeAdvanceRequest(*saveTime);

  waitFor(
      owner,
      [&] { return owner.recorder().timeAdvanceGrants().size() == 1U; },
      options,
      "timed active save-callback regulator grant");
  require(
      owner.recorder().timeAdvanceGrants().front().encoded ==
          encodeTime(*saveTime) &&
          owner.federateSaveInitiations().empty(),
      "Timed save did not deliver the regulator grant before its save callback");

  waitFor(
      constrained,
      [&] {
        return constrained.federateSaveInitiations().size() == 1U &&
            constrained.recorder().timeAdvanceGrants().size() == 1U;
      },
      options,
      "timed active save-callback constrained initiation and grant");
  require(
      rejectedInsideCallback &&
          constrained.federateSaveInitiations() ==
              std::vector<std::wstring>{saveLabel} &&
          constrained.federateSaveTimes().size() == 1U &&
          constrained.federateSaveTimes().front().encoded ==
              encodeTime(*saveTime) &&
          constrained.recorder().timeAdvanceGrants().front().encoded ==
              encodeTime(*saveTime) &&
          constrained.recorder().callbackOrder() ==
              std::vector<std::string>{"federate-save-initiate", "grant"},
      "Timed constrained save did not reject callback-time resignation before its grant");

  waitFor(
      owner,
      [&] { return owner.federateSaveInitiations().size() == 1U; },
      options,
      "timed active save-callback owner initiation");
  require(
      owner.federateSaveInitiations() == std::vector<std::wstring>{saveLabel} &&
          owner.federateSaveTimes().size() == 1U &&
          owner.federateSaveTimes().front().encoded == encodeTime(*saveTime) &&
          owner.recorder().callbackOrder() ==
              std::vector<std::string>{"grant", "federate-save-initiate"},
      "Timed save owner received its initiation at the wrong callback boundary");

  owner.rtiAmbassador().federateSaveBegun();
  constrained.rtiAmbassador().federateSaveBegun();
  owner.rtiAmbassador().federateSaveComplete();
  constrained.rtiAmbassador().federateSaveComplete();
  waitForSessions(
      {&owner, &constrained},
      [&] {
        return owner.federationSavedCount() == 1U &&
            constrained.federationSavedCount() == 1U;
      },
      options,
      "timed active save-callback successful completion");
  require(
      owner.federationNotSavedReasons().empty() &&
          constrained.federationNotSavedReasons().empty() &&
          owner.recorder().callbackOrder() ==
              std::vector<std::string>{
                  "grant", "federate-save-initiate", "federation-saved"} &&
          constrained.recorder().callbackOrder() ==
              std::vector<std::string>{
                  "federate-save-initiate", "grant", "federation-saved"},
      "Timed save did not complete in callback order after callback-time resignation was rejected");

  constrained.rtiAmbassador().disableTimeConstrained();
  owner.rtiAmbassador().disableTimeRegulation();
  constrained.resign(rti::NO_ACTION);
  owner.resign(rti::NO_ACTION);
  owner.rtiAmbassador().destroyFederationExecution(federation);
  constrained.disconnect();
  owner.disconnect();
}

}  // namespace

void scenarioSaveActiveCallbackResignation(
    Options const& options, rti::CallbackModel model) {
  runSaveActiveCallbackResignation(options, model);
}

void scenarioSaveActiveCallbackResignationContract(
    Options const& options, rti::CallbackModel model) {
  runSaveActiveCallbackResignation(options, model);
}

void scenarioTimedSaveActiveCallbackResignation(
    Options const& options, rti::CallbackModel model) {
  runTimedSaveActiveCallbackResignation(options, model);
}

void scenarioTimedSaveActiveCallbackResignationContract(
    Options const& options, rti::CallbackModel model) {
  runTimedSaveActiveCallbackResignation(options, model);
}

}  // namespace hla_rti_cpp_tck::detail
