#include "tck_default_rate_no_reduction.hpp"
#include "tck_main_scenarios.hpp"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace hla_rti_cpp_tck::detail {

void scenarioDefaultRateSubscriptionDoesNotReduce(
    Options const& options,
    rti::CallbackModel model) {
  Session publisher(options, model, "publisher");
  Session subscriber(options, model, "subscriber");
  auto const federation =
      federationName(options, "default-rate-subscription-no-reduction");
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
      "default-rate subscription lookup returned an invalid handle");

  rti::AttributeHandleSet const publishedAttributes{publisherAttribute};
  rti::AttributeHandleSet const subscribedAttributes{subscriberAttribute};
  publisher.rtiAmbassador().publishObjectClassAttributes(
      publisherClass, publishedAttributes);
  subscriber.rtiAmbassador().subscribeObjectClassAttributes(
      subscriberClass, subscribedAttributes, true, L"");
  auto const object = publisher.rtiAmbassador().registerObjectInstance(
      publisherClass);
  require(
      object.isValid(),
      "default-rate subscription registration returned an invalid handle");
  waitFor(
      subscriber,
      [&] { return subscriber.recorder().hasDiscovery(object); },
      options,
      "default-rate subscription object discovery");

  std::vector<std::vector<std::uint8_t>> const values{
      {0x44U, 0x45U, 0x46U, 0x31U},
      {0x44U, 0x45U, 0x46U, 0x32U},
      {0x44U, 0x45U, 0x46U, 0x33U}};
  std::vector<std::vector<std::uint8_t>> const tags{
      {0x54U, 0x41U, 0x47U, 0x31U},
      {0x54U, 0x41U, 0x47U, 0x32U},
      {0x54U, 0x41U, 0x47U, 0x33U}};
  for (std::size_t index = 0; index < values.size(); ++index) {
    rti::AttributeHandleValueMap const attributeValues{
        {publisherAttribute,
         rti::VariableLengthData(values[index].data(), values[index].size())}};
    rti::VariableLengthData const userTag(tags[index].data(), tags[index].size());
    publisher.rtiAmbassador().updateAttributeValues(
        object, attributeValues, userTag);
  }

  waitFor(
      subscriber,
      [&] { return subscriber.recorder().reflections().size() >= values.size(); },
      options,
      "default-rate subscription unreduced update burst");
  auto const reflections = subscriber.recorder().reflections();
  require(
      reflections.size() == values.size(),
      "default-rate subscription did not reflect every update in the burst");
  for (std::size_t index = 0; index < values.size(); ++index) {
    require(
        reflections[index].object == object &&
            reflections[index].values.size() == 1U &&
            reflections[index].values.count(subscriberAttribute) == 1U &&
            copyBytes(reflections[index].values.at(subscriberAttribute)) ==
                values[index] &&
            reflections[index].tag == tags[index],
        "default-rate subscription altered or reduced the update burst");
  }

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
