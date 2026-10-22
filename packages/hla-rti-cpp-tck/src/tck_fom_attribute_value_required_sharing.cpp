#include "tck_fom_module_scenarios.hpp"

namespace hla_rti_cpp_tck::detail {
namespace {

std::filesystem::path const& requiredFom(Options const& options,
                                         std::string const& key) {
  auto const found = options.scenarioFoms.find(key);
  require(found != options.scenarioFoms.end(),
          "Unshared-attribute Value Required checks need adapter-supplied FOMs");
  return found->second;
}

}  // namespace

void scenarioFomAttributeValueRequiredSharing(Options const& options,
                                              rti::CallbackModel model) {
  require(!options.logicalTimeImplementationName.empty(),
          "Unshared-attribute Value Required checks need adapter-supplied logical time");
  auto const& valid = requiredFom(options, "attribute-neither-value-required-valid");
  auto const& invalid = requiredFom(options, "attribute-neither-value-required-true");
  Session creator(options, model, "attribute-value-required-sharing-creator");
  auto const federation = federationName(options, "fom-attribute-value-required-sharing");
  creator.connect();
  auto const create = [&](std::filesystem::path const& module) {
    creator.rtiAmbassador().createFederationExecution(
        federation, std::vector<std::wstring>{module.wstring()},
        options.logicalTimeImplementationName);
  };
  create(valid);
  creator.rtiAmbassador().destroyFederationExecution(federation);
  requireException([&] { create(invalid); }, L"InconsistentFOM",
                   "sharing=Neither with valueRequired=true");
  create(valid);
  creator.rtiAmbassador().destroyFederationExecution(federation);
  creator.disconnect();
}

void scenarioFomAttributeValueRequiredSharingContract(
    Options const& options, rti::CallbackModel model) {
  scenarioFomAttributeValueRequiredSharing(options, model);
}

}  // namespace hla_rti_cpp_tck::detail
