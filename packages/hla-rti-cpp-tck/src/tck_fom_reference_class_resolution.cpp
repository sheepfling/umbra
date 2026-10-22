#include "tck_fom_module_scenarios.hpp"

namespace hla_rti_cpp_tck::detail {
namespace {

std::filesystem::path const& requiredFom(Options const& options,
                                         std::string const& key) {
  auto const found = options.scenarioFoms.find(key);
  require(found != options.scenarioFoms.end(),
          "Reference class resolution requires an adapter-supplied FOM module");
  return found->second;
}

}  // namespace

void scenarioFomReferenceClassResolution(Options const& options,
                                         rti::CallbackModel model) {
  require(!options.logicalTimeImplementationName.empty(),
          "Reference class resolution requires adapter-supplied logical time");
  auto const& consumer = requiredFom(options, "reference-class-consumer");
  auto const& provider = requiredFom(options, "reference-class-provider");
  auto const& unresolved = requiredFom(options, "reference-class-unresolved");
  Session creator(options, model, "reference-class-creator");
  auto const federation = federationName(options, "fom-reference-class-resolution");
  creator.connect();
  auto const createResolved = [&] {
    creator.rtiAmbassador().createFederationExecution(
        federation,
        std::vector<std::wstring>{consumer.wstring(), provider.wstring()},
        options.logicalTimeImplementationName);
  };
  createResolved();
  creator.join(options.ownerFederateName, options.federateType, federation);
  auto& rti = creator.rtiAmbassador();
  auto const className = std::wstring{L"HLAobjectRoot.TckReferenceFixtureClass"};
  auto const objectClass = rti.getObjectClassHandle(className);
  require(objectClass.isValid() && rti.getObjectClassName(objectClass) == className,
          "Later FOM module did not supply the referenced class");
  creator.resign(rti::NO_ACTION);
  rti.destroyFederationExecution(federation);

  requireException(
      [&] {
        rti.createFederationExecution(
            federation, std::vector<std::wstring>{unresolved.wstring()},
            options.logicalTimeImplementationName);
      },
      L"InconsistentFOM", "unresolved reference-data class");
  createResolved();
  rti.destroyFederationExecution(federation);
  creator.disconnect();
}

void scenarioFomReferenceClassResolutionContract(Options const& options,
                                                 rti::CallbackModel model) {
  scenarioFomReferenceClassResolution(options, model);
}

}  // namespace hla_rti_cpp_tck::detail
