#include "tck_main_scenarios.hpp"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <exception>
#include <future>
#include <set>
#include <string>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>

namespace hla_rti_cpp_tck::detail {

namespace {

template <typename Handle>
std::string encodedFomHandle(Handle const& handle) {
  auto const encoded = handle.encode();
  return std::string(
      static_cast<char const*>(encoded.data()),
      encoded.size());
}

std::vector<std::string> encodedFddNamedHandles(Session& session) {
  auto& ambassador = session.rtiAmbassador();
  auto const objectClass =
      ambassador.getObjectClassHandle(L"HLAobjectRoot.TckHandleObject");
  auto const objectChild = ambassador.getObjectClassHandle(
      L"HLAobjectRoot.TckHandleObject.TckHandleObjectChild");
  auto const interaction = ambassador.getInteractionClassHandle(
      L"HLAinteractionRoot.TckHandleInteraction");
  auto const interactionChild = ambassador.getInteractionClassHandle(
      L"HLAinteractionRoot.TckHandleInteraction.TckHandleInteractionChild");

  return {
      encodedFomHandle(objectClass),
      encodedFomHandle(objectChild),
      encodedFomHandle(
          ambassador.getAttributeHandle(objectClass, L"TckHandleAttribute")),
      encodedFomHandle(ambassador.getAttributeHandle(
          objectChild, L"TckHandleChildAttribute")),
      encodedFomHandle(interaction),
      encodedFomHandle(interactionChild),
      encodedFomHandle(
          ambassador.getParameterHandle(interaction, L"TckHandleParameter")),
      encodedFomHandle(ambassador.getParameterHandle(
          interactionChild, L"TckHandleChildParameter")),
      encodedFomHandle(ambassador.getDimensionHandle(L"TckHandleDimension")),
      encodedFomHandle(
          ambassador.getDimensionHandle(L"TckHandleDimensionSecond")),
      encodedFomHandle(
          ambassador.getTransportationTypeHandle(L"TckHandleTransportation")),
  };
}

}  // namespace

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

void scenarioAttributeNameLookup(Options const& options, rti::CallbackModel model) {
  Session owner(options, model, "attribute-name-lookup-owner");
  Session member(options, model, "attribute-name-lookup-member");
  auto const federation = federationName(options, "attribute-name-lookup");
  connectAndJoin(owner, member, options, federation, options.fom);
  rti::ObjectClassHandle invalidObjectClass;
  rti::AttributeHandle invalidAttribute;

  auto const objectClass = owner.rtiAmbassador().getObjectClassHandle(options.objectClassName);
  auto const memberObjectClass =
      member.rtiAmbassador().getObjectClassHandle(options.objectClassName);
  auto const attribute =
      owner.rtiAmbassador().getAttributeHandle(objectClass, options.attributeName);
  require(
      objectClass.isValid() && memberObjectClass == objectClass,
      "Object-class lookup returned an invalid or federate-inconsistent handle");
  require(attribute.isValid(), "Attribute handle lookup returned an invalid handle");
  std::vector<std::wstring> attributeNames{
      options.attributeName,
      options.secondaryAttributeName,
      options.objectClassThirdAttributeName,
  };
  for (auto const& attributeName : attributeNames) {
    auto const configuredAttribute =
        owner.rtiAmbassador().getAttributeHandle(objectClass, attributeName);
    require(
        configuredAttribute.isValid(),
        "Attribute handle lookup returned an invalid handle for a configured attribute");
    auto const memberAttribute =
        member.rtiAmbassador().getAttributeHandle(memberObjectClass, attributeName);
    require(
        memberAttribute.isValid() && memberAttribute == configuredAttribute,
        "Member attribute lookup returned a different handle for a configured attribute");
    require(
        owner.rtiAmbassador().getAttributeName(objectClass, configuredAttribute) ==
                attributeName &&
            member.rtiAmbassador().getAttributeName(memberObjectClass, memberAttribute) ==
                attributeName,
        "Get Attribute Name did not return the name associated with a configured "
        "object-class and attribute handle through both joined federates");
  }

  auto const derivedObjectClass = owner.rtiAmbassador().getObjectClassHandle(
      options.secondaryDirectedObjectClassName);
  auto const memberDerivedObjectClass = member.rtiAmbassador().getObjectClassHandle(
      options.secondaryDirectedObjectClassName);
  require(
      derivedObjectClass.isValid() && memberDerivedObjectClass == derivedObjectClass,
      "Derived object-class lookup returned an invalid or federate-inconsistent handle");
  for (auto const& attributeName : attributeNames) {
    auto const inheritedAttribute =
        owner.rtiAmbassador().getAttributeHandle(derivedObjectClass, attributeName);
    require(
        inheritedAttribute.isValid(),
        "Inherited attribute handle lookup returned an invalid handle");
    auto const memberInheritedAttribute =
        member.rtiAmbassador().getAttributeHandle(memberDerivedObjectClass, attributeName);
    require(
        memberInheritedAttribute.isValid() &&
            memberInheritedAttribute == inheritedAttribute,
        "Member inherited-attribute lookup returned a different handle");
    require(
        owner.rtiAmbassador().getAttributeName(derivedObjectClass, inheritedAttribute) ==
                attributeName &&
            member.rtiAmbassador().getAttributeName(
                memberDerivedObjectClass,
                memberInheritedAttribute) == attributeName,
        "Get Attribute Name did not return the name associated with an inherited "
        "attribute and derived object-class handle through both joined federates");
  }

  requireException(
      [&] {
        static_cast<void>(owner.rtiAmbassador().getAttributeName(
            invalidObjectClass,
            attribute));
      },
      L"InvalidObjectClassHandle",
      "looking up an attribute name with an invalid object-class handle");
  requireException(
      [&] {
        static_cast<void>(owner.rtiAmbassador().getAttributeName(
            objectClass,
            invalidAttribute));
      },
      L"InvalidAttributeHandle",
      "looking up an attribute name with an invalid attribute handle");
  auto const objectRootClass =
      owner.rtiAmbassador().getObjectClassHandle(L"HLAobjectRoot");
  requireException(
      [&] {
        static_cast<void>(owner.rtiAmbassador().getAttributeName(
            objectRootClass,
            attribute));
      },
      L"AttributeNotDefined",
      "looking up an attribute name through a class that does not define it");

  owner.resign(rti::DELETE_OBJECTS);
  member.resign(rti::NO_ACTION);
}

void scenarioInteractionClassHandleLookup(Options const& options, rti::CallbackModel model) {
  Session owner(options, model, "interaction-class-handle-owner");
  Session member(options, model, "interaction-class-handle-member");
  auto const federation = federationName(options, "interaction-class-handle-lookup");
  connectAndJoin(owner, member, options, federation, options.fom);
  rti::InteractionClassHandle invalidInteractionClass;

  std::vector<std::wstring> interactionClassNames{
      options.interactionClassName,
      options.preferredTimestampInteractionClassName,
      options.preferredReceiveInteractionClassName,
      options.secondaryDirectedInteractionClassName,
  };
  for (auto const& interactionClassName : interactionClassNames) {
    auto const interactionClass =
        owner.rtiAmbassador().getInteractionClassHandle(interactionClassName);
    require(
        interactionClass.isValid(),
        "Interaction class handle lookup returned an invalid handle for a configured class");
    require(
        owner.rtiAmbassador().getInteractionClassName(interactionClass) ==
            interactionClassName,
        "Get Interaction Class Handle did not resolve a configured interaction-class name");
    auto const memberInteractionClass =
        member.rtiAmbassador().getInteractionClassHandle(interactionClassName);
    require(
        memberInteractionClass.isValid() &&
            memberInteractionClass == interactionClass &&
            member.rtiAmbassador().getInteractionClassName(memberInteractionClass) ==
                interactionClassName,
        "joined federates did not resolve the same interaction-class handle and name");
  }
  requireException(
      [&] {
        static_cast<void>(owner.rtiAmbassador().getInteractionClassHandle(
            L"CppTckInteractionClassThatIsNotDefined"));
      },
      L"NameNotFound",
      "looking up an undefined interaction-class name");
  requireException(
      [&] {
        static_cast<void>(owner.rtiAmbassador().getInteractionClassName(
            invalidInteractionClass));
      },
      L"InvalidInteractionClassHandle",
      "looking up an interaction-class name with an invalid handle");

  owner.resign(rti::DELETE_OBJECTS);
  member.resign(rti::NO_ACTION);
}

void scenarioParameterHandleNameLookup(Options const& options, rti::CallbackModel model) {
  Session owner(options, model, "parameter-handle-name-owner");
  Session member(options, model, "parameter-handle-name-member");
  auto const federation = federationName(options, "parameter-handle-name-lookup");
  connectAndJoin(owner, member, options, federation, options.fom);
  rti::InteractionClassHandle invalidInteractionClass;
  rti::ParameterHandle invalidParameter;

  auto const interactionClass =
      owner.rtiAmbassador().getInteractionClassHandle(options.interactionClassName);
  require(interactionClass.isValid(), "Interaction class handle lookup returned an invalid handle");
  auto const parameter =
      owner.rtiAmbassador().getParameterHandle(interactionClass, options.parameterName);
  require(parameter.isValid(), "Parameter handle lookup returned an invalid handle");
  require(
      owner.rtiAmbassador().getParameterName(interactionClass, parameter) == options.parameterName,
      "Parameter handle/name lookup did not round-trip the supplied interaction parameter name");
  auto const secondaryInteractionClass =
      owner.rtiAmbassador().getInteractionClassHandle(
          options.secondaryDirectedInteractionClassName);
  require(
      secondaryInteractionClass.isValid(),
      "Secondary directed interaction-class handle lookup returned an invalid handle");
  auto const secondaryParameter = owner.rtiAmbassador().getParameterHandle(
      secondaryInteractionClass,
      options.secondaryDirectedInteractionParameterName);
  require(
      secondaryParameter.isValid(),
      "Secondary directed parameter handle lookup returned an invalid handle");
  require(
      owner.rtiAmbassador().getParameterName(
          secondaryInteractionClass,
          secondaryParameter) == options.secondaryDirectedInteractionParameterName,
      "Secondary directed parameter handle/name lookup did not round-trip the supplied parameter name");
  auto const memberInteractionClass =
      member.rtiAmbassador().getInteractionClassHandle(options.interactionClassName);
  auto const memberSecondaryInteractionClass =
      member.rtiAmbassador().getInteractionClassHandle(
          options.secondaryDirectedInteractionClassName);
  require(
      memberInteractionClass == interactionClass &&
          memberSecondaryInteractionClass == secondaryInteractionClass,
      "member received different handles for the same interaction classes");
  auto const memberParameter =
      member.rtiAmbassador().getParameterHandle(memberInteractionClass, options.parameterName);
  auto const memberSecondaryParameter = member.rtiAmbassador().getParameterHandle(
      memberSecondaryInteractionClass,
      options.secondaryDirectedInteractionParameterName);
  require(
      memberParameter.isValid() && memberParameter == parameter &&
          memberSecondaryParameter.isValid() && memberSecondaryParameter == secondaryParameter,
      "member received different handles for the same interaction parameters");
  require(
      member.rtiAmbassador().getParameterName(memberInteractionClass, memberParameter) ==
              options.parameterName &&
          member.rtiAmbassador().getParameterName(
              memberSecondaryInteractionClass,
              memberSecondaryParameter) == options.secondaryDirectedInteractionParameterName,
      "member parameter handle/name lookups did not round-trip the configured parameter names");
  requireException(
      [&] {
        static_cast<void>(owner.rtiAmbassador().getParameterHandle(
            interactionClass,
            L"CppTckParameterThatIsNotDefined"));
      },
      L"NameNotFound",
      "looking up an undefined parameter name");
  requireException(
      [&] {
        static_cast<void>(owner.rtiAmbassador().getParameterHandle(
            interactionClass,
            options.secondaryDirectedInteractionParameterName));
      },
      L"NameNotFound",
      "looking up a parameter name defined only on a different interaction class");
  requireException(
      [&] {
        static_cast<void>(owner.rtiAmbassador().getParameterName(
            invalidInteractionClass,
            parameter));
      },
      L"InvalidInteractionClassHandle",
      "looking up a parameter name with an invalid interaction-class handle");
  requireException(
      [&] {
        static_cast<void>(owner.rtiAmbassador().getParameterName(
            interactionClass,
            invalidParameter));
      },
      L"InvalidParameterHandle",
      "looking up a parameter name with an invalid parameter handle");
  requireException(
      [&] {
        static_cast<void>(owner.rtiAmbassador().getParameterName(
            interactionClass,
            secondaryParameter));
      },
      L"InteractionParameterNotDefined",
      "looking up a parameter name through an interaction class that does not define it");

  owner.resign(rti::DELETE_OBJECTS);
  member.resign(rti::NO_ACTION);
}

void scenarioAvailableObjectClassDimensions(Options const& options, rti::CallbackModel model) {
  require(
      !options.ddmFom.empty() && options.ddmDimensionNames.size() >= 2U,
      "Available object-class dimension lookup requires the adapter's DDM FOM and dimensions");
  Session owner(options, model, "available-object-class-dimensions-owner");
  Session member(options, model, "available-object-class-dimensions-member");
  auto const federation = federationName(options, "available-object-class-dimensions");
  connectAndJoin(owner, member, options, federation, options.ddmFom);
  rti::ObjectClassHandle invalidObjectClass;

  auto const objectClass = owner.rtiAmbassador().getObjectClassHandle(options.objectClassName);
  auto const firstDimension =
      owner.rtiAmbassador().getDimensionHandle(options.ddmDimensionNames.at(0));
  auto const secondDimension =
      owner.rtiAmbassador().getDimensionHandle(options.ddmDimensionNames.at(1));
  auto const rootClass = owner.rtiAmbassador().getObjectClassHandle(L"HLAobjectRoot");
  require(
      objectClass.isValid() && firstDimension.isValid() && secondDimension.isValid() &&
          rootClass.isValid(),
      "Available object-class dimension fixture returned an invalid handle");
  require(firstDimension != secondDimension, "DDM fixture dimension names are not distinct");
  auto const memberObjectClass =
      member.rtiAmbassador().getObjectClassHandle(options.objectClassName);
  auto const memberFirstDimension =
      member.rtiAmbassador().getDimensionHandle(options.ddmDimensionNames.at(0));
  auto const memberSecondDimension =
      member.rtiAmbassador().getDimensionHandle(options.ddmDimensionNames.at(1));
  auto const memberRootClass =
      member.rtiAmbassador().getObjectClassHandle(L"HLAobjectRoot");
  require(
      memberObjectClass == objectClass && memberFirstDimension == firstDimension &&
          memberSecondDimension == secondDimension && memberRootClass == rootClass,
      "member received different handles for the same object class or dimensions");

  auto const objectDimensions =
      owner.rtiAmbassador().getAvailableDimensionsForObjectClass(objectClass);
  auto const memberObjectDimensions =
      member.rtiAmbassador().getAvailableDimensionsForObjectClass(memberObjectClass);
  require(
      objectDimensions == rti::DimensionHandleSet{firstDimension, secondDimension},
      "Get Available Dimensions For Class did not return the DDM class's exact dimension set");
  require(
      memberObjectDimensions ==
          rti::DimensionHandleSet{memberFirstDimension, memberSecondDimension},
      "member Get Available Dimensions For Class returned an incorrect dimension set");
  auto const rootDimensions =
      owner.rtiAmbassador().getAvailableDimensionsForObjectClass(rootClass);
  auto const memberRootDimensions =
      member.rtiAmbassador().getAvailableDimensionsForObjectClass(memberRootClass);
  require(
      rootDimensions.empty(),
      "Get Available Dimensions For Class did not return an empty set for the dimensionless root class");
  require(
      memberRootDimensions.empty(),
      "member Get Available Dimensions For Class did not return an empty root set");
  requireException(
      [&] {
        static_cast<void>(owner.rtiAmbassador().getAvailableDimensionsForObjectClass(
            invalidObjectClass));
      },
      L"InvalidObjectClassHandle",
      "looking up available dimensions for an invalid object-class handle");

  owner.resign(rti::DELETE_OBJECTS);
  member.resign(rti::NO_ACTION);
}

void scenarioAvailableInteractionClassDimensions(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.ddmFom.empty() && options.ddmDimensionNames.size() >= 2U,
      "Available interaction-class dimension lookup requires the adapter's DDM FOM and dimensions");
  Session owner(options, model, "available-interaction-class-dimensions-owner");
  Session member(options, model, "available-interaction-class-dimensions-member");
  auto const federation = federationName(options, "available-interaction-class-dimensions");
  connectAndJoin(owner, member, options, federation, options.ddmFom);
  rti::InteractionClassHandle invalidInteractionClass;

  auto const interactionClass =
      owner.rtiAmbassador().getInteractionClassHandle(options.interactionClassName);
  auto const firstDimension =
      owner.rtiAmbassador().getDimensionHandle(options.ddmDimensionNames.at(0));
  auto const secondDimension =
      owner.rtiAmbassador().getDimensionHandle(options.ddmDimensionNames.at(1));
  auto const rootClass = owner.rtiAmbassador().getInteractionClassHandle(L"HLAinteractionRoot");
  require(
      interactionClass.isValid() && firstDimension.isValid() && secondDimension.isValid() &&
          rootClass.isValid(),
      "Available interaction-class dimension fixture returned an invalid handle");
  require(firstDimension != secondDimension, "DDM fixture dimension names are not distinct");
  auto const memberInteractionClass =
      member.rtiAmbassador().getInteractionClassHandle(options.interactionClassName);
  auto const memberFirstDimension =
      member.rtiAmbassador().getDimensionHandle(options.ddmDimensionNames.at(0));
  auto const memberSecondDimension =
      member.rtiAmbassador().getDimensionHandle(options.ddmDimensionNames.at(1));
  auto const memberRootClass =
      member.rtiAmbassador().getInteractionClassHandle(L"HLAinteractionRoot");
  require(
      memberInteractionClass == interactionClass &&
          memberFirstDimension == firstDimension &&
          memberSecondDimension == secondDimension && memberRootClass == rootClass,
      "member received different handles for the same interaction class or dimensions");

  auto const interactionDimensions =
      owner.rtiAmbassador().getAvailableDimensionsForInteractionClass(interactionClass);
  auto const memberInteractionDimensions =
      member.rtiAmbassador().getAvailableDimensionsForInteractionClass(
          memberInteractionClass);
  require(
      interactionDimensions == rti::DimensionHandleSet{firstDimension, secondDimension},
      "Get Available Dimensions For Interaction Class did not return the DDM class's exact dimension set");
  require(
      memberInteractionDimensions ==
          rti::DimensionHandleSet{memberFirstDimension, memberSecondDimension},
      "member Get Available Dimensions For Interaction Class returned an incorrect set");
  auto const rootDimensions =
      owner.rtiAmbassador().getAvailableDimensionsForInteractionClass(rootClass);
  auto const memberRootDimensions =
      member.rtiAmbassador().getAvailableDimensionsForInteractionClass(memberRootClass);
  require(
      rootDimensions.empty(),
      "Get Available Dimensions For Interaction Class did not return an empty set for the dimensionless root class");
  require(
      memberRootDimensions.empty(),
      "member Get Available Dimensions For Interaction Class did not return an empty root set");
  requireException(
      [&] {
        static_cast<void>(owner.rtiAmbassador()
                              .getAvailableDimensionsForInteractionClass(
                                  invalidInteractionClass));
      },
      L"InvalidInteractionClassHandle",
      "looking up available dimensions for an invalid interaction-class handle");

  owner.resign(rti::DELETE_OBJECTS);
  member.resign(rti::NO_ACTION);
}

void scenarioDimensionHandleNameLookup(Options const& options, rti::CallbackModel model) {
  require(
      !options.ddmFom.empty() && !options.ddmDimensionNames.empty(),
      "Dimension handle/name lookup requires the adapter's DDM FOM and at least one dimension");
  Session owner(options, model, "dimension-handle-name-lookup-owner");
  Session member(options, model, "dimension-handle-name-lookup-member");
  auto const federation = federationName(options, "dimension-handle-name-lookup");
  connectAndJoin(owner, member, options, federation, options.ddmFom);
  rti::DimensionHandle invalidDimension;

  std::vector<rti::DimensionHandle> resolvedDimensions;
  for (auto const& dimensionName : options.ddmDimensionNames) {
    auto const dimension =
        owner.rtiAmbassador().getDimensionHandle(dimensionName);
    require(
        dimension.isValid(),
        "Get Dimension Handle returned an invalid handle for a defined dimension");
    require(
        owner.rtiAmbassador().getDimensionName(dimension) == dimensionName,
        "Get Dimension Name did not return the name associated with the supplied dimension handle");
    auto const memberDimension =
        member.rtiAmbassador().getDimensionHandle(dimensionName);
    require(
        memberDimension == dimension &&
            member.rtiAmbassador().getDimensionName(memberDimension) ==
                dimensionName,
        "member dimension lookup did not preserve handle identity and name");
    require(
        std::find(resolvedDimensions.begin(), resolvedDimensions.end(),
                  dimension) == resolvedDimensions.end(),
        "Distinct defined dimension names resolved to the same handle");
    resolvedDimensions.push_back(dimension);
  }
  requireException(
      [&] {
        static_cast<void>(
            owner.rtiAmbassador().getDimensionHandle(L"CppTckDimensionThatIsNotDefined"));
      },
      L"NameNotFound",
      "looking up an undefined FDD dimension name");
  requireException(
      [&] {
        static_cast<void>(owner.rtiAmbassador().getDimensionName(invalidDimension));
      },
      L"InvalidDimensionHandle",
      "looking up a dimension name with an invalid dimension handle");

  owner.resign(rti::DELETE_OBJECTS);
  member.resign(rti::NO_ACTION);
}

void scenarioDimensionHandleSetForRegion(Options const& options, rti::CallbackModel model) {
  require(
      !options.ddmFom.empty() && options.ddmDimensionNames.size() >= 2U,
      "Dimension handle-set lookup requires the adapter's DDM FOM and two dimensions");
  Session owner(options, model, "dimension-handle-set-region-owner");
  Session member(options, model, "dimension-handle-set-region-member");
  auto const federation = federationName(options, "dimension-handle-set-for-region");
  connectAndJoin(owner, member, options, federation, options.ddmFom);
  rti::RegionHandle invalidRegion;

  auto const firstDimension =
      owner.rtiAmbassador().getDimensionHandle(options.ddmDimensionNames.at(0));
  auto const secondDimension =
      owner.rtiAmbassador().getDimensionHandle(options.ddmDimensionNames.at(1));
  require(
      firstDimension.isValid() && secondDimension.isValid() && firstDimension != secondDimension,
      "DDM fixture did not provide two distinct valid dimensions");

  auto const oneDimensionRegion =
      owner.rtiAmbassador().createRegion(rti::DimensionHandleSet{firstDimension});
  auto const twoDimensionRegion = owner.rtiAmbassador().createRegion(
      rti::DimensionHandleSet{firstDimension, secondDimension});
  require(
      oneDimensionRegion.isValid() && twoDimensionRegion.isValid(),
      "DDM fixture region creation failed");
  require(
      owner.rtiAmbassador().getDimensionHandleSet(oneDimensionRegion) ==
          rti::DimensionHandleSet{firstDimension},
      "Get Dimension Handle Set did not return the single dimension of its region");
  require(
      owner.rtiAmbassador().getDimensionHandleSet(twoDimensionRegion) ==
          rti::DimensionHandleSet{firstDimension, secondDimension},
      "Get Dimension Handle Set did not return the exact dimensions of its region");

  auto const memberFirstDimension =
      member.rtiAmbassador().getDimensionHandle(options.ddmDimensionNames.at(0));
  auto const memberSecondDimension =
      member.rtiAmbassador().getDimensionHandle(options.ddmDimensionNames.at(1));
  require(
      memberFirstDimension.isValid() && memberSecondDimension.isValid() &&
          memberFirstDimension != memberSecondDimension,
      "member did not resolve two distinct valid DDM dimensions");
  auto const memberRegion = member.rtiAmbassador().createRegion(
      rti::DimensionHandleSet{memberFirstDimension, memberSecondDimension});
  require(memberRegion.isValid(), "member DDM region creation failed");
  require(
      member.rtiAmbassador().getDimensionHandleSet(memberRegion) ==
          rti::DimensionHandleSet{memberFirstDimension, memberSecondDimension},
      "Get Dimension Handle Set did not return the exact dimensions of the member's own region");

  requireException(
      [&] {
        static_cast<void>(owner.rtiAmbassador().getDimensionHandleSet(invalidRegion));
      },
      L"InvalidRegion",
      "looking up dimensions for a default-invalid region handle");

  owner.resign(rti::DELETE_OBJECTS);
  member.resign(rti::NO_ACTION);
}

void scenarioUnconveyedPeerRegionDimensionHandleSetInvalidRegion(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.ddmFom.empty() && !options.ddmDimensionNames.empty(),
      "Foreign-region dimension lookup requires the adapter's DDM FOM and a dimension");
  Session owner(options, model, "unconveyed-peer-region-owner");
  Session member(options, model, "unconveyed-peer-region-member");
  auto const federation =
      federationName(options, "unconveyed-peer-region-dimension-handle-set");
  connectAndJoin(owner, member, options, federation, options.ddmFom);

  auto const dimension =
      owner.rtiAmbassador().getDimensionHandle(options.ddmDimensionNames.front());
  require(dimension.isValid(), "DDM fixture returned an invalid region dimension");
  auto const ownerRegion =
      owner.rtiAmbassador().createRegion(rti::DimensionHandleSet{dimension});
  require(ownerRegion.isValid(), "owner could not create the DDM region");
  requireException(
      [&] { static_cast<void>(member.rtiAmbassador().getDimensionHandleSet(ownerRegion)); },
      L"InvalidRegion",
      "Get Dimension Handle Set for a peer-owned region not conveyed to the invoking federate");

  owner.resign(rti::DELETE_OBJECTS);
  member.resign(rti::NO_ACTION);
}

void scenarioRangeBoundsForRegion(Options const& options, rti::CallbackModel model) {
  require(
      !options.ddmFom.empty() && options.ddmDimensionNames.size() >= 2U,
      "Range-bounds lookup requires the adapter's DDM FOM and two dimensions");
  Session owner(options, model, "range-bounds-region-owner");
  Session member(options, model, "range-bounds-region-member");
  auto const federation = federationName(options, "range-bounds-for-region");
  connectAndJoin(owner, member, options, federation, options.ddmFom);

  auto const dimension =
      owner.rtiAmbassador().getDimensionHandle(options.ddmDimensionNames.front());
  auto const outOfRegionDimension =
      owner.rtiAmbassador().getDimensionHandle(options.ddmDimensionNames.at(1));
  require(
      dimension.isValid() && outOfRegionDimension.isValid() &&
          dimension != outOfRegionDimension,
      "DDM fixture returned invalid or duplicate range dimensions");
  auto const region = owner.rtiAmbassador().createRegion(rti::DimensionHandleSet{dimension});
  require(region.isValid(), "DDM fixture range region creation failed");
  constexpr unsigned long lowerBound = 3UL;
  constexpr unsigned long upperBound = 9UL;
  auto const memberDimension =
      member.rtiAmbassador().getDimensionHandle(options.ddmDimensionNames.front());
  require(memberDimension.isValid(), "member received an invalid range dimension");
  requireException(
      [&] {
        member.rtiAmbassador().setRangeBounds(
            region,
            memberDimension,
            rti::RangeBounds(lowerBound, upperBound));
      },
      L"RegionNotCreatedByThisFederate",
      "setting range bounds on a region created by another federate");
  requireException(
      [&] {
        owner.rtiAmbassador().setRangeBounds(
            region,
            dimension,
            rti::RangeBounds(upperBound, lowerBound));
      },
      L"InvalidRangeBound",
      "setting range bounds with the lower bound greater than the upper bound");
  requireException(
      [&] {
        owner.rtiAmbassador().setRangeBounds(
            region,
            outOfRegionDimension,
            rti::RangeBounds(lowerBound, upperBound));
      },
      L"RegionDoesNotContainSpecifiedDimension",
      "setting range bounds for a valid dimension outside the region");
  owner.rtiAmbassador().setRangeBounds(
      region, dimension, rti::RangeBounds(lowerBound, upperBound));
  owner.rtiAmbassador().commitRegionModifications(rti::RegionHandleSet{region});

  auto const bounds = owner.rtiAmbassador().getRangeBounds(region, dimension);
  require(
      bounds.getLowerBound() == lowerBound && bounds.getUpperBound() == upperBound,
      "Get Range Bounds did not return the current bounds for the specified region dimension");
  requireException(
      [&] {
        static_cast<void>(owner.rtiAmbassador().getRangeBounds(
            region,
            outOfRegionDimension));
      },
      L"RegionDoesNotContainSpecifiedDimension",
      "looking up range bounds for a valid dimension outside the region");

  auto const multiDimensionalRegion = owner.rtiAmbassador().createRegion(
      rti::DimensionHandleSet{dimension, outOfRegionDimension});
  require(
      multiDimensionalRegion.isValid(),
      "DDM fixture multi-dimensional range region creation failed");
  constexpr unsigned long firstDimensionLowerBound = 2UL;
  constexpr unsigned long firstDimensionUpperBound = 8UL;
  constexpr unsigned long secondDimensionLowerBound = 1UL;
  constexpr unsigned long secondDimensionUpperBound = 6UL;
  owner.rtiAmbassador().setRangeBounds(
      multiDimensionalRegion,
      dimension,
      rti::RangeBounds(
          firstDimensionLowerBound,
          firstDimensionUpperBound));
  owner.rtiAmbassador().setRangeBounds(
      multiDimensionalRegion,
      outOfRegionDimension,
      rti::RangeBounds(
          secondDimensionLowerBound,
          secondDimensionUpperBound));
  owner.rtiAmbassador().commitRegionModifications(
      rti::RegionHandleSet{multiDimensionalRegion});
  auto const firstDimensionBounds = owner.rtiAmbassador().getRangeBounds(
      multiDimensionalRegion,
      dimension);
  auto const secondDimensionBounds = owner.rtiAmbassador().getRangeBounds(
      multiDimensionalRegion,
      outOfRegionDimension);
  require(
      firstDimensionBounds.getLowerBound() == firstDimensionLowerBound &&
          firstDimensionBounds.getUpperBound() == firstDimensionUpperBound &&
          secondDimensionBounds.getLowerBound() == secondDimensionLowerBound &&
          secondDimensionBounds.getUpperBound() == secondDimensionUpperBound,
      "Range bounds for two dimensions in the same region were not kept independent");

  auto const memberRegion =
      member.rtiAmbassador().createRegion(rti::DimensionHandleSet{memberDimension});
  require(memberRegion.isValid(), "member DDM range region creation failed");
  constexpr unsigned long memberLowerBound = 1UL;
  constexpr unsigned long memberUpperBound = 7UL;
  member.rtiAmbassador().setRangeBounds(
      memberRegion,
      memberDimension,
      rti::RangeBounds(memberLowerBound, memberUpperBound));
  member.rtiAmbassador().commitRegionModifications(
      rti::RegionHandleSet{memberRegion});
  auto const memberBounds =
      member.rtiAmbassador().getRangeBounds(memberRegion, memberDimension);
  require(
      memberBounds.getLowerBound() == memberLowerBound &&
          memberBounds.getUpperBound() == memberUpperBound,
      "Get Range Bounds did not return the current bounds for the member's own region");

  owner.resign(rti::DELETE_OBJECTS);
  member.resign(rti::NO_ACTION);
}

void scenarioUnconveyedPeerRegionRangeBoundsInvalidRegion(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.ddmFom.empty() && !options.ddmDimensionNames.empty(),
      "Foreign-region range lookup requires the adapter's DDM FOM and a dimension");
  Session owner(options, model, "unconveyed-peer-range-region-owner");
  Session member(options, model, "unconveyed-peer-range-region-member");
  auto const federation =
      federationName(options, "unconveyed-peer-region-range-bounds");
  connectAndJoin(owner, member, options, federation, options.ddmFom);

  auto const ownerDimension =
      owner.rtiAmbassador().getDimensionHandle(options.ddmDimensionNames.front());
  auto const memberDimension =
      member.rtiAmbassador().getDimensionHandle(options.ddmDimensionNames.front());
  require(
      ownerDimension.isValid() && memberDimension.isValid(),
      "DDM fixture returned an invalid range dimension");
  auto const ownerRegion = owner.rtiAmbassador().createRegion(
      rti::DimensionHandleSet{ownerDimension});
  require(ownerRegion.isValid(), "owner could not create the DDM range region");
  owner.rtiAmbassador().setRangeBounds(
      ownerRegion, ownerDimension, rti::RangeBounds(2UL, 8UL));
  owner.rtiAmbassador().commitRegionModifications(rti::RegionHandleSet{ownerRegion});

  requireException(
      [&] {
        static_cast<void>(
            member.rtiAmbassador().getRangeBounds(ownerRegion, memberDimension));
      },
      L"InvalidRegion",
      "Get Range Bounds for a peer-created region not conveyed to the invoking federate");

  owner.resign(rti::DELETE_OBJECTS);
  member.resign(rti::NO_ACTION);
}

void scenarioInvalidRegionDesignatorRangeBoundsInvalidRegion(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.ddmFom.empty() && !options.ddmDimensionNames.empty(),
      "Invalid-region-designator lookup requires the adapter's DDM FOM and a dimension");
  Session owner(options, model, "invalid-region-designator-range-bounds-owner");
  Session member(options, model, "invalid-region-designator-range-bounds-member");
  auto const federation =
      federationName(options, "invalid-region-designator-range-bounds");
  connectAndJoin(owner, member, options, federation, options.ddmFom);

  auto const dimension =
      owner.rtiAmbassador().getDimensionHandle(options.ddmDimensionNames.front());
  require(dimension.isValid(), "DDM fixture returned an invalid range dimension");
  rti::RegionHandle invalidRegion;
  require(!invalidRegion.isValid(), "default-constructed RegionHandle was unexpectedly valid");
  requireException(
      [&] {
        static_cast<void>(
            owner.rtiAmbassador().getRangeBounds(invalidRegion, dimension));
      },
      L"InvalidRegion",
      "Get Range Bounds with a designator that is not a region specification "
      "or realization copy");

  owner.resign(rti::NO_ACTION);
  member.resign(rti::NO_ACTION);
}

void scenarioHandleNormalizationEqualInstances(
    Options const& options,
    rti::CallbackModel model) {
  Session owner(options, model, "handle-normalization-equal-owner");
  Session member(options, model, "handle-normalization-equal-member");
  auto const federation = federationName(options, "handle-normalization-equal-instances");
  connectAndJoin(owner, member, options, federation, options.fom);

  rti::ObjectClassHandle objectClass;
  rti::AttributeHandle attribute;
  rti::InteractionClassHandle interactionClass;
  rti::ParameterHandle parameter;
  handles(owner, options, objectClass, attribute, interactionClass, parameter);
  rti::AttributeHandleSet publishedAttributes;
  publishedAttributes.insert(attribute);
  owner.rtiAmbassador().publishObjectClassAttributes(objectClass, publishedAttributes);
  auto const object = owner.rtiAmbassador().registerObjectInstance(objectClass);
  require(object.isValid(), "Handle-normalization test registration returned an invalid handle");

  auto requireEqualInstancesNormalizeEqually = [&](auto const& handle,
                                                    auto const& lookupHandle,
                                                    auto const& normalize,
                                                    std::string const& description) {
    auto const copiedHandle = handle;
    require(handle == copiedHandle,
            description + " copy did not produce an equal handle instance");
    require(handle == lookupHandle,
            description + " name lookup did not recover an equal handle value");
    require(normalize(handle) == normalize(copiedHandle),
            description + " copied equal handle instances produced different normalized values");
    require(normalize(handle) == normalize(lookupHandle),
            description + " looked-up equal handle instances produced different normalized values");
  };
  requireEqualInstancesNormalizeEqually(
      owner.federateHandle(),
      owner.rtiAmbassador().getFederateHandle(
          owner.rtiAmbassador().getFederateName(owner.federateHandle())),
      [&](rti::FederateHandle const& handle) {
        return owner.rtiAmbassador().normalizeFederateHandle(handle);
      },
      "Federate handle");
  requireEqualInstancesNormalizeEqually(
      objectClass,
      owner.rtiAmbassador().getObjectClassHandle(
          owner.rtiAmbassador().getObjectClassName(objectClass)),
      [&](rti::ObjectClassHandle const& handle) {
        return owner.rtiAmbassador().normalizeObjectClassHandle(handle);
      },
      "Object-class handle");
  requireEqualInstancesNormalizeEqually(
      interactionClass,
      owner.rtiAmbassador().getInteractionClassHandle(
          owner.rtiAmbassador().getInteractionClassName(interactionClass)),
      [&](rti::InteractionClassHandle const& handle) {
        return owner.rtiAmbassador().normalizeInteractionClassHandle(handle);
      },
      "Interaction-class handle");
  requireEqualInstancesNormalizeEqually(
      object,
      owner.rtiAmbassador().getObjectInstanceHandle(
          owner.rtiAmbassador().getObjectInstanceName(object)),
      [&](rti::ObjectInstanceHandle const& handle) {
        return owner.rtiAmbassador().normalizeObjectInstanceHandle(handle);
      },
      "Object-instance handle");

  owner.resign(rti::DELETE_OBJECTS);
  member.resign(rti::NO_ACTION);
}

void scenarioNormalizeObjectClassHandleRegionCoordinate(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.modelFom.empty(),
      "Normalize Object Class Handle region-coordinate testing requires an adapter-supplied model FOM");
  require(
      !options.mimFom.empty(),
      "Normalize Object Class Handle region-coordinate testing requires an adapter-supplied standard MIM");

  Session subject(options, model, "normalize-object-class-handle-region-coordinate");
  auto const federation = federationName(
      options,
      "normalize-object-class-handle-region-coordinate");
  subject.connect();
  subject.rtiAmbassador().createFederationExecutionWithMIM(
      federation,
      std::vector<std::wstring>{options.modelFom.wstring()},
      options.mimFom.wstring(),
      options.logicalTimeImplementationName);
  subject.join(
      options.ownerFederateName + L"-normalize-object-class-handle-region-coordinate",
      options.federateType,
      federation);
  Session member(options, model, "normalize-object-class-handle-region-coordinate-member");
  member.connect();
  member.join(
      options.memberFederateName + L"-normalize-object-class-handle-region-coordinate",
      options.federateType,
      federation);

  auto const objectClass = subject.rtiAmbassador().getObjectClassHandle(
      L"HLAobjectRoot");
  auto const dimension = subject.rtiAmbassador().getDimensionHandle(
      L"HLAobjectClass");
  require(
      objectClass.isValid() && dimension.isValid(),
      "Normalize Object Class Handle region-coordinate setup could not resolve the model class and standard HLAobjectClass dimension");

  rti::ObjectClassHandle invalidObjectClass;
  require(
      !invalidObjectClass.isValid(),
      "default-constructed ObjectClassHandle was unexpectedly valid");
  requireException(
      [&] {
        static_cast<void>(subject.rtiAmbassador().normalizeObjectClassHandle(
            invalidObjectClass));
      },
      L"InvalidObjectClassHandle",
      "normalizing an invalid object-class handle");

  auto const normalized =
      subject.rtiAmbassador().normalizeObjectClassHandle(objectClass);
  auto const dimensionUpperBound =
      subject.rtiAmbassador().getDimensionUpperBound(dimension);
  require(
      normalized < dimensionUpperBound,
      "Normalize Object Class Handle returned a value that cannot form a point range in HLAobjectClass");

  auto const region = subject.rtiAmbassador().createRegion(
      rti::DimensionHandleSet{dimension});
  require(
      region.isValid(),
      "Normalize Object Class Handle region-coordinate setup returned an invalid region handle");
  auto const expectedBounds = rti::RangeBounds{normalized, normalized + 1UL};
  subject.rtiAmbassador().setRangeBounds(region, dimension, expectedBounds);
  subject.rtiAmbassador().commitRegionModifications(rti::RegionHandleSet{region});
  auto const actualBounds =
      subject.rtiAmbassador().getRangeBounds(region, dimension);
  require(
      actualBounds.getLowerBound() == expectedBounds.getLowerBound() &&
          actualBounds.getUpperBound() == expectedBounds.getUpperBound(),
      "Get Range Bounds did not preserve the normalized object-class handle as the HLAobjectClass point range");

  auto const memberObjectClass =
      member.rtiAmbassador().getObjectClassHandle(L"HLAobjectRoot");
  auto const memberDimension =
      member.rtiAmbassador().getDimensionHandle(L"HLAobjectClass");
  require(
      memberObjectClass.isValid() && memberDimension.isValid(),
      "member could not resolve its own object-class and HLAobjectClass dimension handles");
  auto const memberNormalized =
      member.rtiAmbassador().normalizeObjectClassHandle(memberObjectClass);
  auto const memberDimensionUpperBound =
      member.rtiAmbassador().getDimensionUpperBound(memberDimension);
  require(
      memberNormalized < memberDimensionUpperBound,
      "member's normalized object-class value cannot form a point range in HLAobjectClass");
  auto const memberRegion = member.rtiAmbassador().createRegion(
      rti::DimensionHandleSet{memberDimension});
  require(
      memberRegion.isValid(),
      "member could not create a region for its HLAobjectClass dimension");
  auto const memberExpectedBounds =
      rti::RangeBounds{memberNormalized, memberNormalized + 1UL};
  member.rtiAmbassador().setRangeBounds(
      memberRegion, memberDimension, memberExpectedBounds);
  member.rtiAmbassador().commitRegionModifications(
      rti::RegionHandleSet{memberRegion});
  auto const memberActualBounds =
      member.rtiAmbassador().getRangeBounds(memberRegion, memberDimension);
  require(
      memberActualBounds.getLowerBound() == memberExpectedBounds.getLowerBound() &&
          memberActualBounds.getUpperBound() == memberExpectedBounds.getUpperBound(),
      "member's normalized object-class value did not round-trip as its own HLAobjectClass point range");

  subject.resign(rti::NO_ACTION);
  member.resign(rti::NO_ACTION);
  subject.rtiAmbassador().destroyFederationExecution(federation);
}

void scenarioNormalizeInteractionClassHandleRegionCoordinate(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.modelFom.empty(),
      "Normalize Interaction Class Handle region-coordinate testing requires an adapter-supplied model FOM");
  require(
      !options.mimFom.empty(),
      "Normalize Interaction Class Handle region-coordinate testing requires an adapter-supplied standard MIM");

  Session subject(options, model, "normalize-interaction-class-handle-region-coordinate");
  auto const federation = federationName(
      options,
      "normalize-interaction-class-handle-region-coordinate");
  subject.connect();
  subject.rtiAmbassador().createFederationExecutionWithMIM(
      federation,
      std::vector<std::wstring>{options.modelFom.wstring()},
      options.mimFom.wstring(),
      options.logicalTimeImplementationName);
  subject.join(
      options.ownerFederateName + L"-normalize-interaction-class-handle-region-coordinate",
      options.federateType,
      federation);
  Session member(options, model, "normalize-interaction-class-handle-region-coordinate-member");
  member.connect();
  member.join(
      options.memberFederateName + L"-normalize-interaction-class-handle-region-coordinate",
      options.federateType,
      federation);

  auto const interactionClass = subject.rtiAmbassador().getInteractionClassHandle(
      L"HLAinteractionRoot");
  auto const dimension = subject.rtiAmbassador().getDimensionHandle(
      L"HLAinteractionClass");
  require(
      interactionClass.isValid() && dimension.isValid(),
      "Normalize Interaction Class Handle region-coordinate setup could not resolve the standard interaction class and HLAinteractionClass dimension");

  rti::InteractionClassHandle invalidInteractionClass;
  require(
      !invalidInteractionClass.isValid(),
      "default-constructed InteractionClassHandle was unexpectedly valid");
  requireException(
      [&] {
        static_cast<void>(subject.rtiAmbassador().normalizeInteractionClassHandle(
            invalidInteractionClass));
      },
      L"InvalidInteractionClassHandle",
      "normalizing an invalid interaction-class handle");

  auto const normalized =
      subject.rtiAmbassador().normalizeInteractionClassHandle(interactionClass);
  auto const dimensionUpperBound =
      subject.rtiAmbassador().getDimensionUpperBound(dimension);
  require(
      normalized < dimensionUpperBound,
      "Normalize Interaction Class Handle returned a value that cannot form a point range in HLAinteractionClass");

  auto const region = subject.rtiAmbassador().createRegion(
      rti::DimensionHandleSet{dimension});
  require(
      region.isValid(),
      "Normalize Interaction Class Handle region-coordinate setup returned an invalid region handle");
  auto const expectedBounds = rti::RangeBounds{normalized, normalized + 1UL};
  subject.rtiAmbassador().setRangeBounds(region, dimension, expectedBounds);
  subject.rtiAmbassador().commitRegionModifications(rti::RegionHandleSet{region});
  auto const actualBounds =
      subject.rtiAmbassador().getRangeBounds(region, dimension);
  require(
      actualBounds.getLowerBound() == expectedBounds.getLowerBound() &&
          actualBounds.getUpperBound() == expectedBounds.getUpperBound(),
      "Get Range Bounds did not preserve the normalized interaction-class handle as the HLAinteractionClass point range");

  auto const memberInteractionClass =
      member.rtiAmbassador().getInteractionClassHandle(L"HLAinteractionRoot");
  auto const memberDimension =
      member.rtiAmbassador().getDimensionHandle(L"HLAinteractionClass");
  require(
      memberInteractionClass.isValid() && memberDimension.isValid(),
      "member could not resolve its own interaction-class and HLAinteractionClass dimension handles");
  auto const memberNormalized =
      member.rtiAmbassador().normalizeInteractionClassHandle(
          memberInteractionClass);
  auto const memberDimensionUpperBound =
      member.rtiAmbassador().getDimensionUpperBound(memberDimension);
  require(
      memberNormalized < memberDimensionUpperBound,
      "member's normalized interaction-class value cannot form a point range in HLAinteractionClass");
  auto const memberRegion = member.rtiAmbassador().createRegion(
      rti::DimensionHandleSet{memberDimension});
  require(
      memberRegion.isValid(),
      "member could not create a region for its HLAinteractionClass dimension");
  auto const memberExpectedBounds =
      rti::RangeBounds{memberNormalized, memberNormalized + 1UL};
  member.rtiAmbassador().setRangeBounds(
      memberRegion, memberDimension, memberExpectedBounds);
  member.rtiAmbassador().commitRegionModifications(
      rti::RegionHandleSet{memberRegion});
  auto const memberActualBounds =
      member.rtiAmbassador().getRangeBounds(memberRegion, memberDimension);
  require(
      memberActualBounds.getLowerBound() == memberExpectedBounds.getLowerBound() &&
          memberActualBounds.getUpperBound() == memberExpectedBounds.getUpperBound(),
      "member's normalized interaction-class value did not round-trip as its own HLAinteractionClass point range");

  subject.resign(rti::NO_ACTION);
  member.resign(rti::NO_ACTION);
  subject.rtiAmbassador().destroyFederationExecution(federation);
}

void scenarioNormalizeObjectInstanceHandleRegionCoordinate(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.fom.empty(),
      "Normalize Object Instance Handle region-coordinate testing requires an adapter-supplied model FOM");
  require(
      !options.mimFom.empty(),
      "Normalize Object Instance Handle region-coordinate testing requires an adapter-supplied standard MIM");

  Session subject(options, model, "normalize-object-instance-handle-region-coordinate");
  auto const federation = federationName(
      options,
      "normalize-object-instance-handle-region-coordinate");
  subject.connect();
  subject.rtiAmbassador().createFederationExecutionWithMIM(
      federation,
      std::vector<std::wstring>{options.fom.wstring()},
      options.mimFom.wstring(),
      options.logicalTimeImplementationName);
  subject.join(
      options.ownerFederateName + L"-normalize-object-instance-handle-region-coordinate",
      options.federateType,
      federation);
  Session member(options, model, "normalize-object-instance-handle-region-coordinate-member");
  member.connect();
  member.join(
      options.memberFederateName + L"-normalize-object-instance-handle-region-coordinate",
      options.federateType,
      federation);

  auto const objectClass = subject.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const attribute = subject.rtiAmbassador().getAttributeHandle(
      objectClass,
      options.attributeName);
  require(
      objectClass.isValid() && attribute.isValid(),
      "Normalize Object Instance Handle region-coordinate setup could not resolve the model object class and attribute");
  rti::AttributeHandleSet attributes;
  attributes.insert(attribute);
  subject.rtiAmbassador().publishObjectClassAttributes(objectClass, attributes);
  auto const object = subject.rtiAmbassador().registerObjectInstance(objectClass);
  auto const dimension = subject.rtiAmbassador().getDimensionHandle(
      L"HLAobjectInstance");
  require(
      object.isValid() && dimension.isValid(),
      "Normalize Object Instance Handle region-coordinate setup could not resolve a registered object instance and standard HLAobjectInstance dimension");

  rti::ObjectInstanceHandle invalidObjectInstance;
  require(
      !invalidObjectInstance.isValid(),
      "default-constructed ObjectInstanceHandle was unexpectedly valid");
  requireException(
      [&] {
        static_cast<void>(subject.rtiAmbassador().normalizeObjectInstanceHandle(
            invalidObjectInstance));
      },
      L"InvalidObjectInstanceHandle",
      "normalizing an invalid object-instance handle");

  auto const normalized =
      subject.rtiAmbassador().normalizeObjectInstanceHandle(object);
  auto const dimensionUpperBound =
      subject.rtiAmbassador().getDimensionUpperBound(dimension);
  require(
      normalized < dimensionUpperBound,
      "Normalize Object Instance Handle returned a value that cannot form a point range in HLAobjectInstance");

  auto const region = subject.rtiAmbassador().createRegion(
      rti::DimensionHandleSet{dimension});
  require(
      region.isValid(),
      "Normalize Object Instance Handle region-coordinate setup returned an invalid region handle");
  auto const expectedBounds = rti::RangeBounds{normalized, normalized + 1UL};
  subject.rtiAmbassador().setRangeBounds(region, dimension, expectedBounds);
  subject.rtiAmbassador().commitRegionModifications(rti::RegionHandleSet{region});
  auto const actualBounds =
      subject.rtiAmbassador().getRangeBounds(region, dimension);
  require(
      actualBounds.getLowerBound() == expectedBounds.getLowerBound() &&
          actualBounds.getUpperBound() == expectedBounds.getUpperBound(),
      "Get Range Bounds did not preserve the normalized object-instance handle as the HLAobjectInstance point range");

  auto const memberObjectClass =
      member.rtiAmbassador().getObjectClassHandle(options.objectClassName);
  auto const memberAttribute = member.rtiAmbassador().getAttributeHandle(
      memberObjectClass, options.attributeName);
  require(
      memberObjectClass.isValid() && memberAttribute.isValid(),
      "member could not resolve its own model object class and attribute handles");
  rti::AttributeHandleSet memberAttributes;
  memberAttributes.insert(memberAttribute);
  member.rtiAmbassador().publishObjectClassAttributes(
      memberObjectClass, memberAttributes);
  auto const memberObject =
      member.rtiAmbassador().registerObjectInstance(memberObjectClass);
  auto const memberDimension =
      member.rtiAmbassador().getDimensionHandle(L"HLAobjectInstance");
  require(
      memberObject.isValid() && memberDimension.isValid(),
      "member could not register its own object instance or resolve HLAobjectInstance");
  auto const memberNormalized =
      member.rtiAmbassador().normalizeObjectInstanceHandle(memberObject);
  auto const memberDimensionUpperBound =
      member.rtiAmbassador().getDimensionUpperBound(memberDimension);
  require(
      memberNormalized < memberDimensionUpperBound,
      "member's normalized object-instance value cannot form a point range in HLAobjectInstance");
  auto const memberRegion = member.rtiAmbassador().createRegion(
      rti::DimensionHandleSet{memberDimension});
  require(
      memberRegion.isValid(),
      "member could not create a region for its HLAobjectInstance dimension");
  auto const memberExpectedBounds =
      rti::RangeBounds{memberNormalized, memberNormalized + 1UL};
  member.rtiAmbassador().setRangeBounds(
      memberRegion, memberDimension, memberExpectedBounds);
  member.rtiAmbassador().commitRegionModifications(
      rti::RegionHandleSet{memberRegion});
  auto const memberActualBounds =
      member.rtiAmbassador().getRangeBounds(memberRegion, memberDimension);
  require(
      memberActualBounds.getLowerBound() == memberExpectedBounds.getLowerBound() &&
          memberActualBounds.getUpperBound() == memberExpectedBounds.getUpperBound(),
      "member's normalized object-instance value did not round-trip as its own HLAobjectInstance point range");

  subject.resign(rti::DELETE_OBJECTS);
  member.resign(rti::DELETE_OBJECTS);
  subject.rtiAmbassador().destroyFederationExecution(federation);
}

void scenarioEvokeCallbackWaitsForTimeoutWithNoPendingCallback(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.fom.empty(),
      "Evoke Callback timeout testing requires an adapter-supplied model FOM");

  Session subject(options, model, "evoke-callback-empty-queue-timeout");
  auto const federation = federationName(
      options,
      "evoke-callback-empty-queue-timeout");
  subject.connect();
  subject.rtiAmbassador().createFederationExecution(
      federation,
      options.fom.wstring(),
      options.logicalTimeImplementationName);
  subject.join(
      options.ownerFederateName + L"-evoke-callback-empty-queue-timeout",
      options.federateType,
      federation);

  constexpr double requestedWaitSeconds = 0.05;
  auto const started = std::chrono::steady_clock::now();
  auto const callbackPending =
      subject.rtiAmbassador().evokeCallback(requestedWaitSeconds);
  auto const elapsed = std::chrono::steady_clock::now() - started;
  auto const requestedWait = std::chrono::duration_cast<
      std::chrono::steady_clock::duration>(
      std::chrono::duration<double>(requestedWaitSeconds));
  auto const precision = std::chrono::duration_cast<
      std::chrono::steady_clock::duration>(std::chrono::milliseconds(1));

  require(
      !callbackPending,
      "Evoke Callback reported pending callbacks when the joined federate had none");
  require(
      elapsed + precision >= requestedWait,
      "Evoke Callback returned before its requested empty-queue timeout");

  subject.resign(rti::NO_ACTION);
  subject.rtiAmbassador().destroyFederationExecution(federation);
}

void scenarioEvokeCallbackSingleDispatchPendingIndicator(
    Options const& options,
    rti::CallbackModel model) {
  require(
      model == rti::HLA_EVOKED,
      "Evoke Callback pending-indicator testing requires HLA_EVOKED");
  require(
      !options.fom.empty(),
      "Evoke Callback pending-indicator testing requires an adapter-supplied model FOM");

  Session subject(options, model, "evoke-callback-single-dispatch-pending-indicator");
  auto const federation = federationName(
      options,
      "evoke-callback-single-dispatch-pending-indicator");
  subject.connect();
  subject.rtiAmbassador().createFederationExecution(
      federation,
      options.fom.wstring(),
      options.logicalTimeImplementationName);
  subject.join(
      options.ownerFederateName + L"-evoke-callback-single-dispatch-pending-indicator",
      options.federateType,
      federation);

  std::size_t callbackInvocationCount = 0U;
  bool secondReportRequested = false;
  subject.recorder().setFederationExecutionsReportHook([&] {
    ++callbackInvocationCount;
    if (callbackInvocationCount == 1U) {
      subject.rtiAmbassador().listFederationExecutions();
      secondReportRequested = true;
    }
  });
  subject.rtiAmbassador().listFederationExecutions();
  require(
      callbackInvocationCount == 0U,
      "List Federation Executions invoked a report callback before an Evoke Callback boundary");

  auto const callbackPendingAfterFirst =
      subject.rtiAmbassador().evokeCallback(1.0);
  require(
      callbackInvocationCount == 1U,
      "one Evoke Callback invocation did not invoke exactly one federation-list callback");
  require(
      secondReportRequested,
      "the first federation-list callback did not queue the second report callback");
  require(
      callbackPendingAfterFirst,
      "Evoke Callback returned FALSE while the second report callback was pending");

  auto const callbackPendingAfterSecond =
      subject.rtiAmbassador().evokeCallback(0.0);
  require(
      callbackInvocationCount == 2U,
      "the second Evoke Callback invocation did not invoke exactly the remaining callback");
  require(
      !callbackPendingAfterSecond,
      "Evoke Callback returned TRUE after the only remaining callback was consumed");

  subject.recorder().setFederationExecutionsReportHook({});
  subject.resign(rti::NO_ACTION);
  subject.rtiAmbassador().destroyFederationExecution(federation);
}

void scenarioEvokeCallbackWaitDurationPrecision(
    Options const& options,
    rti::CallbackModel model) {
  require(
      model == rti::HLA_EVOKED,
      "Evoke Callback wait-duration precision testing requires HLA_EVOKED");
  require(
      !options.fom.empty(),
      "Evoke Callback wait-duration precision testing requires an adapter-supplied model FOM");

  Session subject(options, model, "evoke-callback-wait-duration-precision");
  auto const federation = federationName(
      options,
      "evoke-callback-wait-duration-precision");
  subject.connect();
  subject.rtiAmbassador().createFederationExecution(
      federation,
      options.fom.wstring(),
      options.logicalTimeImplementationName);
  subject.join(
      options.ownerFederateName + L"-evoke-callback-wait-duration-precision",
      options.federateType,
      federation);

  auto measureAddedWait = [&](std::chrono::milliseconds requestedWait) {
    auto measureCall = [&](double waitSeconds) {
      auto const started = std::chrono::steady_clock::now();
      auto const callbackPending =
          subject.rtiAmbassador().evokeCallback(waitSeconds);
      auto const elapsed = std::chrono::steady_clock::now() - started;
      require(
          !callbackPending,
          "Evoke Callback reported a pending callback on the empty-queue precision path");
      return elapsed;
    };

    auto const baselineElapsed = measureCall(0.0);
    auto const requestedWaitSeconds =
        std::chrono::duration<double>(requestedWait).count();
    return measureCall(requestedWaitSeconds) - baselineElapsed;
  };

  constexpr std::size_t sampleCount = 15U;
  std::vector<std::chrono::steady_clock::duration> fifteenMillisecondSamples;
  std::vector<std::chrono::steady_clock::duration> sixteenMillisecondSamples;
  for (std::size_t sample = 0U; sample < sampleCount; ++sample) {
    if ((sample % 2U) == 0U) {
      fifteenMillisecondSamples.push_back(
          measureAddedWait(std::chrono::milliseconds(15)));
      sixteenMillisecondSamples.push_back(
          measureAddedWait(std::chrono::milliseconds(16)));
    } else {
      sixteenMillisecondSamples.push_back(
          measureAddedWait(std::chrono::milliseconds(16)));
      fifteenMillisecondSamples.push_back(
          measureAddedWait(std::chrono::milliseconds(15)));
    }
  }

  auto median = [](auto samples) {
    std::sort(samples.begin(), samples.end());
    return samples[samples.size() / 2U];
  };
  auto const observedOneMillisecondStep =
      median(sixteenMillisecondSamples) - median(fifteenMillisecondSamples);
  auto const expectedOneMillisecondStep = std::chrono::duration_cast<
      std::chrono::steady_clock::duration>(std::chrono::milliseconds(1));
  auto const measurementTolerance = std::chrono::duration_cast<
      std::chrono::steady_clock::duration>(std::chrono::microseconds(750));

  require(
      observedOneMillisecondStep + measurementTolerance >=
              expectedOneMillisecondStep &&
          observedOneMillisecondStep <=
              expectedOneMillisecondStep + measurementTolerance,
      "Evoke Callback did not preserve a 1 ms change in its wait argument; median added-wait step was " +
          std::to_string(std::chrono::duration<double, std::milli>(
                             observedOneMillisecondStep)
                             .count()) +
          " ms");

  subject.resign(rti::NO_ACTION);
  subject.rtiAmbassador().destroyFederationExecution(federation);
}

void scenarioEvokeCallbackImmediateNoEffect(
    Options const& options,
    rti::CallbackModel model) {
  require(
      model == rti::HLA_IMMEDIATE,
      "Evoke Callback no-effect testing requires HLA_IMMEDIATE");
  require(
      !options.fom.empty(),
      "Evoke Callback no-effect testing requires an adapter-supplied model FOM");

  Session subject(options, model, "evoke-callback-immediate-no-effect");
  auto const federation =
      federationName(options, "evoke-callback-immediate-no-effect");
  subject.connect();
  subject.rtiAmbassador().createFederationExecution(
      federation,
      options.fom.wstring(),
      options.logicalTimeImplementationName);
  subject.join(
      options.ownerFederateName + L"-evoke-callback-immediate-no-effect",
      options.federateType,
      federation);

  std::size_t reportCallbackCount = 0U;
  subject.recorder().setFederationExecutionsReportHook(
      [&] { ++reportCallbackCount; });
  subject.rtiAmbassador().disableCallbacks();
  subject.rtiAmbassador().listFederationExecutions();
  require(
      reportCallbackCount == 0U,
      "disabled HLA_IMMEDIATE callbacks were invoked before the no-effect check");

  constexpr double requestedWaitSeconds = 0.25;
  auto const started = std::chrono::steady_clock::now();
  static_cast<void>(
      subject.rtiAmbassador().evokeCallback(requestedWaitSeconds));
  auto const elapsed = std::chrono::steady_clock::now() - started;
  auto const noEffectDeadline = std::chrono::duration_cast<
      std::chrono::steady_clock::duration>(
      std::chrono::duration<double>(requestedWaitSeconds / 2.0));

  require(
      reportCallbackCount == 0U,
      "Evoke Callback invoked a queued HLA_IMMEDIATE callback despite having no effect");
  require(
      elapsed < noEffectDeadline,
      "Evoke Callback applied its wait argument in HLA_IMMEDIATE despite having no effect");

  subject.rtiAmbassador().enableCallbacks();
  require(
      reportCallbackCount == 1U,
      "enabling HLA_IMMEDIATE callbacks did not deliver the callback retained by Evoke Callback");
  subject.recorder().setFederationExecutionsReportHook({});
  subject.resign(rti::NO_ACTION);
  subject.rtiAmbassador().destroyFederationExecution(federation);
}

void scenarioEvokeMultipleCallbacksPendingIndicator(
    Options const& options,
    rti::CallbackModel model) {
  using PendingIndicator = decltype(
      std::declval<rti::RTIambassador&>().evokeMultipleCallbacks(0.0, 0.0));
  static_assert(std::is_same_v<PendingIndicator, bool>);
  require(
      model == rti::HLA_EVOKED || model == rti::HLA_IMMEDIATE,
      "Evoke Multiple Callbacks pending-indicator testing requires a supported callback model");
  require(
      !options.fom.empty(),
      "Evoke Multiple Callbacks pending-indicator testing requires an adapter-supplied model FOM");

  Session subject(options, model, "evoke-multiple-callbacks-pending-indicator");
  auto const federation = federationName(
      options,
      "evoke-multiple-callbacks-pending-indicator");
  subject.connect();
  subject.rtiAmbassador().createFederationExecution(
      federation,
      options.fom.wstring(),
      options.logicalTimeImplementationName);
  subject.join(
      options.ownerFederateName + L"-evoke-multiple-callbacks-pending-indicator",
      options.federateType,
      federation);

  if (model == rti::HLA_IMMEDIATE) {
    std::size_t reportCallbackCount = 0U;
    subject.recorder().setFederationExecutionsReportHook(
        [&] { ++reportCallbackCount; });
    subject.rtiAmbassador().disableCallbacks();
    subject.rtiAmbassador().listFederationExecutions();
    require(
        reportCallbackCount == 0U,
        "disabled HLA_IMMEDIATE callbacks were invoked before the no-effect check");

    constexpr double requestedMinimumWaitSeconds = 0.25;
    constexpr double requestedMaximumWaitSeconds = 0.50;
    auto const started = std::chrono::steady_clock::now();
    static_cast<void>(subject.rtiAmbassador().evokeMultipleCallbacks(
        requestedMinimumWaitSeconds, requestedMaximumWaitSeconds));
    auto const elapsed = std::chrono::steady_clock::now() - started;
    auto const noEffectDeadline = std::chrono::duration_cast<
        std::chrono::steady_clock::duration>(
        std::chrono::duration<double>(requestedMinimumWaitSeconds / 2.0));

    require(
        reportCallbackCount == 0U,
        "Evoke Multiple Callbacks invoked a queued HLA_IMMEDIATE callback "
        "despite having no effect");
    require(
        elapsed < noEffectDeadline,
        "Evoke Multiple Callbacks applied its wait arguments in HLA_IMMEDIATE "
        "despite having no effect");

    subject.rtiAmbassador().enableCallbacks();
    require(
        reportCallbackCount == 1U,
        "enabling HLA_IMMEDIATE callbacks did not deliver the callback retained "
        "by Evoke Multiple Callbacks");
    subject.recorder().setFederationExecutionsReportHook({});
    subject.resign(rti::NO_ACTION);
    subject.rtiAmbassador().destroyFederationExecution(federation);
    return;
  }

  auto const emptyQueuePending =
      subject.rtiAmbassador().evokeMultipleCallbacks(0.0, 0.0);
  require(
      !emptyQueuePending,
      "Evoke Multiple Callbacks reported pending callbacks for an empty queue");

  std::size_t reportCallbackCount = 0U;
  subject.recorder().setFederationExecutionsReportHook([&] {
    ++reportCallbackCount;
  });
  subject.rtiAmbassador().disableCallbacks();
  subject.rtiAmbassador().listFederationExecutions();

  auto const pendingDeadline =
      std::chrono::steady_clock::now() + std::chrono::seconds(1);
  bool callbackPendingWhileDisabled = false;
  while (std::chrono::steady_clock::now() < pendingDeadline) {
    callbackPendingWhileDisabled =
        subject.rtiAmbassador().evokeMultipleCallbacks(0.0, 0.0);
    if (callbackPendingWhileDisabled) {
      break;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
  require(
      reportCallbackCount == 0U,
      "disabled HLA_EVOKED callbacks were invoked before the pending-indicator check");
  require(
      callbackPendingWhileDisabled,
      "Evoke Multiple Callbacks did not report TRUE while the federation-list callback was pending");

  subject.rtiAmbassador().enableCallbacks();
  auto const callbackPendingAfterDrain =
      subject.rtiAmbassador().evokeMultipleCallbacks(0.001, 0.050);
  require(
      reportCallbackCount == 1U,
      "Evoke Multiple Callbacks did not deliver the queued federation-list callback after callbacks were enabled");
  require(
      !callbackPendingAfterDrain,
      "Evoke Multiple Callbacks returned TRUE after the only remaining callback was consumed");

  subject.recorder().setFederationExecutionsReportHook({});
  subject.resign(rti::NO_ACTION);
  subject.rtiAmbassador().destroyFederationExecution(federation);
}

void scenarioDisableCallbacksGateEvokeServices(
    Options const& options,
    rti::CallbackModel model) {
  require(
      model == rti::HLA_EVOKED,
      "Disable Callbacks gating of evoke services requires HLA_EVOKED");
  require(
      !options.fom.empty(),
      "Disable Callbacks gating of evoke services requires an adapter-supplied model FOM");

  Session subject(options, model, "disable-callbacks-gates-evoke-services");
  auto const federation =
      federationName(options, "disable-callbacks-gates-evoke-services");
  subject.connect();
  subject.rtiAmbassador().createFederationExecution(
      federation,
      options.fom.wstring(),
      options.logicalTimeImplementationName);
  subject.join(
      options.ownerFederateName + L"-disable-callbacks-gates-evoke-services",
      options.federateType,
      federation);

  std::size_t reportCallbackCount = 0U;
  subject.recorder().setFederationExecutionsReportHook(
      [&] { ++reportCallbackCount; });

  subject.rtiAmbassador().disableCallbacks();
  subject.rtiAmbassador().listFederationExecutions();
  auto const disabledPendingDeadline =
      std::chrono::steady_clock::now() + std::chrono::seconds(1);
  bool callbackPendingWhileDisabled = false;
  while (std::chrono::steady_clock::now() < disabledPendingDeadline) {
    callbackPendingWhileDisabled =
        subject.rtiAmbassador().evokeMultipleCallbacks(0.0, 0.0);
    if (callbackPendingWhileDisabled) {
      break;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
  require(
      callbackPendingWhileDisabled,
      "the federation-list callback did not become pending while callbacks were disabled");
  static_cast<void>(subject.rtiAmbassador().evokeCallback(0.0));
  auto const pendingAfterSingleEvoke =
      subject.rtiAmbassador().evokeMultipleCallbacks(0.0, 0.0);
  require(
      reportCallbackCount == 0U && pendingAfterSingleEvoke,
      "Evoke Callback processed or discarded a pending callback while callbacks were disabled");
  static_cast<void>(subject.rtiAmbassador().evokeMultipleCallbacks(0.0, 0.0));
  require(
      reportCallbackCount == 0U,
      "Evoke Multiple Callbacks processed a callback while callbacks were disabled");

  subject.rtiAmbassador().enableCallbacks();
  auto const singleEvokeDeadline =
      std::chrono::steady_clock::now() + std::chrono::seconds(1);
  while (reportCallbackCount == 0U &&
         std::chrono::steady_clock::now() < singleEvokeDeadline) {
    static_cast<void>(subject.rtiAmbassador().evokeCallback(0.0));
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
  require(
      reportCallbackCount == 1U,
      "Evoke Callback did not process a pending callback after callbacks were enabled");

  subject.rtiAmbassador().listFederationExecutions();
  auto const multipleEvokeDeadline =
      std::chrono::steady_clock::now() + std::chrono::seconds(1);
  while (reportCallbackCount == 1U &&
         std::chrono::steady_clock::now() < multipleEvokeDeadline) {
    static_cast<void>(
        subject.rtiAmbassador().evokeMultipleCallbacks(0.0, 0.0));
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
  require(
      reportCallbackCount == 2U,
      "Evoke Multiple Callbacks did not process a pending callback after callbacks were enabled");

  std::promise<void> callbackEntered;
  auto callbackEnteredSignal = callbackEntered.get_future();
  std::promise<void> releaseCallback;
  auto releaseCallbackSignal = releaseCallback.get_future().share();
  std::size_t concurrentCallbackCount = 0U;
  std::thread::id evokeThread;
  std::thread::id callbackThread;
  subject.recorder().setFederationExecutionsReportHook([&] {
    ++concurrentCallbackCount;
    if (concurrentCallbackCount == 1U) {
      callbackThread = std::this_thread::get_id();
      callbackEntered.set_value();
      releaseCallbackSignal.wait();
    }
  });
  subject.rtiAmbassador().listFederationExecutions();
  subject.rtiAmbassador().listFederationExecutions();

  std::exception_ptr evokeFailure;
  bool pendingAfterConcurrentDisable = false;
  std::thread evoker([&] {
    evokeThread = std::this_thread::get_id();
    try {
      pendingAfterConcurrentDisable =
          subject.rtiAmbassador().evokeMultipleCallbacks(0.0, 1.0);
    } catch (...) {
      evokeFailure = std::current_exception();
    }
  });

  if (callbackEnteredSignal.wait_for(std::chrono::seconds(2)) !=
      std::future_status::ready) {
    releaseCallback.set_value();
    evoker.join();
    require(
        false,
        "Evoke Multiple Callbacks did not enter the queued callback on its caller thread");
  }

  std::promise<void> disableStarted;
  auto disableStartedSignal = disableStarted.get_future();
  std::promise<void> disableReturned;
  auto disableReturnedSignal = disableReturned.get_future();
  std::exception_ptr disableFailure;
  std::thread disabler([&] {
    disableStarted.set_value();
    try {
      subject.rtiAmbassador().disableCallbacks();
    } catch (...) {
      disableFailure = std::current_exception();
    }
    disableReturned.set_value();
  });
  disableStartedSignal.wait();
  auto const disableReturnedWhileCallbackWasHeld =
      disableReturnedSignal.wait_for(std::chrono::milliseconds(500)) ==
      std::future_status::ready;
  releaseCallback.set_value();
  evoker.join();
  disabler.join();

  if (disableFailure != nullptr) {
    try {
      std::rethrow_exception(disableFailure);
    } catch (std::exception const& error) {
      require(false, "concurrent Disable Callbacks failed: " +
                        std::string(error.what()));
    } catch (...) {
      require(false, "concurrent Disable Callbacks failed unexpectedly");
    }
  }
  if (evokeFailure != nullptr) {
    try {
      std::rethrow_exception(evokeFailure);
    } catch (std::exception const& error) {
      require(false, "concurrent Evoke Multiple Callbacks failed: " +
                        std::string(error.what()));
    } catch (...) {
      require(false, "concurrent Evoke Multiple Callbacks failed unexpectedly");
    }
  }
  require(
      !disableReturnedWhileCallbackWasHeld,
      "Disable Callbacks returned while the in-flight FederateAmbassador callback was held open");
  require(
      callbackThread == evokeThread,
      "HLA_EVOKED callback did not use the Evoke Multiple Callbacks thread context");
  require(
      pendingAfterConcurrentDisable && concurrentCallbackCount == 1U,
      "concurrent Disable Callbacks did not stop the next callback and leave it pending");

  subject.rtiAmbassador().enableCallbacks();
  auto const pendingAfterFinalDrain =
      subject.rtiAmbassador().evokeMultipleCallbacks(0.0, 0.0);
  require(
      !pendingAfterFinalDrain && concurrentCallbackCount == 2U,
      "the callback retained by concurrent Disable Callbacks was not delivered after re-enabling");

  subject.recorder().setFederationExecutionsReportHook({});
  subject.resign(rti::NO_ACTION);
  subject.rtiAmbassador().destroyFederationExecution(federation);
}

void scenarioEvokedCallbacksAreNotConcurrent(
    Options const& options,
    rti::CallbackModel model) {
  require(
      model == rti::HLA_EVOKED,
      "non-concurrent callback delivery requires HLA_EVOKED");
  require(
      !options.fom.empty(),
      "non-concurrent callback delivery requires an adapter-supplied model FOM");

  Session subject(options, model, "evoked-callback-no-concurrency");
  auto const federation =
      federationName(options, "evoked-callback-no-concurrency");
  subject.connect();
  subject.rtiAmbassador().createFederationExecution(
      federation,
      options.fom.wstring(),
      options.logicalTimeImplementationName);
  subject.join(
      options.ownerFederateName + L"-evoked-callback-no-concurrency",
      options.federateType,
      federation);

  std::promise<void> firstCallbackEntered;
  auto firstCallbackEnteredSignal = firstCallbackEntered.get_future();
  std::promise<void> releaseFirstCallback;
  auto releaseFirstCallbackSignal = releaseFirstCallback.get_future().share();
  std::promise<void> secondCallbackEntered;
  auto secondCallbackEnteredSignal = secondCallbackEntered.get_future();
  std::atomic<unsigned> callbackCount{0U};
  std::atomic<unsigned> activeCallbacks{0U};
  std::atomic<unsigned> maximumActiveCallbacks{0U};
  subject.recorder().setFederationExecutionsReportHook([&] {
    auto const active = activeCallbacks.fetch_add(1U) + 1U;
    auto maximum = maximumActiveCallbacks.load();
    while (maximum < active &&
           !maximumActiveCallbacks.compare_exchange_weak(maximum, active)) {
    }

    auto const ordinal = callbackCount.fetch_add(1U) + 1U;
    if (ordinal == 1U) {
      firstCallbackEntered.set_value();
      releaseFirstCallbackSignal.wait();
    } else if (ordinal == 2U) {
      secondCallbackEntered.set_value();
    }
    activeCallbacks.fetch_sub(1U);
  });

  subject.rtiAmbassador().listFederationExecutions();
  subject.rtiAmbassador().listFederationExecutions();

  std::exception_ptr firstEvokeFailure;
  std::thread firstEvoke([&] {
    try {
      static_cast<void>(subject.rtiAmbassador().evokeCallback(0.25));
    } catch (...) {
      firstEvokeFailure = std::current_exception();
    }
  });
  auto const firstCallbackWasEntered =
      firstCallbackEnteredSignal.wait_for(std::chrono::seconds(2)) ==
      std::future_status::ready;

  std::exception_ptr secondEvokeFailure;
  std::thread secondEvoke;
  bool secondEvokeStartedBeforeRelease = false;
  bool secondCallbackEnteredWhileFirstWasOpen = false;
  if (firstCallbackWasEntered) {
    std::promise<void> secondEvokeStarted;
    auto secondEvokeStartedSignal = secondEvokeStarted.get_future();
    secondEvoke = std::thread([&] {
      secondEvokeStarted.set_value();
      try {
        static_cast<void>(subject.rtiAmbassador().evokeCallback(0.25));
      } catch (...) {
        secondEvokeFailure = std::current_exception();
      }
    });
    secondEvokeStartedBeforeRelease =
        secondEvokeStartedSignal.wait_for(std::chrono::seconds(2)) ==
        std::future_status::ready;
    if (secondEvokeStartedBeforeRelease) {
      secondCallbackEnteredWhileFirstWasOpen =
          secondCallbackEnteredSignal.wait_for(std::chrono::milliseconds(500)) ==
          std::future_status::ready;
    }
  }

  releaseFirstCallback.set_value();
  firstEvoke.join();
  if (secondEvoke.joinable()) {
    secondEvoke.join();
  }
  subject.recorder().setFederationExecutionsReportHook({});
  subject.resign(rti::NO_ACTION);
  subject.rtiAmbassador().destroyFederationExecution(federation);

  auto requireNoThreadFailure = [](std::exception_ptr const& failure,
                                   std::string const& service) {
    if (failure == nullptr) {
      return;
    }
    try {
      std::rethrow_exception(failure);
    } catch (std::exception const& error) {
      require(false, service + " failed: " + error.what());
    } catch (...) {
      require(false, service + " failed unexpectedly");
    }
  };
  requireNoThreadFailure(firstEvokeFailure, "first concurrent Evoke Callback");
  requireNoThreadFailure(secondEvokeFailure, "second concurrent Evoke Callback");
  require(
      firstCallbackWasEntered,
      "the first concurrent Evoke Callback did not enter the queued callback");
  require(
      secondEvokeStartedBeforeRelease,
      "the second Evoke Callback call did not start while the first callback was open");
  require(
      !secondCallbackEnteredWhileFirstWasOpen,
      "the RTI entered a second FederateAmbassador callback while the first was in progress");
  require(
      callbackCount.load() == 2U,
      "the two concurrent Evoke Callback calls did not deliver both queued callbacks");
  require(
      maximumActiveCallbacks.load() == 1U && activeCallbacks.load() == 0U,
      "RTI-initiated calls overlapped on the same FederateAmbassador instance");
}

void scenarioConcurrentCallsDifferentAmbassadors(
    Options const& options,
    rti::CallbackModel model) {
  require(
      model == rti::HLA_EVOKED,
      "different-ambassador concurrency requires HLA_EVOKED");
  require(
      !options.fom.empty(),
      "different-ambassador concurrency requires an adapter-supplied model FOM");

  Session owner(options, model, "different-ambassadors-owner");
  Session member(options, model, "different-ambassadors-member");
  auto const federation = federationName(options, "different-ambassadors");
  connectAndJoin(owner, member, options, federation, options.fom);
  require(
      &owner.rtiAmbassador() != &member.rtiAmbassador(),
      "two sessions unexpectedly shared one RTIambassador instance");

  std::size_t ownerReportCount = 0U;
  std::size_t memberReportCount = 0U;
  std::promise<void> ownerCallbackEntered;
  auto ownerCallbackEnteredSignal = ownerCallbackEntered.get_future();
  std::promise<void> releaseOwnerCallback;
  auto releaseOwnerCallbackSignal = releaseOwnerCallback.get_future().share();
  owner.recorder().setFederationExecutionsReportHook([&] {
    ++ownerReportCount;
    if (ownerReportCount == 1U) {
      ownerCallbackEntered.set_value();
      releaseOwnerCallbackSignal.wait();
    }
  });
  member.recorder().setFederationExecutionsReportHook(
      [&] { ++memberReportCount; });

  owner.rtiAmbassador().listFederationExecutions();
  auto const memberInteractionClass =
      member.rtiAmbassador().getInteractionClassHandle(L"HLAinteractionRoot");
  require(
      memberInteractionClass.isValid() &&
          member.rtiAmbassador().getInteractionClassName(memberInteractionClass) ==
              L"HLAinteractionRoot",
      "the member session did not match its interaction-class request with the correct response while the owner list response was pending");

  std::exception_ptr ownerEvokeFailure;
  std::thread ownerEvoker([&] {
    try {
      static_cast<void>(owner.rtiAmbassador().evokeCallback(0.25));
    } catch (...) {
      ownerEvokeFailure = std::current_exception();
    }
  });
  auto const ownerCallbackWasEntered =
      ownerCallbackEnteredSignal.wait_for(std::chrono::seconds(2)) ==
      std::future_status::ready;

  std::exception_ptr memberListFailure;
  if (ownerCallbackWasEntered) {
    std::promise<void> memberServiceStarting;
    auto memberServiceStartingSignal = memberServiceStarting.get_future().share();
    std::thread callbackReleaser([&] {
      memberServiceStartingSignal.wait();
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
      releaseOwnerCallback.set_value();
    });
    memberServiceStarting.set_value();
    try {
      member.rtiAmbassador().listFederationExecutions();
    } catch (...) {
      memberListFailure = std::current_exception();
    }
    callbackReleaser.join();
  } else {
    releaseOwnerCallback.set_value();
  }
  ownerEvoker.join();

  std::exception_ptr memberEvokeFailure;
  if (ownerCallbackWasEntered && memberListFailure == nullptr) {
    try {
      static_cast<void>(member.rtiAmbassador().evokeCallback(0.25));
    } catch (...) {
      memberEvokeFailure = std::current_exception();
    }
  }

  owner.recorder().setFederationExecutionsReportHook({});
  member.recorder().setFederationExecutionsReportHook({});
  member.resign(rti::NO_ACTION);
  owner.resign(rti::NO_ACTION);
  owner.rtiAmbassador().destroyFederationExecution(federation);

  auto requireNoThreadFailure = [](std::exception_ptr const& failure,
                                   std::string const& service) {
    if (failure == nullptr) {
      return;
    }
    try {
      std::rethrow_exception(failure);
    } catch (std::exception const& error) {
      require(false, service + " failed: " + error.what());
    } catch (...) {
      require(false, service + " failed unexpectedly");
    }
  };
  requireNoThreadFailure(ownerEvokeFailure, "owner Evoke Callback");
  requireNoThreadFailure(memberListFailure, "member List Federation Executions");
  requireNoThreadFailure(memberEvokeFailure, "member Evoke Callback");
  require(
      ownerCallbackWasEntered && ownerReportCount == 1U,
      "the owner did not complete its deliberately held federation-list callback");
  require(
      memberReportCount == 1U,
      "the second RTIambassador did not process its concurrent joined service and callback");
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

void scenarioFomNameCaseSensitivity(
    Options const& options,
    rti::CallbackModel model) {
  Session owner(options, model, "fom-name-case-owner");
  Session member(options, model, "fom-name-case-member");
  auto const federation = federationName(options, "fom-name-case-sensitivity");
  connectAndJoin(owner, member, options, federation, options.fom);

  auto lowerFirstAscii = [](std::wstring name) {
    require(!name.empty(), "case-sensitivity fixture name was empty");
    auto& first = name.front();
    if (first >= L'A' && first <= L'Z') {
      first = static_cast<wchar_t>(first - L'A' + L'a');
    } else if (first >= L'a' && first <= L'z') {
      first = static_cast<wchar_t>(first - L'a' + L'A');
    } else {
      throw std::runtime_error(
          "case-sensitivity fixture name did not begin with an ASCII letter");
    }
    return name;
  };

  auto& ambassador = owner.rtiAmbassador();
  auto unqualifiedClassName = [](std::wstring const& name) {
    auto const separator = name.rfind(L'.');
    require(
        separator != std::wstring::npos && separator + 1 < name.size(),
        "fully-qualified-class-name fixture omitted its superclass path");
    return name.substr(separator + 1);
  };

  auto const objectClass =
      ambassador.getObjectClassHandle(options.objectClassName);
  require(
      objectClass.isValid() &&
          ambassador.getObjectClassName(objectClass) == options.objectClassName,
      "exact-case object-class lookup did not preserve its FOM name");
  auto const objectRoot =
      ambassador.getObjectClassHandle(L"HLAobjectRoot");
  require(
      objectRoot.isValid() &&
          ambassador.getObjectClassName(objectRoot) == L"HLAobjectRoot",
      "the HLA object root class was not accepted by its root name");
  auto const unqualifiedObjectClass =
      unqualifiedClassName(options.objectClassName);
  requireException(
      [&] {
        static_cast<void>(
            ambassador.getObjectClassHandle(unqualifiedObjectClass));
      },
      L"NameNotFound",
      "object-class lookup omitting its superclass path");
  auto const lowerCaseObjectClass = lowerFirstAscii(options.objectClassName);
  requireException(
      [&] {
        static_cast<void>(
            ambassador.getObjectClassHandle(lowerCaseObjectClass));
      },
      L"NameNotFound",
      "case-mismatched object-class name lookup");

  auto const attribute =
      ambassador.getAttributeHandle(objectClass, options.attributeName);
  require(
      attribute.isValid() &&
          ambassador.getAttributeName(objectClass, attribute) ==
              options.attributeName,
      "exact-case attribute lookup did not preserve its FOM name");
  auto const lowerCaseAttribute = lowerFirstAscii(options.attributeName);
  requireException(
      [&] {
        static_cast<void>(
            ambassador.getAttributeHandle(objectClass, lowerCaseAttribute));
      },
      L"NameNotFound",
      "case-mismatched attribute name lookup");

  auto const interactionClass =
      ambassador.getInteractionClassHandle(options.interactionClassName);
  require(
      interactionClass.isValid() &&
          ambassador.getInteractionClassName(interactionClass) ==
              options.interactionClassName,
      "exact-case interaction-class lookup did not preserve its FOM name");
  auto const interactionRoot =
      ambassador.getInteractionClassHandle(L"HLAinteractionRoot");
  require(
      interactionRoot.isValid() &&
          ambassador.getInteractionClassName(interactionRoot) ==
              L"HLAinteractionRoot",
      "the HLA interaction root class was not accepted by its root name");
  auto const unqualifiedInteractionClass =
      unqualifiedClassName(options.interactionClassName);
  requireException(
      [&] {
        static_cast<void>(ambassador.getInteractionClassHandle(
            unqualifiedInteractionClass));
      },
      L"NameNotFound",
      "interaction-class lookup omitting its superclass path");
  auto const lowerCaseInteractionClass =
      lowerFirstAscii(options.interactionClassName);
  requireException(
      [&] {
        static_cast<void>(
            ambassador.getInteractionClassHandle(lowerCaseInteractionClass));
      },
      L"NameNotFound",
      "case-mismatched interaction-class name lookup");

  auto const parameter =
      ambassador.getParameterHandle(interactionClass, options.parameterName);
  require(
      parameter.isValid() &&
          ambassador.getParameterName(interactionClass, parameter) ==
              options.parameterName,
      "exact-case parameter lookup did not preserve its FOM name");
  auto const lowerCaseParameter = lowerFirstAscii(options.parameterName);
  requireException(
      [&] {
        static_cast<void>(
            ambassador.getParameterHandle(interactionClass, lowerCaseParameter));
      },
      L"NameNotFound",
      "case-mismatched parameter name lookup");

  owner.resign(rti::NO_ACTION);
  member.resign(rti::NO_ACTION);
  ambassador.destroyFederationExecution(federation);
  member.disconnect();
  owner.disconnect();

  auto const rootlessFom =
      options.scenarioFoms.find("fom-root-classes-omitted");
  require(
      rootlessFom != options.scenarioFoms.end(),
      "FOM name lookup requires a root-class-omitted FOM module");
  Session rootlessOwner(options, model, "fom-rootless-owner");
  auto const rootlessFederation =
      federationName(options, "fom-name-case-sensitivity-rootless");
  rootlessOwner.connect();
  rootlessOwner.rtiAmbassador().createFederationExecution(
      rootlessFederation,
      std::vector<std::wstring>{rootlessFom->second.wstring()},
      options.logicalTimeImplementationName);
  rootlessOwner.join(
      options.ownerFederateName, options.federateType, rootlessFederation);
  auto& rootlessAmbassador = rootlessOwner.rtiAmbassador();
  auto const rootlessObjectRoot =
      rootlessAmbassador.getObjectClassHandle(L"HLAobjectRoot");
  auto const rootlessInteractionRoot =
      rootlessAmbassador.getInteractionClassHandle(L"HLAinteractionRoot");
  auto const rootlessObject = rootlessAmbassador.getObjectClassHandle(
      L"HLAobjectRoot.TckObject");
  auto const rootlessInteraction =
      rootlessAmbassador.getInteractionClassHandle(
          L"HLAinteractionRoot.TckInteraction");
  require(
      rootlessAmbassador.getObjectClassName(rootlessObjectRoot) ==
              L"HLAobjectRoot" &&
          rootlessAmbassador.getInteractionClassName(rootlessInteractionRoot) ==
              L"HLAinteractionRoot" &&
          rootlessAmbassador.getObjectClassName(rootlessObject) ==
              L"HLAobjectRoot.TckObject" &&
          rootlessAmbassador.getInteractionClassName(rootlessInteraction) ==
              L"HLAinteractionRoot.TckInteraction",
      "FOMs omitting standard root declarations did not resolve full class paths");
  rootlessOwner.resign(rti::NO_ACTION);
  rootlessAmbassador.destroyFederationExecution(rootlessFederation);
  rootlessOwner.disconnect();
}

void scenarioFomNameCaseSensitivityContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioFomNameCaseSensitivity(options, model);
}

void scenarioFomHandleUniqueness(
    Options const& options,
    rti::CallbackModel model) {
  Session owner(options, model, "fom-handle-owner");
  Session member(options, model, "fom-handle-member");
  auto const federation = federationName(options, "fom-handle-uniqueness");
  auto const scenarioFom = options.scenarioFoms.find("fom-handle-uniqueness");
  require(
      scenarioFom != options.scenarioFoms.end(),
      "federation-wide FOM handle scenario requires its dedicated FOM");
  connectAndJoin(owner, member, options, federation, scenarioFom->second);

  auto collectHandleEncodings = [](Session& session) {
    auto& ambassador = session.rtiAmbassador();
    auto const reliableTransportation =
        ambassador.getTransportationTypeHandle(L"HLAreliable");
    auto const bestEffortTransportation =
        ambassador.getTransportationTypeHandle(L"HLAbestEffort");
    auto const additionalTransportation =
        ambassador.getTransportationTypeHandle(L"TckHandleTransportation");
    require(
        reliableTransportation.isValid() && bestEffortTransportation.isValid(),
        "RTI did not provide valid handles for both standard transportation types");
    require(
        ambassador.getTransportationTypeName(reliableTransportation) ==
                L"HLAreliable" &&
            ambassador.getTransportationTypeName(bestEffortTransportation) ==
                L"HLAbestEffort",
        "standard transportation handles did not resolve to their required names");
    require(
        additionalTransportation.isValid(),
        "FDD-declared additional transportation type returned an invalid handle");
    require(
        ambassador.getTransportationTypeName(additionalTransportation) ==
            L"TckHandleTransportation",
        "FDD-declared additional transportation handle did not resolve to its name");
    auto const objectClass =
        ambassador.getObjectClassHandle(L"HLAobjectRoot.TckHandleObject");
    auto const objectChild = ambassador.getObjectClassHandle(
        L"HLAobjectRoot.TckHandleObject.TckHandleObjectChild");
    auto const interaction = ambassador.getInteractionClassHandle(
        L"HLAinteractionRoot.TckHandleInteraction");
    auto const interactionChild = ambassador.getInteractionClassHandle(
        L"HLAinteractionRoot.TckHandleInteraction.TckHandleInteractionChild");
    std::vector<std::string> handles{
        encodedFomHandle(ambassador.getObjectClassHandle(L"HLAobjectRoot")),
        encodedFomHandle(objectClass),
        encodedFomHandle(objectChild),
        encodedFomHandle(ambassador.getAttributeHandle(
            objectClass, L"TckHandleAttribute")),
        encodedFomHandle(ambassador.getAttributeHandle(
            objectChild, L"TckHandleChildAttribute")),
        encodedFomHandle(
            ambassador.getInteractionClassHandle(L"HLAinteractionRoot")),
        encodedFomHandle(interaction),
        encodedFomHandle(interactionChild),
        encodedFomHandle(ambassador.getParameterHandle(
            interaction, L"TckHandleParameter")),
        encodedFomHandle(ambassador.getParameterHandle(
            interactionChild, L"TckHandleChildParameter")),
        encodedFomHandle(ambassador.getDimensionHandle(L"TckHandleDimension")),
        encodedFomHandle(
            ambassador.getDimensionHandle(L"TckHandleDimensionSecond")),
        encodedFomHandle(reliableTransportation),
        encodedFomHandle(bestEffortTransportation),
        encodedFomHandle(additionalTransportation),
    };

    require(
        ambassador.getOrderType(L"Receive") !=
            ambassador.getOrderType(L"TimeStamp"),
        "distinct standard order names did not resolve to distinct order values");
    return handles;
  };

  auto const ownerHandles = collectHandleEncodings(owner);
  auto const memberHandles = collectHandleEncodings(member);
  require(
      ownerHandles == memberHandles,
      "federates in one execution received different encodings for the same FOM names");
  require(
      std::set<std::string>(ownerHandles.begin(), ownerHandles.end()).size() ==
          ownerHandles.size(),
      "distinct FOM names in one execution shared an encoded handle value");

  auto const firstExecutionFddHandles = encodedFddNamedHandles(owner);

  owner.resign(rti::NO_ACTION);
  member.resign(rti::NO_ACTION);
  owner.rtiAmbassador().destroyFederationExecution(federation);
  member.disconnect();
  owner.disconnect();

  Session repeatedOwner(options, model, "fom-handle-repeatability-owner");
  Session repeatedMember(options, model, "fom-handle-repeatability-member");
  auto const repeatedFederation =
      federationName(options, "fom-handle-repeatability");
  connectAndJoin(
      repeatedOwner, repeatedMember, options, repeatedFederation,
      scenarioFom->second);
  auto const repeatedExecutionFddHandles =
      encodedFddNamedHandles(repeatedOwner);
  require(
      firstExecutionFddHandles == repeatedExecutionFddHandles,
      "FDD-derived handles changed across executions using the same RTI and FDD");

  repeatedOwner.resign(rti::NO_ACTION);
  repeatedMember.resign(rti::NO_ACTION);
  repeatedOwner.rtiAmbassador().destroyFederationExecution(repeatedFederation);
  repeatedMember.disconnect();
  repeatedOwner.disconnect();
}

} // namespace hla_rti_cpp_tck::detail
