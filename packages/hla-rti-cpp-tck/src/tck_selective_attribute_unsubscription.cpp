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
  Session retainedSubscriber(options, model, "member");
  auto const federation =
      federationName(options, "selective-attribute-unsubscription");
  connectAndJoin(publisher, subscriber, options, federation, options.fom);
  retainedSubscriber.connect();
  retainedSubscriber.join(
      options.memberFederateName + L"-retained",
      options.federateType,
      federation);

  auto const publisherClass =
      publisher.rtiAmbassador().getObjectClassHandle(options.objectClassName);
  auto const subscriberClass =
      subscriber.rtiAmbassador().getObjectClassHandle(options.objectClassName);
  auto const retainedClass =
      retainedSubscriber.rtiAmbassador().getObjectClassHandle(options.objectClassName);
  auto const publisherFirstAttribute = publisher.rtiAmbassador().getAttributeHandle(
      publisherClass, options.attributeName);
  auto const publisherSecondAttribute = publisher.rtiAmbassador().getAttributeHandle(
      publisherClass, options.secondaryAttributeName);
  auto const subscriberFirstAttribute = subscriber.rtiAmbassador().getAttributeHandle(
      subscriberClass, options.attributeName);
  auto const subscriberSecondAttribute = subscriber.rtiAmbassador().getAttributeHandle(
      subscriberClass, options.secondaryAttributeName);
  auto const retainedFirstAttribute = retainedSubscriber.rtiAmbassador().getAttributeHandle(
      retainedClass, options.attributeName);
  auto const retainedSecondAttribute = retainedSubscriber.rtiAmbassador().getAttributeHandle(
      retainedClass, options.secondaryAttributeName);
  require(
      publisherClass.isValid() && subscriberClass.isValid() &&
          retainedClass.isValid() &&
          publisherFirstAttribute.isValid() && publisherSecondAttribute.isValid() &&
          subscriberFirstAttribute.isValid() && subscriberSecondAttribute.isValid() &&
          retainedFirstAttribute.isValid() && retainedSecondAttribute.isValid(),
      "selective unsubscription lookup returned an invalid handle");

  rti::AttributeHandleSet const publishedAttributes{
      publisherFirstAttribute, publisherSecondAttribute};
  rti::AttributeHandleSet const subscribedAttributes{
      subscriberFirstAttribute, subscriberSecondAttribute};
  rti::AttributeHandleSet const retainedAttributes{
      retainedFirstAttribute, retainedSecondAttribute};
  rti::AttributeHandleSet const unsubscribedAttributes{subscriberFirstAttribute};
  publisher.rtiAmbassador().publishObjectClassAttributes(
      publisherClass, publishedAttributes);
  subscriber.rtiAmbassador().subscribeObjectClassAttributes(
      subscriberClass, subscribedAttributes, true, L"");
  retainedSubscriber.rtiAmbassador().subscribeObjectClassAttributes(
      retainedClass, retainedAttributes, true, L"");
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
  waitFor(
      retainedSubscriber,
      [&] { return retainedSubscriber.recorder().hasDiscovery(object); },
      options,
      "retained subscriber object discovery");

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
  waitFor(
      retainedSubscriber,
      [&] { return !retainedSubscriber.recorder().reflections().empty(); },
      options,
      "initial retained-subscriber reflection");
  auto const initialReflections = subscriber.recorder().reflections();
  auto const retainedInitialReflections =
      retainedSubscriber.recorder().reflections();
  require(
      initialReflections.size() == 1U &&
          initialReflections.front().values.size() == 2U &&
          initialReflections.front().values.count(subscriberFirstAttribute) == 1U &&
          initialReflections.front().values.count(subscriberSecondAttribute) == 1U,
      "initial subscription did not receive both published attributes");
  require(
      retainedInitialReflections.size() == 1U &&
          retainedInitialReflections.front().values.size() == 2U &&
          retainedInitialReflections.front().values.count(retainedFirstAttribute) == 1U &&
          retainedInitialReflections.front().values.count(retainedSecondAttribute) == 1U,
      "second federate's independent subscription did not receive both attributes");

  subscriber.recorder().clearReflection();
  retainedSubscriber.recorder().clearReflection();
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
  waitFor(
      retainedSubscriber,
      [&] { return retainedSubscriber.recorder().reflection().present; },
      options,
      "retained subscription reflection after another federate unsubscribes");
  auto const remainingReflection = subscriber.recorder().reflection();
  auto const retainedReflection = retainedSubscriber.recorder().reflection();
  require(
      remainingReflection.object == object &&
          remainingReflection.values.size() == 1U &&
          remainingReflection.values.count(subscriberFirstAttribute) == 0U &&
          remainingReflection.values.count(subscriberSecondAttribute) == 1U &&
          copyBytes(remainingReflection.values.at(subscriberSecondAttribute)) ==
              secondOtherValue &&
          remainingReflection.tag == secondTag,
      "unsubscribing one attribute removed another subscribed attribute or delivered the removed value");
  require(
      retainedReflection.object == object &&
          retainedReflection.values.size() == 2U &&
          retainedReflection.values.count(retainedFirstAttribute) == 1U &&
          retainedReflection.values.count(retainedSecondAttribute) == 1U &&
          copyBytes(retainedReflection.values.at(retainedFirstAttribute)) == secondValue &&
          copyBytes(retainedReflection.values.at(retainedSecondAttribute)) ==
              secondOtherValue &&
          retainedReflection.tag == secondTag,
      "one federate's attribute unsubscription changed another federate's subscription");

  subscriber.rtiAmbassador().unsubscribeObjectClassAttributes(
      subscriberClass,
      rti::AttributeHandleSet{subscriberSecondAttribute});
  retainedSubscriber.rtiAmbassador().unsubscribeObjectClassAttributes(
      retainedClass, retainedAttributes);
  publisher.rtiAmbassador().unpublishObjectClassAttributes(
      publisherClass, publishedAttributes);
  subscriber.resign(rti::NO_ACTION);
  retainedSubscriber.resign(rti::NO_ACTION);
  publisher.resign(rti::NO_ACTION);
  publisher.rtiAmbassador().destroyFederationExecution(federation);
  subscriber.disconnect();
  retainedSubscriber.disconnect();
  publisher.disconnect();
}

}  // namespace hla_rti_cpp_tck::detail
