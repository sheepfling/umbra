#include "tck_main_scenarios.hpp"

#include <cstdint>
#include <vector>

namespace hla_rti_cpp_tck::detail {

void scenarioDirectedInteractionReceivedClassProjection(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.modelFom.empty(),
      "Directed interaction received-class projection requires an adapter-supplied model FOM");

  Session publisher(options, model, "directed-class-projection-publisher");
  Session closestSuperclassSubscriber(
      options, model, "directed-class-projection-superclass-subscriber");
  Session sentClassSubscriber(options, model, "directed-class-projection-sent-subscriber");
  auto const federation = federationName(
      options,
      "directed-interaction-received-class-projection");
  connectAndJoin(
      publisher,
      closestSuperclassSubscriber,
      options,
      federation,
      options.modelFom);
  sentClassSubscriber.connect();
  sentClassSubscriber.join(
      options.memberFederateName + L"-directed-class-projection-sent-class",
      options.federateType,
      federation);

  auto& publisherAmbassador = publisher.rtiAmbassador();
  auto& superclassAmbassador = closestSuperclassSubscriber.rtiAmbassador();
  auto& sentClassAmbassador = sentClassSubscriber.rtiAmbassador();
  auto const publisherObjectClass = publisherAmbassador.getObjectClassHandle(
      options.typedDerivedObjectClassName);
  auto const superclassObjectClass = superclassAmbassador.getObjectClassHandle(
      options.typedDerivedObjectClassName);
  auto const sentClassObjectClass = sentClassAmbassador.getObjectClassHandle(
      options.typedDerivedObjectClassName);
  auto const publisherBaseInteraction = publisherAmbassador.getInteractionClassHandle(
      options.typedInteractionClassName);
  auto const publisherDerivedInteraction = publisherAmbassador.getInteractionClassHandle(
      options.typedDerivedInteractionClassName);
  auto const superclassBaseInteraction = superclassAmbassador.getInteractionClassHandle(
      options.typedInteractionClassName);
  auto const sentClassDerivedInteraction = sentClassAmbassador.getInteractionClassHandle(
      options.typedDerivedInteractionClassName);
  require(
      publisherObjectClass.isValid() &&
          superclassObjectClass == publisherObjectClass &&
          sentClassObjectClass == publisherObjectClass &&
          publisherBaseInteraction.isValid() &&
          publisherDerivedInteraction.isValid() &&
          superclassBaseInteraction == publisherBaseInteraction &&
          sentClassDerivedInteraction == publisherDerivedInteraction,
      "directed received-class projection returned invalid or unstable class handles");

  auto const publisherIdentity = publisherAmbassador.getAttributeHandle(
      publisherObjectClass,
      options.typedIdentityAttributeName);
  auto const superclassIdentity = superclassAmbassador.getAttributeHandle(
      superclassObjectClass,
      options.typedIdentityAttributeName);
  auto const sentClassIdentity = sentClassAmbassador.getAttributeHandle(
      sentClassObjectClass,
      options.typedIdentityAttributeName);
  auto const publisherBaseParameter = publisherAmbassador.getParameterHandle(
      publisherBaseInteraction,
      options.typedIntegerParameterName);
  auto const publisherDerivedBaseParameter = publisherAmbassador.getParameterHandle(
      publisherDerivedInteraction,
      options.typedIntegerParameterName);
  auto const publisherDerivedParameter = publisherAmbassador.getParameterHandle(
      publisherDerivedInteraction,
      options.typedDerivedParameterName);
  auto const superclassBaseParameter = superclassAmbassador.getParameterHandle(
      superclassBaseInteraction,
      options.typedIntegerParameterName);
  auto const sentClassBaseParameter = sentClassAmbassador.getParameterHandle(
      sentClassDerivedInteraction,
      options.typedIntegerParameterName);
  auto const sentClassDerivedParameter = sentClassAmbassador.getParameterHandle(
      sentClassDerivedInteraction,
      options.typedDerivedParameterName);
  require(
      publisherIdentity.isValid() && superclassIdentity == publisherIdentity &&
          sentClassIdentity == publisherIdentity && publisherBaseParameter.isValid() &&
          publisherDerivedBaseParameter == publisherBaseParameter &&
          superclassBaseParameter == publisherBaseParameter &&
          publisherDerivedParameter.isValid() &&
          sentClassBaseParameter == publisherBaseParameter &&
          sentClassDerivedParameter == publisherDerivedParameter,
      "directed received-class projection returned invalid or unstable member handles");

  rti::AttributeHandleSet const publisherAttributes{publisherIdentity};
  rti::AttributeHandleSet const superclassAttributes{superclassIdentity};
  rti::AttributeHandleSet const sentClassAttributes{sentClassIdentity};
  publisherAmbassador.publishObjectClassAttributes(
      publisherObjectClass,
      publisherAttributes);
  superclassAmbassador.subscribeObjectClassAttributes(
      superclassObjectClass,
      superclassAttributes,
      true,
      L"");
  sentClassAmbassador.subscribeObjectClassAttributes(
      sentClassObjectClass,
      sentClassAttributes,
      true,
      L"");

  rti::InteractionClassHandleSet const publishedInteractions{
      publisherBaseInteraction,
      publisherDerivedInteraction};
  rti::InteractionClassHandleSet const superclassSubscription{
      superclassBaseInteraction};
  rti::InteractionClassHandleSet const sentClassSubscription{
      sentClassDerivedInteraction};
  publisherAmbassador.publishObjectClassDirectedInteractions(
      publisherObjectClass,
      publishedInteractions);
  publisherAmbassador.subscribeObjectClassDirectedInteractions(
      publisherObjectClass,
      publishedInteractions,
      true);
  superclassAmbassador.subscribeObjectClassDirectedInteractions(
      superclassObjectClass,
      superclassSubscription,
      true);
  sentClassAmbassador.subscribeObjectClassDirectedInteractions(
      sentClassObjectClass,
      sentClassSubscription,
      true);

  auto const target = publisherAmbassador.registerObjectInstance(
      publisherObjectClass);
  require(target.isValid(), "directed received-class target object handle was invalid");
  waitFor(
      closestSuperclassSubscriber,
      [&] { return closestSuperclassSubscriber.recorder().hasDiscovery(target); },
      options,
      "directed received-class superclass subscriber discovering target");
  waitFor(
      sentClassSubscriber,
      [&] { return sentClassSubscriber.recorder().hasDiscovery(target); },
      options,
      "directed received-class sent-class subscriber discovering target");

  std::vector<std::uint8_t> const baseValue{0x51U};
  std::vector<std::uint8_t> const derivedValue{0x72U};
  rti::ParameterHandleValueMap parameters;
  parameters.emplace(
      publisherDerivedBaseParameter,
      rti::VariableLengthData(baseValue.data(), baseValue.size()));
  parameters.emplace(
      publisherDerivedParameter,
      rti::VariableLengthData(derivedValue.data(), derivedValue.size()));
  std::vector<std::uint8_t> tagBytes{0x44U, 0x49U, 0x52U, 0x50U};
  rti::VariableLengthData tag(tagBytes.data(), tagBytes.size());

  // The derived parameter exists in the FDD, but is unavailable to the base
  // interaction class. A rejected send must not leak a directed callback.
  rti::ParameterHandleValueMap unavailableParameters;
  unavailableParameters.emplace(
      publisherBaseParameter,
      rti::VariableLengthData(baseValue.data(), baseValue.size()));
  unavailableParameters.emplace(
      publisherDerivedParameter,
      rti::VariableLengthData(derivedValue.data(), derivedValue.size()));
  requireException(
      [&] {
        publisherAmbassador.sendDirectedInteraction(
            publisherBaseInteraction, target, unavailableParameters, tag);
      },
      L"InteractionParameterNotDefined",
      "directed send using a parameter unavailable to its interaction class");
  for (int pass = 0; pass != 16; ++pass) {
    publisher.pump();
    closestSuperclassSubscriber.pump();
    sentClassSubscriber.pump();
  }
  require(
      publisher.recorder().directedInteractions().empty() &&
          closestSuperclassSubscriber.recorder().directedInteractions().empty() &&
          sentClassSubscriber.recorder().directedInteractions().empty(),
      "rejected directed send delivered an interaction callback");

  publisherAmbassador.sendDirectedInteraction(
      publisherDerivedInteraction,
      target,
      parameters,
      tag);

  waitFor(
      closestSuperclassSubscriber,
      [&] {
        return closestSuperclassSubscriber.recorder().directedInteractions().size() == 1U;
      },
      options,
      "directed interaction projected to closest subscribed superclass");
  waitFor(
      sentClassSubscriber,
      [&] { return sentClassSubscriber.recorder().directedInteractions().size() == 1U; },
      options,
      "directed interaction received at its subscribed sent class");
  for (int pass = 0; pass != 16; ++pass) {
    publisher.pump();
  }
  require(publisher.recorder().directedInteractions().empty(),
          "directed sender received its own interaction despite a matching subscription");

  auto const superclassDeliveries =
      closestSuperclassSubscriber.recorder().directedInteractions();
  auto const sentClassDeliveries = sentClassSubscriber.recorder().directedInteractions();
  require(
      superclassDeliveries.size() == 1U &&
          superclassDeliveries.front().interaction == superclassBaseInteraction &&
          superclassDeliveries.front().object == target &&
          superclassDeliveries.front().parameters.size() == 1U &&
          superclassDeliveries.front().parameters.count(superclassBaseParameter) == 1U &&
          superclassDeliveries.front().parameters.count(publisherDerivedParameter) == 0U &&
          copyBytes(superclassDeliveries.front().parameters.at(superclassBaseParameter)) ==
              baseValue &&
          superclassDeliveries.front().tag == tagBytes,
      "directed interaction did not report the closest subscribed superclass and its parameter projection");
  require(
      sentClassDeliveries.size() == 1U &&
          sentClassDeliveries.front().interaction == sentClassDerivedInteraction &&
          sentClassDeliveries.front().object == target &&
          sentClassDeliveries.front().parameters.size() == 2U &&
          sentClassDeliveries.front().parameters.count(sentClassBaseParameter) == 1U &&
          sentClassDeliveries.front().parameters.count(sentClassDerivedParameter) == 1U &&
          copyBytes(sentClassDeliveries.front().parameters.at(sentClassBaseParameter)) ==
              baseValue &&
          copyBytes(sentClassDeliveries.front().parameters.at(sentClassDerivedParameter)) ==
              derivedValue &&
          sentClassDeliveries.front().tag == tagBytes,
      "directed interaction did not retain its sent class and inherited/derived parameters for the exact-class subscriber");

  superclassAmbassador.unsubscribeObjectClassDirectedInteractions(
      superclassObjectClass,
      superclassSubscription);
  sentClassAmbassador.unsubscribeObjectClassDirectedInteractions(
      sentClassObjectClass,
      sentClassSubscription);
  publisherAmbassador.unsubscribeObjectClassDirectedInteractions(
      publisherObjectClass,
      publishedInteractions);
  superclassAmbassador.unsubscribeObjectClassAttributes(
      superclassObjectClass,
      superclassAttributes);
  sentClassAmbassador.unsubscribeObjectClassAttributes(
      sentClassObjectClass,
      sentClassAttributes);
  publisherAmbassador.unpublishObjectClassDirectedInteractions(
      publisherObjectClass,
      publishedInteractions);
  publisherAmbassador.unpublishObjectClassAttributes(
      publisherObjectClass,
      publisherAttributes);
  closestSuperclassSubscriber.resign(rti::NO_ACTION);
  sentClassSubscriber.resign(rti::NO_ACTION);
  publisher.resign(rti::DELETE_OBJECTS);
  publisherAmbassador.destroyFederationExecution(federation);
  sentClassSubscriber.disconnect();
  closestSuperclassSubscriber.disconnect();
  publisher.disconnect();
}

}  // namespace hla_rti_cpp_tck::detail
