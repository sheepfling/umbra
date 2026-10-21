#include "tck_fqr_deferred_lookahead.hpp"

namespace hla_rti_cpp_tck::detail {
namespace {

void runFqrDeferredLookahead(Options const& options, rti::CallbackModel model) {
  require(model == rti::HLA_EVOKED,
          "FQR deferred-lookahead has native evidence only for evoked callbacks");
  require(!options.fom.empty() && !options.logicalTimeImplementationName.empty(),
          "FQR deferred-lookahead requires an adapter FOM and time implementation");

  Session requester(options, model, "owner");
  Session peer(options, model, "member");
  auto const federation = federationName(options, "fqr-deferred-lookahead");
  requester.connect();
  peer.connect();
  requester.rtiAmbassador().createFederationExecution(
      federation, options.fom.wstring(), options.logicalTimeImplementationName);
  requester.join(options.ownerFederateName, options.federateType, federation);
  peer.join(options.memberFederateName, options.federateType, federation);

  auto requesterTime = makeTimeContext(requester);
  auto peerTime = makeTimeContext(peer);
  require(requesterTime.factory->getName() == peerTime.factory->getName(),
          "FQR deferred-lookahead members selected different time factories");
  auto interval = [&](TimeContext const& context, std::size_t steps) {
    auto target = timeAfter(*context.factory, *context.initial, *context.epsilon, steps);
    auto result = context.factory->makeZero();
    require(result != nullptr, "FQR deferred-lookahead could not allocate an interval");
    result->setToDifference(*target, *context.initial);
    return result;
  };
  auto const lookahead10 = interval(requesterTime, 10U);
  auto const lookahead2 = interval(requesterTime, 2U);
  auto const lookahead5 = interval(peerTime, 5U);
  auto const time5 = timeAfter(
      *requesterTime.factory, *requesterTime.initial, *requesterTime.epsilon, 5U);
  auto const time10 = timeAfter(
      *requesterTime.factory, *requesterTime.initial, *requesterTime.epsilon, 10U);
  auto queriedLookahead = requesterTime.factory->makeZero();
  auto queriedTime = requesterTime.factory->makeInitial();
  require(queriedLookahead != nullptr && queriedTime != nullptr,
          "FQR deferred-lookahead could not allocate query outputs");

  requester.rtiAmbassador().enableTimeRegulation(*lookahead10);
  waitFor(requester, [&] {
    return requester.recorder().timeRegulationEnabled().size() == 1U;
  }, options, "requester time-regulation enablement");
  peer.rtiAmbassador().enableTimeRegulation(*lookahead5);
  waitFor(peer, [&] {
    return peer.recorder().timeRegulationEnabled().size() == 1U;
  }, options, "peer time-regulation enablement");

  requester.rtiAmbassador().modifyLookahead(*lookahead2);
  requester.rtiAmbassador().queryLookahead(*queriedLookahead);
  require(*queriedLookahead == *lookahead10,
          "FQR deferred-lookahead decrease became actual before a grant");

  requester.rtiAmbassador().flushQueueRequest(*time10);
  waitFor(requester, [&] {
    return requester.recorder().flushQueueGrants().size() == 1U;
  }, options, "FQR deferred-lookahead grant");
  auto const grants = requester.recorder().flushQueueGrants();
  require(grants.size() == 1U &&
              grants.front().time == encodeTime(*time5) &&
              grants.front().optimisticTime == encodeTime(*time10),
          "FQR deferred-lookahead returned the wrong actual or optimistic grant");
  require(requester.recorder().timeAdvanceGrants().empty(),
          "FQR deferred-lookahead also produced a Time Advance Grant");
  requester.rtiAmbassador().queryLogicalTime(*queriedTime);
  requester.rtiAmbassador().queryLookahead(*queriedLookahead);
  require(*queriedTime == *time5 && *queriedLookahead == *lookahead5,
          "FQR grant did not apply max(requested 2, old 10 - advance 5) = 5");

  requester.resign(rti::NO_ACTION);
  peer.resign(rti::NO_ACTION);
  requester.rtiAmbassador().destroyFederationExecution(federation);
  peer.disconnect();
  requester.disconnect();
}

}  // namespace

void scenarioFqrDeferredLookahead(Options const& options, rti::CallbackModel model) {
  runFqrDeferredLookahead(options, model);
}

void scenarioFqrDeferredLookaheadContract(Options const& options, rti::CallbackModel model) {
  runFqrDeferredLookahead(options, model);
}

}  // namespace hla_rti_cpp_tck::detail
