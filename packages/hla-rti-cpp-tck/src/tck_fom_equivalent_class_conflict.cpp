#include "tck_fom_module_scenarios.hpp"

namespace hla_rti_cpp_tck::detail {
namespace {

std::filesystem::path const& requiredFom(Options const& options,
                                         std::string const& key) {
  auto const found = options.scenarioFoms.find(key);
  require(found != options.scenarioFoms.end(),
          "Class-identity composition requires adapter-supplied FOM modules");
  return found->second;
}

}  // namespace

void scenarioFomEquivalentClassConflict(Options const& options,
                                        rti::CallbackModel model) {
  require(!options.logicalTimeImplementationName.empty(),
          "Class-identity composition requires adapter-supplied logical time");
  auto const& base = requiredFom(options, "equivalent-class-base");
  auto const& conflict = requiredFom(options, "equivalent-class-conflict");
  Session creator(options, model, "equivalent-class-creator");
  auto const federation = federationName(options, "fom-equivalent-class-conflict");
  creator.connect();
  auto const create = [&](std::filesystem::path const& second) {
    creator.rtiAmbassador().createFederationExecution(
        federation, std::vector<std::wstring>{base.wstring(), second.wstring()},
        options.logicalTimeImplementationName);
  };
  create(base);
  creator.join(options.ownerFederateName, options.federateType, federation);
  auto& rti = creator.rtiAmbassador();
  auto const className = std::wstring{L"HLAobjectRoot.TckEquivalentClass"};
  auto const objectClass = rti.getObjectClassHandle(className);
  require(objectClass.isValid() && rti.getObjectClassName(objectClass) == className,
          "Equivalent duplicate modules did not preserve the class identity");
  creator.resign(rti::NO_ACTION);
  rti.destroyFederationExecution(federation);

  requireException([&] { create(conflict); }, L"InconsistentFOM",
                   "same-identity object class with conflicting sharing");
  create(base);
  rti.destroyFederationExecution(federation);
  creator.disconnect();
}

void scenarioFomEquivalentClassConflictContract(Options const& options,
                                                rti::CallbackModel model) {
  scenarioFomEquivalentClassConflict(options, model);
}

}  // namespace hla_rti_cpp_tck::detail
