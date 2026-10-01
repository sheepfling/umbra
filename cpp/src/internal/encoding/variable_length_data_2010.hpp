#pragma once

#include <RTI/VariableLengthData.h>
#include <RTI/encoding/EncodingExceptions.h>

#include <vector>

namespace umbra {
namespace detail {
namespace variable_length_data_2010 {

// The 2010 composite decoders copy public encoded values before parsing them.
// Keep this ownership and pointer-validation operation inside the 2010
// binding; it must not be shared with the 2025 public-type stream.
[[nodiscard]] inline std::vector<rti1516e::Octet> copyEncodedBytes(
    rti1516e::VariableLengthData const &value,
    wchar_t const *invalidDataMessage) {
  auto const *bytes = static_cast<rti1516e::Octet const *>(value.data());
  if (value.size() != 0U && bytes == nullptr) {
    throw rti1516e::EncoderException(invalidDataMessage);
  }
  return bytes == nullptr ? std::vector<rti1516e::Octet>{}
                          : std::vector<rti1516e::Octet>(
                                bytes, bytes + value.size());
}

}  // namespace variable_length_data_2010
}  // namespace detail
}  // namespace umbra
