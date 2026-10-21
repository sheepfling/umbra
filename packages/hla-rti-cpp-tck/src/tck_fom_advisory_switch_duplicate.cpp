#include "tck_fom_module_scenarios.hpp"

namespace hla_rti_cpp_tck::detail {
namespace {

std::wstring module(Options const& options, std::string const& name) {
  auto const found = options.scenarioFoms.find(name);
  require(found != options.scenarioFoms.end(),
          "Advisory switch duplicate requires adapter-supplied FOM modules");
  return found->second.wstring();
}

void runAdvisorySwitchDuplicate(Options const& options,
                                rti::CallbackModel model, bool orderConflict,
                                bool omitted) {
  require(!options.logicalTimeImplementationName.empty(),
          "Advisory switch duplicate requires adapter-supplied logical time");
  auto const disabled = module(options, "advisory-switch-duplicate-disabled");
  auto const second = module(options, orderConflict
      ? "advisory-switch-duplicate-enabled"
      : omitted ? "advisory-switch-duplicate-omitted"
                : "advisory-switch-duplicate-equal");
  Session creator(options, model, "advisory-switch-duplicate-creator");
  Session member(options, model, "advisory-switch-duplicate-member");
  auto const federation = federationName(options, orderConflict
      ? "fom-advisory-switch-duplicate-order"
      : omitted ? "fom-advisory-switch-duplicate-omitted"
                : "fom-advisory-switch-duplicate-equal");
  creator.connect();
  member.connect();
  auto create = [&](std::vector<std::wstring> const& modules) {
    creator.rtiAmbassador().createFederationExecution(
        federation, modules, options.logicalTimeImplementationName);
  };
  for (auto const& single : {disabled, second}) {
    create({single});
    creator.rtiAmbassador().destroyFederationExecution(federation);
  }
  auto checkPair = [&](std::wstring const& first, std::wstring const& last,
                       bool expected) {
    create({first, last});
    member.join(options.memberFederateName, options.federateType, federation);
    require(member.rtiAmbassador().getAdvisoriesUseKnownClassSwitch() == expected,
            "Public known-class advisory query did not preserve first FOM setting");
    member.resign(rti::NO_ACTION);
    creator.rtiAmbassador().destroyFederationExecution(federation);
  };
  checkPair(disabled, second, false);
  if (orderConflict || omitted) {
    checkPair(second, disabled, orderConflict);
  }
  member.disconnect();
  creator.disconnect();
}

}  // namespace

void scenarioFomAdvisorySwitchDuplicateEqual(Options const& options,
                                             rti::CallbackModel model) {
  runAdvisorySwitchDuplicate(options, model, false, false);
}

void scenarioFomAdvisorySwitchDuplicateEqualContract(
    Options const& options, rti::CallbackModel model) {
  scenarioFomAdvisorySwitchDuplicateEqual(options, model);
}

void scenarioFomAdvisorySwitchDuplicateOrder(Options const& options,
                                             rti::CallbackModel model) {
  runAdvisorySwitchDuplicate(options, model, true, false);
}

void scenarioFomAdvisorySwitchDuplicateOrderContract(
    Options const& options, rti::CallbackModel model) {
  scenarioFomAdvisorySwitchDuplicateOrder(options, model);
}

void scenarioFomAdvisorySwitchDuplicateOmitted(Options const& options,
                                               rti::CallbackModel model) {
  runAdvisorySwitchDuplicate(options, model, false, true);
}

void scenarioFomAdvisorySwitchDuplicateOmittedContract(
    Options const& options, rti::CallbackModel model) {
  scenarioFomAdvisorySwitchDuplicateOmitted(options, model);
}

}  // namespace hla_rti_cpp_tck::detail
