#include "tck_tara_future_timestamped_interaction.hpp"

#include "tck_harness.hpp"
#include "tck_main_scenarios.hpp"

#include <chrono>
#include <string>
#include <thread>
#include <vector>

namespace hla_rti_cpp_tck::detail {
namespace {

void requireTaraInteractionPending(
    Session& receiver,
    std::string const& context) {
  auto const deadline = Clock::now() + std::chrono::milliseconds(40);
  while (Clock::now() < deadline) {
    receiver.pump();
    require(
        receiver.recorder().timedInteractions().empty() &&
            receiver.recorder().timeAdvanceGrants().empty(),
        context);
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
}

void runTaraFutureTimestampedInteraction(
    Options const& options,
    rti::CallbackModel model) {
  Session receiver(options, model, "owner");
  Session publisher(options, model, "member");
  auto const federation = federationName(
      options,
      "tara-future-timestamped-interaction");
  connectAndJoin(receiver, publisher, options, federation, options.fom);

  auto const receiverInteraction =
      receiver.rtiAmbassador().getInteractionClassHandle(
          options.interactionClassName);
  auto const publisherInteraction =
      publisher.rtiAmbassador().getInteractionClassHandle(
          options.interactionClassName);
  auto const receiverParameter = receiver.rtiAmbassador().getParameterHandle(
      receiverInteraction,
      options.parameterName);
  auto const publisherParameter = publisher.rtiAmbassador().getParameterHandle(
      publisherInteraction,
      options.parameterName);
  require(
      receiverInteraction.isValid() && publisherInteraction.isValid() &&
          receiverParameter.isValid() && publisherParameter.isValid() &&
          receiverInteraction == publisherInteraction &&
          receiverParameter == publisherParameter,
      "TARA future timestamped interaction resolved inconsistent FOM handles");

  receiver.rtiAmbassador().subscribeInteractionClass(
      receiverInteraction,
      true);
  publisher.rtiAmbassador().publishInteractionClass(publisherInteraction);
  publisher.rtiAmbassador().changeInteractionOrderType(
      publisherInteraction,
      rti::TIMESTAMP);

  auto receiverTime = makeTimeContext(receiver);
  auto publisherTime = makeTimeContext(publisher);
  require(
      receiverTime.factory->getName() == publisherTime.factory->getName(),
      "TARA future timestamped interaction participants selected different logical-time factories");
  requireTimeEquals(
      *receiverTime.initial,
      *publisherTime.initial,
      "TARA future timestamped interaction initial logical time");

  receiver.rtiAmbassador().enableTimeConstrained();
  waitFor(
      receiver,
      [&] { return receiver.recorder().timeConstrainedEnabled().size() == 1U; },
      options,
      "TARA future timestamped interaction receiver Time Constrained callback");
  publisher.rtiAmbassador().enableTimeRegulation(*publisherTime.epsilon);
  waitFor(
      publisher,
      [&] { return publisher.recorder().timeRegulationEnabled().size() == 1U; },
      options,
      "TARA future timestamped interaction publisher Time Regulation callback");

  auto const publisherCurrentTime = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      2U);
  publisher.rtiAmbassador().timeAdvanceRequest(*publisherCurrentTime);
  waitFor(
      publisher,
      [&] { return publisher.recorder().timeAdvanceGrants().size() == 1U; },
      options,
      "TARA future timestamped interaction publisher initial advance");
  require(
      publisher.recorder().timeAdvanceGrants().front().encoded ==
          encodeTime(*publisherCurrentTime),
      "TARA future timestamped interaction publisher initial grant was incorrect");

  auto const messageTime = timeAfter(
      *publisherTime.factory,
      *publisherCurrentTime,
      *publisherTime.epsilon,
      3U);
  auto const requestBoundary = timeAfter(
      *receiverTime.factory,
      *receiverTime.initial,
      *receiverTime.epsilon,
      6U);
  auto const publisherAdvanceTarget = timeAfter(
      *publisherTime.factory,
      *publisherCurrentTime,
      *publisherTime.epsilon,
      4U);

  receiver.recorder().clearCallbackOrder();
  receiver.rtiAmbassador().timeAdvanceRequestAvailable(*requestBoundary);
  requireTaraInteractionPending(
      receiver,
      "TARA completed before the future interaction arrived");

  std::vector<std::uint8_t> const parameterBytes{
      0x54U, 0x41U, 0x52U, 0x41U, 0x35U};
  std::vector<std::uint8_t> const tagBytes{
      0x54U, 0x41U, 0x52U, 0x41U};
  rti::ParameterHandleValueMap parameters;
  parameters.emplace(
      publisherParameter,
      rti::VariableLengthData(parameterBytes.data(), parameterBytes.size()));
  rti::VariableLengthData const tag(tagBytes.data(), tagBytes.size());
  auto const retraction = publisher.rtiAmbassador().sendInteraction(
      publisherInteraction,
      parameters,
      tag,
      *messageTime);
  require(
      retraction.isValid(),
      "TARA future timestamped interaction returned an invalid retraction handle");
  requireTaraInteractionPending(
      receiver,
      "TARA delivered the future interaction before its timestamp became safe");

  publisher.rtiAmbassador().timeAdvanceRequest(*publisherAdvanceTarget);
  waitForSessions(
      {&publisher, &receiver},
      [&] {
        return publisher.recorder().timeAdvanceGrants().size() == 2U &&
            receiver.recorder().timedInteractions().size() == 1U &&
            receiver.recorder().timeAdvanceGrants().size() == 1U;
      },
      options,
      "TARA future timestamped interaction delivery and grants");

  auto const interactions = receiver.recorder().timedInteractions();
  auto const receiverGrants = receiver.recorder().timeAdvanceGrants();
  auto const publisherGrants = publisher.recorder().timeAdvanceGrants();
  require(
      interactions.size() == 1U && receiverGrants.size() == 1U &&
          publisherGrants.size() == 2U,
      "TARA future timestamped interaction returned duplicate or missing callbacks");
  auto const& interaction = interactions.front();
  require(
      interaction.interaction == receiverInteraction &&
          interaction.parameters.size() == 1U &&
          interaction.parameters.count(receiverParameter) == 1U &&
          copyBytes(interaction.parameters.at(receiverParameter)) ==
              parameterBytes &&
          interaction.tag == tagBytes &&
          interaction.producer == publisher.federateHandle() &&
          interaction.time == encodeTime(*messageTime) &&
          !interaction.timeText.empty() &&
          interaction.sentOrder == rti::TIMESTAMP &&
          interaction.receivedOrder == rti::TIMESTAMP &&
          interaction.retractionPresent &&
          interaction.retraction == copyBytes(retraction.encode()) &&
          interaction.transportation.isValid(),
      "TARA future timestamped interaction changed callback identity or metadata");
  require(
      receiver.recorder().callbackOrder() ==
              std::vector<std::string>{"interaction", "grant"} &&
          receiverGrants.front().encoded == encodeTime(*requestBoundary),
      "TARA granted before the interaction or at the wrong requested time");
  require(
      publisherGrants.back().encoded == encodeTime(*publisherAdvanceTarget),
      "TARA future timestamped interaction publisher follow-up grant was incorrect");

  auto queriedTime = receiverTime.factory->makeInitial();
  require(
      queriedTime != nullptr,
      "TARA future timestamped interaction could not allocate Query Logical Time value");
  receiver.rtiAmbassador().queryLogicalTime(*queriedTime);
  requireTimeEquals(
      *queriedTime,
      *requestBoundary,
      "TARA future timestamped interaction final logical time");

  receiver.rtiAmbassador().disableTimeConstrained();
  publisher.rtiAmbassador().disableTimeRegulation();
  receiver.rtiAmbassador().unsubscribeInteractionClass(receiverInteraction);
  publisher.rtiAmbassador().unpublishInteractionClass(publisherInteraction);
  receiver.resign(rti::NO_ACTION);
  publisher.resign(rti::NO_ACTION);
  receiver.rtiAmbassador().destroyFederationExecution(federation);
  receiver.disconnect();
  publisher.disconnect();
}

}  // namespace

void scenarioTaraFutureTimestampedInteraction(
    Options const& options,
    rti::CallbackModel model) {
  runTaraFutureTimestampedInteraction(options, model);
}

void scenarioTaraFutureTimestampedInteractionContract(
    Options const& options,
    rti::CallbackModel model) {
  runTaraFutureTimestampedInteraction(options, model);
}

}  // namespace hla_rti_cpp_tck::detail
