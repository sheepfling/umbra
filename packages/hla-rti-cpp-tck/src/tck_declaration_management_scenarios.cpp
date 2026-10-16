#include "tck_main_scenarios.hpp"

namespace hla_rti_cpp_tck::detail {

void scenarioDeclarations(Options const& options, rti::CallbackModel model) {
  Session declarationLifecycle(options, model, "declaration-lifecycle");
  rti::ObjectClassHandle const invalidObjectClass;
  rti::AttributeHandleSet const emptyAttributes;
  rti::InteractionClassHandle const invalidInteractionClass;
  auto requireDeclarationLifecycle = [&](std::wstring const& expected,
                                          std::string const& phase) {
    requireException(
        [&] {
          declarationLifecycle.rtiAmbassador().publishObjectClassAttributes(
              invalidObjectClass,
              emptyAttributes);
        },
        expected,
        "publishing object attributes " + phase);
    requireException(
        [&] {
          declarationLifecycle.rtiAmbassador().unpublishObjectClass(
              invalidObjectClass);
        },
        expected,
        "unpublishing an object class " + phase);
    requireException(
        [&] {
          declarationLifecycle.rtiAmbassador().unpublishObjectClassAttributes(
              invalidObjectClass,
              emptyAttributes);
        },
        expected,
        "unpublishing object attributes " + phase);
    requireException(
        [&] {
          declarationLifecycle.rtiAmbassador().subscribeObjectClassAttributes(
              invalidObjectClass,
              emptyAttributes);
        },
        expected,
        "subscribing to object attributes " + phase);
    requireException(
        [&] {
          declarationLifecycle.rtiAmbassador().unsubscribeObjectClass(
              invalidObjectClass);
        },
        expected,
        "unsubscribing an object class " + phase);
    requireException(
        [&] {
          declarationLifecycle.rtiAmbassador().unsubscribeObjectClassAttributes(
              invalidObjectClass,
              emptyAttributes);
        },
        expected,
        "unsubscribing object attributes " + phase);
    requireException(
        [&] {
          declarationLifecycle.rtiAmbassador().publishInteractionClass(
              invalidInteractionClass);
        },
        expected,
        "publishing an interaction class " + phase);
    requireException(
        [&] {
          declarationLifecycle.rtiAmbassador().unpublishInteractionClass(
              invalidInteractionClass);
        },
        expected,
        "unpublishing an interaction class " + phase);
    requireException(
        [&] {
          declarationLifecycle.rtiAmbassador().subscribeInteractionClass(
              invalidInteractionClass);
        },
        expected,
        "subscribing to an interaction class " + phase);
    requireException(
        [&] {
          declarationLifecycle.rtiAmbassador().unsubscribeInteractionClass(
              invalidInteractionClass);
        },
        expected,
        "unsubscribing an interaction class " + phase);
  };
  requireDeclarationLifecycle(L"NotConnected", "before connect");
  declarationLifecycle.connect();
  requireDeclarationLifecycle(L"FederateNotExecutionMember", "after connect before join");
  declarationLifecycle.disconnect();

  Session owner(options, model, "owner");
  Session member(options, model, "member");
  Session passive(options, model, "passive");
  auto const federation = federationName(options, "declarations");
  connectAndJoin(owner, member, options, federation, options.fom);
  passive.connect();
  passive.join(
      options.memberFederateName + L"-passive",
      options.federateType,
      federation);

  rti::ObjectClassHandle objectClass;
  rti::AttributeHandle attribute;
  rti::InteractionClassHandle interactionClass;
  rti::ParameterHandle parameter;
  handles(owner, options, objectClass, attribute, interactionClass, parameter);
  rti::AttributeHandleSet attributes;
  attributes.insert(attribute);
  rti::AttributeHandleSet const invalidAttributes{rti::AttributeHandle{}};
  requireException(
      [&] {
        owner.rtiAmbassador().publishObjectClassAttributes(
            invalidObjectClass,
            emptyAttributes);
      },
      L"ObjectClassNotDefined",
      "publishing attributes for an invalid object class");
  requireException(
      [&] {
        owner.rtiAmbassador().publishObjectClassAttributes(
            objectClass,
            invalidAttributes);
      },
      L"AttributeNotDefined",
      "publishing an invalid object attribute");
  requireException(
      [&] { owner.rtiAmbassador().unpublishObjectClass(invalidObjectClass); },
      L"ObjectClassNotDefined",
      "unpublishing an invalid object class");
  requireException(
      [&] {
        owner.rtiAmbassador().unpublishObjectClassAttributes(
            invalidObjectClass,
            emptyAttributes);
      },
      L"ObjectClassNotDefined",
      "unpublishing attributes for an invalid object class");
  requireException(
      [&] {
        owner.rtiAmbassador().unpublishObjectClassAttributes(
            objectClass,
            invalidAttributes);
      },
      L"AttributeNotDefined",
      "unpublishing an invalid object attribute");
  requireException(
      [&] {
        member.rtiAmbassador().subscribeObjectClassAttributes(
            invalidObjectClass,
            emptyAttributes);
      },
      L"ObjectClassNotDefined",
      "subscribing to an invalid object class");
  requireException(
      [&] {
        member.rtiAmbassador().subscribeObjectClassAttributes(
            objectClass,
            invalidAttributes);
      },
      L"AttributeNotDefined",
      "subscribing to an invalid object attribute");
  requireException(
      [&] { member.rtiAmbassador().unsubscribeObjectClass(invalidObjectClass); },
      L"ObjectClassNotDefined",
      "unsubscribing from an invalid object class");
  requireException(
      [&] {
        member.rtiAmbassador().unsubscribeObjectClassAttributes(
            invalidObjectClass,
            emptyAttributes);
      },
      L"ObjectClassNotDefined",
      "unsubscribing attributes from an invalid object class");
  requireException(
      [&] {
        member.rtiAmbassador().unsubscribeObjectClassAttributes(
            objectClass,
            invalidAttributes);
      },
      L"AttributeNotDefined",
      "unsubscribing an invalid object attribute");

  requireException(
      [&] { owner.rtiAmbassador().publishInteractionClass(invalidInteractionClass); },
      L"InteractionClassNotDefined",
      "publishing an invalid interaction class");
  requireException(
      [&] { owner.rtiAmbassador().unpublishInteractionClass(invalidInteractionClass); },
      L"InteractionClassNotDefined",
      "unpublishing an invalid interaction class");
  requireException(
      [&] { member.rtiAmbassador().subscribeInteractionClass(invalidInteractionClass); },
      L"InteractionClassNotDefined",
      "subscribing to an invalid interaction class");
  requireException(
      [&] { member.rtiAmbassador().unsubscribeInteractionClass(invalidInteractionClass); },
      L"InteractionClassNotDefined",
      "unsubscribing from an invalid interaction class");

  owner.rtiAmbassador().publishObjectClassAttributes(objectClass, attributes);
  member.rtiAmbassador().subscribeObjectClassAttributes(objectClass, attributes, true, L"");
  auto const passiveClass = passive.rtiAmbassador().getObjectClassHandle(options.objectClassName);
  auto const passiveAttribute =
      passive.rtiAmbassador().getAttributeHandle(passiveClass, options.attributeName);
  require(passiveClass.isValid() && passiveAttribute.isValid(),
          "Passive declaration handle lookup returned an invalid handle");
  rti::AttributeHandleSet passiveAttributes;
  passiveAttributes.insert(passiveAttribute);
  passive.rtiAmbassador().subscribeObjectClassAttributes(
      passiveClass, passiveAttributes, false, L"");
  owner.rtiAmbassador().publishInteractionClass(interactionClass);
  member.rtiAmbassador().subscribeInteractionClass(interactionClass, true);
  auto const passiveInteraction =
      passive.rtiAmbassador().getInteractionClassHandle(options.interactionClassName);
  require(passiveInteraction.isValid(),
          "Passive declaration interaction lookup returned an invalid handle");
  passive.rtiAmbassador().subscribeInteractionClass(passiveInteraction, false);

  member.rtiAmbassador().unsubscribeObjectClassAttributes(objectClass, attributes);
  member.rtiAmbassador().unsubscribeObjectClass(objectClass);
  member.rtiAmbassador().unsubscribeInteractionClass(interactionClass);
  passive.rtiAmbassador().unsubscribeObjectClassAttributes(passiveClass, passiveAttributes);
  passive.rtiAmbassador().unsubscribeObjectClass(passiveClass);
  passive.rtiAmbassador().unsubscribeInteractionClass(passiveInteraction);
  owner.rtiAmbassador().unpublishObjectClassAttributes(objectClass, attributes);
  owner.rtiAmbassador().unpublishObjectClass(objectClass);
  owner.rtiAmbassador().unpublishInteractionClass(interactionClass);
  passive.resign(rti::NO_ACTION);
}

void scenarioInteractionFullUnpublication(
    Options const& options,
    rti::CallbackModel model) {
  Session publisher(options, model, "publisher");
  Session subscriber(options, model, "subscriber");
  auto const federation = federationName(
      options,
      "interaction-full-unpublication");
  connectAndJoin(publisher, subscriber, options, federation, options.fom);

  auto const firstInteraction = publisher.rtiAmbassador().getInteractionClassHandle(
      options.interactionClassName);
  auto const secondInteraction = publisher.rtiAmbassador().getInteractionClassHandle(
      options.secondaryDirectedInteractionClassName);
  auto const subscriberFirstInteraction =
      subscriber.rtiAmbassador().getInteractionClassHandle(
          options.interactionClassName);
  auto const subscriberSecondInteraction =
      subscriber.rtiAmbassador().getInteractionClassHandle(
          options.secondaryDirectedInteractionClassName);
  auto const firstParameter = publisher.rtiAmbassador().getParameterHandle(
      firstInteraction,
      options.parameterName);
  auto const secondParameter = publisher.rtiAmbassador().getParameterHandle(
      secondInteraction,
      options.secondaryDirectedInteractionParameterName);
  require(
      firstInteraction.isValid() && secondInteraction.isValid() &&
          firstInteraction != secondInteraction &&
          subscriberFirstInteraction == firstInteraction &&
          subscriberSecondInteraction == secondInteraction &&
          firstParameter.isValid() && secondParameter.isValid(),
      "portable FOM did not provide two distinct interaction classes and parameters");

  publisher.rtiAmbassador().publishInteractionClass(firstInteraction);
  publisher.rtiAmbassador().publishInteractionClass(secondInteraction);
  subscriber.rtiAmbassador().subscribeInteractionClass(
      subscriberFirstInteraction,
      true);
  subscriber.rtiAmbassador().subscribeInteractionClass(
      subscriberSecondInteraction,
      true);

  auto sendInteraction = [&](rti::InteractionClassHandle interaction,
                             rti::ParameterHandle parameter,
                             std::uint8_t payload) {
    std::vector<std::uint8_t> const value{payload};
    std::vector<std::uint8_t> const tagBytes{0x55U, payload};
    rti::ParameterHandleValueMap values;
    values.emplace(
        parameter,
        rti::VariableLengthData(value.data(), value.size()));
    rti::VariableLengthData tag(tagBytes.data(), tagBytes.size());
    publisher.rtiAmbassador().sendInteraction(interaction, values, tag);
  };

  sendInteraction(firstInteraction, firstParameter, 0xA1U);
  sendInteraction(secondInteraction, secondParameter, 0xB1U);
  waitFor(
      subscriber,
      [&] { return subscriber.recorder().interactions().size() == 2U; },
      options,
      "baseline delivery for both published interaction classes");
  auto const baselineInteractions = subscriber.recorder().interactions();
  require(
      baselineInteractions.size() == 2U &&
          std::any_of(
              baselineInteractions.begin(),
              baselineInteractions.end(),
              [&](InteractionRecord const& received) {
                return received.interaction == firstInteraction;
              }) &&
          std::any_of(
              baselineInteractions.begin(),
              baselineInteractions.end(),
              [&](InteractionRecord const& received) {
                return received.interaction == secondInteraction;
              }),
      "baseline sends did not deliver both interaction classes to the subscriber");

  subscriber.recorder().clearInteraction();
  publisher.rtiAmbassador().unpublishInteractionClass(firstInteraction);
  requireException(
      [&] { sendInteraction(firstInteraction, firstParameter, 0xA2U); },
      L"InteractionClassNotPublished",
      "sending an interaction after fully unpublishing its class");
  sendInteraction(secondInteraction, secondParameter, 0xB2U);
  waitFor(
      subscriber,
      [&] { return subscriber.recorder().interactions().size() == 1U; },
      options,
      "remaining published interaction delivery after unpublishing another class");
  auto const afterFirstUnpublish = subscriber.recorder().interactions();
  require(
      afterFirstUnpublish.size() == 1U &&
          afterFirstUnpublish.front().interaction == secondInteraction,
      "unpublished interaction was delivered or the other publication stopped working");

  subscriber.recorder().clearInteraction();
  publisher.rtiAmbassador().publishInteractionClass(firstInteraction);
  sendInteraction(firstInteraction, firstParameter, 0xA3U);
  waitFor(
      subscriber,
      [&] { return subscriber.recorder().interactions().size() == 1U; },
      options,
      "republished interaction positive control");
  require(
      subscriber.recorder().interactions().front().interaction == firstInteraction,
      "republished interaction class did not resume delivery");

  subscriber.recorder().clearInteraction();
  publisher.rtiAmbassador().unpublishInteractionClass(secondInteraction);
  requireException(
      [&] { sendInteraction(secondInteraction, secondParameter, 0xB3U); },
      L"InteractionClassNotPublished",
      "sending an interaction after unpublishing the remaining class");
  publisher.resign(rti::NO_ACTION);
  subscriber.resign(rti::NO_ACTION);
}

void scenarioUnsubscribeLastAttributeStopsDiscovery(
    Options const& options,
    rti::CallbackModel model) {
  Session owner(options, model, "owner");
  Session subscriber(options, model, "subscriber");
  auto const federation = federationName(
      options,
      "unsubscribe-last-attribute-stops-discovery");
  connectAndJoin(owner, subscriber, options, federation, options.fom);

  auto const ownerClass = owner.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const subscriberClass = subscriber.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const ownerAttribute = owner.rtiAmbassador().getAttributeHandle(
      ownerClass,
      options.attributeName);
  auto const subscriberAttribute = subscriber.rtiAmbassador().getAttributeHandle(
      subscriberClass,
      options.attributeName);
  require(
      ownerClass.isValid() && subscriberClass.isValid() &&
          ownerClass == subscriberClass && ownerAttribute.isValid() &&
          ownerAttribute == subscriberAttribute,
      "adapter FOM did not provide stable object-class and attribute handles");

  rti::AttributeHandleSet const ownerAttributes{ownerAttribute};
  rti::AttributeHandleSet const subscriberAttributes{subscriberAttribute};
  owner.rtiAmbassador().publishObjectClassAttributes(ownerClass, ownerAttributes);
  subscriber.rtiAmbassador().subscribeObjectClassAttributes(
      subscriberClass,
      subscriberAttributes);

  auto const initialObject = owner.rtiAmbassador().registerObjectInstance(ownerClass);
  require(
      initialObject.isValid(),
      "registration before unsubscription returned an invalid handle");
  waitFor(
      subscriber,
      [&] { return subscriber.recorder().hasDiscovery(initialObject); },
      options,
      "initial discovery before class-attribute unsubscription");
  subscriber.recorder().clearDiscovery();

  subscriber.rtiAmbassador().unsubscribeObjectClassAttributes(
      subscriberClass,
      subscriberAttributes);
  auto const laterObject = owner.rtiAmbassador().registerObjectInstance(ownerClass);
  require(
      laterObject.isValid() && laterObject != initialObject,
      "post-unsubscription registration returned an invalid or reused handle");

  auto const deadline = Clock::now() +
      std::chrono::milliseconds(options.timeoutMilliseconds);
  while (Clock::now() < deadline &&
         !subscriber.recorder().hasDiscovery(laterObject)) {
    owner.pump();
    subscriber.pump();
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
  require(
      !subscriber.recorder().hasDiscovery(laterObject),
      "the RTI delivered Discover Object Instance after no class attributes remained subscribed");

  subscriber.resign(rti::NO_ACTION);
  owner.resign(rti::DELETE_OBJECTS);
  owner.rtiAmbassador().destroyFederationExecution(federation);
}

void scenarioBaseSubscriptionRejectsDerivedOnlyAttribute(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.knownClassFom.empty(),
      "base-class attribute scope testing requires an adapter-supplied FOM");
  Session member(options, model, "member");
  auto const federation = federationName(options, "base-subscription-derived-attribute");
  member.connect();
  member.rtiAmbassador().createFederationExecution(
      federation,
      options.knownClassFom.wstring(),
      options.logicalTimeImplementationName);
  member.join(options.memberFederateName, options.federateType, federation);

  auto const baseClass = member.rtiAmbassador().getObjectClassHandle(
      options.knownClassObjectClassName);
  auto const derivedClass = member.rtiAmbassador().getObjectClassHandle(
      options.knownClassDerivedObjectClassName);
  auto const derivedAttribute = member.rtiAmbassador().getAttributeHandle(
      derivedClass,
      options.knownClassDerivedAttributeName);
  require(
      baseClass.isValid() && derivedClass.isValid() && baseClass != derivedClass &&
          derivedAttribute.isValid(),
      "adapter FOM did not provide the configured base/derived class and attribute handles");

  rti::AttributeHandleSet const derivedAttributes{derivedAttribute};
  requireException(
      [&] {
        member.rtiAmbassador().subscribeObjectClassAttributes(
            baseClass,
            derivedAttributes);
      },
      L"AttributeNotDefined",
      "subscribing to a base class with an attribute defined only by its derived class");

  member.resign(rti::NO_ACTION);
  member.rtiAmbassador().destroyFederationExecution(federation);
  member.disconnect();
}

void scenarioBaseSubscriptionRejectsDerivedOnlyAttributeContract(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.knownClassFom.empty(),
      "base-class attribute scope testing requires an adapter-supplied FOM");
  Session member(options, model, "member");
  auto const federation = federationName(options, "base-subscription-derived-attribute-contract");
  member.connect();
  member.rtiAmbassador().createFederationExecution(
      federation,
      options.knownClassFom.wstring(),
      options.logicalTimeImplementationName);
  member.join(options.memberFederateName, options.federateType, federation);

  auto const baseClass = member.rtiAmbassador().getObjectClassHandle(
      options.knownClassObjectClassName);
  auto const derivedClass = member.rtiAmbassador().getObjectClassHandle(
      options.knownClassDerivedObjectClassName);
  auto const baseAttribute = member.rtiAmbassador().getAttributeHandle(
      baseClass,
      options.knownClassKnownAttributeName);
  auto const derivedAttribute = member.rtiAmbassador().getAttributeHandle(
      derivedClass,
      options.knownClassDerivedAttributeName);
  require(
      baseClass.isValid() && derivedClass.isValid() && baseClass != derivedClass &&
          baseAttribute.isValid() && derivedAttribute.isValid(),
      "adapter FOM did not provide the configured base/derived class and attribute handles");

  member.rtiAmbassador().subscribeObjectClassAttributes(
      baseClass,
      rti::AttributeHandleSet{baseAttribute});
  member.rtiAmbassador().unsubscribeObjectClassAttributes(
      baseClass,
      rti::AttributeHandleSet{baseAttribute});
  member.rtiAmbassador().subscribeObjectClassAttributes(
      derivedClass,
      rti::AttributeHandleSet{derivedAttribute});
  member.rtiAmbassador().unsubscribeObjectClassAttributes(
      derivedClass,
      rti::AttributeHandleSet{derivedAttribute});
  requireException(
      [&] {
        member.rtiAmbassador().subscribeObjectClassAttributes(
            baseClass,
            rti::AttributeHandleSet{derivedAttribute});
      },
      L"AttributeNotDefined",
      "rejecting a derived-only attribute while preserving valid base and derived subscriptions");

  member.resign(rti::NO_ACTION);
  member.rtiAmbassador().destroyFederationExecution(federation);
  member.disconnect();
}

void scenarioBasePublicationRejectsDerivedOnlyAttribute(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.knownClassFom.empty(),
      "base-class attribute scope testing requires an adapter-supplied FOM");
  Session member(options, model, "member");
  auto const federation = federationName(options, "base-publication-derived-attribute");
  member.connect();
  member.rtiAmbassador().createFederationExecution(
      federation,
      options.knownClassFom.wstring(),
      options.logicalTimeImplementationName);
  member.join(options.memberFederateName, options.federateType, federation);

  auto const baseClass = member.rtiAmbassador().getObjectClassHandle(
      options.knownClassObjectClassName);
  auto const derivedClass = member.rtiAmbassador().getObjectClassHandle(
      options.knownClassDerivedObjectClassName);
  auto const baseAttribute = member.rtiAmbassador().getAttributeHandle(
      baseClass,
      options.knownClassKnownAttributeName);
  auto const derivedAttribute = member.rtiAmbassador().getAttributeHandle(
      derivedClass,
      options.knownClassDerivedAttributeName);
  require(
      baseClass.isValid() && derivedClass.isValid() && baseClass != derivedClass &&
          baseAttribute.isValid() && derivedAttribute.isValid(),
      "adapter FOM did not provide the configured base/derived class and attribute handles");

  rti::AttributeHandleSet const baseAttributes{baseAttribute};
  rti::AttributeHandleSet const derivedAttributes{derivedAttribute};
  member.rtiAmbassador().publishObjectClassAttributes(baseClass, baseAttributes);
  member.rtiAmbassador().publishObjectClassAttributes(derivedClass, derivedAttributes);
  requireException(
      [&] {
        member.rtiAmbassador().publishObjectClassAttributes(
            baseClass,
            derivedAttributes);
      },
      L"AttributeNotDefined",
      "publishing a base class with an attribute defined only by its derived class");

  member.rtiAmbassador().unpublishObjectClassAttributes(baseClass, baseAttributes);
  member.rtiAmbassador().unpublishObjectClassAttributes(derivedClass, derivedAttributes);
  member.resign(rti::NO_ACTION);
  member.rtiAmbassador().destroyFederationExecution(federation);
  member.disconnect();
}

} // namespace hla_rti_cpp_tck::detail
