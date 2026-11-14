#include "tck_federate_update_threshold_scenarios.hpp"

#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

namespace hla_rti_cpp_tck::detail {

void scenarioFederateSelectedUpdateThresholds(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.fom.empty(),
      "Federate-selected update-threshold testing requires an adapter FOM");

  Session fastPublisher(options, model, "update-threshold-fast-publisher");
  Session slowPublisher(options, model, "update-threshold-slow-publisher");
  Session receiver(options, model, "update-threshold-receiver");
  auto const federation = federationName(options, "federate-selected-update-thresholds");

  fastPublisher.connect();
  slowPublisher.connect();
  receiver.connect();
  fastPublisher.rtiAmbassador().createFederationExecution(
      federation,
      options.fom.wstring(),
      options.logicalTimeImplementationName);
  fastPublisher.join(options.ownerFederateName, options.federateType, federation);
  slowPublisher.join(
      options.ownerFederateName + L"-threshold-3",
      options.federateType,
      federation);
  receiver.join(options.memberFederateName, options.federateType, federation);

  auto const fastClass = fastPublisher.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const slowClass = slowPublisher.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const receiverClass = receiver.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const fastAttribute = fastPublisher.rtiAmbassador().getAttributeHandle(
      fastClass,
      options.attributeName);
  auto const slowAttribute = slowPublisher.rtiAmbassador().getAttributeHandle(
      slowClass,
      options.attributeName);
  auto const receiverAttribute = receiver.rtiAmbassador().getAttributeHandle(
      receiverClass,
      options.attributeName);
  require(
      fastClass.isValid() && slowClass.isValid() && receiverClass.isValid() &&
          fastAttribute.isValid() && slowAttribute.isValid() &&
          receiverAttribute.isValid() && fastClass == slowClass &&
          fastClass == receiverClass && fastAttribute == slowAttribute &&
          fastAttribute == receiverAttribute,
      "Federate-selected update-threshold FOM lookup returned inconsistent handles");

  rti::AttributeHandleSet const fastAttributes{fastAttribute};
  rti::AttributeHandleSet const slowAttributes{slowAttribute};
  rti::AttributeHandleSet const receiverAttributes{receiverAttribute};
  fastPublisher.rtiAmbassador().publishObjectClassAttributes(
      fastClass,
      fastAttributes);
  slowPublisher.rtiAmbassador().publishObjectClassAttributes(
      slowClass,
      slowAttributes);
  receiver.rtiAmbassador().subscribeObjectClassAttributes(
      receiverClass,
      receiverAttributes,
      true,
      L"");

  auto const fastObject = fastPublisher.rtiAmbassador().registerObjectInstance(fastClass);
  auto const slowObject = slowPublisher.rtiAmbassador().registerObjectInstance(slowClass);
  require(
      fastObject.isValid() && slowObject.isValid() && fastObject != slowObject,
      "Federate-selected update-threshold registration returned invalid or duplicate objects");
  waitFor(
      receiver,
      [&] {
        return receiver.recorder().hasDiscovery(fastObject) &&
            receiver.recorder().hasDiscovery(slowObject);
      },
      options,
      "federate-selected update-threshold object discovery");

  constexpr std::uint8_t fastThreshold = 2U;
  constexpr std::uint8_t slowThreshold = 3U;
  auto sendAtThreshold = [](
                          Session& publisher,
                          rti::ObjectInstanceHandle const& object,
                          rti::AttributeHandle const& attribute,
                          std::uint8_t threshold,
                          std::uint8_t sample) {
    std::vector<std::uint8_t> const valueBytes{sample};
    std::vector<std::uint8_t> const tagBytes{threshold, sample};
    rti::AttributeHandleValueMap values;
    values.emplace(
        attribute,
        rti::VariableLengthData(valueBytes.data(), valueBytes.size()));
    publisher.rtiAmbassador().updateAttributeValues(
        object,
        values,
        rti::VariableLengthData(tagBytes.data(), tagBytes.size()));
  };

  for (std::uint8_t sample = 1U; sample <= 6U; ++sample) {
    if (sample % fastThreshold == 0U) {
      sendAtThreshold(
          fastPublisher,
          fastObject,
          fastAttribute,
          fastThreshold,
          sample);
    }
    if (sample % slowThreshold == 0U) {
      sendAtThreshold(
          slowPublisher,
          slowObject,
          slowAttribute,
          slowThreshold,
          sample);
    }
  }

  waitFor(
      receiver,
      [&] { return receiver.recorder().reflections().size() >= 5U; },
      options,
      "federate-selected threshold update reflections");
  std::vector<std::uint8_t> fastSamples;
  std::vector<std::uint8_t> slowSamples;
  for (auto const& reflection : receiver.recorder().reflections()) {
    require(
        reflection.present && reflection.values.size() == 1U &&
            reflection.values.count(receiverAttribute) == 1U &&
            !reflection.regions.has_value(),
        "Federate-selected threshold reflection returned unexpected attribute metadata");
    auto const valueBytes = copyBytes(reflection.values.at(receiverAttribute));
    require(
        valueBytes.size() == 1U,
        "Federate-selected threshold reflection returned a malformed sample value");
    auto const sample = valueBytes.front();
    auto const expectedTag = std::vector<std::uint8_t>{
        reflection.object == fastObject ? fastThreshold : slowThreshold,
        sample};
    require(
        reflection.tag == expectedTag,
        "Federate-selected threshold reflection returned the wrong sender threshold tag");
    if (reflection.object == fastObject) {
      require(
          reflection.producer == fastPublisher.federateHandle(),
          "Fast-threshold update reflection identified the wrong producer");
      fastSamples.push_back(sample);
    } else if (reflection.object == slowObject) {
      require(
          reflection.producer == slowPublisher.federateHandle(),
          "Slow-threshold update reflection identified the wrong producer");
      slowSamples.push_back(sample);
    } else {
      throw std::runtime_error(
          "Federate-selected threshold test received an unknown object");
    }
  }
  require(
      receiver.recorder().reflections().size() == 5U &&
          fastSamples == std::vector<std::uint8_t>{2U, 4U, 6U} &&
          slowSamples == std::vector<std::uint8_t>{3U, 6U},
      "RTI reflections did not match the two federates' distinct threshold-selected updates");

  receiver.rtiAmbassador().unsubscribeObjectClassAttributes(
      receiverClass,
      receiverAttributes);
  fastPublisher.rtiAmbassador().unpublishObjectClassAttributes(
      fastClass,
      fastAttributes);
  slowPublisher.rtiAmbassador().unpublishObjectClassAttributes(
      slowClass,
      slowAttributes);
  fastPublisher.resign(rti::DELETE_OBJECTS);
  slowPublisher.resign(rti::DELETE_OBJECTS);
  receiver.resign(rti::NO_ACTION);
  fastPublisher.rtiAmbassador().destroyFederationExecution(federation);
  fastPublisher.disconnect();
  slowPublisher.disconnect();
  receiver.disconnect();
}

void scenarioFederateSelectedUpdateThresholdsContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioFederateSelectedUpdateThresholds(options, model);
}

}  // namespace hla_rti_cpp_tck::detail
