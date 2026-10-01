#pragma once

#include <cstddef>
#include <cstdint>

namespace umbra {
namespace detail {

// DataElement::hash uses the same byte-level FNV-1a contract in both public
// binding streams.  Keep the primitive independent of either RTI type so the
// 2010 and 2025 implementations can share it without sharing their API code.
[[nodiscard]] inline std::uint64_t fnv1aHash(
    void const *data,
    std::size_t size) noexcept {
  auto const *bytes = static_cast<std::uint8_t const *>(data);
  std::uint64_t hash = 14695981039346656037ULL;
  for (std::size_t index = 0U; index < size; ++index) {
    hash ^= static_cast<std::uint64_t>(bytes[index]);
    hash *= 1099511628211ULL;
  }
  return hash;
}

}  // namespace detail
}  // namespace umbra
