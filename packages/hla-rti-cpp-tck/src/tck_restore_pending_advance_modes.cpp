#include "tck_restore_pending_advance_modes.hpp"

#include <array>

namespace hla_rti_cpp_tck::detail {
namespace {

enum class AdvanceMode { tara, nmr, nmra };

char const* modeName(AdvanceMode mode) {
  switch (mode) {
    case AdvanceMode::tara: return "tara";
    case AdvanceMode::nmr: return "nmr";
    case AdvanceMode::nmra: return "nmra";
  }
  throw std::runtime_error("Unknown advance mode");
}

void submitAdvance(
    rti::RTIambassador& ambassador,
    AdvanceMode mode,
    rti::LogicalTime const& target) {
  switch (mode) {
    case AdvanceMode::tara:
      ambassador.timeAdvanceRequestAvailable(target);
      return;
    case AdvanceMode::nmr:
      ambassador.nextMessageRequest(target);
      return;
    case AdvanceMode::nmra:
      ambassador.nextMessageRequestAvailable(target);
      return;
  }
  throw std::runtime_error("Unknown advance mode");
}

void runMode(
    Options const& options,
    rti::CallbackModel model,
    AdvanceMode mode) {
  require(!options.fom.empty(),
          "Pending-mode restore requires an adapter-supplied standard NRG-enabled FOM");
  require(!options.logicalTimeImplementationName.empty(),
          "Pending-mode restore requires an adapter-supplied logical-time implementation");
  require(model == rti::HLA_EVOKED || model == rti::HLA_IMMEDIATE,
          "Pending-mode restore requires a standard callback model");
  bool const immediate = model == rti::HLA_IMMEDIATE;
  std::string const modeId = modeName(mode);
  std::string const callbackId = immediate ? "immediate" : "evoked";
  Session member(options, model, "member");
  auto const federation = federationName(
      options, "restore-pending-" + modeId + "-" + callbackId);
  auto const saveLabel = toWide("tck-pending-" + modeId + "-" + callbackId);

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
  if (immediate) {
    require(member.recorder().timeConstrainedEnabled().size() == 1U,
            "Immediate constrained enablement was not delivered directly");
  } else {
    waitFor(member,
            [&] { return member.recorder().timeConstrainedEnabled().size() == 1U; },
            options, "pending-mode constrained enablement");
  }

  member.recorder().clearCallbackOrder();
  member.recorder().setFederateSaveInitiateHook([&] {
    member.rtiAmbassador().federateSaveBegun();
    member.rtiAmbassador().federateSaveComplete();
  });
  member.rtiAmbassador().requestFederationSave(saveLabel);
  require(member.federateSaveInitiations().empty(),
          "Untimed save initiated before the pending-mode advance request");
  submitAdvance(member.rtiAmbassador(), mode, *savedTarget);
  if (!immediate) {
    waitFor(member,
            [&] {
              return member.federateSaveInitiations().size() == 1U &&
                  member.recorder().timeAdvanceGrants().size() == 1U;
            }, options, "pending-mode save initiation and first grant");
    waitFor(member, [&] { return member.federationSavedCount() == 1U; },
            options, "pending-mode federation save completion");
  }
  require(member.federateSaveInitiations() ==
                  std::vector<std::wstring>{saveLabel} &&
              member.federationSavedCount() == 1U &&
              member.federationNotSavedReasons().empty() &&
              member.recorder().timeAdvanceGrants().size() == 1U &&
              member.recorder().timeAdvanceGrants().back().encoded == savedEncoded,
          "Pending-mode save did not capture the request before its first grant");
  if (!immediate) {
    while (member.rtiAmbassador().evokeCallback(0.0)) {
    }
  }

  member.recorder().setFederateSaveInitiateHook({});
  submitAdvance(member.rtiAmbassador(), mode, *laterTarget);
  if (!immediate) {
    waitFor(member,
            [&] { return member.recorder().timeAdvanceGrants().size() == 2U; },
            options, "pending-mode post-save grant");
  }
  require(member.recorder().timeAdvanceGrants().size() == 2U &&
              member.recorder().timeAdvanceGrants().back().encoded == laterEncoded &&
              *queryTime() == *laterTarget,
          "Pending-mode post-save advance did not reach time 9");

  member.recorder().clearCallbackOrder();
  member.rtiAmbassador().requestFederationRestore(saveLabel);
  if (!immediate) {
    waitFor(member,
            [&] { return member.federateRestoreInitiations().size() == 1U; },
            options, "pending-mode restore initiation");
  }
  require(member.federationRestoreRequestsSucceeded() ==
                  std::vector<std::wstring>{saveLabel} &&
              member.federationRestoreBegunCount() == 1U &&
              member.federateRestoreInitiations().size() == 1U &&
              member.federateRestoreInitiations().front().label == saveLabel,
          "Pending-mode restore control callbacks did not arrive");
  member.rtiAmbassador().federateRestoreComplete();
  if (!immediate) {
    require(*queryTime() == *time.initial,
            "Evoked restore did not return to the saved initial logical time");
    requireException(
        [&] { member.rtiAmbassador().timeAdvanceRequest(*nextTarget); },
        L"InTimeAdvancingState",
        "ordinary TAR while a restored pending-mode request is active");
    static_cast<void>(member.rtiAmbassador().evokeCallback(0.0));
    require(member.federationRestoredCount() == 1U &&
                member.recorder().timeAdvanceGrants().size() == 2U,
            "Restored pending-mode grant arrived before Federation Restored");
    static_cast<void>(member.rtiAmbassador().evokeCallback(0.0));
  }
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
          "Restored pending-mode grant did not follow Federation Restored");

  submitAdvance(member.rtiAmbassador(), mode, *nextTarget);
  if (!immediate) {
    waitFor(member,
            [&] { return member.recorder().timeAdvanceGrants().size() == 4U; },
            options, "pending-mode post-restore grant");
  }
  require(member.recorder().timeAdvanceGrants().size() == 4U &&
              member.recorder().timeAdvanceGrants().back().encoded == nextEncoded,
          "Advance mode was not usable after restored grant");

  member.resign(rti::NO_ACTION);
  member.rtiAmbassador().destroyFederationExecution(federation);
  member.disconnect();
}

void runMatrix(Options const& options, rti::CallbackModel model) {
  for (auto const mode :
       std::array{AdvanceMode::tara, AdvanceMode::nmr, AdvanceMode::nmra}) {
    runMode(options, model, mode);
  }
}

}  // namespace

void scenarioRestorePendingAdvanceModes(
    Options const& options, rti::CallbackModel model) {
  runMatrix(options, model);
}

void scenarioRestorePendingAdvanceModesContract(
    Options const& options, rti::CallbackModel model) {
  runMatrix(options, model);
}

}  // namespace hla_rti_cpp_tck::detail
