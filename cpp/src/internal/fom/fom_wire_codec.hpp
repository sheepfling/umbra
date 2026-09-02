#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <iterator>
#include <limits>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace umbra::detail {

// The FOM catalog is intentionally independent from the public RTI
// DataElement hierarchy. These small byte-level helpers are the legacy RPR
// adapter's codec seam: callers provide child-type encoders/decoders and
// enclosing bounds where the RPR wire format does not carry them.
using FomWireOctet = std::uint8_t;
using FomWireBytes = std::vector<FomWireOctet>;

class FomWireCodecError final : public std::runtime_error {
 public:
  using std::runtime_error::runtime_error;
};

[[nodiscard]] inline bool rprWireCodecAvailable(
    std::string_view sourceLabel) noexcept {
  return sourceLabel == "RPRnullTerminatedArray" ||
      sourceLabel == "RPRlengthlessArray" ||
      sourceLabel == "RPRpaddingTo32Array" ||
      sourceLabel == "RPRpaddingTo64Array" ||
      sourceLabel == "RPRextendedVariantRecord";
}

// RPR names these four primitive representations explicitly rather than using
// the standard HLAinteger*BE vocabulary.  Keep the recognizer here with the
// opt-in RPR codec; the neutral catalog header must not learn these labels.
[[nodiscard]] inline bool rprUnsignedIntegerWireCodecAvailable(
    std::string_view sourceLabel) noexcept {
  return sourceLabel == "RPRunsignedInteger8BE" ||
      sourceLabel == "RPRunsignedInteger16BE" ||
      sourceLabel == "RPRunsignedInteger32BE" ||
      sourceLabel == "RPRunsignedInteger64BE";
}

struct FomWireDecodeResult {
  FomWireBytes value;
  std::size_t nextOffset = 0U;
};

using FomWireElementDecoder = std::function<std::size_t(
    std::span<const FomWireOctet> bytes,
    std::size_t offset,
    std::size_t endOffset)>;

struct FomWireLengthlessArrayDecodeResult {
  std::vector<FomWireBytes> elements;
  std::size_t nextOffset = 0U;
};

struct FomRprExtendedVariantRecordDecodeResult {
  FomWireBytes discriminant;
  FomWireBytes alternative;
  std::size_t nextOffset = 0U;
};

struct FomRprUnsignedIntegerDecodeResult {
  std::uint64_t value = 0U;
  std::size_t nextOffset = 0U;
};

namespace fom_wire_codec_detail {

[[noreturn]] inline void fail(std::string_view message) {
  throw FomWireCodecError(std::string(message));
}

[[nodiscard]] inline std::size_t checkedAdd(
    std::size_t left,
    std::size_t right,
    std::string_view message) {
  if (right > std::numeric_limits<std::size_t>::max() - left) {
    fail(message);
  }
  return left + right;
}

inline void requireRange(
    std::span<const FomWireOctet> bytes,
    std::size_t offset,
    std::size_t length,
    std::string_view message) {
  if (offset > bytes.size() || length > bytes.size() - offset) {
    fail(message);
  }
}

[[nodiscard]] inline std::size_t paddingToBoundary(
    std::size_t offset,
    std::uint32_t alignmentOctets) {
  if (alignmentOctets == 0U) {
    fail("The RPR alignment boundary must be nonzero.");
  }
  auto const alignment = static_cast<std::size_t>(alignmentOctets);
  auto const remainder = offset % alignment;
  return remainder == 0U ? 0U : alignment - remainder;
}

inline void requireZeroPadding(
    std::span<const FomWireOctet> bytes,
    std::size_t offset,
    std::size_t length,
    std::string_view message) {
  requireRange(bytes, offset, length, message);
  for (std::size_t index = 0U; index < length; ++index) {
    if (bytes[offset + index] != 0U) {
      fail(message);
    }
  }
}

inline void appendZeroPadding(FomWireBytes& bytes, std::size_t length) {
  bytes.insert(bytes.end(), length, static_cast<FomWireOctet>(0U));
}

inline void appendUnsigned32BigEndian(FomWireBytes& bytes, std::uint32_t value) {
  bytes.push_back(static_cast<FomWireOctet>((value >> 24U) & 0xffU));
  bytes.push_back(static_cast<FomWireOctet>((value >> 16U) & 0xffU));
  bytes.push_back(static_cast<FomWireOctet>((value >> 8U) & 0xffU));
  bytes.push_back(static_cast<FomWireOctet>(value & 0xffU));
}

[[nodiscard]] inline std::size_t rprUnsignedIntegerOctetCount(
    std::uint32_t sizeBits) {
  switch (sizeBits) {
    case 8U:
    case 16U:
    case 32U:
    case 64U:
      return static_cast<std::size_t>(sizeBits / 8U);
    default:
      fail(
          "RPR unsigned integer codecs require an 8, 16, 32, or 64-bit "
          "width.");
  }
}

[[nodiscard]] inline std::uint64_t rprUnsignedIntegerMaximum(
    std::uint32_t sizeBits) {
  switch (sizeBits) {
    case 8U:
      return std::numeric_limits<std::uint8_t>::max();
    case 16U:
      return std::numeric_limits<std::uint16_t>::max();
    case 32U:
      return std::numeric_limits<std::uint32_t>::max();
    case 64U:
      return std::numeric_limits<std::uint64_t>::max();
    default:
      static_cast<void>(rprUnsignedIntegerOctetCount(sizeBits));
      return 0U;
  }
}

[[nodiscard]] inline std::uint32_t readUnsigned32BigEndian(
    std::span<const FomWireOctet> bytes,
    std::size_t offset) {
  return (static_cast<std::uint32_t>(bytes[offset]) << 24U) |
      (static_cast<std::uint32_t>(bytes[offset + 1U]) << 16U) |
      (static_cast<std::uint32_t>(bytes[offset + 2U]) << 8U) |
      static_cast<std::uint32_t>(bytes[offset + 3U]);
}

[[nodiscard]] inline FomWireBytes copyRange(
    std::span<const FomWireOctet> bytes,
    std::size_t begin,
    std::size_t end) {
  return FomWireBytes(
      bytes.begin() + static_cast<std::ptrdiff_t>(begin),
      bytes.begin() + static_cast<std::ptrdiff_t>(end));
}

}  // namespace fom_wire_codec_detail

// RPRunsignedInteger*BE values are unsigned, fixed-width, big-endian
// quantities.  This helper intentionally accepts a width rather than an RTI
// DataElement so it can be used by the isolated RPR adapter and by callers
// that are translating a catalog-described payload.
[[nodiscard]] inline FomWireBytes encodeRprUnsignedInteger(
    std::uint64_t value,
    std::uint32_t sizeBits) {
  auto const octetCount =
      fom_wire_codec_detail::rprUnsignedIntegerOctetCount(sizeBits);
  if (value > fom_wire_codec_detail::rprUnsignedIntegerMaximum(sizeBits)) {
    fom_wire_codec_detail::fail(
        "The RPR unsigned integer value does not fit its declared width.");
  }

  FomWireBytes result;
  result.reserve(octetCount);
  for (std::size_t index = octetCount; index != 0U; --index) {
    auto const shift = static_cast<unsigned>((index - 1U) * 8U);
    result.push_back(static_cast<FomWireOctet>((value >> shift) & 0xffU));
  }
  return result;
}

[[nodiscard]] inline FomRprUnsignedIntegerDecodeResult
decodeRprUnsignedInteger(
    std::span<const FomWireOctet> bytes,
    std::size_t offset,
    std::uint32_t sizeBits) {
  auto const octetCount =
      fom_wire_codec_detail::rprUnsignedIntegerOctetCount(sizeBits);
  fom_wire_codec_detail::requireRange(
      bytes,
      offset,
      octetCount,
      "The RPR unsigned integer payload is truncated.");

  std::uint64_t value = 0U;
  for (std::size_t index = 0U; index < octetCount; ++index) {
    value = (value << 8U) | static_cast<std::uint64_t>(bytes[offset + index]);
  }
  return {
      value,
      fom_wire_codec_detail::checkedAdd(
          offset,
          octetCount,
          "The RPR unsigned integer offset overflows size_t.")};
}

// RPRnullTerminatedArray is defined for octet-sized character elements. The
// zero octet is the delimiter, so an embedded zero is rejected at the
// boundary instead of being silently interpreted as an early end.
[[nodiscard]] inline FomWireBytes encodeRprNullTerminatedArray(
    std::span<const FomWireOctet> payload) {
  if (std::find(payload.begin(), payload.end(), static_cast<FomWireOctet>(0U)) !=
      payload.end()) {
    fom_wire_codec_detail::fail(
        "An RPR null-terminated array element contains the terminator octet.");
  }

  FomWireBytes result;
  result.reserve(fom_wire_codec_detail::checkedAdd(
      payload.size(),
      1U,
      "The RPR null-terminated array length overflows size_t."));
  result.insert(result.end(), payload.begin(), payload.end());
  result.push_back(static_cast<FomWireOctet>(0U));
  return result;
}

// The decoder returns the first value after the terminator so a null-terminated
// field can be embedded in a larger record. A standalone caller must compare
// nextOffset with its enclosing bound if trailing bytes are not allowed.
[[nodiscard]] inline FomWireDecodeResult decodeRprNullTerminatedArray(
    std::span<const FomWireOctet> bytes,
    std::size_t offset = 0U) {
  if (offset > bytes.size()) {
    fom_wire_codec_detail::fail(
        "The RPR null-terminated array decode offset is outside the input.");
  }

  auto const terminator = std::find(
      bytes.begin() + static_cast<std::ptrdiff_t>(offset),
      bytes.end(),
      static_cast<FomWireOctet>(0U));
  if (terminator == bytes.end()) {
    fom_wire_codec_detail::fail(
        "The RPR null-terminated array is missing its terminator.");
  }

  auto const terminatorOffset = static_cast<std::size_t>(
      std::distance(bytes.begin(), terminator));
  return {
      fom_wire_codec_detail::copyRange(bytes, offset, terminatorOffset),
      fom_wire_codec_detail::checkedAdd(
          terminatorOffset,
          1U,
          "The RPR null-terminated array terminator offset overflows size_t.")};
}

// RPRlengthlessArray has no count or byte-length prefix. Encoding is therefore
// concatenation plus the HLAfixedArray-style padding between non-final
// elements; decoding requires the enclosing field's byte bound, an optional
// externally known element count, the element boundary, and a child decoder
// that returns an absolute next offset within that bound. When no count is
// supplied, the child decoder is applied until the enclosing bound is
// consumed, but only when the boundary cannot add inter-element padding. At a
// padded boundary the external count is required so final padding cannot be
// mistaken for another element.
[[nodiscard]] inline FomWireBytes encodeRprLengthlessArray(
    std::vector<FomWireBytes> const& elements,
    std::uint32_t elementBoundaryOctets) {
  static_cast<void>(
      fom_wire_codec_detail::paddingToBoundary(0U, elementBoundaryOctets));
  std::size_t totalLength = 0U;
  for (std::size_t index = 0U; index < elements.size(); ++index) {
    auto const& element = elements[index];
    totalLength = fom_wire_codec_detail::checkedAdd(
        totalLength,
        element.size(),
        "The RPR lengthless array length overflows size_t.");
    if (index + 1U != elements.size()) {
      totalLength = fom_wire_codec_detail::checkedAdd(
          totalLength,
          fom_wire_codec_detail::paddingToBoundary(
              element.size(),
              elementBoundaryOctets),
          "The RPR lengthless array padding length overflows size_t.");
    }
  }

  FomWireBytes result;
  result.reserve(totalLength);
  for (std::size_t index = 0U; index < elements.size(); ++index) {
    auto const& element = elements[index];
    result.insert(result.end(), element.begin(), element.end());
    if (index + 1U != elements.size()) {
      fom_wire_codec_detail::appendZeroPadding(
          result,
          fom_wire_codec_detail::paddingToBoundary(
              element.size(),
              elementBoundaryOctets));
    }
  }
  return result;
}

[[nodiscard]] inline FomWireLengthlessArrayDecodeResult decodeRprLengthlessArray(
    std::span<const FomWireOctet> bytes,
    std::size_t offset,
    std::size_t endOffset,
    std::optional<std::size_t> elementCount,
    std::uint32_t elementBoundaryOctets,
    FomWireElementDecoder const& decodeElement) {
  if (offset > endOffset || endOffset > bytes.size()) {
    fom_wire_codec_detail::fail(
        "The RPR lengthless array bounds are outside the input.");
  }
  if (!decodeElement) {
    fom_wire_codec_detail::fail(
        "The RPR lengthless array requires an element decoder.");
  }
  static_cast<void>(
      fom_wire_codec_detail::paddingToBoundary(0U, elementBoundaryOctets));
  if (!elementCount.has_value() && elementBoundaryOctets != 1U) {
    fom_wire_codec_detail::fail(
        "The RPR lengthless array requires an external element count when "
        "its element boundary can add padding.");
  }

  // This seam deliberately requires each child decoder to consume at least
  // one octet. Without an inline count, zero-width elements cannot be
  // delimited safely by a byte-level adapter.
  if (elementCount.has_value() && *elementCount > endOffset - offset) {
    fom_wire_codec_detail::fail(
        "The RPR lengthless array element count exceeds its byte bound.");
  }

  FomWireLengthlessArrayDecodeResult result;
  if (elementCount.has_value()) {
    result.elements.reserve(*elementCount);
  }
  auto cursor = offset;
  std::size_t index = 0U;
  while (cursor < endOffset &&
      (!elementCount.has_value() || index < *elementCount)) {
    auto const elementStart = cursor;
    auto const next = decodeElement(bytes, elementStart, endOffset);
    if (next <= elementStart || next > endOffset) {
      fom_wire_codec_detail::fail(
          "The RPR lengthless array element decoder returned an invalid offset.");
    }
    result.elements.push_back(
        fom_wire_codec_detail::copyRange(bytes, elementStart, next));
    cursor = next;
    auto const hasFollowingElement = elementCount.has_value()
        ? index + 1U < *elementCount
        : cursor < endOffset;
    if (hasFollowingElement) {
      auto const padding = fom_wire_codec_detail::paddingToBoundary(
          next - elementStart,
          elementBoundaryOctets);
      fom_wire_codec_detail::requireZeroPadding(
          bytes,
          cursor,
          padding,
          "The RPR lengthless array contains truncated or nonzero padding.");
      cursor = fom_wire_codec_detail::checkedAdd(
          cursor,
          padding,
          "The RPR lengthless array cursor overflows size_t.");
    }
    ++index;
  }

  if (elementCount.has_value() && index != *elementCount) {
    fom_wire_codec_detail::fail(
        "The RPR lengthless array ended before its external element count.");
  }

  if (cursor != endOffset) {
    fom_wire_codec_detail::fail(
        "The RPR lengthless array decoder left bytes inside its external bound.");
  }
  result.nextOffset = cursor;
  return result;
}

// Padding arrays are structural zero octets, not meaningful array elements.
// The caller supplies the expected end of the padding run because a zero byte
// at the start of the following field is otherwise indistinguishable from
// padding at this low level.
[[nodiscard]] inline FomWireBytes encodeRprPaddingTo32Array(
    std::size_t currentOffset) {
  auto const length = fom_wire_codec_detail::paddingToBoundary(currentOffset, 4U);
  return FomWireBytes(length, static_cast<FomWireOctet>(0U));
}

[[nodiscard]] inline std::size_t decodeRprPaddingTo32Array(
    std::span<const FomWireOctet> bytes,
    std::size_t currentOffset,
    std::size_t paddingEndOffset) {
  if (currentOffset > paddingEndOffset || paddingEndOffset > bytes.size()) {
    fom_wire_codec_detail::fail(
        "The RPR 32-bit padding bounds are outside the input.");
  }
  auto const expectedLength =
      fom_wire_codec_detail::paddingToBoundary(currentOffset, 4U);
  auto const expectedEnd = fom_wire_codec_detail::checkedAdd(
      currentOffset,
      expectedLength,
      "The RPR 32-bit padding end overflows size_t.");
  if (paddingEndOffset != expectedEnd) {
    fom_wire_codec_detail::fail(
        "The RPR 32-bit padding does not end at the required boundary.");
  }
  fom_wire_codec_detail::requireZeroPadding(
      bytes,
      currentOffset,
      expectedLength,
      "The RPR 32-bit padding is truncated or contains a nonzero octet.");
  return paddingEndOffset;
}

[[nodiscard]] inline FomWireBytes encodeRprPaddingTo64Array(
    std::size_t currentOffset) {
  auto const length = fom_wire_codec_detail::paddingToBoundary(currentOffset, 8U);
  return FomWireBytes(length, static_cast<FomWireOctet>(0U));
}

[[nodiscard]] inline std::size_t decodeRprPaddingTo64Array(
    std::span<const FomWireOctet> bytes,
    std::size_t currentOffset,
    std::size_t paddingEndOffset) {
  if (currentOffset > paddingEndOffset || paddingEndOffset > bytes.size()) {
    fom_wire_codec_detail::fail(
        "The RPR 64-bit padding bounds are outside the input.");
  }
  auto const expectedLength =
      fom_wire_codec_detail::paddingToBoundary(currentOffset, 8U);
  auto const expectedEnd = fom_wire_codec_detail::checkedAdd(
      currentOffset,
      expectedLength,
      "The RPR 64-bit padding end overflows size_t.");
  if (paddingEndOffset != expectedEnd) {
    fom_wire_codec_detail::fail(
        "The RPR 64-bit padding does not end at the required boundary.");
  }
  fom_wire_codec_detail::requireZeroPadding(
      bytes,
      currentOffset,
      expectedLength,
      "The RPR 64-bit padding is truncated or contains a nonzero octet.");
  return paddingEndOffset;
}

// RPRextendedVariantRecord adds an unsigned 32-bit big-endian alternative
// length immediately after the discriminant. Unlike the standard
// HLAextendableVariantRecord envelope, the RPR form does not add a separate
// discriminant or alternative alignment pad. The raw discriminant and
// alternative are intentionally returned; catalog-level callers remain
// responsible for selecting/validating child datatypes.
[[nodiscard]] inline FomWireBytes encodeRprExtendedVariantRecord(
    std::span<const FomWireOctet> discriminant,
    std::span<const FomWireOctet> alternative) {
  if (discriminant.empty()) {
    fom_wire_codec_detail::fail(
        "An RPR extended variant record requires a discriminant.");
  }
  if (alternative.size() >
      static_cast<std::size_t>(std::numeric_limits<std::uint32_t>::max())) {
    fom_wire_codec_detail::fail(
        "The RPR extended variant alternative exceeds uint32 length.");
  }

  auto const prefixLength = fom_wire_codec_detail::checkedAdd(
      discriminant.size(),
      4U,
      "The RPR extended variant prefix length overflows size_t.");
  auto const totalLength = fom_wire_codec_detail::checkedAdd(
      prefixLength,
      alternative.size(),
      "The RPR extended variant length overflows size_t.");

  FomWireBytes result;
  result.reserve(totalLength);
  result.insert(result.end(), discriminant.begin(), discriminant.end());
  fom_wire_codec_detail::appendUnsigned32BigEndian(
      result,
      static_cast<std::uint32_t>(alternative.size()));
  result.insert(result.end(), alternative.begin(), alternative.end());
  return result;
}

[[nodiscard]] inline FomRprExtendedVariantRecordDecodeResult
decodeRprExtendedVariantRecord(
    std::span<const FomWireOctet> bytes,
    std::size_t offset,
    std::size_t discriminantLength) {
  if (discriminantLength == 0U) {
    fom_wire_codec_detail::fail(
        "An RPR extended variant record requires a discriminant length.");
  }
  fom_wire_codec_detail::requireRange(
      bytes,
      offset,
      discriminantLength,
      "The RPR extended variant discriminant is truncated.");

  auto const discriminantEnd = fom_wire_codec_detail::checkedAdd(
      offset,
      discriminantLength,
      "The RPR extended variant discriminant offset overflows size_t.");
  auto const lengthOffset = discriminantEnd;
  fom_wire_codec_detail::requireRange(
      bytes,
      lengthOffset,
      4U,
      "The RPR extended variant alternative length is truncated.");

  auto const encodedLength =
      fom_wire_codec_detail::readUnsigned32BigEndian(bytes, lengthOffset);
  auto const alternativeStart = fom_wire_codec_detail::checkedAdd(
      lengthOffset,
      4U,
      "The RPR extended variant alternative offset overflows size_t.");
  auto const alternativeEnd = fom_wire_codec_detail::checkedAdd(
      alternativeStart,
      static_cast<std::size_t>(encodedLength),
      "The RPR extended variant alternative end overflows size_t.");
  fom_wire_codec_detail::requireRange(
      bytes,
      alternativeStart,
      static_cast<std::size_t>(encodedLength),
      "The RPR extended variant alternative is truncated.");

  return {
      fom_wire_codec_detail::copyRange(bytes, offset, discriminantEnd),
      fom_wire_codec_detail::copyRange(
          bytes,
          alternativeStart,
          alternativeEnd),
      alternativeEnd};
}

}  // namespace umbra::detail
