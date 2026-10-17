#include "tck_main_scenarios.hpp"

namespace hla_rti_cpp_tck::detail {

void scenarioSentTsoPerRecipientOrderConversion(
    Options const& options, rti::CallbackModel model) {
  Session sender(options, model, "owner");
  Session constrained(options, model, "member");
  Session unconstrained(options, model, "member");
  auto const federation = federationName(options, "sent-tso-per-recipient-order");
  connectAndJoin(sender, constrained, options, federation, options.fom);
  unconstrained.connect();
  auto const unconstrainedName = options.memberFederateName + L"-unconstrained";
  unconstrained.join(unconstrainedName, options.federateType, federation);

  auto const senderClass = sender.rtiAmbassador().getInteractionClassHandle(
      options.interactionClassName);
  auto const constrainedClass = constrained.rtiAmbassador().getInteractionClassHandle(
      options.interactionClassName);
  auto const unconstrainedClass = unconstrained.rtiAmbassador().getInteractionClassHandle(
      options.interactionClassName);
  auto const senderParameter = sender.rtiAmbassador().getParameterHandle(
      senderClass, options.parameterName);
  auto const constrainedParameter = constrained.rtiAmbassador().getParameterHandle(
      constrainedClass, options.parameterName);
  auto const unconstrainedParameter = unconstrained.rtiAmbassador().getParameterHandle(
      unconstrainedClass, options.parameterName);
  require(
      senderClass.isValid() && constrainedClass.isValid() &&
          unconstrainedClass.isValid() && senderParameter.isValid() &&
          constrainedParameter.isValid() && unconstrainedParameter.isValid() &&
          senderClass == constrainedClass && senderClass == unconstrainedClass &&
          senderParameter == constrainedParameter &&
          senderParameter == unconstrainedParameter,
      "per-recipient order conversion did not resolve common FOM handles");

  sender.rtiAmbassador().publishInteractionClass(senderClass);
  constrained.rtiAmbassador().subscribeInteractionClass(constrainedClass);
  unconstrained.rtiAmbassador().subscribeInteractionClass(unconstrainedClass);
  sender.rtiAmbassador().changeInteractionOrderType(senderClass, rti::TIMESTAMP);

  auto senderTime = makeTimeContext(sender);
  auto constrainedTime = makeTimeContext(constrained);
  auto unconstrainedTime = makeTimeContext(unconstrained);
  require(
      senderTime.factory->getName() == constrainedTime.factory->getName() &&
          senderTime.factory->getName() == unconstrainedTime.factory->getName(),
      "per-recipient members selected different logical-time factories");

  constrained.rtiAmbassador().enableTimeConstrained();
  waitFor(
      constrained,
      [&] { return constrained.recorder().timeConstrainedEnabled().size() == 1U; },
      options,
      "per-recipient constrained-enable callback");

  auto lookahead = senderTime.factory->makeZero();
  require(lookahead != nullptr,
          "per-recipient time factory returned no zero interval");
  for (std::size_t step = 0U; step != 5U; ++step) {
    *lookahead += *senderTime.epsilon;
  }
  sender.rtiAmbassador().enableTimeRegulation(*lookahead);
  waitFor(
      sender,
      [&] { return sender.recorder().timeRegulationEnabled().size() == 1U; },
      options,
      "per-recipient regulation-enable callback");

  auto const messageTime = timeAfter(
      *senderTime.factory, *senderTime.initial, *senderTime.epsilon, 6U);
  auto const senderTarget = timeAfter(
      *senderTime.factory, *senderTime.initial, *senderTime.epsilon, 2U);
  std::vector<std::uint8_t> const payload{0x54U, 0x53U, 0x4FU};
  std::vector<std::uint8_t> const tagBytes{0x50U, 0x52U};
  rti::ParameterHandleValueMap parameters;
  parameters.emplace(
      senderParameter,
      rti::VariableLengthData(payload.data(), payload.size()));
  rti::VariableLengthData tag(tagBytes.data(), tagBytes.size());

  auto const retraction = sender.rtiAmbassador().sendInteraction(
      senderClass, parameters, tag, *messageTime);
  require(retraction.isValid(),
          "timestamped TSO-preferred send returned no retraction handle");

  waitFor(
      unconstrained,
      [&] { return unconstrained.recorder().timedInteractions().size() == 1U; },
      options,
      "unconstrained recipient's time-bearing RO callback");
  require(
      constrained.recorder().timedInteractions().empty() &&
          constrained.recorder().timeAdvanceGrants().empty(),
      "constrained recipient received TSO data before advancing to its time");
  require(
      unconstrained.recorder().interactions().empty() &&
          unconstrained.recorder().timeAdvanceGrants().empty() &&
          unconstrained.recorder().callbackOrder() ==
              std::vector<std::string>{"interaction"},
      "unconstrained recipient did not receive exactly one time-bearing interaction");

  auto const checkDelivery = [&](
                                  TimedInteractionRecord const& received,
                                  rti::InteractionClassHandle const& receiverClass,
                                  rti::ParameterHandle const& receiverParameter,
                                  rti::FederateHandle const& producer,
                                  rti::OrderType receivedOrder,
                                  std::string const& description) {
    require(
        received.interaction == receiverClass &&
            received.parameters.size() == 1U &&
            received.parameters.count(receiverParameter) == 1U &&
            copyBytes(received.parameters.at(receiverParameter)) == payload &&
            received.tag == tagBytes && received.producer == producer &&
            received.time == encodeTime(*messageTime) &&
            !received.timeText.empty() && received.sentOrder == rti::TIMESTAMP &&
            received.receivedOrder == receivedOrder && received.retractionPresent &&
            received.retraction == copyBytes(retraction.encode()) &&
            received.transportation.isValid(),
        description + " did not preserve the sent message's standard data");
  };

  auto const unconstrainedDelivery =
      unconstrained.recorder().timedInteractions().front();
  checkDelivery(
      unconstrainedDelivery,
      unconstrainedClass,
      unconstrainedParameter,
      sender.federateHandle(),
      rti::RECEIVE,
      "unconstrained RO conversion");
  require(
      !unconstrained.rtiAmbassador()
           .getTransportationTypeName(unconstrainedDelivery.transportation)
           .empty(),
      "unconstrained RO conversion returned an unresolvable transportation type");

  constrained.rtiAmbassador().timeAdvanceRequest(*messageTime);
  sender.rtiAmbassador().timeAdvanceRequest(*senderTarget);
  waitFor(
      sender,
      constrained,
      [&] {
        return sender.recorder().timeAdvanceGrants().size() == 1U &&
            constrained.recorder().timedInteractions().size() == 1U &&
            constrained.recorder().timeAdvanceGrants().size() == 1U;
      },
      options,
      "constrained TSO delivery and matching grant");
  require(
      constrained.recorder().callbackOrder() ==
          std::vector<std::string>{"interaction", "grant"},
      "constrained TSO callback did not precede its matching grant");
  auto const constrainedDelivery =
      constrained.recorder().timedInteractions().front();
  checkDelivery(
      constrainedDelivery,
      constrainedClass,
      constrainedParameter,
      sender.federateHandle(),
      rti::TIMESTAMP,
      "constrained TSO delivery");
  auto const constrainedGrant = constrained.recorder().timeAdvanceGrants().front();
  require(
      constrainedGrant.encoded == encodeTime(*messageTime),
      "constrained recipient received a grant different from its request");

  constrained.rtiAmbassador().disableTimeConstrained();
  sender.rtiAmbassador().disableTimeRegulation();
  constrained.rtiAmbassador().unsubscribeInteractionClass(constrainedClass);
  unconstrained.rtiAmbassador().unsubscribeInteractionClass(unconstrainedClass);
  sender.rtiAmbassador().unpublishInteractionClass(senderClass);
  unconstrained.resign(rti::NO_ACTION);
  constrained.resign(rti::NO_ACTION);
  sender.resign(rti::NO_ACTION);
  sender.rtiAmbassador().destroyFederationExecution(federation);
  unconstrained.disconnect();
  constrained.disconnect();
  sender.disconnect();
}

void scenarioSentTsoPerRecipientOrderConversionContract(
    Options const& options, rti::CallbackModel model) {
  scenarioSentTsoPerRecipientOrderConversion(options, model);
}

}  // namespace hla_rti_cpp_tck::detail
