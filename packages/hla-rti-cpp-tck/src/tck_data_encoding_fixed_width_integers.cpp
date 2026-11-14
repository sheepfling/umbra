#include "tck_data_encoding.hpp"

#include <limits>

namespace hla_rti_cpp_tck::detail {

template <typename Element, typename Value>
void requireIntegerBoundaryVector(
    Value value,
    std::vector<std::uint8_t> expectedBytes,
    std::string const& description) {
  Element encoded{value};
  requireEncodedBytes(encoded, expectedBytes, description);
  Element decoded;
  decoded.decode(encoded.encode());
  require(
      decoded.get() == value,
      description + " did not round-trip");
}

template <typename Element, typename Value>
void requireNonterminalIntegerCursor(
    Value value,
    std::size_t encodedWidth,
    std::string const& description) {
  Element encoded{value};
  std::vector<rti::Octet> buffer{static_cast<rti::Octet>(0x7eU)};
  encoded.encodeInto(buffer);
  auto const expectedCursor = buffer.size();
  buffer.push_back(static_cast<rti::Octet>(0xa5U));

  Element decoded;
  auto const cursor = decoded.decodeFrom(buffer, 1U);
  require(
      expectedCursor == 1U + encodedWidth && cursor == expectedCursor &&
          cursor < buffer.size() &&
          buffer[cursor] == static_cast<rti::Octet>(0xa5U) &&
          decoded.get() == value,
      description +
          " decodeFrom did not return the cursor before the following value");
}

void verifyFixedWidthIntegerBoundariesContract() {
  auto const signed16Min = std::numeric_limits<std::int16_t>::min();
  auto const signed16Max = std::numeric_limits<std::int16_t>::max();
  auto const unsigned16Min = std::numeric_limits<std::uint16_t>::min();
  auto const unsigned16Max = std::numeric_limits<std::uint16_t>::max();
  requireIntegerBoundaryVector<rti::HLAinteger16BE>(
      signed16Min, {0x80U, 0U}, "HLAinteger16BE minimum");
  requireIntegerBoundaryVector<rti::HLAinteger16LE>(
      signed16Min, {0U, 0x80U}, "HLAinteger16LE minimum");
  requireIntegerBoundaryVector<rti::HLAinteger16BE>(
      signed16Max, {0x7fU, 0xffU}, "HLAinteger16BE maximum");
  requireIntegerBoundaryVector<rti::HLAinteger16LE>(
      signed16Max, {0xffU, 0x7fU}, "HLAinteger16LE maximum");
  requireIntegerBoundaryVector<rti::HLAunsignedInteger16BE>(
      unsigned16Min, {0U, 0U}, "HLAunsignedInteger16BE minimum");
  requireIntegerBoundaryVector<rti::HLAunsignedInteger16LE>(
      unsigned16Min, {0U, 0U}, "HLAunsignedInteger16LE minimum");
  requireIntegerBoundaryVector<rti::HLAunsignedInteger16BE>(
      unsigned16Max, {0xffU, 0xffU}, "HLAunsignedInteger16BE maximum");
  requireIntegerBoundaryVector<rti::HLAunsignedInteger16LE>(
      unsigned16Max, {0xffU, 0xffU}, "HLAunsignedInteger16LE maximum");

  auto const signed32Min = std::numeric_limits<std::int32_t>::min();
  auto const signed32Max = std::numeric_limits<std::int32_t>::max();
  auto const unsigned32Min = std::numeric_limits<std::uint32_t>::min();
  auto const unsigned32Max = std::numeric_limits<std::uint32_t>::max();
  requireIntegerBoundaryVector<rti::HLAinteger32BE>(
      signed32Min, {0x80U, 0U, 0U, 0U}, "HLAinteger32BE minimum");
  requireIntegerBoundaryVector<rti::HLAinteger32LE>(
      signed32Min, {0U, 0U, 0U, 0x80U}, "HLAinteger32LE minimum");
  requireIntegerBoundaryVector<rti::HLAinteger32BE>(
      signed32Max, {0x7fU, 0xffU, 0xffU, 0xffU}, "HLAinteger32BE maximum");
  requireIntegerBoundaryVector<rti::HLAinteger32LE>(
      signed32Max, {0xffU, 0xffU, 0xffU, 0x7fU}, "HLAinteger32LE maximum");
  requireIntegerBoundaryVector<rti::HLAunsignedInteger32BE>(
      unsigned32Min, {0U, 0U, 0U, 0U}, "HLAunsignedInteger32BE minimum");
  requireIntegerBoundaryVector<rti::HLAunsignedInteger32LE>(
      unsigned32Min, {0U, 0U, 0U, 0U}, "HLAunsignedInteger32LE minimum");
  requireIntegerBoundaryVector<rti::HLAunsignedInteger32BE>(
      unsigned32Max, {0xffU, 0xffU, 0xffU, 0xffU}, "HLAunsignedInteger32BE maximum");
  requireIntegerBoundaryVector<rti::HLAunsignedInteger32LE>(
      unsigned32Max, {0xffU, 0xffU, 0xffU, 0xffU}, "HLAunsignedInteger32LE maximum");

  auto const signed64Min = std::numeric_limits<std::int64_t>::min();
  auto const signed64Max = std::numeric_limits<std::int64_t>::max();
  auto const unsigned64Min = std::numeric_limits<std::uint64_t>::min();
  auto const unsigned64Max = std::numeric_limits<std::uint64_t>::max();
  requireIntegerBoundaryVector<rti::HLAinteger64BE>(
      signed64Min,
      {0x80U, 0U, 0U, 0U, 0U, 0U, 0U, 0U},
      "HLAinteger64BE minimum");
  requireIntegerBoundaryVector<rti::HLAinteger64LE>(
      signed64Min,
      {0U, 0U, 0U, 0U, 0U, 0U, 0U, 0x80U},
      "HLAinteger64LE minimum");
  requireIntegerBoundaryVector<rti::HLAinteger64BE>(
      signed64Max,
      {0x7fU, 0xffU, 0xffU, 0xffU, 0xffU, 0xffU, 0xffU, 0xffU},
      "HLAinteger64BE maximum");
  requireIntegerBoundaryVector<rti::HLAinteger64LE>(
      signed64Max,
      {0xffU, 0xffU, 0xffU, 0xffU, 0xffU, 0xffU, 0xffU, 0x7fU},
      "HLAinteger64LE maximum");
  requireIntegerBoundaryVector<rti::HLAunsignedInteger64BE>(
      unsigned64Min,
      {0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U},
      "HLAunsignedInteger64BE minimum");
  requireIntegerBoundaryVector<rti::HLAunsignedInteger64LE>(
      unsigned64Min,
      {0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U},
      "HLAunsignedInteger64LE minimum");
  requireIntegerBoundaryVector<rti::HLAunsignedInteger64BE>(
      unsigned64Max,
      {0xffU, 0xffU, 0xffU, 0xffU, 0xffU, 0xffU, 0xffU, 0xffU},
      "HLAunsignedInteger64BE maximum");
  requireIntegerBoundaryVector<rti::HLAunsignedInteger64LE>(
      unsigned64Max,
      {0xffU, 0xffU, 0xffU, 0xffU, 0xffU, 0xffU, 0xffU, 0xffU},
      "HLAunsignedInteger64LE maximum");
}

void verifyBasicDataElementsIntegerContract() {
  verifyFixedWidthIntegerBoundariesContract();
  auto const signed16 = static_cast<rti::Integer16>(-2);
  auto const signed32 = static_cast<rti::Integer32>(-2);
  auto const signed64 = static_cast<rti::Integer64>(-2);
  auto const unsigned16 = static_cast<rti::UnsignedInteger16>(0x1234U);
  auto const unsigned32 = static_cast<rti::UnsignedInteger32>(0x01234567U);
  auto const unsigned64 =
      static_cast<rti::UnsignedInteger64>(0x0123456789abcdefULL);
  requireNonterminalIntegerCursor<rti::HLAinteger16BE>(
      signed16, 2U, "HLAinteger16BE");
  requireNonterminalIntegerCursor<rti::HLAinteger16LE>(
      signed16, 2U, "HLAinteger16LE");
  requireNonterminalIntegerCursor<rti::HLAinteger32BE>(
      signed32, 4U, "HLAinteger32BE");
  requireNonterminalIntegerCursor<rti::HLAinteger32LE>(
      signed32, 4U, "HLAinteger32LE");
  requireNonterminalIntegerCursor<rti::HLAinteger64BE>(
      signed64, 8U, "HLAinteger64BE");
  requireNonterminalIntegerCursor<rti::HLAinteger64LE>(
      signed64, 8U, "HLAinteger64LE");
  requireNonterminalIntegerCursor<rti::HLAunsignedInteger16BE>(
      unsigned16, 2U, "HLAunsignedInteger16BE");
  requireNonterminalIntegerCursor<rti::HLAunsignedInteger16LE>(
      unsigned16, 2U, "HLAunsignedInteger16LE");
  requireNonterminalIntegerCursor<rti::HLAunsignedInteger32BE>(
      unsigned32, 4U, "HLAunsignedInteger32BE");
  requireNonterminalIntegerCursor<rti::HLAunsignedInteger32LE>(
      unsigned32, 4U, "HLAunsignedInteger32LE");
  requireNonterminalIntegerCursor<rti::HLAunsignedInteger64BE>(
      unsigned64, 8U, "HLAunsignedInteger64BE");
  requireNonterminalIntegerCursor<rti::HLAunsignedInteger64LE>(
      unsigned64, 8U, "HLAunsignedInteger64LE");
  rti::HLAinteger16BE signed16Be{-2};
  rti::HLAinteger16LE signed16Le{-2};
  rti::HLAinteger32BE signed32Be{-2};
  rti::HLAinteger32LE signed32Le{-2};
  rti::HLAinteger64BE signed64Be{-2};
  rti::HLAinteger64LE signed64Le{-2};
  requireEncodedBytes(signed16Be, {0xffU, 0xfeU}, "HLAinteger16BE");
  std::vector<rti::Octet> signed16BeRawBuffer{
      static_cast<rti::Octet>(0x7eU)};
  signed16Be.encodeInto(signed16BeRawBuffer);
  std::vector<rti::Octet> signed16BeRawExpected{
      static_cast<rti::Octet>(0x7eU),
      static_cast<rti::Octet>(0xffU),
      static_cast<rti::Octet>(0xfeU)};
  require(
      signed16BeRawBuffer == signed16BeRawExpected,
      "HLAinteger16BE encodeInto did not append its big-endian bytes");
  rti::HLAinteger16BE rawDecodedSigned16Be;
  auto const rawSigned16BeCursor =
      rawDecodedSigned16Be.decodeFrom(signed16BeRawBuffer, 1U);
  require(
      rawSigned16BeCursor == signed16BeRawBuffer.size() &&
          rawDecodedSigned16Be.get() == -2,
      "HLAinteger16BE decodeFrom did not decode the prefixed slice");
  std::vector<rti::Octet> truncatedSigned16BeRawBuffer{
      static_cast<rti::Octet>(0x7eU), static_cast<rti::Octet>(0xffU)};
  rti::HLAinteger16BE truncatedDecodedSigned16Be;
  requireException(
      [&] {
        truncatedDecodedSigned16Be.decodeFrom(
            truncatedSigned16BeRawBuffer, 1U);
      },
      L"EncoderException",
      "truncated HLAinteger16BE decodeFrom buffer");
  rti::HLAinteger16BE endIndexDecodedSigned16Be;
  requireException(
      [&] {
        endIndexDecodedSigned16Be.decodeFrom(
            signed16BeRawBuffer, signed16BeRawBuffer.size());
      },
      L"EncoderException",
      "end-of-buffer HLAinteger16BE decodeFrom index");
  rti::HLAinteger16BE outOfRangeDecodedSigned16Be;
  requireException(
      [&] {
        outOfRangeDecodedSigned16Be.decodeFrom(
            signed16BeRawBuffer, signed16BeRawBuffer.size() + 1U);
      },
      L"EncoderException",
      "out-of-range HLAinteger16BE decodeFrom index");
  requireEncodedBytes(signed16Le, {0xfeU, 0xffU}, "HLAinteger16LE");
  std::vector<rti::Octet> signed16LeRawBuffer{
      static_cast<rti::Octet>(0x7eU)};
  signed16Le.encodeInto(signed16LeRawBuffer);
  std::vector<rti::Octet> signed16LeRawExpected{
      static_cast<rti::Octet>(0x7eU),
      static_cast<rti::Octet>(0xfeU),
      static_cast<rti::Octet>(0xffU)};
  require(
      signed16LeRawBuffer == signed16LeRawExpected,
      "HLAinteger16LE encodeInto did not append its little-endian bytes");
  rti::HLAinteger16LE rawDecodedSigned16Le;
  auto const rawSigned16LeCursor =
      rawDecodedSigned16Le.decodeFrom(signed16LeRawBuffer, 1U);
  require(
      rawSigned16LeCursor == signed16LeRawBuffer.size() &&
          rawDecodedSigned16Le.get() == -2,
      "HLAinteger16LE decodeFrom did not decode the prefixed slice");
  std::vector<rti::Octet> truncatedSigned16LeRawBuffer{
      static_cast<rti::Octet>(0x7eU), static_cast<rti::Octet>(0xfeU)};
  rti::HLAinteger16LE truncatedDecodedSigned16Le;
  requireException(
      [&] {
        truncatedDecodedSigned16Le.decodeFrom(
            truncatedSigned16LeRawBuffer, 1U);
      },
      L"EncoderException",
      "truncated HLAinteger16LE decodeFrom buffer");
  rti::HLAinteger16LE endIndexDecodedSigned16Le;
  requireException(
      [&] {
        endIndexDecodedSigned16Le.decodeFrom(
            signed16LeRawBuffer, signed16LeRawBuffer.size());
      },
      L"EncoderException",
      "end-of-buffer HLAinteger16LE decodeFrom index");
  rti::HLAinteger16LE outOfRangeDecodedSigned16Le;
  requireException(
      [&] {
        outOfRangeDecodedSigned16Le.decodeFrom(
            signed16LeRawBuffer, signed16LeRawBuffer.size() + 1U);
      },
      L"EncoderException",
      "out-of-range HLAinteger16LE decodeFrom index");
  requireEncodedBytes(
      signed32Be,
      {0xffU, 0xffU, 0xffU, 0xfeU},
      "HLAinteger32BE");
  std::vector<rti::Octet> signed32BeRawBuffer{
      static_cast<rti::Octet>(0x7eU)};
  signed32Be.encodeInto(signed32BeRawBuffer);
  std::vector<rti::Octet> signed32BeRawExpected{
      static_cast<rti::Octet>(0x7eU),
      static_cast<rti::Octet>(0xffU),
      static_cast<rti::Octet>(0xffU),
      static_cast<rti::Octet>(0xffU),
      static_cast<rti::Octet>(0xfeU)};
  require(
      signed32BeRawBuffer == signed32BeRawExpected,
      "HLAinteger32BE encodeInto did not append its big-endian bytes");
  rti::HLAinteger32BE rawDecodedSigned32;
  auto const rawSigned32Cursor =
      rawDecodedSigned32.decodeFrom(signed32BeRawBuffer, 1U);
  require(
      rawSigned32Cursor == signed32BeRawBuffer.size() &&
          rawDecodedSigned32.get() == -2,
      "HLAinteger32BE decodeFrom did not decode the prefixed slice");
  std::vector<rti::Octet> truncatedSigned32BeRawBuffer{
      static_cast<rti::Octet>(0x7eU), static_cast<rti::Octet>(0xffU),
      static_cast<rti::Octet>(0xffU), static_cast<rti::Octet>(0xffU)};
  rti::HLAinteger32BE truncatedDecodedSigned32Be;
  requireException(
      [&] {
        truncatedDecodedSigned32Be.decodeFrom(
            truncatedSigned32BeRawBuffer, 1U);
      },
      L"EncoderException",
      "truncated HLAinteger32BE decodeFrom buffer");
  rti::HLAinteger32BE outOfRangeDecodedSigned32Be;
  requireException(
      [&] {
        outOfRangeDecodedSigned32Be.decodeFrom(
            signed32BeRawBuffer, signed32BeRawBuffer.size() + 1U);
      },
      L"EncoderException",
      "out-of-range HLAinteger32BE decodeFrom index");
  requireEncodedBytes(
      signed32Le,
      {0xfeU, 0xffU, 0xffU, 0xffU},
      "HLAinteger32LE");
  std::vector<rti::Octet> signed32LeRawBuffer{
      static_cast<rti::Octet>(0x7eU)};
  signed32Le.encodeInto(signed32LeRawBuffer);
  std::vector<rti::Octet> signed32LeRawExpected{
      static_cast<rti::Octet>(0x7eU),
      static_cast<rti::Octet>(0xfeU),
      static_cast<rti::Octet>(0xffU),
      static_cast<rti::Octet>(0xffU),
      static_cast<rti::Octet>(0xffU)};
  require(
      signed32LeRawBuffer == signed32LeRawExpected,
      "HLAinteger32LE encodeInto did not append its little-endian bytes");
  rti::HLAinteger32LE rawDecodedSigned32Le;
  auto const rawSigned32LeCursor =
      rawDecodedSigned32Le.decodeFrom(signed32LeRawBuffer, 1U);
  require(
      rawSigned32LeCursor == signed32LeRawBuffer.size() &&
          rawDecodedSigned32Le.get() == -2,
      "HLAinteger32LE decodeFrom did not decode the prefixed slice");
  std::vector<rti::Octet> truncatedSigned32LeRawBuffer{
      static_cast<rti::Octet>(0x7eU), static_cast<rti::Octet>(0xfeU),
      static_cast<rti::Octet>(0xffU), static_cast<rti::Octet>(0xffU)};
  rti::HLAinteger32LE truncatedDecodedSigned32Le;
  requireException(
      [&] {
        truncatedDecodedSigned32Le.decodeFrom(
            truncatedSigned32LeRawBuffer, 1U);
      },
      L"EncoderException",
      "truncated HLAinteger32LE decodeFrom buffer");
  rti::HLAinteger32LE outOfRangeDecodedSigned32Le;
  requireException(
      [&] {
        outOfRangeDecodedSigned32Le.decodeFrom(
            signed32LeRawBuffer, signed32LeRawBuffer.size() + 1U);
      },
      L"EncoderException",
      "out-of-range HLAinteger32LE decodeFrom index");
  rti::Integer32 externalSigned32BeValue = static_cast<rti::Integer32>(-2);
  rti::HLAinteger32BE externalSigned32Be{&externalSigned32BeValue};
  externalSigned32BeValue = static_cast<rti::Integer32>(0x12345678);
  std::vector<rti::Octet> externalSigned32BeEncoding{
      static_cast<rti::Octet>(0x7eU)};
  externalSigned32Be.encodeInto(externalSigned32BeEncoding);
  require(
      externalSigned32Be.get() == static_cast<rti::Integer32>(0x12345678) &&
          externalSigned32BeEncoding ==
              std::vector<rti::Octet>{
                  static_cast<rti::Octet>(0x7eU),
                  static_cast<rti::Octet>(0x12U),
                  static_cast<rti::Octet>(0x34U),
                  static_cast<rti::Octet>(0x56U),
                  static_cast<rti::Octet>(0x78U)},
      "HLAinteger32BE did not observe caller-owned memory changes");
  externalSigned32Be.set(static_cast<rti::Integer32>(-7));
  require(
      externalSigned32BeValue == static_cast<rti::Integer32>(-7),
      "HLAinteger32BE set did not update caller-owned memory");
  rti::HLAinteger32BE decodedExternalSigned32BeValue{
      static_cast<rti::Integer32>(1000)};
  externalSigned32Be.decode(decodedExternalSigned32BeValue.encode());
  require(
      externalSigned32BeValue == static_cast<rti::Integer32>(1000) &&
          externalSigned32Be.get() == static_cast<rti::Integer32>(1000),
      "HLAinteger32BE decode did not update caller-owned memory");

  rti::Integer32 externalSigned32LeValue = static_cast<rti::Integer32>(-2);
  rti::HLAinteger32LE externalSigned32Le{&externalSigned32LeValue};
  externalSigned32LeValue = static_cast<rti::Integer32>(0x12345678);
  std::vector<rti::Octet> externalSigned32LeEncoding{
      static_cast<rti::Octet>(0x7eU)};
  externalSigned32Le.encodeInto(externalSigned32LeEncoding);
  require(
      externalSigned32Le.get() == static_cast<rti::Integer32>(0x12345678) &&
          externalSigned32LeEncoding ==
              std::vector<rti::Octet>{
                  static_cast<rti::Octet>(0x7eU),
                  static_cast<rti::Octet>(0x78U),
                  static_cast<rti::Octet>(0x56U),
                  static_cast<rti::Octet>(0x34U),
                  static_cast<rti::Octet>(0x12U)},
      "HLAinteger32LE did not observe caller-owned memory changes");
  externalSigned32Le.set(static_cast<rti::Integer32>(-7));
  require(
      externalSigned32LeValue == static_cast<rti::Integer32>(-7),
      "HLAinteger32LE set did not update caller-owned memory");
  rti::HLAinteger32LE decodedExternalSigned32LeValue{
      static_cast<rti::Integer32>(1000)};
  externalSigned32Le.decode(decodedExternalSigned32LeValue.encode());
  require(
      externalSigned32LeValue == static_cast<rti::Integer32>(1000) &&
          externalSigned32Le.get() == static_cast<rti::Integer32>(1000),
      "HLAinteger32LE decode did not update caller-owned memory");
  requireEncodedBytes(
      signed64Be,
      {0xffU, 0xffU, 0xffU, 0xffU, 0xffU, 0xffU, 0xffU, 0xfeU},
      "HLAinteger64BE");
  std::vector<rti::Octet> signed64BeRawBuffer{
      static_cast<rti::Octet>(0x7eU)};
  signed64Be.encodeInto(signed64BeRawBuffer);
  std::vector<rti::Octet> signed64BeRawExpected{
      static_cast<rti::Octet>(0x7eU),
      static_cast<rti::Octet>(0xffU),
      static_cast<rti::Octet>(0xffU),
      static_cast<rti::Octet>(0xffU),
      static_cast<rti::Octet>(0xffU),
      static_cast<rti::Octet>(0xffU),
      static_cast<rti::Octet>(0xffU),
      static_cast<rti::Octet>(0xffU),
      static_cast<rti::Octet>(0xfeU)};
  require(
      signed64BeRawBuffer == signed64BeRawExpected,
      "HLAinteger64BE encodeInto did not append its big-endian bytes");
  rti::HLAinteger64BE rawDecodedSigned64Be;
  auto const rawSigned64BeCursor =
      rawDecodedSigned64Be.decodeFrom(signed64BeRawBuffer, 1U);
  require(
      rawSigned64BeCursor == signed64BeRawBuffer.size() &&
          rawDecodedSigned64Be.get() == -2,
      "HLAinteger64BE decodeFrom did not decode the prefixed slice");
  std::vector<rti::Octet> truncatedSigned64BeRawBuffer{
      static_cast<rti::Octet>(0x7eU), static_cast<rti::Octet>(0xffU),
      static_cast<rti::Octet>(0xffU), static_cast<rti::Octet>(0xffU),
      static_cast<rti::Octet>(0xffU), static_cast<rti::Octet>(0xffU),
      static_cast<rti::Octet>(0xffU), static_cast<rti::Octet>(0xffU)};
  rti::HLAinteger64BE truncatedDecodedSigned64Be;
  requireException(
      [&] {
        truncatedDecodedSigned64Be.decodeFrom(
            truncatedSigned64BeRawBuffer, 1U);
      },
      L"EncoderException",
      "truncated HLAinteger64BE decodeFrom buffer");
  rti::HLAinteger64BE outOfRangeDecodedSigned64Be;
  requireException(
      [&] {
        outOfRangeDecodedSigned64Be.decodeFrom(
            signed64BeRawBuffer, signed64BeRawBuffer.size() + 1U);
      },
      L"EncoderException",
      "out-of-range HLAinteger64BE decodeFrom index");
  requireEncodedBytes(
      signed64Le,
      {0xfeU, 0xffU, 0xffU, 0xffU, 0xffU, 0xffU, 0xffU, 0xffU},
      "HLAinteger64LE");
  std::vector<rti::Octet> signed64LeRawBuffer{
      static_cast<rti::Octet>(0x7eU)};
  signed64Le.encodeInto(signed64LeRawBuffer);
  rti::HLAoctet followingSigned64LeOctet{
      static_cast<rti::Octet>(0xa5U)};
  followingSigned64LeOctet.encodeInto(signed64LeRawBuffer);
  std::vector<rti::Octet> signed64LeRawExpected{
      static_cast<rti::Octet>(0x7eU),
      static_cast<rti::Octet>(0xfeU),
      static_cast<rti::Octet>(0xffU),
      static_cast<rti::Octet>(0xffU),
      static_cast<rti::Octet>(0xffU),
      static_cast<rti::Octet>(0xffU),
      static_cast<rti::Octet>(0xffU),
      static_cast<rti::Octet>(0xffU),
      static_cast<rti::Octet>(0xffU),
      static_cast<rti::Octet>(0xa5U)};
  require(
      signed64LeRawBuffer == signed64LeRawExpected,
      "HLAinteger64LE encodeInto did not append its little-endian bytes");
  rti::HLAinteger64LE rawDecodedSigned64Le;
  auto const rawSigned64LeCursor =
      rawDecodedSigned64Le.decodeFrom(signed64LeRawBuffer, 1U);
  require(
      rawSigned64LeCursor == 1U + 8U &&
          rawDecodedSigned64Le.get() == -2,
      "HLAinteger64LE decodeFrom did not stop before the following octet");
  require(
      signed64LeRawBuffer.at(rawSigned64LeCursor) ==
          static_cast<rti::Octet>(0xa5U),
      "HLAinteger64LE decodeFrom consumed the following octet");
  rti::HLAoctet decodedFollowingSigned64LeOctet;
  auto const followingSigned64LeCursor =
      decodedFollowingSigned64LeOctet.decodeFrom(
          signed64LeRawBuffer, rawSigned64LeCursor);
  require(
      followingSigned64LeCursor == signed64LeRawBuffer.size() &&
          decodedFollowingSigned64LeOctet.get() ==
              static_cast<rti::Octet>(0xa5U),
      "following HLAoctet did not decode from the returned cursor");
  std::vector<rti::Octet> truncatedSigned64LeRawBuffer{
      static_cast<rti::Octet>(0x7eU), static_cast<rti::Octet>(0xfeU),
      static_cast<rti::Octet>(0xffU), static_cast<rti::Octet>(0xffU),
      static_cast<rti::Octet>(0xffU), static_cast<rti::Octet>(0xffU),
      static_cast<rti::Octet>(0xffU), static_cast<rti::Octet>(0xffU)};
  rti::HLAinteger64LE truncatedDecodedSigned64Le;
  requireException(
      [&] {
        truncatedDecodedSigned64Le.decodeFrom(
            truncatedSigned64LeRawBuffer, 1U);
      },
      L"EncoderException",
      "truncated HLAinteger64LE decodeFrom buffer");
  rti::HLAinteger64LE outOfRangeDecodedSigned64Le;
  requireException(
      [&] {
        outOfRangeDecodedSigned64Le.decodeFrom(
            signed64LeRawBuffer, signed64LeRawBuffer.size() + 1U);
      },
      L"EncoderException",
      "out-of-range HLAinteger64LE decodeFrom index");
  rti::Integer64 externalSigned64BeValue = static_cast<rti::Integer64>(-2);
  rti::HLAinteger64BE externalSigned64Be{&externalSigned64BeValue};
  externalSigned64BeValue = static_cast<rti::Integer64>(0x123456789abcdef0LL);
  std::vector<rti::Octet> externalSigned64BeEncoding{
      static_cast<rti::Octet>(0x7eU)};
  externalSigned64Be.encodeInto(externalSigned64BeEncoding);
  require(
      externalSigned64Be.get() ==
              static_cast<rti::Integer64>(0x123456789abcdef0LL) &&
          externalSigned64BeEncoding ==
              std::vector<rti::Octet>{
                  static_cast<rti::Octet>(0x7eU),
                  static_cast<rti::Octet>(0x12U),
                  static_cast<rti::Octet>(0x34U),
                  static_cast<rti::Octet>(0x56U),
                  static_cast<rti::Octet>(0x78U),
                  static_cast<rti::Octet>(0x9aU),
                  static_cast<rti::Octet>(0xbcU),
                  static_cast<rti::Octet>(0xdeU),
                  static_cast<rti::Octet>(0xf0U)},
      "HLAinteger64BE did not observe caller-owned memory changes");
  externalSigned64Be.set(static_cast<rti::Integer64>(-7));
  require(
      externalSigned64BeValue == static_cast<rti::Integer64>(-7),
      "HLAinteger64BE set did not update caller-owned memory");
  rti::HLAinteger64BE decodedExternalSigned64BeValue{
      static_cast<rti::Integer64>(0x0102030405060708LL)};
  externalSigned64Be.decode(decodedExternalSigned64BeValue.encode());
  require(
      externalSigned64BeValue ==
              static_cast<rti::Integer64>(0x0102030405060708LL) &&
          externalSigned64Be.get() ==
              static_cast<rti::Integer64>(0x0102030405060708LL),
      "HLAinteger64BE decode did not update caller-owned memory");

  rti::Integer64 externalSigned64LeValue = static_cast<rti::Integer64>(-2);
  rti::HLAinteger64LE externalSigned64Le{&externalSigned64LeValue};
  externalSigned64LeValue = static_cast<rti::Integer64>(0x123456789abcdef0LL);
  std::vector<rti::Octet> externalSigned64LeEncoding{
      static_cast<rti::Octet>(0x7eU)};
  externalSigned64Le.encodeInto(externalSigned64LeEncoding);
  require(
      externalSigned64Le.get() ==
              static_cast<rti::Integer64>(0x123456789abcdef0LL) &&
          externalSigned64LeEncoding ==
              std::vector<rti::Octet>{
                  static_cast<rti::Octet>(0x7eU),
                  static_cast<rti::Octet>(0xf0U),
                  static_cast<rti::Octet>(0xdeU),
                  static_cast<rti::Octet>(0xbcU),
                  static_cast<rti::Octet>(0x9aU),
                  static_cast<rti::Octet>(0x78U),
                  static_cast<rti::Octet>(0x56U),
                  static_cast<rti::Octet>(0x34U),
                  static_cast<rti::Octet>(0x12U)},
      "HLAinteger64LE did not observe caller-owned memory changes");
  externalSigned64Le.set(static_cast<rti::Integer64>(-7));
  require(
      externalSigned64LeValue == static_cast<rti::Integer64>(-7),
      "HLAinteger64LE set did not update caller-owned memory");
  rti::HLAinteger64LE decodedExternalSigned64LeValue{
      static_cast<rti::Integer64>(0x0102030405060708LL)};
  externalSigned64Le.decode(decodedExternalSigned64LeValue.encode());
  require(
      externalSigned64LeValue ==
              static_cast<rti::Integer64>(0x0102030405060708LL) &&
          externalSigned64Le.get() ==
              static_cast<rti::Integer64>(0x0102030405060708LL),
      "HLAinteger64LE decode did not update caller-owned memory");
  require(
      signed32Be.getEncodedLength() == 4U && signed32Be.getOctetBoundary() == 4U,
      "signed basic integer metadata was incorrect");

  rti::HLAinteger32BE decodedSigned32;
  decodedSigned32.decode(signed32Be.encode());
  require(decodedSigned32.get() == -2, "HLAinteger32BE did not round-trip");

  rti::HLAunsignedInteger16BE unsigned16Be{0x1234U};
  rti::HLAunsignedInteger16LE unsigned16Le{0x1234U};
  rti::HLAunsignedInteger32BE unsigned32Be{0x1234abcdU};
  rti::HLAunsignedInteger32LE unsigned32Le{0x1234abcdU};
  rti::HLAunsignedInteger64BE unsigned64Be{0x0123456789abcdefULL};
  rti::HLAunsignedInteger64LE unsigned64Le{0x0123456789abcdefULL};
  requireEncodedBytes(unsigned16Be, {0x12U, 0x34U}, "HLAunsignedInteger16BE");
  std::vector<rti::Octet> unsigned16BeRawBuffer{
      static_cast<rti::Octet>(0x7eU)};
  unsigned16Be.encodeInto(unsigned16BeRawBuffer);
  std::vector<rti::Octet> unsigned16BeRawExpected{
      static_cast<rti::Octet>(0x7eU),
      static_cast<rti::Octet>(0x12U),
      static_cast<rti::Octet>(0x34U)};
  require(
      unsigned16BeRawBuffer == unsigned16BeRawExpected,
      "HLAunsignedInteger16BE encodeInto did not append its big-endian bytes");
  rti::HLAunsignedInteger16BE rawDecodedUnsigned16Be;
  auto const rawUnsigned16BeCursor =
      rawDecodedUnsigned16Be.decodeFrom(unsigned16BeRawBuffer, 1U);
  require(
      rawUnsigned16BeCursor == unsigned16BeRawBuffer.size() &&
          rawDecodedUnsigned16Be.get() == 0x1234U,
      "HLAunsignedInteger16BE decodeFrom did not decode the prefixed slice");
  std::vector<rti::Octet> truncatedUnsigned16BeRawBuffer{
      static_cast<rti::Octet>(0x7eU), static_cast<rti::Octet>(0x12U)};
  rti::HLAunsignedInteger16BE truncatedDecodedUnsigned16Be;
  requireException(
      [&] {
        truncatedDecodedUnsigned16Be.decodeFrom(
            truncatedUnsigned16BeRawBuffer, 1U);
      },
      L"EncoderException",
      "truncated HLAunsignedInteger16BE decodeFrom buffer");
  rti::HLAunsignedInteger16BE outOfRangeDecodedUnsigned16Be;
  requireException(
      [&] {
        outOfRangeDecodedUnsigned16Be.decodeFrom(
            unsigned16BeRawBuffer, unsigned16BeRawBuffer.size() + 1U);
      },
      L"EncoderException",
      "out-of-range HLAunsignedInteger16BE decodeFrom index");
  requireEncodedBytes(unsigned16Le, {0x34U, 0x12U}, "HLAunsignedInteger16LE");
  std::vector<rti::Octet> unsigned16LeRawBuffer{
      static_cast<rti::Octet>(0x7eU)};
  unsigned16Le.encodeInto(unsigned16LeRawBuffer);
  std::vector<rti::Octet> unsigned16LeRawExpected{
      static_cast<rti::Octet>(0x7eU),
      static_cast<rti::Octet>(0x34U),
      static_cast<rti::Octet>(0x12U)};
  require(
      unsigned16LeRawBuffer == unsigned16LeRawExpected,
      "HLAunsignedInteger16LE encodeInto did not append its little-endian bytes");
  rti::HLAunsignedInteger16LE rawDecodedUnsigned16Le;
  auto const rawUnsigned16LeCursor =
      rawDecodedUnsigned16Le.decodeFrom(unsigned16LeRawBuffer, 1U);
  require(
      rawUnsigned16LeCursor == unsigned16LeRawBuffer.size() &&
          rawDecodedUnsigned16Le.get() == 0x1234U,
      "HLAunsignedInteger16LE decodeFrom did not decode the prefixed slice");
  std::vector<rti::Octet> truncatedUnsigned16LeRawBuffer{
      static_cast<rti::Octet>(0x7eU), static_cast<rti::Octet>(0x34U)};
  rti::HLAunsignedInteger16LE truncatedDecodedUnsigned16Le;
  requireException(
      [&] {
        truncatedDecodedUnsigned16Le.decodeFrom(
            truncatedUnsigned16LeRawBuffer, 1U);
      },
      L"EncoderException",
      "truncated HLAunsignedInteger16LE decodeFrom buffer");
  rti::HLAunsignedInteger16LE outOfRangeDecodedUnsigned16Le;
  requireException(
      [&] {
        outOfRangeDecodedUnsigned16Le.decodeFrom(
            unsigned16LeRawBuffer, unsigned16LeRawBuffer.size() + 1U);
      },
      L"EncoderException",
      "out-of-range HLAunsignedInteger16LE decodeFrom index");
  requireEncodedBytes(
      unsigned32Be,
      {0x12U, 0x34U, 0xabU, 0xcdU},
      "HLAunsignedInteger32BE");
  std::vector<rti::Octet> unsigned32BeRawBuffer{
      static_cast<rti::Octet>(0x7eU)};
  unsigned32Be.encodeInto(unsigned32BeRawBuffer);
  std::vector<rti::Octet> unsigned32BeRawExpected{
      static_cast<rti::Octet>(0x7eU),
      static_cast<rti::Octet>(0x12U),
      static_cast<rti::Octet>(0x34U),
      static_cast<rti::Octet>(0xabU),
      static_cast<rti::Octet>(0xcdU)};
  require(
      unsigned32BeRawBuffer == unsigned32BeRawExpected,
      "HLAunsignedInteger32BE encodeInto did not append its big-endian bytes");
  rti::HLAunsignedInteger32BE rawDecodedUnsigned32Be;
  auto const rawUnsigned32BeCursor =
      rawDecodedUnsigned32Be.decodeFrom(unsigned32BeRawBuffer, 1U);
  require(
      rawUnsigned32BeCursor == unsigned32BeRawBuffer.size() &&
          rawDecodedUnsigned32Be.get() == 0x1234abcdU,
      "HLAunsignedInteger32BE decodeFrom did not decode the prefixed slice");
  std::vector<rti::Octet> truncatedUnsigned32BeRawBuffer{
      static_cast<rti::Octet>(0x7eU), static_cast<rti::Octet>(0x12U),
      static_cast<rti::Octet>(0x34U), static_cast<rti::Octet>(0xabU)};
  rti::HLAunsignedInteger32BE truncatedDecodedUnsigned32Be;
  requireException(
      [&] {
        truncatedDecodedUnsigned32Be.decodeFrom(
            truncatedUnsigned32BeRawBuffer, 1U);
      },
      L"EncoderException",
      "truncated HLAunsignedInteger32BE decodeFrom buffer");
  rti::HLAunsignedInteger32BE outOfRangeDecodedUnsigned32Be;
  requireException(
      [&] {
        outOfRangeDecodedUnsigned32Be.decodeFrom(
            unsigned32BeRawBuffer, unsigned32BeRawBuffer.size() + 1U);
      },
      L"EncoderException",
      "out-of-range HLAunsignedInteger32BE decodeFrom index");
  requireEncodedBytes(
      unsigned32Le,
      {0xcdU, 0xabU, 0x34U, 0x12U},
      "HLAunsignedInteger32LE");
  std::vector<rti::Octet> unsigned32LeRawBuffer{
      static_cast<rti::Octet>(0x7eU)};
  unsigned32Le.encodeInto(unsigned32LeRawBuffer);
  std::vector<rti::Octet> unsigned32LeRawExpected{
      static_cast<rti::Octet>(0x7eU),
      static_cast<rti::Octet>(0xcdU),
      static_cast<rti::Octet>(0xabU),
      static_cast<rti::Octet>(0x34U),
      static_cast<rti::Octet>(0x12U)};
  require(
      unsigned32LeRawBuffer == unsigned32LeRawExpected,
      "HLAunsignedInteger32LE encodeInto did not append its little-endian bytes");
  rti::HLAunsignedInteger32LE rawDecodedUnsigned32Le;
  auto const rawUnsigned32LeCursor =
      rawDecodedUnsigned32Le.decodeFrom(unsigned32LeRawBuffer, 1U);
  require(
      rawUnsigned32LeCursor == unsigned32LeRawBuffer.size() &&
          rawDecodedUnsigned32Le.get() == 0x1234abcdU,
      "HLAunsignedInteger32LE decodeFrom did not decode the prefixed slice");
  std::vector<rti::Octet> truncatedUnsigned32LeRawBuffer{
      static_cast<rti::Octet>(0x7eU), static_cast<rti::Octet>(0xcdU),
      static_cast<rti::Octet>(0xabU), static_cast<rti::Octet>(0x34U)};
  rti::HLAunsignedInteger32LE truncatedDecodedUnsigned32Le;
  requireException(
      [&] {
        truncatedDecodedUnsigned32Le.decodeFrom(
            truncatedUnsigned32LeRawBuffer, 1U);
      },
      L"EncoderException",
      "truncated HLAunsignedInteger32LE decodeFrom buffer");
  rti::HLAunsignedInteger32LE outOfRangeDecodedUnsigned32Le;
  requireException(
      [&] {
        outOfRangeDecodedUnsigned32Le.decodeFrom(
            unsigned32LeRawBuffer, unsigned32LeRawBuffer.size() + 1U);
      },
      L"EncoderException",
      "out-of-range HLAunsignedInteger32LE decodeFrom index");
  rti::UnsignedInteger32 externalUnsigned32BeValue = 0x12345678U;
  rti::HLAunsignedInteger32BE externalUnsigned32Be{
      &externalUnsigned32BeValue};
  externalUnsigned32BeValue = 0x9abcdef0U;
  std::vector<rti::Octet> externalUnsigned32BeEncoding{
      static_cast<rti::Octet>(0x7eU)};
  externalUnsigned32Be.encodeInto(externalUnsigned32BeEncoding);
  require(
      externalUnsigned32Be.get() == 0x9abcdef0U &&
          externalUnsigned32BeEncoding ==
              std::vector<rti::Octet>{
                  static_cast<rti::Octet>(0x7eU),
                  static_cast<rti::Octet>(0x9aU),
                  static_cast<rti::Octet>(0xbcU),
                  static_cast<rti::Octet>(0xdeU),
                  static_cast<rti::Octet>(0xf0U)},
      "HLAunsignedInteger32BE did not observe caller-owned memory changes");
  externalUnsigned32Be.set(0x87654321U);
  require(
      externalUnsigned32BeValue == 0x87654321U,
      "HLAunsignedInteger32BE set did not update caller-owned memory");
  rti::HLAunsignedInteger32BE decodedExternalUnsigned32BeValue{0x01020304U};
  externalUnsigned32Be.decode(decodedExternalUnsigned32BeValue.encode());
  require(
      externalUnsigned32BeValue == 0x01020304U &&
          externalUnsigned32Be.get() == 0x01020304U,
      "HLAunsignedInteger32BE decode did not update caller-owned memory");

  rti::UnsignedInteger32 externalUnsigned32LeValue = 0x12345678U;
  rti::HLAunsignedInteger32LE externalUnsigned32Le{
      &externalUnsigned32LeValue};
  externalUnsigned32LeValue = 0x9abcdef0U;
  std::vector<rti::Octet> externalUnsigned32LeEncoding{
      static_cast<rti::Octet>(0x7eU)};
  externalUnsigned32Le.encodeInto(externalUnsigned32LeEncoding);
  require(
      externalUnsigned32Le.get() == 0x9abcdef0U &&
          externalUnsigned32LeEncoding ==
              std::vector<rti::Octet>{
                  static_cast<rti::Octet>(0x7eU),
                  static_cast<rti::Octet>(0xf0U),
                  static_cast<rti::Octet>(0xdeU),
                  static_cast<rti::Octet>(0xbcU),
                  static_cast<rti::Octet>(0x9aU)},
      "HLAunsignedInteger32LE did not observe caller-owned memory changes");
  externalUnsigned32Le.set(0x87654321U);
  require(
      externalUnsigned32LeValue == 0x87654321U,
      "HLAunsignedInteger32LE set did not update caller-owned memory");
  rti::HLAunsignedInteger32LE decodedExternalUnsigned32LeValue{0x01020304U};
  externalUnsigned32Le.decode(decodedExternalUnsigned32LeValue.encode());
  require(
      externalUnsigned32LeValue == 0x01020304U &&
          externalUnsigned32Le.get() == 0x01020304U,
      "HLAunsignedInteger32LE decode did not update caller-owned memory");
  requireEncodedBytes(
      unsigned64Be,
      {0x01U, 0x23U, 0x45U, 0x67U, 0x89U, 0xabU, 0xcdU, 0xefU},
      "HLAunsignedInteger64BE");
  std::vector<rti::Octet> unsigned64BeRawBuffer{
      static_cast<rti::Octet>(0x7eU)};
  unsigned64Be.encodeInto(unsigned64BeRawBuffer);
  std::vector<rti::Octet> unsigned64BeRawExpected{
      static_cast<rti::Octet>(0x7eU),
      static_cast<rti::Octet>(0x01U),
      static_cast<rti::Octet>(0x23U),
      static_cast<rti::Octet>(0x45U),
      static_cast<rti::Octet>(0x67U),
      static_cast<rti::Octet>(0x89U),
      static_cast<rti::Octet>(0xabU),
      static_cast<rti::Octet>(0xcdU),
      static_cast<rti::Octet>(0xefU)};
  require(
      unsigned64BeRawBuffer == unsigned64BeRawExpected,
      "HLAunsignedInteger64BE encodeInto did not append its big-endian bytes");
  rti::HLAunsignedInteger64BE rawDecodedUnsigned64Be;
  auto const rawUnsigned64BeCursor =
      rawDecodedUnsigned64Be.decodeFrom(unsigned64BeRawBuffer, 1U);
  require(
      rawUnsigned64BeCursor == unsigned64BeRawBuffer.size() &&
          rawDecodedUnsigned64Be.get() == 0x0123456789abcdefULL,
      "HLAunsignedInteger64BE decodeFrom did not decode the prefixed slice");
  std::vector<rti::Octet> truncatedUnsigned64BeRawBuffer{
      static_cast<rti::Octet>(0x7eU), static_cast<rti::Octet>(0x01U),
      static_cast<rti::Octet>(0x23U), static_cast<rti::Octet>(0x45U),
      static_cast<rti::Octet>(0x67U), static_cast<rti::Octet>(0x89U),
      static_cast<rti::Octet>(0xabU), static_cast<rti::Octet>(0xcdU)};
  rti::HLAunsignedInteger64BE truncatedDecodedUnsigned64Be;
  requireException(
      [&] {
        truncatedDecodedUnsigned64Be.decodeFrom(
            truncatedUnsigned64BeRawBuffer, 1U);
      },
      L"EncoderException",
      "truncated HLAunsignedInteger64BE decodeFrom buffer");
  rti::HLAunsignedInteger64BE outOfRangeDecodedUnsigned64Be;
  requireException(
      [&] {
        outOfRangeDecodedUnsigned64Be.decodeFrom(
            unsigned64BeRawBuffer, unsigned64BeRawBuffer.size() + 1U);
      },
      L"EncoderException",
      "out-of-range HLAunsignedInteger64BE decodeFrom index");
  requireEncodedBytes(
      unsigned64Le,
      {0xefU, 0xcdU, 0xabU, 0x89U, 0x67U, 0x45U, 0x23U, 0x01U},
      "HLAunsignedInteger64LE");
  std::vector<rti::Octet> unsigned64LeRawBuffer{
      static_cast<rti::Octet>(0x7eU)};
  unsigned64Le.encodeInto(unsigned64LeRawBuffer);
  std::vector<rti::Octet> unsigned64LeRawExpected{
      static_cast<rti::Octet>(0x7eU),
      static_cast<rti::Octet>(0xefU),
      static_cast<rti::Octet>(0xcdU),
      static_cast<rti::Octet>(0xabU),
      static_cast<rti::Octet>(0x89U),
      static_cast<rti::Octet>(0x67U),
      static_cast<rti::Octet>(0x45U),
      static_cast<rti::Octet>(0x23U),
      static_cast<rti::Octet>(0x01U)};
  require(
      unsigned64LeRawBuffer == unsigned64LeRawExpected,
      "HLAunsignedInteger64LE encodeInto did not append its little-endian bytes");
  rti::HLAunsignedInteger64LE rawDecodedUnsigned64Le;
  auto const rawUnsigned64LeCursor =
      rawDecodedUnsigned64Le.decodeFrom(unsigned64LeRawBuffer, 1U);
  require(
      rawUnsigned64LeCursor == unsigned64LeRawBuffer.size() &&
          rawDecodedUnsigned64Le.get() == 0x0123456789abcdefULL,
      "HLAunsignedInteger64LE decodeFrom did not decode the prefixed slice");
  std::vector<rti::Octet> truncatedUnsigned64LeRawBuffer{
      static_cast<rti::Octet>(0x7eU), static_cast<rti::Octet>(0xefU),
      static_cast<rti::Octet>(0xcdU), static_cast<rti::Octet>(0xabU),
      static_cast<rti::Octet>(0x89U), static_cast<rti::Octet>(0x67U),
      static_cast<rti::Octet>(0x45U), static_cast<rti::Octet>(0x23U)};
  rti::HLAunsignedInteger64LE truncatedDecodedUnsigned64Le;
  requireException(
      [&] {
        truncatedDecodedUnsigned64Le.decodeFrom(
            truncatedUnsigned64LeRawBuffer, 1U);
      },
      L"EncoderException",
      "truncated HLAunsignedInteger64LE decodeFrom buffer");
  rti::HLAunsignedInteger64LE outOfRangeDecodedUnsigned64Le;
  requireException(
      [&] {
        outOfRangeDecodedUnsigned64Le.decodeFrom(
            unsigned64LeRawBuffer, unsigned64LeRawBuffer.size() + 1U);
      },
      L"EncoderException",
      "out-of-range HLAunsignedInteger64LE decodeFrom index");
  rti::HLAunsignedInteger64LE decodedUnsigned64;
  decodedUnsigned64.decode(unsigned64Le.encode());
  require(
      decodedUnsigned64.get() == 0x0123456789abcdefULL,
      "HLAunsignedInteger64LE did not round-trip");

  rti::UnsignedInteger64 externalUnsigned64BeValue =
      0x0123456789abcdefULL;
  rti::HLAunsignedInteger64BE externalUnsigned64Be{
      &externalUnsigned64BeValue};
  externalUnsigned64BeValue = 0x9abcdef012345678ULL;
  std::vector<rti::Octet> externalUnsigned64BeEncoding{
      static_cast<rti::Octet>(0x7eU)};
  externalUnsigned64Be.encodeInto(externalUnsigned64BeEncoding);
  require(
      externalUnsigned64Be.get() == 0x9abcdef012345678ULL &&
          externalUnsigned64BeEncoding ==
              std::vector<rti::Octet>{
                  static_cast<rti::Octet>(0x7eU),
                  static_cast<rti::Octet>(0x9aU),
                  static_cast<rti::Octet>(0xbcU),
                  static_cast<rti::Octet>(0xdeU),
                  static_cast<rti::Octet>(0xf0U),
                  static_cast<rti::Octet>(0x12U),
                  static_cast<rti::Octet>(0x34U),
                  static_cast<rti::Octet>(0x56U),
                  static_cast<rti::Octet>(0x78U)},
      "HLAunsignedInteger64BE did not observe caller-owned memory changes");
  externalUnsigned64Be.set(0x87654321fedcba98ULL);
  require(
      externalUnsigned64BeValue == 0x87654321fedcba98ULL,
      "HLAunsignedInteger64BE set did not update caller-owned memory");
  rti::HLAunsignedInteger64BE decodedExternalUnsigned64BeValue{
      0xfedcba9876543210ULL};
  externalUnsigned64Be.decode(decodedExternalUnsigned64BeValue.encode());
  require(
      externalUnsigned64BeValue == 0xfedcba9876543210ULL &&
          externalUnsigned64Be.get() == 0xfedcba9876543210ULL,
      "HLAunsignedInteger64BE decode did not update caller-owned memory");

  rti::UnsignedInteger64 externalUnsigned64LeValue =
      0x0123456789abcdefULL;
  rti::HLAunsignedInteger64LE externalUnsigned64Le{
      &externalUnsigned64LeValue};
  externalUnsigned64LeValue = 0x9abcdef012345678ULL;
  std::vector<rti::Octet> externalUnsigned64LeEncoding{
      static_cast<rti::Octet>(0x7eU)};
  externalUnsigned64Le.encodeInto(externalUnsigned64LeEncoding);
  require(
      externalUnsigned64Le.get() == 0x9abcdef012345678ULL &&
          externalUnsigned64LeEncoding ==
              std::vector<rti::Octet>{
                  static_cast<rti::Octet>(0x7eU),
                  static_cast<rti::Octet>(0x78U),
                  static_cast<rti::Octet>(0x56U),
                  static_cast<rti::Octet>(0x34U),
                  static_cast<rti::Octet>(0x12U),
                  static_cast<rti::Octet>(0xf0U),
                  static_cast<rti::Octet>(0xdeU),
                  static_cast<rti::Octet>(0xbcU),
                  static_cast<rti::Octet>(0x9aU)},
      "HLAunsignedInteger64LE did not observe caller-owned memory changes");
  externalUnsigned64Le.set(0x87654321fedcba98ULL);
  require(
      externalUnsigned64LeValue == 0x87654321fedcba98ULL,
      "HLAunsignedInteger64LE set did not update caller-owned memory");
  rti::HLAunsignedInteger64LE decodedExternalUnsigned64LeValue{
      0xfedcba9876543210ULL};
  externalUnsigned64Le.decode(decodedExternalUnsigned64LeValue.encode());
  require(
      externalUnsigned64LeValue == 0xfedcba9876543210ULL &&
          externalUnsigned64Le.get() == 0xfedcba9876543210ULL,
      "HLAunsignedInteger64LE decode did not update caller-owned memory");
}

}  // namespace hla_rti_cpp_tck::detail
