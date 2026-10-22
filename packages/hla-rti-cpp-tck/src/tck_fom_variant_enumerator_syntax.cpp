#include "tck_fom_module_scenarios.hpp"

namespace hla_rti_cpp_tck::detail {

void scenarioFomVariantEnumeratorSyntax(Options const& options,
                                        rti::CallbackModel model) {
  require(!options.logicalTimeImplementationName.empty(),
          "Variant enumerator syntax requires adapter-supplied logical time");
  auto const range = options.scenarioFoms.find("variant-enumerator-range");
  auto const hlaother = options.scenarioFoms.find("variant-enumerator-hlaother");
  auto const omitted = options.scenarioFoms.find("variant-enumerator-omitted");
  auto const malformed = options.scenarioFoms.find("variant-enumerator-malformed-range");
  auto const composite = options.scenarioFoms.find("variant-enumerator-composite-hlaother");
  require(range != options.scenarioFoms.end() &&
              hlaother != options.scenarioFoms.end() &&
              omitted != options.scenarioFoms.end() &&
              malformed != options.scenarioFoms.end() &&
              composite != options.scenarioFoms.end(),
          "Variant enumerator syntax requires adapter-supplied FOM modules");

  Session creator(options, model, "variant-enumerator-syntax-creator");
  auto const federation = federationName(options, "fom-variant-enumerator-syntax");
  creator.connect();
  auto& rti = creator.rtiAmbassador();
  auto const create = [&](std::filesystem::path const& module) {
    rti.createFederationExecution(federation, module.wstring(),
                                  options.logicalTimeImplementationName);
  };
  for (auto const& accepted : {range->second, hlaother->second, omitted->second}) {
    create(accepted);
    rti.destroyFederationExecution(federation);
  }
  for (auto const& rejected : {malformed->second, composite->second}) {
    requireException([&] { create(rejected); }, L"InconsistentFOM",
                     "malformed variant discriminant enumerator");
  }
  create(range->second);
  rti.destroyFederationExecution(federation);
  creator.disconnect();
}

void scenarioFomVariantEnumeratorSyntaxContract(Options const& options,
                                                rti::CallbackModel model) {
  scenarioFomVariantEnumeratorSyntax(options, model);
}

}  // namespace hla_rti_cpp_tck::detail
