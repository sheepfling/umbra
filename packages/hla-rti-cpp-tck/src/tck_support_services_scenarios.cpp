#include "tck_main_scenarios.hpp"

namespace hla_rti_cpp_tck::detail {

void verifyResignExceptionPrecedence(
    Options const& options,
    rti::CallbackModel model) {
  Session disconnected(options, model, "exception-precedence-disconnected");
  auto const invalidResignAction = static_cast<rti::ResignAction>(99);
  requireException(
      [&] {
        disconnected.rtiAmbassador().resignFederationExecution(
            invalidResignAction);
      },
      L"NotConnected",
      "resigning while disconnected with an invalid resign action");
}

void verifyDisconnectedFederationManagementServices(
    Options const& options,
    rti::CallbackModel model) {
  Session disconnected(options, model, "disconnected-federation-management");
  auto const federation = federationName(
      options, "disconnected-federation-management");
  requireException(
      [&] {
        disconnected.rtiAmbassador().createFederationExecution(
            federation, options.fom);
      },
      L"NotConnected",
      "creating a federation execution before Connect");
  requireException(
      [&] {
        disconnected.rtiAmbassador().joinFederationExecution(
            L"disconnected-member", federation);
      },
      L"NotConnected",
      "joining a federation execution before Connect");
  requireException(
      [&] {
        disconnected.rtiAmbassador().destroyFederationExecution(federation);
      },
      L"NotConnected",
      "destroying a federation execution before Connect");
}

void scenarioFederateApplicationMultipleExecutions(
    Options const& options,
    rti::CallbackModel model) {
  Session creator(options, model, "owner");
  Session firstOne(options, model, "multi-execution-first-one");
  Session firstTwo(options, model, "multi-execution-first-two");
  Session secondOne(options, model, "multi-execution-second-one");
  Session secondTwo(options, model, "multi-execution-second-two");
  auto const firstFederation =
      federationName(options, "multiple-federate-executions-first");
  auto const secondFederation = firstFederation + L"-second";

  creator.connect();
  firstOne.connect();
  firstTwo.connect();
  secondOne.connect();
  secondTwo.connect();
  creator.rtiAmbassador().createFederationExecution(
      firstFederation,
      options.fom.wstring(),
      options.logicalTimeImplementationName);
  creator.rtiAmbassador().createFederationExecution(
      secondFederation,
      options.fom.wstring(),
      options.logicalTimeImplementationName);

  firstOne.join(L"multi-execution-first-one", options.federateType, firstFederation);
  secondOne.join(L"multi-execution-second-one", options.federateType, secondFederation);
  firstTwo.join(L"multi-execution-first-two", options.federateType, firstFederation);
  secondTwo.join(L"multi-execution-second-two", options.federateType, secondFederation);

  require(
      firstOne.federateHandle().isValid() &&
          secondOne.federateHandle().isValid() &&
          firstTwo.federateHandle().isValid() &&
          secondTwo.federateHandle().isValid(),
      "one application could not join federates across both federation executions");

  firstTwo.resign(rti::NO_ACTION);
  firstTwo.join(
      L"multi-execution-first-two-rejoined",
      options.federateType,
      secondFederation);
  require(
      firstTwo.federateHandle().isValid(),
      "an application could not reuse an RTIambassador in another execution");

  secondOne.resign(rti::NO_ACTION);
  firstOne.resign(rti::NO_ACTION);
  secondTwo.resign(rti::NO_ACTION);
  firstTwo.resign(rti::NO_ACTION);
  creator.rtiAmbassador().destroyFederationExecution(firstFederation);
  creator.rtiAmbassador().destroyFederationExecution(secondFederation);
  creator.disconnect();
  firstOne.disconnect();
  firstTwo.disconnect();
  secondOne.disconnect();
  secondTwo.disconnect();
}

void scenarioFederateApplicationMultipleExecutionsContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioFederateApplicationMultipleExecutions(options, model);
}

void scenarioFederateSaveInitiationEligibility(
    Options const& options,
    rti::CallbackModel model) {
  Session requester(options, model, "owner");
  Session member(options, model, "save-initiation-eligibility-member");
  auto const federation =
      federationName(options, "save-initiation-unconstrained-federate");
  auto const saveLabel = L"tck-unconstrained-federate-save";

  requester.connect();
  member.connect();
  requester.rtiAmbassador().createFederationExecution(
      federation,
      options.fom.wstring(),
      options.logicalTimeImplementationName);
  requester.join(options.ownerFederateName, options.federateType, federation);
  member.join(options.memberFederateName, options.federateType, federation);
  requester.rtiAmbassador().requestFederationSave(saveLabel);

  waitForSessions(
      {&requester, &member},
      [&] {
        return requester.federateSaveInitiations().size() >= 1U &&
            member.federateSaveInitiations().size() >= 1U;
      },
      options,
      "Initiate Federate Save for every unconstrained joined federate");
  require(
      requester.federateSaveInitiations() ==
              std::vector<std::wstring>{saveLabel} &&
          member.federateSaveInitiations() ==
          std::vector<std::wstring>{saveLabel},
      "the untimed save did not initiate every joined unconstrained federate");

  requester.rtiAmbassador().federateSaveBegun();
  member.rtiAmbassador().federateSaveBegun();
  requester.rtiAmbassador().federateSaveComplete();
  member.rtiAmbassador().federateSaveComplete();
  waitForSessions(
      {&requester, &member},
      [&] {
        return requester.federationSavedCount() == 1U &&
            member.federationSavedCount() == 1U;
      },
      options,
      "federation-saved callbacks after all unconstrained federates complete");
  require(
      requester.federationNotSavedReasons().empty() &&
          member.federationNotSavedReasons().empty(),
      "the untimed save unexpectedly failed for a joined federate");

  member.resign(rti::NO_ACTION);
  requester.resign(rti::NO_ACTION);
  requester.rtiAmbassador().destroyFederationExecution(federation);
  member.disconnect();
  requester.disconnect();
}

void scenarioFederateSaveInitiationTimeConstrainedEligibility(
    Options const& options,
    rti::CallbackModel model) {
  Session requester(options, model, "owner");
  Session member(options, model, "time-constrained-save-eligibility-member");
  Session secondMember(options, model, "time-constrained-save-eligibility-second-member");
  auto const federation =
      federationName(options, "time-constrained-save-initiation-eligibility");
  auto const saveLabel = L"tck-time-constrained-federate-save";

  requester.connect();
  member.connect();
  secondMember.connect();
  requester.rtiAmbassador().createFederationExecution(
      federation,
      options.fom.wstring(),
      options.logicalTimeImplementationName);
  requester.join(options.ownerFederateName, options.federateType, federation);
  member.join(options.memberFederateName, options.federateType, federation);
  secondMember.join(
      options.memberFederateName + L"-second",
      options.federateType,
      federation);

  auto requesterTime = makeTimeContext(requester);
  auto memberTime = makeTimeContext(member);
  auto secondMemberTime = makeTimeContext(secondMember);
  require(
      requesterTime.factory->getName() == memberTime.factory->getName() &&
          requesterTime.factory->getName() == secondMemberTime.factory->getName(),
      "save-eligibility members selected different logical-time factories");

  requester.rtiAmbassador().enableTimeRegulation(*requesterTime.epsilon);
  waitFor(
      requester,
      [&] { return requester.recorder().timeRegulationEnabled().size() == 1U; },
      options,
      "save-eligibility time-regulation callback");
  member.rtiAmbassador().enableTimeConstrained();
  waitFor(
      member,
      [&] { return member.recorder().timeConstrainedEnabled().size() == 1U; },
      options,
      "save-eligibility time-constrained callback");
  secondMember.rtiAmbassador().enableTimeConstrained();
  waitFor(
      secondMember,
      [&] { return secondMember.recorder().timeConstrainedEnabled().size() == 1U; },
      options,
      "second save-eligibility time-constrained callback");

  requester.rtiAmbassador().requestFederationSave(saveLabel);
  requester.pump();
  member.pump();
  secondMember.pump();
  require(
      requester.federateSaveInitiations().empty() &&
            member.federateSaveInitiations().empty() &&
            secondMember.federateSaveInitiations().empty(),
        "untimed save began before any time-constrained member was Time Advancing");

  auto memberTarget = timeAfter(
      *memberTime.factory, *memberTime.initial, *memberTime.epsilon, 5U);
  member.rtiAmbassador().timeAdvanceRequest(*memberTarget);
  requester.pump();
  member.pump();
  secondMember.pump();
  require(
      requester.federateSaveInitiations().empty() &&
          member.federateSaveInitiations().empty() &&
          secondMember.federateSaveInitiations().empty() &&
          member.recorder().timeAdvanceGrants().empty(),
      "the untimed save began before every constrained TAR was accepted");

  auto secondMemberTarget = timeAfter(
      *secondMemberTime.factory,
      *secondMemberTime.initial,
      *secondMemberTime.epsilon,
      5U);
  secondMember.rtiAmbassador().timeAdvanceRequest(*secondMemberTarget);
  waitForSessions(
      {&requester, &member, &secondMember},
      [&] {
        return requester.federateSaveInitiations().size() == 1U &&
            secondMember.federateSaveInitiations().size() == 1U &&
            member.federateSaveInitiations().size() == 1U;
      },
      options,
      "all constrained save initiations after both TARs");
  require(
      requester.federateSaveInitiations() == std::vector<std::wstring>{saveLabel} &&
          member.federateSaveInitiations() == std::vector<std::wstring>{saveLabel} &&
          secondMember.federateSaveInitiations() == std::vector<std::wstring>{saveLabel},
      "the untimed save did not instruct every joined federate exactly once");
  require(
      member.recorder().callbackOrder() ==
              std::vector<std::string>{"federate-save-initiate"} &&
          secondMember.recorder().callbackOrder() ==
              std::vector<std::string>{"federate-save-initiate"} &&
          member.recorder().timeAdvanceGrants().empty() &&
          secondMember.recorder().timeAdvanceGrants().empty(),
      "constrained save initiation did not occur while both members were Time Advancing");

  requester.rtiAmbassador().federateSaveBegun();
  member.rtiAmbassador().federateSaveBegun();
  secondMember.rtiAmbassador().federateSaveBegun();
  requester.rtiAmbassador().federateSaveComplete();
  member.rtiAmbassador().federateSaveComplete();
  secondMember.rtiAmbassador().federateSaveComplete();
  waitForSessions(
      {&requester, &member, &secondMember},
      [&] {
        return requester.federationSavedCount() == 1U &&
            member.federationSavedCount() == 1U &&
            secondMember.federationSavedCount() == 1U;
      },
      options,
      "Federation Saved after constrained save initiation completes");
  require(
      requester.federationNotSavedReasons().empty() &&
          member.federationNotSavedReasons().empty() &&
          secondMember.federationNotSavedReasons().empty(),
      "the untimed constrained-member save unexpectedly failed");

  auto requesterTarget = timeAfter(
      *requesterTime.factory,
      *requesterTime.initial,
      *requesterTime.epsilon,
      10U);
  requester.rtiAmbassador().timeAdvanceRequest(*requesterTarget);
  waitForSessions(
      {&requester, &member, &secondMember},
      [&] {
        return requester.recorder().timeAdvanceGrants().size() == 1U &&
            member.recorder().timeAdvanceGrants().size() == 1U &&
            secondMember.recorder().timeAdvanceGrants().size() == 1U;
      },
      options,
      "time grants after the save completes");

  secondMember.resign(rti::NO_ACTION);
  member.resign(rti::NO_ACTION);
  requester.resign(rti::NO_ACTION);
  requester.rtiAmbassador().destroyFederationExecution(federation);
  secondMember.disconnect();
  member.disconnect();
  requester.disconnect();
}

void scenarioHandleLookups(Options const& options, rti::CallbackModel model) {
  Session decoderLifecycle(options, model, "decoder-lifecycle");
  rti::ObjectInstanceHandle const invalidLifecycleObject;
  rti::AttributeHandle const invalidLifecycleAttribute;
  auto requireSupportLookupLifecycle = [&](std::wstring const& expected,
                                            std::string const& phase) {
    requireException(
        [&] {
          static_cast<void>(decoderLifecycle.rtiAmbassador().getFederateHandle(
              L"tck-lifecycle-missing-federate"));
        },
        expected,
        "looking up a federate handle " + phase);
    requireException(
        [&] {
          static_cast<void>(decoderLifecycle.rtiAmbassador().getFederateName(
              rti::FederateHandle{}));
        },
        expected,
        "looking up a federate name " + phase);
    requireException(
        [&] {
          static_cast<void>(decoderLifecycle.rtiAmbassador().getObjectClassHandle(
              options.objectClassName));
        },
        expected,
        "looking up an object class handle " + phase);
    requireException(
        [&] {
          static_cast<void>(decoderLifecycle.rtiAmbassador().getObjectClassName(
              rti::ObjectClassHandle{}));
        },
        expected,
        "looking up an object class name " + phase);
    requireException(
        [&] {
          static_cast<void>(decoderLifecycle.rtiAmbassador().getKnownObjectClassHandle(
              invalidLifecycleObject));
        },
        expected,
        "looking up a known object class " + phase);
    requireException(
        [&] {
          static_cast<void>(decoderLifecycle.rtiAmbassador().getObjectInstanceHandle(
              L"tck-lifecycle-missing-object"));
        },
        expected,
        "looking up an object instance handle " + phase);
    requireException(
        [&] {
          static_cast<void>(decoderLifecycle.rtiAmbassador().getObjectInstanceName(
              invalidLifecycleObject));
        },
        expected,
        "looking up an object instance name " + phase);
    requireException(
        [&] {
          static_cast<void>(decoderLifecycle.rtiAmbassador().getAttributeHandle(
              rti::ObjectClassHandle{},
              options.attributeName));
        },
        expected,
        "looking up an attribute handle " + phase);
    requireException(
        [&] {
          static_cast<void>(decoderLifecycle.rtiAmbassador().getAttributeName(
              rti::ObjectClassHandle{},
              invalidLifecycleAttribute));
        },
        expected,
        "looking up an attribute name " + phase);
    requireException(
        [&] {
          static_cast<void>(decoderLifecycle.rtiAmbassador().getInteractionClassHandle(
              options.interactionClassName));
        },
        expected,
        "looking up an interaction class handle " + phase);
    requireException(
        [&] {
          static_cast<void>(decoderLifecycle.rtiAmbassador().getInteractionClassName(
              rti::InteractionClassHandle{}));
        },
        expected,
        "looking up an interaction class name " + phase);
    requireException(
        [&] {
          static_cast<void>(decoderLifecycle.rtiAmbassador().getParameterHandle(
              rti::InteractionClassHandle{},
              options.parameterName));
        },
        expected,
        "looking up a parameter handle " + phase);
    requireException(
        [&] {
          static_cast<void>(decoderLifecycle.rtiAmbassador().getParameterName(
              rti::InteractionClassHandle{},
              rti::ParameterHandle{}));
        },
        expected,
        "looking up a parameter name " + phase);
    requireException(
        [&] {
          static_cast<void>(decoderLifecycle.rtiAmbassador().getAvailableDimensionsForObjectClass(
              rti::ObjectClassHandle{}));
        },
        expected,
        "looking up object-class dimensions " + phase);
    requireException(
        [&] {
          static_cast<void>(decoderLifecycle.rtiAmbassador()
                                .getAvailableDimensionsForInteractionClass(
                                    rti::InteractionClassHandle{}));
        },
        expected,
        "looking up interaction-class dimensions " + phase);
    requireException(
        [&] {
          static_cast<void>(decoderLifecycle.rtiAmbassador().getDimensionHandle(
              L"TckDimension"));
        },
        expected,
        "looking up a dimension handle " + phase);
    requireException(
        [&] {
          static_cast<void>(decoderLifecycle.rtiAmbassador().getDimensionName(
              rti::DimensionHandle{}));
        },
        expected,
        "looking up a dimension name " + phase);
    requireException(
        [&] {
          static_cast<void>(decoderLifecycle.rtiAmbassador().getDimensionUpperBound(
              rti::DimensionHandle{}));
        },
        expected,
        "looking up a dimension upper bound " + phase);
    requireException(
        [&] {
          static_cast<void>(decoderLifecycle.rtiAmbassador().getOrderType(
              L"Receive"));
        },
        expected,
        "looking up an order type " + phase);
    requireException(
        [&] {
          static_cast<void>(decoderLifecycle.rtiAmbassador().getOrderName(
              rti::RECEIVE));
        },
        expected,
        "looking up an order name " + phase);
    requireException(
        [&] {
          static_cast<void>(decoderLifecycle.rtiAmbassador().getTransportationTypeHandle(
              L"HLAreliable"));
        },
        expected,
        "looking up a transportation type " + phase);
    requireException(
        [&] {
          static_cast<void>(decoderLifecycle.rtiAmbassador().getTransportationTypeName(
              rti::TransportationTypeHandle{}));
        },
        expected,
        "looking up a transportation name " + phase);
    requireException(
        [&] {
          static_cast<void>(decoderLifecycle.rtiAmbassador().normalizeServiceGroup(
              rti::SUPPORT_SERVICES));
        },
        expected,
        "normalizing a service group " + phase);
    requireException(
        [&] {
          static_cast<void>(decoderLifecycle.rtiAmbassador().getUpdateRateValue(
              L"High"));
        },
        expected,
        "looking up an update rate " + phase);
    requireException(
        [&] {
          static_cast<void>(decoderLifecycle.rtiAmbassador().getUpdateRateValueForAttribute(
              invalidLifecycleObject,
              invalidLifecycleAttribute));
        },
        expected,
        "looking up an instance update rate " + phase);
  };
  auto requireDecoderLifecycle = [&](std::wstring const& expected,
                                     std::string const& phase) {
    requireException(
        [&] {
          static_cast<void>(decoderLifecycle.rtiAmbassador().decodeFederateHandle(
              rti::VariableLengthData{}));
        },
        expected,
        "federate-handle decoder " + phase);
    requireException(
        [&] {
          static_cast<void>(decoderLifecycle.rtiAmbassador().decodeObjectClassHandle(
              rti::VariableLengthData{}));
        },
        expected,
        "object-class decoder " + phase);
    requireException(
        [&] {
          static_cast<void>(decoderLifecycle.rtiAmbassador().decodeInteractionClassHandle(
              rti::VariableLengthData{}));
        },
        expected,
        "interaction-class decoder " + phase);
    requireException(
        [&] {
          static_cast<void>(decoderLifecycle.rtiAmbassador().decodeObjectInstanceHandle(
              rti::VariableLengthData{}));
        },
        expected,
        "object-instance decoder " + phase);
    requireException(
        [&] {
          static_cast<void>(decoderLifecycle.rtiAmbassador().decodeAttributeHandle(
              rti::VariableLengthData{}));
        },
        expected,
        "attribute decoder " + phase);
    requireException(
        [&] {
          static_cast<void>(decoderLifecycle.rtiAmbassador().decodeParameterHandle(
              rti::VariableLengthData{}));
        },
        expected,
        "parameter decoder " + phase);
    requireException(
        [&] {
          static_cast<void>(decoderLifecycle.rtiAmbassador().decodeDimensionHandle(
              rti::VariableLengthData{}));
        },
        expected,
        "dimension decoder " + phase);
    requireException(
        [&] {
          static_cast<void>(decoderLifecycle.rtiAmbassador().decodeMessageRetractionHandle(
              rti::VariableLengthData{}));
        },
        expected,
        "message-retraction decoder " + phase);
  };
  requireSupportLookupLifecycle(L"NotConnected", "before connect");
  requireDecoderLifecycle(L"NotConnected", "before connect");
  decoderLifecycle.connect();
  requireSupportLookupLifecycle(
      L"FederateNotExecutionMember",
      "after connect before join");
  requireDecoderLifecycle(L"FederateNotExecutionMember", "after connect before join");
  decoderLifecycle.disconnect();

  Session owner(options, model, "owner");
  Session member(options, model, "member");
  auto const federation = federationName(options, "lookups");
  connectAndJoin(owner, member, options, federation, options.fom);

  rti::ObjectClassHandle objectClass;
  rti::AttributeHandle attribute;
  rti::InteractionClassHandle interactionClass;
  rti::ParameterHandle parameter;
  handles(owner, options, objectClass, attribute, interactionClass, parameter);
  require(owner.rtiAmbassador().getObjectClassName(objectClass) == options.objectClassName,
          "Object class handle-to-name lookup did not round-trip");
  require(owner.rtiAmbassador().getAttributeName(objectClass, attribute) == options.attributeName,
          "Attribute handle-to-name lookup did not round-trip");
  require(owner.rtiAmbassador().getInteractionClassName(interactionClass) ==
              options.interactionClassName,
          "Interaction class handle-to-name lookup did not round-trip");
  require(owner.rtiAmbassador().getParameterName(interactionClass, parameter) ==
              options.parameterName,
          "Parameter handle-to-name lookup did not round-trip");
  require(owner.rtiAmbassador().getOrderType(L"Receive") == rti::RECEIVE,
          "Receive order name lookup did not resolve");
  require(owner.rtiAmbassador().getOrderName(rti::RECEIVE) == L"Receive",
          "Receive order handle lookup did not round-trip");
  auto const reliable = owner.rtiAmbassador().getTransportationTypeHandle(L"HLAreliable");
  require(reliable.isValid(), "Reliable transportation lookup returned an invalid handle");
  require(owner.rtiAmbassador().getTransportationTypeName(reliable) == L"HLAreliable",
          "Reliable transportation lookup did not round-trip");

  requireException(
      [&] {
        static_cast<void>(owner.rtiAmbassador().getFederateHandle(L"missing-federate"));
      },
      L"NameNotFound",
      "missing federate name lookup");
  requireException(
      [&] { static_cast<void>(owner.rtiAmbassador().getFederateName(rti::FederateHandle{})); },
      L"InvalidFederateHandle",
      "invalid federate handle lookup");
  requireException(
      [&] {
        static_cast<void>(owner.rtiAmbassador().getObjectClassHandle(
            L"HLAobjectRoot.Missing"));
      },
      L"NameNotFound",
      "missing object-class name lookup");
  requireException(
      [&] { static_cast<void>(owner.rtiAmbassador().getObjectClassName(
                   rti::ObjectClassHandle{})); },
      L"InvalidObjectClassHandle",
      "invalid object-class handle lookup");
  requireException(
      [&] {
        static_cast<void>(owner.rtiAmbassador().getKnownObjectClassHandle(
            rti::ObjectInstanceHandle{}));
      },
      L"ObjectInstanceNotKnown",
      "unknown object instance class lookup");
  requireException(
      [&] {
        static_cast<void>(owner.rtiAmbassador().getObjectInstanceHandle(
            L"missing-object-instance"));
      },
      L"ObjectInstanceNotKnown",
      "missing object instance name lookup");
  requireException(
      [&] { static_cast<void>(owner.rtiAmbassador().getObjectInstanceName(
                   rti::ObjectInstanceHandle{})); },
      L"ObjectInstanceNotKnown",
      "invalid object instance handle lookup");
  requireException(
      [&] {
        static_cast<void>(owner.rtiAmbassador().getAttributeHandle(
            rti::ObjectClassHandle{}, options.attributeName));
      },
      L"InvalidObjectClassHandle",
      "attribute lookup with an invalid object-class handle");
  requireException(
      [&] {
        static_cast<void>(owner.rtiAmbassador().getAttributeHandle(
            objectClass, L"MissingAttribute"));
      },
      L"NameNotFound",
      "missing attribute name lookup");
  requireException(
      [&] {
        static_cast<void>(owner.rtiAmbassador().getAttributeName(
            objectClass, rti::AttributeHandle{}));
      },
      L"InvalidAttributeHandle",
      "invalid attribute handle lookup");
  requireException(
      [&] {
        static_cast<void>(owner.rtiAmbassador().getUpdateRateValue(
            L"missing-update-rate"));
      },
      L"InvalidUpdateRateDesignator",
      "missing update-rate lookup");
  requireException(
      [&] {
        static_cast<void>(owner.rtiAmbassador().getUpdateRateValueForAttribute(
            rti::ObjectInstanceHandle{}, attribute));
      },
      L"ObjectInstanceNotKnown",
      "update-rate lookup for an unknown object");
  requireException(
      [&] {
        static_cast<void>(owner.rtiAmbassador().getInteractionClassHandle(
            L"HLAinteractionRoot.Missing"));
      },
      L"NameNotFound",
      "missing interaction-class name lookup");
  requireException(
      [&] { static_cast<void>(owner.rtiAmbassador().getInteractionClassName(
                   rti::InteractionClassHandle{})); },
      L"InvalidInteractionClassHandle",
      "invalid interaction-class handle lookup");
  requireException(
      [&] {
        static_cast<void>(owner.rtiAmbassador().getParameterHandle(
            rti::InteractionClassHandle{}, options.parameterName));
      },
      L"InvalidInteractionClassHandle",
      "parameter lookup with an invalid interaction-class handle");
  requireException(
      [&] {
        static_cast<void>(owner.rtiAmbassador().getParameterHandle(
            interactionClass, L"MissingParameter"));
      },
      L"NameNotFound",
      "missing parameter name lookup");
  requireException(
      [&] {
        static_cast<void>(owner.rtiAmbassador().getParameterName(
            interactionClass, rti::ParameterHandle{}));
      },
      L"InvalidParameterHandle",
      "invalid parameter handle lookup");
  requireException(
      [&] {
        static_cast<void>(owner.rtiAmbassador().getOrderType(L"MissingOrder"));
      },
      L"InvalidOrderName",
      "missing order name lookup");
  requireException(
      [&] {
        static_cast<void>(owner.rtiAmbassador().getOrderName(
            static_cast<rti::OrderType>(0x7fU)));
      },
      L"InvalidOrderType",
      "invalid order type lookup");
  requireException(
      [&] {
        static_cast<void>(owner.rtiAmbassador().getTransportationTypeHandle(
            L"MissingTransportation"));
      },
      L"InvalidTransportationName",
      "missing transportation name lookup");
  requireException(
      [&] {
        static_cast<void>(owner.rtiAmbassador().getTransportationTypeName(
            rti::TransportationTypeHandle{}));
      },
      L"InvalidTransportationTypeHandle",
      "invalid transportation handle lookup");
  requireException(
      [&] {
        static_cast<void>(owner.rtiAmbassador().getAvailableDimensionsForObjectClass(
            rti::ObjectClassHandle{}));
      },
      L"InvalidObjectClassHandle",
      "available object-class dimensions with an invalid handle");
  requireException(
      [&] {
        static_cast<void>(owner.rtiAmbassador().getAvailableDimensionsForInteractionClass(
            rti::InteractionClassHandle{}));
      },
      L"InvalidInteractionClassHandle",
      "available interaction-class dimensions with an invalid handle");
  requireException(
      [&] {
        static_cast<void>(owner.rtiAmbassador().normalizeServiceGroup(
            static_cast<rti::ServiceGroup>(0x7fU)));
      },
      L"InvalidServiceGroup",
      "invalid service-group normalization");
  requireException(
      [&] {
        static_cast<void>(owner.rtiAmbassador().normalizeFederateHandle(
            rti::FederateHandle{}));
      },
      L"InvalidFederateHandle",
      "invalid federate normalization");
  requireException(
      [&] {
        static_cast<void>(owner.rtiAmbassador().normalizeObjectClassHandle(
            rti::ObjectClassHandle{}));
      },
      L"InvalidObjectClassHandle",
      "invalid object-class normalization");
  requireException(
      [&] {
        static_cast<void>(owner.rtiAmbassador().normalizeInteractionClassHandle(
            rti::InteractionClassHandle{}));
      },
      L"InvalidInteractionClassHandle",
      "invalid interaction-class normalization");
  requireException(
      [&] {
        static_cast<void>(owner.rtiAmbassador().normalizeObjectInstanceHandle(
            rti::ObjectInstanceHandle{}));
      },
      L"InvalidObjectInstanceHandle",
      "invalid object-instance normalization");

  auto const memberFederate =
      owner.rtiAmbassador().getFederateHandle(options.memberFederateName);
  require(memberFederate == member.federateHandle(),
          "Member federate name-to-handle lookup did not round-trip");

  rti::AttributeHandleSet publishedAttributes;
  publishedAttributes.insert(attribute);
  owner.rtiAmbassador().publishObjectClassAttributes(objectClass, publishedAttributes);
  auto const object = owner.rtiAmbassador().registerObjectInstance(objectClass);
  require(object.isValid(), "Support handle scenario registration returned an invalid handle");

  auto const serviceGroup =
      owner.rtiAmbassador().normalizeServiceGroup(rti::SUPPORT_SERVICES);
  require(
      serviceGroup == owner.rtiAmbassador().normalizeServiceGroup(rti::SUPPORT_SERVICES),
      "Service-group normalization was not stable");
  auto const normalizedFederate =
      owner.rtiAmbassador().normalizeFederateHandle(owner.federateHandle());
  auto const normalizedObjectClass =
      owner.rtiAmbassador().normalizeObjectClassHandle(objectClass);
  auto const normalizedInteraction =
      owner.rtiAmbassador().normalizeInteractionClassHandle(interactionClass);
  auto const normalizedObject =
      owner.rtiAmbassador().normalizeObjectInstanceHandle(object);
  require(
      normalizedFederate ==
          owner.rtiAmbassador().normalizeFederateHandle(owner.federateHandle()),
      "Federate normalization was not stable");
  require(
      normalizedObjectClass ==
          owner.rtiAmbassador().normalizeObjectClassHandle(objectClass),
      "Object-class normalization was not stable");
  require(
      normalizedInteraction ==
          owner.rtiAmbassador().normalizeInteractionClassHandle(interactionClass),
      "Interaction-class normalization was not stable");
  require(
      normalizedObject ==
          owner.rtiAmbassador().normalizeObjectInstanceHandle(object),
      "Object-instance normalization was not stable");

  auto requireHandleStorageSemantics = [&](auto const& handle,
                                           std::string const& description) {
    auto const copiedHandle = handle;
    require(
        handle == copiedHandle && !(handle != copiedHandle) &&
            handle.hash() == copiedHandle.hash() && !(handle < copiedHandle) &&
            !(copiedHandle < handle),
        description + " copy, equality, hash, or ordering semantics were not stable");
    requirePortableHandleEncoding(handle, description);
  };
  requireHandleStorageSemantics(owner.federateHandle(), "Federate handle");
  requireHandleStorageSemantics(objectClass, "Object-class handle");
  requireHandleStorageSemantics(attribute, "Attribute handle");
  requireHandleStorageSemantics(interactionClass, "Interaction-class handle");
  requireHandleStorageSemantics(parameter, "Parameter handle");
  requireHandleStorageSemantics(object, "Object-instance handle");

  auto requireHandleRoundTrip = [&](auto const& handle, auto decoder,
                                    std::string const& description) {
    auto const encoded = handle.encode();
    require(encoded.size() == handle.encodedLength(),
            description + " reported an inconsistent encoded length");
    require(encoded.size() > 0U, description + " produced an empty encoding");
    require(decoder(encoded) == handle, description + " did not round-trip");
  };
  requireHandleRoundTrip(
      owner.federateHandle(),
      [&](rti::VariableLengthData const& encoded) {
        return owner.rtiAmbassador().decodeFederateHandle(encoded);
      },
      "Federate handle");
  requireHandleRoundTrip(
      objectClass,
      [&](rti::VariableLengthData const& encoded) {
        return owner.rtiAmbassador().decodeObjectClassHandle(encoded);
      },
      "Object-class handle");
  requireHandleRoundTrip(
      interactionClass,
      [&](rti::VariableLengthData const& encoded) {
        return owner.rtiAmbassador().decodeInteractionClassHandle(encoded);
      },
      "Interaction-class handle");
  requireHandleRoundTrip(
      object,
      [&](rti::VariableLengthData const& encoded) {
        return owner.rtiAmbassador().decodeObjectInstanceHandle(encoded);
      },
      "Object-instance handle");
  requireHandleRoundTrip(
      attribute,
      [&](rti::VariableLengthData const& encoded) {
        return owner.rtiAmbassador().decodeAttributeHandle(encoded);
      },
      "Attribute handle");
  requireHandleRoundTrip(
      parameter,
      [&](rti::VariableLengthData const& encoded) {
        return owner.rtiAmbassador().decodeParameterHandle(encoded);
      },
      "Parameter handle");

  rti::AttributeHandleSet attributeSet;
  attributeSet.insert(attribute);
  auto const copiedAttributeSet = attributeSet;
  rti::AttributeHandleSet assignedAttributeSet;
  assignedAttributeSet = attributeSet;
  auto const erased = copiedAttributeSet.find(attribute);
  auto mutableCopiedAttributeSet = copiedAttributeSet;
  mutableCopiedAttributeSet.erase(attribute);
  require(
      attributeSet.size() == 1U && attributeSet.count(attribute) == 1U &&
          erased != copiedAttributeSet.end() &&
          assignedAttributeSet.size() == 1U &&
          assignedAttributeSet.count(attribute) == 1U &&
          mutableCopiedAttributeSet.empty() &&
          attributeSet.count(attribute) == 1U,
      "AttributeHandleSet copy, assignment, lookup, or erase semantics were not preserved");

  auto requireValueMapCopySemantics = [&](auto const& handle,
                                          auto values,
                                          std::string const& description) {
    std::vector<std::uint8_t> initialBytes{0x61U, 0x62U};
    std::vector<std::uint8_t> replacementBytes{0x71U, 0x72U, 0x73U};
    rti::VariableLengthData initialValue(initialBytes.data(), initialBytes.size());
    values.emplace(handle, initialValue);
    auto const copiedValues = values;
    decltype(values) assignedValues;
    assignedValues = values;

    values.at(handle).setData(replacementBytes.data(), replacementBytes.size());
    require(
        copyBytes(values.at(handle)) == replacementBytes,
        description + " did not retain a replacement value in the source map");
    require(
        copyBytes(copiedValues.at(handle)) == initialBytes &&
            copyBytes(assignedValues.at(handle)) == initialBytes,
        description + " copy or assignment did not preserve independent value storage");
  };
  requireValueMapCopySemantics(
      attribute,
      rti::AttributeHandleValueMap{},
      "AttributeHandleValueMap");
  requireValueMapCopySemantics(
      parameter,
      rti::ParameterHandleValueMap{},
      "ParameterHandleValueMap");

  auto requireEmptyDecodeRejected = [&](auto decoder, std::string const& description) {
    requireException(
        [&] { static_cast<void>(decoder(rti::VariableLengthData{})); },
        L"CouldNotDecode",
        description + " accepted an empty encoding");
  };
  requireEmptyDecodeRejected(
      [&](rti::VariableLengthData const& encoded) {
        return owner.rtiAmbassador().decodeFederateHandle(encoded);
      },
      "Federate handle decoder");
  requireEmptyDecodeRejected(
      [&](rti::VariableLengthData const& encoded) {
        return owner.rtiAmbassador().decodeObjectClassHandle(encoded);
      },
      "Object-class handle decoder");
  requireEmptyDecodeRejected(
      [&](rti::VariableLengthData const& encoded) {
        return owner.rtiAmbassador().decodeInteractionClassHandle(encoded);
      },
      "Interaction-class handle decoder");
  requireEmptyDecodeRejected(
      [&](rti::VariableLengthData const& encoded) {
        return owner.rtiAmbassador().decodeObjectInstanceHandle(encoded);
      },
      "Object-instance handle decoder");
  requireEmptyDecodeRejected(
      [&](rti::VariableLengthData const& encoded) {
        return owner.rtiAmbassador().decodeAttributeHandle(encoded);
      },
      "Attribute handle decoder");
  requireEmptyDecodeRejected(
      [&](rti::VariableLengthData const& encoded) {
        return owner.rtiAmbassador().decodeParameterHandle(encoded);
      },
      "Parameter handle decoder");

  owner.resign(rti::DELETE_OBJECTS);
  member.resign(rti::NO_ACTION);
}



void scenarioStandardOrderAndTransportationLookups(
    Options const& options,
    rti::CallbackModel model) {
  Session lifecycle(options, model, "standard-support-type-lifecycle");
  auto requireLookupLifecycle = [&](std::wstring const& expected,
                                   std::string const& phase) {
    requireException(
        [&] {
          static_cast<void>(lifecycle.rtiAmbassador().getOrderType(L"Receive"));
        },
        expected,
        "looking up Receive order type " + phase);
    requireException(
        [&] {
          static_cast<void>(lifecycle.rtiAmbassador().getOrderName(rti::RECEIVE));
        },
        expected,
        "looking up Receive order name " + phase);
    requireException(
        [&] {
          static_cast<void>(lifecycle.rtiAmbassador().getOrderType(L"TimeStamp"));
        },
        expected,
        "looking up TimeStamp order type " + phase);
    requireException(
        [&] {
          static_cast<void>(lifecycle.rtiAmbassador().getOrderName(rti::TIMESTAMP));
        },
        expected,
        "looking up TimeStamp order name " + phase);
    requireException(
        [&] {
          static_cast<void>(lifecycle.rtiAmbassador().getTransportationTypeHandle(
              L"HLAreliable"));
        },
        expected,
        "looking up reliable transportation type " + phase);
    requireException(
        [&] {
          static_cast<void>(lifecycle.rtiAmbassador().getTransportationTypeName(
              rti::TransportationTypeHandle{}));
        },
        expected,
        "looking up an invalid transportation type " + phase);
  };

  requireLookupLifecycle(L"NotConnected", "before connect");
  lifecycle.connect();
  requireLookupLifecycle(
      L"FederateNotExecutionMember",
      "after connect before join");
  lifecycle.disconnect();

  Session owner(options, model, "standard-support-type-owner");
  Session member(options, model, "standard-support-type-member");
  auto const federation = federationName(options, "standard-support-type-lookups");
  connectAndJoin(owner, member, options, federation, options.fom);

  require(
      owner.rtiAmbassador().getOrderType(L"Receive") == rti::RECEIVE &&
          owner.rtiAmbassador().getOrderType(L"TimeStamp") == rti::TIMESTAMP,
      "standard order names did not resolve to the mandatory order types");
  require(
      owner.rtiAmbassador().getOrderName(rti::RECEIVE) == L"Receive" &&
          owner.rtiAmbassador().getOrderName(rti::TIMESTAMP) == L"TimeStamp",
      "mandatory order types did not round-trip to their standard names");
  require(
      member.rtiAmbassador().getOrderType(L"Receive") == rti::RECEIVE &&
          member.rtiAmbassador().getOrderType(L"TimeStamp") == rti::TIMESTAMP,
      "member order lookups did not resolve to the mandatory order types");

  auto const ownerReliable = owner.rtiAmbassador().getTransportationTypeHandle(
      L"HLAreliable");
  auto const ownerBestEffort = owner.rtiAmbassador().getTransportationTypeHandle(
      L"HLAbestEffort");
  auto const memberReliable = member.rtiAmbassador().getTransportationTypeHandle(
      L"HLAreliable");
  auto const memberBestEffort = member.rtiAmbassador().getTransportationTypeHandle(
      L"HLAbestEffort");
  require(
      ownerReliable.isValid() && ownerBestEffort.isValid() &&
          memberReliable.isValid() && memberBestEffort.isValid(),
      "mandatory transportation lookup returned an invalid handle");
  require(
      ownerReliable != ownerBestEffort && memberReliable != memberBestEffort,
      "mandatory transportation types did not remain distinct");
  require(
      ownerReliable == memberReliable && ownerBestEffort == memberBestEffort,
      "mandatory transportation handles were not stable across federates");
  require(
      owner.rtiAmbassador().getTransportationTypeName(ownerReliable) ==
              L"HLAreliable" &&
          owner.rtiAmbassador().getTransportationTypeName(ownerBestEffort) ==
              L"HLAbestEffort" &&
          member.rtiAmbassador().getTransportationTypeName(memberReliable) ==
              L"HLAreliable" &&
          member.rtiAmbassador().getTransportationTypeName(memberBestEffort) ==
              L"HLAbestEffort",
      "mandatory transportation handles did not round-trip to their standard names");

  requireException(
      [&] {
        static_cast<void>(owner.rtiAmbassador().getOrderType(L"MissingOrder"));
      },
      L"InvalidOrderName",
      "missing order name lookup");
  requireException(
      [&] {
        static_cast<void>(owner.rtiAmbassador().getOrderName(
            static_cast<rti::OrderType>(0x7fU)));
      },
      L"InvalidOrderType",
      "invalid order type lookup");
  requireException(
      [&] {
        static_cast<void>(owner.rtiAmbassador().getTransportationTypeHandle(
            L"MissingTransportation"));
      },
      L"InvalidTransportationName",
      "missing transportation name lookup");
  requireException(
      [&] {
        static_cast<void>(owner.rtiAmbassador().getTransportationTypeName(
            rti::TransportationTypeHandle{}));
      },
      L"InvalidTransportationTypeHandle",
      "invalid transportation handle lookup");

  owner.resign(rti::NO_ACTION);
  member.resign(rti::NO_ACTION);
  owner.rtiAmbassador().destroyFederationExecution(federation);
}

} // namespace hla_rti_cpp_tck::detail
