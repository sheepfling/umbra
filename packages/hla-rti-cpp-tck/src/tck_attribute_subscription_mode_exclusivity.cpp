#include "tck_main_scenarios.hpp"

namespace hla_rti_cpp_tck::detail {
namespace {

struct AttributeSubscriptionHandles {
  rti::ObjectClassHandle objectClass;
  rti::AttributeHandle first;
  rti::AttributeHandle second;
};

AttributeSubscriptionHandles resolveHandles(
    Session& session,
    Options const& options) {
  auto& ambassador = session.rtiAmbassador();
  AttributeSubscriptionHandles handles;
  handles.objectClass = ambassador.getObjectClassHandle(
      options.multiAttributeObjectClassName);
  handles.first = ambassador.getAttributeHandle(
      handles.objectClass,
      options.multiAttributeFirstName);
  handles.second = ambassador.getAttributeHandle(
      handles.objectClass,
      options.multiAttributeSecondName);
  require(handles.objectClass.isValid() && handles.first.isValid() &&
              handles.second.isValid() && handles.first != handles.second,
          "attribute subscription mode lookup returned invalid or duplicate handles");
  return handles;
}

bool hasValue(
    std::vector<ReflectionRecord> const& reflections,
    rti::ObjectInstanceHandle const& object,
    rti::AttributeHandle const& attribute,
    std::vector<std::uint8_t> const& expectedValue,
    std::vector<std::uint8_t> const& expectedTag,
    rti::FederateHandle const& expectedProducer) {
  return std::any_of(
      reflections.begin(),
      reflections.end(),
      [&](ReflectionRecord const& reflection) {
        auto const found = reflection.values.find(attribute);
        return reflection.present && reflection.object == object &&
            found != reflection.values.end() &&
            copyBytes(found->second) == expectedValue &&
            reflection.tag == expectedTag &&
            reflection.producer == expectedProducer;
      });
}

} // namespace

void scenarioAttributeSubscriptionModeExclusivity(
    Options const& options,
    rti::CallbackModel model) {
  require(!options.multiAttributeFom.empty(),
          "Attribute subscription mode testing requires a multi-attribute FOM");

  Session publisher(options, model, "subscription-mode-publisher");
  Session subscriber(options, model, "subscription-mode-subscriber");
  auto const federation = federationName(
      options,
      "attribute-subscription-mode-exclusivity");
  connectAndJoin(
      publisher,
      subscriber,
      options,
      federation,
      options.multiAttributeFom);

  auto const publisherHandles = resolveHandles(publisher, options);
  auto const subscriberHandles = resolveHandles(subscriber, options);
  require(publisherHandles.objectClass == subscriberHandles.objectClass &&
              publisherHandles.first == subscriberHandles.first &&
              publisherHandles.second == subscriberHandles.second,
          "attribute subscription mode members resolved inconsistent handles");

  rti::AttributeHandleSet const publisherAttributes{
      publisherHandles.first,
      publisherHandles.second};
  rti::AttributeHandleSet const firstAttribute{subscriberHandles.first};
  rti::AttributeHandleSet const secondAttribute{subscriberHandles.second};
  publisher.rtiAmbassador().publishObjectClassAttributes(
      publisherHandles.objectClass,
      publisherAttributes);

  // The first call omits the optional active and update-rate arguments, so the
  // public C++ defaults must still establish an active subscription. The
  // second call explicitly establishes a passive subscription on the same
  // object class; it must not replace or activate the first.
  subscriber.rtiAmbassador().subscribeObjectClassAttributes(
      subscriberHandles.objectClass,
      firstAttribute);
  subscriber.rtiAmbassador().subscribeObjectClassAttributes(
      subscriberHandles.objectClass,
      secondAttribute,
      false,
      L"");

  auto const object = publisher.rtiAmbassador().registerObjectInstance(
      publisherHandles.objectClass);
  require(object.isValid(),
          "attribute subscription mode registration returned an invalid object");
  waitFor(
      subscriber,
      [&] { return subscriber.recorder().hasDiscovery(object); },
      options,
      "active attribute subscription discovery");

  auto sendBoth = [&](std::vector<std::uint8_t> const& firstValue,
                      std::vector<std::uint8_t> const& secondValue,
                      std::vector<std::uint8_t> const& tag) {
    rti::AttributeHandleValueMap values;
    values.emplace(
        publisherHandles.first,
        rti::VariableLengthData(firstValue.data(), firstValue.size()));
    values.emplace(
        publisherHandles.second,
        rti::VariableLengthData(secondValue.data(), secondValue.size()));
    publisher.rtiAmbassador().updateAttributeValues(
        object,
        values,
        rti::VariableLengthData(tag.data(), tag.size()));
  };

  rti::FederateHandle const producer = publisher.federateHandle();
  std::vector<std::uint8_t> const firstInitial{0x71U, 0x01U};
  std::vector<std::uint8_t> const secondInitial{0x72U, 0x01U};
  std::vector<std::uint8_t> const initialTag{0x61U, 0x01U};
  sendBoth(firstInitial, secondInitial, initialTag);
  waitFor(
      subscriber,
      [&] {
        return hasValue(
            subscriber.recorder().reflections(),
            object,
            subscriberHandles.first,
            firstInitial,
            initialTag,
            producer);
      },
      options,
      "initial active attribute delivery");
  for (int pass = 0; pass != 8; ++pass) {
    publisher.pump();
    subscriber.pump();
  }
  auto const initiallyActiveReflections = subscriber.recorder().reflections();
  require(
      initiallyActiveReflections.size() == 1U &&
          initiallyActiveReflections.front().values.size() == 1U &&
          initiallyActiveReflections.front().values.count(
              subscriberHandles.first) == 1U &&
          initiallyActiveReflections.front().values.count(
              subscriberHandles.second) == 0U &&
          hasValue(
              initiallyActiveReflections,
              object,
              subscriberHandles.first,
              firstInitial,
              initialTag,
              producer),
      "passive attribute was reflected or the active attribute was lost");

  subscriber.recorder().clearReflection();
  // Re-subscribing each attribute with the opposite mode must replace that
  // attribute's mode without leaving the old active/passive mode in effect.
  subscriber.rtiAmbassador().subscribeObjectClassAttributes(
      subscriberHandles.objectClass,
      firstAttribute,
      false,
      L"");
  subscriber.rtiAmbassador().subscribeObjectClassAttributes(
      subscriberHandles.objectClass,
      secondAttribute,
      true,
      L"");

  std::vector<std::uint8_t> const firstAfterChange{0x71U, 0x02U};
  std::vector<std::uint8_t> const secondAfterChange{0x72U, 0x02U};
  std::vector<std::uint8_t> const changedTag{0x61U, 0x02U};
  sendBoth(firstAfterChange, secondAfterChange, changedTag);
  waitFor(
      subscriber,
      [&] {
        return hasValue(
            subscriber.recorder().reflections(),
            object,
            subscriberHandles.second,
            secondAfterChange,
            changedTag,
            producer);
      },
      options,
      "active delivery after switching per-attribute subscription modes");
  for (int pass = 0; pass != 8; ++pass) {
    publisher.pump();
    subscriber.pump();
  }

  auto const changedReflections = subscriber.recorder().reflections();
  require(
      changedReflections.size() == 1U &&
          changedReflections.front().values.size() == 1U &&
          changedReflections.front().values.count(
              subscriberHandles.first) == 0U &&
          changedReflections.front().values.count(
              subscriberHandles.second) == 1U &&
          hasValue(
              changedReflections,
              object,
              subscriberHandles.second,
              secondAfterChange,
              changedTag,
              producer),
      "active/passive mode change left an attribute active and passive together");

  subscriber.rtiAmbassador().unsubscribeObjectClassAttributes(
      subscriberHandles.objectClass,
      rti::AttributeHandleSet{
          subscriberHandles.first,
          subscriberHandles.second});
  publisher.rtiAmbassador().unpublishObjectClassAttributes(
      publisherHandles.objectClass,
      publisherAttributes);
  publisher.resign(rti::DELETE_OBJECTS);
  subscriber.resign(rti::NO_ACTION);
  publisher.rtiAmbassador().destroyFederationExecution(federation);
  subscriber.disconnect();
  publisher.disconnect();
}

void scenarioAttributeSubscriptionModeExclusivityContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioAttributeSubscriptionModeExclusivity(options, model);
}

} // namespace hla_rti_cpp_tck::detail
