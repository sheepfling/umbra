#include "tck_main_scenarios.hpp"

namespace hla_rti_cpp_tck::detail {
namespace {

void runDisableTimeRegulationSendOrder(Options const& options, rti::CallbackModel model) {
  require(model == rti::HLA_IMMEDIATE,
          "This native-green send-order transition is scoped to HLA_IMMEDIATE");
  require(!options.logicalTimeImplementationName.empty(),
          "Send-order transition requires an adapter-supplied time implementation");

  Session sender(options, model, "sender");
  Session receiver(options, model, "receiver");
  auto const federation = federationName(options, "disable-time-regulation-send-order");
  connectAndJoin(sender, receiver, options, federation, options.fom);

  auto const sentClass = sender.rtiAmbassador().getInteractionClassHandle(
      options.interactionClassName);
  auto const receivedClass = receiver.rtiAmbassador().getInteractionClassHandle(
      options.interactionClassName);
  require(sentClass.isValid() && receivedClass.isValid(),
          "Adapter interaction class could not be resolved");
  sender.rtiAmbassador().publishInteractionClass(sentClass);
  sender.rtiAmbassador().changeInteractionOrderType(sentClass, rti::TIMESTAMP);
  receiver.rtiAmbassador().subscribeInteractionClass(receivedClass);

  auto const time = makeTimeContext(sender);
  sender.rtiAmbassador().enableTimeRegulation(*time.epsilon);
  waitFor(sender,
          [&] { return sender.recorder().timeRegulationEnabled().size() == 1U; },
          options, "Time Regulation Enabled callback before disable");
  sender.rtiAmbassador().disableTimeRegulation();

  auto const stamp = timeAfter(*time.factory, *time.initial, *time.epsilon, 5U);
  sender.rtiAmbassador().sendInteraction(
      sentClass, rti::ParameterHandleValueMap{}, rti::VariableLengthData{}, *stamp);
  waitFor(receiver,
          [&] {
            return receiver.recorder().interactions().size() +
                receiver.recorder().timedInteractions().size() == 1U;
          }, options, "post-disable receive-order interaction");

  auto const ordinary = receiver.recorder().interactions();
  auto const timed = receiver.recorder().timedInteractions();
  require(ordinary.size() + timed.size() == 1U,
          "Post-disable timestamped send produced more than one interaction");
  if (ordinary.empty()) {
    require(timed.front().interaction == receivedClass &&
                timed.front().sentOrder == rti::RECEIVE &&
                timed.front().receivedOrder == rti::RECEIVE,
            "Post-disable timestamped callback did not report receive order");
  } else {
    require(ordinary.front().interaction == receivedClass,
            "Post-disable receive-order callback had the wrong interaction class");
  }

  receiver.resign(rti::NO_ACTION);
  sender.resign(rti::NO_ACTION);
  sender.rtiAmbassador().destroyFederationExecution(federation);
  receiver.disconnect();
  sender.disconnect();
}

}  // namespace

void scenarioDisableTimeRegulationSendOrder(Options const& options, rti::CallbackModel model) {
  runDisableTimeRegulationSendOrder(options, model);
}

void scenarioDisableTimeRegulationSendOrderContract(
    Options const& options, rti::CallbackModel model) {
  runDisableTimeRegulationSendOrder(options, model);
}

}  // namespace hla_rti_cpp_tck::detail
