#include "tck_timestamped_interaction_scheduling_scenarios.hpp"
#include "tck_main_scenarios.hpp"

namespace hla_rti_cpp_tck::detail {

void scenarioTimestampedInteractionSourceResignation(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.logicalTimeImplementationName.empty(),
      "Timestamped interaction source resignation requires an adapter-supplied logical-time implementation");

  Session publisher(options, model, "owner");
  Session receiver(options, model, "member");
  Session clock(options, model, "source-resignation-clock");
  auto const federation = federationName(
      options,
      "timestamped-interaction-source-resignation");
  connectAndJoin(publisher, receiver, options, federation, options.fom);
  clock.connect();
  clock.join(
      options.memberFederateName + L"-timestamped-interaction-resignation-clock",
      options.federateType,
      federation);

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
      "timestamped interaction source resignation lookup returned an invalid handle");

  publisher.rtiAmbassador().publishInteractionClass(publisherInteraction);
  publisher.rtiAmbassador().changeInteractionOrderType(
      publisherInteraction,
      rti::TIMESTAMP);
  receiver.rtiAmbassador().subscribeInteractionClass(receiverInteraction, true);

  auto publisherTime = makeTimeContext(publisher);
  auto receiverTime = makeTimeContext(receiver);
  auto clockTime = makeTimeContext(clock);
  require(
      publisherTime.factory->getName() == receiverTime.factory->getName() &&
          publisherTime.factory->getName() == clockTime.factory->getName(),
      "timestamped interaction source resignation members selected different logical-time factories");

  auto makeLookahead = [](TimeContext const& time) {
    auto target = timeAfter(*time.factory, *time.initial, *time.epsilon, 5U);
    auto result = time.factory->makeZero();
    require(
        result != nullptr,
        "timestamped interaction source resignation could not allocate a lookahead interval");
    result->setToDifference(*target, *time.initial);
    return result;
  };
  auto publisherLookahead = makeLookahead(publisherTime);
  auto clockLookahead = makeLookahead(clockTime);

  receiver.rtiAmbassador().enableTimeConstrained();
  waitFor(
      receiver,
      [&] { return receiver.recorder().timeConstrainedEnabled().size() >= 1U; },
      options,
      "timestamped interaction source resignation time-constrained callback");
  publisher.rtiAmbassador().enableTimeRegulation(*publisherLookahead);
  waitFor(
      publisher,
      [&] { return publisher.recorder().timeRegulationEnabled().size() >= 1U; },
      options,
      "timestamped interaction source resignation publisher time-regulation callback");
  clock.rtiAmbassador().enableTimeRegulation(*clockLookahead);
  waitFor(
      clock,
      [&] { return clock.recorder().timeRegulationEnabled().size() >= 1U; },
      options,
      "timestamped interaction source resignation clock time-regulation callback");

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
  auto const clockTarget = timeAfter(
      *clockTime.factory,
      *clockTime.initial,
      *clockTime.epsilon,
      2U);
  std::vector<std::uint8_t> parameterBytes{
      0x52U, 0x45U, 0x53U, 0x49U, 0x47U};
  std::vector<std::uint8_t> tagBytes{
      0x54U, 0x53U, 0x4FU, 0x2DU, 0x52U};
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
      "timestamped interaction source resignation send returned an invalid retraction handle");
  require(
      receiver.recorder().timedInteractions().empty(),
      "timestamped interaction source resignation delivered before the receiver frontier");

  receiver.rtiAmbassador().timeAdvanceRequest(*receiverTarget);
  require(
      receiver.recorder().timeAdvanceGrants().empty(),
      "timestamped interaction source resignation granted before the producer frontier");
  publisher.resign(rti::NO_ACTION);
  requireException(
      [&] { publisher.rtiAmbassador().retract(retraction); },
      L"FederateNotExecutionMember",
      "timestamped interaction source resignation post-resignation retraction");
  require(
      receiver.recorder().timedInteractions().empty() &&
          receiver.recorder().timeAdvanceGrants().empty(),
      "timestamped interaction source resignation released the queued callback too early");

  clock.rtiAmbassador().timeAdvanceRequest(*clockTarget);
  waitFor(
      clock,
      [&] { return clock.recorder().timeAdvanceGrants().size() >= 1U; },
      options,
      "timestamped interaction source resignation clock frontier");
  waitFor(
      receiver,
      [&] {
        return receiver.recorder().timedInteractions().size() >= 1U &&
            receiver.recorder().timeAdvanceGrants().size() >= 1U;
      },
      options,
      "timestamped interaction source resignation delivery and grant");

  require(
      receiver.recorder().timedInteractions().size() == 1U &&
          receiver.recorder().timeAdvanceGrants().size() == 1U,
      "timestamped interaction source resignation returned duplicate callbacks");
  require(
      receiver.recorder().callbackOrder() ==
          std::vector<std::string>{"interaction", "grant"},
      "timestamped interaction source resignation delivered after its grant");
  auto const interactions = receiver.recorder().timedInteractions();
  auto const& interaction = interactions.front();
  require(
      interaction.interaction == receiverInteraction &&
          interaction.parameters.size() == 1U &&
          interaction.parameters.count(receiverParameter) == 1U &&
          copyBytes(interaction.parameters.at(receiverParameter)) == parameterBytes,
      "timestamped interaction source resignation returned the wrong class or parameter");
  require(
      interaction.tag == tagBytes && interaction.time == encodeTime(*messageTime) &&
          !interaction.timeText.empty(),
      "timestamped interaction source resignation did not preserve payload or time");
  require(
      interaction.producer == publisher.federateHandle() &&
          interaction.sentOrder == rti::TIMESTAMP &&
          interaction.receivedOrder == rti::TIMESTAMP,
      "timestamped interaction source resignation returned the wrong producer or order metadata");
  require(
      interaction.transportation.isValid() &&
          !receiver.rtiAmbassador().getTransportationTypeName(
              interaction.transportation).empty(),
      "timestamped interaction source resignation returned an unresolvable transport");
  require(
      !interaction.regions.has_value(),
      "timestamped interaction source resignation unexpectedly supplied source regions");
  require(
      interaction.retractionPresent &&
          interaction.retraction == copyBytes(retraction.encode()),
      "timestamped interaction source resignation omitted its retraction handle");
  require(
      receiver.recorder().timeAdvanceGrants().front().encoded ==
          encodeTime(*receiverTarget),
      "timestamped interaction source resignation returned the wrong receiver grant");

  receiver.rtiAmbassador().disableTimeConstrained();
  clock.rtiAmbassador().disableTimeRegulation();
  receiver.rtiAmbassador().unsubscribeInteractionClass(receiverInteraction);
  receiver.resign(rti::NO_ACTION);
  clock.resign(rti::NO_ACTION);
  receiver.rtiAmbassador().destroyFederationExecution(federation);
  publisher.disconnect();
  receiver.disconnect();
  clock.disconnect();
}

void scenarioTimestampedInteractionSourceResignationFanout(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.logicalTimeImplementationName.empty(),
      "Timestamped interaction source resignation fanout requires an adapter-supplied logical-time implementation");

  Session publisher(options, model, "owner");
  Session first(options, model, "first");
  Session second(options, model, "second");
  Session clock(options, model, "source-resignation-clock");
  auto const federation = federationName(
      options,
      "timestamped-interaction-source-resignation-fanout");
  connectAndJoin(publisher, first, options, federation, options.fom);
  second.connect();
  second.join(
      options.memberFederateName + L"-timestamped-interaction-resignation-second",
      options.federateType,
      federation);
  clock.connect();
  clock.join(
      options.memberFederateName + L"-timestamped-interaction-resignation-clock",
      options.federateType,
      federation);

  auto const publisherInteraction = publisher.rtiAmbassador().getInteractionClassHandle(
      options.interactionClassName);
  auto const firstInteraction = first.rtiAmbassador().getInteractionClassHandle(
      options.interactionClassName);
  auto const secondInteraction = second.rtiAmbassador().getInteractionClassHandle(
      options.interactionClassName);
  auto const publisherParameter = publisher.rtiAmbassador().getParameterHandle(
      publisherInteraction,
      options.parameterName);
  auto const firstParameter = first.rtiAmbassador().getParameterHandle(
      firstInteraction,
      options.parameterName);
  auto const secondParameter = second.rtiAmbassador().getParameterHandle(
      secondInteraction,
      options.parameterName);
  require(
      publisherInteraction.isValid() && firstInteraction.isValid() &&
          secondInteraction.isValid() && publisherParameter.isValid() &&
          firstParameter.isValid() && secondParameter.isValid(),
      "timestamped interaction source resignation fanout lookup returned an invalid handle");

  publisher.rtiAmbassador().publishInteractionClass(publisherInteraction);
  publisher.rtiAmbassador().changeInteractionOrderType(
      publisherInteraction,
      rti::TIMESTAMP);
  first.rtiAmbassador().subscribeInteractionClass(firstInteraction, true);
  second.rtiAmbassador().subscribeInteractionClass(secondInteraction, true);

  auto publisherTime = makeTimeContext(publisher);
  auto firstTime = makeTimeContext(first);
  auto secondTime = makeTimeContext(second);
  auto clockTime = makeTimeContext(clock);
  require(
      publisherTime.factory->getName() == firstTime.factory->getName() &&
          publisherTime.factory->getName() == secondTime.factory->getName() &&
          publisherTime.factory->getName() == clockTime.factory->getName(),
      "timestamped interaction source resignation fanout members selected different logical-time factories");

  auto makeLookahead = [](TimeContext const& time) {
    auto target = timeAfter(*time.factory, *time.initial, *time.epsilon, 5U);
    auto result = time.factory->makeZero();
    require(
        result != nullptr,
        "timestamped interaction source resignation fanout could not allocate a lookahead interval");
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
      "timestamped interaction source resignation fanout first time-constrained callback");
  second.rtiAmbassador().enableTimeConstrained();
  waitFor(
      second,
      [&] { return second.recorder().timeConstrainedEnabled().size() >= 1U; },
      options,
      "timestamped interaction source resignation fanout second time-constrained callback");
  publisher.rtiAmbassador().enableTimeRegulation(*publisherLookahead);
  waitFor(
      publisher,
      [&] { return publisher.recorder().timeRegulationEnabled().size() >= 1U; },
      options,
      "timestamped interaction source resignation fanout publisher time-regulation callback");
  clock.rtiAmbassador().enableTimeRegulation(*clockLookahead);
  waitFor(
      clock,
      [&] { return clock.recorder().timeRegulationEnabled().size() >= 1U; },
      options,
      "timestamped interaction source resignation fanout clock time-regulation callback");

  auto const messageTime = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      6U);
  auto const firstTarget = timeAfter(
      *firstTime.factory,
      *firstTime.initial,
      *firstTime.epsilon,
      6U);
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
  std::vector<std::uint8_t> parameterBytes{
      0x46U, 0x41U, 0x4EU, 0x2DU, 0x4FU};
  std::vector<std::uint8_t> tagBytes{
      0x46U, 0x41U, 0x4EU, 0x2DU, 0x54U, 0x53U, 0x4FU};
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
      "timestamped interaction source resignation fanout send returned an invalid retraction handle");
  require(
      first.recorder().timedInteractions().empty() &&
          second.recorder().timedInteractions().empty(),
      "timestamped interaction source resignation fanout delivered before either recipient frontier");

  first.rtiAmbassador().timeAdvanceRequest(*firstTarget);
  second.rtiAmbassador().nextMessageRequest(*secondTarget);
  require(
      first.recorder().timeAdvanceGrants().empty() &&
          second.recorder().timeAdvanceGrants().empty(),
      "timestamped interaction source resignation fanout granted before the producer frontier");
  publisher.resign(rti::NO_ACTION);
  requireException(
      [&] { publisher.rtiAmbassador().retract(retraction); },
      L"FederateNotExecutionMember",
      "timestamped interaction source resignation fanout post-resignation retraction");
  require(
      first.recorder().timedInteractions().empty() &&
          second.recorder().timedInteractions().empty() &&
          first.recorder().timeAdvanceGrants().empty() &&
          second.recorder().timeAdvanceGrants().empty(),
      "timestamped interaction source resignation fanout released queued callbacks too early");

  clock.rtiAmbassador().timeAdvanceRequest(*clockTarget);
  waitFor(
      clock,
      [&] { return clock.recorder().timeAdvanceGrants().size() >= 1U; },
      options,
      "timestamped interaction source resignation fanout clock frontier");
  waitFor(
      first,
      [&] {
        return first.recorder().timedInteractions().size() >= 1U &&
            first.recorder().timeAdvanceGrants().size() >= 1U;
      },
      options,
      "timestamped interaction source resignation fanout first delivery and grant");
  waitFor(
      second,
      [&] {
        return second.recorder().timedInteractions().size() >= 1U &&
            second.recorder().timeAdvanceGrants().size() >= 1U;
      },
      options,
      "timestamped interaction source resignation fanout second delivery and grant");

  auto assertInteraction = [&](Session& receiver,
                               rti::InteractionClassHandle const& expectedInteraction,
                               rti::ParameterHandle const& expectedParameter,
                               TimedInteractionRecord const& record,
                               std::vector<std::uint8_t> const& expectedGrant) {
    require(
        record.interaction == expectedInteraction &&
            record.parameters.size() == 1U &&
            record.parameters.count(expectedParameter) == 1U &&
            copyBytes(record.parameters.at(expectedParameter)) == parameterBytes,
        "timestamped interaction source resignation fanout returned the wrong class or parameter");
    require(
        record.tag == tagBytes && record.time == encodeTime(*messageTime) &&
            !record.timeText.empty(),
        "timestamped interaction source resignation fanout did not preserve payload or time");
    require(
        record.producer == publisher.federateHandle() &&
            record.sentOrder == rti::TIMESTAMP &&
            record.receivedOrder == rti::TIMESTAMP,
        "timestamped interaction source resignation fanout returned the wrong producer or order metadata");
    require(
        record.transportation.isValid() &&
            !receiver.rtiAmbassador().getTransportationTypeName(
                record.transportation).empty(),
        "timestamped interaction source resignation fanout returned an unresolvable transport");
    require(
        !record.regions.has_value(),
        "timestamped interaction source resignation fanout unexpectedly supplied source regions");
    require(
        record.retractionPresent &&
            record.retraction == copyBytes(retraction.encode()),
        "timestamped interaction source resignation fanout omitted its retraction handle");
    require(
        receiver.recorder().timeAdvanceGrants().size() == 1U &&
            receiver.recorder().timeAdvanceGrants().front().encoded == expectedGrant,
        "timestamped interaction source resignation fanout returned the wrong recipient grant");
    require(
        receiver.recorder().callbackOrder() ==
            std::vector<std::string>{"interaction", "grant"},
        "timestamped interaction source resignation fanout delivered after a grant");
  };
  require(
      first.recorder().timedInteractions().size() == 1U &&
          second.recorder().timedInteractions().size() == 1U,
      "timestamped interaction source resignation fanout returned duplicate deliveries");
  assertInteraction(
      first,
      firstInteraction,
      firstParameter,
      first.recorder().timedInteractions().front(),
      encodeTime(*firstTarget));
  assertInteraction(
      second,
      secondInteraction,
      secondParameter,
      second.recorder().timedInteractions().front(),
      encodeTime(*messageTime));

  first.rtiAmbassador().disableTimeConstrained();
  second.rtiAmbassador().disableTimeConstrained();
  clock.rtiAmbassador().disableTimeRegulation();
  first.rtiAmbassador().unsubscribeInteractionClass(firstInteraction);
  second.rtiAmbassador().unsubscribeInteractionClass(secondInteraction);
  first.resign(rti::NO_ACTION);
  second.resign(rti::NO_ACTION);
  clock.resign(rti::NO_ACTION);
  first.rtiAmbassador().destroyFederationExecution(federation);
  publisher.disconnect();
  first.disconnect();
  second.disconnect();
  clock.disconnect();
}

void scenarioTimestampedInteractionFlushQueueFutureInput(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.logicalTimeImplementationName.empty(),
      "Timestamped interaction Flush Queue future-input test requires an adapter-supplied logical-time implementation");

  Session publisher(options, model, "owner");
  Session receiver(options, model, "member");
  auto const federation = federationName(
      options,
      "timestamped-interaction-flush-queue-future-input");

  publisher.connect();
  receiver.connect();
  publisher.rtiAmbassador().createFederationExecution(
      federation,
      options.fom.wstring(),
      options.logicalTimeImplementationName);
  publisher.join(options.ownerFederateName, options.federateType, federation);
  receiver.join(
      options.memberFederateName + L"-interaction-future-input",
      options.federateType,
      federation);

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
      "Timestamped interaction Flush Queue future-input lookup returned an invalid handle");

  publisher.rtiAmbassador().publishInteractionClass(publisherInteraction);
  publisher.rtiAmbassador().changeInteractionOrderType(
      publisherInteraction,
      rti::TIMESTAMP);
  receiver.rtiAmbassador().publishInteractionClass(receiverInteraction);
  receiver.rtiAmbassador().changeInteractionOrderType(
      receiverInteraction,
      rti::TIMESTAMP);
  receiver.rtiAmbassador().subscribeInteractionClass(receiverInteraction, true);

  auto publisherTime = makeTimeContext(publisher);
  auto receiverTime = makeTimeContext(receiver);
  require(
      publisherTime.factory->getName() == receiverTime.factory->getName(),
      "Timestamped interaction Flush Queue future-input members selected different logical-time factories");
  receiver.rtiAmbassador().enableTimeConstrained();
  waitFor(
      receiver,
      [&] { return receiver.recorder().timeConstrainedEnabled().size() >= 1U; },
      options,
      "Timestamped interaction Flush Queue future-input time-constrained callback");
  publisher.rtiAmbassador().enableTimeRegulation(*publisherTime.epsilon);
  waitFor(
      publisher,
      [&] { return publisher.recorder().timeRegulationEnabled().size() >= 1U; },
      options,
      "Timestamped interaction Flush Queue future-input time-regulation callback");
  receiver.rtiAmbassador().enableTimeRegulation(*receiverTime.epsilon);
  waitFor(
      receiver,
      [&] { return receiver.recorder().timeRegulationEnabled().size() >= 1U; },
      options,
      "Timestamped interaction Flush Queue future-input receiver time-regulation callback");

  auto const firstTime = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      11U);
  auto const secondTime = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      12U);
  auto const boundaryTime = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      10U);
  auto const retractedTime = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      13U);
  auto const publisherTarget = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      4U);
  auto const requestBoundary = timeAfter(
      *receiverTime.factory,
      *receiverTime.initial,
      *receiverTime.epsilon,
      10U);
  require(
      encodeTime(*boundaryTime) == encodeTime(*requestBoundary) &&
          *requestBoundary < *firstTime && *firstTime < *secondTime &&
          *secondTime < *retractedTime,
      "Timestamped interaction Flush Queue scenario requires boundary, future, and retractable TSO timestamps");

  std::vector<std::uint8_t> const boundaryParameterBytes{0x42U, 0x4FU, 0x55U, 0x4EU, 0x44U};
  std::vector<std::uint8_t> const firstParameterBytes{
      0x46U, 0x55U, 0x54U, 0x55U, 0x52U, 0x45U};
  std::vector<std::uint8_t> const secondParameterBytes{
      0x46U, 0x55U, 0x54U, 0x55U, 0x52U, 0x45U, 0x2DU, 0x32U};
  std::vector<std::uint8_t> const boundaryTagBytes{0x46U, 0x51U, 0x30U};
  std::vector<std::uint8_t> const firstTagBytes{0x46U, 0x51U, 0x31U};
  std::vector<std::uint8_t> const secondTagBytes{0x46U, 0x51U, 0x32U};
  std::vector<std::uint8_t> const retractedTagBytes{0x46U, 0x51U, 0x52U};
  rti::ParameterHandleValueMap boundaryParameters;
  boundaryParameters.emplace(
      publisherParameter,
      rti::VariableLengthData(
          boundaryParameterBytes.data(),
          boundaryParameterBytes.size()));
  rti::ParameterHandleValueMap firstParameters;
  firstParameters.emplace(
      publisherParameter,
      rti::VariableLengthData(
          firstParameterBytes.data(),
          firstParameterBytes.size()));
  rti::ParameterHandleValueMap secondParameters;
  secondParameters.emplace(
      publisherParameter,
      rti::VariableLengthData(
          secondParameterBytes.data(),
          secondParameterBytes.size()));
  rti::VariableLengthData boundaryTag(
      boundaryTagBytes.data(), boundaryTagBytes.size());
  rti::VariableLengthData firstTag(firstTagBytes.data(), firstTagBytes.size());
  rti::VariableLengthData secondTag(secondTagBytes.data(), secondTagBytes.size());
  rti::VariableLengthData retractedTag(
      retractedTagBytes.data(), retractedTagBytes.size());

  // Queue one message at the inclusive request boundary and two above it.
  // Retract a later queued message before FQR; only the other three may cross
  // the callback boundary before the grant.
  auto const boundaryRetraction = publisher.rtiAmbassador().sendInteraction(
      publisherInteraction,
      boundaryParameters,
      boundaryTag,
      *boundaryTime);
  auto const firstRetraction = publisher.rtiAmbassador().sendInteraction(
      publisherInteraction,
      firstParameters,
      firstTag,
      *firstTime);
  auto const secondRetraction = publisher.rtiAmbassador().sendInteraction(
      publisherInteraction,
      secondParameters,
      secondTag,
      *secondTime);
  auto const retractedRetraction = publisher.rtiAmbassador().sendInteraction(
      publisherInteraction,
      firstParameters,
      retractedTag,
      *retractedTime);
  require(
      boundaryRetraction.isValid() && firstRetraction.isValid() &&
          secondRetraction.isValid() && retractedRetraction.isValid(),
      "Timestamped interaction Flush Queue future-input send returned an invalid retraction handle");
  publisher.rtiAmbassador().retract(retractedRetraction);
  require(
      receiver.recorder().timedInteractions().empty(),
      "Timestamped interaction Flush Queue future-input delivered before time-constrained servicing");
  receiver.recorder().clearCallbackOrder();
  publisher.recorder().clearCallbackOrder();
  receiver.rtiAmbassador().flushQueueRequest(*requestBoundary);
  if (model == rti::HLA_EVOKED) {
    require(
        receiver.recorder().timedInteractions().empty(),
        "Timestamped interaction Flush Queue future-input delivered before callback servicing");
  }
  publisher.rtiAmbassador().timeAdvanceRequest(*publisherTarget);

  waitForSessions(
      {&publisher, &receiver},
      [&] {
        return publisher.recorder().timeAdvanceGrants().size() >= 1U &&
            receiver.recorder().timedInteractions().size() >= 3U &&
            receiver.recorder().flushQueueGrants().size() >= 1U;
      },
      options,
      "Timestamped interaction Flush Queue future-input deliveries and grant");

  require(
      publisher.recorder().timeAdvanceGrants().size() == 1U,
      "Timestamped interaction Flush Queue future-input publisher received the wrong grant count");
  require(
      receiver.recorder().timedInteractions().size() == 3U &&
          receiver.recorder().flushQueueGrants().size() == 1U &&
          receiver.recorder().timeAdvanceGrants().empty(),
      "Timestamped interaction Flush Queue future-input returned the wrong callback counts");
  require(
      receiver.recorder().callbackOrder() ==
          std::vector<std::string>{"interaction", "interaction", "interaction", "flush-grant"},
      "Timestamped interaction Flush Queue future-input granted before queued delivery");

  require(
      publisher.recorder().timeAdvanceGrants().front().encoded ==
          encodeTime(*publisherTarget),
      "Timestamped interaction Flush Queue future-input publisher received the wrong grant time");

  auto assertInteraction = [&](TimedInteractionRecord const& record,
                               std::vector<std::uint8_t> const& expectedParameter,
                               std::vector<std::uint8_t> const& expectedTag,
                               std::vector<std::uint8_t> const& expectedTime,
                               std::vector<std::uint8_t> const& expectedRetraction) {
    require(
        record.interaction == receiverInteraction &&
            record.parameters.size() == 1U &&
            record.parameters.count(receiverParameter) == 1U,
        "Timestamped interaction Flush Queue future-input callback returned the wrong class or parameter set");
    require(
        copyBytes(record.parameters.at(receiverParameter)) == expectedParameter &&
            record.tag == expectedTag,
        "Timestamped interaction Flush Queue future-input callback did not preserve payload or tag");
    require(
        record.producer == publisher.federateHandle() &&
            record.time == expectedTime &&
            !record.timeText.empty(),
        "Timestamped interaction Flush Queue future-input callback returned the wrong producer or time");
    require(
        record.transportation.isValid() &&
            !receiver.rtiAmbassador().getTransportationTypeName(record.transportation).empty(),
        "Timestamped interaction Flush Queue future-input callback returned an unresolvable transport");
    require(
        record.sentOrder == rti::TIMESTAMP && record.receivedOrder == rti::TIMESTAMP,
        "Timestamped interaction Flush Queue future-input callback returned the wrong order metadata");
    require(
        !record.regions.has_value(),
        "Timestamped interaction Flush Queue future-input callback unexpectedly supplied regions");
    require(
        record.retractionPresent && record.retraction == expectedRetraction,
        "Timestamped interaction Flush Queue future-input callback returned the wrong retraction handle");
  };
  auto const interactions = receiver.recorder().timedInteractions();
  assertInteraction(
      interactions.at(0),
      boundaryParameterBytes,
      boundaryTagBytes,
      encodeTime(*boundaryTime),
      copyBytes(boundaryRetraction.encode()));
  assertInteraction(
      interactions.at(1),
      firstParameterBytes,
      firstTagBytes,
      encodeTime(*firstTime),
      copyBytes(firstRetraction.encode()));
  assertInteraction(
      interactions.at(2),
      secondParameterBytes,
      secondTagBytes,
      encodeTime(*secondTime),
      copyBytes(secondRetraction.encode()));

  auto const flushGrants = receiver.recorder().flushQueueGrants();
  require(
      flushGrants.front().optimisticTime == encodeTime(*requestBoundary),
      "Timestamped interaction Flush Queue future-input grant did not cap its optimistic frontier at the request time");
  require(
      !flushGrants.front().time.empty() &&
          !flushGrants.front().timeText.empty() &&
          !flushGrants.front().optimisticTimeText.empty(),
      "Timestamped interaction Flush Queue future-input grant returned incomplete time metadata");
  auto actualFlushTime = decodeTime(*receiverTime.factory, flushGrants.front().time);
  auto optimisticFlushTime =
      decodeTime(*receiverTime.factory, flushGrants.front().optimisticTime);
  require(
      actualFlushTime != nullptr && optimisticFlushTime != nullptr &&
          !(*actualFlushTime < *receiverTime.initial) &&
          !(*requestBoundary < *actualFlushTime) &&
          !(*optimisticFlushTime < *actualFlushTime),
      "Timestamped interaction Flush Queue future-input grant returned time outside bounds");
  auto queriedReceiverTime = receiverTime.factory->makeInitial();
  receiver.rtiAmbassador().queryLogicalTime(*queriedReceiverTime);
  require(
      encodeTime(*queriedReceiverTime) == flushGrants.front().time,
      "Timestamped interaction Flush Queue future-input query did not match its grant");

  auto queriedReceiverLookahead = receiverTime.factory->makeZero();
  receiver.rtiAmbassador().queryLookahead(*queriedReceiverLookahead);
  require(
      *queriedReceiverLookahead == *receiverTime.epsilon,
      "Timestamped interaction Flush Queue future-input receiver lookahead changed unexpectedly");
  auto const postGrantSendTime = timeAfter(
      *receiverTime.factory,
      *optimisticFlushTime,
      *queriedReceiverLookahead,
      1U);
  rti::ParameterHandleValueMap postGrantParameters;
  postGrantParameters.emplace(
      receiverParameter,
      rti::VariableLengthData(
          firstParameterBytes.data(),
          firstParameterBytes.size()));
  rti::VariableLengthData postGrantTag(
      firstTagBytes.data(),
      firstTagBytes.size());
  requireException(
      [&] {
        static_cast<void>(receiver.rtiAmbassador().sendInteraction(
            receiverInteraction,
            postGrantParameters,
            postGrantTag,
            *optimisticFlushTime));
      },
      L"InvalidLogicalTime",
      "FQG optimistic time plus lookahead rejected a lower TSO timestamp");
  auto const postGrantRetraction = receiver.rtiAmbassador().sendInteraction(
      receiverInteraction,
      postGrantParameters,
      postGrantTag,
      *postGrantSendTime);
  require(
      postGrantRetraction.isValid(),
      "Timestamped interaction Flush Queue future-input send at optimistic time plus lookahead returned an invalid retraction handle");
  auto logicalTimeAfterSend = receiverTime.factory->makeInitial();
  receiver.rtiAmbassador().queryLogicalTime(*logicalTimeAfterSend);
  require(
      encodeTime(*logicalTimeAfterSend) == flushGrants.front().time,
      "Timestamped interaction Flush Queue future-input post-grant send advanced logical time");

  // Retract the receiver's post-grant probe so the second FQR isolates replay
  // of the three interactions already delivered by the first request.
  receiver.rtiAmbassador().retract(postGrantRetraction);
  auto const secondRequestBoundary = timeAfter(
      *receiverTime.factory,
      *receiverTime.initial,
      *receiverTime.epsilon,
      20U);
  receiver.recorder().clearCallbackOrder();
  receiver.rtiAmbassador().flushQueueRequest(*secondRequestBoundary);
  waitFor(
      receiver,
      [&] { return receiver.recorder().flushQueueGrants().size() >= 2U; },
      options,
      "Timestamped interaction second Flush Queue grant without replay");
  auto const secondFlushGrants = receiver.recorder().flushQueueGrants();
  require(
      secondFlushGrants.size() == 2U &&
          receiver.recorder().timedInteractions().size() == 3U &&
          receiver.recorder().callbackOrder() ==
              std::vector<std::string>{"flush-grant"},
      "Timestamped interaction second Flush Queue request replayed a delivered or retracted interaction");
  require(
      secondFlushGrants.back().optimisticTime == encodeTime(*secondRequestBoundary),
      "Timestamped interaction second Flush Queue grant did not report its request frontier");
  auto queriedSecondFlushTime = receiverTime.factory->makeInitial();
  receiver.rtiAmbassador().queryLogicalTime(*queriedSecondFlushTime);
  require(
      encodeTime(*queriedSecondFlushTime) == secondFlushGrants.back().time,
      "Timestamped interaction second Flush Queue query did not match its grant");

  receiver.rtiAmbassador().disableTimeConstrained();
  receiver.rtiAmbassador().disableTimeRegulation();
  publisher.rtiAmbassador().disableTimeRegulation();
  receiver.rtiAmbassador().unsubscribeInteractionClass(receiverInteraction);
  receiver.rtiAmbassador().unpublishInteractionClass(receiverInteraction);
  publisher.rtiAmbassador().unpublishInteractionClass(publisherInteraction);
  receiver.resign(rti::NO_ACTION);
  publisher.resign(rti::NO_ACTION);
  publisher.rtiAmbassador().destroyFederationExecution(federation);
  receiver.disconnect();
  publisher.disconnect();
}

void scenarioTimestampedInteractionTsoDesignatorTerminalization(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.logicalTimeImplementationName.empty(),
      "Timestamped interaction TSO designator terminalization requires an adapter-supplied logical-time implementation");

  Session publisher(options, model, "owner");
  Session receiver(options, model, "member");
  auto const federation = federationName(
      options,
      "timestamped-interaction-tso-designator-terminalization");
  publisher.connect();
  receiver.connect();
  publisher.rtiAmbassador().createFederationExecution(
      federation,
      options.fom.wstring(),
      options.logicalTimeImplementationName);
  publisher.join(options.ownerFederateName, options.federateType, federation);
  receiver.join(
      options.memberFederateName + L"-tso-terminalization",
      options.federateType,
      federation);

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
      "Timestamped interaction TSO designator terminalization lookup returned an invalid handle");

  publisher.rtiAmbassador().publishInteractionClass(publisherInteraction);
  publisher.rtiAmbassador().changeInteractionOrderType(
      publisherInteraction,
      rti::TIMESTAMP);
  receiver.rtiAmbassador().subscribeInteractionClass(receiverInteraction, true);

  auto publisherTime = makeTimeContext(publisher);
  auto receiverTime = makeTimeContext(receiver);
  require(
      publisherTime.factory->getName() == receiverTime.factory->getName(),
      "Timestamped interaction TSO designator terminalization members selected different logical-time factories");
  auto makeLookahead = [](TimeContext const& time, std::size_t steps) {
    auto const target = timeAfter(
        *time.factory,
        *time.initial,
        *time.epsilon,
        steps);
    auto result = time.factory->makeZero();
    require(
        result != nullptr,
        "Timestamped interaction TSO designator terminalization could not allocate a lookahead interval");
    result->setToDifference(*target, *time.initial);
    return result;
  };
  auto publisherLookahead = makeLookahead(publisherTime, 5U);

  receiver.rtiAmbassador().enableTimeConstrained();
  waitFor(
      receiver,
      [&] { return receiver.recorder().timeConstrainedEnabled().size() >= 1U; },
      options,
      "Timestamped interaction TSO designator terminalization time-constrained callback");
  publisher.rtiAmbassador().enableTimeRegulation(*publisherLookahead);
  waitFor(
      publisher,
      [&] { return publisher.recorder().timeRegulationEnabled().size() >= 1U; },
      options,
      "Timestamped interaction TSO designator terminalization time-regulation callback");

  std::vector<std::uint8_t> const parameterBytes{0x54U, 0x53U, 0x4FU};
  std::vector<std::uint8_t> const tagBytes{0x54U, 0x53U, 0x4FU};
  rti::ParameterHandleValueMap parameters;
  parameters.emplace(
      publisherParameter,
      rti::VariableLengthData(parameterBytes.data(), parameterBytes.size()));
  rti::VariableLengthData tag(tagBytes.data(), tagBytes.size());

  auto sendAt = [&](std::size_t timestampSteps) {
    auto const timestamp = timeAfter(
        *publisherTime.factory,
        *publisherTime.initial,
        *publisherTime.epsilon,
        timestampSteps);
    auto const retraction = publisher.rtiAmbassador().sendInteraction(
        publisherInteraction,
        parameters,
        tag,
        *timestamp);
    require(
        retraction.isValid(),
        "Timestamped interaction TSO designator terminalization returned an invalid retraction handle");
    return retraction;
  };
  auto requireTerminal = [&](rti::MessageRetractionHandle const& retraction,
                             std::string const& description) {
    requireException(
        [&] { publisher.rtiAmbassador().retract(retraction); },
        L"MessageCanNoLongerBeRetracted",
        description);
  };
  auto requestAndTerminalize = [&](auto request,
                                   std::size_t timestampSteps,
                                   std::string const& description) {
    auto const retraction = sendAt(timestampSteps);
    auto const target = timeAfter(
        *publisherTime.factory,
        *publisherTime.initial,
        *publisherTime.epsilon,
        timestampSteps - 5U);
    auto const grantCount = publisher.recorder().timeAdvanceGrants().size();
    request(*target);
    waitFor(
        publisher,
        [&] {
          return publisher.recorder().timeAdvanceGrants().size() > grantCount;
        },
        options,
        description + " time-advance grant");
    require(
        publisher.recorder().timeAdvanceGrants().back().encoded ==
            encodeTime(*target),
        description + " returned the wrong grant time");
    requireTerminal(retraction, description + " terminal retraction");
    require(
        receiver.recorder().timedInteractions().empty(),
        description + " delivered a timestamped interaction to an idle constrained receiver");
  };

  requestAndTerminalize(
      [&](rti::LogicalTime const& target) {
        publisher.rtiAmbassador().timeAdvanceRequest(target);
      },
      6U,
      "Timestamped interaction TSO designator TAR");
  requestAndTerminalize(
      [&](rti::LogicalTime const& target) {
        publisher.rtiAmbassador().timeAdvanceRequestAvailable(target);
      },
      7U,
      "Timestamped interaction TSO designator TARA");
  requestAndTerminalize(
      [&](rti::LogicalTime const& target) {
        publisher.rtiAmbassador().nextMessageRequest(target);
      },
      8U,
      "Timestamped interaction TSO designator NMR");
  requestAndTerminalize(
      [&](rti::LogicalTime const& target) {
        publisher.rtiAmbassador().nextMessageRequestAvailable(target);
      },
      9U,
      "Timestamped interaction TSO designator NMRA");

  auto const flushRetraction = sendAt(10U);
  auto const flushBoundary = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      5U);
  auto const flushGrantCount = publisher.recorder().flushQueueGrants().size();
  publisher.rtiAmbassador().flushQueueRequest(*flushBoundary);
  waitFor(
      publisher,
      [&] {
        return publisher.recorder().flushQueueGrants().size() > flushGrantCount;
      },
      options,
      "Timestamped interaction TSO designator FQR grant");
  auto const flushGrant = publisher.recorder().flushQueueGrants().back();
  require(
      !flushGrant.time.empty() && !flushGrant.timeText.empty() &&
          !flushGrant.optimisticTime.empty() &&
          !flushGrant.optimisticTimeText.empty(),
      "Timestamped interaction TSO designator FQR returned incomplete time metadata");
  auto actualFlushTime = decodeTime(*publisherTime.factory, flushGrant.time);
  require(
      actualFlushTime != nullptr &&
          !(*actualFlushTime < *publisherTime.initial) &&
          !(*flushBoundary < *actualFlushTime),
      "Timestamped interaction TSO designator FQR returned a grant outside its boundary");
  auto queriedTime = publisherTime.factory->makeInitial();
  publisher.rtiAmbassador().queryLogicalTime(*queriedTime);
  require(
      encodeTime(*queriedTime) == flushGrant.time,
      "Timestamped interaction TSO designator FQR query did not match its grant");
  requireTerminal(
      flushRetraction,
      "Timestamped interaction TSO designator FQR terminal retraction");
  require(
      receiver.recorder().timedInteractions().empty(),
      "Timestamped interaction TSO designator FQR delivered to an idle constrained receiver");

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

void scenarioTimestampedAttributeUpdateFlushQueueFutureInput(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.logicalTimeImplementationName.empty(),
      "Timestamped attribute-update Flush Queue future-input test requires an adapter-supplied logical-time implementation");

  Session publisher(options, model, "owner");
  Session receiver(options, model, "member");
  auto const federation = federationName(
      options,
      "timestamped-attribute-update-flush-queue-future-input");
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
      "Timestamped attribute-update Flush Queue future-input lookup returned an invalid handle");

  rti::AttributeHandleSet publisherAttributes;
  publisherAttributes.insert(publisherAttribute);
  rti::AttributeHandleSet receiverAttributes;
  receiverAttributes.insert(receiverAttribute);
  publisher.rtiAmbassador().publishObjectClassAttributes(
      publisherClass,
      publisherAttributes);
  publisher.rtiAmbassador().changeDefaultAttributeOrderType(
      publisherClass,
      publisherAttributes,
      rti::TIMESTAMP);
  receiver.rtiAmbassador().subscribeObjectClassAttributes(
      receiverClass,
      receiverAttributes,
      true,
      L"");

  auto const object = publisher.rtiAmbassador().registerObjectInstance(publisherClass);
  require(
      object.isValid(),
      "Timestamped attribute-update Flush Queue future-input registration returned an invalid handle");
  waitFor(
      receiver,
      [&] { return receiver.recorder().hasDiscovery(object); },
      options,
      "Timestamped attribute-update Flush Queue future-input discovery");

  auto publisherTime = makeTimeContext(publisher);
  auto receiverTime = makeTimeContext(receiver);
  require(
      publisherTime.factory->getName() == receiverTime.factory->getName(),
      "Timestamped attribute-update Flush Queue future-input members selected different logical-time factories");
  receiver.rtiAmbassador().enableTimeConstrained();
  waitFor(
      receiver,
      [&] { return receiver.recorder().timeConstrainedEnabled().size() >= 1U; },
      options,
      "Timestamped attribute-update Flush Queue future-input time-constrained callback");
  publisher.rtiAmbassador().enableTimeRegulation(*publisherTime.epsilon);
  waitFor(
      publisher,
      [&] { return publisher.recorder().timeRegulationEnabled().size() >= 1U; },
      options,
      "Timestamped attribute-update Flush Queue future-input time-regulation callback");

  auto const firstTime = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      9U);
  auto const secondTime = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      12U);
  auto const publisherTarget = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      4U);
  auto const requestBoundary = timeAfter(
      *receiverTime.factory,
      *receiverTime.initial,
      *receiverTime.epsilon,
      10U);

  std::vector<std::uint8_t> const firstValueBytes{
      0x41U, 0x54U, 0x54U, 0x52U, 0x2DU, 0x46U, 0x51U, 0x31U};
  std::vector<std::uint8_t> const secondValueBytes{
      0x41U, 0x54U, 0x54U, 0x52U, 0x2DU, 0x46U, 0x51U, 0x32U};
  std::vector<std::uint8_t> const firstTagBytes{0x46U, 0x51U, 0x41U, 0x31U};
  std::vector<std::uint8_t> const secondTagBytes{0x46U, 0x51U, 0x41U, 0x32U};
  rti::AttributeHandleValueMap firstValues;
  firstValues.emplace(
      publisherAttribute,
      rti::VariableLengthData(firstValueBytes.data(), firstValueBytes.size()));
  rti::AttributeHandleValueMap secondValues;
  secondValues.emplace(
      publisherAttribute,
      rti::VariableLengthData(secondValueBytes.data(), secondValueBytes.size()));
  rti::VariableLengthData firstTag(firstTagBytes.data(), firstTagBytes.size());
  rti::VariableLengthData secondTag(secondTagBytes.data(), secondTagBytes.size());

  auto const firstRetraction = publisher.rtiAmbassador().updateAttributeValues(
      object,
      firstValues,
      firstTag,
      *firstTime);
  auto const secondRetraction = publisher.rtiAmbassador().updateAttributeValues(
      object,
      secondValues,
      secondTag,
      *secondTime);
  require(
      firstRetraction.isValid() && secondRetraction.isValid(),
      "Timestamped attribute-update Flush Queue future-input update returned an invalid retraction handle");
  require(
      receiver.recorder().timedReflections().empty(),
      "Timestamped attribute-update Flush Queue future-input delivered before time-constrained servicing");
  receiver.recorder().clearCallbackOrder();
  publisher.recorder().clearCallbackOrder();
  receiver.rtiAmbassador().flushQueueRequest(*requestBoundary);
  if (model == rti::HLA_EVOKED) {
    require(
        receiver.recorder().timedReflections().empty(),
        "Timestamped attribute-update Flush Queue future-input delivered before callback servicing");
  }

  publisher.rtiAmbassador().timeAdvanceRequest(*publisherTarget);
  waitForSessions(
      {&publisher, &receiver},
      [&] {
        return publisher.recorder().timeAdvanceGrants().size() >= 1U &&
            receiver.recorder().timedReflections().size() >= 2U &&
            receiver.recorder().flushQueueGrants().size() >= 1U;
      },
      options,
      "Timestamped attribute-update Flush Queue future-input deliveries and grant");

  require(
      publisher.recorder().timeAdvanceGrants().size() == 1U,
      "Timestamped attribute-update Flush Queue future-input publisher received the wrong grant count");
  require(
      receiver.recorder().timedReflections().size() == 2U &&
          receiver.recorder().flushQueueGrants().size() == 1U &&
          receiver.recorder().timeAdvanceGrants().empty(),
      "Timestamped attribute-update Flush Queue future-input returned the wrong callback counts");
  require(
      receiver.recorder().callbackOrder() ==
          std::vector<std::string>{"reflect", "reflect", "flush-grant"},
      "Timestamped attribute-update Flush Queue future-input granted before queued delivery");
  require(
      publisher.recorder().timeAdvanceGrants().front().encoded ==
          encodeTime(*publisherTarget),
      "Timestamped attribute-update Flush Queue future-input publisher received the wrong grant time");

  auto assertReflection = [&](TimedReflectionRecord const& record,
                              rti::AttributeHandle const& expectedAttribute,
                              std::vector<std::uint8_t> const& expectedValue,
                              std::vector<std::uint8_t> const& expectedTag,
                              std::vector<std::uint8_t> const& expectedTime,
                              std::vector<std::uint8_t> const& expectedRetraction) {
    require(
        record.object == object &&
            record.values.size() == 1U &&
            record.values.count(expectedAttribute) == 1U,
        "Timestamped attribute-update Flush Queue future-input callback returned the wrong object or attribute set");
    require(
        copyBytes(record.values.at(expectedAttribute)) == expectedValue &&
            record.tag == expectedTag,
        "Timestamped attribute-update Flush Queue future-input callback did not preserve value or tag");
    require(
        record.producer == publisher.federateHandle() &&
            record.time == expectedTime &&
            !record.timeText.empty(),
        "Timestamped attribute-update Flush Queue future-input callback returned the wrong producer or time");
    require(
        record.transportation.isValid() &&
            !receiver.rtiAmbassador().getTransportationTypeName(record.transportation).empty(),
        "Timestamped attribute-update Flush Queue future-input callback returned an unresolvable transport");
    require(
        record.sentOrder == rti::TIMESTAMP && record.receivedOrder == rti::TIMESTAMP,
        "Timestamped attribute-update Flush Queue future-input callback returned the wrong order metadata");
    require(
        !record.regions.has_value(),
        "Timestamped attribute-update Flush Queue future-input callback unexpectedly supplied regions");
    require(
        record.retractionPresent && record.retraction == expectedRetraction,
        "Timestamped attribute-update Flush Queue future-input callback returned the wrong retraction handle");
  };
  auto const reflections = receiver.recorder().timedReflections();
  assertReflection(
      reflections.at(0),
      receiverAttribute,
      firstValueBytes,
      firstTagBytes,
      encodeTime(*firstTime),
      copyBytes(firstRetraction.encode()));
  assertReflection(
      reflections.at(1),
      receiverAttribute,
      secondValueBytes,
      secondTagBytes,
      encodeTime(*secondTime),
      copyBytes(secondRetraction.encode()));

  auto const flushGrants = receiver.recorder().flushQueueGrants();
  require(
      flushGrants.front().optimisticTime == encodeTime(*firstTime),
      "Timestamped attribute-update Flush Queue future-input grant returned the wrong optimistic frontier");
  require(
      !flushGrants.front().time.empty() &&
          !flushGrants.front().timeText.empty() &&
          !flushGrants.front().optimisticTimeText.empty(),
      "Timestamped attribute-update Flush Queue future-input grant returned incomplete time metadata");
  auto actualFlushTime = decodeTime(*receiverTime.factory, flushGrants.front().time);
  auto optimisticFlushTime =
      decodeTime(*receiverTime.factory, flushGrants.front().optimisticTime);
  require(
      actualFlushTime != nullptr && optimisticFlushTime != nullptr &&
          !(*actualFlushTime < *receiverTime.initial) &&
          !(*requestBoundary < *actualFlushTime) &&
          !(*optimisticFlushTime < *actualFlushTime),
      "Timestamped attribute-update Flush Queue future-input grant returned time outside standard bounds");
  auto queriedReceiverTime = receiverTime.factory->makeInitial();
  receiver.rtiAmbassador().queryLogicalTime(*queriedReceiverTime);
  require(
      encodeTime(*queriedReceiverTime) == flushGrants.front().time,
      "Timestamped attribute-update Flush Queue future-input query did not match its grant");

  receiver.rtiAmbassador().disableTimeConstrained();
  publisher.rtiAmbassador().disableTimeRegulation();
  receiver.rtiAmbassador().unsubscribeObjectClassAttributes(
      receiverClass,
      receiverAttributes);
  publisher.rtiAmbassador().unpublishObjectClassAttributes(
      publisherClass,
      publisherAttributes);
  receiver.resign(rti::NO_ACTION);
  publisher.resign(rti::NO_ACTION);
  publisher.rtiAmbassador().destroyFederationExecution(federation);
  receiver.disconnect();
  publisher.disconnect();
}

void scenarioFqrPostgrantLowerTsoRetraction(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.logicalTimeImplementationName.empty(),
      "FQR post-grant lower-TSO test requires an adapter-supplied logical-time implementation");

  Session publisher(options, model, "owner");
  Session receiver(options, model, "member");
  auto const federation = federationName(options, "fqr-postgrant-lower-tso-retraction");
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
      "FQR post-grant adapter FOM did not resolve interaction handles");
  require(
      publisherInteraction == receiverInteraction &&
          publisherParameter == receiverParameter,
      "FQR post-grant interaction handles changed across federates");
  publisher.rtiAmbassador().publishInteractionClass(publisherInteraction);
  publisher.rtiAmbassador().changeInteractionOrderType(
      publisherInteraction,
      rti::TIMESTAMP);
  receiver.rtiAmbassador().subscribeInteractionClass(receiverInteraction, true);

  auto publisherTime = makeTimeContext(publisher);
  auto receiverTime = makeTimeContext(receiver);
  require(
      publisherTime.factory->getName() == receiverTime.factory->getName(),
      "FQR post-grant members selected different logical-time factories");
  requireTimeEquals(
      *publisherTime.initial,
      *receiverTime.initial,
      "FQR post-grant initial logical time");

  receiver.rtiAmbassador().enableTimeConstrained();
  waitFor(
      receiver,
      [&] { return receiver.recorder().timeConstrainedEnabled().size() == 1U; },
      options,
      "FQR post-grant receiver Time Constrained callback");
  auto const lookaheadTime = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      5U);
  auto lookahead = publisherTime.factory->makeZero();
  require(lookahead != nullptr, "FQR post-grant could not allocate lookahead");
  lookahead->setToDifference(*lookaheadTime, *publisherTime.initial);
  publisher.rtiAmbassador().enableTimeRegulation(*lookahead);
  waitFor(
      publisher,
      [&] { return publisher.recorder().timeRegulationEnabled().size() == 1U; },
      options,
      "FQR post-grant publisher Time Regulation callback");
  auto observedLookahead = publisherTime.factory->makeZero();
  require(observedLookahead != nullptr, "FQR post-grant could not query lookahead");
  publisher.rtiAmbassador().queryLookahead(*observedLookahead);
  require(
      *observedLookahead == *lookahead,
      "FQR post-grant publisher did not retain its configured lookahead");

  auto const firstTime = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      12U);
  auto const rejectedTime = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      4U);
  auto const laterLowerTime = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      6U);
  auto const requestBoundary = timeAfter(
      *receiverTime.factory,
      *receiverTime.initial,
      *receiverTime.epsilon,
      10U);
  auto const expectedActualGrant = timeAfter(
      *receiverTime.factory,
      *receiverTime.initial,
      *receiverTime.epsilon,
      5U);
  std::vector<std::uint8_t> firstPayload{0x46U, 0x51U, 0x52U, 0x2DU, 0x31U};
  std::vector<std::uint8_t> laterPayload{0x46U, 0x51U, 0x52U, 0x2DU, 0x32U};
  std::vector<std::uint8_t> firstTag{0x46U, 0x51U, 0x52U, 0x2DU, 0x31U};
  std::vector<std::uint8_t> laterTag{0x46U, 0x51U, 0x52U, 0x2DU, 0x32U};
  rti::ParameterHandleValueMap firstParameters;
  firstParameters.emplace(
      publisherParameter,
      rti::VariableLengthData(firstPayload.data(), firstPayload.size()));
  rti::ParameterHandleValueMap laterParameters;
  laterParameters.emplace(
      publisherParameter,
      rti::VariableLengthData(laterPayload.data(), laterPayload.size()));
  rti::VariableLengthData firstUserTag(firstTag.data(), firstTag.size());
  rti::VariableLengthData laterUserTag(laterTag.data(), laterTag.size());

  auto const firstRetraction = publisher.rtiAmbassador().sendInteraction(
      publisherInteraction,
      firstParameters,
      firstUserTag,
      *firstTime);
  require(
      firstRetraction.isValid(),
      "FQR post-grant future interaction returned an invalid retraction handle");
  require(
      receiver.recorder().timedInteractions().empty(),
      "FQR post-grant future interaction was delivered before Flush Queue Request");
  receiver.recorder().clearCallbackOrder();
  receiver.rtiAmbassador().flushQueueRequest(*requestBoundary);
  waitFor(
      receiver,
      [&] {
        return receiver.recorder().timedInteractions().size() == 1U &&
            receiver.recorder().flushQueueGrants().size() == 1U;
      },
      options,
      "FQR post-grant first future interaction and grant");

  auto const firstDeliveries = receiver.recorder().timedInteractions();
  auto const firstFlushGrants = receiver.recorder().flushQueueGrants();
  require(
      receiver.recorder().callbackOrder() ==
          std::vector<std::string>{"interaction", "flush-grant"},
      "FQR post-grant delivered the first queued interaction after its grant");
  require(
      firstDeliveries.front().interaction == receiverInteraction &&
          firstDeliveries.front().parameters.size() == 1U &&
          firstDeliveries.front().parameters.count(receiverParameter) == 1U &&
          copyBytes(firstDeliveries.front().parameters.at(receiverParameter)) == firstPayload,
      "FQR post-grant first callback returned the wrong interaction or parameter value");
  require(
      firstDeliveries.front().time == encodeTime(*firstTime) &&
          firstDeliveries.front().tag == firstTag &&
          firstDeliveries.front().producer == publisher.federateHandle() &&
          firstDeliveries.front().sentOrder == rti::TIMESTAMP &&
          firstDeliveries.front().receivedOrder == rti::TIMESTAMP &&
          firstDeliveries.front().retractionPresent &&
          firstDeliveries.front().retraction == copyBytes(firstRetraction.encode()),
      "FQR post-grant first callback did not preserve timestamped interaction metadata");
  require(
      firstFlushGrants.front().time == encodeTime(*expectedActualGrant) &&
          firstFlushGrants.front().optimisticTime == encodeTime(*requestBoundary),
      "FQR post-grant first callback returned the wrong actual or optimistic time");
  auto receiverTimeAfterFirstFlush = receiverTime.factory->makeInitial();
  require(
      receiverTimeAfterFirstFlush != nullptr,
      "FQR post-grant could not allocate its first logical-time query");
  receiver.rtiAmbassador().queryLogicalTime(*receiverTimeAfterFirstFlush);
  requireTimeEquals(
      *receiverTimeAfterFirstFlush,
      *expectedActualGrant,
      "FQR post-grant first actual logical time");
  auto const belowOptimisticFloor = timeAfter(
      *receiverTime.factory,
      *receiverTimeAfterFirstFlush,
      *receiverTime.epsilon,
      1U);
  require(
      *expectedActualGrant < *belowOptimisticFloor &&
          *belowOptimisticFloor < *requestBoundary,
      "FQR post-grant advance probe did not distinguish actual and optimistic time");
  requireException(
      [&] { receiver.rtiAmbassador().timeAdvanceRequest(*belowOptimisticFloor); },
      L"LogicalTimeAlreadyPassed",
      "Time Advance Request after FQG below its optimistic logical time");
  requireException(
      [&] { receiver.rtiAmbassador().timeAdvanceRequestAvailable(*belowOptimisticFloor); },
      L"LogicalTimeAlreadyPassed",
      "Time Advance Request Available after FQG below its optimistic logical time");
  requireException(
      [&] { receiver.rtiAmbassador().nextMessageRequest(*belowOptimisticFloor); },
      L"LogicalTimeAlreadyPassed",
      "Next Message Request after FQG below its optimistic logical time");
  requireException(
      [&] { receiver.rtiAmbassador().nextMessageRequestAvailable(*belowOptimisticFloor); },
      L"LogicalTimeAlreadyPassed",
      "Next Message Request Available after FQG below its optimistic logical time");
  requireException(
      [&] { receiver.rtiAmbassador().flushQueueRequest(*belowOptimisticFloor); },
      L"LogicalTimeAlreadyPassed",
      "Flush Queue Request after FQG below its optimistic logical time");
  auto receiverTimeAfterRejectedAdvance = receiverTime.factory->makeInitial();
  receiver.rtiAmbassador().queryLogicalTime(*receiverTimeAfterRejectedAdvance);
  requireTimeEquals(
      *receiverTimeAfterRejectedAdvance,
      *expectedActualGrant,
      "rejected post-FQG advance changed the actual logical time");

  bool rejectedBelowLookahead = false;
  try {
    (void)publisher.rtiAmbassador().sendInteraction(
        publisherInteraction,
        firstParameters,
        firstUserTag,
        *rejectedTime);
  } catch (rti::InvalidLogicalTime const&) {
    rejectedBelowLookahead = true;
  }
  require(
      rejectedBelowLookahead,
      "FQR post-grant publisher accepted a timestamp below its current-time-plus-lookahead floor");
  require(
      receiver.recorder().timedInteractions().size() == 1U,
      "rejected FQR post-grant input produced a receiver callback");

  auto const laterLowerRetraction = publisher.rtiAmbassador().sendInteraction(
      publisherInteraction,
      laterParameters,
      laterUserTag,
      *laterLowerTime);
  require(
      laterLowerRetraction.isValid(),
      "FQR post-grant later lower TSO returned an invalid retraction handle");
  receiver.recorder().clearCallbackOrder();
  receiver.rtiAmbassador().flushQueueRequest(*requestBoundary);
  waitFor(
      receiver,
      [&] {
        return receiver.recorder().timedInteractions().size() == 2U &&
            receiver.recorder().flushQueueGrants().size() == 2U;
      },
      options,
      "FQR post-grant later lower TSO and second grant");
  auto const allDeliveries = receiver.recorder().timedInteractions();
  auto const allFlushGrants = receiver.recorder().flushQueueGrants();
  require(
      allDeliveries[0].time == encodeTime(*firstTime) &&
          allDeliveries[1].time == encodeTime(*laterLowerTime) &&
          allDeliveries[1].parameters.size() == 1U &&
          allDeliveries[1].parameters.count(receiverParameter) == 1U &&
          copyBytes(allDeliveries[1].parameters.at(receiverParameter)) == laterPayload &&
          allDeliveries[1].tag == laterTag &&
          allDeliveries[1].retractionPresent &&
          allDeliveries[1].retraction == copyBytes(laterLowerRetraction.encode()),
      "FQR post-grant did not deliver the later lower TSO with its own metadata");
  require(
      receiver.recorder().callbackOrder() ==
          std::vector<std::string>{"interaction", "flush-grant"},
      "FQR post-grant second delivery did not precede its grant");
  require(
      allFlushGrants.size() == 2U,
      "FQR post-grant returned the wrong number of flush grants");

  receiver.recorder().clearRetractions();
  receiver.recorder().clearCallbackOrder();
  publisher.rtiAmbassador().retract(firstRetraction);
  waitFor(
      receiver,
      [&] { return receiver.recorder().retractions().size() == 1U; },
      options,
      "FQR post-grant Request Retraction callback");
  auto const retractions = receiver.recorder().retractions();
  require(
      retractions.size() == 1U && retractions.front().valid &&
          retractions.front().encoded == copyBytes(firstRetraction.encode()),
      "FQR post-grant Request Retraction callback did not identify the delivered TSO");

  receiver.resign(rti::NO_ACTION);
  publisher.resign(rti::NO_ACTION);
  publisher.rtiAmbassador().destroyFederationExecution(federation);
  receiver.disconnect();
  publisher.disconnect();
}

void scenarioFqrPostgrantLowerTsoRetractionContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioFqrPostgrantLowerTsoRetraction(options, model);
}

void scenarioFqgPostgrantDeliveryCutoff(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.logicalTimeImplementationName.empty(),
      "FQG post-grant delivery cutoff requires an adapter-supplied logical-time implementation");

  Session firstPublisher(options, model, "owner");
  Session receiver(options, model, "member");
  Session latePublisher(options, model, "member");
  auto const federation = federationName(options, "fqg-postgrant-delivery-cutoff");
  firstPublisher.connect();
  receiver.connect();
  latePublisher.connect();
  firstPublisher.rtiAmbassador().createFederationExecution(
      federation,
      options.fom.wstring(),
      options.logicalTimeImplementationName);
  firstPublisher.join(
      options.ownerFederateName + L"-first-publisher",
      options.federateType,
      federation);
  receiver.join(
      options.memberFederateName + L"-receiver",
      options.federateType,
      federation);

  auto const firstClass = firstPublisher.rtiAmbassador().getInteractionClassHandle(
      options.interactionClassName);
  auto const firstParameter = firstPublisher.rtiAmbassador().getParameterHandle(
      firstClass,
      options.parameterName);
  auto const receiverClass = receiver.rtiAmbassador().getInteractionClassHandle(
      options.interactionClassName);
  auto const receiverParameter = receiver.rtiAmbassador().getParameterHandle(
      receiverClass,
      options.parameterName);
  require(
      firstClass.isValid() && firstParameter.isValid() &&
          receiverClass.isValid() && receiverParameter.isValid() &&
          firstClass == receiverClass && firstParameter == receiverParameter,
      "FQG adapter FOM did not resolve stable interaction handles");
  firstPublisher.rtiAmbassador().publishInteractionClass(firstClass);
  firstPublisher.rtiAmbassador().changeInteractionOrderType(firstClass, rti::TIMESTAMP);
  receiver.rtiAmbassador().subscribeInteractionClass(receiverClass, true);

  auto firstTime = makeTimeContext(firstPublisher);
  auto receiverTime = makeTimeContext(receiver);
  require(
      firstTime.factory->getName() == receiverTime.factory->getName(),
      "FQG initial members selected different logical-time factories");
  requireTimeEquals(
      *firstTime.initial,
      *receiverTime.initial,
      "FQG initial receiver logical time");

  receiver.rtiAmbassador().enableTimeConstrained();
  waitFor(
      receiver,
      [&] { return receiver.recorder().timeConstrainedEnabled().size() == 1U; },
      options,
      "FQG receiver Time Constrained callback");
  auto const firstLookaheadTime = timeAfter(
      *firstTime.factory,
      *firstTime.initial,
      *firstTime.epsilon,
      5U);
  auto firstLookahead = firstTime.factory->makeZero();
  require(firstLookahead != nullptr, "FQG could not allocate the first lookahead");
  firstLookahead->setToDifference(*firstLookaheadTime, *firstTime.initial);
  firstPublisher.rtiAmbassador().enableTimeRegulation(*firstLookahead);
  waitFor(
      firstPublisher,
      [&] { return firstPublisher.recorder().timeRegulationEnabled().size() == 1U; },
      options,
      "FQG first publisher Time Regulation callback");

  auto const timestampSeven = timeAfter(
      *firstTime.factory, *firstTime.initial, *firstTime.epsilon, 7U);
  auto const firstTargetTwo = timeAfter(
      *firstTime.factory, *firstTime.initial, *firstTime.epsilon, 2U);
  auto const requestBoundary = timeAfter(
      *receiverTime.factory, *receiverTime.initial, *receiverTime.epsilon, 10U);
  std::vector<std::uint8_t> firstPayload{0x46U, 0x51U, 0x47U, 0x2DU, 0x37U};
  std::vector<std::uint8_t> belowGrantPayload{0x46U, 0x51U, 0x47U, 0x2DU, 0x36U};
  std::vector<std::uint8_t> laterPayload{0x46U, 0x51U, 0x47U, 0x2DU, 0x38U};
  std::vector<std::uint8_t> firstTag{0x46U, 0x51U, 0x47U, 0x2DU, 0x37U};
  std::vector<std::uint8_t> belowGrantTag{0x46U, 0x51U, 0x47U, 0x2DU, 0x36U};
  std::vector<std::uint8_t> laterTag{0x46U, 0x51U, 0x47U, 0x2DU, 0x38U};
  rti::ParameterHandleValueMap firstValues;
  firstValues.emplace(
      firstParameter,
      rti::VariableLengthData(firstPayload.data(), firstPayload.size()));
  auto const firstRetraction = firstPublisher.rtiAmbassador().sendInteraction(
      firstClass,
      firstValues,
      rti::VariableLengthData(firstTag.data(), firstTag.size()),
      *timestampSeven);
  require(firstRetraction.isValid(), "FQG first timestamped interaction had no retraction handle");
  require(
      receiver.recorder().timedInteractions().empty(),
      "FQG first timestamped interaction was delivered before a request");
  receiver.recorder().clearCallbackOrder();
  firstPublisher.rtiAmbassador().timeAdvanceRequest(*firstTargetTwo);
  waitForSessions(
      {&firstPublisher},
      [&] { return firstPublisher.recorder().timeAdvanceGrants().size() == 1U; },
      options,
      "FQG first publisher TAR(2)");
  receiver.rtiAmbassador().flushQueueRequest(*requestBoundary);
  waitFor(
      receiver,
      [&] {
        return receiver.recorder().timedInteractions().size() == 1U &&
            receiver.recorder().flushQueueGrants().size() == 1U;
      },
      options,
      "FQG first timestamped delivery and grant");
  auto const firstDelivery = receiver.recorder().timedInteractions().front();
  auto const firstGrant = receiver.recorder().flushQueueGrants().front();
  require(
      firstDelivery.time == encodeTime(*timestampSeven) &&
          firstDelivery.parameters.size() == 1U &&
          firstDelivery.parameters.count(receiverParameter) == 1U &&
          copyBytes(firstDelivery.parameters.at(receiverParameter)) == firstPayload &&
          firstDelivery.tag == firstTag &&
          firstDelivery.producer == firstPublisher.federateHandle() &&
          firstDelivery.sentOrder == rti::TIMESTAMP &&
          firstDelivery.receivedOrder == rti::TIMESTAMP &&
          firstDelivery.retractionPresent &&
          firstDelivery.retraction == copyBytes(firstRetraction.encode()),
      "FQG first callback changed the queued timestamped interaction metadata");
  require(
      firstGrant.time == encodeTime(*timestampSeven),
      "FQG first actual grant time did not equal timestamp 7");
  require(
      receiver.recorder().callbackOrder() ==
          std::vector<std::string>{"interaction", "flush-grant"},
      "FQG first interaction callback did not precede Flush Queue Grant");

  latePublisher.join(
      options.ownerFederateName + L"-late-publisher",
      options.federateType,
      federation);
  auto const lateClass = latePublisher.rtiAmbassador().getInteractionClassHandle(
      options.interactionClassName);
  auto const lateParameter = latePublisher.rtiAmbassador().getParameterHandle(
      lateClass,
      options.parameterName);
  require(
      lateClass.isValid() && lateParameter.isValid() &&
          lateClass == receiverClass && lateParameter == receiverParameter,
      "FQG late publisher did not resolve the adapter interaction handles");
  latePublisher.rtiAmbassador().publishInteractionClass(lateClass);
  latePublisher.rtiAmbassador().changeInteractionOrderType(lateClass, rti::TIMESTAMP);
  auto lateTime = makeTimeContext(latePublisher);
  require(
      firstTime.factory->getName() == lateTime.factory->getName(),
      "FQG late publisher selected a different logical-time factory");
  requireTimeEquals(
      *firstTime.initial,
      *lateTime.initial,
      "FQG late publisher initial logical time");
  latePublisher.rtiAmbassador().enableTimeRegulation(*lateTime.epsilon);
  waitFor(
      latePublisher,
      [&] { return latePublisher.recorder().timeRegulationEnabled().size() == 1U; },
      options,
      "FQG late publisher Time Regulation callback");

  auto const timestampSix = timeAfter(
      *lateTime.factory, *lateTime.initial, *lateTime.epsilon, 6U);
  auto const timestampEight = timeAfter(
      *lateTime.factory, *lateTime.initial, *lateTime.epsilon, 8U);
  rti::ParameterHandleValueMap belowGrantValues;
  belowGrantValues.emplace(
      lateParameter,
      rti::VariableLengthData(belowGrantPayload.data(), belowGrantPayload.size()));
  rti::ParameterHandleValueMap laterValues;
  laterValues.emplace(
      lateParameter,
      rti::VariableLengthData(laterPayload.data(), laterPayload.size()));
  auto const belowGrantRetraction = latePublisher.rtiAmbassador().sendInteraction(
      lateClass,
      belowGrantValues,
      rti::VariableLengthData(belowGrantTag.data(), belowGrantTag.size()),
      *timestampSix);
  auto const laterRetraction = latePublisher.rtiAmbassador().sendInteraction(
      lateClass,
      laterValues,
      rti::VariableLengthData(laterTag.data(), laterTag.size()),
      *timestampEight);
  require(
      belowGrantRetraction.isValid() && laterRetraction.isValid(),
      "FQG late timestamped interactions returned invalid retraction handles");

  auto advanceAndWait = [&](Session& session,
                            rti::LogicalTime const& target,
                            std::size_t expectedCount,
                            std::string const& description) {
    session.rtiAmbassador().timeAdvanceRequest(target);
    waitFor(
        session,
        [&] { return session.recorder().timeAdvanceGrants().size() == expectedCount; },
        options,
        description);
  };
  auto const lateTargetTwo = timeAfter(
      *lateTime.factory, *lateTime.initial, *lateTime.epsilon, 2U);
  auto const lateTargetFour = timeAfter(
      *lateTime.factory, *lateTime.initial, *lateTime.epsilon, 4U);
  auto const lateTargetSix = timeAfter(
      *lateTime.factory, *lateTime.initial, *lateTime.epsilon, 6U);
  auto const lateTargetEight = timeAfter(
      *lateTime.factory, *lateTime.initial, *lateTime.epsilon, 8U);
  auto const lateTargetTen = timeAfter(
      *lateTime.factory, *lateTime.initial, *lateTime.epsilon, 10U);
  auto const firstTargetThree = timeAfter(
      *firstTime.factory, *firstTime.initial, *firstTime.epsilon, 3U);
  auto const firstTargetFive = timeAfter(
      *firstTime.factory, *firstTime.initial, *firstTime.epsilon, 5U);
  auto const firstTargetSeven = timeAfter(
      *firstTime.factory, *firstTime.initial, *firstTime.epsilon, 7U);
  auto const firstTargetEight = timeAfter(
      *firstTime.factory, *firstTime.initial, *firstTime.epsilon, 8U);
  advanceAndWait(latePublisher, *lateTargetTwo, 1U, "FQG late publisher TAR(2)");
  advanceAndWait(firstPublisher, *firstTargetThree, 2U, "FQG first publisher TAR(3)");
  advanceAndWait(latePublisher, *lateTargetFour, 2U, "FQG late publisher TAR(4)");
  advanceAndWait(firstPublisher, *firstTargetFive, 3U, "FQG first publisher TAR(5)");
  advanceAndWait(latePublisher, *lateTargetSix, 3U, "FQG late publisher TAR(6)");
  advanceAndWait(firstPublisher, *firstTargetSeven, 4U, "FQG first publisher TAR(7)");
  advanceAndWait(latePublisher, *lateTargetEight, 4U, "FQG late publisher TAR(8)");
  advanceAndWait(firstPublisher, *firstTargetEight, 5U, "FQG first publisher TAR(8)");
  advanceAndWait(latePublisher, *lateTargetTen, 5U, "FQG late publisher TAR(10)");

  auto const receiverTargetNine = timeAfter(
      *receiverTime.factory, *receiverTime.initial, *receiverTime.epsilon, 9U);
  receiver.rtiAmbassador().timeAdvanceRequestAvailable(*receiverTargetNine);
  waitFor(
      receiver,
      [&] { return receiver.recorder().timeAdvanceGrants().size() == 1U; },
      options,
      "FQG receiver TARA grant");
  auto const deliveries = receiver.recorder().timedInteractions();
  auto const grants = receiver.recorder().timeAdvanceGrants();
  auto const actualReceiverTime = receiverTime.factory->makeInitial();
  require(actualReceiverTime != nullptr, "FQG could not allocate receiver time query");
  receiver.rtiAmbassador().queryLogicalTime(*actualReceiverTime);
  auto const expectedSecondGrant = timeAfter(
      *receiverTime.factory, *receiverTime.initial, *receiverTime.epsilon, 8U);
  require(
      firstPublisher.recorder().timeAdvanceGrants().back().encoded == encodeTime(*firstTargetEight) &&
          latePublisher.recorder().timeAdvanceGrants().back().encoded == encodeTime(*lateTargetTen),
      "FQG time-regulating producers reached different final frontiers than requested");
  require(
      deliveries.size() == 2U &&
          deliveries[0].time == encodeTime(*timestampSeven) &&
          deliveries[0].producer == firstPublisher.federateHandle() &&
          copyBytes(deliveries[0].parameters.at(receiverParameter)) == firstPayload &&
          deliveries[1].time == encodeTime(*timestampEight) &&
          deliveries[1].producer == latePublisher.federateHandle() &&
          copyBytes(deliveries[1].parameters.at(receiverParameter)) == laterPayload,
      "FQG delivered a post-grant timestamp below 7 or lost the legal timestamp-8 control");
  require(
      grants.size() == 1U && grants.front().encoded == encodeTime(*expectedSecondGrant),
      "FQG TARA did not grant at the next deliverable timestamp");
  requireTimeEquals(
      *actualReceiverTime,
      *expectedSecondGrant,
      "FQG receiver logical time after TARA");
  require(
      receiver.recorder().callbackOrder() ==
          std::vector<std::string>{"interaction", "flush-grant", "interaction", "grant"},
      "FQG delivered the legal later interaction after its TARA grant");

  latePublisher.resign(rti::NO_ACTION);
  receiver.resign(rti::NO_ACTION);
  firstPublisher.resign(rti::CANCEL_THEN_DELETE_THEN_DIVEST);
  firstPublisher.rtiAmbassador().destroyFederationExecution(federation);
  latePublisher.disconnect();
  receiver.disconnect();
  firstPublisher.disconnect();
}

void scenarioFqgPostgrantDeliveryCutoffContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioFqgPostgrantDeliveryCutoff(options, model);
}

void scenarioTarQueuedTsoBeforeGrant(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.logicalTimeImplementationName.empty(),
      "TAR queued-TSO test requires an adapter-supplied logical-time implementation");

  Session publisher(options, model, "owner");
  Session receiver(options, model, "member");
  auto const federation = federationName(options, "tar-queued-tso-before-grant");
  connectAndJoin(publisher, receiver, options, federation, options.fom);

  auto const publisherClass = publisher.rtiAmbassador().getInteractionClassHandle(
      options.interactionClassName);
  auto const publisherParameter = publisher.rtiAmbassador().getParameterHandle(
      publisherClass,
      options.parameterName);
  auto const receiverClass = receiver.rtiAmbassador().getInteractionClassHandle(
      options.interactionClassName);
  auto const receiverParameter = receiver.rtiAmbassador().getParameterHandle(
      receiverClass,
      options.parameterName);
  require(
      publisherClass.isValid() && publisherParameter.isValid() &&
          receiverClass.isValid() && receiverParameter.isValid() &&
          publisherClass == receiverClass && publisherParameter == receiverParameter,
      "TAR adapter FOM did not resolve stable interaction handles");
  publisher.rtiAmbassador().publishInteractionClass(publisherClass);
  publisher.rtiAmbassador().changeInteractionOrderType(
      publisherClass,
      rti::TIMESTAMP);
  receiver.rtiAmbassador().subscribeInteractionClass(receiverClass, true);

  auto publisherTime = makeTimeContext(publisher);
  auto receiverTime = makeTimeContext(receiver);
  require(
      publisherTime.factory->getName() == receiverTime.factory->getName(),
      "TAR members selected different logical-time factories");
  requireTimeEquals(
      *publisherTime.initial,
      *receiverTime.initial,
      "TAR initial logical time");

  receiver.rtiAmbassador().enableTimeConstrained();
  waitFor(
      receiver,
      [&] { return receiver.recorder().timeConstrainedEnabled().size() == 1U; },
      options,
      "TAR receiver Time Constrained callback");
  auto const lookaheadTime = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      5U);
  auto lookahead = publisherTime.factory->makeZero();
  require(lookahead != nullptr, "TAR could not allocate publisher lookahead");
  lookahead->setToDifference(*lookaheadTime, *publisherTime.initial);
  publisher.rtiAmbassador().enableTimeRegulation(*lookahead);
  waitFor(
      publisher,
      [&] { return publisher.recorder().timeRegulationEnabled().size() == 1U; },
      options,
      "TAR publisher Time Regulation callback");

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
  auto const receiverTarget = timeAfter(
      *receiverTime.factory,
      *receiverTime.initial,
      *receiverTime.epsilon,
      6U);
  std::vector<std::uint8_t> payload{0x54U, 0x41U, 0x52U, 0x2DU, 0x35U};
  std::vector<std::uint8_t> tag{0x54U, 0x41U, 0x52U, 0x2DU, 0x35U};
  rti::ParameterHandleValueMap values;
  values.emplace(
      publisherParameter,
      rti::VariableLengthData(payload.data(), payload.size()));
  auto const retraction = publisher.rtiAmbassador().sendInteraction(
      publisherClass,
      values,
      rti::VariableLengthData(tag.data(), tag.size()),
      *messageTime);
  require(retraction.isValid(), "TAR queued timestamped interaction returned no retraction handle");
  require(
      receiver.recorder().timedInteractions().empty(),
      "TAR queued TSO was delivered before the receiver advance request");

  publisher.rtiAmbassador().timeAdvanceRequest(*publisherTarget);
  waitFor(
      publisher,
      [&] { return publisher.recorder().timeAdvanceGrants().size() == 1U; },
      options,
      "TAR regulator TAR(2) grant");
  require(
      receiver.recorder().timedInteractions().empty(),
      "TAR queued TSO was delivered before the receiver TAR was submitted");

  receiver.recorder().clearCallbackOrder();
  receiver.rtiAmbassador().timeAdvanceRequest(*receiverTarget);
  try {
    waitFor(
        receiver,
        [&] {
          return receiver.recorder().timedInteractions().size() == 1U &&
              receiver.recorder().timeAdvanceGrants().size() == 1U;
        },
        options,
        "TAR queued TSO and grant callbacks");
  } catch (std::runtime_error const& error) {
    std::string temporalState = " (temporal queries unavailable)";
    try {
      auto observedTime = receiverTime.factory->makeInitial();
      auto observedGalt = receiverTime.factory->makeInitial();
      auto observedLits = receiverTime.factory->makeInitial();
      if (observedTime && observedGalt && observedLits) {
        receiver.rtiAmbassador().queryLogicalTime(*observedTime);
        bool const hasGalt = receiver.rtiAmbassador().queryGALT(*observedGalt);
        bool const hasLits = receiver.rtiAmbassador().queryLITS(*observedLits);
        temporalState = " (logical time=" + toNarrow(observedTime->toString()) +
            ", GALT=" + (hasGalt ? toNarrow(observedGalt->toString()) : "undefined") +
            ", LITS=" + (hasLits ? toNarrow(observedLits->toString()) : "undefined") + ")";
      }
    } catch (std::exception const& queryError) {
      temporalState = " (temporal queries failed: " + std::string(queryError.what()) + ")";
    } catch (...) {
      temporalState = " (temporal queries failed with unknown exception)";
    }
    throw std::runtime_error(
        std::string(error.what()) + " (receive-order callbacks=" +
        std::to_string(receiver.recorder().interactions().size()) +
        ", timestamped callbacks=" +
        std::to_string(receiver.recorder().timedInteractions().size()) +
        ", grants=" +
        std::to_string(receiver.recorder().timeAdvanceGrants().size()) + ")" +
        temporalState);
  }

  auto const delivery = receiver.recorder().timedInteractions().front();
  auto const grant = receiver.recorder().timeAdvanceGrants().front();
  auto actualTime = receiverTime.factory->makeInitial();
  require(actualTime != nullptr, "TAR could not allocate receiver logical-time query");
  receiver.rtiAmbassador().queryLogicalTime(*actualTime);
  require(
      delivery.interaction == receiverClass &&
          delivery.parameters.size() == 1U &&
          delivery.parameters.count(receiverParameter) == 1U &&
          copyBytes(delivery.parameters.at(receiverParameter)) == payload &&
          delivery.tag == tag &&
          delivery.producer == publisher.federateHandle() &&
          delivery.time == encodeTime(*messageTime) &&
          delivery.sentOrder == rti::TIMESTAMP &&
          delivery.receivedOrder == rti::TIMESTAMP &&
          delivery.retractionPresent &&
          delivery.retraction == copyBytes(retraction.encode()),
      "TAR delivered the wrong queued timestamped interaction or metadata");
  require(
      grant.encoded == encodeTime(*receiverTarget),
      "TAR granted at a time other than its requested value");
  requireTimeEquals(*actualTime, *receiverTarget, "TAR queried receiver logical time");
  require(
      receiver.recorder().callbackOrder() ==
          std::vector<std::string>{"interaction", "grant"},
      "TAR delivered the queued TSO after its time-advance grant");

  receiver.resign(rti::NO_ACTION);
  publisher.resign(rti::CANCEL_THEN_DELETE_THEN_DIVEST);
  publisher.rtiAmbassador().destroyFederationExecution(federation);
  receiver.disconnect();
  publisher.disconnect();
}

void scenarioTarQueuedTsoBeforeGrantContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioTarQueuedTsoBeforeGrant(options, model);
}

void scenarioTimestampedInteractionCrossProducerOrder(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.logicalTimeImplementationName.empty(),
      "Timestamped interaction cross-producer order requires an adapter-supplied logical-time implementation");

  Session early(options, model, "owner");
  Session late(options, model, "member");
  Session receiver(options, model, "member");
  Session receiverPeer(options, model, "member");
  auto const federation = federationName(
      options,
      "timestamped-interaction-cross-producer-order");
  early.connect();
  late.connect();
  receiver.connect();
  receiverPeer.connect();
  early.rtiAmbassador().createFederationExecution(
      federation,
      options.fom.wstring(),
      options.logicalTimeImplementationName);
  early.join(
      options.ownerFederateName + L"-early",
      options.federateType,
      federation);
  auto const earlyHandle = early.federateHandle();
  require(earlyHandle.isValid(),
          "Timestamped interaction cross-producer early join returned an invalid handle");
  late.join(
      options.ownerFederateName + L"-late",
      options.federateType,
      federation);
  auto const lateHandle = late.federateHandle();
  require(lateHandle.isValid(),
          "Timestamped interaction cross-producer late join returned an invalid handle");
  receiver.join(
      options.memberFederateName + L"-receiver",
      options.federateType,
      federation);
  receiverPeer.join(
      options.memberFederateName + L"-receiver-peer",
      options.federateType,
      federation);
  require(
      receiver.federateHandle().isValid() && receiverPeer.federateHandle().isValid(),
      "Timestamped interaction cross-producer receiver join returned an invalid handle");

  auto const earlyInteraction = early.rtiAmbassador().getInteractionClassHandle(
      options.interactionClassName);
  auto const lateInteraction = late.rtiAmbassador().getInteractionClassHandle(
      options.interactionClassName);
  auto const receiverInteraction = receiver.rtiAmbassador().getInteractionClassHandle(
      options.interactionClassName);
  auto const receiverPeerInteraction = receiverPeer.rtiAmbassador().getInteractionClassHandle(
      options.interactionClassName);
  auto const earlyParameter = early.rtiAmbassador().getParameterHandle(
      earlyInteraction,
      options.parameterName);
  auto const lateParameter = late.rtiAmbassador().getParameterHandle(
      lateInteraction,
      options.parameterName);
  auto const receiverParameter = receiver.rtiAmbassador().getParameterHandle(
      receiverInteraction,
      options.parameterName);
  auto const receiverPeerParameter = receiverPeer.rtiAmbassador().getParameterHandle(
      receiverPeerInteraction,
      options.parameterName);
  require(
      earlyInteraction.isValid() && lateInteraction.isValid() &&
          receiverInteraction.isValid() && receiverPeerInteraction.isValid() &&
          earlyParameter.isValid() && lateParameter.isValid() &&
          receiverParameter.isValid() && receiverPeerParameter.isValid(),
      "Timestamped interaction cross-producer lookup returned an invalid handle");

  early.rtiAmbassador().publishInteractionClass(earlyInteraction);
  late.rtiAmbassador().publishInteractionClass(lateInteraction);
  early.rtiAmbassador().changeInteractionOrderType(earlyInteraction, rti::TIMESTAMP);
  late.rtiAmbassador().changeInteractionOrderType(lateInteraction, rti::TIMESTAMP);
  receiver.rtiAmbassador().subscribeInteractionClass(receiverInteraction, true);
  receiverPeer.rtiAmbassador().subscribeInteractionClass(receiverPeerInteraction, true);

  auto earlyTime = makeTimeContext(early);
  auto lateTime = makeTimeContext(late);
  auto receiverTime = makeTimeContext(receiver);
  auto receiverPeerTime = makeTimeContext(receiverPeer);
  require(
      earlyTime.factory->getName() == lateTime.factory->getName() &&
          earlyTime.factory->getName() == receiverTime.factory->getName() &&
          earlyTime.factory->getName() == receiverPeerTime.factory->getName(),
      "Timestamped interaction cross-producer members selected different logical-time factories");
  receiver.rtiAmbassador().enableTimeConstrained();
  waitFor(
      receiver,
      [&] { return receiver.recorder().timeConstrainedEnabled().size() >= 1U; },
      options,
      "Timestamped interaction cross-producer receiver time-constrained callback");
  receiverPeer.rtiAmbassador().enableTimeConstrained();
  waitFor(
      receiverPeer,
      [&] { return receiverPeer.recorder().timeConstrainedEnabled().size() >= 1U; },
      options,
      "Timestamped interaction cross-producer peer time-constrained callback");
  early.rtiAmbassador().enableTimeRegulation(*earlyTime.epsilon);
  waitFor(
      early,
      [&] { return early.recorder().timeRegulationEnabled().size() >= 1U; },
      options,
      "Timestamped interaction cross-producer early time-regulation callback");
  late.rtiAmbassador().enableTimeRegulation(*lateTime.epsilon);
  waitFor(
      late,
      [&] { return late.recorder().timeRegulationEnabled().size() >= 1U; },
      options,
      "Timestamped interaction cross-producer late time-regulation callback");

  auto const timeFive = timeAfter(
      *earlyTime.factory,
      *earlyTime.initial,
      *earlyTime.epsilon,
      5U);
  auto const timeSeven = timeAfter(
      *earlyTime.factory,
      *earlyTime.initial,
      *earlyTime.epsilon,
      7U);
  auto const receiverFive = timeAfter(
      *receiverTime.factory,
      *receiverTime.initial,
      *receiverTime.epsilon,
      5U);
  auto const receiverSeven = timeAfter(
      *receiverTime.factory,
      *receiverTime.initial,
      *receiverTime.epsilon,
      7U);
  auto const receiverPeerFive = timeAfter(
      *receiverPeerTime.factory,
      *receiverPeerTime.initial,
      *receiverPeerTime.epsilon,
      5U);
  auto const receiverPeerSeven = timeAfter(
      *receiverPeerTime.factory,
      *receiverPeerTime.initial,
      *receiverPeerTime.epsilon,
      7U);
  auto const timeEight = timeAfter(
      *earlyTime.factory,
      *earlyTime.initial,
      *earlyTime.epsilon,
      8U);
  auto const lateEight = timeAfter(
      *lateTime.factory,
      *lateTime.initial,
      *lateTime.epsilon,
      8U);
  auto const receiverEight = timeAfter(
      *receiverTime.factory,
      *receiverTime.initial,
      *receiverTime.epsilon,
      8U);
  auto const timeNine = timeAfter(
      *earlyTime.factory,
      *earlyTime.initial,
      *earlyTime.epsilon,
      9U);
  auto const receiverNine = timeAfter(
      *receiverTime.factory,
      *receiverTime.initial,
      *receiverTime.epsilon,
      9U);

  auto const earlyObjectClass = early.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const earlyAttribute = early.rtiAmbassador().getAttributeHandle(
      earlyObjectClass,
      options.attributeName);
  auto const receiverObjectClass = receiver.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const receiverAttribute = receiver.rtiAmbassador().getAttributeHandle(
      receiverObjectClass,
      options.attributeName);
  require(
      earlyObjectClass.isValid() && earlyAttribute.isValid() &&
          receiverObjectClass.isValid() && receiverAttribute.isValid(),
      "Timestamped cross-producer attribute-handle lookup returned an invalid handle");
  rti::AttributeHandleSet const earlyAttributes{earlyAttribute};
  rti::AttributeHandleSet const receiverAttributes{receiverAttribute};
  early.rtiAmbassador().publishObjectClassAttributes(
      earlyObjectClass,
      earlyAttributes);
  early.rtiAmbassador().changeDefaultAttributeOrderType(
      earlyObjectClass,
      earlyAttributes,
      rti::TIMESTAMP);
  receiver.rtiAmbassador().subscribeObjectClassAttributes(
      receiverObjectClass,
      receiverAttributes);
  auto const object = early.rtiAmbassador().registerObjectInstance(earlyObjectClass);
  require(
      object.isValid(),
      "Timestamped cross-producer attribute-handle check registered an invalid object");
  waitFor(
      receiver,
      [&] { return receiver.recorder().hasDiscovery(object); },
      options,
      "Timestamped cross-producer attribute receiver discovery");
  auto const deletionObject = early.rtiAmbassador().registerObjectInstance(earlyObjectClass);
  require(
      deletionObject.isValid(),
      "Timestamped cross-producer deletion registration returned an invalid object handle");
  waitFor(
      receiver,
      [&] { return receiver.recorder().hasDiscovery(deletionObject); },
      options,
      "Timestamped cross-producer deletion receiver discovery");

  std::vector<std::uint8_t> const valueBytes{0x53U, 0x54U, 0x4FU};
  std::vector<std::uint8_t> const earlyTag{0x45U, 0x41U};
  std::vector<std::uint8_t> const lateTag{0x4CU, 0x41U};
  std::vector<std::uint8_t> const lateEqualTag{0x4CU, 0x45U};
  std::vector<std::uint8_t> const attributeValueBytes{0x41U};
  std::vector<std::uint8_t> const attributeTagBytes{0x41U, 0x54U};
  std::vector<std::uint8_t> const deletionTagBytes{0x44U, 0x45U};
  rti::ParameterHandleValueMap earlyParameters;
  earlyParameters.emplace(
      earlyParameter,
      rti::VariableLengthData(valueBytes.data(), valueBytes.size()));
  rti::ParameterHandleValueMap lateParameters;
  lateParameters.emplace(
      lateParameter,
      rti::VariableLengthData(valueBytes.data(), valueBytes.size()));
  rti::VariableLengthData earlyTagData(earlyTag.data(), earlyTag.size());
  rti::VariableLengthData lateTagData(lateTag.data(), lateTag.size());
  rti::VariableLengthData lateEqualTagData(
      lateEqualTag.data(),
      lateEqualTag.size());

  auto const lateMessage = late.rtiAmbassador().sendInteraction(
      lateInteraction,
      lateParameters,
      lateTagData,
      *timeSeven);
  auto const earlyMessage = early.rtiAmbassador().sendInteraction(
      earlyInteraction,
      earlyParameters,
      earlyTagData,
      *timeFive);
  auto const lateEqualMessage = late.rtiAmbassador().sendInteraction(
      lateInteraction,
      lateParameters,
      lateEqualTagData,
      *timeFive);
  rti::AttributeHandleValueMap attributeValues;
  attributeValues.emplace(
      earlyAttribute,
      rti::VariableLengthData(
          attributeValueBytes.data(),
          attributeValueBytes.size()));
  rti::VariableLengthData attributeTagData(
      attributeTagBytes.data(),
      attributeTagBytes.size());
  auto const attributeMessage = early.rtiAmbassador().updateAttributeValues(
      object,
      attributeValues,
      attributeTagData,
      *timeEight);
  rti::VariableLengthData deletionTagData(
      deletionTagBytes.data(),
      deletionTagBytes.size());
  auto const deletionMessage = early.rtiAmbassador().deleteObjectInstance(
      deletionObject,
      deletionTagData,
      *timeNine);
  require(
      lateMessage.isValid() && earlyMessage.isValid() && lateEqualMessage.isValid() &&
          attributeMessage.isValid() && deletionMessage.isValid(),
      "Timestamped cross-service send returned an invalid retraction handle");
  require(
      !(lateMessage == earlyMessage) &&
          !(lateMessage == lateEqualMessage) &&
          !(earlyMessage == lateEqualMessage) &&
          !(lateMessage == attributeMessage) &&
          !(earlyMessage == attributeMessage) &&
          !(lateEqualMessage == attributeMessage) &&
          !(lateMessage == deletionMessage) &&
          !(earlyMessage == deletionMessage) &&
          !(lateEqualMessage == deletionMessage) &&
          !(attributeMessage == deletionMessage),
      "Timestamped interaction, attribute-update, and object-deletion services returned non-unique federation retraction handles");
  require(
      receiver.recorder().timedInteractions().empty() &&
          receiverPeer.recorder().timedInteractions().empty() &&
          receiver.recorder().timedReflections().empty() &&
          receiver.recorder().timedRemovals().empty() &&
          early.recorder().timedRemovals().empty(),
      "Timestamped interaction cross-producer delivered before a recipient request");

  early.rtiAmbassador().timeAdvanceRequest(*timeSeven);
  late.rtiAmbassador().timeAdvanceRequest(*timeSeven);
  receiver.rtiAmbassador().timeAdvanceRequest(*receiverFive);
  receiverPeer.rtiAmbassador().timeAdvanceRequest(*receiverPeerFive);
  waitForSessions(
      {&early, &late, &receiver, &receiverPeer},
      [&] {
        return receiver.recorder().timedInteractions().size() >= 2U &&
            receiver.recorder().timeAdvanceGrants().size() >= 1U &&
            receiverPeer.recorder().timedInteractions().size() >= 2U &&
            receiverPeer.recorder().timeAdvanceGrants().size() >= 1U;
      },
      options,
      "Timestamped interaction cross-producer first boundary");
  require(
      receiver.recorder().timedInteractions().size() == 2U &&
          receiver.recorder().timeAdvanceGrants().size() == 1U &&
          receiver.recorder().callbackOrder() ==
              std::vector<std::string>{"interaction", "interaction", "grant"},
      "Timestamped interaction cross-producer receiver first boundary was unordered");
  require(
      receiverPeer.recorder().timedInteractions().size() == 2U &&
          receiverPeer.recorder().timeAdvanceGrants().size() == 1U &&
          receiverPeer.recorder().callbackOrder() ==
              std::vector<std::string>{"interaction", "interaction", "grant"},
      "Timestamped interaction cross-producer peer first boundary was unordered");

  receiver.rtiAmbassador().timeAdvanceRequest(*receiverSeven);
  receiverPeer.rtiAmbassador().timeAdvanceRequest(*receiverPeerSeven);
  waitForSessions(
      {&early, &late, &receiver, &receiverPeer},
      [&] {
        return receiver.recorder().timedInteractions().size() >= 3U &&
            receiver.recorder().timeAdvanceGrants().size() >= 2U &&
            receiverPeer.recorder().timedInteractions().size() >= 3U &&
            receiverPeer.recorder().timeAdvanceGrants().size() >= 2U;
      },
      options,
      "Timestamped interaction cross-producer second boundary");
  waitFor(
      early,
      [&] { return early.recorder().timeAdvanceGrants().size() == 1U; },
      options,
      "Timestamped interaction cross-producer early sender grant at time seven");
  require(
      early.recorder().timeAdvanceGrants().front().encoded == encodeTime(*timeSeven),
      "Timestamped interaction cross-producer early sender received the wrong time-seven grant");
  waitFor(
      late,
      [&] { return late.recorder().timeAdvanceGrants().size() == 1U; },
      options,
      "Timestamped interaction cross-producer late sender grant at time seven");
  require(
      late.recorder().timeAdvanceGrants().front().encoded == encodeTime(*timeSeven),
      "Timestamped interaction cross-producer late sender received the wrong time-seven grant");
  require(
      receiver.recorder().timedInteractions().size() == 3U &&
          receiver.recorder().timeAdvanceGrants().size() == 2U &&
          receiver.recorder().callbackOrder() ==
              std::vector<std::string>{"interaction", "interaction", "grant", "interaction", "grant"},
      "Timestamped interaction cross-producer receiver second boundary was unordered");
  require(
      receiverPeer.recorder().timedInteractions().size() == 3U &&
          receiverPeer.recorder().timeAdvanceGrants().size() == 2U &&
          receiverPeer.recorder().callbackOrder() ==
              std::vector<std::string>{"interaction", "interaction", "grant", "interaction", "grant"},
      "Timestamped interaction cross-producer peer second boundary was unordered");

  auto assertCommon = [&](TimedInteractionRecord const& record,
                          rti::InteractionClassHandle const& expectedInteraction,
                          rti::ParameterHandle const& expectedParameter,
                          Session& inspector) {
    require(
        record.interaction == expectedInteraction &&
            record.parameters.size() == 1U &&
            record.parameters.count(expectedParameter) == 1U &&
            copyBytes(record.parameters.at(expectedParameter)) == valueBytes,
        "Timestamped interaction cross-producer callback returned the wrong payload");
    require(
        record.transportation.isValid() &&
            !inspector.rtiAmbassador().getTransportationTypeName(record.transportation).empty(),
        "Timestamped interaction cross-producer callback returned an unresolvable transport");
    require(
        record.timeText.size() != 0U &&
            record.sentOrder == rti::TIMESTAMP &&
            record.receivedOrder == rti::TIMESTAMP &&
            record.retractionPresent,
        "Timestamped interaction cross-producer callback returned incomplete metadata");
    require(
        !record.regions.has_value(),
        "Timestamped interaction cross-producer callback unexpectedly supplied regions");
  };
  auto assertOrder = [&](std::vector<TimedInteractionRecord> const& records,
                         Session& inspector) {
    require(records.size() == 3U,
            "Timestamped interaction cross-producer callback count changed during validation");
    auto const& first = records.at(0);
    auto const& second = records.at(1);
    auto const& third = records.at(2);
    require(
        first.time == encodeTime(*timeFive) &&
            second.time == encodeTime(*timeFive) &&
            third.time == encodeTime(*timeSeven),
        "Timestamped interaction cross-producer delivery did not sort by timestamp");
    require(
        ((first.producer == earlyHandle && second.producer == lateHandle) ||
         (first.producer == lateHandle && second.producer == earlyHandle)) &&
            third.producer == lateHandle,
        "Timestamped interaction cross-producer delivery returned the wrong producers");
    require(
        ((first.tag == earlyTag && second.tag == lateEqualTag) ||
         (first.tag == lateEqualTag && second.tag == earlyTag)) &&
            third.tag == lateTag,
        "Timestamped interaction cross-producer delivery returned the wrong tag ordering");
    assertCommon(first, receiverInteraction, receiverParameter, inspector);
    assertCommon(second, receiverInteraction, receiverParameter, inspector);
    assertCommon(third, receiverInteraction, receiverParameter, inspector);
  };
  assertOrder(receiver.recorder().timedInteractions(), receiver);
  assertOrder(receiverPeer.recorder().timedInteractions(), receiverPeer);

  require(
      receiver.recorder().timeAdvanceGrants().back().encoded == encodeTime(*receiverSeven) &&
          receiverPeer.recorder().timeAdvanceGrants().back().encoded ==
              encodeTime(*receiverPeerSeven),
      "Timestamped interaction cross-producer returned the wrong final grants");

  receiver.rtiAmbassador().timeAdvanceRequest(*receiverEight);
  waitFor(
      receiver,
      [&] {
        return receiver.recorder().timedReflections().size() == 1U &&
            receiver.recorder().timeAdvanceGrants().size() == 3U;
      },
      options,
      "Timestamped cross-service attribute delivery at time eight");
  auto const attributeReflections = receiver.recorder().timedReflections();
  auto const& attributeReflection = attributeReflections.front();
  require(
      attributeReflection.object == object &&
          attributeReflection.values.size() == 1U &&
          attributeReflection.values.count(receiverAttribute) == 1U &&
          copyBytes(attributeReflection.values.at(receiverAttribute)) == attributeValueBytes &&
          attributeReflection.tag == attributeTagBytes &&
          attributeReflection.producer == earlyHandle &&
          attributeReflection.time == encodeTime(*timeEight) &&
          attributeReflection.sentOrder == rti::TIMESTAMP &&
          attributeReflection.receivedOrder == rti::TIMESTAMP &&
          attributeReflection.retractionPresent &&
          attributeReflection.retraction == copyBytes(attributeMessage.encode()),
      "Timestamped cross-service attribute reflection returned incorrect metadata");
  auto const callbackOrder = receiver.recorder().callbackOrder();
  require(
      callbackOrder.size() == 7U && callbackOrder.at(5U) == "reflect" &&
          callbackOrder.at(6U) == "grant" &&
          receiver.recorder().timeAdvanceGrants().back().encoded == encodeTime(*receiverEight),
      "Timestamped cross-service attribute reflection was not delivered before its grant");

  early.rtiAmbassador().timeAdvanceRequest(*timeEight);
  late.rtiAmbassador().timeAdvanceRequest(*lateEight);
  waitFor(
      early,
      [&] { return early.recorder().timeAdvanceGrants().size() == 2U; },
      options,
      "Timestamped cross-service sender frontier at time eight");
  require(
      early.recorder().timeAdvanceGrants().back().encoded == encodeTime(*timeEight),
      "Timestamped cross-service sender advanced to the wrong frontier");
  waitFor(
      late,
      [&] { return late.recorder().timeAdvanceGrants().size() == 2U; },
      options,
      "Timestamped cross-service second sender frontier at time eight");
  require(
      late.recorder().timeAdvanceGrants().back().encoded == encodeTime(*lateEight),
      "Timestamped cross-service second sender advanced to the wrong frontier");

  receiver.rtiAmbassador().timeAdvanceRequest(*receiverNine);
  waitFor(
      receiver,
      [&] {
        return receiver.recorder().timedRemovals().size() == 1U &&
            receiver.recorder().timeAdvanceGrants().size() == 4U;
      },
      options,
      "Timestamped cross-service object deletion at time nine");
  auto const deletionRecords = receiver.recorder().timedRemovals();
  auto const& deletionRecord = deletionRecords.front();
  require(
      deletionRecord.object == deletionObject && deletionRecord.tag == deletionTagBytes &&
          deletionRecord.producer == earlyHandle &&
          deletionRecord.time == encodeTime(*timeNine) &&
          deletionRecord.sentOrder == rti::TIMESTAMP &&
          deletionRecord.receivedOrder == rti::TIMESTAMP &&
          deletionRecord.retractionPresent &&
          deletionRecord.retraction == copyBytes(deletionMessage.encode()),
      "Timestamped cross-service object-removal callback returned incorrect metadata");
  require(
      early.recorder().timedRemovals().empty() &&
          receiverPeer.recorder().timedRemovals().empty(),
      "Timestamped object deletion looped back to its sender or reached an unsubscribed peer");
  auto const deletionCallbackOrder = receiver.recorder().callbackOrder();
  require(
      deletionCallbackOrder.size() == 9U &&
          deletionCallbackOrder.at(7U) == "remove" &&
          deletionCallbackOrder.at(8U) == "grant" &&
          receiver.recorder().timeAdvanceGrants().back().encoded == encodeTime(*receiverNine),
      "Timestamped object removal was not delivered before its matching grant");

  receiver.rtiAmbassador().disableTimeConstrained();
  receiverPeer.rtiAmbassador().disableTimeConstrained();
  early.rtiAmbassador().disableTimeRegulation();
  late.rtiAmbassador().disableTimeRegulation();
  receiver.rtiAmbassador().unsubscribeInteractionClass(receiverInteraction);
  receiver.rtiAmbassador().unsubscribeObjectClassAttributes(
      receiverObjectClass,
      receiverAttributes);
  receiverPeer.rtiAmbassador().unsubscribeInteractionClass(receiverPeerInteraction);
  early.rtiAmbassador().unpublishInteractionClass(earlyInteraction);
  early.rtiAmbassador().unpublishObjectClassAttributes(
      earlyObjectClass,
      earlyAttributes);
  late.rtiAmbassador().unpublishInteractionClass(lateInteraction);
  receiverPeer.resign(rti::NO_ACTION);
  receiver.resign(rti::NO_ACTION);
  late.resign(rti::NO_ACTION);
  early.resign(rti::NO_ACTION);
  early.rtiAmbassador().destroyFederationExecution(federation);
  receiverPeer.disconnect();
  receiver.disconnect();
  late.disconnect();
  early.disconnect();
}

void scenarioTimestampedInteractionNoFanout(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.logicalTimeImplementationName.empty(),
      "Timestamped interaction no-fanout testing requires an adapter-supplied logical-time implementation");

  Session publisher(options, model, "owner");
  auto const federation = federationName(
      options,
      "timestamped-interaction-no-fanout");
  publisher.connect();
  publisher.rtiAmbassador().createFederationExecution(
      federation,
      options.fom.wstring(),
      options.logicalTimeImplementationName);
  publisher.join(options.ownerFederateName, options.federateType, federation);

  auto const interaction = publisher.rtiAmbassador().getInteractionClassHandle(
      options.interactionClassName);
  auto const parameter = publisher.rtiAmbassador().getParameterHandle(
      interaction,
      options.parameterName);
  require(
      interaction.isValid(),
      "timestamped interaction no-fanout lookup returned an invalid class handle");
  require(
      parameter.isValid(),
      "timestamped interaction no-fanout lookup returned an invalid parameter handle");

  publisher.rtiAmbassador().publishInteractionClass(interaction);
  publisher.rtiAmbassador().changeInteractionOrderType(
      interaction,
      rti::TIMESTAMP);

  auto const time = makeTimeContext(publisher);
  publisher.rtiAmbassador().enableTimeRegulation(*time.epsilon);
  waitFor(
      publisher,
      [&] { return publisher.recorder().timeRegulationEnabled().size() >= 1U; },
      options,
      "timestamped interaction no-fanout time-regulation callback");

  std::vector<std::uint8_t> const valueBytes{0x4EU, 0x4FU, 0x4EU, 0x45U};
  std::vector<std::uint8_t> const tagBytes{0x4EU, 0x4FU, 0x46U};
  rti::ParameterHandleValueMap parameters;
  parameters.emplace(
      parameter,
      rti::VariableLengthData(valueBytes.data(), valueBytes.size()));
  rti::VariableLengthData const tag(tagBytes.data(), tagBytes.size());
  auto const timestamp = timeAfter(
      *time.factory,
      *time.initial,
      *time.epsilon,
      3U);

  // There is deliberately no subscribing federate. The standard still
  // returns a retraction designator for the accepted timestamped send, but
  // there must be no receive or request-retraction callback fan-out.
  auto const retractable = publisher.rtiAmbassador().sendInteraction(
      interaction,
      parameters,
      tag,
      *timestamp);
  require(
      retractable.isValid(),
      "timestamped interaction no-fanout returned an invalid retraction handle");
  for (int pass = 0; pass != 8; ++pass) {
    publisher.pump();
  }
  require(
      publisher.recorder().timedInteractions().empty() &&
          publisher.recorder().retractions().empty(),
      "timestamped interaction no-fanout unexpectedly delivered a callback");

  publisher.rtiAmbassador().retract(retractable);
  requireException(
      [&] { publisher.rtiAmbassador().retract(retractable); },
      L"MessageCanNoLongerBeRetracted",
      "retracting an already retracted no-fanout interaction");

  auto const expired = publisher.rtiAmbassador().sendInteraction(
      interaction,
      parameters,
      tag,
      *timestamp);
  require(
      expired.isValid(),
      "timestamped interaction no-fanout returned an invalid second retraction handle");
  auto const beyondTimestamp = timeAfter(
      *time.factory,
      *timestamp,
      *time.epsilon,
      1U);
  publisher.rtiAmbassador().timeAdvanceRequest(*beyondTimestamp);
  waitFor(
      publisher,
      [&] { return publisher.recorder().timeAdvanceGrants().size() >= 1U; },
      options,
      "timestamped interaction no-fanout time-advance grant");
  require(
      publisher.recorder().timedInteractions().empty() &&
          publisher.recorder().retractions().empty(),
      "timestamped interaction no-fanout delivered a callback without a recipient");
  requireException(
      [&] { publisher.rtiAmbassador().retract(expired); },
      L"MessageCanNoLongerBeRetracted",
      "retracting an expired no-fanout interaction");

  publisher.rtiAmbassador().disableTimeRegulation();
  publisher.rtiAmbassador().unpublishInteractionClass(interaction);
  publisher.resign(rti::NO_ACTION);
  publisher.rtiAmbassador().destroyFederationExecution(federation);
  publisher.disconnect();
}

void scenarioTimestampedInteractionRetractionFanout(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.logicalTimeImplementationName.empty(),
      "Timestamped interaction retraction fanout requires an adapter-supplied logical-time implementation");

  std::string activeStage = "federation setup and declarations";
  try {

  Session publisher(options, model, "owner");
  Session immediate(options, model, "member");
  Session constrained(options, model, "member");
  auto const federation = federationName(
      options,
      "timestamped-interaction-retraction-fanout");
  activeStage = "connecting publisher";
  publisher.connect();
  activeStage = "connecting immediate receiver";
  immediate.connect();
  activeStage = "creating federation execution";
  publisher.rtiAmbassador().createFederationExecution(
      federation,
      options.fom.wstring(),
      options.logicalTimeImplementationName);
  activeStage = "joining publisher";
  publisher.join(options.ownerFederateName, options.federateType, federation);
  activeStage = "joining immediate receiver";
  immediate.join(options.memberFederateName, options.federateType, federation);
  activeStage = "joining constrained receiver";
  constrained.connect();
  constrained.join(
      options.memberFederateName + L"-timestamped-retraction-constrained",
      options.federateType,
      federation);

  activeStage = "resolving interaction and parameter handles";
  auto const publisherInteraction = publisher.rtiAmbassador().getInteractionClassHandle(
      options.interactionClassName);
  auto const immediateInteraction = immediate.rtiAmbassador().getInteractionClassHandle(
      options.interactionClassName);
  auto const constrainedInteraction = constrained.rtiAmbassador().getInteractionClassHandle(
      options.interactionClassName);
  auto const publisherParameter = publisher.rtiAmbassador().getParameterHandle(
      publisherInteraction,
      options.parameterName);
  auto const immediateParameter = immediate.rtiAmbassador().getParameterHandle(
      immediateInteraction,
      options.parameterName);
  auto const constrainedParameter = constrained.rtiAmbassador().getParameterHandle(
      constrainedInteraction,
      options.parameterName);
  require(
      publisherInteraction.isValid() && immediateInteraction.isValid() &&
          constrainedInteraction.isValid(),
      "timestamped interaction retraction fanout lookup returned an invalid class handle");
  require(
      publisherParameter.isValid() && immediateParameter.isValid() &&
          constrainedParameter.isValid(),
      "timestamped interaction retraction fanout lookup returned an invalid parameter handle");

  activeStage = "publishing interaction and selecting timestamp order";
  publisher.rtiAmbassador().publishInteractionClass(publisherInteraction);
  publisher.rtiAmbassador().changeInteractionOrderType(
      publisherInteraction,
      rti::TIMESTAMP);
  activeStage = "subscribing the immediate receiver";
  immediate.rtiAmbassador().subscribeInteractionClass(immediateInteraction, true);
  activeStage = "subscribing the constrained receiver";
  constrained.rtiAmbassador().subscribeInteractionClass(constrainedInteraction, true);

  activeStage = "creating time contexts and enabling constrained/regulation roles";
  auto publisherTime = makeTimeContext(publisher);
  auto constrainedTime = makeTimeContext(constrained);
  require(
      publisherTime.factory->getName() == constrainedTime.factory->getName(),
      "timestamped interaction retraction fanout members selected different logical-time factories");
  constrained.rtiAmbassador().enableTimeConstrained();
  waitFor(
      constrained,
      [&] { return constrained.recorder().timeConstrainedEnabled().size() >= 1U; },
      options,
      "timestamped interaction retraction fanout time-constrained callback");
  publisher.rtiAmbassador().enableTimeRegulation(*publisherTime.epsilon);
  waitFor(
      publisher,
      [&] { return publisher.recorder().timeRegulationEnabled().size() >= 1U; },
      options,
      "timestamped interaction retraction fanout time-regulation callback");

  activeStage = "first timestamped send and receive-order delivery";

  auto assertInteraction = [&](TimedInteractionRecord const& record,
                               rti::InteractionClassHandle const& expectedInteraction,
                               rti::ParameterHandle const& expectedParameter,
                               std::vector<std::uint8_t> const& expectedValue,
                               std::vector<std::uint8_t> const& expectedTag,
                               rti::LogicalTime const& expectedTime,
                               rti::MessageRetractionHandle const& expectedRetraction,
                               rti::OrderType expectedReceivedOrder,
                               Session& inspector) {
    require(
        record.interaction == expectedInteraction && record.parameters.size() == 1U &&
            record.parameters.count(expectedParameter) == 1U &&
            copyBytes(record.parameters.at(expectedParameter)) == expectedValue,
        "timestamped interaction retraction fanout returned the wrong interaction payload");
    require(
        record.tag == expectedTag && record.producer == publisher.federateHandle(),
        "timestamped interaction retraction fanout returned the wrong tag or producer");
    require(
        record.transportation.isValid() &&
            !inspector.rtiAmbassador().getTransportationTypeName(
                record.transportation).empty(),
        "timestamped interaction retraction fanout returned an unresolvable transport");
    require(
        record.time == encodeTime(expectedTime) && !record.timeText.empty() &&
            record.sentOrder == rti::TIMESTAMP &&
            record.receivedOrder == expectedReceivedOrder,
        "timestamped interaction retraction fanout returned the wrong time or order metadata");
    require(
        record.retractionPresent &&
            record.retraction == copyBytes(expectedRetraction.encode()),
        "timestamped interaction retraction fanout omitted the retraction handle");
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
  std::vector<std::uint8_t> const firstValueBytes{0x52U, 0x54U, 0x49U};
  std::vector<std::uint8_t> const firstTagBytes{0x46U, 0x41U, 0x4EU};
  rti::ParameterHandleValueMap firstParameters;
  firstParameters.emplace(
      publisherParameter,
      rti::VariableLengthData(firstValueBytes.data(), firstValueBytes.size()));
  rti::VariableLengthData firstTag(firstTagBytes.data(), firstTagBytes.size());
  auto const firstRetraction = publisher.rtiAmbassador().sendInteraction(
      publisherInteraction,
      firstParameters,
      firstTag,
      *firstTime);
  require(
      firstRetraction.isValid(),
      "timestamped interaction retraction fanout returned an invalid first retraction handle");
  waitFor(
      immediate,
      [&] { return immediate.recorder().timedInteractions().size() >= 1U; },
      options,
      "timestamped interaction retraction fanout immediate delivery");
  require(
      immediate.recorder().timedInteractions().size() == 1U &&
          constrained.recorder().timedInteractions().empty(),
      "timestamped interaction retraction fanout delivered the first copy to the wrong recipients");
  assertInteraction(
      immediate.recorder().timedInteractions().front(),
      immediateInteraction,
      immediateParameter,
      firstValueBytes,
      firstTagBytes,
      *firstTime,
      firstRetraction,
      rti::RECEIVE,
      immediate);

  activeStage = "retracting the delivered copy and notifying its receiver";
  publisher.rtiAmbassador().retract(firstRetraction);
  waitFor(
      immediate,
      [&] { return immediate.recorder().retractions().size() >= 1U; },
      options,
      "timestamped interaction retraction fanout delivered-copy callback");
  auto const firstRetractionCallbacks = immediate.recorder().retractions();
  require(
      firstRetractionCallbacks.size() == 1U && firstRetractionCallbacks.front().valid &&
          firstRetractionCallbacks.front().encoded == copyBytes(firstRetraction.encode()),
      "timestamped interaction retraction fanout returned the wrong delivered-copy callback");
  require(
      constrained.recorder().timedInteractions().empty() &&
          constrained.recorder().retractions().empty(),
      "timestamped interaction retraction fanout leaked the pending copy after retraction");
  require(
      immediate.recorder().callbackOrder() ==
          std::vector<std::string>{"interaction", "request-retraction"},
      "timestamped interaction retraction fanout returned the wrong delivered-copy callback order");

  activeStage = "advancing publisher and constrained receiver after retraction";
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
      "timestamped interaction retraction fanout first grants");
  require(
      constrained.recorder().timedInteractions().empty() &&
          constrained.recorder().retractions().empty(),
      "timestamped interaction retraction fanout delivered a retracted pending copy at its grant");

  activeStage = "resigning the constrained receiver";
  constrained.resign(rti::NO_ACTION);

  auto const secondTime = timeAfter(
      *publisherTime.factory,
      *firstTime,
      *publisherTime.epsilon,
      3U);
  std::vector<std::uint8_t> const secondValueBytes{0x53U, 0x45U, 0x43U};
  std::vector<std::uint8_t> const secondTagBytes{0x4FU, 0x4EU, 0x4CU, 0x59U};
  rti::ParameterHandleValueMap secondParameters;
  secondParameters.emplace(
      publisherParameter,
      rti::VariableLengthData(secondValueBytes.data(), secondValueBytes.size()));
  rti::VariableLengthData secondTag(secondTagBytes.data(), secondTagBytes.size());
  activeStage = "second timestamped send after constrained receiver resignation";
  auto const secondRetraction = publisher.rtiAmbassador().sendInteraction(
      publisherInteraction,
      secondParameters,
      secondTag,
      *secondTime);
  require(
      secondRetraction.isValid(),
      "timestamped interaction retraction fanout returned an invalid second retraction handle");
  waitFor(
      immediate,
      [&] { return immediate.recorder().timedInteractions().size() >= 2U; },
      options,
      "timestamped interaction retraction fanout second immediate delivery");
  auto const secondInteractions = immediate.recorder().timedInteractions();
  require(
      secondInteractions.size() == 2U,
      "timestamped interaction retraction fanout duplicated the second delivery");
  assertInteraction(
      secondInteractions.at(1),
      immediateInteraction,
      immediateParameter,
      secondValueBytes,
      secondTagBytes,
      *secondTime,
      secondRetraction,
      rti::RECEIVE,
      immediate);
  activeStage = "retracting the second delivered copy";
  publisher.rtiAmbassador().retract(secondRetraction);
  waitFor(
      immediate,
      [&] { return immediate.recorder().retractions().size() >= 2U; },
      options,
      "timestamped interaction retraction fanout second delivered-copy callback");
  auto const secondRetractionCallbacks = immediate.recorder().retractions();
  require(
      secondRetractionCallbacks.size() == 2U &&
          secondRetractionCallbacks.back().valid &&
          secondRetractionCallbacks.back().encoded == copyBytes(secondRetraction.encode()),
      "timestamped interaction retraction fanout returned the wrong second callback");

  publisher.rtiAmbassador().disableTimeRegulation();
  immediate.rtiAmbassador().unsubscribeInteractionClass(immediateInteraction);
  publisher.rtiAmbassador().unpublishInteractionClass(publisherInteraction);
  immediate.resign(rti::NO_ACTION);
  publisher.resign(rti::NO_ACTION);
  publisher.rtiAmbassador().destroyFederationExecution(federation);
  constrained.disconnect();
  immediate.disconnect();
  publisher.disconnect();
  } catch (rti::Exception const& error) {
    throw std::runtime_error(
        "timestamped interaction retraction fanout failed during " + activeStage + ": " +
        toNarrow(error.name()) + ": " + toNarrow(error.what()));
  }
}

}  // namespace hla_rti_cpp_tck::detail
