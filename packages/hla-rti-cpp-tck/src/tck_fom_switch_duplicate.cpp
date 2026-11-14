#include "tck_fom_module_scenarios.hpp"

#include <array>
#include <utility>

namespace hla_rti_cpp_tck::detail {
namespace {

std::wstring module(Options const& options, std::string const& name) {
  auto const found = options.scenarioFoms.find(name);
  require(found != options.scenarioFoms.end(),
          "Switch duplicate requires adapter-supplied FOM modules");
  return found->second.wstring();
}

enum class Case { equal, order, omitted };

template <typename Operation>
void requireSwitchConflictDiagnostic(
    Operation operation,
    std::string const& description) {
  try {
    operation();
  } catch (rti::Exception const& error) {
    require(
        error.name() == L"InconsistentFOM",
        description + " threw " + toNarrow(error.name()) +
            " instead of InconsistentFOM");
    require(
        error.what().find(L"nonRegulatedGrant") != std::wstring::npos,
        description +
            " did not return exception details identifying the conflicting switch");
    return;
  } catch (...) {
    throw std::runtime_error(description + " threw a non-HLA exception");
  }
  throw std::runtime_error(
      description + " completed without InconsistentFOM");
}

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
  Session joiner(options, model, "switch-duplicate-module-joiner");
  auto const federation = federationName(options, kind == Case::equal
      ? "fom-switch-duplicate-equal"
      : kind == Case::order ? "fom-switch-duplicate-order"
                            : "fom-switch-duplicate-omitted");
  creator.connect();
  member.connect();
  if (kind == Case::order) {
    joiner.connect();
  }
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
  if (kind == Case::order) {
    auto rejectConflictThenRetry = [&](std::wstring const& first,
                                       std::wstring const& last,
                                       bool expected) {
      requireSwitchConflictDiagnostic(
          [&] { create({first, last}); },
          "different definitions of the same named support switch");
      // A rejected composition must leave the federation name reusable.
      create({first});
      member.join(options.memberFederateName, options.federateType, federation);
      require(member.rtiAmbassador().getNonRegulatedGrantSwitch() == expected,
              "Valid retry did not preserve its single switch definition");
      member.resign(rti::NO_ACTION);
      creator.rtiAmbassador().destroyFederationExecution(federation);
    };
    rejectConflictThenRetry(disabled, second, false);
    rejectConflictThenRetry(second, disabled, true);

    auto rejectJoinConflictThenRetry = [&](std::wstring const& existing,
                                           std::wstring const& conflicting,
                                           bool expected) {
      create({existing});
      member.join(options.memberFederateName, options.federateType, federation);
      requireSwitchConflictDiagnostic(
          [&] {
            joiner.join(options.ownerFederateName, options.federateType,
                        federation, {conflicting});
          },
          "loading a conflicting named support switch during Join");
      require(member.rtiAmbassador().getNonRegulatedGrantSwitch() == expected,
              "Rejected Join changed the current federation switch value");
      // Reuse both the federation and federate name with an equivalent module
      // to verify the rejected extension was not retained.
      joiner.join(options.ownerFederateName, options.federateType,
                  federation, {existing});
      require(joiner.rtiAmbassador().getNonRegulatedGrantSwitch() == expected,
              "Valid Join retry did not preserve the existing switch value");
      joiner.resign(rti::NO_ACTION);
      member.resign(rti::NO_ACTION);
      creator.rtiAmbassador().destroyFederationExecution(federation);
    };
    rejectJoinConflictThenRetry(disabled, second, false);
    rejectJoinConflictThenRetry(second, disabled, true);
  } else {
    checkPair(disabled, second, false);
    if (kind == Case::omitted) {
      checkPair(second, disabled, false);
    }
  }
  if (kind == Case::order) {
    joiner.disconnect();
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

void scenarioFomAutomaticResignActionSetting(Options const& options,
                                             rti::CallbackModel model) {
  require(!options.mimFom.empty() &&
              !options.logicalTimeImplementationName.empty(),
          "Automatic Resign Action setting requires adapter-supplied MIM and logical time");
  Session creator(options, model, "resign-action-setting-creator");
  Session member(options, model, "resign-action-setting-member");
  auto const federation = federationName(options, "fom-resign-action-setting");
  creator.connect();
  member.connect();
  auto const createWithMim = [&](std::wstring const& modulePath) {
    creator.rtiAmbassador().createFederationExecutionWithMIM(
        federation, std::vector<std::wstring>{modulePath}, options.mimFom.wstring(),
        options.logicalTimeImplementationName);
  };

  requireException(
      [&] {
        createWithMim(module(options, "resign-invalid"));
      },
      L"InvalidFOM",
      "an Automatic Resign Action outside the IEEE 1516.2-2025 allowed set");

  std::array<std::pair<std::string, rti::ResignAction>, 6> const allowedActions{{
      {"resign-uncond", rti::UNCONDITIONALLY_DIVEST_ATTRIBUTES},
      {"resign-delete", rti::DELETE_OBJECTS},
      {"resign-cancel", rti::CANCEL_PENDING_OWNERSHIP_ACQUISITIONS},
      {"resign-delete-divest", rti::DELETE_OBJECTS_THEN_DIVEST},
      {"resign-cancel-delete-divest", rti::CANCEL_THEN_DELETE_THEN_DIVEST},
      {"resign-no-action", rti::NO_ACTION},
  }};
  for (auto const& [fixture, expected] : allowedActions) {
    createWithMim(module(options, fixture));
    member.join(options.memberFederateName, options.federateType, federation);
    require(member.rtiAmbassador().getAutomaticResignDirective() == expected,
            "the public Automatic Resign Action value did not match its FOM switches-table setting");
    member.resign(rti::NO_ACTION);
    creator.rtiAmbassador().destroyFederationExecution(federation);
  }
  member.disconnect();
  creator.disconnect();
}

void scenarioFomAutomaticResignActionSettingContract(Options const& options,
                                                    rti::CallbackModel model) {
  scenarioFomAutomaticResignActionSetting(options, model);
}

}  // namespace hla_rti_cpp_tck::detail
