#include "tck_fom_module_scenarios.hpp"

namespace hla_rti_cpp_tck::detail {
namespace {

std::filesystem::path const& requiredFom(Options const& options,
                                         std::string const& key) {
  auto const found = options.scenarioFoms.find(key);
  require(found != options.scenarioFoms.end(),
          "Inherited member names require adapter-supplied FOM modules");
  return found->second;
}

}  // namespace

void scenarioFomInheritedMemberNames(Options const& options,
                                     rti::CallbackModel model) {
  require(!options.logicalTimeImplementationName.empty(),
          "Inherited member names require adapter-supplied logical time");
  auto const& valid = requiredFom(options, "inherited-member-names-valid");
  auto const& duplicateAttribute =
      requiredFom(options, "inherited-attribute-duplicate");
  auto const& duplicateParameter =
      requiredFom(options, "inherited-parameter-duplicate");
  Session creator(options, model, "inherited-member-names-creator");
  auto const federation = federationName(options, "fom-inherited-member-names");
  creator.connect();
  auto const create = [&](std::filesystem::path const& module) {
    creator.rtiAmbassador().createFederationExecution(
        federation, std::vector<std::wstring>{module.wstring()},
        options.logicalTimeImplementationName);
  };
  create(valid);
  creator.join(options.ownerFederateName, options.federateType, federation);
  auto& rti = creator.rtiAmbassador();
  auto const objectBase = rti.getObjectClassHandle(
      L"HLAobjectRoot.TckInheritedAttributeBase");
  auto const objectChild = rti.getObjectClassHandle(
      L"HLAobjectRoot.TckInheritedAttributeBase.TckInheritedAttributeChild");
  auto const baseAttribute =
      rti.getAttributeHandle(objectBase, L"TckInheritedAttribute");
  auto const childInheritedAttribute =
      rti.getAttributeHandle(objectChild, L"TckInheritedAttribute");
  auto const childAttribute =
      rti.getAttributeHandle(objectChild, L"TckChildAttribute");
  require(objectBase.isValid() && objectChild.isValid() &&
              baseAttribute.isValid() && childAttribute.isValid() &&
              baseAttribute == childInheritedAttribute &&
              childAttribute != childInheritedAttribute &&
              rti.getAttributeName(objectChild, childInheritedAttribute) ==
                  L"TckInheritedAttribute",
          "Valid object hierarchy did not preserve inherited attribute identity");

  auto const interactionBase = rti.getInteractionClassHandle(
      L"HLAinteractionRoot.TckInheritedParameterBase");
  auto const interactionChild = rti.getInteractionClassHandle(
      L"HLAinteractionRoot.TckInheritedParameterBase.TckInheritedParameterChild");
  auto const baseParameter =
      rti.getParameterHandle(interactionBase, L"TckInheritedParameter");
  auto const childInheritedParameter =
      rti.getParameterHandle(interactionChild, L"TckInheritedParameter");
  auto const childParameter =
      rti.getParameterHandle(interactionChild, L"TckChildParameter");
  require(interactionBase.isValid() && interactionChild.isValid() &&
              baseParameter.isValid() && childParameter.isValid() &&
              baseParameter == childInheritedParameter &&
              childParameter != childInheritedParameter &&
              rti.getParameterName(interactionChild, childInheritedParameter) ==
                  L"TckInheritedParameter",
          "Valid interaction hierarchy did not preserve inherited parameter identity");
  creator.resign(rti::NO_ACTION);
  rti.destroyFederationExecution(federation);

  for (auto const& invalid : {duplicateAttribute, duplicateParameter}) {
    requireException([&] { create(invalid); }, L"InconsistentFOM",
                     "duplicate inherited member name");
    create(valid);
    rti.destroyFederationExecution(federation);
  }
  creator.disconnect();
}

void scenarioFomInheritedMemberNamesContract(Options const& options,
                                             rti::CallbackModel model) {
  scenarioFomInheritedMemberNames(options, model);
}

}  // namespace hla_rti_cpp_tck::detail
