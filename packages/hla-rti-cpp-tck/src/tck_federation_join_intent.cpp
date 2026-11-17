#include "tck_main_scenarios.hpp"

namespace hla_rti_cpp_tck::detail {

void scenarioJoinFederationIntent(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.fom.empty(),
      "Federation join-intent witness requires an adapter-supplied FOM");

  Session creator(options, model, "join-intent-creator");
  Session intendingFederate(options, model, "join-intent-application");
  Session targetObserver(options, model, "join-intent-target-observer");
  Session decoyObserver(options, model, "join-intent-decoy-observer");

  auto const targetFederation = federationName(options, "join-intent-target");
  auto const decoyFederation = options.federationOverride.empty()
      ? federationName(options, "join-intent-decoy")
      : targetFederation + L"-decoy";
  require(
      targetFederation != decoyFederation,
      "Join-intent fixture did not create distinct target and decoy executions");

  creator.connect();
  creator.rtiAmbassador().createFederationExecution(
      targetFederation,
      options.fom.wstring(),
      options.logicalTimeImplementationName);
  creator.rtiAmbassador().createFederationExecution(
      decoyFederation,
      options.fom.wstring(),
      options.logicalTimeImplementationName);

  targetObserver.connect();
  targetObserver.join(
      L"tck-join-intent-target-observer",
      options.federateType,
      targetFederation);
  decoyObserver.connect();
  decoyObserver.join(
      L"tck-join-intent-decoy-observer",
      options.federateType,
      decoyFederation);

  struct JoinIntent final {
    std::wstring federationExecutionName;
    std::wstring federateName;
    std::wstring federateType;
  };
  JoinIntent const intent{
      targetFederation,
      L"tck-join-intent-intending-federate",
      options.federateType};

  intendingFederate.connect();
  intendingFederate.join(
      intent.federateName,
      intent.federateType,
      intent.federationExecutionName);
  auto const joinedHandle = intendingFederate.federateHandle();
  require(joinedHandle.isValid(), "Join did not return a valid federate handle");

  require(
      targetObserver.rtiAmbassador().getFederateHandle(intent.federateName) ==
          joinedHandle &&
          targetObserver.rtiAmbassador().getFederateName(joinedHandle) ==
              intent.federateName,
      "Join intent did not place the application in the specified federation");
  requireException(
      [&] {
        static_cast<void>(
            decoyObserver.rtiAmbassador().getFederateHandle(intent.federateName));
      },
      L"NameNotFound",
      "the intended federate appearing in the unselected federation");

  intendingFederate.resign(rti::NO_ACTION);
  targetObserver.resign(rti::NO_ACTION);
  decoyObserver.resign(rti::NO_ACTION);
  creator.rtiAmbassador().destroyFederationExecution(targetFederation);
  creator.rtiAmbassador().destroyFederationExecution(decoyFederation);
  intendingFederate.disconnect();
  targetObserver.disconnect();
  decoyObserver.disconnect();
  creator.disconnect();
}

}  // namespace hla_rti_cpp_tck::detail
