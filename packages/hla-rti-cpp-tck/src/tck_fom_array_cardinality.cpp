#include "tck_fom_module_scenarios.hpp"

namespace hla_rti_cpp_tck::detail {
namespace {

std::filesystem::path const& requiredFom(Options const& options,
                                         std::string const& name) {
  auto const found = options.scenarioFoms.find(name);
  require(found != options.scenarioFoms.end(),
          "Array cardinality requires an adapter-supplied FOM module");
  return found->second;
}

void create(Session& creator, std::wstring const& federation,
            Options const& options, std::filesystem::path const& module) {
  creator.rtiAmbassador().createFederationExecution(
      federation, std::vector<std::wstring>{module.wstring()},
      options.logicalTimeImplementationName);
}

void runArrayCardinality(Options const& options, rti::CallbackModel model,
                         bool invalidForms) {
  require(!options.logicalTimeImplementationName.empty(),
          "Array cardinality requires adapter-supplied logical time");
  auto const& valid = requiredFom(options, "array-cardinality-valid");
  Session creator(options, model, "array-cardinality-creator");
  auto const federation = federationName(options, invalidForms
      ? "fom-array-cardinality-rejections" : "fom-array-cardinality-forms");
  creator.connect();
  create(creator, federation, options, valid);
  creator.rtiAmbassador().destroyFederationExecution(federation);

  if (invalidForms) {
    for (auto const& name : {"array-cardinality-reversed-range",
                             "array-cardinality-fixed-encoding-mismatch",
                             "array-cardinality-variable-encoding-mismatch"}) {
      auto const& invalid = requiredFom(options, name);
      requireException(
          [&] { create(creator, federation, options, invalid); },
          L"InconsistentFOM", name);
      // A rejected Create must not reserve the federation name.
      create(creator, federation, options, valid);
      creator.rtiAmbassador().destroyFederationExecution(federation);
    }
  }
  creator.disconnect();
}

}  // namespace

void scenarioFomArrayCardinalityForms(Options const& options,
                                       rti::CallbackModel model) {
  runArrayCardinality(options, model, false);
}

void scenarioFomArrayCardinalityFormsContract(Options const& options,
                                               rti::CallbackModel model) {
  scenarioFomArrayCardinalityForms(options, model);
}

void scenarioFomArrayCardinalityRejections(Options const& options,
                                            rti::CallbackModel model) {
  runArrayCardinality(options, model, true);
}

void scenarioFomArrayCardinalityRejectionsContract(Options const& options,
                                                    rti::CallbackModel model) {
  scenarioFomArrayCardinalityRejections(options, model);
}

}  // namespace hla_rti_cpp_tck::detail
