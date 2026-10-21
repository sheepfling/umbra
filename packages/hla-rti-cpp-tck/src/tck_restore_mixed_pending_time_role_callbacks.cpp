#include "tck_restore_mixed_pending_time_role_callbacks.hpp"

#include "tck_main_scenarios.hpp"

namespace hla_rti_cpp_tck::detail {
namespace {

void runRestoreMixedPendingTimeRoleCallbacks(Options const& options,
                                             rti::CallbackModel model) {
  require(!options.fom.empty() && !options.logicalTimeImplementationName.empty(),
          "Mixed pending time-role restore requires adapter FOM and time implementation");
  Session regulator(options, model, "owner");
  Session constrained(options, model, "member");
  auto const federation = federationName(options, "restore-mixed-pending-time-role-callbacks");
  std::wstring const saveLabel = model == rti::HLA_EVOKED
      ? L"tck-mixed-pending-role-evoked"
      : L"tck-mixed-pending-role-immediate";

  regulator.connect();
  constrained.connect();
  regulator.rtiAmbassador().createFederationExecution(
      federation, options.fom.wstring(), options.logicalTimeImplementationName);
  regulator.join(options.ownerFederateName, options.federateType + L"-regulator", federation);
  constrained.join(options.memberFederateName, options.federateType + L"-constrained", federation);

  auto& regulationAmbassador = regulator.rtiAmbassador();
  auto& constrainedAmbassador = constrained.rtiAmbassador();
  if (model == rti::HLA_IMMEDIATE) {
    regulationAmbassador.disableCallbacks();
    constrainedAmbassador.disableCallbacks();
  }
  auto const time = makeTimeContext(regulator);
  auto lookahead = time.factory->makeZero();
  require(lookahead != nullptr, "Time factory returned no lookahead interval");
  *lookahead += *time.epsilon;
  regulationAmbassador.enableTimeRegulation(*lookahead);
  constrainedAmbassador.enableTimeConstrained();
  require(regulator.recorder().timeRegulationEnabled().empty() &&
              constrained.recorder().timeConstrainedEnabled().empty(),
          "A pending time-role callback was delivered before the save boundary");

  regulationAmbassador.requestFederationSave(saveLabel);
  regulationAmbassador.federateSaveBegun();
  regulationAmbassador.federateSaveComplete();
  constrainedAmbassador.federateSaveBegun();
  constrainedAmbassador.federateSaveComplete();
  require(regulator.federationSavedCount() == 0U &&
              constrained.federationSavedCount() == 0U,
          "Save callbacks were delivered before the restore boundary");

  regulationAmbassador.requestFederationRestore(saveLabel);
  regulationAmbassador.federateRestoreComplete();
  constrainedAmbassador.federateRestoreComplete();

  if (model == rti::HLA_IMMEDIATE) {
    regulationAmbassador.enableCallbacks();
  } else {
    while (regulationAmbassador.evokeCallback(0.0)) {
    }
  }
  require(regulator.recorder().timeRegulationEnabled().size() == 1U &&
              constrained.recorder().timeConstrainedEnabled().empty() &&
              regulator.federationRestoredCount() == 1U &&
              constrained.federationRestoredCount() == 0U,
          "First restored route state: regulation=" +
              std::to_string(regulator.recorder().timeRegulationEnabled().size()) +
              ", constrained=" +
              std::to_string(constrained.recorder().timeConstrainedEnabled().size()) +
              ", regulator-restored=" +
              std::to_string(regulator.federationRestoredCount()) +
              ", constrained-restored=" +
              std::to_string(constrained.federationRestoredCount()));

  if (model == rti::HLA_IMMEDIATE) {
    constrainedAmbassador.enableCallbacks();
  } else {
    while (constrainedAmbassador.evokeCallback(0.0)) {
    }
  }
  waitForSessions({&regulator, &constrained}, [&] {
    return regulator.federationSavedCount() == 1U &&
           constrained.federationSavedCount() == 1U &&
           regulator.federationRestoredCount() == 1U &&
           constrained.federationRestoredCount() == 1U;
  }, options, "mixed pending time-role save/restore callbacks");

  auto const regulationReports = regulator.recorder().timeRegulationEnabled();
  auto const constrainedReports = constrained.recorder().timeConstrainedEnabled();
  require(regulationReports.size() == 1U && constrainedReports.size() == 1U &&
              regulationReports.front().encoded == copyBytes(time.initial->encode()) &&
              constrainedReports.front().encoded == copyBytes(time.initial->encode()) &&
              regulator.recorder().timeRegulationEnablementOrder() ==
                  std::vector<std::string>{"regulation-enabled"} &&
              constrained.recorder().timeConstrainedEnablementOrder() ==
                  std::vector<std::string>{"time-constrained-enabled"},
          "Restore lost, duplicated, or cross-routed pending time-role callbacks");

  regulator.resign(rti::NO_ACTION);
  constrained.resign(rti::NO_ACTION);
  regulationAmbassador.destroyFederationExecution(federation);
  constrained.disconnect();
  regulator.disconnect();
}

}  // namespace

void scenarioRestoreMixedPendingTimeRoleCallbacks(
    Options const& options, rti::CallbackModel model) {
  runRestoreMixedPendingTimeRoleCallbacks(options, model);
}

void scenarioRestoreMixedPendingTimeRoleCallbacksContract(
    Options const& options, rti::CallbackModel model) {
  runRestoreMixedPendingTimeRoleCallbacks(options, model);
}

}  // namespace hla_rti_cpp_tck::detail
