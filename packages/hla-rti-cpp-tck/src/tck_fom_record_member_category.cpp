#include "tck_fom_module_scenarios.hpp"

namespace hla_rti_cpp_tck::detail {

void scenarioFomRecordMemberCategory(Options const& options,
                                     rti::CallbackModel model) {
  require(!options.logicalTimeImplementationName.empty(),
          "Record member type requires adapter-supplied logical time");
  auto const valid = options.scenarioFoms.find("record-member-valid");
  auto const omitted = options.scenarioFoms.find("record-member-omitted");
  auto const basicField = options.scenarioFoms.find("record-member-basic-field");
  auto const basicAlternative = options.scenarioFoms.find("record-member-basic-alternative");
  require(valid != options.scenarioFoms.end() &&
              omitted != options.scenarioFoms.end() &&
              basicField != options.scenarioFoms.end() &&
              basicAlternative != options.scenarioFoms.end(),
          "Record member type requires adapter-supplied FOM modules");

  Session creator(options, model, "record-member-creator");
  auto const federation = federationName(options, "fom-record-member-category");
  creator.connect();
  auto& rti = creator.rtiAmbassador();
  auto const create = [&](std::filesystem::path const& module) {
    rti.createFederationExecution(federation, module.wstring(),
                                  options.logicalTimeImplementationName);
  };
  create(valid->second);
  rti.destroyFederationExecution(federation);
  create(omitted->second);
  rti.destroyFederationExecution(federation);

  requireException([&] { create(basicField->second); }, L"InconsistentFOM",
                   "raw basic-data fixed-record Field Type");
  requireException([&] { create(basicAlternative->second); }, L"InconsistentFOM",
                   "raw basic-data variant-record Alternative Type");
  create(valid->second);
  rti.destroyFederationExecution(federation);
  creator.disconnect();
}

void scenarioFomRecordMemberCategoryContract(Options const& options,
                                             rti::CallbackModel model) {
  scenarioFomRecordMemberCategory(options, model);
}

}  // namespace hla_rti_cpp_tck::detail
