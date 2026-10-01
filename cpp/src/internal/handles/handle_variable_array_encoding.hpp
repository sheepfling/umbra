#pragma once

#include "internal/encoding/handle_variable_array.hpp"

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
    umbra::detail::kHandleVariableArrayEncodedLength;

[[nodiscard]] inline std::array<unsigned char, kUmbraHandleVariableArrayEncodedLength>
encodeUmbraHandleVariableArray(std::uint64_t value) noexcept {
  return umbra::detail::encodeHandleVariableArray(value);
}

[[nodiscard]] inline std::optional<std::uint64_t> decodeUmbraHandleVariableArray(
    VariableLengthData const& encodedValue) noexcept {
  auto const result = umbra::detail::decodeHandleVariableArray(encodedValue);
  if (!result.valid) {
    return std::nullopt;
  }
  return result.value;
}

}  // namespace rti1516_2025::umbra_binding_detail
