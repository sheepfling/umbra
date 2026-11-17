#include "tck_main_scenarios.hpp"

#include <set>
#include <thread>

namespace hla_rti_cpp_tck::detail {

void scenarioReliableTransportOneCopyDelivery(
    Options const& options,
    rti::CallbackModel model) {
  Session sender(options, model, "reliable-transport-one-copy-sender");
  Session receiver(options, model, "reliable-transport-one-copy-receiver");
  auto const federation = federationName(options, "reliable-transport-one-copy");
  connectAndJoin(sender, receiver, options, federation, options.fom);

  auto const senderInteraction = sender.rtiAmbassador().getInteractionClassHandle(
      options.interactionClassName);
  auto const receiverInteraction = receiver.rtiAmbassador().getInteractionClassHandle(
      options.interactionClassName);
  auto const senderReliable = sender.rtiAmbassador().getTransportationTypeHandle(
      L"HLAreliable");
  auto const receiverReliable = receiver.rtiAmbassador().getTransportationTypeHandle(
      L"HLAreliable");
  require(
      senderInteraction.isValid() && receiverInteraction.isValid() &&
          senderReliable.isValid() && receiverReliable.isValid() &&
          sender.rtiAmbassador().getTransportationTypeName(senderReliable) ==
              L"HLAreliable" &&
          receiver.rtiAmbassador().getTransportationTypeName(receiverReliable) ==
              L"HLAreliable",
      "the standard reliable transport or selected interaction was not available");

  sender.rtiAmbassador().publishInteractionClass(senderInteraction);
  receiver.rtiAmbassador().subscribeInteractionClass(receiverInteraction, true);

  std::vector<std::vector<std::uint8_t>> const expectedTags{
      {0x31U}, {0x32U}, {0x33U}};
  rti::ParameterHandleValueMap const parameters;
  for (auto const& tagBytes : expectedTags) {
    rti::VariableLengthData const tag(tagBytes.data(), tagBytes.size());
    sender.rtiAmbassador().sendInteraction(senderInteraction, parameters, tag);
  }

  waitFor(
      receiver,
      [&] { return receiver.recorder().interactions().size() >= expectedTags.size(); },
      options,
      "one reliable delivery for each sent interaction");

  auto const settleDeadline = Clock::now() + std::chrono::milliseconds(40);
  while (Clock::now() < settleDeadline) {
    sender.pump();
    receiver.pump();
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }

  auto const deliveries = receiver.recorder().interactions();
  require(
      deliveries.size() == expectedTags.size(),
      "reliable receive-order transport did not deliver exactly one callback per send");
  std::set<std::vector<std::uint8_t>> deliveredTags;
  for (auto const& delivery : deliveries) {
    require(
        delivery.interaction == receiverInteraction &&
            delivery.producer == sender.federateHandle() &&
            delivery.transportation == receiverReliable &&
            receiver.rtiAmbassador().getTransportationTypeName(
                delivery.transportation) == L"HLAreliable",
        "reliable delivery changed its interaction, producer, or transport identity");
    require(
        deliveredTags.insert(delivery.tag).second,
        "reliable transport delivered a duplicate copy of one sent interaction");
  }
  require(
      deliveredTags ==
          std::set<std::vector<std::uint8_t>>(expectedTags.begin(), expectedTags.end()),
      "reliable transport did not deliver every sent interaction exactly once");

  receiver.rtiAmbassador().unsubscribeInteractionClass(receiverInteraction);
  sender.rtiAmbassador().unpublishInteractionClass(senderInteraction);
  sender.resign(rti::NO_ACTION);
  receiver.resign(rti::NO_ACTION);
  sender.rtiAmbassador().destroyFederationExecution(federation);
  receiver.disconnect();
  sender.disconnect();
}

}  // namespace hla_rti_cpp_tck::detail
