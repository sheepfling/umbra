#include "tck_main_scenarios.hpp"

#include <cstdint>
#include <vector>

namespace hla_rti_cpp_tck::detail {

void scenarioDirectedRetractionMixedFanout(
    Options const& options, rti::CallbackModel model) {
  require(
      !options.logicalTimeImplementationName.empty(),
      "Directed mixed-fanout retraction requires an adapter-supplied time implementation");

  Session publisher(options, model, "directed-mixed-retraction-publisher");
  Session immediate(options, model, "directed-mixed-retraction-immediate");
  Session constrained(options, model, "directed-mixed-retraction-constrained");
  auto const federation = federationName(options, "directed-retraction-mixed-fanout");
  connectAndJoin(publisher, immediate, options, federation, options.fom);
  constrained.connect();
  constrained.join(
      options.memberFederateName + L"-directed-mixed-retraction-constrained",
      options.federateType, federation);

  auto& sender = publisher.rtiAmbassador();
  auto& immediateRti = immediate.rtiAmbassador();
  auto& constrainedRti = constrained.rtiAmbassador();
  auto const objectClass = sender.getObjectClassHandle(options.objectClassName);
  auto const marker = sender.getAttributeHandle(objectClass, options.attributeName);
  auto const interaction = sender.getInteractionClassHandle(options.interactionClassName);
  auto const parameter = sender.getParameterHandle(interaction, options.parameterName);
  auto const immediateClass = immediateRti.getObjectClassHandle(options.objectClassName);
  auto const constrainedClass = constrainedRti.getObjectClassHandle(options.objectClassName);
  auto const immediateInteraction = immediateRti.getInteractionClassHandle(
      options.interactionClassName);
  auto const constrainedInteraction = constrainedRti.getInteractionClassHandle(
      options.interactionClassName);
  auto const immediateParameter = immediateRti.getParameterHandle(
      immediateInteraction, options.parameterName);
  require(
      objectClass.isValid() && marker.isValid() && interaction.isValid() &&
          parameter.isValid() && immediateClass == objectClass &&
          constrainedClass == objectClass && immediateInteraction == interaction &&
          constrainedInteraction == interaction && immediateParameter == parameter,
      "directed mixed-fanout adapter FOM returned inconsistent standard handles");

  rti::AttributeHandleSet const attributes{marker};
  rti::InteractionClassHandleSet const directed{interaction};
  sender.publishObjectClassAttributes(objectClass, attributes);
  immediateRti.subscribeObjectClassAttributes(immediateClass, attributes, true, L"");
  constrainedRti.subscribeObjectClassAttributes(constrainedClass, attributes, true, L"");
  sender.publishObjectClassDirectedInteractions(objectClass, directed);
  sender.changeInteractionOrderType(interaction, rti::TIMESTAMP);
  immediateRti.subscribeObjectClassDirectedInteractions(immediateClass, directed, true);
  constrainedRti.subscribeObjectClassDirectedInteractions(constrainedClass, directed, true);

  auto const target = sender.registerObjectInstance(objectClass);
  waitForSessions(
      {&immediate, &constrained},
      [&] {
        return immediate.recorder().hasDiscovery(target) &&
            constrained.recorder().hasDiscovery(target);
      },
      options, "directed mixed-fanout target discovery");

  auto publisherTime = makeTimeContext(publisher);
  auto constrainedTime = makeTimeContext(constrained);
  require(
      publisherTime.factory->getName() == constrainedTime.factory->getName(),
      "directed mixed-fanout members selected different time factories");
  constrainedRti.enableTimeConstrained();
  waitFor(
      constrained,
      [&] { return constrained.recorder().timeConstrainedEnabled().size() == 1U; },
      options, "directed mixed-fanout Time Constrained Enabled");
  sender.enableTimeRegulation(*publisherTime.epsilon);
  waitFor(
      publisher,
      [&] { return publisher.recorder().timeRegulationEnabled().size() == 1U; },
      options, "directed mixed-fanout Time Regulation Enabled");
  immediate.recorder().clearTimedDirectedInteractions();
  immediate.recorder().clearRetractions();
  immediate.recorder().clearCallbackOrder();
  constrained.recorder().clearTimedDirectedInteractions();
  constrained.recorder().clearRetractions();
  constrained.recorder().clearTimeCallbacks();
  publisher.recorder().clearTimeCallbacks();

  auto const messageTime = timeAfter(
      *publisherTime.factory, *publisherTime.initial, *publisherTime.epsilon, 2U);
  auto const constrainedGrant = timeAfter(
      *constrainedTime.factory, *constrainedTime.initial, *constrainedTime.epsilon, 2U);
  std::vector<std::uint8_t> const value{0x4DU, 0x58U};
  std::vector<std::uint8_t> const tagBytes{0x52U, 0x44U, 0x49U};
  rti::ParameterHandleValueMap values;
  values.emplace(parameter, rti::VariableLengthData(value.data(), value.size()));
  auto const designator = sender.sendDirectedInteraction(
      interaction, target, values,
      rti::VariableLengthData(tagBytes.data(), tagBytes.size()), *messageTime);
  require(designator.isValid(), "timestamped directed send returned no retraction handle");

  waitFor(
      immediate,
      [&] { return immediate.recorder().timedDirectedInteractions().size() == 1U; },
      options, "nonconstrained directed recipient delivery");
  auto const delivered = immediate.recorder().timedDirectedInteractions();
  require(
      delivered.size() == 1U && delivered.front().interaction == immediateInteraction &&
          delivered.front().object == target &&
          delivered.front().parameters.size() == 1U &&
          delivered.front().parameters.count(immediateParameter) == 1U &&
          copyBytes(delivered.front().parameters.at(immediateParameter)) == value &&
          delivered.front().tag == tagBytes &&
          delivered.front().producer == publisher.federateHandle(),
      "nonconstrained directed delivery changed its public message payload");
  require(
      delivered.front().time == encodeTime(*messageTime) &&
          delivered.front().sentOrder == rti::TIMESTAMP &&
          delivered.front().receivedOrder == rti::RECEIVE &&
          delivered.front().retractionPresent &&
          delivered.front().retraction == copyBytes(designator.encode()),
      "nonconstrained directed delivery changed time, order, or retraction metadata");
  require(
      constrained.recorder().timedDirectedInteractions().empty(),
      "constrained recipient received its directed copy before advancing");

  sender.retract(designator);
  waitFor(
      immediate,
      [&] { return immediate.recorder().retractions().size() == 1U; },
      options, "delivered directed recipient Request Retraction callback");
  auto const retractions = immediate.recorder().retractions();
  require(
      retractions.size() == 1U && retractions.front().valid &&
          retractions.front().encoded == copyBytes(designator.encode()) &&
          immediate.recorder().callbackOrder() ==
              std::vector<std::string>{"directed", "request-retraction"},
      "delivered directed recipient did not receive one matching Request Retraction");
  require(
      constrained.recorder().timedDirectedInteractions().empty() &&
          constrained.recorder().retractions().empty(),
      "pending directed recipient received a stale callback before its grant");

  constrainedRti.timeAdvanceRequest(*constrainedGrant);
  sender.timeAdvanceRequest(*messageTime);
  waitForSessions(
      {&publisher, &constrained},
      [&] {
        return publisher.recorder().timeAdvanceGrants().size() == 1U &&
            constrained.recorder().timeAdvanceGrants().size() == 1U;
      },
      options, "directed mixed-fanout post-retraction grants");
  for (int pass = 0; pass != 8; ++pass) constrained.pump();
  require(
      constrained.recorder().timeAdvanceGrants().front().encoded ==
              encodeTime(*constrainedGrant) &&
          constrained.recorder().timedDirectedInteractions().empty() &&
          constrained.recorder().retractions().empty(),
      "retracted pending directed copy escaped at the constrained grant");

  constrainedRti.disableTimeConstrained();
  sender.disableTimeRegulation();
  immediateRti.unsubscribeObjectClassDirectedInteractions(immediateClass, directed);
  constrainedRti.unsubscribeObjectClassDirectedInteractions(constrainedClass, directed);
  sender.unpublishObjectClassDirectedInteractions(objectClass, directed);
  immediateRti.unsubscribeObjectClassAttributes(immediateClass, attributes);
  constrainedRti.unsubscribeObjectClassAttributes(constrainedClass, attributes);
  sender.unpublishObjectClassAttributes(objectClass, attributes);
  immediate.resign(rti::NO_ACTION);
  constrained.resign(rti::NO_ACTION);
  publisher.resign(rti::DELETE_OBJECTS);
  sender.destroyFederationExecution(federation);
  immediate.disconnect();
  constrained.disconnect();
  publisher.disconnect();
}

void scenarioDirectedRetractionMixedFanoutContract(
    Options const& options, rti::CallbackModel model) {
  scenarioDirectedRetractionMixedFanout(options, model);
}

void scenarioDirectedRetractionImmediateOnly(
    Options const& options, rti::CallbackModel model) {
  require(
      !options.logicalTimeImplementationName.empty(),
      "Directed immediate-only retraction requires an adapter-supplied time implementation");

  Session publisher(options, model, "directed-immediate-retraction-publisher");
  Session receiver(options, model, "directed-immediate-retraction-receiver");
  auto const federation = federationName(options, "directed-retraction-immediate-only");
  connectAndJoin(publisher, receiver, options, federation, options.fom);
  auto& sender = publisher.rtiAmbassador();
  auto& recipient = receiver.rtiAmbassador();
  auto const objectClass = sender.getObjectClassHandle(options.objectClassName);
  auto const receiverClass = recipient.getObjectClassHandle(options.objectClassName);
  auto const marker = sender.getAttributeHandle(objectClass, options.attributeName);
  auto const receiverMarker = recipient.getAttributeHandle(receiverClass, options.attributeName);
  auto const interaction = sender.getInteractionClassHandle(options.interactionClassName);
  auto const receiverInteraction = recipient.getInteractionClassHandle(
      options.interactionClassName);
  auto const parameter = sender.getParameterHandle(interaction, options.parameterName);
  auto const receiverParameter = recipient.getParameterHandle(
      receiverInteraction, options.parameterName);
  require(
      objectClass.isValid() && receiverClass == objectClass &&
          marker.isValid() && receiverMarker == marker &&
          interaction.isValid() && receiverInteraction == interaction &&
          parameter.isValid() && receiverParameter == parameter,
      "directed immediate-only adapter FOM returned inconsistent standard handles");

  rti::AttributeHandleSet const attributes{marker};
  rti::InteractionClassHandleSet const directed{interaction};
  sender.publishObjectClassAttributes(objectClass, attributes);
  recipient.subscribeObjectClassAttributes(receiverClass, attributes, true, L"");
  sender.publishObjectClassDirectedInteractions(objectClass, directed);
  sender.changeInteractionOrderType(interaction, rti::TIMESTAMP);
  recipient.subscribeObjectClassDirectedInteractions(receiverClass, directed, true);
  auto const target = sender.registerObjectInstance(objectClass);
  waitFor(
      receiver, [&] { return receiver.recorder().hasDiscovery(target); },
      options, "directed immediate-only target discovery");

  auto time = makeTimeContext(publisher);
  sender.enableTimeRegulation(*time.epsilon);
  waitFor(
      publisher,
      [&] { return publisher.recorder().timeRegulationEnabled().size() == 1U; },
      options, "directed immediate-only Time Regulation Enabled");
  auto const grantTime = timeAfter(*time.factory, *time.initial, *time.epsilon, 2U);
  auto const messageTime = timeAfter(*time.factory, *time.initial, *time.epsilon, 4U);
  publisher.recorder().clearTimeCallbacks();
  sender.timeAdvanceRequest(*grantTime);
  waitFor(
      publisher,
      [&] { return publisher.recorder().timeAdvanceGrants().size() == 1U; },
      options, "directed immediate-only publisher advance");
  require(
      publisher.recorder().timeAdvanceGrants().front().encoded == encodeTime(*grantTime),
      "directed immediate-only publisher received the wrong grant time");
  require(
      receiver.recorder().timeConstrainedEnabled().empty(),
      "directed immediate-only receiver unexpectedly enabled Time Constrained");
  receiver.recorder().clearTimedDirectedInteractions();
  receiver.recorder().clearRetractions();
  receiver.recorder().clearCallbackOrder();

  std::vector<std::uint8_t> const value{0x49U, 0x4FU};
  std::vector<std::uint8_t> const tagBytes{0x52U, 0x44U, 0x49U};
  rti::ParameterHandleValueMap values;
  values.emplace(parameter, rti::VariableLengthData(value.data(), value.size()));
  auto const designator = sender.sendDirectedInteraction(
      interaction, target, values,
      rti::VariableLengthData(tagBytes.data(), tagBytes.size()), *messageTime);
  require(designator.isValid(), "immediate-only directed send returned no retraction handle");
  waitFor(
      receiver,
      [&] { return receiver.recorder().timedDirectedInteractions().size() == 1U; },
      options, "immediate-only directed delivery");
  auto const deliveries = receiver.recorder().timedDirectedInteractions();
  require(
      deliveries.size() == 1U && deliveries.front().interaction == receiverInteraction &&
          deliveries.front().object == target &&
          deliveries.front().parameters.size() == 1U &&
          deliveries.front().parameters.count(receiverParameter) == 1U &&
          copyBytes(deliveries.front().parameters.at(receiverParameter)) == value &&
          deliveries.front().tag == tagBytes &&
          deliveries.front().producer == publisher.federateHandle(),
      "immediate-only directed delivery changed its target, payload, or metadata");
  require(
      deliveries.front().time == encodeTime(*messageTime) &&
          deliveries.front().sentOrder == rti::TIMESTAMP &&
          deliveries.front().receivedOrder == rti::RECEIVE &&
          deliveries.front().retractionPresent &&
          deliveries.front().retraction == copyBytes(designator.encode()),
      "immediate-only directed delivery lost time, order, or retraction metadata");

  sender.retract(designator);
  waitFor(
      receiver,
      [&] { return receiver.recorder().retractions().size() == 1U; },
      options, "immediate-only directed Request Retraction");
  for (int pass = 0; pass != 8; ++pass) receiver.pump();
  auto const retractions = receiver.recorder().retractions();
  require(
      receiver.recorder().timedDirectedInteractions().size() == 1U &&
          retractions.size() == 1U && retractions.front().valid &&
          retractions.front().encoded == copyBytes(designator.encode()) &&
          receiver.recorder().callbackOrder() ==
              std::vector<std::string>{"directed", "request-retraction"},
      "immediate-only directed recipient did not receive one matching retraction callback");

  sender.disableTimeRegulation();
  recipient.unsubscribeObjectClassDirectedInteractions(receiverClass, directed);
  sender.unpublishObjectClassDirectedInteractions(objectClass, directed);
  recipient.unsubscribeObjectClassAttributes(receiverClass, attributes);
  sender.unpublishObjectClassAttributes(objectClass, attributes);
  receiver.resign(rti::NO_ACTION);
  publisher.resign(rti::DELETE_OBJECTS);
  sender.destroyFederationExecution(federation);
  receiver.disconnect();
  publisher.disconnect();
}

void scenarioDirectedRetractionImmediateOnlyContract(
    Options const& options, rti::CallbackModel model) {
  scenarioDirectedRetractionImmediateOnly(options, model);
}

}  // namespace hla_rti_cpp_tck::detail
