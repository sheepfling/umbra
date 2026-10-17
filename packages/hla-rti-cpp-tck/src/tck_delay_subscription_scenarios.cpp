#include "tck_main_scenarios.hpp"
#include "tck_delay_subscription_scenarios.hpp"

namespace hla_rti_cpp_tck::detail {

void scenarioDelaySubscriptionEvaluationInteraction(
    Options const& options,
    rti::CallbackModel model) {
  require(!options.switchesFom.empty(),
          "switch-declaration FOM input was not supplied by the adapter");

  auto delaySubscriptionEvaluationFomModules =
      [&](bool const delaySubscriptionEvaluationEnabled) {
        std::vector<std::wstring> fomModules{options.fom.wstring()};
        if (delaySubscriptionEvaluationEnabled) {
          fomModules.push_back(options.switchesFom.wstring());
        }
        return fomModules;
      };

  auto callbackBoundary = [&](Session& receiver) {
    if (model == rti::HLA_IMMEDIATE) {
      receiver.rtiAmbassador().enableCallbacks();
    } else {
      while (receiver.rtiAmbassador().evokeCallback(0.0)) {
      }
    }
  };

  auto runConfiguration = [&](bool const delaySubscriptionEvaluationEnabled) {
    Session publisher(options, model, "delay-subscription-publisher");
    Session receiver(options, model, "delay-subscription-receiver");
    auto const federation = federationName(
        options,
        delaySubscriptionEvaluationEnabled
            ? "delay-subscription-evaluation-interaction-enabled"
            : "delay-subscription-evaluation-interaction-disabled");

    publisher.connect();
    receiver.connect();
    auto const fomModules =
        delaySubscriptionEvaluationFomModules(delaySubscriptionEvaluationEnabled);
    publisher.rtiAmbassador().createFederationExecution(
        federation,
        fomModules,
        options.logicalTimeImplementationName);
    publisher.join(
        options.ownerFederateName,
        options.federateType,
        federation);
    receiver.join(
        options.memberFederateName,
        options.federateType,
        federation);

    auto const publisherInteraction =
        publisher.rtiAmbassador().getInteractionClassHandle(
            options.interactionClassName);
    auto const receiverInteraction =
        receiver.rtiAmbassador().getInteractionClassHandle(
            options.interactionClassName);
    require(
        publisherInteraction.isValid() && receiverInteraction.isValid(),
        "delay-subscription interaction lookup returned an invalid handle");
    require(
        publisher.rtiAmbassador().getDelaySubscriptionEvaluationSwitch() ==
                delaySubscriptionEvaluationEnabled &&
            receiver.rtiAmbassador().getDelaySubscriptionEvaluationSwitch() ==
                delaySubscriptionEvaluationEnabled,
        "delay-subscription switch did not match the adapter FOM configuration");

    publisher.rtiAmbassador().publishInteractionClass(publisherInteraction);

    // A receive-order interaction generated before the receiver subscribes is
    // retained only when the federation's standard Delay Subscription
    // Evaluation switch is enabled. Immediate callbacks use the official
    // callback-control services to create the same callback boundary.
    if (model == rti::HLA_IMMEDIATE) {
      receiver.rtiAmbassador().disableCallbacks();
    }
    publisher.rtiAmbassador().sendInteraction(
        publisherInteraction,
        rti::ParameterHandleValueMap{},
        rti::VariableLengthData{});
    receiver.rtiAmbassador().subscribeInteractionClass(receiverInteraction);
    callbackBoundary(receiver);

    auto const firstInteraction = receiver.recorder().interaction();
    require(
        !delaySubscriptionEvaluationEnabled || firstInteraction.present,
        "enabled delay-subscription evaluation did not retain the interaction");
    if (firstInteraction.present) {
      require(
          firstInteraction.interaction == receiverInteraction &&
              firstInteraction.parameters.empty() &&
              firstInteraction.tag.empty() &&
              firstInteraction.producer == publisher.federateHandle() &&
              !firstInteraction.regions.has_value(),
          "delayed interaction delivery did not preserve standard callback metadata");
    }

    // With the switch disabled, immediate generation-time filtering is
    // optional; the first message may therefore have been retained or
    // rejected. A message accepted while subscribed must still use the
    // current subscription at its actual callback boundary.
    receiver.recorder().clearInteraction();
    if (model == rti::HLA_IMMEDIATE) {
      receiver.rtiAmbassador().disableCallbacks();
    }
    publisher.rtiAmbassador().sendInteraction(
        publisherInteraction,
        rti::ParameterHandleValueMap{},
        rti::VariableLengthData{});
    receiver.rtiAmbassador().unsubscribeInteractionClass(receiverInteraction);
    callbackBoundary(receiver);
    auto const afterUnsubscribe = receiver.recorder().interaction();
    require(
        !afterUnsubscribe.present,
        "callback-boundary subscription removal did not suppress the interaction");

    receiver.resign(rti::NO_ACTION);
    publisher.resign(rti::NO_ACTION);
    publisher.rtiAmbassador().destroyFederationExecution(federation);
    receiver.disconnect();
    publisher.disconnect();
  };

  runConfiguration(false);
  runConfiguration(true);
}

void scenarioDelaySubscriptionEvaluationDirectedInteraction(
    Options const& options,
    rti::CallbackModel model) {
  require(!options.switchesFom.empty(),
          "switch-declaration FOM input was not supplied by the adapter");

  auto delaySubscriptionEvaluationFomModules =
      [&](bool const delaySubscriptionEvaluationEnabled) {
        std::vector<std::wstring> fomModules{options.fom.wstring()};
        if (delaySubscriptionEvaluationEnabled) {
          fomModules.push_back(options.switchesFom.wstring());
        }
        return fomModules;
      };

  auto callbackBoundary = [&](Session& receiver) {
    if (model == rti::HLA_IMMEDIATE) {
      receiver.rtiAmbassador().enableCallbacks();
    } else {
      while (receiver.rtiAmbassador().evokeCallback(0.0)) {
      }
    }
  };

  auto runConfiguration = [&](bool const delaySubscriptionEvaluationEnabled) {
    Session owner(options, model, "delay-directed-owner");
    Session receiver(options, model, "delay-directed-receiver");
    auto const federation = federationName(
        options,
        delaySubscriptionEvaluationEnabled
            ? "delay-subscription-evaluation-directed-enabled"
            : "delay-subscription-evaluation-directed-disabled");

    owner.connect();
    receiver.connect();
    owner.rtiAmbassador().createFederationExecution(
        federation,
        delaySubscriptionEvaluationFomModules(delaySubscriptionEvaluationEnabled),
        options.logicalTimeImplementationName);
    owner.join(
        options.ownerFederateName + L"-delay-directed-owner",
        options.federateType,
        federation);
    receiver.join(
        options.memberFederateName + L"-delay-directed-receiver",
        options.federateType,
        federation);

    auto const ownerClass = owner.rtiAmbassador().getObjectClassHandle(
        options.objectClassName);
    auto const receiverClass = receiver.rtiAmbassador().getObjectClassHandle(
        options.objectClassName);
    auto const ownerAttribute = owner.rtiAmbassador().getAttributeHandle(
        ownerClass,
        options.attributeName);
    auto const receiverAttribute = receiver.rtiAmbassador().getAttributeHandle(
        receiverClass,
        options.attributeName);
    auto const ownerInteraction = owner.rtiAmbassador().getInteractionClassHandle(
        options.interactionClassName);
    auto const receiverInteraction = receiver.rtiAmbassador().getInteractionClassHandle(
        options.interactionClassName);
    require(
        ownerClass.isValid() && receiverClass.isValid() && ownerAttribute.isValid() &&
            receiverAttribute.isValid() && ownerInteraction.isValid() &&
            receiverInteraction.isValid(),
        "delay-subscription directed lookup returned an invalid handle");
    require(
        owner.rtiAmbassador().getDelaySubscriptionEvaluationSwitch() ==
                delaySubscriptionEvaluationEnabled &&
            receiver.rtiAmbassador().getDelaySubscriptionEvaluationSwitch() ==
                delaySubscriptionEvaluationEnabled,
        "delay-subscription directed switch did not match the adapter FOM configuration");

    rti::AttributeHandleSet const ownerAttributes{ownerAttribute};
    rti::AttributeHandleSet const receiverAttributes{receiverAttribute};
    owner.rtiAmbassador().publishObjectClassAttributes(ownerClass, ownerAttributes);
    receiver.rtiAmbassador().subscribeObjectClassAttributes(
        receiverClass,
        receiverAttributes,
        true,
        L"");
    rti::InteractionClassHandleSet const ownerDirected{ownerInteraction};
    rti::InteractionClassHandleSet const receiverDirected{receiverInteraction};
    owner.rtiAmbassador().publishObjectClassDirectedInteractions(
        ownerClass,
        ownerDirected);

    auto const object = owner.rtiAmbassador().registerObjectInstance(ownerClass);
    require(object.isValid(), "delay-subscription directed registration returned an invalid handle");
    waitFor(
        receiver,
        [&] { return receiver.recorder().hasDiscovery(object); },
        options,
        "delay-subscription directed object discovery");

    receiver.recorder().clearDirectedInteractions();
    if (model == rti::HLA_IMMEDIATE) {
      receiver.rtiAmbassador().disableCallbacks();
    }
    owner.rtiAmbassador().sendDirectedInteraction(
        ownerInteraction,
        object,
        rti::ParameterHandleValueMap{},
        rti::VariableLengthData{});
    receiver.rtiAmbassador().subscribeObjectClassDirectedInteractions(
        receiverClass,
        receiverDirected,
        true);
    callbackBoundary(receiver);

    auto const firstInteractions = receiver.recorder().directedInteractions();
    require(
        delaySubscriptionEvaluationEnabled
            ? firstInteractions.size() == 1U
            : firstInteractions.size() <= 1U,
        "delay-subscription directed evaluation delivered an unexpected first-message count");
    if (!firstInteractions.empty()) {
      auto const& first = firstInteractions.front();
      require(
          first.interaction == receiverInteraction && first.object == object &&
              first.parameters.empty() && first.tag.empty() &&
              first.producer == owner.federateHandle() && first.transportation.isValid(),
          "delayed directed interaction delivery did not preserve standard callback metadata");
    }

    // An enabled switch must retain the pre-subscription message. With the
    // switch disabled, §8.1.8 permits either generation-time rejection or
    // delayed filtering. After a second message is generated while selected,
    // the current selector still governs delivery at the callback boundary.
    receiver.recorder().clearDirectedInteractions();
    if (model == rti::HLA_IMMEDIATE) {
      receiver.rtiAmbassador().disableCallbacks();
    }
    owner.rtiAmbassador().sendDirectedInteraction(
        ownerInteraction,
        object,
        rti::ParameterHandleValueMap{},
        rti::VariableLengthData{});
    receiver.rtiAmbassador().unsubscribeObjectClassDirectedInteractions(
        receiverClass,
        receiverDirected);
    callbackBoundary(receiver);
    auto const afterUnsubscribe = receiver.recorder().directedInteractions();
    require(
        afterUnsubscribe.empty(),
        "callback-boundary directed subscription removal did not suppress delivery");

    receiver.resign(rti::NO_ACTION);
    owner.resign(rti::DELETE_OBJECTS);
    owner.rtiAmbassador().destroyFederationExecution(federation);
    receiver.disconnect();
    owner.disconnect();
  };

  runConfiguration(false);
  runConfiguration(true);
}

void scenarioDelaySubscriptionEvaluationAttributeUpdate(
    Options const& options,
    rti::CallbackModel model) {
  require(!options.switchesFom.empty(),
          "switch-declaration FOM input was not supplied by the adapter");

  auto runConfiguration = [&](bool const delaySubscriptionEvaluationEnabled) {
    Session publisher(options, model, "delay-subscription-attribute-publisher");
    Session receiver(options, model, "delay-subscription-attribute-receiver");
    auto const federation = federationName(
        options,
        delaySubscriptionEvaluationEnabled
            ? "delay-subscription-evaluation-attribute-enabled"
            : "delay-subscription-evaluation-attribute-disabled");

    publisher.connect();
    receiver.connect();
    std::vector<std::wstring> fomModules{options.fom.wstring()};
    if (delaySubscriptionEvaluationEnabled) {
      fomModules.push_back(options.switchesFom.wstring());
    }
    publisher.rtiAmbassador().createFederationExecution(
        federation,
        fomModules,
        options.logicalTimeImplementationName);
    publisher.join(
        options.ownerFederateName,
        options.federateType,
        federation);
    receiver.join(
        options.memberFederateName,
        options.federateType,
        federation);

    auto const publisherClass =
        publisher.rtiAmbassador().getObjectClassHandle(options.objectClassName);
    auto const receiverClass =
        receiver.rtiAmbassador().getObjectClassHandle(options.objectClassName);
    auto const publisherAttribute = publisher.rtiAmbassador().getAttributeHandle(
        publisherClass,
        options.attributeName);
    auto const receiverAttribute = receiver.rtiAmbassador().getAttributeHandle(
        receiverClass,
        options.attributeName);
    require(
        publisherClass.isValid() && receiverClass.isValid(),
        "delay-subscription attribute object-class lookup returned an invalid handle");
    require(
        publisherAttribute.isValid() && receiverAttribute.isValid(),
        "delay-subscription attribute lookup returned an invalid handle");
    require(
        publisher.rtiAmbassador().getDelaySubscriptionEvaluationSwitch() ==
                delaySubscriptionEvaluationEnabled &&
            receiver.rtiAmbassador().getDelaySubscriptionEvaluationSwitch() ==
                delaySubscriptionEvaluationEnabled,
        "delay-subscription switch did not match the adapter FOM configuration");

    rti::AttributeHandleSet const publisherAttributes{publisherAttribute};
    rti::AttributeHandleSet const receiverAttributes{receiverAttribute};
    publisher.rtiAmbassador().publishObjectClassAttributes(
        publisherClass,
        publisherAttributes);
    receiver.rtiAmbassador().subscribeObjectClassAttributes(
        receiverClass,
        receiverAttributes);
    auto const object = publisher.rtiAmbassador().registerObjectInstance(publisherClass);
    require(object.isValid(), "delay-subscription attribute registration returned an invalid handle");
    if (model == rti::HLA_EVOKED) {
      while (receiver.rtiAmbassador().evokeCallback(0.0)) {
      }
    }
    require(
        receiver.recorder().discovery().present,
        "delay-subscription attribute object discovery did not complete");
    require(
        receiver.rtiAmbassador().getKnownObjectClassHandle(object) == receiverClass,
        "delay-subscription attribute discovery did not establish the known object class");
    receiver.rtiAmbassador().unsubscribeObjectClassAttributes(
        receiverClass,
        receiverAttributes);

    unsigned char const valueBytes[] = {0xD5, 0x45};
    rti::AttributeHandleValueMap values;
    values.emplace(
        publisherAttribute,
        rti::VariableLengthData(valueBytes, sizeof(valueBytes)));

    // The receiver is known but has no active declaration at generation. The
    // enabled switch must retain a route until the callback boundary. When
    // disabled, §8.1.8 permits either generation-time rejection or delayed
    // filtering, so this test does not require one implementation choice.
    if (model == rti::HLA_IMMEDIATE) {
      receiver.rtiAmbassador().disableCallbacks();
    }
    publisher.rtiAmbassador().updateAttributeValues(
        object,
        values,
        rti::VariableLengthData{});
    receiver.rtiAmbassador().subscribeObjectClassAttributes(
        receiverClass,
        receiverAttributes);
    if (model == rti::HLA_IMMEDIATE) {
      receiver.rtiAmbassador().enableCallbacks();
    } else {
      while (receiver.rtiAmbassador().evokeCallback(0.0)) {
      }
    }

    auto const firstReflection = receiver.recorder().reflection();
    require(
        !delaySubscriptionEvaluationEnabled || firstReflection.present,
        "enabled delay-subscription evaluation did not retain the attribute update");
    if (firstReflection.present) {
      require(
          firstReflection.object == object &&
              firstReflection.values.size() == 1U &&
              firstReflection.values.count(receiverAttribute) == 1U &&
              copyBytes(firstReflection.values.at(receiverAttribute)) ==
                  std::vector<std::uint8_t>(valueBytes, valueBytes + sizeof(valueBytes)) &&
              firstReflection.tag.empty() &&
              firstReflection.producer == publisher.federateHandle() &&
              firstReflection.transportation.isValid() &&
              !firstReflection.regions.has_value(),
          "delayed attribute reflection did not preserve standard callback metadata");
    }

    // A recipient accepted while subscribed is still re-evaluated against
    // the current declaration immediately before ordinary delivery.
    receiver.recorder().clearReflection();
    if (model == rti::HLA_IMMEDIATE) {
      receiver.rtiAmbassador().disableCallbacks();
    }
    publisher.rtiAmbassador().updateAttributeValues(
        object,
        values,
        rti::VariableLengthData{});
    receiver.rtiAmbassador().unsubscribeObjectClassAttributes(
        receiverClass,
        receiverAttributes);
    if (model == rti::HLA_IMMEDIATE) {
      receiver.rtiAmbassador().enableCallbacks();
    } else {
      while (receiver.rtiAmbassador().evokeCallback(0.0)) {
      }
    }
    auto const afterUnsubscribe = receiver.recorder().reflection();
    require(
        !afterUnsubscribe.present,
        "callback-boundary attribute unsubscribe did not suppress the reflection");

    receiver.resign(rti::NO_ACTION);
    publisher.resign(rti::DELETE_OBJECTS);
    publisher.rtiAmbassador().destroyFederationExecution(federation);
    receiver.disconnect();
    publisher.disconnect();
  };

  runConfiguration(false);
  runConfiguration(true);
}

void scenarioDelaySubscriptionEvaluationReceiveOrderRegionalAttributeUpdate(
    Options const& options,
    rti::CallbackModel model) {
  require(!options.switchesFom.empty(),
          "regional delay-subscription testing requires a switch-declaration FOM");
  require(!options.ddmFom.empty(),
          "regional delay-subscription testing requires a dimensional FOM");

  auto runConfiguration = [&](bool const delayEnabled) {
    Session publisher(options, model, "delay-subscription-ro-regional-publisher");
    Session receiver(options, model, "delay-subscription-ro-regional-receiver");
    auto const federation = federationName(
        options,
        delayEnabled ? "delay-subscription-ro-regional-enabled"
                     : "delay-subscription-ro-regional-disabled");
    publisher.connect();
    receiver.connect();
    std::vector<std::wstring> modules{options.ddmFom.wstring()};
    if (delayEnabled) modules.push_back(options.switchesFom.wstring());
    publisher.rtiAmbassador().createFederationExecution(
        federation, modules, options.logicalTimeImplementationName);
    publisher.join(options.ownerFederateName, options.federateType, federation);
    receiver.join(options.memberFederateName, options.federateType, federation);

    auto const sent = ddmHandles(publisher, options);
    auto const received = ddmHandles(receiver, options);
    verifyDdmClassDimensions(
        publisher, options, sent, "receive-order regional delay-subscription evaluation");
    require(
        publisher.rtiAmbassador().getDelaySubscriptionEvaluationSwitch() == delayEnabled &&
            receiver.rtiAmbassador().getDelaySubscriptionEvaluationSwitch() == delayEnabled,
        "regional delay-subscription switch did not match the FOM composition");
    rti::AttributeHandleSet const sentAttributes{sent.attribute};
    rti::AttributeHandleSet const receivedAttributes{received.attribute};
    publisher.rtiAmbassador().publishObjectClassAttributes(
        sent.objectClass, sentAttributes);
    receiver.rtiAmbassador().setConveyRegionDesignatorSetsSwitch(true);
    require(receiver.rtiAmbassador().getConveyRegionDesignatorSetsSwitch(),
            "regional delay-subscription receiver did not enable source-region callbacks");

    auto const sourceRegion = createDdmRegion(publisher, sent, 1UL, 5UL, 1UL, 5UL);
    auto const receiverRegion = createDdmRegion(receiver, received, 2UL, 6UL, 2UL, 6UL);
    rti::AttributeHandleSetRegionHandleSetPairVector const publisherPair{{
        sentAttributes, rti::RegionHandleSet{sourceRegion},
    }};
    rti::AttributeHandleSetRegionHandleSetPairVector const receiverPair{{
        receivedAttributes, rti::RegionHandleSet{receiverRegion},
    }};
    receiver.rtiAmbassador().subscribeObjectClassAttributesWithRegions(
        received.objectClass, receiverPair);
    auto const object = publisher.rtiAmbassador().registerObjectInstanceWithRegions(
        sent.objectClass, publisherPair);
    require(object.isValid(), "regional delay-subscription registration returned an invalid handle");
    waitForSessions(
        {&publisher, &receiver},
        [&] { return receiver.recorder().hasDiscovery(object); },
        options,
        "regional delay-subscription object discovery");
    require(receiver.rtiAmbassador().getKnownObjectClassHandle(object) ==
                received.objectClass,
            "regional delay-subscription discovery did not establish a known object");

    auto setCommittedRange = [&](Session& session,
                                 DdmHandles const& handles,
                                 rti::RegionHandle const& region,
                                 unsigned long lower,
                                 unsigned long upper) {
      session.rtiAmbassador().setRangeBounds(
          region, handles.firstDimension, rti::RangeBounds(lower, upper));
      session.rtiAmbassador().setRangeBounds(
          region, handles.secondDimension, rti::RangeBounds(lower, upper));
      session.rtiAmbassador().commitRegionModifications(rti::RegionHandleSet{region});
    };
    auto update = [&](unsigned char marker) {
      rti::AttributeHandleValueMap values;
      values.emplace(sent.attribute, rti::VariableLengthData(&marker, 1U));
      publisher.rtiAmbassador().updateAttributeValues(
          object, values, rti::VariableLengthData(&marker, 1U));
    };
    auto checkReflection = [&](ReflectionRecord const& reflection,
                               unsigned char marker,
                               std::string const& description) {
      require(
          reflection.object == object && reflection.values.size() == 1U &&
              reflection.values.count(received.attribute) == 1U &&
              copyBytes(reflection.values.at(received.attribute)) ==
                  std::vector<std::uint8_t>{marker},
          description + " returned the wrong object, attribute, or value");
      require(
          reflection.tag == std::vector<std::uint8_t>{marker} &&
              reflection.producer == publisher.federateHandle() &&
              reflection.transportation.isValid() && reflection.regions.has_value() &&
              reflection.regions->size() == 1U &&
              reflection.regions->count(sourceRegion) == 1U,
          description + " did not preserve tag, producer, transport, or source region");
    };
    auto openCallbackGate = [&] {
      if (model == rti::HLA_IMMEDIATE) receiver.rtiAmbassador().enableCallbacks();
    };
    auto closeCallbackGate = [&] {
      if (model == rti::HLA_IMMEDIATE) receiver.rtiAmbassador().disableCallbacks();
    };

    // A known object is unsubscribed and region-disjoint when this ordinary
    // update is generated. With DSE enabled, eligibility uses the restored
    // subscription and the source region's committed bounds at send time.
    closeCallbackGate();
    receiver.rtiAmbassador().unsubscribeObjectClassAttributesWithRegions(
        received.objectClass, receiverPair);
    setCommittedRange(receiver, received, receiverRegion, 7UL, 9UL);
    update(0xD1U);
    setCommittedRange(publisher, sent, sourceRegion, 7UL, 9UL);
    setCommittedRange(receiver, received, receiverRegion, 2UL, 6UL);
    receiver.rtiAmbassador().subscribeObjectClassAttributesWithRegions(
        received.objectClass, receiverPair);
    openCallbackGate();
    if (delayEnabled) {
      waitForSessions(
          {&publisher, &receiver},
          [&] { return receiver.recorder().reflectionCount() >= 1U; },
          options,
          "enabled receive-order regional late subscription");
    } else {
      receiver.pump();
    }
    auto const first = receiver.recorder().reflections();
    require(first.size() == (delayEnabled ? 1U : 0U),
            "regional late subscription produced an unexpected reflection count");
    if (!first.empty()) checkReflection(first.front(), 0xD1U, "late regional reflection");
    receiver.recorder().clearReflection();

    // The first queued passel now loses its regional match. A subsequent
    // matching passel is a positive delivery fence for both callback models.
    closeCallbackGate();
    setCommittedRange(publisher, sent, sourceRegion, 1UL, 5UL);
    update(0xD2U);
    setCommittedRange(receiver, received, receiverRegion, 7UL, 9UL);
    setCommittedRange(publisher, sent, sourceRegion, 7UL, 9UL);
    update(0xD3U);
    openCallbackGate();
    waitForSessions(
        {&publisher, &receiver},
        [&] { return receiver.recorder().reflectionCount() >= 1U; },
        options,
        "receive-order regional subscription suppression and successor delivery");
    auto const afterRangeChange = receiver.recorder().reflections();
    require(afterRangeChange.size() == 1U,
            "disjoint regional passel escaped the callback-boundary recheck");
    checkReflection(afterRangeChange.front(), 0xD3U, "matching successor reflection");

    receiver.rtiAmbassador().unsubscribeObjectClassAttributesWithRegions(
        received.objectClass, receiverPair);
    publisher.rtiAmbassador().unassociateRegionsForUpdates(object, publisherPair);
    receiver.rtiAmbassador().deleteRegion(receiverRegion);
    publisher.rtiAmbassador().deleteRegion(sourceRegion);
    receiver.resign(rti::NO_ACTION);
    publisher.resign(rti::DELETE_OBJECTS);
    publisher.rtiAmbassador().destroyFederationExecution(federation);
    receiver.disconnect();
    publisher.disconnect();
  };

  runConfiguration(false);
  runConfiguration(true);
}

void scenarioDelaySubscriptionEvaluationTimestampedInteraction(
    Options const& options,
    rti::CallbackModel model) {
  require(!options.switchesFom.empty(),
          "switch-declaration FOM input was not supplied by the adapter");
  require(!options.logicalTimeImplementationName.empty(),
          "timestamped delay-subscription scenario requires a logical-time implementation");

  auto runConfiguration = [&](bool const delaySubscriptionEvaluationEnabled) {
    Session publisher(options, model, "delay-subscription-timestamped-publisher");
    Session receiver(options, model, "delay-subscription-timestamped-receiver");
    auto const federation = federationName(
        options,
        delaySubscriptionEvaluationEnabled
            ? "delay-subscription-evaluation-timestamped-interaction-enabled"
            : "delay-subscription-evaluation-timestamped-interaction-disabled");

    publisher.connect();
    receiver.connect();
    std::vector<std::wstring> fomModules{options.fom.wstring()};
    if (delaySubscriptionEvaluationEnabled) {
      fomModules.push_back(options.switchesFom.wstring());
    }
    publisher.rtiAmbassador().createFederationExecution(
        federation,
        fomModules,
        options.logicalTimeImplementationName);
    publisher.join(
        options.ownerFederateName,
        options.federateType,
        federation);
    receiver.join(
        options.memberFederateName,
        options.federateType,
        federation);

    auto const publisherInteraction =
        publisher.rtiAmbassador().getInteractionClassHandle(options.interactionClassName);
    auto const receiverInteraction =
        receiver.rtiAmbassador().getInteractionClassHandle(options.interactionClassName);
    auto const publisherParameter = publisher.rtiAmbassador().getParameterHandle(
        publisherInteraction,
        options.parameterName);
    auto const receiverParameter = receiver.rtiAmbassador().getParameterHandle(
        receiverInteraction,
        options.parameterName);
    require(
        publisherInteraction.isValid() && receiverInteraction.isValid(),
        "delay-subscription timestamped interaction lookup returned an invalid handle");
    require(
        publisherParameter.isValid() && receiverParameter.isValid(),
        "delay-subscription timestamped interaction parameter lookup returned an invalid handle");
    require(
        publisher.rtiAmbassador().getDelaySubscriptionEvaluationSwitch() ==
                delaySubscriptionEvaluationEnabled &&
            receiver.rtiAmbassador().getDelaySubscriptionEvaluationSwitch() ==
                delaySubscriptionEvaluationEnabled,
        "delay-subscription timestamped interaction switch did not match the adapter FOM configuration");
    publisher.rtiAmbassador().publishInteractionClass(publisherInteraction);
    publisher.rtiAmbassador().changeInteractionOrderType(
        publisherInteraction,
        rti::TIMESTAMP);

    auto publisherTime = makeTimeContext(publisher);
    auto receiverTime = makeTimeContext(receiver);
    enableTimestampedRoles(
        publisher,
        receiver,
        publisherTime,
        receiverTime,
        options,
        "delay-subscription timestamped interaction");

    unsigned char const payloadBytes[] = {0xA1};
    rti::ParameterHandleValueMap const parameters{
        {publisherParameter, rti::VariableLengthData(payloadBytes, sizeof(payloadBytes))}};
    auto assertInteraction = [&](TimedInteractionRecord const& record,
                                 std::vector<std::uint8_t> const& expectedTime,
                                 rti::OrderType expectedReceivedOrder,
                                 std::string const& description) {
      require(
          record.interaction == receiverInteraction &&
              record.parameters.size() == 1U &&
              record.parameters.count(receiverParameter) == 1U &&
              copyBytes(record.parameters.at(receiverParameter)) ==
                  std::vector<std::uint8_t>(payloadBytes, payloadBytes + sizeof(payloadBytes)) &&
              record.tag.empty() &&
              record.producer == publisher.federateHandle() &&
              record.transportation.isValid() &&
              !receiver.rtiAmbassador().getTransportationTypeName(record.transportation).empty() &&
              record.time == expectedTime && !record.timeText.empty() &&
              record.sentOrder == rti::TIMESTAMP &&
              record.receivedOrder == expectedReceivedOrder &&
              !record.regions.has_value(),
          description + " did not preserve standard timestamped interaction metadata");
    };

    auto const firstTime = timeAfter(
        *publisherTime.factory,
        *publisherTime.initial,
        *publisherTime.epsilon,
        2U);
    auto const firstReceiverTime = timeAfter(
        *receiverTime.factory,
        *receiverTime.initial,
        *receiverTime.epsilon,
        2U);
    auto const firstRetraction = publisher.rtiAmbassador().sendInteraction(
        publisherInteraction,
        parameters,
        rti::VariableLengthData{},
        *firstTime);
    require(
        !delaySubscriptionEvaluationEnabled || firstRetraction.isValid(),
        "delay-subscription enabled timestamped interaction returned an invalid retraction handle");
    receiver.rtiAmbassador().subscribeInteractionClass(receiverInteraction);
    receiver.rtiAmbassador().timeAdvanceRequest(*firstReceiverTime);
    publisher.rtiAmbassador().timeAdvanceRequest(*firstTime);
    auto const minimumFirstCount = delaySubscriptionEvaluationEnabled ? 1U : 0U;
    waitFor(
        publisher,
        receiver,
        [&] {
          return publisher.recorder().timeAdvanceGrants().size() >= 1U &&
              receiver.recorder().timeAdvanceGrants().size() >= 1U &&
              receiver.recorder().timedInteractions().size() >= minimumFirstCount;
        },
        options,
        "delay-subscription timestamped interaction first grant");
    auto const firstDeliveryCount = receiver.recorder().timedInteractions().size();
    require(
        delaySubscriptionEvaluationEnabled
            ? firstDeliveryCount == 1U
            : firstDeliveryCount <= 1U,
        "delay-subscription timestamped interaction delivered an unexpected first-message count");
    if (firstDeliveryCount != 0U) {
      auto const firstInteractions = receiver.recorder().timedInteractions();
      assertInteraction(
          firstInteractions.front(),
          encodeTime(*firstTime),
          rti::TIMESTAMP,
          "first delayed timestamped interaction");
      require(
          firstInteractions.front().retractionPresent &&
              firstInteractions.front().retraction == copyBytes(firstRetraction.encode()),
          "first delayed timestamped interaction returned the wrong retraction handle");
    }

    // The second message is generated while subscribed, but its recipient is
    // removed before the next grant. Generation-time eligibility is not a
    // promise of delivery in either switch mode.
    auto const secondTime = timeAfter(
        *publisherTime.factory,
        *firstTime,
        *publisherTime.epsilon,
        1U);
    auto const secondReceiverTime = timeAfter(
        *receiverTime.factory,
        *firstReceiverTime,
        *receiverTime.epsilon,
        1U);
    auto const secondRetraction = publisher.rtiAmbassador().sendInteraction(
        publisherInteraction,
        parameters,
        rti::VariableLengthData{},
        *secondTime);
    require(
        secondRetraction.isValid(),
        "delay-subscription timestamped interaction returned an invalid second retraction handle");
    receiver.rtiAmbassador().unsubscribeInteractionClass(receiverInteraction);
    receiver.rtiAmbassador().timeAdvanceRequest(*secondReceiverTime);
    publisher.rtiAmbassador().timeAdvanceRequest(*secondTime);
    waitFor(
        publisher,
        receiver,
        [&] {
          return publisher.recorder().timeAdvanceGrants().size() >= 2U &&
              receiver.recorder().timeAdvanceGrants().size() >= 2U;
        },
        options,
        "delay-subscription timestamped interaction second grant");
    require(
        receiver.recorder().timedInteractions().size() == firstDeliveryCount,
        "delay-subscription timestamped interaction delivered after unsubscribe");

    receiver.resign(rti::NO_ACTION);
    publisher.resign(rti::NO_ACTION);
    publisher.rtiAmbassador().destroyFederationExecution(federation);
    receiver.disconnect();
    publisher.disconnect();
  };

  runConfiguration(false);
  runConfiguration(true);
}

void scenarioDelaySubscriptionEvaluationTimestampedDirectedInteraction(
    Options const& options,
    rti::CallbackModel model) {
  require(!options.switchesFom.empty(),
          "switch-declaration FOM input was not supplied by the adapter");
  require(!options.logicalTimeImplementationName.empty(),
          "timestamped directed delay-subscription scenario requires a logical-time implementation");

  auto runConfiguration = [&](bool const delaySubscriptionEvaluationEnabled) {
    Session owner(options, model, "delay-tso-directed-owner");
    Session receiver(options, model, "delay-tso-directed-receiver");
    auto const federation = federationName(
        options,
        delaySubscriptionEvaluationEnabled
            ? "delay-subscription-evaluation-timestamped-directed-enabled"
            : "delay-subscription-evaluation-timestamped-directed-disabled");

    owner.connect();
    receiver.connect();
    std::vector<std::wstring> fomModules{options.fom.wstring()};
    if (delaySubscriptionEvaluationEnabled) {
      fomModules.push_back(options.switchesFom.wstring());
    }
    owner.rtiAmbassador().createFederationExecution(
        federation,
        fomModules,
        options.logicalTimeImplementationName);
    owner.join(
        options.ownerFederateName + L"-delay-tso-directed-owner",
        options.federateType,
        federation);
    receiver.join(
        options.memberFederateName + L"-delay-tso-directed-receiver",
        options.federateType,
        federation);

    auto const ownerClass = owner.rtiAmbassador().getObjectClassHandle(
        options.objectClassName);
    auto const receiverClass = receiver.rtiAmbassador().getObjectClassHandle(
        options.objectClassName);
    auto const ownerAttribute = owner.rtiAmbassador().getAttributeHandle(
        ownerClass,
        options.attributeName);
    auto const receiverAttribute = receiver.rtiAmbassador().getAttributeHandle(
        receiverClass,
        options.attributeName);
    auto const ownerInteraction = owner.rtiAmbassador().getInteractionClassHandle(
        options.interactionClassName);
    auto const receiverInteraction = receiver.rtiAmbassador().getInteractionClassHandle(
        options.interactionClassName);
    require(
        ownerClass.isValid() && receiverClass.isValid() && ownerAttribute.isValid() &&
            receiverAttribute.isValid() && ownerInteraction.isValid() &&
            receiverInteraction.isValid(),
        "delay-subscription timestamped directed lookup returned an invalid handle");
    require(
        owner.rtiAmbassador().getDelaySubscriptionEvaluationSwitch() ==
                delaySubscriptionEvaluationEnabled &&
            receiver.rtiAmbassador().getDelaySubscriptionEvaluationSwitch() ==
                delaySubscriptionEvaluationEnabled,
        "delay-subscription timestamped directed switch did not match the adapter FOM configuration");

    rti::AttributeHandleSet const ownerAttributes{ownerAttribute};
    rti::AttributeHandleSet const receiverAttributes{receiverAttribute};
    owner.rtiAmbassador().publishObjectClassAttributes(ownerClass, ownerAttributes);
    receiver.rtiAmbassador().subscribeObjectClassAttributes(
        receiverClass,
        receiverAttributes,
        true,
        L"");
    rti::InteractionClassHandleSet const ownerDirected{ownerInteraction};
    rti::InteractionClassHandleSet const receiverDirected{receiverInteraction};
    owner.rtiAmbassador().publishObjectClassDirectedInteractions(
        ownerClass,
        ownerDirected);
    owner.rtiAmbassador().changeInteractionOrderType(
        ownerInteraction,
        rti::TIMESTAMP);

    auto const object = owner.rtiAmbassador().registerObjectInstance(ownerClass);
    require(object.isValid(), "delay-subscription timestamped directed registration returned an invalid handle");
    waitFor(
        receiver,
        [&] { return receiver.recorder().hasDiscovery(object); },
        options,
        "delay-subscription timestamped directed object discovery");

    auto ownerTime = makeTimeContext(owner);
    auto receiverTime = makeTimeContext(receiver);
    enableTimestampedRoles(
        owner,
        receiver,
        ownerTime,
        receiverTime,
        options,
        "delay-subscription timestamped directed interaction");

    auto const firstTime = timeAfter(
        *ownerTime.factory,
        *ownerTime.initial,
        *ownerTime.epsilon,
        2U);
    auto const firstReceiverTime = timeAfter(
        *receiverTime.factory,
        *receiverTime.initial,
        *receiverTime.epsilon,
        2U);
    auto const firstRetraction = owner.rtiAmbassador().sendDirectedInteraction(
        ownerInteraction,
        object,
        rti::ParameterHandleValueMap{},
        rti::VariableLengthData{},
        *firstTime);
    require(
        !delaySubscriptionEvaluationEnabled || firstRetraction.isValid(),
        "delay-subscription enabled timestamped directed send returned an invalid first retraction handle");
    receiver.rtiAmbassador().subscribeObjectClassDirectedInteractions(
        receiverClass,
        receiverDirected,
        true);
    receiver.rtiAmbassador().timeAdvanceRequest(*firstReceiverTime);
    owner.rtiAmbassador().timeAdvanceRequest(*firstTime);
    auto const minimumFirstCount = delaySubscriptionEvaluationEnabled ? 1U : 0U;
    waitFor(
        owner,
        receiver,
        [&] {
          return owner.recorder().timeAdvanceGrants().size() >= 1U &&
              receiver.recorder().timeAdvanceGrants().size() >= 1U &&
              receiver.recorder().timedDirectedInteractions().size() >= minimumFirstCount;
        },
        options,
        "delay-subscription timestamped directed interaction first grant");
    auto const firstDeliveryCount = receiver.recorder().timedDirectedInteractions().size();
    require(
        delaySubscriptionEvaluationEnabled
            ? firstDeliveryCount == 1U
            : firstDeliveryCount <= 1U,
        "delay-subscription timestamped directed interaction delivered an unexpected first-message count");
    if (firstDeliveryCount != 0U) {
      auto const firstInteractions = receiver.recorder().timedDirectedInteractions();
      auto const& first = firstInteractions.front();
      require(
          first.interaction == receiverInteraction && first.object == object &&
              first.parameters.empty() && first.tag.empty() &&
              first.producer == owner.federateHandle() && first.transportation.isValid() &&
              first.time == copyBytes(firstTime->encode()) &&
              first.sentOrder == rti::TIMESTAMP && first.receivedOrder == rti::TIMESTAMP,
          "first delayed timestamped directed interaction did not preserve standard metadata");
    }

    auto const secondTime = timeAfter(
        *ownerTime.factory,
        *firstTime,
        *ownerTime.epsilon,
        1U);
    auto const secondReceiverTime = timeAfter(
        *receiverTime.factory,
        *firstReceiverTime,
        *receiverTime.epsilon,
        1U);
    auto const secondRetraction = owner.rtiAmbassador().sendDirectedInteraction(
        ownerInteraction,
        object,
        rti::ParameterHandleValueMap{},
        rti::VariableLengthData{},
        *secondTime);
    require(
        secondRetraction.isValid(),
        "delay-subscription timestamped directed send returned an invalid second retraction handle");
    receiver.rtiAmbassador().unsubscribeObjectClassDirectedInteractions(
        receiverClass,
        receiverDirected);
    receiver.rtiAmbassador().timeAdvanceRequest(*secondReceiverTime);
    owner.rtiAmbassador().timeAdvanceRequest(*secondTime);
    waitFor(
        owner,
        receiver,
        [&] {
          return owner.recorder().timeAdvanceGrants().size() >= 2U &&
              receiver.recorder().timeAdvanceGrants().size() >= 2U;
        },
        options,
        "delay-subscription timestamped directed interaction second grant");
    require(
        receiver.recorder().timedDirectedInteractions().size() == firstDeliveryCount,
        "delay-subscription timestamped directed interaction delivered after unsubscribe");

    receiver.resign(rti::NO_ACTION);
    owner.resign(rti::DELETE_OBJECTS);
    owner.rtiAmbassador().destroyFederationExecution(federation);
    receiver.disconnect();
    owner.disconnect();
  };

  runConfiguration(false);
  runConfiguration(true);
}

void scenarioDelaySubscriptionEvaluationTimestampedAttributeUpdate(
    Options const& options,
    rti::CallbackModel model) {
  require(!options.switchesFom.empty(),
          "switch-declaration FOM input was not supplied by the adapter");
  require(!options.logicalTimeImplementationName.empty(),
          "timestamped delay-subscription scenario requires a logical-time implementation");

  auto runConfiguration = [&](bool const delaySubscriptionEvaluationEnabled) {
    Session publisher(options, model, "delay-subscription-timestamped-attribute-publisher");
    Session receiver(options, model, "delay-subscription-timestamped-attribute-receiver");
    auto const federation = federationName(
        options,
        delaySubscriptionEvaluationEnabled
            ? "delay-subscription-evaluation-timestamped-attribute-enabled"
            : "delay-subscription-evaluation-timestamped-attribute-disabled");

    publisher.connect();
    receiver.connect();
    std::vector<std::wstring> fomModules{options.fom.wstring()};
    if (delaySubscriptionEvaluationEnabled) {
      fomModules.push_back(options.switchesFom.wstring());
    }
    publisher.rtiAmbassador().createFederationExecution(
        federation,
        fomModules,
        options.logicalTimeImplementationName);
    publisher.join(
        options.ownerFederateName,
        options.federateType,
        federation);
    receiver.join(
        options.memberFederateName,
        options.federateType,
        federation);

    auto const publisherClass =
        publisher.rtiAmbassador().getObjectClassHandle(options.objectClassName);
    auto const receiverClass =
        receiver.rtiAmbassador().getObjectClassHandle(options.objectClassName);
    auto const publisherAttribute = publisher.rtiAmbassador().getAttributeHandle(
        publisherClass,
        options.attributeName);
    auto const receiverAttribute = receiver.rtiAmbassador().getAttributeHandle(
        receiverClass,
        options.attributeName);
    require(
        publisherClass.isValid() && receiverClass.isValid(),
        "delay-subscription timestamped attribute object-class lookup returned an invalid handle");
    require(
        publisherAttribute.isValid() && receiverAttribute.isValid(),
        "delay-subscription timestamped attribute lookup returned an invalid handle");
    require(
        publisher.rtiAmbassador().getDelaySubscriptionEvaluationSwitch() ==
                delaySubscriptionEvaluationEnabled &&
            receiver.rtiAmbassador().getDelaySubscriptionEvaluationSwitch() ==
                delaySubscriptionEvaluationEnabled,
        "delay-subscription timestamped attribute switch did not match the adapter FOM configuration");

    rti::AttributeHandleSet const publisherAttributes{publisherAttribute};
    rti::AttributeHandleSet const receiverAttributes{receiverAttribute};
    publisher.rtiAmbassador().publishObjectClassAttributes(
        publisherClass,
        publisherAttributes);
    publisher.rtiAmbassador().changeDefaultAttributeOrderType(
        publisherClass,
        publisherAttributes,
        rti::TIMESTAMP);
    receiver.rtiAmbassador().subscribeObjectClassAttributes(
        receiverClass,
        receiverAttributes);
    auto const object = publisher.rtiAmbassador().registerObjectInstance(publisherClass);
    require(
        object.isValid(),
        "delay-subscription timestamped attribute registration returned an invalid handle");
    if (model == rti::HLA_EVOKED) {
      while (receiver.rtiAmbassador().evokeCallback(0.0)) {
      }
    }
    require(
        receiver.recorder().discovery().present &&
            receiver.rtiAmbassador().getKnownObjectClassHandle(object) == receiverClass,
        "delay-subscription timestamped attribute discovery did not establish the known object");
    receiver.rtiAmbassador().unsubscribeObjectClassAttributes(
        receiverClass,
        receiverAttributes);

    auto publisherTime = makeTimeContext(publisher);
    auto receiverTime = makeTimeContext(receiver);
    enableTimestampedRoles(
        publisher,
        receiver,
        publisherTime,
        receiverTime,
        options,
        "delay-subscription timestamped attribute");

    unsigned char const valueBytes[] = {0xD5, 0x54};
    rti::AttributeHandleValueMap values;
    values.emplace(
        publisherAttribute,
        rti::VariableLengthData(valueBytes, sizeof(valueBytes)));
    auto assertReflection = [&](TimedReflectionRecord const& reflection,
                                std::vector<std::uint8_t> const& expectedTime,
                                rti::OrderType expectedReceivedOrder,
                                std::vector<std::uint8_t> const& expectedRetraction,
                                std::string const& description) {
      require(
          reflection.object == object &&
              reflection.values.size() == 1U &&
              reflection.values.count(receiverAttribute) == 1U &&
              copyBytes(reflection.values.at(receiverAttribute)) ==
                  std::vector<std::uint8_t>(valueBytes, valueBytes + sizeof(valueBytes)) &&
              reflection.tag.empty() &&
              reflection.producer == publisher.federateHandle() &&
              reflection.transportation.isValid() &&
              !receiver.rtiAmbassador().getTransportationTypeName(reflection.transportation).empty() &&
              reflection.time == expectedTime && !reflection.timeText.empty() &&
              reflection.sentOrder == rti::TIMESTAMP &&
              reflection.receivedOrder == expectedReceivedOrder &&
              reflection.retractionPresent &&
              reflection.retraction == expectedRetraction &&
              !reflection.regions.has_value(),
          description + " did not preserve standard timestamped attribute metadata");
    };

    auto const firstTime = timeAfter(
        *publisherTime.factory,
        *publisherTime.initial,
        *publisherTime.epsilon,
        2U);
    auto const firstReceiverTime = timeAfter(
        *receiverTime.factory,
        *receiverTime.initial,
        *receiverTime.epsilon,
        2U);
    auto const firstRetraction = publisher.rtiAmbassador().updateAttributeValues(
        object,
        values,
        rti::VariableLengthData{},
        *firstTime);
    require(
        !delaySubscriptionEvaluationEnabled || firstRetraction.isValid(),
        "delay-subscription enabled timestamped attribute update returned an invalid retraction handle");
    receiver.rtiAmbassador().subscribeObjectClassAttributes(
        receiverClass,
        receiverAttributes);
    receiver.rtiAmbassador().timeAdvanceRequest(*firstReceiverTime);
    publisher.rtiAmbassador().timeAdvanceRequest(*firstTime);
    auto const minimumFirstCount = delaySubscriptionEvaluationEnabled ? 1U : 0U;
    waitFor(
        publisher,
        receiver,
        [&] {
          return publisher.recorder().timeAdvanceGrants().size() >= 1U &&
              receiver.recorder().timeAdvanceGrants().size() >= 1U &&
              receiver.recorder().timedReflections().size() >= minimumFirstCount;
        },
        options,
        "delay-subscription timestamped attribute first grant");
    auto const firstDeliveryCount = receiver.recorder().timedReflections().size();
    require(
        delaySubscriptionEvaluationEnabled
            ? firstDeliveryCount == 1U
            : firstDeliveryCount <= 1U,
        "delay-subscription timestamped attribute delivered an unexpected first-reflection count");
    if (firstDeliveryCount != 0U) {
      assertReflection(
          receiver.recorder().timedReflections().front(),
          encodeTime(*firstTime),
          rti::TIMESTAMP,
          copyBytes(firstRetraction.encode()),
          "first delayed timestamped attribute reflection");
    }

    // The second passel is accepted while subscribed but the declaration is
    // removed before the next grant, so current subscription state suppresses
    // it in both switch modes.
    auto const secondTime = timeAfter(
        *publisherTime.factory,
        *firstTime,
        *publisherTime.epsilon,
        1U);
    auto const secondReceiverTime = timeAfter(
        *receiverTime.factory,
        *firstReceiverTime,
        *receiverTime.epsilon,
        1U);
    auto const secondRetraction = publisher.rtiAmbassador().updateAttributeValues(
        object,
        values,
        rti::VariableLengthData{},
        *secondTime);
    require(
        secondRetraction.isValid(),
        "delay-subscription timestamped attribute update returned an invalid second retraction handle");
    receiver.rtiAmbassador().unsubscribeObjectClassAttributes(
        receiverClass,
        receiverAttributes);
    receiver.rtiAmbassador().timeAdvanceRequest(*secondReceiverTime);
    publisher.rtiAmbassador().timeAdvanceRequest(*secondTime);
    waitFor(
        publisher,
        receiver,
        [&] {
          return publisher.recorder().timeAdvanceGrants().size() >= 2U &&
              receiver.recorder().timeAdvanceGrants().size() >= 2U;
        },
        options,
        "delay-subscription timestamped attribute second grant");
    require(
        receiver.recorder().timedReflections().size() == firstDeliveryCount,
        "delay-subscription timestamped attribute delivered after unsubscribe");

    receiver.rtiAmbassador().disableTimeConstrained();
    publisher.rtiAmbassador().disableTimeRegulation();
    receiver.resign(rti::NO_ACTION);
    publisher.resign(rti::DELETE_OBJECTS);
    publisher.rtiAmbassador().destroyFederationExecution(federation);
    receiver.disconnect();
    publisher.disconnect();
  };

  runConfiguration(false);
  runConfiguration(true);
}

void scenarioDelaySubscriptionEvaluationTimestampedRegionalAttributeUpdate(
    Options const& options,
    rti::CallbackModel model) {
  require(!options.switchesFom.empty(),
          "switch-declaration FOM input was not supplied by the adapter");
  require(!options.ddmFom.empty(),
          "timestamped regional delay-subscription testing requires an adapter-supplied dimensional FOM");
  require(!options.logicalTimeImplementationName.empty(),
          "timestamped regional delay-subscription testing requires a logical-time implementation");

  auto runConfiguration = [&](bool const delaySubscriptionEvaluationEnabled) {
    Session publisher(options, model, "delay-subscription-regional-attribute-publisher");
    Session receiver(options, model, "delay-subscription-regional-attribute-receiver");
    auto const federation = federationName(
        options,
        delaySubscriptionEvaluationEnabled
            ? "delay-subscription-evaluation-timestamped-regional-attribute-enabled"
            : "delay-subscription-evaluation-timestamped-regional-attribute-disabled");

    publisher.connect();
    receiver.connect();
    std::vector<std::wstring> fomModules{options.ddmFom.wstring()};
    if (delaySubscriptionEvaluationEnabled) {
      fomModules.push_back(options.switchesFom.wstring());
    }
    publisher.rtiAmbassador().createFederationExecution(
        federation,
        fomModules,
        options.logicalTimeImplementationName);
    publisher.join(options.ownerFederateName, options.federateType, federation);
    receiver.join(options.memberFederateName, options.federateType, federation);

    auto const publisherHandles = ddmHandles(publisher, options);
    auto const receiverHandles = ddmHandles(receiver, options);
    verifyDdmClassDimensions(
        publisher,
        options,
        publisherHandles,
        "timestamped regional delay-subscription evaluation");
    require(
        publisher.rtiAmbassador().getDelaySubscriptionEvaluationSwitch() ==
                delaySubscriptionEvaluationEnabled &&
            receiver.rtiAmbassador().getDelaySubscriptionEvaluationSwitch() ==
                delaySubscriptionEvaluationEnabled,
        "timestamped regional delay-subscription switch did not match the adapter FOM configuration");

    rti::AttributeHandleSet const publisherAttributes{publisherHandles.attribute};
    rti::AttributeHandleSet const receiverAttributes{receiverHandles.attribute};
    publisher.rtiAmbassador().publishObjectClassAttributes(
        publisherHandles.objectClass,
        publisherAttributes);
    publisher.rtiAmbassador().changeDefaultAttributeOrderType(
        publisherHandles.objectClass,
        publisherAttributes,
        rti::TIMESTAMP);
    receiver.rtiAmbassador().setConveyRegionDesignatorSetsSwitch(true);
    require(
        receiver.rtiAmbassador().getConveyRegionDesignatorSetsSwitch(),
        "timestamped regional delay-subscription receiver did not enable region designator callbacks");

    auto const sourceRegion = createDdmRegion(
        publisher,
        publisherHandles,
        1UL,
        5UL,
        1UL,
        5UL);
    auto const receiverRegion = createDdmRegion(
        receiver,
        receiverHandles,
        2UL,
        6UL,
        2UL,
        6UL);
    rti::AttributeHandleSetRegionHandleSetPairVector const publisherPair{{
        publisherAttributes,
        rti::RegionHandleSet{sourceRegion},
    }};
    rti::AttributeHandleSetRegionHandleSetPairVector const receiverPair{{
        receiverAttributes,
        rti::RegionHandleSet{receiverRegion},
    }};
    auto const object = publisher.rtiAmbassador().registerObjectInstanceWithRegions(
        publisherHandles.objectClass,
        publisherPair);
    require(
        object.isValid(),
        "timestamped regional delay-subscription registration returned an invalid object handle");
    receiver.rtiAmbassador().subscribeObjectClassAttributesWithRegions(
        receiverHandles.objectClass,
        receiverPair);
    waitForSessions(
        {&publisher, &receiver},
        [&] { return receiver.recorder().hasDiscovery(object); },
        options,
        "timestamped regional delay-subscription object discovery");
    require(
        receiver.rtiAmbassador().getKnownObjectClassHandle(object) ==
            receiverHandles.objectClass,
        "timestamped regional delay-subscription discovery did not establish the known object");
    receiver.rtiAmbassador().unsubscribeObjectClassAttributesWithRegions(
        receiverHandles.objectClass,
        receiverPair);

    auto publisherTime = makeTimeContext(publisher);
    auto receiverTime = makeTimeContext(receiver);
    enableTimestampedRoles(
        publisher,
        receiver,
        publisherTime,
        receiverTime,
        options,
        "timestamped regional delay-subscription evaluation");

    unsigned char const firstValueBytes[] = {0x44, 0x53, 0x52, 0x31};
    unsigned char const secondValueBytes[] = {0x44, 0x53, 0x52, 0x32};
    rti::AttributeHandleValueMap firstValues;
    firstValues.emplace(
        publisherHandles.attribute,
        rti::VariableLengthData(firstValueBytes, sizeof(firstValueBytes)));
    rti::AttributeHandleValueMap secondValues;
    secondValues.emplace(
        publisherHandles.attribute,
        rti::VariableLengthData(secondValueBytes, sizeof(secondValueBytes)));

    auto assertReflection = [&](TimedReflectionRecord const& reflection,
                                std::vector<std::uint8_t> const& expectedValue,
                                std::vector<std::uint8_t> const& expectedTime,
                                std::vector<std::uint8_t> const& expectedRetraction,
                                std::string const& description) {
      require(
          reflection.object == object && reflection.values.size() == 1U &&
              reflection.values.count(receiverHandles.attribute) == 1U &&
              copyBytes(reflection.values.at(receiverHandles.attribute)) == expectedValue,
          description + " returned the wrong regional object, attribute, or value");
      require(
          reflection.tag.empty() && reflection.time == expectedTime &&
              !reflection.timeText.empty() &&
              reflection.retractionPresent &&
              reflection.retraction == expectedRetraction,
          description + " did not preserve timestamp, tag, or retraction metadata");
      require(
          reflection.producer == publisher.federateHandle() &&
              reflection.transportation.isValid() &&
              !receiver.rtiAmbassador()
                   .getTransportationTypeName(reflection.transportation)
                   .empty(),
          description + " returned the wrong producer or an unresolvable transport");
      require(
          reflection.sentOrder == rti::TIMESTAMP &&
              reflection.receivedOrder == rti::TIMESTAMP,
          description + " returned the wrong order metadata");
      require(
          reflection.regions.has_value() && reflection.regions->size() == 1U &&
              reflection.regions->count(sourceRegion) == 1U,
          description + " omitted the source region designator set");
    };

    auto const firstTime = timeAfter(
        *publisherTime.factory,
        *publisherTime.initial,
        *publisherTime.epsilon,
        2U);
    auto const firstReceiverTime = timeAfter(
        *receiverTime.factory,
        *receiverTime.initial,
        *receiverTime.epsilon,
        2U);
    auto const firstRetraction = publisher.rtiAmbassador().updateAttributeValues(
        object,
        firstValues,
        rti::VariableLengthData{},
        *firstTime);
    require(
        !delaySubscriptionEvaluationEnabled || firstRetraction.isValid(),
        "enabled timestamped regional attribute update returned an invalid retraction handle");
    require(
        receiver.recorder().timedReflections().empty(),
        "timestamped regional attribute update arrived before the constrained grant");

    // Restore the matching declaration after generation but before the
    // timestamped grant. The Enabled switch requires this passel to be
    // re-evaluated against the current regional subscription.
    receiver.rtiAmbassador().subscribeObjectClassAttributesWithRegions(
        receiverHandles.objectClass,
        receiverPair);
    receiver.rtiAmbassador().timeAdvanceRequest(*firstReceiverTime);
    publisher.rtiAmbassador().timeAdvanceRequest(*firstTime);
    auto const minimumFirstCount = delaySubscriptionEvaluationEnabled ? 1U : 0U;
    waitForSessions(
        {&publisher, &receiver},
        [&] {
          return publisher.recorder().timeAdvanceGrants().size() >= 1U &&
              receiver.recorder().timeAdvanceGrants().size() >= 1U &&
              receiver.recorder().timedReflections().size() >= minimumFirstCount;
        },
        options,
        "timestamped regional delay-subscription first grant");
    auto const firstDeliveryCount = receiver.recorder().timedReflections().size();
    require(
        delaySubscriptionEvaluationEnabled
            ? firstDeliveryCount == 1U
            : firstDeliveryCount <= 1U,
        "timestamped regional delay-subscription evaluation delivered an unexpected first-reflection count");
    if (firstDeliveryCount != 0U) {
      assertReflection(
          receiver.recorder().timedReflections().front(),
          std::vector<std::uint8_t>(
              firstValueBytes,
              firstValueBytes + sizeof(firstValueBytes)),
          encodeTime(*firstTime),
          copyBytes(firstRetraction.encode()),
          "first timestamped regional attribute reflection");
    }

    auto const secondTime = timeAfter(
        *publisherTime.factory,
        *firstTime,
        *publisherTime.epsilon,
        1U);
    auto const secondReceiverTime = timeAfter(
        *receiverTime.factory,
        *firstReceiverTime,
        *receiverTime.epsilon,
        1U);
    auto const secondRetraction = publisher.rtiAmbassador().updateAttributeValues(
        object,
        secondValues,
        rti::VariableLengthData{},
        *secondTime);
    require(
        secondRetraction.isValid(),
        "timestamped regional attribute update returned an invalid second retraction handle");
    receiver.rtiAmbassador().unsubscribeObjectClassAttributesWithRegions(
        receiverHandles.objectClass,
        receiverPair);
    receiver.rtiAmbassador().timeAdvanceRequest(*secondReceiverTime);
    publisher.rtiAmbassador().timeAdvanceRequest(*secondTime);
    waitForSessions(
        {&publisher, &receiver},
        [&] {
          return publisher.recorder().timeAdvanceGrants().size() >= 2U &&
              receiver.recorder().timeAdvanceGrants().size() >= 2U;
        },
        options,
        "timestamped regional delay-subscription second grant");
    require(
        receiver.recorder().timedReflections().size() == firstDeliveryCount,
        "timestamped regional attribute update was delivered after unsubscribe");

    receiver.rtiAmbassador().disableTimeConstrained();
    publisher.rtiAmbassador().disableTimeRegulation();
    publisher.rtiAmbassador().unassociateRegionsForUpdates(object, publisherPair);
    receiver.rtiAmbassador().deleteRegion(receiverRegion);
    publisher.rtiAmbassador().deleteRegion(sourceRegion);
    receiver.resign(rti::NO_ACTION);
    publisher.resign(rti::DELETE_OBJECTS);
    publisher.rtiAmbassador().destroyFederationExecution(federation);
    receiver.disconnect();
    publisher.disconnect();
  };

  runConfiguration(false);
  runConfiguration(true);
}

void scenarioDelaySubscriptionEvaluationTimestampedRegionalInteraction(
    Options const& options,
    rti::CallbackModel model) {
  require(!options.switchesFom.empty(),
          "switch-declaration FOM input was not supplied by the adapter");
  require(!options.ddmFom.empty(),
          "timestamped regional delay-subscription testing requires an adapter-supplied dimensional FOM");
  require(!options.logicalTimeImplementationName.empty(),
          "timestamped regional delay-subscription testing requires a logical-time implementation");

  auto runConfiguration = [&](bool const delaySubscriptionEvaluationEnabled) {
    Session publisher(options, model, "delay-subscription-regional-interaction-publisher");
    Session receiver(options, model, "delay-subscription-regional-interaction-receiver");
    auto const federation = federationName(
        options,
        delaySubscriptionEvaluationEnabled
            ? "delay-subscription-evaluation-timestamped-regional-interaction-enabled"
            : "delay-subscription-evaluation-timestamped-regional-interaction-disabled");

    publisher.connect();
    receiver.connect();
    std::vector<std::wstring> fomModules{options.ddmFom.wstring()};
    if (delaySubscriptionEvaluationEnabled) {
      fomModules.push_back(options.switchesFom.wstring());
    }
    publisher.rtiAmbassador().createFederationExecution(
        federation,
        fomModules,
        options.logicalTimeImplementationName);
    publisher.join(options.ownerFederateName, options.federateType, federation);
    receiver.join(options.memberFederateName, options.federateType, federation);

    auto const publisherHandles = ddmHandles(publisher, options);
    auto const receiverHandles = ddmHandles(receiver, options);
    verifyDdmClassDimensions(
        publisher,
        options,
        publisherHandles,
        "timestamped regional delay-subscription interaction");
    require(
        publisher.rtiAmbassador().getDelaySubscriptionEvaluationSwitch() ==
                delaySubscriptionEvaluationEnabled &&
            receiver.rtiAmbassador().getDelaySubscriptionEvaluationSwitch() ==
                delaySubscriptionEvaluationEnabled,
        "timestamped regional interaction delay-subscription switch did not match the adapter FOM configuration");

    publisher.rtiAmbassador().publishInteractionClass(
        publisherHandles.interactionClass);
    publisher.rtiAmbassador().changeInteractionOrderType(
        publisherHandles.interactionClass,
        rti::TIMESTAMP);
    receiver.rtiAmbassador().setConveyRegionDesignatorSetsSwitch(true);
    require(
        receiver.rtiAmbassador().getConveyRegionDesignatorSetsSwitch(),
        "timestamped regional delay-subscription receiver did not enable region designator callbacks");

    auto const sourceRegion = createDdmRegion(
        publisher,
        publisherHandles,
        1UL,
        5UL,
        1UL,
        5UL);
    auto const receiverRegion = createDdmRegion(
        receiver,
        receiverHandles,
        2UL,
        6UL,
        2UL,
        6UL);
    rti::RegionHandleSet const sourceRegions{sourceRegion};
    rti::RegionHandleSet const receiverRegions{receiverRegion};

    auto publisherTime = makeTimeContext(publisher);
    auto receiverTime = makeTimeContext(receiver);
    enableTimestampedRoles(
        publisher,
        receiver,
        publisherTime,
        receiverTime,
        options,
        "timestamped regional delay-subscription interaction");

    unsigned char const firstParameterBytes[] = {0x44, 0x53, 0x52, 0x49};
    unsigned char const secondParameterBytes[] = {0x44, 0x53, 0x52, 0x32};
    unsigned char const tagBytes[] = {0x44, 0x53, 0x52, 0x54};
    rti::ParameterHandleValueMap firstParameters;
    firstParameters.emplace(
        publisherHandles.parameter,
        rti::VariableLengthData(
            firstParameterBytes,
            sizeof(firstParameterBytes)));
    rti::ParameterHandleValueMap secondParameters;
    secondParameters.emplace(
        publisherHandles.parameter,
        rti::VariableLengthData(
            secondParameterBytes,
            sizeof(secondParameterBytes)));
    rti::VariableLengthData const userTag(tagBytes, sizeof(tagBytes));

    auto assertInteraction = [&](TimedInteractionRecord const& interaction,
                                  std::vector<std::uint8_t> const& expectedValue,
                                  std::vector<std::uint8_t> const& expectedTime,
                                  rti::MessageRetractionHandle const& expectedRetraction,
                                  std::string const& description) {
      require(
          interaction.interaction == receiverHandles.interactionClass &&
              interaction.parameters.size() == 1U &&
              interaction.parameters.count(receiverHandles.parameter) == 1U &&
              copyBytes(interaction.parameters.at(receiverHandles.parameter)) ==
                  expectedValue,
          description + " returned the wrong regional interaction or parameter value");
      require(
          interaction.tag == copyBytes(userTag) &&
              interaction.time == expectedTime &&
              !interaction.timeText.empty(),
          description + " did not preserve tag or timestamp metadata");
      require(
          interaction.producer == publisher.federateHandle() &&
              interaction.transportation.isValid() &&
              !receiver.rtiAmbassador()
                   .getTransportationTypeName(interaction.transportation)
                   .empty(),
          description + " returned the wrong producer or an unresolvable transport");
      require(
          interaction.sentOrder == rti::TIMESTAMP &&
              interaction.receivedOrder == rti::TIMESTAMP,
          description + " returned the wrong order metadata");
      require(
          interaction.regions.has_value() &&
              interaction.regions->size() == 1U &&
              interaction.regions->count(sourceRegion) == 1U,
          description + " omitted the source region designator set");
      if (expectedRetraction.isValid()) {
        require(
            interaction.retractionPresent &&
                interaction.retraction == copyBytes(expectedRetraction.encode()),
            description + " returned the wrong retraction metadata");
      }
    };

    auto const firstTime = timeAfter(
        *publisherTime.factory,
        *publisherTime.initial,
        *publisherTime.epsilon,
        2U);
    auto const firstReceiverTime = timeAfter(
        *receiverTime.factory,
        *receiverTime.initial,
        *receiverTime.epsilon,
        2U);
    auto const firstRetraction =
        publisher.rtiAmbassador().sendInteractionWithRegions(
            publisherHandles.interactionClass,
            firstParameters,
            sourceRegions,
            userTag,
            *firstTime);
    require(
        !delaySubscriptionEvaluationEnabled || firstRetraction.isValid(),
        "enabled timestamped regional interaction returned an invalid retraction handle");
    require(
        receiver.recorder().timedInteractions().empty(),
        "timestamped regional interaction arrived before the constrained grant");

    // The passel is generated with no active regional declaration. The late
    // matching declaration can make it eligible at the grant only when the
    // creation-time Delay Subscription Evaluation switch is Enabled.
    receiver.rtiAmbassador().subscribeInteractionClassWithRegions(
        receiverHandles.interactionClass,
        receiverRegions);
    receiver.rtiAmbassador().timeAdvanceRequest(*firstReceiverTime);
    publisher.rtiAmbassador().timeAdvanceRequest(*firstTime);
    auto const minimumFirstCount = delaySubscriptionEvaluationEnabled ? 1U : 0U;
    waitForSessions(
        {&publisher, &receiver},
        [&] {
          return publisher.recorder().timeAdvanceGrants().size() >= 1U &&
              receiver.recorder().timeAdvanceGrants().size() >= 1U &&
              receiver.recorder().timedInteractions().size() >= minimumFirstCount;
        },
        options,
        "timestamped regional delay-subscription first grant");
    auto const firstDeliveryCount =
        receiver.recorder().timedInteractions().size();
    require(
        delaySubscriptionEvaluationEnabled
            ? firstDeliveryCount == 1U
            : firstDeliveryCount <= 1U,
        "timestamped regional delay-subscription evaluation delivered an unexpected first-interaction count");
    if (firstDeliveryCount != 0U) {
      auto const firstInteractions = receiver.recorder().timedInteractions();
      assertInteraction(
          firstInteractions.front(),
          std::vector<std::uint8_t>(
              firstParameterBytes,
              firstParameterBytes + sizeof(firstParameterBytes)),
          encodeTime(*firstTime),
          firstRetraction,
          "first timestamped regional interaction");
    }

    auto const secondTime = timeAfter(
        *publisherTime.factory,
        *firstTime,
        *publisherTime.epsilon,
        1U);
    auto const secondReceiverTime = timeAfter(
        *receiverTime.factory,
        *firstReceiverTime,
        *receiverTime.epsilon,
        1U);
    auto const secondRetraction =
        publisher.rtiAmbassador().sendInteractionWithRegions(
            publisherHandles.interactionClass,
            secondParameters,
            sourceRegions,
            userTag,
            *secondTime);
    require(
        secondRetraction.isValid(),
        "timestamped regional interaction returned an invalid second retraction handle");
    receiver.rtiAmbassador().unsubscribeInteractionClassWithRegions(
        receiverHandles.interactionClass,
        receiverRegions);
    receiver.rtiAmbassador().timeAdvanceRequest(*secondReceiverTime);
    publisher.rtiAmbassador().timeAdvanceRequest(*secondTime);
    waitForSessions(
        {&publisher, &receiver},
        [&] {
          return publisher.recorder().timeAdvanceGrants().size() >= 2U &&
              receiver.recorder().timeAdvanceGrants().size() >= 2U;
        },
        options,
        "timestamped regional delay-subscription second grant");
    require(
        receiver.recorder().timedInteractions().size() == firstDeliveryCount,
        "timestamped regional interaction was delivered after unsubscribe");

    receiver.rtiAmbassador().disableTimeConstrained();
    publisher.rtiAmbassador().disableTimeRegulation();
    publisher.rtiAmbassador().unpublishInteractionClass(
        publisherHandles.interactionClass);
    receiver.rtiAmbassador().deleteRegion(receiverRegion);
    publisher.rtiAmbassador().deleteRegion(sourceRegion);
    receiver.resign(rti::NO_ACTION);
    publisher.resign(rti::NO_ACTION);
    publisher.rtiAmbassador().destroyFederationExecution(federation);
    receiver.disconnect();
    publisher.disconnect();
  };

  runConfiguration(false);
  runConfiguration(true);
}

} // namespace hla_rti_cpp_tck::detail
