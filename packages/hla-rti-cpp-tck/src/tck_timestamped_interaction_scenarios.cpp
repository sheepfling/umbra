#include "tck_main_scenarios.hpp"

namespace hla_rti_cpp_tck::detail {

void scenarioTimestampedDefaultRegionInteraction(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.ddmFom.empty(),
      "Timestamped default-region interaction requires an adapter-supplied dimensional FOM");
  require(
      !options.logicalTimeImplementationName.empty(),
      "Timestamped default-region interaction requires an adapter-supplied logical-time implementation");

  Session publisher(options, model, "owner");
  Session receiver(options, model, "member");
  auto const federation = federationName(
      options,
      "timestamped-default-region-interaction");
  connectAndJoin(publisher, receiver, options, federation, options.ddmFom);

  auto const publisherHandles = ddmHandles(publisher, options);
  auto const receiverHandles = ddmHandles(receiver, options);
  verifyDdmClassDimensions(
      publisher,
      options,
      publisherHandles,
      "timestamped default-region interaction");
  publisher.rtiAmbassador().publishInteractionClass(
      publisherHandles.interactionClass);
  publisher.rtiAmbassador().changeInteractionOrderType(
      publisherHandles.interactionClass,
      rti::TIMESTAMP);
  auto const receiverRegion = createDdmRegion(
      receiver,
      receiverHandles,
      2UL,
      6UL,
      2UL,
      6UL);
  auto const receiverRegionSet = rti::RegionHandleSet{receiverRegion};
  receiver.rtiAmbassador().setConveyRegionDesignatorSetsSwitch(true);
  receiver.rtiAmbassador().subscribeInteractionClassWithRegions(
      receiverHandles.interactionClass,
      receiverRegionSet,
      true);

  auto publisherTime = makeTimeContext(publisher);
  auto receiverTime = makeTimeContext(receiver);
  require(
      publisherTime.factory->getName() == receiverTime.factory->getName(),
      "timestamped default-region interaction members selected different logical-time factories");
  auto makeLookahead = [](TimeContext const& time, std::size_t steps) {
    auto target = timeAfter(*time.factory, *time.initial, *time.epsilon, steps);
    auto result = time.factory->makeZero();
    require(
        result != nullptr,
        "timestamped default-region interaction could not allocate a lookahead interval");
    result->setToDifference(*target, *time.initial);
    return result;
  };
  auto publisherLookahead = makeLookahead(publisherTime, 5U);

  receiver.rtiAmbassador().enableTimeConstrained();
  waitFor(
      receiver,
      [&] { return receiver.recorder().timeConstrainedEnabled().size() >= 1U; },
      options,
      "timestamped default-region interaction time-constrained callback");
  publisher.rtiAmbassador().enableTimeRegulation(*publisherLookahead);
  waitFor(
      publisher,
      [&] { return publisher.recorder().timeRegulationEnabled().size() >= 1U; },
      options,
      "timestamped default-region interaction time-regulation callback");

  auto const messageTime = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      6U);
  auto const receiverTarget = timeAfter(
      *receiverTime.factory,
      *receiverTime.initial,
      *receiverTime.epsilon,
      6U);
  auto const publisherTarget = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      2U);
  std::vector<std::uint8_t> parameterBytes{0xD3U, 0x0FU};
  std::vector<std::uint8_t> tagBytes{0x54U, 0x53U, 0x4FU};
  rti::ParameterHandleValueMap parameters;
  parameters.emplace(
      publisherHandles.parameter,
      rti::VariableLengthData(parameterBytes.data(), parameterBytes.size()));
  rti::VariableLengthData tag(tagBytes.data(), tagBytes.size());
  auto const retraction = publisher.rtiAmbassador().sendInteraction(
      publisherHandles.interactionClass,
      parameters,
      tag,
      *messageTime);
  require(
      retraction.isValid(),
      "timestamped default-region interaction send returned an invalid retraction handle");
  require(
      receiver.recorder().timedInteractions().empty(),
      "timestamped default-region interaction delivered before the recipient frontier");

  receiver.rtiAmbassador().timeAdvanceRequest(*receiverTarget);
  require(
      receiver.recorder().timeAdvanceGrants().empty(),
      "timestamped default-region interaction granted before the publisher frontier");
  publisher.rtiAmbassador().timeAdvanceRequest(*publisherTarget);
  waitForSessions(
      {&publisher, &receiver},
      [&] {
        return publisher.recorder().timeAdvanceGrants().size() >= 1U &&
            receiver.recorder().timedInteractions().size() >= 1U &&
            receiver.recorder().timeAdvanceGrants().size() >= 1U;
      },
      options,
      "timestamped default-region interaction delivery and grants");

  auto const interactions = receiver.recorder().timedInteractions();
  require(
      interactions.size() == 1U,
      "timestamped default-region interaction delivered a duplicate callback");
  auto const& interaction = interactions.front();
  require(
      interaction.interaction == receiverHandles.interactionClass &&
          interaction.parameters.size() == 1U &&
          interaction.parameters.count(receiverHandles.parameter) == 1U,
      "timestamped default-region interaction returned the wrong class or parameter set");
  require(
      copyBytes(interaction.parameters.at(receiverHandles.parameter)) == parameterBytes &&
          interaction.tag == tagBytes && interaction.time == encodeTime(*messageTime),
      "timestamped default-region interaction did not preserve payload or time");
  require(
      interaction.producer == publisher.federateHandle() &&
          interaction.sentOrder == rti::TIMESTAMP &&
          interaction.receivedOrder == rti::TIMESTAMP &&
          !interaction.timeText.empty(),
      "timestamped default-region interaction returned the wrong producer or order metadata");
  require(
      interaction.transportation.isValid() &&
          !receiver.rtiAmbassador().getTransportationTypeName(
              interaction.transportation).empty(),
      "timestamped default-region interaction returned an unresolvable transport");
  require(
      interaction.regions.has_value() && interaction.regions->empty(),
      "timestamped default-region interaction returned a non-empty source region");
  require(
      interaction.retractionPresent &&
          interaction.retraction == copyBytes(retraction.encode()),
      "timestamped default-region interaction omitted its retraction handle");
  require(
      publisher.recorder().timeAdvanceGrants().size() == 1U &&
          publisher.recorder().timeAdvanceGrants().front().encoded ==
              encodeTime(*publisherTarget),
      "timestamped default-region interaction returned the wrong publisher grant");
  require(
      receiver.recorder().timeAdvanceGrants().size() == 1U &&
          receiver.recorder().timeAdvanceGrants().front().encoded ==
              encodeTime(*receiverTarget),
      "timestamped default-region interaction returned the wrong receiver grant");
  require(
      receiver.recorder().callbackOrder() ==
          std::vector<std::string>{"interaction", "grant"},
      "timestamped default-region interaction delivered after its grant");
  auto queriedTime = receiverTime.factory->makeInitial();
  require(
      queriedTime != nullptr,
      "timestamped default-region interaction could not allocate a query time");
  receiver.rtiAmbassador().queryLogicalTime(*queriedTime);
  requireTimeEquals(
      *queriedTime,
      *receiverTarget,
      "timestamped default-region interaction logical-time query");
  requireException(
      [&] { publisher.rtiAmbassador().retract(retraction); },
      L"MessageCanNoLongerBeRetracted",
      "timestamped default-region interaction post-delivery retraction");

  receiver.rtiAmbassador().disableTimeConstrained();
  publisher.rtiAmbassador().disableTimeRegulation();
  receiver.rtiAmbassador().unsubscribeInteractionClassWithRegions(
      receiverHandles.interactionClass,
      receiverRegionSet);
  receiver.rtiAmbassador().deleteRegion(receiverRegion);
  receiver.resign(rti::NO_ACTION);
  publisher.resign(rti::CANCEL_THEN_DELETE_THEN_DIVEST);
  publisher.rtiAmbassador().destroyFederationExecution(federation);
  receiver.disconnect();
  publisher.disconnect();
}

void scenarioTimestampedDefaultRegionInteractionContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioTimestampedDefaultRegionInteraction(options, model);
}

void scenarioTimestampedDefaultRegionInteractionAlternateAdvances(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.ddmFom.empty(),
      "Timestamped default-region interaction alternate advances requires an adapter-supplied dimensional FOM");
  require(
      !options.logicalTimeImplementationName.empty(),
      "Timestamped default-region interaction alternate advances requires an adapter-supplied logical-time implementation");

  Session publisher(options, model, "owner");
  Session flushQueue(options, model, "member");
  Session timeAdvanceAvailable(options, model, "time-advance-available-member");
  Session nextMessageAvailable(options, model, "next-message-available-member");
  auto const federation = federationName(
      options,
      "timestamped-default-region-interaction-alternate-advances");
  connectAndJoin(publisher, flushQueue, options, federation, options.ddmFom);
  timeAdvanceAvailable.connect();
  timeAdvanceAvailable.join(
      options.memberFederateName + L"-default-region-interaction-tara",
      options.federateType,
      federation);
  nextMessageAvailable.connect();
  nextMessageAvailable.join(
      options.memberFederateName + L"-default-region-interaction-nmra",
      options.federateType,
      federation);

  auto const publisherHandles = ddmHandles(publisher, options);
  auto const flushQueueHandles = ddmHandles(flushQueue, options);
  auto const timeAdvanceAvailableHandles = ddmHandles(timeAdvanceAvailable, options);
  auto const nextMessageAvailableHandles = ddmHandles(nextMessageAvailable, options);
  verifyDdmClassDimensions(
      publisher,
      options,
      publisherHandles,
      "timestamped default-region interaction alternate advances");
  publisher.rtiAmbassador().publishInteractionClass(
      publisherHandles.interactionClass);
  publisher.rtiAmbassador().changeInteractionOrderType(
      publisherHandles.interactionClass,
      rti::TIMESTAMP);

  auto const flushQueueRegion = createDdmRegion(
      flushQueue,
      flushQueueHandles,
      2UL,
      6UL,
      2UL,
      6UL);
  auto const timeAdvanceAvailableRegion = createDdmRegion(
      timeAdvanceAvailable,
      timeAdvanceAvailableHandles,
      2UL,
      6UL,
      2UL,
      6UL);
  auto const nextMessageAvailableRegion = createDdmRegion(
      nextMessageAvailable,
      nextMessageAvailableHandles,
      2UL,
      6UL,
      2UL,
      6UL);
  auto const flushQueueRegionSet = rti::RegionHandleSet{flushQueueRegion};
  auto const timeAdvanceAvailableRegionSet =
      rti::RegionHandleSet{timeAdvanceAvailableRegion};
  auto const nextMessageAvailableRegionSet =
      rti::RegionHandleSet{nextMessageAvailableRegion};
  flushQueue.rtiAmbassador().setConveyRegionDesignatorSetsSwitch(true);
  timeAdvanceAvailable.rtiAmbassador().setConveyRegionDesignatorSetsSwitch(true);
  nextMessageAvailable.rtiAmbassador().setConveyRegionDesignatorSetsSwitch(true);
  flushQueue.rtiAmbassador().subscribeInteractionClassWithRegions(
      flushQueueHandles.interactionClass,
      flushQueueRegionSet,
      true);
  timeAdvanceAvailable.rtiAmbassador().subscribeInteractionClassWithRegions(
      timeAdvanceAvailableHandles.interactionClass,
      timeAdvanceAvailableRegionSet,
      true);
  nextMessageAvailable.rtiAmbassador().subscribeInteractionClassWithRegions(
      nextMessageAvailableHandles.interactionClass,
      nextMessageAvailableRegionSet,
      true);

  auto publisherTime = makeTimeContext(publisher);
  auto flushQueueTime = makeTimeContext(flushQueue);
  auto timeAdvanceAvailableTime = makeTimeContext(timeAdvanceAvailable);
  auto nextMessageAvailableTime = makeTimeContext(nextMessageAvailable);
  require(
      publisherTime.factory->getName() == flushQueueTime.factory->getName() &&
          publisherTime.factory->getName() == timeAdvanceAvailableTime.factory->getName() &&
          publisherTime.factory->getName() == nextMessageAvailableTime.factory->getName(),
      "timestamped default-region interaction alternate members selected different logical-time factories");
  for (auto* member :
       std::vector<Session*>{&flushQueue, &timeAdvanceAvailable, &nextMessageAvailable}) {
    member->rtiAmbassador().enableTimeConstrained();
    waitFor(
        *member,
        [&] { return member->recorder().timeConstrainedEnabled().size() >= 1U; },
        options,
        "timestamped default-region interaction alternate time-constrained callback");
  }
  publisher.rtiAmbassador().enableTimeRegulation(*publisherTime.epsilon);
  waitFor(
      publisher,
      [&] { return publisher.recorder().timeRegulationEnabled().size() >= 1U; },
      options,
      "timestamped default-region interaction alternate time-regulation callback");

  auto const messageTime = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      3U);
  auto const publisherTarget = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      2U);
  auto const requestBoundary = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      5U);
  std::vector<std::uint8_t> parameterBytes{0x41U, 0x56U, 0x41U, 0x2DU, 0x52U};
  std::vector<std::uint8_t> tagBytes{0x41U, 0x56U, 0x41U, 0x2DU, 0x44U};
  rti::ParameterHandleValueMap parameters;
  parameters.emplace(
      publisherHandles.parameter,
      rti::VariableLengthData(parameterBytes.data(), parameterBytes.size()));
  rti::VariableLengthData tag(tagBytes.data(), tagBytes.size());
  auto const retraction = publisher.rtiAmbassador().sendInteraction(
      publisherHandles.interactionClass,
      parameters,
      tag,
      *messageTime);
  require(
      retraction.isValid(),
      "timestamped default-region interaction alternate send returned an invalid retraction handle");
  require(
      flushQueue.recorder().timedInteractions().empty() &&
          timeAdvanceAvailable.recorder().timedInteractions().empty() &&
          nextMessageAvailable.recorder().timedInteractions().empty(),
      "timestamped default-region interaction alternate delivered before its request");

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
      options,
      "timestamped default-region interaction alternate deliveries and grants");

  require(
      publisher.recorder().timeAdvanceGrants().size() == 1U &&
          flushQueue.recorder().timedInteractions().size() == 1U &&
          flushQueue.recorder().flushQueueGrants().size() == 1U &&
          timeAdvanceAvailable.recorder().timedInteractions().size() == 1U &&
          timeAdvanceAvailable.recorder().timeAdvanceGrants().size() == 1U &&
          nextMessageAvailable.recorder().timedInteractions().size() == 1U &&
          nextMessageAvailable.recorder().timeAdvanceGrants().size() == 1U,
      "timestamped default-region interaction alternate returned the wrong callback counts");
  require(
      flushQueue.recorder().timeAdvanceGrants().empty() &&
          timeAdvanceAvailable.recorder().flushQueueGrants().empty() &&
          nextMessageAvailable.recorder().flushQueueGrants().empty(),
      "timestamped default-region interaction alternate mixed grant types");
  require(
      flushQueue.recorder().callbackOrder() ==
          std::vector<std::string>{"interaction", "flush-grant"} &&
          timeAdvanceAvailable.recorder().callbackOrder() ==
              std::vector<std::string>{"interaction", "grant"} &&
          nextMessageAvailable.recorder().callbackOrder() ==
              std::vector<std::string>{"interaction", "grant"},
      "timestamped default-region interaction alternate granted before delivery");

  auto assertDefaultRegionInteraction = [&](Session& receiver,
                                             DdmHandles const& receiverHandles,
                                             TimedInteractionRecord const& record) {
    require(
        record.interaction == receiverHandles.interactionClass &&
            record.parameters.size() == 1U &&
            record.parameters.count(receiverHandles.parameter) == 1U &&
            copyBytes(record.parameters.at(receiverHandles.parameter)) == parameterBytes,
        "timestamped default-region interaction alternate returned the wrong class or parameter");
    require(
        record.tag == tagBytes && record.time == encodeTime(*messageTime) &&
            !record.timeText.empty(),
        "timestamped default-region interaction alternate did not preserve value metadata");
    require(
        record.producer == publisher.federateHandle() &&
            record.sentOrder == rti::TIMESTAMP &&
            record.receivedOrder == rti::TIMESTAMP,
        "timestamped default-region interaction alternate returned the wrong producer or order");
    require(
        record.transportation.isValid() &&
            !receiver.rtiAmbassador().getTransportationTypeName(
                record.transportation).empty(),
        "timestamped default-region interaction alternate returned an unresolvable transport");
    require(
        record.regions.has_value() && record.regions->empty(),
        "timestamped default-region interaction alternate returned a non-empty source region");
    require(
        record.retractionPresent &&
            record.retraction == copyBytes(retraction.encode()),
        "timestamped default-region interaction alternate omitted the retraction handle");
  };
  assertDefaultRegionInteraction(
      flushQueue,
      flushQueueHandles,
      flushQueue.recorder().timedInteractions().front());
  assertDefaultRegionInteraction(
      timeAdvanceAvailable,
      timeAdvanceAvailableHandles,
      timeAdvanceAvailable.recorder().timedInteractions().front());
  assertDefaultRegionInteraction(
      nextMessageAvailable,
      nextMessageAvailableHandles,
      nextMessageAvailable.recorder().timedInteractions().front());
  require(
      publisher.recorder().timeAdvanceGrants().front().encoded ==
          encodeTime(*publisherTarget),
      "timestamped default-region interaction alternate returned the wrong publisher grant");
  auto const flushGrants = flushQueue.recorder().flushQueueGrants();
  require(
      flushGrants.front().optimisticTime == encodeTime(*messageTime) &&
          !flushGrants.front().time.empty() &&
          !flushGrants.front().timeText.empty() &&
          !flushGrants.front().optimisticTimeText.empty(),
      "timestamped default-region interaction alternate returned wrong Flush Queue metadata");
  auto actualFlushTime = decodeTime(*flushQueueTime.factory, flushGrants.front().time);
  auto optimisticFlushTime = decodeTime(
      *flushQueueTime.factory,
      flushGrants.front().optimisticTime);
  require(
      actualFlushTime != nullptr && optimisticFlushTime != nullptr &&
          !(*actualFlushTime < *flushQueueTime.initial) &&
          !(*requestBoundary < *actualFlushTime) &&
          !(*optimisticFlushTime < *actualFlushTime),
      "timestamped default-region interaction alternate Flush Queue bounds were invalid");
  require(
      timeAdvanceAvailable.recorder().timeAdvanceGrants().front().encoded ==
          encodeTime(*messageTime) &&
          nextMessageAvailable.recorder().timeAdvanceGrants().front().encoded ==
              encodeTime(*messageTime),
      "timestamped default-region interaction alternate returned the wrong recipient grants");
  auto queriedFlushQueue = flushQueueTime.factory->makeInitial();
  auto queriedTimeAdvanceAvailable = timeAdvanceAvailableTime.factory->makeInitial();
  auto queriedNextMessageAvailable = nextMessageAvailableTime.factory->makeInitial();
  require(
      queriedFlushQueue != nullptr && queriedTimeAdvanceAvailable != nullptr &&
          queriedNextMessageAvailable != nullptr,
      "timestamped default-region interaction alternate could not allocate query times");
  flushQueue.rtiAmbassador().queryLogicalTime(*queriedFlushQueue);
  timeAdvanceAvailable.rtiAmbassador().queryLogicalTime(*queriedTimeAdvanceAvailable);
  nextMessageAvailable.rtiAmbassador().queryLogicalTime(*queriedNextMessageAvailable);
  require(
      encodeTime(*queriedFlushQueue) == flushGrants.front().time &&
          *queriedTimeAdvanceAvailable == *messageTime &&
          *queriedNextMessageAvailable == *messageTime,
      "timestamped default-region interaction alternate query returned the wrong time");

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
      "timestamped default-region interaction alternate Flush Queue past-time request");
  requireException(
      [&] {
        timeAdvanceAvailable.rtiAmbassador().timeAdvanceRequestAvailable(
            *pastTimeAdvanceAvailableTime);
      },
      L"LogicalTimeAlreadyPassed",
      "timestamped default-region interaction alternate TARA past-time request");
  requireException(
      [&] {
        nextMessageAvailable.rtiAmbassador().nextMessageRequestAvailable(
            *pastNextMessageAvailableTime);
      },
      L"LogicalTimeAlreadyPassed",
      "timestamped default-region interaction alternate NMRA past-time request");
  requireException(
      [&] { publisher.rtiAmbassador().retract(retraction); },
      L"MessageCanNoLongerBeRetracted",
      "timestamped default-region interaction alternate post-delivery retraction");

  flushQueue.rtiAmbassador().disableTimeConstrained();
  timeAdvanceAvailable.rtiAmbassador().disableTimeConstrained();
  nextMessageAvailable.rtiAmbassador().disableTimeConstrained();
  publisher.rtiAmbassador().disableTimeRegulation();
  flushQueue.rtiAmbassador().unsubscribeInteractionClassWithRegions(
      flushQueueHandles.interactionClass,
      flushQueueRegionSet);
  timeAdvanceAvailable.rtiAmbassador().unsubscribeInteractionClassWithRegions(
      timeAdvanceAvailableHandles.interactionClass,
      timeAdvanceAvailableRegionSet);
  nextMessageAvailable.rtiAmbassador().unsubscribeInteractionClassWithRegions(
      nextMessageAvailableHandles.interactionClass,
      nextMessageAvailableRegionSet);
  publisher.rtiAmbassador().unpublishInteractionClass(
      publisherHandles.interactionClass);
  flushQueue.rtiAmbassador().deleteRegion(flushQueueRegion);
  timeAdvanceAvailable.rtiAmbassador().deleteRegion(timeAdvanceAvailableRegion);
  nextMessageAvailable.rtiAmbassador().deleteRegion(nextMessageAvailableRegion);
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

void scenarioTimestampedDefaultRegionInteractionAlternateAdvancesContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioTimestampedDefaultRegionInteractionAlternateAdvances(options, model);
}

void scenarioTimestampedDefaultRegionInteractionMixedFanout(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.ddmFom.empty(),
      "Timestamped default-region interaction mixed fanout requires an adapter-supplied dimensional FOM");
  require(
      !options.logicalTimeImplementationName.empty(),
      "Timestamped default-region interaction mixed fanout requires an adapter-supplied logical-time implementation");

  Session publisher(options, model, "owner");
  Session constrained(options, model, "constrained");
  Session immediate(options, model, "immediate");
  auto const federation = federationName(
      options,
      "timestamped-default-region-interaction-mixed-fanout");
  connectAndJoin(publisher, constrained, options, federation, options.ddmFom);
  immediate.connect();
  immediate.join(
      options.memberFederateName + L"-default-region-interaction-mixed-immediate",
      options.federateType,
      federation);

  auto const publisherHandles = ddmHandles(publisher, options);
  auto const constrainedHandles = ddmHandles(constrained, options);
  auto const immediateHandles = ddmHandles(immediate, options);
  verifyDdmClassDimensions(
      publisher,
      options,
      publisherHandles,
      "timestamped default-region interaction mixed fanout");
  publisher.rtiAmbassador().publishInteractionClass(
      publisherHandles.interactionClass);
  publisher.rtiAmbassador().changeInteractionOrderType(
      publisherHandles.interactionClass,
      rti::TIMESTAMP);

  auto const constrainedRegion = createDdmRegion(
      constrained,
      constrainedHandles,
      2UL,
      6UL,
      2UL,
      6UL);
  auto const immediateRegion = createDdmRegion(
      immediate,
      immediateHandles,
      2UL,
      6UL,
      2UL,
      6UL);
  auto const constrainedRegionSet = rti::RegionHandleSet{constrainedRegion};
  auto const immediateRegionSet = rti::RegionHandleSet{immediateRegion};
  constrained.rtiAmbassador().setConveyRegionDesignatorSetsSwitch(true);
  immediate.rtiAmbassador().setConveyRegionDesignatorSetsSwitch(true);
  constrained.rtiAmbassador().subscribeInteractionClassWithRegions(
      constrainedHandles.interactionClass,
      constrainedRegionSet,
      true);
  immediate.rtiAmbassador().subscribeInteractionClassWithRegions(
      immediateHandles.interactionClass,
      immediateRegionSet,
      true);

  auto publisherTime = makeTimeContext(publisher);
  auto constrainedTime = makeTimeContext(constrained);
  auto immediateTime = makeTimeContext(immediate);
  require(
      publisherTime.factory->getName() == constrainedTime.factory->getName() &&
          publisherTime.factory->getName() == immediateTime.factory->getName(),
      "timestamped default-region interaction mixed fanout members selected different logical-time factories");
  auto makeLookahead = [](TimeContext const& time, std::size_t steps) {
    auto target = timeAfter(*time.factory, *time.initial, *time.epsilon, steps);
    auto result = time.factory->makeZero();
    require(
        result != nullptr,
        "timestamped default-region interaction mixed fanout could not allocate a lookahead interval");
    result->setToDifference(*target, *time.initial);
    return result;
  };
  auto publisherLookahead = makeLookahead(publisherTime, 5U);
  constrained.rtiAmbassador().enableTimeConstrained();
  waitFor(
      constrained,
      [&] { return constrained.recorder().timeConstrainedEnabled().size() >= 1U; },
      options,
      "timestamped default-region interaction mixed fanout time-constrained callback");
  publisher.rtiAmbassador().enableTimeRegulation(*publisherLookahead);
  waitFor(
      publisher,
      [&] { return publisher.recorder().timeRegulationEnabled().size() >= 1U; },
      options,
      "timestamped default-region interaction mixed fanout time-regulation callback");

  auto const messageTime = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      6U);
  auto const publisherTarget = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      2U);
  std::vector<std::uint8_t> parameterBytes{0x44U, 0x45U, 0x46U, 0x49U};
  std::vector<std::uint8_t> tagBytes{0x44U, 0x45U, 0x46U, 0x2DU, 0x54U, 0x53U, 0x4FU};
  rti::ParameterHandleValueMap parameters;
  parameters.emplace(
      publisherHandles.parameter,
      rti::VariableLengthData(parameterBytes.data(), parameterBytes.size()));
  rti::VariableLengthData tag(tagBytes.data(), tagBytes.size());
  constrained.recorder().clearCallbackOrder();
  immediate.recorder().clearCallbackOrder();
  auto const retraction = publisher.rtiAmbassador().sendInteraction(
      publisherHandles.interactionClass,
      parameters,
      tag,
      *messageTime);
  require(
      retraction.isValid(),
      "timestamped default-region interaction mixed fanout send returned an invalid retraction handle");
  require(
      constrained.recorder().timedInteractions().empty(),
      "timestamped default-region interaction mixed fanout delivered the constrained copy before its frontier");

  waitFor(
      immediate,
      [&] { return immediate.recorder().timedInteractions().size() >= 1U; },
      options,
      "timestamped default-region interaction mixed fanout immediate delivery");
  auto const immediateInteractions = immediate.recorder().timedInteractions();
  require(
      immediateInteractions.size() == 1U,
      "timestamped default-region interaction mixed fanout delivered duplicate immediate callbacks");
  auto const& immediateInteraction = immediateInteractions.front();
  require(
      immediateInteraction.interaction == immediateHandles.interactionClass &&
          immediateInteraction.parameters.size() == 1U &&
          immediateInteraction.parameters.count(immediateHandles.parameter) == 1U &&
          copyBytes(immediateInteraction.parameters.at(immediateHandles.parameter)) ==
              parameterBytes,
      "timestamped default-region interaction mixed fanout returned the wrong immediate class or parameter");
  require(
      immediateInteraction.tag == tagBytes &&
          immediateInteraction.time == encodeTime(*messageTime) &&
          !immediateInteraction.timeText.empty(),
      "timestamped default-region interaction mixed fanout did not preserve immediate payload or time");
  require(
      immediateInteraction.producer == publisher.federateHandle() &&
          immediateInteraction.sentOrder == rti::TIMESTAMP &&
          immediateInteraction.receivedOrder == rti::RECEIVE,
      "timestamped default-region interaction mixed fanout returned the wrong immediate producer or order");
  require(
      immediateInteraction.transportation.isValid() &&
          !immediate.rtiAmbassador().getTransportationTypeName(
              immediateInteraction.transportation).empty(),
      "timestamped default-region interaction mixed fanout returned an unresolvable immediate transport");
  require(
      immediateInteraction.regions.has_value() && immediateInteraction.regions->empty(),
      "timestamped default-region interaction mixed fanout returned a non-empty immediate source region");
  require(
      immediateInteraction.retractionPresent &&
          immediateInteraction.retraction == copyBytes(retraction.encode()),
      "timestamped default-region interaction mixed fanout omitted the immediate retraction handle");
  require(
      immediate.recorder().callbackOrder() == std::vector<std::string>{"interaction"},
      "timestamped default-region interaction mixed fanout returned the wrong immediate callback order");

  constrained.rtiAmbassador().timeAdvanceRequest(*messageTime);
  require(
      constrained.recorder().timeAdvanceGrants().empty(),
      "timestamped default-region interaction mixed fanout granted before the publisher frontier");
  publisher.rtiAmbassador().retract(retraction);
  require(
      constrained.recorder().timedInteractions().empty() &&
          constrained.recorder().callbackOrder().empty(),
      "timestamped default-region interaction mixed fanout leaked the retracted constrained callback");
  waitFor(
      immediate,
      [&] { return immediate.recorder().retractions().size() >= 1U; },
      options,
      "timestamped default-region interaction mixed fanout immediate retraction callback");
  auto const retractions = immediate.recorder().retractions();
  require(
      retractions.size() == 1U && retractions.front().valid &&
          retractions.front().encoded == copyBytes(retraction.encode()),
      "timestamped default-region interaction mixed fanout returned the wrong retraction callback");
  require(
      immediate.recorder().callbackOrder() ==
          std::vector<std::string>{"interaction", "request-retraction"},
      "timestamped default-region interaction mixed fanout returned the wrong immediate retraction order");

  publisher.rtiAmbassador().timeAdvanceRequest(*publisherTarget);
  waitForSessions(
      {&publisher, &constrained},
      [&] {
        return publisher.recorder().timeAdvanceGrants().size() >= 1U &&
            constrained.recorder().timeAdvanceGrants().size() >= 1U;
      },
      options,
      "timestamped default-region interaction mixed fanout grants");
  require(
      publisher.recorder().timeAdvanceGrants().size() == 1U &&
          constrained.recorder().timeAdvanceGrants().size() == 1U &&
          constrained.recorder().timedInteractions().empty() &&
          constrained.recorder().retractions().empty(),
      "timestamped default-region interaction mixed fanout returned the wrong constrained callback counts");
  require(
      publisher.recorder().timeAdvanceGrants().front().encoded ==
              encodeTime(*publisherTarget) &&
          constrained.recorder().timeAdvanceGrants().front().encoded ==
              encodeTime(*messageTime),
      "timestamped default-region interaction mixed fanout returned the wrong grants");
  require(
      constrained.recorder().callbackOrder() == std::vector<std::string>{"grant"},
      "timestamped default-region interaction mixed fanout granted after a suppressed callback");
  requireException(
      [&] { publisher.rtiAmbassador().retract(retraction); },
      L"MessageCanNoLongerBeRetracted",
      "timestamped default-region interaction mixed fanout terminal retraction");

  constrained.rtiAmbassador().disableTimeConstrained();
  publisher.rtiAmbassador().disableTimeRegulation();
  constrained.rtiAmbassador().unsubscribeInteractionClassWithRegions(
      constrainedHandles.interactionClass,
      constrainedRegionSet);
  immediate.rtiAmbassador().unsubscribeInteractionClassWithRegions(
      immediateHandles.interactionClass,
      immediateRegionSet);
  publisher.rtiAmbassador().unpublishInteractionClass(
      publisherHandles.interactionClass);
  constrained.rtiAmbassador().deleteRegion(constrainedRegion);
  immediate.rtiAmbassador().deleteRegion(immediateRegion);
  constrained.resign(rti::NO_ACTION);
  immediate.resign(rti::NO_ACTION);
  publisher.resign(rti::CANCEL_THEN_DELETE_THEN_DIVEST);
  publisher.rtiAmbassador().destroyFederationExecution(federation);
  immediate.disconnect();
  constrained.disconnect();
  publisher.disconnect();
}

void scenarioTimestampedDefaultRegionInteractionMixedFanoutContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioTimestampedDefaultRegionInteractionMixedFanout(options, model);
}

void scenarioTimestampedDefaultRegionInteractionSourceResignation(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.ddmFom.empty(),
      "Timestamped default-region interaction source resignation requires an adapter-supplied dimensional FOM");
  require(
      !options.logicalTimeImplementationName.empty(),
      "Timestamped default-region interaction source resignation requires an adapter-supplied logical-time implementation");

  Session publisher(options, model, "owner");
  Session first(options, model, "first");
  Session second(options, model, "second");
  Session clock(options, model, "source-resignation-clock");
  auto const federation = federationName(
      options,
      "timestamped-default-region-interaction-source-resignation");
  connectAndJoin(publisher, first, options, federation, options.ddmFom);
  second.connect();
  second.join(
      options.memberFederateName + L"-default-region-interaction-resignation-second",
      options.federateType,
      federation);
  clock.connect();
  clock.join(
      options.memberFederateName + L"-default-region-interaction-resignation-clock",
      options.federateType,
      federation);

  auto const publisherHandles = ddmHandles(publisher, options);
  auto const firstHandles = ddmHandles(first, options);
  auto const secondHandles = ddmHandles(second, options);
  verifyDdmClassDimensions(
      publisher,
      options,
      publisherHandles,
      "timestamped default-region interaction source resignation");
  publisher.rtiAmbassador().publishInteractionClass(
      publisherHandles.interactionClass);
  publisher.rtiAmbassador().changeInteractionOrderType(
      publisherHandles.interactionClass,
      rti::TIMESTAMP);

  auto const firstRegion = createDdmRegion(
      first,
      firstHandles,
      2UL,
      6UL,
      2UL,
      6UL);
  auto const secondRegion = createDdmRegion(
      second,
      secondHandles,
      2UL,
      6UL,
      2UL,
      6UL);
  auto const firstRegionSet = rti::RegionHandleSet{firstRegion};
  auto const secondRegionSet = rti::RegionHandleSet{secondRegion};
  first.rtiAmbassador().setConveyRegionDesignatorSetsSwitch(true);
  second.rtiAmbassador().setConveyRegionDesignatorSetsSwitch(true);
  first.rtiAmbassador().subscribeInteractionClassWithRegions(
      firstHandles.interactionClass,
      firstRegionSet,
      true);
  second.rtiAmbassador().subscribeInteractionClassWithRegions(
      secondHandles.interactionClass,
      secondRegionSet,
      true);

  auto publisherTime = makeTimeContext(publisher);
  auto firstTime = makeTimeContext(first);
  auto secondTime = makeTimeContext(second);
  auto clockTime = makeTimeContext(clock);
  require(
      publisherTime.factory->getName() == firstTime.factory->getName() &&
          publisherTime.factory->getName() == secondTime.factory->getName() &&
          publisherTime.factory->getName() == clockTime.factory->getName(),
      "timestamped default-region interaction source resignation members selected different logical-time factories");

  auto makeLookahead = [](TimeContext const& time, std::size_t steps) {
    auto target = timeAfter(*time.factory, *time.initial, *time.epsilon, steps);
    auto result = time.factory->makeZero();
    require(
        result != nullptr,
        "timestamped default-region interaction source resignation could not allocate a lookahead interval");
    result->setToDifference(*target, *time.initial);
    return result;
  };
  auto publisherLookahead = makeLookahead(publisherTime, 5U);
  auto clockLookahead = makeLookahead(clockTime, 5U);

  first.rtiAmbassador().enableTimeConstrained();
  waitFor(
      first,
      [&] { return first.recorder().timeConstrainedEnabled().size() >= 1U; },
      options,
      "timestamped default-region interaction source resignation first time-constrained callback");
  second.rtiAmbassador().enableTimeConstrained();
  waitFor(
      second,
      [&] { return second.recorder().timeConstrainedEnabled().size() >= 1U; },
      options,
      "timestamped default-region interaction source resignation second time-constrained callback");
  publisher.rtiAmbassador().enableTimeRegulation(*publisherLookahead);
  waitFor(
      publisher,
      [&] { return publisher.recorder().timeRegulationEnabled().size() >= 1U; },
      options,
      "timestamped default-region interaction source resignation publisher time-regulation callback");
  clock.rtiAmbassador().enableTimeRegulation(*clockLookahead);
  waitFor(
      clock,
      [&] { return clock.recorder().timeRegulationEnabled().size() >= 1U; },
      options,
      "timestamped default-region interaction source resignation clock time-regulation callback");

  auto const messageTime = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      7U);
  auto const firstTarget = timeAfter(
      *firstTime.factory,
      *firstTime.initial,
      *firstTime.epsilon,
      7U);
  auto const secondTarget = timeAfter(
      *secondTime.factory,
      *secondTime.initial,
      *secondTime.epsilon,
      10U);
  auto const clockTarget = timeAfter(
      *clockTime.factory,
      *clockTime.initial,
      *clockTime.epsilon,
      2U);
  std::vector<std::uint8_t> parameterBytes{0x44U, 0x45U, 0x46U, 0x2DU, 0x46U};
  std::vector<std::uint8_t> tagBytes{0x44U, 0x45U, 0x46U, 0x2DU, 0x52U, 0x53U};
  rti::ParameterHandleValueMap parameters;
  parameters.emplace(
      publisherHandles.parameter,
      rti::VariableLengthData(parameterBytes.data(), parameterBytes.size()));
  rti::VariableLengthData tag(tagBytes.data(), tagBytes.size());
  auto const retraction = publisher.rtiAmbassador().sendInteraction(
      publisherHandles.interactionClass,
      parameters,
      tag,
      *messageTime);
  require(
      retraction.isValid(),
      "timestamped default-region interaction source resignation send returned an invalid retraction handle");
  require(
      first.recorder().timedInteractions().empty() &&
          second.recorder().timedInteractions().empty(),
      "timestamped default-region interaction source resignation delivered before either recipient frontier");

  first.rtiAmbassador().timeAdvanceRequest(*firstTarget);
  second.rtiAmbassador().nextMessageRequest(*secondTarget);
  require(
      first.recorder().timeAdvanceGrants().empty() &&
          second.recorder().timeAdvanceGrants().empty(),
      "timestamped default-region interaction source resignation granted before the producer frontier");
  publisher.resign(rti::NO_ACTION);
  require(
      first.recorder().timedInteractions().empty() &&
          second.recorder().timedInteractions().empty() &&
          first.recorder().timeAdvanceGrants().empty() &&
          second.recorder().timeAdvanceGrants().empty(),
      "timestamped default-region interaction source resignation released queued passels too early");

  clock.rtiAmbassador().timeAdvanceRequest(*clockTarget);
  waitFor(
      clock,
      [&] { return clock.recorder().timeAdvanceGrants().size() >= 1U; },
      options,
      "timestamped default-region interaction source resignation clock frontier");
  waitFor(
      first,
      [&] {
        return first.recorder().timedInteractions().size() >= 1U &&
            first.recorder().timeAdvanceGrants().size() >= 1U;
      },
      options,
      "timestamped default-region interaction source resignation first delivery and grant");
  waitFor(
      second,
      [&] {
        return second.recorder().timedInteractions().size() >= 1U &&
            second.recorder().timeAdvanceGrants().size() >= 1U;
      },
      options,
      "timestamped default-region interaction source resignation second delivery and grant");

  auto assertDefaultRegionInteraction = [&](Session& receiver,
                                             DdmHandles const& receiverHandles,
                                             TimedInteractionRecord const& record) {
    require(
        record.interaction == receiverHandles.interactionClass &&
            record.parameters.size() == 1U &&
            record.parameters.count(receiverHandles.parameter) == 1U &&
            copyBytes(record.parameters.at(receiverHandles.parameter)) == parameterBytes,
        "timestamped default-region interaction source resignation returned the wrong class or parameter");
    require(
        record.tag == tagBytes && record.time == encodeTime(*messageTime) &&
            !record.timeText.empty(),
        "timestamped default-region interaction source resignation did not preserve payload or time");
    require(
        record.producer == publisher.federateHandle() &&
            record.sentOrder == rti::TIMESTAMP &&
            record.receivedOrder == rti::TIMESTAMP,
        "timestamped default-region interaction source resignation returned the wrong producer or order metadata");
    require(
        record.transportation.isValid() &&
            !receiver.rtiAmbassador().getTransportationTypeName(
                record.transportation).empty(),
        "timestamped default-region interaction source resignation returned an unresolvable transport");
    require(
        record.regions.has_value() && record.regions->empty(),
        "timestamped default-region interaction source resignation returned a non-empty source region");
    require(
        record.retractionPresent &&
            record.retraction == copyBytes(retraction.encode()),
        "timestamped default-region interaction source resignation omitted its retraction handle");
  };
  require(
      first.recorder().callbackOrder() ==
          std::vector<std::string>{"interaction", "grant"} &&
          second.recorder().callbackOrder() ==
              std::vector<std::string>{"interaction", "grant"},
      "timestamped default-region interaction source resignation delivered after a grant");
  require(
      first.recorder().timedInteractions().size() == 1U &&
          second.recorder().timedInteractions().size() == 1U &&
          first.recorder().timeAdvanceGrants().size() == 1U &&
          second.recorder().timeAdvanceGrants().size() == 1U,
      "timestamped default-region interaction source resignation returned the wrong callback counts");
  assertDefaultRegionInteraction(
      first,
      firstHandles,
      first.recorder().timedInteractions().front());
  assertDefaultRegionInteraction(
      second,
      secondHandles,
      second.recorder().timedInteractions().front());
  require(
      first.recorder().timeAdvanceGrants().front().encoded == encodeTime(*firstTarget) &&
          second.recorder().timeAdvanceGrants().front().encoded == encodeTime(*messageTime),
      "timestamped default-region interaction source resignation returned the wrong recipient grants");
  requireException(
      [&] { publisher.rtiAmbassador().retract(retraction); },
      L"FederateNotExecutionMember",
      "timestamped default-region interaction source resignation post-resignation retraction");

  first.rtiAmbassador().disableTimeConstrained();
  second.rtiAmbassador().disableTimeConstrained();
  clock.rtiAmbassador().disableTimeRegulation();
  first.rtiAmbassador().unsubscribeInteractionClassWithRegions(
      firstHandles.interactionClass,
      firstRegionSet);
  second.rtiAmbassador().unsubscribeInteractionClassWithRegions(
      secondHandles.interactionClass,
      secondRegionSet);
  first.rtiAmbassador().deleteRegion(firstRegion);
  second.rtiAmbassador().deleteRegion(secondRegion);
  second.resign(rti::NO_ACTION);
  first.resign(rti::NO_ACTION);
  clock.resign(rti::NO_ACTION);
  first.rtiAmbassador().destroyFederationExecution(federation);
  publisher.disconnect();
  first.disconnect();
  second.disconnect();
  clock.disconnect();
}

void scenarioTimestampedDefaultRegionInteractionSourceResignationContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioTimestampedDefaultRegionInteractionSourceResignation(options, model);
}

void scenarioTimestampedDefaultRegionInteractionReenable(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.ddmFom.empty(),
      "Timestamped default-region interaction re-enable requires an adapter-supplied dimensional FOM");
  require(
      !options.logicalTimeImplementationName.empty(),
      "Timestamped default-region interaction re-enable requires an adapter-supplied logical-time implementation");

  Session publisher(options, model, "owner");
  Session receiver(options, model, "constrained");
  auto const federation = federationName(
      options,
      "timestamped-default-region-interaction-reenable");
  connectAndJoin(publisher, receiver, options, federation, options.ddmFom);

  auto const publisherHandles = ddmHandles(publisher, options);
  auto const receiverHandles = ddmHandles(receiver, options);
  verifyDdmClassDimensions(
      publisher,
      options,
      publisherHandles,
      "timestamped default-region interaction re-enable");
  publisher.rtiAmbassador().publishInteractionClass(
      publisherHandles.interactionClass);
  publisher.rtiAmbassador().changeInteractionOrderType(
      publisherHandles.interactionClass,
      rti::TIMESTAMP);

  auto const receiverRegion = receiver.rtiAmbassador().createRegion(
      rti::DimensionHandleSet{receiverHandles.firstDimension});
  require(
      receiverRegion.isValid(),
      "timestamped default-region interaction re-enable returned an invalid receiver region");
  receiver.rtiAmbassador().setRangeBounds(
      receiverRegion,
      receiverHandles.firstDimension,
      rti::RangeBounds(2UL, 6UL));
  receiver.rtiAmbassador().commitRegionModifications(
      rti::RegionHandleSet{receiverRegion});
  auto const receiverRegionSet = rti::RegionHandleSet{receiverRegion};
  receiver.rtiAmbassador().setConveyRegionDesignatorSetsSwitch(true);
  receiver.rtiAmbassador().subscribeInteractionClassWithRegions(
      receiverHandles.interactionClass,
      receiverRegionSet);

  auto publisherTime = makeTimeContext(publisher);
  auto receiverTime = makeTimeContext(receiver);
  require(
      publisherTime.factory->getName() == receiverTime.factory->getName(),
      "timestamped default-region interaction re-enable members selected different logical-time factories");
  auto publisherLookaheadTime = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      5U);
  auto publisherLookahead = publisherTime.factory->makeZero();
  require(
      publisherLookahead != nullptr,
      "timestamped default-region interaction re-enable could not allocate a lookahead interval");
  publisherLookahead->setToDifference(
      *publisherLookaheadTime,
      *publisherTime.initial);
  receiver.rtiAmbassador().enableTimeConstrained();
  waitFor(
      receiver,
      [&] { return receiver.recorder().timeConstrainedEnabled().size() >= 1U; },
      options,
      "timestamped default-region interaction re-enable time-constrained callback");
  publisher.rtiAmbassador().enableTimeRegulation(*publisherLookahead);
  waitFor(
      publisher,
      [&] { return publisher.recorder().timeRegulationEnabled().size() >= 1U; },
      options,
      "timestamped default-region interaction re-enable time-regulation callback");

  auto const messageTime = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      6U);
  auto const publisherTarget = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      2U);
  std::vector<std::uint8_t> parameterBytes{0x52U, 0x45U, 0x49U, 0x4EU};
  std::vector<std::uint8_t> tagBytes{0x44U, 0x45U, 0x46U, 0x52U};
  rti::ParameterHandleValueMap parameters;
  parameters.emplace(
      publisherHandles.parameter,
      rti::VariableLengthData(parameterBytes.data(), parameterBytes.size()));
  rti::VariableLengthData tag(tagBytes.data(), tagBytes.size());
  auto const retraction = publisher.rtiAmbassador().sendInteraction(
      publisherHandles.interactionClass,
      parameters,
      tag,
      *messageTime);
  require(
      retraction.isValid(),
      "timestamped default-region interaction re-enable send returned an invalid retraction handle");
  require(
      receiver.recorder().timedInteractions().empty(),
      "timestamped default-region interaction re-enable delivered before its grant");

  receiver.rtiAmbassador().disableTimeConstrained();
  for (int pass = 0; pass != 8; ++pass) {
    receiver.pump();
  }
  require(
      receiver.recorder().timedInteractions().empty(),
      "timestamped default-region interaction re-enable delivered while time constrained was disabled");
  receiver.rtiAmbassador().enableTimeConstrained();
  waitFor(
      receiver,
      [&] { return receiver.recorder().timeConstrainedEnabled().size() >= 2U; },
      options,
      "timestamped default-region interaction re-enable callback");
  receiver.recorder().clearCallbackOrder();
  publisher.recorder().clearCallbackOrder();
  receiver.rtiAmbassador().timeAdvanceRequest(*messageTime);
  publisher.rtiAmbassador().timeAdvanceRequest(*publisherTarget);
  publisher.pump();
  receiver.pump();
  require(
      publisher.recorder().timeAdvanceGrants().size() >= 1U &&
          receiver.recorder().timeAdvanceGrants().size() >= 1U &&
          receiver.recorder().timedInteractions().size() >= 1U,
      "timestamped default-region interaction re-enable delivery and grant");

  require(
      receiver.recorder().timedInteractions().size() == 1U &&
          receiver.recorder().timeConstrainedEnabled().size() == 2U &&
          publisher.recorder().timeAdvanceGrants().size() == 1U &&
          receiver.recorder().timeAdvanceGrants().size() == 1U,
      "timestamped default-region interaction re-enable returned the wrong callback counts");
  require(
      receiver.recorder().callbackOrder() ==
          std::vector<std::string>{"interaction", "grant"},
      "timestamped default-region interaction re-enable delivered after its grant");
  auto const interactions = receiver.recorder().timedInteractions();
  auto const& interaction = interactions.front();
  require(
      interaction.interaction == receiverHandles.interactionClass &&
          interaction.parameters.size() == 1U &&
          interaction.parameters.count(receiverHandles.parameter) == 1U,
      "timestamped default-region interaction re-enable returned the wrong class or parameter");
  require(
      interaction.tag == tagBytes &&
          interaction.time == encodeTime(*messageTime) &&
          !interaction.timeText.empty(),
      "timestamped default-region interaction re-enable did not preserve payload or time");
  require(
      interaction.producer == publisher.federateHandle() &&
          interaction.sentOrder == rti::TIMESTAMP &&
          interaction.receivedOrder == rti::TIMESTAMP,
      "timestamped default-region interaction re-enable returned the wrong producer or order");
  require(
      interaction.transportation.isValid() &&
          !receiver.rtiAmbassador().getTransportationTypeName(
              interaction.transportation).empty(),
      "timestamped default-region interaction re-enable returned an unresolvable transport");
  require(
      interaction.regions.has_value() && interaction.regions->empty(),
      "timestamped default-region interaction re-enable returned a non-empty source region");
  require(
      interaction.retractionPresent &&
          interaction.retraction == copyBytes(retraction.encode()),
      "timestamped default-region interaction re-enable omitted its retraction handle");
  require(
      publisher.recorder().timeAdvanceGrants().front().encoded ==
              encodeTime(*publisherTarget) &&
          receiver.recorder().timeAdvanceGrants().front().encoded ==
              encodeTime(*messageTime),
      "timestamped default-region interaction re-enable returned the wrong grants");
  requireException(
      [&] { publisher.rtiAmbassador().retract(retraction); },
      L"MessageCanNoLongerBeRetracted",
      "timestamped default-region interaction re-enable post-delivery retraction");

  receiver.rtiAmbassador().disableTimeConstrained();
  publisher.rtiAmbassador().disableTimeRegulation();
  receiver.rtiAmbassador().unsubscribeInteractionClassWithRegions(
      receiverHandles.interactionClass,
      receiverRegionSet);
  publisher.rtiAmbassador().unpublishInteractionClass(
      publisherHandles.interactionClass);
  receiver.rtiAmbassador().deleteRegion(receiverRegion);
  receiver.resign(rti::NO_ACTION);
  publisher.resign(rti::CANCEL_THEN_DELETE_THEN_DIVEST);
  publisher.rtiAmbassador().destroyFederationExecution(federation);
  receiver.disconnect();
  publisher.disconnect();
}

void scenarioTimestampedDefaultRegionInteractionReenableContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioTimestampedDefaultRegionInteractionReenable(options, model);
}

void scenarioTimestampedDefaultRegionInteractionRegulationReenable(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.ddmFom.empty(),
      "Timestamped default-region interaction regulation re-enable requires an adapter-supplied dimensional FOM");
  require(
      !options.logicalTimeImplementationName.empty(),
      "Timestamped default-region interaction regulation re-enable requires an adapter-supplied logical-time implementation");

  Session publisher(options, model, "owner");
  Session receiver(options, model, "constrained");
  auto const federation = federationName(
      options,
      "timestamped-default-region-interaction-regulation-reenable");
  connectAndJoin(publisher, receiver, options, federation, options.ddmFom);

  auto const publisherHandles = ddmHandles(publisher, options);
  auto const receiverHandles = ddmHandles(receiver, options);
  verifyDdmClassDimensions(
      publisher,
      options,
      publisherHandles,
      "timestamped default-region interaction regulation re-enable");
  publisher.rtiAmbassador().publishInteractionClass(
      publisherHandles.interactionClass);
  publisher.rtiAmbassador().changeInteractionOrderType(
      publisherHandles.interactionClass,
      rti::TIMESTAMP);

  auto const receiverRegion = receiver.rtiAmbassador().createRegion(
      rti::DimensionHandleSet{receiverHandles.firstDimension});
  require(
      receiverRegion.isValid(),
      "timestamped default-region interaction regulation re-enable returned an invalid receiver region");
  receiver.rtiAmbassador().setRangeBounds(
      receiverRegion,
      receiverHandles.firstDimension,
      rti::RangeBounds(2UL, 6UL));
  receiver.rtiAmbassador().commitRegionModifications(
      rti::RegionHandleSet{receiverRegion});
  auto const receiverRegionSet = rti::RegionHandleSet{receiverRegion};
  receiver.rtiAmbassador().setConveyRegionDesignatorSetsSwitch(true);
  receiver.rtiAmbassador().subscribeInteractionClassWithRegions(
      receiverHandles.interactionClass,
      receiverRegionSet);

  auto publisherTime = makeTimeContext(publisher);
  auto receiverTime = makeTimeContext(receiver);
  require(
      publisherTime.factory->getName() == receiverTime.factory->getName(),
      "timestamped default-region interaction regulation re-enable members selected different logical-time factories");
  auto publisherLookaheadTime = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      5U);
  auto publisherLookahead = publisherTime.factory->makeZero();
  require(
      publisherLookahead != nullptr,
      "timestamped default-region interaction regulation re-enable could not allocate a lookahead interval");
  publisherLookahead->setToDifference(
      *publisherLookaheadTime,
      *publisherTime.initial);
  receiver.rtiAmbassador().enableTimeConstrained();
  waitFor(
      receiver,
      [&] { return receiver.recorder().timeConstrainedEnabled().size() >= 1U; },
      options,
      "timestamped default-region interaction regulation re-enable time-constrained callback");
  publisher.rtiAmbassador().enableTimeRegulation(*publisherLookahead);
  waitFor(
      publisher,
      [&] { return publisher.recorder().timeRegulationEnabled().size() >= 1U; },
      options,
      "timestamped default-region interaction regulation re-enable time-regulation callback");

  auto const messageTime = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      5U);
  auto const publisherTarget = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      2U);
  auto const changedLookaheadTime = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      3U);
  auto changedLookahead = publisherTime.factory->makeZero();
  require(
      changedLookahead != nullptr,
      "timestamped default-region interaction regulation re-enable could not allocate the changed lookahead");
  changedLookahead->setToDifference(
      *changedLookaheadTime,
      *publisherTime.initial);
  std::vector<std::uint8_t> parameterBytes{0x52U, 0x45U, 0x47U, 0x44U};
  std::vector<std::uint8_t> tagBytes{0x44U, 0x45U, 0x46U, 0x52U};
  rti::ParameterHandleValueMap parameters;
  parameters.emplace(
      publisherHandles.parameter,
      rti::VariableLengthData(parameterBytes.data(), parameterBytes.size()));
  rti::VariableLengthData tag(tagBytes.data(), tagBytes.size());
  auto const retraction = publisher.rtiAmbassador().sendInteraction(
      publisherHandles.interactionClass,
      parameters,
      tag,
      *messageTime);
  require(
      retraction.isValid(),
      "timestamped default-region interaction regulation re-enable send returned an invalid retraction handle");
  require(
      receiver.recorder().timedInteractions().empty(),
      "timestamped default-region interaction regulation re-enable delivered before its grant");

  publisher.rtiAmbassador().disableTimeRegulation();
  publisher.rtiAmbassador().enableTimeRegulation(*changedLookahead);
  waitFor(
      publisher,
      [&] { return publisher.recorder().timeRegulationEnabled().size() >= 2U; },
      options,
      "timestamped default-region interaction regulation re-enable callback");
  auto queriedLookahead = publisherTime.factory->makeZero();
  require(
      queriedLookahead != nullptr,
      "timestamped default-region interaction regulation re-enable could not allocate a lookahead query");
  publisher.rtiAmbassador().queryLookahead(*queriedLookahead);
  require(
      *queriedLookahead == *changedLookahead,
      "timestamped default-region interaction regulation re-enable did not expose the changed lookahead");

  receiver.recorder().clearCallbackOrder();
  publisher.recorder().clearCallbackOrder();
  receiver.rtiAmbassador().timeAdvanceRequest(*messageTime);
  publisher.rtiAmbassador().timeAdvanceRequest(*publisherTarget);
  publisher.pump();
  receiver.pump();
  require(
      publisher.recorder().timeAdvanceGrants().size() >= 1U &&
          receiver.recorder().timeAdvanceGrants().size() >= 1U &&
          receiver.recorder().timedInteractions().size() >= 1U,
      "timestamped default-region interaction regulation re-enable delivery and grant");

  require(
      receiver.recorder().timedInteractions().size() == 1U &&
          publisher.recorder().timeRegulationEnabled().size() == 2U &&
          publisher.recorder().timeAdvanceGrants().size() == 1U &&
          receiver.recorder().timeAdvanceGrants().size() == 1U,
      "timestamped default-region interaction regulation re-enable returned the wrong callback counts");
  require(
      receiver.recorder().callbackOrder() ==
          std::vector<std::string>{"interaction", "grant"},
      "timestamped default-region interaction regulation re-enable delivered after its grant");
  auto const interactions = receiver.recorder().timedInteractions();
  auto const& interaction = interactions.front();
  require(
      interaction.interaction == receiverHandles.interactionClass &&
          interaction.parameters.size() == 1U &&
          interaction.parameters.count(receiverHandles.parameter) == 1U &&
          copyBytes(interaction.parameters.at(receiverHandles.parameter)) == parameterBytes,
      "timestamped default-region interaction regulation re-enable returned the wrong class or parameter");
  require(
      interaction.tag == tagBytes &&
          interaction.time == encodeTime(*messageTime) &&
          !interaction.timeText.empty(),
      "timestamped default-region interaction regulation re-enable did not preserve payload or time");
  require(
      interaction.producer == publisher.federateHandle() &&
          interaction.sentOrder == rti::TIMESTAMP &&
          interaction.receivedOrder == rti::TIMESTAMP,
      "timestamped default-region interaction regulation re-enable returned the wrong producer or order");
  require(
      interaction.transportation.isValid() &&
          !receiver.rtiAmbassador().getTransportationTypeName(
              interaction.transportation).empty(),
      "timestamped default-region interaction regulation re-enable returned an unresolvable transport");
  require(
      interaction.regions.has_value() && interaction.regions->empty(),
      "timestamped default-region interaction regulation re-enable returned a non-empty source region");
  require(
      interaction.retractionPresent &&
          interaction.retraction == copyBytes(retraction.encode()),
      "timestamped default-region interaction regulation re-enable omitted its retraction handle");
  require(
      publisher.recorder().timeAdvanceGrants().front().encoded ==
              encodeTime(*publisherTarget) &&
          receiver.recorder().timeAdvanceGrants().front().encoded ==
              encodeTime(*messageTime),
      "timestamped default-region interaction regulation re-enable returned the wrong grants");
  requireException(
      [&] { publisher.rtiAmbassador().retract(retraction); },
      L"MessageCanNoLongerBeRetracted",
      "timestamped default-region interaction regulation re-enable post-delivery retraction");

  receiver.rtiAmbassador().disableTimeConstrained();
  publisher.rtiAmbassador().disableTimeRegulation();
  receiver.rtiAmbassador().unsubscribeInteractionClassWithRegions(
      receiverHandles.interactionClass,
      receiverRegionSet);
  publisher.rtiAmbassador().unpublishInteractionClass(
      publisherHandles.interactionClass);
  receiver.rtiAmbassador().deleteRegion(receiverRegion);
  receiver.resign(rti::NO_ACTION);
  publisher.resign(rti::CANCEL_THEN_DELETE_THEN_DIVEST);
  publisher.rtiAmbassador().destroyFederationExecution(federation);
  receiver.disconnect();
  publisher.disconnect();
}

void scenarioTimestampedRegionalAttributeAlternateAdvancesContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioTimestampedRegionalAttributeAlternateAdvances(options, model);
}

void scenarioTimestampedRegionalAttributeRegulationReenable(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.ddmFom.empty(),
      "Timestamped regional regulation re-enable requires an adapter-supplied dimensional FOM");
  require(
      !options.logicalTimeImplementationName.empty(),
      "Timestamped regional regulation re-enable requires an adapter-supplied logical-time implementation");

  Session publisher(options, model, "owner");
  Session receiver(options, model, "constrained");
  auto const federation = federationName(
      options,
      "timestamped-regional-attribute-regulation-reenable");
  connectAndJoin(publisher, receiver, options, federation, options.ddmFom);

  auto const publisherHandles = ddmHandles(publisher, options);
  auto const receiverHandles = ddmHandles(receiver, options);
  verifyDdmClassDimensions(
      publisher,
      options,
      publisherHandles,
      "timestamped regional regulation re-enable");
  rti::AttributeHandleSet const publisherAttributes{publisherHandles.attribute};
  rti::AttributeHandleSet const receiverAttributes{receiverHandles.attribute};
  publisher.rtiAmbassador().publishObjectClassAttributes(
      publisherHandles.objectClass,
      publisherAttributes);
  publisher.rtiAmbassador().changeDefaultAttributeOrderType(
      publisherHandles.objectClass,
      publisherAttributes,
      rti::TIMESTAMP);

  auto const publisherRegion = createDdmRegion(
      publisher,
      publisherHandles,
      1UL,
      3UL,
      1UL,
      3UL);
  auto const receiverRegion = createDdmRegion(
      receiver,
      receiverHandles,
      2UL,
      4UL,
      2UL,
      4UL);
  auto const publisherRegionSet = rti::RegionHandleSet{publisherRegion};
  auto const receiverRegionSet = rti::RegionHandleSet{receiverRegion};
  rti::AttributeHandleSetRegionHandleSetPairVector const publisherPair{{
      publisherAttributes,
      publisherRegionSet,
  }};
  rti::AttributeHandleSetRegionHandleSetPairVector const receiverPair{{
      receiverAttributes,
      receiverRegionSet,
  }};
  receiver.rtiAmbassador().setConveyRegionDesignatorSetsSwitch(true);
  receiver.rtiAmbassador().subscribeObjectClassAttributesWithRegions(
      receiverHandles.objectClass,
      receiverPair,
      true,
      L"");

  auto const object = publisher.rtiAmbassador().registerObjectInstanceWithRegions(
      publisherHandles.objectClass,
      publisherPair);
  require(
      object.isValid(),
      "timestamped regional regulation re-enable registration returned an invalid object handle");
  waitFor(
      receiver,
      [&] { return receiver.recorder().hasDiscovery(object); },
      options,
      "timestamped regional regulation re-enable discovery");

  auto publisherTime = makeTimeContext(publisher);
  auto receiverTime = makeTimeContext(receiver);
  require(
      publisherTime.factory->getName() == receiverTime.factory->getName(),
      "timestamped regional regulation re-enable members selected different logical-time factories");
  enableTimestampedRoles(
      publisher,
      receiver,
      publisherTime,
      receiverTime,
      options,
      "timestamped regional regulation re-enable");

  auto const updateTime = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      5U);
  auto const publisherTarget = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      2U);
  auto const changedLookaheadTime = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      3U);
  auto changedLookahead = publisherTime.factory->makeZero();
  require(
      changedLookahead != nullptr,
      "timestamped regional regulation re-enable could not allocate the changed lookahead");
  changedLookahead->setToDifference(
      *changedLookaheadTime,
      *publisherTime.initial);

  std::vector<std::uint8_t> valueBytes{0x52U, 0x47U, 0x41U, 0x43U};
  std::vector<std::uint8_t> tagBytes{0x43U, 0x48U, 0x47U, 0x52U};
  rti::AttributeHandleValueMap values;
  values.emplace(
      publisherHandles.attribute,
      rti::VariableLengthData(valueBytes.data(), valueBytes.size()));
  rti::VariableLengthData tag(tagBytes.data(), tagBytes.size());
  auto const retraction = publisher.rtiAmbassador().updateAttributeValues(
      object,
      values,
      tag,
      *updateTime);
  require(
      retraction.isValid(),
      "timestamped regional regulation re-enable update returned an invalid retraction handle");
  require(
      receiver.recorder().timedReflections().empty(),
      "timestamped regional regulation re-enable delivered before its grant");

  publisher.rtiAmbassador().setRangeBounds(
      publisherRegion,
      publisherHandles.firstDimension,
      rti::RangeBounds(0UL, 1UL));
  publisher.rtiAmbassador().setRangeBounds(
      publisherRegion,
      publisherHandles.secondDimension,
      rti::RangeBounds(0UL, 1UL));
  publisher.rtiAmbassador().commitRegionModifications(
      rti::RegionHandleSet{publisherRegion});
  publisher.rtiAmbassador().disableTimeRegulation();
  publisher.rtiAmbassador().enableTimeRegulation(*changedLookahead);
  waitFor(
      publisher,
      [&] { return publisher.recorder().timeRegulationEnabled().size() >= 2U; },
      options,
      "timestamped regional regulation re-enable callback");
  auto queriedLookahead = publisherTime.factory->makeZero();
  require(
      queriedLookahead != nullptr,
      "timestamped regional regulation re-enable could not allocate a lookahead query");
  publisher.rtiAmbassador().queryLookahead(*queriedLookahead);
  require(
      *queriedLookahead == *changedLookahead,
      "timestamped regional regulation re-enable did not expose the changed lookahead");

  receiver.recorder().clearCallbackOrder();
  publisher.recorder().clearCallbackOrder();
  receiver.rtiAmbassador().timeAdvanceRequest(*updateTime);
  publisher.rtiAmbassador().timeAdvanceRequest(*publisherTarget);
  waitFor(
      publisher,
      receiver,
      [&] {
        return publisher.recorder().timeAdvanceGrants().size() >= 1U &&
            receiver.recorder().timeAdvanceGrants().size() >= 1U &&
            receiver.recorder().timedReflections().size() >= 1U;
      },
      options,
      "timestamped regional regulation re-enable delivery");
  auto const reflections = receiver.recorder().timedReflections();
  require(
      reflections.size() == 1U &&
          publisher.recorder().timeRegulationEnabled().size() == 2U,
      "timestamped regional regulation re-enable returned the wrong callback counts");
  require(
      receiver.recorder().callbackOrder() == std::vector<std::string>{"reflect", "grant"},
      "timestamped regional regulation re-enable delivered after its grant");
  auto const& reflection = reflections.front();
  require(
      reflection.object == object && reflection.values.size() == 1U &&
          reflection.values.count(receiverHandles.attribute) == 1U &&
          copyBytes(reflection.values.at(receiverHandles.attribute)) == valueBytes,
      "timestamped regional regulation re-enable returned the wrong object or value");
  require(
      reflection.tag == tagBytes && reflection.time == encodeTime(*updateTime) &&
          !reflection.timeText.empty(),
      "timestamped regional regulation re-enable did not preserve value metadata");
  require(
      reflection.producer == publisher.federateHandle() &&
          reflection.sentOrder == rti::TIMESTAMP &&
          reflection.receivedOrder == rti::TIMESTAMP,
      "timestamped regional regulation re-enable returned the wrong producer or order");
  require(
      reflection.transportation.isValid() &&
          !receiver.rtiAmbassador().getTransportationTypeName(reflection.transportation).empty(),
      "timestamped regional regulation re-enable returned an unresolvable transport");
  require(
      reflection.regions.has_value() && reflection.regions->size() == 1U &&
          reflection.regions->count(publisherRegion) == 1U,
      "timestamped regional regulation re-enable did not preserve the source region snapshot");
  require(
      reflection.retractionPresent && reflection.retraction == copyBytes(retraction.encode()),
      "timestamped regional regulation re-enable omitted the retraction handle");
  require(
      publisher.recorder().timeAdvanceGrants().front().encoded == encodeTime(*publisherTarget),
      "timestamped regional regulation re-enable returned the wrong producer grant");
  require(
      receiver.recorder().timeAdvanceGrants().front().encoded == encodeTime(*updateTime),
      "timestamped regional regulation re-enable returned the wrong receiver grant");
  requireException(
      [&] { publisher.rtiAmbassador().retract(retraction); },
      L"MessageCanNoLongerBeRetracted",
      "timestamped regional regulation re-enable retraction after delivery");

  receiver.rtiAmbassador().disableTimeConstrained();
  publisher.rtiAmbassador().disableTimeRegulation();
  publisher.rtiAmbassador().unassociateRegionsForUpdates(object, publisherPair);
  receiver.rtiAmbassador().unsubscribeObjectClassAttributesWithRegions(
      receiverHandles.objectClass,
      receiverPair);
  receiver.rtiAmbassador().deleteRegion(receiverRegion);
  publisher.rtiAmbassador().deleteRegion(publisherRegion);
  receiver.resign(rti::NO_ACTION);
  publisher.resign(rti::CANCEL_THEN_DELETE_THEN_DIVEST);
  publisher.rtiAmbassador().destroyFederationExecution(federation);
  receiver.disconnect();
  publisher.disconnect();
}

void scenarioTimestampedRegionalAttributeRegulationReenableContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioTimestampedRegionalAttributeRegulationReenable(options, model);
}

void scenarioTimestampedRegionalAttributeAssociationReplacement(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.ddmFom.empty(),
      "Timestamped regional association replacement requires an adapter-supplied dimensional FOM");
  require(
      !options.logicalTimeImplementationName.empty(),
      "Timestamped regional association replacement requires an adapter-supplied logical-time implementation");

  Session publisher(options, model, "owner");
  Session receiver(options, model, "constrained");
  auto const federation =
      federationName(options, "timestamped-regional-attribute-association-replacement");
  connectAndJoin(publisher, receiver, options, federation, options.ddmFom);

  auto const publisherHandles = ddmHandles(publisher, options);
  auto const receiverHandles = ddmHandles(receiver, options);
  verifyDdmClassDimensions(
      publisher,
      options,
      publisherHandles,
      "timestamped regional association replacement");
  rti::AttributeHandleSet const publisherAttributes{publisherHandles.attribute};
  rti::AttributeHandleSet const receiverAttributes{receiverHandles.attribute};
  publisher.rtiAmbassador().publishObjectClassAttributes(
      publisherHandles.objectClass,
      publisherAttributes);
  publisher.rtiAmbassador().changeDefaultAttributeOrderType(
      publisherHandles.objectClass,
      publisherAttributes,
      rti::TIMESTAMP);

  auto const sourceRegionA = createDdmRegion(
      publisher,
      publisherHandles,
      1UL,
      3UL,
      1UL,
      3UL);
  auto const sourceRegionB = createDdmRegion(
      publisher,
      publisherHandles,
      1UL,
      3UL,
      1UL,
      3UL);
  auto const receiverRegion = createDdmRegion(
      receiver,
      receiverHandles,
      1UL,
      3UL,
      1UL,
      3UL);
  auto const sourceRegionSetA = rti::RegionHandleSet{sourceRegionA};
  auto const sourceRegionSetB = rti::RegionHandleSet{sourceRegionB};
  auto const receiverRegionSet = rti::RegionHandleSet{receiverRegion};
  rti::AttributeHandleSetRegionHandleSetPairVector const sourcePairA{{
      publisherAttributes,
      sourceRegionSetA,
  }};
  rti::AttributeHandleSetRegionHandleSetPairVector const sourcePairB{{
      publisherAttributes,
      sourceRegionSetB,
  }};
  rti::AttributeHandleSetRegionHandleSetPairVector const receiverPair{{
      receiverAttributes,
      receiverRegionSet,
  }};
  auto const object = publisher.rtiAmbassador().registerObjectInstanceWithRegions(
      publisherHandles.objectClass,
      sourcePairA);
  require(
      object.isValid(),
      "timestamped regional association replacement returned an invalid object handle");
  receiver.rtiAmbassador().subscribeObjectClassAttributesWithRegions(
      receiverHandles.objectClass,
      receiverPair,
      true,
      L"");
  waitFor(
      receiver,
      [&] { return receiver.recorder().hasDiscovery(object); },
      options,
      "timestamped regional association replacement discovery");
  receiver.rtiAmbassador().setConveyRegionDesignatorSetsSwitch(true);

  auto publisherTime = makeTimeContext(publisher);
  auto receiverTime = makeTimeContext(receiver);
  require(
      publisherTime.factory->getName() == receiverTime.factory->getName(),
      "timestamped regional association replacement members selected different logical-time factories");
  enableTimestampedRoles(
      publisher,
      receiver,
      publisherTime,
      receiverTime,
      options,
      "timestamped regional association replacement");

  auto const firstTime = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      6U);
  std::vector<std::uint8_t> firstValue{0x53U, 0x52U, 0x31U};
  std::vector<std::uint8_t> firstTag{0x52U, 0x45U, 0x50U, 0x31U};
  rti::AttributeHandleValueMap firstValues;
  firstValues.emplace(
      publisherHandles.attribute,
      rti::VariableLengthData(firstValue.data(), firstValue.size()));
  rti::VariableLengthData firstUserTag(firstTag.data(), firstTag.size());
  auto const firstRetraction = publisher.rtiAmbassador().updateAttributeValues(
      object,
      firstValues,
      firstUserTag,
      *firstTime);
  require(
      firstRetraction.isValid(),
      "timestamped regional association replacement first update returned an invalid retraction");
  require(
      receiver.recorder().timedReflections().empty(),
      "timestamped regional association replacement delivered before its grant");

  publisher.rtiAmbassador().unassociateRegionsForUpdates(object, sourcePairA);
  publisher.rtiAmbassador().associateRegionsForUpdates(object, sourcePairB);
  receiver.rtiAmbassador().timeAdvanceRequest(*firstTime);
  auto const firstPublisherTarget = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      2U);
  publisher.rtiAmbassador().timeAdvanceRequest(*firstPublisherTarget);
  waitFor(
      publisher,
      receiver,
      [&] {
        return publisher.recorder().timeAdvanceGrants().size() >= 1U &&
            receiver.recorder().timeAdvanceGrants().size() >= 1U;
      },
      options,
      "timestamped regional association replacement first grants");
  for (int pass = 0; pass != 8; ++pass) {
    receiver.pump();
  }
  require(
      receiver.recorder().timedReflections().empty() &&
          receiver.recorder().retractions().empty(),
      "queued timestamped regional update was retargeted after source replacement");
  receiver.recorder().clearCallbackOrder();
  publisher.recorder().clearCallbackOrder();

  auto const secondTime = timeAfter(
      *publisherTime.factory,
      *firstTime,
      *publisherTime.epsilon,
      1U);
  std::vector<std::uint8_t> secondValue{0x53U, 0x52U, 0x32U};
  std::vector<std::uint8_t> secondTag{0x52U, 0x45U, 0x50U, 0x32U};
  rti::AttributeHandleValueMap secondValues;
  secondValues.emplace(
      publisherHandles.attribute,
      rti::VariableLengthData(secondValue.data(), secondValue.size()));
  rti::VariableLengthData secondUserTag(secondTag.data(), secondTag.size());
  auto const secondRetraction = publisher.rtiAmbassador().updateAttributeValues(
      object,
      secondValues,
      secondUserTag,
      *secondTime);
  require(
      secondRetraction.isValid(),
      "timestamped regional association replacement second update returned an invalid retraction");
  receiver.rtiAmbassador().timeAdvanceRequest(*secondTime);
  auto const secondPublisherTarget = timeAfter(
      *publisherTime.factory,
      *firstPublisherTarget,
      *publisherTime.epsilon,
      1U);
  publisher.rtiAmbassador().timeAdvanceRequest(*secondPublisherTarget);
  waitFor(
      publisher,
      receiver,
      [&] {
        return publisher.recorder().timeAdvanceGrants().size() >= 2U &&
            receiver.recorder().timeAdvanceGrants().size() >= 2U &&
            receiver.recorder().timedReflections().size() >= 1U;
      },
      options,
      "timestamped regional association replacement second delivery");
  auto const reflections = receiver.recorder().timedReflections();
  require(
      reflections.size() == 1U,
      "timestamped regional association replacement returned the wrong reflection count");
  auto const& reflection = reflections.front();
  require(
      reflection.object == object && reflection.values.size() == 1U &&
          reflection.values.count(receiverHandles.attribute) == 1U &&
          copyBytes(reflection.values.at(receiverHandles.attribute)) == secondValue,
      "timestamped regional association replacement returned the wrong object or value");
  require(
      reflection.tag == secondTag && reflection.time == encodeTime(*secondTime) &&
          !reflection.timeText.empty(),
      "timestamped regional association replacement did not preserve second metadata");
  require(
      reflection.producer == publisher.federateHandle() && reflection.sentOrder == rti::TIMESTAMP &&
          reflection.receivedOrder == rti::TIMESTAMP,
      "timestamped regional association replacement returned the wrong producer or order");
  require(
      reflection.transportation.isValid() &&
          !receiver.rtiAmbassador().getTransportationTypeName(reflection.transportation).empty(),
      "timestamped regional association replacement returned an unresolvable transport");
  require(
      reflection.regions.has_value() && reflection.regions->size() == 1U &&
          reflection.regions->count(sourceRegionB) == 1U,
      "timestamped regional association replacement did not carry the replacement source region");
  require(
      reflection.retractionPresent &&
          reflection.retraction == copyBytes(secondRetraction.encode()),
      "timestamped regional association replacement omitted the second retraction handle");
  require(
      receiver.recorder().callbackOrder() ==
          std::vector<std::string>{"reflect", "grant"},
      "timestamped regional association replacement delivered after its grant");

  receiver.rtiAmbassador().disableTimeConstrained();
  publisher.rtiAmbassador().disableTimeRegulation();
  receiver.rtiAmbassador().unsubscribeObjectClassAttributesWithRegions(
      receiverHandles.objectClass,
      receiverPair);
  publisher.rtiAmbassador().unassociateRegionsForUpdates(object, sourcePairB);
  publisher.rtiAmbassador().unpublishObjectClassAttributes(
      publisherHandles.objectClass,
      publisherAttributes);
  receiver.rtiAmbassador().deleteRegion(receiverRegion);
  publisher.rtiAmbassador().deleteRegion(sourceRegionB);
  publisher.rtiAmbassador().deleteRegion(sourceRegionA);
  receiver.resign(rti::NO_ACTION);
  publisher.resign(rti::CANCEL_THEN_DELETE_THEN_DIVEST);
  publisher.rtiAmbassador().destroyFederationExecution(federation);
  receiver.disconnect();
  publisher.disconnect();
}

void scenarioTimestampedRegionalAttributeAssociationReplacementContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioTimestampedRegionalAttributeAssociationReplacement(options, model);
}

void scenarioTimestampedRegionalAttributeSourceResignation(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.ddmFom.empty(),
      "Timestamped regional attribute source resignation requires an adapter-supplied dimensional FOM");
  require(
      !options.logicalTimeImplementationName.empty(),
      "Timestamped regional attribute source resignation requires an adapter-supplied logical-time implementation");

  Session publisher(options, model, "owner");
  Session receiver(options, model, "member");
  Session clock(options, model, "source-resignation-clock");
  auto const federation = federationName(
      options,
      "timestamped-regional-attribute-source-resignation");
  connectAndJoin(publisher, receiver, options, federation, options.ddmFom);
  clock.connect();
  clock.join(
      options.memberFederateName + L"-timestamped-regional-attribute-resignation-clock",
      options.federateType,
      federation);

  auto const publisherHandles = ddmHandles(publisher, options);
  auto const receiverHandles = ddmHandles(receiver, options);
  verifyDdmClassDimensions(
      publisher,
      options,
      publisherHandles,
      "timestamped regional attribute source resignation");
  rti::AttributeHandleSet const publisherAttributes{publisherHandles.attribute};
  rti::AttributeHandleSet const receiverAttributes{receiverHandles.attribute};
  publisher.rtiAmbassador().publishObjectClassAttributes(
      publisherHandles.objectClass,
      publisherAttributes);
  publisher.rtiAmbassador().changeDefaultAttributeOrderType(
      publisherHandles.objectClass,
      publisherAttributes,
      rti::TIMESTAMP);

  auto const sourceRegion = createDdmRegion(
      publisher,
      publisherHandles,
      1UL,
      5UL,
      1UL,
      5UL);
  auto const receiverRegion = createDdmRegion(
      receiver,
      receiverHandles,
      2UL,
      6UL,
      2UL,
      6UL);
  auto const sourceRegionSet = rti::RegionHandleSet{sourceRegion};
  auto const receiverRegionSet = rti::RegionHandleSet{receiverRegion};
  rti::AttributeHandleSetRegionHandleSetPairVector const publisherPair{{
      publisherAttributes,
      sourceRegionSet,
  }};
  rti::AttributeHandleSetRegionHandleSetPairVector const receiverPair{{
      receiverAttributes,
      receiverRegionSet,
  }};
  receiver.rtiAmbassador().setConveyRegionDesignatorSetsSwitch(true);
  receiver.rtiAmbassador().subscribeObjectClassAttributesWithRegions(
      receiverHandles.objectClass,
      receiverPair,
      true,
      L"");
  auto const object = publisher.rtiAmbassador().registerObjectInstanceWithRegions(
      publisherHandles.objectClass,
      publisherPair);
  require(
      object.isValid(),
      "timestamped regional attribute source resignation returned an invalid object handle");
  waitFor(
      receiver,
      [&] { return receiver.recorder().hasDiscovery(object); },
      options,
      "timestamped regional attribute source resignation object discovery");

  auto publisherTime = makeTimeContext(publisher);
  auto receiverTime = makeTimeContext(receiver);
  auto clockTime = makeTimeContext(clock);
  require(
      publisherTime.factory->getName() == receiverTime.factory->getName() &&
          publisherTime.factory->getName() == clockTime.factory->getName(),
      "timestamped regional attribute source resignation members selected different logical-time factories");
  auto makeLookahead = [](TimeContext const& time, std::size_t steps) {
    auto target = timeAfter(*time.factory, *time.initial, *time.epsilon, steps);
    auto result = time.factory->makeZero();
    require(
        result != nullptr,
        "timestamped regional attribute source resignation could not allocate a lookahead interval");
    result->setToDifference(*target, *time.initial);
    return result;
  };
  auto publisherLookahead = makeLookahead(publisherTime, 5U);
  auto clockLookahead = makeLookahead(clockTime, 5U);

  receiver.rtiAmbassador().enableTimeConstrained();
  waitFor(
      receiver,
      [&] { return receiver.recorder().timeConstrainedEnabled().size() >= 1U; },
      options,
      "timestamped regional attribute source resignation time-constrained callback");
  publisher.rtiAmbassador().enableTimeRegulation(*publisherLookahead);
  waitFor(
      publisher,
      [&] { return publisher.recorder().timeRegulationEnabled().size() >= 1U; },
      options,
      "timestamped regional attribute source resignation publisher time-regulation callback");
  clock.rtiAmbassador().enableTimeRegulation(*clockLookahead);
  waitFor(
      clock,
      [&] { return clock.recorder().timeRegulationEnabled().size() >= 1U; },
      options,
      "timestamped regional attribute source resignation clock time-regulation callback");

  auto const messageTime = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      7U);
  auto const receiverTarget = timeAfter(
      *receiverTime.factory,
      *receiverTime.initial,
      *receiverTime.epsilon,
      7U);
  auto const clockTarget = timeAfter(
      *clockTime.factory,
      *clockTime.initial,
      *clockTime.epsilon,
      2U);
  std::vector<std::uint8_t> valueBytes{0xA9U, 0x72U, 0x31U};
  std::vector<std::uint8_t> tagBytes{0x52U, 0x45U, 0x47U, 0x2DU, 0x52U};
  rti::AttributeHandleValueMap values;
  values.emplace(
      publisherHandles.attribute,
      rti::VariableLengthData(valueBytes.data(), valueBytes.size()));
  rti::VariableLengthData tag(tagBytes.data(), tagBytes.size());
  auto const retraction = publisher.rtiAmbassador().updateAttributeValues(
      object,
      values,
      tag,
      *messageTime);
  require(
      retraction.isValid(),
      "timestamped regional attribute source resignation update returned an invalid retraction handle");
  require(
      receiver.recorder().timedReflections().empty(),
      "timestamped regional attribute source resignation reflected before the recipient frontier");

  receiver.rtiAmbassador().timeAdvanceRequest(*receiverTarget);
  require(
      receiver.recorder().timeAdvanceGrants().empty(),
      "timestamped regional attribute source resignation granted before an independent frontier");
  publisher.resign(rti::UNCONDITIONALLY_DIVEST_ATTRIBUTES);
  require(
      receiver.recorder().timedReflections().empty() &&
          receiver.recorder().timeAdvanceGrants().empty(),
      "timestamped regional attribute source resignation released the queued update too early");

  clock.rtiAmbassador().timeAdvanceRequest(*clockTarget);
  waitFor(
      clock,
      [&] { return clock.recorder().timeAdvanceGrants().size() >= 1U; },
      options,
      "timestamped regional attribute source resignation clock frontier");
  waitFor(
      receiver,
      [&] {
        return receiver.recorder().timedReflections().size() >= 1U &&
            receiver.recorder().timeAdvanceGrants().size() >= 1U;
      },
      options,
      "timestamped regional attribute source resignation reflection and grant");

  auto const reflections = receiver.recorder().timedReflections();
  require(
      reflections.size() == 1U,
      "timestamped regional attribute source resignation delivered a duplicate update");
  auto const& reflection = reflections.front();
  require(
      reflection.object == object && reflection.values.size() == 1U &&
          reflection.values.count(receiverHandles.attribute) == 1U,
      "timestamped regional attribute source resignation returned the wrong object or attribute set");
  require(
      copyBytes(reflection.values.at(receiverHandles.attribute)) == valueBytes &&
          reflection.tag == tagBytes && reflection.time == encodeTime(*messageTime),
      "timestamped regional attribute source resignation did not preserve value or time");
  require(
      reflection.producer == publisher.federateHandle() &&
          reflection.sentOrder == rti::TIMESTAMP &&
          reflection.receivedOrder == rti::TIMESTAMP &&
          !reflection.timeText.empty(),
      "timestamped regional attribute source resignation returned the wrong producer or order metadata");
  require(
      reflection.transportation.isValid() &&
          !receiver.rtiAmbassador().getTransportationTypeName(
              reflection.transportation).empty(),
      "timestamped regional attribute source resignation returned an unresolvable transport");
  require(
      reflection.regions.has_value() && reflection.regions->size() == 1U &&
          reflection.regions->count(sourceRegion) == 1U,
      "timestamped regional attribute source resignation omitted the source region snapshot");
  require(
      reflection.retractionPresent &&
          reflection.retraction == copyBytes(retraction.encode()),
      "timestamped regional attribute source resignation omitted its retraction handle");
  auto const grants = receiver.recorder().timeAdvanceGrants();
  require(
      grants.size() == 1U && grants.front().encoded == encodeTime(*receiverTarget),
      "timestamped regional attribute source resignation returned the wrong recipient grant");
  require(
      receiver.recorder().callbackOrder() ==
          std::vector<std::string>{"reflect", "grant"},
      "timestamped regional attribute source resignation reflected after its grant");
  requireException(
      [&] { publisher.rtiAmbassador().retract(retraction); },
      L"FederateNotExecutionMember",
      "timestamped regional attribute source resignation post-resignation retraction");

  receiver.rtiAmbassador().disableTimeConstrained();
  clock.rtiAmbassador().disableTimeRegulation();
  receiver.rtiAmbassador().unsubscribeObjectClassAttributesWithRegions(
      receiverHandles.objectClass,
      receiverPair);
  receiver.rtiAmbassador().deleteRegion(receiverRegion);
  receiver.resign(rti::NO_ACTION);
  clock.resign(rti::NO_ACTION);
  receiver.rtiAmbassador().destroyFederationExecution(federation);
  publisher.disconnect();
  receiver.disconnect();
  clock.disconnect();
}

void scenarioTimestampedRegionalAttributeSourceResignationContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioTimestampedRegionalAttributeSourceResignation(options, model);
}

void scenarioTimestampedObjectDeletionSourceResignationFanout(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.logicalTimeImplementationName.empty(),
      "Timestamped object-deletion source resignation fanout requires an adapter-supplied logical-time implementation");

  Session publisher(options, model, "owner");
  Session first(options, model, "first");
  Session second(options, model, "second");
  Session clock(options, model, "source-resignation-clock");
  auto const federation = federationName(
      options,
      "timestamped-object-deletion-source-resignation-fanout");
  connectAndJoin(publisher, first, options, federation, options.fom);
  second.connect();
  second.join(
      options.memberFederateName + L"-timestamped-deletion-resignation-second",
      options.federateType,
      federation);
  clock.connect();
  clock.join(
      options.memberFederateName + L"-timestamped-deletion-resignation-clock",
      options.federateType,
      federation);

  auto const publisherClass = publisher.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const firstClass = first.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const secondClass = second.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const publisherAttribute = publisher.rtiAmbassador().getAttributeHandle(
      publisherClass,
      options.attributeName);
  auto const firstAttribute = first.rtiAmbassador().getAttributeHandle(
      firstClass,
      options.attributeName);
  auto const secondAttribute = second.rtiAmbassador().getAttributeHandle(
      secondClass,
      options.attributeName);
  require(
      publisherClass.isValid() && firstClass.isValid() && secondClass.isValid() &&
          publisherAttribute.isValid() && firstAttribute.isValid() &&
          secondAttribute.isValid(),
      "timestamped object-deletion source resignation fanout lookup returned an invalid handle");

  rti::AttributeHandleSet const publisherAttributes{publisherAttribute};
  rti::AttributeHandleSet const firstAttributes{firstAttribute};
  rti::AttributeHandleSet const secondAttributes{secondAttribute};
  publisher.rtiAmbassador().publishObjectClassAttributes(
      publisherClass,
      publisherAttributes);
  first.rtiAmbassador().subscribeObjectClassAttributes(
      firstClass,
      firstAttributes,
      true,
      L"");
  second.rtiAmbassador().subscribeObjectClassAttributes(
      secondClass,
      secondAttributes,
      true,
      L"");

  auto const object = publisher.rtiAmbassador().registerObjectInstance(publisherClass);
  require(
      object.isValid(),
      "timestamped object-deletion source resignation fanout registration returned an invalid handle");
  waitFor(
      first,
      [&] { return first.recorder().hasDiscovery(object); },
      options,
      "timestamped object-deletion source resignation fanout first discovery");
  waitFor(
      second,
      [&] { return second.recorder().hasDiscovery(object); },
      options,
      "timestamped object-deletion source resignation fanout second discovery");

  auto publisherTime = makeTimeContext(publisher);
  auto firstTime = makeTimeContext(first);
  auto secondTime = makeTimeContext(second);
  auto clockTime = makeTimeContext(clock);
  require(
      publisherTime.factory->getName() == firstTime.factory->getName() &&
          publisherTime.factory->getName() == secondTime.factory->getName() &&
          publisherTime.factory->getName() == clockTime.factory->getName(),
      "timestamped object-deletion source resignation fanout members selected different logical-time factories");

  auto makeLookahead = [](TimeContext const& time) {
    auto target = timeAfter(*time.factory, *time.initial, *time.epsilon, 5U);
    auto result = time.factory->makeZero();
    require(
        result != nullptr,
        "timestamped object-deletion source resignation fanout could not allocate a lookahead interval");
    result->setToDifference(*target, *time.initial);
    return result;
  };
  auto publisherLookahead = makeLookahead(publisherTime);
  auto clockLookahead = makeLookahead(clockTime);

  first.rtiAmbassador().enableTimeConstrained();
  waitFor(
      first,
      [&] { return first.recorder().timeConstrainedEnabled().size() >= 1U; },
      options,
      "timestamped object-deletion source resignation fanout first time-constrained callback");
  second.rtiAmbassador().enableTimeConstrained();
  waitFor(
      second,
      [&] { return second.recorder().timeConstrainedEnabled().size() >= 1U; },
      options,
      "timestamped object-deletion source resignation fanout second time-constrained callback");
  publisher.rtiAmbassador().enableTimeRegulation(*publisherLookahead);
  waitFor(
      publisher,
      [&] { return publisher.recorder().timeRegulationEnabled().size() >= 1U; },
      options,
      "timestamped object-deletion source resignation fanout publisher time-regulation callback");
  clock.rtiAmbassador().enableTimeRegulation(*clockLookahead);
  waitFor(
      clock,
      [&] { return clock.recorder().timeRegulationEnabled().size() >= 1U; },
      options,
      "timestamped object-deletion source resignation fanout clock time-regulation callback");

  auto const messageTime = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      7U);
  auto const firstTarget = timeAfter(
      *firstTime.factory,
      *firstTime.initial,
      *firstTime.epsilon,
      7U);
  auto const secondTarget = timeAfter(
      *secondTime.factory,
      *secondTime.initial,
      *secondTime.epsilon,
      10U);
  auto const clockTarget = timeAfter(
      *clockTime.factory,
      *clockTime.initial,
      *clockTime.epsilon,
      2U);
  std::vector<std::uint8_t> const tagBytes{
      0x52U, 0x45U, 0x53U, 0x49U, 0x47U, 0x2DU, 0x44U};
  rti::VariableLengthData tag(tagBytes.data(), tagBytes.size());
  auto const retraction = publisher.rtiAmbassador().deleteObjectInstance(
      object,
      tag,
      *messageTime);
  require(
      retraction.isValid(),
      "timestamped object-deletion source resignation fanout returned an invalid retraction handle");
  require(
      first.recorder().timedRemovals().empty() &&
          second.recorder().timedRemovals().empty(),
      "timestamped object-deletion source resignation fanout delivered before either recipient frontier");

  first.rtiAmbassador().timeAdvanceRequest(*firstTarget);
  second.rtiAmbassador().nextMessageRequest(*secondTarget);
  require(
      first.recorder().timeAdvanceGrants().empty() &&
          second.recorder().timeAdvanceGrants().empty(),
      "timestamped object-deletion source resignation fanout granted before the producer frontier");
  publisher.resign(rti::UNCONDITIONALLY_DIVEST_ATTRIBUTES);
  requireException(
      [&] { publisher.rtiAmbassador().retract(retraction); },
      L"FederateNotExecutionMember",
      "timestamped object-deletion source resignation fanout post-resignation retraction");
  require(
      first.recorder().timedRemovals().empty() &&
          second.recorder().timedRemovals().empty() &&
          first.recorder().timeAdvanceGrants().empty() &&
          second.recorder().timeAdvanceGrants().empty(),
      "timestamped object-deletion source resignation fanout released queued callbacks too early");

  first.recorder().clearCallbackOrder();
  second.recorder().clearCallbackOrder();
  clock.rtiAmbassador().timeAdvanceRequest(*clockTarget);
  waitFor(
      clock,
      [&] { return clock.recorder().timeAdvanceGrants().size() >= 1U; },
      options,
      "timestamped object-deletion source resignation fanout clock frontier");
  waitFor(
      first,
      [&] {
        return first.recorder().timedRemovals().size() >= 1U &&
            first.recorder().timeAdvanceGrants().size() >= 1U;
      },
      options,
      "timestamped object-deletion source resignation fanout first delivery and grant");
  waitFor(
      second,
      [&] {
        return second.recorder().timedRemovals().size() >= 1U &&
            second.recorder().timeAdvanceGrants().size() >= 1U;
      },
      options,
      "timestamped object-deletion source resignation fanout second delivery and grant");

  auto assertRemoval = [&](Session& receiver,
                           TimedRemovalRecord const& removal,
                           std::vector<std::uint8_t> const& expectedGrant) {
    require(
        removal.object == object && removal.tag == tagBytes &&
            removal.producer == publisher.federateHandle(),
        "timestamped object-deletion source resignation fanout returned the wrong object or producer");
    require(
        removal.time == encodeTime(*messageTime) && !removal.timeText.empty() &&
            removal.sentOrder == rti::TIMESTAMP &&
            removal.receivedOrder == rti::TIMESTAMP,
        "timestamped object-deletion source resignation fanout returned the wrong time or order metadata");
    require(
        removal.retractionPresent &&
            removal.retraction == copyBytes(retraction.encode()),
        "timestamped object-deletion source resignation fanout omitted its retraction handle");
    require(
        receiver.recorder().timeAdvanceGrants().size() == 1U &&
            receiver.recorder().timeAdvanceGrants().front().encoded == expectedGrant,
        "timestamped object-deletion source resignation fanout returned the wrong recipient grant");
    require(
        receiver.recorder().callbackOrder() ==
            std::vector<std::string>{"remove", "grant"},
        "timestamped object-deletion source resignation fanout removed after its grant");
  };

  auto const firstRemovals = first.recorder().timedRemovals();
  auto const secondRemovals = second.recorder().timedRemovals();
  require(
      firstRemovals.size() == 1U && secondRemovals.size() == 1U,
      "timestamped object-deletion source resignation fanout returned duplicate removals");
  assertRemoval(first, firstRemovals.front(), encodeTime(*firstTarget));
  assertRemoval(second, secondRemovals.front(), encodeTime(*messageTime));

  first.rtiAmbassador().disableTimeConstrained();
  second.rtiAmbassador().disableTimeConstrained();
  clock.rtiAmbassador().disableTimeRegulation();
  first.rtiAmbassador().unsubscribeObjectClassAttributes(firstClass, firstAttributes);
  second.rtiAmbassador().unsubscribeObjectClassAttributes(secondClass, secondAttributes);
  first.resign(rti::NO_ACTION);
  second.resign(rti::NO_ACTION);
  clock.resign(rti::NO_ACTION);
  first.rtiAmbassador().destroyFederationExecution(federation);
  publisher.disconnect();
  first.disconnect();
  second.disconnect();
  clock.disconnect();
}

void scenarioTimestampedObjectDeletionMixedAdvances(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.logicalTimeImplementationName.empty(),
      "Timestamped object-deletion mixed advances require an adapter-supplied logical-time implementation");

  Session publisher(options, model, "owner");
  Session flushQueue(options, model, "member");
  Session timeAdvanceAvailable(options, model, "member");
  Session nextMessageAvailable(options, model, "member");
  auto const federation = federationName(
      options,
      "timestamped-object-deletion-mixed-advances");

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
      options.memberFederateName + L"-deletion-flush-queue",
      options.federateType,
      federation);
  timeAdvanceAvailable.join(
      options.memberFederateName + L"-deletion-time-advance-available",
      options.federateType,
      federation);
  nextMessageAvailable.join(
      options.memberFederateName + L"-deletion-next-message-available",
      options.federateType,
      federation);

  auto const publisherClass = publisher.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const publisherAttribute = publisher.rtiAmbassador().getAttributeHandle(
      publisherClass,
      options.attributeName);
  require(
      publisherClass.isValid() && publisherAttribute.isValid(),
      "timestamped object-deletion mixed advances publisher lookup returned an invalid handle");
  rti::AttributeHandleSet const publisherAttributes{publisherAttribute};
  publisher.rtiAmbassador().publishObjectClassAttributes(
      publisherClass,
      publisherAttributes);

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
    auto const memberClass = member->rtiAmbassador().getObjectClassHandle(
        options.objectClassName);
    auto const memberAttribute = member->rtiAmbassador().getAttributeHandle(
        memberClass,
        options.attributeName);
    require(
        memberClass.isValid() && memberAttribute.isValid(),
        "timestamped object-deletion mixed advances member lookup returned an invalid handle");
    member->rtiAmbassador().subscribeObjectClassAttributes(
        memberClass,
        rti::AttributeHandleSet{memberAttribute},
        true,
        L"");
    memberClasses.push_back(memberClass);
    memberAttributes.push_back(memberAttribute);
  }

  auto const object = publisher.rtiAmbassador().registerObjectInstance(publisherClass);
  require(
      object.isValid(),
      "timestamped object-deletion mixed advances registration returned an invalid handle");
  for (auto* member : members) {
    waitFor(
        *member,
        [&] { return member->recorder().hasDiscovery(object); },
        options,
        "timestamped object-deletion mixed advances object discovery");
  }

  auto publisherTime = makeTimeContext(publisher);
  auto flushQueueTime = makeTimeContext(flushQueue);
  auto timeAdvanceAvailableTime = makeTimeContext(timeAdvanceAvailable);
  auto nextMessageAvailableTime = makeTimeContext(nextMessageAvailable);
  require(
      publisherTime.factory->getName() == flushQueueTime.factory->getName() &&
          publisherTime.factory->getName() == timeAdvanceAvailableTime.factory->getName() &&
          publisherTime.factory->getName() == nextMessageAvailableTime.factory->getName(),
      "timestamped object-deletion mixed advances members selected different logical-time factories");

  for (auto* member : members) {
    member->rtiAmbassador().enableTimeConstrained();
    waitFor(
        *member,
        [&] { return member->recorder().timeConstrainedEnabled().size() >= 1U; },
        options,
        "timestamped object-deletion mixed advances time-constrained callback");
  }

  auto publisherLookaheadTime = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      5U);
  auto publisherLookahead = publisherTime.factory->makeZero();
  require(
      publisherLookahead != nullptr,
      "timestamped object-deletion mixed advances could not allocate the publisher lookahead");
  publisherLookahead->setToDifference(
      *publisherLookaheadTime,
      *publisherTime.initial);
  publisher.rtiAmbassador().enableTimeRegulation(*publisherLookahead);
  waitFor(
      publisher,
      [&] { return publisher.recorder().timeRegulationEnabled().size() >= 1U; },
      options,
      "timestamped object-deletion mixed advances time-regulation callback");

  auto const deletionTime = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      7U);
  auto const flushQueueBoundary = timeAfter(
      *flushQueueTime.factory,
      *flushQueueTime.initial,
      *flushQueueTime.epsilon,
      10U);
  auto const timeAdvanceAvailableTarget = timeAfter(
      *timeAdvanceAvailableTime.factory,
      *timeAdvanceAvailableTime.initial,
      *timeAdvanceAvailableTime.epsilon,
      7U);
  auto const nextMessageAvailableBoundary = timeAfter(
      *nextMessageAvailableTime.factory,
      *nextMessageAvailableTime.initial,
      *nextMessageAvailableTime.epsilon,
      10U);
  auto const publisherTarget = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      2U);

  std::vector<std::uint8_t> const tagBytes{0xD8U, 0x61U, 0x4EU};
  rti::VariableLengthData tag(tagBytes.data(), tagBytes.size());
  auto const retraction = publisher.rtiAmbassador().deleteObjectInstance(
      object,
      tag,
      *deletionTime);
  require(
      retraction.isValid(),
      "timestamped object-deletion mixed advances delete returned an invalid retraction handle");
  for (auto* member : members) {
    require(
        member->recorder().timedRemovals().empty(),
        "timestamped object-deletion mixed advances delivered before its request");
    member->recorder().clearCallbackOrder();
  }
  publisher.recorder().clearCallbackOrder();

  flushQueue.rtiAmbassador().flushQueueRequest(*flushQueueBoundary);
  timeAdvanceAvailable.rtiAmbassador().timeAdvanceRequestAvailable(
      *timeAdvanceAvailableTarget);
  nextMessageAvailable.rtiAmbassador().nextMessageRequestAvailable(
      *nextMessageAvailableBoundary);
  publisher.rtiAmbassador().timeAdvanceRequest(*publisherTarget);

  auto allSessions = members;
  allSessions.insert(allSessions.begin(), &publisher);
  waitForSessions(
      allSessions,
      [&] {
        return publisher.recorder().timeAdvanceGrants().size() >= 1U &&
            flushQueue.recorder().timedRemovals().size() >= 1U &&
            flushQueue.recorder().flushQueueGrants().size() >= 1U &&
            timeAdvanceAvailable.recorder().timedRemovals().size() >= 1U &&
            timeAdvanceAvailable.recorder().timeAdvanceGrants().size() >= 1U &&
            nextMessageAvailable.recorder().timedRemovals().size() >= 1U &&
            nextMessageAvailable.recorder().timeAdvanceGrants().size() >= 1U;
      },
      options,
      "timestamped object-deletion mixed advances delivery and grants");
  publisher.pump();

  require(
      publisher.recorder().timeAdvanceGrants().size() == 1U &&
          publisher.recorder().timeAdvanceGrants().front().encoded ==
              encodeTime(*publisherTarget),
      "timestamped object-deletion mixed advances returned the wrong publisher grant");
  require(
      flushQueue.recorder().timedRemovals().size() == 1U &&
          flushQueue.recorder().flushQueueGrants().size() == 1U &&
          flushQueue.recorder().timeAdvanceGrants().empty(),
      "Flush Queue Request did not produce exactly one removal and flush grant");
  require(
      timeAdvanceAvailable.recorder().timedRemovals().size() == 1U &&
          timeAdvanceAvailable.recorder().timeAdvanceGrants().size() == 1U &&
          timeAdvanceAvailable.recorder().flushQueueGrants().empty(),
      "Time Advance Request Available did not produce exactly one removal and grant");
  require(
      nextMessageAvailable.recorder().timedRemovals().size() == 1U &&
          nextMessageAvailable.recorder().timeAdvanceGrants().size() == 1U &&
          nextMessageAvailable.recorder().flushQueueGrants().empty(),
      "Next Message Request Available did not produce exactly one removal and grant");
  require(
      flushQueue.recorder().callbackOrder() ==
          std::vector<std::string>{"remove", "flush-grant"},
      "Flush Queue Request delivered its grant before the object removal");
  require(
      timeAdvanceAvailable.recorder().callbackOrder() ==
          std::vector<std::string>{"remove", "grant"},
      "Time Advance Request Available delivered its grant before the object removal");
  require(
      nextMessageAvailable.recorder().callbackOrder() ==
          std::vector<std::string>{"remove", "grant"},
      "Next Message Request Available delivered its grant before the object removal");

  auto assertRemoval = [&](TimedRemovalRecord const& removal) {
    require(
        removal.object == object && removal.tag == tagBytes &&
            removal.producer == publisher.federateHandle(),
        "timestamped object-deletion mixed advances returned the wrong removal identity or metadata");
    require(
        removal.time == encodeTime(*deletionTime) && !removal.timeText.empty() &&
            removal.sentOrder == rti::TIMESTAMP &&
            removal.receivedOrder == rti::TIMESTAMP,
        "timestamped object-deletion mixed advances returned the wrong time or order metadata");
    require(
        removal.retractionPresent && removal.retraction == copyBytes(retraction.encode()),
        "timestamped object-deletion mixed advances omitted the retraction handle");
  };
  assertRemoval(flushQueue.recorder().timedRemovals().front());
  assertRemoval(timeAdvanceAvailable.recorder().timedRemovals().front());
  assertRemoval(nextMessageAvailable.recorder().timedRemovals().front());

  auto const flushGrants = flushQueue.recorder().flushQueueGrants();
  require(
      flushGrants.front().optimisticTime == encodeTime(*deletionTime) &&
          !flushGrants.front().time.empty() &&
          !flushGrants.front().timeText.empty() &&
          !flushGrants.front().optimisticTimeText.empty(),
      "Flush Queue Grant returned the wrong timestamped deletion times");
  auto actualFlushTime = decodeTime(
      *flushQueueTime.factory,
      flushGrants.front().time);
  auto optimisticFlushTime = decodeTime(
      *flushQueueTime.factory,
      flushGrants.front().optimisticTime);
  require(
      actualFlushTime != nullptr && optimisticFlushTime != nullptr &&
          !(*actualFlushTime < *flushQueueTime.initial) &&
          !(*flushQueueBoundary < *actualFlushTime) &&
          !(*optimisticFlushTime < *actualFlushTime),
      "Flush Queue Grant returned an actual time outside its standard bounds");
  require(
      timeAdvanceAvailable.recorder().timeAdvanceGrants().front().encoded ==
              encodeTime(*timeAdvanceAvailableTarget) &&
          nextMessageAvailable.recorder().timeAdvanceGrants().front().encoded ==
              encodeTime(*deletionTime),
      "alternate object-deletion advances returned the wrong grant time");

  auto queriedFlushQueue = flushQueueTime.factory->makeInitial();
  flushQueue.rtiAmbassador().queryLogicalTime(*queriedFlushQueue);
  require(
      encodeTime(*queriedFlushQueue) == flushGrants.front().time,
      "Flush Queue Request query did not match its actual object-deletion grant time");
  auto queriedTimeAdvanceAvailable = timeAdvanceAvailableTime.factory->makeInitial();
  timeAdvanceAvailable.rtiAmbassador().queryLogicalTime(*queriedTimeAdvanceAvailable);
  requireTimeEquals(
      *queriedTimeAdvanceAvailable,
      *timeAdvanceAvailableTarget,
      "Time Advance Request Available object-deletion logical-time query");
  auto queriedNextMessageAvailable = nextMessageAvailableTime.factory->makeInitial();
  nextMessageAvailable.rtiAmbassador().queryLogicalTime(*queriedNextMessageAvailable);
  requireTimeEquals(
      *queriedNextMessageAvailable,
      *deletionTime,
      "Next Message Request Available object-deletion logical-time query");
  requireException(
      [&] { publisher.rtiAmbassador().retract(retraction); },
      L"MessageCanNoLongerBeRetracted",
      "retracting a delivered timestamped object deletion through alternate advances");

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

void scenarioTimestampedObjectDeletion(Options const& options, rti::CallbackModel model) {
  Session publisher(options, model, "owner");
  Session immediate(options, model, "member");
  Session constrained(options, model, "member");
  auto const federation = federationName(options, "timestamped-object-deletion");
  connectAndJoin(publisher, immediate, options, federation, options.fom);
  constrained.connect();
  constrained.join(
      options.memberFederateName + L"-timestamped-deletion-constrained",
      options.federateType,
      federation);

  auto const publisherClass = publisher.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const immediateClass = immediate.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const constrainedClass = constrained.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const publisherAttribute = publisher.rtiAmbassador().getAttributeHandle(
      publisherClass, options.attributeName);
  auto const immediateAttribute = immediate.rtiAmbassador().getAttributeHandle(
      immediateClass, options.attributeName);
  auto const constrainedAttribute = constrained.rtiAmbassador().getAttributeHandle(
      constrainedClass, options.attributeName);
  require(
      publisherClass.isValid() && immediateClass.isValid() && constrainedClass.isValid(),
      "timestamped object-deletion object-class lookup returned an invalid handle");
  require(
      publisherAttribute.isValid() && immediateAttribute.isValid() &&
          constrainedAttribute.isValid(),
      "timestamped object-deletion attribute lookup returned an invalid handle");

  rti::AttributeHandleSet publisherAttributes;
  publisherAttributes.insert(publisherAttribute);
  rti::AttributeHandleSet immediateAttributes;
  immediateAttributes.insert(immediateAttribute);
  rti::AttributeHandleSet constrainedAttributes;
  constrainedAttributes.insert(constrainedAttribute);
  publisher.rtiAmbassador().publishObjectClassAttributes(
      publisherClass, publisherAttributes);
  immediate.rtiAmbassador().subscribeObjectClassAttributes(
      immediateClass, immediateAttributes, true, L"");
  constrained.rtiAmbassador().subscribeObjectClassAttributes(
      constrainedClass, constrainedAttributes, true, L"");

  auto registerAndDiscover = [&] {
    immediate.recorder().clearDiscovery();
    constrained.recorder().clearDiscovery();
    auto const object = publisher.rtiAmbassador().registerObjectInstance(publisherClass);
    require(object.isValid(), "timestamped object-deletion registration returned an invalid handle");
    auto const name = publisher.rtiAmbassador().getObjectInstanceName(object);
    require(!name.empty(), "timestamped object-deletion registration returned an empty name");
    waitFor(
        immediate,
        [&] { return immediate.recorder().discovery().present; },
        options,
        "timestamped object-deletion immediate discovery");
    waitFor(
        constrained,
        [&] { return constrained.recorder().discovery().present; },
        options,
        "timestamped object-deletion constrained discovery");
    require(
        immediate.recorder().discovery().object == object &&
            constrained.recorder().discovery().object == object,
        "timestamped object-deletion discovery returned the wrong object handle");
    return std::pair{object, name};
  };

  auto const first = registerAndDiscover();
  auto publisherTime = makeTimeContext(publisher);
  auto immediateTime = makeTimeContext(immediate);
  auto constrainedTime = makeTimeContext(constrained);
  require(
      publisherTime.factory->getName() == immediateTime.factory->getName() &&
          publisherTime.factory->getName() == constrainedTime.factory->getName(),
      "timestamped object-deletion members selected different logical-time factories");
  enableTimestampedRoles(
      publisher,
      constrained,
      publisherTime,
      constrainedTime,
      options,
      "timestamped object-deletion");
  auto const invalidDeletionTime = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      3U);
  requireException(
      [&] {
        publisher.rtiAmbassador().deleteObjectInstance(
            rti::ObjectInstanceHandle{},
            rti::VariableLengthData{},
            *invalidDeletionTime);
      },
      L"ObjectInstanceNotKnown",
      "timestamped object deletion for an unknown object");

  auto assertRemoval = [&](TimedRemovalRecord const& record,
                           rti::ObjectInstanceHandle const& expectedObject,
                           std::vector<std::uint8_t> const& expectedTag,
                           std::vector<std::uint8_t> const& expectedTime,
                           rti::OrderType expectedReceivedOrder) {
    require(record.object == expectedObject,
            "timestamped object removal returned the wrong object handle");
    require(record.tag == expectedTag,
            "timestamped object removal did not preserve the user tag");
    require(record.producer == publisher.federateHandle(),
            "timestamped object removal returned the wrong producing federate");
    require(record.time == expectedTime,
            "timestamped object removal returned the wrong logical time");
    require(!record.timeText.empty(),
            "timestamped object removal returned no logical-time representation");
    require(record.sentOrder == rti::TIMESTAMP,
            "timestamped object removal returned a non-timestamp sent order");
    require(record.receivedOrder == expectedReceivedOrder,
            "timestamped object removal returned the wrong received order");
  };

  auto firstTime = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      3U);
  auto firstConstrainedTime = timeAfter(
      *constrainedTime.factory,
      *constrainedTime.initial,
      *constrainedTime.epsilon,
      3U);
  std::vector<std::uint8_t> firstTagBytes{0xD1U, 0xE2U, 0xF3U};
  rti::VariableLengthData firstTag(firstTagBytes.data(), firstTagBytes.size());
  auto firstRetraction = publisher.rtiAmbassador().deleteObjectInstance(
      first.first,
      firstTag,
      *firstTime);
  require(
      firstRetraction.isValid(),
      "timestamped object deletion returned an invalid retraction handle");
  waitFor(
      immediate,
      [&] { return immediate.recorder().timedRemovals().size() >= 1U; },
      options,
      "non-constrained timestamped object removal");
  auto const firstImmediate = immediate.recorder().timedRemovals();
  require(
      firstImmediate.size() == 1U,
      "non-constrained recipient received duplicate timestamped object removal");
  assertRemoval(
      firstImmediate.front(), first.first, firstTagBytes, encodeTime(*firstTime), rti::RECEIVE);
  require(
      firstImmediate.front().retractionPresent &&
          firstImmediate.front().retraction == copyBytes(firstRetraction.encode()),
      "timestamped object removal returned the wrong retraction handle");
  require(
      constrained.recorder().timedRemovals().empty(),
      "time-constrained recipient received a timestamped object removal before its grant");
  requireException(
      [&] {
        static_cast<void>(immediate.rtiAmbassador().getObjectInstanceHandle(first.second));
      },
      L"ObjectInstanceNotKnown",
      "object lookup after immediate timestamped removal");
  require(
      constrained.rtiAmbassador().getObjectInstanceHandle(first.second) == first.first,
      "pending timestamped object removal changed constrained object identity");

  publisher.rtiAmbassador().retract(firstRetraction);
  waitFor(
      immediate,
      [&] { return immediate.recorder().retractions().size() >= 1U; },
      options,
      "retraction callback for delivered timestamped object removal");
  auto const firstRetractionCallbacks = immediate.recorder().retractions();
  require(
      firstRetractionCallbacks.size() == 1U && firstRetractionCallbacks.front().valid &&
          firstRetractionCallbacks.front().encoded == copyBytes(firstRetraction.encode()),
      "timestamped object-removal retraction callback returned the wrong message handle");
  require(
      constrained.recorder().retractions().empty(),
      "pending timestamped object removal produced a retraction callback before delivery");
  require(
      immediate.rtiAmbassador().getObjectInstanceHandle(first.second) == first.first &&
          constrained.rtiAmbassador().getObjectInstanceHandle(first.second) == first.first,
      "timestamped object-removal retraction did not reconstitute the object identity");

  immediate.recorder().clearCallbackOrder();
  constrained.recorder().clearCallbackOrder();
  publisher.recorder().clearCallbackOrder();
  constrained.rtiAmbassador().timeAdvanceRequest(*firstConstrainedTime);
  publisher.rtiAmbassador().timeAdvanceRequest(*firstTime);
  waitFor(
      publisher,
      constrained,
      [&] {
        return publisher.recorder().timeAdvanceGrants().size() >= 1U &&
            constrained.recorder().timeAdvanceGrants().size() >= 1U;
      },
      options,
      "first timestamped object-deletion time advance grants");
  require(
      constrained.recorder().timedRemovals().empty(),
      "retracted timestamped object removal was delivered at its grant");

  auto const second = registerAndDiscover();
  auto secondTime = timeAfter(
      *publisherTime.factory,
      *firstTime,
      *publisherTime.epsilon,
      3U);
  auto secondConstrainedTime = timeAfter(
      *constrainedTime.factory,
      *firstConstrainedTime,
      *constrainedTime.epsilon,
      3U);
  std::vector<std::uint8_t> secondTagBytes{0xA1U, 0xB2U, 0xC3U};
  rti::VariableLengthData secondTag(secondTagBytes.data(), secondTagBytes.size());
  auto secondRetraction = publisher.rtiAmbassador().deleteObjectInstance(
      second.first,
      secondTag,
      *secondTime);
  require(
      secondRetraction.isValid(),
      "second timestamped object deletion returned an invalid retraction handle");
  waitFor(
      immediate,
      [&] { return immediate.recorder().timedRemovals().size() >= 2U; },
      options,
      "second non-constrained timestamped object removal");
  auto const secondImmediate = immediate.recorder().timedRemovals();
  require(
      secondImmediate.size() == 2U,
      "non-constrained recipient received duplicate second timestamped object removal");
  assertRemoval(
      secondImmediate.at(1), second.first, secondTagBytes, encodeTime(*secondTime), rti::RECEIVE);
  require(
      secondImmediate.at(1).retractionPresent &&
          secondImmediate.at(1).retraction == copyBytes(secondRetraction.encode()),
      "second timestamped object removal returned the wrong retraction handle");

  constrained.rtiAmbassador().timeAdvanceRequest(*secondConstrainedTime);
  publisher.rtiAmbassador().timeAdvanceRequest(*secondTime);
  waitFor(
      publisher,
      constrained,
      [&] {
        return publisher.recorder().timeAdvanceGrants().size() >= 2U &&
            constrained.recorder().timeAdvanceGrants().size() >= 2U &&
            constrained.recorder().timedRemovals().size() >= 1U;
      },
      options,
      "second timestamped object removal and time advance grants");
  auto const constrainedRemovals = constrained.recorder().timedRemovals();
  require(
      constrainedRemovals.size() == 1U,
      "time-constrained recipient received the wrong number of timestamped object removals");
  assertRemoval(
      constrainedRemovals.front(),
      second.first,
      secondTagBytes,
      encodeTime(*secondTime),
      rti::TIMESTAMP);
  require(
      constrainedRemovals.front().retractionPresent &&
          constrainedRemovals.front().retraction == copyBytes(secondRetraction.encode()),
      "time-constrained timestamped object removal returned the wrong retraction handle");
  requireException(
      [&] { publisher.rtiAmbassador().retract(secondRetraction); },
      L"MessageCanNoLongerBeRetracted",
      "retracting a delivered timestamped object deletion at the producer boundary");
  requireException(
      [&] {
        static_cast<void>(immediate.rtiAmbassador().getObjectInstanceHandle(second.second));
      },
      L"ObjectInstanceNotKnown",
      "object lookup after terminal timestamped removal at immediate recipient");
  requireException(
      [&] {
        static_cast<void>(constrained.rtiAmbassador().getObjectInstanceHandle(second.second));
      },
      L"ObjectInstanceNotKnown",
      "object lookup after terminal timestamped removal at constrained recipient");

  constrained.rtiAmbassador().disableTimeConstrained();
  publisher.rtiAmbassador().disableTimeRegulation();
  immediate.rtiAmbassador().unsubscribeObjectClassAttributes(
      immediateClass, immediateAttributes);
  constrained.rtiAmbassador().unsubscribeObjectClassAttributes(
      constrainedClass, constrainedAttributes);
  publisher.rtiAmbassador().unpublishObjectClassAttributes(
      publisherClass, publisherAttributes);
  constrained.resign(rti::NO_ACTION);
  immediate.resign(rti::NO_ACTION);
  publisher.resign(rti::NO_ACTION);
  publisher.rtiAmbassador().destroyFederationExecution(federation);
  constrained.disconnect();
  immediate.disconnect();
  publisher.disconnect();
}

void scenarioTimestampedObjectDeletionJoinedOwnerRetraction(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.multiAttributeFom.empty(),
      "Joined-owner timestamped deletion testing requires an adapter-supplied multi-attribute FOM");
  require(
      !options.logicalTimeImplementationName.empty(),
      "Joined-owner timestamped deletion testing requires an adapter-supplied logical-time implementation");

  Session publisher(options, model, "joined-owner-deletion-publisher");
  Session formerOwner(options, model, "joined-owner-deletion-former-owner");
  Session constrained(options, model, "joined-owner-deletion-constrained");
  auto const federation = federationName(
      options,
      "timestamped-object-deletion-retraction-joined-owners");

  publisher.connect();
  formerOwner.connect();
  constrained.connect();
  publisher.rtiAmbassador().createFederationExecution(
      federation,
      options.multiAttributeFom.wstring(),
      options.logicalTimeImplementationName);
  publisher.join(
      options.ownerFederateName + L"-joined-owner-deletion-publisher",
      options.federateType,
      federation);
  formerOwner.join(
      options.memberFederateName + L"-joined-owner-deletion-former-owner",
      options.federateType,
      federation);
  constrained.join(
      options.memberFederateName + L"-joined-owner-deletion-constrained",
      options.federateType,
      federation);

  auto const publisherClass = publisher.rtiAmbassador().getObjectClassHandle(
      options.multiAttributeObjectClassName);
  auto const formerClass = formerOwner.rtiAmbassador().getObjectClassHandle(
      options.multiAttributeObjectClassName);
  auto const constrainedClass = constrained.rtiAmbassador().getObjectClassHandle(
      options.multiAttributeObjectClassName);
  auto const publisherFirst = publisher.rtiAmbassador().getAttributeHandle(
      publisherClass,
      options.multiAttributeFirstName);
  auto const publisherSecond = publisher.rtiAmbassador().getAttributeHandle(
      publisherClass,
      options.multiAttributeSecondName);
  auto const formerFirst = formerOwner.rtiAmbassador().getAttributeHandle(
      formerClass,
      options.multiAttributeFirstName);
  auto const formerSecond = formerOwner.rtiAmbassador().getAttributeHandle(
      formerClass,
      options.multiAttributeSecondName);
  auto const constrainedFirst = constrained.rtiAmbassador().getAttributeHandle(
      constrainedClass,
      options.multiAttributeFirstName);
  auto const constrainedSecond = constrained.rtiAmbassador().getAttributeHandle(
      constrainedClass,
      options.multiAttributeSecondName);
  require(
      publisherClass.isValid() && formerClass.isValid() &&
          constrainedClass.isValid(),
      "Joined-owner timestamped deletion object-class lookup returned an invalid handle");
  require(
      publisherFirst.isValid() && publisherSecond.isValid() &&
          formerFirst.isValid() && formerSecond.isValid() &&
          constrainedFirst.isValid() && constrainedSecond.isValid(),
      "Joined-owner timestamped deletion attribute lookup returned an invalid handle");
  require(
      publisherFirst != publisherSecond,
      "Joined-owner timestamped deletion adapter supplied duplicate attribute names");

  rti::AttributeHandleSet const publisherAttributes{
      publisherFirst,
      publisherSecond};
  rti::AttributeHandleSet const formerAttributes{
      formerFirst,
      formerSecond};
  rti::AttributeHandleSet const constrainedAttributes{
      constrainedFirst,
      constrainedSecond};
  rti::AttributeHandleSet const publisherFirstOnly{publisherFirst};
  rti::AttributeHandleSet const publisherSecondOnly{publisherSecond};
  rti::AttributeHandleSet const formerSecondOnly{formerSecond};

  publisher.rtiAmbassador().publishObjectClassAttributes(
      publisherClass,
      publisherAttributes);
  formerOwner.rtiAmbassador().subscribeObjectClassAttributes(
      formerClass,
      formerAttributes,
      true,
      L"");
  constrained.rtiAmbassador().subscribeObjectClassAttributes(
      constrainedClass,
      constrainedAttributes,
      true,
      L"");

  auto const object = publisher.rtiAmbassador().registerObjectInstance(
      publisherClass);
  require(
      object.isValid(),
      "Joined-owner timestamped deletion registration returned an invalid object handle");
  auto const objectName = publisher.rtiAmbassador().getObjectInstanceName(object);
  require(
      !objectName.empty(),
      "Joined-owner timestamped deletion registration returned an empty object name");
  waitFor(
      formerOwner,
      [&] { return formerOwner.recorder().hasDiscovery(object); },
      options,
      "joined-owner timestamped deletion former-owner discovery");
  waitFor(
      constrained,
      [&] { return constrained.recorder().hasDiscovery(object); },
      options,
      "joined-owner timestamped deletion constrained discovery");
  require(
      formerOwner.rtiAmbassador().getObjectInstanceHandle(objectName) == object &&
          constrained.rtiAmbassador().getObjectInstanceHandle(objectName) == object,
      "Joined-owner timestamped deletion discovery changed object identity");

  // Transfer only the second attribute to the member that will resign. The
  // first attribute stays with the producer so the later retraction can prove
  // that a departed owner's former attribute remains unowned.
  formerOwner.rtiAmbassador().publishObjectClassAttributes(
      formerClass,
      formerSecondOnly);
  formerOwner.recorder().clearOwnershipRecords();
  publisher.recorder().clearOwnershipRecords();
  std::vector<std::uint8_t> const ownershipTagBytes{
      0x4AU, 0x4FU, 0x49U, 0x4EU, 0x45U, 0x44U};
  rti::VariableLengthData ownershipTag(
      ownershipTagBytes.data(),
      ownershipTagBytes.size());
  if (model == rti::HLA_EVOKED) {
    formerOwner.rtiAmbassador().attributeOwnershipAcquisitionIfAvailable(
        object,
        formerSecondOnly,
        ownershipTag);
  } else {
    formerOwner.rtiAmbassador().attributeOwnershipAcquisition(
        object,
        formerSecondOnly,
        ownershipTag);
  }
  rti::AttributeHandleSet divested;
  publisher.rtiAmbassador().attributeOwnershipDivestitureIfWanted(
      object,
      publisherSecondOnly,
      ownershipTag,
      divested);
  require(
      divested == publisherSecondOnly,
      "Joined-owner timestamped deletion did not transfer exactly the selected attribute");
  waitFor(
      formerOwner,
      [&] { return formerOwner.recorder().ownershipAcquisition().has_value(); },
      options,
      "joined-owner timestamped deletion ownership acquisition");
  auto const acquisition = formerOwner.recorder().ownershipAcquisition();
  require(
      acquisition->object == object &&
          acquisition->attributes == formerSecondOnly &&
          acquisition->tag == ownershipTagBytes,
      "Joined-owner timestamped deletion ownership acquisition returned the wrong transfer");
  require(
      publisher.rtiAmbassador().isAttributeOwnedByFederate(object, publisherFirst) &&
          !publisher.rtiAmbassador().isAttributeOwnedByFederate(object, publisherSecond) &&
          formerOwner.rtiAmbassador().isAttributeOwnedByFederate(object, formerSecond),
      "Joined-owner timestamped deletion did not establish split ownership");

  auto publisherTime = makeTimeContext(publisher);
  auto constrainedTime = makeTimeContext(constrained);
  require(
      publisherTime.factory->getName() == constrainedTime.factory->getName(),
      "Joined-owner timestamped deletion members selected different logical-time factories");
  constrained.rtiAmbassador().enableTimeConstrained();
  waitFor(
      constrained,
      [&] { return constrained.recorder().timeConstrainedEnabled().size() >= 1U; },
      options,
      "joined-owner timestamped deletion time-constrained callback");
  auto const lookaheadTarget = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      5U);
  auto lookahead = publisherTime.factory->makeZero();
  require(
      lookahead != nullptr,
      "Joined-owner timestamped deletion could not allocate a lookahead");
  lookahead->setToDifference(*lookaheadTarget, *publisherTime.initial);
  publisher.rtiAmbassador().enableTimeRegulation(*lookahead);
  waitFor(
      publisher,
      [&] { return publisher.recorder().timeRegulationEnabled().size() >= 1U; },
      options,
      "joined-owner timestamped deletion time-regulation callback");

  auto const deletionTime = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      6U);
  std::vector<std::uint8_t> const deletionTagBytes{
      0x44U, 0x37U, 0x2DU, 0x4AU, 0x4FU, 0x49U, 0x4EU, 0x45U, 0x44U};
  rti::VariableLengthData deletionTag(
      deletionTagBytes.data(),
      deletionTagBytes.size());
  auto const retraction = publisher.rtiAmbassador().deleteObjectInstance(
      object,
      deletionTag,
      *deletionTime);
  require(
      retraction.isValid(),
      "Joined-owner timestamped deletion returned an invalid retraction handle");
  waitFor(
      formerOwner,
      [&] { return formerOwner.recorder().timedRemovals().size() >= 1U; },
      options,
      "joined-owner timestamped deletion former-owner removal");
  auto const formerRemovals = formerOwner.recorder().timedRemovals();
  require(
      formerRemovals.size() == 1U &&
          formerRemovals.front().object == object &&
          formerRemovals.front().tag == deletionTagBytes &&
          formerRemovals.front().producer == publisher.federateHandle() &&
          formerRemovals.front().time == encodeTime(*deletionTime) &&
          formerRemovals.front().sentOrder == rti::TIMESTAMP &&
          formerRemovals.front().receivedOrder == rti::RECEIVE &&
          formerRemovals.front().retractionPresent &&
          formerRemovals.front().retraction == copyBytes(retraction.encode()),
      "Joined-owner timestamped deletion returned the wrong former-owner removal");
  require(
      constrained.recorder().timedRemovals().empty(),
      "Joined-owner timestamped deletion delivered before the constrained grant");

  formerOwner.resign(rti::CANCEL_THEN_DELETE_THEN_DIVEST);
  require(
      formerOwner.recorder().retractions().empty(),
      "Joined-owner timestamped deletion delivered a retraction callback during resignation");

  auto const constrainedTarget = timeAfter(
      *constrainedTime.factory,
      *constrainedTime.initial,
      *constrainedTime.epsilon,
      6U);
  constrained.rtiAmbassador().timeAdvanceRequest(*constrainedTarget);
  publisher.rtiAmbassador().retract(retraction);
  require(
      formerOwner.recorder().retractions().empty(),
      "Joined-owner timestamped deletion notified a departed former owner");
  require(
      publisher.rtiAmbassador().getObjectInstanceName(object) == objectName &&
          constrained.rtiAmbassador().getObjectInstanceHandle(objectName) == object,
      "Joined-owner timestamped deletion did not reconstitute joined object identity");
  require(
      publisher.rtiAmbassador().isAttributeOwnedByFederate(object, publisherFirst) &&
          !publisher.rtiAmbassador().isAttributeOwnedByFederate(object, publisherSecond),
      "Joined-owner timestamped deletion reconstituted a departed owner's former attribute");

  publisher.recorder().clearOwnershipRecords();
  publisher.rtiAmbassador().queryAttributeOwnership(object, publisherSecondOnly);
  waitFor(
      publisher,
      [&] { return publisher.recorder().ownershipNotOwned().has_value(); },
      options,
      "joined-owner timestamped deletion unowned-attribute query");
  auto const unowned = publisher.recorder().ownershipNotOwned();
  require(
      unowned->object == object && unowned->attributes == publisherSecondOnly,
      "Joined-owner timestamped deletion ownership query returned the wrong unowned attribute");

  auto const publisherTarget = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      2U);
  publisher.rtiAmbassador().timeAdvanceRequest(*publisherTarget);
  waitForSessions(
      {&publisher, &constrained},
      [&] {
        return publisher.recorder().timeAdvanceGrants().size() >= 1U &&
            constrained.recorder().timeAdvanceGrants().size() >= 1U;
      },
      options,
      "joined-owner timestamped deletion post-retraction grants");
  require(
      constrained.recorder().timedRemovals().empty() &&
          constrained.recorder().retractions().empty(),
      "Joined-owner timestamped deletion delivered stale callbacks after retraction");

  constrained.rtiAmbassador().disableTimeConstrained();
  publisher.rtiAmbassador().disableTimeRegulation();
  constrained.rtiAmbassador().unsubscribeObjectClassAttributes(
      constrainedClass,
      constrainedAttributes);
  publisher.rtiAmbassador().unpublishObjectClassAttributes(
      publisherClass,
      publisherAttributes);
  constrained.resign(rti::NO_ACTION);
  publisher.resign(rti::CANCEL_THEN_DELETE_THEN_DIVEST);
  publisher.rtiAmbassador().destroyFederationExecution(federation);
  constrained.disconnect();
  formerOwner.disconnect();
  publisher.disconnect();
}

void scenarioTimestampedDefaultRegionInteractionRegulationReenableContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioTimestampedDefaultRegionInteractionRegulationReenable(options, model);
}

void scenarioTimestampedLocalDeleteObject(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.logicalTimeImplementationName.empty(),
      "Timestamped local object deletion requires an adapter-supplied logical-time implementation");

  Session publisher(options, model, "timestamped-local-delete-publisher");
  Session local(options, model, "timestamped-local-delete-local");
  Session surviving(options, model, "timestamped-local-delete-surviving");
  auto const federation = federationName(options, "timestamped-local-delete-object");
  connectAndJoin(publisher, local, options, federation, options.fom);
  surviving.connect();
  surviving.join(
      options.memberFederateName + L"-timestamped-local-delete-surviving",
      options.federateType,
      federation);

  auto const publisherClass = publisher.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const localClass = local.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const survivingClass = surviving.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const publisherAttribute = publisher.rtiAmbassador().getAttributeHandle(
      publisherClass,
      options.attributeName);
  auto const localAttribute = local.rtiAmbassador().getAttributeHandle(
      localClass,
      options.attributeName);
  auto const survivingAttribute = surviving.rtiAmbassador().getAttributeHandle(
      survivingClass,
      options.attributeName);
  require(
      publisherClass.isValid() && localClass.isValid() && survivingClass.isValid(),
      "Timestamped local object deletion object-class lookup returned an invalid handle");
  require(
      publisherAttribute.isValid() && localAttribute.isValid() &&
          survivingAttribute.isValid(),
      "Timestamped local object deletion attribute lookup returned an invalid handle");

  rti::AttributeHandleSet const publisherAttributes{publisherAttribute};
  rti::AttributeHandleSet const localAttributes{localAttribute};
  rti::AttributeHandleSet const survivingAttributes{survivingAttribute};
  publisher.rtiAmbassador().publishObjectClassAttributes(
      publisherClass,
      publisherAttributes);
  publisher.rtiAmbassador().changeDefaultAttributeOrderType(
      publisherClass,
      publisherAttributes,
      rti::TIMESTAMP);
  local.rtiAmbassador().subscribeObjectClassAttributes(
      localClass,
      localAttributes,
      true,
      L"");
  surviving.rtiAmbassador().subscribeObjectClassAttributes(
      survivingClass,
      survivingAttributes,
      true,
      L"");

  auto const object = publisher.rtiAmbassador().registerObjectInstance(publisherClass);
  require(
      object.isValid(),
      "Timestamped local object deletion registration returned an invalid object handle");
  auto const objectName = publisher.rtiAmbassador().getObjectInstanceName(object);
  require(
      !objectName.empty(),
      "Timestamped local object deletion registration returned an empty object name");
  waitFor(
      local,
      [&] { return local.recorder().hasDiscovery(object); },
      options,
      "Timestamped local object deletion local discovery");
  waitFor(
      surviving,
      [&] { return surviving.recorder().hasDiscovery(object); },
      options,
      "Timestamped local object deletion surviving discovery");
  require(
      local.rtiAmbassador().getObjectInstanceHandle(objectName) == object &&
          surviving.rtiAmbassador().getObjectInstanceHandle(objectName) == object,
      "Timestamped local object deletion discovery changed object identity");

  auto publisherTime = makeTimeContext(publisher);
  auto localTime = makeTimeContext(local);
  auto survivingTime = makeTimeContext(surviving);
  require(
      publisherTime.factory->getName() == localTime.factory->getName() &&
          publisherTime.factory->getName() == survivingTime.factory->getName(),
      "Timestamped local object deletion members selected different logical-time factories");
  auto makeLookahead = [](TimeContext const& time) {
    auto const target = timeAfter(*time.factory, *time.initial, *time.epsilon, 5U);
    auto result = time.factory->makeZero();
    require(
        result != nullptr,
        "Timestamped local object deletion could not allocate a lookahead interval");
    result->setToDifference(*target, *time.initial);
    return result;
  };
  auto publisherLookahead = makeLookahead(publisherTime);

  local.rtiAmbassador().enableTimeConstrained();
  waitFor(
      local,
      [&] { return local.recorder().timeConstrainedEnabled().size() >= 1U; },
      options,
      "Timestamped local object deletion local time-constrained callback");
  surviving.rtiAmbassador().enableTimeConstrained();
  waitFor(
      surviving,
      [&] { return surviving.recorder().timeConstrainedEnabled().size() >= 1U; },
      options,
      "Timestamped local object deletion surviving time-constrained callback");
  publisher.rtiAmbassador().enableTimeRegulation(*publisherLookahead);
  waitFor(
      publisher,
      [&] { return publisher.recorder().timeRegulationEnabled().size() >= 1U; },
      options,
      "Timestamped local object deletion publisher time-regulation callback");

  auto const messageTime = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      6U);
  auto const localTarget = timeAfter(
      *localTime.factory,
      *localTime.initial,
      *localTime.epsilon,
      6U);
  auto const survivingTarget = timeAfter(
      *survivingTime.factory,
      *survivingTime.initial,
      *survivingTime.epsilon,
      6U);
  auto const publisherTarget = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      2U);
  std::vector<std::uint8_t> const tagBytes{
      0x4CU, 0x4FU, 0x43U, 0x41U, 0x4CU, 0x44U, 0x45U, 0x4CU};
  rti::VariableLengthData tag(tagBytes.data(), tagBytes.size());
  auto const retraction = publisher.rtiAmbassador().deleteObjectInstance(
      object,
      tag,
      *messageTime);
  require(
      retraction.isValid(),
      "Timestamped local object deletion returned an invalid retraction handle");
  require(
      local.recorder().timedRemovals().empty() &&
          surviving.recorder().timedRemovals().empty(),
      "Timestamped local object deletion delivered before either recipient frontier");

  local.rtiAmbassador().localDeleteObjectInstance(object);
  requireException(
      [&] { static_cast<void>(local.rtiAmbassador().getKnownObjectClassHandle(object)); },
      L"ObjectInstanceNotKnown",
      "known-object lookup after timestamped local object deletion");

  local.recorder().clearCallbackOrder();
  surviving.recorder().clearCallbackOrder();
  publisher.recorder().clearCallbackOrder();
  local.rtiAmbassador().timeAdvanceRequest(*localTarget);
  surviving.rtiAmbassador().timeAdvanceRequest(*survivingTarget);
  publisher.rtiAmbassador().timeAdvanceRequest(*publisherTarget);
  waitForSessions(
      {&publisher, &local, &surviving},
      [&] {
        return publisher.recorder().timeAdvanceGrants().size() >= 1U &&
            local.recorder().timeAdvanceGrants().size() >= 1U &&
            surviving.recorder().timedRemovals().size() >= 1U &&
            surviving.recorder().timeAdvanceGrants().size() >= 1U;
      },
      options,
      "Timestamped local object deletion deliveries and grants");

  require(
      local.recorder().timedRemovals().empty() &&
          local.recorder().timeAdvanceGrants().size() == 1U &&
          local.recorder().timeAdvanceGrants().front().encoded == encodeTime(*localTarget),
      "Timestamped local object deletion did not suppress only the local queued removal");
  require(
      local.recorder().callbackOrder() == std::vector<std::string>{"grant"},
      "Timestamped local object deletion delivered a stale local removal callback");

  auto const survivingRemovals = surviving.recorder().timedRemovals();
  require(
      survivingRemovals.size() == 1U,
      "Timestamped local object deletion changed the surviving removal fanout");
  auto const& removal = survivingRemovals.front();
  require(
      removal.object == object && removal.tag == tagBytes &&
          removal.producer == publisher.federateHandle(),
      "Timestamped local object deletion returned the wrong surviving removal identity");
  require(
      removal.time == encodeTime(*messageTime) && !removal.timeText.empty() &&
          removal.sentOrder == rti::TIMESTAMP &&
          removal.receivedOrder == rti::TIMESTAMP,
      "Timestamped local object deletion returned the wrong surviving removal metadata");
  require(
      removal.retractionPresent &&
          removal.retraction == copyBytes(retraction.encode()),
      "Timestamped local object deletion omitted the surviving retraction handle");
  require(
      surviving.recorder().timeAdvanceGrants().size() == 1U &&
          surviving.recorder().timeAdvanceGrants().front().encoded ==
              encodeTime(*survivingTarget),
      "Timestamped local object deletion returned the wrong surviving grant");
  auto const survivingCallbackOrder = surviving.recorder().callbackOrder();
  auto const removalPosition = std::find(
      survivingCallbackOrder.begin(),
      survivingCallbackOrder.end(),
      "remove");
  auto const grantPosition = std::find(
      survivingCallbackOrder.begin(),
      survivingCallbackOrder.end(),
      "grant");
  require(
      removalPosition != survivingCallbackOrder.end() &&
          grantPosition != survivingCallbackOrder.end() &&
          removalPosition < grantPosition,
      "Timestamped local object deletion delivered the surviving removal after its grant");
  requireException(
      [&] {
        static_cast<void>(surviving.rtiAmbassador().getObjectInstanceHandle(objectName));
      },
      L"ObjectInstanceNotKnown",
      "object lookup after timestamped local object deletion at surviving recipient");

  local.rtiAmbassador().disableTimeConstrained();
  surviving.rtiAmbassador().disableTimeConstrained();
  publisher.rtiAmbassador().disableTimeRegulation();
  local.rtiAmbassador().unsubscribeObjectClassAttributes(localClass, localAttributes);
  surviving.rtiAmbassador().unsubscribeObjectClassAttributes(
      survivingClass,
      survivingAttributes);
  publisher.rtiAmbassador().unpublishObjectClassAttributes(
      publisherClass,
      publisherAttributes);
  local.resign(rti::NO_ACTION);
  surviving.resign(rti::NO_ACTION);
  publisher.resign(rti::CANCEL_THEN_DELETE_THEN_DIVEST);
  publisher.rtiAmbassador().destroyFederationExecution(federation);
  local.disconnect();
  surviving.disconnect();
  publisher.disconnect();
}

void scenarioTimestampedLocalDeleteAttribute(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.logicalTimeImplementationName.empty(),
      "Timestamped local attribute deletion requires an adapter-supplied logical-time implementation");

  Session publisher(options, model, "timestamped-local-delete-attribute-publisher");
  Session local(options, model, "timestamped-local-delete-attribute-local");
  Session surviving(options, model, "timestamped-local-delete-attribute-surviving");
  auto const federation = federationName(
      options,
      "timestamped-local-delete-attribute");
  connectAndJoin(publisher, local, options, federation, options.fom);
  surviving.connect();
  surviving.join(
      options.memberFederateName + L"-timestamped-local-delete-attribute-surviving",
      options.federateType,
      federation);

  auto const publisherClass = publisher.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const localClass = local.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const survivingClass = surviving.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const publisherAttribute = publisher.rtiAmbassador().getAttributeHandle(
      publisherClass,
      options.attributeName);
  auto const localAttribute = local.rtiAmbassador().getAttributeHandle(
      localClass,
      options.attributeName);
  auto const survivingAttribute = surviving.rtiAmbassador().getAttributeHandle(
      survivingClass,
      options.attributeName);
  require(
      publisherClass.isValid() && localClass.isValid() && survivingClass.isValid(),
      "Timestamped local attribute deletion object-class lookup returned an invalid handle");
  require(
      publisherAttribute.isValid() && localAttribute.isValid() &&
          survivingAttribute.isValid(),
      "Timestamped local attribute deletion attribute lookup returned an invalid handle");

  rti::AttributeHandleSet const publisherAttributes{publisherAttribute};
  rti::AttributeHandleSet const localAttributes{localAttribute};
  rti::AttributeHandleSet const survivingAttributes{survivingAttribute};
  publisher.rtiAmbassador().publishObjectClassAttributes(
      publisherClass,
      publisherAttributes);
  publisher.rtiAmbassador().changeDefaultAttributeOrderType(
      publisherClass,
      publisherAttributes,
      rti::TIMESTAMP);
  local.rtiAmbassador().subscribeObjectClassAttributes(
      localClass,
      localAttributes,
      true,
      L"");
  surviving.rtiAmbassador().subscribeObjectClassAttributes(
      survivingClass,
      survivingAttributes,
      true,
      L"");

  auto const object = publisher.rtiAmbassador().registerObjectInstance(publisherClass);
  require(
      object.isValid(),
      "Timestamped local attribute deletion registration returned an invalid object handle");
  auto const objectName = publisher.rtiAmbassador().getObjectInstanceName(object);
  require(
      !objectName.empty(),
      "Timestamped local attribute deletion registration returned an empty object name");
  waitFor(
      local,
      [&] { return local.recorder().hasDiscovery(object); },
      options,
      "Timestamped local attribute deletion local discovery");
  waitFor(
      surviving,
      [&] { return surviving.recorder().hasDiscovery(object); },
      options,
      "Timestamped local attribute deletion surviving discovery");
  require(
      local.rtiAmbassador().getObjectInstanceHandle(objectName) == object &&
          surviving.rtiAmbassador().getObjectInstanceHandle(objectName) == object,
      "Timestamped local attribute deletion discovery changed object identity");

  auto publisherTime = makeTimeContext(publisher);
  auto localTime = makeTimeContext(local);
  auto survivingTime = makeTimeContext(surviving);
  require(
      publisherTime.factory->getName() == localTime.factory->getName() &&
          publisherTime.factory->getName() == survivingTime.factory->getName(),
      "Timestamped local attribute deletion members selected different logical-time factories");
  auto makeLookahead = [](TimeContext const& time) {
    auto const target = timeAfter(*time.factory, *time.initial, *time.epsilon, 5U);
    auto result = time.factory->makeZero();
    require(
        result != nullptr,
        "Timestamped local attribute deletion could not allocate a lookahead interval");
    result->setToDifference(*target, *time.initial);
    return result;
  };
  auto publisherLookahead = makeLookahead(publisherTime);

  local.rtiAmbassador().enableTimeConstrained();
  waitFor(
      local,
      [&] { return local.recorder().timeConstrainedEnabled().size() >= 1U; },
      options,
      "Timestamped local attribute deletion local time-constrained callback");
  surviving.rtiAmbassador().enableTimeConstrained();
  waitFor(
      surviving,
      [&] { return surviving.recorder().timeConstrainedEnabled().size() >= 1U; },
      options,
      "Timestamped local attribute deletion surviving time-constrained callback");
  publisher.rtiAmbassador().enableTimeRegulation(*publisherLookahead);
  waitFor(
      publisher,
      [&] { return publisher.recorder().timeRegulationEnabled().size() >= 1U; },
      options,
      "Timestamped local attribute deletion publisher time-regulation callback");

  auto assertReflection = [&](TimedReflectionRecord const& record,
                              rti::AttributeHandle const& expectedAttribute,
                              std::vector<std::uint8_t> const& expectedValue,
                              std::vector<std::uint8_t> const& expectedTag,
                              std::vector<std::uint8_t> const& expectedTime,
                              rti::FederateHandle const& expectedProducer,
                              rti::MessageRetractionHandle const& expectedRetraction,
                              rti::OrderType expectedReceivedOrder,
                              Session& inspector) {
    require(
        record.object == object,
        "Timestamped local attribute reflection returned the wrong object handle");
    require(
        record.values.size() == 1U && record.values.count(expectedAttribute) == 1U,
        "Timestamped local attribute reflection returned the wrong attribute set");
    require(
        copyBytes(record.values.at(expectedAttribute)) == expectedValue,
        "Timestamped local attribute reflection returned the wrong attribute value");
    require(
        record.tag == expectedTag,
        "Timestamped local attribute reflection did not preserve the user tag");
    require(
        record.producer == expectedProducer,
        "Timestamped local attribute reflection returned the wrong producing federate");
    require(
        record.transportation.isValid() &&
            !inspector.rtiAmbassador()
                 .getTransportationTypeName(record.transportation)
                 .empty(),
        "Timestamped local attribute reflection returned an unresolvable transportation type");
    require(
        record.time == expectedTime && !record.timeText.empty(),
        "Timestamped local attribute reflection returned the wrong logical time");
    require(
        record.sentOrder == rti::TIMESTAMP &&
            record.receivedOrder == expectedReceivedOrder,
        "Timestamped local attribute reflection returned the wrong order metadata");
    require(
        record.retractionPresent &&
            record.retraction == copyBytes(expectedRetraction.encode()),
        "Timestamped local attribute reflection returned the wrong retraction handle");
    require(
        !record.regions.has_value(),
        "Timestamped local attribute reflection unexpectedly carried regions");
  };

  auto const firstTime = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      6U);
  auto const localFirstTarget = timeAfter(
      *localTime.factory,
      *localTime.initial,
      *localTime.epsilon,
      6U);
  auto const survivingFirstTarget = timeAfter(
      *survivingTime.factory,
      *survivingTime.initial,
      *survivingTime.epsilon,
      6U);
  auto const publisherFirstTarget = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      2U);
  std::vector<std::uint8_t> const valueBytes{0x5AU, 0x31U};
  std::vector<std::uint8_t> const tagBytes{0xD2U, 0x31U};
  rti::AttributeHandleValueMap values;
  values.emplace(
      publisherAttribute,
      rti::VariableLengthData(valueBytes.data(), valueBytes.size()));
  rti::VariableLengthData tag(tagBytes.data(), tagBytes.size());
  auto const firstRetraction = publisher.rtiAmbassador().updateAttributeValues(
      object,
      values,
      tag,
      *firstTime);
  require(
      firstRetraction.isValid(),
      "Timestamped local attribute update returned an invalid retraction handle");
  require(
      local.recorder().timedReflections().empty() &&
          surviving.recorder().timedReflections().empty(),
      "Timestamped local attribute update delivered before either recipient frontier");

  local.rtiAmbassador().localDeleteObjectInstance(object);
  requireException(
      [&] { static_cast<void>(local.rtiAmbassador().getKnownObjectClassHandle(object)); },
      L"ObjectInstanceNotKnown",
      "known-object lookup after timestamped local attribute deletion");

  local.recorder().clearCallbackOrder();
  surviving.recorder().clearCallbackOrder();
  publisher.recorder().clearCallbackOrder();
  local.rtiAmbassador().timeAdvanceRequest(*localFirstTarget);
  surviving.rtiAmbassador().timeAdvanceRequest(*survivingFirstTarget);
  publisher.rtiAmbassador().timeAdvanceRequest(*publisherFirstTarget);
  waitForSessions(
      {&publisher, &local, &surviving},
      [&] {
        return publisher.recorder().timeAdvanceGrants().size() >= 1U &&
            local.recorder().timeAdvanceGrants().size() >= 1U &&
            surviving.recorder().timedReflections().size() >= 1U &&
            surviving.recorder().timeAdvanceGrants().size() >= 1U;
      },
      options,
      "Timestamped local attribute deletion first deliveries and grants");

  require(
      local.recorder().timedReflections().empty() &&
          local.recorder().timeAdvanceGrants().size() == 1U &&
          local.recorder().timeAdvanceGrants().front().encoded ==
              encodeTime(*localFirstTarget),
      "Timestamped local attribute deletion did not suppress only the local queued reflection");
  require(
      local.recorder().callbackOrder() == std::vector<std::string>{"grant"},
      "Timestamped local attribute deletion delivered a stale local reflection callback");

  auto const survivingFirstReflections = surviving.recorder().timedReflections();
  require(
      survivingFirstReflections.size() == 1U,
      "Timestamped local attribute deletion changed the surviving reflection fanout");
  assertReflection(
      survivingFirstReflections.front(),
      survivingAttribute,
      valueBytes,
      tagBytes,
      encodeTime(*firstTime),
      publisher.federateHandle(),
      firstRetraction,
      rti::TIMESTAMP,
      surviving);
  require(
      surviving.recorder().timeAdvanceGrants().size() == 1U &&
          surviving.recorder().timeAdvanceGrants().front().encoded ==
              encodeTime(*survivingFirstTarget),
      "Timestamped local attribute deletion returned the wrong surviving grant");
  auto const survivingFirstCallbackOrder = surviving.recorder().callbackOrder();
  auto const firstReflectionPosition = std::find(
      survivingFirstCallbackOrder.begin(),
      survivingFirstCallbackOrder.end(),
      "reflect");
  auto const firstGrantPosition = std::find(
      survivingFirstCallbackOrder.begin(),
      survivingFirstCallbackOrder.end(),
      "grant");
  require(
      firstReflectionPosition != survivingFirstCallbackOrder.end() &&
          firstGrantPosition != survivingFirstCallbackOrder.end() &&
          firstReflectionPosition < firstGrantPosition,
      "Timestamped local attribute deletion delivered the surviving reflection after its grant");
  requireException(
      [&] {
        static_cast<void>(local.rtiAmbassador().getObjectInstanceHandle(objectName));
      },
      L"ObjectInstanceNotKnown",
      "object lookup after timestamped local attribute deletion at local recipient");
  require(
      surviving.rtiAmbassador().getObjectInstanceHandle(objectName) == object,
      "Timestamped local attribute deletion lost the surviving object identity");

  local.recorder().clearDiscovery();
  local.rtiAmbassador().subscribeObjectClassAttributes(
      localClass,
      localAttributes,
      true,
      L"");
  waitFor(
      local,
      [&] { return local.recorder().hasDiscovery(object); },
      options,
      "Timestamped local attribute deletion rediscovery");
  require(
      local.rtiAmbassador().getObjectInstanceHandle(objectName) == object,
      "Timestamped local attribute deletion rediscovery changed object identity");

  auto const secondTime = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      7U);
  auto const localSecondTarget = timeAfter(
      *localTime.factory,
      *localTime.initial,
      *localTime.epsilon,
      7U);
  auto const survivingSecondTarget = timeAfter(
      *survivingTime.factory,
      *survivingTime.initial,
      *survivingTime.epsilon,
      7U);
  auto const publisherSecondTarget = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      2U);
  auto const secondRetraction = publisher.rtiAmbassador().updateAttributeValues(
      object,
      values,
      tag,
      *secondTime);
  require(
      secondRetraction.isValid(),
      "Second timestamped local attribute update returned an invalid retraction handle");

  local.recorder().clearCallbackOrder();
  surviving.recorder().clearCallbackOrder();
  publisher.recorder().clearCallbackOrder();
  local.rtiAmbassador().timeAdvanceRequest(*localSecondTarget);
  surviving.rtiAmbassador().timeAdvanceRequest(*survivingSecondTarget);
  publisher.rtiAmbassador().timeAdvanceRequest(*publisherSecondTarget);
  waitForSessions(
      {&publisher, &local, &surviving},
      [&] {
        return publisher.recorder().timeAdvanceGrants().size() >= 2U &&
            local.recorder().timedReflections().size() >= 1U &&
            local.recorder().timeAdvanceGrants().size() >= 2U &&
            surviving.recorder().timedReflections().size() >= 2U &&
            surviving.recorder().timeAdvanceGrants().size() >= 2U;
      },
      options,
      "Timestamped local attribute deletion second deliveries and grants");

  auto const localReflections = local.recorder().timedReflections();
  require(
      localReflections.size() == 1U,
      "Timestamped local attribute deletion delivered the wrong local reflection count");
  assertReflection(
      localReflections.front(),
      localAttribute,
      valueBytes,
      tagBytes,
      encodeTime(*secondTime),
      publisher.federateHandle(),
      secondRetraction,
      rti::TIMESTAMP,
      local);
  require(
      local.recorder().timeAdvanceGrants().back().encoded ==
          encodeTime(*localSecondTarget),
      "Timestamped local attribute deletion returned the wrong local second grant");
  auto const localSecondCallbackOrder = local.recorder().callbackOrder();
  auto const localReflectionPosition = std::find(
      localSecondCallbackOrder.begin(),
      localSecondCallbackOrder.end(),
      "reflect");
  auto const localGrantPosition = std::find(
      localSecondCallbackOrder.begin(),
      localSecondCallbackOrder.end(),
      "grant");
  require(
      localReflectionPosition != localSecondCallbackOrder.end() &&
          localGrantPosition != localSecondCallbackOrder.end() &&
          localReflectionPosition < localGrantPosition,
      "Timestamped local attribute deletion delivered the local reflection after its grant");

  auto const survivingReflections = surviving.recorder().timedReflections();
  require(
      survivingReflections.size() == 2U,
      "Timestamped local attribute deletion delivered the wrong surviving reflection count");
  assertReflection(
      survivingReflections.back(),
      survivingAttribute,
      valueBytes,
      tagBytes,
      encodeTime(*secondTime),
      publisher.federateHandle(),
      secondRetraction,
      rti::TIMESTAMP,
      surviving);
  auto const survivingSecondCallbackOrder = surviving.recorder().callbackOrder();
  auto const survivingReflectionPosition = std::find(
      survivingSecondCallbackOrder.begin(),
      survivingSecondCallbackOrder.end(),
      "reflect");
  auto const survivingGrantPosition = std::find(
      survivingSecondCallbackOrder.begin(),
      survivingSecondCallbackOrder.end(),
      "grant");
  require(
      survivingReflectionPosition != survivingSecondCallbackOrder.end() &&
          survivingGrantPosition != survivingSecondCallbackOrder.end() &&
          survivingReflectionPosition < survivingGrantPosition,
      "Timestamped local attribute deletion delivered the second surviving reflection after its grant");

  local.rtiAmbassador().disableTimeConstrained();
  surviving.rtiAmbassador().disableTimeConstrained();
  publisher.rtiAmbassador().disableTimeRegulation();
  local.rtiAmbassador().unsubscribeObjectClassAttributes(localClass, localAttributes);
  surviving.rtiAmbassador().unsubscribeObjectClassAttributes(
      survivingClass,
      survivingAttributes);
  publisher.rtiAmbassador().unpublishObjectClassAttributes(
      publisherClass,
      publisherAttributes);
  local.resign(rti::NO_ACTION);
  surviving.resign(rti::NO_ACTION);
  publisher.resign(rti::CANCEL_THEN_DELETE_THEN_DIVEST);
  publisher.rtiAmbassador().destroyFederationExecution(federation);
  local.disconnect();
  surviving.disconnect();
  publisher.disconnect();
}

void scenarioTimestampedLocalDeleteObjectContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioTimestampedLocalDeleteObject(options, model);
}

void scenarioTimestampedLocalDeleteAttributeContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioTimestampedLocalDeleteAttribute(options, model);
}

void scenarioTimestampedObjectDeletionTombstone(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.logicalTimeImplementationName.empty(),
      "Timestamped object-deletion tombstone requires an adapter-supplied logical-time implementation");

  Session owner(options, model, "owner");
  auto const federation = federationName(
      options,
      "timestamped-object-deletion-tombstone");
  owner.connect();
  owner.rtiAmbassador().createFederationExecution(
      federation,
      options.fom.wstring(),
      options.logicalTimeImplementationName);
  owner.join(options.ownerFederateName, options.federateType, federation);

  auto const objectClass = owner.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const attribute = owner.rtiAmbassador().getAttributeHandle(
      objectClass,
      options.attributeName);
  require(
      objectClass.isValid() && attribute.isValid(),
      "Timestamped object-deletion tombstone lookup returned an invalid handle");
  rti::AttributeHandleSet const attributes{attribute};
  owner.rtiAmbassador().publishObjectClassAttributes(objectClass, attributes);

  auto const objectName = federation + L"-terminal-deletion";
  owner.recorder().clearReservations();
  owner.rtiAmbassador().reserveObjectInstanceName(objectName);
  if (model == rti::HLA_EVOKED) {
    require(
        !owner.recorder().reservationSucceeded(objectName),
        "Timestamped object-deletion tombstone reservation callback arrived before servicing");
  }
  waitFor(
      owner,
      [&] { return owner.recorder().reservationSucceeded(objectName); },
      options,
      "timestamped object-deletion tombstone initial name reservation");

  auto const original = owner.rtiAmbassador().registerObjectInstance(
      objectClass,
      objectName);
  require(
      original.isValid(),
      "Timestamped object-deletion tombstone returned an invalid original object handle");

  auto ownerTime = makeTimeContext(owner);
  auto const lookaheadTarget = timeAfter(
      *ownerTime.factory,
      *ownerTime.initial,
      *ownerTime.epsilon,
      5U);
  auto lookahead = ownerTime.factory->makeZero();
  require(
      lookahead != nullptr,
      "Timestamped object-deletion tombstone could not allocate a lookahead");
  lookahead->setToDifference(*lookaheadTarget, *ownerTime.initial);
  owner.rtiAmbassador().enableTimeRegulation(*lookahead);
  waitFor(
      owner,
      [&] { return owner.recorder().timeRegulationEnabled().size() >= 1U; },
      options,
      "timestamped object-deletion tombstone time-regulation callback");

  auto const deletionTime = timeAfter(
      *ownerTime.factory,
      *ownerTime.initial,
      *ownerTime.epsilon,
      6U);
  auto const advanceTarget = timeAfter(
      *ownerTime.factory,
      *ownerTime.initial,
      *ownerTime.epsilon,
      1U);
  std::vector<std::uint8_t> const tagBytes{0x54U, 0x4FU, 0x4DU};
  rti::VariableLengthData tag(tagBytes.data(), tagBytes.size());
  auto const deletion = owner.rtiAmbassador().deleteObjectInstance(
      original,
      tag,
      *deletionTime);
  require(
      deletion.isValid(),
      "Timestamped object-deletion tombstone returned an invalid deletion handle");

  // The target is exactly current time plus the actual lookahead. The
  // timestamp is therefore no longer eligible for Retract under the strict
  // timestamp boundary, while the terminal deletion tombstone may reclaim
  // the named-instance mapping.
  owner.rtiAmbassador().timeAdvanceRequest(*advanceTarget);
  waitFor(
      owner,
      [&] { return owner.recorder().timeAdvanceGrants().size() >= 1U; },
      options,
      "timestamped object-deletion tombstone terminalizing grant");
  requireException(
      [&] { owner.rtiAmbassador().retract(deletion); },
      L"MessageCanNoLongerBeRetracted",
      "retracting a terminal timestamped object-deletion tombstone");

  owner.recorder().clearReservations();
  owner.rtiAmbassador().reserveObjectInstanceName(objectName);
  waitFor(
      owner,
      [&] { return owner.recorder().reservationSucceeded(objectName); },
      options,
      "timestamped object-deletion tombstone name reuse");
  auto const replacement = owner.rtiAmbassador().registerObjectInstance(
      objectClass,
      objectName);
  require(
      replacement.isValid(),
      "Timestamped object-deletion tombstone did not permit named re-registration");
  require(
      owner.rtiAmbassador().getObjectInstanceHandle(objectName) == replacement,
      "Timestamped object-deletion tombstone named registration did not round-trip");

  owner.resign(rti::CANCEL_THEN_DELETE_THEN_DIVEST);
  owner.rtiAmbassador().destroyFederationExecution(federation);
  owner.disconnect();
}

void scenarioTimestampedObjectDeletionNoFanout(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.logicalTimeImplementationName.empty(),
      "Timestamped object-deletion no-fanout testing requires an adapter-supplied logical-time implementation");

  Session owner(options, model, "owner");
  auto const federation = federationName(
      options,
      "timestamped-object-deletion-no-fanout");
  owner.connect();
  owner.rtiAmbassador().createFederationExecution(
      federation,
      options.fom.wstring(),
      options.logicalTimeImplementationName);
  owner.join(options.ownerFederateName, options.federateType, federation);

  auto const objectClass = owner.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const attribute = owner.rtiAmbassador().getAttributeHandle(
      objectClass,
      options.attributeName);
  require(
      objectClass.isValid() && attribute.isValid(),
      "Timestamped object-deletion no-fanout lookup returned an invalid handle");
  rti::AttributeHandleSet const attributes{attribute};
  owner.rtiAmbassador().publishObjectClassAttributes(objectClass, attributes);

  auto ownerTime = makeTimeContext(owner);
  auto const lookaheadTarget = timeAfter(
      *ownerTime.factory,
      *ownerTime.initial,
      *ownerTime.epsilon,
      5U);
  auto lookahead = ownerTime.factory->makeZero();
  require(
      lookahead != nullptr,
      "Timestamped object-deletion no-fanout could not allocate a lookahead");
  lookahead->setToDifference(*lookaheadTarget, *ownerTime.initial);
  owner.rtiAmbassador().enableTimeRegulation(*lookahead);
  waitFor(
      owner,
      [&] { return owner.recorder().timeRegulationEnabled().size() >= 1U; },
      options,
      "timestamped object-deletion no-fanout time-regulation callback");

  std::vector<std::uint8_t> const boundaryTagBytes{0x42U, 0x44U, 0x42U};
  rti::VariableLengthData boundaryTag(
      boundaryTagBytes.data(),
      boundaryTagBytes.size());
  auto const boundaryObject = owner.rtiAmbassador().registerObjectInstance(objectClass);
  require(
      boundaryObject.isValid(),
      "Timestamped object-deletion no-fanout boundary registration returned an invalid handle");
  auto const boundaryTime = timeAfter(
      *ownerTime.factory,
      *ownerTime.initial,
      *ownerTime.epsilon,
      5U);
  auto const boundaryRetraction = owner.rtiAmbassador().deleteObjectInstance(
      boundaryObject,
      boundaryTag,
      *boundaryTime);
  require(
      boundaryRetraction.isValid(),
      "Timestamped object-deletion no-fanout boundary deletion returned an invalid retraction handle");
  requireException(
      [&] { owner.rtiAmbassador().retract(boundaryRetraction); },
      L"MessageCanNoLongerBeRetracted",
      "retracting a timestamped object deletion at the exact lookahead boundary");

  auto const object = owner.rtiAmbassador().registerObjectInstance(objectClass);
  require(
      object.isValid(),
      "Timestamped object-deletion no-fanout registration returned an invalid object handle");
  auto const objectName = owner.rtiAmbassador().getObjectInstanceName(object);
  require(
      !objectName.empty(),
      "Timestamped object-deletion no-fanout registration returned an empty object name");
  auto const deletionTime = timeAfter(
      *ownerTime.factory,
      *ownerTime.initial,
      *ownerTime.epsilon,
      6U);
  std::vector<std::uint8_t> const tagBytes{0x4EU, 0x46U, 0x44U};
  rti::VariableLengthData tag(tagBytes.data(), tagBytes.size());
  auto const retraction = owner.rtiAmbassador().deleteObjectInstance(
      object,
      tag,
      *deletionTime);
  require(
      retraction.isValid(),
      "Timestamped object-deletion no-fanout returned an invalid retraction handle");
  requireException(
      [&] { static_cast<void>(owner.rtiAmbassador().getObjectInstanceHandle(objectName)); },
      L"ObjectInstanceNotKnown",
      "looking up an object after an unretracted no-fanout timestamped deletion");

  owner.rtiAmbassador().retract(retraction);
  require(
      owner.rtiAmbassador().getObjectInstanceHandle(objectName) == object,
      "retracting a no-fanout timestamped deletion did not restore the object name mapping");
  require(
      owner.rtiAmbassador().isAttributeOwnedByFederate(object, attribute),
      "retracting a no-fanout timestamped deletion did not restore local attribute ownership");
  require(
      owner.recorder().retractions().empty(),
      "timestamped object-deletion no-fanout unexpectedly delivered a request-retraction callback");
  requireException(
      [&] { owner.rtiAmbassador().retract(retraction); },
      L"MessageCanNoLongerBeRetracted",
      "retracting an already retracted no-fanout timestamped object deletion");

  owner.resign(rti::CANCEL_THEN_DELETE_THEN_DIVEST);
  owner.rtiAmbassador().destroyFederationExecution(federation);
  owner.disconnect();
}

void scenarioTimestampedObjectDeletionRegulationReenable(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.logicalTimeImplementationName.empty(),
      "Timestamped object-deletion regulation re-enable requires an adapter-supplied logical-time implementation");

  Session publisher(options, model, "owner");
  Session receiver(options, model, "constrained");
  auto const federation = federationName(
      options,
      "timestamped-object-deletion-regulation-reenable");
  connectAndJoin(publisher, receiver, options, federation, options.fom);

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
  require(
      publisherClass.isValid() && receiverClass.isValid() &&
          publisherAttribute.isValid() && receiverAttribute.isValid(),
      "timestamped object-deletion regulation re-enable lookup returned an invalid handle");
  rti::AttributeHandleSet const publisherAttributes{publisherAttribute};
  rti::AttributeHandleSet const receiverAttributes{receiverAttribute};
  publisher.rtiAmbassador().publishObjectClassAttributes(
      publisherClass,
      publisherAttributes);
  receiver.rtiAmbassador().subscribeObjectClassAttributes(
      receiverClass,
      receiverAttributes,
      true,
      L"");

  auto const object = publisher.rtiAmbassador().registerObjectInstance(publisherClass);
  require(
      object.isValid(),
      "timestamped object-deletion regulation re-enable registration returned an invalid object handle");
  auto const objectName = publisher.rtiAmbassador().getObjectInstanceName(object);
  require(
      !objectName.empty(),
      "timestamped object-deletion regulation re-enable registration returned an empty name");
  waitFor(
      receiver,
      [&] { return receiver.recorder().hasDiscovery(object); },
      options,
      "timestamped object-deletion regulation re-enable discovery");
  require(
      receiver.rtiAmbassador().getObjectInstanceHandle(objectName) == object,
      "timestamped object-deletion regulation re-enable discovery changed object identity");

  auto publisherTime = makeTimeContext(publisher);
  auto receiverTime = makeTimeContext(receiver);
  require(
      publisherTime.factory->getName() == receiverTime.factory->getName(),
      "timestamped object-deletion regulation re-enable members selected different logical-time factories");
  enableTimestampedRoles(
      publisher,
      receiver,
      publisherTime,
      receiverTime,
      options,
      "timestamped object-deletion regulation re-enable");

  auto const deletionTime = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      5U);
  auto const publisherTarget = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      2U);
  auto const changedLookaheadTime = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      3U);
  auto changedLookahead = publisherTime.factory->makeZero();
  require(
      changedLookahead != nullptr,
      "timestamped object-deletion regulation re-enable could not allocate the changed lookahead");
  changedLookahead->setToDifference(
      *changedLookaheadTime,
      *publisherTime.initial);

  std::vector<std::uint8_t> tagBytes{0x43U, 0x48U, 0x47U, 0x2DU, 0x4CU, 0x41U};
  rti::VariableLengthData tag(tagBytes.data(), tagBytes.size());
  auto const retraction = publisher.rtiAmbassador().deleteObjectInstance(
      object,
      tag,
      *deletionTime);
  require(
      retraction.isValid(),
      "timestamped object-deletion regulation re-enable delete returned an invalid retraction handle");
  require(
      receiver.recorder().timedRemovals().empty(),
      "timestamped object-deletion regulation re-enable delivered before its grant");
  require(
      receiver.rtiAmbassador().getObjectInstanceHandle(objectName) == object,
      "timestamped object-deletion regulation re-enable changed identity before delivery");

  publisher.rtiAmbassador().disableTimeRegulation();
  publisher.rtiAmbassador().enableTimeRegulation(*changedLookahead);
  waitFor(
      publisher,
      [&] { return publisher.recorder().timeRegulationEnabled().size() >= 2U; },
      options,
      "timestamped object-deletion regulation re-enable callback");
  auto queriedLookahead = publisherTime.factory->makeZero();
  require(
      queriedLookahead != nullptr,
      "timestamped object-deletion regulation re-enable could not allocate a lookahead query");
  publisher.rtiAmbassador().queryLookahead(*queriedLookahead);
  require(
      *queriedLookahead == *changedLookahead,
      "timestamped object-deletion regulation re-enable did not expose the changed lookahead");

  receiver.recorder().clearCallbackOrder();
  publisher.recorder().clearCallbackOrder();
  receiver.rtiAmbassador().timeAdvanceRequest(*deletionTime);
  require(
      receiver.recorder().timedRemovals().empty() &&
          receiver.recorder().timeAdvanceGrants().empty(),
      "timestamped object-deletion regulation re-enable delivered before the producer boundary");
  publisher.rtiAmbassador().timeAdvanceRequest(*publisherTarget);
  waitFor(
      publisher,
      receiver,
      [&] {
        return publisher.recorder().timeAdvanceGrants().size() >= 1U &&
            receiver.recorder().timeAdvanceGrants().size() >= 1U &&
            receiver.recorder().timedRemovals().size() >= 1U;
      },
      options,
      "timestamped object-deletion regulation re-enable delivery");
  auto const removals = receiver.recorder().timedRemovals();
  require(
      removals.size() == 1U &&
          publisher.recorder().timeRegulationEnabled().size() == 2U,
      "timestamped object-deletion regulation re-enable returned the wrong callback counts");
  require(
      receiver.recorder().callbackOrder() == std::vector<std::string>{"remove", "grant"},
      "timestamped object-deletion regulation re-enable delivered after its grant");
  auto const& removal = removals.front();
  require(
      removal.object == object && removal.tag == tagBytes &&
          removal.producer == publisher.federateHandle(),
      "timestamped object-deletion regulation re-enable returned the wrong removal identity or metadata");
  require(
      removal.time == encodeTime(*deletionTime) && !removal.timeText.empty() &&
          removal.sentOrder == rti::TIMESTAMP &&
          removal.receivedOrder == rti::TIMESTAMP,
      "timestamped object-deletion regulation re-enable returned the wrong time or order metadata");
  require(
      removal.retractionPresent && removal.retraction == copyBytes(retraction.encode()),
      "timestamped object-deletion regulation re-enable omitted the retraction handle");
  require(
      publisher.recorder().timeAdvanceGrants().front().encoded == encodeTime(*publisherTarget),
      "timestamped object-deletion regulation re-enable returned the wrong producer grant");
  require(
      receiver.recorder().timeAdvanceGrants().front().encoded == encodeTime(*deletionTime),
      "timestamped object-deletion regulation re-enable returned the wrong receiver grant");
  requireException(
      [&] { receiver.rtiAmbassador().getObjectInstanceHandle(objectName); },
      L"ObjectInstanceNotKnown",
      "timestamped object-deletion regulation re-enable retained the removed receiver object");
  requireException(
      [&] { publisher.rtiAmbassador().getObjectInstanceName(object); },
      L"ObjectInstanceNotKnown",
      "timestamped object-deletion regulation re-enable retained the removed publisher object");
  requireException(
      [&] { publisher.rtiAmbassador().retract(retraction); },
      L"MessageCanNoLongerBeRetracted",
      "timestamped object-deletion regulation re-enable retraction after delivery");

  receiver.rtiAmbassador().disableTimeConstrained();
  publisher.rtiAmbassador().disableTimeRegulation();
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
