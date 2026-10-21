#pragma once

#include "tck_main_scenarios.hpp"

namespace hla_rti_cpp_tck::detail {

void scenarioRestoreTerminalTsoAttribute(Options const& options, rti::CallbackModel model);
void scenarioRestoreTerminalTsoAttributeContract(
    Options const& options, rti::CallbackModel model);

}  // namespace hla_rti_cpp_tck::detail
