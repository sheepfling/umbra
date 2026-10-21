#include "tck_restore_time_window_state.hpp"

namespace hla_rti_cpp_tck::detail {
namespace {

void runRestoreTimeWindowState(Options const& options, rti::CallbackModel model) {
  require(model == rti::HLA_EVOKED,
          "Saved time-window restore has native evidence only for evoked callbacks");
  require(!options.fom.empty() && !options.logicalTimeImplementationName.empty(),
          "Saved time-window restore requires adapter-supplied FOM and time implementation");

  Session member(options, model, "member");
  auto const federation = federationName(options, "restore-time-window-state");
  std::wstring const saveLabel = L"tck-time-window-baseline";
  member.connect();
  member.rtiAmbassador().createFederationExecution(
      federation, options.fom.wstring(), options.logicalTimeImplementationName);
  member.join(options.ownerFederateName, options.federateType, federation);

  auto const time = makeTimeContext(member);
  auto const time3 = timeAfter(*time.factory, *time.initial, *time.epsilon, 3U);
  auto const time5 = timeAfter(*time.factory, *time.initial, *time.epsilon, 5U);
  auto interval = [&](unsigned steps) {
    auto value = time.factory->makeZero();
    require(value != nullptr, "Could not allocate time-window interval");
    for (unsigned index = 0; index != steps; ++index) *value += *time.epsilon;
    return value;
  };
  auto const lookahead2 = interval(2U);
  auto const lookahead5 = interval(5U);
  auto queryTime = [&] {
    auto value = time.factory->makeInitial();
    require(value != nullptr, "Could not allocate queried logical time");
    member.rtiAmbassador().queryLogicalTime(*value);
    return value;
  };
  auto queryLookahead = [&] {
    auto value = time.factory->makeZero();
    require(value != nullptr, "Could not allocate queried lookahead");
    member.rtiAmbassador().queryLookahead(*value);
    return value;
  };

  member.rtiAmbassador().enableTimeRegulation(*lookahead2);
  waitFor(member, [&] {
    return member.recorder().timeRegulationEnabled().size() == 1U;
  }, options, "time-window regulation enablement");
  require(*queryTime() == *time.initial && *queryLookahead() == *lookahead2,
          "Initial regulated time window did not match zero and lookahead two");

  member.rtiAmbassador().timeAdvanceRequest(*time3);
  waitFor(member, [&] {
    return member.recorder().timeAdvanceGrants().size() == 1U;
  }, options, "time-window grant at time three");
  require(*queryTime() == *time3,
          "Logical time did not reach the pre-save time three");

  member.rtiAmbassador().requestFederationSave(saveLabel);
  waitFor(member, [&] {
    return member.federateSaveInitiations() == std::vector<std::wstring>{saveLabel};
  }, options, "time-window save initiation");
  member.rtiAmbassador().federateSaveBegun();
  member.rtiAmbassador().federateSaveComplete();
  waitFor(member, [&] { return member.federationSavedCount() == 1U; },
          options, "time-window federation saved");
  require(member.federationNotSavedReasons().empty(),
          "Time-window federation save failed");

  member.rtiAmbassador().modifyLookahead(*lookahead5);
  require(*queryLookahead() == *lookahead5,
          "Post-save lookahead did not change to five");
  member.rtiAmbassador().timeAdvanceRequest(*time5);
  waitFor(member, [&] {
    return member.recorder().timeAdvanceGrants().size() == 2U;
  }, options, "post-save time-window grant at time five");
  require(*queryTime() == *time5,
          "Post-save logical time did not advance to five");

  member.rtiAmbassador().requestFederationRestore(saveLabel);
  waitFor(member, [&] {
    return member.federationRestoreRequestsSucceeded() ==
               std::vector<std::wstring>{saveLabel} &&
           member.federateRestoreInitiations().size() == 1U;
  }, options, "time-window restore initiation");
  require(member.federateRestoreInitiations().front().label == saveLabel,
          "Time-window restore initiated from an unexpected label");
  member.rtiAmbassador().federateRestoreComplete();
  waitFor(member, [&] { return member.federationRestoredCount() == 1U; },
          options, "time-window federation restored");
  require(member.federationNotRestoredReasons().empty(),
          "Time-window federation restore failed");
  require(*queryTime() == *time3 && *queryLookahead() == *lookahead2,
          "Restore did not reinstate saved logical time and actual lookahead");

  member.resign(rti::NO_ACTION);
  member.rtiAmbassador().destroyFederationExecution(federation);
  member.disconnect();
}

}  // namespace

void scenarioRestoreTimeWindowState(Options const& options, rti::CallbackModel model) {
  runRestoreTimeWindowState(options, model);
}

void scenarioRestoreTimeWindowStateContract(Options const& options, rti::CallbackModel model) {
  runRestoreTimeWindowState(options, model);
}

}  // namespace hla_rti_cpp_tck::detail
