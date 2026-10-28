#include "tck_main_scenarios.hpp"

namespace hla_rti_cpp_tck::detail {

void scenarioNonRegulatingFederatePeerAdvanceIndependence(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.logicalTimeImplementationName.empty(),
      "Non-regulating peer advancement requires an adapter-supplied logical-time implementation");

  Session nonRegulator(options, model, "non-regulating-peer");
  Session regulator(options, model, "owner");
  Session constrained(options, model, "time-constrained-peer");
  auto const federation = federationName(
      options, "non-regulating-federate-peer-advance-independence");

  nonRegulator.connect();
  regulator.connect();
  constrained.connect();
  regulator.rtiAmbassador().createFederationExecution(
      federation,
      options.fom.wstring(),
      options.logicalTimeImplementationName);
  regulator.join(toWide("advancing-regulator"), options.federateType, federation);
  nonRegulator.join(
      toWide("non-regulating-peer"), options.federateType, federation);
  constrained.join(
      toWide("time-constrained-peer"), options.federateType, federation);

  auto nonRegulatorTime = makeTimeContext(nonRegulator);
  auto regulatorTime = makeTimeContext(regulator);
  auto constrainedTime = makeTimeContext(constrained);
  require(
      nonRegulatorTime.factory->getName() == regulatorTime.factory->getName() &&
          regulatorTime.factory->getName() == constrainedTime.factory->getName(),
      "Non-regulating peer advancement selected inconsistent logical-time factories");

  auto const nonRegulatorTarget = timeAfter(
      *nonRegulatorTime.factory,
      *nonRegulatorTime.initial,
      *nonRegulatorTime.epsilon,
      1U);
  nonRegulator.rtiAmbassador().timeAdvanceRequest(*nonRegulatorTarget);
  waitFor(
      nonRegulator,
      [&] { return nonRegulator.recorder().timeAdvanceGrants().size() == 1U; },
      options,
      "non-regulating peer's time-1 grant");
  require(
      nonRegulator.recorder().timeAdvanceGrants().front().encoded ==
          encodeTime(*nonRegulatorTarget),
      "Non-regulating peer received a grant at the wrong time");

  auto const regulatorTarget = timeAfter(
      *regulatorTime.factory,
      *regulatorTime.initial,
      *regulatorTime.epsilon,
      10U);
  regulator.rtiAmbassador().timeAdvanceRequest(*regulatorTarget);
  waitFor(
      regulator,
      [&] { return regulator.recorder().timeAdvanceGrants().size() == 1U; },
      options,
      "regulator's time-10 grant before enabling regulation");
  require(
      regulator.recorder().timeAdvanceGrants().front().encoded ==
          encodeTime(*regulatorTarget),
      "Regulator received a grant at the wrong time before enabling regulation");

  regulator.rtiAmbassador().enableTimeRegulation(*regulatorTime.epsilon);
  waitFor(
      regulator,
      [&] { return regulator.recorder().timeRegulationEnabled().size() == 1U; },
      options,
      "regulator's time-regulation callback");
  require(
      regulator.recorder().timeRegulationEnabled().front().encoded ==
          encodeTime(*regulatorTarget),
      "Regulator enabled at a time other than its completed grant");

  constrained.rtiAmbassador().enableTimeConstrained();
  waitFor(
      constrained,
      [&] { return constrained.recorder().timeConstrainedEnabled().size() == 1U; },
      options,
      "time-constrained peer's enablement callback");
  require(
      constrained.recorder().timeConstrainedEnabled().front().encoded ==
          encodeTime(*constrainedTime.initial),
      "Time-constrained peer did not enable at its initial time");

  auto const constrainedTarget = timeAfter(
      *constrainedTime.factory,
      *constrainedTime.initial,
      *constrainedTime.epsilon,
      10U);
  constrained.rtiAmbassador().timeAdvanceRequest(*constrainedTarget);
  waitForSessions(
      {&nonRegulator, &regulator, &constrained},
      [&] { return constrained.recorder().timeAdvanceGrants().size() == 1U; },
      options,
      "time-constrained peer's time-10 grant");

  require(
      constrained.recorder().timeAdvanceGrants().front().encoded ==
          encodeTime(*constrainedTarget),
      "Time-constrained peer did not advance to time 10 under the regulator's frontier");
  require(
      nonRegulator.recorder().timeAdvanceGrants().size() == 1U &&
          regulator.recorder().timeAdvanceGrants().size() == 1U,
      "Peer advancement changed an already completed federate grant");

  constrained.rtiAmbassador().disableTimeConstrained();
  regulator.rtiAmbassador().disableTimeRegulation();
  nonRegulator.resign(rti::NO_ACTION);
  constrained.resign(rti::NO_ACTION);
  regulator.resign(rti::NO_ACTION);
  regulator.rtiAmbassador().destroyFederationExecution(federation);
  nonRegulator.disconnect();
  constrained.disconnect();
  regulator.disconnect();
}

void scenarioNonRegulatingFederatePeerAdvanceIndependenceContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioNonRegulatingFederatePeerAdvanceIndependence(options, model);
}

}  // namespace hla_rti_cpp_tck::detail
