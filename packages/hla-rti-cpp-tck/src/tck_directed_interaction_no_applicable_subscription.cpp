#include "tck_main_scenarios.hpp"

#include <algorithm>
#include <chrono>
#include <thread>
#include <vector>

namespace hla_rti_cpp_tck::detail {

void scenarioDirectedInteractionNoApplicableSubscription(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.modelFom.empty(),
      "Directed-interaction no-subscription case requires an adapter-supplied model FOM");

  Session publisher(options, model, "directed-no-subscription-publisher");
  Session receiver(options, model, "directed-no-subscription-receiver");
  auto const federation = federationName(
      options,
      "directed-interaction-no-applicable-subscription");
  connectAndJoin(publisher, receiver, options, federation, options.modelFom);

  auto& publisherAmbassador = publisher.rtiAmbassador();
  auto& receiverAmbassador = receiver.rtiAmbassador();
  auto const publisherObjectClass = publisherAmbassador.getObjectClassHandle(
      options.typedObjectClassName);
  auto const receiverObjectClass = receiverAmbassador.getObjectClassHandle(
      options.typedObjectClassName);
  auto const publisherInteraction = publisherAmbassador.getInteractionClassHandle(
      options.typedInteractionClassName);
  auto const receiverInteraction = receiverAmbassador.getInteractionClassHandle(
      options.typedInteractionClassName);
  require(
      publisherObjectClass.isValid() && receiverObjectClass == publisherObjectClass &&
          publisherInteraction.isValid() && receiverInteraction == publisherInteraction,
      "directed no-subscription FOM class handles were invalid or unstable");

  auto const publisherIdentity = publisherAmbassador.getAttributeHandle(
      publisherObjectClass,
      options.typedIdentityAttributeName);
  auto const receiverIdentity = receiverAmbassador.getAttributeHandle(
      receiverObjectClass,
      options.typedIdentityAttributeName);
  auto const publisherParameter = publisherAmbassador.getParameterHandle(
      publisherInteraction,
      options.typedIntegerParameterName);
  auto const receiverParameter = receiverAmbassador.getParameterHandle(
      receiverInteraction,
      options.typedIntegerParameterName);
  require(
      publisherIdentity.isValid() && receiverIdentity == publisherIdentity &&
          publisherParameter.isValid() && receiverParameter == publisherParameter,
      "directed no-subscription FOM member handles were invalid or unstable");

  rti::AttributeHandleSet const publisherAttributes{publisherIdentity};
  rti::AttributeHandleSet const receiverAttributes{receiverIdentity};
  publisherAmbassador.publishObjectClassAttributes(
      publisherObjectClass,
      publisherAttributes);
  receiverAmbassador.subscribeObjectClassAttributes(
      receiverObjectClass,
      receiverAttributes,
      true,
      L"");

  rti::InteractionClassHandleSet const publishedInteractions{publisherInteraction};
  publisherAmbassador.publishObjectClassDirectedInteractions(
      publisherObjectClass,
      publishedInteractions);

  auto const target = publisherAmbassador.registerObjectInstance(publisherObjectClass);
  require(target.isValid(), "directed no-subscription target handle was invalid");
  waitFor(
      receiver,
      [&] { return receiver.recorder().hasDiscovery(target); },
      options,
      "receiver discovering the known directed-interaction target");

  std::vector<std::uint8_t> const parameterBytes{0x4eU};
  rti::ParameterHandleValueMap const parameters{{
      publisherParameter,
      rti::VariableLengthData(parameterBytes.data(), parameterBytes.size()),
  }};
  std::vector<std::uint8_t> const tagBytes{0x4eU, 0x4fU, 0x53U, 0x55U};
  rti::VariableLengthData const tag(tagBytes.data(), tagBytes.size());
  publisherAmbassador.sendDirectedInteraction(
      publisherInteraction,
      target,
      parameters,
      tag);

  auto const observationMilliseconds = std::max(
      1,
      std::min(options.timeoutMilliseconds, 500));
  auto const deadline = std::chrono::steady_clock::now() +
      std::chrono::milliseconds(observationMilliseconds);
  do {
    receiver.pump();
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  } while (std::chrono::steady_clock::now() < deadline);
  require(
      receiver.recorder().directedInteractions().empty() &&
          receiver.recorder().interactions().empty(),
      "the receiver without an applicable directed-interaction subscription received an interaction callback");

  receiverAmbassador.unsubscribeObjectClassAttributes(
      receiverObjectClass,
      receiverAttributes);
  publisherAmbassador.unpublishObjectClassDirectedInteractions(
      publisherObjectClass,
      publishedInteractions);
  publisherAmbassador.unpublishObjectClassAttributes(
      publisherObjectClass,
      publisherAttributes);
  receiver.resign(rti::NO_ACTION);
  publisher.resign(rti::DELETE_OBJECTS);
  publisherAmbassador.destroyFederationExecution(federation);
  receiver.disconnect();
  publisher.disconnect();
}

}  // namespace hla_rti_cpp_tck::detail
