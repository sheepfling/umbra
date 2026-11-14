#include "tck_fom_module_scenarios.hpp"

namespace hla_rti_cpp_tck::detail {
namespace {

std::filesystem::path const& noncompliantUpdateRateFom(Options const& options) {
  auto const found = options.scenarioFoms.find("noncompliant-update-rate");
  require(found != options.scenarioFoms.end(),
          "Noncompliant-FOM testing requires an adapter-supplied zero-rate module");
  return found->second;
}

std::filesystem::path const& noncompliantAtomicityFom(Options const& options) {
  auto const found = options.scenarioFoms.find("noncompliant-module-atomicity");
  require(found != options.scenarioFoms.end(),
          "Noncompliant-FOM atomicity testing requires an adapter-supplied mixed module");
  return found->second;
}

void runNoncompliantModuleScenario(Options const& options, rti::CallbackModel model) {
  require(!options.fom.empty(),
          "Noncompliant-FOM testing requires an adapter-supplied valid base FOM");
  require(!options.logicalTimeImplementationName.empty(),
          "Noncompliant-FOM testing requires adapter-supplied logical time");

  Session creator(options, model, "noncompliant-fom-creator");
  auto const federation = federationName(options, "fom-noncompliant-update-rate");
  creator.connect();
  std::vector<std::wstring> rejectedCreateModules{
      options.fom.wstring(), noncompliantUpdateRateFom(options).wstring()};

  requireExceptionOneOf(
      [&] {
        creator.rtiAmbassador().createFederationExecution(
            federation,
            rejectedCreateModules,
            options.logicalTimeImplementationName);
      },
      {L"InvalidFOM", L"InconsistentFOM"},
      "rejecting a valid-plus-noncompliant Create module set atomically");

  creator.rtiAmbassador().createFederationExecution(
      federation, options.fom.wstring(), options.logicalTimeImplementationName);
  creator.join(options.ownerFederateName, options.federateType, federation);
  require(creator.federateHandle().isValid(),
          "valid FOM retry after noncompliant-module rejection did not permit Join");
  creator.resign(rti::NO_ACTION);
  creator.rtiAmbassador().destroyFederationExecution(federation);
  creator.disconnect();

  Session owner(options, model, "noncompliant-module-atomicity-owner");
  Session joiner(options, model, "noncompliant-module-atomicity-joiner");
  auto const atomicityFederation =
      federationName(options, "fom-noncompliant-module-atomicity");
  owner.connect();
  joiner.connect();
  owner.rtiAmbassador().createFederationExecution(
      atomicityFederation,
      options.fom.wstring(),
      options.logicalTimeImplementationName);
  owner.join(options.ownerFederateName, options.federateType, atomicityFederation);

  auto const rejectedClassName = L"HLAobjectRoot.TckRejectedModuleObject";
  requireException(
      [&] {
        static_cast<void>(owner.rtiAmbassador().getObjectClassHandle(rejectedClassName));
      },
      L"NameNotFound",
      "looking up the mixed-module class before its join attempt");
  std::vector<std::wstring> rejectedJoinModules{
      options.fom.wstring(), noncompliantAtomicityFom(options).wstring()};
  requireExceptionOneOf(
      [&] {
        joiner.join(
            options.memberFederateName,
            options.federateType,
            atomicityFederation,
            rejectedJoinModules);
      },
      {L"InvalidFOM", L"InconsistentFOM"},
      "rejecting a Join module with a noncompliant rate and a valid class definition");
  requireException(
      [&] {
        static_cast<void>(owner.rtiAmbassador().getFederateHandle(
            options.memberFederateName));
      },
      L"NameNotFound",
      "looking up a federate after the rejected noncompliant-module Join");
  requireException(
      [&] {
        static_cast<void>(owner.rtiAmbassador().getObjectClassHandle(rejectedClassName));
      },
      L"NameNotFound",
      "checking that the rejected module added no class to the current FDD");

  joiner.join(
      options.memberFederateName,
      options.federateType,
      atomicityFederation);
  require(joiner.federateHandle().isValid(),
          "valid Join retry after noncompliant-module rejection did not establish membership");
  requireException(
      [&] {
        static_cast<void>(owner.rtiAmbassador().getObjectClassHandle(rejectedClassName));
      },
      L"NameNotFound",
      "checking that the rejected module class remains absent after a valid Join retry");

  joiner.resign(rti::NO_ACTION);
  owner.resign(rti::NO_ACTION);
  owner.rtiAmbassador().destroyFederationExecution(atomicityFederation);
  joiner.disconnect();
  owner.disconnect();
}

}  // namespace

void scenarioFomNoncompliantModule(Options const& options, rti::CallbackModel model) {
  runNoncompliantModuleScenario(options, model);
}

void scenarioFomNoncompliantModuleContract(Options const& options,
                                           rti::CallbackModel model) {
  runNoncompliantModuleScenario(options, model);
}

}  // namespace hla_rti_cpp_tck::detail
