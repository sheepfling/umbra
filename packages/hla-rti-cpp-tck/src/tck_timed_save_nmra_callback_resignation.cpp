#include "tck_timed_save_nmra_callback_resignation.hpp"

namespace hla_rti_cpp_tck::detail {
namespace {

void runTimedSaveNmraCallbackResignation(
    Options const& options, rti::CallbackModel model) {
  require(model == rti::HLA_EVOKED,
          "Timed NMRA save-callback resignation requires evoked callbacks");
  require(!options.fom.empty(),
          "Timed NMRA save-callback resignation requires an adapter-supplied FOM");
  require(!options.logicalTimeImplementationName.empty(),
          "Timed NMRA save-callback resignation requires an adapter-supplied logical-time implementation");

  Session owner(options, model, "owner");
  Session nmra(options, model, "member");
  auto const federation = federationName(options, "timed-save-nmra-callback-resignation");
  std::wstring const saveLabel = L"tck-timed-nmra-callback-resignation-save";
  connectAndJoin(owner, nmra, options, federation, options.fom);

  auto const ownerTime = makeTimeContext(owner);
  auto const nmraTime = makeTimeContext(nmra);
  require(ownerTime.factory->getName() == nmraTime.factory->getName(),
          "Timed NMRA save members selected different logical-time factories");
  auto const saveTime = timeAfter(*ownerTime.factory, *ownerTime.initial,
                                  *ownerTime.epsilon, 5U);
  auto const advanceTime = timeAfter(*ownerTime.factory, *ownerTime.initial,
                                     *ownerTime.epsilon, 6U);

  nmra.rtiAmbassador().enableTimeConstrained();
  waitFor(nmra,
          [&] { return nmra.recorder().timeConstrainedEnabled().size() == 1U; },
          options, "timed NMRA save constrained-role enablement");
  owner.rtiAmbassador().enableTimeRegulation(*ownerTime.epsilon);
  waitFor(owner,
          [&] { return owner.recorder().timeRegulationEnabled().size() == 1U; },
          options, "timed NMRA save regulator enablement");
  owner.recorder().clearCallbackOrder();
  nmra.recorder().clearCallbackOrder();

  bool resignationRejectedInsideCallback = false;
  nmra.recorder().setFederateSaveInitiateHook([&] {
    try {
      nmra.rtiAmbassador().resignFederationExecution(rti::NO_ACTION);
    } catch (rti::CallNotAllowedFromWithinCallback const&) {
      resignationRejectedInsideCallback = true;
    }
  });

  owner.rtiAmbassador().requestFederationSave(saveLabel, *saveTime);
  nmra.rtiAmbassador().nextMessageRequestAvailable(*advanceTime);
  owner.rtiAmbassador().timeAdvanceRequest(*saveTime);

  waitFor(owner,
          [&] { return owner.recorder().timeAdvanceGrants().size() == 1U; },
          options, "timed NMRA save regulator grant");
  require(owner.recorder().timeAdvanceGrants().front().encoded ==
              encodeTime(*saveTime) &&
              owner.federateSaveInitiations().empty(),
          "Regulator did not grant the save time before its initiation callback");

  waitFor(nmra,
          [&] {
            return nmra.federateSaveInitiations().size() == 1U &&
                nmra.recorder().timeAdvanceGrants().size() == 1U;
          }, options, "timed NMRA save initiation and grant");
  require(resignationRejectedInsideCallback &&
              nmra.federateSaveInitiations() ==
                  std::vector<std::wstring>{saveLabel} &&
              nmra.federateSaveTimes().size() == 1U &&
              nmra.federateSaveTimes().front().encoded == encodeTime(*saveTime) &&
              nmra.recorder().timeAdvanceGrants().front().encoded ==
                  encodeTime(*advanceTime) &&
              nmra.recorder().callbackOrder() ==
                  std::vector<std::string>{"federate-save-initiate", "grant"},
          "Timed NMRA callback did not reject resignation before its grant");

  waitFor(owner,
          [&] { return owner.federateSaveInitiations().size() == 1U; },
          options, "timed NMRA save owner initiation");
  require(owner.federateSaveInitiations() ==
              std::vector<std::wstring>{saveLabel} &&
              owner.federateSaveTimes().size() == 1U &&
              owner.federateSaveTimes().front().encoded ==
                  encodeTime(*saveTime) &&
              owner.recorder().callbackOrder() ==
                  std::vector<std::string>{"grant", "federate-save-initiate"},
          "Owner did not receive grant before its timed save initiation");

  owner.rtiAmbassador().federateSaveBegun();
  nmra.rtiAmbassador().federateSaveBegun();
  owner.rtiAmbassador().federateSaveComplete();
  nmra.rtiAmbassador().federateSaveComplete();
  waitForSessions(
      {&owner, &nmra},
      [&] {
        return owner.federationSavedCount() == 1U &&
            nmra.federationSavedCount() == 1U;
      }, options, "timed NMRA save completion");
  require(owner.federationNotSavedReasons().empty() &&
              nmra.federationNotSavedReasons().empty() &&
              owner.recorder().callbackOrder() ==
                  std::vector<std::string>{"grant", "federate-save-initiate",
                                           "federation-saved"} &&
              nmra.recorder().callbackOrder() ==
                  std::vector<std::string>{"federate-save-initiate", "grant",
                                           "federation-saved"},
          "Rejected callback resignation incorrectly failed the timed NMRA save");

  nmra.rtiAmbassador().disableTimeConstrained();
  owner.rtiAmbassador().disableTimeRegulation();
  nmra.resign(rti::NO_ACTION);
  owner.resign(rti::NO_ACTION);
  owner.rtiAmbassador().destroyFederationExecution(federation);
  nmra.disconnect();
  owner.disconnect();
}

}  // namespace

void scenarioTimedSaveNmraCallbackResignation(
    Options const& options, rti::CallbackModel model) {
  runTimedSaveNmraCallbackResignation(options, model);
}

void scenarioTimedSaveNmraCallbackResignationContract(
    Options const& options, rti::CallbackModel model) {
  runTimedSaveNmraCallbackResignation(options, model);
}

}  // namespace hla_rti_cpp_tck::detail
