#pragma once

#include "internal/encoding/byte_order.hpp"

#include <RTI/VariableLengthData.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>

namespace umbra {
namespace detail {

constexpr std::size_t kHandleIdentityOctetCount = 8U;
constexpr std::size_t kVariableArrayCountOctetCount = 4U;
constexpr std::size_t kHandleVariableArrayEncodedLength =
    kVariableArrayCountOctetCount + kHandleIdentityOctetCount;

struct HandleVariableArrayDecodeResult {
  bool valid = false;
  std::uint64_t value = 0U;
};

inline std::array<unsigned char, kHandleVariableArrayEncodedLength>
encodeHandleVariableArray(std::uint64_t value) noexcept {
  std::array<unsigned char, kHandleVariableArrayEncodedLength> encoded{};
  auto const count = encodeUnsigned(
      static_cast<std::uint32_t>(kHandleIdentityOctetCount), ByteOrder::big);
  auto const identity = encodeUnsigned(value, ByteOrder::big);
  std::memcpy(encoded.data(), count.data(), count.size());
  std::memcpy(encoded.data() + count.size(), identity.data(), identity.size());
  return encoded;
}

template <typename VariableLengthData>
HandleVariableArrayDecodeResult decodeHandleVariableArray(
    VariableLengthData const &encodedValue) noexcept {
  if (encodedValue.size() != kHandleVariableArrayEncodedLength ||
      encodedValue.data() == nullptr) {
    return {};
  }

  auto const *bytes = static_cast<unsigned char const *>(encodedValue.data());
  std::uint32_t count = 0U;
  if (!readUnsigned(bytes, kVariableArrayCountOctetCount, ByteOrder::big, count) ||
      count != kHandleIdentityOctetCount) {
    return {};
  }

  std::uint64_t value = 0U;
  if (!readUnsigned(
          bytes + kVariableArrayCountOctetCount,
          kHandleIdentityOctetCount,
          ByteOrder::big,
          value)) {
    return {};
  }
  return {true, value};
}

}  // namespace detail
}  // namespace umbra
