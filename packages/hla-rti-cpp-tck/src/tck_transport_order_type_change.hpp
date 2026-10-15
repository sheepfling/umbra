#pragma once

#include "tck_harness.hpp"

namespace hla_rti_cpp_tck::detail {

void scenarioTransportationTypeChange(Options const&, rti::CallbackModel);
void scenarioTransportationTypeChangeContract(Options const&, rti::CallbackModel);
void scenarioOrderTypeChange(Options const&, rti::CallbackModel);
void scenarioOrderTypeChangeContract(Options const&, rti::CallbackModel);

}  // namespace hla_rti_cpp_tck::detail
