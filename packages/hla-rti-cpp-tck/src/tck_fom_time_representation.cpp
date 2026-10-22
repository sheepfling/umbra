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

void scenarioFomDefaultLogicalTimeSelection(Options const& options,
                                            rti::CallbackModel model) {
  auto const& unconstrained = requiredFom(options, "default-logical-time");
  auto const& integerTime = requiredFom(options, "time-representation-valid");
  Session member(options, model, "fom-default-logical-time-member");
  auto const federation = federationName(options, "fom-default-logical-time");
  member.connect();

  // Omitting the time name selects the standard default, which conflicts with
  // an FDD that explicitly documents HLAinteger64Time.
  requireException(
      [&] {
        member.rtiAmbassador().createFederationExecution(
            federation, integerTime.wstring());
      },
      L"InconsistentFOM",
      "default logical time against an integer64 FDD");

  // A rejected Create must not reserve the name. The adapter supplies a FOM
  // without a declared time representation for the positive default path.
  member.rtiAmbassador().createFederationExecution(
      federation, unconstrained.wstring());
  member.join(options.ownerFederateName, options.federateType, federation);
  auto factory = member.rtiAmbassador().getTimeFactory();
  require(factory != nullptr && factory->getName() == L"HLAfloat64Time",
          "Create without a time name did not select HLAfloat64Time");
  member.resign(rti::NO_ACTION);
  member.rtiAmbassador().destroyFederationExecution(federation);
  member.disconnect();
}

void scenarioFomDefaultLogicalTimeSelectionContract(Options const& options,
                                                    rti::CallbackModel model) {
  scenarioFomDefaultLogicalTimeSelection(options, model);
}

void scenarioFomUnavailableLogicalTimeFactory(Options const& options,
                                              rti::CallbackModel model) {
  auto const& unconstrained = requiredFom(options, "default-logical-time");
  require(!options.unsupportedLogicalTimeImplementationName.empty(),
          "Unavailable-factory testing requires an adapter-declared unsupported name");
  Session creator(options, model, "fom-unavailable-time-factory-creator");
  auto const federation = federationName(options, "fom-unavailable-time-factory");
  creator.connect();
  requireException(
      [&] {
        creator.rtiAmbassador().createFederationExecution(
            federation, unconstrained.wstring(),
            options.unsupportedLogicalTimeImplementationName);
      },
      L"CouldNotCreateLogicalTimeFactory",
      "Create with an adapter-declared unavailable logical-time factory");
  creator.rtiAmbassador().createFederationExecution(
      federation, unconstrained.wstring());
  creator.rtiAmbassador().destroyFederationExecution(federation);
  creator.disconnect();
}

void scenarioFomUnavailableLogicalTimeFactoryContract(Options const& options,
                                                      rti::CallbackModel model) {
  scenarioFomUnavailableLogicalTimeFactory(options, model);
}

}  // namespace hla_rti_cpp_tck::detail
