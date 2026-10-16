#include "tck_selective_attribute_unsubscription.hpp"
#include "tck_main_scenarios.hpp"

#include <cstdint>
#include <vector>

namespace hla_rti_cpp_tck::detail {

void scenarioSelectiveAttributeUnsubscription(
    Options const& options,
    rti::CallbackModel model) {
  Session publisher(options, model, "publisher");
  Session subscriber(options, model, "subscriber");
  auto const federation =
      federationName(options, "selective-attribute-unsubscription");
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
      "selective unsubscription lookup returned an invalid handle");

  rti::AttributeHandleSet const publishedAttributes{
      publisherFirstAttribute, publisherSecondAttribute};
  rti::AttributeHandleSet const subscribedAttributes{
      subscriberFirstAttribute, subscriberSecondAttribute};
  rti::AttributeHandleSet const unsubscribedAttributes{subscriberFirstAttribute};
  publisher.rtiAmbassador().publishObjectClassAttributes(
      publisherClass, publishedAttributes);
  subscriber.rtiAmbassador().subscribeObjectClassAttributes(
      subscriberClass, subscribedAttributes, true, L"");
  auto const object = publisher.rtiAmbassador().registerObjectInstance(
      publisherClass);
  require(
      object.isValid(),
      "selective unsubscription registration returned an invalid handle");
  waitFor(
      subscriber,
      [&] { return subscriber.recorder().hasDiscovery(object); },
      options,
      "selective unsubscription object discovery");

  std::vector<std::uint8_t> const firstValue{0x53U, 0x45U, 0x4CU, 0x31U};
  std::vector<std::uint8_t> const firstOtherValue{0x52U, 0x45U, 0x4DU, 0x31U};
  std::vector<std::uint8_t> const firstTag{0x49U, 0x4EU, 0x49U};
  auto sendUpdate = [&](std::vector<std::uint8_t> const& value,
                        std::vector<std::uint8_t> const& otherValue,
                        std::vector<std::uint8_t> const& tagBytes) {
    rti::AttributeHandleValueMap const values{
        {publisherFirstAttribute,
         rti::VariableLengthData(value.data(), value.size())},
        {publisherSecondAttribute,
         rti::VariableLengthData(otherValue.data(), otherValue.size())}};
    rti::VariableLengthData const tag(tagBytes.data(), tagBytes.size());
    publisher.rtiAmbassador().updateAttributeValues(object, values, tag);
  };
  sendUpdate(firstValue, firstOtherValue, firstTag);
  waitFor(
      subscriber,
      [&] { return !subscriber.recorder().reflections().empty(); },
      options,
      "initial two-attribute subscription reflection");
  auto const initialReflections = subscriber.recorder().reflections();
  require(
      initialReflections.size() == 1U &&
          initialReflections.front().values.size() == 2U &&
          initialReflections.front().values.count(subscriberFirstAttribute) == 1U &&
          initialReflections.front().values.count(subscriberSecondAttribute) == 1U,
      "initial subscription did not receive both published attributes");

  subscriber.recorder().clearReflection();
  subscriber.rtiAmbassador().unsubscribeObjectClassAttributes(
      subscriberClass, unsubscribedAttributes);
  std::vector<std::uint8_t> const secondValue{0x53U, 0x45U, 0x4CU, 0x32U};
  std::vector<std::uint8_t> const secondOtherValue{0x52U, 0x45U, 0x4DU, 0x32U};
  std::vector<std::uint8_t> const secondTag{0x41U, 0x46U, 0x54U};
  sendUpdate(secondValue, secondOtherValue, secondTag);
  waitFor(
      subscriber,
      [&] { return subscriber.recorder().reflection().present; },
      options,
      "remaining-attribute reflection after selective unsubscription");
  auto const remainingReflection = subscriber.recorder().reflection();
  require(
      remainingReflection.object == object &&
          remainingReflection.values.size() == 1U &&
          remainingReflection.values.count(subscriberFirstAttribute) == 0U &&
          remainingReflection.values.count(subscriberSecondAttribute) == 1U &&
          copyBytes(remainingReflection.values.at(subscriberSecondAttribute)) ==
              secondOtherValue &&
          remainingReflection.tag == secondTag,
      "unsubscribing one attribute removed another subscribed attribute or delivered the removed value");

  subscriber.rtiAmbassador().unsubscribeObjectClassAttributes(
      subscriberClass,
      rti::AttributeHandleSet{subscriberSecondAttribute});
  publisher.rtiAmbassador().unpublishObjectClassAttributes(
      publisherClass, publishedAttributes);
  subscriber.resign(rti::NO_ACTION);
  publisher.resign(rti::NO_ACTION);
  publisher.rtiAmbassador().destroyFederationExecution(federation);
  subscriber.disconnect();
  publisher.disconnect();
}

}  // namespace hla_rti_cpp_tck::detail
