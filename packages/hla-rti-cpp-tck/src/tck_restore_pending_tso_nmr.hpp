#pragma once

#include "tck_main_scenarios.hpp"

namespace hla_rti_cpp_tck::detail {

void scenarioRestorePendingTsoNmr(
    Options const& options, rti::CallbackModel model);
void scenarioRestorePendingTsoNmrContract(
    Options const& options, rti::CallbackModel model);
void scenarioRestorePendingTsoNmra(
    Options const& options, rti::CallbackModel model);
void scenarioRestorePendingTsoNmraContract(
    Options const& options, rti::CallbackModel model);

}  // namespace hla_rti_cpp_tck::detail
