#include "tck_fom_module_scenarios.hpp"

namespace hla_rti_cpp_tck::detail {
namespace {

std::filesystem::path const& dimensionReferenceFom(Options const& options,
                                                   std::string const& key) {
  auto const found = options.scenarioFoms.find(key);
  require(found != options.scenarioFoms.end(),
          "Dimension reference resolution requires an adapter-supplied FOM");
  return found->second;
}

}  // namespace

void scenarioFomDimensionReferenceResolution(Options const& options,
                                             rti::CallbackModel model) {
  require(!options.logicalTimeImplementationName.empty(),
          "Dimension reference resolution requires adapter-supplied logical time");
  auto const& consumer = dimensionReferenceFom(options, "dimension-reference-consumer");
  auto const& provider = dimensionReferenceFom(options, "dimension-reference-provider");
  auto const& missing = dimensionReferenceFom(options, "dimension-reference-missing");
  Session creator(options, model, "dimension-reference-creator");
  auto const federation = federationName(options, "fom-dimension-reference-resolution");
  creator.connect();
  auto const create = [&](std::vector<std::filesystem::path> const& modules) {
    std::vector<std::wstring> paths;
    for (auto const& module : modules) paths.push_back(module.wstring());
    creator.rtiAmbassador().createFederationExecution(
        federation, paths, options.logicalTimeImplementationName);
  };
  create({consumer, provider});
  creator.join(options.ownerFederateName, options.federateType, federation);
  auto& rti = creator.rtiAmbassador();
  auto const dimension = rti.getDimensionHandle(L"TckDeferredDimension");
  auto const objectClass =
      rti.getObjectClassHandle(L"HLAobjectRoot.TckDimensionReferenceObject");
  auto const interaction =
      rti.getInteractionClassHandle(L"HLAinteractionRoot.TckDimensionReferenceInteraction");
  auto const objectDimensions = rti.getAvailableDimensionsForObjectClass(objectClass);
  auto const interactionDimensions =
      rti.getAvailableDimensionsForInteractionClass(interaction);
  require(dimension.isValid() && objectClass.isValid() && interaction.isValid() &&
              objectDimensions.size() == 1U && objectDimensions.count(dimension) == 1U &&
              interactionDimensions.size() == 1U &&
              interactionDimensions.count(dimension) == 1U &&
              rti.getDimensionUpperBound(dimension) == 100UL,
          "Later FOM module did not supply the available dimension to both classes");
  creator.resign(rti::NO_ACTION);
  rti.destroyFederationExecution(federation);

  requireException([&] { create({missing}); }, L"InconsistentFOM",
                   "unresolved available dimension");
  create({consumer, provider});
  rti.destroyFederationExecution(federation);
  creator.disconnect();
}

void scenarioFomDimensionReferenceResolutionContract(Options const& options,
                                                     rti::CallbackModel model) {
  scenarioFomDimensionReferenceResolution(options, model);
}

}  // namespace hla_rti_cpp_tck::detail
