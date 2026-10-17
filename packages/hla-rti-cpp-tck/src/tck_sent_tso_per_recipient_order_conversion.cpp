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

void scenarioSentTsoAttributePerRecipientOrderConversion(
    Options const& options, rti::CallbackModel model) {
  Session sender(options, model, "owner");
  Session constrained(options, model, "member");
  Session unconstrained(options, model, "member");
  auto const federation = federationName(
      options, "sent-tso-attribute-per-recipient-order");
  connectAndJoin(sender, constrained, options, federation, options.fom);
  unconstrained.connect();
  auto const unconstrainedName = options.memberFederateName + L"-unconstrained";
  unconstrained.join(unconstrainedName, options.federateType, federation);

  auto const senderClass = sender.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const constrainedClass = constrained.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const unconstrainedClass = unconstrained.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const senderAttribute = sender.rtiAmbassador().getAttributeHandle(
      senderClass, options.attributeName);
  auto const constrainedAttribute = constrained.rtiAmbassador().getAttributeHandle(
      constrainedClass, options.attributeName);
  auto const unconstrainedAttribute = unconstrained.rtiAmbassador().getAttributeHandle(
      unconstrainedClass, options.attributeName);
  require(
      senderClass.isValid() && constrainedClass.isValid() &&
          unconstrainedClass.isValid() && senderAttribute.isValid() &&
          constrainedAttribute.isValid() && unconstrainedAttribute.isValid() &&
          senderClass == constrainedClass && senderClass == unconstrainedClass &&
          senderAttribute == constrainedAttribute &&
          senderAttribute == unconstrainedAttribute,
      "per-recipient timestamped attribute conversion did not resolve common FOM handles");

  rti::AttributeHandleSet const senderAttributes{senderAttribute};
  rti::AttributeHandleSet const constrainedAttributes{constrainedAttribute};
  rti::AttributeHandleSet const unconstrainedAttributes{unconstrainedAttribute};
  sender.rtiAmbassador().publishObjectClassAttributes(senderClass, senderAttributes);
  constrained.rtiAmbassador().subscribeObjectClassAttributes(
      constrainedClass, constrainedAttributes, true, L"");
  unconstrained.rtiAmbassador().subscribeObjectClassAttributes(
      unconstrainedClass, unconstrainedAttributes, true, L"");
  sender.rtiAmbassador().changeDefaultAttributeOrderType(
      senderClass, senderAttributes, rti::TIMESTAMP);

  auto const object = sender.rtiAmbassador().registerObjectInstance(senderClass);
  require(
      object.isValid(),
      "per-recipient timestamped attribute registration returned an invalid handle");
  waitFor(
      constrained,
      [&] { return constrained.recorder().hasDiscovery(object); },
      options,
      "constrained attribute recipient discovery");
  waitFor(
      unconstrained,
      [&] { return unconstrained.recorder().hasDiscovery(object); },
      options,
      "unconstrained attribute recipient discovery");

  auto senderTime = makeTimeContext(sender);
  auto constrainedTime = makeTimeContext(constrained);
  auto unconstrainedTime = makeTimeContext(unconstrained);
  require(
      senderTime.factory->getName() == constrainedTime.factory->getName() &&
          senderTime.factory->getName() == unconstrainedTime.factory->getName() &&
          encodeTime(*senderTime.initial) == encodeTime(*constrainedTime.initial) &&
          encodeTime(*senderTime.initial) == encodeTime(*unconstrainedTime.initial),
      "per-recipient attribute members selected incompatible logical-time representations");

  constrained.rtiAmbassador().enableTimeConstrained();
  waitFor(
      constrained,
      [&] { return constrained.recorder().timeConstrainedEnabled().size() == 1U; },
      options,
      "constrained attribute recipient enablement");
  auto lookahead = senderTime.factory->makeZero();
  require(lookahead != nullptr,
          "per-recipient attribute time factory returned no zero interval");
  for (std::size_t step = 0U; step != 5U; ++step) {
    *lookahead += *senderTime.epsilon;
  }
  sender.rtiAmbassador().enableTimeRegulation(*lookahead);
  waitFor(
      sender,
      [&] { return sender.recorder().timeRegulationEnabled().size() == 1U; },
      options,
      "attribute sender regulation enablement");

  sender.recorder().clearCallbackOrder();
  constrained.recorder().clearCallbackOrder();
  unconstrained.recorder().clearCallbackOrder();
  auto const messageTime = timeAfter(
      *senderTime.factory, *senderTime.initial, *senderTime.epsilon, 6U);
  auto const senderTarget = timeAfter(
      *senderTime.factory, *senderTime.initial, *senderTime.epsilon, 2U);
  std::vector<std::uint8_t> const valueBytes{0x54U, 0x53U, 0x4FU};
  std::vector<std::uint8_t> const tagBytes{0x50U, 0x52U};
  rti::AttributeHandleValueMap values;
  values.emplace(
      senderAttribute,
      rti::VariableLengthData(valueBytes.data(), valueBytes.size()));
  rti::VariableLengthData tag(tagBytes.data(), tagBytes.size());
  auto const retraction = sender.rtiAmbassador().updateAttributeValues(
      object, values, tag, *messageTime);
  require(
      retraction.isValid(),
      "timestamped Update Attribute Values returned no retraction handle");

  waitFor(
      unconstrained,
      [&] { return unconstrained.recorder().timedReflections().size() == 1U; },
      options,
      "unconstrained recipient's time-bearing receive-order reflection");
  require(
      unconstrained.recorder().reflections().empty() &&
          unconstrained.recorder().timeAdvanceGrants().empty() &&
          constrained.recorder().timedReflections().empty() &&
          constrained.recorder().timeAdvanceGrants().empty(),
      "timestamped attribute update did not separate unconstrained RO from constrained TSO delivery");
  require(
      unconstrained.recorder().callbackOrder() ==
          std::vector<std::string>{"reflect"},
      "unconstrained recipient did not receive exactly one reflection before any grant");

  auto const checkDelivery = [&](TimedReflectionRecord const& received,
                                rti::AttributeHandle const& receiverAttribute,
                                rti::FederateHandle const& producer,
                                rti::OrderType receivedOrder,
                                Session& inspector,
                                std::string const& description) {
    require(
        received.object == object && received.values.size() == 1U &&
            received.values.count(receiverAttribute) == 1U &&
            copyBytes(received.values.at(receiverAttribute)) == valueBytes &&
            received.tag == tagBytes && received.producer == producer &&
            received.time == encodeTime(*messageTime) &&
            received.sentOrder == rti::TIMESTAMP &&
            received.receivedOrder == receivedOrder && received.retractionPresent &&
            received.retraction == copyBytes(retraction.encode()) &&
            received.transportation.isValid(),
        description + " changed the attribute message or its order/retraction metadata");
    require(
        !inspector.rtiAmbassador()
             .getTransportationTypeName(received.transportation)
             .empty(),
        description + " returned an unresolvable transportation type");
  };

  auto const unconstrainedDelivery =
      unconstrained.recorder().timedReflections().front();
  checkDelivery(
      unconstrainedDelivery,
      unconstrainedAttribute,
      sender.federateHandle(),
      rti::RECEIVE,
      unconstrained,
      "unconstrained receive-order conversion");

  constrained.rtiAmbassador().timeAdvanceRequest(*messageTime);
  sender.rtiAmbassador().timeAdvanceRequest(*senderTarget);
  waitFor(
      sender,
      constrained,
      [&] {
        return sender.recorder().timeAdvanceGrants().size() == 1U &&
            constrained.recorder().timedReflections().size() == 1U &&
            constrained.recorder().timeAdvanceGrants().size() == 1U;
      },
      options,
      "constrained timestamped attribute delivery and matching grants");
  require(
      constrained.recorder().callbackOrder() ==
          std::vector<std::string>{"reflect", "grant"},
      "constrained timestamped reflection did not precede its matching grant");
  auto const constrainedDelivery =
      constrained.recorder().timedReflections().front();
  checkDelivery(
      constrainedDelivery,
      constrainedAttribute,
      sender.federateHandle(),
      rti::TIMESTAMP,
      constrained,
      "constrained timestamp-order delivery");
  require(
      constrained.recorder().timeAdvanceGrants().front().encoded ==
          encodeTime(*messageTime),
      "constrained recipient received a grant different from its request");
  require(
      unconstrained.recorder().timedReflections().size() == 1U &&
          unconstrained.recorder().callbackOrder() ==
              std::vector<std::string>{"reflect"},
      "unconstrained recipient received a duplicate reflection or grant");

  constrained.rtiAmbassador().disableTimeConstrained();
  sender.rtiAmbassador().disableTimeRegulation();
  constrained.rtiAmbassador().unsubscribeObjectClassAttributes(
      constrainedClass, constrainedAttributes);
  unconstrained.rtiAmbassador().unsubscribeObjectClassAttributes(
      unconstrainedClass, unconstrainedAttributes);
  sender.rtiAmbassador().unpublishObjectClassAttributes(
      senderClass, senderAttributes);
  unconstrained.resign(rti::NO_ACTION);
  constrained.resign(rti::NO_ACTION);
  sender.resign(rti::NO_ACTION);
  sender.rtiAmbassador().destroyFederationExecution(federation);
  unconstrained.disconnect();
  constrained.disconnect();
  sender.disconnect();
}

void scenarioSentTsoAttributePerRecipientOrderConversionContract(
    Options const& options, rti::CallbackModel model) {
  scenarioSentTsoAttributePerRecipientOrderConversion(options, model);
}

}  // namespace hla_rti_cpp_tck::detail
