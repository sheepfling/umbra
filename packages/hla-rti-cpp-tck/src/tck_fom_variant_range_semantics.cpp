#include "tck_fom_module_scenarios.hpp"

namespace hla_rti_cpp_tck::detail {

void scenarioFomVariantRangeSemantics(Options const& options,
                                      rti::CallbackModel model) {
  require(!options.logicalTimeImplementationName.empty(),
          "Variant range semantics requires adapter-supplied logical time");
  auto const base = options.scenarioFoms.find("variant-range-order-base");
  auto const valid = options.scenarioFoms.find("variant-range-order-valid-extension");
  auto const conflict = options.scenarioFoms.find("variant-range-order-conflict-extension");
  auto const hlaother = options.scenarioFoms.find("variant-range-other-complement");
  require(base != options.scenarioFoms.end() &&
              valid != options.scenarioFoms.end() &&
              conflict != options.scenarioFoms.end() &&
              hlaother != options.scenarioFoms.end(),
          "Variant range semantics requires adapter-supplied FOM modules");

  Session creator(options, model, "variant-range-semantics-creator");
  auto const federation = federationName(options, "fom-variant-range-semantics");
  creator.connect();
  auto& rti = creator.rtiAmbassador();
  auto const create = [&](std::vector<std::wstring> const& modules) {
    rti.createFederationExecution(federation, modules,
                                  options.logicalTimeImplementationName);
  };
  auto const resolved = std::vector<std::wstring>{base->second.wstring(),
                                                  valid->second.wstring()};
  create(resolved);
  rti.destroyFederationExecution(federation);
  create({hlaother->second.wstring()});
  rti.destroyFederationExecution(federation);
  requireException(
      [&] { create({base->second.wstring(), conflict->second.wstring()}); },
      L"InconsistentFOM", "merged range overlapping a different named alternative");
  create(resolved);
  rti.destroyFederationExecution(federation);
  creator.disconnect();
}

void scenarioFomVariantRangeSemanticsContract(Options const& options,
                                              rti::CallbackModel model) {
  scenarioFomVariantRangeSemantics(options, model);
}

}  // namespace hla_rti_cpp_tck::detail
