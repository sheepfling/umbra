#pragma once

#include <RTI/VariableLengthData.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>

namespace rti1516_2025::umbra_binding_detail {

// The standard MIM declares every public RTI handle value as an
// HLAvariableArray of HLAbyte.  An Umbra handle identity has a fixed,
// implementation-owned eight-octet payload, but Handle::encode() is used
// directly as an attribute or parameter value.  Its public wire value must
// therefore include the HLAvariableArray's HLAinteger32BE element count.
inline constexpr std::size_t kUmbraHandleIdentityOctetCount = 8U;
inline constexpr std::size_t kHlaVariableArrayCountOctetCount = 4U;
inline constexpr std::size_t kUmbraHandleVariableArrayEncodedLength =
    kHlaVariableArrayCountOctetCount + kUmbraHandleIdentityOctetCount;

[[nodiscard]] inline std::array<unsigned char, kUmbraHandleVariableArrayEncodedLength>
encodeUmbraHandleVariableArray(std::uint64_t value) noexcept {
  std::array<unsigned char, kUmbraHandleVariableArrayEncodedLength> encoded{};
  // HLAvariableArray begins with a signed HLAinteger32BE element count.  The
  // eight octets of the implementation-owned identity are HLAbyte elements.
  encoded[3] = static_cast<unsigned char>(kUmbraHandleIdentityOctetCount);
  for (std::size_t index = 0; index < kUmbraHandleIdentityOctetCount; ++index) {
    std::size_t const shift = (kUmbraHandleIdentityOctetCount - index - 1U) * 8U;
    encoded[kHlaVariableArrayCountOctetCount + index] =
        static_cast<unsigned char>(value >> shift);
  }
  return encoded;
}

[[nodiscard]] inline std::optional<std::uint64_t> decodeUmbraHandleVariableArray(
    VariableLengthData const& encodedValue) noexcept {
  if (encodedValue.size() != kUmbraHandleVariableArrayEncodedLength ||
      encodedValue.data() == nullptr) {
    return std::nullopt;
  }

  auto const* bytes = static_cast<unsigned char const*>(encodedValue.data());
  if (bytes[0] != 0U || bytes[1] != 0U || bytes[2] != 0U ||
      bytes[3] != kUmbraHandleIdentityOctetCount) {
    return std::nullopt;
  }

  std::uint64_t value = 0;
  for (std::size_t index = 0; index < kUmbraHandleIdentityOctetCount; ++index) {
    value = (value << 8U) |
        bytes[kHlaVariableArrayCountOctetCount + index];
  }
  return value;
}

}  // namespace rti1516_2025::umbra_binding_detail
