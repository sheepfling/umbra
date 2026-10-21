#pragma once

#include <optional>

namespace hla_rti_cpp_tck::detail {

[[nodiscard]] std::optional<int> tryRunTimestampedAttributeTaraScenario(
    int argc,
    char** argv);

}  // namespace hla_rti_cpp_tck::detail
