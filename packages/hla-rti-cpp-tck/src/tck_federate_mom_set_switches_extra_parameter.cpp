#include "tck_main_scenarios.hpp"

#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace hla_rti_cpp_tck::detail {

void scenarioFederateMomSetSwitchesExtraParameter(
    Options const& options,
    rti::CallbackModel model) {
  auto const scenarioFom = options.scenarioFoms.find(
      "mom-set-switches-extra-parameter");
  require(
      scenarioFom != options.scenarioFoms.end() &&
          !scenarioFom->second.empty() && !options.mimFom.empty() &&
          !options.logicalTimeImplementationName.empty(),
      "HLAsetSwitches extra-parameter TCK requires its extension FOM, standard MIM, and logical-time configuration");

  Session federate(options, model, "mom-set-switches-extra-parameter");
  auto const federation = federationName(
      options, "federate-mom-set-switches-extra-parameter");
  federate.connect();
  federate.rtiAmbassador().createFederationExecutionWithMIM(
      federation,
      std::vector<std::wstring>{scenarioFom->second.wstring()},
      options.mimFom.wstring(),
      options.logicalTimeImplementationName);
  federate.join(
      options.ownerFederateName + L"-mom-set-switches-extra-parameter",
      options.federateType,
      federation);

  auto const setSwitches = federate.rtiAmbassador().getInteractionClassHandle(
      L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAadjust.HLAsetSwitches");
  auto const serviceReporting = federate.rtiAmbassador().getParameterHandle(
      setSwitches, L"HLAserviceReporting");
  auto const extensionPayload = federate.rtiAmbassador().getParameterHandle(
      setSwitches, L"TckExtensionSwitchPayload");
  require(
      setSwitches.isValid() && serviceReporting.isValid() &&
          extensionPayload.isValid(),
      "The standard HLAsetSwitches class or one of its standard/extension parameters is unavailable");
  require(
      !federate.rtiAmbassador().getServiceReportingSwitch(),
      "The HLAserviceReporting switch did not start in its defined false state");
  auto const initialAutomaticResign =
      federate.rtiAmbassador().getAutomaticResignDirective();

  std::array<unsigned char, 3U> const payload{0x01U, 0x02U, 0x03U};
  rti::ParameterHandleValueMap const extensionOnlyParameters{
      {extensionPayload,
       rti::VariableLengthData{payload.data(), payload.size()}},
  };
  // Promotion can leave no predefined parameters in this interaction. The
  // standard HLAsetSwitches handler may reject that empty predefined view;
  // either outcome must leave its predefined switches unchanged.
  try {
    federate.rtiAmbassador().sendInteraction(
        setSwitches, extensionOnlyParameters, rti::VariableLengthData{});
  } catch (rti::InteractionParameterNotDefined const&) {
  }
  require(
      !federate.rtiAmbassador().getServiceReportingSwitch(),
      "An extension-only HLAsetSwitches parameter changed the predefined HLAserviceReporting switch");
  require(
      federate.rtiAmbassador().getAutomaticResignDirective() ==
          initialAutomaticResign,
      "An extension-only HLAsetSwitches parameter changed an unrelated predefined switch");

  rti::ParameterHandleValueMap const parameters{
      {serviceReporting, rti::HLAinteger32BE{1}.encode()},
      {extensionPayload,
       rti::VariableLengthData{payload.data(), payload.size()}},
  };
  federate.rtiAmbassador().sendInteraction(
      setSwitches, parameters, rti::VariableLengthData{});
  require(
      federate.rtiAmbassador().getServiceReportingSwitch(),
      "HLAsetSwitches did not process the predefined HLAserviceReporting parameter when an extension parameter was present");
  require(
      federate.rtiAmbassador().getAutomaticResignDirective() ==
          initialAutomaticResign,
      "The extension-only HLAsetSwitches parameter changed an unrelated predefined switch");

  federate.resign(rti::NO_ACTION);
  federate.rtiAmbassador().destroyFederationExecution(federation);
  federate.disconnect();
}

}  // namespace hla_rti_cpp_tck::detail
