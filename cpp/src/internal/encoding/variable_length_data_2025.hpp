#pragma once

#include <RTI/VariableLengthData.h>
#include <RTI/encoding/EncodingExceptions.h>

#include <cstdint>
#include <vector>

namespace umbra {
namespace detail {
namespace variable_length_data_2025 {

// 2025 process and runtime paths copy encoded public values into owned byte
// buffers before crossing an asynchronous or process boundary.  Keep that
// lifetime operation in one place, while leaving the 2010 binding untouched.
inline std::vector<std::uint8_t> copyBytes(
    rti1516_2025::VariableLengthData const &value) {
  auto const *bytes = static_cast<std::uint8_t const *>(value.data());
  if (bytes == nullptr || value.size() == 0U) {
    return {};
  }
  return {bytes, bytes + value.size()};
}

// 2025 encoders use the same ownership operation but must retain their
// public-type-specific invalid-buffer diagnostic.  The message remains at
// each binding call site while the pointer validation and copy stay shared.
inline void validateEncodedBytes(
    rti1516_2025::VariableLengthData const &value,
    wchar_t const *invalidDataMessage) {
  auto const *bytes =
      static_cast<rti1516_2025::Octet const *>(value.data());
  if (value.size() != 0U && bytes == nullptr) {
    throw rti1516_2025::EncoderException(invalidDataMessage);
  }
}

inline void appendEncodedBytes(
    std::vector<rti1516_2025::Octet> &output,
    rti1516_2025::VariableLengthData const &value,
    wchar_t const *invalidDataMessage) {
  validateEncodedBytes(value, invalidDataMessage);
  auto const *bytes =
      static_cast<rti1516_2025::Octet const *>(value.data());
  if (bytes != nullptr) {
    output.insert(output.end(), bytes, bytes + value.size());
  }
}

[[nodiscard]] inline std::vector<rti1516_2025::Octet> copyEncodedBytes(
    rti1516_2025::VariableLengthData const &value,
    wchar_t const *invalidDataMessage) {
  validateEncodedBytes(value, invalidDataMessage);
  auto const *bytes = static_cast<rti1516_2025::Octet const *>(value.data());
  return bytes == nullptr ? std::vector<rti1516_2025::Octet>{}
                          : std::vector<rti1516_2025::Octet>(
                                bytes, bytes + value.size());
}

}  // namespace variable_length_data_2025
}  // namespace detail
}  // namespace umbra
