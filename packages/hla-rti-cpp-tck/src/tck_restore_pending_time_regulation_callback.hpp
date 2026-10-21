#pragma once

#include "tck_main_scenarios.hpp"

namespace hla_rti_cpp_tck::detail {

void scenarioRestorePendingTimeRegulationCallback(
    Options const& options, rti::CallbackModel model);
void scenarioRestorePendingTimeRegulationCallbackContract(
    Options const& options, rti::CallbackModel model);
void scenarioRestorePendingTimeConstrainedCallback(
    Options const& options, rti::CallbackModel model);
void scenarioRestorePendingTimeConstrainedCallbackContract(
    Options const& options, rti::CallbackModel model);

}  // namespace hla_rti_cpp_tck::detail
