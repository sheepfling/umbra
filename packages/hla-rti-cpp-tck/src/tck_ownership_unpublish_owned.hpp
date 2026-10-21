#pragma once

#include "tck_harness.hpp"

namespace hla_rti_cpp_tck::detail {

void scenarioOwnershipUnpublishOwned(Options const&, rti::CallbackModel);
void scenarioOwnershipUnpublishOwnedContract(Options const&, rti::CallbackModel);

}  // namespace hla_rti_cpp_tck::detail
