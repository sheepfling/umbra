#include "tck_fom_module_scenarios.hpp"

namespace hla_rti_cpp_tck::detail {
namespace {

std::filesystem::path const& requiredFom(Options const& options,
                                         std::string const& key) {
  auto const found = options.scenarioFoms.find(key);
  require(found != options.scenarioFoms.end(),
          "Reference attribute resolution requires an adapter-supplied FOM");
  return found->second;
}

}  // namespace

void scenarioFomReferenceAttributeResolution(Options const& options,
                                             rti::CallbackModel model) {
  require(!options.logicalTimeImplementationName.empty(),
          "Reference attribute resolution requires adapter-supplied logical time");
  auto const& consumer = requiredFom(options, "reference-attribute-consumer");
  auto const& provider = requiredFom(options, "reference-attribute-provider");
  auto const& missing = requiredFom(options, "reference-attribute-missing");
  auto const& mismatched = requiredFom(options, "reference-attribute-mismatched");
  Session creator(options, model, "reference-attribute-creator");
  auto const federation = federationName(options, "fom-reference-attribute-resolution");
  creator.connect();
  auto const create = [&](std::filesystem::path const& first) {
    creator.rtiAmbassador().createFederationExecution(
        federation, std::vector<std::wstring>{first.wstring(), provider.wstring()},
        options.logicalTimeImplementationName);
  };
  create(consumer);
  creator.join(options.ownerFederateName, options.federateType, federation);
  auto& rti = creator.rtiAmbassador();
  auto const className = std::wstring{
      L"HLAobjectRoot.TckReferenceAttributeBase.TckReferenceAttributeChild"};
  auto const objectClass = rti.getObjectClassHandle(className);
  auto const identifier = rti.getAttributeHandle(objectClass, L"Identifier");
  require(objectClass.isValid() && identifier.isValid() &&
              rti.getAttributeName(objectClass, identifier) == L"Identifier",
          "Later FOM module did not supply the inherited referenced attribute");
  creator.resign(rti::NO_ACTION);
  rti.destroyFederationExecution(federation);

  for (auto const& invalid : {missing, mismatched}) {
    requireException([&] { create(invalid); }, L"InconsistentFOM",
                     "unresolved or mismatched reference attribute");
    create(consumer);
    rti.destroyFederationExecution(federation);
  }
  creator.disconnect();
}

void scenarioFomReferenceAttributeResolutionContract(Options const& options,
                                                     rti::CallbackModel model) {
  scenarioFomReferenceAttributeResolution(options, model);
}

}  // namespace hla_rti_cpp_tck::detail
