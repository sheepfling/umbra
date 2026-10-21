#include "tck_restore_pending_flush_queue_immediate.hpp"

namespace hla_rti_cpp_tck::detail {
namespace {

void runRestorePendingFlushQueueImmediate(
    Options const& options, rti::CallbackModel model) {
  require(model == rti::HLA_IMMEDIATE,
          "Pending-FQR immediate restore has native evidence only for immediate callbacks");
  require(!options.fom.empty() && !options.logicalTimeImplementationName.empty(),
          "Pending-FQR immediate restore requires adapter NRG-enabled FOM and time implementation");

  Session member(options, model, "member");
  auto const federation = federationName(options, "restore-pending-flush-queue-immediate");
  std::wstring const saveLabel = L"tck-pending-flush-queue-immediate";
  member.connect();
  member.rtiAmbassador().createFederationExecution(
      federation, options.fom.wstring(), options.logicalTimeImplementationName);
  member.join(options.ownerFederateName, options.federateType, federation);

  auto const time = makeTimeContext(member);
  auto const saveTime = timeAfter(*time.factory, *time.initial, *time.epsilon, 5U);
  auto const flushTime = timeAfter(*time.factory, *time.initial, *time.epsilon, 6U);
  auto const nextTime = timeAfter(*time.factory, *time.initial, *time.epsilon, 7U);
  auto const laterTime = timeAfter(*time.factory, *time.initial, *time.epsilon, 9U);
  auto const flushEncoded = encodeTime(*flushTime);
  auto const nextEncoded = encodeTime(*nextTime);
  auto const laterEncoded = encodeTime(*laterTime);
  auto queryTime = [&] {
    auto result = time.factory->makeInitial();
    require(result != nullptr, "Could not allocate Query Logical Time result");
    member.rtiAmbassador().queryLogicalTime(*result);
    return result;
  };

  member.rtiAmbassador().enableTimeConstrained();
  require(member.recorder().timeConstrainedEnabled().size() == 1U,
          "Immediate constrained enablement was not delivered directly");
  member.rtiAmbassador().enableTimeRegulation(*time.epsilon);
  require(member.recorder().timeRegulationEnabled().size() == 1U,
          "Immediate regulation enablement was not delivered directly");

  member.recorder().clearCallbackOrder();
  member.recorder().setFederateSaveInitiateHook([&] {
    member.rtiAmbassador().federateSaveBegun();
    member.rtiAmbassador().federateSaveComplete();
  });
  member.rtiAmbassador().requestFederationSave(saveLabel, *saveTime);
  member.rtiAmbassador().flushQueueRequest(*flushTime);
  require(member.federateSaveInitiations() == std::vector<std::wstring>{saveLabel} &&
              member.federationSavedCount() == 1U &&
              member.federationNotSavedReasons().empty() &&
              member.recorder().flushQueueGrants().size() == 1U &&
              member.recorder().flushQueueGrants().back().time == flushEncoded &&
              member.recorder().callbackOrder() ==
                  std::vector<std::string>{
                      "federate-save-initiate", "federation-saved", "flush-grant"},
          "Immediate save did not complete re-entrantly before the first FQG");
  require(member.federateSaveTimes().size() == 1U &&
              member.federateSaveTimes().front().encoded == encodeTime(*saveTime),
          "Immediate saved FQR used an unexpected save timestamp");

  member.recorder().setFederateSaveInitiateHook({});
  member.rtiAmbassador().timeAdvanceRequest(*laterTime);
  require(member.recorder().timeAdvanceGrants().size() == 1U &&
              member.recorder().timeAdvanceGrants().back().encoded == laterEncoded &&
              *queryTime() == *laterTime,
          "Post-save ordinary TAR did not reach the later time");

  member.recorder().clearCallbackOrder();
  member.rtiAmbassador().requestFederationRestore(saveLabel);
  require(member.federationRestoreRequestsSucceeded() ==
                  std::vector<std::wstring>{saveLabel} &&
              member.federationRestoreBegunCount() == 1U &&
              member.federateRestoreInitiations().size() == 1U &&
              member.federateRestoreInitiations().front().label == saveLabel,
          "Immediate FQR restore control callbacks were not delivered directly");
  member.rtiAmbassador().federateRestoreComplete();
  require(member.federationRestoredCount() == 1U &&
              member.federationNotRestoredReasons().empty() &&
              member.recorder().flushQueueGrants().size() == 2U &&
              member.recorder().flushQueueGrants().back().time == flushEncoded &&
              member.recorder().callbackOrder() ==
                  std::vector<std::string>{
                      "federation-restore-request-succeeded",
                      "federation-restore-begun",
                      "federate-restore-initiate",
                      "federation-restored",
                      "flush-grant"} &&
              *queryTime() == *flushTime,
          "Immediate Federation Restored did not precede the reconstructed FQG");

  member.rtiAmbassador().flushQueueRequest(*nextTime);
  require(member.recorder().flushQueueGrants().size() == 3U &&
              member.recorder().flushQueueGrants().back().time == nextEncoded,
          "Restored FQG did not release Time Advancing for the next FQR");

  member.resign(rti::NO_ACTION);
  member.rtiAmbassador().destroyFederationExecution(federation);
  member.disconnect();
}

}  // namespace

void scenarioRestorePendingFlushQueueImmediate(
    Options const& options, rti::CallbackModel model) {
  runRestorePendingFlushQueueImmediate(options, model);
}

void scenarioRestorePendingFlushQueueImmediateContract(
    Options const& options, rti::CallbackModel model) {
  runRestorePendingFlushQueueImmediate(options, model);
}

}  // namespace hla_rti_cpp_tck::detail
