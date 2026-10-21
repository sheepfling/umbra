#include "tck_main_scenarios.hpp"

namespace hla_rti_cpp_tck::detail {

void scenarioFqrNonTimeConstrainedInTransitTso(
    Options const& options,
    rti::CallbackModel model) {
  Session publisher(options, model, "owner");
  Session receiver(options, model, "member");
  auto const federation = federationName(
      options,
      "fqr-non-time-constrained-in-transit-tso");

  publisher.connect();
  receiver.connect();
  publisher.rtiAmbassador().createFederationExecution(
      federation,
      options.fom.wstring(),
      options.logicalTimeImplementationName);
  publisher.join(options.ownerFederateName, options.federateType, federation);
  receiver.join(
      options.memberFederateName + L"-fqr-unconstrained",
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
      "non-time-constrained FQR interaction lookup returned an invalid handle");

  publisher.rtiAmbassador().publishInteractionClass(publisherInteraction);
  publisher.rtiAmbassador().changeInteractionOrderType(
      publisherInteraction,
      rti::TIMESTAMP);
  receiver.rtiAmbassador().subscribeInteractionClass(receiverInteraction);

  auto publisherTime = makeTimeContext(publisher);
  auto receiverTime = makeTimeContext(receiver);
  require(
      publisherTime.factory->getName() == receiverTime.factory->getName(),
      "non-time-constrained FQR members selected different logical-time factories");
  publisher.rtiAmbassador().enableTimeRegulation(*publisherTime.epsilon);
  waitFor(
      publisher,
      [&] { return publisher.recorder().timeRegulationEnabled().size() >= 1U; },
      options,
      "non-time-constrained FQR producer time-regulation callback");

  auto const messageTime = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      7U);
  auto const requestedTime = timeAfter(
      *receiverTime.factory,
      *receiverTime.initial,
      *receiverTime.epsilon,
      10U);
  std::vector<std::uint8_t> const payload{0x46U, 0x51U, 0x52U, 0x2DU, 0x4EU, 0x54U, 0x43U};
  std::vector<std::uint8_t> const tagBytes{0x46U, 0x51U, 0x52U};
  rti::VariableLengthData const tag(tagBytes.data(), tagBytes.size());
  rti::ParameterHandleValueMap parameters;
  parameters.emplace(
      publisherParameter,
      rti::VariableLengthData(payload.data(), payload.size()));

  auto const retraction = publisher.rtiAmbassador().sendInteraction(
      publisherInteraction,
      parameters,
      tag,
      *messageTime);
  require(retraction.isValid(),
          "timestamped interaction send returned an invalid retraction handle");

  // The receiver never enables Time Constrained. Keep the producer at its
  // initial time so its timestamp-7 send frontier cannot be advanced to 7.
  auto producerTimeBeforeGrant = publisherTime.factory->makeInitial();
  require(producerTimeBeforeGrant != nullptr,
          "logical-time factory returned no producer query value");
  publisher.rtiAmbassador().queryLogicalTime(*producerTimeBeforeGrant);
  require(producerTimeBeforeGrant->isInitial(),
          "producer advanced before the non-time-constrained FQR request");

  receiver.rtiAmbassador().flushQueueRequest(*requestedTime);
  waitFor(
      receiver,
      [&] {
        return receiver.recorder().timedInteractions().size() >= 1U &&
            receiver.recorder().flushQueueGrants().size() >= 1U;
      },
      options,
      "non-time-constrained FQR receive-order delivery and grant");

  auto const interactions = receiver.recorder().timedInteractions();
  auto const grants = receiver.recorder().flushQueueGrants();
  require(interactions.size() == 1U,
          "non-time-constrained FQR delivered the timestamped interaction more than once");
  require(grants.size() == 1U,
          "non-time-constrained FQR issued the wrong number of grants");
  auto const& interaction = interactions.front();
  require(interaction.interaction == receiverInteraction,
          "non-time-constrained FQR delivered the wrong interaction class");
  require(interaction.time == encodeTime(*messageTime),
          "non-time-constrained FQR changed the timestamped interaction time");
  require(interaction.sentOrder == rti::TIMESTAMP &&
              interaction.receivedOrder == rti::RECEIVE,
          "non-time-constrained FQR did not convert the interaction to receive order");
  auto const receivedParameter = interaction.parameters.find(receiverParameter);
  require(receivedParameter != interaction.parameters.end(),
          "non-time-constrained FQR omitted the interaction parameter");
  require(copyBytes(receivedParameter->second) == payload,
          "non-time-constrained FQR changed the interaction payload");
  require(interaction.tag == tagBytes,
          "non-time-constrained FQR changed the interaction tag");
  require(
      receiver.recorder().callbackOrder() ==
          std::vector<std::string>{"interaction", "flush-grant"},
      "non-time-constrained FQR grant preceded receive-order delivery");

  auto producerTimeAfterGrant = publisherTime.factory->makeInitial();
  require(producerTimeAfterGrant != nullptr,
          "logical-time factory returned no producer verification value");
  publisher.rtiAmbassador().queryLogicalTime(*producerTimeAfterGrant);
  require(producerTimeAfterGrant->isInitial(),
          "non-time-constrained FQR waited for the regulating producer to advance");

  receiver.resign(rti::NO_ACTION);
  publisher.resign(rti::NO_ACTION);
  publisher.rtiAmbassador().destroyFederationExecution(federation);
  receiver.disconnect();
  publisher.disconnect();
}

}  // namespace hla_rti_cpp_tck::detail
