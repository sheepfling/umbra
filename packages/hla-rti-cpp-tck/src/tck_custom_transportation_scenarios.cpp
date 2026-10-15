#include "tck_custom_transportation_scenarios.hpp"
#include "tck_main_scenarios.hpp"

namespace hla_rti_cpp_tck::detail {

void scenarioCustomTransportationAttributeDelivery(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.modelFom.empty(),
      "Custom transportation attribute testing requires an adapter-supplied rich FOM");

  auto ordinary = options;
  ordinary.objectClassName = ordinary.customRegionalObjectClassName;
  ordinary.attributeName = ordinary.customRegionalAttributeName;
  ordinary.requireCustomTransportation = true;

  Session publisher(ordinary, model, "custom-transportation-attribute-publisher");
  Session subscriber(ordinary, model, "custom-transportation-attribute-subscriber");
  auto const federation = federationName(
      ordinary,
      "custom-transportation-attribute-delivery");
  connectAndJoin(publisher, subscriber, ordinary, federation, ordinary.modelFom);

  auto const publisherClass = publisher.rtiAmbassador().getObjectClassHandle(
      ordinary.objectClassName);
  auto const subscriberClass = subscriber.rtiAmbassador().getObjectClassHandle(
      ordinary.objectClassName);
  auto const publisherAttribute = publisher.rtiAmbassador().getAttributeHandle(
      publisherClass,
      ordinary.attributeName);
  auto const subscriberAttribute = subscriber.rtiAmbassador().getAttributeHandle(
      subscriberClass,
      ordinary.attributeName);
  require(
      publisherClass.isValid() && subscriberClass.isValid() &&
          publisherAttribute.isValid() && subscriberAttribute.isValid(),
      "custom transportation ordinary attribute lookup returned an invalid handle");
  require(
      publisher.rtiAmbassador().getObjectClassName(publisherClass) ==
              ordinary.objectClassName &&
          subscriber.rtiAmbassador().getObjectClassName(subscriberClass) ==
              ordinary.objectClassName &&
          publisher.rtiAmbassador().getAttributeName(
              publisherClass,
              publisherAttribute) == ordinary.attributeName &&
          subscriber.rtiAmbassador().getAttributeName(
              subscriberClass,
              subscriberAttribute) == ordinary.attributeName,
      "custom transportation ordinary attribute lookup did not round-trip names");

  auto const publisherTransportation = publisher.rtiAmbassador().getTransportationTypeHandle(
      ordinary.fomCustomTransportationName);
  auto const subscriberTransportation = subscriber.rtiAmbassador().getTransportationTypeHandle(
      ordinary.fomCustomTransportationName);
  require(
      publisherTransportation.isValid() && subscriberTransportation.isValid() &&
          publisher.rtiAmbassador().getTransportationTypeName(publisherTransportation) ==
              ordinary.fomCustomTransportationName &&
          subscriber.rtiAmbassador().getTransportationTypeName(subscriberTransportation) ==
              ordinary.fomCustomTransportationName,
      "custom transportation ordinary attribute lookup did not round-trip the transport name");

  rti::AttributeHandleSet const publisherAttributes{publisherAttribute};
  rti::AttributeHandleSet const subscriberAttributes{subscriberAttribute};
  publisher.rtiAmbassador().publishObjectClassAttributes(
      publisherClass,
      publisherAttributes);
  subscriber.rtiAmbassador().subscribeObjectClassAttributes(
      subscriberClass,
      subscriberAttributes,
      true,
      L"");

  auto const object = publisher.rtiAmbassador().registerObjectInstance(publisherClass);
  require(
      object.isValid(),
      "custom transportation ordinary attribute registration returned an invalid handle");
  auto const objectName = publisher.rtiAmbassador().getObjectInstanceName(object);
  require(
      !objectName.empty() &&
          publisher.rtiAmbassador().getObjectInstanceHandle(objectName) == object,
      "custom transportation ordinary attribute registration did not preserve object naming");
  waitFor(
      subscriber,
      [&] { return subscriber.recorder().hasDiscovery(object); },
      ordinary,
      "custom transportation ordinary attribute discovery");
  auto const discovery = subscriber.recorder().discovery();
  require(
      discovery.object == object &&
          discovery.objectClass == subscriberClass &&
          discovery.producer == publisher.federateHandle() &&
          !discovery.name.empty() &&
          subscriber.rtiAmbassador().getObjectInstanceHandle(discovery.name) == object &&
          subscriber.rtiAmbassador().getKnownObjectClassHandle(object) == subscriberClass,
      "custom transportation ordinary attribute discovery changed standard object metadata");

  std::vector<std::uint8_t> const valueBytes{0xA1U, 0xB2U, 0xC3U};
  std::vector<std::uint8_t> const tagBytes{0x54U, 0x41U, 0x47U};
  rti::AttributeHandleValueMap values;
  values.emplace(
      publisherAttribute,
      rti::VariableLengthData(valueBytes.data(), valueBytes.size()));
  rti::VariableLengthData const tag(tagBytes.data(), tagBytes.size());
  publisher.rtiAmbassador().updateAttributeValues(object, values, tag);
  waitFor(
      subscriber,
      [&] { return subscriber.recorder().reflection().present; },
      ordinary,
      "custom transportation ordinary attribute reflection");

  auto const reflection = subscriber.recorder().reflection();
  require(
      reflection.object == object &&
          reflection.values.size() == 1U &&
          reflection.values.count(subscriberAttribute) == 1U &&
          copyBytes(reflection.values.at(subscriberAttribute)) == valueBytes &&
          reflection.tag == tagBytes &&
          reflection.producer == publisher.federateHandle(),
      "custom transportation ordinary attribute reflection changed value or metadata");
  require(
      reflection.transportation.isValid() &&
          subscriber.rtiAmbassador().getTransportationTypeName(
              reflection.transportation) == ordinary.fomCustomTransportationName,
      "custom transportation ordinary attribute reflection changed transportation identity");

  subscriber.rtiAmbassador().unsubscribeObjectClassAttributes(
      subscriberClass,
      subscriberAttributes);
  publisher.rtiAmbassador().unpublishObjectClassAttributes(
      publisherClass,
      publisherAttributes);
  publisher.resign(rti::DELETE_OBJECTS);
  subscriber.resign(rti::NO_ACTION);
  publisher.rtiAmbassador().destroyFederationExecution(federation);
  subscriber.disconnect();
  publisher.disconnect();
}

void scenarioCustomTransportationAttributeDeliveryContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioCustomTransportationAttributeDelivery(options, model);
}

void scenarioCustomTransportationTimestampedAttributeDelivery(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.modelFom.empty(),
      "Custom transportation timestamped attribute testing requires an adapter-supplied rich FOM");
  require(
      !options.logicalTimeImplementationName.empty(),
      "Custom transportation timestamped attribute testing requires an adapter-supplied logical-time implementation");

  auto timestamped = options;
  timestamped.objectClassName = timestamped.customRegionalObjectClassName;
  timestamped.attributeName = timestamped.customRegionalAttributeName;
  timestamped.requireCustomTransportation = true;

  Session publisher(
      timestamped,
      model,
      "custom-transportation-timestamped-attribute-publisher");
  Session immediate(
      timestamped,
      model,
      "custom-transportation-timestamped-attribute-immediate");
  Session constrained(
      timestamped,
      model,
      "custom-transportation-timestamped-attribute-constrained");
  auto const federation = federationName(
      timestamped,
      "custom-transportation-timestamped-attribute-delivery");
  connectAndJoin(
      publisher,
      immediate,
      timestamped,
      federation,
      timestamped.modelFom);
  constrained.connect();
  constrained.join(
      timestamped.memberFederateName + L"-custom-transportation-timestamped-attribute-constrained",
      timestamped.federateType,
      federation);

  auto const publisherClass = publisher.rtiAmbassador().getObjectClassHandle(
      timestamped.objectClassName);
  auto const immediateClass = immediate.rtiAmbassador().getObjectClassHandle(
      timestamped.objectClassName);
  auto const constrainedClass = constrained.rtiAmbassador().getObjectClassHandle(
      timestamped.objectClassName);
  auto const publisherAttribute = publisher.rtiAmbassador().getAttributeHandle(
      publisherClass,
      timestamped.attributeName);
  auto const immediateAttribute = immediate.rtiAmbassador().getAttributeHandle(
      immediateClass,
      timestamped.attributeName);
  auto const constrainedAttribute = constrained.rtiAmbassador().getAttributeHandle(
      constrainedClass,
      timestamped.attributeName);
  require(
      publisherClass.isValid() && immediateClass.isValid() &&
          constrainedClass.isValid() && publisherAttribute.isValid() &&
          immediateAttribute.isValid() && constrainedAttribute.isValid(),
      "custom transportation timestamped attribute lookup returned an invalid handle");
  require(
      publisher.rtiAmbassador().getObjectClassName(publisherClass) ==
              timestamped.objectClassName &&
          immediate.rtiAmbassador().getObjectClassName(immediateClass) ==
              timestamped.objectClassName &&
          constrained.rtiAmbassador().getObjectClassName(constrainedClass) ==
              timestamped.objectClassName &&
          publisher.rtiAmbassador().getAttributeName(
              publisherClass,
              publisherAttribute) == timestamped.attributeName &&
          immediate.rtiAmbassador().getAttributeName(
              immediateClass,
              immediateAttribute) == timestamped.attributeName &&
          constrained.rtiAmbassador().getAttributeName(
              constrainedClass,
              constrainedAttribute) == timestamped.attributeName,
      "custom transportation timestamped attribute lookup did not round-trip names");

  auto const publisherTransportation = publisher.rtiAmbassador().getTransportationTypeHandle(
      timestamped.fomCustomTransportationName);
  auto const immediateTransportation = immediate.rtiAmbassador().getTransportationTypeHandle(
      timestamped.fomCustomTransportationName);
  auto const constrainedTransportation = constrained.rtiAmbassador().getTransportationTypeHandle(
      timestamped.fomCustomTransportationName);
  require(
      publisherTransportation.isValid() && immediateTransportation.isValid() &&
          constrainedTransportation.isValid() &&
          publisher.rtiAmbassador().getTransportationTypeName(publisherTransportation) ==
              timestamped.fomCustomTransportationName &&
          immediate.rtiAmbassador().getTransportationTypeName(immediateTransportation) ==
              timestamped.fomCustomTransportationName &&
          constrained.rtiAmbassador().getTransportationTypeName(constrainedTransportation) ==
              timestamped.fomCustomTransportationName,
      "custom transportation timestamped attribute lookup did not round-trip the transport name");

  rti::AttributeHandleSet const publisherAttributes{publisherAttribute};
  rti::AttributeHandleSet const immediateAttributes{immediateAttribute};
  rti::AttributeHandleSet const constrainedAttributes{constrainedAttribute};
  publisher.rtiAmbassador().publishObjectClassAttributes(
      publisherClass,
      publisherAttributes);
  publisher.rtiAmbassador().changeDefaultAttributeOrderType(
      publisherClass,
      publisherAttributes,
      rti::TIMESTAMP);
  immediate.rtiAmbassador().subscribeObjectClassAttributes(
      immediateClass,
      immediateAttributes,
      true,
      L"");
  constrained.rtiAmbassador().subscribeObjectClassAttributes(
      constrainedClass,
      constrainedAttributes,
      true,
      L"");

  auto const object = publisher.rtiAmbassador().registerObjectInstance(publisherClass);
  require(
      object.isValid(),
      "custom transportation timestamped attribute registration returned an invalid handle");
  waitForSessions(
      {&publisher, &immediate, &constrained},
      [&] {
        return immediate.recorder().hasDiscovery(object) &&
            constrained.recorder().hasDiscovery(object);
      },
      timestamped,
      "custom transportation timestamped attribute discovery");
  require(
      immediate.recorder().discovery().object == object &&
          constrained.recorder().discovery().object == object &&
          immediate.recorder().discovery().objectClass == immediateClass &&
          constrained.recorder().discovery().objectClass == constrainedClass,
      "custom transportation timestamped attribute discovery changed object metadata");

  auto publisherTime = makeTimeContext(publisher);
  auto constrainedTime = makeTimeContext(constrained);
  enableTimestampedRoles(
      publisher,
      constrained,
      publisherTime,
      constrainedTime,
      timestamped,
      "custom transportation timestamped attribute");

  auto assertReflection = [&](TimedReflectionRecord const& record,
                              rti::AttributeHandle const& expectedAttribute,
                              std::vector<std::uint8_t> const& expectedValue,
                              std::vector<std::uint8_t> const& expectedTag,
                              std::vector<std::uint8_t> const& expectedTime,
                              rti::OrderType expectedReceivedOrder,
                              Session& inspector) {
    require(
        record.object == object && record.values.size() == 1U &&
            record.values.count(expectedAttribute) == 1U,
        "custom transportation timestamped attribute reflection returned the wrong object or attribute set");
    require(
        copyBytes(record.values.at(expectedAttribute)) == expectedValue &&
            record.tag == expectedTag && record.time == expectedTime,
        "custom transportation timestamped attribute reflection changed value, tag, or time");
    require(
        record.producer == publisher.federateHandle() && !record.timeText.empty(),
        "custom transportation timestamped attribute reflection changed producer or time metadata");
    require(
        record.sentOrder == rti::TIMESTAMP &&
            record.receivedOrder == expectedReceivedOrder,
        "custom transportation timestamped attribute reflection changed order metadata");
    require(
        record.transportation.isValid() &&
            inspector.rtiAmbassador().getTransportationTypeName(record.transportation) ==
                timestamped.fomCustomTransportationName,
        "custom transportation timestamped attribute reflection changed transportation identity");
  };

  auto const firstTime = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      3U);
  auto const firstConstrainedTime = timeAfter(
      *constrainedTime.factory,
      *constrainedTime.initial,
      *constrainedTime.epsilon,
      3U);
  std::vector<std::uint8_t> const firstValueBytes{0x41U, 0x42U, 0x43U};
  std::vector<std::uint8_t> const firstTagBytes{0x51U, 0x52U};
  rti::AttributeHandleValueMap firstValues;
  firstValues.emplace(
      publisherAttribute,
      rti::VariableLengthData(firstValueBytes.data(), firstValueBytes.size()));
  rti::VariableLengthData const firstTag(firstTagBytes.data(), firstTagBytes.size());
  auto const firstRetraction = publisher.rtiAmbassador().updateAttributeValues(
      object,
      firstValues,
      firstTag,
      *firstTime);
  require(
      firstRetraction.isValid(),
      "custom transportation timestamped attribute update returned an invalid retraction handle");
  require(
      constrained.recorder().timedReflections().empty(),
      "custom transportation timestamped attribute delivered before the constrained grant");
  waitFor(
      immediate,
      [&] { return immediate.recorder().timedReflections().size() >= 1U; },
      timestamped,
      "custom transportation timestamped attribute immediate reflection");
  auto const firstImmediate = immediate.recorder().timedReflections();
  require(
      firstImmediate.size() == 1U,
      "custom transportation timestamped attribute delivered a duplicate immediate reflection");
  assertReflection(
      firstImmediate.front(),
      immediateAttribute,
      firstValueBytes,
      firstTagBytes,
      encodeTime(*firstTime),
      rti::RECEIVE,
      immediate);
  require(
      firstImmediate.front().retractionPresent &&
          firstImmediate.front().retraction == copyBytes(firstRetraction.encode()),
      "custom transportation timestamped attribute reflection returned the wrong retraction handle");

  publisher.rtiAmbassador().retract(firstRetraction);
  waitFor(
      immediate,
      [&] { return immediate.recorder().retractions().size() >= 1U; },
      timestamped,
      "custom transportation timestamped attribute retraction callback");
  auto const firstRetractionCallbacks = immediate.recorder().retractions();
  require(
      firstRetractionCallbacks.size() == 1U &&
          firstRetractionCallbacks.front().valid &&
          firstRetractionCallbacks.front().encoded == copyBytes(firstRetraction.encode()),
      "custom transportation timestamped attribute retraction callback returned the wrong handle");
  require(
      constrained.recorder().timedReflections().empty() &&
          constrained.recorder().retractions().empty(),
      "custom transportation timestamped attribute retraction leaked to the pending receiver");

  constrained.rtiAmbassador().timeAdvanceRequest(*firstConstrainedTime);
  publisher.rtiAmbassador().timeAdvanceRequest(*firstTime);
  waitForSessions(
      {&publisher, &constrained},
      [&] {
        return publisher.recorder().timeAdvanceGrants().size() >= 1U &&
            constrained.recorder().timeAdvanceGrants().size() >= 1U;
      },
      timestamped,
      "custom transportation timestamped attribute first time-advance grants");
  for (int pass = 0; pass != 8; ++pass) {
    constrained.pump();
  }
  require(
      constrained.recorder().timedReflections().empty(),
      "custom transportation timestamped attribute delivered a retracted update at its grant");

  auto const secondTime = timeAfter(
      *publisherTime.factory,
      *firstTime,
      *publisherTime.epsilon,
      3U);
  auto const secondConstrainedTime = timeAfter(
      *constrainedTime.factory,
      *firstConstrainedTime,
      *constrainedTime.epsilon,
      3U);
  std::vector<std::uint8_t> const secondValueBytes{0x61U, 0x62U, 0x63U};
  std::vector<std::uint8_t> const secondTagBytes{0x71U, 0x72U};
  rti::AttributeHandleValueMap secondValues;
  secondValues.emplace(
      publisherAttribute,
      rti::VariableLengthData(secondValueBytes.data(), secondValueBytes.size()));
  rti::VariableLengthData const secondTag(secondTagBytes.data(), secondTagBytes.size());
  auto const secondRetraction = publisher.rtiAmbassador().updateAttributeValues(
      object,
      secondValues,
      secondTag,
      *secondTime);
  require(
      secondRetraction.isValid(),
      "custom transportation timestamped attribute second update returned an invalid retraction handle");
  waitFor(
      immediate,
      [&] { return immediate.recorder().timedReflections().size() >= 2U; },
      timestamped,
      "custom transportation timestamped attribute second immediate reflection");
  auto const secondImmediate = immediate.recorder().timedReflections();
  require(
      secondImmediate.size() == 2U,
      "custom transportation timestamped attribute delivered a duplicate second reflection");
  assertReflection(
      secondImmediate.at(1),
      immediateAttribute,
      secondValueBytes,
      secondTagBytes,
      encodeTime(*secondTime),
      rti::RECEIVE,
      immediate);
  require(
      secondImmediate.at(1).retractionPresent &&
          secondImmediate.at(1).retraction == copyBytes(secondRetraction.encode()),
      "custom transportation timestamped attribute second reflection returned the wrong retraction handle");

  constrained.rtiAmbassador().timeAdvanceRequest(*secondConstrainedTime);
  publisher.rtiAmbassador().timeAdvanceRequest(*secondTime);
  waitForSessions(
      {&publisher, &constrained},
      [&] {
        return publisher.recorder().timeAdvanceGrants().size() >= 2U &&
            constrained.recorder().timeAdvanceGrants().size() >= 2U &&
            constrained.recorder().timedReflections().size() >= 1U;
      },
      timestamped,
      "custom transportation timestamped attribute second delivery and grants");
  auto const constrainedReflections = constrained.recorder().timedReflections();
  require(
      constrainedReflections.size() == 1U,
      "custom transportation timestamped attribute delivered the wrong number of constrained reflections");
  assertReflection(
      constrainedReflections.front(),
      constrainedAttribute,
      secondValueBytes,
      secondTagBytes,
      encodeTime(*secondTime),
      rti::TIMESTAMP,
      constrained);
  require(
      constrainedReflections.front().retractionPresent &&
          constrainedReflections.front().retraction == copyBytes(secondRetraction.encode()),
      "custom transportation timestamped attribute constrained reflection returned the wrong retraction handle");
  requireException(
      [&] { publisher.rtiAmbassador().retract(secondRetraction); },
      L"MessageCanNoLongerBeRetracted",
      "retracting a delivered custom transportation timestamped attribute update");

  constrained.rtiAmbassador().disableTimeConstrained();
  publisher.rtiAmbassador().disableTimeRegulation();
  immediate.rtiAmbassador().unsubscribeObjectClassAttributes(
      immediateClass,
      immediateAttributes);
  constrained.rtiAmbassador().unsubscribeObjectClassAttributes(
      constrainedClass,
      constrainedAttributes);
  publisher.rtiAmbassador().unpublishObjectClassAttributes(
      publisherClass,
      publisherAttributes);
  constrained.resign(rti::NO_ACTION);
  immediate.resign(rti::NO_ACTION);
  publisher.resign(rti::NO_ACTION);
  publisher.rtiAmbassador().destroyFederationExecution(federation);
  constrained.disconnect();
  immediate.disconnect();
  publisher.disconnect();
}

void scenarioCustomTransportationTimestampedAttributeDeliveryContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioCustomTransportationTimestampedAttributeDelivery(options, model);
}

void scenarioCustomTransportationTimestampedAttributeAlternateAdvances(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.modelFom.empty(),
      "Custom transportation timestamped attribute alternate-advance testing requires an adapter-supplied rich FOM");
  require(
      !options.logicalTimeImplementationName.empty(),
      "Custom transportation timestamped attribute alternate-advance testing requires an adapter-supplied logical-time implementation");

  auto alternate = options;
  alternate.fom = alternate.modelFom;
  alternate.objectClassName = alternate.customRegionalObjectClassName;
  alternate.attributeName = alternate.customRegionalAttributeName;
  alternate.requireCustomTransportation = true;

  // The established standard oracle covers the three alternate advance
  // services, grant ordering, query boundaries, and retraction terminalization.
  scenarioTimestampedAttributeUpdateAlternateAdvances(alternate, model);

  Session publisher(
      alternate,
      model,
      "custom-transportation-timestamped-attribute-alternate-publisher");
  Session flushQueue(
      alternate,
      model,
      "custom-transportation-timestamped-attribute-alternate-flush-queue");
  Session timeAdvanceAvailable(
      alternate,
      model,
      "custom-transportation-timestamped-attribute-alternate-tara");
  Session nextMessageAvailable(
      alternate,
      model,
      "custom-transportation-timestamped-attribute-alternate-nmra");
  auto const federation = federationName(
      alternate,
      "custom-transportation-timestamped-attribute-alternate-advances");

  publisher.connect();
  flushQueue.connect();
  timeAdvanceAvailable.connect();
  nextMessageAvailable.connect();
  publisher.rtiAmbassador().createFederationExecution(
      federation,
      alternate.modelFom.wstring(),
      alternate.logicalTimeImplementationName);
  publisher.join(alternate.ownerFederateName, alternate.federateType, federation);
  flushQueue.join(
      alternate.memberFederateName + L"-custom-transportation-flush-queue",
      alternate.federateType,
      federation);
  timeAdvanceAvailable.join(
      alternate.memberFederateName + L"-custom-transportation-tara",
      alternate.federateType,
      federation);
  nextMessageAvailable.join(
      alternate.memberFederateName + L"-custom-transportation-nmra",
      alternate.federateType,
      federation);

  auto const publisherClass = publisher.rtiAmbassador().getObjectClassHandle(
      alternate.objectClassName);
  auto const publisherAttribute = publisher.rtiAmbassador().getAttributeHandle(
      publisherClass,
      alternate.attributeName);
  require(
      publisherClass.isValid() && publisherAttribute.isValid() &&
          publisher.rtiAmbassador().getObjectClassName(publisherClass) ==
              alternate.objectClassName &&
          publisher.rtiAmbassador().getAttributeName(
              publisherClass,
              publisherAttribute) == alternate.attributeName,
      "custom transportation timestamped attribute alternate-advance publisher lookup did not round-trip");

  auto const publisherTransportation =
      publisher.rtiAmbassador().getTransportationTypeHandle(
          alternate.fomCustomTransportationName);
  require(
      publisherTransportation.isValid() &&
          publisher.rtiAmbassador().getTransportationTypeName(
              publisherTransportation) == alternate.fomCustomTransportationName,
      "custom transportation timestamped attribute alternate-advance publisher transport lookup did not round-trip");

  auto const members = std::vector<Session*>{
      &flushQueue,
      &timeAdvanceAvailable,
      &nextMessageAvailable,
  };
  std::vector<rti::ObjectClassHandle> memberClasses;
  std::vector<rti::AttributeHandle> memberAttributes;
  memberClasses.reserve(members.size());
  memberAttributes.reserve(members.size());
  for (auto* member : members) {
    auto const objectClass = member->rtiAmbassador().getObjectClassHandle(
        alternate.objectClassName);
    auto const attribute = member->rtiAmbassador().getAttributeHandle(
        objectClass,
        alternate.attributeName);
    auto const transportation = member->rtiAmbassador().getTransportationTypeHandle(
        alternate.fomCustomTransportationName);
    require(
        objectClass.isValid() && attribute.isValid() && transportation.isValid() &&
            member->rtiAmbassador().getObjectClassName(objectClass) ==
                alternate.objectClassName &&
            member->rtiAmbassador().getAttributeName(objectClass, attribute) ==
                alternate.attributeName &&
            member->rtiAmbassador().getTransportationTypeName(transportation) ==
                alternate.fomCustomTransportationName,
        "custom transportation timestamped attribute alternate-advance member lookup did not round-trip");
    memberClasses.push_back(objectClass);
    memberAttributes.push_back(attribute);
  }

  rti::AttributeHandleSet const publisherAttributes{publisherAttribute};
  publisher.rtiAmbassador().publishObjectClassAttributes(
      publisherClass,
      publisherAttributes);
  publisher.rtiAmbassador().changeDefaultAttributeOrderType(
      publisherClass,
      publisherAttributes,
      rti::TIMESTAMP);
  for (std::size_t index = 0U; index != members.size(); ++index) {
    members.at(index)->rtiAmbassador().subscribeObjectClassAttributes(
        memberClasses.at(index),
        rti::AttributeHandleSet{memberAttributes.at(index)},
        true,
        L"");
  }

  auto const object = publisher.rtiAmbassador().registerObjectInstance(publisherClass);
  require(
      object.isValid(),
      "custom transportation timestamped attribute alternate-advance registration returned an invalid object handle");
  waitForSessions(
      {&publisher, &flushQueue, &timeAdvanceAvailable, &nextMessageAvailable},
      [&] {
        return flushQueue.recorder().hasDiscovery(object) &&
            timeAdvanceAvailable.recorder().hasDiscovery(object) &&
            nextMessageAvailable.recorder().hasDiscovery(object);
      },
      alternate,
      "custom transportation timestamped attribute alternate-advance discovery");

  auto publisherTime = makeTimeContext(publisher);
  auto flushQueueTime = makeTimeContext(flushQueue);
  auto timeAdvanceAvailableTime = makeTimeContext(timeAdvanceAvailable);
  auto nextMessageAvailableTime = makeTimeContext(nextMessageAvailable);
  require(
      publisherTime.factory->getName() == flushQueueTime.factory->getName() &&
          publisherTime.factory->getName() == timeAdvanceAvailableTime.factory->getName() &&
          publisherTime.factory->getName() == nextMessageAvailableTime.factory->getName(),
      "custom transportation timestamped attribute alternate-advance members selected different logical-time factories");

  for (auto* member : members) {
    member->rtiAmbassador().enableTimeConstrained();
    waitFor(
        *member,
        [&] { return member->recorder().timeConstrainedEnabled().size() >= 1U; },
        alternate,
        "custom transportation timestamped attribute alternate-advance time-constrained callback");
  }
  publisher.rtiAmbassador().enableTimeRegulation(*publisherTime.epsilon);
  waitFor(
      publisher,
      [&] { return publisher.recorder().timeRegulationEnabled().size() >= 1U; },
      alternate,
      "custom transportation timestamped attribute alternate-advance time-regulation callback");

  auto const messageTime = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      5U);
  auto const publisherTarget = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      4U);
  auto const requestBoundary = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      10U);
  std::vector<std::uint8_t> const valueBytes{0x43U, 0x55U, 0x53U, 0x54U};
  std::vector<std::uint8_t> const tagBytes{0x41U, 0x4CU, 0x54U};
  rti::AttributeHandleValueMap values;
  values.emplace(
      publisherAttribute,
      rti::VariableLengthData(valueBytes.data(), valueBytes.size()));
  rti::VariableLengthData const tag(tagBytes.data(), tagBytes.size());
  auto const retraction = publisher.rtiAmbassador().updateAttributeValues(
      object,
      values,
      tag,
      *messageTime);
  require(
      retraction.isValid(),
      "custom transportation timestamped attribute alternate-advance update returned an invalid retraction handle");
  for (auto* member : members) {
    require(
        member->recorder().timedReflections().empty(),
        "custom transportation timestamped attribute alternate-advance delivered before its request");
  }

  flushQueue.rtiAmbassador().flushQueueRequest(*requestBoundary);
  timeAdvanceAvailable.rtiAmbassador().timeAdvanceRequestAvailable(*messageTime);
  nextMessageAvailable.rtiAmbassador().nextMessageRequestAvailable(*requestBoundary);
  publisher.rtiAmbassador().timeAdvanceRequest(*publisherTarget);
  waitForSessions(
      {&publisher, &flushQueue, &timeAdvanceAvailable, &nextMessageAvailable},
      [&] {
        return publisher.recorder().timeAdvanceGrants().size() >= 1U &&
            flushQueue.recorder().timedReflections().size() >= 1U &&
            flushQueue.recorder().flushQueueGrants().size() >= 1U &&
            timeAdvanceAvailable.recorder().timedReflections().size() >= 1U &&
            timeAdvanceAvailable.recorder().timeAdvanceGrants().size() >= 1U &&
            nextMessageAvailable.recorder().timedReflections().size() >= 1U &&
            nextMessageAvailable.recorder().timeAdvanceGrants().size() >= 1U;
      },
      alternate,
      "custom transportation timestamped attribute alternate-advance delivery and grants");

  auto assertReflection = [&](TimedReflectionRecord const& record,
                              rti::AttributeHandle const& expectedAttribute,
                              Session& inspector) {
    require(
        record.object == object && record.values.size() == 1U &&
            record.values.count(expectedAttribute) == 1U &&
            copyBytes(record.values.at(expectedAttribute)) == valueBytes &&
            record.tag == tagBytes && record.time == encodeTime(*messageTime),
        "custom transportation timestamped attribute alternate-advance callback changed the payload");
    require(
        record.producer == publisher.federateHandle() && !record.timeText.empty() &&
            record.sentOrder == rti::TIMESTAMP &&
            record.receivedOrder == rti::TIMESTAMP,
        "custom transportation timestamped attribute alternate-advance callback changed time or order metadata");
    require(
        record.transportation.isValid() &&
            inspector.rtiAmbassador().getTransportationTypeName(
                record.transportation) == alternate.fomCustomTransportationName,
        "custom transportation timestamped attribute alternate-advance callback changed transportation identity");
    require(
        record.retractionPresent &&
            record.retraction == copyBytes(retraction.encode()),
        "custom transportation timestamped attribute alternate-advance callback returned the wrong retraction handle");
  };
  require(
      flushQueue.recorder().timedReflections().size() == 1U &&
          timeAdvanceAvailable.recorder().timedReflections().size() == 1U &&
          nextMessageAvailable.recorder().timedReflections().size() == 1U,
      "custom transportation timestamped attribute alternate-advance delivered an unexpected callback count");
  assertReflection(
      flushQueue.recorder().timedReflections().front(),
      memberAttributes.at(0),
      flushQueue);
  assertReflection(
      timeAdvanceAvailable.recorder().timedReflections().front(),
      memberAttributes.at(1),
      timeAdvanceAvailable);
  assertReflection(
      nextMessageAvailable.recorder().timedReflections().front(),
      memberAttributes.at(2),
      nextMessageAvailable);
  requireException(
      [&] { publisher.rtiAmbassador().retract(retraction); },
      L"MessageCanNoLongerBeRetracted",
      "retracting a delivered custom transportation timestamped attribute alternate-advance update");

  for (auto* member : members) {
    member->rtiAmbassador().disableTimeConstrained();
  }
  publisher.rtiAmbassador().disableTimeRegulation();
  for (std::size_t index = 0U; index != members.size(); ++index) {
    members.at(index)->rtiAmbassador().unsubscribeObjectClassAttributes(
        memberClasses.at(index),
        rti::AttributeHandleSet{memberAttributes.at(index)});
  }
  publisher.rtiAmbassador().unpublishObjectClassAttributes(
      publisherClass,
      publisherAttributes);
  nextMessageAvailable.resign(rti::NO_ACTION);
  timeAdvanceAvailable.resign(rti::NO_ACTION);
  flushQueue.resign(rti::NO_ACTION);
  publisher.resign(rti::NO_ACTION);
  publisher.rtiAmbassador().destroyFederationExecution(federation);
  nextMessageAvailable.disconnect();
  timeAdvanceAvailable.disconnect();
  flushQueue.disconnect();
  publisher.disconnect();
}

void scenarioCustomTransportationTimestampedAttributeAlternateAdvancesContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioCustomTransportationTimestampedAttributeAlternateAdvances(options, model);
}

void scenarioCustomTransportationTimestampedInteractionAlternateAdvances(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.modelFom.empty(),
      "Custom transportation timestamped interaction alternate-advance testing requires an adapter-supplied rich FOM");
  require(
      !options.logicalTimeImplementationName.empty(),
      "Custom transportation timestamped interaction alternate-advance testing requires an adapter-supplied logical-time implementation");

  auto alternate = options;
  alternate.fom = alternate.modelFom;
  alternate.interactionClassName = alternate.customRegionalInteractionClassName;
  alternate.parameterName = alternate.customRegionalParameterName;
  alternate.requireCustomTransportation = true;

  // The established standard oracle covers the three alternate advance
  // services, grant ordering, query boundaries, and retraction terminalization.
  scenarioTimestampedInteractionMixedAdvances(alternate, model);

  Session publisher(
      alternate,
      model,
      "custom-transportation-timestamped-interaction-alternate-publisher");
  Session flushQueue(
      alternate,
      model,
      "custom-transportation-timestamped-interaction-alternate-flush-queue");
  Session timeAdvanceAvailable(
      alternate,
      model,
      "custom-transportation-timestamped-interaction-alternate-tara");
  Session nextMessageAvailable(
      alternate,
      model,
      "custom-transportation-timestamped-interaction-alternate-nmra");
  auto const federation = federationName(
      alternate,
      "custom-transportation-timestamped-interaction-alternate-advances");

  publisher.connect();
  flushQueue.connect();
  timeAdvanceAvailable.connect();
  nextMessageAvailable.connect();
  publisher.rtiAmbassador().createFederationExecution(
      federation,
      alternate.modelFom.wstring(),
      alternate.logicalTimeImplementationName);
  publisher.join(alternate.ownerFederateName, alternate.federateType, federation);
  flushQueue.join(
      alternate.memberFederateName + L"-custom-transportation-interaction-flush-queue",
      alternate.federateType,
      federation);
  timeAdvanceAvailable.join(
      alternate.memberFederateName + L"-custom-transportation-interaction-tara",
      alternate.federateType,
      federation);
  nextMessageAvailable.join(
      alternate.memberFederateName + L"-custom-transportation-interaction-nmra",
      alternate.federateType,
      federation);

  auto const publisherInteraction = publisher.rtiAmbassador().getInteractionClassHandle(
      alternate.interactionClassName);
  auto const publisherParameter = publisher.rtiAmbassador().getParameterHandle(
      publisherInteraction,
      alternate.parameterName);
  require(
      publisherInteraction.isValid() && publisherParameter.isValid() &&
          publisher.rtiAmbassador().getInteractionClassName(publisherInteraction) ==
              alternate.interactionClassName &&
          publisher.rtiAmbassador().getParameterName(
              publisherInteraction,
              publisherParameter) == alternate.parameterName,
      "custom transportation timestamped interaction alternate-advance publisher lookup did not round-trip");

  auto const publisherTransportation =
      publisher.rtiAmbassador().getTransportationTypeHandle(
          alternate.fomCustomTransportationName);
  require(
      publisherTransportation.isValid() &&
          publisher.rtiAmbassador().getTransportationTypeName(
              publisherTransportation) == alternate.fomCustomTransportationName,
      "custom transportation timestamped interaction alternate-advance publisher transport lookup did not round-trip");

  auto const members = std::vector<Session*>{
      &flushQueue,
      &timeAdvanceAvailable,
      &nextMessageAvailable,
  };
  std::vector<rti::InteractionClassHandle> memberInteractions;
  std::vector<rti::ParameterHandle> memberParameters;
  memberInteractions.reserve(members.size());
  memberParameters.reserve(members.size());
  for (auto* member : members) {
    auto const interaction = member->rtiAmbassador().getInteractionClassHandle(
        alternate.interactionClassName);
    auto const parameter = member->rtiAmbassador().getParameterHandle(
        interaction,
        alternate.parameterName);
    auto const transportation = member->rtiAmbassador().getTransportationTypeHandle(
        alternate.fomCustomTransportationName);
    require(
        interaction.isValid() && parameter.isValid() && transportation.isValid() &&
            member->rtiAmbassador().getInteractionClassName(interaction) ==
                alternate.interactionClassName &&
            member->rtiAmbassador().getParameterName(interaction, parameter) ==
                alternate.parameterName &&
            member->rtiAmbassador().getTransportationTypeName(transportation) ==
                alternate.fomCustomTransportationName,
        "custom transportation timestamped interaction alternate-advance member lookup did not round-trip");
    memberInteractions.push_back(interaction);
    memberParameters.push_back(parameter);
  }

  publisher.rtiAmbassador().publishInteractionClass(publisherInteraction);
  publisher.rtiAmbassador().changeInteractionOrderType(
      publisherInteraction,
      rti::TIMESTAMP);
  for (auto const& interaction : memberInteractions) {
    auto const index = static_cast<std::size_t>(
        &interaction - memberInteractions.data());
    members.at(index)->rtiAmbassador().subscribeInteractionClass(interaction, true);
  }

  auto publisherTime = makeTimeContext(publisher);
  auto flushQueueTime = makeTimeContext(flushQueue);
  auto timeAdvanceAvailableTime = makeTimeContext(timeAdvanceAvailable);
  auto nextMessageAvailableTime = makeTimeContext(nextMessageAvailable);
  require(
      publisherTime.factory->getName() == flushQueueTime.factory->getName() &&
          publisherTime.factory->getName() == timeAdvanceAvailableTime.factory->getName() &&
          publisherTime.factory->getName() == nextMessageAvailableTime.factory->getName(),
      "custom transportation timestamped interaction alternate-advance members selected different logical-time factories");

  for (auto* member : members) {
    member->rtiAmbassador().enableTimeConstrained();
    waitFor(
        *member,
        [&] { return member->recorder().timeConstrainedEnabled().size() >= 1U; },
        alternate,
        "custom transportation timestamped interaction alternate-advance time-constrained callback");
  }
  publisher.rtiAmbassador().enableTimeRegulation(*publisherTime.epsilon);
  waitFor(
      publisher,
      [&] { return publisher.recorder().timeRegulationEnabled().size() >= 1U; },
      alternate,
      "custom transportation timestamped interaction alternate-advance time-regulation callback");

  auto const messageTime = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      5U);
  auto const publisherTarget = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      4U);
  auto const requestBoundary = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      10U);
  std::vector<std::uint8_t> const parameterBytes{0x49U, 0x41U, 0x4CU, 0x54U};
  std::vector<std::uint8_t> const tagBytes{0x54U, 0x41U, 0x4CU};
  rti::ParameterHandleValueMap parameters;
  parameters.emplace(
      publisherParameter,
      rti::VariableLengthData(parameterBytes.data(), parameterBytes.size()));
  rti::VariableLengthData const tag(tagBytes.data(), tagBytes.size());
  auto const retraction = publisher.rtiAmbassador().sendInteraction(
      publisherInteraction,
      parameters,
      tag,
      *messageTime);
  require(
      retraction.isValid(),
      "custom transportation timestamped interaction alternate-advance send returned an invalid retraction handle");
  for (auto* member : members) {
    require(
        member->recorder().timedInteractions().empty(),
        "custom transportation timestamped interaction alternate-advance delivered before its request");
  }

  flushQueue.rtiAmbassador().flushQueueRequest(*requestBoundary);
  timeAdvanceAvailable.rtiAmbassador().timeAdvanceRequestAvailable(*messageTime);
  nextMessageAvailable.rtiAmbassador().nextMessageRequestAvailable(*requestBoundary);
  publisher.rtiAmbassador().timeAdvanceRequest(*publisherTarget);
  waitForSessions(
      {&publisher, &flushQueue, &timeAdvanceAvailable, &nextMessageAvailable},
      [&] {
        return publisher.recorder().timeAdvanceGrants().size() >= 1U &&
            flushQueue.recorder().timedInteractions().size() >= 1U &&
            flushQueue.recorder().flushQueueGrants().size() >= 1U &&
            timeAdvanceAvailable.recorder().timedInteractions().size() >= 1U &&
            timeAdvanceAvailable.recorder().timeAdvanceGrants().size() >= 1U &&
            nextMessageAvailable.recorder().timedInteractions().size() >= 1U &&
            nextMessageAvailable.recorder().timeAdvanceGrants().size() >= 1U;
      },
      alternate,
      "custom transportation timestamped interaction alternate-advance delivery and grants");

  auto assertInteraction = [&](TimedInteractionRecord const& record,
                               rti::InteractionClassHandle const& expectedInteraction,
                               rti::ParameterHandle const& expectedParameter,
                               Session& inspector) {
    require(
        record.interaction == expectedInteraction && record.parameters.size() == 1U &&
            record.parameters.count(expectedParameter) == 1U &&
            copyBytes(record.parameters.at(expectedParameter)) == parameterBytes &&
            record.tag == tagBytes && record.time == encodeTime(*messageTime),
        "custom transportation timestamped interaction alternate-advance callback changed the payload");
    require(
        record.producer == publisher.federateHandle() && !record.timeText.empty() &&
            record.sentOrder == rti::TIMESTAMP &&
            record.receivedOrder == rti::TIMESTAMP,
        "custom transportation timestamped interaction alternate-advance callback changed time or order metadata");
    require(
        !record.regions.has_value() && record.transportation.isValid() &&
            inspector.rtiAmbassador().getTransportationTypeName(
                record.transportation) == alternate.fomCustomTransportationName,
        "custom transportation timestamped interaction alternate-advance callback changed transportation identity or regions");
    require(
        record.retractionPresent &&
            record.retraction == copyBytes(retraction.encode()),
        "custom transportation timestamped interaction alternate-advance callback returned the wrong retraction handle");
  };
  require(
      flushQueue.recorder().timedInteractions().size() == 1U &&
          timeAdvanceAvailable.recorder().timedInteractions().size() == 1U &&
          nextMessageAvailable.recorder().timedInteractions().size() == 1U,
      "custom transportation timestamped interaction alternate-advance delivered an unexpected callback count");
  assertInteraction(
      flushQueue.recorder().timedInteractions().front(),
      memberInteractions.at(0),
      memberParameters.at(0),
      flushQueue);
  assertInteraction(
      timeAdvanceAvailable.recorder().timedInteractions().front(),
      memberInteractions.at(1),
      memberParameters.at(1),
      timeAdvanceAvailable);
  assertInteraction(
      nextMessageAvailable.recorder().timedInteractions().front(),
      memberInteractions.at(2),
      memberParameters.at(2),
      nextMessageAvailable);
  requireException(
      [&] { publisher.rtiAmbassador().retract(retraction); },
      L"MessageCanNoLongerBeRetracted",
      "retracting a delivered custom transportation timestamped interaction alternate-advance message");

  for (auto* member : members) {
    member->rtiAmbassador().disableTimeConstrained();
  }
  publisher.rtiAmbassador().disableTimeRegulation();
  for (std::size_t index = 0U; index != members.size(); ++index) {
    members.at(index)->rtiAmbassador().unsubscribeInteractionClass(
        memberInteractions.at(index));
  }
  publisher.rtiAmbassador().unpublishInteractionClass(publisherInteraction);
  nextMessageAvailable.resign(rti::NO_ACTION);
  timeAdvanceAvailable.resign(rti::NO_ACTION);
  flushQueue.resign(rti::NO_ACTION);
  publisher.resign(rti::NO_ACTION);
  publisher.rtiAmbassador().destroyFederationExecution(federation);
  nextMessageAvailable.disconnect();
  timeAdvanceAvailable.disconnect();
  flushQueue.disconnect();
  publisher.disconnect();
}

void scenarioCustomTransportationTimestampedInteractionAlternateAdvancesContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioCustomTransportationTimestampedInteractionAlternateAdvances(options, model);
}

void scenarioCustomTransportationTimestampedDirectedDeliveryPortable(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.modelFom.empty(),
      "Custom transportation timestamped directed delivery requires an adapter-supplied rich FOM");
  require(
      !options.logicalTimeImplementationName.empty(),
      "Custom transportation timestamped directed delivery requires an adapter-supplied logical-time implementation");

  // Keep the established standard directed-delivery oracle behind the portable
  // adapter boundary. It uses only the official API; the adapter supplies the
  // provider, FOM, endpoint, logical-time, and callback configuration.
  scenarioCustomTransportationTimestampedDirectedDelivery(options, model);
}

void scenarioCustomTransportationTimestampedDirectedDeliveryPortableContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioCustomTransportationTimestampedDirectedDeliveryPortable(options, model);
}

void scenarioCustomTransportationInteractionDeliveryPortable(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.modelFom.empty(),
      "Custom transportation interaction delivery requires an adapter-supplied rich FOM");
  scenarioCustomTransportationInteractionDelivery(options, model);
}

void scenarioCustomTransportationInteractionDeliveryPortableContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioCustomTransportationInteractionDeliveryPortable(options, model);
}

void scenarioCustomTransportationTimestampedDeliveryPortable(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.modelFom.empty(),
      "Custom transportation timestamped interaction delivery requires an adapter-supplied rich FOM");
  require(
      !options.logicalTimeImplementationName.empty(),
      "Custom transportation timestamped interaction delivery requires an adapter-supplied logical-time implementation");
  scenarioCustomTransportationTimestampedDelivery(options, model);
}

void scenarioCustomTransportationTimestampedDeliveryPortableContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioCustomTransportationTimestampedDeliveryPortable(options, model);
}

void scenarioCustomTransportationDirectedInteractionDelivery(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.modelFom.empty(),
      "Custom transportation directed interaction testing requires an adapter-supplied rich FOM");

  auto directed = options;
  directed.fom = directed.modelFom;
  directed.objectClassName = directed.typedObjectClassName;
  directed.attributeName = directed.typedIntegerAttributeName;
  directed.interactionClassName = directed.typedInteractionClassName;
  directed.parameterName = directed.typedIntegerParameterName;
  directed.requireCustomTransportation = true;

  Session publisher(
      directed,
      model,
      "custom-transportation-directed-interaction-publisher");
  Session targeted(
      directed,
      model,
      "custom-transportation-directed-interaction-targeted");
  Session universal(
      directed,
      rti::HLA_IMMEDIATE,
      "custom-transportation-directed-interaction-universal");
  auto const federation = federationName(
      directed,
      "custom-transportation-directed-interaction-delivery");
  connectAndJoin(
      publisher,
      targeted,
      directed,
      federation,
      directed.modelFom);
  universal.connect();
  universal.join(
      directed.memberFederateName + L"-custom-transportation-directed-interaction-universal",
      directed.federateType,
      federation);

  auto const publisherClass = publisher.rtiAmbassador().getObjectClassHandle(
      directed.objectClassName);
  auto const targetedClass = targeted.rtiAmbassador().getObjectClassHandle(
      directed.objectClassName);
  auto const universalClass = universal.rtiAmbassador().getObjectClassHandle(
      directed.objectClassName);
  auto const publisherAttribute = publisher.rtiAmbassador().getAttributeHandle(
      publisherClass,
      directed.attributeName);
  auto const targetedAttribute = targeted.rtiAmbassador().getAttributeHandle(
      targetedClass,
      directed.attributeName);
  auto const universalAttribute = universal.rtiAmbassador().getAttributeHandle(
      universalClass,
      directed.attributeName);
  auto const publisherInteraction =
      publisher.rtiAmbassador().getInteractionClassHandle(
          directed.interactionClassName);
  auto const targetedInteraction =
      targeted.rtiAmbassador().getInteractionClassHandle(
          directed.interactionClassName);
  auto const universalInteraction =
      universal.rtiAmbassador().getInteractionClassHandle(
          directed.interactionClassName);
  auto const publisherParameter = publisher.rtiAmbassador().getParameterHandle(
      publisherInteraction,
      directed.parameterName);
  auto const targetedParameter = targeted.rtiAmbassador().getParameterHandle(
      targetedInteraction,
      directed.parameterName);
  auto const universalParameter = universal.rtiAmbassador().getParameterHandle(
      universalInteraction,
      directed.parameterName);
  auto const publisherTransportation =
      publisher.rtiAmbassador().getTransportationTypeHandle(
          directed.fomCustomTransportationName);
  auto const targetedTransportation =
      targeted.rtiAmbassador().getTransportationTypeHandle(
          directed.fomCustomTransportationName);
  auto const universalTransportation =
      universal.rtiAmbassador().getTransportationTypeHandle(
          directed.fomCustomTransportationName);
  require(
      publisherClass.isValid() && targetedClass.isValid() &&
          universalClass.isValid() && publisherAttribute.isValid() &&
          targetedAttribute.isValid() && universalAttribute.isValid() &&
          publisherInteraction.isValid() && targetedInteraction.isValid() &&
          universalInteraction.isValid() && publisherParameter.isValid() &&
          targetedParameter.isValid() && universalParameter.isValid() &&
          publisherTransportation.isValid() && targetedTransportation.isValid() &&
          universalTransportation.isValid(),
      "custom transportation directed interaction lookup returned an invalid handle");
  require(
      publisher.rtiAmbassador().getObjectClassName(publisherClass) ==
              directed.objectClassName &&
          targeted.rtiAmbassador().getObjectClassName(targetedClass) ==
              directed.objectClassName &&
          universal.rtiAmbassador().getObjectClassName(universalClass) ==
              directed.objectClassName &&
          publisher.rtiAmbassador().getAttributeName(
              publisherClass,
              publisherAttribute) == directed.attributeName &&
          targeted.rtiAmbassador().getAttributeName(
              targetedClass,
              targetedAttribute) == directed.attributeName &&
          universal.rtiAmbassador().getAttributeName(
              universalClass,
              universalAttribute) == directed.attributeName &&
          publisher.rtiAmbassador().getInteractionClassName(
              publisherInteraction) == directed.interactionClassName &&
          targeted.rtiAmbassador().getInteractionClassName(
              targetedInteraction) == directed.interactionClassName &&
          universal.rtiAmbassador().getInteractionClassName(
              universalInteraction) == directed.interactionClassName &&
          publisher.rtiAmbassador().getParameterName(
              publisherInteraction,
              publisherParameter) == directed.parameterName &&
          targeted.rtiAmbassador().getParameterName(
              targetedInteraction,
              targetedParameter) == directed.parameterName &&
          universal.rtiAmbassador().getParameterName(
              universalInteraction,
              universalParameter) == directed.parameterName &&
          publisher.rtiAmbassador().getTransportationTypeName(
              publisherTransportation) == directed.fomCustomTransportationName &&
          targeted.rtiAmbassador().getTransportationTypeName(
              targetedTransportation) == directed.fomCustomTransportationName &&
          universal.rtiAmbassador().getTransportationTypeName(
              universalTransportation) == directed.fomCustomTransportationName,
      "custom transportation directed interaction lookup did not round-trip names");

  rti::AttributeHandleSet const publisherAttributes{publisherAttribute};
  rti::AttributeHandleSet const targetedAttributes{targetedAttribute};
  rti::AttributeHandleSet const universalAttributes{universalAttribute};
  publisher.rtiAmbassador().subscribeObjectClassAttributes(
      publisherClass,
      publisherAttributes,
      true,
      L"");
  targeted.rtiAmbassador().publishObjectClassAttributes(
      targetedClass,
      targetedAttributes);
  universal.rtiAmbassador().subscribeObjectClassAttributes(
      universalClass,
      universalAttributes,
      true,
      L"");
  targeted.rtiAmbassador().subscribeObjectClassDirectedInteractions(
      targetedClass,
      rti::InteractionClassHandleSet{targetedInteraction},
      false);
  universal.rtiAmbassador().subscribeObjectClassDirectedInteractions(
      universalClass,
      rti::InteractionClassHandleSet{universalInteraction},
      true);
  publisher.rtiAmbassador().publishObjectClassDirectedInteractions(
      publisherClass,
      rti::InteractionClassHandleSet{publisherInteraction});

  auto const target = targeted.rtiAmbassador().registerObjectInstance(
      targetedClass);
  require(
      target.isValid(),
      "custom transportation directed interaction registration returned an invalid target");
  auto const targetName = targeted.rtiAmbassador().getObjectInstanceName(target);
  require(
      !targetName.empty() &&
          targeted.rtiAmbassador().getObjectInstanceHandle(targetName) == target,
      "custom transportation directed interaction registration did not preserve object naming");
  waitForSessions(
      {&publisher, &universal},
      [&] {
        return publisher.recorder().hasDiscovery(target) &&
            universal.recorder().hasDiscovery(target);
      },
      directed,
      "custom transportation directed interaction target discovery");

  std::vector<std::uint8_t> const parameterBytes{
      0x43U,
      0x55U,
      0x53U,
      0x54U,
  };
  std::vector<std::uint8_t> const tagBytes{
      0x44U,
      0x49U,
      0x52U,
      0x2DU,
      0x43U,
  };
  rti::ParameterHandleValueMap parameters;
  parameters.emplace(
      publisherParameter,
      rti::VariableLengthData(parameterBytes.data(), parameterBytes.size()));
  rti::VariableLengthData const tag(tagBytes.data(), tagBytes.size());
  publisher.rtiAmbassador().sendDirectedInteraction(
      publisherInteraction,
      target,
      parameters,
      tag);
  waitFor(
      targeted,
      [&] { return targeted.recorder().directedInteractions().size() >= 1U; },
      directed,
      "custom transportation directed interaction targeted delivery");
  require(
      universal.recorder().directedInteractions().size() == 1U,
      "custom transportation directed interaction universal delivery returned the wrong count");
  require(
      publisher.recorder().directedInteractions().empty(),
      "custom transportation directed interaction looped back to its publisher");

  auto assertDelivery = [&](DirectedInteractionRecord const& record,
                            rti::InteractionClassHandle const& expectedInteraction,
                            rti::ParameterHandle const& expectedParameter,
                            Session& inspector,
                            std::string const& description) {
    require(
        record.interaction == expectedInteraction && record.object == target &&
            record.parameters.size() == 1U &&
            record.parameters.count(expectedParameter) == 1U &&
            copyBytes(record.parameters.at(expectedParameter)) == parameterBytes,
        description + " returned the wrong target or parameter value");
    require(
        record.tag == tagBytes && record.producer == publisher.federateHandle(),
        description + " changed the standard interaction metadata");
    require(
        record.transportation.isValid() &&
            inspector.rtiAmbassador().getTransportationTypeName(
                record.transportation) == directed.fomCustomTransportationName,
        description + " changed the custom transportation identity");
  };
  assertDelivery(
      targeted.recorder().directedInteractions().front(),
      targetedInteraction,
      targetedParameter,
      targeted,
      "custom transportation directed interaction targeted callback");
  assertDelivery(
      universal.recorder().directedInteractions().front(),
      universalInteraction,
      universalParameter,
      universal,
      "custom transportation directed interaction universal callback");

  targeted.rtiAmbassador().unsubscribeObjectClassDirectedInteractions(
      targetedClass,
      rti::InteractionClassHandleSet{targetedInteraction});
  universal.rtiAmbassador().unsubscribeObjectClassDirectedInteractions(
      universalClass,
      rti::InteractionClassHandleSet{universalInteraction});
  publisher.rtiAmbassador().unpublishObjectClassDirectedInteractions(
      publisherClass,
      rti::InteractionClassHandleSet{publisherInteraction});
  publisher.rtiAmbassador().unsubscribeObjectClassAttributes(
      publisherClass,
      publisherAttributes);
  universal.rtiAmbassador().unsubscribeObjectClassAttributes(
      universalClass,
      universalAttributes);
  targeted.rtiAmbassador().unpublishObjectClassAttributes(
      targetedClass,
      targetedAttributes);
  universal.resign(rti::NO_ACTION);
  publisher.resign(rti::NO_ACTION);
  targeted.resign(rti::DELETE_OBJECTS);
  targeted.rtiAmbassador().destroyFederationExecution(federation);
  universal.disconnect();
  publisher.disconnect();
  targeted.disconnect();
}

void scenarioCustomTransportationDirectedInteractionDeliveryContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioCustomTransportationDirectedInteractionDelivery(options, model);
}

void scenarioCustomTransportationTimestampedDirectedInteractionAlternateAdvances(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.modelFom.empty(),
      "Custom transportation timestamped directed interaction alternate-advance testing requires an adapter-supplied rich FOM");
  require(
      !options.logicalTimeImplementationName.empty(),
      "Custom transportation timestamped directed interaction alternate-advance testing requires an adapter-supplied logical-time implementation");

  auto alternate = options;
  alternate.fom = alternate.modelFom;
  alternate.objectClassName = alternate.typedObjectClassName;
  alternate.attributeName = alternate.typedIntegerAttributeName;
  alternate.interactionClassName = alternate.typedInteractionClassName;
  alternate.parameterName = alternate.typedIntegerParameterName;
  alternate.requireCustomTransportation = true;

  // The established standard oracle covers the three alternate advance
  // services, grant ordering, query boundaries, and retraction terminalization.
  scenarioTimestampedDirectedAlternateAdvances(alternate, model);

  Session publisher(
      alternate,
      model,
      "custom-transportation-timestamped-directed-interaction-alternate-publisher");
  Session flushQueue(
      alternate,
      model,
      "custom-transportation-timestamped-directed-interaction-alternate-flush-queue");
  Session timeAdvanceAvailable(
      alternate,
      model,
      "custom-transportation-timestamped-directed-interaction-alternate-tara");
  Session nextMessageAvailable(
      alternate,
      model,
      "custom-transportation-timestamped-directed-interaction-alternate-nmra");
  auto const federation = federationName(
      alternate,
      "custom-transportation-timestamped-directed-interaction-alternate-advances");

  publisher.connect();
  flushQueue.connect();
  timeAdvanceAvailable.connect();
  nextMessageAvailable.connect();
  publisher.rtiAmbassador().createFederationExecution(
      federation,
      alternate.modelFom.wstring(),
      alternate.logicalTimeImplementationName);
  publisher.join(alternate.ownerFederateName, alternate.federateType, federation);
  flushQueue.join(
      alternate.memberFederateName + L"-custom-transportation-directed-flush-queue",
      alternate.federateType,
      federation);
  timeAdvanceAvailable.join(
      alternate.memberFederateName + L"-custom-transportation-directed-tara",
      alternate.federateType,
      federation);
  nextMessageAvailable.join(
      alternate.memberFederateName + L"-custom-transportation-directed-nmra",
      alternate.federateType,
      federation);

  auto const publisherClass = publisher.rtiAmbassador().getObjectClassHandle(
      alternate.objectClassName);
  auto const publisherAttribute = publisher.rtiAmbassador().getAttributeHandle(
      publisherClass,
      alternate.attributeName);
  auto const publisherInteraction =
      publisher.rtiAmbassador().getInteractionClassHandle(
          alternate.interactionClassName);
  auto const publisherParameter = publisher.rtiAmbassador().getParameterHandle(
      publisherInteraction,
      alternate.parameterName);
  auto const publisherTransportation =
      publisher.rtiAmbassador().getTransportationTypeHandle(
          alternate.fomCustomTransportationName);
  require(
      publisherClass.isValid() && publisherAttribute.isValid() &&
          publisherInteraction.isValid() && publisherParameter.isValid() &&
          publisherTransportation.isValid() &&
          publisher.rtiAmbassador().getObjectClassName(publisherClass) ==
              alternate.objectClassName &&
          publisher.rtiAmbassador().getAttributeName(
              publisherClass,
              publisherAttribute) == alternate.attributeName &&
          publisher.rtiAmbassador().getInteractionClassName(
              publisherInteraction) == alternate.interactionClassName &&
          publisher.rtiAmbassador().getParameterName(
              publisherInteraction,
              publisherParameter) == alternate.parameterName &&
          publisher.rtiAmbassador().getTransportationTypeName(
              publisherTransportation) == alternate.fomCustomTransportationName,
      "custom transportation timestamped directed interaction alternate-advance publisher lookup did not round-trip");

  auto const members = std::vector<Session*>{
      &flushQueue,
      &timeAdvanceAvailable,
      &nextMessageAvailable,
  };
  std::vector<rti::ObjectClassHandle> memberClasses;
  std::vector<rti::AttributeHandle> memberAttributes;
  std::vector<rti::InteractionClassHandle> memberInteractions;
  std::vector<rti::ParameterHandle> memberParameters;
  memberClasses.reserve(members.size());
  memberAttributes.reserve(members.size());
  memberInteractions.reserve(members.size());
  memberParameters.reserve(members.size());
  for (auto* member : members) {
    auto const objectClass = member->rtiAmbassador().getObjectClassHandle(
        alternate.objectClassName);
    auto const attribute = member->rtiAmbassador().getAttributeHandle(
        objectClass,
        alternate.attributeName);
    auto const interaction = member->rtiAmbassador().getInteractionClassHandle(
        alternate.interactionClassName);
    auto const parameter = member->rtiAmbassador().getParameterHandle(
        interaction,
        alternate.parameterName);
    auto const transportation = member->rtiAmbassador().getTransportationTypeHandle(
        alternate.fomCustomTransportationName);
    require(
        objectClass.isValid() && attribute.isValid() && interaction.isValid() &&
            parameter.isValid() && transportation.isValid() &&
            member->rtiAmbassador().getObjectClassName(objectClass) ==
                alternate.objectClassName &&
            member->rtiAmbassador().getAttributeName(objectClass, attribute) ==
                alternate.attributeName &&
            member->rtiAmbassador().getInteractionClassName(interaction) ==
                alternate.interactionClassName &&
            member->rtiAmbassador().getParameterName(interaction, parameter) ==
                alternate.parameterName &&
            member->rtiAmbassador().getTransportationTypeName(transportation) ==
                alternate.fomCustomTransportationName,
        "custom transportation timestamped directed interaction alternate-advance member lookup did not round-trip");
    memberClasses.push_back(objectClass);
    memberAttributes.push_back(attribute);
    memberInteractions.push_back(interaction);
    memberParameters.push_back(parameter);
  }

  rti::AttributeHandleSet const publisherAttributes{publisherAttribute};
  rti::InteractionClassHandleSet const publisherDirected{publisherInteraction};
  publisher.rtiAmbassador().publishObjectClassAttributes(
      publisherClass,
      publisherAttributes);
  publisher.rtiAmbassador().publishObjectClassDirectedInteractions(
      publisherClass,
      publisherDirected);
  publisher.rtiAmbassador().changeInteractionOrderType(
      publisherInteraction,
      rti::TIMESTAMP);
  for (std::size_t index = 0U; index != members.size(); ++index) {
    members.at(index)->rtiAmbassador().subscribeObjectClassAttributes(
        memberClasses.at(index),
        rti::AttributeHandleSet{memberAttributes.at(index)},
        true,
        L"");
    members.at(index)->rtiAmbassador().subscribeObjectClassDirectedInteractions(
        memberClasses.at(index),
        rti::InteractionClassHandleSet{memberInteractions.at(index)},
        true);
  }

  auto const target = publisher.rtiAmbassador().registerObjectInstance(publisherClass);
  require(
      target.isValid(),
      "custom transportation timestamped directed interaction alternate-advance registration returned an invalid handle");
  auto const targetName = publisher.rtiAmbassador().getObjectInstanceName(target);
  require(
      !targetName.empty() &&
          publisher.rtiAmbassador().getObjectInstanceHandle(targetName) == target,
      "custom transportation timestamped directed interaction alternate-advance registration did not round-trip its name");
  waitForSessions(
      {&publisher, &flushQueue, &timeAdvanceAvailable, &nextMessageAvailable},
      [&] {
        return flushQueue.recorder().hasDiscovery(target) &&
            timeAdvanceAvailable.recorder().hasDiscovery(target) &&
            nextMessageAvailable.recorder().hasDiscovery(target);
      },
      alternate,
      "custom transportation timestamped directed interaction alternate-advance target discovery");

  auto publisherTime = makeTimeContext(publisher);
  auto flushQueueTime = makeTimeContext(flushQueue);
  auto timeAdvanceAvailableTime = makeTimeContext(timeAdvanceAvailable);
  auto nextMessageAvailableTime = makeTimeContext(nextMessageAvailable);
  require(
      publisherTime.factory->getName() == flushQueueTime.factory->getName() &&
          publisherTime.factory->getName() == timeAdvanceAvailableTime.factory->getName() &&
          publisherTime.factory->getName() == nextMessageAvailableTime.factory->getName(),
      "custom transportation timestamped directed interaction alternate-advance members selected different logical-time factories");

  for (auto* member : members) {
    member->rtiAmbassador().enableTimeConstrained();
    waitFor(
        *member,
        [&] { return member->recorder().timeConstrainedEnabled().size() >= 1U; },
        alternate,
        "custom transportation timestamped directed interaction alternate-advance time-constrained callback");
  }
  publisher.rtiAmbassador().enableTimeRegulation(*publisherTime.epsilon);
  waitFor(
      publisher,
      [&] { return publisher.recorder().timeRegulationEnabled().size() >= 1U; },
      alternate,
      "custom transportation timestamped directed interaction alternate-advance time-regulation callback");

  auto const messageTime = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      5U);
  auto const publisherTarget = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      4U);
  auto const requestBoundary = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      10U);
  auto const parameterBytes = copyBytes(rti::HLAinteger32BE{42}.encode());
  std::vector<std::uint8_t> const tagBytes{
      0x44U,
      0x49U,
      0x52U,
      0x2DU,
      0x41U};
  rti::ParameterHandleValueMap parameters;
  parameters.emplace(
      publisherParameter,
      rti::VariableLengthData(parameterBytes.data(), parameterBytes.size()));
  rti::VariableLengthData const tag(tagBytes.data(), tagBytes.size());
  auto const retraction = publisher.rtiAmbassador().sendDirectedInteraction(
      publisherInteraction,
      target,
      parameters,
      tag,
      *messageTime);
  require(
      retraction.isValid(),
      "custom transportation timestamped directed interaction alternate-advance send returned an invalid retraction handle");
  for (auto* member : members) {
    require(
        member->recorder().timedDirectedInteractions().empty(),
        "custom transportation timestamped directed interaction alternate-advance delivered before its request");
    member->recorder().clearCallbackOrder();
  }
  publisher.recorder().clearCallbackOrder();

  flushQueue.rtiAmbassador().flushQueueRequest(*requestBoundary);
  timeAdvanceAvailable.rtiAmbassador().timeAdvanceRequestAvailable(*messageTime);
  nextMessageAvailable.rtiAmbassador().nextMessageRequestAvailable(*requestBoundary);
  publisher.rtiAmbassador().timeAdvanceRequest(*publisherTarget);
  waitForSessions(
      {&publisher, &flushQueue, &timeAdvanceAvailable, &nextMessageAvailable},
      [&] {
        return publisher.recorder().timeAdvanceGrants().size() >= 1U &&
            flushQueue.recorder().timedDirectedInteractions().size() >= 1U &&
            flushQueue.recorder().flushQueueGrants().size() >= 1U &&
            timeAdvanceAvailable.recorder().timedDirectedInteractions().size() >= 1U &&
            timeAdvanceAvailable.recorder().timeAdvanceGrants().size() >= 1U &&
            nextMessageAvailable.recorder().timedDirectedInteractions().size() >= 1U &&
            nextMessageAvailable.recorder().timeAdvanceGrants().size() >= 1U;
      },
      alternate,
      "custom transportation timestamped directed interaction alternate-advance delivery and grants");

  require(
      flushQueue.recorder().timedDirectedInteractions().size() == 1U &&
          flushQueue.recorder().flushQueueGrants().size() == 1U &&
          flushQueue.recorder().timeAdvanceGrants().empty(),
      "Flush Queue Request did not produce exactly one custom directed interaction and flush grant");
  require(
      timeAdvanceAvailable.recorder().timedDirectedInteractions().size() == 1U &&
          timeAdvanceAvailable.recorder().timeAdvanceGrants().size() == 1U &&
          timeAdvanceAvailable.recorder().flushQueueGrants().empty(),
      "Time Advance Request Available did not produce exactly one custom directed interaction and grant");
  require(
      nextMessageAvailable.recorder().timedDirectedInteractions().size() == 1U &&
          nextMessageAvailable.recorder().timeAdvanceGrants().size() == 1U &&
          nextMessageAvailable.recorder().flushQueueGrants().empty(),
      "Next Message Request Available did not produce exactly one custom directed interaction and grant");
  require(
      flushQueue.recorder().callbackOrder() ==
          std::vector<std::string>{"directed", "flush-grant"},
      "Flush Queue Request delivered its custom directed interaction after the flush grant");
  require(
      timeAdvanceAvailable.recorder().callbackOrder() ==
          std::vector<std::string>{"directed", "grant"},
      "Time Advance Request Available delivered its custom directed interaction after the grant");
  require(
      nextMessageAvailable.recorder().callbackOrder() ==
          std::vector<std::string>{"directed", "grant"},
      "Next Message Request Available delivered its custom directed interaction after the grant");

  auto assertDirected = [&](TimedDirectedInteractionRecord const& record,
                            rti::InteractionClassHandle const& expectedInteraction,
                            rti::ParameterHandle const& expectedParameter,
                            Session& inspector) {
    require(
        record.interaction == expectedInteraction && record.object == target &&
            record.parameters.size() == 1U &&
            record.parameters.count(expectedParameter) == 1U &&
            copyBytes(record.parameters.at(expectedParameter)) == parameterBytes,
        "custom transportation timestamped directed interaction alternate-advance callback changed the target or payload");
    require(
        record.tag == tagBytes && record.time == encodeTime(*messageTime) &&
            !record.timeText.empty() &&
            record.producer == publisher.federateHandle(),
        "custom transportation timestamped directed interaction alternate-advance callback changed standard metadata");
    require(
        record.sentOrder == rti::TIMESTAMP &&
            record.receivedOrder == rti::TIMESTAMP && record.transportation.isValid() &&
            inspector.rtiAmbassador().getTransportationTypeName(record.transportation) ==
                alternate.fomCustomTransportationName,
        "custom transportation timestamped directed interaction alternate-advance callback changed order or transport identity");
    require(
        record.retractionPresent &&
            record.retraction == copyBytes(retraction.encode()),
        "custom transportation timestamped directed interaction alternate-advance callback returned the wrong retraction handle");
  };
  assertDirected(
      flushQueue.recorder().timedDirectedInteractions().front(),
      memberInteractions.at(0),
      memberParameters.at(0),
      flushQueue);
  assertDirected(
      timeAdvanceAvailable.recorder().timedDirectedInteractions().front(),
      memberInteractions.at(1),
      memberParameters.at(1),
      timeAdvanceAvailable);
  assertDirected(
      nextMessageAvailable.recorder().timedDirectedInteractions().front(),
      memberInteractions.at(2),
      memberParameters.at(2),
      nextMessageAvailable);
  requireException(
      [&] { publisher.rtiAmbassador().retract(retraction); },
      L"MessageCanNoLongerBeRetracted",
      "retracting a delivered custom transportation timestamped directed interaction alternate-advance message");

  for (auto* member : members) {
    member->rtiAmbassador().disableTimeConstrained();
  }
  publisher.rtiAmbassador().disableTimeRegulation();
  for (std::size_t index = 0U; index != members.size(); ++index) {
    members.at(index)->rtiAmbassador().unsubscribeObjectClassDirectedInteractions(
        memberClasses.at(index),
        rti::InteractionClassHandleSet{memberInteractions.at(index)});
    members.at(index)->rtiAmbassador().unsubscribeObjectClassAttributes(
        memberClasses.at(index),
        rti::AttributeHandleSet{memberAttributes.at(index)});
  }
  publisher.rtiAmbassador().unpublishObjectClassDirectedInteractions(
      publisherClass,
      publisherDirected);
  publisher.rtiAmbassador().unpublishObjectClassAttributes(
      publisherClass,
      publisherAttributes);
  nextMessageAvailable.resign(rti::NO_ACTION);
  timeAdvanceAvailable.resign(rti::NO_ACTION);
  flushQueue.resign(rti::NO_ACTION);
  publisher.resign(rti::NO_ACTION);
  publisher.rtiAmbassador().destroyFederationExecution(federation);
  nextMessageAvailable.disconnect();
  timeAdvanceAvailable.disconnect();
  flushQueue.disconnect();
  publisher.disconnect();
}

void scenarioCustomTransportationTimestampedDirectedInteractionAlternateAdvancesContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioCustomTransportationTimestampedDirectedInteractionAlternateAdvances(options, model);
}

}  // namespace hla_rti_cpp_tck::detail
