#pragma once

#include "internal/encoding/byte_order.hpp"

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace umbra {
namespace detail {
namespace transport_wire {

inline void appendBigEndian16(
    std::vector<std::uint8_t> &bytes,
    std::uint16_t value) {
  appendUnsigned(bytes, value, ByteOrder::big);
}

inline void appendBigEndian32(
    std::vector<std::uint8_t> &bytes,
    std::uint32_t value) {
  appendUnsigned(bytes, value, ByteOrder::big);
}

inline void appendBigEndian64(
    std::vector<std::uint8_t> &bytes,
    std::uint64_t value) {
  appendUnsigned(bytes, value, ByteOrder::big);
}

template <typename Unsigned>
[[nodiscard]] Unsigned readBigEndian(
    std::span<std::uint8_t const> bytes,
    std::size_t offset) noexcept {
  Unsigned value = 0U;
  if (offset > bytes.size() || bytes.size() - offset < sizeof(Unsigned)) {
    return value;
  }
  static_cast<void>(readUnsigned(
      static_cast<void const *>(bytes.data() + offset),
      sizeof(Unsigned),
      ByteOrder::big,
      value));
  return value;
}

inline std::uint16_t readBigEndian16(
    std::span<std::uint8_t const> bytes,
    std::size_t offset) noexcept {
  return readBigEndian<std::uint16_t>(bytes, offset);
}

inline std::uint32_t readBigEndian32(
    std::span<std::uint8_t const> bytes,
    std::size_t offset) noexcept {
  return readBigEndian<std::uint32_t>(bytes, offset);
}

inline std::uint64_t readBigEndian64(
    std::span<std::uint8_t const> bytes,
    std::size_t offset) noexcept {
  return readBigEndian<std::uint64_t>(bytes, offset);
}

}  // namespace transport_wire
}  // namespace detail
}  // namespace umbra
