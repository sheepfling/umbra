#include "tck_empty_attribute_subscription.hpp"
#include "tck_main_scenarios.hpp"

namespace hla_rti_cpp_tck::detail {

void scenarioEmptyAttributeSubscriptionIsNoOp(
    Options const& options,
    rti::CallbackModel model) {
  Session publisher(options, model, "publisher");
  Session subscriber(options, model, "subscriber");
  auto const federation =
      federationName(options, "empty-attribute-subscription-no-op");
  connectAndJoin(publisher, subscriber, options, federation, options.fom);

  auto const publisherClass =
      publisher.rtiAmbassador().getObjectClassHandle(options.objectClassName);
  auto const subscriberClass =
      subscriber.rtiAmbassador().getObjectClassHandle(options.objectClassName);
  auto const publisherAttribute = publisher.rtiAmbassador().getAttributeHandle(
      publisherClass, options.attributeName);
  auto const subscriberAttribute = subscriber.rtiAmbassador().getAttributeHandle(
      subscriberClass, options.attributeName);
  require(
      publisherClass.isValid() && subscriberClass.isValid() &&
          publisherAttribute.isValid() && subscriberAttribute.isValid(),
      "empty-attribute-subscription lookup returned an invalid handle");

  rti::AttributeHandleSet const publishedAttributes{publisherAttribute};
  rti::AttributeHandleSet const subscribedAttributes{subscriberAttribute};
  rti::AttributeHandleSet const emptyAttributes;
  publisher.rtiAmbassador().publishObjectClassAttributes(
      publisherClass, publishedAttributes);
  auto const object =
      publisher.rtiAmbassador().registerObjectInstance(publisherClass);
  require(
      object.isValid(),
      "empty-attribute-subscription registration returned an invalid handle");

  std::vector<std::uint8_t> const initialValue{0x45U, 0x4DU, 0x50U, 0x54U};
  std::vector<std::uint8_t> const initialTag{0x49U, 0x4EU, 0x49U};
  rti::AttributeHandleValueMap initialValues{
      {publisherAttribute,
       rti::VariableLengthData(initialValue.data(), initialValue.size())}};
  rti::VariableLengthData initialUserTag(initialTag.data(), initialTag.size());
  publisher.rtiAmbassador().updateAttributeValues(
      object, initialValues, initialUserTag);

  subscriber.rtiAmbassador().subscribeObjectClassAttributes(
      subscriberClass, subscribedAttributes, true, L"");
  waitFor(
      subscriber,
      [&] { return subscriber.recorder().hasDiscovery(object); },
      options,
      "empty-attribute-subscription initial discovery");
  waitFor(
      subscriber,
      [&] {
        auto const currentReflections = subscriber.recorder().reflections();
        return std::any_of(
            currentReflections.begin(),
            currentReflections.end(),
            [&](ReflectionRecord const& reflection) {
              return reflection.object == object &&
                  reflection.values.count(subscriberAttribute) == 1U;
            });
      },
      options,
      "empty-attribute-subscription initial reflection");

  subscriber.rtiAmbassador().subscribeObjectClassAttributes(
      subscriberClass, emptyAttributes, true, L"");
  std::vector<std::uint8_t> const nextValue{0x4EU, 0x4FU, 0x2DU, 0x4FU};
  std::vector<std::uint8_t> const nextTag{0x4EU, 0x45U, 0x58U};
  rti::AttributeHandleValueMap nextValues{
      {publisherAttribute,
       rti::VariableLengthData(nextValue.data(), nextValue.size())}};
  rti::VariableLengthData nextUserTag(nextTag.data(), nextTag.size());
  publisher.rtiAmbassador().updateAttributeValues(
      object, nextValues, nextUserTag);
  waitFor(
      subscriber,
      [&] { return subscriber.recorder().reflections().size() >= 2U; },
      options,
      "empty-attribute-subscription retained active update delivery");

  auto const reflections = subscriber.recorder().reflections();
  require(
      reflections.size() == 2U &&
          reflections.back().object == object &&
          reflections.back().values.size() == 1U &&
          reflections.back().values.count(subscriberAttribute) == 1U &&
          copyBytes(reflections.back().values.at(subscriberAttribute)) ==
              nextValue &&
          reflections.back().tag == nextTag,
      "empty attribute set changed the existing active subscription");

  subscriber.rtiAmbassador().unsubscribeObjectClassAttributes(
      subscriberClass, subscribedAttributes);
  subscriber.resign(rti::NO_ACTION);
  publisher.resign(rti::NO_ACTION);
  publisher.rtiAmbassador().destroyFederationExecution(federation);
  subscriber.disconnect();
  publisher.disconnect();
}

}  // namespace hla_rti_cpp_tck::detail
