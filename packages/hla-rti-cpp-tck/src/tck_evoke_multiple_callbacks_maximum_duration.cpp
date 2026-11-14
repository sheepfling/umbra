#include "tck_main_scenarios.hpp"

#include <chrono>
#include <exception>
#include <string>
#include <thread>

namespace hla_rti_cpp_tck::detail {

void scenarioEvokeMultipleCallbacksMaximumDuration(
    Options const& options,
    rti::CallbackModel model) {
  require(
      model == rti::HLA_EVOKED,
      "Evoke Multiple Callbacks maximum-duration evidence requires HLA_EVOKED");

  Session publisher(options, model, "owner");
  Session receiver(options, model, "member");
  auto const federation = federationName(
      options,
      "evoke-multiple-callbacks-maximum-duration");
  connectAndJoin(publisher, receiver, options, federation, options.fom);

  rti::ObjectClassHandle publisherObjectClass;
  rti::AttributeHandle publisherAttribute;
  rti::InteractionClassHandle publisherInteraction;
  rti::ParameterHandle publisherParameter;
  handles(
      publisher,
      options,
      publisherObjectClass,
      publisherAttribute,
      publisherInteraction,
      publisherParameter);
  rti::ObjectClassHandle receiverObjectClass;
  rti::AttributeHandle receiverAttribute;
  rti::InteractionClassHandle receiverInteraction;
  rti::ParameterHandle receiverParameter;
  handles(
      receiver,
      options,
      receiverObjectClass,
      receiverAttribute,
      receiverInteraction,
      receiverParameter);
  require(
      publisherInteraction == receiverInteraction &&
          publisherParameter == receiverParameter,
      "publisher and receiver resolved different interaction handles");

  publisher.rtiAmbassador().publishInteractionClass(publisherInteraction);
  receiver.rtiAmbassador().subscribeInteractionClass(receiverInteraction, true);
  receiver.recorder().setReceiveInteractionHook(
      [](rti::RegionHandleSet const*) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
      });

  constexpr std::size_t callbackCount = 120U;
  std::vector<std::vector<std::uint8_t>> payloads;
  std::vector<std::vector<std::uint8_t>> tags;
  payloads.reserve(callbackCount);
  tags.reserve(callbackCount);
  for (std::size_t index = 0; index < callbackCount; ++index) {
    auto const high = static_cast<std::uint8_t>((index >> 8U) & 0xffU);
    auto const low = static_cast<std::uint8_t>(index & 0xffU);
    payloads.push_back({0x45U, high, low});
    tags.push_back({0x54U, high, low});

    rti::ParameterHandleValueMap parameters;
    parameters.emplace(
        publisherParameter,
        rti::VariableLengthData(payloads.back().data(), payloads.back().size()));
    rti::VariableLengthData tag(tags.back().data(), tags.back().size());
    publisher.rtiAmbassador().sendInteraction(
        publisherInteraction,
        parameters,
        tag);
  }
  // Let process-boundary events reach the receiver before evoking. The slow
  // test callback keeps this finite backlog non-empty through the max window.
  std::this_thread::sleep_for(std::chrono::milliseconds(50));

  constexpr double minimumSeconds = 0.15;
  constexpr double maximumSeconds = 0.50;
  constexpr auto maximumDuration = std::chrono::milliseconds(500);
  constexpr auto timingTolerance = std::chrono::milliseconds(100);
  auto const evokeStarted = Clock::now();
  std::exception_ptr evokeFailure;
  try {
    static_cast<void>(receiver.rtiAmbassador().evokeMultipleCallbacks(
        minimumSeconds,
        maximumSeconds));
  } catch (...) {
    evokeFailure = std::current_exception();
  }
  auto const elapsed = Clock::now() - evokeStarted;
  if (evokeFailure) {
    std::rethrow_exception(evokeFailure);
  }

  require(
      elapsed + timingTolerance >= maximumDuration,
      "Evoke Multiple Callbacks returned materially before its maximum interval with callbacks pending");
  require(
      elapsed <= maximumDuration + timingTolerance,
      "Evoke Multiple Callbacks substantially exceeded its approximate maximum interval");

  auto const received = receiver.recorder().interactions();
  auto const elapsedMilliseconds = std::chrono::duration_cast<
      std::chrono::milliseconds>(elapsed).count();
  require(
      received.size() >= 20U,
      "Evoke Multiple Callbacks delivered only " +
          std::to_string(received.size()) + " callbacks in " +
          std::to_string(elapsedMilliseconds) +
          " ms; expected continued delivery after the minimum interval");
  require(
      received.size() <= callbackCount,
      "Evoke Multiple Callbacks delivered more callbacks than the test queued");
  for (std::size_t index = 0; index < received.size(); ++index) {
    require(
        received[index].interaction == receiverInteraction &&
            received[index].producer == publisher.federateHandle() &&
            received[index].parameters.size() == 1U &&
            received[index].parameters.count(receiverParameter) == 1U &&
            copyBytes(received[index].parameters.at(receiverParameter)) ==
                payloads[index] &&
            received[index].tag == tags[index],
        "Evoke Multiple Callbacks changed the order or contents of a queued receive-order callback");
  }

  receiver.rtiAmbassador().unsubscribeInteractionClass(receiverInteraction);
  publisher.rtiAmbassador().unpublishInteractionClass(publisherInteraction);
  receiver.resign(rti::NO_ACTION);
  publisher.resign(rti::NO_ACTION);
  publisher.rtiAmbassador().destroyFederationExecution(federation);
  receiver.disconnect();
  publisher.disconnect();
}

}  // namespace hla_rti_cpp_tck::detail
