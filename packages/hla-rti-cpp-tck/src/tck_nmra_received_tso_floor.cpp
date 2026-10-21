#include "tck_main_scenarios.hpp"

namespace hla_rti_cpp_tck::detail {

void scenarioNmraReceivedTsoActualLookaheadFloor(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.logicalTimeImplementationName.empty(),
      "NMRA received-TSO floor requires an adapter-supplied logical-time implementation");

  Session requester(options, model, "owner");
  Session peer(options, model, "member");
  auto const federation = federationName(
      options, "nmra-received-tso-actual-lookahead-floor");
  connectAndJoin(requester, peer, options, federation, options.fom);

  auto const requesterInteraction =
      requester.rtiAmbassador().getInteractionClassHandle(
          options.interactionClassName);
  auto const requesterParameter = requester.rtiAmbassador().getParameterHandle(
      requesterInteraction, options.parameterName);
  auto const peerInteraction = peer.rtiAmbassador().getInteractionClassHandle(
      options.interactionClassName);
  auto const peerParameter = peer.rtiAmbassador().getParameterHandle(
      peerInteraction, options.parameterName);
  require(
      requesterInteraction.isValid() && requesterParameter.isValid() &&
          peerInteraction.isValid() && peerParameter.isValid(),
      "NMRA received-TSO floor could not resolve the adapter interaction and parameter");
  requester.rtiAmbassador().publishInteractionClass(requesterInteraction);
  requester.rtiAmbassador().subscribeInteractionClass(
      requesterInteraction, true);
  requester.rtiAmbassador().changeInteractionOrderType(
      requesterInteraction, rti::TIMESTAMP);
  peer.rtiAmbassador().publishInteractionClass(peerInteraction);
  peer.rtiAmbassador().changeInteractionOrderType(
      peerInteraction, rti::TIMESTAMP);

  auto requesterTime = makeTimeContext(requester);
  auto peerTime = makeTimeContext(peer);
  require(
      requesterTime.factory->getName() == peerTime.factory->getName(),
      "NMRA received-TSO members selected different logical-time factories");
  requireTimeEquals(
      *requesterTime.initial,
      *peerTime.initial,
      "NMRA received-TSO initial logical time");

  requester.rtiAmbassador().enableTimeConstrained();
  waitFor(
      requester,
      [&] { return requester.recorder().timeConstrainedEnabled().size() == 1U; },
      options,
      "NMRA received-TSO requester Time Constrained callback");
  requester.rtiAmbassador().enableTimeRegulation(*requesterTime.epsilon);
  waitFor(
      requester,
      [&] { return requester.recorder().timeRegulationEnabled().size() == 1U; },
      options,
      "NMRA received-TSO requester Time Regulation callback");
  peer.rtiAmbassador().enableTimeRegulation(*peerTime.epsilon);
  waitFor(
      peer,
      [&] { return peer.recorder().timeRegulationEnabled().size() == 1U; },
      options,
      "NMRA received-TSO peer Time Regulation callback");

  auto actualLookahead = requesterTime.factory->makeZero();
  require(
      actualLookahead != nullptr,
      "NMRA received-TSO floor could not allocate a lookahead query value");
  requester.rtiAmbassador().queryLookahead(*actualLookahead);
  require(
      *actualLookahead == *requesterTime.epsilon,
      "NMRA received-TSO requester did not retain its actual epsilon lookahead");

  auto const messageTime = timeAfter(
      *peerTime.factory, *peerTime.initial, *peerTime.epsilon, 5U);
  auto const peerAdvanceTime = timeAfter(
      *peerTime.factory, *peerTime.initial, *peerTime.epsilon, 4U);
  auto const requestTime = timeAfter(
      *requesterTime.factory, *requesterTime.initial, *requesterTime.epsilon, 10U);
  auto const actualGrantTime = timeAfter(
      *requesterTime.factory, *requesterTime.initial, *requesterTime.epsilon, 5U);

  std::vector<std::uint8_t> const payload{0x4EU, 0x4DU, 0x52U, 0x41U};
  std::vector<std::uint8_t> const tagBytes{0x52U, 0x58U, 0x54U, 0x53U, 0x4FU};
  rti::ParameterHandleValueMap incomingValues;
  incomingValues.emplace(
      peerParameter,
      rti::VariableLengthData(payload.data(), payload.size()));
  rti::VariableLengthData const tag(tagBytes.data(), tagBytes.size());
  auto const retraction = peer.rtiAmbassador().sendInteraction(
      peerInteraction, incomingValues, tag, *messageTime);
  require(
      retraction.isValid(),
      "NMRA received-TSO peer Send Interaction returned an invalid retraction handle");

  requester.rtiAmbassador().nextMessageRequestAvailable(*requestTime);
  require(
      requester.recorder().timeAdvanceGrants().empty(),
      "NMRA received-TSO requester granted before the peer advanced its frontier");
  peer.rtiAmbassador().timeAdvanceRequest(*peerAdvanceTime);
  waitFor(
      peer,
      [&] { return peer.recorder().timeAdvanceGrants().size() == 1U; },
      options,
      "NMRA received-TSO peer Time Advance Grant");
  require(
      peer.recorder().timeAdvanceGrants().front().encoded ==
          encodeTime(*peerAdvanceTime),
      "NMRA received-TSO peer did not reach its requested frontier");
  waitFor(
      requester,
      [&] {
        return requester.recorder().timedInteractions().size() == 1U &&
            requester.recorder().timeAdvanceGrants().size() == 1U;
      },
      options,
      "NMRA received-TSO interaction and grant callbacks");

  auto const interactions = requester.recorder().timedInteractions();
  auto const grants = requester.recorder().timeAdvanceGrants();
  require(
      grants.front().encoded == encodeTime(*actualGrantTime),
      "NMRA with timestamp-5 input did not grant at actual time 5");
  require(
      interactions.front().interaction == requesterInteraction &&
          interactions.front().time == encodeTime(*actualGrantTime) &&
          interactions.front().tag == tagBytes &&
          interactions.front().receivedOrder == rti::TIMESTAMP,
      "NMRA received-TSO callback did not precede its grant at timestamp 5");
  require(
      requester.recorder().callbackOrder() ==
          std::vector<std::string>{"interaction", "grant"},
      "NMRA received-TSO interaction was not acknowledged before its grant");

  auto queriedTime = requesterTime.factory->makeInitial();
  require(
      queriedTime != nullptr,
      "NMRA received-TSO floor could not allocate a logical-time query value");
  requester.rtiAmbassador().queryLogicalTime(*queriedTime);
  requireTimeEquals(
      *queriedTime, *actualGrantTime, "NMRA received-TSO actual grant time");

  auto illegalTimestamp = requesterTime.factory->makeInitial();
  auto const legalTimestamp = timeAfter(
      *requesterTime.factory, *actualGrantTime, *actualLookahead, 1U);
  require(
      illegalTimestamp != nullptr,
      "NMRA received-TSO floor could not clone the actual grant time");
  *illegalTimestamp = *actualGrantTime;
  rti::ParameterHandleValueMap outgoingValues;
  outgoingValues.emplace(
      requesterParameter,
      rti::VariableLengthData(payload.data(), payload.size()));
  requireException(
      [&] {
        requester.rtiAmbassador().sendInteraction(
            requesterInteraction, outgoingValues, tag, *illegalTimestamp);
      },
      L"InvalidLogicalTime",
      "NMRA send below the actual grant time plus actual lookahead");
  auto const outgoingRetraction = requester.rtiAmbassador().sendInteraction(
      requesterInteraction, outgoingValues, tag, *legalTimestamp);
  require(
      outgoingRetraction.isValid(),
      "NMRA send at actual grant time plus actual lookahead was rejected");

  requester.resign(rti::NO_ACTION);
  peer.resign(rti::NO_ACTION);
  requester.rtiAmbassador().destroyFederationExecution(federation);
  requester.disconnect();
  peer.disconnect();
}

void scenarioNmraReceivedTsoActualLookaheadFloorContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioNmraReceivedTsoActualLookaheadFloor(options, model);
}

}  // namespace hla_rti_cpp_tck::detail
