#pragma once

#include "tck_harness.hpp"

namespace hla_rti_cpp_tck::detail {

void scenarioReceiveOrderAttributeUpdate(
    Options const& options,
    rti::CallbackModel model);
void scenarioReceiveOrderAttributeUpdateContract(
    Options const& options,
    rti::CallbackModel model);

}  // namespace hla_rti_cpp_tck::detail
