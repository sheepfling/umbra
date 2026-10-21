#pragma once

#include "tck_main_scenarios.hpp"

namespace hla_rti_cpp_tck::detail {

void scenarioSaveActiveCallbackResignation(
    Options const& options, rti::CallbackModel model);
void scenarioSaveActiveCallbackResignationContract(
    Options const& options, rti::CallbackModel model);
void scenarioTimedSaveActiveCallbackResignation(
    Options const& options, rti::CallbackModel model);
void scenarioTimedSaveActiveCallbackResignationContract(
    Options const& options, rti::CallbackModel model);

}  // namespace hla_rti_cpp_tck::detail
