#pragma once

#include "internal/fom/fom_wire_codec.hpp"
#include "internal/fom/fom_wire_encoding.hpp"

#include <cstdint>

namespace umbra::detail {

[[nodiscard]] inline std::uint32_t rprUnsignedIntegerSizeBits(
    std::string_view sourceLabel) noexcept {
  if (sourceLabel == "RPRunsignedInteger8BE") {
    return 8U;
  }
  if (sourceLabel == "RPRunsignedInteger16BE") {
    return 16U;
  }
  if (sourceLabel == "RPRunsignedInteger32BE") {
    return 32U;
  }
  if (sourceLabel == "RPRunsignedInteger64BE") {
    return 64U;
  }
  return 0U;
}

[[nodiscard]] inline std::string_view rprUnsignedIntegerSourceEncoding(
    std::string_view typeName) noexcept {
  if (typeName == "RPRunsignedInteger8BE") {
    return "8-bit unsigned integer.";
  }
  if (typeName == "RPRunsignedInteger16BE") {
    return "16-bit unsigned integer.";
  }
  if (typeName == "RPRunsignedInteger32BE") {
    return "32-bit unsigned integer.";
  }
  if (typeName == "RPRunsignedInteger64BE") {
    return "64-bit unsigned integer.";
  }
  return {};
}

// This is the only adapter that promotes RPR structural metadata to a
// byte-codec claim. Keep it in the validation/composition dependency cone so
// standard RTI translation units can consume the neutral descriptor without
// importing RPR codec implementation details.
[[nodiscard]] inline FomWireEncodingDescriptor normalizeRprFomWireEncoding(
    std::string_view sourceLabel) {
  auto result = normalizeFomWireEncoding(
      sourceLabel,
      FomSourceCompatibility::rpr_2010);
  if (rprWireCodecAvailable(sourceLabel)) {
    result.codecStatus = FomWireCodecStatus::available;
  }
  return result;
}

// RPR basic-data declarations carry a prose <encoding> plus explicit size and
// endian fields. Promote the codec only when all of those fields agree with
// one of the four exact RPRunsignedInteger*BE names; malformed or merely
// similar third-party declarations remain metadata-only.
[[nodiscard]] inline FomWireEncodingDescriptor
normalizeRprBasicFomWireEncoding(
    std::string_view typeName,
    std::string_view sourceLabel,
    std::uint32_t sizeBits,
    FomByteOrder byteOrder) {
  auto result = makeBasicFomWireEncoding(sourceLabel, sizeBits, byteOrder);
  auto const expectedSize = rprUnsignedIntegerSizeBits(typeName);
  auto const expectedEncoding = rprUnsignedIntegerSourceEncoding(typeName);
  if (expectedSize != 0U &&
      rprUnsignedIntegerWireCodecAvailable(typeName) &&
      sourceLabel == expectedEncoding && sizeBits == expectedSize &&
      byteOrder == FomByteOrder::big) {
    result.codecStatus = FomWireCodecStatus::available;
  }
  return result;
}

// RPR also uses the custom unsigned representations for simple and enumerated
// data declarations. Those declarations have no <size>/<endian> children, so
// the width is derived only from the exact representation name.
[[nodiscard]] inline FomWireEncodingDescriptor
normalizeRprUnsignedIntegerFomWireEncoding(std::string_view sourceLabel) {
  auto const sizeBits = rprUnsignedIntegerSizeBits(sourceLabel);
  if (sizeBits == 0U) {
    return normalizeRprFomWireEncoding(sourceLabel);
  }
  auto result = makeBasicFomWireEncoding(
      sourceLabel,
      sizeBits,
      FomByteOrder::big);
  if (rprUnsignedIntegerWireCodecAvailable(sourceLabel)) {
    result.codecStatus = FomWireCodecStatus::available;
  }
  return result;
}

}  // namespace umbra::detail
