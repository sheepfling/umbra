#include "tck_nrg_disabled_regulator_activation.hpp"

namespace hla_rti_cpp_tck::detail {
namespace {

void runNrgDisabledRegulatorActivation(
    Options const& options, rti::CallbackModel model) {
  require(!options.fom.empty(),
          "NRG-disabled regulator activation requires an adapter-supplied FOM");
  require(!options.logicalTimeImplementationName.empty(),
          "NRG-disabled regulator activation requires an adapter-supplied logical-time implementation");

  Session receiver(options, model, "nrg-disabled-receiver");
  Session regulator(options, model, "nrg-disabled-regulator");
  auto const federation = federationName(options, "nrg-disabled-regulator-activation");
  receiver.connect();
  regulator.connect();
  receiver.rtiAmbassador().createFederationExecution(
      federation, options.fom.wstring(), options.logicalTimeImplementationName);
  receiver.join(options.ownerFederateName, options.federateType, federation);
  regulator.join(options.memberFederateName, options.federateType, federation);
  require(!receiver.rtiAmbassador().getNonRegulatedGrantSwitch() &&
              !regulator.rtiAmbassador().getNonRegulatedGrantSwitch(),
          "Adapter FOM did not disable the standard non-regulated grant switch");

  auto const receiverTime = makeTimeContext(receiver);
  auto const regulatorTime = makeTimeContext(regulator);
  require(receiverTime.factory->getName() == regulatorTime.factory->getName(),
          "NRG-disabled participants selected different logical-time factories");
  auto const target = timeAfter(*receiverTime.factory, *receiverTime.initial,
                                *receiverTime.epsilon, 1U);
  auto lookahead = regulatorTime.factory->makeZero();
  require(lookahead != nullptr,
          "NRG-disabled regulator could not allocate lookahead");
  *lookahead += *regulatorTime.epsilon;
  *lookahead += *regulatorTime.epsilon;

  receiver.rtiAmbassador().enableTimeConstrained();
  waitFor(receiver,
          [&] { return receiver.recorder().timeConstrainedEnabled().size() == 1U; },
          options, "NRG-disabled receiver time-constrained enablement");
  receiver.rtiAmbassador().timeAdvanceRequest(*target);
  if (model == rti::HLA_EVOKED) receiver.rtiAmbassador().evokeCallback(0.0);
  require(receiver.recorder().timeAdvanceGrants().empty(),
          "NRG-disabled constrained TAR was granted without a regulator");

  // A regulator at initial time with lookahead greater than the requested
  // advance establishes a non-limiting GALT and releases the pending TAR.
  regulator.rtiAmbassador().enableTimeRegulation(*lookahead);
  waitFor(regulator,
          [&] { return regulator.recorder().timeRegulationEnabled().size() == 1U; },
          options, "NRG-disabled regulator enablement");
  waitFor(receiver,
          [&] { return receiver.recorder().timeAdvanceGrants().size() == 1U; },
          options, "NRG-disabled grant after regulator activation");
  auto const grants = receiver.recorder().timeAdvanceGrants();
  require(grants.size() == 1U && grants.front().encoded == encodeTime(*target),
          "Regulator activation did not grant the pending TAR at its requested time");

  regulator.resign(rti::NO_ACTION);
  receiver.resign(rti::NO_ACTION);
  receiver.rtiAmbassador().destroyFederationExecution(federation);
  regulator.disconnect();
  receiver.disconnect();
}

}  // namespace

void scenarioNrgDisabledRegulatorActivation(
    Options const& options, rti::CallbackModel model) {
  runNrgDisabledRegulatorActivation(options, model);
}

void scenarioNrgDisabledRegulatorActivationContract(
    Options const& options, rti::CallbackModel model) {
  runNrgDisabledRegulatorActivation(options, model);
}

}  // namespace hla_rti_cpp_tck::detail
