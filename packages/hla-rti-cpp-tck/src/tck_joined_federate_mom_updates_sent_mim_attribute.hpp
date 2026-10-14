#pragma once

#include "tck_harness.hpp"

namespace hla_rti_cpp_tck::detail {

void scenarioJoinedFederateMomUpdatesSentAttribute(
    Options const& options,
    rti::CallbackModel model);
void scenarioJoinedFederateMomUpdatesSentAttributeContract(
    Options const& options,
    rti::CallbackModel model);

}  // namespace hla_rti_cpp_tck::detail
