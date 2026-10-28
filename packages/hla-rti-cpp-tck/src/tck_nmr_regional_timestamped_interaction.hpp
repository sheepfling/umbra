#pragma once

#include "tck_main_scenarios.hpp"

namespace hla_rti_cpp_tck::detail {

void scenarioNmrRegionalTimestampedInteractionMessageSet(
    Options const& options,
    rti::CallbackModel model);
void scenarioNmrRegionalTimestampedInteractionMessageSetContract(
    Options const& options,
    rti::CallbackModel model);

}  // namespace hla_rti_cpp_tck::detail
