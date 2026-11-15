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
  Session passiveSubscriber(options, model, "passive-subscriber");
  auto const federation =
      federationName(options, "empty-attribute-unsubscription-no-op");
  connectAndJoin(publisher, subscriber, options, federation, options.fom);
  passiveSubscriber.connect();
  passiveSubscriber.join(
      options.memberFederateName + L"-passive-empty-unsubscribe",
      options.federateType,
      federation);

  auto const publisherClass =
      publisher.rtiAmbassador().getObjectClassHandle(options.objectClassName);
  auto const subscriberClass =
      subscriber.rtiAmbassador().getObjectClassHandle(options.objectClassName);
  auto const passiveSubscriberClass =
      passiveSubscriber.rtiAmbassador().getObjectClassHandle(options.objectClassName);
  auto const publisherFirstAttribute = publisher.rtiAmbassador().getAttributeHandle(
      publisherClass, options.attributeName);
  auto const publisherSecondAttribute = publisher.rtiAmbassador().getAttributeHandle(
      publisherClass, options.secondaryAttributeName);
  auto const subscriberFirstAttribute = subscriber.rtiAmbassador().getAttributeHandle(
      subscriberClass, options.attributeName);
  auto const subscriberSecondAttribute = subscriber.rtiAmbassador().getAttributeHandle(
      subscriberClass, options.secondaryAttributeName);
  auto const passiveSubscriberFirstAttribute =
      passiveSubscriber.rtiAmbassador().getAttributeHandle(
          passiveSubscriberClass, options.attributeName);
  auto const passiveSubscriberSecondAttribute =
      passiveSubscriber.rtiAmbassador().getAttributeHandle(
          passiveSubscriberClass, options.secondaryAttributeName);
  require(
      publisherClass.isValid() && subscriberClass.isValid() &&
          passiveSubscriberClass.isValid() &&
          publisherFirstAttribute.isValid() && publisherSecondAttribute.isValid() &&
          subscriberFirstAttribute.isValid() && subscriberSecondAttribute.isValid() &&
          passiveSubscriberFirstAttribute.isValid() &&
          passiveSubscriberSecondAttribute.isValid(),
      "empty-set unsubscription lookup returned an invalid handle");

  rti::AttributeHandleSet const publishedAttributes{
      publisherFirstAttribute, publisherSecondAttribute};
  rti::AttributeHandleSet const subscribedAttributes{
      subscriberFirstAttribute, subscriberSecondAttribute};
  rti::AttributeHandleSet const passiveSubscribedAttributes{
      passiveSubscriberFirstAttribute, passiveSubscriberSecondAttribute};
  rti::AttributeHandleSet const emptyAttributes;
  publisher.rtiAmbassador().publishObjectClassAttributes(
      publisherClass, publishedAttributes);
  subscriber.rtiAmbassador().subscribeObjectClassAttributes(
      subscriberClass, subscribedAttributes, true, L"");
  passiveSubscriber.rtiAmbassador().subscribeObjectClassAttributes(
      passiveSubscriberClass,
      passiveSubscribedAttributes,
      false,
      L"");
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
  waitFor(
      passiveSubscriber,
      [&] { return passiveSubscriber.recorder().hasDiscovery(object); },
      options,
      "passive subscription discovery positive control before empty-set unsubscription");

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
  waitFor(
      passiveSubscriber,
      [&] { return passiveSubscriber.recorder().reflection().present; },
      options,
      "passive subscription reflection positive control before empty-set unsubscription");
  auto const beforeUnsubscribe = subscriber.recorder().reflection();
  auto const passiveBeforeUnsubscribe =
      passiveSubscriber.recorder().reflection();
  require(
      beforeUnsubscribe.object == object &&
          beforeUnsubscribe.values.size() == 2U &&
          beforeUnsubscribe.values.count(subscriberFirstAttribute) == 1U &&
          beforeUnsubscribe.values.count(subscriberSecondAttribute) == 1U,
      "active subscription did not receive both attributes before empty-set unsubscription");
  require(
      passiveBeforeUnsubscribe.object == object &&
          passiveBeforeUnsubscribe.values.size() == 2U &&
          passiveBeforeUnsubscribe.values.count(passiveSubscriberFirstAttribute) == 1U &&
          passiveBeforeUnsubscribe.values.count(passiveSubscriberSecondAttribute) == 1U,
      "passive subscription did not receive both attributes under active interest");

  subscriber.recorder().clearReflection();
  passiveSubscriber.recorder().clearReflection();
  subscriber.rtiAmbassador().unsubscribeObjectClassAttributes(
      subscriberClass, emptyAttributes);
  passiveSubscriber.rtiAmbassador().unsubscribeObjectClassAttributes(
      passiveSubscriberClass,
      emptyAttributes);
  sendUpdate(0x32U);
  waitFor(
      subscriber,
      [&] { return subscriber.recorder().reflection().present; },
      options,
      "active subscription delivery after empty-set unsubscription");
  waitFor(
      passiveSubscriber,
      [&] { return passiveSubscriber.recorder().reflection().present; },
      options,
      "passive subscription delivery after empty-set unsubscription");
  auto const afterUnsubscribe = subscriber.recorder().reflection();
  auto const passiveAfterUnsubscribe =
      passiveSubscriber.recorder().reflection();
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
  require(
      passiveAfterUnsubscribe.object == object &&
          passiveAfterUnsubscribe.values.size() == 2U &&
          passiveAfterUnsubscribe.values.count(passiveSubscriberFirstAttribute) == 1U &&
          passiveAfterUnsubscribe.values.count(passiveSubscriberSecondAttribute) == 1U &&
          copyBytes(passiveAfterUnsubscribe.values.at(passiveSubscriberFirstAttribute)) ==
              std::vector<std::uint8_t>{0x45U, 0x4DU, 0x50U, 0x32U} &&
          copyBytes(passiveAfterUnsubscribe.values.at(passiveSubscriberSecondAttribute)) ==
              std::vector<std::uint8_t>{0x54U, 0x41U, 0x49U, 0x32U} &&
          passiveAfterUnsubscribe.tag == std::vector<std::uint8_t>{0x45U, 0x4DU, 0x32U},
      "empty attribute set removed a passive subscription from the object class");

  subscriber.rtiAmbassador().unsubscribeObjectClassAttributes(
      subscriberClass, subscribedAttributes);
  passiveSubscriber.rtiAmbassador().unsubscribeObjectClassAttributes(
      passiveSubscriberClass,
      passiveSubscribedAttributes);
  publisher.rtiAmbassador().unpublishObjectClassAttributes(
      publisherClass, publishedAttributes);
  subscriber.resign(rti::NO_ACTION);
  passiveSubscriber.resign(rti::NO_ACTION);
  publisher.resign(rti::NO_ACTION);
  publisher.rtiAmbassador().destroyFederationExecution(federation);
  subscriber.disconnect();
  passiveSubscriber.disconnect();
  publisher.disconnect();
}

}  // namespace hla_rti_cpp_tck::detail
