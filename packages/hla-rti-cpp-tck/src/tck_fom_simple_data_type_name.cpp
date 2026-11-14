#include "tck_fom_module_scenarios.hpp"

namespace hla_rti_cpp_tck::detail {
namespace {

std::filesystem::path const& simpleDataTypeNameFom(Options const& options,
                                                  std::string const& key) {
  auto const found = options.scenarioFoms.find(key);
  require(found != options.scenarioFoms.end(),
          "Simple-data-type name check needs adapter-supplied control and invalid FOMs");
  return found->second;
}

}  // namespace

void scenarioFomSimpleDataTypeNameConvention(Options const& options,
                                             rti::CallbackModel model) {
  require(!options.logicalTimeImplementationName.empty(),
          "Simple-data-type name check needs adapter-supplied logical time");
  auto const& valid = simpleDataTypeNameFom(options, "simple-data-type-valid-name");
  auto const& invalid = simpleDataTypeNameFom(options, "simple-data-type-invalid-name");
  auto const& reservedPrefix = simpleDataTypeNameFom(
      options, "simple-data-type-reserved-prefix-name");
  auto const& reservedNa = simpleDataTypeNameFom(
      options, "simple-data-type-reserved-na-name");
  Session creator(options, model, "simple-data-type-name-creator");
  auto const federation = federationName(options, "fom-simple-data-type-name");
  creator.connect();
  creator.rtiAmbassador().createFederationExecution(
      federation,
      std::vector<std::wstring>{valid.wstring()},
      options.logicalTimeImplementationName);
  creator.rtiAmbassador().destroyFederationExecution(federation);

  auto requireRejectedAsInconsistentFom = [&](std::filesystem::path const& fom,
                                               char const* failure) {
    bool rejectedAsInconsistentFom = false;
    bool federationWasCreated = false;
    try {
      creator.rtiAmbassador().createFederationExecution(
          federation,
          std::vector<std::wstring>{fom.wstring()},
          options.logicalTimeImplementationName);
      federationWasCreated = true;
    } catch (rti::InconsistentFOM const&) {
      rejectedAsInconsistentFom = true;
    }
    if (federationWasCreated) {
      creator.rtiAmbassador().destroyFederationExecution(federation);
    }
    require(rejectedAsInconsistentFom, failure);
  };
  requireRejectedAsInconsistentFom(
      invalid,
      "a simple data type name containing a period was not rejected as InconsistentFOM");
  requireRejectedAsInconsistentFom(
      reservedPrefix,
      "a simple data type name beginning with a mixed-case HLA prefix was not rejected as InconsistentFOM");
  requireRejectedAsInconsistentFom(
      reservedNa,
      "a simple data type name matching the mixed-case reserved NA marker was not rejected as InconsistentFOM");
  creator.disconnect();
}

void scenarioFomSimpleDataTypeNameConventionContract(
    Options const& options, rti::CallbackModel model) {
  scenarioFomSimpleDataTypeNameConvention(options, model);
}

}  // namespace hla_rti_cpp_tck::detail
