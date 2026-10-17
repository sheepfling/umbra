#include "tck_main_scenarios.hpp"

namespace hla_rti_cpp_tck::detail {

void scenarioUnregulatedSenderTimestampedInteraction(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.logicalTimeImplementationName.empty(),
      "Unregulated timestamped interaction requires an adapter-supplied logical-time implementation");

  Session publisher(options, model, "unregulated-timestamped-interaction-publisher");
  Session receiver(options, model, "unregulated-timestamped-interaction-receiver");
  Session regulator(options, model, "unregulated-timestamped-interaction-regulator");
  auto const federation = federationName(
      options,
      "unregulated-sender-timestamped-interaction");
  connectAndJoin(publisher, receiver, options, federation, options.fom);
  regulator.connect();
  regulator.join(L"unregulated-timestamped-interaction-regulator", options.federateType, federation);

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
      "Unregulated timestamped interaction resolved an invalid FOM handle");

  publisher.rtiAmbassador().publishInteractionClass(publisherInteraction);
  publisher.rtiAmbassador().changeInteractionOrderType(
      publisherInteraction,
      rti::TIMESTAMP);
  receiver.rtiAmbassador().subscribeInteractionClass(receiverInteraction, true);

  auto const publisherTime = makeTimeContext(publisher);
  auto const receiverTime = makeTimeContext(receiver);
  auto const regulatorTime = makeTimeContext(regulator);
  require(publisherTime.factory->getName() == receiverTime.factory->getName() &&
              publisherTime.factory->getName() == regulatorTime.factory->getName(),
          "Unregulated timestamped interaction selected different logical-time factories");
  receiver.rtiAmbassador().enableTimeConstrained();
  waitFor(
      receiver,
      [&] { return receiver.recorder().timeConstrainedEnabled().size() == 1U; },
      options,
      "unregulated timestamped interaction time-constrained callback");
  regulator.rtiAmbassador().enableTimeRegulation(*regulatorTime.epsilon);
  waitFor(
      regulator,
      [&] { return regulator.recorder().timeRegulationEnabled().size() >= 1U; },
      options,
      "unregulated timestamped interaction GALT regulator");

  auto const expectedGalt = timeAfter(
      *regulatorTime.factory,
      *regulatorTime.initial,
      *regulatorTime.epsilon,
      1U);
  auto senderGalt = publisherTime.factory->makeInitial();
  require(
      senderGalt != nullptr && publisher.rtiAmbassador().queryGALT(*senderGalt),
      "unregulated timestamped interaction sender did not observe a defined GALT");
  requireTimeEquals(
      *senderGalt,
      *expectedGalt,
      "unregulated timestamped interaction sender GALT");
  auto receiverPosition = receiverTime.factory->makeInitial();
  require(receiverPosition != nullptr,
          "logical-time factory returned no receiver position value");
  receiver.rtiAmbassador().queryLogicalTime(*receiverPosition);
  requireTimeEquals(
      *receiverPosition,
      *receiverTime.initial,
      "unregulated timestamped interaction receiver initial position");

  auto const timestamp = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      2U);
  auto const receiverTarget = timeAfter(
      *receiverTime.factory,
      *receiverTime.initial,
      *receiverTime.epsilon,
      2U);
  require(
      *timestamp == *receiverTarget && *expectedGalt < *timestamp,
      "timestamp was not later than the defined GALT and receiver's TAR target");
  std::vector<std::uint8_t> const parameterBytes{0x55U, 0x4EU, 0x52U};
  std::vector<std::uint8_t> const tagBytes{0x54U, 0x53U, 0x4FU};
  rti::ParameterHandleValueMap parameters;
  parameters.emplace(
      publisherParameter,
      rti::VariableLengthData(parameterBytes.data(), parameterBytes.size()));
  rti::VariableLengthData const tag(tagBytes.data(), tagBytes.size());
  publisher.rtiAmbassador().sendInteraction(
      publisherInteraction,
      parameters,
      tag,
      *timestamp);
  receiver.rtiAmbassador().timeAdvanceRequest(*receiverTarget);
  waitFor(
      regulator,
      receiver,
      [&] { return receiver.recorder().timedInteractions().size() == 1U; },
      options,
      "unregulated timestamped interaction delivery");
  auto const interactions = receiver.recorder().timedInteractions();
  require(
      interactions.size() == 1U,
      "unregulated timestamped interaction was not delivered once");
  auto const& interaction = interactions.front();
  require(
      interaction.interaction == receiverInteraction &&
          interaction.parameters.size() == 1U &&
          interaction.parameters.count(receiverParameter) == 1U &&
          copyBytes(interaction.parameters.at(receiverParameter)) == parameterBytes &&
          interaction.tag == tagBytes &&
          interaction.time == encodeTime(*timestamp),
      "RTI did not pass the unregulated sender's supplied interaction timestamp unchanged");

  receiver.rtiAmbassador().unsubscribeInteractionClass(receiverInteraction);
  publisher.rtiAmbassador().unpublishInteractionClass(publisherInteraction);
  receiver.resign(rti::NO_ACTION);
  regulator.resign(rti::NO_ACTION);
  publisher.resign(rti::NO_ACTION);
  publisher.rtiAmbassador().destroyFederationExecution(federation);
  receiver.disconnect();
  regulator.disconnect();
  publisher.disconnect();
}

void scenarioUnregulatedSenderTimestampedInteractionContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioUnregulatedSenderTimestampedInteraction(options, model);
}

}  // namespace hla_rti_cpp_tck::detail
