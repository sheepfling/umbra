#pragma once

#include "tck_harness.hpp"

namespace hla_rti_cpp_tck::detail {

void scenarioTaraFutureTimestampedInteraction(
    Options const&, rti::CallbackModel);
void scenarioTaraFutureTimestampedInteractionContract(
    Options const&, rti::CallbackModel);

}  // namespace hla_rti_cpp_tck::detail
