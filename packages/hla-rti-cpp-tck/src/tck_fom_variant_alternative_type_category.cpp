#include "tck_fom_module_scenarios.hpp"

namespace hla_rti_cpp_tck::detail {
namespace {

std::filesystem::path const& variantAlternativeTypeFom(Options const& options,
                                                       std::string const& key) {
  auto const found = options.scenarioFoms.find(key);
  require(found != options.scenarioFoms.end(),
          "Variant Alternative Type check needs adapter-supplied control and invalid FOMs");
  return found->second;
}

}  // namespace

void scenarioFomVariantAlternativeTypeCategory(Options const& options,
                                               rti::CallbackModel model) {
  require(!options.logicalTimeImplementationName.empty(),
          "Variant Alternative Type check needs adapter-supplied logical time");
  auto const& valid = variantAlternativeTypeFom(options, "variant-alternative-type-valid");
  auto const& invalid = variantAlternativeTypeFom(options, "variant-alternative-type-invalid");
  Session creator(options, model, "variant-alternative-type-creator");
  auto const federation = federationName(options, "fom-variant-alternative-type");
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
          "a basic data type was accepted as a Variant Record Alternative Type");
}

void scenarioFomVariantAlternativeTypeCategoryContract(Options const& options,
                                                       rti::CallbackModel model) {
  scenarioFomVariantAlternativeTypeCategory(options, model);
}

}  // namespace hla_rti_cpp_tck::detail
