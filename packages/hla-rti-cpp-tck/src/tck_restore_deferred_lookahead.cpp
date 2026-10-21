#include "tck_restore_deferred_lookahead.hpp"

namespace hla_rti_cpp_tck::detail {
namespace {

void runRestoreDeferredLookahead(Options const& options, rti::CallbackModel model) {
  require(model == rti::HLA_EVOKED,
          "Deferred-lookahead restore has native evidence only for evoked callbacks");
  require(!options.fom.empty() && !options.logicalTimeImplementationName.empty(),
          "Deferred-lookahead restore requires adapter-supplied FOM and time implementation");

  Session member(options, model, "member");
  auto const federation = federationName(options, "restore-deferred-lookahead");
  std::wstring const saveLabel = L"tck-deferred-lookahead-baseline";
  member.connect();
  member.rtiAmbassador().createFederationExecution(
      federation, options.fom.wstring(), options.logicalTimeImplementationName);
  member.join(options.ownerFederateName, options.federateType, federation);

  auto const time = makeTimeContext(member);
  auto const time3 = timeAfter(*time.factory, *time.initial, *time.epsilon, 3U);
  auto interval = [&](unsigned steps) {
    auto value = time.factory->makeZero();
    require(value != nullptr, "Could not allocate deferred-lookahead interval");
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

  member.rtiAmbassador().enableTimeRegulation(*lookahead5);
  waitFor(member, [&] {
    return member.recorder().timeRegulationEnabled().size() == 1U;
  }, options, "deferred-lookahead regulation enablement");
  require(*queryTime() == *time.initial && *queryLookahead() == *lookahead5,
          "Initial time window was not zero with actual lookahead five");

  member.rtiAmbassador().modifyLookahead(*time.epsilon);
  require(*queryLookahead() == *lookahead5,
          "Lookahead decrease became actual before any time advance");
  member.rtiAmbassador().requestFederationSave(saveLabel);
  waitFor(member, [&] {
    return member.federateSaveInitiations() == std::vector<std::wstring>{saveLabel};
  }, options, "deferred-lookahead save initiation");
  member.rtiAmbassador().federateSaveBegun();
  member.rtiAmbassador().federateSaveComplete();
  waitFor(member, [&] { return member.federationSavedCount() == 1U; },
          options, "deferred-lookahead federation saved");
  require(member.federationNotSavedReasons().empty(),
          "Deferred-lookahead federation save failed");

  member.rtiAmbassador().timeAdvanceRequest(*time3);
  waitFor(member, [&] {
    return member.recorder().timeAdvanceGrants().size() == 1U;
  }, options, "post-save deferred-lookahead grant");
  require(*queryTime() == *time3 && *queryLookahead() == *lookahead2,
          "Post-save advance did not partially consume the lookahead decrease");

  member.rtiAmbassador().requestFederationRestore(saveLabel);
  waitFor(member, [&] {
    return member.federationRestoreRequestsSucceeded() ==
               std::vector<std::wstring>{saveLabel} &&
           member.federateRestoreInitiations().size() == 1U;
  }, options, "deferred-lookahead restore initiation");
  require(member.federateRestoreInitiations().front().label == saveLabel,
          "Deferred-lookahead restore initiated from an unexpected label");
  member.rtiAmbassador().federateRestoreComplete();
  waitFor(member, [&] { return member.federationRestoredCount() == 1U; },
          options, "deferred-lookahead federation restored");
  require(member.federationNotRestoredReasons().empty(),
          "Deferred-lookahead federation restore failed");
  require(*queryTime() == *time.initial && *queryLookahead() == *lookahead5,
          "Restore did not return to initial time and actual lookahead five");

  member.rtiAmbassador().timeAdvanceRequest(*time3);
  waitFor(member, [&] {
    return member.recorder().timeAdvanceGrants().size() == 2U;
  }, options, "restored deferred-lookahead grant");
  require(*queryTime() == *time3 && *queryLookahead() == *lookahead2,
          "Restored deferred lookahead target was not replayed after the grant");

  member.resign(rti::NO_ACTION);
  member.rtiAmbassador().destroyFederationExecution(federation);
  member.disconnect();
}

}  // namespace

void scenarioRestoreDeferredLookahead(Options const& options, rti::CallbackModel model) {
  runRestoreDeferredLookahead(options, model);
}

void scenarioRestoreDeferredLookaheadContract(Options const& options, rti::CallbackModel model) {
  runRestoreDeferredLookahead(options, model);
}

}  // namespace hla_rti_cpp_tck::detail
