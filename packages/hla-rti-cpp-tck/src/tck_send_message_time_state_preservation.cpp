#include "tck_main_scenarios.hpp"

namespace hla_rti_cpp_tck::detail {

void scenarioSendMessageTimeStatePreservationGranted(
    Options const& options, rti::CallbackModel model) {
  Session sender(options, model, "owner");
  Session receiver(options, model, "member");
  auto const federation = federationName(options, "send-message-time-granted");
  connectAndJoin(sender, receiver, options, federation, options.fom);

  auto const senderInteraction = sender.rtiAmbassador().getInteractionClassHandle(
      options.interactionClassName);
  auto const receiverInteraction = receiver.rtiAmbassador().getInteractionClassHandle(
      options.interactionClassName);
  auto const senderParameter = sender.rtiAmbassador().getParameterHandle(
      senderInteraction, options.parameterName);
  auto const receiverParameter = receiver.rtiAmbassador().getParameterHandle(
      receiverInteraction, options.parameterName);
  require(
      senderInteraction.isValid() && receiverInteraction.isValid() &&
          senderParameter.isValid() && receiverParameter.isValid() &&
          senderInteraction == receiverInteraction && senderParameter == receiverParameter,
      "Time Granted Send Message interaction handles did not resolve consistently");

  sender.rtiAmbassador().publishInteractionClass(senderInteraction);
  receiver.rtiAmbassador().subscribeInteractionClass(receiverInteraction, true);
  receiver.rtiAmbassador().enableTimeConstrained();
  waitFor(receiver,
          [&] { return receiver.recorder().timeConstrainedEnabled().size() == 1U; },
          options,
          "Time Granted Send Message time-constrained callback");
  receiver.rtiAmbassador().enableAsynchronousDelivery();

  auto senderTime = makeTimeContext(sender);
  sender.rtiAmbassador().enableTimeRegulation(*senderTime.epsilon);
  waitFor(sender,
          [&] { return sender.recorder().timeRegulationEnabled().size() == 1U; },
          options,
          "Time Granted Send Message time-regulation callback");

  auto const firstGrant = timeAfter(
      *senderTime.factory, *senderTime.initial, *senderTime.epsilon, 10U);
  auto const secondGrant = timeAfter(
      *senderTime.factory, *senderTime.initial, *senderTime.epsilon, 20U);
  sender.rtiAmbassador().timeAdvanceRequest(*firstGrant);
  waitFor(sender,
          [&] { return sender.recorder().timeAdvanceGrants().size() == 1U; },
          options,
          "Time Granted Send Message initial grant");
  requireTimeEquals(
      *decodeTime(*senderTime.factory,
                  sender.recorder().timeAdvanceGrants().front().encoded),
      *firstGrant,
      "Time Granted Send Message initial grant time");

  auto queriedTime = senderTime.factory->makeInitial();
  sender.rtiAmbassador().queryLogicalTime(*queriedTime);
  requireTimeEquals(*queriedTime, *firstGrant,
                    "Time Granted Send Message pre-send logical time");

  std::vector<std::uint8_t> valueBytes{0x41U, 0x42U};
  std::vector<std::uint8_t> tagBytes{0x54U, 0x47U};
  rti::ParameterHandleValueMap parameters;
  parameters.emplace(senderParameter,
                     rti::VariableLengthData(valueBytes.data(), valueBytes.size()));
  rti::VariableLengthData tag(tagBytes.data(), tagBytes.size());
  sender.rtiAmbassador().sendInteraction(senderInteraction, parameters, tag);
  waitFor(receiver,
          [&] { return receiver.recorder().interactions().size() == 1U; },
          options,
          "Time Granted Send Message receive-order callback");

  auto const received = receiver.recorder().interactions().front();
  require(received.interaction == receiverInteraction &&
              received.parameters.size() == 1U &&
              received.parameters.count(receiverParameter) == 1U &&
              copyBytes(received.parameters.at(receiverParameter)) == valueBytes &&
              received.tag == tagBytes &&
              received.producer == sender.federateHandle(),
          "Time Granted Send Message delivered incorrect interaction data");
  require(receiver.recorder().timedInteractions().empty() &&
              receiver.recorder().timeAdvanceGrants().empty(),
          "Time Granted Send Message produced an unexpected timestamped delivery or grant");
  require(sender.recorder().timeAdvanceGrants().size() == 1U,
          "Send Message displaced or duplicated the sender's existing grant");
  sender.rtiAmbassador().queryLogicalTime(*queriedTime);
  requireTimeEquals(*queriedTime, *firstGrant,
                    "Time Granted Send Message post-send logical time");

  sender.rtiAmbassador().timeAdvanceRequest(*secondGrant);
  waitFor(sender,
          [&] { return sender.recorder().timeAdvanceGrants().size() == 2U; },
          options,
          "Time Granted Send Message subsequent grant");
  auto const grants = sender.recorder().timeAdvanceGrants();
  require(grants.size() == 2U,
          "Time Granted Send Message returned the wrong number of grants");
  auto secondGrantTime = decodeTime(*senderTime.factory, grants.back().encoded);
  require(secondGrantTime != nullptr,
          "Time Granted Send Message subsequent grant had invalid logical time");
  requireTimeEquals(*secondGrantTime, *secondGrant,
                    "Time Granted Send Message subsequent grant time");

  receiver.rtiAmbassador().disableAsynchronousDelivery();
  receiver.rtiAmbassador().disableTimeConstrained();
  receiver.rtiAmbassador().unsubscribeInteractionClass(receiverInteraction);
  sender.rtiAmbassador().unpublishInteractionClass(senderInteraction);
  receiver.resign(rti::NO_ACTION);
  sender.resign(rti::NO_ACTION);
  sender.rtiAmbassador().destroyFederationExecution(federation);
  receiver.disconnect();
  sender.disconnect();
}

void scenarioSendMessageTimeStatePreservationGrantedContract(
    Options const& options, rti::CallbackModel model) {
  scenarioSendMessageTimeStatePreservationGranted(options, model);
}

}  // namespace hla_rti_cpp_tck::detail
