#include "tck_main_scenarios.hpp"

namespace hla_rti_cpp_tck::detail {

void scenarioTimeConstrainedEnableAtGaltEquality(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.logicalTimeImplementationName.empty(),
      "Time-constrained GALT equality requires an adapter-supplied logical-time implementation");

  Session regulator(options, model, "owner");
  Session requester(options, model, "member");
  auto const federation = federationName(options, "time-constrained-enable-galt-equality");
  connectAndJoin(regulator, requester, options, federation, options.fom);
  auto regulatorTime = makeTimeContext(regulator);
  auto requesterTime = makeTimeContext(requester);
  require(
      regulatorTime.factory->getName() == requesterTime.factory->getName(),
      "Time-constrained GALT equality members selected different logical-time factories");

  auto lookahead = regulatorTime.factory->makeZero();
  require(lookahead != nullptr, "Time-constrained GALT equality could not allocate lookahead");
  auto const one = timeAfter(
      *regulatorTime.factory,
      *regulatorTime.initial,
      *regulatorTime.epsilon,
      1U);
  lookahead->setToDifference(*one, *regulatorTime.initial);
  regulator.rtiAmbassador().enableTimeRegulation(*lookahead);
  waitFor(
      regulator,
      [&] { return regulator.recorder().timeRegulationEnabled().size() == 1U; },
      options,
      "Time-constrained GALT equality regulation callback");

  auto const five = timeAfter(
      *requesterTime.factory,
      *requesterTime.initial,
      *requesterTime.epsilon,
      5U);
  requester.rtiAmbassador().timeAdvanceRequest(*five);
  waitFor(
      requester,
      [&] { return requester.recorder().timeAdvanceGrants().size() == 1U; },
      options,
      "Time-constrained GALT equality unconstrained grant to five");
  auto observedGalt = requesterTime.factory->makeInitial();
  require(
      observedGalt != nullptr && requester.rtiAmbassador().queryGALT(*observedGalt),
      "Time-constrained GALT equality was undefined before constrained enablement");
  requireTimeEquals(
      *observedGalt,
      *one,
      "Time-constrained GALT equality pre-enable boundary");
  auto const regulatorTargetFour = timeAfter(
      *regulatorTime.factory,
      *regulatorTime.initial,
      *regulatorTime.epsilon,
      4U);
  regulator.rtiAmbassador().timeAdvanceRequest(*regulatorTargetFour);
  waitFor(
      regulator,
      [&] { return regulator.recorder().timeAdvanceGrants().size() == 1U; },
      options,
      "Time-constrained GALT equality regulator grant to four");
  observedGalt = requesterTime.factory->makeInitial();
  require(
      observedGalt != nullptr && requester.rtiAmbassador().queryGALT(*observedGalt),
      "Time-constrained GALT equality was undefined at the current-time boundary");
  requireTimeEquals(
      *observedGalt,
      *five,
      "Time-constrained GALT equality boundary");
  requester.rtiAmbassador().enableTimeConstrained();
  requester.pump();
  require(
      requester.recorder().timeConstrainedEnabled().empty(),
      "Time Constrained Enabled fired while GALT equaled the current logical time");

  auto const regulatorTargetFive = timeAfter(
      *regulatorTime.factory,
      *regulatorTime.initial,
      *regulatorTime.epsilon,
      5U);
  regulator.rtiAmbassador().timeAdvanceRequest(*regulatorTargetFive);
  waitFor(
      regulator,
      [&] { return regulator.recorder().timeAdvanceGrants().size() == 2U; },
      options,
      "Time-constrained GALT equality subsequent regulator grant");
  auto const six = timeAfter(
      *requesterTime.factory,
      *requesterTime.initial,
      *requesterTime.epsilon,
      6U);
  observedGalt = requesterTime.factory->makeInitial();
  require(
      observedGalt != nullptr && requester.rtiAmbassador().queryGALT(*observedGalt),
      "Time-constrained GALT equality was undefined after the regulator advanced");
  requireTimeEquals(
      *observedGalt,
      *six,
      "Time-constrained GALT greater-than-current boundary");
  if (model == rti::CallbackModel::HLA_EVOKED) {
    require(
        requester.recorder().timeConstrainedEnabled().empty(),
        "Time-constrained GALT release escaped evoked servicing");
  }
  waitFor(
      requester,
      [&] { return requester.recorder().timeConstrainedEnabled().size() == 1U; },
      options,
      "Time-constrained GALT greater-than-current callback");
  require(
      requester.recorder().timeConstrainedEnabled().front().text == five->toString(),
      "Time-constrained GALT release returned the wrong logical time");
  auto enabledTime = requesterTime.factory->makeInitial();
  require(
      enabledTime != nullptr,
      "Time-constrained GALT release could not allocate a logical-time query");
  requester.rtiAmbassador().queryLogicalTime(*enabledTime);
  requireTimeEquals(
      *enabledTime,
      *five,
      "Time-constrained enablement did not set the callback's logical time");

  auto const eight = timeAfter(
      *requesterTime.factory,
      *requesterTime.initial,
      *requesterTime.epsilon,
      8U);
  auto const regulatorEight = timeAfter(
      *regulatorTime.factory,
      *regulatorTime.initial,
      *regulatorTime.epsilon,
      8U);
  observedGalt = requesterTime.factory->makeInitial();
  require(
      observedGalt != nullptr && requester.rtiAmbassador().queryGALT(*observedGalt),
      "Regulator-constrained advance had no GALT before the request");
  requireTimeEquals(*observedGalt, *six, "Regulator-constrained pre-request GALT");
  requester.rtiAmbassador().timeAdvanceRequest(*eight);
  auto const heldDeadline = Clock::now() + std::chrono::milliseconds(50);
  while (Clock::now() < heldDeadline) {
    regulator.pump();
    requester.pump();
    require(
        requester.recorder().timeAdvanceGrants().size() == 1U,
        "Time-constrained requester advanced beyond the regulating peer's GALT");
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
  auto heldTime = requesterTime.factory->makeInitial();
  requester.rtiAmbassador().queryLogicalTime(*heldTime);
  requireTimeEquals(*heldTime, *five, "Regulator-constrained held logical time");
  observedGalt = requesterTime.factory->makeInitial();
  require(
      observedGalt != nullptr && requester.rtiAmbassador().queryGALT(*observedGalt),
      "Regulator-constrained advance lost GALT while TAR was pending");
  requireTimeEquals(
      *observedGalt,
      *six,
      "Regulator-constrained GALT while TAR was pending");

  regulator.rtiAmbassador().timeAdvanceRequest(*regulatorEight);
  waitFor(
      regulator,
      requester,
      [&] {
        return regulator.recorder().timeAdvanceGrants().size() == 3U &&
            requester.recorder().timeAdvanceGrants().size() == 2U;
      },
      options,
      "Regulator-constrained requester grant after peer advance");
  require(
      requester.recorder().timeAdvanceGrants().back().encoded == encodeTime(*eight),
      "Time-constrained requester granted at the wrong time after regulator advance");
  auto releasedTime = requesterTime.factory->makeInitial();
  requester.rtiAmbassador().queryLogicalTime(*releasedTime);
  requireTimeEquals(*releasedTime, *eight, "Regulator-constrained released logical time");

  requester.rtiAmbassador().disableTimeConstrained();
  regulator.rtiAmbassador().disableTimeRegulation();
  requester.resign(rti::NO_ACTION);
  regulator.resign(rti::NO_ACTION);
  regulator.rtiAmbassador().destroyFederationExecution(federation);
  requester.disconnect();
  regulator.disconnect();
}

void scenarioTimeConstrainedEnableAtGaltEqualityContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioTimeConstrainedEnableAtGaltEquality(options, model);
}

void scenarioTimeConstrainedEnableRegulatingFederateCurrentTime(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.logicalTimeImplementationName.empty(),
      "Regulating-federate Time Constrained enable requires an adapter-supplied logical-time implementation");

  Session regulator(options, model, "owner");
  Session requester(options, model, "member");
  auto const federation = federationName(
      options,
      "time-constrained-enable-regulating-federate-current-time");
  connectAndJoin(regulator, requester, options, federation, options.fom);
  auto regulatorTime = makeTimeContext(regulator);
  auto requesterTime = makeTimeContext(requester);
  require(
      regulatorTime.factory->getName() == requesterTime.factory->getName(),
      "Regulating-federate Time Constrained members selected different logical-time factories");

  regulator.rtiAmbassador().enableTimeRegulation(*regulatorTime.epsilon);
  waitFor(
      regulator,
      [&] { return regulator.recorder().timeRegulationEnabled().size() == 1U; },
      options,
      "Regulating-federate Time Constrained regulator callback");
  auto const five = timeAfter(
      *regulatorTime.factory,
      *regulatorTime.initial,
      *regulatorTime.epsilon,
      5U);
  regulator.rtiAmbassador().timeAdvanceRequest(*five);
  waitFor(
      regulator,
      [&] { return regulator.recorder().timeAdvanceGrants().size() == 1U; },
      options,
      "Regulating-federate Time Constrained regulator grant");

  requester.rtiAmbassador().timeAdvanceRequest(*five);
  waitFor(
      requester,
      [&] { return requester.recorder().timeAdvanceGrants().size() == 1U; },
      options,
      "Regulating-federate Time Constrained requester grant");
  requester.rtiAmbassador().enableTimeRegulation(*requesterTime.epsilon);
  waitFor(
      requester,
      [&] { return requester.recorder().timeRegulationEnabled().size() == 1U; },
      options,
      "Regulating-federate Time Constrained requester regulation callback");

  auto currentBeforeEnable = requesterTime.factory->makeInitial();
  require(
      currentBeforeEnable != nullptr,
      "Regulating-federate Time Constrained could not allocate a current-time query");
  requester.rtiAmbassador().queryLogicalTime(*currentBeforeEnable);
  requester.rtiAmbassador().enableTimeConstrained();
  waitFor(
      requester,
      [&] { return requester.recorder().timeConstrainedEnabled().size() == 1U; },
      options,
      "Regulating-federate Time Constrained callback");

  auto currentAfterEnable = requesterTime.factory->makeInitial();
  require(
      currentAfterEnable != nullptr,
      "Regulating-federate Time Constrained could not allocate a post-enable time query");
  requester.rtiAmbassador().queryLogicalTime(*currentAfterEnable);
  requireTimeEquals(
      *currentAfterEnable,
      *currentBeforeEnable,
      "Regulating-federate Time Constrained enable changed current logical time");
  require(
      requester.recorder().timeConstrainedEnabled().front().text ==
          currentAfterEnable->toString(),
      "Time Constrained Enabled did not report the current logical time for a regulating federate");

  requester.rtiAmbassador().disableTimeConstrained();
  requester.rtiAmbassador().disableTimeRegulation();
  regulator.rtiAmbassador().disableTimeRegulation();
  requester.resign(rti::NO_ACTION);
  regulator.resign(rti::NO_ACTION);
  regulator.rtiAmbassador().destroyFederationExecution(federation);
  requester.disconnect();
  regulator.disconnect();
}

void scenarioTimeConstrainedEnableRegulatingFederateCurrentTimeContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioTimeConstrainedEnableRegulatingFederateCurrentTime(options, model);
}

void scenarioTimeConstrainedEnableRetainsQueuedTso(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.logicalTimeImplementationName.empty(),
      "Time-constrained queued-TSO transition requires an adapter-supplied logical-time implementation");

  Session regulator(options, model, "owner");
  Session receiver(options, model, "member");
  auto const federation = federationName(
      options,
      "time-constrained-enable-retains-queued-tso");
  connectAndJoin(regulator, receiver, options, federation, options.fom);

  auto const regulatorInteraction =
      regulator.rtiAmbassador().getInteractionClassHandle(options.interactionClassName);
  auto const receiverInteraction =
      receiver.rtiAmbassador().getInteractionClassHandle(options.interactionClassName);
  auto const regulatorParameter = regulator.rtiAmbassador().getParameterHandle(
      regulatorInteraction,
      options.parameterName);
  require(
      regulatorInteraction.isValid() && receiverInteraction.isValid() &&
          regulatorParameter.isValid(),
      "Time-constrained queued-TSO transition lookup returned an invalid interaction or parameter handle");
  regulator.rtiAmbassador().publishInteractionClass(regulatorInteraction);
  regulator.rtiAmbassador().changeInteractionOrderType(
      regulatorInteraction,
      rti::TIMESTAMP);
  receiver.rtiAmbassador().subscribeInteractionClass(receiverInteraction, true);

  auto regulatorTime = makeTimeContext(regulator);
  auto receiverTime = makeTimeContext(receiver);
  require(
      regulatorTime.factory->getName() == receiverTime.factory->getName(),
      "Time-constrained queued-TSO members selected different logical-time factories");
  auto const one = timeAfter(
      *regulatorTime.factory,
      *regulatorTime.initial,
      *regulatorTime.epsilon,
      1U);
  auto lookahead = regulatorTime.factory->makeZero();
  require(
      lookahead != nullptr,
      "Time-constrained queued-TSO transition could not allocate a lookahead interval");
  lookahead->setToDifference(*one, *regulatorTime.initial);
  regulator.rtiAmbassador().enableTimeRegulation(*lookahead);
  waitFor(
      regulator,
      [&] { return regulator.recorder().timeRegulationEnabled().size() == 1U; },
      options,
      "Time-constrained queued-TSO regulator callback");

  auto const five = timeAfter(
      *receiverTime.factory,
      *receiverTime.initial,
      *receiverTime.epsilon,
      5U);
  receiver.rtiAmbassador().timeAdvanceRequest(*five);
  waitFor(
      receiver,
      [&] { return receiver.recorder().timeAdvanceGrants().size() == 1U; },
      options,
      "Time-constrained queued-TSO unconstrained grant to five");
  require(
      receiver.recorder().timeAdvanceGrants().front().text == five->toString(),
      "Time-constrained queued-TSO receiver did not reach logical time five");

  receiver.rtiAmbassador().enableTimeConstrained();
  receiver.pump();
  require(
      receiver.recorder().timeConstrainedEnabled().empty(),
      "Time Constrained Enabled fired before the regulator advanced beyond receiver time five");

  auto const six = timeAfter(
      *regulatorTime.factory,
      *regulatorTime.initial,
      *regulatorTime.epsilon,
      6U);
  std::vector<std::uint8_t> const parameterBytes{0x54U, 0x53U, 0x4FU};
  std::vector<std::uint8_t> const tagBytes{0x54U, 0x43U, 0x45U, 0x2DU, 0x54U, 0x53U, 0x4FU};
  rti::ParameterHandleValueMap parameters;
  parameters.emplace(
      regulatorParameter,
      rti::VariableLengthData(parameterBytes.data(), parameterBytes.size()));
  rti::VariableLengthData tag(tagBytes.data(), tagBytes.size());
  auto const retraction = regulator.rtiAmbassador().sendInteraction(
      regulatorInteraction,
      parameters,
      tag,
      *six);
  require(
      retraction.isValid(),
      "Time-constrained queued-TSO send returned an invalid retraction handle");
  require(
      receiver.recorder().interactions().empty() &&
          receiver.recorder().timedInteractions().empty(),
      "A timestamped interaction escaped while Time Constrained enablement was pending");

  auto const four = timeAfter(
      *regulatorTime.factory,
      *regulatorTime.initial,
      *regulatorTime.epsilon,
      4U);
  regulator.rtiAmbassador().timeAdvanceRequest(*four);
  waitFor(
      regulator,
      [&] { return regulator.recorder().timeAdvanceGrants().size() == 1U; },
      options,
      "Time-constrained queued-TSO regulator grant to four");
  receiver.pump();
  require(
      receiver.recorder().timeConstrainedEnabled().empty() &&
          receiver.recorder().interactions().empty() &&
          receiver.recorder().timedInteractions().empty(),
      "The role transition or queued TSO completed while GALT remained at receiver time five");

  auto const regulatorFive = timeAfter(
      *regulatorTime.factory,
      *regulatorTime.initial,
      *regulatorTime.epsilon,
      5U);
  regulator.rtiAmbassador().timeAdvanceRequest(*regulatorFive);
  waitFor(
      regulator,
      [&] { return regulator.recorder().timeAdvanceGrants().size() == 2U; },
      options,
      "Time-constrained queued-TSO regulator grant to five");
  waitFor(
      receiver,
      [&] { return receiver.recorder().timeConstrainedEnabled().size() == 1U; },
      options,
      "Time-constrained queued-TSO role-transition callback");
  require(
      receiver.recorder().interactions().empty() &&
          receiver.recorder().timedInteractions().empty(),
      "The queued TSO was delivered before the constrained receiver advanced to its timestamp");
  auto const callbackOrderBeforeTsoAdvance = receiver.recorder().callbackOrder();

  receiver.rtiAmbassador().timeAdvanceRequest(*six);
  waitFor(
      receiver,
      [&] {
        return receiver.recorder().timedInteractions().size() == 1U &&
            receiver.recorder().timeAdvanceGrants().size() == 2U;
      },
      options,
      "Time-constrained queued-TSO delivery before grant six");
  auto const timedInteractions = receiver.recorder().timedInteractions();
  auto const grants = receiver.recorder().timeAdvanceGrants();
  auto const callbackOrder = receiver.recorder().callbackOrder();
  require(
      receiver.recorder().interactions().empty() && timedInteractions.size() == 1U &&
          timedInteractions.front().interaction == receiverInteraction &&
          timedInteractions.front().timeText == six->toString() &&
          timedInteractions.front().sentOrder == rti::TIMESTAMP &&
          timedInteractions.front().receivedOrder == rti::TIMESTAMP,
      "The queued interaction did not remain timestamp-order across Time Constrained enablement");
  require(
      grants.size() == 2U && grants.back().text == six->toString(),
      "Time-constrained queued-TSO receiver did not receive its grant at time six");
  require(
      callbackOrder.size() == callbackOrderBeforeTsoAdvance.size() + 2U &&
          callbackOrder[callbackOrderBeforeTsoAdvance.size()] == "interaction" &&
          callbackOrder.back() == "grant",
      "The queued timestamped interaction did not precede its time-advance grant");

  receiver.rtiAmbassador().disableTimeConstrained();
  regulator.rtiAmbassador().disableTimeRegulation();
  receiver.resign(rti::NO_ACTION);
  regulator.resign(rti::NO_ACTION);
  regulator.rtiAmbassador().destroyFederationExecution(federation);
  receiver.disconnect();
  regulator.disconnect();
}

void scenarioTimeConstrainedEnableRetainsQueuedTsoContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioTimeConstrainedEnableRetainsQueuedTso(options, model);
}

void scenarioTimeConstrainedEnableRetainsQueuedTsoAttribute(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.logicalTimeImplementationName.empty(),
      "Time-constrained queued-attribute TSO transition requires an adapter-supplied logical-time implementation");

  Session publisher(options, model, "queued-attribute-tso-publisher");
  Session receiver(options, model, "queued-attribute-tso-receiver");
  auto const federation = federationName(
      options,
      "time-constrained-enable-retains-queued-tso-attribute");
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
  require(
      publisherClass.isValid() && receiverClass.isValid() &&
          publisherAttribute.isValid() && receiverAttribute.isValid(),
      "Time-constrained queued-attribute TSO transition resolved an invalid handle");

  rti::AttributeHandleSet const publisherAttributes{publisherAttribute};
  rti::AttributeHandleSet const receiverAttributes{receiverAttribute};
  publisher.rtiAmbassador().publishObjectClassAttributes(
      publisherClass,
      publisherAttributes);
  receiver.rtiAmbassador().subscribeObjectClassAttributes(
      receiverClass,
      receiverAttributes,
      true,
      L"");
  publisher.rtiAmbassador().changeDefaultAttributeOrderType(
      publisherClass,
      publisherAttributes,
      rti::TIMESTAMP);
  auto const object = publisher.rtiAmbassador().registerObjectInstance(
      publisherClass);
  require(
      object.isValid(),
      "Time-constrained queued-attribute TSO registration returned an invalid object handle");
  waitFor(
      receiver,
      [&] { return receiver.recorder().hasDiscovery(object); },
      options,
      "Time-constrained queued-attribute TSO object discovery");

  auto publisherTime = makeTimeContext(publisher);
  auto receiverTime = makeTimeContext(receiver);
  require(
      publisherTime.factory->getName() == receiverTime.factory->getName(),
      "Time-constrained queued-attribute TSO members selected different logical-time factories");
  auto const one = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      1U);
  auto lookahead = publisherTime.factory->makeZero();
  require(
      lookahead != nullptr,
      "Time-constrained queued-attribute TSO transition could not allocate lookahead");
  lookahead->setToDifference(*one, *publisherTime.initial);
  publisher.rtiAmbassador().enableTimeRegulation(*lookahead);
  waitFor(
      publisher,
      [&] { return publisher.recorder().timeRegulationEnabled().size() == 1U; },
      options,
      "Time-constrained queued-attribute TSO regulator callback");

  auto const five = timeAfter(
      *receiverTime.factory,
      *receiverTime.initial,
      *receiverTime.epsilon,
      5U);
  receiver.rtiAmbassador().timeAdvanceRequest(*five);
  waitFor(
      receiver,
      [&] { return receiver.recorder().timeAdvanceGrants().size() == 1U; },
      options,
      "Time-constrained queued-attribute TSO unconstrained grant to five");
  require(
      receiver.recorder().timeAdvanceGrants().front().text == five->toString(),
      "Time-constrained queued-attribute TSO receiver did not reach logical time five");

  receiver.rtiAmbassador().enableTimeConstrained();
  receiver.pump();
  require(
      receiver.recorder().timeConstrainedEnabled().empty(),
      "Time Constrained Enabled fired before the regulator advanced beyond receiver time five");

  auto const six = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      6U);
  std::vector<std::uint8_t> const valueBytes{0x54U, 0x53U, 0x4FU};
  std::vector<std::uint8_t> const tagBytes{0x54U, 0x43U, 0x45U, 0x2DU, 0x41U};
  rti::AttributeHandleValueMap values;
  values.emplace(
      publisherAttribute,
      rti::VariableLengthData(valueBytes.data(), valueBytes.size()));
  rti::VariableLengthData const tag(tagBytes.data(), tagBytes.size());
  auto const retraction = publisher.rtiAmbassador().updateAttributeValues(
      object,
      values,
      tag,
      *six);
  require(
      retraction.isValid(),
      "Time-constrained queued-attribute TSO update returned an invalid retraction handle");
  require(
      receiver.recorder().reflections().empty() &&
          receiver.recorder().timedReflections().empty(),
      "Timestamped attribute update escaped while Time Constrained enablement was pending");

  auto const four = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      4U);
  publisher.rtiAmbassador().timeAdvanceRequest(*four);
  waitFor(
      publisher,
      [&] { return publisher.recorder().timeAdvanceGrants().size() == 1U; },
      options,
      "Time-constrained queued-attribute TSO regulator grant to four");
  receiver.pump();
  require(
      receiver.recorder().timeConstrainedEnabled().empty() &&
          receiver.recorder().reflections().empty() &&
          receiver.recorder().timedReflections().empty(),
      "The role transition or queued attribute TSO completed while GALT remained at receiver time five");

  auto const regulatorFive = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      5U);
  publisher.rtiAmbassador().timeAdvanceRequest(*regulatorFive);
  waitFor(
      publisher,
      [&] { return publisher.recorder().timeAdvanceGrants().size() == 2U; },
      options,
      "Time-constrained queued-attribute TSO regulator grant to five");
  waitFor(
      receiver,
      [&] { return receiver.recorder().timeConstrainedEnabled().size() == 1U; },
      options,
      "Time-constrained queued-attribute TSO role-transition callback");
  require(
      receiver.recorder().reflections().empty() &&
          receiver.recorder().timedReflections().empty(),
      "The queued timestamped attribute update was delivered before receiver advancement");
  auto const callbackOrderBeforeTsoAdvance = receiver.recorder().callbackOrder();

  receiver.rtiAmbassador().timeAdvanceRequest(*six);
  waitFor(
      receiver,
      [&] {
        return receiver.recorder().timedReflections().size() == 1U &&
            receiver.recorder().timeAdvanceGrants().size() == 2U;
      },
      options,
      "Time-constrained queued-attribute TSO reflection before grant six");
  auto const reflections = receiver.recorder().timedReflections();
  auto const grants = receiver.recorder().timeAdvanceGrants();
  auto const callbackOrder = receiver.recorder().callbackOrder();
  require(
      receiver.recorder().reflections().empty() && reflections.size() == 1U &&
          reflections.front().object == object &&
          reflections.front().values.size() == 1U &&
          reflections.front().values.count(receiverAttribute) == 1U &&
          copyBytes(reflections.front().values.at(receiverAttribute)) == valueBytes &&
          reflections.front().tag == tagBytes &&
          reflections.front().timeText == six->toString() &&
          reflections.front().sentOrder == rti::TIMESTAMP &&
          reflections.front().receivedOrder == rti::TIMESTAMP &&
          reflections.front().retractionPresent &&
          reflections.front().retraction == copyBytes(retraction.encode()),
      "The queued attribute update did not remain timestamp-order across Time Constrained enablement");
  require(
      grants.size() == 2U && grants.back().text == six->toString(),
      "Time-constrained queued-attribute TSO receiver did not receive its grant at time six");
  require(
      callbackOrder.size() == callbackOrderBeforeTsoAdvance.size() + 2U &&
          callbackOrder[callbackOrderBeforeTsoAdvance.size()] == "reflect" &&
          callbackOrder.back() == "grant",
      "The queued timestamped attribute reflection did not precede its time-advance grant");

  receiver.rtiAmbassador().disableTimeConstrained();
  publisher.rtiAmbassador().disableTimeRegulation();
  receiver.resign(rti::NO_ACTION);
  publisher.resign(rti::CANCEL_THEN_DELETE_THEN_DIVEST);
  publisher.rtiAmbassador().destroyFederationExecution(federation);
  receiver.disconnect();
  publisher.disconnect();
}

void scenarioTimeConstrainedEnableRetainsQueuedTsoAttributeContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioTimeConstrainedEnableRetainsQueuedTsoAttribute(options, model);
}

void scenarioTimeConstrainedEnableReceiveOrderBoundary(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.logicalTimeImplementationName.empty(),
      "Time-constrained receive-order boundary requires an adapter-supplied logical-time implementation");

  Session regulator(options, model, "owner");
  Session receiver(options, model, "member");
  auto const federation = federationName(
      options,
      "time-constrained-enable-receive-order-boundary");
  connectAndJoin(regulator, receiver, options, federation, options.fom);

  auto const regulatorInteraction =
      regulator.rtiAmbassador().getInteractionClassHandle(options.interactionClassName);
  auto const receiverInteraction =
      receiver.rtiAmbassador().getInteractionClassHandle(options.interactionClassName);
  auto const regulatorParameter = regulator.rtiAmbassador().getParameterHandle(
      regulatorInteraction,
      options.parameterName);
  require(
      regulatorInteraction.isValid() && receiverInteraction.isValid() &&
          regulatorParameter.isValid(),
      "Time-constrained receive-order boundary lookup returned an invalid interaction or parameter handle");
  regulator.rtiAmbassador().publishInteractionClass(regulatorInteraction);
  regulator.rtiAmbassador().changeInteractionOrderType(
      regulatorInteraction,
      rti::TIMESTAMP);
  receiver.rtiAmbassador().subscribeInteractionClass(receiverInteraction, true);

  auto regulatorTime = makeTimeContext(regulator);
  auto receiverTime = makeTimeContext(receiver);
  require(
      regulatorTime.factory->getName() == receiverTime.factory->getName(),
      "Time-constrained receive-order members selected different logical-time factories");
  auto const one = timeAfter(
      *regulatorTime.factory,
      *regulatorTime.initial,
      *regulatorTime.epsilon,
      1U);
  auto lookahead = regulatorTime.factory->makeZero();
  require(
      lookahead != nullptr,
      "Time-constrained receive-order boundary could not allocate a lookahead interval");
  lookahead->setToDifference(*one, *regulatorTime.initial);
  regulator.rtiAmbassador().enableTimeRegulation(*lookahead);
  waitFor(
      regulator,
      [&] { return regulator.recorder().timeRegulationEnabled().size() == 1U; },
      options,
      "Time-constrained receive-order regulator callback");

  auto const five = timeAfter(
      *regulatorTime.factory,
      *regulatorTime.initial,
      *regulatorTime.epsilon,
      5U);
  std::vector<std::uint8_t> const parameterBytes{0x54U, 0x43U, 0x45U};
  std::vector<std::uint8_t> const futureTagBytes{0x52U, 0x4FU, 0x2DU, 0x46U};
  std::vector<std::uint8_t> const equalTimeTagBytes{0x52U, 0x4FU, 0x2DU, 0x45U};
  rti::ParameterHandleValueMap parameters;
  parameters.emplace(
      regulatorParameter,
      rti::VariableLengthData(parameterBytes.data(), parameterBytes.size()));
  rti::VariableLengthData futureTag(
      futureTagBytes.data(), futureTagBytes.size());
  auto const beforeTransition = regulator.rtiAmbassador().sendInteraction(
      regulatorInteraction,
      parameters,
      futureTag,
      *five);
  require(
      beforeTransition.isValid(),
      "Pre-transition timestamped interaction returned an invalid retraction handle");

  waitFor(
      receiver,
      [&] { return receiver.recorder().timedInteractions().size() == 1U; },
      options,
      "Pre-transition future timestamped interaction as receive order");
  auto const futureInteraction = receiver.recorder().timedInteractions().front();
  require(
      futureInteraction.timeText == five->toString() &&
          futureInteraction.sentOrder == rti::TIMESTAMP &&
          futureInteraction.receivedOrder == rti::RECEIVE &&
          futureInteraction.tag == futureTagBytes,
      "The future pre-transition interaction did not retain receive-order delivery and its identity");

  // An unconstrained federate may reach the timestamp before it enables Time
  // Constrained. Exercise the exact current-time boundary separately from the
  // earlier future event so this lane does not hide equality in a broad range.
  receiver.rtiAmbassador().timeAdvanceRequest(*five);
  waitFor(
      receiver,
      [&] { return receiver.recorder().timeAdvanceGrants().size() == 1U; },
      options,
      "Unconstrained receiver grant to the Time Constrained transition timestamp");
  require(
      receiver.recorder().timeAdvanceGrants().front().text == five->toString(),
      "The unconstrained receiver did not reach the exact pre-transition TSO timestamp");

  rti::VariableLengthData equalTimeTag(
      equalTimeTagBytes.data(), equalTimeTagBytes.size());
  auto const atCurrentTime = regulator.rtiAmbassador().sendInteraction(
      regulatorInteraction,
      parameters,
      equalTimeTag,
      *five);
  require(
      atCurrentTime.isValid(),
      "Equal-time pre-transition interaction returned an invalid retraction handle");
  receiver.rtiAmbassador().enableTimeConstrained();
  receiver.pump();
  waitFor(
      receiver,
      [&] { return receiver.recorder().timedInteractions().size() == 2U; },
      options,
      "Equal-time pre-transition interaction as receive order");
  require(
      receiver.recorder().timeConstrainedEnabled().empty(),
      "Time Constrained Enabled preceded the regulator's safe frontier");
  auto const equalTimeInteraction = receiver.recorder().timedInteractions().back();
  require(
      equalTimeInteraction.timeText == five->toString() &&
          equalTimeInteraction.sentOrder == rti::TIMESTAMP &&
          equalTimeInteraction.receivedOrder == rti::RECEIVE &&
          equalTimeInteraction.tag == equalTimeTagBytes,
      "The equal-time pre-transition interaction did not retain receive-order delivery and its identity");

  regulator.rtiAmbassador().timeAdvanceRequest(*five);
  waitFor(
      regulator,
      [&] { return regulator.recorder().timeAdvanceGrants().size() == 1U; },
      options,
      "Time-constrained receive-order regulator grant to five");
  waitFor(
      receiver,
      [&] { return receiver.recorder().timeConstrainedEnabled().size() == 1U; },
      options,
      "Time-constrained receive-order boundary transition callback");
  require(
      receiver.recorder().timeConstrainedEnabled().front().text == five->toString(),
      "Time Constrained Enabled did not report the receiver's current logical time");
  auto const preTransitionCallbacks = receiver.recorder().timedInteractions();
  require(
      preTransitionCallbacks.size() == 2U &&
          preTransitionCallbacks.front().interaction == receiverInteraction &&
          preTransitionCallbacks.back().interaction == receiverInteraction,
      "The pre-transition interactions did not retain their receiver class identity");
  require(
      receiver.recorder().timeConstrainedEnablementOrder() ==
          std::vector<std::string>{
              "interaction", "interaction", "time-constrained-enabled"},
      "The pre-transition receive-order callbacks did not precede Time Constrained Enabled");

  auto const seven = timeAfter(
      *regulatorTime.factory,
      *regulatorTime.initial,
      *regulatorTime.epsilon,
      7U);
  auto const eight = timeAfter(
      *regulatorTime.factory,
      *regulatorTime.initial,
      *regulatorTime.epsilon,
      8U);
  std::vector<std::uint8_t> const postTransitionTagBytes{
      0x52U, 0x4FU, 0x2DU, 0x54U};
  rti::VariableLengthData postTransitionTag(
      postTransitionTagBytes.data(), postTransitionTagBytes.size());
  auto const afterTransition = regulator.rtiAmbassador().sendInteraction(
      regulatorInteraction,
      parameters,
      postTransitionTag,
      *seven);
  require(
      afterTransition.isValid(),
      "Post-transition timestamped interaction returned an invalid retraction handle");
  require(
      receiver.recorder().timedInteractions().size() == 2U,
      "The post-transition TSO escaped before the constrained receiver advanced");

  auto const callbackOrderBeforeAdvance = receiver.recorder().callbackOrder();
  receiver.rtiAmbassador().timeAdvanceRequest(*eight);
  require(
      receiver.recorder().timeAdvanceGrants().size() == 1U,
      "The constrained receiver granted TAR(8) before the regulator advanced its frontier");
  auto const six = timeAfter(
      *regulatorTime.factory,
      *regulatorTime.initial,
      *regulatorTime.epsilon,
      6U);
  regulator.rtiAmbassador().timeAdvanceRequest(*six);
  waitFor(
      regulator,
      [&] { return regulator.recorder().timeAdvanceGrants().size() == 2U; },
      options,
      "Time-constrained receive-order regulator grant to six");
  require(
      receiver.recorder().timeAdvanceGrants().size() == 1U,
      "The receiver granted TAR(8) when the regulator frontier was only seven");
  regulator.rtiAmbassador().timeAdvanceRequest(*eight);
  waitFor(
      regulator,
      [&] { return regulator.recorder().timeAdvanceGrants().size() == 3U; },
      options,
      "Time-constrained receive-order regulator grant to eight");
  waitFor(
      receiver,
      [&] {
        return receiver.recorder().timedInteractions().size() == 3U &&
            receiver.recorder().timeAdvanceGrants().size() == 2U;
      },
      options,
      "Post-transition TSO before TAR(8) grant");

  auto const allInteractions = receiver.recorder().timedInteractions();
  auto const callbackOrder = receiver.recorder().callbackOrder();
  require(
      allInteractions.size() == 3U &&
          allInteractions.back().interaction == receiverInteraction &&
          allInteractions.back().timeText == seven->toString() &&
          allInteractions.back().sentOrder == rti::TIMESTAMP &&
          allInteractions.back().receivedOrder == rti::TIMESTAMP &&
          allInteractions.back().tag == postTransitionTagBytes,
      "The post-transition interaction did not retain timestamp-order delivery");
  require(
      callbackOrder.size() == callbackOrderBeforeAdvance.size() + 2U &&
          callbackOrder[callbackOrderBeforeAdvance.size()] == "interaction" &&
          callbackOrder.back() == "grant",
      "The post-transition timestamp-order callback did not precede TAR(8)'s grant");
  require(
      receiver.recorder().timeConstrainedEnablementOrder() ==
          std::vector<std::string>{
              "interaction", "interaction", "time-constrained-enabled", "interaction"},
      "The interaction callback sequence crossed the Time Constrained transition boundary");
  require(
      receiver.recorder().timeAdvanceGrants().back().text == eight->toString(),
      "The constrained receiver did not reach its requested post-transition time");

  receiver.rtiAmbassador().disableTimeConstrained();
  regulator.rtiAmbassador().disableTimeRegulation();
  receiver.resign(rti::NO_ACTION);
  regulator.resign(rti::NO_ACTION);
  regulator.rtiAmbassador().destroyFederationExecution(federation);
  receiver.disconnect();
  regulator.disconnect();
}

void scenarioTimeConstrainedEnableReceiveOrderBoundaryContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioTimeConstrainedEnableReceiveOrderBoundary(options, model);
}

void scenarioTimeConstrainedDisableQueuedInteractionToReceiveOrder(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.logicalTimeImplementationName.empty(),
      "Time Constrained disable requires an adapter-supplied logical-time implementation");

  Session publisher(options, model, "publisher");
  Session receiver(options, model, "subscriber");
  auto const federation = federationName(
      options,
      "time-constrained-disable-queued-interaction-to-receive-order");
  connectAndJoin(publisher, receiver, options, federation, options.fom);

  auto const publishedClass = publisher.rtiAmbassador().getInteractionClassHandle(
      options.interactionClassName);
  auto const subscribedClass = receiver.rtiAmbassador().getInteractionClassHandle(
      options.interactionClassName);
  auto const parameter = publisher.rtiAmbassador().getParameterHandle(
      publishedClass,
      options.parameterName);
  require(
      publishedClass.isValid() && subscribedClass.isValid() && parameter.isValid(),
      "Time Constrained disable resolved an invalid interaction or parameter handle");
  publisher.rtiAmbassador().publishInteractionClass(publishedClass);
  publisher.rtiAmbassador().changeInteractionOrderType(publishedClass, rti::TIMESTAMP);
  receiver.rtiAmbassador().subscribeInteractionClass(subscribedClass, true);

  auto publisherTime = makeTimeContext(publisher);
  auto receiverTime = makeTimeContext(receiver);
  require(
      publisherTime.factory->getName() == receiverTime.factory->getName(),
      "Time Constrained disable members selected different logical-time factories");
  publisher.rtiAmbassador().enableTimeRegulation(*publisherTime.epsilon);
  waitFor(
      publisher,
      [&] { return publisher.recorder().timeRegulationEnabled().size() == 1U; },
      options,
      "Time Constrained disable publisher regulation callback");
  receiver.rtiAmbassador().enableTimeConstrained();
  waitFor(
      receiver,
      [&] { return receiver.recorder().timeConstrainedEnabled().size() == 1U; },
      options,
      "Time Constrained disable receiver enablement callback");

  std::vector<std::uint8_t> const parameterBytes{0x54U, 0x44U};
  std::vector<std::uint8_t> const queuedTagBytes{0x51U};
  std::vector<std::uint8_t> const laterTagBytes{0x53U};
  rti::ParameterHandleValueMap parameters;
  parameters.emplace(
      parameter,
      rti::VariableLengthData(parameterBytes.data(), parameterBytes.size()));
  rti::VariableLengthData const queuedTag(
      queuedTagBytes.data(), queuedTagBytes.size());
  rti::VariableLengthData const laterTag(
      laterTagBytes.data(), laterTagBytes.size());
  auto const futureTime = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      5U);
  auto const queued = publisher.rtiAmbassador().sendInteraction(
      publishedClass,
      parameters,
      queuedTag,
      *futureTime);
  require(queued.isValid(), "Queued timestamped send returned an invalid retraction handle");
  receiver.pump();
  require(
      receiver.recorder().interactions().empty() &&
          receiver.recorder().timedInteractions().empty(),
      "Future timestamped interaction escaped before Time Constrained was disabled");

  receiver.rtiAmbassador().disableTimeConstrained();
  waitFor(
      receiver,
      [&] { return receiver.recorder().interactions().size() == 1U; },
      options,
      "queued timestamped interaction delivered in receive order after disable");
  auto const queuedDelivery = receiver.recorder().interactions().front();
  require(
      receiver.recorder().timedInteractions().empty() &&
          receiver.recorder().timeAdvanceGrants().empty() &&
          queuedDelivery.interaction == subscribedClass &&
          queuedDelivery.parameters.size() == 1U &&
          queuedDelivery.parameters.count(parameter) == 1U &&
          copyBytes(queuedDelivery.parameters.at(parameter)) == parameterBytes &&
          queuedDelivery.tag == queuedTagBytes &&
          queuedDelivery.producer == publisher.federateHandle(),
      "Queued interaction did not retain its identity and payload in receive order");

  auto const laterTime = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      3U);
  auto const later = publisher.rtiAmbassador().sendInteraction(
      publishedClass,
      parameters,
      laterTag,
      *laterTime);
  require(later.isValid(), "Subsequent timestamped send returned an invalid retraction handle");
  waitFor(
      receiver,
      [&] { return receiver.recorder().timedInteractions().size() == 1U; },
      options,
      "subsequent timestamped interaction received after disable");
  auto const subsequentDelivery = receiver.recorder().timedInteractions().front();
  require(
      receiver.recorder().interactions().size() == 1U &&
          subsequentDelivery.interaction == subscribedClass &&
          subsequentDelivery.parameters.size() == 1U &&
          subsequentDelivery.parameters.count(parameter) == 1U &&
          copyBytes(subsequentDelivery.parameters.at(parameter)) == parameterBytes &&
          subsequentDelivery.tag == laterTagBytes &&
          subsequentDelivery.producer == publisher.federateHandle() &&
          subsequentDelivery.timeText == laterTime->toString() &&
          subsequentDelivery.sentOrder == rti::TIMESTAMP &&
          subsequentDelivery.receivedOrder == rti::RECEIVE,
      "Subsequent timestamped interaction did not retain receive-order delivery");

  publisher.rtiAmbassador().disableTimeRegulation();
  receiver.resign(rti::NO_ACTION);
  publisher.resign(rti::NO_ACTION);
  publisher.rtiAmbassador().destroyFederationExecution(federation);
  receiver.disconnect();
  publisher.disconnect();
}

void scenarioTimeConstrainedDisableQueuedInteractionToReceiveOrderContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioTimeConstrainedDisableQueuedInteractionToReceiveOrder(options, model);
}

void scenarioTimeConstrainedDisableQueuedAttributeToReceiveOrder(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.logicalTimeImplementationName.empty(),
      "Time Constrained attribute disable requires an adapter-supplied logical-time implementation");

  Session publisher(options, model, "attribute-publisher");
  Session receiver(options, model, "attribute-subscriber");
  auto const federation = federationName(
      options,
      "time-constrained-disable-queued-attribute-to-receive-order");
  connectAndJoin(publisher, receiver, options, federation, options.fom);

  auto const publishedClass = publisher.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const subscribedClass = receiver.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const publishedAttribute = publisher.rtiAmbassador().getAttributeHandle(
      publishedClass,
      options.attributeName);
  auto const subscribedAttribute = receiver.rtiAmbassador().getAttributeHandle(
      subscribedClass,
      options.attributeName);
  require(
      publishedClass.isValid() && subscribedClass.isValid() &&
          publishedAttribute.isValid() && subscribedAttribute.isValid(),
      "Time Constrained attribute disable resolved an invalid object or attribute handle");
  rti::AttributeHandleSet const publishedAttributes{publishedAttribute};
  rti::AttributeHandleSet const subscribedAttributes{subscribedAttribute};
  publisher.rtiAmbassador().publishObjectClassAttributes(
      publishedClass,
      publishedAttributes);
  receiver.rtiAmbassador().subscribeObjectClassAttributes(
      subscribedClass,
      subscribedAttributes,
      true,
      L"");
  publisher.rtiAmbassador().changeDefaultAttributeOrderType(
      publishedClass,
      publishedAttributes,
      rti::TIMESTAMP);
  auto const object = publisher.rtiAmbassador().registerObjectInstance(publishedClass);
  require(object.isValid(), "Time Constrained attribute disable registration returned an invalid object");
  waitFor(
      receiver,
      [&] { return receiver.recorder().hasDiscovery(object); },
      options,
      "Time Constrained attribute disable object discovery");

  auto publisherTime = makeTimeContext(publisher);
  auto receiverTime = makeTimeContext(receiver);
  require(
      publisherTime.factory->getName() == receiverTime.factory->getName(),
      "Time Constrained attribute disable members selected different logical-time factories");
  publisher.rtiAmbassador().enableTimeRegulation(*publisherTime.epsilon);
  waitFor(
      publisher,
      [&] { return publisher.recorder().timeRegulationEnabled().size() == 1U; },
      options,
      "Time Constrained attribute disable regulation callback");
  receiver.rtiAmbassador().enableTimeConstrained();
  waitFor(
      receiver,
      [&] { return receiver.recorder().timeConstrainedEnabled().size() == 1U; },
      options,
      "Time Constrained attribute disable enablement callback");

  std::vector<std::uint8_t> const valueBytes{0x63U, 0x64U};
  std::vector<std::uint8_t> const queuedTagBytes{0x65U};
  std::vector<std::uint8_t> const laterTagBytes{0x66U};
  rti::AttributeHandleValueMap values;
  values.emplace(
      publishedAttribute,
      rti::VariableLengthData(valueBytes.data(), valueBytes.size()));
  rti::VariableLengthData const queuedTag(
      queuedTagBytes.data(), queuedTagBytes.size());
  rti::VariableLengthData const laterTag(
      laterTagBytes.data(), laterTagBytes.size());
  auto const futureTime = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      5U);
  auto const queued = publisher.rtiAmbassador().updateAttributeValues(
      object,
      values,
      queuedTag,
      *futureTime);
  require(queued.isValid(), "Queued timestamped update returned an invalid retraction handle");
  receiver.pump();
  require(
      receiver.recorder().reflections().empty() &&
          receiver.recorder().timedReflections().empty() &&
          receiver.recorder().timeAdvanceGrants().empty(),
      "Future attribute reflection escaped before Time Constrained was disabled");

  receiver.rtiAmbassador().disableTimeConstrained();
  waitFor(
      receiver,
      [&] { return receiver.recorder().reflections().size() == 1U; },
      options,
      "queued timestamped attribute reflection in receive order after disable");
  auto const queuedReflection = receiver.recorder().reflections().front();
  require(
      receiver.recorder().timedReflections().empty() &&
          receiver.recorder().timeAdvanceGrants().empty() &&
          queuedReflection.object == object &&
          queuedReflection.values.size() == 1U &&
          queuedReflection.values.count(subscribedAttribute) == 1U &&
          copyBytes(queuedReflection.values.at(subscribedAttribute)) == valueBytes &&
          queuedReflection.tag == queuedTagBytes &&
          queuedReflection.producer == publisher.federateHandle(),
      "Queued attribute update did not retain its object, value, tag, and publisher in receive order");

  auto const laterTime = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      3U);
  auto const later = publisher.rtiAmbassador().updateAttributeValues(
      object,
      values,
      laterTag,
      *laterTime);
  require(later.isValid(), "Subsequent timestamped update returned an invalid retraction handle");
  waitFor(
      receiver,
      [&] { return receiver.recorder().timedReflections().size() == 1U; },
      options,
      "subsequent timestamped attribute reflection after disable");
  auto const subsequentReflection = receiver.recorder().timedReflections().front();
  require(
      receiver.recorder().reflections().size() == 1U &&
          subsequentReflection.object == object &&
          subsequentReflection.values.size() == 1U &&
          subsequentReflection.values.count(subscribedAttribute) == 1U &&
          copyBytes(subsequentReflection.values.at(subscribedAttribute)) == valueBytes &&
          subsequentReflection.tag == laterTagBytes &&
          subsequentReflection.producer == publisher.federateHandle() &&
          subsequentReflection.timeText == laterTime->toString() &&
          subsequentReflection.sentOrder == rti::TIMESTAMP &&
          subsequentReflection.receivedOrder == rti::RECEIVE,
      "Subsequent timestamped attribute update did not retain receive-order delivery");

  publisher.rtiAmbassador().disableTimeRegulation();
  receiver.resign(rti::NO_ACTION);
  publisher.resign(rti::CANCEL_THEN_DELETE_THEN_DIVEST);
  publisher.rtiAmbassador().destroyFederationExecution(federation);
  receiver.disconnect();
  publisher.disconnect();
}

void scenarioTimeConstrainedDisableQueuedAttributeToReceiveOrderContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioTimeConstrainedDisableQueuedAttributeToReceiveOrder(options, model);
}

void scenarioTimeConstrainedDisableQueuedDeletionToReceiveOrder(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.logicalTimeImplementationName.empty(),
      "Time Constrained deletion disable requires an adapter-supplied logical-time implementation");

  Session publisher(options, model, "deletion-publisher");
  Session receiver(options, model, "deletion-subscriber");
  auto const federation = federationName(
      options,
      "time-constrained-disable-queued-deletion-to-receive-order");
  connectAndJoin(publisher, receiver, options, federation, options.fom);

  auto const publishedClass = publisher.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const subscribedClass = receiver.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const publishedAttribute = publisher.rtiAmbassador().getAttributeHandle(
      publishedClass,
      options.attributeName);
  auto const subscribedAttribute = receiver.rtiAmbassador().getAttributeHandle(
      subscribedClass,
      options.attributeName);
  require(
      publishedClass.isValid() && subscribedClass.isValid() &&
          publishedAttribute.isValid() && subscribedAttribute.isValid(),
      "Time Constrained deletion disable resolved an invalid object or attribute handle");
  publisher.rtiAmbassador().publishObjectClassAttributes(
      publishedClass,
      rti::AttributeHandleSet{publishedAttribute});
  receiver.rtiAmbassador().subscribeObjectClassAttributes(
      subscribedClass,
      rti::AttributeHandleSet{subscribedAttribute},
      true,
      L"");
  auto const queuedObject = publisher.rtiAmbassador().registerObjectInstance(publishedClass);
  auto const laterObject = publisher.rtiAmbassador().registerObjectInstance(publishedClass);
  require(
      queuedObject.isValid() && laterObject.isValid() && queuedObject != laterObject,
      "Time Constrained deletion disable did not register two distinct objects");
  waitFor(
      receiver,
      [&] {
        return receiver.recorder().hasDiscovery(queuedObject) &&
            receiver.recorder().hasDiscovery(laterObject);
      },
      options,
      "Time Constrained deletion disable discovery of both objects");

  auto publisherTime = makeTimeContext(publisher);
  auto receiverTime = makeTimeContext(receiver);
  require(
      publisherTime.factory->getName() == receiverTime.factory->getName(),
      "Time Constrained deletion disable members selected different logical-time factories");
  publisher.rtiAmbassador().enableTimeRegulation(*publisherTime.epsilon);
  waitFor(
      publisher,
      [&] { return publisher.recorder().timeRegulationEnabled().size() == 1U; },
      options,
      "Time Constrained deletion disable regulation callback");
  receiver.rtiAmbassador().enableTimeConstrained();
  waitFor(
      receiver,
      [&] { return receiver.recorder().timeConstrainedEnabled().size() == 1U; },
      options,
      "Time Constrained deletion disable enablement callback");

  std::vector<std::uint8_t> const queuedTagBytes{0x54U, 0x43U, 0x44U, 0xD1U};
  std::vector<std::uint8_t> const laterTagBytes{0x54U, 0x43U, 0x44U, 0xD2U};
  rti::VariableLengthData const queuedTag(
      queuedTagBytes.data(), queuedTagBytes.size());
  rti::VariableLengthData const laterTag(
      laterTagBytes.data(), laterTagBytes.size());
  auto const futureTime = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      5U);
  auto const queued = publisher.rtiAmbassador().deleteObjectInstance(
      queuedObject,
      queuedTag,
      *futureTime);
  require(queued.isValid(), "Queued timestamped deletion returned an invalid retraction handle");
  receiver.pump();
  require(
      receiver.recorder().removals().empty() &&
          receiver.recorder().timedRemovals().empty() &&
          receiver.recorder().timeAdvanceGrants().empty(),
      "Future object removal escaped before Time Constrained was disabled");

  receiver.rtiAmbassador().disableTimeConstrained();
  waitFor(
      receiver,
      [&] { return receiver.recorder().removals().size() == 1U; },
      options,
      "queued timestamped object removal in receive order after disable");
  auto const queuedRemoval = receiver.recorder().removals().front();
  require(
      receiver.recorder().timedRemovals().empty() &&
          receiver.recorder().timeAdvanceGrants().empty() &&
          queuedRemoval.object == queuedObject &&
          queuedRemoval.tag == queuedTagBytes &&
          queuedRemoval.producer == publisher.federateHandle(),
      "Queued deletion did not retain its object, tag, and publisher in receive order");

  auto const laterTime = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      3U);
  auto const later = publisher.rtiAmbassador().deleteObjectInstance(
      laterObject,
      laterTag,
      *laterTime);
  require(later.isValid(), "Subsequent timestamped deletion returned an invalid retraction handle");
  waitFor(
      receiver,
      [&] { return receiver.recorder().timedRemovals().size() == 1U; },
      options,
      "subsequent timestamped object removal after disable");
  auto const subsequentRemoval = receiver.recorder().timedRemovals().front();
  require(
      receiver.recorder().removals().size() == 1U &&
          subsequentRemoval.object == laterObject &&
          subsequentRemoval.tag == laterTagBytes &&
          subsequentRemoval.producer == publisher.federateHandle() &&
          subsequentRemoval.timeText == laterTime->toString() &&
          subsequentRemoval.sentOrder == rti::TIMESTAMP &&
          subsequentRemoval.receivedOrder == rti::RECEIVE,
      "Subsequent timestamped deletion did not retain receive-order delivery");

  publisher.rtiAmbassador().disableTimeRegulation();
  receiver.resign(rti::NO_ACTION);
  publisher.resign(rti::CANCEL_THEN_DELETE_THEN_DIVEST);
  publisher.rtiAmbassador().destroyFederationExecution(federation);
  receiver.disconnect();
  publisher.disconnect();
}

void scenarioTimeConstrainedDisableQueuedDeletionToReceiveOrderContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioTimeConstrainedDisableQueuedDeletionToReceiveOrder(options, model);
}

void scenarioTimeConstrainedDisableQueuedDirectedToReceiveOrder(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.logicalTimeImplementationName.empty(),
      "Directed Time Constrained disable requires an adapter-supplied logical-time implementation");

  Session publisher(options, model, "directed-publisher");
  Session receiver(options, model, "directed-subscriber");
  auto const federation = federationName(
      options,
      "time-constrained-disable-queued-directed-to-receive-order");
  connectAndJoin(publisher, receiver, options, federation, options.fom);

  auto const publishedObject = publisher.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const subscribedObject = receiver.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const publishedAttribute = publisher.rtiAmbassador().getAttributeHandle(
      publishedObject,
      options.attributeName);
  auto const subscribedAttribute = receiver.rtiAmbassador().getAttributeHandle(
      subscribedObject,
      options.attributeName);
  auto const publishedInteraction = publisher.rtiAmbassador().getInteractionClassHandle(
      options.interactionClassName);
  auto const subscribedInteraction = receiver.rtiAmbassador().getInteractionClassHandle(
      options.interactionClassName);
  require(
      publishedObject.isValid() && subscribedObject.isValid() &&
          publishedAttribute.isValid() && subscribedAttribute.isValid() &&
          publishedInteraction.isValid() && subscribedInteraction.isValid(),
      "Directed Time Constrained disable could not resolve adapter FOM handles");
  publisher.rtiAmbassador().publishObjectClassAttributes(
      publishedObject,
      rti::AttributeHandleSet{publishedAttribute});
  receiver.rtiAmbassador().subscribeObjectClassAttributes(
      subscribedObject,
      rti::AttributeHandleSet{subscribedAttribute},
      true,
      L"");
  publisher.rtiAmbassador().publishObjectClassDirectedInteractions(
      publishedObject,
      rti::InteractionClassHandleSet{publishedInteraction});
  receiver.rtiAmbassador().subscribeObjectClassDirectedInteractions(
      subscribedObject,
      rti::InteractionClassHandleSet{subscribedInteraction},
      true);
  publisher.rtiAmbassador().changeInteractionOrderType(
      publishedInteraction,
      rti::TIMESTAMP);
  auto const target = publisher.rtiAmbassador().registerObjectInstance(publishedObject);
  require(target.isValid(), "Directed Time Constrained disable registered an invalid target");
  waitFor(
      receiver,
      [&] { return receiver.recorder().hasDiscovery(target); },
      options,
      "directed Time Constrained disable target discovery");

  auto publisherTime = makeTimeContext(publisher);
  auto receiverTime = makeTimeContext(receiver);
  require(
      publisherTime.factory->getName() == receiverTime.factory->getName(),
      "Directed Time Constrained disable members selected different time factories");
  publisher.rtiAmbassador().enableTimeRegulation(*publisherTime.epsilon);
  waitFor(
      publisher,
      [&] { return publisher.recorder().timeRegulationEnabled().size() == 1U; },
      options,
      "directed Time Constrained disable publisher regulation");
  receiver.rtiAmbassador().enableTimeConstrained();
  waitFor(
      receiver,
      [&] { return receiver.recorder().timeConstrainedEnabled().size() == 1U; },
      options,
      "directed Time Constrained disable receiver enablement");

  std::vector<std::uint8_t> const queuedTagBytes{0x72U, 0xB1U, 0x1CU};
  std::vector<std::uint8_t> const laterTagBytes{0x72U, 0xB1U, 0x1DU};
  rti::VariableLengthData const queuedTag(
      queuedTagBytes.data(), queuedTagBytes.size());
  rti::VariableLengthData const laterTag(
      laterTagBytes.data(), laterTagBytes.size());
  auto const futureTime = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      5U);
  auto const queued = publisher.rtiAmbassador().sendDirectedInteraction(
      publishedInteraction,
      target,
      rti::ParameterHandleValueMap{},
      queuedTag,
      *futureTime);
  require(queued.isValid(), "Queued directed send returned an invalid retraction handle");
  receiver.pump();
  require(
      receiver.recorder().directedInteractions().empty() &&
          receiver.recorder().timedDirectedInteractions().empty() &&
          receiver.recorder().timeAdvanceGrants().empty(),
      "Future directed interaction escaped before Time Constrained was disabled");

  receiver.rtiAmbassador().disableTimeConstrained();
  waitFor(
      receiver,
      [&] { return receiver.recorder().directedInteractions().size() == 1U; },
      options,
      "queued timestamped directed interaction in receive order after disable");
  auto const queuedDelivery = receiver.recorder().directedInteractions().front();
  require(
      receiver.recorder().timedDirectedInteractions().empty() &&
          receiver.recorder().timeAdvanceGrants().empty() &&
          queuedDelivery.interaction == subscribedInteraction &&
          queuedDelivery.object == target &&
          queuedDelivery.parameters.empty() &&
          queuedDelivery.tag == queuedTagBytes &&
          queuedDelivery.producer == publisher.federateHandle() &&
          queuedDelivery.transportation == receiver.rtiAmbassador().getTransportationTypeHandle(L"HLAreliable"),
      "Queued directed interaction lost its target, tag, producer, or transportation in receive order");

  auto const laterTime = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      3U);
  auto const later = publisher.rtiAmbassador().sendDirectedInteraction(
      publishedInteraction,
      target,
      rti::ParameterHandleValueMap{},
      laterTag,
      *laterTime);
  require(later.isValid(), "Subsequent directed send returned an invalid retraction handle");
  waitFor(
      receiver,
      [&] { return receiver.recorder().timedDirectedInteractions().size() == 1U; },
      options,
      "subsequent timestamped directed interaction after disable");
  auto const subsequent = receiver.recorder().timedDirectedInteractions().front();
  require(
      receiver.recorder().directedInteractions().size() == 1U &&
          subsequent.interaction == subscribedInteraction &&
          subsequent.object == target &&
          subsequent.parameters.empty() &&
          subsequent.tag == laterTagBytes &&
          subsequent.producer == publisher.federateHandle() &&
          subsequent.timeText == laterTime->toString() &&
          subsequent.sentOrder == rti::TIMESTAMP &&
          subsequent.receivedOrder == rti::RECEIVE,
      "Subsequent timestamped directed interaction did not retain receive-order delivery");

  publisher.rtiAmbassador().disableTimeRegulation();
  receiver.resign(rti::NO_ACTION);
  publisher.resign(rti::CANCEL_THEN_DELETE_THEN_DIVEST);
  publisher.rtiAmbassador().destroyFederationExecution(federation);
  receiver.disconnect();
  publisher.disconnect();
}

void scenarioTimeConstrainedDisableQueuedDirectedToReceiveOrderContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioTimeConstrainedDisableQueuedDirectedToReceiveOrder(options, model);
}

}  // namespace hla_rti_cpp_tck::detail
