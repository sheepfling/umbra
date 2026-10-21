#include "tck_restore_pending_time_advance.hpp"

namespace hla_rti_cpp_tck::detail {
namespace {

void runRestorePendingTimeAdvance(
    Options const& options, rti::CallbackModel model) {
  require(model == rti::HLA_EVOKED,
          "Pending-TAR restore requires evoked callbacks");
  require(!options.fom.empty(),
          "Pending-TAR restore requires an adapter-supplied standard NRG-enabled FOM");
  require(!options.logicalTimeImplementationName.empty(),
          "Pending-TAR restore requires an adapter-supplied logical-time implementation");

  Session member(options, model, "member");
  auto const federation = federationName(options, "restore-pending-time-advance");
  std::wstring const saveLabel = L"tck-pending-time-advance-baseline";
  member.connect();
  member.rtiAmbassador().createFederationExecution(
      federation, options.fom.wstring(), options.logicalTimeImplementationName);
  member.join(options.ownerFederateName, options.federateType, federation);

  auto const time = makeTimeContext(member);
  auto const savedTarget = timeAfter(*time.factory, *time.initial, *time.epsilon, 5U);
  auto const invalidTarget = timeAfter(*time.factory, *time.initial, *time.epsilon, 6U);
  auto const laterTarget = timeAfter(*time.factory, *time.initial, *time.epsilon, 9U);
  auto const savedEncoded = encodeTime(*savedTarget);
  auto const laterEncoded = encodeTime(*laterTarget);
  auto queryTime = [&] {
    auto result = time.factory->makeInitial();
    require(result != nullptr, "Could not allocate Query Logical Time result");
    member.rtiAmbassador().queryLogicalTime(*result);
    return result;
  };

  member.rtiAmbassador().enableTimeConstrained();
  waitFor(member,
          [&] { return member.recorder().timeConstrainedEnabled().size() == 1U; },
          options, "pending-TAR constrained-role enablement");

  member.recorder().clearCallbackOrder();
  member.recorder().setFederateSaveInitiateHook([&] {
    member.rtiAmbassador().federateSaveBegun();
    member.rtiAmbassador().federateSaveComplete();
  });
  member.rtiAmbassador().requestFederationSave(saveLabel);
  require(member.federateSaveInitiations().empty(),
          "Evoked save callback arrived before callback servicing");
  member.rtiAmbassador().timeAdvanceRequest(*savedTarget);
  waitFor(member,
          [&] {
            return member.federateSaveInitiations().size() == 1U &&
                member.recorder().timeAdvanceGrants().size() == 1U;
          }, options, "pending-TAR save initiation and first grant");
  require(member.federateSaveInitiations() ==
                  std::vector<std::wstring>{saveLabel} &&
              member.recorder().timeAdvanceGrants().front().encoded == savedEncoded &&
              member.recorder().callbackOrder() ==
                  std::vector<std::string>{"federate-save-initiate", "grant"},
          "Pending TAR did not initiate save before the first grant");
  waitFor(member, [&] { return member.federationSavedCount() == 1U; },
          options, "pending-TAR federation save completion");
  require(member.federationNotSavedReasons().empty(),
          "Pending-TAR federation save failed");
  while (member.rtiAmbassador().evokeCallback(0.0)) {
  }

  member.recorder().setFederateSaveInitiateHook({});
  member.rtiAmbassador().timeAdvanceRequest(*laterTarget);
  waitFor(member,
          [&] { return member.recorder().timeAdvanceGrants().size() == 2U; },
          options, "post-save TAR grant");
  require(member.recorder().timeAdvanceGrants().back().encoded == laterEncoded &&
              *queryTime() == *laterTarget,
          "Post-save TAR did not reach the later logical time");

  member.recorder().clearCallbackOrder();
  member.rtiAmbassador().requestFederationRestore(saveLabel);
  waitFor(member,
          [&] { return member.federateRestoreInitiations().size() == 1U; },
          options, "pending-TAR restore initiation");
  require(member.federationRestoreRequestsSucceeded() ==
                  std::vector<std::wstring>{saveLabel} &&
              member.federationRestoreBegunCount() == 1U &&
              member.federateRestoreInitiations().front().label == saveLabel,
          "Pending-TAR restore did not begin from the saved label");
  member.rtiAmbassador().federateRestoreComplete();
  require(*queryTime() == *time.initial,
          "Pending-TAR restore did not return to initial logical time");
  requireException(
      [&] { member.rtiAmbassador().timeAdvanceRequest(*invalidTarget); },
      L"InTimeAdvancingState",
      "ordinary TAR while restored TAR is pending");

  static_cast<void>(member.rtiAmbassador().evokeCallback(0.0));
  require(member.federationRestoredCount() == 1U &&
              member.recorder().timeAdvanceGrants().size() == 2U,
          "Restored TAR grant arrived before Federation Restored");
  static_cast<void>(member.rtiAmbassador().evokeCallback(0.0));
  require(member.recorder().timeAdvanceGrants().size() == 3U &&
              member.recorder().timeAdvanceGrants().back().encoded == savedEncoded &&
              member.recorder().callbackOrder() ==
                  std::vector<std::string>{
                      "federation-restore-request-succeeded",
                      "federation-restore-begun",
                      "federate-restore-initiate",
                      "federation-restored",
                      "grant"} &&
              *queryTime() == *savedTarget,
          "Restored pending TAR did not grant its saved target after Federation Restored");
  require(member.federationNotRestoredReasons().empty(),
          "Pending-TAR federation restore failed");

  member.resign(rti::NO_ACTION);
  member.rtiAmbassador().destroyFederationExecution(federation);
  member.disconnect();
}

void runRestorePendingTimeAdvanceImmediate(
    Options const& options, rti::CallbackModel model) {
  require(model == rti::HLA_IMMEDIATE,
          "Immediate pending-TAR restore requires immediate callbacks");
  require(!options.fom.empty(),
          "Immediate pending-TAR restore requires an adapter-supplied standard NRG-enabled FOM");
  require(!options.logicalTimeImplementationName.empty(),
          "Immediate pending-TAR restore requires an adapter-supplied logical-time implementation");

  Session member(options, model, "member");
  auto const federation = federationName(options, "restore-pending-time-advance-immediate");
  std::wstring const saveLabel = L"tck-pending-time-advance-immediate";
  member.connect();
  member.rtiAmbassador().createFederationExecution(
      federation, options.fom.wstring(), options.logicalTimeImplementationName);
  member.join(options.ownerFederateName, options.federateType, federation);

  auto const time = makeTimeContext(member);
  auto const savedTarget = timeAfter(*time.factory, *time.initial, *time.epsilon, 5U);
  auto const laterTarget = timeAfter(*time.factory, *time.initial, *time.epsilon, 9U);
  auto const nextTarget = timeAfter(*time.factory, *time.initial, *time.epsilon, 6U);
  auto const savedEncoded = encodeTime(*savedTarget);
  auto const laterEncoded = encodeTime(*laterTarget);
  auto const nextEncoded = encodeTime(*nextTarget);
  auto queryTime = [&] {
    auto result = time.factory->makeInitial();
    require(result != nullptr, "Could not allocate Query Logical Time result");
    member.rtiAmbassador().queryLogicalTime(*result);
    return result;
  };

  member.rtiAmbassador().enableTimeConstrained();
  require(member.recorder().timeConstrainedEnabled().size() == 1U,
          "Immediate time-constrained enablement was not delivered directly");

  member.recorder().clearCallbackOrder();
  member.recorder().setFederateSaveInitiateHook([&] {
    member.rtiAmbassador().federateSaveBegun();
    member.rtiAmbassador().federateSaveComplete();
  });
  member.rtiAmbassador().requestFederationSave(saveLabel);
  member.rtiAmbassador().timeAdvanceRequest(*savedTarget);
  require(member.federateSaveInitiations() ==
                  std::vector<std::wstring>{saveLabel} &&
              member.federationSavedCount() == 1U &&
              member.federationNotSavedReasons().empty() &&
              member.recorder().timeAdvanceGrants().size() == 1U &&
              member.recorder().timeAdvanceGrants().back().encoded == savedEncoded &&
              member.recorder().callbackOrder() ==
                  std::vector<std::string>{
                      "federate-save-initiate", "federation-saved", "grant"},
          "Immediate save did not complete re-entrantly before its first grant");

  member.recorder().setFederateSaveInitiateHook({});
  member.rtiAmbassador().timeAdvanceRequest(*laterTarget);
  require(member.recorder().timeAdvanceGrants().size() == 2U &&
              member.recorder().timeAdvanceGrants().back().encoded == laterEncoded &&
              *queryTime() == *laterTarget,
          "Immediate post-save TAR did not reach the later time");

  member.recorder().clearCallbackOrder();
  member.rtiAmbassador().requestFederationRestore(saveLabel);
  require(member.federationRestoreRequestsSucceeded() ==
                  std::vector<std::wstring>{saveLabel} &&
              member.federationRestoreBegunCount() == 1U &&
              member.federateRestoreInitiations().size() == 1U &&
              member.federateRestoreInitiations().front().label == saveLabel,
          "Immediate restore control callbacks were not delivered directly");
  member.rtiAmbassador().federateRestoreComplete();
  require(member.federationRestoredCount() == 1U &&
              member.federationNotRestoredReasons().empty() &&
              member.recorder().timeAdvanceGrants().size() == 3U &&
              member.recorder().timeAdvanceGrants().back().encoded == savedEncoded &&
              member.recorder().callbackOrder() ==
                  std::vector<std::string>{
                      "federation-restore-request-succeeded",
                      "federation-restore-begun",
                      "federate-restore-initiate",
                      "federation-restored",
                      "grant"} &&
              *queryTime() == *savedTarget,
          "Immediate restore completion did not precede the reconstructed grant");

  member.rtiAmbassador().timeAdvanceRequest(*nextTarget);
  require(member.recorder().timeAdvanceGrants().size() == 4U &&
              member.recorder().timeAdvanceGrants().back().encoded == nextEncoded &&
              *queryTime() == *nextTarget,
          "Immediate restored grant did not release Time Advancing state");

  member.resign(rti::NO_ACTION);
  member.rtiAmbassador().destroyFederationExecution(federation);
  member.disconnect();
}

}  // namespace

void scenarioRestorePendingTimeAdvance(
    Options const& options, rti::CallbackModel model) {
  runRestorePendingTimeAdvance(options, model);
}

void scenarioRestorePendingTimeAdvanceContract(
    Options const& options, rti::CallbackModel model) {
  runRestorePendingTimeAdvance(options, model);
}

void scenarioRestorePendingTimeAdvanceImmediate(
    Options const& options, rti::CallbackModel model) {
  runRestorePendingTimeAdvanceImmediate(options, model);
}

void scenarioRestorePendingTimeAdvanceImmediateContract(
    Options const& options, rti::CallbackModel model) {
  runRestorePendingTimeAdvanceImmediate(options, model);
}

}  // namespace hla_rti_cpp_tck::detail
