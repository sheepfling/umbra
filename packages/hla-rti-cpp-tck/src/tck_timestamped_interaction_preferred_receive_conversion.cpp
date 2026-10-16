#include "tck_main_scenarios.hpp"

namespace hla_rti_cpp_tck::detail {

void scenarioTimestampedInteractionPreferredReceiveConversion(
    Options const& options, rti::CallbackModel model) {
  Session sender(options, model, "owner");
  Session receiver(options, model, "member");
  auto const federation = federationName(options, "preferred-receive-conversion");
  connectAndJoin(sender, receiver, options, federation, options.fom);

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
          senderParameter.isValid() && receiverParameter.isValid() &&
          senderInteraction == receiverInteraction && senderParameter == receiverParameter,
      "preferred-RECEIVE interaction handles did not resolve consistently");

  sender.rtiAmbassador().publishInteractionClass(senderInteraction);
  receiver.rtiAmbassador().subscribeInteractionClass(receiverInteraction, true);
  sender.rtiAmbassador().changeInteractionOrderType(senderInteraction, rti::RECEIVE);

  auto senderTime = makeTimeContext(sender);
  auto receiverTime = makeTimeContext(receiver);
  require(
      senderTime.factory->getName() == receiverTime.factory->getName(),
      "preferred-RECEIVE members selected different logical-time factories");
  receiver.rtiAmbassador().enableTimeConstrained();
  waitFor(receiver,
          [&] { return receiver.recorder().timeConstrainedEnabled().size() == 1U; },
          options,
          "preferred-RECEIVE time-constrained callback");
  receiver.rtiAmbassador().enableAsynchronousDelivery();
  auto lookahead = senderTime.factory->makeZero();
  require(lookahead != nullptr,
          "preferred-RECEIVE time factory returned no lookahead interval");
  for (std::size_t step = 0U; step != 5U; ++step) {
    *lookahead += *senderTime.epsilon;
  }
  sender.rtiAmbassador().enableTimeRegulation(*lookahead);
  waitFor(sender,
          [&] { return sender.recorder().timeRegulationEnabled().size() == 1U; },
          options,
          "preferred-RECEIVE time-regulation callback");

  auto const messageTime = timeAfter(
      *senderTime.factory, *senderTime.initial, *senderTime.epsilon, 6U);
  auto const senderTarget = timeAfter(
      *senderTime.factory, *senderTime.initial, *senderTime.epsilon, 2U);
  auto const valueBytes = std::vector<std::uint8_t>{0x52U, 0x4FU};
  auto const tagBytes = std::vector<std::uint8_t>{0x50U, 0x52U};
  rti::ParameterHandleValueMap values;
  values.emplace(senderParameter,
                 rti::VariableLengthData(valueBytes.data(), valueBytes.size()));
  rti::VariableLengthData tag(tagBytes.data(), tagBytes.size());
  auto const retraction = sender.rtiAmbassador().sendInteraction(
      senderInteraction, values, tag, *messageTime);
  require(!retraction.isValid(),
          "timestamped Send Interaction with preferred RECEIVE returned a retraction handle");
  waitFor(receiver,
          [&] { return receiver.recorder().timedInteractions().size() == 1U; },
          options,
          "preferred-RECEIVE timestamped callback");
  require(receiver.recorder().interactions().empty() &&
              receiver.recorder().timeAdvanceGrants().empty(),
          "preferred-RECEIVE conversion used an untimed callback or granted early");

  auto const received = receiver.recorder().timedInteractions().front();
  require(received.interaction == receiverInteraction &&
              received.parameters.size() == 1U &&
              received.parameters.count(receiverParameter) == 1U &&
              copyBytes(received.parameters.at(receiverParameter)) == valueBytes &&
              received.tag == tagBytes &&
              received.producer == sender.federateHandle(),
          "preferred-RECEIVE conversion delivered incorrect interaction data");
  require(received.time == encodeTime(*messageTime) && !received.timeText.empty(),
          "preferred-RECEIVE conversion did not preserve the supplied timestamp");
  require(received.sentOrder == rti::RECEIVE &&
              received.receivedOrder == rti::RECEIVE,
          "preferred-RECEIVE conversion returned incorrect order metadata");
  require(!received.retractionPresent,
          "preferred-RECEIVE conversion callback unexpectedly supplied a retraction handle");
  require(received.transportation.isValid() &&
              !receiver.rtiAmbassador()
                   .getTransportationTypeName(received.transportation).empty(),
          "preferred-RECEIVE conversion returned an unresolvable transportation type");

  receiver.rtiAmbassador().timeAdvanceRequest(*messageTime);
  sender.rtiAmbassador().timeAdvanceRequest(*senderTarget);
  sender.pump();
  waitFor(receiver,
          [&] { return receiver.recorder().timeAdvanceGrants().size() == 1U; },
          options,
          "preferred-RECEIVE matching receiver grant");
  require(receiver.recorder().timedInteractions().size() == 1U &&
              receiver.recorder().callbackOrder() ==
                  std::vector<std::string>{"interaction", "grant"},
          "preferred-RECEIVE conversion callback did not precede its grant");
  auto receiverGrant = decodeTime(
      *receiverTime.factory,
      receiver.recorder().timeAdvanceGrants().front().encoded);
  require(receiverGrant != nullptr,
          "preferred-RECEIVE conversion returned an invalid grant time");
  requireTimeEquals(*receiverGrant, *messageTime,
                    "preferred-RECEIVE conversion receiver grant");

  receiver.rtiAmbassador().disableAsynchronousDelivery();
  receiver.rtiAmbassador().disableTimeConstrained();
  sender.rtiAmbassador().disableTimeRegulation();
  receiver.rtiAmbassador().unsubscribeInteractionClass(receiverInteraction);
  sender.rtiAmbassador().unpublishInteractionClass(senderInteraction);
  receiver.resign(rti::NO_ACTION);
  sender.resign(rti::NO_ACTION);
  sender.rtiAmbassador().destroyFederationExecution(federation);
  receiver.disconnect();
  sender.disconnect();
}

void scenarioTimestampedInteractionPreferredReceiveConversionContract(
    Options const& options, rti::CallbackModel model) {
  scenarioTimestampedInteractionPreferredReceiveConversion(options, model);
}

}  // namespace hla_rti_cpp_tck::detail
