#include "tck_main_scenarios.hpp"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <string>
#include <thread>
#include <vector>

namespace hla_rti_cpp_tck::detail {

void scenarioTimestampedDirectedInteractionSubscriptionKind(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.logicalTimeImplementationName.empty(),
      "Timestamped directed-interaction subscriptions require an adapter-supplied logical-time implementation");

  Session owner(options, model, "owner");
  Session sender(options, model, "member");
  Session ownershipSubscriber(options, model, "member");
  Session universalSubscriber(options, model, "member");
  auto const federation = federationName(
      options,
      "timestamped-directed-interaction-subscription-kind");

  owner.connect();
  sender.connect();
  ownershipSubscriber.connect();
  universalSubscriber.connect();
  owner.rtiAmbassador().createFederationExecution(
      federation,
      options.fom.wstring(),
      options.logicalTimeImplementationName);
  owner.join(
      options.ownerFederateName + L"-timestamped-directed-owner",
      options.federateType,
      federation);
  sender.join(
      options.memberFederateName + L"-timestamped-directed-sender",
      options.federateType,
      federation);
  ownershipSubscriber.join(
      options.memberFederateName + L"-timestamped-directed-by-owner",
      options.federateType,
      federation);
  universalSubscriber.join(
      options.memberFederateName + L"-timestamped-directed-universal",
      options.federateType,
      federation);

  auto const ownerClass = owner.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const senderClass = sender.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const ownershipClass = ownershipSubscriber.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const universalClass = universalSubscriber.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const ownerAttribute = owner.rtiAmbassador().getAttributeHandle(
      ownerClass,
      options.attributeName);
  auto const senderAttribute = sender.rtiAmbassador().getAttributeHandle(
      senderClass,
      options.attributeName);
  auto const ownershipAttribute = ownershipSubscriber.rtiAmbassador().getAttributeHandle(
      ownershipClass,
      options.attributeName);
  auto const universalAttribute = universalSubscriber.rtiAmbassador().getAttributeHandle(
      universalClass,
      options.attributeName);
  auto const senderInteraction = sender.rtiAmbassador().getInteractionClassHandle(
      options.interactionClassName);
  auto const ownerInteraction = owner.rtiAmbassador().getInteractionClassHandle(
      options.interactionClassName);
  auto const ownershipInteraction =
      ownershipSubscriber.rtiAmbassador().getInteractionClassHandle(
          options.interactionClassName);
  auto const universalInteraction =
      universalSubscriber.rtiAmbassador().getInteractionClassHandle(
          options.interactionClassName);
  auto const senderParameter = sender.rtiAmbassador().getParameterHandle(
      senderInteraction,
      options.parameterName);
  auto const ownerParameter = owner.rtiAmbassador().getParameterHandle(
      ownerInteraction,
      options.parameterName);
  auto const ownershipParameter =
      ownershipSubscriber.rtiAmbassador().getParameterHandle(
          ownershipInteraction,
          options.parameterName);
  auto const universalParameter =
      universalSubscriber.rtiAmbassador().getParameterHandle(
          universalInteraction,
          options.parameterName);
  require(
      ownerClass.isValid() && senderClass.isValid() && ownershipClass.isValid() &&
          universalClass.isValid() && ownerAttribute.isValid() &&
          senderAttribute.isValid() && ownershipAttribute.isValid() &&
          universalAttribute.isValid() && senderInteraction.isValid() &&
          ownerInteraction.isValid() && ownershipInteraction.isValid() &&
          universalInteraction.isValid() && senderParameter.isValid() &&
          ownerParameter.isValid() && ownershipParameter.isValid() &&
          universalParameter.isValid(),
      "timestamped directed-interaction subscription lookup returned an invalid handle");

  rti::AttributeHandleSet const ownerAttributes{ownerAttribute};
  rti::AttributeHandleSet const senderAttributes{senderAttribute};
  rti::AttributeHandleSet const ownershipAttributes{ownershipAttribute};
  rti::AttributeHandleSet const universalAttributes{universalAttribute};
  owner.rtiAmbassador().publishObjectClassAttributes(ownerClass, ownerAttributes);
  sender.rtiAmbassador().subscribeObjectClassAttributes(
      senderClass,
      senderAttributes,
      true,
      L"");
  ownershipSubscriber.rtiAmbassador().subscribeObjectClassAttributes(
      ownershipClass,
      ownershipAttributes,
      true,
      L"");
  universalSubscriber.rtiAmbassador().subscribeObjectClassAttributes(
      universalClass,
      universalAttributes,
      true,
      L"");

  rti::InteractionClassHandleSet const senderDirected{senderInteraction};
  rti::InteractionClassHandleSet const ownerDirected{ownerInteraction};
  rti::InteractionClassHandleSet const ownershipDirected{ownershipInteraction};
  rti::InteractionClassHandleSet const universalDirected{universalInteraction};
  sender.rtiAmbassador().publishObjectClassDirectedInteractions(
      senderClass,
      senderDirected);
  owner.rtiAmbassador().subscribeObjectClassDirectedInteractions(
      ownerClass,
      ownerDirected,
      false);
  ownershipSubscriber.rtiAmbassador().subscribeObjectClassDirectedInteractions(
      ownershipClass,
      ownershipDirected,
      false);
  universalSubscriber.rtiAmbassador().subscribeObjectClassDirectedInteractions(
      universalClass,
      universalDirected,
      true);
  sender.rtiAmbassador().changeInteractionOrderType(
      senderInteraction,
      rti::TIMESTAMP);

  auto const target = owner.rtiAmbassador().registerObjectInstance(ownerClass);
  require(target.isValid(), "timestamped directed-interaction target was invalid");
  waitFor(
      sender,
      [&] { return sender.recorder().hasDiscovery(target); },
      options,
      "timestamped directed-interaction sender discovery");
  waitFor(
      ownershipSubscriber,
      [&] { return ownershipSubscriber.recorder().hasDiscovery(target); },
      options,
      "timestamped directed-interaction by-ownership subscriber discovery");
  waitFor(
      universalSubscriber,
      [&] { return universalSubscriber.recorder().hasDiscovery(target); },
      options,
      "timestamped directed-interaction universal subscriber discovery");

  auto ownerTime = makeTimeContext(owner);
  auto senderTime = makeTimeContext(sender);
  auto ownershipTime = makeTimeContext(ownershipSubscriber);
  auto universalTime = makeTimeContext(universalSubscriber);
  require(
      ownerTime.factory->getName() == senderTime.factory->getName() &&
          ownerTime.factory->getName() == ownershipTime.factory->getName() &&
          ownerTime.factory->getName() == universalTime.factory->getName(),
      "timestamped directed-interaction federates selected different time factories");

  owner.rtiAmbassador().enableTimeConstrained();
  waitFor(
      owner,
      [&] { return !owner.recorder().timeConstrainedEnabled().empty(); },
      options,
      "timestamped directed-interaction target owner's Time Constrained callback");
  ownershipSubscriber.rtiAmbassador().enableTimeConstrained();
  waitFor(
      ownershipSubscriber,
      [&] {
        return !ownershipSubscriber.recorder().timeConstrainedEnabled().empty();
      },
      options,
      "timestamped directed-interaction by-ownership Time Constrained callback");
  universalSubscriber.rtiAmbassador().enableTimeConstrained();
  waitFor(
      universalSubscriber,
      [&] {
        return !universalSubscriber.recorder().timeConstrainedEnabled().empty();
      },
      options,
      "timestamped directed-interaction universal Time Constrained callback");
  sender.rtiAmbassador().enableTimeRegulation(*senderTime.epsilon);
  waitFor(
      sender,
      [&] { return !sender.recorder().timeRegulationEnabled().empty(); },
      options,
      "timestamped directed-interaction sender's Time Regulation callback");

  auto const timestamp = timeAfter(
      *senderTime.factory,
      *senderTime.initial,
      *senderTime.epsilon,
      2U);
  auto const ownerTarget = timeAfter(
      *ownerTime.factory,
      *ownerTime.initial,
      *ownerTime.epsilon,
      2U);
  auto const ownershipTarget = timeAfter(
      *ownershipTime.factory,
      *ownershipTime.initial,
      *ownershipTime.epsilon,
      2U);
  auto const universalTarget = timeAfter(
      *universalTime.factory,
      *universalTime.initial,
      *universalTime.epsilon,
      2U);
  require(
      encodeTime(*timestamp) == encodeTime(*ownerTarget) &&
          encodeTime(*timestamp) == encodeTime(*ownershipTarget) &&
          encodeTime(*timestamp) == encodeTime(*universalTarget),
      "timestamped directed-interaction members constructed different advance targets");

  std::vector<std::uint8_t> const valueBytes{0xD1U, 0x52U};
  std::vector<std::uint8_t> const tagBytes{0x54U, 0x53U, 0x4FU};
  rti::ParameterHandleValueMap parameters;
  parameters.emplace(
      senderParameter,
      rti::VariableLengthData(valueBytes.data(), valueBytes.size()));
  rti::VariableLengthData tag(tagBytes.data(), tagBytes.size());
  auto const retraction = sender.rtiAmbassador().sendDirectedInteraction(
      senderInteraction,
      target,
      parameters,
      tag,
      *timestamp);
  require(
      retraction.isValid(),
      "timestamped directed-interaction send returned an invalid retraction handle");

  owner.pump();
  ownershipSubscriber.pump();
  universalSubscriber.pump();
  require(
      owner.recorder().timedDirectedInteractions().empty() &&
          ownershipSubscriber.recorder().timedDirectedInteractions().empty() &&
          universalSubscriber.recorder().timedDirectedInteractions().empty(),
      "timestamped directed interaction arrived before the constrained receivers advanced");
  owner.recorder().clearCallbackOrder();
  sender.recorder().clearCallbackOrder();
  ownershipSubscriber.recorder().clearCallbackOrder();
  universalSubscriber.recorder().clearCallbackOrder();

  owner.rtiAmbassador().timeAdvanceRequest(*ownerTarget);
  ownershipSubscriber.rtiAmbassador().timeAdvanceRequest(*ownershipTarget);
  universalSubscriber.rtiAmbassador().timeAdvanceRequest(*universalTarget);
  sender.rtiAmbassador().timeAdvanceRequest(*timestamp);

  auto const deadline = std::chrono::steady_clock::now() +
      std::chrono::milliseconds(options.timeoutMilliseconds);
  auto complete = [&] {
    return owner.recorder().timedDirectedInteractions().size() == 1U &&
        owner.recorder().timeAdvanceGrants().size() == 1U &&
        ownershipSubscriber.recorder().timedDirectedInteractions().empty() &&
        ownershipSubscriber.recorder().timeAdvanceGrants().size() == 1U &&
        universalSubscriber.recorder().timedDirectedInteractions().size() == 1U &&
        universalSubscriber.recorder().timeAdvanceGrants().size() == 1U &&
        sender.recorder().timeAdvanceGrants().size() == 1U;
  };
  while (std::chrono::steady_clock::now() < deadline && !complete()) {
    owner.pump();
    sender.pump();
    ownershipSubscriber.pump();
    universalSubscriber.pump();
    if (!complete()) {
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
  }
  require(
      complete(),
      "timestamped directed-interaction delivery or time-advance callbacks were incomplete");

  auto const assertDelivery = [&](
                              TimedDirectedInteractionRecord const& received,
                              rti::InteractionClassHandle const& expectedInteraction,
                              rti::ParameterHandle const& expectedParameter,
                              std::vector<std::string> const& callbackOrder,
                              std::string const& description) {
    require(
        received.interaction == expectedInteraction && received.object == target &&
            received.parameters.size() == 1U &&
            received.parameters.count(expectedParameter) == 1U &&
            copyBytes(received.parameters.at(expectedParameter)) == valueBytes &&
            received.tag == tagBytes && received.producer == sender.federateHandle() &&
            received.time == encodeTime(*timestamp) &&
            received.sentOrder == rti::TIMESTAMP &&
            received.receivedOrder == rti::TIMESTAMP && received.retractionPresent,
        description + " carried incorrect interaction, target, data, time, order, or producer");
    auto const directed = std::find(
        callbackOrder.begin(), callbackOrder.end(), "directed");
    auto const grant = std::find(callbackOrder.begin(), callbackOrder.end(), "grant");
    require(
        directed != callbackOrder.end() && grant != callbackOrder.end() && directed < grant,
        description + " was not delivered before the Time Advance Grant");
  };

  auto const ownerDeliveries = owner.recorder().timedDirectedInteractions();
  auto const universalDeliveries =
      universalSubscriber.recorder().timedDirectedInteractions();
  assertDelivery(
      ownerDeliveries.front(),
      ownerInteraction,
      ownerParameter,
      owner.recorder().callbackOrder(),
      "timestamped directed interaction to the target owner");
  assertDelivery(
      universalDeliveries.front(),
      universalInteraction,
      universalParameter,
      universalSubscriber.recorder().callbackOrder(),
      "timestamped directed interaction through a universal subscription");
  require(
      owner.recorder().timeAdvanceGrants().front().encoded == encodeTime(*ownerTarget) &&
          ownershipSubscriber.recorder().timeAdvanceGrants().front().encoded ==
              encodeTime(*ownershipTarget) &&
          universalSubscriber.recorder().timeAdvanceGrants().front().encoded ==
              encodeTime(*universalTarget) &&
          sender.recorder().timeAdvanceGrants().front().encoded == encodeTime(*timestamp),
      "timestamped directed-interaction federates received the wrong grant times");

  sender.resign(rti::NO_ACTION);
  ownershipSubscriber.resign(rti::NO_ACTION);
  universalSubscriber.resign(rti::NO_ACTION);
  owner.resign(rti::CANCEL_THEN_DELETE_THEN_DIVEST);
  owner.rtiAmbassador().destroyFederationExecution(federation);
  sender.disconnect();
  ownershipSubscriber.disconnect();
  universalSubscriber.disconnect();
  owner.disconnect();
}

}  // namespace hla_rti_cpp_tck::detail
