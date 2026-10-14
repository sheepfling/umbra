#include "tck_order_type_controls.hpp"
#include "tck_main_scenarios.hpp"

#include <cstdint>
#include <optional>
#include <vector>

namespace hla_rti_cpp_tck::detail {

void scenarioOrderTypeControls(Options const& options, rti::CallbackModel model) {
  require(
      !options.logicalTimeImplementationName.empty(),
      "Order-type control test requires an adapter-supplied logical-time implementation");

  Session publisher(options, model, "owner");
  Session receiver(options, model, "member");
  auto const federation = federationName(options, "order-type-controls");
  publisher.connect();
  receiver.connect();
  publisher.rtiAmbassador().createFederationExecution(
      federation,
      options.fom.wstring(),
      options.logicalTimeImplementationName);
  publisher.join(options.ownerFederateName, options.federateType, federation);
  receiver.join(
      options.memberFederateName + L"-order-controls",
      options.federateType,
      federation);

  auto const publisherClass = publisher.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const receiverClass = receiver.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const publisherAttribute = publisher.rtiAmbassador().getAttributeHandle(
      publisherClass,
      options.attributeName);
  auto const receiverAttribute = receiver.rtiAmbassador().getAttributeHandle(
      receiverClass,
      options.attributeName);
  auto const publisherInteraction =
      publisher.rtiAmbassador().getInteractionClassHandle(options.interactionClassName);
  auto const receiverInteraction =
      receiver.rtiAmbassador().getInteractionClassHandle(options.interactionClassName);
  auto const publisherParameter = publisher.rtiAmbassador().getParameterHandle(
      publisherInteraction,
      options.parameterName);
  auto const receiverParameter = receiver.rtiAmbassador().getParameterHandle(
      receiverInteraction,
      options.parameterName);
  require(
      publisherClass.isValid() && receiverClass.isValid() &&
          publisherAttribute.isValid() && receiverAttribute.isValid() &&
          publisherInteraction.isValid() && receiverInteraction.isValid() &&
          publisherParameter.isValid() && receiverParameter.isValid(),
      "Order-type control lookup returned an invalid adapter-supplied handle");

  rti::AttributeHandleSet const publisherAttributes{publisherAttribute};
  rti::AttributeHandleSet const receiverAttributes{receiverAttribute};
  publisher.rtiAmbassador().publishObjectClassAttributes(
      publisherClass,
      publisherAttributes);
  receiver.rtiAmbassador().subscribeObjectClassAttributes(
      receiverClass,
      receiverAttributes,
      true);
  publisher.rtiAmbassador().publishInteractionClass(publisherInteraction);
  receiver.rtiAmbassador().subscribeInteractionClass(receiverInteraction, true);

  // Establish a known class default, then verify that a later default change
  // affects only later registrations.  The explicit instance change covers
  // the per-object override independently of the FOM's initial default.
  publisher.rtiAmbassador().changeDefaultAttributeOrderType(
      publisherClass,
      publisherAttributes,
      rti::RECEIVE);
  auto const receiveObject = publisher.rtiAmbassador().registerObjectInstance(
      publisherClass);
  require(receiveObject.isValid(), "Receive-order registration returned an invalid object");
  waitFor(
      receiver,
      [&] { return receiver.recorder().hasDiscovery(receiveObject); },
      options,
      "Receive-order object discovery");

  publisher.rtiAmbassador().changeDefaultAttributeOrderType(
      publisherClass,
      publisherAttributes,
      rti::TIMESTAMP);
  auto const defaultTimestampObject =
      publisher.rtiAmbassador().registerObjectInstance(publisherClass);
  require(
      defaultTimestampObject.isValid(),
      "Timestamp-order default registration returned an invalid object");
  waitFor(
      receiver,
      [&] { return receiver.recorder().hasDiscovery(defaultTimestampObject); },
      options,
      "Timestamp-order default object discovery");

  publisher.rtiAmbassador().changeDefaultAttributeOrderType(
      publisherClass,
      publisherAttributes,
      rti::RECEIVE);
  auto const explicitTimestampObject =
      publisher.rtiAmbassador().registerObjectInstance(publisherClass);
  require(
      explicitTimestampObject.isValid(),
      "Explicit timestamp-order registration returned an invalid object");
  waitFor(
      receiver,
      [&] { return receiver.recorder().hasDiscovery(explicitTimestampObject); },
      options,
      "Explicit timestamp-order object discovery");
  publisher.rtiAmbassador().changeAttributeOrderType(
      explicitTimestampObject,
      publisherAttributes,
      rti::TIMESTAMP);

  auto publisherTime = makeTimeContext(publisher);
  auto receiverTime = makeTimeContext(receiver);
  require(
      publisherTime.factory->getName() == receiverTime.factory->getName(),
      "Order-type control members selected different logical-time factories");
  auto const lookaheadTime = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      5U);
  auto lookahead = publisherTime.factory->makeZero();
  require(lookahead != nullptr, "Order-type control could not allocate lookahead");
  lookahead->setToDifference(*lookaheadTime, *publisherTime.initial);
  receiver.rtiAmbassador().enableTimeConstrained();
  waitFor(
      receiver,
      [&] { return receiver.recorder().timeConstrainedEnabled().size() >= 1U; },
      options,
      "Order-type control time-constrained callback");
  receiver.rtiAmbassador().enableAsynchronousDelivery();
  publisher.rtiAmbassador().enableTimeRegulation(*lookahead);
  waitFor(
      publisher,
      [&] { return publisher.recorder().timeRegulationEnabled().size() >= 1U; },
      options,
      "Order-type control time-regulation callback");

  auto const timestamp = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      6U);
  std::vector<std::uint8_t> const receiveBytes{0x11U};
  std::vector<std::uint8_t> const defaultTimestampBytes{0x22U};
  std::vector<std::uint8_t> const explicitTimestampBytes{0x33U};
  std::vector<std::uint8_t> const attributeTagBytes{0x4FU, 0x52, 0x44};
  rti::AttributeHandleValueMap receiveValues;
  receiveValues.emplace(
      publisherAttribute,
      rti::VariableLengthData(receiveBytes.data(), receiveBytes.size()));
  rti::AttributeHandleValueMap defaultTimestampValues;
  defaultTimestampValues.emplace(
      publisherAttribute,
      rti::VariableLengthData(
          defaultTimestampBytes.data(),
          defaultTimestampBytes.size()));
  rti::AttributeHandleValueMap explicitTimestampValues;
  explicitTimestampValues.emplace(
      publisherAttribute,
      rti::VariableLengthData(
          explicitTimestampBytes.data(),
          explicitTimestampBytes.size()));
  rti::VariableLengthData attributeTag(
      attributeTagBytes.data(),
      attributeTagBytes.size());

  publisher.rtiAmbassador().updateAttributeValues(
      receiveObject,
      receiveValues,
      attributeTag);
  auto const defaultTimestampRetraction =
      publisher.rtiAmbassador().updateAttributeValues(
          defaultTimestampObject,
          defaultTimestampValues,
          attributeTag,
          *timestamp);
  auto const explicitTimestampRetraction =
      publisher.rtiAmbassador().updateAttributeValues(
          explicitTimestampObject,
          explicitTimestampValues,
          attributeTag,
          *timestamp);
  require(
      defaultTimestampRetraction.isValid() &&
          explicitTimestampRetraction.isValid(),
      "Order-type control returned the wrong retraction validity");

  auto const receiverTarget = timeAfter(
      *receiverTime.factory,
      *receiverTime.initial,
      *receiverTime.epsilon,
      6U);
  auto const publisherTarget = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      6U);
  receiver.rtiAmbassador().timeAdvanceRequest(*receiverTarget);
  publisher.rtiAmbassador().timeAdvanceRequest(*publisherTarget);
  waitForSessions(
      {&publisher, &receiver},
      [&] { return receiver.recorder().timedReflections().size() >= 2U; },
      options,
      "Order-type control timestamped attribute deliveries");
  waitForSessions(
      {&publisher, &receiver},
      [&] { return receiver.recorder().reflections().size() >= 1U; },
      options,
      "Order-type control ordinary attribute delivery");
  waitFor(
      publisher,
      [&] { return publisher.recorder().timeAdvanceGrants().size() >= 1U; },
      options,
      "Order-type control publisher time advance grant");
  require(
      receiver.recorder().reflections().size() == 1U &&
          receiver.recorder().timedReflections().size() == 2U,
      "Order-type control delivered the wrong ordinary/timestamped split");

  auto const ordinaryReflection = receiver.recorder().reflections().front();
  require(
      ordinaryReflection.object == receiveObject &&
          ordinaryReflection.values.size() == 1U &&
          ordinaryReflection.values.count(receiverAttribute) == 1U &&
          ordinaryReflection.tag == attributeTagBytes,
      "Receive-order attribute reflection did not preserve the object or payload");
  auto const timedReflections = receiver.recorder().timedReflections();
  for (auto const& reflection : timedReflections) {
    require(
        (reflection.object == defaultTimestampObject ||
         reflection.object == explicitTimestampObject) &&
            reflection.values.size() == 1U &&
            reflection.values.count(receiverAttribute) == 1U &&
            reflection.tag == attributeTagBytes &&
            reflection.time == encodeTime(*timestamp) &&
            reflection.sentOrder == rti::TIMESTAMP &&
            reflection.receivedOrder == rti::TIMESTAMP &&
            reflection.retractionPresent && reflection.regions == std::nullopt,
        "Timestamp-order attribute reflection returned the wrong metadata");
  }
  require(
      timedReflections.at(0).object != timedReflections.at(1).object,
      "Order-type control collapsed the two timestamped object instances");
  auto const findTimedRetraction = [&](rti::ObjectInstanceHandle const& object) {
    for (auto const& reflection : timedReflections) {
      if (reflection.object == object) {
        return reflection.retraction;
      }
    }
    return std::vector<std::uint8_t>{};
  };
  require(
      findTimedRetraction(defaultTimestampObject) ==
          copyBytes(defaultTimestampRetraction.encode()) &&
          findTimedRetraction(explicitTimestampObject) ==
              copyBytes(explicitTimestampRetraction.encode()),
      "Timestamp-order attribute reflection returned the wrong retraction handle");

  publisher.rtiAmbassador().changeInteractionOrderType(
      publisherInteraction,
      rti::RECEIVE);
  std::vector<std::uint8_t> const parameterBytes{0x49U, 0x4EU, 0x54};
  std::vector<std::uint8_t> const interactionTagBytes{0x49U, 0x4EU, 0x54};
  rti::ParameterHandleValueMap parameters;
  parameters.emplace(
      publisherParameter,
      rti::VariableLengthData(parameterBytes.data(), parameterBytes.size()));
  rti::VariableLengthData interactionTag(
      interactionTagBytes.data(),
      interactionTagBytes.size());
  auto const interactionTime = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      12U);
  auto const interactionRetraction = publisher.rtiAmbassador().sendInteraction(
      publisherInteraction,
      parameters,
      interactionTag,
      *interactionTime);
  require(
      !interactionRetraction.isValid(),
      "Receive-order interaction unexpectedly returned a retraction handle");
  waitFor(
      receiver,
      [&] { return receiver.recorder().timedInteractions().size() >= 1U; },
      options,
      "Order-type control receive-order interaction");
  require(
      receiver.recorder().timedInteractions().size() == 1U &&
          receiver.recorder().interactions().empty(),
      "Receive-order interaction used the wrong callback overload");
  auto const interaction = receiver.recorder().timedInteractions().front();
  require(
      interaction.interaction == receiverInteraction &&
          interaction.parameters.size() == 1U &&
          interaction.parameters.count(receiverParameter) == 1U &&
          interaction.parameters.at(receiverParameter).size() == parameterBytes.size() &&
          copyBytes(interaction.parameters.at(receiverParameter)) == parameterBytes &&
          interaction.tag == interactionTagBytes &&
          interaction.time == encodeTime(*interactionTime) &&
          interaction.sentOrder == rti::RECEIVE &&
          interaction.receivedOrder == rti::RECEIVE &&
          !interaction.retractionPresent &&
          interaction.regions == std::nullopt,
      "Receive-order interaction did not preserve the adapter FOM payload");

  receiver.rtiAmbassador().disableTimeConstrained();
  publisher.rtiAmbassador().disableTimeRegulation();
  receiver.rtiAmbassador().unsubscribeInteractionClass(receiverInteraction);
  publisher.rtiAmbassador().unpublishInteractionClass(publisherInteraction);
  receiver.rtiAmbassador().unsubscribeObjectClassAttributes(
      receiverClass,
      receiverAttributes);
  publisher.rtiAmbassador().unpublishObjectClassAttributes(
      publisherClass,
      publisherAttributes);
  receiver.resign(rti::NO_ACTION);
  publisher.resign(rti::CANCEL_THEN_DELETE_THEN_DIVEST);
  publisher.rtiAmbassador().destroyFederationExecution(federation);
  receiver.disconnect();
  publisher.disconnect();
}

}  // namespace hla_rti_cpp_tck::detail
