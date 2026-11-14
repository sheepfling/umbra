#include "tck_fom_module_scenarios.hpp"

namespace hla_rti_cpp_tck::detail {
namespace {

std::filesystem::path const& invalidReferenceDataTypeNameFom(Options const& options) {
  auto const found = options.scenarioFoms.find("reference-data-type-invalid-name");
  require(found != options.scenarioFoms.end(),
          "Reference-data-type name check needs an adapter-supplied FOM");
  return found->second;
}

std::filesystem::path const& validReferenceDataTypeNameFom(Options const& options) {
  auto const found = options.scenarioFoms.find("reference-data-type-valid-name");
  require(found != options.scenarioFoms.end(),
          "Reference-data-type name check needs a valid adapter-supplied control FOM");
  return found->second;
}

}  // namespace

void scenarioFomReferenceDataTypeNameConvention(Options const& options,
                                                rti::CallbackModel model) {
  require(!options.logicalTimeImplementationName.empty(),
          "Reference-data-type name check needs adapter-supplied logical time");
  auto const& valid = validReferenceDataTypeNameFom(options);
  auto const& invalid = invalidReferenceDataTypeNameFom(options);
  Session creator(options, model, "reference-data-type-name-creator");
  auto const federation = federationName(options, "fom-reference-data-type-name");
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
          "a reference data type name containing a period was not rejected as InconsistentFOM");
}

void scenarioFomReferenceDataTypeNameConventionContract(
    Options const& options, rti::CallbackModel model) {
  scenarioFomReferenceDataTypeNameConvention(options, model);
}

}  // namespace hla_rti_cpp_tck::detail
