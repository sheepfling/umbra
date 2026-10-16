#include "tck_whole_class_attribute_unsubscription.hpp"
#include "tck_main_scenarios.hpp"

#include <cstdint>
#include <vector>

namespace hla_rti_cpp_tck::detail {

void scenarioWholeClassAttributeUnsubscription(
    Options const& options,
    rti::CallbackModel model) {
  Session publisher(options, model, "publisher");
  Session subscriber(options, model, "subscriber");
  auto const federation =
      federationName(options, "whole-class-attribute-unsubscription");
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
      "whole-class unsubscription lookup returned an invalid handle");

  rti::AttributeHandleSet const publishedAttributes{
      publisherFirstAttribute, publisherSecondAttribute};
  rti::AttributeHandleSet const subscribedAttributes{
      subscriberFirstAttribute, subscriberSecondAttribute};
  publisher.rtiAmbassador().publishObjectClassAttributes(
      publisherClass, publishedAttributes);
  subscriber.rtiAmbassador().subscribeObjectClassAttributes(
      subscriberClass, subscribedAttributes, true, L"");
  auto const object = publisher.rtiAmbassador().registerObjectInstance(
      publisherClass);
  require(
      object.isValid(),
      "whole-class unsubscription registration returned an invalid handle");
  waitFor(
      subscriber,
      [&] { return subscriber.recorder().hasDiscovery(object); },
      options,
      "whole-class unsubscription object discovery");

  auto sendUpdate = [&](std::uint8_t sequence) {
    std::vector<std::uint8_t> const firstValue{0x41U, 0x4CU, 0x4CU, sequence};
    std::vector<std::uint8_t> const secondValue{0x41U, 0x4CU, 0x4CU, sequence, 0x32U};
    std::vector<std::uint8_t> const tagBytes{0x41U, 0x4CU, sequence};
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
      "whole-class unsubscription positive delivery control");
  auto const beforeUnsubscribe = subscriber.recorder().reflection();
  require(
      beforeUnsubscribe.object == object &&
          beforeUnsubscribe.values.size() == 2U &&
          beforeUnsubscribe.values.count(subscriberFirstAttribute) == 1U &&
          beforeUnsubscribe.values.count(subscriberSecondAttribute) == 1U,
      "active subscription did not receive both attributes before class unsubscription");

  subscriber.recorder().clearReflection();
  subscriber.rtiAmbassador().unsubscribeObjectClass(subscriberClass);
  sendUpdate(0x32U);
  for (int pass = 0; pass != 8; ++pass) {
    subscriber.pump();
  }
  require(
      !subscriber.recorder().reflection().present,
      "whole-class unsubscription left one or more class attributes subscribed");

  publisher.rtiAmbassador().unpublishObjectClassAttributes(
      publisherClass, publishedAttributes);
  subscriber.resign(rti::NO_ACTION);
  publisher.resign(rti::NO_ACTION);
  publisher.rtiAmbassador().destroyFederationExecution(federation);
  subscriber.disconnect();
  publisher.disconnect();
}

}  // namespace hla_rti_cpp_tck::detail
