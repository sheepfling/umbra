#include "tck_restore_resignation_callback_lifetime.hpp"

namespace hla_rti_cpp_tck::detail {
namespace {

void runRestoreResignationCallbackLifetime(
    Options const& options, rti::CallbackModel model) {
  require(model == rti::HLA_EVOKED,
          "Restore resignation callback lifetime requires evoked callbacks");
  require(!options.fom.empty(),
          "Restore resignation callback lifetime requires an adapter-supplied FOM");

  Session survivor(options, model, "owner");
  Session departing(options, model, "member");
  auto const federation = federationName(options, "restore-resignation-callback-lifetime");
  std::wstring const label = L"tck-restore-resignation-baseline";

  survivor.connect();
  departing.connect();
  survivor.rtiAmbassador().createFederationExecution(
      federation, options.fom.wstring());
  survivor.join(options.ownerFederateName, options.federateType, federation);
  departing.join(options.memberFederateName, options.federateType, federation);

  survivor.rtiAmbassador().requestFederationSave(label);
  waitForSessions(
      {&survivor, &departing},
      [&] {
        return survivor.federateSaveInitiations().size() == 1U &&
            departing.federateSaveInitiations().size() == 1U;
      }, options, "both save initiations before restore resignation");
  require(survivor.federateSaveInitiations() ==
                  std::vector<std::wstring>{label} &&
              departing.federateSaveInitiations() ==
                  std::vector<std::wstring>{label},
          "Baseline save did not initiate for both members");
  survivor.rtiAmbassador().federateSaveBegun();
  departing.rtiAmbassador().federateSaveBegun();
  survivor.rtiAmbassador().federateSaveComplete();
  departing.rtiAmbassador().federateSaveComplete();
  waitForSessions(
      {&survivor, &departing},
      [&] {
        return survivor.federationSavedCount() == 1U &&
            departing.federationSavedCount() == 1U;
      }, options, "both baseline federation-saved callbacks");
  require(survivor.federationNotSavedReasons().empty() &&
              departing.federationNotSavedReasons().empty(),
          "Baseline save failed");
  while (survivor.rtiAmbassador().evokeCallback(0.0)) {
  }
  while (departing.rtiAmbassador().evokeCallback(0.0)) {
  }

  survivor.recorder().clearCallbackOrder();
  departing.recorder().clearCallbackOrder();
  survivor.rtiAmbassador().requestFederationRestore(label);
  require(survivor.recorder().callbackOrder().empty() &&
              departing.recorder().callbackOrder().empty(),
          "Evoked restore callbacks arrived before callback servicing");

  // The departing member has not evoked any restore callback.  Resignation
  // invalidates its queued restore work and fails the survivor's restore.
  departing.resign(rti::NO_ACTION);
  waitFor(survivor,
          [&] { return survivor.federationNotRestoredReasons().size() == 1U; },
          options, "survivor restore failure after peer resignation");
  require(survivor.federationRestoreRequestsSucceeded() ==
                  std::vector<std::wstring>{label} &&
              survivor.federationRestoreBegunCount() == 1U &&
              survivor.federateRestoreInitiations().size() == 1U &&
              survivor.federateRestoreInitiations().front().label == label &&
              survivor.federationNotRestoredReasons() ==
                  std::vector<rti::RestoreFailureReason>{
                      rti::FEDERATE_RESIGNED_DURING_RESTORE} &&
              survivor.recorder().callbackOrder() ==
                  std::vector<std::string>{
                      "federation-restore-request-succeeded",
                      "federation-restore-begun",
                      "federate-restore-initiate",
                      "federation-not-restored"},
          "Survivor did not receive queued restore initiation before resignation failure");

  while (departing.rtiAmbassador().evokeCallback(0.0)) {
  }
  require(departing.recorder().callbackOrder().empty() &&
              departing.federationRestoreRequestsSucceeded().empty() &&
              departing.federationRestoreBegunCount() == 0U &&
              departing.federateRestoreInitiations().empty() &&
              departing.federationNotRestoredReasons().empty(),
          "Resigned member received stale public restore callbacks");

  survivor.resign(rti::NO_ACTION);
  survivor.rtiAmbassador().destroyFederationExecution(federation);
  departing.disconnect();
  survivor.disconnect();
}

}  // namespace

void scenarioRestoreResignationCallbackLifetime(
    Options const& options, rti::CallbackModel model) {
  runRestoreResignationCallbackLifetime(options, model);
}

void scenarioRestoreResignationCallbackLifetimeContract(
    Options const& options, rti::CallbackModel model) {
  runRestoreResignationCallbackLifetime(options, model);
}

}  // namespace hla_rti_cpp_tck::detail
