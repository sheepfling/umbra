#pragma once

#include <RTI/VariableLengthData.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <utility>

namespace rti1516e {
namespace umbra_binding_detail {

// The standard MIM represents every public handle as an HLAvariableArray of
// HLAbyte.  The reference 2010 provider uses an eight-octet implementation
// identity, with the four-octet variable-array element count in front.
static const std::size_t kUmbraHandleIdentityOctetCount2010 = 8U;
static const std::size_t kHlaVariableArrayCountOctetCount2010 = 4U;
static const std::size_t kUmbraHandleVariableArrayEncodedLength2010 =
    kHlaVariableArrayCountOctetCount2010 + kUmbraHandleIdentityOctetCount2010;

[[nodiscard]] inline std::array<unsigned char, kUmbraHandleVariableArrayEncodedLength2010>
encodeUmbraHandleVariableArray2010(std::uint64_t value) noexcept {
  std::array<unsigned char, kUmbraHandleVariableArrayEncodedLength2010> encoded{};
  encoded[3] = static_cast<unsigned char>(kUmbraHandleIdentityOctetCount2010);
  for (std::size_t index = 0; index < kUmbraHandleIdentityOctetCount2010; ++index) {
    std::size_t const shift = (kUmbraHandleIdentityOctetCount2010 - index - 1U) * 8U;
    encoded[kHlaVariableArrayCountOctetCount2010 + index] =
        static_cast<unsigned char>(value >> shift);
  }
  return encoded;
}

[[nodiscard]] inline std::pair<bool, std::uint64_t> decodeUmbraHandleVariableArray2010(
    VariableLengthData const& encodedValue) noexcept {
  if (encodedValue.size() != kUmbraHandleVariableArrayEncodedLength2010 ||
      encodedValue.data() == nullptr) {
    return std::make_pair(false, 0U);
  }
  auto const* bytes = static_cast<unsigned char const*>(encodedValue.data());
  if (bytes[0] != 0U || bytes[1] != 0U || bytes[2] != 0U ||
      bytes[3] != kUmbraHandleIdentityOctetCount2010) {
    return std::make_pair(false, 0U);
  }
  std::uint64_t value = 0;
  for (std::size_t index = 0; index < kUmbraHandleIdentityOctetCount2010; ++index) {
    value = (value << 8U) |
        bytes[kHlaVariableArrayCountOctetCount2010 + index];
  }
  return std::make_pair(true, value);
}

}  // namespace umbra_binding_detail
}  // namespace rti1516e
