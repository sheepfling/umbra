#include "tck_fom_module_scenarios.hpp"

namespace hla_rti_cpp_tck::detail {

void scenarioFomArrayElementCategory(Options const& options,
                                     rti::CallbackModel model) {
  require(!options.logicalTimeImplementationName.empty(),
          "Array Element Type requires adapter-supplied logical time");
  auto const valid = options.scenarioFoms.find("array-element-valid");
  auto const omitted = options.scenarioFoms.find("array-element-omitted");
  auto const basic = options.scenarioFoms.find("array-element-basic");
  require(valid != options.scenarioFoms.end() &&
              omitted != options.scenarioFoms.end() &&
              basic != options.scenarioFoms.end(),
          "Array Element Type requires adapter-supplied FOM modules");

  Session creator(options, model, "array-element-creator");
  auto const federation = federationName(options, "fom-array-element-category");
  creator.connect();
  auto& rti = creator.rtiAmbassador();
  auto const create = [&](std::filesystem::path const& module) {
    rti.createFederationExecution(federation, module.wstring(),
                                  options.logicalTimeImplementationName);
  };
  create(valid->second);
  rti.destroyFederationExecution(federation);
  create(omitted->second);
  rti.destroyFederationExecution(federation);

  requireException([&] { create(basic->second); }, L"InconsistentFOM",
                   "raw basic-data array Element Type");
  create(valid->second);
  rti.destroyFederationExecution(federation);
  creator.disconnect();
}

void scenarioFomArrayElementCategoryContract(Options const& options,
                                             rti::CallbackModel model) {
  scenarioFomArrayElementCategory(options, model);
}

}  // namespace hla_rti_cpp_tck::detail
