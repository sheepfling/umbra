#include "tck_fom_module_scenarios.hpp"

namespace hla_rti_cpp_tck::detail {
namespace {

void create(Session& session, std::wstring const& federation,
            Options const& options,
            std::filesystem::path const& first,
            std::filesystem::path const& second) {
  session.rtiAmbassador().createFederationExecution(
      federation, std::vector<std::wstring>{first.wstring(), second.wstring()},
      options.logicalTimeImplementationName);
}

}  // namespace

void scenarioFomInteractionParameterMergeConflict(Options const& options,
                                                   rti::CallbackModel model) {
  require(!options.interactionParameterBaseFom.empty() &&
              !options.interactionParameterDisjointFom.empty() &&
              !options.interactionParameterConflictFom.empty(),
          "Annex C.3 interaction-parameter test requires three adapter-supplied FOM modules");
  require(!options.logicalTimeImplementationName.empty(),
          "Annex C.3 interaction-parameter test requires adapter-supplied logical time");
  Session creator(options, model, "fom-parameter-creator");
  Session member(options, model, "fom-parameter-member");
  auto const federation = federationName(options, "fom-interaction-parameter-merge");
  creator.connect();
  member.connect();

  create(creator, federation, options,
         options.interactionParameterBaseFom,
         options.interactionParameterDisjointFom);
  member.join(options.memberFederateName, options.federateType, federation);
  auto const interaction = member.rtiAmbassador().getInteractionClassHandle(
      L"HLAinteractionRoot.TckParameterMergeInteraction");
  require(interaction.isValid(),
          "Merged interaction class was not visible to a joined member");
  require(member.rtiAmbassador().getParameterHandle(interaction, L"FirstValue").isValid() &&
              member.rtiAmbassador().getParameterHandle(interaction, L"SecondValue").isValid(),
          "Merged interaction class did not retain both disjoint parameters");
  member.resign(rti::NO_ACTION);
  creator.rtiAmbassador().destroyFederationExecution(federation);

  requireException(
      [&] { create(creator, federation, options,
                   options.interactionParameterBaseFom,
                   options.interactionParameterConflictFom); },
      L"InconsistentFOM",
      "merging a same-named interaction parameter with conflicting data types");
  create(creator, federation, options,
         options.interactionParameterBaseFom,
         options.interactionParameterDisjointFom);
  creator.rtiAmbassador().destroyFederationExecution(federation);
  member.disconnect();
  creator.disconnect();
}

void scenarioFomInteractionParameterMergeConflictContract(Options const& options,
                                                           rti::CallbackModel model) {
  scenarioFomInteractionParameterMergeConflict(options, model);
}

}  // namespace hla_rti_cpp_tck::detail
