#include "tck_fom_module_scenarios.hpp"

namespace hla_rti_cpp_tck::detail {

void scenarioFomRecordMemberCategory(Options const& options,
                                     rti::CallbackModel model) {
  require(!options.logicalTimeImplementationName.empty(),
          "Record member type requires adapter-supplied logical time");
  auto const valid = options.scenarioFoms.find("record-member-valid");
  auto const fieldTypes = options.scenarioFoms.find("record-member-field-types");
  auto const encodingMismatch =
      options.scenarioFoms.find("record-member-encoding-mismatch");
  auto const omitted = options.scenarioFoms.find("record-member-omitted");
  auto const basicField = options.scenarioFoms.find("record-member-basic-field");
  auto const basicAlternative = options.scenarioFoms.find("record-member-basic-alternative");
  auto const reservedHlaName =
      options.scenarioFoms.find("record-member-reserved-hla-name");
  auto const reservedNaName =
      options.scenarioFoms.find("record-member-reserved-na-name");
  auto const reservedHlaFieldName =
      options.scenarioFoms.find("record-member-reserved-hla-field-name");
  auto const reservedNaFieldName =
      options.scenarioFoms.find("record-member-reserved-na-field-name");
  require(valid != options.scenarioFoms.end() &&
              fieldTypes != options.scenarioFoms.end() &&
              encodingMismatch != options.scenarioFoms.end() &&
              omitted != options.scenarioFoms.end() &&
              basicField != options.scenarioFoms.end() &&
              basicAlternative != options.scenarioFoms.end() &&
              reservedHlaName != options.scenarioFoms.end() &&
              reservedNaName != options.scenarioFoms.end() &&
              reservedHlaFieldName != options.scenarioFoms.end() &&
              reservedNaFieldName != options.scenarioFoms.end(),
          "Record member/type/name tests require adapter-supplied FOM modules");

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
  create(fieldTypes->second);
  rti.destroyFederationExecution(federation);
  create(omitted->second);
  rti.destroyFederationExecution(federation);

  requireException([&] { create(basicField->second); }, L"InconsistentFOM",
                   "raw basic-data fixed-record Field Type");
  requireException([&] { create(basicAlternative->second); }, L"InconsistentFOM",
                   "raw basic-data variant-record Alternative Type");
  requireException([&] { create(encodingMismatch->second); }, L"InconsistentFOM",
                   "predefined variant-record encoding assigned to a fixed record");
  requireException([&] { create(reservedHlaName->second); }, L"InconsistentFOM",
                   "case-insensitive HLA-reserved fixed-record Record Name");
  requireException([&] { create(reservedNaName->second); }, L"InconsistentFOM",
                   "case-insensitive NA-marker fixed-record Record Name");
  requireException([&] { create(reservedHlaFieldName->second); }, L"InconsistentFOM",
                   "case-insensitive HLA-reserved fixed-record Field Name");
  requireException([&] { create(reservedNaFieldName->second); }, L"InconsistentFOM",
                   "case-insensitive NA-marker fixed-record Field Name");
  create(valid->second);
  rti.destroyFederationExecution(federation);
  creator.disconnect();
}

void scenarioFomRecordMemberCategoryContract(Options const& options,
                                             rti::CallbackModel model) {
  scenarioFomRecordMemberCategory(options, model);
}

}  // namespace hla_rti_cpp_tck::detail
