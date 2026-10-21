#include "tck_fom_module_scenarios.hpp"

namespace hla_rti_cpp_tck::detail {
namespace {

std::filesystem::path const& requiredFom(Options const& options,
                                         std::string const& name) {
  auto const found = options.scenarioFoms.find(name);
  require(found != options.scenarioFoms.end(),
          "Extendable-variant HLAother test requires adapter-supplied FOMs");
  return found->second;
}

void create(Session& creator, std::wstring const& federation,
            Options const& options, std::filesystem::path const& module) {
  creator.rtiAmbassador().createFederationExecution(
      federation, std::vector<std::wstring>{module.wstring()},
      options.logicalTimeImplementationName);
}

}  // namespace

void scenarioFomExtendableVariantHlaOther(Options const& options,
                                           rti::CallbackModel model) {
  require(!options.logicalTimeImplementationName.empty(),
          "Extendable-variant HLAother test requires adapter-supplied logical time");
  auto const& valid = requiredFom(options, "extendable-hlaother-valid");
  auto const& invalid = requiredFom(options, "extendable-hlaother-invalid");
  Session creator(options, model, "extendable-hlaother-creator");
  auto const federation = federationName(options, "fom-extendable-hlaother");
  creator.connect();

  create(creator, federation, options, valid);
  creator.rtiAmbassador().destroyFederationExecution(federation);
  requireException(
      [&] { create(creator, federation, options, invalid); },
      L"InconsistentFOM",
      "HLAother alternative in an HLAextendableVariantRecord FOM module");
  create(creator, federation, options, valid);
  creator.rtiAmbassador().destroyFederationExecution(federation);
  creator.disconnect();
}

void scenarioFomExtendableVariantHlaOtherContract(Options const& options,
                                                   rti::CallbackModel model) {
  scenarioFomExtendableVariantHlaOther(options, model);
}

}  // namespace hla_rti_cpp_tck::detail
