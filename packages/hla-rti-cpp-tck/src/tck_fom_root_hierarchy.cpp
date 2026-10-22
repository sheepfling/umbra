#include "tck_fom_module_scenarios.hpp"

namespace hla_rti_cpp_tck::detail {
namespace {

std::filesystem::path const& requiredFom(Options const& options,
                                         std::string const& key) {
  auto const found = options.scenarioFoms.find(key);
  require(found != options.scenarioFoms.end(),
          "Root hierarchy requires an adapter-supplied FOM module");
  return found->second;
}

}  // namespace

void scenarioFomRootHierarchy(Options const& options, rti::CallbackModel model) {
  require(!options.logicalTimeImplementationName.empty(),
          "Root hierarchy requires adapter-supplied logical time");
  auto const& valid = requiredFom(options, "root-hierarchy-valid");
  auto const& objectRoot = requiredFom(options, "root-hierarchy-invalid-object");
  auto const& interactionRoot =
      requiredFom(options, "root-hierarchy-invalid-interaction");
  Session creator(options, model, "root-hierarchy-creator");
  auto const federation = federationName(options, "fom-root-hierarchy");
  creator.connect();
  auto const create = [&](std::filesystem::path const& module) {
    creator.rtiAmbassador().createFederationExecution(
        federation, std::vector<std::wstring>{module.wstring()},
        options.logicalTimeImplementationName);
  };
  create(valid);
  creator.join(options.ownerFederateName, options.federateType, federation);
  auto& rti = creator.rtiAmbassador();
  auto const objectName = std::wstring{L"HLAobjectRoot.TckRootObject"};
  auto const interactionName = std::wstring{L"HLAinteractionRoot.TckRootInteraction"};
  auto const object = rti.getObjectClassHandle(objectName);
  auto const interaction = rti.getInteractionClassHandle(interactionName);
  require(object.isValid() && interaction.isValid() &&
              rti.getObjectClassName(object) == objectName &&
              rti.getInteractionClassName(interaction) == interactionName,
          "Accepted standard roots did not expose their child class names");
  creator.resign(rti::NO_ACTION);
  rti.destroyFederationExecution(federation);

  for (auto const& invalid : {objectRoot, interactionRoot}) {
    requireException([&] { create(invalid); }, L"InconsistentFOM",
                     "nonstandard top-level class");
    create(valid);
    rti.destroyFederationExecution(federation);
  }
  creator.disconnect();
}

void scenarioFomRootHierarchyContract(Options const& options,
                                      rti::CallbackModel model) {
  scenarioFomRootHierarchy(options, model);
}

}  // namespace hla_rti_cpp_tck::detail
