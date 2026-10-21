#include "tck_timed_save_fqr_callback_resignation.hpp"

namespace hla_rti_cpp_tck::detail {
namespace {

void runTimedSaveFqrCallbackResignation(
    Options const& options, rti::CallbackModel model) {
  require(model == rti::HLA_EVOKED,
          "Timed FQR save-callback resignation requires evoked callbacks");
  require(!options.fom.empty(),
          "Timed FQR save-callback resignation requires an adapter-supplied FOM");
  require(!options.logicalTimeImplementationName.empty(),
          "Timed FQR save-callback resignation requires an adapter-supplied logical-time implementation");

  Session owner(options, model, "owner");
  Session fqr(options, model, "member");
  auto const federation = federationName(options, "timed-save-fqr-callback-resignation");
  std::wstring const saveLabel = L"tck-timed-fqr-callback-resignation-save";
  connectAndJoin(owner, fqr, options, federation, options.fom);

  auto const ownerTime = makeTimeContext(owner);
  auto const fqrTime = makeTimeContext(fqr);
  require(ownerTime.factory->getName() == fqrTime.factory->getName(),
          "Timed FQR save members selected different logical-time factories");
  auto const saveTime = timeAfter(*ownerTime.factory, *ownerTime.initial,
                                  *ownerTime.epsilon, 5U);
  auto const flushTime = timeAfter(*ownerTime.factory, *ownerTime.initial,
                                   *ownerTime.epsilon, 6U);

  fqr.rtiAmbassador().enableTimeConstrained();
  waitFor(fqr,
          [&] { return fqr.recorder().timeConstrainedEnabled().size() == 1U; },
          options, "timed FQR save constrained-role enablement");
  owner.rtiAmbassador().enableTimeRegulation(*ownerTime.epsilon);
  waitFor(owner,
          [&] { return owner.recorder().timeRegulationEnabled().size() == 1U; },
          options, "timed FQR save regulator enablement");
  owner.recorder().clearCallbackOrder();
  fqr.recorder().clearCallbackOrder();

  bool resignationRejectedInsideCallback = false;
  fqr.recorder().setFederateSaveInitiateHook([&] {
    try {
      fqr.rtiAmbassador().resignFederationExecution(rti::NO_ACTION);
    } catch (rti::CallNotAllowedFromWithinCallback const&) {
      resignationRejectedInsideCallback = true;
    }
  });

  owner.rtiAmbassador().requestFederationSave(saveLabel, *saveTime);
  fqr.rtiAmbassador().flushQueueRequest(*flushTime);
  owner.rtiAmbassador().timeAdvanceRequest(*saveTime);

  waitFor(owner,
          [&] { return owner.recorder().timeAdvanceGrants().size() == 1U; },
          options, "timed FQR save regulator grant");
  require(owner.recorder().timeAdvanceGrants().front().encoded ==
              encodeTime(*saveTime) &&
              owner.federateSaveInitiations().empty(),
          "Regulator did not grant the save time before its initiation callback");

  waitFor(fqr,
          [&] {
            return fqr.federateSaveInitiations().size() == 1U &&
                fqr.recorder().flushQueueGrants().size() == 1U;
          }, options, "timed FQR save initiation and flush grant");
  require(resignationRejectedInsideCallback &&
              fqr.federateSaveInitiations() ==
                  std::vector<std::wstring>{saveLabel} &&
              fqr.federateSaveTimes().size() == 1U &&
              fqr.federateSaveTimes().front().encoded == encodeTime(*saveTime) &&
              fqr.recorder().flushQueueGrants().front().time ==
                  encodeTime(*flushTime) &&
              fqr.recorder().callbackOrder() ==
                  std::vector<std::string>{"federate-save-initiate", "flush-grant"},
          "Timed FQR callback did not reject resignation before its flush grant");

  waitFor(owner,
          [&] { return owner.federateSaveInitiations().size() == 1U; },
          options, "timed FQR save owner initiation");
  require(owner.federateSaveInitiations() ==
              std::vector<std::wstring>{saveLabel} &&
              owner.federateSaveTimes().size() == 1U &&
              owner.federateSaveTimes().front().encoded ==
                  encodeTime(*saveTime) &&
              owner.recorder().callbackOrder() ==
                  std::vector<std::string>{"grant", "federate-save-initiate"},
          "Owner did not receive grant before its timed save initiation");

  owner.rtiAmbassador().federateSaveBegun();
  fqr.rtiAmbassador().federateSaveBegun();
  owner.rtiAmbassador().federateSaveComplete();
  fqr.rtiAmbassador().federateSaveComplete();
  waitForSessions(
      {&owner, &fqr},
      [&] {
        return owner.federationSavedCount() == 1U &&
            fqr.federationSavedCount() == 1U;
      }, options, "timed FQR save completion");
  require(owner.federationNotSavedReasons().empty() &&
              fqr.federationNotSavedReasons().empty() &&
              owner.recorder().callbackOrder() ==
                  std::vector<std::string>{"grant", "federate-save-initiate",
                                           "federation-saved"} &&
              fqr.recorder().callbackOrder() ==
                  std::vector<std::string>{"federate-save-initiate",
                                           "flush-grant", "federation-saved"},
          "Rejected callback resignation incorrectly failed the timed FQR save");

  fqr.rtiAmbassador().disableTimeConstrained();
  owner.rtiAmbassador().disableTimeRegulation();
  fqr.resign(rti::NO_ACTION);
  owner.resign(rti::NO_ACTION);
  owner.rtiAmbassador().destroyFederationExecution(federation);
  fqr.disconnect();
  owner.disconnect();
}

}  // namespace

void scenarioTimedSaveFqrCallbackResignation(
    Options const& options, rti::CallbackModel model) {
  runTimedSaveFqrCallbackResignation(options, model);
}

void scenarioTimedSaveFqrCallbackResignationContract(
    Options const& options, rti::CallbackModel model) {
  runTimedSaveFqrCallbackResignation(options, model);
}

}  // namespace hla_rti_cpp_tck::detail
