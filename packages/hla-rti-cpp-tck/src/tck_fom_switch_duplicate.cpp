#include "tck_fom_module_scenarios.hpp"

namespace hla_rti_cpp_tck::detail {
namespace {

std::wstring module(Options const& options, std::string const& name) {
  auto const found = options.scenarioFoms.find(name);
  require(found != options.scenarioFoms.end(),
          "Switch duplicate requires adapter-supplied FOM modules");
  return found->second.wstring();
}

enum class Case { equal, order, omitted };

void runSwitchDuplicate(Options const& options, rti::CallbackModel model,
                        Case kind) {
  require(!options.logicalTimeImplementationName.empty(),
          "Switch duplicate requires adapter-supplied logical time");
  auto const disabled = module(options, "switch-duplicate-disabled");
  auto const second = module(options, kind == Case::equal
      ? "switch-duplicate-equal"
      : kind == Case::order ? "switch-duplicate-enabled"
                            : "switch-duplicate-omitted");
  Session creator(options, model, "switch-duplicate-creator");
  Session member(options, model, "switch-duplicate-member");
  auto const federation = federationName(options, kind == Case::equal
      ? "fom-switch-duplicate-equal"
      : kind == Case::order ? "fom-switch-duplicate-order"
                            : "fom-switch-duplicate-omitted");
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
    require(member.rtiAmbassador().getNonRegulatedGrantSwitch() == expected,
            "Public non-regulated-grant query did not preserve first switch setting");
    member.resign(rti::NO_ACTION);
    creator.rtiAmbassador().destroyFederationExecution(federation);
  };
  checkPair(disabled, second, false);
  if (kind != Case::equal) {
    checkPair(second, disabled, kind == Case::order);
  }
  member.disconnect();
  creator.disconnect();
}

}  // namespace

void scenarioFomSwitchDuplicateEqual(Options const& options,
                                     rti::CallbackModel model) {
  runSwitchDuplicate(options, model, Case::equal);
}

void scenarioFomSwitchDuplicateEqualContract(Options const& options,
                                             rti::CallbackModel model) {
  scenarioFomSwitchDuplicateEqual(options, model);
}

void scenarioFomSwitchDuplicateOrder(Options const& options,
                                     rti::CallbackModel model) {
  runSwitchDuplicate(options, model, Case::order);
}

void scenarioFomSwitchDuplicateOrderContract(Options const& options,
                                             rti::CallbackModel model) {
  scenarioFomSwitchDuplicateOrder(options, model);
}

void scenarioFomSwitchDuplicateOmitted(Options const& options,
                                       rti::CallbackModel model) {
  runSwitchDuplicate(options, model, Case::omitted);
}

void scenarioFomSwitchDuplicateOmittedContract(Options const& options,
                                               rti::CallbackModel model) {
  scenarioFomSwitchDuplicateOmitted(options, model);
}

}  // namespace hla_rti_cpp_tck::detail
