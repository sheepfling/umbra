#include "tck_fom_module_scenarios.hpp"

namespace hla_rti_cpp_tck::detail {
namespace {

std::filesystem::path const& requiredFom(Options const& options,
                                         std::string const& name) {
  auto const found = options.scenarioFoms.find(name);
  require(found != options.scenarioFoms.end(),
          "Named variant-alternative merge requires two adapter-supplied FOMs");
  return found->second;
}

}  // namespace

void scenarioFomNamedVariantAlternativeMerge(Options const& options,
                                              rti::CallbackModel model) {
  require(!options.logicalTimeImplementationName.empty(),
          "Named variant-alternative merge requires adapter-supplied logical time");
  auto const& first = requiredFom(options, "named-variant-alternative-first");
  auto const& second = requiredFom(options, "named-variant-alternative-second");
  Session creator(options, model, "named-variant-alternative-creator");
  auto const federation = federationName(options, "fom-named-variant-alternative");
  creator.connect();

  // The modules are independently valid. Their combined acceptance is the
  // public-API witness for disjoint enumerators on one named alternative.
  for (auto const& modules : {
           std::vector<std::wstring>{first.wstring()},
           std::vector<std::wstring>{second.wstring()},
           std::vector<std::wstring>{first.wstring(), second.wstring()}}) {
    creator.rtiAmbassador().createFederationExecution(
        federation, modules, options.logicalTimeImplementationName);
    creator.rtiAmbassador().destroyFederationExecution(federation);
  }
  creator.disconnect();
}

void scenarioFomNamedVariantAlternativeMergeContract(Options const& options,
                                                      rti::CallbackModel model) {
  scenarioFomNamedVariantAlternativeMerge(options, model);
}

}  // namespace hla_rti_cpp_tck::detail
