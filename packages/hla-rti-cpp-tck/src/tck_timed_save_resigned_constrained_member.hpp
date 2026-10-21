#pragma once

#include "tck_main_scenarios.hpp"

namespace hla_rti_cpp_tck::detail {

void scenarioTimedSaveResignedConstrainedMember(
    Options const& options, rti::CallbackModel model);
void scenarioTimedSaveResignedConstrainedMemberContract(
    Options const& options, rti::CallbackModel model);
void scenarioTimedSaveResignedPendingFqr(
    Options const& options, rti::CallbackModel model);
void scenarioTimedSaveResignedPendingFqrContract(
    Options const& options, rti::CallbackModel model);

}  // namespace hla_rti_cpp_tck::detail
