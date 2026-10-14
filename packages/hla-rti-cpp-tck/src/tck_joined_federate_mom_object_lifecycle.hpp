#pragma once

#include "tck_harness.hpp"

namespace hla_rti_cpp_tck::detail {

void scenarioJoinedFederateMomObjectLifecycle(
    Options const& options,
    rti::CallbackModel model);
void scenarioJoinedFederateMomObjectLifecycleContract(
    Options const& options,
    rti::CallbackModel model);

}  // namespace hla_rti_cpp_tck::detail
