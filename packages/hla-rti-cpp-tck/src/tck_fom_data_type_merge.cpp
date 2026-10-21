#include "tck_fom_module_scenarios.hpp"

namespace hla_rti_cpp_tck::detail {
namespace {

void requireMergeModules(Options const& options) {
  require(!options.dataTypeMergeConsumerFom.empty() &&
              !options.dataTypeMergeProviderFom.empty(),
          "Annex C.3 data-type merge requires adapter-supplied consumer and provider FOM modules");
  require(!options.logicalTimeImplementationName.empty(),
          "Annex C.3 data-type merge requires an adapter-supplied logical-time implementation");
}

void create(Session& session, std::wstring const& federation,
            Options const& options, std::vector<std::filesystem::path> const& modules) {
  std::vector<std::wstring> moduleNames;
  moduleNames.reserve(modules.size());
  for (auto const& module : modules) {
    moduleNames.push_back(module.wstring());
  }
  session.rtiAmbassador().createFederationExecution(
      federation, moduleNames, options.logicalTimeImplementationName);
}

}  // namespace

void scenarioFomDataTypeForwardReference(Options const& options,
                                          rti::CallbackModel model) {
  requireMergeModules(options);
  Session creator(options, model, "fom-data-type-creator");
  auto const federation = federationName(options, "fom-data-type-forward-reference");
  creator.connect();

  requireException(
      [&] { create(creator, federation, options,
                   {options.dataTypeMergeConsumerFom}); },
      L"InconsistentFOM",
      "creating a federation with an unresolved array element data type");
  create(creator, federation, options,
         {options.dataTypeMergeConsumerFom, options.dataTypeMergeProviderFom});
  creator.rtiAmbassador().destroyFederationExecution(federation);
  creator.disconnect();
}

void scenarioFomDataTypeForwardReferenceContract(Options const& options,
                                                  rti::CallbackModel model) {
  scenarioFomDataTypeForwardReference(options, model);
}

void scenarioFomDataTypeConflictingDefinition(Options const& options,
                                               rti::CallbackModel model) {
  requireMergeModules(options);
  require(!options.dataTypeMergeConflictingProviderFom.empty(),
          "Annex C.3 conflicting-definition test requires an adapter-supplied conflicting provider FOM");
  Session creator(options, model, "fom-data-type-creator");
  auto const federation = federationName(options, "fom-data-type-conflict");
  creator.connect();

  // Each provider composes with the consumer separately. Only their
  // incompatible definitions of one named type make the combined set fail.
  create(creator, federation, options,
         {options.dataTypeMergeConsumerFom, options.dataTypeMergeProviderFom});
  creator.rtiAmbassador().destroyFederationExecution(federation);
  create(creator, federation, options,
         {options.dataTypeMergeConsumerFom,
          options.dataTypeMergeConflictingProviderFom});
  creator.rtiAmbassador().destroyFederationExecution(federation);
  requireException(
      [&] { create(creator, federation, options,
                   {options.dataTypeMergeConsumerFom,
                    options.dataTypeMergeProviderFom,
                    options.dataTypeMergeConflictingProviderFom}); },
      L"InconsistentFOM",
      "creating a federation with conflicting named data-type definitions");
  create(creator, federation, options,
         {options.dataTypeMergeConsumerFom, options.dataTypeMergeProviderFom});
  creator.rtiAmbassador().destroyFederationExecution(federation);
  creator.disconnect();
}

void scenarioFomDataTypeConflictingDefinitionContract(Options const& options,
                                                       rti::CallbackModel model) {
  scenarioFomDataTypeConflictingDefinition(options, model);
}

}  // namespace hla_rti_cpp_tck::detail
