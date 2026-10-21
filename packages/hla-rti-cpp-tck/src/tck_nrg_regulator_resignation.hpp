#pragma once

#include "tck_main_scenarios.hpp"

namespace hla_rti_cpp_tck::detail {

void scenarioNrgRegulatorResignation(Options const&, rti::CallbackModel);
void scenarioNrgRegulatorResignationContract(Options const&, rti::CallbackModel);
void scenarioNrgRegulatorDisable(Options const&, rti::CallbackModel);
void scenarioNrgRegulatorDisableContract(Options const&, rti::CallbackModel);

}  // namespace hla_rti_cpp_tck::detail
