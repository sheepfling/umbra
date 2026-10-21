#include "tck_fom_module_scenarios.hpp"

namespace hla_rti_cpp_tck::detail {
namespace {

std::filesystem::path const& requiredFom(Options const& options,
                                         std::string const& name) {
  auto const found = options.scenarioFoms.find(name);
  require(found != options.scenarioFoms.end(),
          "Class-dimension merge requires two adapter-supplied FOM modules");
  return found->second;
}

void runClassDimensionMerge(Options const& options, rti::CallbackModel model,
                            bool interaction) {
  require(!options.logicalTimeImplementationName.empty(),
          "Class-dimension merge requires adapter-supplied logical time");
  auto const& first = requiredFom(options, interaction
      ? "interaction-dimension-merge-first" : "object-dimension-merge-first");
  auto const& second = requiredFom(options, interaction
      ? "interaction-dimension-merge-second" : "object-dimension-merge-second");
  auto const firstDimension = interaction
      ? L"TckInteractionMergeFirstDimension" : L"TckObjectMergeFirstDimension";
  auto const secondDimension = interaction
      ? L"TckInteractionMergeSecondDimension" : L"TckObjectMergeSecondDimension";
  Session creator(options, model, "class-dimension-creator");
  Session member(options, model, "class-dimension-member");
  auto const federation = federationName(options, interaction
      ? "fom-interaction-dimension-merge" : "fom-object-dimension-merge");
  creator.connect();
  member.connect();

  // Independent acceptance rules out a malformed input masquerading as a
  // composition failure. The final creation exercises the two-module union.
  for (auto const& module : {first.wstring(), second.wstring()}) {
    creator.rtiAmbassador().createFederationExecution(
        federation, std::vector<std::wstring>{module},
        options.logicalTimeImplementationName);
    creator.rtiAmbassador().destroyFederationExecution(federation);
  }
  creator.rtiAmbassador().createFederationExecution(
      federation, std::vector<std::wstring>{first.wstring(), second.wstring()},
      options.logicalTimeImplementationName);
  member.join(options.memberFederateName, options.federateType, federation);

  auto& rti = member.rtiAmbassador();
  auto const firstHandle = rti.getDimensionHandle(firstDimension);
  auto const secondHandle = rti.getDimensionHandle(secondDimension);
  require(firstHandle.isValid() && secondHandle.isValid() &&
              firstHandle != secondHandle &&
              rti.getDimensionName(firstHandle) == firstDimension &&
              rti.getDimensionName(secondHandle) == secondDimension,
          "Merged dimensions did not resolve as distinct named handles");
  auto const dimensions = interaction
      ? rti.getAvailableDimensionsForInteractionClass(
            rti.getInteractionClassHandle(
                L"HLAinteractionRoot.TckDimensionMergeInteraction"))
      : rti.getAvailableDimensionsForObjectClass(
            rti.getObjectClassHandle(L"HLAobjectRoot.TckDimensionMergeObject"));
  require(dimensions.size() == 2U && dimensions.count(firstHandle) == 1U &&
              dimensions.count(secondHandle) == 1U,
          "Same-named class did not retain both available dimensions");

  member.resign(rti::NO_ACTION);
  creator.rtiAmbassador().destroyFederationExecution(federation);
  member.disconnect();
  creator.disconnect();
}

}  // namespace

void scenarioFomObjectClassDimensionMerge(Options const& options,
                                           rti::CallbackModel model) {
  runClassDimensionMerge(options, model, false);
}

void scenarioFomObjectClassDimensionMergeContract(Options const& options,
                                                   rti::CallbackModel model) {
  scenarioFomObjectClassDimensionMerge(options, model);
}

void scenarioFomInteractionClassDimensionMerge(Options const& options,
                                                rti::CallbackModel model) {
  runClassDimensionMerge(options, model, true);
}

void scenarioFomInteractionClassDimensionMergeContract(Options const& options,
                                                        rti::CallbackModel model) {
  scenarioFomInteractionClassDimensionMerge(options, model);
}

}  // namespace hla_rti_cpp_tck::detail
