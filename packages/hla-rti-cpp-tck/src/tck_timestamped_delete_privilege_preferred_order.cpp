#include "tck_main_scenarios.hpp"

namespace hla_rti_cpp_tck::detail {

void runTimestampedDeletePrivilegeOrder(
    Options const& options,
    rti::CallbackModel model,
    rti::OrderType preferredOrder) {
  Session sender(options, model, "timestamped-delete-order-sender");
  Session receiver(options, model, "timestamped-delete-order-receiver");
  auto const federation =
      federationName(options, "timestamped-delete-privilege-preferred-order");
  connectAndJoin(sender, receiver, options, federation, options.fom);

  auto const senderClass = sender.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const receiverClass = receiver.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const senderAttribute = sender.rtiAmbassador().getAttributeHandle(
      senderClass, options.attributeName);
  auto const receiverAttribute = receiver.rtiAmbassador().getAttributeHandle(
      receiverClass, options.attributeName);
  auto const privilegeToDelete = sender.rtiAmbassador().getAttributeHandle(
      senderClass, L"HLAprivilegeToDeleteObject");
  require(
      senderClass.isValid() && receiverClass.isValid() &&
          senderClass == receiverClass && senderAttribute.isValid() &&
          receiverAttribute.isValid() && senderAttribute == receiverAttribute &&
          privilegeToDelete.isValid(),
      "adapter FOM did not provide the configured object and standard delete-privilege attribute");

  rti::AttributeHandleSet const senderAttributes{senderAttribute};
  rti::AttributeHandleSet const receiverAttributes{receiverAttribute};
  sender.rtiAmbassador().publishObjectClassAttributes(senderClass, senderAttributes);
  receiver.rtiAmbassador().subscribeObjectClassAttributes(
      receiverClass, receiverAttributes, true, L"");
  auto const object = sender.rtiAmbassador().registerObjectInstance(senderClass);
  require(object.isValid(), "timestamped delete registration returned an invalid handle");
  waitFor(
      receiver,
      [&] { return receiver.recorder().hasDiscovery(object); },
      options,
      "timestamped delete object discovery");

  auto senderTime = makeTimeContext(sender);
  sender.rtiAmbassador().enableTimeRegulation(*senderTime.epsilon);
  waitFor(
      sender,
      [&] { return sender.recorder().timeRegulationEnabled().size() == 1U; },
      options,
      "timestamped delete sender regulation callback");

  sender.rtiAmbassador().changeAttributeOrderType(
      object, rti::AttributeHandleSet{privilegeToDelete}, preferredOrder);
  auto const timestamp = timeAfter(
      *senderTime.factory, *senderTime.initial, *senderTime.epsilon, 2U);
  std::vector<std::uint8_t> const tagBytes{0x50U, 0x52U, 0x45U, 0x46U};
  rti::VariableLengthData const tag(tagBytes.data(), tagBytes.size());
  auto const retraction = sender.rtiAmbassador().deleteObjectInstance(
      object, tag, *timestamp);
  auto const timestampPreferred = preferredOrder == rti::TIMESTAMP;
  require(
      retraction.isValid() == timestampPreferred,
      "timestamped deletion returned a retraction designator inconsistent with the preferred order");
  std::vector<std::uint8_t> expectedRetraction;
  if (timestampPreferred) {
    expectedRetraction = copyBytes(retraction.encode());
  }

  waitFor(
      receiver,
      [&] { return receiver.recorder().timedRemovals().size() == 1U; },
      options,
      "order-preferred timestamped object removal");
  auto const removals = receiver.recorder().timedRemovals();
  auto const callbackOrder = receiver.recorder().callbackOrder();
  require(
      removals.size() == 1U && receiver.recorder().removals().empty(),
      "timestamped deletion did not use exactly one timestamped removal callback");
  auto const& removal = removals.front();
  require(
      removal.object == object && removal.tag == tagBytes &&
          removal.producer == sender.federateHandle(),
      "timestamped removal changed the object, tag, or producer");
  require(
      removal.time == encodeTime(*timestamp) &&
          removal.timeText == timestamp->toString(),
      "timestamped removal changed the supplied logical time");
  require(
      removal.sentOrder == preferredOrder,
      "timestamped removal reported a sent order inconsistent with the configured preference");
  require(
      removal.receivedOrder == (timestampPreferred ? rti::RECEIVE : preferredOrder),
      "timestamped removal did not report the receiver's preferred order");
  require(
      removal.retractionPresent == timestampPreferred &&
          removal.retraction == expectedRetraction,
      "timestamped removal returned a retraction handle inconsistent with the configured preference");
  require(
      std::count(callbackOrder.begin(), callbackOrder.end(), "remove") == 1,
      "timestamped deletion delivered a duplicate removal callback");

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

void scenarioTimestampedDeletePrivilegePreferredOrder(
    Options const& options, rti::CallbackModel model) {
  runTimestampedDeletePrivilegeOrder(options, model, rti::RECEIVE);
}

void scenarioTimestampedDeletePrivilegePreferredOrderContract(
    Options const& options, rti::CallbackModel model) {
  scenarioTimestampedDeletePrivilegePreferredOrder(options, model);
}

void scenarioTimestampedDeletePrivilegeTimestampOrder(
    Options const& options, rti::CallbackModel model) {
  runTimestampedDeletePrivilegeOrder(options, model, rti::TIMESTAMP);
}

void scenarioTimestampedDeletePrivilegeTimestampOrderContract(
    Options const& options, rti::CallbackModel model) {
  scenarioTimestampedDeletePrivilegeTimestampOrder(options, model);
}

}  // namespace hla_rti_cpp_tck::detail
