#pragma once

#include "tck_harness.hpp"

namespace hla_rti_cpp_tck::detail {

void scenarioFederateSelectedUpdateThresholds(Options const&, rti::CallbackModel);
void scenarioFederateSelectedUpdateThresholdsContract(
    Options const&, rti::CallbackModel);

}  // namespace hla_rti_cpp_tck::detail
