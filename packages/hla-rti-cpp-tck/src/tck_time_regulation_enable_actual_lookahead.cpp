#include "tck_main_scenarios.hpp"

namespace hla_rti_cpp_tck::detail {

void scenarioTimeRegulationEnableActualLookaheadTsoSend(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.logicalTimeImplementationName.empty(),
      "Time Regulation lookahead requires an adapter-supplied logical-time implementation");

  Session sender(options, model, "owner");
  auto const federation = federationName(
      options, "time-regulation-enable-actual-lookahead-tso-send");
  sender.connect();
  sender.rtiAmbassador().createFederationExecution(
      federation, options.fom.wstring(), options.logicalTimeImplementationName);
  sender.join(options.ownerFederateName, options.federateType, federation);

  auto const interaction = sender.rtiAmbassador().getInteractionClassHandle(
      options.interactionClassName);
  require(interaction.isValid(), "Time Regulation lookahead interaction lookup failed");
  sender.rtiAmbassador().publishInteractionClass(interaction);
  sender.rtiAmbassador().changeInteractionOrderType(interaction, rti::TIMESTAMP);

  auto const time = makeTimeContext(sender);
  auto const one = timeAfter(
      *time.factory, *time.initial, *time.epsilon, 1U);
  auto const two = timeAfter(
      *time.factory, *time.initial, *time.epsilon, 2U);
  auto requestedLookahead = time.factory->makeZero();
  require(requestedLookahead != nullptr, "Could not allocate requested lookahead");
  requestedLookahead->setToDifference(*two, *time.initial);

  sender.rtiAmbassador().enableTimeRegulation(*requestedLookahead);
  waitFor(
      sender,
      [&] { return sender.recorder().timeRegulationEnabled().size() == 1U; },
      options,
      "Time Regulation Enabled callback for actual-lookahead send boundary");
  require(
      sender.recorder().timeRegulationEnabled().front().encoded ==
          encodeTime(*time.initial),
      "Time Regulation Enabled reported the wrong assigned logical time");

  auto actualLookahead = time.factory->makeZero();
  auto currentTime = time.factory->makeInitial();
  require(
      actualLookahead != nullptr && currentTime != nullptr,
      "Could not allocate post-enable time queries");
  sender.rtiAmbassador().queryLookahead(*actualLookahead);
  sender.rtiAmbassador().queryLogicalTime(*currentTime);
  require(
      *actualLookahead == *requestedLookahead && *currentTime == *time.initial,
      "Enable Time Regulation did not retain the requested lookahead and assigned time");

  rti::ParameterHandleValueMap const parameters;
  rti::VariableLengthData const tag;
  bool belowFloorRejected = false;
  try {
    static_cast<void>(sender.rtiAmbassador().sendInteraction(
        interaction, parameters, tag, *one));
  } catch (rti::InvalidLogicalTime const&) {
    belowFloorRejected = true;
  }
  require(
      belowFloorRejected,
      "Send Interaction accepted a TSO timestamp below the actual-lookahead floor");
  auto const retraction = sender.rtiAmbassador().sendInteraction(
      interaction, parameters, tag, *two);
  require(
      retraction.isValid(),
      "Send Interaction rejected a TSO timestamp at the actual-lookahead floor");

  sender.rtiAmbassador().disableTimeRegulation();
  sender.resign(rti::NO_ACTION);
  sender.rtiAmbassador().destroyFederationExecution(federation);
  sender.disconnect();
}

void scenarioTimeRegulationEnableActualLookaheadTsoSendContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioTimeRegulationEnableActualLookaheadTsoSend(options, model);
}

}  // namespace hla_rti_cpp_tck::detail
