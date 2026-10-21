#pragma once

#include "tck_main_scenarios.hpp"

namespace hla_rti_cpp_tck::detail {

void scenarioTimedSaveOrdinaryBeforeFqr(
    Options const& options, rti::CallbackModel model);
void scenarioTimedSaveOrdinaryBeforeFqrContract(
    Options const& options, rti::CallbackModel model);

}  // namespace hla_rti_cpp_tck::detail
