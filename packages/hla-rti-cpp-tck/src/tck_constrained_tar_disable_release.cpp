#include "tck_constrained_tar_disable_release.hpp"

namespace hla_rti_cpp_tck::detail {
namespace {

void runConstrainedTarDisableRelease(
    Options const& options, rti::CallbackModel model) {
  require(!options.fom.empty(),
          "Constrained TAR role-disable release requires an adapter-supplied FOM");
  require(!options.logicalTimeImplementationName.empty(),
          "Constrained TAR role-disable release requires an adapter-supplied logical-time implementation");

  Session receiver(options, model, "constrained-tar-disable-receiver");
  auto const federation = federationName(options, "constrained-tar-disable-release");
  receiver.connect();
  receiver.rtiAmbassador().createFederationExecution(
      federation, options.fom.wstring(), options.logicalTimeImplementationName);
  receiver.join(options.ownerFederateName, options.federateType, federation);
  require(!receiver.rtiAmbassador().getNonRegulatedGrantSwitch(),
          "Adapter FOM did not disable the standard non-regulated grant switch");

  auto const receiverTime = makeTimeContext(receiver);
  auto const target = timeAfter(*receiverTime.factory, *receiverTime.initial,
                                *receiverTime.epsilon, 1U);
  receiver.rtiAmbassador().enableTimeConstrained();
  waitFor(receiver,
          [&] { return receiver.recorder().timeConstrainedEnabled().size() == 1U; },
          options, "constrained TAR receiver role enablement");

  receiver.rtiAmbassador().timeAdvanceRequest(*target);
  if (model == rti::HLA_EVOKED) receiver.rtiAmbassador().evokeCallback(0.0);
  require(receiver.recorder().timeAdvanceGrants().empty(),
          "NRG-disabled constrained TAR was granted without a regulator");

  receiver.rtiAmbassador().disableTimeConstrained();
  waitFor(receiver,
          [&] { return receiver.recorder().timeAdvanceGrants().size() == 1U; },
          options, "grant after disabling time-constrained role");
  auto const grants = receiver.recorder().timeAdvanceGrants();
  require(grants.size() == 1U && grants.front().encoded == encodeTime(*target),
          "Disabling the constrained role did not grant the pending TAR at its requested time");

  receiver.resign(rti::NO_ACTION);
  receiver.rtiAmbassador().destroyFederationExecution(federation);
  receiver.disconnect();
}

}  // namespace

void scenarioConstrainedTarDisableRelease(
    Options const& options, rti::CallbackModel model) {
  runConstrainedTarDisableRelease(options, model);
}

void scenarioConstrainedTarDisableReleaseContract(
    Options const& options, rti::CallbackModel model) {
  runConstrainedTarDisableRelease(options, model);
}

}  // namespace hla_rti_cpp_tck::detail
