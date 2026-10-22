#include "tck_fom_module_scenarios.hpp"

namespace hla_rti_cpp_tck::detail {

void scenarioFomEnumeratedRepresentation(Options const& options,
                                          rti::CallbackModel model) {
  require(!options.logicalTimeImplementationName.empty(),
          "Enumerated representation requires adapter-supplied logical time");
  auto const valid = options.scenarioFoms.find("enumerated-representation-omitted");
  auto const invalid = options.scenarioFoms.find("enumerated-representation-nonbasic");
  require(valid != options.scenarioFoms.end() &&
              invalid != options.scenarioFoms.end(),
          "Enumerated representation requires adapter-supplied FOM modules");

  Session creator(options, model, "enumerated-representation-creator");
  auto const federation = federationName(options, "fom-enumerated-representation");
  creator.connect();
  auto& rti = creator.rtiAmbassador();
  auto const createValid = [&] {
    rti.createFederationExecution(federation, valid->second.wstring(),
                                  options.logicalTimeImplementationName);
  };
  createValid();
  rti.destroyFederationExecution(federation);

  requireException(
      [&] {
        rti.createFederationExecution(federation, invalid->second.wstring(),
                                      options.logicalTimeImplementationName);
      },
      L"InconsistentFOM", "non-basic enumerated representation");
  createValid();
  rti.destroyFederationExecution(federation);
  creator.disconnect();
}

void scenarioFomEnumeratedRepresentationContract(Options const& options,
                                                  rti::CallbackModel model) {
  scenarioFomEnumeratedRepresentation(options, model);
}

}  // namespace hla_rti_cpp_tck::detail
