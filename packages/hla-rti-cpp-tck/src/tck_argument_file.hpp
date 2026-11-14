#pragma once

#include <string>
#include <vector>

namespace hla_rti_cpp_tck::detail {

[[nodiscard]] std::vector<std::string> expandArgumentFiles(
    int argc,
    char** argv);

}  // namespace hla_rti_cpp_tck::detail
