#include "tck_fom_module_scenarios.hpp"

namespace hla_rti_cpp_tck::detail {

void scenarioFomVariantEnumeratorMembership(Options const& options,
                                            rti::CallbackModel model) {
  require(!options.logicalTimeImplementationName.empty(),
          "Variant enumerator membership requires adapter-supplied logical time");
  auto const unknownMember = options.scenarioFoms.find("variant-member-unknown");
  auto const unknownEndpoint = options.scenarioFoms.find("variant-member-unknown-endpoint");
  auto const incomplete = options.scenarioFoms.find("variant-member-incomplete");
  auto const consumer = options.scenarioFoms.find("variant-member-later-consumer");
  auto const provider = options.scenarioFoms.find("variant-member-later-provider");
  require(unknownMember != options.scenarioFoms.end() &&
              unknownEndpoint != options.scenarioFoms.end() &&
              incomplete != options.scenarioFoms.end() &&
              consumer != options.scenarioFoms.end() &&
              provider != options.scenarioFoms.end(),
          "Variant enumerator membership requires adapter-supplied FOM modules");

  Session creator(options, model, "variant-enumerator-membership-creator");
  auto const federation = federationName(options, "fom-variant-enumerator-membership");
  creator.connect();
  auto& rti = creator.rtiAmbassador();
  auto const create = [&](std::vector<std::wstring> const& modules) {
    rti.createFederationExecution(federation, modules,
                                  options.logicalTimeImplementationName);
  };
  auto const resolved = std::vector<std::wstring>{consumer->second.wstring(),
                                                  provider->second.wstring()};
  create(resolved);
  rti.destroyFederationExecution(federation);
  requireException([&] { create({unknownMember->second.wstring()}); },
                   L"InconsistentFOM", "undeclared variant discriminant member");
  requireException([&] { create({unknownEndpoint->second.wstring()}); },
                   L"InconsistentFOM", "undeclared variant range endpoint");
  create({incomplete->second.wstring()});
  rti.destroyFederationExecution(federation);
  create(resolved);
  rti.destroyFederationExecution(federation);
  creator.disconnect();
}

void scenarioFomVariantEnumeratorMembershipContract(Options const& options,
                                                    rti::CallbackModel model) {
  scenarioFomVariantEnumeratorMembership(options, model);
}

}  // namespace hla_rti_cpp_tck::detail
