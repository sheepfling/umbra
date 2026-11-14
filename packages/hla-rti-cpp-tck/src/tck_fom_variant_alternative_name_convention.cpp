#include "tck_fom_module_scenarios.hpp"

namespace hla_rti_cpp_tck::detail {
namespace {

std::filesystem::path const& variantAlternativeNameFom(Options const& options,
                                                       std::string const& key) {
  auto const found = options.scenarioFoms.find(key);
  require(found != options.scenarioFoms.end(),
          "Variant Alternative Name check needs adapter-supplied control and invalid FOMs");
  return found->second;
}

}  // namespace

void scenarioFomVariantAlternativeNameConvention(Options const& options,
                                                 rti::CallbackModel model) {
  require(!options.logicalTimeImplementationName.empty(),
          "Variant Alternative Name check needs adapter-supplied logical time");
  auto const& valid = variantAlternativeNameFom(options, "variant-alternative-valid-name");
  auto const& invalid = variantAlternativeNameFom(options, "variant-alternative-invalid-name");
  Session creator(options, model, "variant-alternative-name-creator");
  auto const federation = federationName(options, "fom-variant-alternative-name");
  creator.connect();
  creator.rtiAmbassador().createFederationExecution(
      federation,
      std::vector<std::wstring>{valid.wstring()},
      options.logicalTimeImplementationName);
  creator.rtiAmbassador().destroyFederationExecution(federation);

  bool rejectedAsInconsistentFom = false;
  bool federationWasCreated = false;
  try {
    creator.rtiAmbassador().createFederationExecution(
        federation,
        std::vector<std::wstring>{invalid.wstring()},
        options.logicalTimeImplementationName);
    federationWasCreated = true;
  } catch (rti::InconsistentFOM const&) {
    rejectedAsInconsistentFom = true;
  }
  if (federationWasCreated) {
    creator.rtiAmbassador().destroyFederationExecution(federation);
  }
  creator.disconnect();
  require(rejectedAsInconsistentFom,
          "a Variant Record Alternative Name containing a period was not rejected as InconsistentFOM");
}

void scenarioFomVariantAlternativeNameConventionContract(Options const& options,
                                                         rti::CallbackModel model) {
  scenarioFomVariantAlternativeNameConvention(options, model);
}

}  // namespace hla_rti_cpp_tck::detail
