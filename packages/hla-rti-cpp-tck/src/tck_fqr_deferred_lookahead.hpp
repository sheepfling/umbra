#pragma once

#include "tck_main_scenarios.hpp"

namespace hla_rti_cpp_tck::detail {

void scenarioFqrDeferredLookahead(Options const& options, rti::CallbackModel model);
void scenarioFqrDeferredLookaheadContract(Options const& options, rti::CallbackModel model);

}  // namespace hla_rti_cpp_tck::detail
