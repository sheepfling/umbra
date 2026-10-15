#include "tck_custom_transportation_interaction_scenarios.hpp"
#include "tck_main_scenarios.hpp"

namespace hla_rti_cpp_tck::detail {

void scenarioCustomTransportationInteractionDelivery(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.modelFom.empty(),
      "Custom transportation delivery requires an adapter-supplied rich FOM model");

  Session producer(options, model, "custom-transportation-producer");
  Session consumer(options, model, "custom-transportation-consumer");
  auto const federation = federationName(
      options,
      "custom-transportation-interaction-delivery");
  connectAndJoin(producer, consumer, options, federation, options.modelFom);

  auto const producerInteraction = producer.rtiAmbassador().getInteractionClassHandle(
      options.typedInteractionClassName);
  auto const consumerInteraction = consumer.rtiAmbassador().getInteractionClassHandle(
      options.typedInteractionClassName);
  auto const producerParameter = producer.rtiAmbassador().getParameterHandle(
      producerInteraction,
      options.typedIntegerParameterName);
  auto const consumerParameter = consumer.rtiAmbassador().getParameterHandle(
      consumerInteraction,
      options.typedIntegerParameterName);
  auto const producerTransportation = producer.rtiAmbassador().getTransportationTypeHandle(
      options.fomCustomTransportationName);
  auto const consumerTransportation = consumer.rtiAmbassador().getTransportationTypeHandle(
      options.fomCustomTransportationName);
  require(
      producerInteraction.isValid() && consumerInteraction.isValid() &&
          producerParameter.isValid() && consumerParameter.isValid() &&
          producerTransportation.isValid() && consumerTransportation.isValid(),
      "Custom transportation interaction lookup returned an invalid handle");
  require(
      producer.rtiAmbassador().getTransportationTypeName(producerTransportation) ==
              options.fomCustomTransportationName &&
          consumer.rtiAmbassador().getTransportationTypeName(consumerTransportation) ==
              options.fomCustomTransportationName,
      "Custom transportation name lookup did not round-trip");

  producer.rtiAmbassador().publishInteractionClass(producerInteraction);
  consumer.rtiAmbassador().subscribeInteractionClass(consumerInteraction, true);

  auto const payload = encodeDataElement(rti::HLAinteger32BE{42});
  rti::ParameterHandleValueMap parameters;
  parameters.emplace(
      producerParameter,
      rti::VariableLengthData(payload.data(), payload.size()));
  std::vector<std::uint8_t> const tagBytes{0x43U, 0x54U, 0x58U};
  rti::VariableLengthData const tag(tagBytes.data(), tagBytes.size());
  producer.rtiAmbassador().sendInteraction(
      producerInteraction,
      parameters,
      tag);

  waitFor(
      consumer,
      [&] { return consumer.recorder().interaction().present; },
      options,
      "custom transportation interaction delivery");
  auto const received = consumer.recorder().interaction();
  require(
      received.interaction == consumerInteraction &&
          received.parameters.size() == 1U &&
          received.parameters.count(consumerParameter) == 1U &&
          copyBytes(received.parameters.at(consumerParameter)) == payload &&
          received.tag == tagBytes &&
          received.producer == producer.federateHandle() &&
          received.transportation.isValid() &&
          consumer.rtiAmbassador().getTransportationTypeName(received.transportation) ==
              options.fomCustomTransportationName &&
          !received.regions.has_value(),
      "Custom transportation interaction delivery changed the standard callback metadata");

  producer.recorder().clearTransportationRecords();
  producer.rtiAmbassador().queryInteractionTransportationType(
      producer.federateHandle(),
      producerInteraction);
  waitFor(
      producer,
      [&] {
        return !producer.recorder().interactionTransportationReports().empty();
      },
      options,
      "custom transportation interaction query");
  auto const reports = producer.recorder().interactionTransportationReports();
  auto const report = std::find_if(
      reports.begin(),
      reports.end(),
      [&](auto const& candidate) {
        return candidate.federate == producer.federateHandle() &&
            candidate.interaction == producerInteraction;
      });
  require(
      report != reports.end() && report->transportation.isValid() &&
          producer.rtiAmbassador().getTransportationTypeName(report->transportation) ==
              options.fomCustomTransportationName,
      "Custom transportation interaction query did not report the committed type");

  consumer.rtiAmbassador().unsubscribeInteractionClass(consumerInteraction);
  producer.rtiAmbassador().unpublishInteractionClass(producerInteraction);
  consumer.resign(rti::NO_ACTION);
  producer.resign(rti::NO_ACTION);
  producer.rtiAmbassador().destroyFederationExecution(federation);
  consumer.disconnect();
  producer.disconnect();
}

void scenarioCustomTransportationInteractionDeliveryContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioCustomTransportationInteractionDelivery(options, model);
}

void scenarioCustomTransportationTimestampedDelivery(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.modelFom.empty(),
      "Custom transportation timestamped delivery requires an adapter-supplied rich FOM model");
  require(
      !options.logicalTimeImplementationName.empty(),
      "Custom transportation timestamped delivery requires an adapter-supplied logical-time implementation");

  Session producer(options, model, "custom-transportation-timestamped-producer");
  Session receiver(options, model, "custom-transportation-timestamped-receiver");
  auto const federation = federationName(
      options,
      "custom-transportation-timestamped-delivery");
  connectAndJoin(producer, receiver, options, federation, options.modelFom);

  auto const producerInteraction = producer.rtiAmbassador().getInteractionClassHandle(
      options.typedInteractionClassName);
  auto const receiverInteraction = receiver.rtiAmbassador().getInteractionClassHandle(
      options.typedInteractionClassName);
  auto const producerParameter = producer.rtiAmbassador().getParameterHandle(
      producerInteraction,
      options.typedIntegerParameterName);
  auto const receiverParameter = receiver.rtiAmbassador().getParameterHandle(
      receiverInteraction,
      options.typedIntegerParameterName);
  auto const producerTransportation = producer.rtiAmbassador().getTransportationTypeHandle(
      options.fomCustomTransportationName);
  auto const receiverTransportation = receiver.rtiAmbassador().getTransportationTypeHandle(
      options.fomCustomTransportationName);
  require(
      producerInteraction.isValid() && receiverInteraction.isValid() &&
          producerParameter.isValid() && receiverParameter.isValid() &&
          producerTransportation.isValid() && receiverTransportation.isValid(),
      "Custom transportation timestamped lookup returned an invalid handle");
  require(
      producer.rtiAmbassador().getTransportationTypeName(producerTransportation) ==
              options.fomCustomTransportationName &&
          receiver.rtiAmbassador().getTransportationTypeName(receiverTransportation) ==
              options.fomCustomTransportationName,
      "Custom transportation timestamped name lookup did not round-trip");

  producer.rtiAmbassador().publishInteractionClass(producerInteraction);
  producer.rtiAmbassador().changeInteractionOrderType(
      producerInteraction,
      rti::TIMESTAMP);
  receiver.rtiAmbassador().subscribeInteractionClass(receiverInteraction, true);

  auto const producerTime = makeTimeContext(producer);
  auto const receiverTime = makeTimeContext(receiver);
  enableTimestampedRoles(
      producer,
      receiver,
      producerTime,
      receiverTime,
      options,
      "custom transportation timestamped delivery");

  auto const messageTime = timeAfter(
      *producerTime.factory,
      *producerTime.initial,
      *producerTime.epsilon,
      3U);
  auto const receiverTarget = timeAfter(
      *receiverTime.factory,
      *receiverTime.initial,
      *receiverTime.epsilon,
      3U);
  auto const parameterBytes = copyBytes(rti::HLAinteger32BE{42}.encode());
  std::vector<std::uint8_t> const tagBytes{0x54U, 0x53U, 0x4FU};
  rti::ParameterHandleValueMap parameters;
  parameters.emplace(
      producerParameter,
      rti::VariableLengthData(parameterBytes.data(), parameterBytes.size()));
  rti::VariableLengthData const tag(tagBytes.data(), tagBytes.size());

  auto const retraction = producer.rtiAmbassador().sendInteraction(
      producerInteraction,
      parameters,
      tag,
      *messageTime);
  require(
      retraction.isValid(),
      "Custom transportation timestamped send returned an invalid retraction handle");
  require(
      receiver.recorder().timedInteractions().empty(),
      "Custom transportation timestamped interaction delivered before its grant");

  receiver.rtiAmbassador().timeAdvanceRequest(*receiverTarget);
  producer.rtiAmbassador().timeAdvanceRequest(*messageTime);
  waitForSessions(
      {&producer, &receiver},
      [&] {
        return producer.recorder().timeAdvanceGrants().size() >= 1U &&
            receiver.recorder().timeAdvanceGrants().size() >= 1U &&
            receiver.recorder().timedInteractions().size() >= 1U;
      },
      options,
      "custom transportation timestamped delivery and grants");

  auto const deliveries = receiver.recorder().timedInteractions();
  require(
      deliveries.size() == 1U,
      "Custom transportation timestamped delivery produced duplicate callbacks");
  auto const& delivery = deliveries.front();
  require(
      delivery.interaction == receiverInteraction &&
          delivery.parameters.size() == 1U &&
          delivery.parameters.count(receiverParameter) == 1U &&
          copyBytes(delivery.parameters.at(receiverParameter)) == parameterBytes,
      "Custom transportation timestamped delivery changed the interaction payload");
  require(
      delivery.tag == tagBytes && delivery.producer == producer.federateHandle() &&
          delivery.time == encodeTime(*messageTime) && !delivery.timeText.empty(),
      "Custom transportation timestamped delivery changed standard callback metadata");
  require(
      delivery.sentOrder == rti::TIMESTAMP &&
          delivery.receivedOrder == rti::TIMESTAMP &&
          delivery.retractionPresent &&
          delivery.retraction == copyBytes(retraction.encode()),
      "Custom transportation timestamped delivery changed order or retraction metadata");
  require(
      delivery.transportation.isValid() &&
          receiver.rtiAmbassador().getTransportationTypeName(delivery.transportation) ==
              options.fomCustomTransportationName &&
          !delivery.regions.has_value(),
      "Custom transportation timestamped delivery changed transportation or region metadata");

  producer.rtiAmbassador().disableTimeRegulation();
  receiver.rtiAmbassador().disableTimeConstrained();
  receiver.rtiAmbassador().unsubscribeInteractionClass(receiverInteraction);
  producer.rtiAmbassador().unpublishInteractionClass(producerInteraction);
  receiver.resign(rti::NO_ACTION);
  producer.resign(rti::NO_ACTION);
  producer.rtiAmbassador().destroyFederationExecution(federation);
  receiver.disconnect();
  producer.disconnect();
}

void scenarioCustomTransportationTimestampedDeliveryContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioCustomTransportationTimestampedDelivery(options, model);
}

void scenarioCustomTransportationTimestampedDirectedDelivery(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.modelFom.empty(),
      "Custom transportation timestamped directed delivery requires an adapter-supplied rich FOM model");
  require(
      !options.logicalTimeImplementationName.empty(),
      "Custom transportation timestamped directed delivery requires an adapter-supplied logical-time implementation");

  Session producer(options, model, "custom-transportation-timestamped-directed-producer");
  Session receiver(options, model, "custom-transportation-timestamped-directed-receiver");
  auto const federation = federationName(
      options,
      "custom-transportation-timestamped-directed-delivery");
  connectAndJoin(producer, receiver, options, federation, options.modelFom);

  auto const producerClass = producer.rtiAmbassador().getObjectClassHandle(
      options.typedObjectClassName);
  auto const receiverClass = receiver.rtiAmbassador().getObjectClassHandle(
      options.typedObjectClassName);
  auto const producerAttribute = producer.rtiAmbassador().getAttributeHandle(
      producerClass,
      options.typedIntegerAttributeName);
  auto const receiverAttribute = receiver.rtiAmbassador().getAttributeHandle(
      receiverClass,
      options.typedIntegerAttributeName);
  auto const producerInteraction = producer.rtiAmbassador().getInteractionClassHandle(
      options.typedInteractionClassName);
  auto const receiverInteraction = receiver.rtiAmbassador().getInteractionClassHandle(
      options.typedInteractionClassName);
  auto const producerParameter = producer.rtiAmbassador().getParameterHandle(
      producerInteraction,
      options.typedIntegerParameterName);
  auto const receiverParameter = receiver.rtiAmbassador().getParameterHandle(
      receiverInteraction,
      options.typedIntegerParameterName);
  auto const producerTransportation = producer.rtiAmbassador().getTransportationTypeHandle(
      options.fomCustomTransportationName);
  auto const receiverTransportation = receiver.rtiAmbassador().getTransportationTypeHandle(
      options.fomCustomTransportationName);
  require(
      producerClass.isValid() && receiverClass.isValid() &&
          producerAttribute.isValid() && receiverAttribute.isValid() &&
          producerInteraction.isValid() && receiverInteraction.isValid() &&
          producerParameter.isValid() && receiverParameter.isValid() &&
          producerTransportation.isValid() && receiverTransportation.isValid(),
      "Custom transportation timestamped directed lookup returned an invalid handle");
  require(
      producer.rtiAmbassador().getTransportationTypeName(producerTransportation) ==
              options.fomCustomTransportationName &&
          receiver.rtiAmbassador().getTransportationTypeName(receiverTransportation) ==
              options.fomCustomTransportationName,
      "Custom transportation timestamped directed name lookup did not round-trip");

  rti::AttributeHandleSet const producerAttributes{producerAttribute};
  rti::AttributeHandleSet const receiverAttributes{receiverAttribute};
  rti::InteractionClassHandleSet const producerDirected{producerInteraction};
  rti::InteractionClassHandleSet const receiverDirected{receiverInteraction};
  producer.rtiAmbassador().publishObjectClassAttributes(
      producerClass,
      producerAttributes);
  receiver.rtiAmbassador().subscribeObjectClassAttributes(
      receiverClass,
      receiverAttributes,
      true,
      L"");
  producer.rtiAmbassador().publishObjectClassDirectedInteractions(
      producerClass,
      producerDirected);
  producer.rtiAmbassador().changeInteractionOrderType(
      producerInteraction,
      rti::TIMESTAMP);
  receiver.rtiAmbassador().subscribeObjectClassDirectedInteractions(
      receiverClass,
      receiverDirected,
      true);

  auto const target = producer.rtiAmbassador().registerObjectInstance(producerClass);
  require(
      target.isValid(),
      "Custom transportation timestamped directed registration returned an invalid handle");
  waitFor(
      receiver,
      [&] { return receiver.recorder().hasDiscovery(target); },
      options,
      "custom transportation timestamped directed target discovery");

  auto const producerTime = makeTimeContext(producer);
  auto const receiverTime = makeTimeContext(receiver);
  enableTimestampedRoles(
      producer,
      receiver,
      producerTime,
      receiverTime,
      options,
      "custom transportation timestamped directed delivery");

  auto const messageTime = timeAfter(
      *producerTime.factory,
      *producerTime.initial,
      *producerTime.epsilon,
      3U);
  auto const receiverTarget = timeAfter(
      *receiverTime.factory,
      *receiverTime.initial,
      *receiverTime.epsilon,
      3U);
  auto const parameterBytes = copyBytes(rti::HLAinteger32BE{42}.encode());
  std::vector<std::uint8_t> const tagBytes{
      0x54U,
      0x53U,
      0x4FU,
      0x2DU,
      0x44U};
  rti::ParameterHandleValueMap parameters;
  parameters.emplace(
      producerParameter,
      rti::VariableLengthData(parameterBytes.data(), parameterBytes.size()));
  rti::VariableLengthData const tag(tagBytes.data(), tagBytes.size());

  auto const retraction = producer.rtiAmbassador().sendDirectedInteraction(
      producerInteraction,
      target,
      parameters,
      tag,
      *messageTime);
  require(
      retraction.isValid(),
      "Custom transportation timestamped directed send returned an invalid retraction handle");
  require(
      receiver.recorder().timedDirectedInteractions().empty(),
      "Custom transportation timestamped directed interaction delivered before its grant");

  receiver.rtiAmbassador().timeAdvanceRequest(*receiverTarget);
  producer.rtiAmbassador().timeAdvanceRequest(*messageTime);
  waitForSessions(
      {&producer, &receiver},
      [&] {
        return producer.recorder().timeAdvanceGrants().size() >= 1U &&
            receiver.recorder().timeAdvanceGrants().size() >= 1U &&
            receiver.recorder().timedDirectedInteractions().size() >= 1U;
      },
      options,
      "custom transportation timestamped directed delivery and grants");

  auto const deliveries = receiver.recorder().timedDirectedInteractions();
  require(
      deliveries.size() == 1U,
      "Custom transportation timestamped directed delivery produced duplicate callbacks");
  auto const& delivery = deliveries.front();
  require(
      delivery.interaction == receiverInteraction && delivery.object == target &&
          delivery.parameters.size() == 1U &&
          delivery.parameters.count(receiverParameter) == 1U &&
          copyBytes(delivery.parameters.at(receiverParameter)) == parameterBytes,
      "Custom transportation timestamped directed delivery changed the interaction payload or target");
  require(
      delivery.tag == tagBytes && delivery.producer == producer.federateHandle() &&
          delivery.time == encodeTime(*messageTime) && !delivery.timeText.empty(),
      "Custom transportation timestamped directed delivery changed standard callback metadata");
  require(
      delivery.sentOrder == rti::TIMESTAMP &&
          delivery.receivedOrder == rti::TIMESTAMP &&
          delivery.retractionPresent &&
          delivery.retraction == copyBytes(retraction.encode()),
      "Custom transportation timestamped directed delivery changed order or retraction metadata");
  require(
      delivery.transportation.isValid() &&
          receiver.rtiAmbassador().getTransportationTypeName(delivery.transportation) ==
              options.fomCustomTransportationName,
      "Custom transportation timestamped directed delivery changed transportation identity");

  producer.rtiAmbassador().disableTimeRegulation();
  receiver.rtiAmbassador().disableTimeConstrained();
  receiver.rtiAmbassador().unsubscribeObjectClassDirectedInteractions(
      receiverClass,
      receiverDirected);
  receiver.rtiAmbassador().unsubscribeObjectClassAttributes(
      receiverClass,
      receiverAttributes);
  producer.rtiAmbassador().unpublishObjectClassDirectedInteractions(
      producerClass,
      producerDirected);
  producer.rtiAmbassador().unpublishObjectClassAttributes(
      producerClass,
      producerAttributes);
  receiver.resign(rti::NO_ACTION);
  producer.resign(rti::NO_ACTION);
  producer.rtiAmbassador().destroyFederationExecution(federation);
  receiver.disconnect();
  producer.disconnect();
}

void scenarioCustomTransportationTimestampedDirectedDeliveryContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioCustomTransportationTimestampedDirectedDelivery(options, model);
}

}  // namespace hla_rti_cpp_tck::detail
