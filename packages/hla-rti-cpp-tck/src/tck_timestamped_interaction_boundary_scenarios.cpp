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
  auto const publisherTarget = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      equalTaraBoundary || equalTarBoundary ? 4U : 5U);
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
  auto const retraction = publisher.rtiAmbassador().sendInteraction(
      publisherInteraction,
      parameters,
      tag,
      *messageTime);
  require(
      retraction.isValid(),
      "Timestamped interaction mixed-advance send returned an invalid retraction handle");
  for (auto* member : members) {
    require(
        member->recorder().timedInteractions().empty(),
        "Timestamped interaction mixed-advance delivered before its request");
    member->recorder().clearCallbackOrder();
  }
  publisher.recorder().clearCallbackOrder();

  flushQueue.rtiAmbassador().flushQueueRequest(*requestBoundary);
  if (equalTarBoundary) {
    timeAdvanceAvailable.rtiAmbassador().timeAdvanceRequest(
        *timeAdvanceAvailableBoundary);
  } else {
    timeAdvanceAvailable.rtiAmbassador().timeAdvanceRequestAvailable(
        *timeAdvanceAvailableBoundary);
  }
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
      "Timestamped interaction mixed-advance deliveries and grants");

  require(
      publisher.recorder().timeAdvanceGrants().size() == 1U,
      "Timestamped interaction mixed-advance publisher received the wrong grant count");
  require(
      flushQueue.recorder().timedInteractions().size() == 1U &&
          flushQueue.recorder().flushQueueGrants().size() == 1U &&
          flushQueue.recorder().timeAdvanceGrants().empty(),
      "Timestamped interaction Flush Queue Request returned the wrong callback pair");
  require(
      timeAdvanceAvailable.recorder().timedInteractions().size() == 1U &&
          timeAdvanceAvailable.recorder().timeAdvanceGrants().size() == 1U &&
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
  require(
      timeAdvanceAvailable.recorder().callbackOrder() ==
          (equalTaraBoundary
               ? std::vector<std::string>{"grant", "interaction"}
               : std::vector<std::string>{"interaction", "grant"}),
      equalTaraBoundary
          ? "Timestamped interaction at the Time Advance Request Available boundary was delivered before the grant"
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
          encodeTime(*publisherTarget),
      "Timestamped interaction mixed-advance publisher received the wrong grant time");
  require(
      timeAdvanceAvailable.recorder().timeAdvanceGrants().front().encoded ==
          encodeTime(*messageTime) &&
          nextMessageAvailable.recorder().timeAdvanceGrants().front().encoded ==
              encodeTime(*messageTime),
      "Timestamped interaction mixed-advance returned the wrong alternate grant time");

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

void scenarioTimestampedInteractionTarEqualBoundary(
    Options const& options,
    rti::CallbackModel model) {
  runTimestampedInteractionMixedAdvancesScenario(
      options, model, TimestampedInteractionBoundaryMode::tar_equal);
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

}  // namespace hla_rti_cpp_tck::detail
