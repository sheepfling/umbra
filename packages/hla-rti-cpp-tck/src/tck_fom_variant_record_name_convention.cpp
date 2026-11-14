#include "tck_fom_module_scenarios.hpp"

namespace hla_rti_cpp_tck::detail {
namespace {

std::filesystem::path const& variantRecordNameFom(Options const& options,
                                                  std::string const& key) {
  auto const found = options.scenarioFoms.find(key);
  require(found != options.scenarioFoms.end(),
          "Variant Record Name check needs adapter-supplied control and invalid FOMs");
  return found->second;
}

}  // namespace

void scenarioFomVariantRecordNameConvention(Options const& options,
                                           rti::CallbackModel model) {
  require(!options.logicalTimeImplementationName.empty(),
          "Variant Record Name check needs adapter-supplied logical time");
  auto const& valid = variantRecordNameFom(options, "variant-record-valid-name");
  auto const& invalid = variantRecordNameFom(options, "variant-record-invalid-name");
  Session creator(options, model, "variant-record-name-creator");
  auto const federation = federationName(options, "fom-variant-record-name");
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
          "a Variant Record Name containing a period was not rejected as InconsistentFOM");
}

void scenarioFomVariantRecordNameConventionContract(Options const& options,
                                                    rti::CallbackModel model) {
  scenarioFomVariantRecordNameConvention(options, model);
}

}  // namespace hla_rti_cpp_tck::detail
