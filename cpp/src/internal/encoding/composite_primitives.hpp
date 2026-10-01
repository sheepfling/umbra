#pragma once

#include <cstddef>
#include <limits>
#include <vector>

namespace umbra {
namespace detail {
namespace composite_primitives {

// These operations are independent of either RTI binding.  The public
// 2010/2025 composite adapters import them into separate version namespaces.
[[nodiscard]] inline bool tryCheckedAdd(
    std::size_t left,
    std::size_t right,
    std::size_t &result) noexcept {
  if (right > (std::numeric_limits<std::size_t>::max)() - left) {
    return false;
  }
  result = left + right;
  return true;
}

[[nodiscard]] inline bool tryPaddingToBoundary(
    std::size_t encodedLength,
    unsigned int boundary,
    std::size_t &result) noexcept {
  if (boundary == 0U) {
    return false;
  }
  auto const divisor = static_cast<std::size_t>(boundary);
  auto const remainder = encodedLength % divisor;
  result = remainder == 0U ? 0U : divisor - remainder;
  return true;
}

template <typename Byte>
void appendZeroPadding(std::vector<Byte> &bytes, std::size_t count) {
  bytes.insert(bytes.end(), count, static_cast<Byte>(0));
}

template <typename Bytes>
[[nodiscard]] inline bool hasRange(
    Bytes const &bytes,
    std::size_t index,
    std::size_t count) noexcept {
  return index <= bytes.size() && count <= bytes.size() - index;
}

template <typename Bytes>
[[nodiscard]] inline bool isZeroPadding(
    Bytes const &bytes,
    std::size_t index,
    std::size_t count) noexcept {
  if (!hasRange(bytes, index, count)) {
    return false;
  }
  for (std::size_t offset = 0U; offset < count; ++offset) {
    if (bytes[index + offset] != 0) {
      return false;
    }
  }
  return true;
}

}  // namespace composite_primitives
}  // namespace detail
}  // namespace umbra
