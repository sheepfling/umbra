#pragma once

#include "tck_main_scenarios.hpp"

namespace hla_rti_cpp_tck::detail {

void scenarioTimedSaveFqrCallbackResignation(
    Options const& options, rti::CallbackModel model);
void scenarioTimedSaveFqrCallbackResignationContract(
    Options const& options, rti::CallbackModel model);

}  // namespace hla_rti_cpp_tck::detail
