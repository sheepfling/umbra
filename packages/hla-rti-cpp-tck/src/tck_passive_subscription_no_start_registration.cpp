#include "tck_passive_subscription_no_start_registration.hpp"
#include "tck_main_scenarios.hpp"

namespace hla_rti_cpp_tck::detail {

void scenarioPassiveSubscriptionDoesNotStartRegistration(
    Options const& options,
    rti::CallbackModel model) {
  Session publisher(options, model, "publisher");
  Session subscriber(options, model, "subscriber");
  auto const federation =
      federationName(options, "passive-subscription-no-start-registration");
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
      "passive-subscription registration-advisory lookup returned an invalid handle");

  rti::AttributeHandleSet const publishedAttributes{publisherAttribute};
  rti::AttributeHandleSet const subscribedAttributes{subscriberAttribute};
  publisher.rtiAmbassador().publishObjectClassAttributes(
      publisherClass, publishedAttributes);

  subscriber.rtiAmbassador().subscribeObjectClassAttributes(
      subscriberClass, subscribedAttributes, true, L"");
  waitFor(
      publisher,
      [&] { return !publisher.recorder().registrationStarts().empty(); },
      options,
      "active subscription Start Registration positive control");
  auto const activeStarts = publisher.recorder().registrationStarts();
  require(
      activeStarts.size() == 1U && activeStarts.front() == publisherClass,
      "active subscription registration positive control reported an unexpected class");

  subscriber.rtiAmbassador().unsubscribeObjectClassAttributes(
      subscriberClass, subscribedAttributes);
  for (int pass = 0; pass != 8; ++pass) {
    publisher.pump();
  }
  publisher.recorder().clearAdvisories();

  subscriber.rtiAmbassador().subscribeObjectClassAttributes(
      subscriberClass, subscribedAttributes, false, L"");
  for (int pass = 0; pass != 8; ++pass) {
    publisher.pump();
  }
  require(
      publisher.recorder().registrationStarts().empty(),
      "passive subscription caused Start Registration For Object Class at another federate");

  subscriber.rtiAmbassador().unsubscribeObjectClassAttributes(
      subscriberClass, subscribedAttributes);
  publisher.rtiAmbassador().unpublishObjectClassAttributes(
      publisherClass, publishedAttributes);
  subscriber.resign(rti::NO_ACTION);
  publisher.resign(rti::NO_ACTION);
  publisher.rtiAmbassador().destroyFederationExecution(federation);
  subscriber.disconnect();
  publisher.disconnect();
}

}  // namespace hla_rti_cpp_tck::detail
