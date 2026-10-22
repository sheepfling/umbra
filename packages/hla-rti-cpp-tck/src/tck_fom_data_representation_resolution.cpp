#include "tck_fom_module_scenarios.hpp"

namespace hla_rti_cpp_tck::detail {
namespace {

std::filesystem::path const& representationFom(Options const& options,
                                               std::string const& key) {
  auto const found = options.scenarioFoms.find(key);
  require(found != options.scenarioFoms.end(),
          "Data representation resolution requires an adapter-supplied FOM");
  return found->second;
}

}  // namespace

void scenarioFomDataRepresentationResolution(Options const& options,
                                             rti::CallbackModel model) {
  require(!options.logicalTimeImplementationName.empty(),
          "Data representation resolution requires adapter-supplied logical time");
  auto const& special = representationFom(options, "representation-special");
  auto const& missingSimple = representationFom(options, "representation-missing-simple");
  auto const& missingEnumerated =
      representationFom(options, "representation-missing-enumerated");
  auto const& invalidSpecial =
      representationFom(options, "representation-invalid-special");
  auto const& invalidBasic =
      representationFom(options, "representation-invalid-basic");
  auto const& attributeProvider =
      representationFom(options, "reference-attribute-provider");

  Session creator(options, model, "representation-creator");
  auto const federation = federationName(options, "fom-data-representation-resolution");
  creator.connect();
  auto const create = [&](std::vector<std::filesystem::path> const& modules) {
    std::vector<std::wstring> paths;
    for (auto const& module : modules) paths.push_back(module.wstring());
    creator.rtiAmbassador().createFederationExecution(
        federation, paths, options.logicalTimeImplementationName);
  };
  create({special});
  creator.join(options.ownerFederateName, options.federateType, federation);
  require(creator.federateHandle().isValid(),
          "Special object-instance identifier references did not permit Join");
  creator.resign(rti::NO_ACTION);
  creator.rtiAmbassador().destroyFederationExecution(federation);

  for (auto const& invalid : {missingSimple, missingEnumerated, invalidSpecial}) {
    requireException([&] { create({invalid}); }, L"InconsistentFOM",
                     "invalid data representation");
    create({special});
    creator.rtiAmbassador().destroyFederationExecution(federation);
  }
  requireException([&] { create({invalidBasic, attributeProvider}); },
                   L"InconsistentFOM", "invalid ordinary reference representation");
  create({special});
  creator.rtiAmbassador().destroyFederationExecution(federation);
  creator.disconnect();
}

void scenarioFomDataRepresentationResolutionContract(Options const& options,
                                                     rti::CallbackModel model) {
  scenarioFomDataRepresentationResolution(options, model);
}

}  // namespace hla_rti_cpp_tck::detail
