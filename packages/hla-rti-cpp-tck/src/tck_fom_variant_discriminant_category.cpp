#include "tck_fom_module_scenarios.hpp"

namespace hla_rti_cpp_tck::detail {

void scenarioFomVariantDiscriminantCategory(Options const& options,
                                            rti::CallbackModel model) {
  require(!options.logicalTimeImplementationName.empty(),
          "Variant discriminant type requires adapter-supplied logical time");
  auto const valid = options.scenarioFoms.find("variant-discriminant-valid");
  auto const omitted = options.scenarioFoms.find("variant-discriminant-omitted");
  auto const basic = options.scenarioFoms.find("variant-discriminant-basic");
  auto const simple = options.scenarioFoms.find("variant-discriminant-simple");
  auto const na = options.scenarioFoms.find("variant-discriminant-na");
  require(valid != options.scenarioFoms.end() &&
              omitted != options.scenarioFoms.end() &&
              basic != options.scenarioFoms.end() &&
              simple != options.scenarioFoms.end() &&
              na != options.scenarioFoms.end(),
          "Variant discriminant type requires adapter-supplied FOM modules");

  Session creator(options, model, "variant-discriminant-creator");
  auto const federation = federationName(options, "fom-variant-discriminant-category");
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

  for (auto const& invalid : {basic->second, simple->second, na->second}) {
    requireException([&] { create(invalid); }, L"InconsistentFOM",
                     "non-enumerated variant Discriminant Type");
  }
  create(valid->second);
  rti.destroyFederationExecution(federation);
  creator.disconnect();
}

void scenarioFomVariantDiscriminantCategoryContract(Options const& options,
                                                    rti::CallbackModel model) {
  scenarioFomVariantDiscriminantCategory(options, model);
}

}  // namespace hla_rti_cpp_tck::detail
