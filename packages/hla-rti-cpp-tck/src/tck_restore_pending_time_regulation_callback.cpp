#include "tck_restore_pending_time_regulation_callback.hpp"

namespace hla_rti_cpp_tck::detail {
namespace {

void runRestorePendingTimeRoleCallback(Options const& options,
                                       rti::CallbackModel model,
                                       bool constrained) {
  require(!options.fom.empty() && !options.logicalTimeImplementationName.empty(),
          "Pending time-role restore requires adapter FOM and time implementation");
  Session member(options, model, "owner");
  auto const federation = federationName(
      options, constrained ? "restore-pending-time-constrained-callback"
                           : "restore-pending-time-regulation-callback");
  std::wstring const saveLabel =
      constrained
          ? (model == rti::HLA_EVOKED ? L"tck-pending-constrained-evoked"
                                       : L"tck-pending-constrained-immediate")
          : (model == rti::HLA_EVOKED ? L"tck-pending-regulation-evoked"
                                       : L"tck-pending-regulation-immediate");

  member.connect();
  auto& ambassador = member.rtiAmbassador();
  ambassador.createFederationExecution(
      federation, options.fom.wstring(), options.logicalTimeImplementationName);
  member.join(options.ownerFederateName, options.federateType, federation);
  auto const time = makeTimeContext(member);
  if (model == rti::HLA_IMMEDIATE) ambassador.disableCallbacks();
  if (constrained) {
    ambassador.enableTimeConstrained();
  } else {
    auto lookahead = time.factory->makeZero();
    require(lookahead != nullptr, "Time factory returned no lookahead interval");
    *lookahead += *time.epsilon;
    *lookahead += *time.epsilon;
    ambassador.enableTimeRegulation(*lookahead);
  }
  auto roleEnabled = [&] {
    return constrained ? member.recorder().timeConstrainedEnabled()
                       : member.recorder().timeRegulationEnabled();
  };
  require(roleEnabled().empty(), "The pending time-role callback was not gated");

  // The native public-API case intentionally completes the save while the
  // callback queue still contains the pre-restore role-enable request.
  ambassador.requestFederationSave(saveLabel);
  ambassador.federateSaveBegun();
  ambassador.federateSaveComplete();
  require(member.federationSavedCount() == 0U,
          "The saved callback was delivered before the restore boundary");
  ambassador.requestFederationRestore(saveLabel);
  ambassador.federateRestoreComplete();

  if (model == rti::HLA_IMMEDIATE) ambassador.enableCallbacks();
  waitForSessions({&member}, [&] {
    return roleEnabled().size() >= 1U &&
           member.federationSavedCount() >= 1U &&
           member.federationRestoredCount() >= 1U;
  }, options, "rebound time-regulation and restore callbacks");
  auto const enabled = roleEnabled();
  require(enabled.size() == 1U &&
              enabled.front().encoded == copyBytes(time.initial->encode()) &&
              (constrained ? member.recorder().timeConstrainedEnablementOrder()
                           : member.recorder().timeRegulationEnablementOrder()) ==
                  std::vector<std::string>{constrained ? "time-constrained-enabled"
                                                        : "regulation-enabled"} &&
              member.federationSavedCount() == 1U &&
              member.federationRestoredCount() == 1U,
          "Restore duplicated or lost the pending time-role callback");

  member.resign(rti::NO_ACTION);
  ambassador.destroyFederationExecution(federation);
  member.disconnect();
}

}  // namespace

void scenarioRestorePendingTimeRegulationCallback(
    Options const& options, rti::CallbackModel model) {
  runRestorePendingTimeRoleCallback(options, model, false);
}

void scenarioRestorePendingTimeRegulationCallbackContract(
    Options const& options, rti::CallbackModel model) {
  runRestorePendingTimeRoleCallback(options, model, false);
}

void scenarioRestorePendingTimeConstrainedCallback(
    Options const& options, rti::CallbackModel model) {
  runRestorePendingTimeRoleCallback(options, model, true);
}

void scenarioRestorePendingTimeConstrainedCallbackContract(
    Options const& options, rti::CallbackModel model) {
  runRestorePendingTimeRoleCallback(options, model, true);
}

}  // namespace hla_rti_cpp_tck::detail
