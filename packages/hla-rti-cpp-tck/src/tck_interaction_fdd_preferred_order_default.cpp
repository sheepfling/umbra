#include "tck_main_scenarios.hpp"

namespace hla_rti_cpp_tck::detail {

void scenarioInteractionFddPreferredOrderDefault(
    Options const& options, rti::CallbackModel model) {
  Session sender(options, model, "interaction-fdd-order-sender");
  Session receiver(options, model, "interaction-fdd-order-receiver");
  auto const federation = federationName(options, "interaction-fdd-preferred-order-default");
  connectAndJoin(sender, receiver, options, federation, options.fom);

  auto const senderTimestampClass = sender.rtiAmbassador().getInteractionClassHandle(
      options.preferredTimestampInteractionClassName);
  auto const senderReceiveClass = sender.rtiAmbassador().getInteractionClassHandle(
      options.preferredReceiveInteractionClassName);
  auto const receiverTimestampClass = receiver.rtiAmbassador().getInteractionClassHandle(
      options.preferredTimestampInteractionClassName);
  auto const receiverReceiveClass = receiver.rtiAmbassador().getInteractionClassHandle(
      options.preferredReceiveInteractionClassName);
  auto const senderTimestampParameter = sender.rtiAmbassador().getParameterHandle(
      senderTimestampClass, options.parameterName);
  auto const senderReceiveParameter = sender.rtiAmbassador().getParameterHandle(
      senderReceiveClass, options.parameterName);
  auto const receiverTimestampParameter = receiver.rtiAmbassador().getParameterHandle(
      receiverTimestampClass, options.parameterName);
  auto const receiverReceiveParameter = receiver.rtiAmbassador().getParameterHandle(
      receiverReceiveClass, options.parameterName);
  require(
      senderTimestampClass.isValid() && senderReceiveClass.isValid() &&
          receiverTimestampClass.isValid() && receiverReceiveClass.isValid() &&
          senderTimestampParameter.isValid() && senderReceiveParameter.isValid() &&
          receiverTimestampParameter.isValid() && receiverReceiveParameter.isValid(),
      "adapter FOM did not provide both preferred-order interaction classes and parameters");

  sender.rtiAmbassador().publishInteractionClass(senderTimestampClass);
  sender.rtiAmbassador().publishInteractionClass(senderReceiveClass);
  receiver.rtiAmbassador().subscribeInteractionClass(receiverTimestampClass);
  receiver.rtiAmbassador().subscribeInteractionClass(receiverReceiveClass);

  auto senderTime = makeTimeContext(sender);
  auto receiverTime = makeTimeContext(receiver);
  require(
      senderTime.factory->getName() == receiverTime.factory->getName(),
      "FDD-default interaction members selected different logical-time factories");
  auto lookahead = senderTime.factory->makeZero();
  require(lookahead != nullptr,
          "FDD-default interaction time factory returned no lookahead interval");
  *lookahead += *senderTime.epsilon;
  sender.rtiAmbassador().enableTimeRegulation(*lookahead);
  waitFor(
      sender,
      [&] { return sender.recorder().timeRegulationEnabled().size() == 1U; },
      options,
      "FDD-default interaction sender time-regulation callback");

  auto const timestampTime = timeAfter(
      *senderTime.factory, *senderTime.initial, *senderTime.epsilon, 5U);
  auto const receiveTime = timeAfter(
      *senderTime.factory, *senderTime.initial, *senderTime.epsilon, 6U);
  std::vector<std::uint8_t> const payload{0x4FU, 0x4BU};
  std::vector<std::uint8_t> const tagBytes{0x46U, 0x44U, 0x44U};
  rti::ParameterHandleValueMap timestampValues;
  timestampValues.emplace(
      senderTimestampParameter,
      rti::VariableLengthData(payload.data(), payload.size()));
  rti::VariableLengthData tag(tagBytes.data(), tagBytes.size());
  auto const timestampRetraction = sender.rtiAmbassador().sendInteraction(
      senderTimestampClass, timestampValues, tag, *timestampTime);
  rti::ParameterHandleValueMap receiveValues;
  receiveValues.emplace(
      senderReceiveParameter,
      rti::VariableLengthData(payload.data(), payload.size()));
  auto const receiveRetraction = sender.rtiAmbassador().sendInteraction(
      senderReceiveClass, receiveValues, tag, *receiveTime);
  require(
      timestampRetraction.isValid(),
      "FDD-preferred timestamp-order send did not return a retraction designator");
  require(
      !receiveRetraction.isValid(),
      "FDD-preferred receive-order send unexpectedly returned a retraction designator");

  waitFor(
      receiver,
      [&] { return receiver.recorder().timedInteractions().size() == 2U; },
      options,
      "FDD-preferred timestamp-bearing interaction callbacks");
  auto const deliveries = receiver.recorder().timedInteractions();
  auto const timestampDelivery = std::find_if(
      deliveries.begin(), deliveries.end(), [&](auto const& delivery) {
        return delivery.interaction == receiverTimestampClass;
      });
  auto const receiveDelivery = std::find_if(
      deliveries.begin(), deliveries.end(), [&](auto const& delivery) {
        return delivery.interaction == receiverReceiveClass;
      });
  require(
      receiver.recorder().interactions().empty() &&
          receiver.recorder().timeAdvanceGrants().empty() && deliveries.size() == 2U &&
          timestampDelivery != deliveries.end() && receiveDelivery != deliveries.end(),
      "FDD-preferred sends did not produce exactly two timestamp-bearing callbacks");
  require(
      timestampDelivery->parameters.size() == 1U &&
          timestampDelivery->parameters.count(receiverTimestampParameter) == 1U &&
          copyBytes(timestampDelivery->parameters.at(receiverTimestampParameter)) == payload &&
          timestampDelivery->tag == tagBytes &&
          timestampDelivery->producer == sender.federateHandle() &&
          timestampDelivery->time == encodeTime(*timestampTime) &&
          timestampDelivery->sentOrder == rti::TIMESTAMP &&
          timestampDelivery->retractionPresent &&
          timestampDelivery->retraction == copyBytes(timestampRetraction.encode()),
      "timestamp-preferred interaction did not follow its FDD order default");
  require(
      receiveDelivery->parameters.size() == 1U &&
          receiveDelivery->parameters.count(receiverReceiveParameter) == 1U &&
          copyBytes(receiveDelivery->parameters.at(receiverReceiveParameter)) == payload &&
          receiveDelivery->tag == tagBytes &&
          receiveDelivery->producer == sender.federateHandle() &&
          receiveDelivery->time == encodeTime(*receiveTime) &&
          receiveDelivery->sentOrder == rti::RECEIVE &&
          !receiveDelivery->retractionPresent,
      "receive-preferred interaction did not follow its FDD order default");
  auto const callbackOrder = receiver.recorder().callbackOrder();
  require(
      std::count(callbackOrder.begin(), callbackOrder.end(), "interaction") == 2,
      "FDD-preferred sends did not both use timestamp-bearing interaction callbacks");
  require(
      timestampDelivery->transportation.isValid() && receiveDelivery->transportation.isValid() &&
          !receiver.rtiAmbassador()
               .getTransportationTypeName(timestampDelivery->transportation)
               .empty() &&
          !receiver.rtiAmbassador()
               .getTransportationTypeName(receiveDelivery->transportation)
               .empty(),
      "FDD-preferred callbacks returned an unresolvable transportation type");

  sender.rtiAmbassador().disableTimeRegulation();
  receiver.rtiAmbassador().unsubscribeInteractionClass(receiverReceiveClass);
  receiver.rtiAmbassador().unsubscribeInteractionClass(receiverTimestampClass);
  sender.rtiAmbassador().unpublishInteractionClass(senderReceiveClass);
  sender.rtiAmbassador().unpublishInteractionClass(senderTimestampClass);
  receiver.resign(rti::NO_ACTION);
  sender.resign(rti::CANCEL_THEN_DELETE_THEN_DIVEST);
  sender.rtiAmbassador().destroyFederationExecution(federation);
  receiver.disconnect();
  sender.disconnect();
}

void scenarioInteractionFddPreferredOrderDefaultContract(
    Options const& options, rti::CallbackModel model) {
  scenarioInteractionFddPreferredOrderDefault(options, model);
}

}  // namespace hla_rti_cpp_tck::detail
