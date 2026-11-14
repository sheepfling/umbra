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

namespace {

void runInheritedAttributeDefaultScope(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.knownClassFom.empty() &&
          !options.logicalTimeImplementationName.empty(),
      "Inherited attribute-order scope requires the adapter known-class FOM and logical time");
  require(
      !options.knownClassObjectClassName.empty() &&
          !options.knownClassDerivedObjectClassName.empty() &&
          !options.knownClassKnownAttributeName.empty(),
      "Inherited attribute-order scope requires the base/derived object fixture names");

  Session publisher(options, model, "owner");
  Session peer(options, model, "member");
  Session receiver(options, model, "member");
  auto const federation = federationName(
      options,
      "inherited-attribute-default-order-scope");
  publisher.connect();
  peer.connect();
  receiver.connect();
  publisher.rtiAmbassador().createFederationExecution(
      federation,
      options.knownClassFom.wstring(),
      options.logicalTimeImplementationName);
  publisher.join(
      options.ownerFederateName + L"-inherited-order-publisher",
      options.federateType,
      federation);
  peer.join(
      options.memberFederateName + L"-inherited-order-peer",
      options.federateType,
      federation);
  receiver.join(
      options.memberFederateName + L"-inherited-order-receiver",
      options.federateType,
      federation);

  auto const publisherBaseClass = publisher.rtiAmbassador().getObjectClassHandle(
      options.knownClassObjectClassName);
  auto const publisherDerivedClass = publisher.rtiAmbassador().getObjectClassHandle(
      options.knownClassDerivedObjectClassName);
  auto const peerBaseClass = peer.rtiAmbassador().getObjectClassHandle(
      options.knownClassObjectClassName);
  auto const receiverBaseClass = receiver.rtiAmbassador().getObjectClassHandle(
      options.knownClassObjectClassName);
  auto const publisherBaseAttribute = publisher.rtiAmbassador().getAttributeHandle(
      publisherBaseClass,
      options.knownClassKnownAttributeName);
  auto const publisherDerivedAttribute = publisher.rtiAmbassador().getAttributeHandle(
      publisherDerivedClass,
      options.knownClassKnownAttributeName);
  auto const peerBaseAttribute = peer.rtiAmbassador().getAttributeHandle(
      peerBaseClass,
      options.knownClassKnownAttributeName);
  auto const receiverBaseAttribute = receiver.rtiAmbassador().getAttributeHandle(
      receiverBaseClass,
      options.knownClassKnownAttributeName);
  require(
      publisherBaseClass.isValid() && publisherDerivedClass.isValid() &&
          peerBaseClass.isValid() && receiverBaseClass.isValid() &&
          publisherBaseAttribute.isValid() && publisherDerivedAttribute.isValid() &&
          peerBaseAttribute.isValid() && receiverBaseAttribute.isValid() &&
          publisherBaseAttribute == publisherDerivedAttribute &&
          publisherBaseAttribute == peerBaseAttribute &&
          publisherBaseAttribute == receiverBaseAttribute,
      "Inherited attribute-order fixture returned inconsistent class or attribute handles");

  rti::AttributeHandleSet const publisherBaseAttributes{publisherBaseAttribute};
  rti::AttributeHandleSet const publisherDerivedAttributes{publisherDerivedAttribute};
  rti::AttributeHandleSet const peerBaseAttributes{peerBaseAttribute};
  rti::AttributeHandleSet const receiverBaseAttributes{receiverBaseAttribute};
  publisher.rtiAmbassador().publishObjectClassAttributes(
      publisherBaseClass,
      publisherBaseAttributes);
  publisher.rtiAmbassador().publishObjectClassAttributes(
      publisherDerivedClass,
      publisherDerivedAttributes);
  peer.rtiAmbassador().publishObjectClassAttributes(
      peerBaseClass,
      peerBaseAttributes);
  receiver.rtiAmbassador().subscribeObjectClassAttributes(
      receiverBaseClass,
      receiverBaseAttributes,
      true);

  auto publisherTime = makeTimeContext(publisher);
  auto peerTime = makeTimeContext(peer);
  require(
      publisherTime.factory->getName() == peerTime.factory->getName(),
      "Inherited attribute-order federates selected different logical-time factories");
  publisher.rtiAmbassador().enableTimeRegulation(*publisherTime.epsilon);
  waitFor(
      publisher,
      [&] { return publisher.recorder().timeRegulationEnabled().size() == 1U; },
      options,
      "Inherited attribute-order publisher time-regulation callback");
  auto const existingReceiveObject =
      publisher.rtiAmbassador().registerObjectInstance(publisherBaseClass);
  require(
      existingReceiveObject.isValid(),
      "Inherited attribute-order baseline registration returned an invalid object");
  waitFor(
      receiver,
      [&] { return receiver.recorder().hasDiscovery(existingReceiveObject); },
      options,
      "Inherited attribute-order baseline discovery");

  auto const sendAndObserve = [&](Session& producer,
                                  TimeContext const& producerTime,
                                  rti::ObjectInstanceHandle const& object,
                                  rti::AttributeHandle const& attribute,
                                  std::size_t timeStep,
                                  std::uint8_t value,
                                  std::vector<std::uint8_t> const& tagBytes,
                                  bool timestampOrderExpected,
                                  std::string const& description) {
    auto const updateTime = timeAfter(
        *producerTime.factory,
        *producerTime.initial,
        *producerTime.epsilon,
        timeStep);
    rti::AttributeHandleValueMap values;
    values.emplace(
        attribute,
        rti::VariableLengthData(&value, sizeof(value)));
    rti::VariableLengthData const tag(tagBytes.data(), tagBytes.size());
    auto const ordinaryBefore = receiver.recorder().reflections().size();
    auto const timestampedBefore = receiver.recorder().timedReflections().size();
    bool retractionPresent = false;
    if (timestampOrderExpected) {
      auto const retraction = producer.rtiAmbassador().updateAttributeValues(
          object,
          values,
          tag,
          *updateTime);
      retractionPresent = retraction.isValid();
      require(
          retractionPresent,
          description + " did not return a TSO retraction handle");
    } else {
      producer.rtiAmbassador().updateAttributeValues(object, values, tag);
    }
    waitFor(
        receiver,
        [&] {
          return receiver.recorder().reflections().size() > ordinaryBefore ||
              receiver.recorder().timedReflections().size() > timestampedBefore;
        },
        options,
        description + " reflection");

    auto const ordinaryRecords = receiver.recorder().reflections();
    auto const timestampedRecords = receiver.recorder().timedReflections();
    auto const ordinary = std::find_if(
        ordinaryRecords.begin(),
        ordinaryRecords.end(),
        [&](auto const& reflection) {
          return reflection.object == object && reflection.tag == tagBytes;
        });
    auto const timestamped = std::find_if(
        timestampedRecords.begin(),
        timestampedRecords.end(),
        [&](auto const& reflection) {
          return reflection.object == object && reflection.tag == tagBytes;
        });
    if (timestampOrderExpected) {
      require(
          ordinary == ordinaryRecords.end() &&
              timestamped != timestampedRecords.end(),
          description + " did not use the timestamped reflection callback");
      require(
          timestamped->values.size() == 1U &&
              timestamped->values.count(receiverBaseAttribute) == 1U &&
              copyBytes(timestamped->values.at(receiverBaseAttribute)) ==
                  std::vector<std::uint8_t>{value},
          description + " reflection did not carry the expected attribute value");
      require(
          timestamped->tag == tagBytes &&
              timestamped->time == encodeTime(*updateTime) &&
              timestamped->producer == producer.federateHandle() &&
              timestamped->sentOrder == rti::TIMESTAMP &&
              timestamped->receivedOrder == rti::RECEIVE &&
              timestamped->retractionPresent &&
              timestamped->regions == std::nullopt,
          description + " timestamped reflection metadata was incorrect");
    } else {
      require(
          ordinary != ordinaryRecords.end() &&
              timestamped == timestampedRecords.end(),
          description + " did not use the receive-order reflection callback");
      require(
          ordinary->values.size() == 1U &&
              ordinary->values.count(receiverBaseAttribute) == 1U &&
              copyBytes(ordinary->values.at(receiverBaseAttribute)) ==
                  std::vector<std::uint8_t>{value} &&
              ordinary->tag == tagBytes &&
              !retractionPresent,
          description + " receive-order reflection metadata was incorrect");
    }
  };

  // The adapter known-class fixture declares Name with FDD default RECEIVE.
  // An existing instance remains RECEIVE when the inherited default is
  // changed through the derived class handle, while a new base instance is TSO.
  auto const initialTag = std::vector<std::uint8_t>{0xA1U};
  sendAndObserve(
      publisher,
      publisherTime,
      existingReceiveObject,
      publisherBaseAttribute,
      5U,
      0x11U,
      initialTag,
      false,
      "Inherited attribute-order FDD-default baseline");
  publisher.rtiAmbassador().changeDefaultAttributeOrderType(
      publisherDerivedClass,
      publisherDerivedAttributes,
      rti::TIMESTAMP);
  auto const futureBaseTimestampObject =
      publisher.rtiAmbassador().registerObjectInstance(publisherBaseClass);
  require(
      futureBaseTimestampObject.isValid(),
      "Derived-handle order-default change returned an invalid base instance");
  waitFor(
      receiver,
      [&] { return receiver.recorder().hasDiscovery(futureBaseTimestampObject); },
      options,
      "Inherited attribute-order future base-instance discovery");
  sendAndObserve(
      publisher,
      publisherTime,
      futureBaseTimestampObject,
      publisherBaseAttribute,
      6U,
      0x12U,
      {0xA2U},
      true,
      "Derived-handle default change on future base instance");
  sendAndObserve(
      publisher,
      publisherTime,
      existingReceiveObject,
      publisherBaseAttribute,
      7U,
      0x13U,
      {0xA3U},
      false,
      "Existing base instance after derived-handle default change");

  auto const peerReceiveObject =
      peer.rtiAmbassador().registerObjectInstance(peerBaseClass);
  require(
      peerReceiveObject.isValid(),
      "Unmodified federate order-default registration returned an invalid object");
  waitFor(
      receiver,
      [&] { return receiver.recorder().hasDiscovery(peerReceiveObject); },
      options,
      "Inherited attribute-order peer-object discovery");
  sendAndObserve(
      peer,
      peerTime,
      peerReceiveObject,
      peerBaseAttribute,
      8U,
      0x14U,
      {0xA4U},
      false,
      "Other federate default after publisher change");

  // Reverse the class-handle direction: a base-handle change controls a
  // future derived instance, while the already timestamp-ordered base instance
  // keeps its prior order.
  publisher.rtiAmbassador().changeDefaultAttributeOrderType(
      publisherBaseClass,
      publisherBaseAttributes,
      rti::RECEIVE);
  auto const futureDerivedReceiveObject =
      publisher.rtiAmbassador().registerObjectInstance(publisherDerivedClass);
  require(
      futureDerivedReceiveObject.isValid(),
      "Base-handle order-default change returned an invalid derived instance");
  waitFor(
      receiver,
      [&] { return receiver.recorder().hasDiscovery(futureDerivedReceiveObject); },
      options,
      "Inherited attribute-order future derived-instance discovery");
  sendAndObserve(
      publisher,
      publisherTime,
      futureDerivedReceiveObject,
      publisherDerivedAttribute,
      9U,
      0x15U,
      {0xA5U},
      false,
      "Base-handle default change on future derived instance");
  sendAndObserve(
      publisher,
      publisherTime,
      futureBaseTimestampObject,
      publisherBaseAttribute,
      10U,
      0x16U,
      {0xA6U},
      true,
      "Existing timestamp instance after base-handle default change");

  receiver.rtiAmbassador().unsubscribeObjectClassAttributes(
      receiverBaseClass,
      receiverBaseAttributes);
  receiver.resign(rti::NO_ACTION);
  peer.resign(rti::CANCEL_THEN_DELETE_THEN_DIVEST);
  publisher.resign(rti::CANCEL_THEN_DELETE_THEN_DIVEST);
  publisher.rtiAmbassador().destroyFederationExecution(federation);
  receiver.disconnect();
  peer.disconnect();
  publisher.disconnect();
}

void runFddAttributeOrderDefaultInitialization(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.knownClassFom.empty() &&
          !options.logicalTimeImplementationName.empty(),
      "FDD order-default initialization requires an adapter-supplied FOM and logical time");
  require(
      !options.knownClassObjectClassName.empty() &&
          !options.knownClassKnownAttributeName.empty(),
      "FDD order-default initialization requires an object class and attribute");

  Session initialFederate(options, model, "owner");
  Session observer(options, model, "member");
  Session lateFederate(options, model, "member");
  auto const federation = federationName(
      options,
      "fdd-attribute-order-default-initialization");
  initialFederate.connect();
  observer.connect();
  lateFederate.connect();
  initialFederate.rtiAmbassador().createFederationExecution(
      federation,
      options.knownClassFom.wstring(),
      options.logicalTimeImplementationName);
  initialFederate.join(
      options.ownerFederateName + L"-fdd-order-initial",
      options.federateType,
      federation);
  observer.join(
      options.memberFederateName + L"-fdd-order-observer",
      options.federateType,
      federation);

  auto const initialClass = initialFederate.rtiAmbassador().getObjectClassHandle(
      options.knownClassObjectClassName);
  auto const initialAttribute = initialFederate.rtiAmbassador().getAttributeHandle(
      initialClass,
      options.knownClassKnownAttributeName);
  auto const observerClass = observer.rtiAmbassador().getObjectClassHandle(
      options.knownClassObjectClassName);
  auto const observerAttribute = observer.rtiAmbassador().getAttributeHandle(
      observerClass,
      options.knownClassKnownAttributeName);
  require(
      initialClass.isValid() && initialAttribute.isValid() &&
          observerClass.isValid() && observerAttribute.isValid() &&
          initialAttribute == observerAttribute,
      "FDD order-default fixture returned inconsistent class or attribute handles");
  rti::AttributeHandleSet const initialAttributes{initialAttribute};
  rti::AttributeHandleSet const observerAttributes{observerAttribute};
  initialFederate.rtiAmbassador().publishObjectClassAttributes(
      initialClass,
      initialAttributes);
  observer.rtiAmbassador().subscribeObjectClassAttributes(
      observerClass,
      observerAttributes,
      true);

  auto const initialTime = makeTimeContext(initialFederate);
  initialFederate.rtiAmbassador().enableTimeRegulation(*initialTime.epsilon);
  waitFor(
      initialFederate,
      [&] { return initialFederate.recorder().timeRegulationEnabled().size() == 1U; },
      options,
      "FDD order-default initial federate time-regulation callback");

  auto const sendTimestampedAndObserve = [&](Session& producer,
                                              TimeContext const& producerTime,
                                              rti::ObjectInstanceHandle const& object,
                                              rti::AttributeHandle const& attribute,
                                              std::size_t timeStep,
                                              std::uint8_t value,
                                              std::vector<std::uint8_t> const& tagBytes,
                                              rti::OrderType expectedSentOrder,
                                              std::string const& description) {
    auto const updateTime = timeAfter(
        *producerTime.factory,
        *producerTime.initial,
        *producerTime.epsilon,
        timeStep);
    rti::AttributeHandleValueMap values;
    values.emplace(
        attribute,
        rti::VariableLengthData(&value, sizeof(value)));
    rti::VariableLengthData const tag(tagBytes.data(), tagBytes.size());
    auto const ordinaryBefore = observer.recorder().reflections().size();
    auto const timestampedBefore = observer.recorder().timedReflections().size();
    auto const retraction = producer.rtiAmbassador().updateAttributeValues(
        object,
        values,
        tag,
        *updateTime);
    require(
        retraction.isValid() == (expectedSentOrder == rti::TIMESTAMP),
        description + " returned retraction state inconsistent with the FDD/default order");
    waitFor(
        observer,
        [&] {
          return observer.recorder().reflections().size() > ordinaryBefore ||
              observer.recorder().timedReflections().size() > timestampedBefore;
        },
        options,
        description + " reflection");

    auto const ordinaryRecords = observer.recorder().reflections();
    auto const timestampedRecords = observer.recorder().timedReflections();
    auto const ordinary = std::find_if(
        ordinaryRecords.begin(),
        ordinaryRecords.end(),
        [&](auto const& reflection) {
          return reflection.object == object && reflection.tag == tagBytes;
        });
    auto const timestamped = std::find_if(
        timestampedRecords.begin(),
        timestampedRecords.end(),
        [&](auto const& reflection) {
          return reflection.object == object && reflection.tag == tagBytes;
        });
    require(
        ordinary == ordinaryRecords.end() &&
            timestamped != timestampedRecords.end(),
        description + " did not preserve the timestamped-update callback");
    require(
        timestamped->values.size() == 1U &&
            timestamped->values.count(observerAttribute) == 1U &&
            copyBytes(timestamped->values.at(observerAttribute)) ==
                std::vector<std::uint8_t>{value} &&
            timestamped->tag == tagBytes &&
            timestamped->time == encodeTime(*updateTime) &&
            timestamped->producer == producer.federateHandle() &&
            timestamped->sentOrder == expectedSentOrder &&
            timestamped->receivedOrder == rti::RECEIVE &&
            timestamped->retractionPresent ==
                (expectedSentOrder == rti::TIMESTAMP) &&
            timestamped->regions == std::nullopt,
        description + " reflection did not preserve order, value, or metadata");
  };

  auto const initialObject =
      initialFederate.rtiAmbassador().registerObjectInstance(initialClass);
  require(
      initialObject.isValid(),
      "Initial federate could not register its FDD-default object");
  waitFor(
      observer,
      [&] { return observer.recorder().hasDiscovery(initialObject); },
      options,
      "Initial federate FDD-default object discovery");
  sendTimestampedAndObserve(
      initialFederate,
      initialTime,
      initialObject,
      initialAttribute,
      5U,
      0x51U,
      {0xA1U},
      rti::RECEIVE,
      "Initial federate FDD RECEIVE default");

  initialFederate.rtiAmbassador().changeDefaultAttributeOrderType(
      initialClass,
      initialAttributes,
      rti::TIMESTAMP);
  auto const changedDefaultObject =
      initialFederate.rtiAmbassador().registerObjectInstance(initialClass);
  require(
      changedDefaultObject.isValid(),
      "Initial federate could not register after changing its class default");
  waitFor(
      observer,
      [&] { return observer.recorder().hasDiscovery(changedDefaultObject); },
      options,
      "Changed-default timestamp object discovery");
  sendTimestampedAndObserve(
      initialFederate,
      initialTime,
      changedDefaultObject,
      initialAttribute,
      6U,
      0x52U,
      {0xA2U},
      rti::TIMESTAMP,
      "Changed federate TIMESTAMP default positive control");

  lateFederate.join(
      options.memberFederateName + L"-fdd-order-late-joiner",
      options.federateType,
      federation);
  auto const lateClass = lateFederate.rtiAmbassador().getObjectClassHandle(
      options.knownClassObjectClassName);
  auto const lateAttribute = lateFederate.rtiAmbassador().getAttributeHandle(
      lateClass,
      options.knownClassKnownAttributeName);
  require(
      lateClass.isValid() && lateAttribute.isValid() &&
          lateAttribute == initialAttribute,
      "Late federate resolved an inconsistent FDD class-attribute identity");
  rti::AttributeHandleSet const lateAttributes{lateAttribute};
  lateFederate.rtiAmbassador().publishObjectClassAttributes(
      lateClass,
      lateAttributes);
  auto const lateTime = makeTimeContext(lateFederate);
  require(
      initialTime.factory->getName() == lateTime.factory->getName(),
      "Late federate selected a different logical-time factory");
  lateFederate.rtiAmbassador().enableTimeRegulation(*lateTime.epsilon);
  waitFor(
      lateFederate,
      [&] { return lateFederate.recorder().timeRegulationEnabled().size() == 1U; },
      options,
      "Late federate FDD-default time-regulation callback");
  auto const lateObject =
      lateFederate.rtiAmbassador().registerObjectInstance(lateClass);
  require(
      lateObject.isValid(),
      "Late federate could not register its FDD-default object");
  waitFor(
      observer,
      [&] { return observer.recorder().hasDiscovery(lateObject); },
      options,
      "Late federate FDD-default object discovery");
  sendTimestampedAndObserve(
      lateFederate,
      lateTime,
      lateObject,
      lateAttribute,
      7U,
      0x53U,
      {0xA3U},
      rti::RECEIVE,
      "Late federate retained the FDD RECEIVE default after another federate changed its default");

  observer.rtiAmbassador().unsubscribeObjectClassAttributes(
      observerClass,
      observerAttributes);
  observer.resign(rti::NO_ACTION);
  lateFederate.resign(rti::CANCEL_THEN_DELETE_THEN_DIVEST);
  initialFederate.resign(rti::CANCEL_THEN_DELETE_THEN_DIVEST);
  initialFederate.rtiAmbassador().destroyFederationExecution(federation);
  observer.disconnect();
  lateFederate.disconnect();
  initialFederate.disconnect();
}

void runDefaultOrderAttributeClassHandleValidation(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.ddmFom.empty() && !options.logicalTimeImplementationName.empty(),
      "Default-order class-handle validation requires an adapter FOM and logical time");
  require(
      !options.knownClassObjectClassName.empty() &&
          !options.knownClassDerivedObjectClassName.empty() &&
          !options.knownClassDerivedAttributeName.empty(),
      "Default-order class-handle validation requires a derived-only attribute fixture");

  Session subject(options, model, "owner");
  auto const federation = federationName(
      options,
      "default-order-attribute-class-handle-validation");
  subject.connect();
  subject.rtiAmbassador().createFederationExecution(
      federation,
      options.ddmFom.wstring(),
      options.logicalTimeImplementationName);
  subject.join(
      options.ownerFederateName + L"-default-order-class-handle",
      options.federateType,
      federation);

  auto const baseClass = subject.rtiAmbassador().getObjectClassHandle(
      options.knownClassObjectClassName);
  auto const derivedClass = subject.rtiAmbassador().getObjectClassHandle(
      options.knownClassDerivedObjectClassName);
  require(
      baseClass.isValid() && derivedClass.isValid(),
      "Default-order class-handle fixture returned an invalid class handle");
  auto const derivedOnlyAttribute = subject.rtiAmbassador().getAttributeHandle(
      derivedClass,
      options.knownClassDerivedAttributeName);
  require(
      derivedOnlyAttribute.isValid(),
      "Default-order fixture did not resolve its derived-only attribute");
  rti::AttributeHandleSet const derivedOnlyAttributes{derivedOnlyAttribute};

  requireException(
      [&] {
        subject.rtiAmbassador().changeDefaultAttributeOrderType(
            baseClass,
            derivedOnlyAttributes,
            rti::TIMESTAMP);
      },
      L"AttributeNotDefined",
      "changing an order default for a derived-only attribute through its base class");
  subject.rtiAmbassador().changeDefaultAttributeOrderType(
      derivedClass,
      derivedOnlyAttributes,
      rti::TIMESTAMP);

  subject.resign(rti::CANCEL_THEN_DELETE_THEN_DIVEST);
  subject.rtiAmbassador().destroyFederationExecution(federation);
  subject.disconnect();
}

}  // namespace

void scenarioInheritedAttributeDefaultOrderScope(
    Options const& options,
    rti::CallbackModel model) {
  runInheritedAttributeDefaultScope(options, model);
}

void scenarioInheritedAttributeDefaultOrderScopeContract(
    Options const& options,
    rti::CallbackModel model) {
  runInheritedAttributeDefaultScope(options, model);
}

void scenarioFddAttributeOrderDefaultInitialization(
    Options const& options,
    rti::CallbackModel model) {
  runFddAttributeOrderDefaultInitialization(options, model);
}

void scenarioFddAttributeOrderDefaultInitializationContract(
    Options const& options,
    rti::CallbackModel model) {
  runFddAttributeOrderDefaultInitialization(options, model);
}

void scenarioDefaultOrderAttributeClassHandleValidation(
    Options const& options,
    rti::CallbackModel model) {
  runDefaultOrderAttributeClassHandleValidation(options, model);
}

void scenarioDefaultOrderAttributeClassHandleValidationContract(
    Options const& options,
    rti::CallbackModel model) {
  runDefaultOrderAttributeClassHandleValidation(options, model);
}

}  // namespace hla_rti_cpp_tck::detail
