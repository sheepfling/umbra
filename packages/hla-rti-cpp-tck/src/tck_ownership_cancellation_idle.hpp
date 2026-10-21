#pragma once

#include "tck_harness.hpp"

namespace hla_rti_cpp_tck::detail {

void scenarioOwnershipCancellationIdle(Options const&, rti::CallbackModel);
void scenarioOwnershipCancellationIdleContract(Options const&, rti::CallbackModel);

}  // namespace hla_rti_cpp_tck::detail
