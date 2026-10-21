#pragma once

#include "tck_harness.hpp"

namespace hla_rti_cpp_tck::detail {

void scenarioOwnershipMixedIfAvailable(Options const&, rti::CallbackModel);
void scenarioOwnershipMixedIfAvailableContract(Options const&, rti::CallbackModel);

}  // namespace hla_rti_cpp_tck::detail
