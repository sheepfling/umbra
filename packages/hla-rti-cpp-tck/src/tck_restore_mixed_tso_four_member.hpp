#pragma once

#include "tck_main_scenarios.hpp"

namespace hla_rti_cpp_tck::detail {

void scenarioRestoreMixedTsoFourMember(
    Options const& options, rti::CallbackModel model);
void scenarioRestoreMixedTsoFourMemberContract(
    Options const& options, rti::CallbackModel model);

}  // namespace hla_rti_cpp_tck::detail
