#include "tck_main_scenarios.hpp"

namespace hla_rti_cpp_tck::detail {

void scenarioAttributeFddPreferredOrderDefault(
    Options const& options, rti::CallbackModel model) {
  Session sender(options, model, "attribute-fdd-order-sender");
  Session receiver(options, model, "attribute-fdd-order-receiver");
  auto const federation = federationName(options, "attribute-fdd-preferred-order-default");
  connectAndJoin(sender, receiver, options, federation, options.fom);

  auto const senderClass = sender.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const receiverClass = receiver.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const senderTimestampAttribute = sender.rtiAmbassador().getAttributeHandle(
      senderClass, options.attributeName);
  auto const senderReceiveAttribute = sender.rtiAmbassador().getAttributeHandle(
      senderClass, options.objectClassThirdAttributeName);
  auto const receiverTimestampAttribute = receiver.rtiAmbassador().getAttributeHandle(
      receiverClass, options.attributeName);
  auto const receiverReceiveAttribute = receiver.rtiAmbassador().getAttributeHandle(
      receiverClass, options.objectClassThirdAttributeName);
  require(
      senderClass.isValid() && receiverClass.isValid() &&
          senderClass == receiverClass && senderTimestampAttribute.isValid() &&
          senderReceiveAttribute.isValid() && receiverTimestampAttribute.isValid() &&
          receiverReceiveAttribute.isValid() &&
          senderTimestampAttribute == receiverTimestampAttribute &&
          senderReceiveAttribute == receiverReceiveAttribute,
      "adapter FOM did not provide the configured object class and both ordered attributes");

  rti::AttributeHandleSet const senderAttributes{
      senderTimestampAttribute, senderReceiveAttribute};
  rti::AttributeHandleSet const receiverAttributes{
      receiverTimestampAttribute, receiverReceiveAttribute};
  sender.rtiAmbassador().publishObjectClassAttributes(senderClass, senderAttributes);
  receiver.rtiAmbassador().subscribeObjectClassAttributes(
      receiverClass, receiverAttributes, true, L"");
  auto const object = sender.rtiAmbassador().registerObjectInstance(senderClass);
  require(object.isValid(), "FDD-default attribute registration returned an invalid handle");
  waitFor(
      receiver,
      [&] { return receiver.recorder().hasDiscovery(object); },
      options,
      "FDD-default attribute object discovery");

  auto senderTime = makeTimeContext(sender);
  auto receiverTime = makeTimeContext(receiver);
  require(
      senderTime.factory->getName() == receiverTime.factory->getName() &&
          encodeTime(*senderTime.initial) == encodeTime(*receiverTime.initial),
      "FDD-default attribute members selected incompatible logical-time representations");
  auto lookahead = senderTime.factory->makeZero();
  require(lookahead != nullptr,
          "FDD-default attribute time factory returned no zero interval");
  *lookahead += *senderTime.epsilon;
  sender.rtiAmbassador().enableTimeRegulation(*lookahead);
  waitFor(
      sender,
      [&] { return sender.recorder().timeRegulationEnabled().size() == 1U; },
      options,
      "FDD-default attribute sender regulation callback");

  auto const timestampTime = timeAfter(
      *senderTime.factory, *senderTime.initial, *senderTime.epsilon, 5U);
  auto const receiveTime = timeAfter(
      *senderTime.factory, *senderTime.initial, *senderTime.epsilon, 6U);
  std::vector<std::uint8_t> const timestampValue{0x46U, 0x44U, 0x44U, 0x54U};
  std::vector<std::uint8_t> const receiveValue{0x46U, 0x44U, 0x44U, 0x52U};
  std::vector<std::uint8_t> const timestampTag{0x54U, 0x53U};
  std::vector<std::uint8_t> const receiveTag{0x52U, 0x4FU};
  rti::VariableLengthData timestampBytes(timestampValue.data(), timestampValue.size());
  rti::VariableLengthData receiveBytes(receiveValue.data(), receiveValue.size());
  rti::VariableLengthData timestampTagData(timestampTag.data(), timestampTag.size());
  rti::VariableLengthData receiveTagData(receiveTag.data(), receiveTag.size());
  rti::AttributeHandleValueMap timestampValues;
  timestampValues.emplace(senderTimestampAttribute, timestampBytes);
  rti::AttributeHandleValueMap receiveValues;
  receiveValues.emplace(senderReceiveAttribute, receiveBytes);
  auto const timestampRetraction = sender.rtiAmbassador().updateAttributeValues(
      object, timestampValues, timestampTagData, *timestampTime);
  auto const receiveRetraction = sender.rtiAmbassador().updateAttributeValues(
      object, receiveValues, receiveTagData, *receiveTime);
  require(
      timestampRetraction.isValid(),
      "FDD-preferred timestamp-order update returned no retraction designator");
  require(
      !receiveRetraction.isValid(),
      "FDD-preferred receive-order update unexpectedly returned a retraction designator");

  waitFor(
      receiver,
      [&] { return receiver.recorder().timedReflections().size() == 2U; },
      options,
      "both FDD-preferred timestamp-bearing reflection callbacks");
  auto const deliveries = receiver.recorder().timedReflections();
  auto const timestampDelivery = std::find_if(
      deliveries.begin(), deliveries.end(), [&](auto const& delivery) {
        return delivery.values.count(receiverTimestampAttribute) == 1U;
      });
  auto const receiveDelivery = std::find_if(
      deliveries.begin(), deliveries.end(), [&](auto const& delivery) {
        return delivery.values.count(receiverReceiveAttribute) == 1U;
      });
  require(
      receiver.recorder().reflections().empty() && deliveries.size() == 2U &&
          timestampDelivery != deliveries.end() && receiveDelivery != deliveries.end(),
      "FDD-preferred updates did not produce exactly two timestamp-bearing reflections");
  auto const checkDelivery = [&](TimedReflectionRecord const& delivery,
                                 rti::AttributeHandle const& attribute,
                                 std::vector<std::uint8_t> const& expectedValue,
                                 std::vector<std::uint8_t> const& expectedTag,
                                 rti::LogicalTime const& expectedTime,
                                 rti::OrderType sentOrder,
                                 bool retractionPresent,
                                 std::vector<std::uint8_t> const& expectedRetraction,
                                 std::string const& description) {
    require(
        delivery.object == object && delivery.values.size() == 1U &&
            delivery.values.count(attribute) == 1U &&
            copyBytes(delivery.values.at(attribute)) == expectedValue &&
            delivery.tag == expectedTag && delivery.producer == sender.federateHandle() &&
            delivery.time == encodeTime(expectedTime) && delivery.sentOrder == sentOrder &&
            delivery.receivedOrder == rti::RECEIVE &&
            delivery.retractionPresent == retractionPresent &&
            delivery.retraction == expectedRetraction && delivery.transportation.isValid(),
        description + " did not preserve its FDD-selected order and standard data");
    require(
        !receiver.rtiAmbassador()
             .getTransportationTypeName(delivery.transportation)
             .empty(),
        description + " returned an unresolvable transportation type");
  };
  checkDelivery(
      *timestampDelivery,
      receiverTimestampAttribute,
      timestampValue,
      timestampTag,
      *timestampTime,
      rti::TIMESTAMP,
      true,
      copyBytes(timestampRetraction.encode()),
      "timestamp-preferred object attribute");
  checkDelivery(
      *receiveDelivery,
      receiverReceiveAttribute,
      receiveValue,
      receiveTag,
      *receiveTime,
      rti::RECEIVE,
      false,
      {},
      "receive-preferred object attribute");
  auto const callbackOrder = receiver.recorder().callbackOrder();
  require(
      std::count(callbackOrder.begin(), callbackOrder.end(), "reflect") == 2,
      "FDD-preferred updates did not produce exactly two reflection callbacks");

  sender.rtiAmbassador().disableTimeRegulation();
  receiver.rtiAmbassador().unsubscribeObjectClassAttributes(
      receiverClass, receiverAttributes);
  sender.rtiAmbassador().unpublishObjectClassAttributes(senderClass, senderAttributes);
  receiver.resign(rti::NO_ACTION);
  sender.resign(rti::NO_ACTION);
  sender.rtiAmbassador().destroyFederationExecution(federation);
  receiver.disconnect();
  sender.disconnect();
}

void scenarioAttributeFddPreferredOrderDefaultContract(
    Options const& options, rti::CallbackModel model) {
  scenarioAttributeFddPreferredOrderDefault(options, model);
}

}  // namespace hla_rti_cpp_tck::detail
