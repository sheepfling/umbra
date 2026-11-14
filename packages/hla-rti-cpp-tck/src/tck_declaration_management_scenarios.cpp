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

void scenarioInteractionActiveAndPassiveSubscriptionModes(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.fom.empty(),
      "Active/passive interaction-subscription testing requires an adapter-supplied FOM");

  Session publisher(options, model, "interaction-subscription-modes-publisher");
  Session activeSubscriber(options, model, "interaction-subscription-modes-active");
  Session passiveSubscriber(options, model, "interaction-subscription-modes-passive");
  auto const federation = federationName(
      options,
      "interaction-active-and-passive-subscription-modes");
  connectAndJoin(
      publisher,
      activeSubscriber,
      options,
      federation,
      options.fom);
  passiveSubscriber.connect();
  passiveSubscriber.join(
      options.memberFederateName + L"-passive-interaction-subscription",
      options.federateType,
      federation);

  auto const publisherInteraction =
      publisher.rtiAmbassador().getInteractionClassHandle(
          options.interactionClassName);
  auto const activeInteraction =
      activeSubscriber.rtiAmbassador().getInteractionClassHandle(
          options.interactionClassName);
  auto const passiveInteraction =
      passiveSubscriber.rtiAmbassador().getInteractionClassHandle(
          options.interactionClassName);
  auto const publisherParameter = publisher.rtiAmbassador().getParameterHandle(
      publisherInteraction,
      options.parameterName);
  auto const activeParameter = activeSubscriber.rtiAmbassador().getParameterHandle(
      activeInteraction,
      options.parameterName);
  auto const passiveParameter = passiveSubscriber.rtiAmbassador().getParameterHandle(
      passiveInteraction,
      options.parameterName);
  require(
      publisherInteraction.isValid() && activeInteraction.isValid() &&
          passiveInteraction.isValid() && publisherParameter.isValid() &&
          activeParameter.isValid() && passiveParameter.isValid(),
      "Active/passive interaction lookup returned an invalid standard handle");

  publisher.rtiAmbassador().publishInteractionClass(publisherInteraction);
  auto sendInteraction = [&](std::vector<std::uint8_t> const& value) {
    rti::ParameterHandleValueMap parameters;
    parameters.emplace(
        publisherParameter,
        rti::VariableLengthData(value.data(), value.size()));
    publisher.rtiAmbassador().sendInteraction(
        publisherInteraction,
        parameters,
        rti::VariableLengthData{});
  };

  std::vector<std::uint8_t> const beforeSubscription{0x4eU, 0x53U};
  sendInteraction(beforeSubscription);
  auto const noInitialSubscriptionDeadline =
      Clock::now() + std::chrono::milliseconds(options.timeoutMilliseconds);
  while (Clock::now() < noInitialSubscriptionDeadline) {
    publisher.pump();
    activeSubscriber.pump();
    passiveSubscriber.pump();
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
  require(
      activeSubscriber.recorder().interactions().empty() &&
          passiveSubscriber.recorder().interactions().empty(),
      "An interaction was delivered before either joined receiver declared its subscription");

  passiveSubscriber.rtiAmbassador().subscribeInteractionClass(
      passiveInteraction,
      false);
  for (int pass = 0; pass != 8; ++pass) {
    publisher.pump();
  }
  require(
      publisher.recorder().interactionsOn().empty(),
      "passive interaction subscription caused Turn Interactions On at another joined federate");

  std::vector<std::uint8_t> const passiveOnlyValue{0x50U, 0x4fU};
  sendInteraction(passiveOnlyValue);
  auto const passiveOnlyDeadline =
      Clock::now() + std::chrono::milliseconds(options.timeoutMilliseconds);
  while (Clock::now() < passiveOnlyDeadline) {
    publisher.pump();
    activeSubscriber.pump();
    passiveSubscriber.pump();
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
  require(
      activeSubscriber.recorder().interactions().empty() &&
          passiveSubscriber.recorder().interactions().empty(),
      "passive interaction subscription alone arranged delivery");

  activeSubscriber.rtiAmbassador().subscribeInteractionClass(
      activeInteraction,
      true);
  waitFor(
      publisher,
      [&] { return publisher.recorder().interactionsOn().size() >= 1U; },
      options,
      "active interaction subscription relevance advisory");
  auto const activeAdvisories = publisher.recorder().interactionsOn();
  require(
      activeAdvisories.size() == 1U &&
          activeAdvisories.front() == publisherInteraction,
      "active interaction subscription did not produce exactly one Turn Interactions On advisory");

  std::vector<std::uint8_t> const value{0x41U, 0x50U};
  sendInteraction(value);

  if (model == rti::HLA_EVOKED) {
    require(
        activeSubscriber.recorder().interactions().empty() &&
            passiveSubscriber.recorder().interactions().empty(),
        "Active/passive interaction callbacks arrived before evoked callback servicing");

    auto const activeCallbackDeadline =
        Clock::now() + std::chrono::milliseconds(options.timeoutMilliseconds);
    while (activeSubscriber.recorder().interactions().empty() &&
           Clock::now() < activeCallbackDeadline) {
      static_cast<void>(activeSubscriber.rtiAmbassador().evokeCallback(0.0));
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    require(
        activeSubscriber.recorder().interactions().size() == 1U,
        "EVOKED active interaction callback did not arrive after explicit Evoke Callback calls");

    auto const passiveCallbackDeadline =
        Clock::now() + std::chrono::milliseconds(options.timeoutMilliseconds);
    while (passiveSubscriber.recorder().interactions().empty() &&
           Clock::now() < passiveCallbackDeadline) {
      static_cast<void>(passiveSubscriber.rtiAmbassador().evokeCallback(0.0));
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    require(
        passiveSubscriber.recorder().interactions().size() == 1U,
        "EVOKED passive interaction callback did not arrive after explicit Evoke Callback calls");
  }
  waitFor(
      activeSubscriber,
      [&] { return activeSubscriber.recorder().interactions().size() >= 1U; },
      options,
      "active ordinary interaction delivery");
  auto assertDelivery = [&](Session& receiver,
                            rti::InteractionClassHandle const& expectedInteraction,
                            rti::ParameterHandle const& expectedParameter,
                            std::string const& description) {
    auto const received = receiver.recorder().interactions();
    require(
        received.size() == 1U && received.front().present &&
            received.front().interaction == expectedInteraction &&
            received.front().parameters.size() == 1U &&
            received.front().parameters.count(expectedParameter) == 1U &&
            copyBytes(received.front().parameters.at(expectedParameter)) == value,
        description + " received the wrong class, parameter, or parameter value");
  };
  assertDelivery(
      activeSubscriber,
      activeInteraction,
      activeParameter,
      "active interaction subscriber");
  waitFor(
      passiveSubscriber,
      [&] { return passiveSubscriber.recorder().interactions().size() >= 1U; },
      options,
      "passive interaction delivery under separate active interest");
  assertDelivery(
      passiveSubscriber,
      passiveInteraction,
      passiveParameter,
      "passive subscriber with active interest elsewhere");
  require(
      publisher.recorder().interactions().empty(),
      "Ordinary interaction callback looped back to its publisher");

  passiveSubscriber.rtiAmbassador().unsubscribeInteractionClass(
      passiveInteraction);
  activeSubscriber.rtiAmbassador().unsubscribeInteractionClass(activeInteraction);
  passiveSubscriber.resign(rti::NO_ACTION);
  activeSubscriber.resign(rti::NO_ACTION);
  publisher.rtiAmbassador().unpublishInteractionClass(publisherInteraction);
  publisher.resign(rti::NO_ACTION);
  publisher.rtiAmbassador().destroyFederationExecution(federation);
  passiveSubscriber.disconnect();
  activeSubscriber.disconnect();
  publisher.disconnect();
}

void scenarioInteractionActiveAndPassiveSubscriptionModesContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioInteractionActiveAndPassiveSubscriptionModes(options, model);
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

void scenarioDeclarationPublicationGate(
    Options const& options,
    rti::CallbackModel model) {
  Session publisher(options, model, "publisher");
  Session subscriber(options, model, "subscriber");
  auto const federation = federationName(
      options,
      "declaration-publication-gate");
  connectAndJoin(publisher, subscriber, options, federation, options.fom);

  auto const interaction = publisher.rtiAmbassador().getInteractionClassHandle(
      options.interactionClassName);
  auto const subscriberInteraction =
      subscriber.rtiAmbassador().getInteractionClassHandle(
          options.interactionClassName);
  auto const parameter = publisher.rtiAmbassador().getParameterHandle(
      interaction,
      options.parameterName);
  auto const subscriberParameter =
      subscriber.rtiAmbassador().getParameterHandle(
          subscriberInteraction,
          options.parameterName);
  require(
      interaction.isValid() && interaction == subscriberInteraction &&
          parameter.isValid() && parameter == subscriberParameter,
      "portable FOM did not provide matching interaction handles");

  subscriber.rtiAmbassador().subscribeInteractionClass(
      subscriberInteraction,
      true);

  std::vector<std::uint8_t> const payload{0x51U};
  std::vector<std::uint8_t> const tagBytes{0xA5U, 0x5AU};
  rti::ParameterHandleValueMap values;
  values.emplace(
      parameter,
      rti::VariableLengthData(payload.data(), payload.size()));
  rti::VariableLengthData const tag(tagBytes.data(), tagBytes.size());

  requireException(
      [&] {
        publisher.rtiAmbassador().sendInteraction(
            interaction,
            values,
            tag);
      },
      L"InteractionClassNotPublished",
      "sending before declaring the interaction class");
  require(
      subscriber.recorder().interactions().empty(),
      "an interaction sent before publication produced a receive callback");

  publisher.rtiAmbassador().publishInteractionClass(interaction);
  publisher.rtiAmbassador().sendInteraction(interaction, values, tag);
  waitFor(
      subscriber,
      [&] { return subscriber.recorder().interactions().size() == 1U; },
      options,
      "interaction delivery after declaring the class");

  auto const received = subscriber.recorder().interactions();
  require(
      received.size() == 1U &&
          received.front().interaction == interaction &&
          received.front().tag == tagBytes &&
          received.front().parameters.size() == 1U &&
          received.front().parameters.contains(parameter),
      "published interaction did not produce the expected single callback");
  auto const& receivedPayload = received.front().parameters.at(parameter);
  require(
      receivedPayload.size() == payload.size() &&
          static_cast<std::uint8_t const*>(receivedPayload.data())[0] ==
              payload.front(),
      "published interaction callback did not preserve its parameter value");

  publisher.resign(rti::NO_ACTION);
  subscriber.resign(rti::NO_ACTION);
}

void scenarioDeclarationObjectRegistrationGate(
    Options const& options,
    rti::CallbackModel model) {
  Session registrar(options, model, "registrar");
  Session member(options, model, "member");
  auto const federation = federationName(
      options,
      "declaration-object-registration-gate");
  connectAndJoin(registrar, member, options, federation, options.fom);

  auto const objectClass = registrar.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const attribute = registrar.rtiAmbassador().getAttributeHandle(
      objectClass,
      options.attributeName);
  require(
      objectClass.isValid() && attribute.isValid(),
      "portable FOM did not provide the object publication handles");

  std::wstring const objectName =
      options.ownerFederateName + L"-publication-gate-object";
  registrar.rtiAmbassador().reserveObjectInstanceName(objectName);
  requireException(
      [&] {
        registrar.rtiAmbassador().registerObjectInstance(
            objectClass,
            objectName);
      },
      L"ObjectClassNotPublished",
      "registering an object before declaring its class attributes");

  rti::AttributeHandleSet attributes;
  attributes.insert(attribute);
  registrar.rtiAmbassador().publishObjectClassAttributes(
      objectClass,
      attributes);
  auto const registered = registrar.rtiAmbassador().registerObjectInstance(
      objectClass,
      objectName);
  require(
      registered.isValid(),
      "object registration did not succeed after class publication");

  registrar.resign(rti::DELETE_OBJECTS);
  member.resign(rti::NO_ACTION);
}

void scenarioDeclarationDirectedPublicationGate(
    Options const& options,
    rti::CallbackModel model) {
  Session publisher(options, model, "publisher");
  Session receiver(options, model, "receiver");
  auto const federation = federationName(
      options,
      "declaration-directed-publication-gate");
  connectAndJoin(publisher, receiver, options, federation, options.fom);

  auto const publisherClass = publisher.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const receiverClass = receiver.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const publisherAttribute = publisher.rtiAmbassador().getAttributeHandle(
      publisherClass,
      options.attributeName);
  auto const receiverAttribute = receiver.rtiAmbassador().getAttributeHandle(
      receiverClass,
      options.attributeName);
  auto const publisherInteraction =
      publisher.rtiAmbassador().getInteractionClassHandle(
          options.interactionClassName);
  auto const receiverInteraction =
      receiver.rtiAmbassador().getInteractionClassHandle(
          options.interactionClassName);
  auto const publisherParameter = publisher.rtiAmbassador().getParameterHandle(
      publisherInteraction,
      options.parameterName);
  auto const receiverParameter = receiver.rtiAmbassador().getParameterHandle(
      receiverInteraction,
      options.parameterName);
  require(
      publisherClass.isValid() && publisherClass == receiverClass &&
          publisherAttribute.isValid() && publisherAttribute == receiverAttribute &&
          publisherInteraction.isValid() &&
          publisherInteraction == receiverInteraction &&
          publisherParameter.isValid() &&
          publisherParameter == receiverParameter,
      "portable FOM did not provide matching directed interaction handles");

  rti::AttributeHandleSet publisherAttributes;
  publisherAttributes.insert(publisherAttribute);
  rti::AttributeHandleSet receiverAttributes;
  receiverAttributes.insert(receiverAttribute);
  publisher.rtiAmbassador().publishObjectClassAttributes(
      publisherClass,
      publisherAttributes);
  receiver.rtiAmbassador().publishObjectClassAttributes(
      receiverClass,
      receiverAttributes);
  publisher.rtiAmbassador().subscribeObjectClassAttributes(
      publisherClass,
      publisherAttributes,
      true,
      L"");
  receiver.rtiAmbassador().subscribeObjectClassAttributes(
      receiverClass,
      receiverAttributes,
      true,
      L"");

  rti::InteractionClassHandleSet directedInteractions;
  directedInteractions.insert(publisherInteraction);
  rti::InteractionClassHandleSet receiverDirectedInteractions;
  receiverDirectedInteractions.insert(receiverInteraction);
  receiver.rtiAmbassador().subscribeObjectClassDirectedInteractions(
      receiverClass,
      receiverDirectedInteractions,
      false);

  auto const target = receiver.rtiAmbassador().registerObjectInstance(
      receiverClass);
  require(target.isValid(), "directed interaction target registration failed");
  waitFor(
      publisher,
      [&] { return publisher.recorder().hasDiscovery(target); },
      options,
      "directed publication gate target discovery");
  require(
      publisher.rtiAmbassador().getKnownObjectClassHandle(target) ==
          publisherClass,
      "directed publication gate discovered the wrong target class");

  std::vector<std::uint8_t> const payload{0xD1U, 0x72U};
  std::vector<std::uint8_t> const tagBytes{0x50U, 0x55U, 0x42U};
  rti::ParameterHandleValueMap parameters;
  parameters.emplace(
      publisherParameter,
      rti::VariableLengthData(payload.data(), payload.size()));
  rti::VariableLengthData const tag(tagBytes.data(), tagBytes.size());
  auto send = [&] {
    publisher.rtiAmbassador().sendDirectedInteraction(
        publisherInteraction,
        target,
        parameters,
        tag);
  };

  requireException(
      send,
      L"InteractionClassNotPublished",
      "sending a directed interaction before its class is published");
  rti::InteractionClassHandleSet emptyDirectedInteractions;
  publisher.rtiAmbassador().publishObjectClassDirectedInteractions(
      publisherClass,
      emptyDirectedInteractions);
  requireException(
      send,
      L"InteractionClassNotPublished",
      "an empty directed-interaction publication must not authorize sending");
  for (int pass = 0; pass != 8; ++pass) {
    receiver.pump();
  }
  require(
      receiver.recorder().directedInteractions().empty(),
      "directed interaction sent before publication produced a callback");

  publisher.rtiAmbassador().publishObjectClassDirectedInteractions(
      publisherClass,
      directedInteractions);
  send();
  waitFor(
      receiver,
      [&] { return receiver.recorder().directedInteractions().size() == 1U; },
      options,
      "directed interaction delivery after publication");
  auto const received = receiver.recorder().directedInteractions();
  require(
      received.size() == 1U &&
          received.front().interaction == receiverInteraction &&
          received.front().object == target &&
          received.front().producer == publisher.federateHandle() &&
          received.front().tag == tagBytes &&
          received.front().parameters.size() == 1U &&
          received.front().parameters.contains(receiverParameter),
      "published directed interaction did not preserve its callback identity");
  auto const& receivedPayload =
      received.front().parameters.at(receiverParameter);
  require(
      receivedPayload.size() == payload.size() &&
          static_cast<std::uint8_t const*>(receivedPayload.data())[0] ==
              payload[0] &&
          static_cast<std::uint8_t const*>(receivedPayload.data())[1] ==
              payload[1],
      "published directed interaction did not preserve its parameter value");

  publisher.rtiAmbassador().unpublishObjectClassDirectedInteractions(
      publisherClass,
      emptyDirectedInteractions);
  send();
  waitFor(
      receiver,
      [&] { return receiver.recorder().directedInteractions().size() == 2U; },
      options,
      "empty unpublication preserves the directed interaction publication");
  require(
      receiver.recorder().directedInteractions().size() == 2U,
      "empty directed-interaction unpublication removed a publication");

  publisher.rtiAmbassador().unpublishObjectClassDirectedInteractions(
      publisherClass,
      directedInteractions);
  requireException(
      send,
      L"InteractionClassNotPublished",
      "sending a directed interaction after its class is unpublished");
  for (int pass = 0; pass != 8; ++pass) {
    receiver.pump();
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
  require(
      receiver.recorder().directedInteractions().size() == 2U,
      "directed interaction sent after unpublication produced a callback");

  publisher.rtiAmbassador().publishObjectClassDirectedInteractions(
      publisherClass,
      directedInteractions);
  send();
  waitFor(
      receiver,
      [&] { return receiver.recorder().directedInteractions().size() == 3U; },
      options,
      "directed interaction delivery after republishing");
  publisher.rtiAmbassador().unpublishObjectClassDirectedInteractions(
      publisherClass);
  requireException(
      send,
      L"InteractionClassNotPublished",
      "sending a directed interaction after whole-class unpublication");
  for (int pass = 0; pass != 8; ++pass) {
    receiver.pump();
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
  require(
      receiver.recorder().directedInteractions().size() == 3U,
      "directed interaction sent after whole-class unpublication produced a callback");

  publisher.resign(rti::NO_ACTION);
  receiver.resign(rti::DELETE_OBJECTS);
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

void scenarioAttributeUnsubscribeStopsReflection(
    Options const& options, rti::CallbackModel model) {
  require(!options.fom.empty(),
          "Attribute unsubscription requires an adapter-supplied FOM");
  Session publisher(options, model, "owner");
  Session subscriber(options, model, "subscriber");
  auto const federation = federationName(
      options, "attribute-unsubscribe-stops-reflection");
  connectAndJoin(publisher, subscriber, options, federation, options.fom);

  auto& publisherRti = publisher.rtiAmbassador();
  auto& subscriberRti = subscriber.rtiAmbassador();
  auto const publisherClass = publisherRti.getObjectClassHandle(options.objectClassName);
  auto const subscriberClass = subscriberRti.getObjectClassHandle(options.objectClassName);
  auto const publisherAttribute =
      publisherRti.getAttributeHandle(publisherClass, options.attributeName);
  auto const subscriberAttribute =
      subscriberRti.getAttributeHandle(subscriberClass, options.attributeName);
  require(publisherClass.isValid() && subscriberClass.isValid() &&
              publisherAttribute.isValid() && subscriberAttribute.isValid(),
          "Adapter FOM lacks the object class or attribute for unsubscription");
  rti::AttributeHandleSet const published{publisherAttribute};
  rti::AttributeHandleSet const subscribed{subscriberAttribute};
  publisherRti.publishObjectClassAttributes(publisherClass, published);
  subscriberRti.subscribeObjectClassAttributes(subscriberClass, subscribed);
  auto const object = publisherRti.registerObjectInstance(publisherClass);
  require(object.isValid(), "Attribute unsubscription registered no object");
  waitFor(subscriber,
          [&] { return subscriber.recorder().hasDiscovery(object); }, options,
          "object discovery before attribute unsubscription");

  auto send = [&](std::vector<std::uint8_t> const& value) {
    rti::AttributeHandleValueMap const values{{
        publisherAttribute,
        rti::VariableLengthData(value.data(), value.size()),
    }};
    rti::VariableLengthData const tag;
    publisherRti.updateAttributeValues(object, values, tag);
  };
  std::vector<std::uint8_t> const firstValue{0x11U, 0x22U};
  send(firstValue);
  waitFor(subscriber,
          [&] { return subscriber.recorder().reflections().size() == 1U; },
          options, "initial subscribed attribute reflection");
  auto const initial = subscriber.recorder().reflections();
  require(initial.size() == 1U && initial.front().object == object &&
              initial.front().values.size() == 1U &&
              initial.front().values.count(subscriberAttribute) == 1U &&
              copyBytes(initial.front().values.at(subscriberAttribute)) == firstValue,
          "Initial active subscription did not reflect the expected attribute");

  subscriber.recorder().clearReflection();
  subscriberRti.unsubscribeObjectClassAttributes(subscriberClass, subscribed);
  send(std::vector<std::uint8_t>{0x33U, 0x44U});
  auto const deadline = Clock::now() +
      std::chrono::milliseconds(options.timeoutMilliseconds);
  while (Clock::now() < deadline) {
    publisher.pump();
    subscriber.pump();
    if (!subscriber.recorder().reflections().empty()) break;
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
  require(subscriber.recorder().reflections().empty(),
          "An attribute update sent after unsubscription was reflected");

  publisherRti.unpublishObjectClassAttributes(publisherClass, published);
  subscriber.resign(rti::NO_ACTION);
  publisher.resign(rti::DELETE_OBJECTS);
  publisherRti.destroyFederationExecution(federation);
}

void scenarioAttributeUnsubscribeStopsReflectionContract(
    Options const& options, rti::CallbackModel model) {
  scenarioAttributeUnsubscribeStopsReflection(options, model);
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
  auto const baseAttribute = member.rtiAmbassador().getAttributeHandle(
      baseClass,
      options.knownClassKnownAttributeName);
  require(
      baseClass.isValid() && derivedClass.isValid() && baseClass != derivedClass &&
          baseAttribute.isValid() && derivedAttribute.isValid(),
      "adapter FOM did not provide the configured base/derived class and attribute handles");

  rti::AttributeHandleSet const baseAttributes{baseAttribute};
  rti::AttributeHandleSet const derivedAttributes{derivedAttribute};
  member.rtiAmbassador().subscribeObjectClassAttributes(
      baseClass,
      baseAttributes);
  member.rtiAmbassador().subscribeObjectClassAttributes(
      derivedClass,
      derivedAttributes);
  requireException(
      [&] {
        member.rtiAmbassador().subscribeObjectClassAttributes(
            baseClass,
            derivedAttributes);
      },
      L"AttributeNotDefined",
      "subscribing to a base class with an attribute defined only by its derived class");

  member.rtiAmbassador().unsubscribeObjectClassAttributes(
      baseClass,
      baseAttributes);
  member.rtiAmbassador().unsubscribeObjectClassAttributes(
      derivedClass,
      derivedAttributes);
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

void scenarioDdmAvailableAttributeDeclarationServices(
    Options const& options,
    rti::CallbackModel model) {
  require(!options.ddmFom.empty(),
          "DDM available-attribute declarations require an adapter-supplied DDM FOM");
  require(options.ddmDimensionNames.size() >= 2U,
          "DDM available-attribute declarations require two dimension names");

  Session member(options, model, "member");
  auto const federation = federationName(options, "ddm-available-attribute-services");
  member.connect();
  member.rtiAmbassador().createFederationExecution(
      federation,
      options.ddmFom.wstring(),
      options.logicalTimeImplementationName);
  member.join(options.memberFederateName, options.federateType, federation);

  auto const handles = ddmHandles(member, options);
  auto& rtiAmbassador = member.rtiAmbassador();
  auto const baseClass =
      rtiAmbassador.getObjectClassHandle(options.knownClassObjectClassName);
  auto const derivedClass = rtiAmbassador.getObjectClassHandle(
      options.knownClassDerivedObjectClassName);
  auto const baseAttribute = rtiAmbassador.getAttributeHandle(
      baseClass, options.knownClassKnownAttributeName);
  auto const derivedAttribute = rtiAmbassador.getAttributeHandle(
      derivedClass, options.knownClassDerivedAttributeName);
  require(handles.firstDimension.isValid() && handles.secondDimension.isValid() &&
              baseClass.isValid() && baseAttribute.isValid() &&
              derivedClass.isValid() && derivedAttribute.isValid() &&
              baseClass != derivedClass,
          "adapter DDM FOM lacks the configured base/derived object-class attributes");

  rti::AttributeHandleSet const baseAttributes{baseAttribute};
  rti::AttributeHandleSet const derivedAttributes{derivedAttribute};
  auto const region = createDdmRegion(member, handles, 0UL, 2UL, 0UL, 2UL);
  rti::AttributeHandleSetRegionHandleSetPairVector const baseAttributeRegion{{
      baseAttributes, rti::RegionHandleSet{region}}};
  rti::AttributeHandleSetRegionHandleSetPairVector const derivedAttributeRegion{{
      derivedAttributes, rti::RegionHandleSet{region}}};

  // IEEE 1516.1-2025 §9.1.5 requires these attribute sets to be available on
  // the named object class. AttributeNotDefined is asserted as C++ API
  // behavior, not as an exception type prescribed by the requirement.
  rtiAmbassador.publishObjectClassAttributes(baseClass, baseAttributes);
  requireException(
      [&] {
        rtiAmbassador.publishObjectClassAttributes(
            baseClass, derivedAttributes);
      },
      L"AttributeNotDefined",
      "publishing a derived-only attribute on the DDM base class");
  requireException(
      [&] {
        rtiAmbassador.unpublishObjectClassAttributes(
            baseClass, derivedAttributes);
      },
      L"AttributeNotDefined",
      "unpublishing a derived-only attribute on the DDM base class");
  rtiAmbassador.publishObjectClassAttributes(derivedClass, derivedAttributes);
  rtiAmbassador.unpublishObjectClassAttributes(derivedClass, derivedAttributes);
  rtiAmbassador.unpublishObjectClassAttributes(derivedClass, baseAttributes);
  rtiAmbassador.unpublishObjectClassAttributes(baseClass, baseAttributes);

  rtiAmbassador.subscribeObjectClassAttributes(baseClass, baseAttributes);
  requireException(
      [&] {
        rtiAmbassador.subscribeObjectClassAttributes(
            baseClass, derivedAttributes);
      },
      L"AttributeNotDefined",
      "subscribing to a derived-only attribute on the DDM base class");
  requireException(
      [&] {
        rtiAmbassador.unsubscribeObjectClassAttributes(
            baseClass, derivedAttributes);
      },
      L"AttributeNotDefined",
      "unsubscribing a derived-only attribute on the DDM base class");
  rtiAmbassador.subscribeObjectClassAttributes(derivedClass, baseAttributes);
  rtiAmbassador.subscribeObjectClassAttributes(derivedClass, derivedAttributes);
  rtiAmbassador.unsubscribeObjectClassAttributes(derivedClass, derivedAttributes);
  rtiAmbassador.unsubscribeObjectClassAttributes(derivedClass, baseAttributes);
  rtiAmbassador.unsubscribeObjectClassAttributes(baseClass, baseAttributes);

  requireException(
      [&] {
        rtiAmbassador.subscribeObjectClassAttributesWithRegions(
            baseClass, derivedAttributeRegion);
      },
      L"AttributeNotDefined",
      "subscribing with regions to a derived-only attribute on the DDM base class");
  requireException(
      [&] {
        rtiAmbassador.unsubscribeObjectClassAttributesWithRegions(
            baseClass, derivedAttributeRegion);
      },
      L"AttributeNotDefined",
      "unsubscribing with regions a derived-only attribute on the DDM base class");
  rtiAmbassador.subscribeObjectClassAttributesWithRegions(
      baseClass, baseAttributeRegion);
  rtiAmbassador.unsubscribeObjectClassAttributesWithRegions(
      baseClass, baseAttributeRegion);
  rtiAmbassador.subscribeObjectClassAttributesWithRegions(
      derivedClass, derivedAttributeRegion);
  rtiAmbassador.unsubscribeObjectClassAttributesWithRegions(
      derivedClass, derivedAttributeRegion);

  rtiAmbassador.deleteRegion(region);
  member.resign(rti::NO_ACTION);
  rtiAmbassador.destroyFederationExecution(federation);
  member.disconnect();
}

void scenarioDdmAvailableAttributeDeclarationServicesContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioDdmAvailableAttributeDeclarationServices(options, model);
}

void scenarioStartRegistrationAdvisorySwitchGating(
    Options const& options,
    rti::CallbackModel model) {
  require(!options.ddmFom.empty(),
          "Start-registration switch testing requires an adapter-supplied DDM FOM");

  Session publisher(options, model, "start-registration-publisher");
  Session subscriber(options, model, "start-registration-subscriber");
  auto const federation =
      federationName(options, "start-registration-advisory-switch-gating");
  connectAndJoin(publisher, subscriber, options, federation, options.ddmFom);

  auto const publisherHandles = ddmHandles(publisher, options);
  auto const subscriberHandles = ddmHandles(subscriber, options);
  verifyDdmClassDimensions(
      publisher,
      options,
      publisherHandles,
      "start-registration advisory publisher");
  verifyDdmClassDimensions(
      subscriber,
      options,
      subscriberHandles,
      "start-registration advisory subscriber");

  publisher.rtiAmbassador().setObjectClassRelevanceAdvisorySwitch(false);
  require(
      !publisher.rtiAmbassador().getObjectClassRelevanceAdvisorySwitch(),
      "object-class relevance advisory switch did not disable");
  publisher.rtiAmbassador().publishObjectClassAttributes(
      publisherHandles.objectClass,
      rti::AttributeHandleSet{publisherHandles.attribute});

  auto const region = createDdmRegion(
      subscriber,
      subscriberHandles,
      0UL,
      2UL,
      0UL,
      2UL);
  rti::AttributeHandleSetRegionHandleSetPairVector const subscription{{
      rti::AttributeHandleSet{subscriberHandles.attribute},
      rti::RegionHandleSet{region},
  }};
  auto const observeForConfiguredWindow = [&] {
    auto const deadline =
        Clock::now() + std::chrono::milliseconds(options.timeoutMilliseconds);
    while (Clock::now() < deadline) {
      publisher.pump();
      subscriber.pump();
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
  };

  publisher.recorder().clearAdvisories();
  subscriber.rtiAmbassador().subscribeObjectClassAttributesWithRegions(
      subscriberHandles.objectClass,
      subscription,
      true);
  observeForConfiguredWindow();
  require(
      publisher.recorder().registrationStarts().empty(),
      "Start Registration was invoked while the publisher's object-class relevance advisory switch was disabled");

  publisher.rtiAmbassador().setObjectClassRelevanceAdvisorySwitch(true);
  require(
      publisher.rtiAmbassador().getObjectClassRelevanceAdvisorySwitch(),
      "object-class relevance advisory switch did not enable while the subscription remained active");
  observeForConfiguredWindow();
  require(
      publisher.recorder().registrationStarts().empty(),
      "Start Registration suppressed while the object-class relevance advisory switch was disabled was replayed after re-enabling");

  subscriber.rtiAmbassador().unsubscribeObjectClassAttributesWithRegions(
      subscriberHandles.objectClass,
      subscription);
  publisher.recorder().clearAdvisories();
  subscriber.rtiAmbassador().subscribeObjectClassAttributesWithRegions(
      subscriberHandles.objectClass,
      subscription,
      true);
  waitFor(
      publisher,
      [&] { return publisher.recorder().registrationStarts().size() >= 1U; },
      options,
      "Start Registration after enabling object-class relevance advisories");
  observeForConfiguredWindow();
  auto const registrationStarts = publisher.recorder().registrationStarts();
  require(
      registrationStarts.size() == 1U &&
          registrationStarts.front() == publisherHandles.objectClass,
      "enabled switch did not produce exactly one Start Registration advisory for the published class");

  publisher.recorder().clearAdvisories();
  publisher.rtiAmbassador().setObjectClassRelevanceAdvisorySwitch(false);
  require(
      !publisher.rtiAmbassador().getObjectClassRelevanceAdvisorySwitch(),
      "object-class relevance advisory switch did not disable before Stop Registration");
  subscriber.rtiAmbassador().unsubscribeObjectClassAttributesWithRegions(
      subscriberHandles.objectClass,
      subscription);
  observeForConfiguredWindow();
  require(
      publisher.recorder().registrationStops().empty(),
      "Stop Registration was invoked while the publisher's object-class relevance advisory switch was disabled");

  publisher.rtiAmbassador().setObjectClassRelevanceAdvisorySwitch(true);
  require(
      publisher.rtiAmbassador().getObjectClassRelevanceAdvisorySwitch(),
      "object-class relevance advisory switch did not re-enable before the positive Stop Registration control");
  publisher.recorder().clearAdvisories();
  subscriber.rtiAmbassador().subscribeObjectClassAttributesWithRegions(
      subscriberHandles.objectClass,
      subscription,
      true);
  waitFor(
      publisher,
      [&] { return publisher.recorder().registrationStarts().size() >= 1U; },
      options,
      "Start Registration after re-enabling object-class relevance advisories");
  require(
      publisher.recorder().registrationStarts().size() == 1U &&
          publisher.recorder().registrationStarts().front() ==
              publisherHandles.objectClass,
      "re-enabled switch did not produce exactly one Start Registration advisory for the published class");

  subscriber.rtiAmbassador().unsubscribeObjectClassAttributesWithRegions(
      subscriberHandles.objectClass,
      subscription);
  waitFor(
      publisher,
      [&] { return publisher.recorder().registrationStops().size() >= 1U; },
      options,
      "Stop Registration after re-enabling object-class relevance advisories");
  require(
      publisher.recorder().registrationStops().size() == 1U &&
          publisher.recorder().registrationStops().front() ==
              publisherHandles.objectClass,
      "re-enabled switch did not produce exactly one Stop Registration advisory for the published class");

  publisher.rtiAmbassador().unpublishObjectClassAttributes(
      publisherHandles.objectClass,
      rti::AttributeHandleSet{publisherHandles.attribute});
  publisher.rtiAmbassador().unpublishObjectClass(publisherHandles.objectClass);
  subscriber.rtiAmbassador().deleteRegion(region);
}

void scenarioDdmUnsubscribePreservesOtherClassSubscription(
    Options const& options,
    rti::CallbackModel model) {
  require(!options.ddmFom.empty(),
          "DDM unsubscribe isolation requires an adapter-supplied DDM FOM");
  require(options.ddmDimensionNames.size() >= 2U,
          "DDM unsubscribe isolation requires two dimension names");

  Session publisher(options, model, "publisher");
  Session subscriber(options, model, "subscriber");
  auto const federation = federationName(
      options, "ddm-unsubscribe-preserves-other-class-subscription");
  publisher.connect();
  publisher.rtiAmbassador().createFederationExecution(
      federation,
      options.ddmFom.wstring(),
      options.logicalTimeImplementationName);
  publisher.join(options.ownerFederateName, options.federateType, federation);
  subscriber.connect();
  subscriber.join(options.memberFederateName, options.federateType, federation);

  auto const publisherHandles = ddmHandles(publisher, options);
  auto const subscriberHandles = ddmHandles(subscriber, options);
  auto const publisherRegionalClass = publisher.rtiAmbassador().getObjectClassHandle(
      options.knownClassObjectClassName);
  auto const subscriberRegionalClass = subscriber.rtiAmbassador().getObjectClassHandle(
      options.knownClassObjectClassName);
  auto const publisherRegionalAttribute = publisher.rtiAmbassador().getAttributeHandle(
      publisherRegionalClass, options.knownClassKnownAttributeName);
  auto const subscriberRegionalAttribute = subscriber.rtiAmbassador().getAttributeHandle(
      subscriberRegionalClass, options.knownClassKnownAttributeName);
  require(publisherHandles.objectClass.isValid() &&
              subscriberHandles.objectClass.isValid() &&
              publisherHandles.attribute.isValid() &&
              subscriberHandles.attribute.isValid() &&
              publisherHandles.objectClass == subscriberHandles.objectClass &&
              publisherHandles.attribute == subscriberHandles.attribute &&
              publisherRegionalClass.isValid() && subscriberRegionalClass.isValid() &&
              publisherRegionalAttribute.isValid() &&
              subscriberRegionalAttribute.isValid() &&
              publisherRegionalClass == subscriberRegionalClass &&
              publisherRegionalAttribute == subscriberRegionalAttribute,
          "adapter DDM FOM lacks the two configured class attributes");

  auto const firstDimensionUpperBound = publisher.rtiAmbassador().getDimensionUpperBound(
      publisherHandles.firstDimension);
  auto const secondDimensionUpperBound = publisher.rtiAmbassador().getDimensionUpperBound(
      publisherHandles.secondDimension);
  require(firstDimensionUpperBound >= 2UL && secondDimensionUpperBound >= 2UL &&
              subscriber.rtiAmbassador().getDimensionUpperBound(
                  subscriberHandles.firstDimension) == firstDimensionUpperBound &&
              subscriber.rtiAmbassador().getDimensionUpperBound(
                  subscriberHandles.secondDimension) == secondDimensionUpperBound,
          "adapter DDM dimensions do not provide matching positive ranges");

  rti::AttributeHandleSet const ordinaryAttributes{publisherHandles.attribute};
  rti::AttributeHandleSet const regionalPublisherAttributes{publisherRegionalAttribute};
  rti::AttributeHandleSet const regionalSubscriberAttributes{subscriberRegionalAttribute};
  publisher.rtiAmbassador().publishObjectClassAttributes(
      publisherHandles.objectClass, ordinaryAttributes);
  publisher.rtiAmbassador().publishObjectClassAttributes(
      publisherRegionalClass, regionalPublisherAttributes);

  auto regionalPublisherHandles = publisherHandles;
  regionalPublisherHandles.objectClass = publisherRegionalClass;
  regionalPublisherHandles.attribute = publisherRegionalAttribute;
  auto regionalSubscriberHandles = subscriberHandles;
  regionalSubscriberHandles.objectClass = subscriberRegionalClass;
  regionalSubscriberHandles.attribute = subscriberRegionalAttribute;
  auto const sourceRegion = createDdmRegion(
      publisher, regionalPublisherHandles,
      0UL, firstDimensionUpperBound - 1UL,
      0UL, secondDimensionUpperBound - 1UL);
  auto const subscriptionRegion = createDdmRegion(
      subscriber, regionalSubscriberHandles,
      0UL, firstDimensionUpperBound - 1UL,
      0UL, secondDimensionUpperBound - 1UL);
  rti::AttributeHandleSetRegionHandleSetPairVector const sourcePair{{
      regionalPublisherAttributes, rti::RegionHandleSet{sourceRegion}}};
  rti::AttributeHandleSetRegionHandleSetPairVector const subscriptionPair{{
      regionalSubscriberAttributes, rti::RegionHandleSet{subscriptionRegion}}};

  auto const ordinaryObject = publisher.rtiAmbassador().registerObjectInstance(
      publisherHandles.objectClass);
  auto const regionalObject = publisher.rtiAmbassador().registerObjectInstanceWithRegions(
      publisherRegionalClass, sourcePair);
  require(ordinaryObject.isValid() && regionalObject.isValid() &&
              ordinaryObject != regionalObject,
          "DDM unsubscribe isolation registered invalid or aliased object instances");

  auto send = [&](rti::ObjectInstanceHandle const& object,
                  rti::AttributeHandle const& attribute,
                  std::vector<std::uint8_t> const& value) {
    rti::AttributeHandleValueMap const values{{
        attribute, rti::VariableLengthData(value.data(), value.size())}};
    rti::VariableLengthData const tag;
    publisher.rtiAmbassador().updateAttributeValues(object, values, tag);
  };
  std::vector<std::uint8_t> const ordinaryBeforeSubscription{0x01U};
  std::vector<std::uint8_t> const regionalBeforeSubscription{0x02U};
  send(ordinaryObject, publisherHandles.attribute, ordinaryBeforeSubscription);
  send(regionalObject, publisherRegionalAttribute, regionalBeforeSubscription);
  auto const initialDeclarationObservationDeadline =
      Clock::now() + std::chrono::milliseconds(options.timeoutMilliseconds);
  while (Clock::now() < initialDeclarationObservationDeadline) {
    publisher.pump();
    subscriber.pump();
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
  require(
      !subscriber.recorder().hasDiscovery(ordinaryObject) &&
          !subscriber.recorder().hasDiscovery(regionalObject) &&
          subscriber.recorder().reflections().empty(),
      "object discovery or attribute reflection occurred before any DM/DDM attribute subscription");

  subscriber.rtiAmbassador().subscribeObjectClassAttributes(
      subscriberHandles.objectClass,
      rti::AttributeHandleSet{subscriberHandles.attribute});
  waitFor(
      subscriber,
      [&] { return subscriber.recorder().hasDiscovery(ordinaryObject); },
      options,
      "ordinary discovery after subscribing to an ordinary class attribute");
  auto const ordinaryDiscovery = subscriber.recorder().discovery();
  require(ordinaryDiscovery.object == ordinaryObject &&
              ordinaryDiscovery.objectClass == publisherHandles.objectClass,
          "ordinary registration did not discover with the registered class argument");

  subscriber.rtiAmbassador().subscribeObjectClassAttributesWithRegions(
      subscriberRegionalClass, subscriptionPair, true, L"");
  waitFor(
      subscriber,
      [&] { return subscriber.recorder().hasDiscovery(regionalObject); },
      options,
      "regional discovery after subscribing to a regional class attribute");
  auto const regionalDiscovery = subscriber.recorder().discovery();
  require(regionalDiscovery.object == regionalObject &&
              regionalDiscovery.objectClass == publisherRegionalClass,
          "regional registration did not discover with the registered class argument");

  std::vector<std::uint8_t> const ordinaryInitial{0x11U};
  std::vector<std::uint8_t> const regionalInitial{0x21U};
  auto const hasValue = [&](std::vector<ReflectionRecord> const& records,
                            rti::ObjectInstanceHandle const& object,
                            rti::AttributeHandle const& attribute,
                            std::vector<std::uint8_t> const& value) {
    return std::any_of(records.begin(), records.end(), [&](auto const& record) {
      auto const found = record.values.find(attribute);
      return record.object == object && found != record.values.end() &&
          copyBytes(found->second) == value;
    });
  };
  send(ordinaryObject, publisherHandles.attribute, ordinaryInitial);
  send(regionalObject, publisherRegionalAttribute, regionalInitial);
  waitForSessions(
      {&publisher, &subscriber},
      [&] {
        auto const records = subscriber.recorder().reflections();
        return hasValue(records, ordinaryObject, subscriberHandles.attribute,
                        ordinaryInitial) &&
            hasValue(records, regionalObject, subscriberRegionalAttribute,
                     regionalInitial);
      },
      options,
      "initial reflections for both object classes");
  auto const initialReflections = subscriber.recorder().reflections();
  // A report queued before declaration may be delivered after subscription;
  // §5.1 constrains reflection before declaration, not post-declaration replay.
  // Assert the two fresh values without imposing a total callback count.
  require(hasValue(initialReflections, ordinaryObject,
                       subscriberHandles.attribute, ordinaryInitial) &&
              hasValue(initialReflections, regionalObject,
                       subscriberRegionalAttribute, regionalInitial),
          "initial declarations did not independently deliver both class attributes");

  subscriber.recorder().clearReflection();
  subscriber.rtiAmbassador().unsubscribeObjectClassAttributes(
      subscriberHandles.objectClass,
      rti::AttributeHandleSet{subscriberHandles.attribute});
  auto const unsubscribedOrdinaryObject =
      publisher.rtiAmbassador().registerObjectInstance(
          publisherHandles.objectClass);
  require(unsubscribedOrdinaryObject.isValid(),
          "ordinary registration after unsubscription returned an invalid handle");
  std::vector<std::uint8_t> const ordinaryAfterUnsubscribe{0x12U};
  std::vector<std::uint8_t> const regionalAfterUnsubscribe{0x22U};
  send(ordinaryObject, publisherHandles.attribute, ordinaryAfterUnsubscribe);
  send(regionalObject, publisherRegionalAttribute, regionalAfterUnsubscribe);

  // Continue pumping for the full observation window: the other class must
  // still reflect, and the unsubscribed class must not leak a late callback.
  auto const deadline = Clock::now() +
      std::chrono::milliseconds(options.timeoutMilliseconds);
  while (Clock::now() < deadline) {
    publisher.pump();
    subscriber.pump();
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
  auto const afterUnsubscribe = subscriber.recorder().reflections();
  require(afterUnsubscribe.size() == 1U &&
              hasValue(afterUnsubscribe, regionalObject,
                       subscriberRegionalAttribute, regionalAfterUnsubscribe) &&
              !subscriber.recorder().hasDiscovery(unsubscribedOrdinaryObject),
          "unsubscribing one object class changed another class subscription or still reflected the unsubscribed class");

  subscriber.rtiAmbassador().unsubscribeObjectClassAttributesWithRegions(
      subscriberRegionalClass, subscriptionPair);
  auto const unsubscribedRegionalObject =
      publisher.rtiAmbassador().registerObjectInstanceWithRegions(
          publisherRegionalClass, sourcePair);
  require(unsubscribedRegionalObject.isValid(),
          "regional registration after unsubscription returned an invalid handle");
  auto const noSubscriptionDeadline = Clock::now() +
      std::chrono::milliseconds(options.timeoutMilliseconds);
  while (Clock::now() < noSubscriptionDeadline) {
    publisher.pump();
    subscriber.pump();
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
  require(
      !subscriber.recorder().hasDiscovery(unsubscribedRegionalObject),
      "regional object discovery continued after the class's final attribute subscription was removed");

  publisher.rtiAmbassador().unpublishObjectClassAttributes(
      publisherRegionalClass, regionalPublisherAttributes);
  publisher.rtiAmbassador().unpublishObjectClassAttributes(
      publisherHandles.objectClass, ordinaryAttributes);
  publisher.rtiAmbassador().deleteRegion(sourceRegion);
  subscriber.rtiAmbassador().deleteRegion(subscriptionRegion);
  subscriber.resign(rti::NO_ACTION);
  publisher.resign(rti::DELETE_OBJECTS);
  publisher.rtiAmbassador().destroyFederationExecution(federation);
}

void scenarioDdmUnsubscribePreservesOtherClassSubscriptionContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioDdmUnsubscribePreservesOtherClassSubscription(options, model);
}

void scenarioDeclarationTimePositionIndependence(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.logicalTimeImplementationName.empty(),
      "declaration time-position independence requires an adapter logical-time implementation");

  Session publisher(options, model, "dm-time-position-publisher");
  Session subscriber(options, model, "dm-time-position-subscriber");
  auto const federation = federationName(
      options,
      "declaration-time-position-independence");
  connectAndJoin(publisher, subscriber, options, federation, options.fom);

  auto& publisherAmbassador = publisher.rtiAmbassador();
  auto& subscriberAmbassador = subscriber.rtiAmbassador();
  auto const publisherObjectClass = publisherAmbassador.getObjectClassHandle(
      options.objectClassName);
  auto const subscriberObjectClass = subscriberAmbassador.getObjectClassHandle(
      options.objectClassName);
  auto const publisherAttribute = publisherAmbassador.getAttributeHandle(
      publisherObjectClass,
      options.attributeName);
  auto const subscriberAttribute = subscriberAmbassador.getAttributeHandle(
      subscriberObjectClass,
      options.attributeName);
  require(
      publisherObjectClass.isValid() && subscriberObjectClass == publisherObjectClass &&
          publisherAttribute.isValid() && subscriberAttribute == publisherAttribute,
      "time-position DM test FOM did not provide stable object handles");

  auto publisherTime = makeTimeContext(publisher);
  auto subscriberTime = makeTimeContext(subscriber);
  require(
      publisherTime.factory->getName() == subscriberTime.factory->getName(),
      "time-position DM test members selected different logical-time factories");
  subscriberAmbassador.enableTimeConstrained();
  waitFor(
      subscriber,
      [&] { return subscriber.recorder().timeConstrainedEnabled().size() == 1U; },
      options,
      "time-position DM subscriber time-constrained callback");
  publisherAmbassador.enableTimeRegulation(*publisherTime.epsilon);
  waitFor(
      publisher,
      [&] { return publisher.recorder().timeRegulationEnabled().size() == 1U; },
      options,
      "time-position DM publisher time-regulation callback");

  auto const publisherTargetTime = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      5U);
  publisherAmbassador.timeAdvanceRequest(*publisherTargetTime);
  waitFor(
      publisher,
      [&] { return publisher.recorder().timeAdvanceGrants().size() == 1U; },
      options,
      "time-position DM publisher grant at the later logical time");
  require(
      publisher.recorder().timeAdvanceGrants().front().encoded ==
          encodeTime(*publisherTargetTime),
      "time-position DM publisher received a grant at the wrong logical time");

  auto queryPublisherTime = publisherTime.factory->makeInitial();
  auto querySubscriberTime = subscriberTime.factory->makeInitial();
  require(
      queryPublisherTime != nullptr && querySubscriberTime != nullptr,
      "time-position DM test could not allocate logical-time query values");
  publisherAmbassador.queryLogicalTime(*queryPublisherTime);
  subscriberAmbassador.queryLogicalTime(*querySubscriberTime);
  requireTimeEquals(
      *queryPublisherTime,
      *publisherTargetTime,
      "time-position DM publisher current time before declaration services");
  requireTimeEquals(
      *querySubscriberTime,
      *subscriberTime.initial,
      "time-position DM subscriber remains at its initial time");
  require(
      encodeTime(*queryPublisherTime) != encodeTime(*querySubscriberTime),
      "time-position DM test did not establish distinct member logical times");

  // Isolate declaration-management effects: discovery must follow matching
  // object publication/subscription despite the members' distinct times.
  // Data-service callback timing is intentionally covered by separate lanes.
  rti::AttributeHandleSet const publisherAttributes{publisherAttribute};
  rti::AttributeHandleSet const subscriberAttributes{subscriberAttribute};
  publisherAmbassador.publishObjectClassAttributes(
      publisherObjectClass,
      publisherAttributes);
  subscriberAmbassador.subscribeObjectClassAttributes(
      subscriberObjectClass,
      subscriberAttributes,
      true,
      L"");

  auto const object = publisherAmbassador.registerObjectInstance(publisherObjectClass);
  require(object.isValid(), "time-position DM object registration returned an invalid handle");
  waitFor(
      subscriber,
      [&] { return subscriber.recorder().hasDiscovery(object); },
      options,
      "time-position DM receiver discovers object after cross-time publication/subscription");
  auto const discovery = subscriber.recorder().discovery();
  require(
      discovery.object == object && discovery.objectClass == subscriberObjectClass &&
          discovery.producer == publisher.federateHandle(),
      "time-position DM discovery did not preserve the registered object identity");

  auto finalPublisherTime = publisherTime.factory->makeInitial();
  auto finalSubscriberTime = subscriberTime.factory->makeInitial();
  require(
      finalPublisherTime != nullptr && finalSubscriberTime != nullptr,
      "time-position DM test could not allocate final logical-time queries");
  publisherAmbassador.queryLogicalTime(*finalPublisherTime);
  subscriberAmbassador.queryLogicalTime(*finalSubscriberTime);
  requireTimeEquals(
      *finalPublisherTime,
      *publisherTargetTime,
      "time-position DM publisher time after data delivery");
  requireTimeEquals(
      *finalSubscriberTime,
      *subscriberTime.initial,
      "time-position DM subscriber time after receive-order delivery");
  require(
      subscriber.recorder().timeAdvanceGrants().empty(),
      "time-position DM subscriber advanced while processing receive-order data");

  publisherAmbassador.unpublishObjectClassAttributes(
      publisherObjectClass,
      publisherAttributes);
  subscriberAmbassador.unsubscribeObjectClassAttributes(
      subscriberObjectClass,
      subscriberAttributes);
  subscriber.resign(rti::NO_ACTION);
  publisher.resign(rti::DELETE_OBJECTS);
  publisherAmbassador.destroyFederationExecution(federation);
}

} // namespace hla_rti_cpp_tck::detail
