#pragma once

#include "tck_harness.hpp"

namespace hla_rti_cpp_tck::detail {

void scenarioTransportOrderPortable(Options const&, rti::CallbackModel);
void scenarioTransportOrderPortableContract(Options const&, rti::CallbackModel);
void scenarioStandardOrderAndTransportationLookupsPortable(Options const&, rti::CallbackModel);
void scenarioStandardOrderAndTransportationLookupsPortableContract(Options const&, rti::CallbackModel);

}  // namespace hla_rti_cpp_tck::detail
