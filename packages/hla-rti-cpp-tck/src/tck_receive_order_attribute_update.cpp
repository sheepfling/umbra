#include "tck_harness.hpp"
#include "tck_main_scenarios.hpp"
#include "tck_receive_order_attribute_update.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace hla_rti_cpp_tck::detail {

void scenarioReceiveOrderAttributeUpdate(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.fom.empty(),
      "Receive-order attribute-update testing requires an adapter-supplied FOM");

  Session publisher(options, model, "receive-order-attribute-publisher");
  Session receiver(options, model, "receive-order-attribute-receiver");
  auto const federation = federationName(
      options,
      "receive-order-attribute-update");
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
      "Receive-order attribute-update lookup returned an invalid standard handle");
  require(
      publisher.rtiAmbassador().getObjectClassName(publisherClass) ==
              options.objectClassName &&
          receiver.rtiAmbassador().getObjectClassName(receiverClass) ==
              options.objectClassName &&
          publisher.rtiAmbassador().getAttributeName(
              publisherClass,
              publisherAttribute) == options.attributeName &&
          receiver.rtiAmbassador().getAttributeName(
              receiverClass,
              receiverAttribute) == options.attributeName,
      "Receive-order attribute-update lookup did not round-trip adapter names");

  rti::AttributeHandleSet const publisherAttributes{publisherAttribute};
  rti::AttributeHandleSet const receiverAttributes{receiverAttribute};
  publisher.rtiAmbassador().publishObjectClassAttributes(
      publisherClass,
      publisherAttributes);
  receiver.rtiAmbassador().subscribeObjectClassAttributes(
      receiverClass,
      receiverAttributes,
      true,
      L"");

  auto const object = publisher.rtiAmbassador().registerObjectInstance(publisherClass);
  require(
      object.isValid(),
      "Receive-order attribute-update registration returned an invalid object handle");
  waitFor(
      receiver,
      [&] { return receiver.recorder().hasDiscovery(object); },
      options,
      "receive-order attribute-update discovery");
  receiver.recorder().clearReflection();
  publisher.recorder().clearReflection();

  std::vector<std::uint8_t> const firstValue{0x52U, 0x31U};
  std::vector<std::uint8_t> const firstTag{0x54U, 0x31U};
  std::vector<std::uint8_t> const secondValue{0x52U, 0x32U};
  std::vector<std::uint8_t> const secondTag{0x54U, 0x32U};
  auto send = [&](std::vector<std::uint8_t> const& value,
                  std::vector<std::uint8_t> const& tagBytes) {
    rti::AttributeHandleValueMap values;
    values.emplace(
        publisherAttribute,
        rti::VariableLengthData(value.data(), value.size()));
    rti::VariableLengthData tag(tagBytes.data(), tagBytes.size());
    publisher.rtiAmbassador().updateAttributeValues(object, values, tag);
  };
  send(firstValue, firstTag);
  send(secondValue, secondTag);

  if (model == rti::HLA_EVOKED) {
    require(
        receiver.recorder().reflections().empty(),
        "Receive-order attribute-update delivered before callback servicing");
  }
  waitFor(
      receiver,
      [&] { return receiver.recorder().reflections().size() >= 2U; },
      options,
      "receive-order attribute-update reflections");

  auto const received = receiver.recorder().reflections();
  require(
      received.size() == 2U,
      "Receive-order attribute-update delivered an unexpected callback count");
  auto assertReflection = [&](ReflectionRecord const& reflection,
                              std::vector<std::uint8_t> const& expectedValue,
                              std::vector<std::uint8_t> const& expectedTag,
                              std::string const& description) {
    require(
        reflection.present && reflection.object == object &&
            reflection.values.size() == 1U &&
            reflection.values.count(receiverAttribute) == 1U &&
            copyBytes(reflection.values.at(receiverAttribute)) == expectedValue,
        description + " returned the wrong object, attribute, or value");
    require(
        reflection.tag == expectedTag &&
            reflection.producer == publisher.federateHandle(),
        description + " returned the wrong tag or producing federate");
    require(
        reflection.transportation.isValid() &&
            !receiver.rtiAmbassador().getTransportationTypeName(
                reflection.transportation).empty() &&
            !reflection.regions.has_value(),
        description + " returned invalid receive-order reflection metadata");
  };
  assertReflection(
      received.front(),
      firstValue,
      firstTag,
      "first receive-order attribute-update reflection");
  assertReflection(
      received.back(),
      secondValue,
      secondTag,
      "second receive-order attribute-update reflection");
  require(
      publisher.recorder().reflections().empty(),
      "Receive-order attribute-update looped a reflection back to its publisher");

  receiver.rtiAmbassador().unsubscribeObjectClassAttributes(
      receiverClass,
      receiverAttributes);
  publisher.rtiAmbassador().unpublishObjectClassAttributes(
      publisherClass,
      publisherAttributes);
  receiver.resign(rti::NO_ACTION);
  publisher.resign(rti::DELETE_OBJECTS);
  publisher.rtiAmbassador().destroyFederationExecution(federation);
  receiver.disconnect();
  publisher.disconnect();
}

void scenarioReceiveOrderAttributeUpdateContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioReceiveOrderAttributeUpdate(options, model);
}

}  // namespace hla_rti_cpp_tck::detail
