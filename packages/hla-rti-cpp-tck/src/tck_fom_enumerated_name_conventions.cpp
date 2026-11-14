#include "tck_fom_module_scenarios.hpp"

namespace hla_rti_cpp_tck::detail {
namespace {

std::filesystem::path const& namedRowFom(Options const& options,
                                          std::string const& key,
                                          std::string const& nameKind) {
  auto const found = options.scenarioFoms.find(key);
  require(found != options.scenarioFoms.end(),
          nameKind + " check needs adapter-supplied control and invalid FOMs");
  return found->second;
}

void scenarioFomNamedRowConvention(Options const& options,
                                   rti::CallbackModel model,
                                   std::string const& validKey,
                                   std::string const& invalidKey,
                                   std::string const& federationSuffix,
                                   std::string const& creatorName,
                                   std::string const& nameKind) {
  require(!options.logicalTimeImplementationName.empty(),
          nameKind + " check needs adapter-supplied logical time");
  auto const& valid = namedRowFom(options, validKey, nameKind);
  auto const& invalid = namedRowFom(options, invalidKey, nameKind);
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
  require(rejectedAsInconsistentFom,
          nameKind + " containing a period was not rejected as InconsistentFOM");
}

}  // namespace

void scenarioFomEnumeratedDataTypeNameConvention(Options const& options,
                                                 rti::CallbackModel model) {
  scenarioFomNamedRowConvention(
      options,
      model,
      "enumerated-data-type-valid-name",
      "enumerated-data-type-invalid-name",
      "fom-enumerated-data-type-name",
      "enumerated-data-type-name-creator",
      "Enumerated-data-type name");
}

void scenarioFomEnumeratedDataTypeNameConventionContract(
    Options const& options, rti::CallbackModel model) {
  scenarioFomEnumeratedDataTypeNameConvention(options, model);
}

void scenarioFomEnumeratorNameConvention(Options const& options,
                                         rti::CallbackModel model) {
  scenarioFomNamedRowConvention(
      options,
      model,
      "enumerator-valid-name",
      "enumerator-invalid-name",
      "fom-enumerator-name",
      "enumerator-name-creator",
      "Enumerator name");
}

void scenarioFomEnumeratorNameConventionContract(Options const& options,
                                                 rti::CallbackModel model) {
  scenarioFomEnumeratorNameConvention(options, model);
}

}  // namespace hla_rti_cpp_tck::detail
