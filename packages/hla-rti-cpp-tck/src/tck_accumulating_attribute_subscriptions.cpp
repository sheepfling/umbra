#include "tck_accumulating_attribute_subscriptions.hpp"
#include "tck_main_scenarios.hpp"

#include <cstdint>
#include <vector>

namespace hla_rti_cpp_tck::detail {

void scenarioAttributeSubscriptionInvocationsAccumulate(
    Options const& options,
    rti::CallbackModel model) {
  Session publisher(options, model, "publisher");
  Session subscriber(options, model, "subscriber");
  auto const federation =
      federationName(options, "attribute-subscription-invocations-accumulate");
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
      "accumulating subscription lookup returned an invalid handle");

  rti::AttributeHandleSet const publishedAttributes{
      publisherFirstAttribute, publisherSecondAttribute};
  rti::AttributeHandleSet const firstSubscription{subscriberFirstAttribute};
  rti::AttributeHandleSet const secondSubscription{subscriberSecondAttribute};
  publisher.rtiAmbassador().publishObjectClassAttributes(
      publisherClass, publishedAttributes);
  subscriber.rtiAmbassador().subscribeObjectClassAttributes(
      subscriberClass, firstSubscription, true, L"");
  auto const object = publisher.rtiAmbassador().registerObjectInstance(
      publisherClass);
  require(
      object.isValid(),
      "accumulating subscription registration returned an invalid handle");
  waitFor(
      subscriber,
      [&] { return subscriber.recorder().hasDiscovery(object); },
      options,
      "first attribute subscription discovery");

  subscriber.rtiAmbassador().subscribeObjectClassAttributes(
      subscriberClass, secondSubscription, true, L"");
  std::vector<std::uint8_t> const firstValue{0x41U, 0x43U, 0x43U, 0x31U};
  std::vector<std::uint8_t> const secondValue{0x41U, 0x43U, 0x43U, 0x32U};
  std::vector<std::uint8_t> const userTag{0x41U, 0x44U, 0x44U};
  rti::AttributeHandleValueMap const values{
      {publisherFirstAttribute,
       rti::VariableLengthData(firstValue.data(), firstValue.size())},
      {publisherSecondAttribute,
       rti::VariableLengthData(secondValue.data(), secondValue.size())}};
  rti::VariableLengthData const tag(userTag.data(), userTag.size());
  publisher.rtiAmbassador().updateAttributeValues(object, values, tag);
  waitFor(
      subscriber,
      [&] { return !subscriber.recorder().reflections().empty(); },
      options,
      "accumulated attribute subscription reflection");

  auto const reflections = subscriber.recorder().reflections();
  require(
      reflections.size() == 1U && reflections.front().object == object &&
          reflections.front().values.size() == 2U &&
          reflections.front().values.count(subscriberFirstAttribute) == 1U &&
          reflections.front().values.count(subscriberSecondAttribute) == 1U &&
          copyBytes(reflections.front().values.at(subscriberFirstAttribute)) ==
              firstValue &&
          copyBytes(reflections.front().values.at(subscriberSecondAttribute)) ==
              secondValue &&
          reflections.front().tag == userTag,
      "a later class subscription replaced rather than accumulated the earlier attribute set");

  subscriber.rtiAmbassador().unsubscribeObjectClassAttributes(
      subscriberClass,
      rti::AttributeHandleSet{subscriberFirstAttribute, subscriberSecondAttribute});
  publisher.rtiAmbassador().unpublishObjectClassAttributes(
      publisherClass, publishedAttributes);
  subscriber.resign(rti::NO_ACTION);
  publisher.resign(rti::NO_ACTION);
  publisher.rtiAmbassador().destroyFederationExecution(federation);
  subscriber.disconnect();
  publisher.disconnect();
}

}  // namespace hla_rti_cpp_tck::detail
