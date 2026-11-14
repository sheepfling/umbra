#include "tck_fom_module_scenarios.hpp"

namespace hla_rti_cpp_tck::detail {
namespace {

std::filesystem::path const& modelIdentificationNameFom(Options const& options) {
  auto const found = options.scenarioFoms.find("model-identification-name-valid");
  require(
      found != options.scenarioFoms.end(),
      "Model Identification name check needs an adapter-supplied FOM");
  return found->second;
}

}  // namespace

void scenarioFomModelIdentificationNameValid(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.logicalTimeImplementationName.empty(),
      "Model Identification name check needs adapter-supplied logical time");
  auto const& fom = modelIdentificationNameFom(options);
  Session creator(options, model, "model-identification-name-creator");
  auto const federation = federationName(options, "fom-model-identification-name");
  creator.connect();
  creator.rtiAmbassador().createFederationExecution(
      federation,
      std::vector<std::wstring>{fom.wstring()},
      options.logicalTimeImplementationName);
  creator.rtiAmbassador().destroyFederationExecution(federation);
  creator.disconnect();
}

void scenarioFomModelIdentificationNameValidContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioFomModelIdentificationNameValid(options, model);
}

}  // namespace hla_rti_cpp_tck::detail
