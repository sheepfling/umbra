#include "tck_harness.hpp"
#include "tck_main_scenarios.hpp"

#include <filesystem>
#include <fstream>

namespace hla_rti_cpp_tck::detail {

void scenarioFederateResignedForcedDeparture(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.federateResignationCoordinationDirectory.empty(),
      "forced-resignation TCK requires an adapter coordination directory");

  Session owner(options, model, "federate-resigned-owner");
  Session departed(options, model, "federate-resigned-target");
  owner.connect();
  departed.connect();

  auto const federation = federationName(
      options,
      "federate-resigned-forced-departure");
  owner.rtiAmbassador().createFederationExecution(
      federation,
      options.fom.wstring(),
      options.logicalTimeImplementationName);
  owner.join(
      options.ownerFederateName,
      options.federateType,
      federation);
  departed.join(
      options.memberFederateName,
      options.federateType,
      federation);

  auto const readyMarker =
      options.federateResignationCoordinationDirectory /
      "federate-resigned-ready.ok";
  std::ofstream marker(readyMarker, std::ios::binary | std::ios::trunc);
  require(
      static_cast<bool>(marker),
      "could not create the forced-resignation readiness marker");
  marker << "ready\n";
  marker.flush();
  require(
      static_cast<bool>(marker),
      "could not publish the forced-resignation readiness marker");

  waitFor(
      departed,
      [&] {
        return !departed.recorder().federateResignedReasons().empty();
      },
      options,
      "the RTI-forced Federate Resigned callback");

  auto const reasons = departed.recorder().federateResignedReasons();
  require(
      reasons.size() == 1U && !reasons.front().empty(),
      "forced departure did not deliver exactly one Federate Resigned reason");
  require(
      !departed.recorder().hasConnectionLost(),
      "the forced Federate Resigned callback was replaced by Connection Lost");
  requireException(
      [&] {
        static_cast<void>(departed.rtiAmbassador().getFederateHandle(
            options.memberFederateName));
      },
      L"FederateNotExecutionMember",
      "querying membership after forced resignation");

  departed.join(
      options.memberFederateName,
      options.federateType,
      federation);
  departed.resign(rti::NO_ACTION);
  if (model == rti::HLA_EVOKED) {
    while (departed.rtiAmbassador().evokeMultipleCallbacks(0.0, 0.0)) {
    }
  }
  require(
      departed.recorder().federateResignedReasons().size() == 1U,
      "federate-initiated resignation generated a Federate Resigned callback");
  owner.resign(rti::NO_ACTION);
  departed.disconnect();
  owner.disconnect();
}

}  // namespace hla_rti_cpp_tck::detail
