#include "tck_main_scenarios.hpp"
#include "tck_timestamped_interaction_boundary_scenarios.hpp"

namespace hla_rti_cpp_tck::detail {

enum class TimestampedInteractionBoundaryMode {
  tara_strictly_before,
  tara_equal,
  tar_equal,
};

void runTimestampedInteractionMixedAdvancesScenario(
    Options const& options,
    rti::CallbackModel model,
    TimestampedInteractionBoundaryMode boundaryMode) {
  bool const equalTaraBoundary =
      boundaryMode == TimestampedInteractionBoundaryMode::tara_equal;
  bool const equalTarBoundary =
      boundaryMode == TimestampedInteractionBoundaryMode::tar_equal;
  require(
      !options.logicalTimeImplementationName.empty(),
      "Timestamped interaction mixed advances requires an adapter-supplied logical-time implementation");

  Session publisher(options, model, "owner");
  Session flushQueue(options, model, "member");
  Session timeAdvanceAvailable(options, model, "member");
  Session nextMessageAvailable(options, model, "member");
  auto const federation = federationName(
      options,
      equalTarBoundary
          ? "timestamped-interaction-tar-equal-boundary"
          : equalTaraBoundary
              ? "timestamped-interaction-tara-equal-boundary"
              : "timestamped-interaction-mixed-advances");

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
      options.memberFederateName + L"-interaction-flush-queue",
      options.federateType,
      federation);
  timeAdvanceAvailable.join(
      options.memberFederateName +
          (equalTarBoundary ? L"-interaction-tar" : L"-interaction-time-advance-available"),
      options.federateType,
      federation);
  nextMessageAvailable.join(
      options.memberFederateName + L"-interaction-next-message-available",
      options.federateType,
      federation);

  auto const publisherInteraction = publisher.rtiAmbassador().getInteractionClassHandle(
      options.interactionClassName);
  auto const publisherParameter = publisher.rtiAmbassador().getParameterHandle(
      publisherInteraction,
      options.parameterName);
  require(
      publisherInteraction.isValid() && publisherParameter.isValid(),
      "Timestamped interaction mixed-advance publisher lookup returned an invalid handle");

  publisher.rtiAmbassador().publishInteractionClass(publisherInteraction);
  publisher.rtiAmbassador().changeInteractionOrderType(
      publisherInteraction,
      rti::TIMESTAMP);

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
        options.interactionClassName);
    auto const parameter = member->rtiAmbassador().getParameterHandle(
        interaction,
        options.parameterName);
    require(
        interaction.isValid() && parameter.isValid(),
        "Timestamped interaction mixed-advance member lookup returned an invalid handle");
    member->rtiAmbassador().subscribeInteractionClass(interaction, true);
    memberInteractions.push_back(interaction);
    memberParameters.push_back(parameter);
  }

  auto publisherTime = makeTimeContext(publisher);
  auto flushQueueTime = makeTimeContext(flushQueue);
  auto timeAdvanceAvailableTime = makeTimeContext(timeAdvanceAvailable);
  auto nextMessageAvailableTime = makeTimeContext(nextMessageAvailable);
  require(
      publisherTime.factory->getName() == flushQueueTime.factory->getName() &&
          publisherTime.factory->getName() == timeAdvanceAvailableTime.factory->getName() &&
          publisherTime.factory->getName() == nextMessageAvailableTime.factory->getName(),
      "Timestamped interaction mixed-advance members selected different logical-time factories");

  for (auto* member : members) {
    member->rtiAmbassador().enableTimeConstrained();
    waitFor(
        *member,
        [&] { return member->recorder().timeConstrainedEnabled().size() >= 1U; },
        options,
        "Timestamped interaction mixed-advance time-constrained callback");
  }
  publisher.rtiAmbassador().enableTimeRegulation(*publisherTime.epsilon);
  waitFor(
      publisher,
      [&] { return publisher.recorder().timeRegulationEnabled().size() >= 1U; },
      options,
      "Timestamped interaction mixed-advance time-regulation callback");

  auto const messageTime = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      5U);
  auto const timeAdvanceAvailableBoundary = timeAfter(
      *timeAdvanceAvailableTime.factory,
      *timeAdvanceAvailableTime.initial,
      *timeAdvanceAvailableTime.epsilon,
      equalTaraBoundary || equalTarBoundary ? 5U : 6U);
  auto const timeAdvanceAvailableFollowupBoundary = timeAfter(
      *timeAdvanceAvailableTime.factory,
      *timeAdvanceAvailableTime.initial,
      *timeAdvanceAvailableTime.epsilon,
      6U);
  // TAR(5) must be below the regulator's GALT before the equal-time message
  // can be granted. Advance the TAR-equal publisher to 5 (lookahead 1 =>
  // GALT 6); only TARA-equal first grants at 5 before the message exists.
  auto const publisherTarget = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      equalTaraBoundary ? 4U : 5U);
  auto const requestBoundary = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      10U);
  std::vector<std::uint8_t> const parameterBytes{
      0x49U, 0x4EU, 0x54U, 0x2DU, 0x41U};
  std::vector<std::uint8_t> const tagBytes{
      0x49U, 0x4EU, 0x54U, 0x2DU, 0x46U};
  rti::ParameterHandleValueMap parameters;
  parameters.emplace(
      publisherParameter,
      rti::VariableLengthData(parameterBytes.data(), parameterBytes.size()));
  rti::VariableLengthData tag(tagBytes.data(), tagBytes.size());
  if (equalTaraBoundary) {
    publisher.rtiAmbassador().timeAdvanceRequest(*publisherTarget);
    waitFor(
        publisher,
        [&] { return publisher.recorder().timeAdvanceGrants().size() == 1U; },
        options,
        "Timestamped interaction TARA-equal publisher pre-message TAR");
    require(
        publisher.recorder().timeAdvanceGrants().front().encoded ==
            encodeTime(*publisherTarget),
        "Timestamped interaction TARA-equal publisher received the wrong pre-message grant");
    timeAdvanceAvailable.rtiAmbassador().timeAdvanceRequestAvailable(
        *timeAdvanceAvailableBoundary);
    waitFor(
        timeAdvanceAvailable,
        [&] { return timeAdvanceAvailable.recorder().timeAdvanceGrants().size() == 1U; },
        options,
        "Timestamped interaction equal-time TARA grant before message arrival");
    require(
        timeAdvanceAvailable.recorder().timeAdvanceGrants().front().encoded ==
                encodeTime(*timeAdvanceAvailableBoundary) &&
            timeAdvanceAvailable.recorder().timedInteractions().empty() &&
            timeAdvanceAvailable.recorder().callbackOrder() ==
                std::vector<std::string>{"grant"},
        "Timestamped interaction TARA-equal did not grant before the equal-time message existed");
  }
  auto const retraction = publisher.rtiAmbassador().sendInteraction(
      publisherInteraction,
      parameters,
      tag,
      *messageTime);
  require(
      retraction.isValid(),
      "Timestamped interaction mixed-advance send returned an invalid retraction handle");
  for (auto* member : members) {
    if (!equalTaraBoundary || member != &timeAdvanceAvailable) {
      require(
          member->recorder().timedInteractions().empty(),
          "Timestamped interaction mixed-advance delivered before its request");
      member->recorder().clearCallbackOrder();
    }
  }
  publisher.recorder().clearCallbackOrder();

  if (equalTaraBoundary) {
    publisher.rtiAmbassador().timeAdvanceRequest(*messageTime);
    waitFor(
        publisher,
        [&] { return publisher.recorder().timeAdvanceGrants().size() == 2U; },
        options,
        "Timestamped interaction TARA-equal publisher post-message TAR");
    require(
        publisher.recorder().timeAdvanceGrants().at(1U).encoded == encodeTime(*messageTime),
        "Timestamped interaction TARA-equal publisher received the wrong post-message grant");
    timeAdvanceAvailable.rtiAmbassador().timeAdvanceRequestAvailable(
        *timeAdvanceAvailableFollowupBoundary);
  }

  flushQueue.rtiAmbassador().flushQueueRequest(*requestBoundary);
  if (equalTarBoundary) {
    timeAdvanceAvailable.rtiAmbassador().timeAdvanceRequest(
        *timeAdvanceAvailableBoundary);
  } else if (!equalTaraBoundary) {
    timeAdvanceAvailable.rtiAmbassador().timeAdvanceRequestAvailable(
        *timeAdvanceAvailableBoundary);
  }
  nextMessageAvailable.rtiAmbassador().nextMessageRequestAvailable(*requestBoundary);
  if (!equalTaraBoundary) {
    publisher.rtiAmbassador().timeAdvanceRequest(*publisherTarget);
  }
  try {
    waitForSessions(
        {&publisher, &flushQueue, &timeAdvanceAvailable, &nextMessageAvailable},
        [&] {
          return publisher.recorder().timeAdvanceGrants().size() >=
                  (equalTaraBoundary ? 2U : 1U) &&
              flushQueue.recorder().timedInteractions().size() >= 1U &&
              flushQueue.recorder().flushQueueGrants().size() >= 1U &&
              timeAdvanceAvailable.recorder().timedInteractions().size() >= 1U &&
              timeAdvanceAvailable.recorder().timeAdvanceGrants().size() >=
                  (equalTaraBoundary ? 2U : 1U) &&
              nextMessageAvailable.recorder().timedInteractions().size() >= 1U &&
              nextMessageAvailable.recorder().timeAdvanceGrants().size() >= 1U;
        },
        options,
        "Timestamped interaction mixed-advance deliveries and grants");
  } catch (std::runtime_error const& error) {
    throw std::runtime_error(
        std::string(error.what()) + "; observed counts: publisher TAG=" +
        std::to_string(publisher.recorder().timeAdvanceGrants().size()) +
        ", FQR TSO/FQG=" +
        std::to_string(flushQueue.recorder().timedInteractions().size()) + "/" +
        std::to_string(flushQueue.recorder().flushQueueGrants().size()) +
        ", TARA TSO/TAG=" +
        std::to_string(timeAdvanceAvailable.recorder().timedInteractions().size()) +
        "/" +
        std::to_string(timeAdvanceAvailable.recorder().timeAdvanceGrants().size()) +
        ", NMRA TSO/TAG=" +
        std::to_string(nextMessageAvailable.recorder().timedInteractions().size()) +
        "/" +
        std::to_string(nextMessageAvailable.recorder().timeAdvanceGrants().size()));
  }

  require(
      publisher.recorder().timeAdvanceGrants().size() ==
          (equalTaraBoundary ? 2U : 1U),
      "Timestamped interaction mixed-advance publisher received the wrong grant count");
  require(
      flushQueue.recorder().timedInteractions().size() == 1U &&
          flushQueue.recorder().flushQueueGrants().size() == 1U &&
          flushQueue.recorder().timeAdvanceGrants().empty(),
      "Timestamped interaction Flush Queue Request returned the wrong callback pair");
  require(
      timeAdvanceAvailable.recorder().timedInteractions().size() == 1U &&
          timeAdvanceAvailable.recorder().timeAdvanceGrants().size() ==
              (equalTaraBoundary ? 2U : 1U) &&
          timeAdvanceAvailable.recorder().flushQueueGrants().empty(),
      "Timestamped interaction Time Advance Request Available returned the wrong callback pair");
  require(
      nextMessageAvailable.recorder().timedInteractions().size() == 1U &&
          nextMessageAvailable.recorder().timeAdvanceGrants().size() == 1U &&
          nextMessageAvailable.recorder().flushQueueGrants().empty(),
      "Timestamped interaction Next Message Request Available returned the wrong callback pair");
  require(
      flushQueue.recorder().callbackOrder() ==
          std::vector<std::string>{"interaction", "flush-grant"},
      "Timestamped interaction Flush Queue Request granted before delivery");
  auto const expectedTimeAdvanceAvailableOrder = equalTaraBoundary
      ? std::vector<std::string>{"grant", "interaction", "grant"}
      : std::vector<std::string>{"interaction", "grant"};
  require(
      timeAdvanceAvailable.recorder().callbackOrder() ==
          expectedTimeAdvanceAvailableOrder,
      equalTaraBoundary
          ? "Later equal-time interaction did not follow the Time Advance Request Available grant"
          : equalTarBoundary
              ? "Timestamped interaction at the Time Advance Request boundary was delivered after the grant"
              : "Timestamped interaction before the Time Advance Request Available boundary was delivered after the grant");
  require(
      nextMessageAvailable.recorder().callbackOrder() ==
          std::vector<std::string>{"interaction", "grant"},
      "Timestamped interaction Next Message Request Available granted before delivery");

  auto assertInteraction = [&](TimedInteractionRecord const& record,
                               rti::InteractionClassHandle const& expectedInteraction,
                               rti::ParameterHandle const& expectedParameter,
                               Session& inspector) {
    require(
        record.interaction == expectedInteraction &&
            record.parameters.size() == 1U &&
            record.parameters.count(expectedParameter) == 1U,
        "Timestamped interaction mixed-advance callback returned the wrong class or parameter set");
    require(
        copyBytes(record.parameters.at(expectedParameter)) == parameterBytes &&
            record.tag == tagBytes,
        "Timestamped interaction mixed-advance callback did not preserve payload or tag");
    require(
        record.producer == publisher.federateHandle() &&
            record.time == encodeTime(*messageTime) &&
            !record.timeText.empty(),
        "Timestamped interaction mixed-advance callback returned the wrong producer or time");
    require(
        record.transportation.isValid() &&
            !inspector.rtiAmbassador().getTransportationTypeName(record.transportation).empty(),
        "Timestamped interaction mixed-advance callback returned an unresolvable transport");
    require(
        record.sentOrder == rti::TIMESTAMP && record.receivedOrder == rti::TIMESTAMP,
        "Timestamped interaction mixed-advance callback returned the wrong order metadata");
    require(
        !record.regions.has_value(),
        "Timestamped interaction mixed-advance callback unexpectedly supplied regions");
    require(
        record.retractionPresent && record.retraction == copyBytes(retraction.encode()),
        "Timestamped interaction mixed-advance callback returned the wrong retraction handle");
  };
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

  require(
      publisher.recorder().timeAdvanceGrants().front().encoded ==
          encodeTime(*publisherTarget) &&
          (!equalTaraBoundary ||
              publisher.recorder().timeAdvanceGrants().at(1U).encoded ==
                  encodeTime(*messageTime)),
      "Timestamped interaction mixed-advance publisher received the wrong grant time");
  auto const timeAdvanceAvailableGrants =
      timeAdvanceAvailable.recorder().timeAdvanceGrants();
  require(
      timeAdvanceAvailableGrants.front().encoded == encodeTime(*messageTime),
      "Timestamped interaction TARA returned the wrong first grant time");
  if (equalTaraBoundary) {
    std::string actualFollowupGrantEncoding;
    for (auto const octet : timeAdvanceAvailableGrants.at(1U).encoded) {
      if (!actualFollowupGrantEncoding.empty()) {
        actualFollowupGrantEncoding += ",";
      }
      actualFollowupGrantEncoding +=
          std::to_string(static_cast<unsigned int>(octet));
    }
    require(
        timeAdvanceAvailableGrants.at(1U).encoded ==
            encodeTime(*messageTime),
        "Timestamped interaction TARA follow-up grant did not stop at the late "
        "interaction time; encoded octets were " +
            actualFollowupGrantEncoding);
  }
  require(
      nextMessageAvailable.recorder().timeAdvanceGrants().front().encoded ==
          encodeTime(*messageTime),
      "Timestamped interaction NMRA returned the wrong grant time");

  auto const flushGrants = flushQueue.recorder().flushQueueGrants();
  require(
      flushGrants.front().optimisticTime == encodeTime(*messageTime) &&
          !flushGrants.front().time.empty() &&
          !flushGrants.front().timeText.empty() &&
          !flushGrants.front().optimisticTimeText.empty(),
      "Timestamped interaction mixed-advance Flush Queue Grant returned incomplete time metadata");
  auto actualFlushTime = decodeTime(*flushQueueTime.factory, flushGrants.front().time);
  auto optimisticFlushTime =
      decodeTime(*flushQueueTime.factory, flushGrants.front().optimisticTime);
  require(
      actualFlushTime != nullptr && optimisticFlushTime != nullptr &&
          !(*actualFlushTime < *flushQueueTime.initial) &&
          !(*requestBoundary < *actualFlushTime) &&
          !(*optimisticFlushTime < *actualFlushTime),
      "Timestamped interaction mixed-advance Flush Queue Grant returned time outside bounds");
  auto queriedFlushQueue = flushQueueTime.factory->makeInitial();
  flushQueue.rtiAmbassador().queryLogicalTime(*queriedFlushQueue);
  require(
      encodeTime(*queriedFlushQueue) == flushGrants.front().time,
      "Timestamped interaction mixed-advance Flush Queue query did not match its grant");
  auto queriedTimeAdvanceAvailable = timeAdvanceAvailableTime.factory->makeInitial();
  timeAdvanceAvailable.rtiAmbassador().queryLogicalTime(*queriedTimeAdvanceAvailable);
  requireTimeEquals(
      *queriedTimeAdvanceAvailable,
      *messageTime,
      "Timestamped interaction mixed-advance Time Advance Request Available query");
  auto queriedNextMessageAvailable = nextMessageAvailableTime.factory->makeInitial();
  nextMessageAvailable.rtiAmbassador().queryLogicalTime(*queriedNextMessageAvailable);
  requireTimeEquals(
      *queriedNextMessageAvailable,
      *messageTime,
      "Timestamped interaction mixed-advance Next Message Request Available query");

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

void scenarioTimestampedInteractionMixedAdvances(
    Options const& options,
    rti::CallbackModel model) {
  runTimestampedInteractionMixedAdvancesScenario(
      options, model, TimestampedInteractionBoundaryMode::tara_strictly_before);
}

void scenarioTimestampedInteractionTaraEqualBoundary(
    Options const& options,
    rti::CallbackModel model) {
  runTimestampedInteractionMixedAdvancesScenario(
      options, model, TimestampedInteractionBoundaryMode::tara_equal);
}

namespace {

void runTimestampedInteractionTaraQueuedEqualBoundary(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.logicalTimeImplementationName.empty(),
      "TARA queued equal-boundary case requires an adapter-supplied logical-time implementation");

  Session publisher(options, model, "tara-queued-equal-publisher");
  Session receiver(options, model, "tara-queued-equal-receiver");
  auto const federation = federationName(
      options, "timestamped-interaction-tara-queued-equal-boundary");
  connectAndJoin(publisher, receiver, options, federation, options.fom);

  auto const publisherInteraction =
      publisher.rtiAmbassador().getInteractionClassHandle(
          options.interactionClassName);
  auto const receiverInteraction =
      receiver.rtiAmbassador().getInteractionClassHandle(
          options.interactionClassName);
  auto const publisherParameter = publisher.rtiAmbassador().getParameterHandle(
      publisherInteraction,
      options.parameterName);
  auto const receiverParameter = receiver.rtiAmbassador().getParameterHandle(
      receiverInteraction,
      options.parameterName);
  require(
      publisherInteraction.isValid() && receiverInteraction.isValid() &&
          publisherParameter.isValid() && receiverParameter.isValid() &&
          publisherInteraction == receiverInteraction &&
          publisherParameter == receiverParameter,
      "TARA queued equal-boundary lookup returned inconsistent standard handles");

  publisher.rtiAmbassador().publishInteractionClass(publisherInteraction);
  publisher.rtiAmbassador().changeInteractionOrderType(
      publisherInteraction,
      rti::TIMESTAMP);
  receiver.rtiAmbassador().subscribeInteractionClass(receiverInteraction, true);

  auto const publisherTime = makeTimeContext(publisher);
  auto const receiverTime = makeTimeContext(receiver);
  require(
      publisherTime.factory->getName() == receiverTime.factory->getName(),
      "TARA queued equal-boundary federates selected different logical-time factories");
  requireTimeEquals(
      *publisherTime.initial,
      *receiverTime.initial,
      "TARA queued equal-boundary initial time");

  receiver.rtiAmbassador().enableTimeConstrained();
  waitFor(
      receiver,
      [&] { return receiver.recorder().timeConstrainedEnabled().size() == 1U; },
      options,
      "TARA queued equal-boundary constrained callback");
  publisher.rtiAmbassador().enableTimeRegulation(*publisherTime.epsilon);
  waitFor(
      publisher,
      [&] { return publisher.recorder().timeRegulationEnabled().size() == 1U; },
      options,
      "TARA queued equal-boundary regulation callback");

  auto const messageTime = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      2U);
  std::vector<std::uint8_t> const payloadBytes{0x54U, 0x41U, 0x52U, 0x41U};
  std::vector<std::uint8_t> const tagBytes{0x51U, 0x45U};
  rti::ParameterHandleValueMap parameters;
  parameters.emplace(
      publisherParameter,
      rti::VariableLengthData(payloadBytes.data(), payloadBytes.size()));
  rti::VariableLengthData tag(tagBytes.data(), tagBytes.size());
  auto const retraction = publisher.rtiAmbassador().sendInteraction(
      publisherInteraction,
      parameters,
      tag,
      *messageTime);
  require(
      retraction.isValid(),
      "TARA queued equal-boundary send returned an invalid retraction handle");
  require(
      receiver.recorder().timedInteractions().empty() &&
          receiver.recorder().timeAdvanceGrants().empty(),
      "TARA queued equal-boundary message was delivered before its request");

  receiver.rtiAmbassador().timeAdvanceRequestAvailable(*messageTime);
  publisher.rtiAmbassador().timeAdvanceRequest(*messageTime);
  waitForSessions(
      {&publisher, &receiver},
      [&] {
        return publisher.recorder().timeAdvanceGrants().size() == 1U &&
            receiver.recorder().timeAdvanceGrants().size() == 1U;
      },
      options,
      "TARA queued equal-boundary grants");

  auto const interactions = receiver.recorder().timedInteractions();
  require(
      interactions.size() == 1U &&
          receiver.recorder().callbackOrder() ==
              std::vector<std::string>{"interaction", "grant"},
      "TARA did not deliver its queued equal-time TSO before the grant");
  auto const& interaction = interactions.front();
  require(
      interaction.interaction == receiverInteraction &&
          interaction.parameters.size() == 1U &&
          interaction.parameters.count(receiverParameter) == 1U &&
          interaction.time == encodeTime(*messageTime) &&
          interaction.sentOrder == rti::TIMESTAMP &&
          interaction.receivedOrder == rti::TIMESTAMP &&
          interaction.tag == tagBytes && interaction.retractionPresent &&
          interaction.retraction == copyBytes(retraction.encode()),
      "TARA queued equal-time callback metadata did not match the sent interaction");
  require(
      copyBytes(interaction.parameters.at(receiverParameter)) == payloadBytes,
      "TARA queued equal-time callback changed the interaction payload");
  require(
      receiver.recorder().timeAdvanceGrants().front().encoded ==
          encodeTime(*messageTime) &&
          publisher.recorder().timeAdvanceGrants().front().encoded ==
              encodeTime(*messageTime),
      "TARA queued equal-boundary participants received the wrong grant time");

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

void runTimestampedInteractionZeroLookaheadSendAtGrant(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.logicalTimeImplementationName.empty(),
      "zero-lookahead post-grant send requires an adapter-supplied logical-time implementation");

  Session publisher(options, model, "zero-lookahead-post-grant-publisher");
  Session receiver(options, model, "zero-lookahead-post-grant-receiver");
  auto const federation = federationName(
      options, "timestamped-interaction-zero-lookahead-send-at-grant");
  connectAndJoin(publisher, receiver, options, federation, options.fom);

  auto const publisherInteraction =
      publisher.rtiAmbassador().getInteractionClassHandle(
          options.interactionClassName);
  auto const receiverInteraction =
      receiver.rtiAmbassador().getInteractionClassHandle(
          options.interactionClassName);
  auto const publisherParameter = publisher.rtiAmbassador().getParameterHandle(
      publisherInteraction,
      options.parameterName);
  auto const receiverParameter = receiver.rtiAmbassador().getParameterHandle(
      receiverInteraction,
      options.parameterName);
  require(
      publisherInteraction.isValid() && receiverInteraction.isValid() &&
          publisherParameter.isValid() && receiverParameter.isValid() &&
          publisherInteraction == receiverInteraction &&
          publisherParameter == receiverParameter,
      "zero-lookahead post-grant lookup returned inconsistent standard handles");

  publisher.rtiAmbassador().publishInteractionClass(publisherInteraction);
  publisher.rtiAmbassador().changeInteractionOrderType(
      publisherInteraction,
      rti::TIMESTAMP);
  receiver.rtiAmbassador().subscribeInteractionClass(receiverInteraction, true);

  auto const publisherTime = makeTimeContext(publisher);
  auto const receiverTime = makeTimeContext(receiver);
  require(
      publisherTime.factory->getName() == receiverTime.factory->getName(),
      "zero-lookahead post-grant federates selected different logical-time factories");
  requireTimeEquals(
      *publisherTime.initial,
      *receiverTime.initial,
      "zero-lookahead post-grant initial time");

  receiver.rtiAmbassador().enableTimeConstrained();
  waitFor(
      receiver,
      [&] { return receiver.recorder().timeConstrainedEnabled().size() == 1U; },
      options,
      "zero-lookahead post-grant constrained callback");
  publisher.rtiAmbassador().enableTimeRegulation(*publisherTime.zero);
  waitFor(
      publisher,
      [&] { return publisher.recorder().timeRegulationEnabled().size() == 1U; },
      options,
      "zero-lookahead post-grant regulation callback");
  auto actualLookahead = publisherTime.factory->makeZero();
  require(
      actualLookahead != nullptr,
      "zero-lookahead post-grant case could not allocate Query Lookahead output");
  publisher.rtiAmbassador().queryLookahead(*actualLookahead);
  require(
      *actualLookahead == *publisherTime.zero,
      "the regulating federate did not retain zero actual lookahead");

  auto const grantedTime = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      5U);
  auto const nextTime = timeAfter(
      *publisherTime.factory,
      *grantedTime,
      *publisherTime.epsilon,
      1U);
  publisher.rtiAmbassador().timeAdvanceRequest(*grantedTime);
  waitFor(
      publisher,
      [&] { return publisher.recorder().timeAdvanceGrants().size() == 1U; },
      options,
      "zero-lookahead publisher's initial grant");
  require(
      publisher.recorder().timeAdvanceGrants().front().encoded ==
          encodeTime(*grantedTime),
      "zero-lookahead publisher received the wrong initial grant");

  std::vector<std::uint8_t> const payloadBytes{0xB5U, 0x05U};
  std::vector<std::uint8_t> const tagBytes{0xB5U, 0x06U};
  rti::ParameterHandleValueMap parameters;
  parameters.emplace(
      publisherParameter,
      rti::VariableLengthData(payloadBytes.data(), payloadBytes.size()));
  rti::VariableLengthData tag(tagBytes.data(), tagBytes.size());
  auto const retraction = publisher.rtiAmbassador().sendInteraction(
      publisherInteraction,
      parameters,
      tag,
      *grantedTime);
  require(
      retraction.isValid(),
      "zero-lookahead post-grant send at the granted logical time was rejected");
  require(
      receiver.recorder().timedInteractions().empty(),
      "zero-lookahead TSO was delivered before the receiver requested its time");

  publisher.rtiAmbassador().timeAdvanceRequest(*nextTime);
  waitFor(
      publisher,
      [&] { return publisher.recorder().timeAdvanceGrants().size() == 2U; },
      options,
      "zero-lookahead publisher's forward grant");

  receiver.rtiAmbassador().timeAdvanceRequest(*grantedTime);
  waitFor(
      receiver,
      [&] { return receiver.recorder().timeAdvanceGrants().size() == 1U; },
      options,
      "zero-lookahead receiver's matching grant");
  auto const interactions = receiver.recorder().timedInteractions();
  require(
      interactions.size() == 1U &&
          receiver.recorder().callbackOrder() ==
              std::vector<std::string>{"interaction", "grant"},
      "zero-lookahead post-grant TSO was not observed before the receiver grant");
  auto const& interaction = interactions.front();
  require(
      interaction.interaction == receiverInteraction &&
          interaction.parameters.size() == 1U &&
          interaction.parameters.count(receiverParameter) == 1U &&
          interaction.time == encodeTime(*grantedTime) &&
          interaction.sentOrder == rti::TIMESTAMP &&
          interaction.receivedOrder == rti::TIMESTAMP &&
          interaction.tag == tagBytes && interaction.retractionPresent &&
          interaction.retraction == copyBytes(retraction.encode()),
      "zero-lookahead post-grant callback metadata did not match the sent interaction");
  require(
      copyBytes(interaction.parameters.at(receiverParameter)) == payloadBytes,
      "zero-lookahead post-grant callback changed the interaction payload");
  require(
      receiver.recorder().timeAdvanceGrants().front().encoded ==
          encodeTime(*grantedTime),
      "zero-lookahead receiver received the wrong grant time");

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

void runTimestampedInteractionZeroLookaheadTarSendBoundary(
    Options const& options,
    rti::CallbackModel model) {
  require(
      model == rti::HLA_EVOKED,
      "zero-lookahead TAR send-boundary scenario requires HLA_EVOKED to observe the Time Advancing state before servicing its grant");
  require(
      !options.logicalTimeImplementationName.empty(),
      "zero-lookahead TAR send-boundary scenario requires an adapter-supplied logical-time implementation");

  Session publisher(options, model, "zero-lookahead-tar-boundary-publisher");
  Session receiver(options, model, "zero-lookahead-tar-boundary-receiver");
  auto const federation = federationName(
      options, "timestamped-interaction-zero-lookahead-tar-send-boundary");
  connectAndJoin(publisher, receiver, options, federation, options.fom);

  auto const publisherInteraction =
      publisher.rtiAmbassador().getInteractionClassHandle(
          options.interactionClassName);
  auto const receiverInteraction =
      receiver.rtiAmbassador().getInteractionClassHandle(
          options.interactionClassName);
  auto const publisherParameter = publisher.rtiAmbassador().getParameterHandle(
      publisherInteraction,
      options.parameterName);
  auto const receiverParameter = receiver.rtiAmbassador().getParameterHandle(
      receiverInteraction,
      options.parameterName);
  require(
      publisherInteraction.isValid() && receiverInteraction.isValid() &&
          publisherParameter.isValid() && receiverParameter.isValid() &&
          publisherInteraction == receiverInteraction &&
          publisherParameter == receiverParameter,
      "zero-lookahead TAR send-boundary lookup returned inconsistent standard handles");

  publisher.rtiAmbassador().publishInteractionClass(publisherInteraction);
  publisher.rtiAmbassador().changeInteractionOrderType(
      publisherInteraction,
      rti::TIMESTAMP);
  receiver.rtiAmbassador().subscribeInteractionClass(receiverInteraction, true);

  auto const publisherTime = makeTimeContext(publisher);
  auto const receiverTime = makeTimeContext(receiver);
  require(
      publisherTime.factory->getName() == receiverTime.factory->getName(),
      "zero-lookahead TAR send-boundary federates selected different logical-time factories");
  requireTimeEquals(
      *publisherTime.initial,
      *receiverTime.initial,
      "zero-lookahead TAR send-boundary initial time");

  receiver.rtiAmbassador().enableTimeConstrained();
  waitFor(
      receiver,
      [&] { return receiver.recorder().timeConstrainedEnabled().size() == 1U; },
      options,
      "zero-lookahead TAR send-boundary constrained callback");
  publisher.rtiAmbassador().enableTimeRegulation(*publisherTime.zero);
  waitFor(
      publisher,
      [&] { return publisher.recorder().timeRegulationEnabled().size() == 1U; },
      options,
      "zero-lookahead TAR send-boundary regulation callback");

  auto const requestedTime = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      5U);
  auto const futureTime = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      6U);
  std::vector<std::uint8_t> const payloadBytes{0xB7U, 0x05U};
  std::vector<std::uint8_t> const tagBytes{0xB7U, 0x06U};
  rti::ParameterHandleValueMap parameters;
  parameters.emplace(
      publisherParameter,
      rti::VariableLengthData(payloadBytes.data(), payloadBytes.size()));
  rti::VariableLengthData tag(tagBytes.data(), tagBytes.size());

  publisher.rtiAmbassador().timeAdvanceRequest(*requestedTime);
  bool boundaryRejected = false;
  try {
    static_cast<void>(publisher.rtiAmbassador().sendInteraction(
        publisherInteraction,
        parameters,
        tag,
        *requestedTime));
  } catch (rti::InvalidLogicalTime const&) {
    boundaryRejected = true;
  }
  require(
      boundaryRejected,
      "zero-lookahead TAR accepted a timestamped interaction at its requested-time boundary");

  auto const futureRetraction = publisher.rtiAmbassador().sendInteraction(
      publisherInteraction,
      parameters,
      tag,
      *futureTime);
  require(
      futureRetraction.isValid(),
      "zero-lookahead TAR rejected a timestamped interaction strictly after its requested time");
  require(
      publisher.recorder().timeAdvanceGrants().empty(),
      "zero-lookahead TAR was serviced before the test explicitly evoked its grant callback");
  waitFor(
      publisher,
      [&] { return publisher.recorder().timeAdvanceGrants().size() == 1U; },
      options,
      "zero-lookahead TAR send-boundary grant");
  require(
      publisher.recorder().timeAdvanceGrants().front().encoded ==
          encodeTime(*requestedTime),
      "zero-lookahead TAR send-boundary received the wrong grant time");
  require(
      receiver.recorder().timedInteractions().empty(),
      "zero-lookahead TAR send-boundary delivered the future interaction before a receiver advance");

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

}  // namespace

void scenarioTimestampedInteractionTaraQueuedEqualBoundary(
    Options const& options,
    rti::CallbackModel model) {
  runTimestampedInteractionTaraQueuedEqualBoundary(options, model);
}

void scenarioTimestampedInteractionTaraQueuedEqualBoundaryContract(
    Options const& options,
    rti::CallbackModel model) {
  runTimestampedInteractionTaraQueuedEqualBoundary(options, model);
}

void scenarioTimestampedInteractionZeroLookaheadSendAtGrant(
    Options const& options,
    rti::CallbackModel model) {
  runTimestampedInteractionZeroLookaheadSendAtGrant(options, model);
}

void scenarioTimestampedInteractionZeroLookaheadSendAtGrantContract(
    Options const& options,
    rti::CallbackModel model) {
  runTimestampedInteractionZeroLookaheadSendAtGrant(options, model);
}

void scenarioTimestampedInteractionZeroLookaheadTarSendBoundary(
    Options const& options,
    rti::CallbackModel model) {
  runTimestampedInteractionZeroLookaheadTarSendBoundary(options, model);
}

void scenarioTimestampedInteractionZeroLookaheadTarSendBoundaryContract(
    Options const& options,
    rti::CallbackModel model) {
  runTimestampedInteractionZeroLookaheadTarSendBoundary(options, model);
}

void scenarioTimestampedInteractionTaraStrictTimestampCutoff(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.logicalTimeImplementationName.empty(),
      "TARA strict timestamp cutoff requires an adapter-supplied logical-time implementation");

  Session publisher(options, model, "tara-strict-cutoff-publisher");
  Session receiver(options, model, "tara-strict-cutoff-receiver");
  auto const federation = federationName(
      options, "timestamped-interaction-tara-strict-timestamp-cutoff");
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
      "TARA strict timestamp cutoff lookup returned an invalid standard handle");
  require(
      publisherInteraction == receiverInteraction &&
          publisherParameter == receiverParameter,
      "TARA strict timestamp cutoff handles differed between federates");

  publisher.rtiAmbassador().publishInteractionClass(publisherInteraction);
  publisher.rtiAmbassador().changeInteractionOrderType(
      publisherInteraction,
      rti::TIMESTAMP);
  receiver.rtiAmbassador().subscribeInteractionClass(receiverInteraction, true);

  auto publisherTime = makeTimeContext(publisher);
  auto receiverTime = makeTimeContext(receiver);
  require(
      publisherTime.factory->getName() == receiverTime.factory->getName(),
      "TARA strict timestamp cutoff federates selected different logical-time factories");
  requireTimeEquals(
      *publisherTime.initial,
      *receiverTime.initial,
      "TARA strict timestamp cutoff initial time");

  receiver.rtiAmbassador().enableTimeConstrained();
  waitFor(
      receiver,
      [&] { return receiver.recorder().timeConstrainedEnabled().size() == 1U; },
      options,
      "TARA strict timestamp cutoff receiver constrained callback");
  publisher.rtiAmbassador().enableTimeRegulation(*publisherTime.epsilon);
  waitFor(
      publisher,
      [&] { return publisher.recorder().timeRegulationEnabled().size() == 1U; },
      options,
      "TARA strict timestamp cutoff publisher regulation callback");

  auto const messageTime = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      5U);
  auto const taraCutoff = timeAfter(
      *receiverTime.factory,
      *receiverTime.initial,
      *receiverTime.epsilon,
      6U);
  auto const nextCutoff = timeAfter(
      *receiverTime.factory,
      *receiverTime.initial,
      *receiverTime.epsilon,
      7U);
  require(
      *messageTime < *taraCutoff,
      "TARA strict timestamp cutoff message time was not earlier than the requested time");
  require(
      *taraCutoff < *nextCutoff,
      "TARA strict timestamp cutoff follow-up time was not later than its first request");

  std::vector<std::uint8_t> const earlierPayloadBytes{0x54U, 0x35U};
  std::vector<std::uint8_t> const cutoffPayloadBytes{0x54U, 0x36U};
  std::vector<std::uint8_t> const earlierTagBytes{'t', '5'};
  std::vector<std::uint8_t> const cutoffTagBytes{'t', '6'};
  rti::ParameterHandleValueMap earlierValues;
  earlierValues.emplace(
      publisherParameter,
      rti::VariableLengthData(
          earlierPayloadBytes.data(), earlierPayloadBytes.size()));
  rti::ParameterHandleValueMap cutoffValues;
  cutoffValues.emplace(
      publisherParameter,
      rti::VariableLengthData(
          cutoffPayloadBytes.data(), cutoffPayloadBytes.size()));
  rti::VariableLengthData earlierTag(
      earlierTagBytes.data(), earlierTagBytes.size());
  rti::VariableLengthData cutoffTag(
      cutoffTagBytes.data(), cutoffTagBytes.size());
  auto const earlierRetraction = publisher.rtiAmbassador().sendInteraction(
      publisherInteraction,
      earlierValues,
      earlierTag,
      *messageTime);
  auto const cutoffRetraction = publisher.rtiAmbassador().sendInteraction(
      publisherInteraction,
      cutoffValues,
      cutoffTag,
      *taraCutoff);
  require(
      earlierRetraction.isValid() && cutoffRetraction.isValid(),
      "TARA strict timestamp cutoff send returned an invalid retraction handle");

  // Leave the sender's GALT at the strict cutoff: timestamp 5 is safe for
  // TARA(6), while timestamp 6 is not safe until the sender advances again.
  publisher.rtiAmbassador().timeAdvanceRequest(*messageTime);
  waitFor(
      publisher,
      [&] { return publisher.recorder().timeAdvanceGrants().size() == 1U; },
      options,
      "TARA strict timestamp cutoff publisher advance");
  require(
      publisher.recorder().timeAdvanceGrants().front().encoded ==
          encodeTime(*messageTime),
      "TARA strict timestamp cutoff publisher received the wrong advance grant");
  require(
      receiver.recorder().timedInteractions().empty() &&
          receiver.recorder().timeAdvanceGrants().empty(),
      "TARA strict timestamp cutoff receiver observed delivery before its request");

  auto const callbackOrderBeforeTara = receiver.recorder().callbackOrder();
  receiver.rtiAmbassador().timeAdvanceRequestAvailable(*taraCutoff);
  waitFor(
      receiver,
      [&] { return receiver.recorder().timeAdvanceGrants().size() == 1U; },
      options,
      "TARA strict timestamp cutoff receiver grant");

  auto const deliveredAtFirstGrant = receiver.recorder().timedInteractions();
  require(
      deliveredAtFirstGrant.size() == 1U,
      "TARA strict timestamp cutoff delivered the equal-time interaction by its first grant");
  require(
      deliveredAtFirstGrant.front().interaction == receiverInteraction &&
          deliveredAtFirstGrant.front().time == encodeTime(*messageTime) &&
          deliveredAtFirstGrant.front().sentOrder == rti::TIMESTAMP &&
          deliveredAtFirstGrant.front().receivedOrder == rti::TIMESTAMP,
      "TARA strict timestamp cutoff did not deliver the earlier interaction at its first grant");
  require(
      copyBytes(deliveredAtFirstGrant.front().parameters.at(receiverParameter)) ==
              earlierPayloadBytes &&
          deliveredAtFirstGrant.front().tag == earlierTagBytes,
      "TARA strict timestamp cutoff changed the earlier interaction parameters or tag");
  auto const callbackOrder = receiver.recorder().callbackOrder();
  require(
      callbackOrder.size() == callbackOrderBeforeTara.size() + 2U &&
          std::equal(
              callbackOrderBeforeTara.begin(),
              callbackOrderBeforeTara.end(),
              callbackOrder.begin()) &&
          callbackOrder.at(callbackOrderBeforeTara.size()) == "interaction" &&
          callbackOrder.at(callbackOrderBeforeTara.size() + 1U) == "grant",
      "TARA strict timestamp cutoff did not deliver the earlier interaction before its grant");

  auto const callbackOrderAfterFirstGrant = callbackOrder;
  receiver.rtiAmbassador().timeAdvanceRequestAvailable(*nextCutoff);
  publisher.rtiAmbassador().timeAdvanceRequest(*taraCutoff);
  waitForSessions(
      {&publisher, &receiver},
      [&] {
        return publisher.recorder().timeAdvanceGrants().size() == 2U &&
            receiver.recorder().timeAdvanceGrants().size() == 2U;
      },
      options,
      "TARA strict timestamp cutoff follow-up advance");

  auto const delivered = receiver.recorder().timedInteractions();
  require(
      delivered.size() == 2U &&
          delivered.at(1U).interaction == receiverInteraction &&
          delivered.at(1U).time == encodeTime(*taraCutoff) &&
          delivered.at(1U).sentOrder == rti::TIMESTAMP &&
          delivered.at(1U).receivedOrder == rti::TIMESTAMP,
      "TARA strict timestamp cutoff did not release the equal-time interaction on the later advance");
  require(
      copyBytes(delivered.at(1U).parameters.at(receiverParameter)) ==
              cutoffPayloadBytes &&
          delivered.at(1U).tag == cutoffTagBytes,
      "TARA strict timestamp cutoff changed the equal-time interaction parameters or tag");
  require(
      publisher.recorder().timeAdvanceGrants().at(1U).encoded ==
          encodeTime(*taraCutoff),
      "TARA strict timestamp cutoff sender received the wrong follow-up grant time");
  auto const callbackOrderAfterSecondGrant = receiver.recorder().callbackOrder();
  require(
      callbackOrderAfterSecondGrant.size() == callbackOrderAfterFirstGrant.size() + 2U &&
          std::equal(
              callbackOrderAfterFirstGrant.begin(),
              callbackOrderAfterFirstGrant.end(),
              callbackOrderAfterSecondGrant.begin()) &&
          callbackOrderAfterSecondGrant.at(callbackOrderAfterFirstGrant.size()) ==
              "interaction" &&
          callbackOrderAfterSecondGrant.at(callbackOrderAfterFirstGrant.size() + 1U) ==
              "grant",
      "TARA strict timestamp cutoff did not deliver the equal-time interaction before the later grant");

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

void scenarioTimestampedInteractionTarStrictTimestampCutoff(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.logicalTimeImplementationName.empty(),
      "TAR strict timestamp cutoff requires an adapter-supplied logical-time implementation");

  Session publisher(options, model, "tar-strict-cutoff-publisher");
  Session receiver(options, model, "tar-strict-cutoff-receiver");
  auto const federation = federationName(
      options, "timestamped-interaction-tar-strict-timestamp-cutoff");
  connectAndJoin(publisher, receiver, options, federation, options.fom);

  auto const publisherInteraction = publisher.rtiAmbassador().getInteractionClassHandle(
      options.interactionClassName);
  auto const receiverInteraction = receiver.rtiAmbassador().getInteractionClassHandle(
      options.interactionClassName);
  auto const publisherParameter = publisher.rtiAmbassador().getParameterHandle(
      publisherInteraction, options.parameterName);
  auto const receiverParameter = receiver.rtiAmbassador().getParameterHandle(
      receiverInteraction, options.parameterName);
  require(
      publisherInteraction.isValid() && receiverInteraction.isValid() &&
          publisherParameter.isValid() && receiverParameter.isValid() &&
          publisherInteraction == receiverInteraction &&
          publisherParameter == receiverParameter,
      "TAR strict timestamp cutoff adapter FOM did not resolve stable handles");
  publisher.rtiAmbassador().publishInteractionClass(publisherInteraction);
  publisher.rtiAmbassador().changeInteractionOrderType(
      publisherInteraction, rti::TIMESTAMP);
  receiver.rtiAmbassador().subscribeInteractionClass(receiverInteraction, true);

  auto publisherTime = makeTimeContext(publisher);
  auto receiverTime = makeTimeContext(receiver);
  require(
      publisherTime.factory->getName() == receiverTime.factory->getName(),
      "TAR strict timestamp cutoff members selected different time factories");
  requireTimeEquals(
      *publisherTime.initial, *receiverTime.initial,
      "TAR strict timestamp cutoff initial time");

  receiver.rtiAmbassador().enableTimeConstrained();
  waitFor(
      receiver,
      [&] { return receiver.recorder().timeConstrainedEnabled().size() == 1U; },
      options,
      "TAR strict timestamp cutoff constrained callback");
  auto const five = timeAfter(
      *publisherTime.factory, *publisherTime.initial, *publisherTime.epsilon, 5U);
  publisher.rtiAmbassador().enableTimeRegulation(*publisherTime.epsilon);
  waitFor(
      publisher,
      [&] { return publisher.recorder().timeRegulationEnabled().size() == 1U; },
      options,
      "TAR strict timestamp cutoff regulation callback");

  auto const six = timeAfter(
      *receiverTime.factory, *receiverTime.initial, *receiverTime.epsilon, 6U);
  auto const eleven = timeAfter(
      *publisherTime.factory, *publisherTime.initial, *publisherTime.epsilon, 11U);
  auto const twelve = timeAfter(
      *receiverTime.factory, *receiverTime.initial, *receiverTime.epsilon, 12U);
  require(*five < *six && *six < *eleven && *eleven < *twelve,
          "TAR strict timestamp cutoff times were not ordered");

  std::vector<std::uint8_t> const payload{0x54U, 0x41U, 0x52U};
  std::vector<std::uint8_t> const tagBytes{'t', 'a', 'r'};
  rti::ParameterHandleValueMap values;
  values.emplace(
      publisherParameter,
      rti::VariableLengthData(payload.data(), payload.size()));
  rti::VariableLengthData tag(tagBytes.data(), tagBytes.size());
  auto const first = publisher.rtiAmbassador().sendInteraction(
      publisherInteraction, values, tag, *five);
  auto const equal = publisher.rtiAmbassador().sendInteraction(
      publisherInteraction, values, tag, *six);
  auto const future = publisher.rtiAmbassador().sendInteraction(
      publisherInteraction, values, tag, *eleven);
  require(first.isValid() && equal.isValid() && future.isValid(),
          "TAR strict timestamp cutoff sends returned invalid retraction handles");
  require(receiver.recorder().timedInteractions().empty(),
          "TAR strict timestamp cutoff delivered TSO before receiver TAR");

  publisher.rtiAmbassador().timeAdvanceRequest(*six);
  waitFor(
      publisher,
      [&] { return publisher.recorder().timeAdvanceGrants().size() == 1U; },
      options,
      "TAR strict timestamp cutoff publisher first grant");
  auto const callbacksBeforeTar = receiver.recorder().callbackOrder();
  receiver.rtiAmbassador().timeAdvanceRequest(*six);
  try {
    waitFor(
        receiver,
        [&] {
          return receiver.recorder().timedInteractions().size() >= 2U &&
              receiver.recorder().timeAdvanceGrants().size() >= 1U;
        },
        options,
        "TAR strict timestamp cutoff first receiver grant");
  } catch (std::runtime_error const& error) {
    throw std::runtime_error(
        std::string(error.what()) + " (timestamped callbacks=" +
        std::to_string(receiver.recorder().timedInteractions().size()) +
        ", receive-order callbacks=" +
        std::to_string(receiver.recorder().interactions().size()) +
        ", grants=" +
        std::to_string(receiver.recorder().timeAdvanceGrants().size()) + ")");
  }
  auto const firstDelivery = receiver.recorder().timedInteractions();
  require(
      firstDelivery.size() == 2U &&
          firstDelivery.at(0U).time == encodeTime(*five) &&
          firstDelivery.at(1U).time == encodeTime(*six),
      "TAR(6) did not deliver timestamps five and six at its grant");
  auto const firstGrantOrder = receiver.recorder().callbackOrder();
  require(
      firstGrantOrder.size() == callbacksBeforeTar.size() + 3U &&
          std::equal(
              callbacksBeforeTar.begin(), callbacksBeforeTar.end(),
              firstGrantOrder.begin()) &&
          firstGrantOrder.at(callbacksBeforeTar.size()) == "interaction" &&
          firstGrantOrder.at(callbacksBeforeTar.size() + 1U) == "interaction" &&
          firstGrantOrder.back() == "grant" &&
          receiver.recorder().timeAdvanceGrants().front().encoded == encodeTime(*six) &&
          receiver.recorder().timedInteractions().size() == 2U,
      "TAR(6) grant did not follow both eligible TSO callbacks or leaked future TSO");

  publisher.rtiAmbassador().timeAdvanceRequest(*twelve);
  waitFor(
      publisher,
      [&] { return publisher.recorder().timeAdvanceGrants().size() == 2U; },
      options,
      "TAR strict timestamp cutoff publisher final advance");
  receiver.rtiAmbassador().timeAdvanceRequest(*twelve);
  waitFor(
      receiver,
      [&] {
        return receiver.recorder().timedInteractions().size() >= 3U &&
            receiver.recorder().timeAdvanceGrants().size() >= 2U;
      },
      options,
      "TAR strict timestamp cutoff second receiver grant");
  auto const delivered = receiver.recorder().timedInteractions();
  auto const callbackOrder = receiver.recorder().callbackOrder();
  require(
      delivered.size() == 3U &&
          delivered.at(2U).time == encodeTime(*eleven) &&
          callbackOrder.size() == firstGrantOrder.size() + 2U &&
          callbackOrder.at(firstGrantOrder.size()) == "interaction" &&
          callbackOrder.back() == "grant" &&
          receiver.recorder().timeAdvanceGrants().at(1U).encoded ==
              encodeTime(*twelve),
      "TAR(12) did not release only the later TSO before its grant");
  for (auto const& delivery : delivered) {
    require(
        delivery.interaction == receiverInteraction &&
            delivery.parameters.size() == 1U &&
            copyBytes(delivery.parameters.at(receiverParameter)) == payload &&
            delivery.tag == tagBytes &&
            delivery.producer == publisher.federateHandle() &&
            delivery.sentOrder == rti::TIMESTAMP &&
            delivery.receivedOrder == rti::TIMESTAMP,
        "TAR strict timestamp cutoff changed interaction callback metadata");
  }

  receiver.rtiAmbassador().disableTimeConstrained();
  publisher.rtiAmbassador().disableTimeRegulation();
  receiver.resign(rti::NO_ACTION);
  publisher.resign(rti::NO_ACTION);
  publisher.rtiAmbassador().destroyFederationExecution(federation);
  receiver.disconnect();
  publisher.disconnect();
}

void scenarioTimestampedInteractionTarStrictTimestampCutoffContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioTimestampedInteractionTarStrictTimestampCutoff(options, model);
}

void scenarioTimestampedInteractionTarEqualBoundary(
    Options const& options,
    rti::CallbackModel model) {
  runTimestampedInteractionMixedAdvancesScenario(
      options, model, TimestampedInteractionBoundaryMode::tar_equal);
}

void scenarioTimestampedInteractionTarPostGrantCutoff(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.logicalTimeImplementationName.empty(),
      "Timestamped interaction post-grant cutoff requires an adapter-supplied logical-time implementation");

  Session receiver(options, model, "tar-post-grant-receiver");
  Session publisher(options, model, "tar-post-grant-publisher");
  Session frontier(options, model, "tar-post-grant-frontier");
  auto const federation = federationName(options, "timestamped-interaction-tar-post-grant-cutoff");
  connectAndJoin(receiver, publisher, options, federation, options.fom);
  frontier.connect();
  frontier.join(
      options.ownerFederateName + L"-tar-post-grant-frontier",
      options.federateType,
      federation);

  auto const receiverInteraction = receiver.rtiAmbassador().getInteractionClassHandle(
      options.interactionClassName);
  auto const publisherInteraction = publisher.rtiAmbassador().getInteractionClassHandle(
      options.interactionClassName);
  auto const receiverParameter = receiver.rtiAmbassador().getParameterHandle(
      receiverInteraction,
      options.parameterName);
  auto const publisherParameter = publisher.rtiAmbassador().getParameterHandle(
      publisherInteraction,
      options.parameterName);
  require(
      receiverInteraction.isValid() && publisherInteraction.isValid() &&
          receiverParameter.isValid() && publisherParameter.isValid(),
      "Timestamped interaction post-grant cutoff lookup returned an invalid standard handle");
  require(
      receiverInteraction == publisherInteraction && receiverParameter == publisherParameter,
      "Timestamped interaction post-grant cutoff handles differed between members");

  publisher.rtiAmbassador().publishInteractionClass(publisherInteraction);
  publisher.rtiAmbassador().changeInteractionOrderType(
      publisherInteraction,
      rti::TIMESTAMP);
  receiver.rtiAmbassador().subscribeInteractionClass(receiverInteraction, true);

  auto receiverTime = makeTimeContext(receiver);
  auto publisherTime = makeTimeContext(publisher);
  auto frontierTime = makeTimeContext(frontier);
  require(
      receiverTime.factory->getName() == publisherTime.factory->getName() &&
          receiverTime.factory->getName() == frontierTime.factory->getName(),
      "Timestamped interaction post-grant cutoff members selected different logical-time factories");
  requireTimeEquals(
      *receiverTime.initial,
      *publisherTime.initial,
      "Timestamped interaction post-grant cutoff publisher initial time");
  requireTimeEquals(
      *receiverTime.initial,
      *frontierTime.initial,
      "Timestamped interaction post-grant cutoff frontier initial time");

  receiver.rtiAmbassador().enableTimeConstrained();
  waitFor(
      receiver,
      [&] { return receiver.recorder().timeConstrainedEnabled().size() == 1U; },
      options,
      "Timestamped interaction post-grant cutoff receiver constrained callback");
  frontier.rtiAmbassador().enableTimeRegulation(*frontierTime.epsilon);
  waitFor(
      frontier,
      [&] { return frontier.recorder().timeRegulationEnabled().size() == 1U; },
      options,
      "Timestamped interaction post-grant cutoff frontier regulation callback");

  auto const receiverAtFive = timeAfter(
      *receiverTime.factory, *receiverTime.initial, *receiverTime.epsilon, 5U);
  auto const frontierAtFive = timeAfter(
      *frontierTime.factory, *frontierTime.initial, *frontierTime.epsilon, 5U);
  frontier.rtiAmbassador().timeAdvanceRequest(*frontierAtFive);
  waitFor(
      frontier,
      [&] { return frontier.recorder().timeAdvanceGrants().size() == 1U; },
      options,
      "Timestamped interaction post-grant cutoff initial frontier grant");
  receiver.rtiAmbassador().timeAdvanceRequest(*receiverAtFive);
  waitFor(
      receiver,
      [&] { return receiver.recorder().timeAdvanceGrants().size() == 1U; },
      options,
      "Timestamped interaction post-grant cutoff completed TAR(5)");
  require(
      receiver.recorder().timeAdvanceGrants().front().encoded == encodeTime(*receiverAtFive) &&
          receiver.recorder().timedInteractions().empty(),
      "Timestamped interaction post-grant cutoff receiver did not complete TAR(5) without delivery");

  // The publisher becomes a time-regulated producer only after the receiver's
  // TAR(5) grant, so the two sends exercise late TSO arrivals at 4 and 5.
  publisher.rtiAmbassador().enableTimeRegulation(*publisherTime.epsilon);
  waitFor(
      publisher,
      [&] { return publisher.recorder().timeRegulationEnabled().size() == 1U; },
      options,
      "Timestamped interaction post-grant cutoff late publisher regulation callback");

  std::vector<std::uint8_t> const payloadBytes{0x47U, 0x52U, 0x41U, 0x4EU, 0x54U};
  std::vector<std::uint8_t> const belowTagBytes{'b', 'e', 'l', 'o', 'w'};
  std::vector<std::uint8_t> const equalTagBytes{'e', 'q', 'u', 'a', 'l'};
  rti::ParameterHandleValueMap values;
  values.emplace(
      publisherParameter,
      rti::VariableLengthData(payloadBytes.data(), payloadBytes.size()));
  rti::VariableLengthData belowTag(belowTagBytes.data(), belowTagBytes.size());
  rti::VariableLengthData equalTag(equalTagBytes.data(), equalTagBytes.size());
  auto const publisherAtFour = timeAfter(
      *publisherTime.factory, *publisherTime.initial, *publisherTime.epsilon, 4U);
  auto const publisherAtFive = timeAfter(
      *publisherTime.factory, *publisherTime.initial, *publisherTime.epsilon, 5U);
  auto const publisherAtSix = timeAfter(
      *publisherTime.factory, *publisherTime.initial, *publisherTime.epsilon, 6U);
  auto const frontierAtSix = timeAfter(
      *frontierTime.factory, *frontierTime.initial, *frontierTime.epsilon, 6U);
  auto const receiverAtSix = timeAfter(
      *receiverTime.factory, *receiverTime.initial, *receiverTime.epsilon, 6U);

  bool belowTimestampAccepted = false;
  try {
    auto const belowRetraction = publisher.rtiAmbassador().sendInteraction(
        publisherInteraction, values, belowTag, *publisherAtFour);
    require(
        belowRetraction.isValid(),
        "Accepted timestamp-4 interaction returned an invalid retraction handle");
    belowTimestampAccepted = true;
  } catch (rti::InvalidLogicalTime const&) {
    // The standard's direct assertion is the post-grant delivery cutoff; it
    // does not require this below-boundary input to be accepted by the sender.
  }
  auto const equalRetraction = publisher.rtiAmbassador().sendInteraction(
      publisherInteraction, values, equalTag, *publisherAtFive);
  require(
      equalRetraction.isValid(),
      "Accepted timestamp-5 interaction returned an invalid retraction handle");
  require(
      receiver.recorder().timedInteractions().empty(),
      "Timestamped interaction post-grant cutoff delivered before producer advancement");

  publisher.rtiAmbassador().timeAdvanceRequest(*publisherAtFour);
  waitFor(
      publisher,
      [&] { return publisher.recorder().timeAdvanceGrants().size() == 1U; },
      options,
      "Timestamped interaction post-grant cutoff publisher TAR(4)");
  require(
      publisher.recorder().timeAdvanceGrants().front().encoded == encodeTime(*publisherAtFour) &&
          receiver.recorder().timedInteractions().empty(),
      "Timestamped interaction post-grant cutoff delivered a late message while publisher advanced to 4");

  publisher.rtiAmbassador().timeAdvanceRequest(*publisherAtSix);
  frontier.rtiAmbassador().timeAdvanceRequest(*frontierAtSix);
  waitForSessions(
      {&publisher, &frontier},
      [&] {
        return publisher.recorder().timeAdvanceGrants().size() == 2U &&
            frontier.recorder().timeAdvanceGrants().size() == 2U;
      },
      options,
      "Timestamped interaction post-grant cutoff producer frontiers at 6");
  require(
      publisher.recorder().timeAdvanceGrants().size() == 2U &&
          publisher.recorder().timeAdvanceGrants()[1].encoded == encodeTime(*publisherAtSix),
      "Timestamped interaction post-grant cutoff publisher did not grant TAR(6)");
  require(
      frontier.recorder().timeAdvanceGrants().size() == 2U &&
          frontier.recorder().timeAdvanceGrants()[1].encoded == encodeTime(*frontierAtSix),
      "Timestamped interaction post-grant cutoff frontier did not grant TAR(6)");
  receiver.rtiAmbassador().timeAdvanceRequest(*receiverAtSix);
  try {
    waitForSessions(
        {&publisher, &frontier, &receiver},
        [&] { return receiver.recorder().timeAdvanceGrants().size() == 2U; },
        options,
        "Timestamped interaction post-grant cutoff later receiver grant");
  } catch (std::runtime_error const& error) {
    auto diagnostic = std::string(error.what()) +
        " (producer grants=" +
        std::to_string(publisher.recorder().timeAdvanceGrants().size()) +
        ", frontier grants=" +
        std::to_string(frontier.recorder().timeAdvanceGrants().size()) +
        ", receiver grants=" +
        std::to_string(receiver.recorder().timeAdvanceGrants().size()) +
        ", receiver interactions=" +
        std::to_string(receiver.recorder().timedInteractions().size()) +
        ", timestamp-4 accepted=" + (belowTimestampAccepted ? "true" : "false");
    try {
      auto observedGalt = receiverTime.factory->makeInitial();
      if (observedGalt != nullptr && receiver.rtiAmbassador().queryGALT(*observedGalt)) {
        auto const encodedGalt = encodeTime(*observedGalt);
        if (encodedGalt == encodeTime(*receiverAtSix)) {
          diagnostic += ", receiver GALT equals TAR(6)";
        } else if (encodedGalt == encodeTime(*receiverAtFive)) {
          diagnostic += ", receiver GALT equals the completed TAR(5)";
        } else if (encodedGalt == encodeTime(*publisherAtFour)) {
          diagnostic += ", receiver GALT equals the publisher TAR(4) boundary";
        } else if (encodedGalt == encodeTime(*receiverTime.initial)) {
          diagnostic += ", receiver GALT equals initial time";
        } else {
          diagnostic += ", receiver GALT is at another boundary";
        }
      } else {
        diagnostic += ", receiver GALT is undefined";
      }
    } catch (std::exception const& queryError) {
      diagnostic += ", queryGALT diagnostic failed: ";
      diagnostic += queryError.what();
    }
    diagnostic += ")";
    throw std::runtime_error(diagnostic);
  }
  require(
      receiver.recorder().timeAdvanceGrants().size() == 2U &&
          receiver.recorder().timeAdvanceGrants().back().encoded == encodeTime(*receiverAtSix) &&
          receiver.recorder().timedInteractions().empty(),
      "Timestamped interaction at or below completed TAR(5) was delivered before TAR(6)");

  receiver.rtiAmbassador().disableTimeConstrained();
  publisher.rtiAmbassador().disableTimeRegulation();
  frontier.rtiAmbassador().disableTimeRegulation();
  receiver.rtiAmbassador().unsubscribeInteractionClass(receiverInteraction);
  publisher.rtiAmbassador().unpublishInteractionClass(publisherInteraction);
  receiver.resign(rti::NO_ACTION);
  publisher.resign(rti::NO_ACTION);
  frontier.resign(rti::NO_ACTION);
  receiver.rtiAmbassador().destroyFederationExecution(federation);
  frontier.disconnect();
  publisher.disconnect();
  receiver.disconnect();
}

void scenarioTimestampedInteractionPublicationSendTransition(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.logicalTimeImplementationName.empty(),
      "Timestamped interaction publication transition requires an adapter-supplied logical-time implementation");
  Session publisher(options, model, "tso-publication-publisher");
  Session receiver(options, model, "tso-publication-receiver");
  auto const federation = federationName(
      options,
      "timestamped-interaction-publication-send-transition");
  connectAndJoin(publisher, receiver, options, federation, options.fom);

  auto const publisherInteraction = publisher.rtiAmbassador().getInteractionClassHandle(
      options.interactionClassName);
  auto const receiverInteraction = receiver.rtiAmbassador().getInteractionClassHandle(
      options.interactionClassName);
  auto const publisherParameter = publisher.rtiAmbassador().getParameterHandle(
      publisherInteraction, options.parameterName);
  auto const receiverParameter = receiver.rtiAmbassador().getParameterHandle(
      receiverInteraction, options.parameterName);
  require(
      publisherInteraction.isValid() && receiverInteraction.isValid() &&
          publisherParameter.isValid() && receiverParameter.isValid(),
      "Timestamped interaction publication transition lookup returned an invalid standard handle");

  publisher.rtiAmbassador().publishInteractionClass(publisherInteraction);
  publisher.rtiAmbassador().changeInteractionOrderType(
      publisherInteraction, rti::TIMESTAMP);
  receiver.rtiAmbassador().subscribeInteractionClass(receiverInteraction, true);
  auto publisherTime = makeTimeContext(publisher);
  auto receiverTime = makeTimeContext(receiver);
  enableTimestampedRoles(
      publisher, receiver, publisherTime, receiverTime, options,
      "Timestamped interaction publication transition");
  require(
      publisherTime.factory->getName() == receiverTime.factory->getName(),
      "Timestamped interaction publication members selected different logical-time factories");
  auto const timestamp = timeAfter(
      *publisherTime.factory, *publisherTime.initial, *publisherTime.epsilon, 5U);
  auto const receiverTarget = timeAfter(
      *receiverTime.factory, *receiverTime.initial, *receiverTime.epsilon, 5U);

  std::vector<std::uint8_t> const publishedValue{0x50U, 0x55U, 0x42U};
  std::vector<std::uint8_t> const unpublishedValue{0x55U, 0x4EU, 0x50U};
  std::vector<std::uint8_t> const publishedTag{0x50U, 0x31U};
  std::vector<std::uint8_t> const unpublishedTag{0x55U, 0x31U};
  auto send = [&](std::vector<std::uint8_t> const& value,
                  std::vector<std::uint8_t> const& tagBytes,
                  rti::LogicalTime const& sendTime) {
    rti::ParameterHandleValueMap parameters;
    parameters.emplace(
        publisherParameter,
        rti::VariableLengthData(value.data(), value.size()));
    rti::VariableLengthData tag(tagBytes.data(), tagBytes.size());
    return publisher.rtiAmbassador().sendInteraction(
        publisherInteraction, parameters, tag, sendTime);
  };

  auto const acceptedRetraction = send(publishedValue, publishedTag, *timestamp);
  require(
      acceptedRetraction.isValid(),
      "Published timestamped Send Interaction returned an invalid retraction handle");
  require(
      receiver.recorder().timedInteractions().empty(),
      "Published timestamped interaction arrived before the receiver's time advance");

  publisher.rtiAmbassador().unpublishInteractionClass(publisherInteraction);
  requireException(
      [&] { static_cast<void>(send(unpublishedValue, unpublishedTag, *timestamp)); },
      L"InteractionClassNotPublished",
      "sending a timestamped interaction after unpublishing its class");
  require(
      receiver.recorder().timedInteractions().empty(),
      "Rejected unpublished timestamped Send Interaction created a receiver callback");

  receiver.rtiAmbassador().unsubscribeInteractionClass(receiverInteraction);
  receiver.resign(rti::NO_ACTION);
  publisher.resign(rti::NO_ACTION);
  publisher.rtiAmbassador().destroyFederationExecution(federation);
  receiver.disconnect();
  publisher.disconnect();
}

void scenarioTimestampedInteractionPublicationSendTransitionContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioTimestampedInteractionPublicationSendTransition(options, model);
}

void scenarioTimestampedInteractionMixedAdvancesContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioTimestampedInteractionMixedAdvances(options, model);
}

void scenarioTimestampedInteractionTaraEqualBoundaryContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioTimestampedInteractionTaraEqualBoundary(options, model);
}

void scenarioTimestampedInteractionTarEqualBoundaryContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioTimestampedInteractionTarEqualBoundary(options, model);
}

void scenarioTimestampedInteractionTarPostGrantCutoffContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioTimestampedInteractionTarPostGrantCutoff(options, model);
}

void scenarioTimestampedInteractionMixedAdvancesPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioTimestampedInteractionMixedAdvances(options, model);
}

void scenarioTimestampedInteractionMixedAdvancesContractPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioTimestampedInteractionMixedAdvancesContract(options, model);
}

void scenarioTimestampedInteractionTaraEqualBoundaryPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioTimestampedInteractionTaraEqualBoundary(options, model);
}

void scenarioTimestampedInteractionTaraEqualBoundaryContractPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioTimestampedInteractionTaraEqualBoundaryContract(options, model);
}

void scenarioTimestampedInteractionTaraQueuedEqualBoundaryPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioTimestampedInteractionTaraQueuedEqualBoundary(options, model);
}

void scenarioTimestampedInteractionTaraQueuedEqualBoundaryContractPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioTimestampedInteractionTaraQueuedEqualBoundaryContract(options, model);
}

void scenarioTimestampedInteractionZeroLookaheadSendAtGrantPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioTimestampedInteractionZeroLookaheadSendAtGrant(options, model);
}

void scenarioTimestampedInteractionZeroLookaheadSendAtGrantContractPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioTimestampedInteractionZeroLookaheadSendAtGrantContract(options, model);
}

void scenarioTimestampedInteractionZeroLookaheadTarSendBoundaryPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioTimestampedInteractionZeroLookaheadTarSendBoundary(options, model);
}

void scenarioTimestampedInteractionZeroLookaheadTarSendBoundaryContractPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioTimestampedInteractionZeroLookaheadTarSendBoundaryContract(options, model);
}

void scenarioTimestampedInteractionTarEqualBoundaryPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioTimestampedInteractionTarEqualBoundary(options, model);
}

void scenarioTimestampedInteractionTarEqualBoundaryContractPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioTimestampedInteractionTarEqualBoundaryContract(options, model);
}

void scenarioTimestampedInteractionTarPostGrantCutoffPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioTimestampedInteractionTarPostGrantCutoff(options, model);
}

void scenarioTimestampedInteractionTarPostGrantCutoffContractPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioTimestampedInteractionTarPostGrantCutoffContract(options, model);
}

void scenarioTimestampedInteractionPublicationSendTransitionPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioTimestampedInteractionPublicationSendTransition(options, model);
}

void scenarioTimestampedInteractionPublicationSendTransitionContractPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioTimestampedInteractionPublicationSendTransitionContract(options, model);
}

}  // namespace hla_rti_cpp_tck::detail
