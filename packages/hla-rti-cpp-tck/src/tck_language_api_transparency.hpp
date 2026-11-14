#pragma once

#include "tck_options.hpp"

#include <RTI/RTI1516.h>

namespace hla_rti_cpp_tck::detail {

void scenarioLanguageApiTransparency(
    Options const& options,
    rti1516_2025::CallbackModel callbackModel);

}  // namespace hla_rti_cpp_tck::detail
