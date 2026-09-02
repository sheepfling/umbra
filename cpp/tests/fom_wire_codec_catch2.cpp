#include <catch2/catch_test_macros.hpp>

#include "internal/fom/fom_wire_codec.hpp"
#include "internal/fom/fom_rpr_wire_encoding.hpp"
#include "internal/fom/fom_wire_encoding.hpp"

#include <array>
#include <cstdint>
#include <limits>
#include <optional>
#include <span>
#include <string_view>
#include <vector>

namespace {

using umbra::detail::FomSourceCompatibility;
using umbra::detail::FomWireCodecStatus;
using umbra::detail::FomWireEncodingKind;
using umbra::detail::FomWireBytes;
using umbra::detail::FomWireCodecError;
using umbra::detail::FomWireElementDecoder;
using umbra::detail::decodeRprExtendedVariantRecord;
using umbra::detail::decodeRprLengthlessArray;
using umbra::detail::decodeRprNullTerminatedArray;
using umbra::detail::decodeRprPaddingTo32Array;
using umbra::detail::decodeRprPaddingTo64Array;
using umbra::detail::encodeRprExtendedVariantRecord;
using umbra::detail::encodeRprLengthlessArray;
using umbra::detail::encodeRprNullTerminatedArray;
using umbra::detail::encodeRprPaddingTo32Array;
using umbra::detail::encodeRprPaddingTo64Array;
using umbra::detail::decodeRprUnsignedInteger;
using umbra::detail::encodeRprUnsignedInteger;
using umbra::detail::normalizeFomWireEncoding;
using umbra::detail::normalizeRprBasicFomWireEncoding;
using umbra::detail::normalizeRprFomWireEncoding;
using umbra::detail::normalizeRprUnsignedIntegerFomWireEncoding;

TEST_CASE(
    "RPR null-terminated arrays encode a unique zero-octet terminator",
    "[unit][fom][rpr][wire]") {
  FomWireBytes const value{0x52U, 0x50U, 0x52U};
  auto const encoded = encodeRprNullTerminatedArray(value);
  REQUIRE(encoded == FomWireBytes{0x52U, 0x50U, 0x52U, 0x00U});

  auto const decoded = decodeRprNullTerminatedArray(encoded);
  REQUIRE(decoded.value == value);
  REQUIRE(decoded.nextOffset == encoded.size());

  FomWireBytes const embedded{0x7fU, 0x52U, 0x50U, 0x52U, 0x00U, 0xa5U};
  auto const embeddedDecoded = decodeRprNullTerminatedArray(embedded, 1U);
  REQUIRE(embeddedDecoded.value == value);
  REQUIRE(embeddedDecoded.nextOffset == 5U);

  REQUIRE(encodeRprNullTerminatedArray(FomWireBytes{}) == FomWireBytes{0x00U});
  REQUIRE_THROWS_AS(
      encodeRprNullTerminatedArray(FomWireBytes{0x41U, 0x00U, 0x42U}),
      FomWireCodecError);
  REQUIRE_THROWS_AS(
      decodeRprNullTerminatedArray(FomWireBytes{0x41U, 0x42U}),
      FomWireCodecError);
  REQUIRE_THROWS_AS(
      decodeRprNullTerminatedArray(FomWireBytes{0x41U}, 2U),
      FomWireCodecError);
}

TEST_CASE(
    "RPR catalog codec status follows the implemented structural labels",
    "[unit][fom][rpr][wire][catalog]") {
  std::array<std::string_view, 5U> const labels{{
      "RPRnullTerminatedArray",
      "RPRlengthlessArray",
      "RPRpaddingTo32Array",
      "RPRpaddingTo64Array",
      "RPRextendedVariantRecord",
  }};
  for (auto const label : labels) {
    auto const neutralDescriptor =
        normalizeFomWireEncoding(label, FomSourceCompatibility::rpr_2010);
    REQUIRE(neutralDescriptor.codecStatus == FomWireCodecStatus::metadata_only);
    REQUIRE(neutralDescriptor.kind != FomWireEncodingKind::unrecognized);

    auto const descriptor = normalizeRprFomWireEncoding(label);
    REQUIRE(descriptor.codecStatus == FomWireCodecStatus::available);
    REQUIRE(descriptor.kind == neutralDescriptor.kind);
  }

  auto const strictDescriptor = normalizeFomWireEncoding(
      "RPRnullTerminatedArray",
      FomSourceCompatibility::strict);
  REQUIRE(strictDescriptor.kind == FomWireEncodingKind::unrecognized);
  REQUIRE(strictDescriptor.codecStatus == FomWireCodecStatus::unsupported);
}

TEST_CASE(
    "RPR unsigned integer representations are exact big-endian scalar codecs",
    "[unit][fom][rpr][wire][scalar]") {
  struct Case final {
    std::uint32_t sizeBits;
    std::uint64_t value;
    FomWireBytes expected;
  };
  std::array<Case, 4U> const cases{{
      {8U, 0xa5U, FomWireBytes{0xa5U}},
      {16U, 0x1234U, FomWireBytes{0x12U, 0x34U}},
      {32U, 0x12345678U, FomWireBytes{0x12U, 0x34U, 0x56U, 0x78U}},
      {64U,
       0x0123456789abcdefULL,
       FomWireBytes{0x01U, 0x23U, 0x45U, 0x67U, 0x89U, 0xabU, 0xcdU, 0xefU}},
  }};

  for (auto const& testCase : cases) {
    auto const encoded = encodeRprUnsignedInteger(
        testCase.value,
        testCase.sizeBits);
    REQUIRE(encoded == testCase.expected);

    FomWireBytes embedded{0xeeU};
    embedded.insert(embedded.end(), encoded.begin(), encoded.end());
    embedded.push_back(0xddU);
    auto const decoded = decodeRprUnsignedInteger(
        embedded,
        1U,
        testCase.sizeBits);
    REQUIRE(decoded.value == testCase.value);
    REQUIRE(decoded.nextOffset == encoded.size() + 1U);
  }

  REQUIRE(encodeRprUnsignedInteger(
              std::numeric_limits<std::uint64_t>::max(),
              64U) ==
          FomWireBytes{
              0xffU,
              0xffU,
              0xffU,
              0xffU,
              0xffU,
              0xffU,
              0xffU,
              0xffU});
  REQUIRE_THROWS_AS(
      encodeRprUnsignedInteger(0x100U, 8U),
      FomWireCodecError);
  REQUIRE_THROWS_AS(
      encodeRprUnsignedInteger(0x10000U, 16U),
      FomWireCodecError);
  REQUIRE_THROWS_AS(
      encodeRprUnsignedInteger(0x100000000ULL, 32U),
      FomWireCodecError);
  REQUIRE_THROWS_AS(
      encodeRprUnsignedInteger(0U, 0U),
      FomWireCodecError);
  REQUIRE_THROWS_AS(
      encodeRprUnsignedInteger(0U, 24U),
      FomWireCodecError);
  REQUIRE_THROWS_AS(
      decodeRprUnsignedInteger(FomWireBytes{0x01U}, 0U, 16U),
      FomWireCodecError);
  REQUIRE_THROWS_AS(
      decodeRprUnsignedInteger(FomWireBytes{0x01U}, 2U, 8U),
      FomWireCodecError);
}

TEST_CASE(
    "RPR unsigned integer catalog promotion remains profile and declaration gated",
    "[unit][fom][rpr][wire][catalog]") {
  auto const neutral = normalizeFomWireEncoding(
      "RPRunsignedInteger16BE",
      FomSourceCompatibility::rpr_2010);
  REQUIRE(neutral.kind == FomWireEncodingKind::unrecognized);
  REQUIRE(neutral.codecStatus == FomWireCodecStatus::unsupported);

  auto const representation = normalizeRprUnsignedIntegerFomWireEncoding(
      "RPRunsignedInteger16BE");
  REQUIRE(representation.kind == FomWireEncodingKind::fixed_width);
  REQUIRE(representation.codecStatus == FomWireCodecStatus::available);
  REQUIRE(representation.sourceLabel == "RPRunsignedInteger16BE");
  REQUIRE(representation.sizeBits == 16U);
  REQUIRE(representation.byteOrder == umbra::detail::FomByteOrder::big);

  auto const basic = normalizeRprBasicFomWireEncoding(
      "RPRunsignedInteger16BE",
      "16-bit unsigned integer.",
      16U,
      umbra::detail::FomByteOrder::big);
  REQUIRE(basic.codecStatus == FomWireCodecStatus::available);
  REQUIRE(basic.sizeBits == 16U);
  REQUIRE(basic.sourceLabel == "16-bit unsigned integer.");

  auto const wrongWidth = normalizeRprBasicFomWireEncoding(
      "RPRunsignedInteger16BE",
      "16-bit unsigned integer.",
      32U,
      umbra::detail::FomByteOrder::big);
  REQUIRE(wrongWidth.codecStatus == FomWireCodecStatus::metadata_only);
  auto const wrongEncoding = normalizeRprBasicFomWireEncoding(
      "RPRunsignedInteger16BE",
      "32-bit unsigned integer.",
      16U,
      umbra::detail::FomByteOrder::big);
  REQUIRE(wrongEncoding.codecStatus == FomWireCodecStatus::metadata_only);
  auto const wrongOrder = normalizeRprBasicFomWireEncoding(
      "RPRunsignedInteger16BE",
      "16-bit unsigned integer.",
      16U,
      umbra::detail::FomByteOrder::little);
  REQUIRE(wrongOrder.codecStatus == FomWireCodecStatus::metadata_only);
}

TEST_CASE(
    "RPR lengthless arrays use an enclosing bound and child decoder",
    "[unit][fom][rpr][wire]") {
  std::vector<FomWireBytes> const elements{
      FomWireBytes{0x10U, 0x11U},
      FomWireBytes{0x20U, 0x21U},
      FomWireBytes{0x30U, 0x31U},
  };
  auto const encoded = encodeRprLengthlessArray(elements, 1U);
  REQUIRE(
      encoded ==
      FomWireBytes{0x10U, 0x11U, 0x20U, 0x21U, 0x30U, 0x31U});
  REQUIRE_THROWS_AS(
      encodeRprLengthlessArray({}, 0U),
      FomWireCodecError);

  std::vector<FomWireBytes> const paddedElements{
      FomWireBytes{0xa1U},
      FomWireBytes{0xb2U, 0xb3U}};
  auto const paddedEncoded = encodeRprLengthlessArray(paddedElements, 4U);
  REQUIRE(
      paddedEncoded ==
      FomWireBytes{0xa1U, 0x00U, 0x00U, 0x00U, 0xb2U, 0xb3U});

  FomWireBytes const enclosing{
      0xe0U,
      0x10U,
      0x11U,
      0x20U,
      0x21U,
      0x30U,
      0x31U,
      0xeeU};
  FomWireElementDecoder const decodeTwoOctetElement =
      [](std::span<const std::uint8_t>, std::size_t offset, std::size_t endOffset) {
        if (endOffset - offset < 2U) {
          throw FomWireCodecError("test element is truncated");
        }
        return offset + 2U;
      };

  auto const decoded = decodeRprLengthlessArray(
      enclosing,
      1U,
      7U,
      elements.size(),
      1U,
      decodeTwoOctetElement);
  REQUIRE(decoded.elements == elements);
  REQUIRE(decoded.nextOffset == 7U);
  auto const inferredDecoded = decodeRprLengthlessArray(
      enclosing,
      1U,
      7U,
      std::nullopt,
      1U,
      decodeTwoOctetElement);
  REQUIRE(inferredDecoded.elements == elements);
  REQUIRE(inferredDecoded.nextOffset == 7U);

  FomWireElementDecoder const decodePaddedElement =
      [](std::span<const std::uint8_t> bytes,
         std::size_t offset,
         std::size_t endOffset) {
        if (offset >= endOffset) {
          throw FomWireCodecError("test padded element is truncated");
        }
        return bytes[offset] == 0xa1U ? offset + 1U : offset + 2U;
      };
  auto const paddedDecoded = decodeRprLengthlessArray(
      paddedEncoded,
      0U,
      paddedEncoded.size(),
      paddedElements.size(),
      4U,
      decodePaddedElement);
  REQUIRE(paddedDecoded.elements == paddedElements);
  REQUIRE_THROWS_AS(
      decodeRprLengthlessArray(
          paddedEncoded,
          0U,
          paddedEncoded.size(),
          std::nullopt,
          4U,
          decodePaddedElement),
      FomWireCodecError);
  auto nonzeroElementPadding = paddedEncoded;
  nonzeroElementPadding[2U] = 0x01U;
  REQUIRE_THROWS_AS(
      decodeRprLengthlessArray(
          nonzeroElementPadding,
          0U,
          nonzeroElementPadding.size(),
          paddedElements.size(),
          4U,
          decodePaddedElement),
      FomWireCodecError);

  REQUIRE_NOTHROW(decodeRprLengthlessArray(
      enclosing,
      7U,
      7U,
      0U,
      1U,
      decodeTwoOctetElement));
  REQUIRE_THROWS_AS(
      decodeRprLengthlessArray(
          enclosing,
          1U,
          7U,
          2U,
          1U,
          decodeTwoOctetElement),
      FomWireCodecError);
  REQUIRE_THROWS_AS(
      decodeRprLengthlessArray(
          enclosing,
          1U,
          7U,
          4U,
          1U,
          decodeTwoOctetElement),
      FomWireCodecError);
  REQUIRE_THROWS_AS(
      decodeRprLengthlessArray(
          enclosing,
          1U,
          7U,
          3U,
          1U,
          FomWireElementDecoder{}),
      FomWireCodecError);
  REQUIRE_THROWS_AS(
      decodeRprLengthlessArray(
          enclosing,
          1U,
          6U,
          3U,
          1U,
          decodeTwoOctetElement),
      FomWireCodecError);

  FomWireElementDecoder const noProgress =
      [](std::span<const std::uint8_t>, std::size_t offset, std::size_t) {
        return offset;
      };
  REQUIRE_THROWS_AS(
      decodeRprLengthlessArray(
          enclosing,
          1U,
          7U,
          1U,
          1U,
          noProgress),
      FomWireCodecError);

  FomWireElementDecoder const escapesBound =
      [](std::span<const std::uint8_t>, std::size_t, std::size_t endOffset) {
        return endOffset + 1U;
      };
  REQUIRE_THROWS_AS(
      decodeRprLengthlessArray(
          enclosing,
          1U,
          7U,
          1U,
          1U,
          escapesBound),
      FomWireCodecError);
}

TEST_CASE(
    "RPR padding arrays emit and validate zero alignment octets",
    "[unit][fom][rpr][wire]") {
  REQUIRE(encodeRprPaddingTo32Array(0U).empty());
  REQUIRE(encodeRprPaddingTo32Array(1U) == FomWireBytes{0x00U, 0x00U, 0x00U});
  REQUIRE(encodeRprPaddingTo32Array(4U).empty());
  REQUIRE(encodeRprPaddingTo64Array(3U) ==
          FomWireBytes{0x00U, 0x00U, 0x00U, 0x00U, 0x00U});
  REQUIRE(encodeRprPaddingTo64Array(8U).empty());

  FomWireBytes const valid{
      0xaaU,
      0x00U,
      0x00U,
      0x00U,
      0xbbU,
      0x00U,
      0x00U,
      0x00U,
      0x00U,
      0x00U,
      0xccU};
  REQUIRE(decodeRprPaddingTo32Array(valid, 1U, 4U) == 4U);
  FomWireBytes const valid64{
      0xaaU,
      0xbbU,
      0xccU,
      0x00U,
      0x00U,
      0x00U,
      0x00U,
      0x00U,
      0xddU};
  REQUIRE(decodeRprPaddingTo64Array(valid64, 3U, 8U) == 8U);
  REQUIRE(decodeRprPaddingTo32Array(valid, 4U, 4U) == 4U);

  auto nonzero = valid;
  nonzero[2U] = 0x01U;
  REQUIRE_THROWS_AS(
      decodeRprPaddingTo32Array(nonzero, 1U, 4U),
      FomWireCodecError);
  REQUIRE_THROWS_AS(
      decodeRprPaddingTo32Array(valid, 1U, 3U),
      FomWireCodecError);
  REQUIRE_THROWS_AS(
      decodeRprPaddingTo64Array(valid64, 3U, 9U),
      FomWireCodecError);
  REQUIRE_THROWS_AS(
      decodeRprPaddingTo64Array(FomWireBytes{0xaaU, 0x00U}, 1U, 6U),
      FomWireCodecError);
}

TEST_CASE(
    "RPR extended variants preserve the discriminant and length-delimited alternative",
    "[unit][fom][rpr][wire]") {
  FomWireBytes const discriminant{0x01U, 0x02U, 0x03U, 0x04U, 0x05U};
  FomWireBytes const alternative{0xa0U, 0xa1U, 0xa2U};
  auto const encoded =
      encodeRprExtendedVariantRecord(discriminant, alternative);

  // The RPR length is an unsigned 32-bit field immediately after the
  // discriminant; the alternative follows that field without extra padding.
  REQUIRE(
      encoded ==
      FomWireBytes{
          0x01U,
          0x02U,
          0x03U,
          0x04U,
          0x05U,
          0x00U,
          0x00U,
          0x00U,
          0x03U,
          0xa0U,
          0xa1U,
          0xa2U});

  auto const decoded = decodeRprExtendedVariantRecord(
      encoded,
      0U,
      discriminant.size());
  REQUIRE(decoded.discriminant == discriminant);
  REQUIRE(decoded.alternative == alternative);
  REQUIRE(decoded.nextOffset == encoded.size());

  FomWireBytes nested{0xffU};
  nested.insert(nested.end(), encoded.begin(), encoded.end());
  nested.push_back(0xeeU);
  auto const nestedDecoded = decodeRprExtendedVariantRecord(
      nested,
      1U,
      discriminant.size());
  REQUIRE(nestedDecoded.discriminant == discriminant);
  REQUIRE(nestedDecoded.alternative == alternative);
  REQUIRE(nestedDecoded.nextOffset == nested.size() - 1U);

  auto oversizedLength = encoded;
  oversizedLength[5U] = 0x80U;
  // The high bit is part of the unsigned length, not a signed-negative
  // sentinel. The vector is rejected only because its declared alternative
  // does not fit in the supplied input.
  REQUIRE_THROWS_AS(
      decodeRprExtendedVariantRecord(
          oversizedLength,
          0U,
          discriminant.size()),
      FomWireCodecError);
  auto truncatedAlternative = encoded;
  truncatedAlternative.pop_back();
  REQUIRE_THROWS_AS(
      decodeRprExtendedVariantRecord(
          truncatedAlternative,
          0U,
          discriminant.size()),
      FomWireCodecError);
  REQUIRE_THROWS_AS(
      decodeRprExtendedVariantRecord(
          FomWireBytes{0x01U, 0x02U, 0x03U, 0x04U, 0x05U},
          0U,
          discriminant.size()),
      FomWireCodecError);
  REQUIRE_THROWS_AS(
      encodeRprExtendedVariantRecord(FomWireBytes{}, alternative),
      FomWireCodecError);
  REQUIRE_THROWS_AS(
      decodeRprExtendedVariantRecord(encoded, 0U, 0U),
      FomWireCodecError);
  REQUIRE_THROWS_AS(
      decodeRprExtendedVariantRecord(
          FomWireBytes{0x01U, 0x02U},
          0U,
          discriminant.size()),
      FomWireCodecError);
}

}  // namespace
