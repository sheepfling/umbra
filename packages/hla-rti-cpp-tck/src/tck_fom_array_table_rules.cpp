#include "tck_fom_module_scenarios.hpp"

namespace hla_rti_cpp_tck::detail {
namespace {

std::filesystem::path const& requiredArrayFom(Options const& options,
                                              std::string const& key,
                                              std::string const& rule) {
  auto const found = options.scenarioFoms.find(key);
  require(found != options.scenarioFoms.end(),
          rule + " check needs adapter-supplied control and invalid FOMs");
  return found->second;
}

void verifyArrayFomRejection(Options const& options,
                             rti::CallbackModel model,
                             std::string const& validKey,
                             std::string const& invalidKey,
                             std::string const& federationSuffix,
                             std::string const& creatorName,
                             std::string const& rule,
                             std::string const& failureMessage) {
  require(!options.logicalTimeImplementationName.empty(),
          rule + " check needs adapter-supplied logical time");
  auto const& valid = requiredArrayFom(options, validKey, rule);
  auto const& invalid = requiredArrayFom(options, invalidKey, rule);
  Session creator(options, model, creatorName);
  auto const federation = federationName(options, federationSuffix);
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
  require(rejectedAsInconsistentFom, failureMessage);
}

}  // namespace

void scenarioFomArrayDataTypeNameConvention(Options const& options,
                                            rti::CallbackModel model) {
  verifyArrayFomRejection(
      options,
      model,
      "array-data-type-valid-name",
      "array-data-type-invalid-name",
      "fom-array-data-type-name",
      "array-data-type-name-creator",
      "Array-data-type name",
      "an array data type name containing a period was not rejected as InconsistentFOM");
}

void scenarioFomArrayDataTypeNameConventionContract(Options const& options,
                                                   rti::CallbackModel model) {
  scenarioFomArrayDataTypeNameConvention(options, model);
}

void scenarioFomArrayElementTypeReference(Options const& options,
                                          rti::CallbackModel model) {
  verifyArrayFomRejection(
      options,
      model,
      "array-element-type-valid",
      "array-element-type-invalid",
      "fom-array-element-type-reference",
      "array-element-type-reference-creator",
      "Array element data type",
      "an unresolved array Element Type was not rejected as InconsistentFOM");
}

void scenarioFomArrayElementTypeReferenceContract(Options const& options,
                                                  rti::CallbackModel model) {
  scenarioFomArrayElementTypeReference(options, model);
}

}  // namespace hla_rti_cpp_tck::detail
