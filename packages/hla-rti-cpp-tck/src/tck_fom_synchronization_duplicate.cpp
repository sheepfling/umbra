#include "tck_fom_module_scenarios.hpp"
#include "tck_federation_mom_content_scenarios.hpp"

namespace hla_rti_cpp_tck::detail {
namespace {

std::filesystem::path const& requiredFom(Options const& options,
                                         std::string const& name) {
  auto const found = options.scenarioFoms.find(name);
  require(found != options.scenarioFoms.end(),
          "Synchronization duplicate requires adapter-supplied FOM modules");
  return found->second;
}

void create(Session& creator, std::wstring const& federation,
            Options const& options, std::vector<std::wstring> const& modules) {
  creator.rtiAmbassador().createFederationExecution(
      federation, modules, options.logicalTimeImplementationName);
}

void runSynchronizationDuplicate(Options const& options,
                                 rti::CallbackModel model, bool noteConflict) {
  require(!options.logicalTimeImplementationName.empty(),
          "Synchronization duplicate requires adapter-supplied logical time");
  auto const& base = requiredFom(options, "synchronization-duplicate-base");
  auto const& second = requiredFom(options, noteConflict
      ? "synchronization-duplicate-note-conflict"
      : "synchronization-duplicate-equal");
  Session creator(options, model, "synchronization-duplicate-creator");
  auto const federation = federationName(options, noteConflict
      ? "fom-synchronization-note-conflict" : "fom-synchronization-duplicate-equal");
  creator.connect();

  // A failed composition must not be confused with an invalid input module.
  for (auto const& module : {base.wstring(), second.wstring()}) {
    create(creator, federation, options, {module});
    creator.rtiAmbassador().destroyFederationExecution(federation);
  }
  if (noteConflict) {
    requireException(
        [&] { create(creator, federation, options,
                     {base.wstring(), second.wstring()}); },
        L"InconsistentFOM", "different note references on one synchronization point");
    create(creator, federation, options, {base.wstring()});
    creator.rtiAmbassador().destroyFederationExecution(federation);
  } else {
    auto const currentFdd = composedCurrentFdd(
        options, model, {base.wstring(), second.wstring()},
        "fom-sync-merge-current-fdd");
    require(currentFdd.find(L"TckMergeReady") != std::wstring::npos,
            "the merged current FDD omitted the synchronization declaration");
  }
  creator.disconnect();
}

void runSynchronizationTableValidation(Options const& options,
                                       rti::CallbackModel model) {
  require(!options.logicalTimeImplementationName.empty(),
          "Synchronization table validation requires adapter-supplied logical time");
  auto const& valid = requiredFom(options, "sync-cap-na");
  auto const& invalidCapability = requiredFom(options, "sync-cap-action");
  auto const& invalidLabel = requiredFom(options, "sync-label-invalid");
  Session creator(options, model, "synchronization-table-creator");
  auto const federation = federationName(options, "fom-synchronization-table");
  creator.connect();

  create(creator, federation, options, {valid.wstring()});
  creator.rtiAmbassador().destroyFederationExecution(federation);
  requireException(
      [&] { create(creator, federation, options, {invalidCapability.wstring()}); },
      L"InconsistentFOM",
      "a FOM synchronization-point Capability field other than NA");
  requireException(
      [&] { create(creator, federation, options, {invalidLabel.wstring()}); },
      L"InconsistentFOM",
      "a user-defined synchronization-point label using the reserved HLA prefix");

  // Failed table checks must leave federation-name reuse available.
  create(creator, federation, options, {valid.wstring()});
  creator.rtiAmbassador().destroyFederationExecution(federation);
  creator.disconnect();
}

}  // namespace

void scenarioFomSynchronizationDuplicateEqual(Options const& options,
                                               rti::CallbackModel model) {
  runSynchronizationDuplicate(options, model, false);
}

void scenarioFomSynchronizationDuplicateEqualContract(Options const& options,
                                                       rti::CallbackModel model) {
  scenarioFomSynchronizationDuplicateEqual(options, model);
}

void scenarioFomSynchronizationDuplicateNoteConflict(Options const& options,
                                                      rti::CallbackModel model) {
  runSynchronizationDuplicate(options, model, true);
}

void scenarioFomSynchronizationDuplicateNoteConflictContract(
    Options const& options, rti::CallbackModel model) {
  scenarioFomSynchronizationDuplicateNoteConflict(options, model);
}

void scenarioFomSynchronizationTableValidation(
    Options const& options, rti::CallbackModel model) {
  runSynchronizationTableValidation(options, model);
}

void scenarioFomSynchronizationTableValidationContract(
    Options const& options, rti::CallbackModel model) {
  scenarioFomSynchronizationTableValidation(options, model);
}

}  // namespace hla_rti_cpp_tck::detail
