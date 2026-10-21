#pragma once

#include "tck_harness.hpp"

namespace hla_rti_cpp_tck::detail {

void scenarioRestoreMixedPendingTimeRoleCallbacks(
    Options const& options, rti::CallbackModel model);
void scenarioRestoreMixedPendingTimeRoleCallbacksContract(
    Options const& options, rti::CallbackModel model);

}  // namespace hla_rti_cpp_tck::detail
