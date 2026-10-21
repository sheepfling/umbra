#pragma once

#include "tck_main_scenarios.hpp"

namespace hla_rti_cpp_tck::detail {

void scenarioRestorePendingTimeAdvance(
    Options const& options, rti::CallbackModel model);
void scenarioRestorePendingTimeAdvanceContract(
    Options const& options, rti::CallbackModel model);
void scenarioRestorePendingTimeAdvanceImmediate(
    Options const& options, rti::CallbackModel model);
void scenarioRestorePendingTimeAdvanceImmediateContract(
    Options const& options, rti::CallbackModel model);

}  // namespace hla_rti_cpp_tck::detail
