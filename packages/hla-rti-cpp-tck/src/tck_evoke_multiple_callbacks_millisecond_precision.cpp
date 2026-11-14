#include "tck_main_scenarios.hpp"

#include <array>
#include <chrono>
#include <cstdint>
#include <string>
#include <thread>
#include <vector>

namespace hla_rti_cpp_tck::detail {

void scenarioEvokeMultipleCallbacksMillisecondPrecision(
    Options const& options,
    rti::CallbackModel model) {
  require(
      model == rti::HLA_EVOKED,
      "Evoke Multiple Callbacks precision evidence requires HLA_EVOKED");

  Session publisher(options, model, "owner");
  Session receiver(options, model, "member");
  auto const federation = federationName(
      options,
      "evoke-multiple-callbacks-millisecond-precision");
  publisher.connect();
  receiver.connect();

  constexpr std::size_t timingSamples = 30U;
  std::chrono::nanoseconds zeroWaitTotal{};
  std::chrono::nanoseconds oneMillisecondTotal{};
  std::chrono::nanoseconds fourMillisecondTotal{};
  auto measureEmptyQueueCall = [&](double seconds) {
    auto const started = Clock::now();
    auto const pending = receiver.rtiAmbassador().evokeMultipleCallbacks(
        seconds,
        seconds);
    auto const elapsed = Clock::now() - started;
    require(!pending, "millisecond timing probe unexpectedly found a callback");
    return elapsed;
  };

  for (std::size_t sample = 0; sample < timingSamples; ++sample) {
    std::array<double, 3U> const waits = sample % 2U == 0U
        ? std::array<double, 3U>{0.0, 0.001, 0.004}
        : std::array<double, 3U>{0.004, 0.001, 0.0};
    for (double const seconds : waits) {
      auto const elapsed = measureEmptyQueueCall(seconds);
      if (seconds == 0.0) {
        zeroWaitTotal += elapsed;
      } else if (seconds == 0.001) {
        oneMillisecondTotal += elapsed;
      } else {
        fourMillisecondTotal += elapsed;
      }
    }
  }
  auto const oneMillisecondIncrement =
      oneMillisecondTotal - zeroWaitTotal;
  auto const fourMillisecondIncrement =
      fourMillisecondTotal - oneMillisecondTotal;
  require(
      oneMillisecondIncrement >= std::chrono::milliseconds(15),
      "a 1 ms minimum wait was not distinguishable from a zero wait");
  require(
      fourMillisecondIncrement >= std::chrono::milliseconds(40),
      "1-to-4 ms minimum-wait delta was only " +
          std::to_string(std::chrono::duration_cast<std::chrono::microseconds>(
                             fourMillisecondIncrement)
                             .count()) +
          " us across " + std::to_string(timingSamples) +
          " samples (zero=" +
          std::to_string(std::chrono::duration_cast<std::chrono::microseconds>(
                             zeroWaitTotal)
                             .count()) +
          " us, one=" +
          std::to_string(std::chrono::duration_cast<std::chrono::microseconds>(
                             oneMillisecondTotal)
                             .count()) +
          " us, four=" +
          std::to_string(std::chrono::duration_cast<std::chrono::microseconds>(
                             fourMillisecondTotal)
                             .count()) +
          " us)");

  publisher.rtiAmbassador().createFederationExecution(
      federation,
      options.fom.wstring(),
      options.logicalTimeImplementationName);
  publisher.join(options.ownerFederateName, options.federateType, federation);
  receiver.join(options.memberFederateName, options.federateType, federation);

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
        auto const callbackWorkDeadline =
            Clock::now() + std::chrono::microseconds(250);
        while (Clock::now() < callbackWorkDeadline) {
        }
      });

  constexpr std::size_t callbackCount = 120U;
  std::vector<std::vector<std::uint8_t>> payloads;
  std::vector<std::vector<std::uint8_t>> tags;
  payloads.reserve(callbackCount);
  tags.reserve(callbackCount);
  for (std::size_t index = 0; index < callbackCount; ++index) {
    auto const high = static_cast<std::uint8_t>((index >> 8U) & 0xffU);
    auto const low = static_cast<std::uint8_t>(index & 0xffU);
    payloads.push_back({0x50U, high, low});
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
  std::this_thread::sleep_for(std::chrono::milliseconds(25));

  auto const firstMaximumStarted = Clock::now();
  auto const firstHasPending = receiver.rtiAmbassador().evokeMultipleCallbacks(
      0.0,
      0.001);
  auto const firstMaximumElapsed = Clock::now() - firstMaximumStarted;
  auto const afterOneMillisecondMaximum = receiver.recorder().interactions();
  require(
      firstHasPending,
      "the 1 ms maximum probe did not leave the preloaded callback backlog pending");
  require(
      !afterOneMillisecondMaximum.empty(),
      "the 1 ms maximum probe did not invoke any queued callback");

  auto const secondMaximumStarted = Clock::now();
  static_cast<void>(receiver.rtiAmbassador().evokeMultipleCallbacks(
      0.0,
      0.004));
  auto const secondMaximumElapsed = Clock::now() - secondMaximumStarted;
  auto const afterFourMillisecondMaximum = receiver.recorder().interactions();
  auto const fourMillisecondCallbackCount =
      afterFourMillisecondMaximum.size() - afterOneMillisecondMaximum.size();
  require(
      fourMillisecondCallbackCount >= 2U,
      "increasing the maximum wait from 1 ms to 4 ms did not admit additional callbacks");
  require(
      fourMillisecondCallbackCount <= 24U,
      "the 4 ms maximum wait behaved like a substantially coarser timing interval");
  require(
      firstMaximumElapsed < std::chrono::milliseconds(30) &&
          secondMaximumElapsed < std::chrono::milliseconds(30),
      "millisecond-scale maximum arguments were interpreted as a coarse wait");

  for (std::size_t index = 0; index < afterFourMillisecondMaximum.size(); ++index) {
    require(
        afterFourMillisecondMaximum[index].interaction == receiverInteraction &&
            afterFourMillisecondMaximum[index].producer == publisher.federateHandle() &&
            afterFourMillisecondMaximum[index].parameters.size() == 1U &&
            afterFourMillisecondMaximum[index].parameters.count(receiverParameter) == 1U &&
            copyBytes(afterFourMillisecondMaximum[index].parameters.at(receiverParameter)) ==
                payloads[index] &&
            afterFourMillisecondMaximum[index].tag == tags[index],
        "the millisecond timing probes changed queued callback order or contents");
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
