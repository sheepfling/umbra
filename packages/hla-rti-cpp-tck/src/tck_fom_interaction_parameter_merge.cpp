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
  auto const transportationConflict =
      options.scenarioFoms.find("interaction-transportation-conflict");
  auto const orderConflict = options.scenarioFoms.find("interaction-order-conflict");
  auto const unresolvedDataType =
      options.scenarioFoms.find("interaction-parameter-unresolved-data-type");
  auto const invalidOrder =
      options.scenarioFoms.find("interaction-order-invalid-value");
  require(transportationConflict != options.scenarioFoms.end() &&
              orderConflict != options.scenarioFoms.end() &&
              unresolvedDataType != options.scenarioFoms.end() &&
              invalidOrder != options.scenarioFoms.end(),
          "Interaction parameter/transportation/order validation requires four adapter-supplied FOM modules");
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
  requireException(
      [&] {
        create(creator, federation, options, options.interactionParameterBaseFom,
               transportationConflict->second);
      },
      L"InconsistentFOM", "merging an interaction-class transportation conflict");
  create(creator, federation, options,
         options.interactionParameterBaseFom,
         options.interactionParameterDisjointFom);
  creator.rtiAmbassador().destroyFederationExecution(federation);
  requireException(
      [&] {
        create(creator, federation, options, options.interactionParameterBaseFom,
               orderConflict->second);
      },
      L"InconsistentFOM", "merging an interaction-class order conflict");
  creator.rtiAmbassador().createFederationExecution(
      federation, std::vector<std::wstring>{orderConflict->second.wstring()},
      options.logicalTimeImplementationName);
  member.join(options.memberFederateName, options.federateType, federation);
  auto const timestampOrderInteraction =
      member.rtiAmbassador().getInteractionClassHandle(
          L"HLAinteractionRoot.TckParameterMergeInteraction");
  require(timestampOrderInteraction.isValid() &&
              member.rtiAmbassador()
                  .getParameterHandle(timestampOrderInteraction, L"FirstValue")
                  .isValid(),
          "TimeStamp order FOM did not expose its interaction parameter table row");
  member.resign(rti::NO_ACTION);
  creator.rtiAmbassador().destroyFederationExecution(federation);
  requireException(
      [&] {
        creator.rtiAmbassador().createFederationExecution(
            federation,
            std::vector<std::wstring>{invalidOrder->second.wstring()},
            options.logicalTimeImplementationName);
      },
      L"InvalidFOM", "creating an interaction with an order outside Receive/TimeStamp");
  create(creator, federation, options,
         options.interactionParameterBaseFom,
         options.interactionParameterDisjointFom);
  creator.rtiAmbassador().destroyFederationExecution(federation);
  requireException(
      [&] {
        creator.rtiAmbassador().createFederationExecution(
            federation,
            std::vector<std::wstring>{unresolvedDataType->second.wstring()},
            options.logicalTimeImplementationName);
      },
      L"InconsistentFOM",
      "creating an interaction parameter with an unresolved data type name");
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
