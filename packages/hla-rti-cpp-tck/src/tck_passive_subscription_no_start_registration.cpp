#include "tck_passive_subscription_no_start_registration.hpp"
#include "tck_main_scenarios.hpp"

#include <cstdint>
#include <vector>

namespace hla_rti_cpp_tck::detail {

void scenarioPassiveSubscriptionDoesNotStartRegistration(
    Options const& options,
    rti::CallbackModel model) {
  Session publisher(options, model, "publisher");
  Session activeSubscriber(options, model, "active-subscriber");
  Session passiveSubscriber(options, model, "passive-subscriber");
  auto const federation =
      federationName(options, "passive-subscription-no-start-registration");
  connectAndJoin(publisher, activeSubscriber, options, federation, options.fom);
  passiveSubscriber.connect();
  passiveSubscriber.join(
      options.memberFederateName + L"-passive-subscription",
      options.federateType,
      federation);

  auto const publisherClass =
      publisher.rtiAmbassador().getObjectClassHandle(options.objectClassName);
  auto const activeSubscriberClass =
      activeSubscriber.rtiAmbassador().getObjectClassHandle(options.objectClassName);
  auto const passiveSubscriberClass =
      passiveSubscriber.rtiAmbassador().getObjectClassHandle(options.objectClassName);
  auto const publisherAttribute = publisher.rtiAmbassador().getAttributeHandle(
      publisherClass, options.attributeName);
  auto const activeSubscriberAttribute =
      activeSubscriber.rtiAmbassador().getAttributeHandle(
          activeSubscriberClass, options.attributeName);
  auto const passiveSubscriberAttribute =
      passiveSubscriber.rtiAmbassador().getAttributeHandle(
          passiveSubscriberClass, options.attributeName);
  require(
      publisherClass.isValid() && activeSubscriberClass.isValid() &&
          passiveSubscriberClass.isValid() && publisherAttribute.isValid() &&
          activeSubscriberAttribute.isValid() && passiveSubscriberAttribute.isValid(),
      "passive-subscription registration-advisory lookup returned an invalid handle");

  rti::AttributeHandleSet const publishedAttributes{publisherAttribute};
  rti::AttributeHandleSet const activeSubscribedAttributes{activeSubscriberAttribute};
  rti::AttributeHandleSet const passiveSubscribedAttributes{passiveSubscriberAttribute};
  publisher.rtiAmbassador().publishObjectClassAttributes(
      publisherClass, publishedAttributes);

  activeSubscriber.rtiAmbassador().subscribeObjectClassAttributes(
      activeSubscriberClass, activeSubscribedAttributes, true, L"");
  waitFor(
      publisher,
      [&] { return !publisher.recorder().registrationStarts().empty(); },
      options,
      "active subscription Start Registration positive control");
  auto const activeStarts = publisher.recorder().registrationStarts();
  require(
      activeStarts.size() == 1U && activeStarts.front() == publisherClass,
      "active subscription registration positive control reported an unexpected class");

  activeSubscriber.rtiAmbassador().unsubscribeObjectClassAttributes(
      activeSubscriberClass, activeSubscribedAttributes);
  for (int pass = 0; pass != 8; ++pass) {
    publisher.pump();
  }
  publisher.recorder().clearAdvisories();

  passiveSubscriber.rtiAmbassador().subscribeObjectClassAttributes(
      passiveSubscriberClass, passiveSubscribedAttributes, false, L"");
  for (int pass = 0; pass != 8; ++pass) {
    publisher.pump();
  }
  require(
      publisher.recorder().registrationStarts().empty(),
      "passive subscription caused Start Registration For Object Class at another federate");

  std::vector<std::uint8_t> const passiveOnlyValue{0x50U, 0x41U, 0x53U};
  rti::AttributeHandleValueMap passiveOnlyValues;
  passiveOnlyValues.emplace(
      publisherAttribute,
      rti::VariableLengthData(
          passiveOnlyValue.data(),
          passiveOnlyValue.size()));
  rti::VariableLengthData const tag{};
  auto const passiveOnlyObject =
      publisher.rtiAmbassador().registerObjectInstance(publisherClass);
  publisher.rtiAmbassador().updateAttributeValues(
      passiveOnlyObject,
      passiveOnlyValues,
      tag);
  auto const passiveOnlyDeadline =
      Clock::now() + std::chrono::milliseconds(options.timeoutMilliseconds);
  while (Clock::now() < passiveOnlyDeadline) {
    publisher.pump();
    activeSubscriber.pump();
    passiveSubscriber.pump();
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
  require(
      activeSubscriber.recorder().discoveryCount() == 0U &&
          passiveSubscriber.recorder().discoveryCount() == 0U &&
          activeSubscriber.recorder().reflections().empty() &&
          passiveSubscriber.recorder().reflections().empty(),
      "passive object-attribute subscription alone arranged discovery or reflection");
  publisher.rtiAmbassador().deleteObjectInstance(passiveOnlyObject, tag);

  activeSubscriber.rtiAmbassador().subscribeObjectClassAttributes(
      activeSubscriberClass, activeSubscribedAttributes, true, L"");
  waitFor(
      publisher,
      [&] { return !publisher.recorder().registrationStarts().empty(); },
      options,
      "active subscription Start Registration positive control after passive-only phase");
  auto const object = publisher.rtiAmbassador().registerObjectInstance(publisherClass);
  waitFor(
      activeSubscriber,
      [&] { return activeSubscriber.recorder().hasDiscovery(object); },
      options,
      "active subscription discovery with passive observer");
  waitFor(
      passiveSubscriber,
      [&] { return passiveSubscriber.recorder().hasDiscovery(object); },
      options,
      "passive subscription discovery after separate active interest");

  std::vector<std::uint8_t> const activeValue{0x41U, 0x43U, 0x54U};
  rti::AttributeHandleValueMap activeValues;
  activeValues.emplace(
      publisherAttribute,
      rti::VariableLengthData(activeValue.data(), activeValue.size()));
  publisher.rtiAmbassador().updateAttributeValues(object, activeValues, tag);
  waitFor(
      activeSubscriber,
      [&] { return !activeSubscriber.recorder().reflections().empty(); },
      options,
      "active object-attribute reflection");
  waitFor(
      passiveSubscriber,
      [&] { return !passiveSubscriber.recorder().reflections().empty(); },
      options,
      "passive object-attribute reflection under separate active interest");
  auto const activeReflections = activeSubscriber.recorder().reflections();
  auto const passiveReflections = passiveSubscriber.recorder().reflections();
  require(
      activeReflections.size() == 1U && activeReflections.front().object == object &&
          activeReflections.front().values.size() == 1U &&
          activeReflections.front().values.count(activeSubscriberAttribute) == 1U &&
          copyBytes(activeReflections.front().values.at(activeSubscriberAttribute)) ==
              activeValue,
      "active subscriber received the wrong attribute update");
  require(
      passiveReflections.size() == 1U && passiveReflections.front().object == object &&
          passiveReflections.front().values.size() == 1U &&
          passiveReflections.front().values.count(passiveSubscriberAttribute) == 1U &&
          copyBytes(passiveReflections.front().values.at(passiveSubscriberAttribute)) ==
              activeValue,
      "passive subscriber did not receive data made relevant by separate active interest");

  activeSubscriber.rtiAmbassador().unsubscribeObjectClassAttributes(
      activeSubscriberClass, activeSubscribedAttributes);
  passiveSubscriber.rtiAmbassador().unsubscribeObjectClassAttributes(
      passiveSubscriberClass, passiveSubscribedAttributes);
  publisher.rtiAmbassador().deleteObjectInstance(object, tag);
  publisher.rtiAmbassador().unpublishObjectClassAttributes(
      publisherClass, publishedAttributes);
  passiveSubscriber.resign(rti::NO_ACTION);
  activeSubscriber.resign(rti::NO_ACTION);
  publisher.resign(rti::NO_ACTION);
  publisher.rtiAmbassador().destroyFederationExecution(federation);
  passiveSubscriber.disconnect();
  activeSubscriber.disconnect();
  publisher.disconnect();
}

}  // namespace hla_rti_cpp_tck::detail
