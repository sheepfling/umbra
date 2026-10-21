#include "tck_nrg_regulator_resignation.hpp"

namespace hla_rti_cpp_tck::detail {
namespace {

void runNrgRegulatorRelease(
    Options const& options, rti::CallbackModel model, bool disableRegulation) {
  require(!options.fom.empty(),
          "NRG regulator release requires an adapter-supplied complete NRG-enabled FOM");
  require(!options.logicalTimeImplementationName.empty(),
          "NRG regulator resignation requires an adapter-supplied logical-time implementation");

  Session receiver(options, model, "nrg-release-receiver");
  Session regulator(options, model, "nrg-release-regulator");
  auto const federation = federationName(
      options, disableRegulation ? "nrg-regulator-disable" : "nrg-regulator-resignation");
  receiver.connect();
  regulator.connect();
  receiver.rtiAmbassador().createFederationExecution(
      federation, options.fom.wstring(), options.logicalTimeImplementationName);
  receiver.join(options.ownerFederateName, options.federateType, federation);
  regulator.join(options.memberFederateName, options.federateType, federation);
  require(receiver.rtiAmbassador().getNonRegulatedGrantSwitch() &&
              regulator.rtiAmbassador().getNonRegulatedGrantSwitch(),
          "Adapter FOM did not enable the standard non-regulated grant switch");

  auto const receiverTime = makeTimeContext(receiver);
  auto const regulatorTime = makeTimeContext(regulator);
  require(receiverTime.factory->getName() == regulatorTime.factory->getName(),
          "NRG participants selected different logical-time factories");
  auto const target = timeAfter(*receiverTime.factory, *receiverTime.initial,
                                *receiverTime.epsilon, 1U);

  receiver.rtiAmbassador().enableTimeConstrained();
  waitFor(receiver,
          [&] { return receiver.recorder().timeConstrainedEnabled().size() == 1U; },
          options, "NRG receiver time-constrained enablement");
  regulator.rtiAmbassador().enableTimeRegulation(*regulatorTime.epsilon);
  waitFor(regulator,
          [&] { return regulator.recorder().timeRegulationEnabled().size() == 1U; },
          options, "NRG regulator time-regulation enablement");

  auto galt = receiverTime.factory->makeInitial();
  require(galt != nullptr && receiver.rtiAmbassador().queryGALT(*galt),
          "NRG setup did not establish a defined GALT");
  requireTimeEquals(*galt, *target, "NRG setup GALT at strict TAR boundary");

  receiver.rtiAmbassador().timeAdvanceRequest(*target);
  if (model == rti::HLA_EVOKED) {
    receiver.rtiAmbassador().evokeCallback(0.0);
  }
  require(receiver.recorder().timeAdvanceGrants().empty(),
          "Strict TAR was granted while the sole regulator still bounded GALT");

  if (disableRegulation) {
    regulator.rtiAmbassador().disableTimeRegulation();
  } else {
    regulator.resign(rti::NO_ACTION);
  }
  waitFor(receiver,
          [&] { return receiver.recorder().timeAdvanceGrants().size() == 1U; },
          options, "NRG grant after sole regulator release");
  auto const grants = receiver.recorder().timeAdvanceGrants();
  require(grants.size() == 1U && grants.front().encoded == encodeTime(*target),
          "Sole regulator release did not grant strict TAR at its requested time");

  if (disableRegulation) regulator.resign(rti::NO_ACTION);
  receiver.resign(rti::NO_ACTION);
  receiver.rtiAmbassador().destroyFederationExecution(federation);
  regulator.disconnect();
  receiver.disconnect();
}

}  // namespace

void scenarioNrgRegulatorResignation(Options const& options, rti::CallbackModel model) {
  runNrgRegulatorRelease(options, model, false);
}

void scenarioNrgRegulatorResignationContract(
    Options const& options, rti::CallbackModel model) {
  runNrgRegulatorRelease(options, model, false);
}

void scenarioNrgRegulatorDisable(Options const& options, rti::CallbackModel model) {
  runNrgRegulatorRelease(options, model, true);
}

void scenarioNrgRegulatorDisableContract(
    Options const& options, rti::CallbackModel model) {
  runNrgRegulatorRelease(options, model, true);
}

}  // namespace hla_rti_cpp_tck::detail
