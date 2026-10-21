#include "tck_fom_module_scenarios.hpp"

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
    create(creator, federation, options,
           {base.wstring(), second.wstring()});
    creator.rtiAmbassador().destroyFederationExecution(federation);
  }
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

}  // namespace hla_rti_cpp_tck::detail
