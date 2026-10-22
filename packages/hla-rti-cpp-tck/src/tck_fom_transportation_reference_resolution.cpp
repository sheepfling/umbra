#include "tck_fom_module_scenarios.hpp"

namespace hla_rti_cpp_tck::detail {
namespace {

std::filesystem::path const& transportationFom(Options const& options,
                                               std::string const& key) {
  auto const found = options.scenarioFoms.find(key);
  require(found != options.scenarioFoms.end(),
          "Transportation reference resolution requires an adapter-supplied FOM");
  return found->second;
}

}  // namespace

void scenarioFomTransportationReferenceResolution(Options const& options,
                                                  rti::CallbackModel model) {
  require(!options.logicalTimeImplementationName.empty(),
          "Transportation reference resolution requires adapter-supplied logical time");
  auto const& consumer = transportationFom(options, "transportation-reference-consumer");
  auto const& provider = transportationFom(options, "transportation-reference-provider");
  auto const& missing = transportationFom(options, "transportation-reference-missing");

  Session creator(options, model, "transportation-reference-creator");
  auto const federation = federationName(options, "fom-transportation-reference-resolution");
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
  auto const transport = rti.getTransportationTypeHandle(L"TckDeferredTransportation");
  auto const objectClass =
      rti.getObjectClassHandle(L"HLAobjectRoot.TckTransportationReferenceObject");
  auto const attribute = rti.getAttributeHandle(objectClass, L"Value");
  auto const interaction =
      rti.getInteractionClassHandle(L"HLAinteractionRoot.TckTransportationReferenceInteraction");
  require(transport.isValid() && objectClass.isValid() && attribute.isValid() &&
              interaction.isValid() &&
              rti.getTransportationTypeName(transport) == L"TckDeferredTransportation" &&
              rti.getAttributeName(objectClass, attribute) == L"Value",
          "Later FOM module did not resolve the transportation-backed classes and attribute");
  creator.resign(rti::NO_ACTION);
  rti.destroyFederationExecution(federation);

  requireException([&] { create({missing}); }, L"InconsistentFOM",
                   "unresolved transportation reference");
  create({consumer, provider});
  rti.destroyFederationExecution(federation);
  creator.disconnect();
}

void scenarioFomTransportationReferenceResolutionContract(Options const& options,
                                                          rti::CallbackModel model) {
  scenarioFomTransportationReferenceResolution(options, model);
}

}  // namespace hla_rti_cpp_tck::detail
