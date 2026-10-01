#pragma once

#include "internal/encoding/handle_variable_array.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <utility>

namespace rti1516e {
namespace umbra_binding_detail {

// The standard MIM represents every public handle as an HLAvariableArray of
// HLAbyte.  The reference 2010 provider uses an eight-octet implementation
// identity, with the four-octet variable-array element count in front.
static const std::size_t kUmbraHandleIdentityOctetCount2010 =
    umbra::detail::kHandleIdentityOctetCount;
static const std::size_t kHlaVariableArrayCountOctetCount2010 =
    umbra::detail::kVariableArrayCountOctetCount;
static const std::size_t kUmbraHandleVariableArrayEncodedLength2010 =
    umbra::detail::kHandleVariableArrayEncodedLength;

[[nodiscard]] inline std::array<unsigned char, kUmbraHandleVariableArrayEncodedLength2010>
encodeUmbraHandleVariableArray2010(std::uint64_t value) noexcept {
  return umbra::detail::encodeHandleVariableArray(value);
}

[[nodiscard]] inline std::pair<bool, std::uint64_t> decodeUmbraHandleVariableArray2010(
    VariableLengthData const& encodedValue) noexcept {
  auto const result = umbra::detail::decodeHandleVariableArray(encodedValue);
  if (!result.valid) {
    return std::make_pair(false, 0U);
  }
  return std::make_pair(true, result.value);
}

}  // namespace umbra_binding_detail
}  // namespace rti1516e
