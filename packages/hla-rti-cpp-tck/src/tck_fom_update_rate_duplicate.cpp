#include "tck_fom_module_scenarios.hpp"

namespace hla_rti_cpp_tck::detail {
namespace {

std::filesystem::path const& requiredFom(Options const& options,
                                         std::string const& name) {
  auto const found = options.scenarioFoms.find(name);
  require(found != options.scenarioFoms.end(),
          "Update-rate duplicate requires adapter-supplied FOM modules");
  return found->second;
}

void create(Session& creator, std::wstring const& federation,
            Options const& options, std::vector<std::wstring> const& modules) {
  creator.rtiAmbassador().createFederationExecution(
      federation, modules, options.logicalTimeImplementationName);
}

void runUpdateRateDuplicate(Options const& options,
                            rti::CallbackModel model, bool noteConflict) {
  require(!options.logicalTimeImplementationName.empty(),
          "Update-rate duplicate requires adapter-supplied logical time");
  auto const& base = requiredFom(options, "update-rate-duplicate-base");
  auto const& second = requiredFom(options, noteConflict
      ? "update-rate-duplicate-note-conflict"
      : "update-rate-duplicate-equal");
  Session creator(options, model, "update-rate-duplicate-creator");
  Session member(options, model, "update-rate-duplicate-member");
  auto const federation = federationName(options, noteConflict
      ? "fom-update-rate-note-conflict" : "fom-update-rate-duplicate-equal");
  creator.connect();
  member.connect();

  for (auto const& module : {base.wstring(), second.wstring()}) {
    create(creator, federation, options, {module});
    creator.rtiAmbassador().destroyFederationExecution(federation);
  }
  if (noteConflict) {
    requireException(
        [&] { create(creator, federation, options,
                     {base.wstring(), second.wstring()}); },
        L"InconsistentFOM", "different note references on one update rate");
    create(creator, federation, options, {base.wstring()});
    creator.rtiAmbassador().destroyFederationExecution(federation);
  } else {
    create(creator, federation, options, {base.wstring(), second.wstring()});
    member.join(options.memberFederateName, options.federateType, federation);
    require(member.rtiAmbassador().getUpdateRateValue(L"TckMergeRate") == 30.0,
            "Equivalent update-rate declaration was not exposed by public lookup");
    member.resign(rti::NO_ACTION);
    creator.rtiAmbassador().destroyFederationExecution(federation);
  }
  member.disconnect();
  creator.disconnect();
}

}  // namespace

void scenarioFomUpdateRateDuplicateEqual(Options const& options,
                                         rti::CallbackModel model) {
  runUpdateRateDuplicate(options, model, false);
}

void scenarioFomUpdateRateDuplicateEqualContract(Options const& options,
                                                 rti::CallbackModel model) {
  scenarioFomUpdateRateDuplicateEqual(options, model);
}

void scenarioFomUpdateRateDuplicateNoteConflict(Options const& options,
                                                rti::CallbackModel model) {
  runUpdateRateDuplicate(options, model, true);
}

void scenarioFomUpdateRateDuplicateNoteConflictContract(
    Options const& options, rti::CallbackModel model) {
  scenarioFomUpdateRateDuplicateNoteConflict(options, model);
}

}  // namespace hla_rti_cpp_tck::detail
