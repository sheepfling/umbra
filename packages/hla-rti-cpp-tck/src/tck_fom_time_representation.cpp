#include "tck_fom_module_scenarios.hpp"

namespace hla_rti_cpp_tck::detail {
namespace {

std::filesystem::path const& requiredFom(Options const& options,
                                         std::string const& key) {
  auto const found = options.scenarioFoms.find(key);
  require(found != options.scenarioFoms.end(),
          "Time representation requires an adapter-supplied FOM module");
  return found->second;
}

void runTimeRepresentation(Options const& options, rti::CallbackModel model,
                           bool rejectInvalid) {
  require(!options.logicalTimeImplementationName.empty(),
          "Time representation requires adapter-supplied logical time");
  auto const& valid = requiredFom(options, "time-representation-valid");
  Session creator(options, model, "time-representation-creator");
  auto const federation = federationName(options, rejectInvalid
      ? "fom-time-representation-rejections"
      : "fom-time-representation-valid");
  creator.connect();
  auto const create = [&](std::filesystem::path const& module) {
    creator.rtiAmbassador().createFederationExecution(
        federation, std::vector<std::wstring>{module.wstring()},
        options.logicalTimeImplementationName);
  };
  create(valid);
  creator.rtiAmbassador().destroyFederationExecution(federation);

  if (rejectInvalid) {
    for (auto const& key : {"time-representation-basic-data",
                            "time-representation-reference-data"}) {
      auto const& invalid = requiredFom(options, key);
      requireException([&] { create(invalid); }, L"InconsistentFOM", key);
      // A rejected Create must not reserve the execution name.
      create(valid);
      creator.rtiAmbassador().destroyFederationExecution(federation);
    }
  }
  creator.disconnect();
}

}  // namespace

void scenarioFomTimeRepresentationValid(Options const& options,
                                        rti::CallbackModel model) {
  runTimeRepresentation(options, model, false);
}

void scenarioFomTimeRepresentationValidContract(Options const& options,
                                                rti::CallbackModel model) {
  scenarioFomTimeRepresentationValid(options, model);
}

void scenarioFomTimeRepresentationRejections(Options const& options,
                                             rti::CallbackModel model) {
  runTimeRepresentation(options, model, true);
}

void scenarioFomTimeRepresentationRejectionsContract(Options const& options,
                                                     rti::CallbackModel model) {
  scenarioFomTimeRepresentationRejections(options, model);
}

}  // namespace hla_rti_cpp_tck::detail
