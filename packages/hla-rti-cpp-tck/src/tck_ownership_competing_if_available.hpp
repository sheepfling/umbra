#pragma once

#include "tck_harness.hpp"

namespace hla_rti_cpp_tck::detail {

void scenarioOwnershipCompetingIfAvailable(Options const&, rti::CallbackModel);
void scenarioOwnershipCompetingIfAvailableContract(Options const&, rti::CallbackModel);

}  // namespace hla_rti_cpp_tck::detail
