#pragma once

#include "tck_portable_scenario_runner.hpp"

#include <optional>

namespace hla_rti_cpp_tck::detail {

[[nodiscard]] std::optional<int> tryRunTimestampedTsoMessageSetScenario(
    int argc,
    char** argv);

}  // namespace hla_rti_cpp_tck::detail
