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
  Session passiveSubscriber(options, model, "passive-subscriber");
  auto const federation =
      federationName(options, "whole-class-attribute-unsubscription");
  auto const independentClassFom = options.scenarioFoms.find(
      "object-attribute-subscription-independent-class");
  require(
      independentClassFom != options.scenarioFoms.end(),
      "whole-class unsubscription isolation requires an adapter-supplied sibling-class FOM module");
  publisher.connect();
  subscriber.connect();
  passiveSubscriber.connect();
  std::vector<std::wstring> const fomModules{
      options.fom.wstring(), independentClassFom->second.wstring()};
  publisher.rtiAmbassador().createFederationExecution(
      federation,
      fomModules,
      options.logicalTimeImplementationName);
  publisher.join(options.ownerFederateName, options.federateType, federation);
  subscriber.join(options.memberFederateName, options.federateType, federation);
  passiveSubscriber.join(
      options.memberFederateName + L"-passive-whole-class-unsubscribe",
      options.federateType,
      federation);

  auto const publisherClass =
      publisher.rtiAmbassador().getObjectClassHandle(options.objectClassName);
  auto const subscriberClass =
      subscriber.rtiAmbassador().getObjectClassHandle(options.objectClassName);
  auto const passiveSubscriberClass =
      passiveSubscriber.rtiAmbassador().getObjectClassHandle(
          options.objectClassName);
  auto const publisherFirstAttribute = publisher.rtiAmbassador().getAttributeHandle(
      publisherClass, options.attributeName);
  auto const publisherSecondAttribute = publisher.rtiAmbassador().getAttributeHandle(
      publisherClass, options.secondaryAttributeName);
  auto const subscriberFirstAttribute = subscriber.rtiAmbassador().getAttributeHandle(
      subscriberClass, options.attributeName);
  auto const subscriberSecondAttribute = subscriber.rtiAmbassador().getAttributeHandle(
      subscriberClass, options.secondaryAttributeName);
  auto const publisherIndependentClass =
      publisher.rtiAmbassador().getObjectClassHandle(
          L"HLAobjectRoot.TckIndependentObject");
  auto const subscriberIndependentClass =
      subscriber.rtiAmbassador().getObjectClassHandle(
          L"HLAobjectRoot.TckIndependentObject");
  auto const passiveIndependentClass =
      passiveSubscriber.rtiAmbassador().getObjectClassHandle(
          L"HLAobjectRoot.TckIndependentObject");
  auto const publisherIndependentAttribute =
      publisher.rtiAmbassador().getAttributeHandle(
          publisherIndependentClass,
          L"IndependentValue");
  auto const subscriberIndependentAttribute =
      subscriber.rtiAmbassador().getAttributeHandle(
          subscriberIndependentClass,
          L"IndependentValue");
  auto const passiveFirstAttribute =
      passiveSubscriber.rtiAmbassador().getAttributeHandle(
          passiveSubscriberClass,
          options.attributeName);
  auto const passiveSecondAttribute =
      passiveSubscriber.rtiAmbassador().getAttributeHandle(
          passiveSubscriberClass,
          options.secondaryAttributeName);
  auto const passiveIndependentAttribute =
      passiveSubscriber.rtiAmbassador().getAttributeHandle(
          passiveIndependentClass,
          L"IndependentValue");
  require(
      publisherClass.isValid() && subscriberClass.isValid() &&
          passiveSubscriberClass.isValid() &&
          publisherFirstAttribute.isValid() && publisherSecondAttribute.isValid() &&
          subscriberFirstAttribute.isValid() && subscriberSecondAttribute.isValid() &&
          passiveFirstAttribute.isValid() && passiveSecondAttribute.isValid() &&
          publisherIndependentClass.isValid() &&
          subscriberIndependentClass.isValid() &&
          passiveIndependentClass.isValid() &&
          publisherIndependentClass != publisherClass &&
          subscriberIndependentClass != subscriberClass &&
          passiveIndependentClass != passiveSubscriberClass &&
          publisherIndependentAttribute.isValid() &&
          subscriberIndependentAttribute.isValid() &&
          passiveIndependentAttribute.isValid(),
      "whole-class unsubscription lookup returned an invalid handle");

  rti::AttributeHandleSet const publishedAttributes{
      publisherFirstAttribute, publisherSecondAttribute};
  rti::AttributeHandleSet const subscribedAttributes{
      subscriberFirstAttribute, subscriberSecondAttribute};
  rti::AttributeHandleSet const passiveAttributes{
      passiveFirstAttribute, passiveSecondAttribute};
  rti::AttributeHandleSet const publishedIndependentAttributes{
      publisherIndependentAttribute};
  rti::AttributeHandleSet const subscribedIndependentAttributes{
      subscriberIndependentAttribute};
  rti::AttributeHandleSet const passiveIndependentAttributes{
      passiveIndependentAttribute};
  publisher.rtiAmbassador().publishObjectClassAttributes(
      publisherClass, publishedAttributes);
  publisher.rtiAmbassador().publishObjectClassAttributes(
      publisherIndependentClass,
      publishedIndependentAttributes);
  subscriber.rtiAmbassador().subscribeObjectClassAttributes(
      subscriberClass, subscribedAttributes, true, L"");
  passiveSubscriber.rtiAmbassador().subscribeObjectClassAttributes(
      passiveSubscriberClass,
      passiveAttributes,
      false,
      L"");
  subscriber.rtiAmbassador().subscribeObjectClassAttributes(
      subscriberIndependentClass,
      subscribedIndependentAttributes,
      true,
      L"");
  passiveSubscriber.rtiAmbassador().subscribeObjectClassAttributes(
      passiveIndependentClass,
      passiveIndependentAttributes,
      true,
      L"");
  auto const object = publisher.rtiAmbassador().registerObjectInstance(
      publisherClass);
  auto const independentObject =
      publisher.rtiAmbassador().registerObjectInstance(
          publisherIndependentClass);
  require(
      object.isValid() && independentObject.isValid(),
      "whole-class unsubscription registration returned an invalid handle");
  waitFor(
      subscriber,
      [&] { return subscriber.recorder().hasDiscovery(object); },
      options,
      "whole-class unsubscription object discovery");
  waitFor(
      passiveSubscriber,
      [&] { return passiveSubscriber.recorder().hasDiscovery(object); },
      options,
      "passive object discovery while separate active interest exists");
  waitFor(
      subscriber,
      [&] { return subscriber.recorder().hasDiscovery(independentObject); },
      options,
      "independent sibling-class object discovery before whole-class unsubscribe");
  waitFor(
      passiveSubscriber,
      [&] { return passiveSubscriber.recorder().hasDiscovery(independentObject); },
      options,
      "passive subscriber's sibling-class object discovery positive control");

  auto sendUpdateTo = [&](rti::ObjectInstanceHandle const& target,
                          std::uint8_t sequence) {
    std::vector<std::uint8_t> const firstValue{0x41U, 0x4CU, 0x4CU, sequence};
    std::vector<std::uint8_t> const secondValue{0x41U, 0x4CU, 0x4CU, sequence, 0x32U};
    std::vector<std::uint8_t> const tagBytes{0x41U, 0x4CU, sequence};
    rti::AttributeHandleValueMap const values{
        {publisherFirstAttribute,
         rti::VariableLengthData(firstValue.data(), firstValue.size())},
        {publisherSecondAttribute,
         rti::VariableLengthData(secondValue.data(), secondValue.size())}};
    rti::VariableLengthData const tag(tagBytes.data(), tagBytes.size());
    publisher.rtiAmbassador().updateAttributeValues(target, values, tag);
  };
  auto sendUpdate = [&](std::uint8_t sequence) {
    sendUpdateTo(object, sequence);
  };
  auto sendIndependentUpdate = [&](std::uint8_t sequence) {
    std::vector<std::uint8_t> const value{0x49U, 0x4EU, sequence};
    std::vector<std::uint8_t> const tagBytes{0x49U, sequence};
    rti::AttributeHandleValueMap const values{{
        publisherIndependentAttribute,
        rti::VariableLengthData(value.data(), value.size())}};
    rti::VariableLengthData const tag(tagBytes.data(), tagBytes.size());
    publisher.rtiAmbassador().updateAttributeValues(
        independentObject,
        values,
        tag);
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
  waitFor(
      passiveSubscriber,
      [&] { return passiveSubscriber.recorder().reflection().present; },
      options,
      "passive base-class delivery while separate active interest exists");
  auto const passiveBeforeUnsubscribe =
      passiveSubscriber.recorder().reflection();
  require(
      passiveBeforeUnsubscribe.object == object &&
          passiveBeforeUnsubscribe.values.size() == 2U &&
          passiveBeforeUnsubscribe.values.count(passiveFirstAttribute) == 1U &&
          passiveBeforeUnsubscribe.values.count(passiveSecondAttribute) == 1U &&
          copyBytes(passiveBeforeUnsubscribe.values.at(passiveFirstAttribute)) ==
              std::vector<std::uint8_t>{0x41U, 0x4CU, 0x4CU, 0x31U} &&
          copyBytes(passiveBeforeUnsubscribe.values.at(passiveSecondAttribute)) ==
              std::vector<std::uint8_t>{0x41U, 0x4CU, 0x4CU, 0x31U, 0x32U},
      "passive base-class subscription did not receive both attributes under active interest");

  subscriber.recorder().clearReflection();
  passiveSubscriber.recorder().clearReflection();
  sendIndependentUpdate(0x31U);
  waitFor(
      subscriber,
      [&] { return subscriber.recorder().reflection().present; },
      options,
      "independent sibling-class subscription positive delivery control");
  waitFor(
      passiveSubscriber,
      [&] { return passiveSubscriber.recorder().reflection().present; },
      options,
      "passive subscriber's sibling-class subscription positive control");
  auto const independentBeforeUnsubscribe = subscriber.recorder().reflection();
  require(
      independentBeforeUnsubscribe.object == independentObject &&
          independentBeforeUnsubscribe.values.size() == 1U &&
          independentBeforeUnsubscribe.values.count(
              subscriberIndependentAttribute) == 1U &&
          copyBytes(independentBeforeUnsubscribe.values.at(
              subscriberIndependentAttribute)) ==
              std::vector<std::uint8_t>{0x49U, 0x4EU, 0x31U},
      "independent sibling-class subscription did not reflect before whole-class unsubscribe");
  auto const passiveIndependentBeforeUnsubscribe =
      passiveSubscriber.recorder().reflection();
  require(
      passiveIndependentBeforeUnsubscribe.object == independentObject &&
          passiveIndependentBeforeUnsubscribe.values.size() == 1U &&
          passiveIndependentBeforeUnsubscribe.values.count(
              passiveIndependentAttribute) == 1U &&
          copyBytes(passiveIndependentBeforeUnsubscribe.values.at(
              passiveIndependentAttribute)) ==
              std::vector<std::uint8_t>{0x49U, 0x4EU, 0x31U},
      "passive subscriber's sibling-class subscription did not reflect before unsubscribe");

  subscriber.recorder().clearReflection();
  passiveSubscriber.recorder().clearReflection();
  subscriber.rtiAmbassador().unsubscribeObjectClass(subscriberClass);

  auto const laterObject = publisher.rtiAmbassador().registerObjectInstance(
      publisherClass);
  require(
      laterObject.isValid(),
      "whole-class unsubscription follow-up registration returned an invalid handle");
  for (int pass = 0; pass != 8; ++pass) {
    subscriber.pump();
    passiveSubscriber.pump();
  }
  require(
      !subscriber.recorder().hasDiscovery(laterObject),
      "whole-class unsubscription still notified the federate of object discovery");

  sendUpdate(0x32U);
  for (int pass = 0; pass != 8; ++pass) {
    subscriber.pump();
    passiveSubscriber.pump();
  }
  require(
      !subscriber.recorder().reflection().present &&
          !passiveSubscriber.recorder().reflection().present,
      "whole-class unsubscription left one or more class attributes subscribed");

  sendIndependentUpdate(0x32U);
  waitFor(
      subscriber,
      [&] { return subscriber.recorder().reflection().present; },
      options,
      "independent sibling-class subscription after whole-class unsubscribe");
  waitFor(
      passiveSubscriber,
      [&] { return passiveSubscriber.recorder().reflection().present; },
      options,
      "passive subscriber's sibling-class subscription after base unsubscribe");
  auto const independentAfterUnsubscribe = subscriber.recorder().reflection();
  require(
      independentAfterUnsubscribe.object == independentObject &&
          independentAfterUnsubscribe.values.size() == 1U &&
          independentAfterUnsubscribe.values.count(
              subscriberIndependentAttribute) == 1U &&
          copyBytes(independentAfterUnsubscribe.values.at(
              subscriberIndependentAttribute)) ==
              std::vector<std::uint8_t>{0x49U, 0x4EU, 0x32U},
      "whole-class unsubscribe disturbed the unrelated sibling-class subscription");
  auto const passiveIndependentAfterUnsubscribe =
      passiveSubscriber.recorder().reflection();
  require(
      passiveIndependentAfterUnsubscribe.object == independentObject &&
          passiveIndependentAfterUnsubscribe.values.size() == 1U &&
          passiveIndependentAfterUnsubscribe.values.count(
              passiveIndependentAttribute) == 1U &&
          copyBytes(passiveIndependentAfterUnsubscribe.values.at(
              passiveIndependentAttribute)) ==
              std::vector<std::uint8_t>{0x49U, 0x4EU, 0x32U},
      "base-class whole unsubscribe disturbed the passive federate's sibling subscription");

  subscriber.recorder().clearReflection();
  passiveSubscriber.recorder().clearReflection();
  subscriber.rtiAmbassador().subscribeObjectClassAttributes(
      subscriberClass,
      subscribedAttributes,
      true,
      L"");
  auto const passivePositiveObject =
      publisher.rtiAmbassador().registerObjectInstance(publisherClass);
  require(
      passivePositiveObject.isValid(),
      "passive whole-class unsubscription positive-control registration returned an invalid handle");
  waitFor(
      subscriber,
      [&] { return subscriber.recorder().hasDiscovery(passivePositiveObject); },
      options,
      "active delivery-interest restoration before passive unsubscribe");
  waitFor(
      passiveSubscriber,
      [&] { return passiveSubscriber.recorder().hasDiscovery(passivePositiveObject); },
      options,
      "passive subscription positive discovery control before whole-class unsubscribe");
  subscriber.recorder().clearReflection();
  passiveSubscriber.recorder().clearReflection();
  sendUpdateTo(passivePositiveObject, 0x33U);
  waitFor(
      subscriber,
      [&] { return subscriber.recorder().reflection().present; },
      options,
      "active base-class delivery control before passive whole-class unsubscribe");
  waitFor(
      passiveSubscriber,
      [&] { return passiveSubscriber.recorder().reflection().present; },
      options,
      "passive base-class positive control before whole-class unsubscribe");
  auto const passiveBeforeWholeClassUnsubscribe =
      passiveSubscriber.recorder().reflection();
  require(
      passiveBeforeWholeClassUnsubscribe.object == passivePositiveObject &&
          passiveBeforeWholeClassUnsubscribe.values.size() == 2U &&
          passiveBeforeWholeClassUnsubscribe.values.count(passiveFirstAttribute) == 1U &&
          passiveBeforeWholeClassUnsubscribe.values.count(passiveSecondAttribute) == 1U &&
          copyBytes(passiveBeforeWholeClassUnsubscribe.values.at(passiveFirstAttribute)) ==
              std::vector<std::uint8_t>{0x41U, 0x4CU, 0x4CU, 0x33U},
      "passive base-class subscription was not active before its whole-class unsubscribe");

  passiveSubscriber.recorder().clearReflection();
  passiveSubscriber.rtiAmbassador().unsubscribeObjectClass(
      passiveSubscriberClass);
  auto const passiveLaterObject = publisher.rtiAmbassador().registerObjectInstance(
      publisherClass);
  require(
      passiveLaterObject.isValid(),
      "passive whole-class unsubscription follow-up registration returned an invalid handle");
  waitFor(
      subscriber,
      [&] { return subscriber.recorder().hasDiscovery(passiveLaterObject); },
      options,
      "active discovery control after passive whole-class unsubscribe");
  for (int pass = 0; pass != 8; ++pass) {
    passiveSubscriber.pump();
  }
  require(
      !passiveSubscriber.recorder().hasDiscovery(passiveLaterObject),
      "passive whole-class unsubscription still notified the federate of object discovery");

  subscriber.recorder().clearReflection();
  sendUpdateTo(passivePositiveObject, 0x34U);
  waitFor(
      subscriber,
      [&] { return subscriber.recorder().reflection().present; },
      options,
      "active reflection control after passive whole-class unsubscribe");
  for (int pass = 0; pass != 8; ++pass) {
    passiveSubscriber.pump();
  }
  require(
      !passiveSubscriber.recorder().reflection().present,
      "passive whole-class unsubscription left base-class attributes subscribed");

  passiveSubscriber.recorder().clearReflection();
  sendIndependentUpdate(0x33U);
  waitFor(
      passiveSubscriber,
      [&] { return passiveSubscriber.recorder().reflection().present; },
      options,
      "sibling-class reflection control after passive whole-class unsubscribe");
  auto const passiveSiblingAfterUnsubscribe =
      passiveSubscriber.recorder().reflection();
  require(
      passiveSiblingAfterUnsubscribe.object == independentObject &&
          passiveSiblingAfterUnsubscribe.values.size() == 1U &&
          passiveSiblingAfterUnsubscribe.values.count(
              passiveIndependentAttribute) == 1U &&
          copyBytes(passiveSiblingAfterUnsubscribe.values.at(
              passiveIndependentAttribute)) ==
              std::vector<std::uint8_t>{0x49U, 0x4EU, 0x33U},
      "passive whole-class unsubscribe disturbed the sibling-class subscription");

  publisher.rtiAmbassador().unpublishObjectClassAttributes(
      publisherClass, publishedAttributes);
  publisher.rtiAmbassador().unpublishObjectClassAttributes(
      publisherIndependentClass,
      publishedIndependentAttributes);
  subscriber.rtiAmbassador().unsubscribeObjectClass(subscriberClass);
  subscriber.rtiAmbassador().unsubscribeObjectClass(
      subscriberIndependentClass);
  passiveSubscriber.rtiAmbassador().unsubscribeObjectClass(
      passiveIndependentClass);
  passiveSubscriber.resign(rti::NO_ACTION);
  subscriber.resign(rti::NO_ACTION);
  publisher.resign(rti::NO_ACTION);
  publisher.rtiAmbassador().destroyFederationExecution(federation);
  subscriber.disconnect();
  passiveSubscriber.disconnect();
  publisher.disconnect();
}

}  // namespace hla_rti_cpp_tck::detail
