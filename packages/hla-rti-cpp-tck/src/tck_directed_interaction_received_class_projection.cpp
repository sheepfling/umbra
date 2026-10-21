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

void scenarioDirectedKnownObjectClassFallback(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.modelFom.empty(),
      "Directed known-class fallback requires an adapter-supplied directed hierarchy FOM");

  Session publisher(options, model, "directed-known-class-publisher");
  Session receiver(options, model, "directed-known-class-receiver");
  auto const federation = federationName(options, "directed-known-object-class-fallback");
  connectAndJoin(publisher, receiver, options, federation, options.modelFom);

  auto& sender = publisher.rtiAmbassador();
  auto& targetMember = receiver.rtiAmbassador();
  auto const publishedObjectClass = sender.getObjectClassHandle(
      options.typedDerivedObjectClassName);
  auto const knownBaseClass = targetMember.getObjectClassHandle(
      options.typedObjectClassName);
  auto const inapplicableChildClass = targetMember.getObjectClassHandle(
      options.typedDerivedObjectClassName);
  auto const sentInteraction = sender.getInteractionClassHandle(
      options.typedDerivedInteractionClassName);
  auto const receivedBaseInteraction = targetMember.getInteractionClassHandle(
      options.typedInteractionClassName);
  auto const childInteraction = targetMember.getInteractionClassHandle(
      options.typedDerivedInteractionClassName);
  require(
      publishedObjectClass.isValid() && knownBaseClass.isValid() &&
          inapplicableChildClass.isValid() && knownBaseClass != inapplicableChildClass &&
          sentInteraction.isValid() && receivedBaseInteraction.isValid() &&
          childInteraction == sentInteraction && receivedBaseInteraction != childInteraction,
      "adapter FOM did not provide the directed object/interaction hierarchies");

  auto const publisherMarker = sender.getAttributeHandle(
      publishedObjectClass, options.typedIdentityAttributeName);
  auto const receiverMarker = targetMember.getAttributeHandle(
      knownBaseClass, options.typedIdentityAttributeName);
  auto const sentBaseParameter = sender.getParameterHandle(
      sentInteraction, options.typedIntegerParameterName);
  auto const sentChildParameter = sender.getParameterHandle(
      sentInteraction, options.typedDerivedParameterName);
  auto const receivedBaseParameter = targetMember.getParameterHandle(
      receivedBaseInteraction, options.typedIntegerParameterName);
  require(
      publisherMarker.isValid() && receiverMarker == publisherMarker &&
          sentBaseParameter.isValid() && sentChildParameter.isValid() &&
          receivedBaseParameter == sentBaseParameter,
      "directed known-class fixture member handles were invalid or unstable");

  rti::AttributeHandleSet const senderAttributes{publisherMarker};
  rti::AttributeHandleSet const receiverAttributes{receiverMarker};
  rti::InteractionClassHandleSet const sentClasses{sentInteraction};
  rti::InteractionClassHandleSet const baseSubscription{receivedBaseInteraction};
  rti::InteractionClassHandleSet const childSubscription{childInteraction};
  sender.publishObjectClassAttributes(publishedObjectClass, senderAttributes);
  targetMember.subscribeObjectClassAttributes(
      knownBaseClass, receiverAttributes, true, L"");
  sender.publishObjectClassDirectedInteractions(publishedObjectClass, sentClasses);
  targetMember.subscribeObjectClassDirectedInteractions(
      knownBaseClass, baseSubscription, true);
  targetMember.subscribeObjectClassDirectedInteractions(
      inapplicableChildClass, childSubscription, true);

  auto const object = sender.registerObjectInstance(publishedObjectClass);
  waitFor(
      receiver,
      [&] { return receiver.recorder().hasDiscovery(object); },
      options,
      "base-class discovery of the directed target");
  auto const discovery = receiver.recorder().discovery();
  require(
      receiver.recorder().discoveryCount() == 1U && discovery.object == object &&
          discovery.objectClass == knownBaseClass &&
          targetMember.getKnownObjectClassHandle(object) == knownBaseClass,
      "receiver did not know the derived target at only its base object class");

  std::vector<std::uint8_t> const baseValue{0x42U};
  std::vector<std::uint8_t> const childValue{0x43U};
  std::vector<std::uint8_t> const tagBytes{0x44U, 0x4BU, 0x43U};
  rti::ParameterHandleValueMap parameters;
  parameters.emplace(
      sentBaseParameter,
      rti::VariableLengthData(baseValue.data(), baseValue.size()));
  parameters.emplace(
      sentChildParameter,
      rti::VariableLengthData(childValue.data(), childValue.size()));
  sender.sendDirectedInteraction(
      sentInteraction,
      object,
      parameters,
      rti::VariableLengthData(tagBytes.data(), tagBytes.size()));
  waitFor(
      receiver,
      [&] { return receiver.recorder().directedInteractions().size() >= 1U; },
      options,
      "receive-order directed fallback to the known object class");
  for (int pass = 0; pass != 8; ++pass) {
    receiver.pump();
  }
  auto const deliveries = receiver.recorder().directedInteractions();
  require(
      deliveries.size() == 1U &&
          deliveries.front().interaction == receivedBaseInteraction &&
          deliveries.front().object == object &&
          deliveries.front().producer == publisher.federateHandle() &&
          deliveries.front().parameters.size() == 1U &&
          deliveries.front().parameters.count(receivedBaseParameter) == 1U &&
          deliveries.front().parameters.count(sentChildParameter) == 0U &&
          copyBytes(deliveries.front().parameters.at(receivedBaseParameter)) == baseValue &&
          deliveries.front().tag == tagBytes,
      "directed receipt did not select the base known-class subscription and parameter projection");

  targetMember.unsubscribeObjectClassDirectedInteractions(
      inapplicableChildClass, childSubscription);
  targetMember.unsubscribeObjectClassDirectedInteractions(
      knownBaseClass, baseSubscription);
  sender.unpublishObjectClassDirectedInteractions(publishedObjectClass, sentClasses);
  targetMember.unsubscribeObjectClassAttributes(knownBaseClass, receiverAttributes);
  sender.unpublishObjectClassAttributes(publishedObjectClass, senderAttributes);
  receiver.resign(rti::NO_ACTION);
  publisher.resign(rti::DELETE_OBJECTS);
  sender.destroyFederationExecution(federation);
  receiver.disconnect();
  publisher.disconnect();
}

void scenarioDirectedKnownObjectClassFallbackContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioDirectedKnownObjectClassFallback(options, model);
}

}  // namespace hla_rti_cpp_tck::detail
