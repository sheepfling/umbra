#include "tck_main_scenarios.hpp"

namespace hla_rti_cpp_tck::detail {

void scenarioAlternateTimeAdvances(Options const& options, rti::CallbackModel model) {
  Session publisher(options, model, "owner");
  Session flushQueue(options, model, "member");
  Session timeAdvanceAvailable(options, model, "member");
  Session nextMessageAvailable(options, model, "member");
  auto const federation = federationName(options, "alternate-time-advances");

  publisher.connect();
  flushQueue.connect();
  timeAdvanceAvailable.connect();
  nextMessageAvailable.connect();
  publisher.rtiAmbassador().createFederationExecution(
      federation,
      options.fom.wstring(),
      options.logicalTimeImplementationName);
  publisher.join(options.ownerFederateName, options.federateType, federation);
  flushQueue.join(
      options.memberFederateName + L"-flush-queue",
      options.federateType,
      federation);
  timeAdvanceAvailable.join(
      options.memberFederateName + L"-time-advance-available",
      options.federateType,
      federation);
  nextMessageAvailable.join(
      options.memberFederateName + L"-next-message-available",
      options.federateType,
      federation);

  auto const publisherClass = publisher.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const publisherAttribute = publisher.rtiAmbassador().getAttributeHandle(
      publisherClass, options.attributeName);
  require(
      publisherClass.isValid() && publisherAttribute.isValid(),
      "alternate time-advance publisher handle lookup returned an invalid handle");
  rti::AttributeHandleSet publisherAttributes;
  publisherAttributes.insert(publisherAttribute);
  publisher.rtiAmbassador().publishObjectClassAttributes(
      publisherClass, publisherAttributes);
  publisher.rtiAmbassador().changeDefaultAttributeOrderType(
      publisherClass, publisherAttributes, rti::TIMESTAMP);

  auto const members = std::vector<Session*>{
      &flushQueue, &timeAdvanceAvailable, &nextMessageAvailable};
  std::vector<rti::ObjectClassHandle> memberClasses;
  memberClasses.reserve(members.size());
  std::vector<rti::AttributeHandle> memberAttributes;
  memberAttributes.reserve(members.size());
  for (auto* member : members) {
    auto const memberClass = member->rtiAmbassador().getObjectClassHandle(
        options.objectClassName);
    auto const memberAttribute = member->rtiAmbassador().getAttributeHandle(
        memberClass, options.attributeName);
    require(
        memberClass.isValid() && memberAttribute.isValid(),
        "alternate time-advance member handle lookup returned an invalid handle");
    rti::AttributeHandleSet attributes;
    attributes.insert(memberAttribute);
    member->rtiAmbassador().subscribeObjectClassAttributes(
        memberClass, attributes, true, L"");
    memberClasses.push_back(memberClass);
    memberAttributes.push_back(memberAttribute);
  }

  auto const object = publisher.rtiAmbassador().registerObjectInstance(publisherClass);
  require(object.isValid(), "alternate time-advance registration returned an invalid handle");
  for (auto* member : members) {
    waitFor(
        *member,
        [&] { return member->recorder().hasDiscovery(object); },
        options,
        "alternate time-advance object discovery");
  }

  auto publisherTime = makeTimeContext(publisher);
  auto flushQueueTime = makeTimeContext(flushQueue);
  auto timeAdvanceAvailableTime = makeTimeContext(timeAdvanceAvailable);
  auto nextMessageAvailableTime = makeTimeContext(nextMessageAvailable);
  require(
      publisherTime.factory->getName() == flushQueueTime.factory->getName() &&
          publisherTime.factory->getName() == timeAdvanceAvailableTime.factory->getName() &&
          publisherTime.factory->getName() == nextMessageAvailableTime.factory->getName(),
      "alternate time-advance members selected different logical-time factories");

  for (auto* member : members) {
    member->rtiAmbassador().enableTimeConstrained();
    waitFor(
        *member,
        [&] { return member->recorder().timeConstrainedEnabled().size() >= 1U; },
        options,
        "alternate time-advance time-constrained callback");
  }
  publisher.rtiAmbassador().enableTimeRegulation(*publisherTime.epsilon);
  waitFor(
      publisher,
      [&] { return publisher.recorder().timeRegulationEnabled().size() >= 1U; },
      options,
      "alternate time-advance time-regulation callback");

  auto messageTime = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      3U);
  auto publisherTarget = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      2U);
  auto requestBoundary = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      5U);
  std::vector<std::uint8_t> valueBytes{0x61U, 0x62U, 0x63U};
  std::vector<std::uint8_t> tagBytes{0x41U, 0x56U, 0x41U};
  rti::AttributeHandleValueMap values;
  values.emplace(
      publisherAttribute,
      rti::VariableLengthData(valueBytes.data(), valueBytes.size()));
  rti::VariableLengthData tag(tagBytes.data(), tagBytes.size());
  auto const retraction = publisher.rtiAmbassador().updateAttributeValues(
      object,
      values,
      tag,
      *messageTime);
  require(
      retraction.isValid(),
      "alternate time-advance timestamped update returned an invalid retraction handle");
  for (auto* member : members) {
    require(
        member->recorder().timedReflections().empty(),
        "alternate time-advance recipient received a reflection before its request");
    member->recorder().clearCallbackOrder();
  }
  publisher.recorder().clearCallbackOrder();

  flushQueue.rtiAmbassador().flushQueueRequest(*requestBoundary);
  timeAdvanceAvailable.rtiAmbassador().timeAdvanceRequestAvailable(*messageTime);
  nextMessageAvailable.rtiAmbassador().nextMessageRequestAvailable(*requestBoundary);
  publisher.rtiAmbassador().timeAdvanceRequest(*publisherTarget);

  auto const deadline = Clock::now() +
      std::chrono::milliseconds(options.timeoutMilliseconds);
  while (Clock::now() < deadline) {
    publisher.pump();
    flushQueue.pump();
    timeAdvanceAvailable.pump();
    nextMessageAvailable.pump();
    if (publisher.recorder().timeAdvanceGrants().size() >= 1U &&
        flushQueue.recorder().timedReflections().size() >= 1U &&
        flushQueue.recorder().flushQueueGrants().size() >= 1U &&
        timeAdvanceAvailable.recorder().timedReflections().size() >= 1U &&
        timeAdvanceAvailable.recorder().timeAdvanceGrants().size() >= 1U &&
        nextMessageAvailable.recorder().timedReflections().size() >= 1U &&
        nextMessageAvailable.recorder().timeAdvanceGrants().size() >= 1U) {
      break;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
  require(
      publisher.recorder().timeAdvanceGrants().size() == 1U,
      "alternate time-advance publisher received the wrong number of grants");
  require(
      flushQueue.recorder().timedReflections().size() == 1U &&
          flushQueue.recorder().flushQueueGrants().size() == 1U &&
          flushQueue.recorder().timeAdvanceGrants().empty(),
      "Flush Queue Request did not produce exactly its standard callback pair");
  require(
      timeAdvanceAvailable.recorder().timedReflections().size() == 1U &&
          timeAdvanceAvailable.recorder().timeAdvanceGrants().size() == 1U &&
          timeAdvanceAvailable.recorder().flushQueueGrants().empty(),
      "Time Advance Request Available did not produce exactly its standard callback pair");
  require(
      nextMessageAvailable.recorder().timedReflections().size() == 1U &&
          nextMessageAvailable.recorder().timeAdvanceGrants().size() == 1U &&
          nextMessageAvailable.recorder().flushQueueGrants().empty(),
      "Next Message Request Available did not produce exactly its standard callback pair");
  require(
      flushQueue.recorder().callbackOrder() ==
          std::vector<std::string>{"reflect", "flush-grant"},
      "Flush Queue Request delivered its grant before the timestamped reflection");
  require(
      timeAdvanceAvailable.recorder().callbackOrder() ==
          std::vector<std::string>{"reflect", "grant"},
      "Time Advance Request Available granted before its already-queued equal-time TSO");
  require(
      nextMessageAvailable.recorder().callbackOrder() ==
          std::vector<std::string>{"reflect", "grant"},
      "Next Message Request Available delivered its grant before the timestamped reflection");

  auto assertReflection = [&](TimedReflectionRecord const& record,
                              rti::AttributeHandle const& expectedAttribute,
                              Session& inspector) {
    require(record.object == object,
            "alternate time-advance reflection returned the wrong object handle");
    require(record.values.size() == 1U && record.values.count(expectedAttribute) == 1U,
            "alternate time-advance reflection returned the wrong attribute set");
    require(copyBytes(record.values.at(expectedAttribute)) == valueBytes,
            "alternate time-advance reflection returned the wrong attribute value");
    require(record.tag == tagBytes,
            "alternate time-advance reflection did not preserve the user tag");
    require(record.producer == publisher.federateHandle(),
            "alternate time-advance reflection returned the wrong producer");
    require(record.transportation.isValid() &&
                !inspector.rtiAmbassador()
                     .getTransportationTypeName(record.transportation)
                     .empty(),
            "alternate time-advance reflection returned an unresolvable transportation type");
    require(record.time == encodeTime(*messageTime),
            "alternate time-advance reflection returned the wrong logical time");
    require(!record.timeText.empty(),
            "alternate time-advance reflection returned no logical-time representation");
    require(record.sentOrder == rti::TIMESTAMP && record.receivedOrder == rti::TIMESTAMP,
            "alternate time-advance reflection returned the wrong order metadata");
    require(
        record.retractionPresent && record.retraction == copyBytes(retraction.encode()),
        "alternate time-advance reflection returned the wrong retraction handle");
  };
  assertReflection(flushQueue.recorder().timedReflections().front(), memberAttributes.at(0), flushQueue);
  assertReflection(
      timeAdvanceAvailable.recorder().timedReflections().front(),
      memberAttributes.at(1),
      timeAdvanceAvailable);
  assertReflection(
      nextMessageAvailable.recorder().timedReflections().front(),
      memberAttributes.at(2),
      nextMessageAvailable);

  auto const flushGrants = flushQueue.recorder().flushQueueGrants();
  require(
      flushGrants.front().optimisticTime == encodeTime(*messageTime),
      "Flush Queue Grant returned the wrong optimistic logical time");
  require(
      !flushGrants.front().time.empty() && !flushGrants.front().timeText.empty() &&
          !flushGrants.front().optimisticTimeText.empty(),
      "Flush Queue Grant returned an invalid actual logical-time encoding");
  auto actualFlushTime = decodeTime(
      *flushQueueTime.factory,
      flushGrants.front().time);
  auto optimisticFlushTime = decodeTime(
      *flushQueueTime.factory,
      flushGrants.front().optimisticTime);
  require(
      actualFlushTime != nullptr && optimisticFlushTime != nullptr &&
          !(*actualFlushTime < *flushQueueTime.initial) &&
          !(*requestBoundary < *actualFlushTime) &&
          !(*optimisticFlushTime < *actualFlushTime),
      "Flush Queue Grant returned an actual time outside its standard bounds");
  require(
      timeAdvanceAvailable.recorder().timeAdvanceGrants().front().encoded ==
          encodeTime(*messageTime),
      "Time Advance Request Available returned the wrong grant time");
  require(
      nextMessageAvailable.recorder().timeAdvanceGrants().front().encoded ==
          encodeTime(*messageTime),
      "Next Message Request Available returned the wrong grant time");
  auto queriedFlushQueue = flushQueueTime.factory->makeInitial();
  flushQueue.rtiAmbassador().queryLogicalTime(*queriedFlushQueue);
  require(
      encodeTime(*queriedFlushQueue) == flushGrants.front().time,
      "Flush Queue Request query did not match its actual grant time");
  auto queriedTimeAdvanceAvailable = timeAdvanceAvailableTime.factory->makeInitial();
  timeAdvanceAvailable.rtiAmbassador().queryLogicalTime(*queriedTimeAdvanceAvailable);
  requireTimeEquals(
      *queriedTimeAdvanceAvailable,
      *messageTime,
      "Time Advance Request Available logical-time query");
  auto queriedNextMessageAvailable = nextMessageAvailableTime.factory->makeInitial();
  nextMessageAvailable.rtiAmbassador().queryLogicalTime(*queriedNextMessageAvailable);
  requireTimeEquals(
      *queriedNextMessageAvailable,
      *messageTime,
      "Next Message Request Available logical-time query");
  auto const pastFlushQueueTime = timeAfter(
      *flushQueueTime.factory,
      *flushQueueTime.initial,
      *flushQueueTime.epsilon,
      1U);
  auto const pastTimeAdvanceAvailableTime = timeAfter(
      *timeAdvanceAvailableTime.factory,
      *timeAdvanceAvailableTime.initial,
      *timeAdvanceAvailableTime.epsilon,
      1U);
  auto const pastNextMessageAvailableTime = timeAfter(
      *nextMessageAvailableTime.factory,
      *nextMessageAvailableTime.initial,
      *nextMessageAvailableTime.epsilon,
      1U);
  requireException(
      [&] { flushQueue.rtiAmbassador().flushQueueRequest(*pastFlushQueueTime); },
      L"LogicalTimeAlreadyPassed",
      "Flush Queue Request with a time before the current logical time");
  requireException(
      [&] {
        timeAdvanceAvailable.rtiAmbassador().timeAdvanceRequestAvailable(
            *pastTimeAdvanceAvailableTime);
      },
      L"LogicalTimeAlreadyPassed",
      "Time Advance Request Available with a time before the current logical time");
  requireException(
      [&] {
        nextMessageAvailable.rtiAmbassador().nextMessageRequestAvailable(
            *pastNextMessageAvailableTime);
      },
      L"LogicalTimeAlreadyPassed",
      "Next Message Request Available with a time before the current logical time");
  requireException(
      [&] { publisher.rtiAmbassador().retract(retraction); },
      L"MessageCanNoLongerBeRetracted",
      "retracting a timestamped update delivered by alternate time advances");

  for (auto* member : members) {
    member->rtiAmbassador().disableTimeConstrained();
  }
  publisher.rtiAmbassador().disableTimeRegulation();
  for (std::size_t index = 0U; index != members.size(); ++index) {
    auto* member = members.at(index);
    rti::AttributeHandleSet attributes;
    attributes.insert(memberAttributes.at(index));
    member->rtiAmbassador().unsubscribeObjectClassAttributes(
        memberClasses.at(index), attributes);
  }
  publisher.rtiAmbassador().unpublishObjectClassAttributes(
      publisherClass, publisherAttributes);
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

void scenarioTimestampedDirectedAlternateAdvances(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.logicalTimeImplementationName.empty(),
      "Timestamped directed alternate advances require an adapter-supplied logical-time implementation");

  Session publisher(options, model, "owner");
  Session flushQueue(options, model, "member");
  Session timeAdvanceAvailable(options, model, "member");
  Session nextMessageAvailable(options, model, "member");
  auto const federation = federationName(options, "timestamped-directed-alternate-advances");

  publisher.connect();
  flushQueue.connect();
  timeAdvanceAvailable.connect();
  nextMessageAvailable.connect();
  publisher.rtiAmbassador().createFederationExecution(
      federation,
      options.fom.wstring(),
      options.logicalTimeImplementationName);
  publisher.join(options.ownerFederateName, options.federateType, federation);
  flushQueue.join(
      options.memberFederateName + L"-directed-flush-queue",
      options.federateType,
      federation);
  timeAdvanceAvailable.join(
      options.memberFederateName + L"-directed-time-advance-available",
      options.federateType,
      federation);
  nextMessageAvailable.join(
      options.memberFederateName + L"-directed-next-message-available",
      options.federateType,
      federation);

  auto const publisherClass = publisher.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const publisherAttribute = publisher.rtiAmbassador().getAttributeHandle(
      publisherClass,
      options.attributeName);
  auto const publisherInteraction = publisher.rtiAmbassador().getInteractionClassHandle(
      options.interactionClassName);
  auto const publisherParameter = publisher.rtiAmbassador().getParameterHandle(
      publisherInteraction,
      options.parameterName);
  require(
      publisherClass.isValid() && publisherAttribute.isValid() &&
          publisherInteraction.isValid() && publisherParameter.isValid(),
      "timestamped directed alternate-advance publisher lookup returned an invalid handle");

  rti::AttributeHandleSet const publisherAttributes{publisherAttribute};
  publisher.rtiAmbassador().publishObjectClassAttributes(
      publisherClass,
      publisherAttributes);
  rti::InteractionClassHandleSet const publisherDirected{publisherInteraction};
  publisher.rtiAmbassador().publishObjectClassDirectedInteractions(
      publisherClass,
      publisherDirected);
  publisher.rtiAmbassador().changeInteractionOrderType(
      publisherInteraction,
      rti::TIMESTAMP);

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
    auto const memberClass = member->rtiAmbassador().getObjectClassHandle(
        options.objectClassName);
    auto const memberAttribute = member->rtiAmbassador().getAttributeHandle(
        memberClass,
        options.attributeName);
    auto const memberInteraction = member->rtiAmbassador().getInteractionClassHandle(
        options.interactionClassName);
    auto const memberParameter = member->rtiAmbassador().getParameterHandle(
        memberInteraction,
        options.parameterName);
    require(
        memberClass.isValid() && memberAttribute.isValid() &&
            memberInteraction.isValid() && memberParameter.isValid(),
        "timestamped directed alternate-advance member lookup returned an invalid handle");
    member->rtiAmbassador().subscribeObjectClassAttributes(
        memberClass,
        rti::AttributeHandleSet{memberAttribute},
        true,
        L"");
    member->rtiAmbassador().subscribeObjectClassDirectedInteractions(
        memberClass,
        rti::InteractionClassHandleSet{memberInteraction},
        true);
    memberClasses.push_back(memberClass);
    memberAttributes.push_back(memberAttribute);
    memberInteractions.push_back(memberInteraction);
    memberParameters.push_back(memberParameter);
  }

  auto const target = publisher.rtiAmbassador().registerObjectInstance(publisherClass);
  require(
      target.isValid(),
      "timestamped directed alternate-advance registration returned an invalid handle");
  for (auto* member : members) {
    waitFor(
        *member,
        [&] { return member->recorder().hasDiscovery(target); },
        options,
        "timestamped directed alternate-advance object discovery");
  }

  auto publisherTime = makeTimeContext(publisher);
  auto flushQueueTime = makeTimeContext(flushQueue);
  auto timeAdvanceAvailableTime = makeTimeContext(timeAdvanceAvailable);
  auto nextMessageAvailableTime = makeTimeContext(nextMessageAvailable);
  require(
      publisherTime.factory->getName() == flushQueueTime.factory->getName() &&
          publisherTime.factory->getName() == timeAdvanceAvailableTime.factory->getName() &&
          publisherTime.factory->getName() == nextMessageAvailableTime.factory->getName(),
      "timestamped directed alternate-advance members selected different logical-time factories");

  for (auto* member : members) {
    member->rtiAmbassador().enableTimeConstrained();
    waitFor(
        *member,
        [&] { return member->recorder().timeConstrainedEnabled().size() >= 1U; },
        options,
        "timestamped directed alternate-advance time-constrained callback");
  }
  publisher.rtiAmbassador().enableTimeRegulation(*publisherTime.epsilon);
  waitFor(
      publisher,
      [&] { return publisher.recorder().timeRegulationEnabled().size() >= 1U; },
      options,
      "timestamped directed alternate-advance time-regulation callback");

  auto const messageTime = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      3U);
  auto const publisherTargetTime = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      2U);
  auto const requestBoundary = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      5U);
  std::vector<std::uint8_t> const parameterBytes{0x61U, 0x62U, 0x63U};
  std::vector<std::uint8_t> const tagBytes{0x41U, 0x44U, 0x56U};
  rti::ParameterHandleValueMap parameters;
  parameters.emplace(
      publisherParameter,
      rti::VariableLengthData(parameterBytes.data(), parameterBytes.size()));
  rti::VariableLengthData tag(tagBytes.data(), tagBytes.size());
  auto const retraction = publisher.rtiAmbassador().sendDirectedInteraction(
      publisherInteraction,
      target,
      parameters,
      tag,
      *messageTime);
  require(
      retraction.isValid(),
      "timestamped directed alternate-advance send returned an invalid retraction handle");
  for (auto* member : members) {
    require(
        member->recorder().timedDirectedInteractions().empty(),
        "timestamped directed alternate-advance delivery occurred before its request");
    member->recorder().clearCallbackOrder();
  }
  publisher.recorder().clearCallbackOrder();

  publisher.rtiAmbassador().timeAdvanceRequest(*publisherTargetTime);
  waitFor(
      publisher,
      [&] { return publisher.recorder().timeAdvanceGrants().size() >= 1U; },
      options,
      "timestamped directed alternate-advance producer pre-message grant");
  flushQueue.rtiAmbassador().flushQueueRequest(*requestBoundary);
  timeAdvanceAvailable.rtiAmbassador().timeAdvanceRequestAvailable(*requestBoundary);
  nextMessageAvailable.rtiAmbassador().nextMessageRequestAvailable(*requestBoundary);

  auto const deadline = Clock::now() +
      std::chrono::milliseconds(options.timeoutMilliseconds);
  while (Clock::now() < deadline) {
    publisher.pump();
    flushQueue.pump();
    timeAdvanceAvailable.pump();
    nextMessageAvailable.pump();
    if (publisher.recorder().timeAdvanceGrants().size() >= 1U &&
        flushQueue.recorder().timedDirectedInteractions().size() >= 1U &&
        flushQueue.recorder().flushQueueGrants().size() >= 1U &&
        timeAdvanceAvailable.recorder().timedDirectedInteractions().size() >= 1U &&
        timeAdvanceAvailable.recorder().timeAdvanceGrants().size() >= 1U &&
        nextMessageAvailable.recorder().timedDirectedInteractions().size() >= 1U &&
        nextMessageAvailable.recorder().timeAdvanceGrants().size() >= 1U) {
      break;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }

  require(
      publisher.recorder().timeAdvanceGrants().size() == 1U,
      "timestamped directed alternate-advance publisher received the wrong grant count");
  require(
      flushQueue.recorder().timedDirectedInteractions().size() == 1U &&
          flushQueue.recorder().flushQueueGrants().size() == 1U &&
          flushQueue.recorder().timeAdvanceGrants().empty(),
      "Flush Queue Request did not produce exactly one directed interaction and flush grant");
  require(
      timeAdvanceAvailable.recorder().timedDirectedInteractions().size() == 1U &&
          timeAdvanceAvailable.recorder().timeAdvanceGrants().size() == 1U &&
          timeAdvanceAvailable.recorder().flushQueueGrants().empty(),
      "Time Advance Request Available did not produce exactly one directed interaction and grant");
  require(
      nextMessageAvailable.recorder().timedDirectedInteractions().size() == 1U &&
          nextMessageAvailable.recorder().timeAdvanceGrants().size() == 1U &&
          nextMessageAvailable.recorder().flushQueueGrants().empty(),
      "Next Message Request Available did not produce exactly one directed interaction and grant");
  require(
      flushQueue.recorder().callbackOrder() ==
          std::vector<std::string>{"directed", "flush-grant"},
      "Flush Queue Request delivered its directed interaction after the flush grant");
  require(
      timeAdvanceAvailable.recorder().callbackOrder() ==
          std::vector<std::string>{"directed", "grant"},
      "Time Advance Request Available granted before its strictly earlier directed TSO");
  require(
      nextMessageAvailable.recorder().callbackOrder() ==
          std::vector<std::string>{"directed", "grant"},
      "Next Message Request Available delivered its directed interaction after the grant");

  auto assertDirected = [&](TimedDirectedInteractionRecord const& record,
                            rti::InteractionClassHandle const& expectedInteraction,
                            rti::ParameterHandle const& expectedParameter,
                            Session& receiver) {
    require(
        record.interaction == expectedInteraction && record.object == target,
        "timestamped directed alternate-advance callback returned the wrong interaction or target");
    require(
        record.parameters.size() == 1U && record.parameters.count(expectedParameter) == 1U,
        "timestamped directed alternate-advance callback returned the wrong parameter set");
    require(
        copyBytes(record.parameters.at(expectedParameter)) == parameterBytes,
        "timestamped directed alternate-advance callback returned the wrong parameter value");
    require(
        record.tag == tagBytes,
        "timestamped directed alternate-advance callback did not preserve the user tag");
    require(
        record.producer == publisher.federateHandle(),
        "timestamped directed alternate-advance callback returned the wrong producer");
    require(
        record.transportation.isValid() &&
            !receiver.rtiAmbassador().getTransportationTypeName(record.transportation).empty(),
        "timestamped directed alternate-advance callback returned an unresolvable transportation type");
    require(
        record.time == encodeTime(*messageTime) && !record.timeText.empty(),
        "timestamped directed alternate-advance callback returned the wrong logical time");
    require(
        record.sentOrder == rti::TIMESTAMP && record.receivedOrder == rti::TIMESTAMP,
        "timestamped directed alternate-advance callback returned the wrong order metadata");
    require(
        record.retractionPresent && record.retraction == copyBytes(retraction.encode()),
        "timestamped directed alternate-advance callback returned the wrong retraction handle");
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
  require(
      flushQueue.recorder().flushQueueGrants().front().optimisticTime ==
          encodeTime(*messageTime),
      "Flush Queue Grant returned the wrong optimistic logical time for directed delivery");
  auto actualAvailableGrant = decodeTime(
      *timeAdvanceAvailableTime.factory,
      timeAdvanceAvailable.recorder().timeAdvanceGrants().front().encoded);
  require(
      actualAvailableGrant != nullptr &&
          !(*actualAvailableGrant < *messageTime) &&
          !(*requestBoundary < *actualAvailableGrant) &&
          nextMessageAvailable.recorder().timeAdvanceGrants().front().encoded ==
              encodeTime(*messageTime),
      "alternate directed advances returned grant times outside their standard bounds");

  for (std::size_t index = 0U; index != members.size(); ++index) {
    auto* member = members.at(index);
    member->rtiAmbassador().disableTimeConstrained();
    member->rtiAmbassador().unsubscribeObjectClassDirectedInteractions(
        memberClasses.at(index),
        rti::InteractionClassHandleSet{memberInteractions.at(index)});
    member->rtiAmbassador().unsubscribeObjectClassAttributes(
        memberClasses.at(index),
        rti::AttributeHandleSet{memberAttributes.at(index)});
    member->resign(rti::NO_ACTION);
  }
  publisher.rtiAmbassador().disableTimeRegulation();
  publisher.rtiAmbassador().unpublishObjectClassDirectedInteractions(
      publisherClass,
      publisherDirected);
  publisher.rtiAmbassador().unpublishObjectClassAttributes(
      publisherClass,
      publisherAttributes);
  publisher.resign(rti::DELETE_OBJECTS);
  publisher.rtiAmbassador().destroyFederationExecution(federation);
  nextMessageAvailable.disconnect();
  timeAdvanceAvailable.disconnect();
  flushQueue.disconnect();
  publisher.disconnect();
}

void scenarioTimestampedDirectedAlternateAdvancesContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioTimestampedDirectedAlternateAdvances(options, model);
}

void scenarioNextMessageRequest(Options const& options, rti::CallbackModel model) {
  Session publisher(options, model, "owner");
  Session receiver(options, model, "member");
  auto const federation = federationName(options, "next-message-request");
  connectAndJoin(publisher, receiver, options, federation, options.fom);

  auto const publisherInteraction = publisher.rtiAmbassador().getInteractionClassHandle(
      options.interactionClassName);
  auto const publisherParameter = publisher.rtiAmbassador().getParameterHandle(
      publisherInteraction, options.parameterName);
  auto const receiverInteraction = receiver.rtiAmbassador().getInteractionClassHandle(
      options.interactionClassName);
  auto const receiverParameter = receiver.rtiAmbassador().getParameterHandle(
      receiverInteraction, options.parameterName);
  require(
      publisherInteraction.isValid() && publisherParameter.isValid() &&
          receiverInteraction.isValid() && receiverParameter.isValid(),
      "Next Message Request interaction handle lookup returned an invalid handle");
  require(
      publisherInteraction == receiverInteraction && publisherParameter == receiverParameter,
      "Next Message Request interaction handles did not retain identity across members");

  publisher.rtiAmbassador().publishInteractionClass(publisherInteraction);
  publisher.rtiAmbassador().changeInteractionOrderType(
      publisherInteraction, rti::TIMESTAMP);
  receiver.rtiAmbassador().subscribeInteractionClass(receiverInteraction, true);

  auto publisherTime = makeTimeContext(publisher);
  auto receiverTime = makeTimeContext(receiver);
  enableTimestampedRoles(
      publisher,
      receiver,
      publisherTime,
      receiverTime,
      options,
      "next-message-request");
  publisher.recorder().clearTimeCallbacks();
  receiver.recorder().clearTimeCallbacks();
  publisher.recorder().clearCallbackOrder();
  receiver.recorder().clearCallbackOrder();

  auto const messageTime = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      3U);
  auto const requestBoundary = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      5U);
  auto const publisherTarget = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      2U);
  std::vector<std::uint8_t> valueBytes{0xC1U, 0xD2U};
  std::vector<std::uint8_t> tagBytes{0x4EU, 0x4DU, 0x52U};
  rti::ParameterHandleValueMap parameters;
  parameters.emplace(
      publisherParameter,
      rti::VariableLengthData(valueBytes.data(), valueBytes.size()));
  rti::VariableLengthData tag(tagBytes.data(), tagBytes.size());
  auto const retraction = publisher.rtiAmbassador().sendInteraction(
      publisherInteraction,
      parameters,
      tag,
      *messageTime);
  require(
      retraction.isValid(),
      "Next Message Request timestamped interaction returned an invalid retraction handle");
  require(
      receiver.recorder().timedInteractions().empty(),
      "Next Message Request delivered a timestamped interaction before the request");

  receiver.rtiAmbassador().nextMessageRequest(*requestBoundary);
  require(
      receiver.recorder().timeAdvanceGrants().empty(),
      "Next Message Request completed before the regulator reached the queued message");
  publisher.rtiAmbassador().timeAdvanceRequest(*publisherTarget);
  waitFor(
      publisher,
      receiver,
      [&] {
        return publisher.recorder().timeAdvanceGrants().size() >= 1U &&
            receiver.recorder().timeAdvanceGrants().size() >= 1U &&
            receiver.recorder().timedInteractions().size() >= 1U;
      },
      options,
      "Next Message Request grant and interaction");

  require(
      publisher.recorder().timeAdvanceGrants().size() == 1U,
      "Next Message Request publisher received the wrong number of grants");
  require(
      receiver.recorder().timeAdvanceGrants().size() == 1U &&
          receiver.recorder().timedInteractions().size() == 1U,
      "Next Message Request receiver received the wrong callback counts");
  require(
      receiver.recorder().callbackOrder() ==
          std::vector<std::string>{"interaction", "grant"},
      "Next Message Request delivered its grant before the queued interaction");

  auto const received = receiver.recorder().timedInteractions().front();
  require(
      received.interaction == receiverInteraction && received.parameters.size() == 1U &&
          received.parameters.count(receiverParameter) == 1U &&
          copyBytes(received.parameters.at(receiverParameter)) == valueBytes,
      "Next Message Request delivered the wrong interaction or parameter value");
  require(
      received.tag == tagBytes,
      "Next Message Request did not preserve the interaction tag");
  require(
      received.transportation.isValid() && received.producer == publisher.federateHandle(),
      "Next Message Request returned invalid interaction metadata");
  require(
      received.time == encodeTime(*messageTime) && !received.timeText.empty(),
      "Next Message Request returned the wrong logical time");
  require(
      received.sentOrder == rti::TIMESTAMP && received.receivedOrder == rti::TIMESTAMP,
      "Next Message Request returned the wrong order metadata");
  require(
      received.retractionPresent && received.retraction == copyBytes(retraction.encode()),
      "Next Message Request returned the wrong retraction handle");
  require(
      receiver.recorder().timeAdvanceGrants().front().encoded == encodeTime(*messageTime),
      "Next Message Request returned the wrong grant time");

  auto queriedTime = receiverTime.factory->makeInitial();
  receiver.rtiAmbassador().queryLogicalTime(*queriedTime);
  requireTimeEquals(
      *queriedTime,
      *messageTime,
      "Next Message Request logical-time query");
  requireException(
      [&] { publisher.rtiAmbassador().retract(retraction); },
      L"MessageCanNoLongerBeRetracted",
      "retracting an interaction delivered by Next Message Request");

  receiver.rtiAmbassador().disableTimeConstrained();
  publisher.rtiAmbassador().disableTimeRegulation();
  receiver.rtiAmbassador().unsubscribeInteractionClass(receiverInteraction);
  publisher.rtiAmbassador().unpublishInteractionClass(publisherInteraction);
  receiver.resign(rti::NO_ACTION);
  publisher.resign(rti::NO_ACTION);
  publisher.rtiAmbassador().destroyFederationExecution(federation);
  receiver.disconnect();
  publisher.disconnect();
}

void scenarioNextMessageRequestContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioNextMessageRequest(options, model);
}

void scenarioAvailableTimeAdvancesInclusiveGalt(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.logicalTimeImplementationName.empty(),
      "Available time advances require an adapter-supplied logical-time implementation");

  Session publisher(options, model, "owner");
  Session receiver(options, model, "member");
  auto const federation = federationName(options, "available-time-advances-inclusive-galt");
  connectAndJoin(publisher, receiver, options, federation, options.fom);

  auto const publisherInteraction = publisher.rtiAmbassador().getInteractionClassHandle(
      options.interactionClassName);
  auto const publisherParameter = publisher.rtiAmbassador().getParameterHandle(
      publisherInteraction,
      options.parameterName);
  auto const receiverInteraction = receiver.rtiAmbassador().getInteractionClassHandle(
      options.interactionClassName);
  auto const receiverParameter = receiver.rtiAmbassador().getParameterHandle(
      receiverInteraction,
      options.parameterName);
  require(
      publisherInteraction.isValid() && publisherParameter.isValid() &&
          receiverInteraction.isValid() && receiverParameter.isValid(),
      "inclusive-GALT interaction lookup returned an invalid handle");
  require(
      publisherInteraction == receiverInteraction && publisherParameter == receiverParameter,
      "inclusive-GALT interaction handles did not retain identity across members");

  publisher.rtiAmbassador().publishInteractionClass(publisherInteraction);
  publisher.rtiAmbassador().changeInteractionOrderType(
      publisherInteraction,
      rti::TIMESTAMP);
  receiver.rtiAmbassador().subscribeInteractionClass(receiverInteraction, true);

  auto publisherTime = makeTimeContext(publisher);
  auto receiverTime = makeTimeContext(receiver);
  require(
      publisherTime.factory->getName() == receiverTime.factory->getName(),
      "inclusive-GALT members selected different logical-time factories");

  auto const lookaheadTime = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      5U);
  auto lookahead = publisherTime.factory->makeZero();
  require(
      lookahead != nullptr,
      "inclusive-GALT test could not allocate a lookahead interval");
  lookahead->setToDifference(*lookaheadTime, *publisherTime.initial);

  receiver.rtiAmbassador().enableTimeConstrained();
  waitFor(
      receiver,
      [&] { return receiver.recorder().timeConstrainedEnabled().size() >= 1U; },
      options,
      "inclusive-GALT time-constrained callback");
  publisher.rtiAmbassador().enableTimeRegulation(*lookahead);
  waitFor(
      publisher,
      [&] { return publisher.recorder().timeRegulationEnabled().size() >= 1U; },
      options,
      "inclusive-GALT time-regulation callback");
  publisher.recorder().clearTimeCallbacks();
  receiver.recorder().clearTimeCallbacks();
  publisher.recorder().clearCallbackOrder();
  receiver.recorder().clearCallbackOrder();

  auto const firstMessageTime = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      7U);
  auto const firstPublisherTarget = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      2U);
  std::vector<std::uint8_t> firstValueBytes{0x41U, 0x56U, 0x41U};
  std::vector<std::uint8_t> firstTagBytes{0x47U, 0x41U, 0x4CU, 0x54U};
  rti::ParameterHandleValueMap firstParameters;
  firstParameters.emplace(
      publisherParameter,
      rti::VariableLengthData(firstValueBytes.data(), firstValueBytes.size()));
  rti::VariableLengthData firstTag(firstTagBytes.data(), firstTagBytes.size());
  auto const firstRetraction = publisher.rtiAmbassador().sendInteraction(
      publisherInteraction,
      firstParameters,
      firstTag,
      *firstMessageTime);
  require(
      firstRetraction.isValid(),
      "inclusive-GALT first timestamped interaction returned an invalid retraction handle");
  receiver.rtiAmbassador().timeAdvanceRequestAvailable(*firstMessageTime);
  require(
      receiver.recorder().timedInteractions().empty() &&
          receiver.recorder().timeAdvanceGrants().empty(),
      "inclusive-GALT available advance completed before the regulator reached GALT");
  publisher.rtiAmbassador().timeAdvanceRequest(*firstPublisherTarget);
  waitFor(
      publisher,
      receiver,
      [&] {
        return publisher.recorder().timeAdvanceGrants().size() >= 1U &&
            receiver.recorder().timedInteractions().size() >= 1U &&
            receiver.recorder().timeAdvanceGrants().size() >= 1U;
      },
      options,
      "inclusive-GALT time-advance-request-available delivery");
  require(
      publisher.recorder().timeAdvanceGrants().size() == 1U &&
          receiver.recorder().timedInteractions().size() == 1U &&
          receiver.recorder().timeAdvanceGrants().size() == 1U,
      "inclusive-GALT time-advance-request-available returned the wrong callback counts");
  require(
      receiver.recorder().callbackOrder() ==
          std::vector<std::string>{"interaction", "grant"},
      "inclusive-GALT available advance granted before its already-queued equal-time TSO");

  auto assertInteraction = [&](TimedInteractionRecord const& record,
                               std::vector<std::uint8_t> const& expectedValue,
                               std::vector<std::uint8_t> const& expectedTag,
                               rti::LogicalTime const& expectedTime,
                               rti::MessageRetractionHandle const& expectedRetraction,
                               std::string const& description) {
    require(
        record.interaction == receiverInteraction && record.parameters.size() == 1U &&
            record.parameters.count(receiverParameter) == 1U,
        description + " returned the wrong interaction or parameter set");
    require(
        copyBytes(record.parameters.at(receiverParameter)) == expectedValue &&
            record.tag == expectedTag,
        description + " did not preserve the parameter value or tag");
    require(
        record.producer == publisher.federateHandle() && record.transportation.isValid() &&
            !receiver.rtiAmbassador().getTransportationTypeName(record.transportation).empty(),
        description + " returned invalid producer or transportation metadata");
    require(
        record.time == encodeTime(expectedTime) && !record.timeText.empty() &&
            record.sentOrder == rti::TIMESTAMP && record.receivedOrder == rti::TIMESTAMP,
        description + " returned the wrong timestamp or order metadata");
    require(
        record.retractionPresent &&
            record.retraction == copyBytes(expectedRetraction.encode()),
        description + " omitted or changed its retraction handle");
  };
  assertInteraction(
      receiver.recorder().timedInteractions().front(),
      firstValueBytes,
      firstTagBytes,
      *firstMessageTime,
      firstRetraction,
      "inclusive-GALT time-advance-request-available interaction");
  require(
      receiver.recorder().timeAdvanceGrants().front().encoded ==
          encodeTime(*firstMessageTime),
      "inclusive-GALT time-advance-request-available returned the wrong grant time");

  publisher.recorder().clearTimeCallbacks();
  receiver.recorder().clearTimeCallbacks();
  receiver.recorder().clearTimedInteractions();
  publisher.recorder().clearCallbackOrder();
  receiver.recorder().clearCallbackOrder();
  auto const secondMessageTime = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      9U);
  auto const secondRequestBoundary = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      10U);
  auto const secondPublisherTarget = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      4U);
  std::vector<std::uint8_t> secondValueBytes{0x4EU, 0x4DU, 0x52U};
  std::vector<std::uint8_t> secondTagBytes{0x47U, 0x41U, 0x4CU, 0x54U, 0x32U};
  rti::ParameterHandleValueMap secondParameters;
  secondParameters.emplace(
      publisherParameter,
      rti::VariableLengthData(secondValueBytes.data(), secondValueBytes.size()));
  rti::VariableLengthData secondTag(secondTagBytes.data(), secondTagBytes.size());
  auto const secondRetraction = publisher.rtiAmbassador().sendInteraction(
      publisherInteraction,
      secondParameters,
      secondTag,
      *secondMessageTime);
  require(
      secondRetraction.isValid(),
      "inclusive-GALT second timestamped interaction returned an invalid retraction handle");
  receiver.rtiAmbassador().nextMessageRequestAvailable(*secondRequestBoundary);
  require(
      receiver.recorder().timedInteractions().empty() &&
          receiver.recorder().timeAdvanceGrants().empty(),
      "inclusive-GALT next-message available advance completed before the regulator reached GALT");
  publisher.rtiAmbassador().timeAdvanceRequest(*secondPublisherTarget);
  waitFor(
      publisher,
      receiver,
      [&] {
        return publisher.recorder().timeAdvanceGrants().size() >= 1U &&
            receiver.recorder().timedInteractions().size() >= 1U &&
            receiver.recorder().timeAdvanceGrants().size() >= 1U;
      },
      options,
      "inclusive-GALT next-message-request-available delivery");
  require(
      publisher.recorder().timeAdvanceGrants().size() == 1U &&
          receiver.recorder().timedInteractions().size() == 1U &&
          receiver.recorder().timeAdvanceGrants().size() == 1U,
      "inclusive-GALT next-message-request-available returned the wrong callback counts");
  require(
      receiver.recorder().callbackOrder() ==
          std::vector<std::string>{"interaction", "grant"},
      "inclusive-GALT next-message-request-available granted before delivery");
  assertInteraction(
      receiver.recorder().timedInteractions().front(),
      secondValueBytes,
      secondTagBytes,
      *secondMessageTime,
      secondRetraction,
      "inclusive-GALT next-message-request-available interaction");
  require(
      receiver.recorder().timeAdvanceGrants().front().encoded ==
          encodeTime(*secondMessageTime),
      "inclusive-GALT next-message-request-available returned the wrong grant time");

  requireException(
      [&] { publisher.rtiAmbassador().retract(firstRetraction); },
      L"MessageCanNoLongerBeRetracted",
      "retracting the delivered inclusive-GALT first interaction");
  requireException(
      [&] { publisher.rtiAmbassador().retract(secondRetraction); },
      L"MessageCanNoLongerBeRetracted",
      "retracting the delivered inclusive-GALT second interaction");

  receiver.rtiAmbassador().disableTimeConstrained();
  publisher.rtiAmbassador().disableTimeRegulation();
  receiver.rtiAmbassador().unsubscribeInteractionClass(receiverInteraction);
  publisher.rtiAmbassador().unpublishInteractionClass(publisherInteraction);
  receiver.resign(rti::NO_ACTION);
  publisher.resign(rti::NO_ACTION);
  publisher.rtiAmbassador().destroyFederationExecution(federation);
  receiver.disconnect();
  publisher.disconnect();
}

void scenarioAvailableTimeAdvancesInclusiveGaltContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioAvailableTimeAdvancesInclusiveGalt(options, model);
}

void scenarioTimeBoundsQueries(Options const& options, rti::CallbackModel model) {
  Session observer(options, model, "owner");
  Session leftRegulator(options, model, "member");
  Session rightRegulator(options, model, "second-regulator");
  auto const federation = federationName(options, "time-bounds-queries");
  observer.connect();
  leftRegulator.connect();
  rightRegulator.connect();
  observer.rtiAmbassador().createFederationExecution(
      federation,
      options.fom.wstring(),
      options.logicalTimeImplementationName);
  observer.join(options.ownerFederateName, options.federateType, federation);
  leftRegulator.join(options.memberFederateName, options.federateType, federation);
  rightRegulator.join(L"tck-second-regulator", options.federateType, federation);

  auto observerTime = makeTimeContext(observer);
  auto leftTime = makeTimeContext(leftRegulator);
  auto rightTime = makeTimeContext(rightRegulator);
  auto galt = observerTime.factory->makeInitial();
  auto lits = observerTime.factory->makeInitial();
  require(galt != nullptr && lits != nullptr,
          "Query GALT/LITS could not allocate output logical times");
  require(
      !observer.rtiAmbassador().queryGALT(*galt),
      "Query GALT was defined before any other regulator became active");
  require(
      !observer.rtiAmbassador().queryLITS(*lits),
      "Query LITS was defined before any other regulator became active");

  auto const makeLookahead = [](TimeContext const& time, std::size_t steps) {
    auto target = timeAfter(*time.factory, *time.initial, *time.epsilon, steps);
    auto result = time.factory->makeZero();
    require(result != nullptr, "logical-time factory returned no lookahead interval");
    result->setToDifference(*target, *time.initial);
    return result;
  };
  auto leftLookahead = makeLookahead(leftTime, 1U);
  auto rightLookahead = makeLookahead(rightTime, 3U);
  leftRegulator.rtiAmbassador().enableTimeRegulation(*leftLookahead);
  waitFor(
      leftRegulator,
      [&] { return leftRegulator.recorder().timeRegulationEnabled().size() >= 1U; },
      options,
      "first Query GALT/LITS time-regulation callback");

  auto expectedOne = timeAfter(
      *observerTime.factory,
      *observerTime.initial,
      *observerTime.epsilon,
      1U);
  require(
      observer.rtiAmbassador().queryGALT(*galt),
      "Query GALT remained undefined after the first regulator became active");
  requireTimeEquals(*galt, *expectedOne, "Query GALT first regulator boundary");
  require(
      observer.rtiAmbassador().queryLITS(*lits),
      "Query LITS remained undefined after the first regulator became active");
  requireTimeEquals(*lits, *expectedOne, "Query LITS first regulator boundary");

  rightRegulator.rtiAmbassador().enableTimeRegulation(*rightLookahead);
  waitFor(
      rightRegulator,
      [&] { return rightRegulator.recorder().timeRegulationEnabled().size() >= 1U; },
      options,
      "second Query GALT/LITS time-regulation callback");
  require(
      observer.rtiAmbassador().queryGALT(*galt),
      "Query GALT became undefined with two active regulators");
  requireTimeEquals(*galt, *expectedOne, "Query GALT minimum regulator boundary");
  require(
      observer.rtiAmbassador().queryLITS(*lits),
      "Query LITS became undefined with two active regulators");
  requireTimeEquals(*lits, *expectedOne, "Query LITS minimum regulator boundary");

  auto leftTarget = timeAfter(*leftTime.factory, *leftTime.initial, *leftTime.epsilon, 4U);
  auto rightTarget = timeAfter(*rightTime.factory, *rightTime.initial, *rightTime.epsilon, 1U);
  leftRegulator.rtiAmbassador().timeAdvanceRequest(*leftTarget);
  rightRegulator.rtiAmbassador().timeAdvanceRequest(*rightTarget);
  auto expectedFour = timeAfter(
      *observerTime.factory,
      *observerTime.initial,
      *observerTime.epsilon,
      4U);
  require(
      observer.rtiAmbassador().queryGALT(*galt),
      "Query GALT became undefined while regulators had pending advances");
  requireTimeEquals(*galt, *expectedFour, "Query GALT pending-advance minimum");
  require(
      observer.rtiAmbassador().queryLITS(*lits),
      "Query LITS became undefined while regulators had pending advances");
  requireTimeEquals(*lits, *expectedFour, "Query LITS pending-advance minimum");

  waitForSessions(
      {&leftRegulator, &rightRegulator},
      [&] {
        return leftRegulator.recorder().timeAdvanceGrants().size() >= 1U &&
            rightRegulator.recorder().timeAdvanceGrants().size() >= 1U;
      },
      options,
      "Query GALT/LITS regulator grants");
  require(
      leftRegulator.recorder().timeAdvanceGrants().front().encoded ==
              encodeTime(*leftTarget) &&
          rightRegulator.recorder().timeAdvanceGrants().front().encoded ==
              encodeTime(*rightTarget),
      "Query GALT/LITS regulator grants returned the wrong times");
  require(
      observer.rtiAmbassador().queryGALT(*galt),
      "Query GALT became undefined after pending advances were granted");
  requireTimeEquals(*galt, *expectedFour, "Query GALT granted minimum");
  require(
      observer.rtiAmbassador().queryLITS(*lits),
      "Query LITS became undefined after pending advances were granted");
  requireTimeEquals(*lits, *expectedFour, "Query LITS granted minimum");

  rightRegulator.resign(rti::NO_ACTION);
  auto expectedFive = timeAfter(
      *observerTime.factory,
      *observerTime.initial,
      *observerTime.epsilon,
      5U);
  require(
      observer.rtiAmbassador().queryGALT(*galt),
      "Query GALT became undefined after the minimum regulator resigned");
  requireTimeEquals(*galt, *expectedFive, "Query GALT post-resignation boundary");
  require(
      observer.rtiAmbassador().queryLITS(*lits),
      "Query LITS became undefined after the minimum regulator resigned");
  requireTimeEquals(*lits, *expectedFive, "Query LITS post-resignation boundary");

  leftRegulator.rtiAmbassador().disableTimeRegulation();
  require(
      !observer.rtiAmbassador().queryGALT(*galt),
      "Query GALT remained defined after every other regulator was disabled");
  require(
      !observer.rtiAmbassador().queryLITS(*lits),
      "Query LITS remained defined after every other regulator was disabled");

  leftRegulator.resign(rti::NO_ACTION);
  observer.resign(rti::NO_ACTION);
  requireException(
      [&] { static_cast<void>(observer.rtiAmbassador().queryGALT(*galt)); },
      L"FederateNotExecutionMember",
      "Query GALT after observer resignation");
  requireException(
      [&] { static_cast<void>(observer.rtiAmbassador().queryLITS(*lits)); },
      L"FederateNotExecutionMember",
      "Query LITS after observer resignation");
  observer.rtiAmbassador().destroyFederationExecution(federation);
  rightRegulator.disconnect();
  leftRegulator.disconnect();
  observer.disconnect();
  requireException(
      [&] { static_cast<void>(observer.rtiAmbassador().queryGALT(*galt)); },
      L"NotConnected",
      "Query GALT after observer disconnect");
  requireException(
      [&] { static_cast<void>(observer.rtiAmbassador().queryLITS(*lits)); },
      L"NotConnected",
      "Query LITS after observer disconnect");
}

void scenarioTimeBoundsQueriesContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioTimeBoundsQueries(options, model);
}

void scenarioTimeConstrainedEnableAtGaltEquality(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.logicalTimeImplementationName.empty(),
      "Time-constrained GALT equality requires an adapter-supplied logical-time implementation");

  Session regulator(options, model, "owner");
  Session requester(options, model, "member");
  auto const federation = federationName(options, "time-constrained-enable-galt-equality");
  connectAndJoin(regulator, requester, options, federation, options.fom);
  auto regulatorTime = makeTimeContext(regulator);
  auto requesterTime = makeTimeContext(requester);
  require(
      regulatorTime.factory->getName() == requesterTime.factory->getName(),
      "Time-constrained GALT equality members selected different logical-time factories");

  auto lookahead = regulatorTime.factory->makeZero();
  require(lookahead != nullptr, "Time-constrained GALT equality could not allocate lookahead");
  auto const one = timeAfter(
      *regulatorTime.factory,
      *regulatorTime.initial,
      *regulatorTime.epsilon,
      1U);
  lookahead->setToDifference(*one, *regulatorTime.initial);
  regulator.rtiAmbassador().enableTimeRegulation(*lookahead);
  waitFor(
      regulator,
      [&] { return regulator.recorder().timeRegulationEnabled().size() == 1U; },
      options,
      "Time-constrained GALT equality regulation callback");

  auto const five = timeAfter(
      *requesterTime.factory,
      *requesterTime.initial,
      *requesterTime.epsilon,
      5U);
  requester.rtiAmbassador().timeAdvanceRequest(*five);
  waitFor(
      requester,
      [&] { return requester.recorder().timeAdvanceGrants().size() == 1U; },
      options,
      "Time-constrained GALT equality unconstrained grant to five");
  auto observedGalt = requesterTime.factory->makeInitial();
  require(
      observedGalt != nullptr && requester.rtiAmbassador().queryGALT(*observedGalt),
      "Time-constrained GALT equality was undefined before constrained enablement");
  requireTimeEquals(
      *observedGalt,
      *one,
      "Time-constrained GALT equality pre-enable boundary");
  requester.rtiAmbassador().enableTimeConstrained();
  require(
      requester.recorder().timeConstrainedEnabled().empty(),
      "Time-constrained GALT equality enabled before GALT reached current time");

  auto const regulatorTargetFour = timeAfter(
      *regulatorTime.factory,
      *regulatorTime.initial,
      *regulatorTime.epsilon,
      4U);
  regulator.rtiAmbassador().timeAdvanceRequest(*regulatorTargetFour);
  waitFor(
      regulator,
      [&] { return regulator.recorder().timeAdvanceGrants().size() == 1U; },
      options,
      "Time-constrained GALT equality regulator grant to four");
  auto const regulatorTargetFive = timeAfter(
      *regulatorTime.factory,
      *regulatorTime.initial,
      *regulatorTime.epsilon,
      5U);
  regulator.rtiAmbassador().timeAdvanceRequest(*regulatorTargetFive);
  waitFor(
      regulator,
      [&] { return regulator.recorder().timeAdvanceGrants().size() == 2U; },
      options,
      "Time-constrained GALT equality subsequent regulator grant");
  if (model == rti::CallbackModel::HLA_EVOKED) {
    require(
        requester.recorder().timeConstrainedEnabled().empty(),
        "Time-constrained GALT equality callback escaped evoked servicing");
  }
  waitFor(
      requester,
      [&] { return requester.recorder().timeConstrainedEnabled().size() == 1U; },
      options,
      "Time-constrained GALT equality callback at current time");
  require(
      requester.recorder().timeConstrainedEnabled().front().text == five->toString(),
      "Time-constrained GALT equality callback returned the wrong logical time");

  requester.rtiAmbassador().disableTimeConstrained();
  regulator.rtiAmbassador().disableTimeRegulation();
  requester.resign(rti::NO_ACTION);
  regulator.resign(rti::NO_ACTION);
  regulator.rtiAmbassador().destroyFederationExecution(federation);
  requester.disconnect();
  regulator.disconnect();
}

void scenarioTimeConstrainedEnableAtGaltEqualityContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioTimeConstrainedEnableAtGaltEquality(options, model);
}

void scenarioQueryLitsAfterSourceResignation(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.logicalTimeImplementationName.empty(),
      "Query LITS after source resignation requires an adapter-supplied logical-time implementation");

  Session publisher(options, model, "owner");
  Session receiver(options, model, "member");
  auto const federation = federationName(
      options,
      "query-lits-source-resignation");
  connectAndJoin(publisher, receiver, options, federation, options.fom);

  auto const publisherInteraction = publisher.rtiAmbassador().getInteractionClassHandle(
      options.interactionClassName);
  auto const receiverInteraction = receiver.rtiAmbassador().getInteractionClassHandle(
      options.interactionClassName);
  auto const publisherParameter = publisher.rtiAmbassador().getParameterHandle(
      publisherInteraction,
      options.parameterName);
  auto const receiverParameter = receiver.rtiAmbassador().getParameterHandle(
      receiverInteraction,
      options.parameterName);
  require(
      publisherInteraction.isValid() && receiverInteraction.isValid() &&
          publisherParameter.isValid() && receiverParameter.isValid(),
      "Query LITS source resignation lookup returned an invalid handle");

  publisher.rtiAmbassador().publishInteractionClass(publisherInteraction);
  publisher.rtiAmbassador().changeInteractionOrderType(
      publisherInteraction,
      rti::TIMESTAMP);
  receiver.rtiAmbassador().subscribeInteractionClass(
      receiverInteraction,
      true);

  auto publisherTime = makeTimeContext(publisher);
  auto receiverTime = makeTimeContext(receiver);
  require(
      publisherTime.factory->getName() == receiverTime.factory->getName(),
      "Query LITS source resignation members selected different logical-time factories");
  auto galt = receiverTime.factory->makeInitial();
  auto lits = receiverTime.factory->makeInitial();
  require(
      galt != nullptr && lits != nullptr,
      "Query LITS source resignation could not allocate output logical times");

  auto lookaheadTarget = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      1U);
  auto lookahead = publisherTime.factory->makeZero();
  require(
      lookahead != nullptr,
      "Query LITS source resignation could not allocate a lookahead interval");
  lookahead->setToDifference(*lookaheadTarget, *publisherTime.initial);

  receiver.rtiAmbassador().enableTimeConstrained();
  waitFor(
      receiver,
      [&] { return receiver.recorder().timeConstrainedEnabled().size() >= 1U; },
      options,
      "Query LITS source resignation receiver time-constrained callback");
  publisher.rtiAmbassador().enableTimeRegulation(*lookahead);
  waitFor(
      publisher,
      [&] { return publisher.recorder().timeRegulationEnabled().size() >= 1U; },
      options,
      "Query LITS source resignation publisher time-regulation callback");

  auto const expectedLookahead = timeAfter(
      *receiverTime.factory,
      *receiverTime.initial,
      *receiverTime.epsilon,
      1U);
  require(
      receiver.rtiAmbassador().queryGALT(*galt),
      "Query GALT source resignation was undefined while the source was active");
  requireTimeEquals(
      *galt,
      *expectedLookahead,
      "Query GALT source resignation active boundary");
  require(
      receiver.rtiAmbassador().queryLITS(*lits),
      "Query LITS source resignation was undefined while the source was active");
  requireTimeEquals(
      *lits,
      *expectedLookahead,
      "Query LITS source resignation active boundary");

  auto const messageTime = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      5U);
  std::vector<std::uint8_t> parameterBytes{
      0x51U, 0x4CU, 0x49U, 0x54U, 0x53U};
  std::vector<std::uint8_t> tagBytes{
      0x51U, 0x55U, 0x45U, 0x55U, 0x45U};
  rti::ParameterHandleValueMap parameters;
  parameters.emplace(
      publisherParameter,
      rti::VariableLengthData(parameterBytes.data(), parameterBytes.size()));
  rti::VariableLengthData tag(tagBytes.data(), tagBytes.size());
  auto const retraction = publisher.rtiAmbassador().sendInteraction(
      publisherInteraction,
      parameters,
      tag,
      *messageTime);
  require(
      retraction.isValid(),
      "Query LITS source resignation send returned an invalid retraction handle");
  require(
      receiver.recorder().timedInteractions().empty(),
      "Query LITS source resignation delivered the queued interaction early");

  publisher.resign(rti::NO_ACTION);
  require(
      !receiver.rtiAmbassador().queryGALT(*galt),
      "Query GALT source resignation remained defined after the source resigned");
  require(
      receiver.rtiAmbassador().queryLITS(*lits),
      "Query LITS source resignation became undefined after the source resigned");
  requireTimeEquals(
      *lits,
      *messageTime,
      "Query LITS source resignation queued-message boundary");
  require(
      receiver.recorder().timedInteractions().empty(),
      "Query LITS source resignation delivered while only querying bounds");

  receiver.rtiAmbassador().disableTimeConstrained();
  receiver.rtiAmbassador().unsubscribeInteractionClass(receiverInteraction);
  receiver.resign(rti::NO_ACTION);
  receiver.rtiAmbassador().destroyFederationExecution(federation);
  publisher.disconnect();
  receiver.disconnect();
}

class ReentrantCallbackFederateAmbassador final
    : public rti::NullFederateAmbassador {
 public:
  rti::RTIambassador* ambassador = nullptr;
  std::size_t reportCount = 0U;
  bool evokeRejected = false;
  bool multipleRejected = false;

  void reportFederationExecutions(
      rti::FederationExecutionInformationVector const&) override {
    ++reportCount;
    try {
      static_cast<void>(ambassador->evokeCallback(0.0));
    } catch (rti::CallNotAllowedFromWithinCallback const&) {
      evokeRejected = true;
    }
    try {
      static_cast<void>(ambassador->evokeMultipleCallbacks(0.0, 0.0));
    } catch (rti::CallNotAllowedFromWithinCallback const&) {
      multipleRejected = true;
    }
  }
};

void scenarioCallbackReentrancy(
    Options const& options,
    rti::CallbackModel model) {
  rti::RTIambassadorFactory factory;
  auto ambassador = factory.createRTIambassador();
  require(
      static_cast<bool>(ambassador),
      "callback re-entrancy test did not create a standard RTIambassador");

  ReentrantCallbackFederateAmbassador federate;
  static_cast<void>(ambassador->connect(federate, model));
  federate.ambassador = ambassador.get();
  ambassador->listFederationExecutions();
  if (model == rti::HLA_EVOKED) {
    auto const maximumWait = static_cast<double>(
        std::max(options.timeoutMilliseconds, 1)) / 1000.0;
    static_cast<void>(
        ambassador->evokeMultipleCallbacks(0.0, maximumWait));
  }
  require(
      federate.reportCount == 1U,
      "callback re-entrancy test did not receive exactly one listing callback");
  require(
      federate.evokeRejected && federate.multipleRejected,
      "callback re-entrancy test admitted callback-service re-entry");
  ambassador->disconnect();
}

void scenarioCallbackReentrancyContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioCallbackReentrancy(options, model);
}


}  // namespace hla_rti_cpp_tck::detail
