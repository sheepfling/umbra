#pragma once

#include "tck_main_scenarios.hpp"

namespace hla_rti_cpp_tck::detail {

void scenarioRestoreSaveAfterFederationDestruction(
    Options const& options, rti::CallbackModel model);
void scenarioRestoreSaveAfterFederationDestructionContract(
    Options const& options, rti::CallbackModel model);
void scenarioSaveRestoreQueuedReceiveOrderInteraction(
    Options const& options, rti::CallbackModel model);

}  // namespace hla_rti_cpp_tck::detail
