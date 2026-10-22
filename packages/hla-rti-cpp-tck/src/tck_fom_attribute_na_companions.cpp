#include "tck_fom_module_scenarios.hpp"

namespace hla_rti_cpp_tck::detail {
namespace {

std::filesystem::path const& requiredFom(Options const& options,
                                         std::string const& key) {
  auto const found = options.scenarioFoms.find(key);
  require(found != options.scenarioFoms.end(),
          "NA attribute companion checks require adapter-supplied FOMs");
  return found->second;
}

void runAttributeNaCompanions(Options const& options,
                             rti::CallbackModel model, bool rejections) {
  require(!options.logicalTimeImplementationName.empty(),
          "NA attribute companion checks require adapter-supplied logical time");
  auto const& valid = requiredFom(options, "attribute-na-valid-companions");
  Session creator(options, model, "attribute-na-companions-creator");
  auto const federation = federationName(options, rejections
      ? "fom-attribute-na-companion-rejections"
      : "fom-attribute-na-companion-completion");
  creator.connect();
  auto const create = [&](std::vector<std::wstring> const& modules) {
    creator.rtiAmbassador().createFederationExecution(
        federation, modules, options.logicalTimeImplementationName);
  };
  auto const validModules = std::vector<std::wstring>{valid.wstring()};
  create(validModules);
  creator.rtiAmbassador().destroyFederationExecution(federation);

  if (rejections) {
    for (auto const& key : {"attribute-na-invalid-update-type",
                            "attribute-na-invalid-update-condition",
                            "attribute-na-invalid-transportation"}) {
      auto const& invalid = requiredFom(options, key);
      requireException([&] { create({invalid.wstring()}); },
                       L"InconsistentFOM", key);
      create(validModules);
      creator.rtiAmbassador().destroyFederationExecution(federation);
    }
  } else {
    auto const& partial = requiredFom(options, "attribute-na-omitted-companions");
    auto const& completion =
        requiredFom(options, "attribute-na-omitted-companions-extension");
    create({partial.wstring(), completion.wstring()});
    creator.rtiAmbassador().destroyFederationExecution(federation);
  }
  creator.disconnect();
}

}  // namespace

void scenarioFomAttributeNaCompanionCompletion(Options const& options,
                                                rti::CallbackModel model) {
  runAttributeNaCompanions(options, model, false);
}

void scenarioFomAttributeNaCompanionCompletionContract(
    Options const& options, rti::CallbackModel model) {
  scenarioFomAttributeNaCompanionCompletion(options, model);
}

void scenarioFomAttributeNaCompanionRejections(Options const& options,
                                                rti::CallbackModel model) {
  runAttributeNaCompanions(options, model, true);
}

void scenarioFomAttributeNaCompanionRejectionsContract(
    Options const& options, rti::CallbackModel model) {
  scenarioFomAttributeNaCompanionRejections(options, model);
}

}  // namespace hla_rti_cpp_tck::detail
