#pragma once

#include "tck_main_scenarios.hpp"

namespace hla_rti_cpp_tck::detail {

void scenarioRestorePendingAdvanceModes(
    Options const& options, rti::CallbackModel model);
void scenarioRestorePendingAdvanceModesContract(
    Options const& options, rti::CallbackModel model);

}  // namespace hla_rti_cpp_tck::detail
