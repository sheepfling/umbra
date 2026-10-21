#pragma once

#include <RTI/Enums.h>

#include "tck_options.hpp"

namespace hla_rti_cpp_tck::detail {

void scenarioFlushQueueNonlimitingGalt(Options const&, rti1516_2025::CallbackModel);
void scenarioFlushQueueNonlimitingGaltContract(
    Options const&, rti1516_2025::CallbackModel);
void scenarioFlushQueueNonlimitingGaltPortable(
    Options const&, rti1516_2025::CallbackModel);
void scenarioFlushQueueNonlimitingGaltContractPortable(
    Options const&, rti1516_2025::CallbackModel);

}  // namespace hla_rti_cpp_tck::detail
