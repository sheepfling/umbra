#include "tck_named_update_rate_subscription.hpp"
#include "tck_main_scenarios.hpp"

namespace hla_rti_cpp_tck::detail {

void scenarioNamedUpdateRateSubscription(
    Options const& options,
    rti::CallbackModel model) {
  Session publisher(options, model, "publisher");
  Session subscriber(options, model, "subscriber");
  auto const federation =
      federationName(options, "named-update-rate-subscription");
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
      "named update-rate subscription lookup returned an invalid handle");

  rti::AttributeHandleSet const publishedAttributes{publisherAttribute};
  rti::AttributeHandleSet const subscribedAttributes{subscriberAttribute};
  publisher.rtiAmbassador().publishObjectClassAttributes(
      publisherClass, publishedAttributes);
  subscriber.rtiAmbassador().subscribeObjectClassAttributes(
      subscriberClass, subscribedAttributes, true, L"High");

  auto const object = publisher.rtiAmbassador().registerObjectInstance(
      publisherClass);
  require(
      object.isValid(),
      "named update-rate subscription registration returned an invalid handle");
  waitFor(
      subscriber,
      [&] { return subscriber.recorder().hasDiscovery(object); },
      options,
      "named update-rate subscription object discovery");

  auto const requestedRate = subscriber.rtiAmbassador().getUpdateRateValue(L"High");
  require(
      requestedRate > 0.0,
      "named update-rate subscription designator did not resolve to a positive rate");
  require(
      subscriber.rtiAmbassador().getUpdateRateValueForAttribute(
          object, subscriberAttribute) == requestedRate,
      "subscribed attribute was not associated with its specified update rate");

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
