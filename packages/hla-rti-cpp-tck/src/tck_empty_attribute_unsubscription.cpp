#include "tck_empty_attribute_unsubscription.hpp"
#include "tck_main_scenarios.hpp"

#include <cstdint>
#include <vector>

namespace hla_rti_cpp_tck::detail {

void scenarioEmptyAttributeUnsubscriptionIsNoOp(
    Options const& options,
    rti::CallbackModel model) {
  Session publisher(options, model, "publisher");
  Session subscriber(options, model, "subscriber");
  auto const federation =
      federationName(options, "empty-attribute-unsubscription-no-op");
  connectAndJoin(publisher, subscriber, options, federation, options.fom);

  auto const publisherClass =
      publisher.rtiAmbassador().getObjectClassHandle(options.objectClassName);
  auto const subscriberClass =
      subscriber.rtiAmbassador().getObjectClassHandle(options.objectClassName);
  auto const publisherFirstAttribute = publisher.rtiAmbassador().getAttributeHandle(
      publisherClass, options.attributeName);
  auto const publisherSecondAttribute = publisher.rtiAmbassador().getAttributeHandle(
      publisherClass, options.secondaryAttributeName);
  auto const subscriberFirstAttribute = subscriber.rtiAmbassador().getAttributeHandle(
      subscriberClass, options.attributeName);
  auto const subscriberSecondAttribute = subscriber.rtiAmbassador().getAttributeHandle(
      subscriberClass, options.secondaryAttributeName);
  require(
      publisherClass.isValid() && subscriberClass.isValid() &&
          publisherFirstAttribute.isValid() && publisherSecondAttribute.isValid() &&
          subscriberFirstAttribute.isValid() && subscriberSecondAttribute.isValid(),
      "empty-set unsubscription lookup returned an invalid handle");

  rti::AttributeHandleSet const publishedAttributes{
      publisherFirstAttribute, publisherSecondAttribute};
  rti::AttributeHandleSet const subscribedAttributes{
      subscriberFirstAttribute, subscriberSecondAttribute};
  rti::AttributeHandleSet const emptyAttributes;
  publisher.rtiAmbassador().publishObjectClassAttributes(
      publisherClass, publishedAttributes);
  subscriber.rtiAmbassador().subscribeObjectClassAttributes(
      subscriberClass, subscribedAttributes, true, L"");
  auto const object = publisher.rtiAmbassador().registerObjectInstance(
      publisherClass);
  require(
      object.isValid(),
      "empty-set unsubscription registration returned an invalid handle");
  waitFor(
      subscriber,
      [&] { return subscriber.recorder().hasDiscovery(object); },
      options,
      "empty-set unsubscription object discovery");

  auto sendUpdate = [&](std::uint8_t sequence) {
    std::vector<std::uint8_t> const firstValue{0x45U, 0x4DU, 0x50U, sequence};
    std::vector<std::uint8_t> const secondValue{0x54U, 0x41U, 0x49U, sequence};
    std::vector<std::uint8_t> const tagBytes{0x45U, 0x4DU, sequence};
    rti::AttributeHandleValueMap const values{
        {publisherFirstAttribute,
         rti::VariableLengthData(firstValue.data(), firstValue.size())},
        {publisherSecondAttribute,
         rti::VariableLengthData(secondValue.data(), secondValue.size())}};
    rti::VariableLengthData const tag(tagBytes.data(), tagBytes.size());
    publisher.rtiAmbassador().updateAttributeValues(object, values, tag);
  };
  sendUpdate(0x31U);
  waitFor(
      subscriber,
      [&] { return subscriber.recorder().reflection().present; },
      options,
      "empty-set unsubscription positive delivery control");
  auto const beforeUnsubscribe = subscriber.recorder().reflection();
  require(
      beforeUnsubscribe.object == object &&
          beforeUnsubscribe.values.size() == 2U &&
          beforeUnsubscribe.values.count(subscriberFirstAttribute) == 1U &&
          beforeUnsubscribe.values.count(subscriberSecondAttribute) == 1U,
      "active subscription did not receive both attributes before empty-set unsubscription");

  subscriber.recorder().clearReflection();
  subscriber.rtiAmbassador().unsubscribeObjectClassAttributes(
      subscriberClass, emptyAttributes);
  sendUpdate(0x32U);
  waitFor(
      subscriber,
      [&] { return subscriber.recorder().reflection().present; },
      options,
      "active subscription delivery after empty-set unsubscription");
  auto const afterUnsubscribe = subscriber.recorder().reflection();
  require(
      afterUnsubscribe.object == object &&
          afterUnsubscribe.values.size() == 2U &&
          afterUnsubscribe.values.count(subscriberFirstAttribute) == 1U &&
          afterUnsubscribe.values.count(subscriberSecondAttribute) == 1U &&
          copyBytes(afterUnsubscribe.values.at(subscriberFirstAttribute)) ==
              std::vector<std::uint8_t>{0x45U, 0x4DU, 0x50U, 0x32U} &&
          copyBytes(afterUnsubscribe.values.at(subscriberSecondAttribute)) ==
              std::vector<std::uint8_t>{0x54U, 0x41U, 0x49U, 0x32U} &&
          afterUnsubscribe.tag == std::vector<std::uint8_t>{0x45U, 0x4DU, 0x32U},
      "empty attribute set removed a subscription from the object class");

  subscriber.rtiAmbassador().unsubscribeObjectClassAttributes(
      subscriberClass, subscribedAttributes);
  publisher.rtiAmbassador().unpublishObjectClassAttributes(
      publisherClass, publishedAttributes);
  subscriber.resign(rti::NO_ACTION);
  publisher.resign(rti::NO_ACTION);
  publisher.rtiAmbassador().destroyFederationExecution(federation);
  subscriber.disconnect();
  publisher.disconnect();
}

}  // namespace hla_rti_cpp_tck::detail
