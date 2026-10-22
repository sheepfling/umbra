#include "tck_fom_module_scenarios.hpp"

namespace hla_rti_cpp_tck::detail {
namespace {

std::filesystem::path const& requiredFom(Options const& options,
                                         std::string const& name) {
  auto const found = options.scenarioFoms.find(name);
  require(found != options.scenarioFoms.end(),
          "Dimension FOM testing requires adapter-supplied modules");
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

void scenarioFomDimensionInputType(Options const& options,
                                   rti::CallbackModel model) {
  require(!options.mimFom.empty() &&
              !options.logicalTimeImplementationName.empty(),
          "Dimension input-type testing requires adapter-supplied MIM and logical time");
  auto const& rawBasic = requiredFom(options, "dimension-input-raw-basic");
  auto const& na = requiredFom(options, "dimension-input-na");
  auto const& tableType = requiredFom(options, "dimension-input-table-type");
  Session creator(options, model, "dimension-input-type-creator");
  Session member(options, model, "dimension-input-type-member");
  auto const federation = federationName(options, "fom-dimension-input-type");
  creator.connect();
  member.connect();
  auto createWithMim = [&](std::filesystem::path const& module) {
    creator.rtiAmbassador().createFederationExecutionWithMIM(
        federation, std::vector<std::wstring>{module.wstring()},
        options.mimFom.wstring(), options.logicalTimeImplementationName);
  };
  requireException([&] { createWithMim(rawBasic); }, L"InconsistentFOM",
                   "a raw basic representation used as a dimension input type");

  auto const checkAccepted = [&](std::filesystem::path const& module,
                                 std::wstring const& dimensionName) {
    // Reusing the rejected federation name also checks failed-create cleanup.
    createWithMim(module);
    member.join(options.memberFederateName, options.federateType, federation);
    auto& rti = member.rtiAmbassador();
    auto const dimension = rti.getDimensionHandle(dimensionName);
    require(dimension.isValid() &&
                rti.getDimensionName(dimension) == dimensionName &&
                rti.getDimensionUpperBound(dimension) == 100U,
            "an accepted dimension input type was not publicly queryable");
    member.resign(rti::NO_ACTION);
    creator.rtiAmbassador().destroyFederationExecution(federation);
  };
  checkAccepted(na, L"TckNoTypeDimensionInput");
  checkAccepted(tableType, L"TckTableDimensionInput");
  member.disconnect();
  creator.disconnect();
}

void scenarioFomDimensionInputTypeContract(Options const& options,
                                           rti::CallbackModel model) {
  scenarioFomDimensionInputType(options, model);
}

void scenarioFomDimensionInputDescription(Options const& options,
                                          rti::CallbackModel model) {
  require(!options.mimFom.empty() &&
              !options.logicalTimeImplementationName.empty(),
          "Dimension input-description testing requires adapter-supplied MIM and logical time");
  auto const& missing = requiredFom(options, "dimension-input-description-na-invalid");
  auto const& described = requiredFom(options, "dimension-input-description-text");
  auto const& namedNa = requiredFom(options, "dimension-input-description-named-na");
  auto const& namedText = requiredFom(options, "dimension-input-description-named-text");
  Session creator(options, model, "dimension-input-description-creator");
  Session member(options, model, "dimension-input-description-member");
  auto const federation = federationName(options, "fom-dimension-input-description");
  creator.connect();
  member.connect();
  auto const createWithMim = [&](std::filesystem::path const& module) {
    creator.rtiAmbassador().createFederationExecutionWithMIM(
        federation, std::vector<std::wstring>{module.wstring()},
        options.mimFom.wstring(), options.logicalTimeImplementationName);
  };
  requireException([&] { createWithMim(missing); }, L"InconsistentFOM",
                   "a dimension with no named input type and NA description");

  auto const checkAccepted = [&](std::filesystem::path const& module,
                                 std::wstring const& dimensionName) {
    createWithMim(module);
    member.join(options.memberFederateName, options.federateType, federation);
    auto& rti = member.rtiAmbassador();
    auto const dimension = rti.getDimensionHandle(dimensionName);
    require(dimension.isValid() &&
                rti.getDimensionName(dimension) == dimensionName &&
                rti.getDimensionUpperBound(dimension) == 100U,
            "an accepted dimension input-description path was not publicly queryable");
    member.resign(rti::NO_ACTION);
    creator.rtiAmbassador().destroyFederationExecution(federation);
  };
  checkAccepted(described, L"TckDescribedInputWithoutNamedType");
  checkAccepted(namedNa, L"TckNamedInputNaDescription");
  checkAccepted(namedText, L"TckTableDimensionInput");
  member.disconnect();
  creator.disconnect();
}

void scenarioFomDimensionInputDescriptionContract(Options const& options,
                                                  rti::CallbackModel model) {
  scenarioFomDimensionInputDescription(options, model);
}

void scenarioFomDimensionInputNaExclusivity(Options const& options,
                                            rti::CallbackModel model) {
  require(!options.mimFom.empty() &&
              !options.logicalTimeImplementationName.empty(),
          "Dimension NA exclusivity requires adapter-supplied MIM and logical time");
  auto const& mixed = requiredFom(options, "dimension-input-mixed-na");
  auto const& explicitNa = requiredFom(options, "dimension-input-na");
  auto const& emptyList = requiredFom(options, "dimension-input-description-text");
  auto const& namedType = requiredFom(options, "dimension-input-table-type");
  Session creator(options, model, "dimension-input-na-exclusivity-creator");
  Session member(options, model, "dimension-input-na-exclusivity-member");
  auto const federation = federationName(options, "fom-dimension-input-na-exclusivity");
  creator.connect();
  member.connect();
  auto const createWithMim = [&](std::filesystem::path const& module) {
    creator.rtiAmbassador().createFederationExecutionWithMIM(
        federation, std::vector<std::wstring>{module.wstring()},
        options.mimFom.wstring(), options.logicalTimeImplementationName);
  };
  requireException([&] { createWithMim(mixed); }, L"InconsistentFOM",
                   "a dimension with both NA and a named input type");

  auto const checkAccepted = [&](std::filesystem::path const& module,
                                 std::wstring const& dimensionName) {
    // Reuse the rejected name to check that failed Create left no execution.
    createWithMim(module);
    member.join(options.memberFederateName, options.federateType, federation);
    auto& rti = member.rtiAmbassador();
    auto const dimension = rti.getDimensionHandle(dimensionName);
    require(dimension.isValid() &&
                rti.getDimensionName(dimension) == dimensionName &&
                rti.getDimensionUpperBound(dimension) == 100U,
            "an accepted dimension input path was not publicly queryable");
    member.resign(rti::NO_ACTION);
    creator.rtiAmbassador().destroyFederationExecution(federation);
  };
  checkAccepted(explicitNa, L"TckNoTypeDimensionInput");
  checkAccepted(emptyList, L"TckDescribedInputWithoutNamedType");
  checkAccepted(namedType, L"TckTableDimensionInput");
  member.disconnect();
  creator.disconnect();
}

void scenarioFomDimensionInputNaExclusivityContract(Options const& options,
                                                    rti::CallbackModel model) {
  scenarioFomDimensionInputNaExclusivity(options, model);
}

}  // namespace hla_rti_cpp_tck::detail
