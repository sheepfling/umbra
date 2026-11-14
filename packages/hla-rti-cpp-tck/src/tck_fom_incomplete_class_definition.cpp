#include "tck_fom_module_scenarios.hpp"

namespace hla_rti_cpp_tck::detail {
namespace {

std::filesystem::path const& requiredScaffoldFom(
    Options const& options,
    std::string const& key) {
  auto const found = options.scenarioFoms.find(key);
  require(found != options.scenarioFoms.end(),
          "Full-class-definition testing requires adapter-supplied scaffold FOM modules");
  return found->second;
}

std::vector<std::wstring> modulePaths(
    std::filesystem::path const& first,
    std::filesystem::path const* second = nullptr) {
  std::vector<std::wstring> modules{first.wstring()};
  if (second != nullptr) {
    modules.push_back(second->wstring());
  }
  return modules;
}

void createFederation(
    Session& session,
    std::wstring const& federation,
    Options const& options,
    std::filesystem::path const& first,
    std::filesystem::path const* second = nullptr) {
  session.rtiAmbassador().createFederationExecution(
      federation,
      modulePaths(first, second),
      options.logicalTimeImplementationName);
}

void verifyCreateRejectsUnfinishedClass(
    Session& creator,
    Options const& options,
    std::filesystem::path const& scaffold,
    std::filesystem::path const& provider,
    std::string const& federationSuffix,
    std::string const& classKind) {
  auto const federation = federationName(options, federationSuffix);
  requireExceptionOneOf(
      [&] { createFederation(creator, federation, options, scaffold); },
      {L"InconsistentFOM", L"InvalidFOM"},
      "rejecting a Create whose module union leaves an " + classKind + " scaffold unfinished");
  createFederation(creator, federation, options, scaffold, &provider);
  creator.rtiAmbassador().destroyFederationExecution(federation);
}

void verifyJoinRejectsUnfinishedObjectClass(
    Session& owner,
    Session& joiner,
    Options const& options,
    std::wstring const& federation,
    std::filesystem::path const& scaffold,
    std::filesystem::path const& provider) {
  auto const className = std::wstring{L"HLAobjectRoot.TckScaffoldObjectClass"};
  requireExceptionOneOf(
      [&] { joiner.join(options.memberFederateName, options.federateType, federation,
                        modulePaths(scaffold)); },
      {L"InconsistentFOM", L"InvalidFOM"},
      "rejecting a Join whose module union leaves an object-class scaffold unfinished");
  requireException(
      [&] { static_cast<void>(owner.rtiAmbassador().getFederateHandle(
                 options.memberFederateName)); },
      L"NameNotFound",
      "checking that a rejected incomplete-object-class Join adds no federate");
  requireException(
      [&] { static_cast<void>(owner.rtiAmbassador().getObjectClassHandle(className)); },
      L"NameNotFound",
      "checking that a rejected incomplete-object-class Join adds no class");

  joiner.join(options.memberFederateName, options.federateType, federation,
              modulePaths(scaffold, &provider));
  require(owner.rtiAmbassador().getObjectClassHandle(className).isValid(),
          "a complete module union did not add the object class on Join");
  joiner.resign(rti::NO_ACTION);
}

void verifyJoinRejectsUnfinishedInteractionClass(
    Session& owner,
    Session& joiner,
    Options const& options,
    std::wstring const& federation,
    std::filesystem::path const& scaffold,
    std::filesystem::path const& provider) {
  auto const className = std::wstring{L"HLAinteractionRoot.TckScaffoldInteractionClass"};
  requireExceptionOneOf(
      [&] { joiner.join(options.memberFederateName, options.federateType, federation,
                        modulePaths(scaffold)); },
      {L"InconsistentFOM", L"InvalidFOM"},
      "rejecting a Join whose module union leaves an interaction-class scaffold unfinished");
  requireException(
      [&] { static_cast<void>(owner.rtiAmbassador().getFederateHandle(
                 options.memberFederateName)); },
      L"NameNotFound",
      "checking that a rejected incomplete-interaction-class Join adds no federate");
  requireException(
      [&] { static_cast<void>(owner.rtiAmbassador().getInteractionClassHandle(className)); },
      L"NameNotFound",
      "checking that a rejected incomplete-interaction-class Join adds no class");

  joiner.join(options.memberFederateName, options.federateType, federation,
              modulePaths(scaffold, &provider));
  require(owner.rtiAmbassador().getInteractionClassHandle(className).isValid(),
          "a complete module union did not add the interaction class on Join");
  joiner.resign(rti::NO_ACTION);
}

void runIncompleteClassDefinitionScenario(
    Options const& options,
    rti::CallbackModel model) {
  require(!options.fom.empty(),
          "Full-class-definition testing requires an adapter-supplied base FOM");
  require(!options.logicalTimeImplementationName.empty(),
          "Full-class-definition testing requires adapter-supplied logical time");

  auto const& incompleteObject = requiredScaffoldFom(options, "incomplete-object-class-scaffold");
  auto const& completeObject = requiredScaffoldFom(options, "complete-object-class-scaffold-provider");
  auto const& incompleteInteraction = requiredScaffoldFom(options, "incomplete-interaction-class-scaffold");
  auto const& completeInteraction = requiredScaffoldFom(options, "complete-interaction-class-scaffold-provider");

  Session creator(options, model, "incomplete-class-create");
  creator.connect();
  verifyCreateRejectsUnfinishedClass(
      creator, options, incompleteObject, completeObject,
      "fom-incomplete-object-class-definition", "object-class");
  verifyCreateRejectsUnfinishedClass(
      creator, options, incompleteInteraction, completeInteraction,
      "fom-incomplete-interaction-class-definition", "interaction-class");
  creator.disconnect();

  Session owner(options, model, "incomplete-class-owner");
  Session joiner(options, model, "incomplete-class-joiner");
  auto const federation = federationName(options, "fom-incomplete-class-join-definition");
  owner.connect();
  joiner.connect();
  owner.rtiAmbassador().createFederationExecution(
      federation, options.fom.wstring(), options.logicalTimeImplementationName);
  owner.join(options.ownerFederateName, options.federateType, federation);

  verifyJoinRejectsUnfinishedObjectClass(
      owner, joiner, options, federation, incompleteObject, completeObject);
  verifyJoinRejectsUnfinishedInteractionClass(
      owner, joiner, options, federation, incompleteInteraction, completeInteraction);

  owner.resign(rti::NO_ACTION);
  owner.rtiAmbassador().destroyFederationExecution(federation);
  joiner.disconnect();
  owner.disconnect();
}

}  // namespace

void scenarioFomIncompleteClassDefinition(Options const& options,
                                          rti::CallbackModel model) {
  runIncompleteClassDefinitionScenario(options, model);
}

void scenarioFomIncompleteClassDefinitionContract(Options const& options,
                                                  rti::CallbackModel model) {
  runIncompleteClassDefinitionScenario(options, model);
}

}  // namespace hla_rti_cpp_tck::detail
