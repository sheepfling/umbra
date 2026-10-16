#include "tck_main_scenarios.hpp"

namespace hla_rti_cpp_tck::detail {

void scenarioObjectAttributeSubscriptionLifecycle(
    Options const& options,
    rti::CallbackModel model) {
  Session publisher(options, model, "publisher");
  Session subscriber(options, model, "subscriber");
  auto const federation = federationName(
      options,
      "object-attribute-subscription-lifecycle");
  connectAndJoin(publisher, subscriber, options, federation, options.fom);

  auto const publisherClass = publisher.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const subscriberClass = subscriber.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const publisherAttribute = publisher.rtiAmbassador().getAttributeHandle(
      publisherClass,
      options.attributeName);
  auto const subscriberAttribute = subscriber.rtiAmbassador().getAttributeHandle(
      subscriberClass,
      options.attributeName);
  auto const publisherSecondaryAttribute =
      publisher.rtiAmbassador().getAttributeHandle(
          publisherClass,
          options.secondaryAttributeName);
  auto const subscriberSecondaryAttribute =
      subscriber.rtiAmbassador().getAttributeHandle(
          subscriberClass,
          options.secondaryAttributeName);
  require(
      publisherClass.isValid() && subscriberClass.isValid() &&
          publisherAttribute.isValid() && subscriberAttribute.isValid() &&
          publisherSecondaryAttribute.isValid() &&
          subscriberSecondaryAttribute.isValid() &&
          publisherSecondaryAttribute != publisherAttribute &&
          subscriberSecondaryAttribute != subscriberAttribute,
      "object attribute subscription lifecycle lookup returned an invalid handle");

  rti::AttributeHandleSet const publisherAttributes{
      publisherAttribute, publisherSecondaryAttribute};
  rti::AttributeHandleSet const subscriberAttributes{subscriberAttribute};
  publisher.rtiAmbassador().publishObjectClassAttributes(
      publisherClass,
      publisherAttributes);
  auto const preexistingObject = publisher.rtiAmbassador().registerObjectInstance(
      publisherClass);
  require(
      preexistingObject.isValid(),
      "object attribute subscription lifecycle pre-subscription registration returned an invalid handle");

  std::vector<std::uint8_t> const preSubscriptionValue{0x49U, 0x4EU, 0x49U};
  std::vector<std::uint8_t> const preSubscriptionFilteredValue{
      0x46U, 0x49U, 0x4CU};
  std::vector<std::uint8_t> const preSubscriptionTag{0x49U, 0x30U};
  rti::AttributeHandleValueMap const preSubscriptionValues{
      {publisherAttribute,
       rti::VariableLengthData(
           preSubscriptionValue.data(), preSubscriptionValue.size())},
      {publisherSecondaryAttribute,
       rti::VariableLengthData(
           preSubscriptionFilteredValue.data(),
           preSubscriptionFilteredValue.size())}};
  rti::VariableLengthData preSubscriptionUserTag(
      preSubscriptionTag.data(), preSubscriptionTag.size());
  publisher.rtiAmbassador().updateAttributeValues(
      preexistingObject,
      preSubscriptionValues,
      preSubscriptionUserTag);

  auto const momFederateClass = subscriber.rtiAmbassador().getObjectClassHandle(
      L"HLAobjectRoot.HLAmanager.HLAfederate");
  auto const momFederateHandleAttribute =
      subscriber.rtiAmbassador().getAttributeHandle(
          momFederateClass,
          L"HLAfederateHandle");
  require(
      momFederateClass.isValid() && momFederateHandleAttribute.isValid(),
      "standard MOM fixture did not provide a valid federate handle attribute");
  rti::AttributeHandleSet const outOfClassAttributes{
      momFederateHandleAttribute};
  requireException(
      [&] {
        subscriber.rtiAmbassador().subscribeObjectClassAttributes(
            subscriberClass,
            outOfClassAttributes,
            true,
            L"");
      },
      L"AttributeNotDefined",
      "subscribing to an attribute handle from an unrelated standard MOM class");
  for (int pass = 0; pass != 8; ++pass) {
    subscriber.pump();
  }
  require(
      !subscriber.recorder().hasDiscovery(preexistingObject),
      "rejected out-of-class subscription changed object discovery state");

  rti::AttributeHandleSet const emptyAttributes;
  subscriber.rtiAmbassador().subscribeObjectClassAttributes(
      subscriberClass,
      emptyAttributes,
      true,
      L"");
  for (int pass = 0; pass != 8; ++pass) {
    subscriber.pump();
  }
  require(
      !subscriber.recorder().hasDiscovery(preexistingObject),
      "empty active object-attribute subscription discovered a pre-existing object");

  // Passive declarations do not discover instances registered before or
  // after the subscription is established.
  subscriber.rtiAmbassador().subscribeObjectClassAttributes(
      subscriberClass,
      subscriberAttributes,
      false,
      L"");
  for (int pass = 0; pass != 8; ++pass) {
    subscriber.pump();
  }
  require(
      !subscriber.recorder().hasDiscovery(preexistingObject),
      "passive object-attribute subscription discovered a pre-existing object");
  auto const object = publisher.rtiAmbassador().registerObjectInstance(
      publisherClass);
  require(
      object.isValid(),
      "object attribute subscription lifecycle registration returned an invalid handle");
  for (int pass = 0; pass != 8; ++pass) {
    subscriber.pump();
  }
  require(
      !subscriber.recorder().hasDiscovery(preexistingObject) &&
          !subscriber.recorder().hasDiscovery(object),
      "passive object-attribute subscription unexpectedly discovered an object");

  // Activating the same declaration makes the existing object discoverable;
  // activation does not replay an attribute update sent while passive.
  subscriber.rtiAmbassador().subscribeObjectClassAttributes(
      subscriberClass,
      subscriberAttributes,
      true,
      L"");
  waitFor(
      subscriber,
      [&] { return subscriber.recorder().hasDiscovery(preexistingObject); },
      options,
      "active object-attribute subscription discovery of a pre-existing object");
  waitFor(
      subscriber,
      [&] { return subscriber.recorder().hasDiscovery(object); },
      options,
      "active object-attribute subscription discovery of a later registration");
  waitFor(
      subscriber,
      [&] {
        auto const reflections = subscriber.recorder().reflections();
        for (auto const& reflection : reflections) {
          if (reflection.object == preexistingObject) {
            return true;
          }
        }
        return false;
      },
      options,
      "active object-attribute subscription initial reflection of a pre-existing object");
  std::optional<ReflectionRecord> initialReflection;
  for (auto const& reflection : subscriber.recorder().reflections()) {
    if (reflection.object != preexistingObject) {
      continue;
    }
    initialReflection = reflection;
    break;
  }
  require(
      initialReflection.has_value(),
      "active subscription did not produce initial reflection for the pre-existing object");
  require(
      initialReflection->values.size() == 1U &&
          initialReflection->values.count(subscriberAttribute) == 1U &&
          initialReflection->values.count(subscriberSecondaryAttribute) == 0U,
      "active subscription initial reflection was not filtered to the subscribed attribute set");
  require(
      copyBytes(initialReflection->values.at(subscriberAttribute)) ==
          preSubscriptionValue,
      "active subscription initial reflection did not retain the pre-subscription value");
  auto const objectName = publisher.rtiAmbassador().getObjectInstanceName(object);
  auto const preexistingObjectName =
      publisher.rtiAmbassador().getObjectInstanceName(preexistingObject);
  require(
      !objectName.empty() &&
          subscriber.rtiAmbassador().getObjectInstanceHandle(objectName) == object &&
          subscriber.rtiAmbassador().getKnownObjectClassHandle(object) == subscriberClass,
      "active object-attribute subscription did not establish stable identity lookups");
  require(
      !preexistingObjectName.empty() &&
          subscriber.rtiAmbassador().getObjectInstanceHandle(preexistingObjectName) ==
              preexistingObject &&
          subscriber.rtiAmbassador().getKnownObjectClassHandle(preexistingObject) ==
              subscriberClass,
      "active object-attribute subscription did not discover and identify the pre-existing object");

  auto sendUpdate = [&](std::vector<std::uint8_t> const& value,
                        std::vector<std::uint8_t> const& tagBytes) {
    rti::AttributeHandleValueMap values;
    values.emplace(
        publisherAttribute,
        rti::VariableLengthData(value.data(), value.size()));
    std::vector<std::uint8_t> const filteredValue{0x46U, 0x49U, 0x4CU};
    values.emplace(
        publisherSecondaryAttribute,
        rti::VariableLengthData(filteredValue.data(), filteredValue.size()));
    rti::VariableLengthData tag(tagBytes.data(), tagBytes.size());
    publisher.rtiAmbassador().updateAttributeValues(object, values, tag);
  };
  auto requireReflection = [&](std::vector<std::uint8_t> const& value,
                               std::vector<std::uint8_t> const& tagBytes,
                               std::string const& description) {
    auto const reflection = subscriber.recorder().reflection();
    require(
        reflection.object == object,
        description + " returned the wrong object handle");
    require(
        reflection.values.size() == 1U &&
            reflection.values.count(subscriberAttribute) == 1U,
        description + " returned the wrong attribute set");
    require(
        copyBytes(reflection.values.at(subscriberAttribute)) == value,
        description + " returned the wrong attribute value");
    require(
        reflection.tag == tagBytes,
        description + " returned the wrong user tag");
    require(
        reflection.producer == publisher.federateHandle(),
        description + " returned the wrong producing federate");
    require(
        reflection.transportation.isValid(),
        description + " returned an invalid transportation handle");
    require(
        !reflection.regions.has_value(),
        description + " unexpectedly returned source regions");
  };

  std::vector<std::uint8_t> const activeValue{0x41U, 0x43U, 0x54U};
  std::vector<std::uint8_t> const activeTag{0x41U, 0x31U};
  sendUpdate(activeValue, activeTag);
  waitFor(
      subscriber,
      [&] { return subscriber.recorder().reflection().present; },
      options,
      "active object-attribute subscription reflection");
  requireReflection(
      activeValue,
      activeTag,
      "active object-attribute subscription reflection");

  // Downgrading the declaration to passive suppresses subsequent ordinary
  // Update/Reflect callbacks while retaining the known object identity.
  subscriber.recorder().clearReflection();
  subscriber.rtiAmbassador().subscribeObjectClassAttributes(
      subscriberClass,
      subscriberAttributes,
      false,
      L"");
  std::vector<std::uint8_t> const downgradedValue{0x44U, 0x4FU, 0x57U};
  std::vector<std::uint8_t> const downgradedTag{0x44U, 0x32U};
  sendUpdate(downgradedValue, downgradedTag);
  for (int pass = 0; pass != 8; ++pass) {
    subscriber.pump();
  }
  require(
      !subscriber.recorder().reflection().present,
      "passive object-attribute downgrade did not suppress reflection");

  // Re-activation restores ordinary reflection without requiring a new
  // registration or changing the object's stable identity.
  subscriber.rtiAmbassador().subscribeObjectClassAttributes(
      subscriberClass,
      subscriberAttributes,
      true,
      L"");
  std::vector<std::uint8_t> const reactivatedValue{0x52U, 0x45U, 0x41U};
  std::vector<std::uint8_t> const reactivatedTag{0x52U, 0x33U};
  sendUpdate(reactivatedValue, reactivatedTag);
  waitFor(
      subscriber,
      [&] { return subscriber.recorder().reflection().present; },
      options,
      "reactivated object-attribute subscription reflection");
  requireReflection(
      reactivatedValue,
      reactivatedTag,
      "reactivated object-attribute subscription reflection");

  // Unsubscription removes the declaration and suppresses later updates.
  subscriber.recorder().clearReflection();
  subscriber.rtiAmbassador().unsubscribeObjectClassAttributes(
      subscriberClass,
      subscriberAttributes);
  std::vector<std::uint8_t> const withdrawnValue{0x57U, 0x49U, 0x54U};
  std::vector<std::uint8_t> const withdrawnTag{0x57U, 0x34U};
  sendUpdate(withdrawnValue, withdrawnTag);
  for (int pass = 0; pass != 8; ++pass) {
    subscriber.pump();
  }
  require(
      !subscriber.recorder().reflection().present,
      "unsubscribed object-attribute declaration unexpectedly reflected an update");

  subscriber.resign(rti::NO_ACTION);
  publisher.resign(rti::DELETE_OBJECTS);
  publisher.rtiAmbassador().destroyFederationExecution(federation);
  subscriber.disconnect();
  publisher.disconnect();
}

} // namespace hla_rti_cpp_tck::detail
