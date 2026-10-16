#include "tck_main_scenarios.hpp"
#include "tck_time_coordinated_interaction.hpp"

namespace hla_rti_cpp_tck::detail {

void scenarioTimeCoordinatedInteraction(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.logicalTimeImplementationName.empty(),
      "time-coordinated interaction requires an adapter-supplied logical-time implementation");

  Session publisher(options, model, "owner");
  Session receiver(options, model, "member");
  auto const federation = federationName(options, "time-coordinated-interaction");
  connectAndJoin(publisher, receiver, options, federation, options.fom);

  auto const publisherInteraction =
      publisher.rtiAmbassador().getInteractionClassHandle(options.interactionClassName);
  auto const receiverInteraction =
      receiver.rtiAmbassador().getInteractionClassHandle(options.interactionClassName);
  auto const publisherParameter = publisher.rtiAmbassador().getParameterHandle(
      publisherInteraction, options.parameterName);
  auto const receiverParameter = receiver.rtiAmbassador().getParameterHandle(
      receiverInteraction, options.parameterName);
  require(
      publisherInteraction.isValid() && receiverInteraction.isValid() &&
          publisherParameter.isValid() && receiverParameter.isValid(),
      "time-coordinated interaction lookup returned invalid handles");

  publisher.rtiAmbassador().publishInteractionClass(publisherInteraction);
  publisher.rtiAmbassador().changeInteractionOrderType(
      publisherInteraction, rti::TIMESTAMP);
  receiver.rtiAmbassador().subscribeInteractionClass(receiverInteraction, true);

  auto publisherTime = makeTimeContext(publisher);
  auto receiverTime = makeTimeContext(receiver);
  require(
      publisherTime.factory->getName() == receiverTime.factory->getName(),
      "time-coordinated interaction members selected different logical-time factories");
  receiver.rtiAmbassador().enableTimeConstrained();
  waitFor(
      receiver,
      [&] { return receiver.recorder().timeConstrainedEnabled().size() == 1U; },
      options,
      "time-coordinated interaction time-constrained callback");
  publisher.rtiAmbassador().enableTimeRegulation(*publisherTime.epsilon);
  waitFor(
      publisher,
      [&] { return publisher.recorder().timeRegulationEnabled().size() == 1U; },
      options,
      "time-coordinated interaction time-regulation callback");

  auto const messageTime = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      5U);
  auto const earlierTime = timeAfter(
      *receiverTime.factory,
      *receiverTime.initial,
      *receiverTime.epsilon,
      2U);
  auto const earlierTimeForPublisher = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      2U);
  auto const messageTimeForReceiver = timeAfter(
      *receiverTime.factory,
      *receiverTime.initial,
      *receiverTime.epsilon,
      5U);
  std::vector<std::uint8_t> const value{0x54U, 0x49U, 0x4DU, 0x45U};
  std::vector<std::uint8_t> const tagBytes{0x43U, 0x4FU, 0x4FU, 0x52U};
  rti::ParameterHandleValueMap parameters;
  parameters.emplace(
      publisherParameter,
      rti::VariableLengthData(value.data(), value.size()));
  rti::VariableLengthData tag(tagBytes.data(), tagBytes.size());
  auto const retraction = publisher.rtiAmbassador().sendInteraction(
      publisherInteraction, parameters, tag, *messageTime);
  require(
      retraction.isValid(),
      "time-coordinated timestamped interaction returned an invalid retraction handle");

  publisher.rtiAmbassador().timeAdvanceRequest(*earlierTimeForPublisher);
  receiver.rtiAmbassador().timeAdvanceRequest(*earlierTime);
  waitFor(
      publisher,
      [&] { return publisher.recorder().timeAdvanceGrants().size() == 1U; },
      options,
      "time-coordinated interaction publisher pre-message grant");
  waitFor(
      receiver,
      [&] { return receiver.recorder().timeAdvanceGrants().size() == 1U; },
      options,
      "time-coordinated interaction pre-message grant");
  require(
      publisher.recorder().timeAdvanceGrants().front().encoded ==
          encodeTime(*earlierTimeForPublisher),
      "time-coordinated interaction publisher received the wrong pre-message grant");
  require(
      receiver.recorder().timeAdvanceGrants().front().encoded ==
          encodeTime(*earlierTime),
      "time-coordinated interaction received the wrong pre-message grant");
  require(
      receiver.recorder().timedInteractions().empty(),
      "timestamped interaction was delivered before its logical timestamp");

  publisher.rtiAmbassador().timeAdvanceRequest(*messageTime);
  receiver.rtiAmbassador().timeAdvanceRequest(*messageTimeForReceiver);
  waitFor(
      publisher,
      [&] { return publisher.recorder().timeAdvanceGrants().size() == 2U; },
      options,
      "time-coordinated interaction publisher message-time grant");
  waitFor(
      receiver,
      [&] {
        return receiver.recorder().timedInteractions().size() == 1U &&
            receiver.recorder().timeAdvanceGrants().size() == 2U;
      },
      options,
      "time-coordinated interaction delivery and matching grant");
  require(
      publisher.recorder().timeAdvanceGrants().back().encoded ==
          encodeTime(*messageTime),
      "time-coordinated interaction publisher received the wrong message-time grant");
  auto const deliveries = receiver.recorder().timedInteractions();
  require(
      deliveries.size() == 1U &&
          deliveries.front().interaction == receiverInteraction &&
          deliveries.front().parameters.size() == 1U &&
          deliveries.front().parameters.count(receiverParameter) == 1U &&
          copyBytes(deliveries.front().parameters.at(receiverParameter)) == value &&
          deliveries.front().tag == tagBytes &&
          deliveries.front().time == encodeTime(*messageTimeForReceiver) &&
          deliveries.front().sentOrder == rti::TIMESTAMP &&
          deliveries.front().receivedOrder == rti::TIMESTAMP,
      "time-coordinated interaction callback did not preserve message payload and logical time");
  require(
      receiver.recorder().timeAdvanceGrants().back().encoded ==
          encodeTime(*messageTimeForReceiver),
      "time-coordinated interaction received the wrong message-time grant");

  receiver.rtiAmbassador().unsubscribeInteractionClass(receiverInteraction);
  receiver.resign(rti::NO_ACTION);
  publisher.resign(rti::NO_ACTION);
  publisher.rtiAmbassador().destroyFederationExecution(federation);
  receiver.disconnect();
  publisher.disconnect();
}

}  // namespace hla_rti_cpp_tck::detail
