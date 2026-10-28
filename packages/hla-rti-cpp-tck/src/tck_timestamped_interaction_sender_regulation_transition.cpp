#include "tck_main_scenarios.hpp"

namespace hla_rti_cpp_tck::detail {

void scenarioTimestampedInteractionSenderRegulationTransition(
    Options const& options, rti::CallbackModel model) {
  Session sender(options, model, "timestamped-interaction-sender-role-sender");
  Session receiver(options, model, "timestamped-interaction-sender-role-receiver");
  Session regulator(options, model, "timestamped-interaction-sender-role-regulator");
  auto const federation = federationName(
      options, "timestamped-interaction-sender-regulation-transition");
  connectAndJoin(sender, receiver, options, federation, options.fom);
  regulator.connect();
  regulator.join(
      L"timestamped-interaction-sender-role-regulator",
      options.federateType,
      federation);

  auto const senderInteraction = sender.rtiAmbassador().getInteractionClassHandle(
      options.interactionClassName);
  auto const receiverInteraction = receiver.rtiAmbassador().getInteractionClassHandle(
      options.interactionClassName);
  auto const senderParameter = sender.rtiAmbassador().getParameterHandle(
      senderInteraction, options.parameterName);
  auto const receiverParameter = receiver.rtiAmbassador().getParameterHandle(
      receiverInteraction, options.parameterName);
  require(
      senderInteraction.isValid() && receiverInteraction.isValid() &&
          senderParameter.isValid() && receiverParameter.isValid(),
      "sender-regulation interaction transition did not resolve the adapter FOM handles");

  sender.rtiAmbassador().publishInteractionClass(senderInteraction);
  sender.rtiAmbassador().changeInteractionOrderType(
      senderInteraction, rti::TIMESTAMP);
  receiver.rtiAmbassador().subscribeInteractionClass(receiverInteraction);

  auto senderTime = makeTimeContext(sender);
  auto receiverTime = makeTimeContext(receiver);
  auto regulatorTime = makeTimeContext(regulator);
  require(
      senderTime.factory->getName() == receiverTime.factory->getName() &&
          senderTime.factory->getName() == regulatorTime.factory->getName(),
      "sender-regulation interaction members selected different logical-time factories");
  require(
      *senderTime.initial == *receiverTime.initial &&
          *senderTime.initial == *regulatorTime.initial,
      "sender-regulation interaction members began at different logical times");

  receiver.rtiAmbassador().enableTimeConstrained();
  waitFor(
      receiver,
      [&] { return receiver.recorder().timeConstrainedEnabled().size() == 1U; },
      options,
      "sender-regulation interaction receiver time-constrained callback");
  receiver.rtiAmbassador().enableAsynchronousDelivery();

  auto regulatorLookahead = regulatorTime.factory->makeZero();
  auto senderLookahead = senderTime.factory->makeZero();
  require(
      regulatorLookahead != nullptr && senderLookahead != nullptr,
      "sender-regulation interaction could not allocate lookahead intervals");
  for (std::size_t step = 0U; step != 5U; ++step) {
    *regulatorLookahead += *regulatorTime.epsilon;
    *senderLookahead += *senderTime.epsilon;
  }
  regulator.rtiAmbassador().enableTimeRegulation(*regulatorLookahead);
  waitFor(
      regulator,
      [&] { return regulator.recorder().timeRegulationEnabled().size() == 1U; },
      options,
      "sender-regulation interaction separate GALT regulator callback");

  auto const messageTime = timeAfter(
      *senderTime.factory, *senderTime.initial, *senderTime.epsilon, 6U);
  auto const receiverTarget = timeAfter(
      *receiverTime.factory, *receiverTime.initial, *receiverTime.epsilon, 6U);
  auto const senderTarget = timeAfter(
      *senderTime.factory, *senderTime.initial, *senderTime.epsilon, 2U);
  auto const regulatorTarget = timeAfter(
      *regulatorTime.factory, *regulatorTime.initial, *regulatorTime.epsilon, 2U);

  std::vector<std::uint8_t> const firstValue{0x41U, 0x42U};
  std::vector<std::uint8_t> const firstTagBytes{0x31U};
  rti::ParameterHandleValueMap firstParameters;
  firstParameters.emplace(
      senderParameter,
      rti::VariableLengthData(firstValue.data(), firstValue.size()));
  rti::VariableLengthData firstTag(firstTagBytes.data(), firstTagBytes.size());
  auto const firstRetraction = sender.rtiAmbassador().sendInteraction(
      senderInteraction, firstParameters, firstTag, *messageTime);
  require(
      !firstRetraction.isValid(),
      "timestamped send before sender Time Regulation returned a retraction handle");
  waitFor(
      receiver,
      [&] { return receiver.recorder().timedInteractions().size() == 1U; },
      options,
      "pre-regulation receive-order timestamped interaction");
  auto const firstDeliveries = receiver.recorder().timedInteractions();
  auto const& first = firstDeliveries.front();
  require(
      receiver.recorder().interactions().empty() &&
          receiver.recorder().timeAdvanceGrants().empty() &&
          receiver.recorder().callbackOrder() ==
              std::vector<std::string>{"interaction"} &&
          first.interaction == receiverInteraction &&
          first.parameters.size() == 1U &&
          first.parameters.count(receiverParameter) == 1U &&
          copyBytes(first.parameters.at(receiverParameter)) == firstValue &&
          first.tag == firstTagBytes &&
          first.producer == sender.federateHandle() &&
          first.time == encodeTime(*messageTime) &&
          first.sentOrder == rti::RECEIVE &&
          first.receivedOrder == rti::RECEIVE &&
          !first.retractionPresent && first.transportation.isValid(),
      "unregulated sender's timestamped interaction was not delivered as receive order");
  require(
      !receiver.rtiAmbassador()
           .getTransportationTypeName(first.transportation)
           .empty(),
      "pre-regulation callback returned an unresolvable transportation type");

  // Keep the preferred order and timestamp fixed; only sender role changes.
  sender.rtiAmbassador().enableTimeRegulation(*senderLookahead);
  waitFor(
      sender,
      [&] { return sender.recorder().timeRegulationEnabled().size() == 1U; },
      options,
      "timestamped interaction sender time-regulation callback");
  std::vector<std::uint8_t> const secondValue{0x54U, 0x53U, 0x4FU};
  std::vector<std::uint8_t> const secondTagBytes{0x32U};
  rti::ParameterHandleValueMap secondParameters;
  secondParameters.emplace(
      senderParameter,
      rti::VariableLengthData(secondValue.data(), secondValue.size()));
  rti::VariableLengthData secondTag(secondTagBytes.data(), secondTagBytes.size());
  auto const secondRetraction = sender.rtiAmbassador().sendInteraction(
      senderInteraction, secondParameters, secondTag, *messageTime);
  require(
      secondRetraction.isValid(),
      "timestamped send after sender Time Regulation returned no retraction handle");
  require(
      receiver.recorder().timedInteractions().size() == 1U &&
          receiver.recorder().timeAdvanceGrants().empty(),
      "post-regulation TSO interaction was delivered before receiver advancement");

  receiver.rtiAmbassador().timeAdvanceRequest(*receiverTarget);
  sender.rtiAmbassador().timeAdvanceRequest(*senderTarget);
  regulator.rtiAmbassador().timeAdvanceRequest(*regulatorTarget);
  waitForSessions(
      {&sender, &receiver, &regulator},
      [&] {
        return receiver.recorder().timedInteractions().size() == 2U &&
            receiver.recorder().timeAdvanceGrants().size() == 1U;
      },
      options,
      "post-regulation timestamp-order interaction and receiver grant");
  auto const deliveries = receiver.recorder().timedInteractions();
  auto const& second = deliveries.back();
  require(
      receiver.recorder().interactions().empty() && deliveries.size() == 2U &&
          receiver.recorder().timeAdvanceGrants().size() == 1U &&
          receiver.recorder().timeAdvanceGrants().front().encoded ==
              encodeTime(*receiverTarget) &&
          second.interaction == receiverInteraction &&
          second.parameters.size() == 1U &&
          second.parameters.count(receiverParameter) == 1U &&
          copyBytes(second.parameters.at(receiverParameter)) == secondValue &&
          second.tag == secondTagBytes &&
          second.producer == sender.federateHandle() &&
          second.time == encodeTime(*messageTime) &&
          second.sentOrder == rti::TIMESTAMP &&
          second.receivedOrder == rti::TIMESTAMP &&
          second.retractionPresent &&
          second.retraction == copyBytes(secondRetraction.encode()) &&
          second.transportation.isValid() &&
          receiver.recorder().callbackOrder() ==
              std::vector<std::string>{"interaction", "interaction", "grant"},
      "regulated sender's timestamped interaction did not arrive as TSO before its grant");
  require(
      !receiver.rtiAmbassador()
           .getTransportationTypeName(second.transportation)
           .empty(),
      "post-regulation callback returned an unresolvable transportation type");

  receiver.rtiAmbassador().disableAsynchronousDelivery();
  receiver.rtiAmbassador().disableTimeConstrained();
  sender.rtiAmbassador().disableTimeRegulation();
  regulator.rtiAmbassador().disableTimeRegulation();
  receiver.rtiAmbassador().unsubscribeInteractionClass(receiverInteraction);
  sender.rtiAmbassador().unpublishInteractionClass(senderInteraction);
  receiver.resign(rti::NO_ACTION);
  regulator.resign(rti::NO_ACTION);
  sender.resign(rti::NO_ACTION);
  sender.rtiAmbassador().destroyFederationExecution(federation);
  receiver.disconnect();
  regulator.disconnect();
  sender.disconnect();
}

void scenarioTimestampedInteractionSenderRegulationTransitionContract(
    Options const& options, rti::CallbackModel model) {
  scenarioTimestampedInteractionSenderRegulationTransition(options, model);
}

}  // namespace hla_rti_cpp_tck::detail
