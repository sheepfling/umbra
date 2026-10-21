#include "tck_timed_save_tara_callback_resignation.hpp"

namespace hla_rti_cpp_tck::detail {
namespace {

void runTimedSaveTaraCallbackResignation(
    Options const& options, rti::CallbackModel model) {
  require(model == rti::HLA_EVOKED,
          "Timed TARA save-callback resignation requires evoked callbacks");
  require(!options.fom.empty(),
          "Timed TARA save-callback resignation requires an adapter-supplied FOM");
  require(!options.logicalTimeImplementationName.empty(),
          "Timed TARA save-callback resignation requires an adapter-supplied logical-time implementation");

  Session owner(options, model, "owner");
  Session tara(options, model, "member");
  auto const federation = federationName(options, "timed-save-tara-callback-resignation");
  std::wstring const saveLabel = L"tck-timed-tara-callback-resignation-save";
  connectAndJoin(owner, tara, options, federation, options.fom);

  auto const ownerTime = makeTimeContext(owner);
  auto const taraTime = makeTimeContext(tara);
  require(ownerTime.factory->getName() == taraTime.factory->getName(),
          "Timed TARA save members selected different logical-time factories");
  auto const saveTime = timeAfter(*ownerTime.factory, *ownerTime.initial,
                                  *ownerTime.epsilon, 5U);
  auto const advanceTime = timeAfter(*ownerTime.factory, *ownerTime.initial,
                                     *ownerTime.epsilon, 6U);

  tara.rtiAmbassador().enableTimeConstrained();
  waitFor(tara,
          [&] { return tara.recorder().timeConstrainedEnabled().size() == 1U; },
          options, "timed TARA save constrained-role enablement");
  owner.rtiAmbassador().enableTimeRegulation(*ownerTime.epsilon);
  waitFor(owner,
          [&] { return owner.recorder().timeRegulationEnabled().size() == 1U; },
          options, "timed TARA save regulator enablement");
  owner.recorder().clearCallbackOrder();
  tara.recorder().clearCallbackOrder();

  bool resignationRejectedInsideCallback = false;
  tara.recorder().setFederateSaveInitiateHook([&] {
    try {
      tara.rtiAmbassador().resignFederationExecution(rti::NO_ACTION);
    } catch (rti::CallNotAllowedFromWithinCallback const&) {
      resignationRejectedInsideCallback = true;
    }
  });

  owner.rtiAmbassador().requestFederationSave(saveLabel, *saveTime);
  tara.rtiAmbassador().timeAdvanceRequestAvailable(*advanceTime);
  owner.rtiAmbassador().timeAdvanceRequest(*saveTime);

  waitFor(owner,
          [&] { return owner.recorder().timeAdvanceGrants().size() == 1U; },
          options, "timed TARA save regulator grant");
  require(owner.recorder().timeAdvanceGrants().front().encoded ==
              encodeTime(*saveTime) &&
              owner.federateSaveInitiations().empty(),
          "Regulator did not grant the save time before its initiation callback");

  waitFor(tara,
          [&] {
            return tara.federateSaveInitiations().size() == 1U &&
                tara.recorder().timeAdvanceGrants().size() == 1U;
          }, options, "timed TARA save initiation and grant");
  require(resignationRejectedInsideCallback &&
              tara.federateSaveInitiations() ==
                  std::vector<std::wstring>{saveLabel} &&
              tara.federateSaveTimes().size() == 1U &&
              tara.federateSaveTimes().front().encoded == encodeTime(*saveTime) &&
              tara.recorder().timeAdvanceGrants().front().encoded ==
                  encodeTime(*advanceTime) &&
              tara.recorder().callbackOrder() ==
                  std::vector<std::string>{"federate-save-initiate", "grant"},
          "Timed TARA callback did not reject resignation before its grant");

  waitFor(owner,
          [&] { return owner.federateSaveInitiations().size() == 1U; },
          options, "timed TARA save owner initiation");
  require(owner.federateSaveInitiations() ==
              std::vector<std::wstring>{saveLabel} &&
              owner.federateSaveTimes().size() == 1U &&
              owner.federateSaveTimes().front().encoded ==
                  encodeTime(*saveTime) &&
              owner.recorder().callbackOrder() ==
                  std::vector<std::string>{"grant", "federate-save-initiate"},
          "Owner did not receive grant before its timed save initiation");

  owner.rtiAmbassador().federateSaveBegun();
  tara.rtiAmbassador().federateSaveBegun();
  owner.rtiAmbassador().federateSaveComplete();
  tara.rtiAmbassador().federateSaveComplete();
  waitForSessions(
      {&owner, &tara},
      [&] {
        return owner.federationSavedCount() == 1U &&
            tara.federationSavedCount() == 1U;
      }, options, "timed TARA save completion");
  require(owner.federationNotSavedReasons().empty() &&
              tara.federationNotSavedReasons().empty() &&
              owner.recorder().callbackOrder() ==
                  std::vector<std::string>{"grant", "federate-save-initiate",
                                           "federation-saved"} &&
              tara.recorder().callbackOrder() ==
                  std::vector<std::string>{"federate-save-initiate", "grant",
                                           "federation-saved"},
          "Rejected callback resignation incorrectly failed the timed TARA save");

  tara.rtiAmbassador().disableTimeConstrained();
  owner.rtiAmbassador().disableTimeRegulation();
  tara.resign(rti::NO_ACTION);
  owner.resign(rti::NO_ACTION);
  owner.rtiAmbassador().destroyFederationExecution(federation);
  tara.disconnect();
  owner.disconnect();
}

}  // namespace

void scenarioTimedSaveTaraCallbackResignation(
    Options const& options, rti::CallbackModel model) {
  runTimedSaveTaraCallbackResignation(options, model);
}

void scenarioTimedSaveTaraCallbackResignationContract(
    Options const& options, rti::CallbackModel model) {
  runTimedSaveTaraCallbackResignation(options, model);
}

}  // namespace hla_rti_cpp_tck::detail
