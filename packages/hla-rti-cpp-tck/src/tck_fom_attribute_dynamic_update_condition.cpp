#include "tck_fom_module_scenarios.hpp"

namespace hla_rti_cpp_tck::detail {
namespace {

std::filesystem::path const& requiredFom(Options const& options,
                                         std::string const& key) {
  auto const found = options.scenarioFoms.find(key);
  require(found != options.scenarioFoms.end(),
          "Dynamic update-condition checks require adapter-supplied FOMs");
  return found->second;
}

void runDynamicCondition(Options const& options, rti::CallbackModel model,
                         bool rejections) {
  require(!options.logicalTimeImplementationName.empty(),
          "Dynamic update-condition checks require adapter-supplied logical time");
  auto const& valid = requiredFom(options, "attribute-dynamic-update-condition-valid");
  Session creator(options, model, "attribute-dynamic-condition-creator");
  auto const federation = federationName(options, rejections
      ? "fom-dynamic-update-condition-rejections"
      : "fom-dynamic-update-condition-valid");
  creator.connect();
  auto const create = [&](std::filesystem::path const& module) {
    creator.rtiAmbassador().createFederationExecution(
        federation, std::vector<std::wstring>{module.wstring()},
        options.logicalTimeImplementationName);
  };
  create(valid);
  creator.rtiAmbassador().destroyFederationExecution(federation);

  if (rejections) {
    for (auto const& key : {"attribute-conditional-update-condition-na",
                            "attribute-periodic-update-condition-na",
                            "attribute-conditional-update-condition-empty"}) {
      auto const& invalid = requiredFom(options, key);
      requireException([&] { create(invalid); }, L"InconsistentFOM", key);
      create(valid);
      creator.rtiAmbassador().destroyFederationExecution(federation);
    }
  }
  creator.disconnect();
}

}  // namespace

void scenarioFomDynamicUpdateConditionValid(Options const& options,
                                             rti::CallbackModel model) {
  runDynamicCondition(options, model, false);
}

void scenarioFomDynamicUpdateConditionValidContract(Options const& options,
                                                     rti::CallbackModel model) {
  scenarioFomDynamicUpdateConditionValid(options, model);
}

void scenarioFomDynamicUpdateConditionRejections(Options const& options,
                                                  rti::CallbackModel model) {
  runDynamicCondition(options, model, true);
}

void scenarioFomDynamicUpdateConditionRejectionsContract(
    Options const& options, rti::CallbackModel model) {
  scenarioFomDynamicUpdateConditionRejections(options, model);
}

}  // namespace hla_rti_cpp_tck::detail
