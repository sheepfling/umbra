#include "tck_attribute_subscription_mode_transitions.hpp"
#include "tck_main_scenarios.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace hla_rti_cpp_tck::detail {
namespace {

void updateBothAttributes(
    Session& publisher,
    rti::ObjectInstanceHandle const& object,
    rti::AttributeHandle const& firstAttribute,
    rti::AttributeHandle const& secondAttribute,
    std::vector<std::uint8_t> const& firstValue,
    std::vector<std::uint8_t> const& secondValue,
    std::vector<std::uint8_t> const& tag) {
  rti::AttributeHandleValueMap const values{
      {firstAttribute,
       rti::VariableLengthData(firstValue.data(), firstValue.size())},
      {secondAttribute,
       rti::VariableLengthData(secondValue.data(), secondValue.size())}};
  rti::VariableLengthData const userTag(tag.data(), tag.size());
  publisher.rtiAmbassador().updateAttributeValues(object, values, userTag);
}

void requireOneAttributeReflection(
    Recorder const& recorder,
    rti::ObjectInstanceHandle const& object,
    rti::AttributeHandle const& expectedAttribute,
    rti::AttributeHandle const& otherAttribute,
    std::vector<std::uint8_t> const& expectedValue,
    std::vector<std::uint8_t> const& expectedTag,
    std::string const& phase) {
  auto const reflections = recorder.reflections();
  require(
      reflections.size() == 1U && reflections.front().object == object &&
          reflections.front().values.size() == 1U &&
          reflections.front().values.count(expectedAttribute) == 1U &&
          reflections.front().values.count(otherAttribute) == 0U &&
          copyBytes(reflections.front().values.at(expectedAttribute)) == expectedValue &&
          reflections.front().tag == expectedTag,
      phase + " did not reflect only the expected active attribute");
}

}  // namespace

void scenarioAttributeSubscriptionPerAttributeModeTransitions(
    Options const& options,
    rti::CallbackModel model) {
  Session publisher(options, model, "subscription-mode-publisher");
  Session subscriber(options, model, "subscription-mode-subscriber");
  auto const federation =
      federationName(options, "attribute-subscription-per-attribute-mode-transitions");
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
      "per-attribute subscription mode lookup returned an invalid handle");

  rti::AttributeHandleSet const publishedAttributes{
      publisherFirstAttribute, publisherSecondAttribute};
  publisher.rtiAmbassador().publishObjectClassAttributes(
      publisherClass, publishedAttributes);

  // Adding the second attribute as passive must preserve the first attribute's
  // active mode while retaining both declarations.
  subscriber.rtiAmbassador().subscribeObjectClassAttributes(
      subscriberClass, rti::AttributeHandleSet{subscriberFirstAttribute}, true, L"");
  subscriber.rtiAmbassador().subscribeObjectClassAttributes(
      subscriberClass, rti::AttributeHandleSet{subscriberSecondAttribute}, false, L"");
  rti::AttributeHandleSet const emptyAttributes;
  subscriber.rtiAmbassador().subscribeObjectClassAttributes(
      subscriberClass, emptyAttributes, true, L"");
  subscriber.rtiAmbassador().subscribeObjectClassAttributes(
      subscriberClass, emptyAttributes, false, L"");

  auto const object = publisher.rtiAmbassador().registerObjectInstance(publisherClass);
  require(
      object.isValid(),
      "per-attribute subscription mode registration returned an invalid handle");
  waitFor(
      subscriber,
      [&] { return subscriber.recorder().hasDiscovery(object); },
      options,
      "per-attribute subscription mode object discovery");

  std::vector<std::uint8_t> firstValue{0x4dU, 0x31U};
  std::vector<std::uint8_t> secondValue{0x4dU, 0x32U};
  std::vector<std::uint8_t> tag{0x4dU, 0x41U};
  updateBothAttributes(
      publisher,
      object,
      publisherFirstAttribute,
      publisherSecondAttribute,
      firstValue,
      secondValue,
      tag);
  waitFor(
      subscriber,
      [&] { return !subscriber.recorder().reflections().empty(); },
      options,
      "reflection for the initially active attribute");
  requireOneAttributeReflection(
      subscriber.recorder(),
      object,
      subscriberFirstAttribute,
      subscriberSecondAttribute,
      firstValue,
      tag,
      "initial active/passive declarations");

  // Change both selected attributes, then send both values again. The RTI
  // should now reflect B only: A is passive and B is active.
  subscriber.recorder().clearReflection();
  subscriber.rtiAmbassador().subscribeObjectClassAttributes(
      subscriberClass, rti::AttributeHandleSet{subscriberFirstAttribute}, false, L"");
  subscriber.rtiAmbassador().subscribeObjectClassAttributes(
      subscriberClass, rti::AttributeHandleSet{subscriberSecondAttribute}, true, L"");
  firstValue = {0x4dU, 0x33U};
  secondValue = {0x4dU, 0x34U};
  tag = {0x4dU, 0x42U};
  updateBothAttributes(
      publisher,
      object,
      publisherFirstAttribute,
      publisherSecondAttribute,
      firstValue,
      secondValue,
      tag);
  waitFor(
      subscriber,
      [&] { return !subscriber.recorder().reflections().empty(); },
      options,
      "reflection for the newly active attribute");
  requireOneAttributeReflection(
      subscriber.recorder(),
      object,
      subscriberSecondAttribute,
      subscriberFirstAttribute,
      secondValue,
      tag,
      "selected active/passive mode changes");

  // Re-activating A alone must leave B active, so both attributes are
  // reflected by the next ordinary update.
  subscriber.recorder().clearReflection();
  subscriber.rtiAmbassador().subscribeObjectClassAttributes(
      subscriberClass, rti::AttributeHandleSet{subscriberFirstAttribute}, true, L"");
  firstValue = {0x4dU, 0x35U};
  secondValue = {0x4dU, 0x36U};
  tag = {0x4dU, 0x43U};
  updateBothAttributes(
      publisher,
      object,
      publisherFirstAttribute,
      publisherSecondAttribute,
      firstValue,
      secondValue,
      tag);
  waitFor(
      subscriber,
      [&] { return !subscriber.recorder().reflections().empty(); },
      options,
      "reflection after reactivating one attribute");
  auto const reflections = subscriber.recorder().reflections();
  require(
      reflections.size() == 1U && reflections.front().object == object &&
          reflections.front().values.size() == 2U &&
          reflections.front().values.count(subscriberFirstAttribute) == 1U &&
          reflections.front().values.count(subscriberSecondAttribute) == 1U &&
          copyBytes(reflections.front().values.at(subscriberFirstAttribute)) == firstValue &&
          copyBytes(reflections.front().values.at(subscriberSecondAttribute)) == secondValue &&
          reflections.front().tag == tag,
      "reactivating one attribute did not preserve the other attribute's active mode");

  subscriber.rtiAmbassador().unsubscribeObjectClassAttributes(
      subscriberClass,
      rti::AttributeHandleSet{subscriberFirstAttribute, subscriberSecondAttribute});
  publisher.rtiAmbassador().unpublishObjectClassAttributes(
      publisherClass, publishedAttributes);
  subscriber.resign(rti::NO_ACTION);
  publisher.resign(rti::DELETE_OBJECTS);
  publisher.rtiAmbassador().destroyFederationExecution(federation);
  subscriber.disconnect();
  publisher.disconnect();
}

void scenarioAttributeSubscriptionPerAttributeModeTransitionsContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioAttributeSubscriptionPerAttributeModeTransitions(options, model);
}

}  // namespace hla_rti_cpp_tck::detail
