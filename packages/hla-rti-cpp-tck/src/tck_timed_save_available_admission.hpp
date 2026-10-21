#pragma once

#include "tck_main_scenarios.hpp"

namespace hla_rti_cpp_tck::detail {

void scenarioTimedSaveAvailableAdmission(
    Options const& options, rti::CallbackModel model);
void scenarioTimedSaveAvailableAdmissionContract(
    Options const& options, rti::CallbackModel model);

}  // namespace hla_rti_cpp_tck::detail
