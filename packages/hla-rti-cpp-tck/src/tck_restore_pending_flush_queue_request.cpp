#include "tck_restore_pending_flush_queue_request.hpp"

namespace hla_rti_cpp_tck::detail {
namespace {

void runRestorePendingFlushQueueRequest(
    Options const& options, rti::CallbackModel model) {
  require(model == rti::HLA_EVOKED,
          "Pending-FQR restore requires evoked callbacks");
  require(!options.fom.empty(),
          "Pending-FQR restore requires an adapter-supplied FOM");
  require(!options.logicalTimeImplementationName.empty(),
          "Pending-FQR restore requires an adapter-supplied logical-time implementation");

  Session member(options, model, "member");
  auto const federation = federationName(options, "restore-pending-flush-queue-request");
  std::wstring const saveLabel = L"tck-pending-flush-queue";
  member.connect();
  member.rtiAmbassador().createFederationExecution(
      federation, options.fom.wstring(), options.logicalTimeImplementationName);
  member.join(options.ownerFederateName, options.federateType, federation);

  auto const time = makeTimeContext(member);
  auto const saveTime = timeAfter(*time.factory, *time.initial, *time.epsilon, 5U);
  auto const flushTime = timeAfter(*time.factory, *time.initial, *time.epsilon, 6U);
  auto const invalidTime = timeAfter(*time.factory, *time.initial, *time.epsilon, 7U);
  auto const laterTime = timeAfter(*time.factory, *time.initial, *time.epsilon, 9U);
  auto const flushEncoded = encodeTime(*flushTime);
  auto const laterEncoded = encodeTime(*laterTime);
  auto queryTime = [&] {
    auto result = time.factory->makeInitial();
    require(result != nullptr, "Could not allocate Query Logical Time result");
    member.rtiAmbassador().queryLogicalTime(*result);
    return result;
  };

  member.rtiAmbassador().enableTimeConstrained();
  waitFor(member,
          [&] { return member.recorder().timeConstrainedEnabled().size() == 1U; },
          options, "pending-FQR constrained-role enablement");
  member.rtiAmbassador().enableTimeRegulation(*time.epsilon);
  waitFor(member,
          [&] { return member.recorder().timeRegulationEnabled().size() == 1U; },
          options, "pending-FQR regulation enablement");

  member.recorder().clearCallbackOrder();
  member.recorder().setFederateSaveInitiateHook([&] {
    member.rtiAmbassador().federateSaveBegun();
    member.rtiAmbassador().federateSaveComplete();
  });
  member.rtiAmbassador().requestFederationSave(saveLabel, *saveTime);
  member.rtiAmbassador().flushQueueRequest(*flushTime);
  waitFor(member,
          [&] {
            return member.federateSaveInitiations().size() == 1U &&
                member.recorder().flushQueueGrants().size() == 1U;
          }, options, "pre-save pending FQR initiation and grant");
  require(member.federateSaveInitiations() ==
                  std::vector<std::wstring>{saveLabel} &&
              member.federateSaveTimes().size() == 1U &&
              member.federateSaveTimes().front().encoded == encodeTime(*saveTime) &&
              member.recorder().flushQueueGrants().front().time == flushEncoded &&
              member.recorder().callbackOrder() ==
                  std::vector<std::string>{"federate-save-initiate", "flush-grant"},
          "Pending FQR did not initiate save before its first grant");
  waitFor(member,
          [&] { return member.federationSavedCount() == 1U; },
          options, "pending-FQR federation save completion");
  require(member.federationNotSavedReasons().empty(),
          "Pending-FQR federation save failed");
  while (member.rtiAmbassador().evokeCallback(0.0)) {
  }

  member.recorder().setFederateSaveInitiateHook({});
  member.rtiAmbassador().timeAdvanceRequest(*laterTime);
  waitFor(member,
          [&] { return member.recorder().timeAdvanceGrants().size() == 1U; },
          options, "post-save TAR grant");
  require(member.recorder().timeAdvanceGrants().front().encoded == laterEncoded &&
              *queryTime() == *laterTime,
          "Post-save TAR did not reach time 9");

  member.recorder().clearCallbackOrder();
  member.rtiAmbassador().requestFederationRestore(saveLabel);
  waitFor(member,
          [&] { return member.federateRestoreInitiations().size() == 1U; },
          options, "pending-FQR restore initiation");
  require(member.federationRestoreRequestsSucceeded() ==
                  std::vector<std::wstring>{saveLabel} &&
              member.federationRestoreBegunCount() == 1U &&
              member.federateRestoreInitiations().front().label == saveLabel,
          "Pending-FQR restore did not begin from the saved label");
  member.rtiAmbassador().federateRestoreComplete();
  require(*queryTime() == *time.initial,
          "Pending-FQR restore did not return to initial logical time");
  requireException(
      [&] { member.rtiAmbassador().timeAdvanceRequest(*invalidTime); },
      L"InTimeAdvancingState",
      "ordinary TAR while restored FQR is pending");

  static_cast<void>(member.rtiAmbassador().evokeCallback(0.0));
  require(member.federationRestoredCount() == 1U &&
              member.recorder().flushQueueGrants().size() == 1U,
          "Restored FQR grant arrived before Federation Restored");
  static_cast<void>(member.rtiAmbassador().evokeCallback(0.0));
  require(member.recorder().flushQueueGrants().size() == 2U &&
              member.recorder().flushQueueGrants().back().time == flushEncoded &&
              member.recorder().callbackOrder() ==
                  std::vector<std::string>{
                      "federation-restore-request-succeeded",
                      "federation-restore-begun",
                      "federate-restore-initiate",
                      "federation-restored",
                      "flush-grant"} &&
              *queryTime() == *flushTime,
          "Restored pending FQR did not grant time 6 after Federation Restored");
  require(member.federationNotRestoredReasons().empty(),
          "Pending-FQR federation restore failed");

  member.resign(rti::NO_ACTION);
  member.rtiAmbassador().destroyFederationExecution(federation);
  member.disconnect();
}

}  // namespace

void scenarioRestorePendingFlushQueueRequest(
    Options const& options, rti::CallbackModel model) {
  runRestorePendingFlushQueueRequest(options, model);
}

void scenarioRestorePendingFlushQueueRequestContract(
    Options const& options, rti::CallbackModel model) {
  runRestorePendingFlushQueueRequest(options, model);
}

}  // namespace hla_rti_cpp_tck::detail
