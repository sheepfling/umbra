#include "tck_order_type_controls.hpp"
#include "tck_main_scenarios.hpp"

#include <algorithm>
#include <cstdint>
#include <optional>
#include <vector>

namespace hla_rti_cpp_tck::detail {

void scenarioOrderTypeControls(Options const& options, rti::CallbackModel model) {
  require(
      !options.logicalTimeImplementationName.empty(),
      "Order-type control test requires an adapter-supplied logical-time implementation");
  require(
      !options.ddmFom.empty() && !options.ddmDimensionNames.empty(),
      "Regional interaction order control requires the adapter-supplied DDM FOM and dimension");

  Session publisher(options, model, "owner");
  Session receiver(options, model, "member");
  Session scopedPublisher(options, model, "scoped-owner");
  auto const federation = federationName(options, "order-type-controls");
  publisher.connect();
  receiver.connect();
  scopedPublisher.connect();
  publisher.rtiAmbassador().createFederationExecution(
      federation,
      options.ddmFom.wstring(),
      options.logicalTimeImplementationName);
  publisher.join(options.ownerFederateName, options.federateType, federation);
  receiver.join(
      options.memberFederateName + L"-order-controls",
      options.federateType,
      federation);
  scopedPublisher.join(
      options.ownerFederateName + L"-order-controls-peer",
      options.federateType,
      federation);

  auto const publisherClass = publisher.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const receiverClass = receiver.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const scopedPublisherClass =
      scopedPublisher.rtiAmbassador().getObjectClassHandle(options.objectClassName);
  auto const publisherAttribute = publisher.rtiAmbassador().getAttributeHandle(
      publisherClass,
      options.attributeName);
  auto const receiverAttribute = receiver.rtiAmbassador().getAttributeHandle(
      receiverClass,
      options.attributeName);
  auto const scopedPublisherAttribute =
      scopedPublisher.rtiAmbassador().getAttributeHandle(
          scopedPublisherClass,
          options.attributeName);
  auto const publisherInteraction =
      publisher.rtiAmbassador().getInteractionClassHandle(options.interactionClassName);
  auto const receiverInteraction =
      receiver.rtiAmbassador().getInteractionClassHandle(options.interactionClassName);
  auto const scopedPublisherInteraction =
      scopedPublisher.rtiAmbassador().getInteractionClassHandle(options.interactionClassName);
  auto const publisherParameter = publisher.rtiAmbassador().getParameterHandle(
      publisherInteraction,
      options.parameterName);
  auto const receiverParameter = receiver.rtiAmbassador().getParameterHandle(
      receiverInteraction,
      options.parameterName);
  auto const scopedPublisherParameter =
      scopedPublisher.rtiAmbassador().getParameterHandle(
          scopedPublisherInteraction,
          options.parameterName);
  auto const publisherDimension = publisher.rtiAmbassador().getDimensionHandle(
      options.ddmDimensionNames.front());
  auto const receiverDimension = receiver.rtiAmbassador().getDimensionHandle(
      options.ddmDimensionNames.front());
  require(
      publisherClass.isValid() && receiverClass.isValid() &&
          scopedPublisherClass.isValid() &&
          publisherAttribute.isValid() && receiverAttribute.isValid() &&
          scopedPublisherAttribute.isValid() &&
          publisherInteraction.isValid() && receiverInteraction.isValid() &&
          scopedPublisherInteraction.isValid() && publisherParameter.isValid() &&
          receiverParameter.isValid() && scopedPublisherParameter.isValid() &&
          publisherDimension.isValid() && receiverDimension.isValid(),
      "Order-type control lookup returned an invalid adapter-supplied handle");

  rti::AttributeHandleSet const publisherAttributes{publisherAttribute};
  rti::AttributeHandleSet const receiverAttributes{receiverAttribute};
  rti::AttributeHandleSet const scopedPublisherAttributes{
      scopedPublisherAttribute};
  publisher.rtiAmbassador().publishObjectClassAttributes(
      publisherClass,
      publisherAttributes);
  scopedPublisher.rtiAmbassador().publishObjectClassAttributes(
      scopedPublisherClass,
      scopedPublisherAttributes);
  receiver.rtiAmbassador().subscribeObjectClassAttributes(
      receiverClass,
      receiverAttributes,
      true);
  scopedPublisher.rtiAmbassador().subscribeObjectClassAttributes(
      scopedPublisherClass,
      scopedPublisherAttributes,
      true);
  publisher.rtiAmbassador().publishInteractionClass(publisherInteraction);
  scopedPublisher.rtiAmbassador().publishInteractionClass(scopedPublisherInteraction);
  auto const publisherRegion = publisher.rtiAmbassador().createRegion(
      rti::DimensionHandleSet{publisherDimension});
  auto const receiverRegion = receiver.rtiAmbassador().createRegion(
      rti::DimensionHandleSet{receiverDimension});
  publisher.rtiAmbassador().setRangeBounds(
      publisherRegion,
      publisherDimension,
      rti::RangeBounds(0UL, 6UL));
  receiver.rtiAmbassador().setRangeBounds(
      receiverRegion,
      receiverDimension,
      rti::RangeBounds(5UL, 10UL));
  publisher.rtiAmbassador().commitRegionModifications(
      rti::RegionHandleSet{publisherRegion});
  receiver.rtiAmbassador().commitRegionModifications(
      rti::RegionHandleSet{receiverRegion});
  receiver.rtiAmbassador().subscribeInteractionClassWithRegions(
      receiverInteraction,
      rti::RegionHandleSet{receiverRegion});

  // Establish a known class default, then verify that a later default change
  // affects only later registrations.  The explicit instance change covers
  // the per-object override independently of the FOM's initial default.
  publisher.rtiAmbassador().changeDefaultAttributeOrderType(
      publisherClass,
      publisherAttributes,
      rti::RECEIVE);
  scopedPublisher.rtiAmbassador().changeDefaultAttributeOrderType(
      scopedPublisherClass,
      scopedPublisherAttributes,
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
  waitFor(
      scopedPublisher,
      [&] { return scopedPublisher.recorder().hasDiscovery(explicitTimestampObject); },
      options,
      "New-owner timestamp-order object discovery");
  publisher.rtiAmbassador().changeAttributeOrderType(
      explicitTimestampObject,
      publisherAttributes,
      rti::TIMESTAMP);

  auto publisherTime = makeTimeContext(publisher);
  auto receiverTime = makeTimeContext(receiver);
  auto scopedPublisherTime = makeTimeContext(scopedPublisher);
  require(
      publisherTime.factory->getName() == receiverTime.factory->getName() &&
          publisherTime.factory->getName() == scopedPublisherTime.factory->getName(),
      "Order-type control members selected different logical-time factories");
  auto const lookaheadTime = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      5U);
  auto lookahead = publisherTime.factory->makeZero();
  require(lookahead != nullptr, "Order-type control could not allocate lookahead");
  lookahead->setToDifference(*lookaheadTime, *publisherTime.initial);
  auto const scopedLookaheadTime = timeAfter(
      *scopedPublisherTime.factory,
      *scopedPublisherTime.initial,
      *scopedPublisherTime.epsilon,
      20U);
  auto scopedLookahead = scopedPublisherTime.factory->makeZero();
  require(
      scopedLookahead != nullptr,
      "Order-type control could not allocate scoped-publisher lookahead");
  scopedLookahead->setToDifference(
      *scopedLookaheadTime,
      *scopedPublisherTime.initial);
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
  scopedPublisher.rtiAmbassador().enableTimeRegulation(*scopedLookahead);
  waitFor(
      scopedPublisher,
      [&] { return scopedPublisher.recorder().timeRegulationEnabled().size() >= 1U; },
      options,
      "Order-type control scoped-publisher time-regulation callback");

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

  std::vector<std::uint8_t> const transferTagBytes{0x4FU, 0x57U, 0x4EU};
  rti::VariableLengthData const transferTag(
      transferTagBytes.data(),
      transferTagBytes.size());
  scopedPublisher.rtiAmbassador().attributeOwnershipAcquisition(
      explicitTimestampObject,
      scopedPublisherAttributes,
      transferTag);
  publisher.rtiAmbassador().negotiatedAttributeOwnershipDivestiture(
      explicitTimestampObject,
      publisherAttributes,
      transferTag);
  waitFor(
      publisher,
      [&] { return publisher.recorder().divestitureConfirmation().has_value(); },
      options,
      "Attribute-order divestiture-confirmation callback");
  auto const divestitureConfirmation =
      publisher.recorder().divestitureConfirmation();
  require(
      divestitureConfirmation->object == explicitTimestampObject &&
          divestitureConfirmation->attributes == publisherAttributes &&
          divestitureConfirmation->tag == transferTagBytes,
      "Attribute-order divestiture confirmation returned the wrong transfer");
  std::vector<std::uint8_t> const confirmationTagBytes{0x43U, 0x46U, 0x4DU};
  rti::VariableLengthData const confirmationTag(
      confirmationTagBytes.data(),
      confirmationTagBytes.size());
  publisher.rtiAmbassador().confirmDivestiture(
      explicitTimestampObject,
      publisherAttributes,
      confirmationTag);
  waitFor(
      scopedPublisher,
      [&] { return scopedPublisher.recorder().ownershipAcquisition().has_value(); },
      options,
      "Attribute-order ownership-acquisition callback");
  auto const ownershipAcquisition =
      scopedPublisher.recorder().ownershipAcquisition();
  require(
      ownershipAcquisition->object == explicitTimestampObject &&
          ownershipAcquisition->attributes == scopedPublisherAttributes &&
          ownershipAcquisition->tag == confirmationTagBytes &&
          !publisher.rtiAmbassador().isAttributeOwnedByFederate(
              explicitTimestampObject,
              publisherAttribute) &&
          scopedPublisher.rtiAmbassador().isAttributeOwnedByFederate(
              explicitTimestampObject,
              scopedPublisherAttribute),
      "Attribute-order ownership transfer did not establish the new owner");

  std::vector<std::uint8_t> const transferredAttributeBytes{0x4EU, 0x45U, 0x57U};
  rti::AttributeHandleValueMap transferredAttributeValues;
  transferredAttributeValues.emplace(
      scopedPublisherAttribute,
      rti::VariableLengthData(
          transferredAttributeBytes.data(),
          transferredAttributeBytes.size()));
  auto const transferredAttributeTime = timeAfter(
      *scopedPublisherTime.factory,
      *scopedPublisherTime.initial,
      *scopedPublisherTime.epsilon,
      32U);
  auto const transferredAttributeRetraction =
      scopedPublisher.rtiAmbassador().updateAttributeValues(
          explicitTimestampObject,
          transferredAttributeValues,
          confirmationTag,
          *transferredAttributeTime);
  require(
      !transferredAttributeRetraction.isValid(),
      "Transferred attribute retained the prior owner's TIMESTAMP preference");
  waitFor(
      receiver,
      [&] { return receiver.recorder().timedReflections().size() >= 3U; },
      options,
      "New-owner default RECEIVE-order timestamped attribute reflection");
  auto const orderResetReflections = receiver.recorder().timedReflections();
  auto const transferredAttributeReflection = std::find_if(
      orderResetReflections.begin(),
      orderResetReflections.end(),
      [&](auto const& candidate) {
        return candidate.object == explicitTimestampObject &&
            candidate.tag == confirmationTagBytes;
      });
  require(
      transferredAttributeReflection != orderResetReflections.end(),
      "New owner produced no timestamp-bearing reflection for the transferred object");
  require(
      transferredAttributeReflection->values.size() == 1U &&
          transferredAttributeReflection->values.count(receiverAttribute) == 1U &&
          copyBytes(transferredAttributeReflection->values.at(receiverAttribute)) ==
              transferredAttributeBytes,
      "New-owner transferred-attribute reflection returned the wrong payload");
  require(
          transferredAttributeReflection->time ==
              encodeTime(*transferredAttributeTime) &&
          transferredAttributeReflection->tag == confirmationTagBytes &&
          transferredAttributeReflection->sentOrder == rti::RECEIVE &&
          transferredAttributeReflection->receivedOrder == rti::RECEIVE &&
          !transferredAttributeReflection->retractionPresent &&
          transferredAttributeReflection->retraction.empty(),
      "New owner did not use its class-default RECEIVE order after acquisition");
  require(
      receiver.recorder().timedReflections().size() == 3U &&
          receiver.recorder().reflections().size() == 1U,
      "New-owner timestamped update used the wrong reflection callback family");

  std::vector<std::uint8_t> const parameterBytes{0x49U, 0x4EU, 0x54};
  std::vector<std::uint8_t> const fddDefaultParameterBytes{0x46U, 0x44U, 0x44U};
  std::vector<std::uint8_t> const interactionTagBytes{0x49U, 0x4EU, 0x54};
  rti::ParameterHandleValueMap parameters;
  parameters.emplace(
      publisherParameter,
      rti::VariableLengthData(parameterBytes.data(), parameterBytes.size()));
  rti::ParameterHandleValueMap fddDefaultParameters;
  fddDefaultParameters.emplace(
      publisherParameter,
      rti::VariableLengthData(
          fddDefaultParameterBytes.data(),
          fddDefaultParameterBytes.size()));
  rti::VariableLengthData interactionTag(
      interactionTagBytes.data(),
      interactionTagBytes.size());
  auto const fddDefaultTime = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      12U);
  auto const fddDefaultRetraction = publisher.rtiAmbassador().sendInteraction(
      publisherInteraction,
      fddDefaultParameters,
      interactionTag,
      *fddDefaultTime);
  require(
      !fddDefaultRetraction.isValid(),
      "FDD-initialized RECEIVE order unexpectedly returned a retraction handle");
  publisher.rtiAmbassador().changeInteractionOrderType(
      publisherInteraction,
      rti::TIMESTAMP);
  auto const changedTimestampTime = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      14U);
  auto const changedTimestampRetraction = publisher.rtiAmbassador().sendInteraction(
      publisherInteraction,
      parameters,
      interactionTag,
      *changedTimestampTime);
  require(
      changedTimestampRetraction.isValid(),
      "Changing the interaction preference to TIMESTAMP did not affect a later send");
  publisher.rtiAmbassador().retract(changedTimestampRetraction);
  scopedPublisher.rtiAmbassador().changeInteractionOrderType(
      scopedPublisherInteraction,
      rti::TIMESTAMP);
  publisher.rtiAmbassador().changeInteractionOrderType(
      publisherInteraction,
      rti::RECEIVE);
  auto const interactionTime = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      16U);
  auto const scopedInteractionTime = timeAfter(
      *scopedPublisherTime.factory,
      *scopedPublisherTime.initial,
      *scopedPublisherTime.epsilon,
      30U);
  auto const scopedInteractionRetraction =
      scopedPublisher.rtiAmbassador().sendInteraction(
          scopedPublisherInteraction,
          parameters,
          interactionTag,
          *scopedInteractionTime);
  require(
      scopedInteractionRetraction.isValid(),
      "Changing another publisher's interaction order altered this federate's timestamp preference");
  scopedPublisher.rtiAmbassador().retract(scopedInteractionRetraction);
  std::vector<std::uint8_t> const regionalParameterBytes{0x52U, 0x45U, 0x47U};
  rti::ParameterHandleValueMap regionalParameters;
  regionalParameters.emplace(
      publisherParameter,
      rti::VariableLengthData(
          regionalParameterBytes.data(),
          regionalParameterBytes.size()));
  auto const regionalInteractionRetraction =
      publisher.rtiAmbassador().sendInteractionWithRegions(
          publisherInteraction,
          regionalParameters,
          rti::RegionHandleSet{publisherRegion},
          interactionTag,
          *interactionTime);
  require(
      !regionalInteractionRetraction.isValid(),
      "Receive-order interaction preference returned a TSO handle for Send Interaction with Regions");
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
      [&] { return receiver.recorder().timedInteractions().size() >= 3U; },
      options,
      "Order-type control receive-order interactions");
  require(
      receiver.recorder().timedInteractions().size() == 3U &&
          receiver.recorder().interactions().empty(),
      "Receive-order interactions used the wrong callback overload");
  auto const interactions = receiver.recorder().timedInteractions();
  auto const fddDefaultInteraction = std::find_if(
      interactions.begin(),
      interactions.end(),
      [&](auto const& candidate) {
        return candidate.parameters.size() == 1U &&
            candidate.parameters.count(receiverParameter) == 1U &&
            copyBytes(candidate.parameters.at(receiverParameter)) ==
                fddDefaultParameterBytes;
      });
  auto const interaction = std::find_if(
      interactions.begin(),
      interactions.end(),
      [&](auto const& candidate) {
        return candidate.parameters.size() == 1U &&
            candidate.parameters.count(receiverParameter) == 1U &&
            copyBytes(candidate.parameters.at(receiverParameter)) == parameterBytes;
      });
  auto const regionalInteraction = std::find_if(
      interactions.begin(),
      interactions.end(),
      [&](auto const& candidate) {
        return candidate.parameters.size() == 1U &&
            candidate.parameters.count(receiverParameter) == 1U &&
            copyBytes(candidate.parameters.at(receiverParameter)) == regionalParameterBytes;
      });
  require(
      fddDefaultInteraction != interactions.end() &&
          interaction != interactions.end() && regionalInteraction != interactions.end(),
      "Order-type control did not deliver its FDD-default, ordinary, and regional interactions");
  auto const validateReceiveOrderInteraction = [&](auto const& received, auto const& time) {
    require(
        received->interaction == receiverInteraction &&
            received->parameters.size() == 1U &&
            received->parameters.count(receiverParameter) == 1U &&
            received->tag == interactionTagBytes &&
            received->time == encodeTime(*time) &&
            received->sentOrder == rti::RECEIVE &&
            received->receivedOrder == rti::RECEIVE &&
            !received->retractionPresent,
        "Receive-order interaction did not preserve its payload and order metadata");
  };
  validateReceiveOrderInteraction(fddDefaultInteraction, fddDefaultTime);
  validateReceiveOrderInteraction(interaction, interactionTime);
  validateReceiveOrderInteraction(regionalInteraction, interactionTime);
  require(
      fddDefaultInteraction->regions == std::nullopt,
      "FDD-default ordinary interaction unexpectedly reported regions");
  publisher.rtiAmbassador().deleteRegion(publisherRegion);
  receiver.rtiAmbassador().unsubscribeInteractionClassWithRegions(
      receiverInteraction,
      rti::RegionHandleSet{receiverRegion});
  receiver.rtiAmbassador().deleteRegion(receiverRegion);

  receiver.rtiAmbassador().disableTimeConstrained();
  publisher.rtiAmbassador().disableTimeRegulation();
  scopedPublisher.rtiAmbassador().disableTimeRegulation();
  publisher.rtiAmbassador().unpublishInteractionClass(publisherInteraction);
  scopedPublisher.rtiAmbassador().unpublishInteractionClass(scopedPublisherInteraction);
  receiver.rtiAmbassador().unsubscribeObjectClassAttributes(
      receiverClass,
      receiverAttributes);
  publisher.rtiAmbassador().unpublishObjectClassAttributes(
      publisherClass,
      publisherAttributes);
  receiver.resign(rti::NO_ACTION);
  publisher.resign(rti::CANCEL_THEN_DELETE_THEN_DIVEST);
  scopedPublisher.resign(rti::CANCEL_THEN_DELETE_THEN_DIVEST);
  publisher.rtiAmbassador().destroyFederationExecution(federation);
  receiver.disconnect();
  publisher.disconnect();
  scopedPublisher.disconnect();
}

}  // namespace hla_rti_cpp_tck::detail
