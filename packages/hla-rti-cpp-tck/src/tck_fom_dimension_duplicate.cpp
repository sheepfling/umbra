#include "tck_fom_module_scenarios.hpp"

namespace hla_rti_cpp_tck::detail {
namespace {

std::filesystem::path const& requiredFom(Options const& options,
                                         std::string const& name) {
  auto const found = options.scenarioFoms.find(name);
  require(found != options.scenarioFoms.end(),
          "Dimension duplicate requires adapter-supplied FOM modules");
  return found->second;
}

void create(Session& creator, std::wstring const& federation,
            Options const& options, std::vector<std::wstring> const& modules) {
  creator.rtiAmbassador().createFederationExecution(
      federation, modules, options.logicalTimeImplementationName);
}

void runDimensionDuplicate(Options const& options, rti::CallbackModel model,
                           bool conflictingUpperBound) {
  require(!options.logicalTimeImplementationName.empty(),
          "Dimension duplicate requires adapter-supplied logical time");
  auto const& base = requiredFom(options, "dimension-duplicate-base");
  auto const& second = requiredFom(options, conflictingUpperBound
      ? "dimension-duplicate-conflicting-upper-bound"
      : "dimension-duplicate-equal-unique");
  Session creator(options, model, "dimension-duplicate-creator");
  Session member(options, model, "dimension-duplicate-member");
  auto const federation = federationName(options, conflictingUpperBound
      ? "fom-dimension-duplicate-conflict" : "fom-dimension-duplicate-union");
  creator.connect();
  member.connect();

  // Independently accepted modules isolate composition from schema failures.
  for (auto const& module : {base.wstring(), second.wstring()}) {
    create(creator, federation, options, {module});
    creator.rtiAmbassador().destroyFederationExecution(federation);
  }

  if (conflictingUpperBound) {
    requireException(
        [&] { create(creator, federation, options,
                     {base.wstring(), second.wstring()}); },
        L"InconsistentFOM", "conflicting upperBound on one named dimension");
    create(creator, federation, options, {base.wstring()});
    creator.rtiAmbassador().destroyFederationExecution(federation);
  } else {
    create(creator, federation, options,
           {base.wstring(), second.wstring()});
    member.join(options.memberFederateName, options.federateType, federation);
    auto& rti = member.rtiAmbassador();
    auto const duplicate = rti.getDimensionHandle(L"TckDuplicateDimension");
    auto const unique = rti.getDimensionHandle(L"TckUniqueDimension");
    require(duplicate.isValid() && unique.isValid() && duplicate != unique &&
                rti.getDimensionName(duplicate) == L"TckDuplicateDimension" &&
                rti.getDimensionName(unique) == L"TckUniqueDimension" &&
                rti.getDimensionUpperBound(duplicate) == 100U &&
                rti.getDimensionUpperBound(unique) == 200U,
            "Equivalent duplicate or unique dimension did not survive composition");
    member.resign(rti::NO_ACTION);
    creator.rtiAmbassador().destroyFederationExecution(federation);
  }
  member.disconnect();
  creator.disconnect();
}

}  // namespace

void scenarioFomDimensionDuplicateUnion(Options const& options,
                                         rti::CallbackModel model) {
  runDimensionDuplicate(options, model, false);
}

void scenarioFomDimensionDuplicateUnionContract(Options const& options,
                                                 rti::CallbackModel model) {
  scenarioFomDimensionDuplicateUnion(options, model);
}

void scenarioFomDimensionDuplicateConflict(Options const& options,
                                            rti::CallbackModel model) {
  runDimensionDuplicate(options, model, true);
}

void scenarioFomDimensionDuplicateConflictContract(Options const& options,
                                                    rti::CallbackModel model) {
  scenarioFomDimensionDuplicateConflict(options, model);
}

}  // namespace hla_rti_cpp_tck::detail
