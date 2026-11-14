#include "tck_main_scenarios.hpp"

#include <exception>
#include <future>
#include <thread>

namespace hla_rti_cpp_tck::detail {

void scenarioEvokeMultipleCallbacksMinimumDuration(
    Options const& options,
    rti::CallbackModel model) {
  require(
      model == rti::HLA_EVOKED,
      "Evoke Multiple Callbacks minimum-duration evidence requires HLA_EVOKED");

  Session publisher(options, model, "owner");
  Session receiver(options, model, "member");
  auto const federation = federationName(
      options,
      "evoke-multiple-callbacks-minimum-duration");
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

  std::vector<std::vector<std::uint8_t>> const payloads{
      {0x45U, 0x31U}, {0x45U, 0x32U}, {0x45U, 0x33U}};
  std::vector<std::vector<std::uint8_t>> const tags{
      {0x54U, 0x31U}, {0x54U, 0x32U}, {0x54U, 0x33U}};
  std::promise<void> beginSending;
  auto sendPermission = beginSending.get_future();
  std::exception_ptr sendFailure;
  std::thread sender([&publisher,
                      &payloads,
                      &tags,
                      publisherInteraction,
                      publisherParameter,
                      permission = std::move(sendPermission),
                      &sendFailure]() mutable {
    permission.wait();
    try {
      for (std::size_t index = 0; index < payloads.size(); ++index) {
        if (index != 0U) {
          std::this_thread::sleep_for(std::chrono::milliseconds(35));
        }
        rti::ParameterHandleValueMap parameters;
        parameters.emplace(
            publisherParameter,
            rti::VariableLengthData(
                payloads[index].data(), payloads[index].size()));
        rti::VariableLengthData tag(tags[index].data(), tags[index].size());
        publisher.rtiAmbassador().sendInteraction(
            publisherInteraction,
            parameters,
            tag);
      }
    } catch (...) {
      sendFailure = std::current_exception();
    }
  });

  constexpr double minimumSeconds = 0.20;
  constexpr double maximumSeconds = 0.75;
  auto const minimumElapsed = std::chrono::milliseconds(200);
  auto const tolerance = std::chrono::milliseconds(20);
  auto const completionTolerance = std::chrono::milliseconds(150);
  auto const evokeStarted = Clock::now();
  beginSending.set_value();
  bool callbacksRemain = false;
  std::exception_ptr evokeFailure;
  try {
    callbacksRemain = receiver.rtiAmbassador().evokeMultipleCallbacks(
        minimumSeconds,
        maximumSeconds);
  } catch (...) {
    evokeFailure = std::current_exception();
  }
  auto const elapsed = Clock::now() - evokeStarted;
  sender.join();
  if (sendFailure) {
    std::rethrow_exception(sendFailure);
  }
  if (evokeFailure) {
    std::rethrow_exception(evokeFailure);
  }

  require(
      elapsed + tolerance >= minimumElapsed,
      "Evoke Multiple Callbacks returned before its approximate minimum wall-clock duration");
  require(
      elapsed <= minimumElapsed + completionTolerance,
      "Evoke Multiple Callbacks did not complete near its minimum when no additional callbacks remained");
  require(
      !callbacksRemain,
      "Evoke Multiple Callbacks reported pending callbacks after draining the test sequence");
  auto const received = receiver.recorder().interactions();
  require(
      received.size() == payloads.size(),
      "Evoke Multiple Callbacks did not process every receive-order callback offered during its minimum interval");
  for (std::size_t index = 0; index < received.size(); ++index) {
    require(
        received[index].interaction == receiverInteraction &&
            received[index].producer == publisher.federateHandle() &&
            received[index].parameters.size() == 1U &&
            received[index].parameters.count(receiverParameter) == 1U &&
            copyBytes(received[index].parameters.at(receiverParameter)) ==
                payloads[index] &&
            received[index].tag == tags[index],
        "Evoke Multiple Callbacks changed the order or contents of an offered callback");
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
