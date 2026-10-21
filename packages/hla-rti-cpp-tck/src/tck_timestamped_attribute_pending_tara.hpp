#pragma once

#include "tck_harness.hpp"

namespace hla_rti_cpp_tck::detail {

void scenarioTaraFutureTimestampedAttributeUpdate(
    Options const& options,
    rti::CallbackModel model);
void scenarioTaraFutureTimestampedAttributeUpdateContract(
    Options const& options,
    rti::CallbackModel model);

}  // namespace hla_rti_cpp_tck::detail
