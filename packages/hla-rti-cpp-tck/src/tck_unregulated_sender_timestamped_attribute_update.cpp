#include "tck_main_scenarios.hpp"

namespace hla_rti_cpp_tck::detail {

void scenarioUnregulatedSenderTimestampedAttributeUpdate(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.logicalTimeImplementationName.empty(),
      "Unregulated timestamped attribute update requires an adapter-supplied logical-time implementation");

  Session publisher(options, model, "unregulated-timestamped-attribute-publisher");
  Session receiver(options, model, "unregulated-timestamped-attribute-receiver");
  auto const federation = federationName(
      options,
      "unregulated-sender-timestamped-attribute-update");
  connectAndJoin(publisher, receiver, options, federation, options.fom);

  auto const publisherClass = publisher.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const receiverClass = receiver.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const publisherAttribute = publisher.rtiAmbassador().getAttributeHandle(
      publisherClass,
      options.attributeName);
  auto const receiverAttribute = receiver.rtiAmbassador().getAttributeHandle(
      receiverClass,
      options.attributeName);
  require(
      publisherClass.isValid() && receiverClass.isValid() &&
          publisherAttribute.isValid() && receiverAttribute.isValid(),
      "Unregulated timestamped attribute update resolved an invalid FOM handle");

  rti::AttributeHandleSet const publisherAttributes{publisherAttribute};
  rti::AttributeHandleSet const receiverAttributes{receiverAttribute};
  publisher.rtiAmbassador().publishObjectClassAttributes(
      publisherClass,
      publisherAttributes);
  receiver.rtiAmbassador().subscribeObjectClassAttributes(
      receiverClass,
      receiverAttributes);
  auto const object = publisher.rtiAmbassador().registerObjectInstance(
      publisherClass);
  waitFor(
      receiver,
      [&] { return receiver.recorder().discovery().present; },
      options,
      "unregulated timestamped attribute object discovery");

  auto const publisherTime = makeTimeContext(publisher);
  auto const receiverTime = makeTimeContext(receiver);
  require(
      publisherTime.factory->getName() == receiverTime.factory->getName(),
      "Unregulated timestamped attribute members selected different time factories");
  auto const timestamp = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      6U);
  std::vector<std::uint8_t> const valueBytes{
      0x55U, 0x4EU, 0x52U, 0x2DU, 0x41U};
  std::vector<std::uint8_t> const tagBytes{
      0x55U, 0x4EU, 0x52U, 0x2DU, 0x54U};
  rti::AttributeHandleValueMap values;
  values.emplace(
      publisherAttribute,
      rti::VariableLengthData(valueBytes.data(), valueBytes.size()));
  rti::VariableLengthData const tag(tagBytes.data(), tagBytes.size());

  // Neither joined member enables time regulation or time constrained.
  publisher.rtiAmbassador().updateAttributeValues(
      object,
      values,
      tag,
      *timestamp);
  waitFor(
      receiver,
      [&] { return receiver.recorder().timedReflections().size() == 1U; },
      options,
      "unregulated timestamped attribute reflection");
  auto const reflections = receiver.recorder().timedReflections();
  require(
      reflections.size() == 1U &&
          reflections.front().object == object &&
          reflections.front().values.size() == 1U &&
          reflections.front().values.count(receiverAttribute) == 1U &&
          copyBytes(reflections.front().values.at(receiverAttribute)) ==
              valueBytes &&
          reflections.front().tag == tagBytes &&
          reflections.front().producer == publisher.federateHandle() &&
          reflections.front().time == encodeTime(*timestamp) &&
          reflections.front().timeText == timestamp->toString(),
      "RTI did not preserve the unregulated sender's timestamped attribute update");

  receiver.rtiAmbassador().unsubscribeObjectClassAttributes(
      receiverClass,
      receiverAttributes);
  publisher.rtiAmbassador().unpublishObjectClassAttributes(
      publisherClass,
      publisherAttributes);
  receiver.resign(rti::NO_ACTION);
  publisher.resign(rti::NO_ACTION);
  publisher.rtiAmbassador().destroyFederationExecution(federation);
  receiver.disconnect();
  publisher.disconnect();
}

void scenarioUnregulatedSenderTimestampedAttributeUpdateContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioUnregulatedSenderTimestampedAttributeUpdate(options, model);
}

void scenarioUnregulatedSenderTimestampedAttributeUpdatePortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioUnregulatedSenderTimestampedAttributeUpdate(options, model);
}

void scenarioUnregulatedSenderTimestampedAttributeUpdateContractPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioUnregulatedSenderTimestampedAttributeUpdateContract(options, model);
}

}  // namespace hla_rti_cpp_tck::detail
