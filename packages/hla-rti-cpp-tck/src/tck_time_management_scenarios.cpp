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
  auto const regulationEnabled = publisher.recorder().timeRegulationEnabled();
  require(
      regulationEnabled.size() == 1U &&
          regulationEnabled.front().encoded == encodeTime(*publisherTime.initial),
      "Time Regulation Enabled did not report the federate's current logical time");

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
  auto const taraCallbackOrder =
      timeAdvanceAvailable.recorder().callbackOrder();
  require(
      taraCallbackOrder == std::vector<std::string>{"reflect", "grant"} ||
          taraCallbackOrder == std::vector<std::string>{"grant", "reflect"},
      "Time Advance Request Available did not deliver its equal-time TSO and grant callbacks");
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
  require(
      receiver.recorder().timedInteractions().empty(),
      "Next Message Request delivered a timestamped interaction before the request");

  receiver.rtiAmbassador().nextMessageRequest(*requestBoundary);
  require(
      receiver.recorder().timeAdvanceGrants().empty(),
      "Next Message Request completed before the regulator reached the queued message");
  auto const retraction = publisher.rtiAmbassador().sendInteraction(
      publisherInteraction,
      parameters,
      tag,
      *messageTime);
  require(
      retraction.isValid(),
      "Next Message Request timestamped interaction returned an invalid retraction handle");
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

void scenarioNmraNoPregrantTsoFloor(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.logicalTimeImplementationName.empty(),
      "NMRA no-pregrant floor requires an adapter-supplied logical-time implementation");

  Session requester(options, model, "owner");
  Session peer(options, model, "member");
  auto const federation = federationName(options, "nmra-no-pregrant-tso-floor");
  connectAndJoin(requester, peer, options, federation, options.fom);

  auto const interaction = requester.rtiAmbassador().getInteractionClassHandle(
      options.interactionClassName);
  auto const parameter = requester.rtiAmbassador().getParameterHandle(
      interaction,
      options.parameterName);
  require(
      interaction.isValid() && parameter.isValid(),
      "NMRA no-pregrant floor could not resolve the adapter interaction and parameter");
  requester.rtiAmbassador().publishInteractionClass(interaction);
  requester.rtiAmbassador().changeInteractionOrderType(
      interaction,
      rti::TIMESTAMP);

  auto requesterTime = makeTimeContext(requester);
  auto peerTime = makeTimeContext(peer);
  require(
      requesterTime.factory->getName() == peerTime.factory->getName(),
      "NMRA no-pregrant members selected different logical-time factories");
  requireTimeEquals(
      *requesterTime.initial,
      *peerTime.initial,
      "NMRA no-pregrant initial logical time");

  requester.rtiAmbassador().enableTimeConstrained();
  waitFor(
      requester,
      [&] { return requester.recorder().timeConstrainedEnabled().size() == 1U; },
      options,
      "NMRA no-pregrant requester Time Constrained callback");
  requester.rtiAmbassador().enableTimeRegulation(*requesterTime.epsilon);
  waitFor(
      requester,
      [&] { return requester.recorder().timeRegulationEnabled().size() == 1U; },
      options,
      "NMRA no-pregrant requester Time Regulation callback");
  peer.rtiAmbassador().enableTimeRegulation(*peerTime.epsilon);
  waitFor(
      peer,
      [&] { return peer.recorder().timeRegulationEnabled().size() == 1U; },
      options,
      "NMRA no-pregrant peer Time Regulation callback");

  auto actualLookahead = requesterTime.factory->makeZero();
  require(
      actualLookahead != nullptr,
      "NMRA no-pregrant floor could not allocate a lookahead query value");
  requester.rtiAmbassador().queryLookahead(*actualLookahead);
  require(
      *actualLookahead == *requesterTime.epsilon,
      "NMRA no-pregrant floor did not retain the enabled epsilon lookahead");

  auto const peerTarget = timeAfter(
      *peerTime.factory,
      *peerTime.initial,
      *peerTime.epsilon,
      12U);
  auto const requestTime = timeAfter(
      *requesterTime.factory,
      *requesterTime.initial,
      *requesterTime.epsilon,
      10U);
  require(
      requester.recorder().timeAdvanceGrants().empty(),
      "NMRA no-pregrant requester had a grant before issuing the request");
  require(
      requester.recorder().timedInteractions().empty(),
      "NMRA no-pregrant requester received an interaction before its request");

  peer.rtiAmbassador().timeAdvanceRequest(*peerTarget);
  waitFor(
      peer,
      [&] { return peer.recorder().timeAdvanceGrants().size() == 1U; },
      options,
      "NMRA no-pregrant peer Time Advance Grant");
  require(
      peer.recorder().timeAdvanceGrants().front().encoded == encodeTime(*peerTarget),
      "NMRA no-pregrant peer did not advance beyond the request time");
  require(
      requester.recorder().timedInteractions().empty(),
      "NMRA no-pregrant requester received TSO before its grant");

  requester.rtiAmbassador().nextMessageRequestAvailable(*requestTime);
  waitFor(
      requester,
      [&] { return requester.recorder().timeAdvanceGrants().size() == 1U; },
      options,
      "NMRA no-pregrant requester Time Advance Grant");
  auto const grants = requester.recorder().timeAdvanceGrants();
  require(
      grants.size() == 1U && grants.front().encoded == encodeTime(*requestTime),
      "NMRA without pre-grant TSO did not grant at its request time");
  require(
      requester.recorder().timedInteractions().empty(),
      "NMRA no-pregrant requester received an unexpected timestamped interaction");
  auto queriedTime = requesterTime.factory->makeInitial();
  require(queriedTime != nullptr, "NMRA no-pregrant floor could not allocate a query time");
  requester.rtiAmbassador().queryLogicalTime(*queriedTime);
  requireTimeEquals(*queriedTime, *requestTime, "NMRA no-pregrant logical-time query");

  auto const legalTime = timeAfter(
      *requesterTime.factory,
      *requestTime,
      *actualLookahead,
      1U);
  std::vector<std::uint8_t> payload{0x4EU, 0x4DU, 0x52U, 0x41U};
  std::vector<std::uint8_t> tagBytes{0x4EU, 0x4FU, 0x50U, 0x52U, 0x45U};
  rti::ParameterHandleValueMap parameters;
  parameters.emplace(
      parameter,
      rti::VariableLengthData(payload.data(), payload.size()));
  rti::VariableLengthData tag(tagBytes.data(), tagBytes.size());
  requireException(
      [&] {
        requester.rtiAmbassador().sendInteraction(
            interaction,
            parameters,
            tag,
            *requestTime);
      },
      L"InvalidLogicalTime",
      "NMRA send at request time below actual lookahead floor");
  auto const retraction = requester.rtiAmbassador().sendInteraction(
      interaction,
      parameters,
      tag,
      *legalTime);
  require(
      retraction.isValid(),
      "NMRA send at request time plus actual lookahead returned an invalid retraction handle");

  requester.resign(rti::NO_ACTION);
  peer.resign(rti::NO_ACTION);
  requester.rtiAmbassador().destroyFederationExecution(federation);
  requester.disconnect();
  peer.disconnect();
}

void scenarioNmraNoPregrantTsoFloorContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioNmraNoPregrantTsoFloor(options, model);
}

void scenarioNmrNoPregrantTsoFloor(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.logicalTimeImplementationName.empty(),
      "NMR no-pregrant floor requires an adapter-supplied logical-time implementation");

  Session requester(options, model, "owner");
  Session peer(options, model, "member");
  auto const federation = federationName(options, "nmr-no-pregrant-tso-floor");
  connectAndJoin(requester, peer, options, federation, options.fom);

  auto const interaction = requester.rtiAmbassador().getInteractionClassHandle(
      options.interactionClassName);
  auto const parameter = requester.rtiAmbassador().getParameterHandle(
      interaction,
      options.parameterName);
  require(
      interaction.isValid() && parameter.isValid(),
      "NMR no-pregrant floor could not resolve the adapter interaction and parameter");
  requester.rtiAmbassador().publishInteractionClass(interaction);
  requester.rtiAmbassador().changeInteractionOrderType(
      interaction,
      rti::TIMESTAMP);

  auto requesterTime = makeTimeContext(requester);
  auto peerTime = makeTimeContext(peer);
  require(
      requesterTime.factory->getName() == peerTime.factory->getName(),
      "NMR no-pregrant members selected different logical-time factories");
  requireTimeEquals(
      *requesterTime.initial,
      *peerTime.initial,
      "NMR no-pregrant initial logical time");

  requester.rtiAmbassador().enableTimeConstrained();
  waitFor(
      requester,
      [&] { return requester.recorder().timeConstrainedEnabled().size() == 1U; },
      options,
      "NMR no-pregrant requester Time Constrained callback");
  requester.rtiAmbassador().enableTimeRegulation(*requesterTime.epsilon);
  waitFor(
      requester,
      [&] { return requester.recorder().timeRegulationEnabled().size() == 1U; },
      options,
      "NMR no-pregrant requester Time Regulation callback");
  peer.rtiAmbassador().enableTimeRegulation(*peerTime.epsilon);
  waitFor(
      peer,
      [&] { return peer.recorder().timeRegulationEnabled().size() == 1U; },
      options,
      "NMR no-pregrant peer Time Regulation callback");

  auto actualLookahead = requesterTime.factory->makeZero();
  require(
      actualLookahead != nullptr,
      "NMR no-pregrant floor could not allocate a lookahead query value");
  requester.rtiAmbassador().queryLookahead(*actualLookahead);
  require(
      *actualLookahead == *requesterTime.epsilon,
      "NMR no-pregrant floor did not retain the enabled epsilon lookahead");

  auto const requestTime = timeAfter(
      *requesterTime.factory,
      *requesterTime.initial,
      *requesterTime.epsilon,
      10U);
  auto const peerTarget = timeAfter(
      *peerTime.factory,
      *peerTime.initial,
      *peerTime.epsilon,
      10U);
  require(
      requester.recorder().timeAdvanceGrants().empty(),
      "NMR no-pregrant requester had a grant before issuing the request");
  require(
      requester.recorder().timedInteractions().empty(),
      "NMR no-pregrant requester received an interaction before its request");

  requester.rtiAmbassador().nextMessageRequest(*requestTime);
  require(
      requester.recorder().timeAdvanceGrants().empty(),
      "NMR without pre-grant TSO input completed before the peer advanced");
  peer.rtiAmbassador().timeAdvanceRequest(*peerTarget);
  waitFor(
      peer,
      [&] { return peer.recorder().timeAdvanceGrants().size() == 1U; },
      options,
      "NMR no-pregrant peer Time Advance Grant");
  waitFor(
      requester,
      [&] { return requester.recorder().timeAdvanceGrants().size() == 1U; },
      options,
      "NMR no-pregrant requester Time Advance Grant");
  require(
      peer.recorder().timeAdvanceGrants().front().encoded == encodeTime(*peerTarget),
      "NMR no-pregrant peer did not advance to the requested target");
  auto const grants = requester.recorder().timeAdvanceGrants();
  require(
      grants.size() == 1U && grants.front().encoded == encodeTime(*requestTime),
      "NMR without pre-grant TSO did not grant at its request time");
  require(
      requester.recorder().timedInteractions().empty(),
      "NMR no-pregrant requester received an unexpected timestamped interaction");
  auto queriedTime = requesterTime.factory->makeInitial();
  require(queriedTime != nullptr, "NMR no-pregrant floor could not allocate a query time");
  requester.rtiAmbassador().queryLogicalTime(*queriedTime);
  requireTimeEquals(*queriedTime, *requestTime, "NMR no-pregrant logical-time query");

  auto const legalTime = timeAfter(
      *requesterTime.factory,
      *requestTime,
      *actualLookahead,
      1U);
  std::vector<std::uint8_t> payload{0x4EU, 0x4DU, 0x52U};
  std::vector<std::uint8_t> tagBytes{0x46U, 0x4CU, 0x4FU, 0x4FU, 0x52U};
  rti::ParameterHandleValueMap parameters;
  parameters.emplace(
      parameter,
      rti::VariableLengthData(payload.data(), payload.size()));
  rti::VariableLengthData tag(tagBytes.data(), tagBytes.size());
  requireException(
      [&] {
        requester.rtiAmbassador().sendInteraction(
            interaction,
            parameters,
            tag,
            *requestTime);
      },
      L"InvalidLogicalTime",
      "NMR send at request time below actual lookahead floor");
  auto const retraction = requester.rtiAmbassador().sendInteraction(
      interaction,
      parameters,
      tag,
      *legalTime);
  require(
      retraction.isValid(),
      "NMR send at request time plus actual lookahead returned an invalid retraction handle");

  requester.resign(rti::NO_ACTION);
  peer.resign(rti::NO_ACTION);
  requester.rtiAmbassador().destroyFederationExecution(federation);
  requester.disconnect();
  peer.disconnect();
}

void scenarioNmrNoPregrantTsoFloorContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioNmrNoPregrantTsoFloor(options, model);
}

void scenarioNmrReceivedTsoFloor(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.logicalTimeImplementationName.empty(),
      "NMR received-TSO floor requires an adapter-supplied logical-time implementation");

  Session requester(options, model, "owner");
  Session peer(options, model, "member");
  auto const federation = federationName(options, "nmr-received-tso-floor");
  connectAndJoin(requester, peer, options, federation, options.fom);

  auto const requesterInteraction = requester.rtiAmbassador().getInteractionClassHandle(
      options.interactionClassName);
  auto const peerInteraction = peer.rtiAmbassador().getInteractionClassHandle(
      options.interactionClassName);
  auto const requesterParameter = requester.rtiAmbassador().getParameterHandle(
      requesterInteraction,
      options.parameterName);
  auto const peerParameter = peer.rtiAmbassador().getParameterHandle(
      peerInteraction,
      options.parameterName);
  require(
      requesterInteraction.isValid() && peerInteraction.isValid() &&
          requesterParameter.isValid() && peerParameter.isValid(),
      "NMR received-TSO FOM lookup returned an invalid interaction handle");
  require(
      requesterInteraction == peerInteraction && requesterParameter == peerParameter,
      "NMR received-TSO handles did not retain identity across members");
  requester.rtiAmbassador().publishInteractionClass(requesterInteraction);
  requester.rtiAmbassador().changeInteractionOrderType(
      requesterInteraction,
      rti::TIMESTAMP);
  requester.rtiAmbassador().subscribeInteractionClass(requesterInteraction, true);
  peer.rtiAmbassador().publishInteractionClass(peerInteraction);
  peer.rtiAmbassador().changeInteractionOrderType(peerInteraction, rti::TIMESTAMP);

  auto requesterTime = makeTimeContext(requester);
  auto peerTime = makeTimeContext(peer);
  require(
      requesterTime.factory->getName() == peerTime.factory->getName(),
      "NMR received-TSO members selected different logical-time factories");
  requireTimeEquals(
      *requesterTime.initial,
      *peerTime.initial,
      "NMR received-TSO initial logical time");

  requester.rtiAmbassador().enableTimeConstrained();
  waitFor(
      requester,
      [&] { return requester.recorder().timeConstrainedEnabled().size() == 1U; },
      options,
      "NMR received-TSO requester Time Constrained callback");
  requester.rtiAmbassador().enableTimeRegulation(*requesterTime.epsilon);
  waitFor(
      requester,
      [&] { return requester.recorder().timeRegulationEnabled().size() == 1U; },
      options,
      "NMR received-TSO requester Time Regulation callback");
  peer.rtiAmbassador().enableTimeRegulation(*peerTime.epsilon);
  waitFor(
      peer,
      [&] { return peer.recorder().timeRegulationEnabled().size() == 1U; },
      options,
      "NMR received-TSO peer Time Regulation callback");
  requester.recorder().clearTimeCallbacks();
  peer.recorder().clearTimeCallbacks();
  requester.recorder().clearCallbackOrder();
  peer.recorder().clearCallbackOrder();

  auto actualLookahead = requesterTime.factory->makeZero();
  require(
      actualLookahead != nullptr,
      "NMR received-TSO floor could not allocate a lookahead query value");
  requester.rtiAmbassador().queryLookahead(*actualLookahead);
  require(
      *actualLookahead == *requesterTime.epsilon,
      "NMR received-TSO floor did not retain the enabled epsilon lookahead");

  auto const messageTime = timeAfter(
      *peerTime.factory,
      *peerTime.initial,
      *peerTime.epsilon,
      5U);
  auto const requestTime = timeAfter(
      *requesterTime.factory,
      *requesterTime.initial,
      *requesterTime.epsilon,
      10U);
  auto const peerTarget = timeAfter(
      *peerTime.factory,
      *peerTime.initial,
      *peerTime.epsilon,
      4U);
  std::vector<std::uint8_t> incomingBytes{0x4EU, 0x4DU, 0x52U};
  std::vector<std::uint8_t> incomingTagBytes{0x4EU, 0x4DU, 0x52U, 0x2DU, 0x46U};
  rti::ParameterHandleValueMap incomingParameters;
  incomingParameters.emplace(
      peerParameter,
      rti::VariableLengthData(incomingBytes.data(), incomingBytes.size()));
  rti::VariableLengthData incomingTag(
      incomingTagBytes.data(), incomingTagBytes.size());
  auto const incomingRetraction = peer.rtiAmbassador().sendInteraction(
      peerInteraction,
      incomingParameters,
      incomingTag,
      *messageTime);
  require(
      incomingRetraction.isValid(),
      "NMR received-TSO input returned an invalid retraction handle");
  require(
      requester.recorder().timedInteractions().empty(),
      "NMR received-TSO input was delivered before the request");

  requester.rtiAmbassador().nextMessageRequest(*requestTime);
  require(
      requester.recorder().timeAdvanceGrants().empty(),
      "NMR received-TSO requester granted before the regulator advanced");
  peer.rtiAmbassador().timeAdvanceRequest(*peerTarget);
  waitFor(
      peer,
      requester,
      [&] {
        return peer.recorder().timeAdvanceGrants().size() >= 1U &&
            requester.recorder().timeAdvanceGrants().size() >= 1U &&
            requester.recorder().timedInteractions().size() >= 1U;
      },
      options,
      "NMR received-TSO delivery and grant");
  require(
      peer.recorder().timeAdvanceGrants().size() == 1U &&
          peer.recorder().timeAdvanceGrants().front().encoded == encodeTime(*peerTarget),
      "NMR received-TSO peer did not advance to its requested bound");
  require(
      requester.recorder().timedInteractions().size() == 1U &&
          requester.recorder().timeAdvanceGrants().size() == 1U,
      "NMR received-TSO requester received the wrong callback counts");

  auto const received = requester.recorder().timedInteractions().front();
  require(
      received.interaction == requesterInteraction &&
          received.parameters.size() == 1U &&
          received.parameters.count(requesterParameter) == 1U &&
          copyBytes(received.parameters.at(requesterParameter)) == incomingBytes,
      "NMR received-TSO callback contained the wrong interaction or parameter value");
  require(
      received.tag == incomingTagBytes && received.producer == peer.federateHandle(),
      "NMR received-TSO callback did not preserve its tag or producer");
  require(
      received.time == encodeTime(*messageTime) &&
          received.sentOrder == rti::TIMESTAMP &&
          received.receivedOrder == rti::TIMESTAMP &&
          received.retractionPresent &&
          received.retraction == copyBytes(incomingRetraction.encode()),
      "NMR received-TSO callback did not preserve its timestamp or metadata");
  require(
      requester.recorder().timeAdvanceGrants().front().encoded == encodeTime(*messageTime),
      "NMR with pre-grant TSO did not grant at the delivered message time");
  auto queriedTime = requesterTime.factory->makeInitial();
  require(queriedTime != nullptr, "NMR received-TSO floor could not allocate a query time");
  requester.rtiAmbassador().queryLogicalTime(*queriedTime);
  requireTimeEquals(*queriedTime, *messageTime, "NMR received-TSO logical-time query");

  auto const legalTime = timeAfter(
      *requesterTime.factory,
      *messageTime,
      *actualLookahead,
      1U);
  std::vector<std::uint8_t> outgoingBytes{0x4EU, 0x4DU, 0x52U, 0x2DU, 0x4FU};
  std::vector<std::uint8_t> outgoingTagBytes{0x4EU, 0x4DU, 0x52U, 0x2DU, 0x53U};
  rti::ParameterHandleValueMap outgoingParameters;
  outgoingParameters.emplace(
      requesterParameter,
      rti::VariableLengthData(outgoingBytes.data(), outgoingBytes.size()));
  rti::VariableLengthData outgoingTag(
      outgoingTagBytes.data(), outgoingTagBytes.size());
  requireException(
      [&] {
        requester.rtiAmbassador().sendInteraction(
            requesterInteraction,
            outgoingParameters,
            outgoingTag,
            *messageTime);
      },
      L"InvalidLogicalTime",
      "NMR Send Interaction at received-message grant time below actual-lookahead floor");
  auto const outgoingRetraction = requester.rtiAmbassador().sendInteraction(
      requesterInteraction,
      outgoingParameters,
      outgoingTag,
      *legalTime);
  require(
      outgoingRetraction.isValid(),
      "NMR Send Interaction at grant time plus actual lookahead returned an invalid retraction handle");

  requester.resign(rti::NO_ACTION);
  peer.resign(rti::NO_ACTION);
  requester.rtiAmbassador().destroyFederationExecution(federation);
  requester.disconnect();
  peer.disconnect();
}

void scenarioNmrReceivedTsoFloorContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioNmrReceivedTsoFloor(options, model);
}

void scenarioNmrReceivedTsoAttributeUpdateFloor(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.logicalTimeImplementationName.empty(),
      "NMR received-TSO attribute floor requires an adapter-supplied logical-time implementation");

  Session requester(options, model, "owner");
  Session peer(options, model, "member");
  auto const federation = federationName(options, "nmr-received-tso-attribute-floor");
  connectAndJoin(requester, peer, options, federation, options.fom);

  auto const requesterInteraction = requester.rtiAmbassador().getInteractionClassHandle(
      options.interactionClassName);
  auto const peerInteraction = peer.rtiAmbassador().getInteractionClassHandle(
      options.interactionClassName);
  auto const requesterParameter = requester.rtiAmbassador().getParameterHandle(
      requesterInteraction,
      options.parameterName);
  auto const peerParameter = peer.rtiAmbassador().getParameterHandle(
      peerInteraction,
      options.parameterName);
  require(
      requesterInteraction.isValid() && peerInteraction.isValid() &&
          requesterParameter.isValid() && peerParameter.isValid(),
      "NMR received-TSO attribute FOM lookup returned an invalid interaction handle");
  require(
      requesterInteraction == peerInteraction && requesterParameter == peerParameter,
      "NMR received-TSO attribute interaction handles differed across members");
  requester.rtiAmbassador().subscribeInteractionClass(requesterInteraction);
  peer.rtiAmbassador().publishInteractionClass(peerInteraction);
  peer.rtiAmbassador().changeInteractionOrderType(peerInteraction, rti::TIMESTAMP);

  auto const objectClass = requester.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const attribute = requester.rtiAmbassador().getAttributeHandle(
      objectClass,
      options.attributeName);
  require(
      objectClass.isValid() && attribute.isValid(),
      "NMR received-TSO attribute FOM lookup returned an invalid object or attribute handle");
  requester.rtiAmbassador().publishObjectClassAttributes(objectClass, {attribute});
  auto const object = requester.rtiAmbassador().registerObjectInstance(objectClass);
  require(object.isValid(), "NMR received-TSO attribute object registration returned an invalid handle");
  requester.rtiAmbassador().changeAttributeOrderType(
      object,
      {attribute},
      rti::TIMESTAMP);

  auto requesterTime = makeTimeContext(requester);
  auto peerTime = makeTimeContext(peer);
  require(
      requesterTime.factory->getName() == peerTime.factory->getName(),
      "NMR received-TSO attribute members selected different logical-time factories");
  requireTimeEquals(
      *requesterTime.initial,
      *peerTime.initial,
      "NMR received-TSO attribute initial logical time");

  requester.rtiAmbassador().enableTimeConstrained();
  waitFor(
      requester,
      [&] { return requester.recorder().timeConstrainedEnabled().size() == 1U; },
      options,
      "NMR received-TSO attribute requester Time Constrained callback");
  requester.rtiAmbassador().enableTimeRegulation(*requesterTime.epsilon);
  waitFor(
      requester,
      [&] { return requester.recorder().timeRegulationEnabled().size() == 1U; },
      options,
      "NMR received-TSO attribute requester Time Regulation callback");
  peer.rtiAmbassador().enableTimeRegulation(*peerTime.epsilon);
  waitFor(
      peer,
      [&] { return peer.recorder().timeRegulationEnabled().size() == 1U; },
      options,
      "NMR received-TSO attribute peer Time Regulation callback");
  requester.recorder().clearTimeCallbacks();
  peer.recorder().clearTimeCallbacks();

  auto actualLookahead = requesterTime.factory->makeZero();
  require(
      actualLookahead != nullptr,
      "NMR received-TSO attribute floor could not allocate a lookahead query value");
  requester.rtiAmbassador().queryLookahead(*actualLookahead);
  require(
      *actualLookahead == *requesterTime.epsilon,
      "NMR received-TSO attribute requester did not retain its enabled epsilon lookahead");

  auto const messageTime = timeAfter(
      *peerTime.factory,
      *peerTime.initial,
      *peerTime.epsilon,
      5U);
  auto const requestTime = timeAfter(
      *requesterTime.factory,
      *requesterTime.initial,
      *requesterTime.epsilon,
      10U);
  auto const peerTarget = timeAfter(
      *peerTime.factory,
      *peerTime.initial,
      *peerTime.epsilon,
      4U);
  std::vector<std::uint8_t> incomingBytes{0x4EU, 0x4DU, 0x52U};
  std::vector<std::uint8_t> incomingTagBytes{0x4EU, 0x4DU, 0x52U, 0x2DU, 0x41U};
  rti::ParameterHandleValueMap incomingParameters;
  incomingParameters.emplace(
      peerParameter,
      rti::VariableLengthData(incomingBytes.data(), incomingBytes.size()));
  rti::VariableLengthData incomingTag(incomingTagBytes.data(), incomingTagBytes.size());
  auto const incomingRetraction = peer.rtiAmbassador().sendInteraction(
      peerInteraction,
      incomingParameters,
      incomingTag,
      *messageTime);
  require(
      incomingRetraction.isValid(),
      "NMR received-TSO attribute input returned an invalid retraction handle");
  require(
      requester.recorder().timedInteractions().empty(),
      "NMR received-TSO attribute input was delivered before the request");

  requester.rtiAmbassador().nextMessageRequest(*requestTime);
  require(
      requester.recorder().timeAdvanceGrants().empty(),
      "NMR received-TSO attribute requester granted before the peer advanced");
  peer.rtiAmbassador().timeAdvanceRequest(*peerTarget);
  waitFor(
      peer,
      requester,
      [&] {
        return peer.recorder().timeAdvanceGrants().size() == 1U &&
            requester.recorder().timeAdvanceGrants().size() == 1U &&
            requester.recorder().timedInteractions().size() == 1U;
      },
      options,
      "NMR received-TSO attribute delivery and grant");
  require(
      peer.recorder().timeAdvanceGrants().front().encoded == encodeTime(*peerTarget),
      "NMR received-TSO attribute peer did not advance to its requested bound");
  require(
      requester.recorder().timedInteractions().front().time == encodeTime(*messageTime) &&
          requester.recorder().timeAdvanceGrants().front().encoded == encodeTime(*messageTime),
      "NMR received-TSO attribute setup did not deliver and grant at the message time");

  std::vector<std::uint8_t> attributeBytes{0x4EU, 0x4DU, 0x52U, 0x2DU, 0x41U};
  rti::AttributeHandleValueMap attributeValues;
  attributeValues.emplace(
      attribute,
      rti::VariableLengthData(attributeBytes.data(), attributeBytes.size()));
  rti::VariableLengthData attributeTag(incomingTagBytes.data(), incomingTagBytes.size());
  requireException(
      [&] {
        requester.rtiAmbassador().updateAttributeValues(
            object,
            attributeValues,
            attributeTag,
            *messageTime);
      },
      L"InvalidLogicalTime",
      "NMR received-TSO timestamped attribute update at grant-time floor");

  auto const legalTime = timeAfter(
      *requesterTime.factory,
      *messageTime,
      *actualLookahead,
      1U);
  auto const updateRetraction = requester.rtiAmbassador().updateAttributeValues(
      object,
      attributeValues,
      attributeTag,
      *legalTime);
  require(
      updateRetraction.isValid(),
      "NMR received-TSO timestamped attribute update above the actual-lookahead floor returned an invalid handle");

  requester.resign(rti::DELETE_OBJECTS);
  peer.resign(rti::NO_ACTION);
  requester.rtiAmbassador().destroyFederationExecution(federation);
  requester.disconnect();
  peer.disconnect();
}

void scenarioNmrReceivedTsoAttributeUpdateFloorContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioNmrReceivedTsoAttributeUpdateFloor(options, model);
}

void scenarioNmrReceivedTsoDirectedInteractionFloor(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.logicalTimeImplementationName.empty(),
      "NMR received-TSO directed-interaction floor requires an adapter-supplied logical-time implementation");
  require(
      !options.secondaryDirectedObjectClassName.empty() &&
          !options.secondaryDirectedInteractionClassName.empty(),
      "NMR received-TSO directed-interaction floor requires adapter-supplied directed FOM names");

  Session requester(options, model, "owner");
  Session peer(options, model, "member");
  auto const federation = federationName(options, "nmr-received-tso-directed-floor");
  connectAndJoin(requester, peer, options, federation, options.fom);

  auto const requesterIncomingInteraction =
      requester.rtiAmbassador().getInteractionClassHandle(options.interactionClassName);
  auto const peerIncomingInteraction =
      peer.rtiAmbassador().getInteractionClassHandle(options.interactionClassName);
  auto const requesterIncomingParameter = requester.rtiAmbassador().getParameterHandle(
      requesterIncomingInteraction,
      options.parameterName);
  auto const peerIncomingParameter = peer.rtiAmbassador().getParameterHandle(
      peerIncomingInteraction,
      options.parameterName);
  auto const requesterDirectedInteraction =
      requester.rtiAmbassador().getInteractionClassHandle(
          options.secondaryDirectedInteractionClassName);
  auto const peerDirectedInteraction = peer.rtiAmbassador().getInteractionClassHandle(
      options.secondaryDirectedInteractionClassName);
  require(
      requesterIncomingInteraction.isValid() && peerIncomingInteraction.isValid() &&
          requesterIncomingParameter.isValid() && peerIncomingParameter.isValid() &&
          requesterDirectedInteraction.isValid() && peerDirectedInteraction.isValid(),
      "NMR received-TSO directed-interaction FOM lookup returned an invalid handle");
  require(
      requesterIncomingInteraction == peerIncomingInteraction &&
          requesterIncomingParameter == peerIncomingParameter &&
          requesterDirectedInteraction == peerDirectedInteraction,
      "NMR received-TSO directed-interaction handles differed across members");

  auto const requesterObjectClass = requester.rtiAmbassador().getObjectClassHandle(
      options.secondaryDirectedObjectClassName);
  auto const peerObjectClass = peer.rtiAmbassador().getObjectClassHandle(
      options.secondaryDirectedObjectClassName);
  auto const requesterAttribute = requester.rtiAmbassador().getAttributeHandle(
      requesterObjectClass,
      options.attributeName);
  auto const peerAttribute = peer.rtiAmbassador().getAttributeHandle(
      peerObjectClass,
      options.attributeName);
  require(
      requesterObjectClass.isValid() && peerObjectClass.isValid() &&
          requesterAttribute.isValid() && peerAttribute.isValid(),
      "NMR received-TSO directed-interaction target FOM lookup returned an invalid handle");
  require(
      requesterObjectClass == peerObjectClass && requesterAttribute == peerAttribute,
      "NMR received-TSO directed-interaction target handles differed across members");

  requester.rtiAmbassador().subscribeInteractionClass(requesterIncomingInteraction);
  peer.rtiAmbassador().publishInteractionClass(peerIncomingInteraction);
  peer.rtiAmbassador().changeInteractionOrderType(
      peerIncomingInteraction,
      rti::TIMESTAMP);
  requester.rtiAmbassador().publishObjectClassAttributes(
      requesterObjectClass,
      {requesterAttribute});
  peer.rtiAmbassador().subscribeObjectClassAttributes(
      peerObjectClass,
      {peerAttribute},
      true,
      L"");
  rti::InteractionClassHandleSet requesterDirectedInteractions;
  requesterDirectedInteractions.insert(requesterDirectedInteraction);
  rti::InteractionClassHandleSet peerDirectedInteractions;
  peerDirectedInteractions.insert(peerDirectedInteraction);
  requester.rtiAmbassador().publishObjectClassDirectedInteractions(
      requesterObjectClass,
      requesterDirectedInteractions);
  peer.rtiAmbassador().subscribeObjectClassDirectedInteractions(
      peerObjectClass,
      peerDirectedInteractions,
      true);
  requester.rtiAmbassador().changeInteractionOrderType(
      requesterDirectedInteraction,
      rti::TIMESTAMP);
  auto const target = requester.rtiAmbassador().registerObjectInstance(
      requesterObjectClass);
  require(target.isValid(), "NMR received-TSO directed target registration returned an invalid handle");

  auto requesterTime = makeTimeContext(requester);
  auto peerTime = makeTimeContext(peer);
  require(
      requesterTime.factory->getName() == peerTime.factory->getName(),
      "NMR received-TSO directed members selected different logical-time factories");
  requireTimeEquals(
      *requesterTime.initial,
      *peerTime.initial,
      "NMR received-TSO directed initial logical time");

  requester.rtiAmbassador().enableTimeConstrained();
  waitFor(
      requester,
      [&] { return requester.recorder().timeConstrainedEnabled().size() == 1U; },
      options,
      "NMR received-TSO directed requester Time Constrained callback");
  requester.rtiAmbassador().enableTimeRegulation(*requesterTime.epsilon);
  waitFor(
      requester,
      [&] { return requester.recorder().timeRegulationEnabled().size() == 1U; },
      options,
      "NMR received-TSO directed requester Time Regulation callback");
  peer.rtiAmbassador().enableTimeRegulation(*peerTime.epsilon);
  waitFor(
      peer,
      [&] { return peer.recorder().timeRegulationEnabled().size() == 1U; },
      options,
      "NMR received-TSO directed peer Time Regulation callback");
  requester.recorder().clearTimeCallbacks();
  peer.recorder().clearTimeCallbacks();

  auto actualLookahead = requesterTime.factory->makeZero();
  require(
      actualLookahead != nullptr,
      "NMR received-TSO directed floor could not allocate a lookahead query value");
  requester.rtiAmbassador().queryLookahead(*actualLookahead);
  require(
      *actualLookahead == *requesterTime.epsilon,
      "NMR received-TSO directed requester did not retain its enabled epsilon lookahead");

  auto const messageTime = timeAfter(
      *peerTime.factory,
      *peerTime.initial,
      *peerTime.epsilon,
      5U);
  auto const requestTime = timeAfter(
      *requesterTime.factory,
      *requesterTime.initial,
      *requesterTime.epsilon,
      10U);
  auto const peerTarget = timeAfter(
      *peerTime.factory,
      *peerTime.initial,
      *peerTime.epsilon,
      4U);
  std::vector<std::uint8_t> incomingBytes{0x4EU, 0x4DU, 0x52U, 0x2DU, 0x44U};
  std::vector<std::uint8_t> incomingTagBytes{0x4EU, 0x4DU, 0x52U, 0x2DU, 0x44U};
  rti::ParameterHandleValueMap incomingParameters;
  incomingParameters.emplace(
      peerIncomingParameter,
      rti::VariableLengthData(incomingBytes.data(), incomingBytes.size()));
  rti::VariableLengthData incomingTag(
      incomingTagBytes.data(),
      incomingTagBytes.size());
  auto const incomingRetraction = peer.rtiAmbassador().sendInteraction(
      peerIncomingInteraction,
      incomingParameters,
      incomingTag,
      *messageTime);
  require(
      incomingRetraction.isValid(),
      "NMR received-TSO directed input returned an invalid retraction handle");
  require(
      requester.recorder().timedInteractions().empty(),
      "NMR received-TSO directed input was delivered before the request");

  requester.rtiAmbassador().nextMessageRequest(*requestTime);
  require(
      requester.recorder().timeAdvanceGrants().empty(),
      "NMR received-TSO directed requester granted before the peer advanced");
  peer.rtiAmbassador().timeAdvanceRequest(*peerTarget);
  waitFor(
      peer,
      requester,
      [&] {
        return peer.recorder().timeAdvanceGrants().size() == 1U &&
            requester.recorder().timeAdvanceGrants().size() == 1U &&
            requester.recorder().timedInteractions().size() == 1U;
      },
      options,
      "NMR received-TSO directed delivery and grant");
  require(
      requester.recorder().timedInteractions().front().time == encodeTime(*messageTime) &&
          requester.recorder().timeAdvanceGrants().front().encoded == encodeTime(*messageTime),
      "NMR received-TSO directed setup did not deliver and grant at the message time");

  std::vector<std::uint8_t> directedTagBytes{0x4EU, 0x4DU, 0x52U, 0x2DU, 0x44U};
  rti::VariableLengthData directedTag(
      directedTagBytes.data(),
      directedTagBytes.size());
  rti::ParameterHandleValueMap directedParameters;
  requireException(
      [&] {
        requester.rtiAmbassador().sendDirectedInteraction(
            requesterDirectedInteraction,
            target,
            directedParameters,
            directedTag,
            *messageTime);
      },
      L"InvalidLogicalTime",
      "NMR received-TSO timestamped directed interaction at grant-time floor");

  auto const legalTime = timeAfter(
      *requesterTime.factory,
      *messageTime,
      *actualLookahead,
      1U);
  static_cast<void>(requester.rtiAmbassador().sendDirectedInteraction(
      requesterDirectedInteraction,
      target,
      directedParameters,
      directedTag,
      *legalTime));

  requester.resign(rti::DELETE_OBJECTS);
  peer.resign(rti::NO_ACTION);
  requester.rtiAmbassador().destroyFederationExecution(federation);
  requester.disconnect();
  peer.disconnect();
}

void scenarioNmrReceivedTsoDirectedInteractionFloorContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioNmrReceivedTsoDirectedInteractionFloor(options, model);
}

void scenarioNmrEmptyMessageSetGrantTime(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.logicalTimeImplementationName.empty(),
      "NMR empty-message grant-time test requires an adapter-supplied logical-time implementation");

  Session requester(options, model, "owner");
  Session peer(options, model, "member");
  auto const federation = federationName(options, "nmr-empty-message-set-grant-time");
  connectAndJoin(requester, peer, options, federation, options.fom);

  auto requesterTime = makeTimeContext(requester);
  auto peerTime = makeTimeContext(peer);
  require(
      requesterTime.factory->getName() == peerTime.factory->getName(),
      "NMR empty-message members selected different logical-time factories");
  requireTimeEquals(
      *requesterTime.initial,
      *peerTime.initial,
      "NMR empty-message initial logical time");

  requester.rtiAmbassador().enableTimeConstrained();
  waitFor(
      requester,
      [&] { return requester.recorder().timeConstrainedEnabled().size() == 1U; },
      options,
      "NMR empty-message requester Time Constrained callback");
  requester.rtiAmbassador().enableTimeRegulation(*requesterTime.epsilon);
  waitFor(
      requester,
      [&] { return requester.recorder().timeRegulationEnabled().size() == 1U; },
      options,
      "NMR empty-message requester Time Regulation callback");
  peer.rtiAmbassador().enableTimeRegulation(*peerTime.epsilon);
  waitFor(
      peer,
      [&] { return peer.recorder().timeRegulationEnabled().size() == 1U; },
      options,
      "NMR empty-message peer Time Regulation callback");
  requester.recorder().clearTimeCallbacks();
  peer.recorder().clearTimeCallbacks();

  auto const requestTime = timeAfter(
      *requesterTime.factory,
      *requesterTime.initial,
      *requesterTime.epsilon,
      10U);
  auto const peerTarget = timeAfter(
      *peerTime.factory,
      *peerTime.initial,
      *peerTime.epsilon,
      10U);
  requireTimeEquals(
      *requestTime,
      *peerTarget,
      "NMR empty-message request and peer advance target");

  requester.rtiAmbassador().nextMessageRequest(*requestTime);
  require(
      requester.recorder().timeAdvanceGrants().empty(),
      "NMR empty-message requester granted before the peer advanced");
  peer.rtiAmbassador().timeAdvanceRequest(*peerTarget);
  waitFor(
      peer,
      requester,
      [&] {
        return peer.recorder().timeAdvanceGrants().size() == 1U &&
            requester.recorder().timeAdvanceGrants().size() == 1U;
      },
      options,
      "NMR empty-message peer and requester grants");

  auto const requesterGrants = requester.recorder().timeAdvanceGrants();
  auto const peerGrants = peer.recorder().timeAdvanceGrants();
  require(
      requesterGrants.size() == 1U &&
          requesterGrants.front().encoded == encodeTime(*requestTime),
      "NMR with no delivered TSO did not grant exactly once at the requested logical time");
  require(
      peerGrants.size() == 1U && peerGrants.front().encoded == encodeTime(*peerTarget),
      "NMR empty-message peer did not reach its requested advance time");
  require(
      requester.recorder().timedInteractions().empty(),
      "NMR empty-message grant unexpectedly delivered a timestamped interaction");
  auto currentTime = requesterTime.factory->makeInitial();
  require(
      currentTime != nullptr,
      "NMR empty-message grant could not allocate a logical-time query value");
  requester.rtiAmbassador().queryLogicalTime(*currentTime);
  requireTimeEquals(
      *currentTime,
      *requestTime,
      "NMR empty-message queried logical time");

  requester.resign(rti::NO_ACTION);
  peer.resign(rti::NO_ACTION);
  requester.rtiAmbassador().destroyFederationExecution(federation);
  requester.disconnect();
  peer.disconnect();
}

void scenarioNmrEmptyMessageSetGrantTimeContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioNmrEmptyMessageSetGrantTime(options, model);
}

void scenarioNmrRequestBelowGaltImmediateGrant(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.logicalTimeImplementationName.empty(),
      "NMR below-GALT test requires an adapter-supplied logical-time implementation");

  Session regulator(options, model, "owner");
  Session requester(options, model, "member");
  auto const federation = federationName(options, "nmr-request-below-galt-grant");
  connectAndJoin(regulator, requester, options, federation, options.fom);

  auto regulatorTime = makeTimeContext(regulator);
  auto requesterTime = makeTimeContext(requester);
  require(
      regulatorTime.factory->getName() == requesterTime.factory->getName(),
      "NMR below-GALT members selected different logical-time factories");
  requireTimeEquals(
      *regulatorTime.initial,
      *requesterTime.initial,
      "NMR below-GALT initial logical time");

  requester.rtiAmbassador().enableTimeConstrained();
  waitFor(
      requester,
      [&] { return requester.recorder().timeConstrainedEnabled().size() == 1U; },
      options,
      "NMR below-GALT requester Time Constrained callback");

  auto const lookaheadTarget = timeAfter(
      *regulatorTime.factory,
      *regulatorTime.initial,
      *regulatorTime.epsilon,
      5U);
  auto regulatorLookahead = regulatorTime.factory->makeZero();
  require(
      regulatorLookahead != nullptr,
      "NMR below-GALT regulator could not allocate a lookahead interval");
  regulatorLookahead->setToDifference(*lookaheadTarget, *regulatorTime.initial);
  regulator.rtiAmbassador().enableTimeRegulation(*regulatorLookahead);
  waitFor(
      regulator,
      [&] { return regulator.recorder().timeRegulationEnabled().size() == 1U; },
      options,
      "NMR below-GALT regulator Time Regulation callback");

  auto observedLookahead = regulatorTime.factory->makeZero();
  require(
      observedLookahead != nullptr,
      "NMR below-GALT regulator could not allocate a lookahead query value");
  regulator.rtiAmbassador().queryLookahead(*observedLookahead);
  require(
      *observedLookahead == *regulatorLookahead,
      "NMR below-GALT regulator did not retain the configured lookahead");
  auto expectedGalt = timeAfter(
      *requesterTime.factory,
      *requesterTime.initial,
      *requesterTime.epsilon,
      5U);
  auto observedGalt = requesterTime.factory->makeInitial();
  require(
      observedGalt != nullptr && requester.rtiAmbassador().queryGALT(*observedGalt),
      "NMR below-GALT requester could not observe a defined GALT");
  requireTimeEquals(
      *observedGalt,
      *expectedGalt,
      "NMR below-GALT regulator boundary");
  auto const requestTime = timeAfter(
      *requesterTime.factory,
      *requesterTime.initial,
      *requesterTime.epsilon,
      3U);
  require(
      requester.recorder().timeAdvanceGrants().empty() &&
          regulator.recorder().timeAdvanceGrants().empty(),
      "NMR below-GALT test began with an unexpected grant callback");

  requester.rtiAmbassador().nextMessageRequest(*requestTime);
  waitFor(
      requester,
      [&] { return requester.recorder().timeAdvanceGrants().size() == 1U; },
      options,
      "NMR request strictly below GALT grant");
  auto const requesterGrants = requester.recorder().timeAdvanceGrants();
  require(
      requesterGrants.size() == 1U &&
          requesterGrants.front().encoded == encodeTime(*requestTime),
      "NMR request below GALT did not grant exactly once at its requested time");
  require(
      regulator.recorder().timeAdvanceGrants().empty(),
      "NMR below-GALT request required or produced a regulator grant");
  auto regulatorCurrentTime = regulatorTime.factory->makeInitial();
  auto requesterCurrentTime = requesterTime.factory->makeInitial();
  require(
      regulatorCurrentTime != nullptr && requesterCurrentTime != nullptr,
      "NMR below-GALT test could not allocate queried logical times");
  regulator.rtiAmbassador().queryLogicalTime(*regulatorCurrentTime);
  requester.rtiAmbassador().queryLogicalTime(*requesterCurrentTime);
  requireTimeEquals(
      *regulatorCurrentTime,
      *regulatorTime.initial,
      "NMR below-GALT regulator time without an advance");
  requireTimeEquals(
      *requesterCurrentTime,
      *requestTime,
      "NMR below-GALT requester logical time");
  require(
      requester.rtiAmbassador().queryGALT(*observedGalt),
      "NMR below-GALT bound became undefined after the immediate grant");
  requireTimeEquals(
      *observedGalt,
      *expectedGalt,
      "NMR below-GALT boundary after immediate grant");

  requester.resign(rti::NO_ACTION);
  regulator.resign(rti::NO_ACTION);
  regulator.rtiAmbassador().destroyFederationExecution(federation);
  requester.disconnect();
  regulator.disconnect();
}

void scenarioNmrRequestBelowGaltImmediateGrantContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioNmrRequestBelowGaltImmediateGrant(options, model);
}

void scenarioNmrLitsBelowGaltImmediateGrant(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.logicalTimeImplementationName.empty(),
      "NMR queued-LITS test requires an adapter-supplied logical-time implementation");

  Session regulator(options, model, "owner");
  Session requester(options, model, "member");
  auto const federation = federationName(options, "nmr-lits-below-galt-grant");
  connectAndJoin(regulator, requester, options, federation, options.fom);

  auto const regulatorInteraction = regulator.rtiAmbassador().getInteractionClassHandle(
      options.interactionClassName);
  auto const regulatorParameter = regulator.rtiAmbassador().getParameterHandle(
      regulatorInteraction,
      options.parameterName);
  auto const requesterInteraction = requester.rtiAmbassador().getInteractionClassHandle(
      options.interactionClassName);
  auto const requesterParameter = requester.rtiAmbassador().getParameterHandle(
      requesterInteraction,
      options.parameterName);
  require(
      regulatorInteraction.isValid() && regulatorParameter.isValid() &&
          requesterInteraction.isValid() && requesterParameter.isValid(),
      "NMR queued-LITS adapter FOM did not resolve interaction handles");
  require(
      regulatorInteraction == requesterInteraction &&
          regulatorParameter == requesterParameter,
      "NMR queued-LITS interaction handles changed across federates");

  regulator.rtiAmbassador().publishInteractionClass(regulatorInteraction);
  regulator.rtiAmbassador().changeInteractionOrderType(
      regulatorInteraction,
      rti::TIMESTAMP);
  requester.rtiAmbassador().subscribeInteractionClass(requesterInteraction, true);

  auto regulatorTime = makeTimeContext(regulator);
  auto requesterTime = makeTimeContext(requester);
  require(
      regulatorTime.factory->getName() == requesterTime.factory->getName(),
      "NMR queued-LITS members selected different logical-time factories");
  requireTimeEquals(
      *regulatorTime.initial,
      *requesterTime.initial,
      "NMR queued-LITS initial logical time");

  requester.rtiAmbassador().enableTimeConstrained();
  waitFor(
      requester,
      [&] { return requester.recorder().timeConstrainedEnabled().size() == 1U; },
      options,
      "NMR queued-LITS requester Time Constrained callback");
  auto const lookahead = regulatorTime.factory->makeZero();
  require(lookahead != nullptr, "NMR queued-LITS could not allocate lookahead");
  lookahead->setToDifference(
      *timeAfter(*regulatorTime.factory, *regulatorTime.initial, *regulatorTime.epsilon, 1U),
      *regulatorTime.initial);
  regulator.rtiAmbassador().enableTimeRegulation(*lookahead);
  waitFor(
      regulator,
      [&] { return regulator.recorder().timeRegulationEnabled().size() == 1U; },
      options,
      "NMR queued-LITS regulator Time Regulation callback");
  auto observedLookahead = regulatorTime.factory->makeZero();
  require(observedLookahead != nullptr, "NMR queued-LITS could not query lookahead");
  regulator.rtiAmbassador().queryLookahead(*observedLookahead);
  require(
      *observedLookahead == *lookahead,
      "NMR queued-LITS regulator did not retain the configured lookahead");

  auto const messageTime = timeAfter(
      *regulatorTime.factory,
      *regulatorTime.initial,
      *regulatorTime.epsilon,
      3U);
  auto const regulatorTarget = timeAfter(
      *regulatorTime.factory,
      *regulatorTime.initial,
      *regulatorTime.epsilon,
      4U);
  auto const requestTime = timeAfter(
      *requesterTime.factory,
      *requesterTime.initial,
      *requesterTime.epsilon,
      10U);
  auto const expectedGalt = timeAfter(
      *requesterTime.factory,
      *requesterTime.initial,
      *requesterTime.epsilon,
      5U);
  require(
      *messageTime < *expectedGalt,
      "NMR queued-LITS message timestamp was not below the configured GALT boundary");
  std::vector<std::uint8_t> payloadBytes{0x4CU, 0x49U, 0x54U, 0x53U};
  std::vector<std::uint8_t> tagBytes{0x4EU, 0x4DU, 0x52U};
  rti::ParameterHandleValueMap values;
  values.emplace(
      regulatorParameter,
      rti::VariableLengthData(payloadBytes.data(), payloadBytes.size()));
  rti::VariableLengthData tag(tagBytes.data(), tagBytes.size());
  auto const retraction = regulator.rtiAmbassador().sendInteraction(
      regulatorInteraction,
      values,
      tag,
      *messageTime);
  require(retraction.isValid(), "NMR queued-LITS send returned an invalid retraction handle");

  regulator.rtiAmbassador().timeAdvanceRequest(*regulatorTarget);
  waitFor(
      regulator,
      [&] { return regulator.recorder().timeAdvanceGrants().size() == 1U; },
      options,
      "NMR queued-LITS regulator advance");
  require(
      regulator.recorder().timeAdvanceGrants().front().encoded == encodeTime(*regulatorTarget),
      "NMR queued-LITS regulator did not reach its requested time");
  require(
      requester.recorder().timedInteractions().empty() &&
          requester.recorder().timeAdvanceGrants().empty(),
      "NMR queued-LITS input was delivered before the request");

  requester.rtiAmbassador().nextMessageRequest(*requestTime);
  waitFor(
      requester,
      [&] {
        return requester.recorder().timedInteractions().size() == 1U &&
            requester.recorder().timeAdvanceGrants().size() == 1U;
      },
      options,
      "NMR queued-LITS delivery and grant");
  auto const received = requester.recorder().timedInteractions().front();
  require(
      received.interaction == requesterInteraction && received.parameters.size() == 1U &&
          received.parameters.count(requesterParameter) == 1U &&
          copyBytes(received.parameters.at(requesterParameter)) == payloadBytes,
      "NMR queued-LITS delivered the wrong interaction or parameter value");
  require(
      received.time == encodeTime(*messageTime) &&
          received.sentOrder == rti::TIMESTAMP &&
          received.receivedOrder == rti::TIMESTAMP,
      "NMR queued-LITS callback did not preserve the queued TSO timestamp/order");
  require(
      requester.recorder().callbackOrder() ==
          std::vector<std::string>{"interaction", "grant"},
      "NMR queued-LITS granted before delivering its queued interaction");
  auto const grants = requester.recorder().timeAdvanceGrants();
  require(
      grants.size() == 1U && grants.front().encoded == encodeTime(*messageTime),
      "NMR queued-LITS did not grant exactly at the queued interaction time");
  require(
      regulator.recorder().timeAdvanceGrants().size() == 1U,
      "NMR queued-LITS required an additional regulator advance");

  auto regulatorCurrentTime = regulatorTime.factory->makeInitial();
  auto requesterCurrentTime = requesterTime.factory->makeInitial();
  require(
      regulatorCurrentTime != nullptr && requesterCurrentTime != nullptr,
      "NMR queued-LITS could not allocate logical-time query values");
  regulator.rtiAmbassador().queryLogicalTime(*regulatorCurrentTime);
  requester.rtiAmbassador().queryLogicalTime(*requesterCurrentTime);
  requireTimeEquals(
      *regulatorCurrentTime,
      *regulatorTarget,
      "NMR queued-LITS regulator time after its single advance");
  requireTimeEquals(
      *requesterCurrentTime,
      *messageTime,
      "NMR queued-LITS requester time at LITS");

  requester.resign(rti::NO_ACTION);
  regulator.resign(rti::NO_ACTION);
  regulator.rtiAmbassador().destroyFederationExecution(federation);
}

void scenarioNmrLitsBelowGaltImmediateGrantContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioNmrLitsBelowGaltImmediateGrant(options, model);
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

  auto zeroLookahead = publisherTime.factory->makeZero();
  require(zeroLookahead != nullptr,
          "post-grant equal-time probe could not allocate zero lookahead");
  publisher.rtiAmbassador().modifyLookahead(*zeroLookahead);
  publisher.recorder().clearTimeCallbacks();
  publisher.rtiAmbassador().timeAdvanceRequest(*secondMessageTime);
  waitFor(
      publisher,
      receiver,
      [&] { return publisher.recorder().timeAdvanceGrants().size() >= 1U; },
      options,
      "post-grant equal-time sender advance");
  require(
      publisher.recorder().timeAdvanceGrants().size() == 1U &&
          publisher.recorder().timeAdvanceGrants().front().encoded ==
              encodeTime(*secondMessageTime),
      "post-grant equal-time sender did not advance to the boundary");
  auto queriedLookahead = publisherTime.factory->makeZero();
  require(queriedLookahead != nullptr,
          "post-grant equal-time probe could not query lookahead");
  publisher.rtiAmbassador().queryLookahead(*queriedLookahead);
  require(*queriedLookahead == *zeroLookahead,
          "zero lookahead was not active at the sender's boundary grant");

  std::vector<std::uint8_t> postGrantValueBytes{0x50U, 0x4FU, 0x53U, 0x54U};
  std::vector<std::uint8_t> postGrantTagBytes{0x45U, 0x51U, 0x55U, 0x41U, 0x4CU};
  rti::ParameterHandleValueMap postGrantParameters;
  postGrantParameters.emplace(
      publisherParameter,
      rti::VariableLengthData(
          postGrantValueBytes.data(), postGrantValueBytes.size()));
  rti::VariableLengthData postGrantTag(
      postGrantTagBytes.data(), postGrantTagBytes.size());
  auto const postGrantRetraction = publisher.rtiAmbassador().sendInteraction(
      publisherInteraction,
      postGrantParameters,
      postGrantTag,
      *secondMessageTime);
  require(
      postGrantRetraction.isValid(),
      "post-grant equal-time interaction was rejected with zero lookahead");

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

void scenarioTimeRegulationImplicitAdvance(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.logicalTimeImplementationName.empty(),
      "Implicit Time Regulation advance requires an adapter-supplied logical-time implementation");

  Session producer(options, model, "owner");
  Session enabling(options, model, "member");
  Session advancedPeer(options, model, "member");
  auto const federation = federationName(options, "time-regulation-implicit-advance");
  connectAndJoin(producer, enabling, options, federation, options.fom);
  advancedPeer.connect();
  advancedPeer.join(
      options.memberFederateName + L"-implicit-peer",
      options.federateType,
      federation);

  auto const producerInteraction = producer.rtiAmbassador().getInteractionClassHandle(
      options.interactionClassName);
  auto const enablingInteraction = enabling.rtiAmbassador().getInteractionClassHandle(
      options.interactionClassName);
  auto const producerParameter = producer.rtiAmbassador().getParameterHandle(
      producerInteraction,
      options.parameterName);
  auto const enablingParameter = enabling.rtiAmbassador().getParameterHandle(
      enablingInteraction,
      options.parameterName);
  require(
      producerInteraction.isValid() && enablingInteraction.isValid() &&
          producerParameter.isValid() && enablingParameter.isValid(),
      "Implicit Time Regulation advance could not resolve the adapter interaction and parameter");
  producer.rtiAmbassador().publishInteractionClass(producerInteraction);
  producer.rtiAmbassador().changeInteractionOrderType(
      producerInteraction,
      rti::TIMESTAMP);
  enabling.rtiAmbassador().subscribeInteractionClass(enablingInteraction, true);

  auto producerTime = makeTimeContext(producer);
  auto enablingTime = makeTimeContext(enabling);
  auto peerTime = makeTimeContext(advancedPeer);
  require(
      producerTime.factory->getName() == enablingTime.factory->getName() &&
          producerTime.factory->getName() == peerTime.factory->getName(),
      "Implicit Time Regulation advance members selected different logical-time factories");

  enabling.rtiAmbassador().enableTimeConstrained();
  advancedPeer.rtiAmbassador().enableTimeConstrained();
  waitFor(
      enabling,
      [&] { return enabling.recorder().timeConstrainedEnabled().size() == 1U; },
      options,
      "implicit-advance member Time Constrained callback");
  waitFor(
      advancedPeer,
      [&] { return advancedPeer.recorder().timeConstrainedEnabled().size() == 1U; },
      options,
      "implicit-advance peer Time Constrained callback");

  auto producerLookahead = producerTime.factory->makeEpsilon();
  require(producerLookahead != nullptr, "Implicit Time Regulation advance could not allocate producer lookahead");
  producer.rtiAmbassador().enableTimeRegulation(*producerLookahead);
  waitFor(
      producer,
      [&] { return producer.recorder().timeRegulationEnabled().size() == 1U; },
      options,
      "implicit-advance producer Time Regulation callback");

  auto const messageTime = timeAfter(
      *producerTime.factory,
      *producerTime.initial,
      *producerTime.epsilon,
      5U);
  auto const producerTarget = timeAfter(
      *producerTime.factory,
      *producerTime.initial,
      *producerTime.epsilon,
      10U);
  auto const peerTarget = timeAfter(
      *peerTime.factory,
      *peerTime.initial,
      *peerTime.epsilon,
      10U);
  auto const minimumFeasibleEnableTime = timeAfter(
      *enablingTime.factory,
      *enablingTime.initial,
      *enablingTime.epsilon,
      8U);
  auto const enablingLookaheadTime = timeAfter(
      *enablingTime.factory,
      *enablingTime.initial,
      *enablingTime.epsilon,
      2U);
  auto enablingLookahead = enablingTime.factory->makeZero();
  require(enablingLookahead != nullptr, "Implicit Time Regulation advance could not allocate enabling lookahead");
  enablingLookahead->setToDifference(*enablingLookaheadTime, *enablingTime.initial);

  std::vector<std::uint8_t> payload{0x49U, 0x41U, 0x41U};
  std::vector<std::uint8_t> tagBytes{0x49U, 0x41U, 0x54U};
  rti::ParameterHandleValueMap parameters;
  parameters.emplace(
      producerParameter,
      rti::VariableLengthData(payload.data(), payload.size()));
  rti::VariableLengthData tag(tagBytes.data(), tagBytes.size());
  auto const retraction = producer.rtiAmbassador().sendInteraction(
      producerInteraction,
      parameters,
      tag,
      *messageTime);
  require(
      retraction.isValid(),
      "Implicit Time Regulation advance timestamped interaction returned an invalid retraction handle");
  require(
      enabling.recorder().timedInteractions().empty(),
      "Implicit Time Regulation advance delivered the queued TSO before the member advanced");

  producer.rtiAmbassador().timeAdvanceRequest(*producerTarget);
  waitFor(
      producer,
      [&] { return producer.recorder().timeAdvanceGrants().size() == 1U; },
      options,
      "implicit-advance producer Time Advance Grant");
  advancedPeer.rtiAmbassador().timeAdvanceRequest(*peerTarget);
  waitFor(
      advancedPeer,
      [&] { return advancedPeer.recorder().timeAdvanceGrants().size() == 1U; },
      options,
      "implicit-advance peer Time Advance Grant");
  require(
      enabling.recorder().timedInteractions().empty(),
      "Implicit Time Regulation advance delivered the queued TSO before regulation was enabled");

  enabling.rtiAmbassador().enableTimeRegulation(*enablingLookahead);
  waitFor(
      enabling,
      [&] { return enabling.recorder().timeRegulationEnabled().size() == 1U; },
      options,
      "implicit-advance member Time Regulation callback");

  auto const enabled = enabling.recorder().timeRegulationEnabled();
  require(enabled.size() == 1U, "Implicit Time Regulation advance produced an unexpected callback count");
  require(
      enabled.front().encoded == copyBytes(minimumFeasibleEnableTime->encode()),
      "Enable Time Regulation did not assign the smallest feasible logical time");
  auto const callbackOrder = enabling.recorder().timeRegulationEnablementOrder();
  require(
      callbackOrder == std::vector<std::string>{"interaction", "regulation-enabled"},
      "queued TSO was not delivered before the Time Regulation Enabled callback");
  require(
      enabling.recorder().timeAdvanceGrants().empty(),
      "Implicit Time Regulation advance incorrectly produced a separate Time Advance Grant");
  auto const delivered = enabling.recorder().timedInteractions();
  require(
      delivered.size() == 1U,
      "Implicit Time Regulation advance did not deliver the queued TSO exactly once");
  auto const& delivery = delivered.front();
  require(
      delivery.interaction == enablingInteraction &&
          delivery.producer == producer.federateHandle() &&
          delivery.time == copyBytes(messageTime->encode()) &&
          delivery.sentOrder == rti::TIMESTAMP &&
          delivery.receivedOrder == rti::TIMESTAMP &&
          delivery.retractionPresent && delivery.tag == tagBytes &&
          delivery.parameters.size() == 1U &&
          copyBytes(delivery.parameters.at(enablingParameter)) == payload,
      "Implicit Time Regulation advance did not preserve the queued TSO callback data");
  auto currentTime = enablingTime.factory->makeInitial();
  require(currentTime != nullptr, "Implicit Time Regulation advance could not allocate a query time");
  enabling.rtiAmbassador().queryLogicalTime(*currentTime);
  requireTimeEquals(
      *currentTime,
      *minimumFeasibleEnableTime,
      "implicit-advance queried logical time");

  advancedPeer.resign(rti::NO_ACTION);
  enabling.resign(rti::NO_ACTION);
  producer.resign(rti::NO_ACTION);
  producer.rtiAmbassador().destroyFederationExecution(federation);
  advancedPeer.disconnect();
  enabling.disconnect();
  producer.disconnect();
}

void scenarioTimeRegulationImplicitAdvanceContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioTimeRegulationImplicitAdvance(options, model);
}

void scenarioTimeRegulationEnableTsoCutoff(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.logicalTimeImplementationName.empty(),
      "Time Regulation TSO cutoff requires an adapter-supplied logical-time implementation");

  Session producer(options, model, "owner");
  Session enabling(options, model, "member");
  Session advancedPeer(options, model, "member");
  auto const federation = federationName(options, "time-regulation-enable-tso-cutoff");
  connectAndJoin(producer, enabling, options, federation, options.fom);
  advancedPeer.connect();
  advancedPeer.join(
      options.memberFederateName + L"-tso-cutoff-peer",
      options.federateType,
      federation);

  auto const producerInteraction = producer.rtiAmbassador().getInteractionClassHandle(
      options.interactionClassName);
  auto const enablingInteraction = enabling.rtiAmbassador().getInteractionClassHandle(
      options.interactionClassName);
  auto const producerParameter = producer.rtiAmbassador().getParameterHandle(
      producerInteraction,
      options.parameterName);
  auto const enablingParameter = enabling.rtiAmbassador().getParameterHandle(
      enablingInteraction,
      options.parameterName);
  require(
      producerInteraction.isValid() && enablingInteraction.isValid() &&
          producerParameter.isValid() && enablingParameter.isValid(),
      "Time Regulation TSO cutoff could not resolve the adapter interaction and parameter");
  producer.rtiAmbassador().publishInteractionClass(producerInteraction);
  producer.rtiAmbassador().changeInteractionOrderType(
      producerInteraction,
      rti::TIMESTAMP);
  enabling.rtiAmbassador().subscribeInteractionClass(enablingInteraction, true);

  auto producerTime = makeTimeContext(producer);
  auto enablingTime = makeTimeContext(enabling);
  auto peerTime = makeTimeContext(advancedPeer);
  require(
      producerTime.factory->getName() == enablingTime.factory->getName() &&
          producerTime.factory->getName() == peerTime.factory->getName(),
      "Time Regulation TSO cutoff members selected different logical-time factories");

  enabling.rtiAmbassador().enableTimeConstrained();
  advancedPeer.rtiAmbassador().enableTimeConstrained();
  waitFor(
      enabling,
      [&] { return enabling.recorder().timeConstrainedEnabled().size() == 1U; },
      options,
      "TSO cutoff receiver Time Constrained callback");
  waitFor(
      advancedPeer,
      [&] { return advancedPeer.recorder().timeConstrainedEnabled().size() == 1U; },
      options,
      "TSO cutoff advanced peer Time Constrained callback");

  auto producerLookahead = producerTime.factory->makeEpsilon();
  require(producerLookahead != nullptr, "TSO cutoff could not allocate producer lookahead");
  producer.rtiAmbassador().enableTimeRegulation(*producerLookahead);
  waitFor(
      producer,
      [&] { return producer.recorder().timeRegulationEnabled().size() == 1U; },
      options,
      "TSO cutoff producer Time Regulation callback");

  auto const suppressedTime = timeAfter(
      *producerTime.factory,
      *producerTime.initial,
      *producerTime.epsilon,
      5U);
  auto const inclusiveTime = timeAfter(
      *producerTime.factory,
      *producerTime.initial,
      *producerTime.epsilon,
      8U);
  auto const producerTarget = timeAfter(
      *producerTime.factory,
      *producerTime.initial,
      *producerTime.epsilon,
      10U);
  auto const peerTarget = timeAfter(
      *peerTime.factory,
      *peerTime.initial,
      *peerTime.epsilon,
      10U);
  auto const assignedTime = timeAfter(
      *enablingTime.factory,
      *enablingTime.initial,
      *enablingTime.epsilon,
      8U);
  auto const enablingLookaheadTime = timeAfter(
      *enablingTime.factory,
      *enablingTime.initial,
      *enablingTime.epsilon,
      2U);
  auto enablingLookahead = enablingTime.factory->makeZero();
  require(enablingLookahead != nullptr, "TSO cutoff could not allocate receiver lookahead");
  enablingLookahead->setToDifference(*enablingLookaheadTime, *enablingTime.initial);

  std::vector<std::uint8_t> const payload{0x54U, 0x53U, 0x4FU};
  std::vector<std::uint8_t> const tagBytes{0x43U, 0x55U, 0x54U};
  rti::ParameterHandleValueMap parameters;
  parameters.emplace(
      producerParameter,
      rti::VariableLengthData(payload.data(), payload.size()));
  rti::VariableLengthData const tag(tagBytes.data(), tagBytes.size());
  auto const suppressedRetraction = producer.rtiAmbassador().sendInteraction(
      producerInteraction,
      parameters,
      tag,
      *suppressedTime);
  auto const inclusiveRetraction = producer.rtiAmbassador().sendInteraction(
      producerInteraction,
      parameters,
      tag,
      *inclusiveTime);
  require(
      suppressedRetraction.isValid() && inclusiveRetraction.isValid(),
      "TSO cutoff timestamped sends returned invalid retraction handles");
  require(
      enabling.recorder().timedInteractions().empty(),
      "TSO cutoff delivered a queued message before the federates advanced");

  producer.rtiAmbassador().timeAdvanceRequest(*producerTarget);
  waitFor(
      producer,
      [&] { return producer.recorder().timeAdvanceGrants().size() == 1U; },
      options,
      "TSO cutoff producer Time Advance Grant");
  advancedPeer.rtiAmbassador().timeAdvanceRequest(*peerTarget);
  waitFor(
      advancedPeer,
      [&] { return advancedPeer.recorder().timeAdvanceGrants().size() == 1U; },
      options,
      "TSO cutoff peer Time Advance Grant");
  require(
      enabling.recorder().timedInteractions().empty(),
      "TSO cutoff delivered a queued message before Time Regulation was enabled");

  enabling.rtiAmbassador().enableTimeRegulation(*enablingLookahead);
  waitFor(
      enabling,
      [&] { return enabling.recorder().timeRegulationEnabled().size() == 1U; },
      options,
      "TSO cutoff receiver Time Regulation callback");

  auto const enabled = enabling.recorder().timeRegulationEnabled();
  require(
      enabled.size() == 1U &&
          enabled.front().encoded == copyBytes(assignedTime->encode()),
      "Time Regulation Enabled reported the wrong assigned federation time");
  auto const callbackOrder = enabling.recorder().timeRegulationEnablementOrder();
  require(
      callbackOrder == std::vector<std::string>{"interaction", "regulation-enabled"},
      "inclusive-boundary TSO was not delivered before Time Regulation Enabled");
  require(
      enabling.recorder().timeAdvanceGrants().empty(),
      "Time Regulation TSO cutoff incorrectly produced a separate Time Advance Grant");
  auto const delivered = enabling.recorder().timedInteractions();
  require(
      delivered.size() == 1U,
      "Time Regulation enable delivered a TSO message below its assigned federation time");
  auto const& delivery = delivered.front();
  require(
      delivery.interaction == enablingInteraction &&
          delivery.producer == producer.federateHandle() &&
          delivery.parameters.size() == 1U &&
          delivery.parameters.count(enablingParameter) == 1U &&
          copyBytes(delivery.parameters.at(enablingParameter)) == payload &&
          delivery.time == copyBytes(inclusiveTime->encode()) &&
          delivery.sentOrder == rti::TIMESTAMP &&
          delivery.receivedOrder == rti::TIMESTAMP &&
          delivery.retractionPresent && delivery.tag == tagBytes,
      "Time Regulation TSO cutoff did not preserve the inclusive-boundary callback");
  auto currentTime = enablingTime.factory->makeInitial();
  require(currentTime != nullptr, "TSO cutoff could not allocate a query time");
  enabling.rtiAmbassador().queryLogicalTime(*currentTime);
  requireTimeEquals(*currentTime, *assignedTime, "TSO cutoff queried logical time");

  advancedPeer.resign(rti::NO_ACTION);
  enabling.resign(rti::NO_ACTION);
  producer.resign(rti::NO_ACTION);
  producer.rtiAmbassador().destroyFederationExecution(federation);
  advancedPeer.disconnect();
  enabling.disconnect();
  producer.disconnect();
}

void scenarioTimeRegulationEnableTsoCutoffContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioTimeRegulationEnableTsoCutoff(options, model);
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
