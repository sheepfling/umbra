#include "tck_main_scenarios.hpp"

#include <algorithm>
#include <string>
#include <vector>

namespace hla_rti_cpp_tck::detail {

void scenarioDirectedInteractionSubscriptionAccumulation(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.modelFom.empty(),
      "Directed-interaction subscription accumulation requires an adapter-supplied model FOM");

  Session publisher(options, model, "directed-subscription-accumulation-publisher");
  Session subscriber(options, model, "directed-subscription-accumulation-subscriber");
  auto const federation = federationName(
      options,
      "directed-interaction-subscription-accumulation");
  connectAndJoin(publisher, subscriber, options, federation, options.modelFom);

  auto& publisherAmbassador = publisher.rtiAmbassador();
  auto& subscriberAmbassador = subscriber.rtiAmbassador();
  auto const publisherObjectClass = publisherAmbassador.getObjectClassHandle(
      options.typedDerivedObjectClassName);
  auto const subscriberObjectClass = subscriberAmbassador.getObjectClassHandle(
      options.typedDerivedObjectClassName);
  auto const publisherIdentity = publisherAmbassador.getAttributeHandle(
      publisherObjectClass,
      L"Identity");
  auto const subscriberIdentity = subscriberAmbassador.getAttributeHandle(
      subscriberObjectClass,
      L"Identity");
  auto const publisherFirstInteraction = publisherAmbassador.getInteractionClassHandle(
      options.typedInteractionClassName);
  auto const publisherSecondInteraction = publisherAmbassador.getInteractionClassHandle(
      L"HLAinteractionRoot.TckTypedAccumulationSiblingInteraction");
  auto const subscriberFirstInteraction = subscriberAmbassador.getInteractionClassHandle(
      options.typedInteractionClassName);
  auto const subscriberSecondInteraction = subscriberAmbassador.getInteractionClassHandle(
      L"HLAinteractionRoot.TckTypedAccumulationSiblingInteraction");
  auto const publisherFirstParameter = publisherAmbassador.getParameterHandle(
      publisherFirstInteraction,
      L"IntegerParameter");
  auto const publisherSecondParameter = publisherAmbassador.getParameterHandle(
      publisherSecondInteraction,
      L"SiblingParameter");
  auto const subscriberFirstParameter = subscriberAmbassador.getParameterHandle(
      subscriberFirstInteraction,
      L"IntegerParameter");
  auto const subscriberSecondParameter = subscriberAmbassador.getParameterHandle(
      subscriberSecondInteraction,
      L"SiblingParameter");
  require(
      publisherObjectClass.isValid() &&
          subscriberObjectClass == publisherObjectClass &&
          publisherIdentity.isValid() && subscriberIdentity == publisherIdentity &&
          publisherFirstInteraction.isValid() && publisherSecondInteraction.isValid() &&
          publisherFirstInteraction != publisherSecondInteraction &&
          subscriberFirstInteraction == publisherFirstInteraction &&
          subscriberSecondInteraction == publisherSecondInteraction &&
          publisherFirstParameter.isValid() && publisherSecondParameter.isValid() &&
          subscriberFirstParameter.isValid() && subscriberSecondParameter.isValid(),
      "directed-subscription accumulation FOM handles were invalid or unstable");

  rti::AttributeHandleSet const publisherAttributes{publisherIdentity};
  rti::AttributeHandleSet const subscriberAttributes{subscriberIdentity};
  publisherAmbassador.publishObjectClassAttributes(
      publisherObjectClass,
      publisherAttributes);
  subscriberAmbassador.subscribeObjectClassAttributes(
      subscriberObjectClass,
      subscriberAttributes,
      true,
      L"");

  rti::InteractionClassHandleSet const publishedInteractions{
      publisherFirstInteraction,
      publisherSecondInteraction};
  rti::InteractionClassHandleSet const firstSubscription{subscriberFirstInteraction};
  rti::InteractionClassHandleSet const secondSubscription{subscriberSecondInteraction};
  publisherAmbassador.publishObjectClassDirectedInteractions(
      publisherObjectClass,
      publishedInteractions);
  subscriberAmbassador.subscribeObjectClassDirectedInteractions(
      subscriberObjectClass,
      firstSubscription,
      true);
  subscriberAmbassador.subscribeObjectClassDirectedInteractions(
      subscriberObjectClass,
      secondSubscription,
      true);

  auto const target = publisherAmbassador.registerObjectInstance(publisherObjectClass);
  require(target.isValid(), "directed-subscription accumulation target handle was invalid");
  waitFor(
      subscriber,
      [&] { return subscriber.recorder().hasDiscovery(target); },
      options,
      "directed-subscription accumulation target discovery");

  std::vector<std::uint8_t> const firstParameterBytes{0x31U};
  std::vector<std::uint8_t> const secondParameterBytes{0x32U};
  rti::VariableLengthData const firstParameterValue(
      firstParameterBytes.data(), firstParameterBytes.size());
  rti::VariableLengthData const secondParameterValue(
      secondParameterBytes.data(), secondParameterBytes.size());
  rti::ParameterHandleValueMap const firstParameters{
      {publisherFirstParameter, firstParameterValue}};
  rti::ParameterHandleValueMap const secondParameters{
      {publisherSecondParameter, secondParameterValue}};
  std::vector<std::uint8_t> const firstTagBytes{0x53U, 0x55U, 0x42U, 0x31U};
  std::vector<std::uint8_t> const secondTagBytes{0x53U, 0x55U, 0x42U, 0x32U};
  rti::VariableLengthData const firstTag(firstTagBytes.data(), firstTagBytes.size());
  rti::VariableLengthData const secondTag(secondTagBytes.data(), secondTagBytes.size());
  publisherAmbassador.sendDirectedInteraction(
      publisherFirstInteraction,
      target,
      firstParameters,
      firstTag);
  publisherAmbassador.sendDirectedInteraction(
      publisherSecondInteraction,
      target,
      secondParameters,
      secondTag);

  waitFor(
      subscriber,
      [&] { return subscriber.recorder().directedInteractions().size() == 2U; },
      options,
      "both directed interactions after sequential subscriptions");
  auto const deliveries = subscriber.recorder().directedInteractions();
  auto const firstDelivery = std::find_if(
      deliveries.begin(),
      deliveries.end(),
      [&](auto const& delivery) {
        return delivery.interaction == subscriberFirstInteraction;
      });
  auto const secondDelivery = std::find_if(
      deliveries.begin(),
      deliveries.end(),
      [&](auto const& delivery) {
        return delivery.interaction == subscriberSecondInteraction;
      });
  require(
      deliveries.size() == 2U &&
          firstDelivery != deliveries.end() && secondDelivery != deliveries.end() &&
          firstDelivery->object == target && secondDelivery->object == target &&
          firstDelivery->tag == firstTagBytes && secondDelivery->tag == secondTagBytes &&
          firstDelivery->parameters.size() == 1U &&
          secondDelivery->parameters.size() == 1U &&
          firstDelivery->parameters.find(subscriberFirstParameter) !=
              firstDelivery->parameters.end() &&
          secondDelivery->parameters.find(subscriberSecondParameter) !=
              secondDelivery->parameters.end() &&
          firstDelivery->parameters.at(subscriberFirstParameter).size() ==
              firstParameterBytes.size() &&
          secondDelivery->parameters.at(subscriberSecondParameter).size() ==
              secondParameterBytes.size() &&
          std::equal(
              firstParameterBytes.begin(),
              firstParameterBytes.end(),
              static_cast<std::uint8_t const*>(
                  firstDelivery->parameters.at(subscriberFirstParameter).data())) &&
          std::equal(
              secondParameterBytes.begin(),
              secondParameterBytes.end(),
              static_cast<std::uint8_t const*>(
                  secondDelivery->parameters.at(subscriberSecondParameter).data())),
      "a later directed-interaction subscription removed the previously selected class");

  subscriberAmbassador.unsubscribeObjectClassDirectedInteractions(
      subscriberObjectClass,
      firstSubscription);
  subscriberAmbassador.unsubscribeObjectClassDirectedInteractions(
      subscriberObjectClass,
      secondSubscription);
  subscriberAmbassador.unsubscribeObjectClassAttributes(
      subscriberObjectClass,
      subscriberAttributes);
  publisherAmbassador.unpublishObjectClassDirectedInteractions(
      publisherObjectClass,
      publishedInteractions);
  publisherAmbassador.unpublishObjectClassAttributes(
      publisherObjectClass,
      publisherAttributes);
  subscriber.resign(rti::NO_ACTION);
  publisher.resign(rti::DELETE_OBJECTS);
  publisherAmbassador.destroyFederationExecution(federation);
  subscriber.disconnect();
  publisher.disconnect();
}

}  // namespace hla_rti_cpp_tck::detail
