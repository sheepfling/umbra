#pragma once

#include <cstdint>
#include <set>
#include <vector>

namespace umbra::detail::process_federation_payload {

inline std::vector<std::uint64_t> parameterVector(
    std::set<std::uint64_t> const& handles) {
  return {handles.begin(), handles.end()};
}

}  // namespace umbra::detail::process_federation_payload
