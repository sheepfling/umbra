#include "tck_fom_module_scenarios.hpp"

namespace hla_rti_cpp_tck::detail {
namespace {

std::filesystem::path const& variantRecordEncodingFom(Options const& options,
                                                      std::string const& key) {
  auto const found = options.scenarioFoms.find(key);
  require(found != options.scenarioFoms.end(),
          "Variant Record Encoding check needs adapter-supplied control and invalid FOMs");
  return found->second;
}

}  // namespace

void scenarioFomVariantRecordEncoding(Options const& options,
                                      rti::CallbackModel model) {
  require(!options.logicalTimeImplementationName.empty(),
          "Variant Record Encoding check needs adapter-supplied logical time");
  auto const& valid = variantRecordEncodingFom(options, "variant-record-encoding-valid");
  auto const& invalid = variantRecordEncodingFom(options, "variant-record-encoding-invalid");
  Session creator(options, model, "variant-record-encoding-creator");
  auto const federation = federationName(options, "fom-variant-record-encoding");
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
          "a Variant Record using mismatched predefined HLAfixedRecord encoding was not rejected as InconsistentFOM");
}

void scenarioFomVariantRecordEncodingContract(Options const& options,
                                              rti::CallbackModel model) {
  scenarioFomVariantRecordEncoding(options, model);
}

}  // namespace hla_rti_cpp_tck::detail
