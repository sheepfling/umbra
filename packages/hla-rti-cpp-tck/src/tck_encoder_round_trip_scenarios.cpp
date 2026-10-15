#include "tck_data_encoding.hpp"

namespace hla_rti_cpp_tck::detail {

void scenarioEncoderRoundTrip(Options const&, rti::CallbackModel) {
  rti::HLAinteger16BE signed16Be{-2};
  rti::HLAinteger16LE signed16Le{-2};
  rti::HLAinteger32BE signed32Be{0x1234abcd};
  rti::HLAinteger32LE signed32Le{0x1234abcd};
  rti::HLAinteger64BE signed64Be{-2};
  rti::HLAinteger64LE signed64Le{-2};
  requireEncodedBytes(signed16Be, {0xffU, 0xfeU}, "HLAinteger16BE");
  requireEncodedBytes(signed16Le, {0xfeU, 0xffU}, "HLAinteger16LE");
  requireEncodedBytes(signed32Be, {0x12U, 0x34U, 0xabU, 0xcdU}, "HLAinteger32BE");
  requireEncodedBytes(signed32Le, {0xcdU, 0xabU, 0x34U, 0x12U}, "HLAinteger32LE");
  requireEncodedBytes(
      signed64Be,
      {0xffU, 0xffU, 0xffU, 0xffU, 0xffU, 0xffU, 0xffU, 0xfeU},
      "HLAinteger64BE");
  requireEncodedBytes(
      signed64Le,
      {0xfeU, 0xffU, 0xffU, 0xffU, 0xffU, 0xffU, 0xffU, 0xffU},
      "HLAinteger64LE");

  rti::HLAinteger32BE decoded32;
  decoded32.decode(signed32Be.encode());
  require(decoded32.get() == 0x1234abcd, "HLAinteger32BE did not round-trip");

  rti::HLAunsignedInteger32BE unsigned32{0x1234abcdU};
  rti::HLAunsignedInteger32LE unsigned32Le{0x1234abcdU};
  requireEncodedBytes(unsigned32, {0x12U, 0x34U, 0xabU, 0xcdU}, "HLAunsignedInteger32BE");
  requireEncodedBytes(unsigned32Le, {0xcdU, 0xabU, 0x34U, 0x12U}, "HLAunsignedInteger32LE");

  rti::HLAunsignedInteger16BE unsigned16Be{0x1234U};
  rti::HLAunsignedInteger16LE unsigned16Le{0x1234U};
  rti::HLAunsignedInteger64BE unsigned64Be{0x0123456789abcdefULL};
  rti::HLAunsignedInteger64LE unsigned64Le{0x0123456789abcdefULL};
  requireEncodedBytes(unsigned16Be, {0x12U, 0x34U}, "HLAunsignedInteger16BE");
  requireEncodedBytes(unsigned16Le, {0x34U, 0x12U}, "HLAunsignedInteger16LE");
  requireEncodedBytes(
      unsigned64Be,
      {0x01U, 0x23U, 0x45U, 0x67U, 0x89U, 0xabU, 0xcdU, 0xefU},
      "HLAunsignedInteger64BE");
  requireEncodedBytes(
      unsigned64Le,
      {0xefU, 0xcdU, 0xabU, 0x89U, 0x67U, 0x45U, 0x23U, 0x01U},
      "HLAunsignedInteger64LE");

  rti::HLAunsignedInteger16BE decoded16;
  decoded16.decode(unsigned16Be.encode());
  require(decoded16.get() == 0x1234U, "HLAunsignedInteger16BE did not round-trip");
  rti::HLAunsignedInteger64LE decoded64;
  decoded64.decode(unsigned64Le.encode());
  require(
      decoded64.get() == 0x0123456789abcdefULL,
      "HLAunsignedInteger64LE did not round-trip");

  rti::HLAboolean trueValue{true};
  rti::HLAboolean falseValue{false};
  requireEncodedBytes(trueValue, {0U, 0U, 0U, 1U}, "HLAboolean true");
  requireEncodedBytes(falseValue, {0U, 0U, 0U, 0U}, "HLAboolean false");
  rti::HLAboolean decodedBoolean;
  decodedBoolean.decode(trueValue.encode());
  require(decodedBoolean.get(), "HLAboolean did not round-trip");
  std::vector<std::uint8_t> invalidBooleanBytes{0U, 0U, 0U, 2U};
  rti::VariableLengthData invalidBooleanData(
      invalidBooleanBytes.data(),
      invalidBooleanBytes.size());
  requireException(
      [&] { decodedBoolean.decode(invalidBooleanData); },
      L"EncoderException",
      "invalid HLAboolean encoding");

  rti::HLAbyte byteValue{static_cast<rti::Octet>(0xa5U)};
  rti::HLAoctet octetValue{static_cast<rti::Octet>(0xa5U)};
  requireEncodedBytes(byteValue, {0xa5U}, "HLAbyte");
  requireEncodedBytes(octetValue, {0xa5U}, "HLAoctet");

  rti::HLAfloat32BE float32Be{-2.5F};
  rti::HLAfloat32LE float32Le{-2.5F};
  rti::HLAfloat64BE float64Be{-2.5};
  rti::HLAfloat64LE float64Le{-2.5};
  requireEncodedBytes(float32Be, {0xc0U, 0x20U, 0U, 0U}, "HLAfloat32BE");
  requireEncodedBytes(float32Le, {0U, 0U, 0x20U, 0xc0U}, "HLAfloat32LE");
  requireEncodedBytes(
      float64Be,
      {0xc0U, 0x04U, 0U, 0U, 0U, 0U, 0U, 0U},
      "HLAfloat64BE");
  requireEncodedBytes(
      float64Le,
      {0U, 0U, 0U, 0U, 0U, 0U, 0x04U, 0xc0U},
      "HLAfloat64LE");

  rti::HLAASCIIchar asciiCharacter{'A'};
  rti::HLAASCIIstring asciiString{std::string{"A\0Z", 3U}};
  rti::HLAunicodeChar unicodeCharacter{static_cast<wchar_t>(0x03a9U)};
  rti::HLAunicodeString unicodeString{L"portable"};
  requireEncodedBytes(asciiCharacter, {0x41U}, "HLAASCIIchar");
  requireEncodedBytes(asciiString, {0U, 0U, 0U, 3U, 0x41U, 0U, 0x5aU}, "HLAASCIIstring");
  requireEncodedBytes(unicodeCharacter, {0x03U, 0xa9U}, "HLAunicodeChar");
  auto const encodedUnicode = encodeDataElement(unicodeString);
  rti::HLAunicodeString decodedUnicode;
  decodedUnicode.decode(unicodeString.encode());
  require(decodedUnicode.get() == L"portable", "HLAunicodeString did not round-trip");
  require(!encodedUnicode.empty(), "HLAunicodeString produced no encoding");

  rti::HLAunicodeString unicodeSupplementary{L"A\U0001f600"};
  requireEncodedBytes(
      unicodeSupplementary,
      {0U, 0U, 0U, 3U, 0U, 0x41U, 0xd8U, 0x3dU, 0xdeU, 0U},
      "HLAunicodeString UTF-16BE element count");
  requireEncodedInto(
      unicodeSupplementary,
      {0U, 0U, 0U, 3U, 0U, 0x41U, 0xd8U, 0x3dU, 0xdeU, 0U},
      "HLAunicodeString UTF-16BE element count");
  rti::HLAunicodeString decodedSupplementary;
  decodedSupplementary.decode(unicodeSupplementary.encode());
  require(
      decodedSupplementary.get() == L"A\U0001f600",
      "HLAunicodeString supplementary code point did not round-trip");
  std::vector<std::uint8_t> malformedUnicodeBytes{
      0U, 0U, 0U, 2U, 0xd8U, 0U};
  rti::VariableLengthData malformedUnicodeData(
      malformedUnicodeBytes.data(), malformedUnicodeBytes.size());
  requireException(
      [&] { decodedSupplementary.decode(malformedUnicodeData); },
      L"EncoderException",
      "HLAunicodeString unmatched high surrogate");
  std::vector<std::uint8_t> wrongUnicodeCountBytes{
      0U, 0U, 0U, 6U,
      0U, 0x41U,
      0xd8U, 0x3dU,
      0xdeU, 0U};
  rti::VariableLengthData wrongUnicodeCountData(
      wrongUnicodeCountBytes.data(), wrongUnicodeCountBytes.size());
  requireException(
      [&] { decodedSupplementary.decode(wrongUnicodeCountData); },
      L"EncoderException",
      "HLAunicodeString byte count mistaken for element count");
  rti::HLAunicodeString unmatchedHighSurrogate{
      std::wstring(1U, static_cast<wchar_t>(0xd800U))};
  requireException(
      [&] { unmatchedHighSurrogate.encode(); },
      L"EncoderException",
      "HLAunicodeString unmatched high surrogate encoding");

  rti::HLAASCIIchar decodedAscii;
  std::vector<std::uint8_t> invalidAsciiBytes{0x80U};
  rti::VariableLengthData invalidAsciiData(invalidAsciiBytes.data(), invalidAsciiBytes.size());
  requireException(
      [&] { decodedAscii.decode(invalidAsciiData); },
      L"EncoderException",
      "non-ASCII HLAASCIIchar encoding");
  std::vector<std::uint8_t> truncatedUnicodeBytes{0U, 0U, 0U, 1U, 0U};
  rti::VariableLengthData truncatedUnicodeData(
      truncatedUnicodeBytes.data(),
      truncatedUnicodeBytes.size());
  requireException(
      [&] { decodedUnicode.decode(truncatedUnicodeData); },
      L"EncoderException",
      "truncated HLAunicodeString encoding");

  std::vector<rti::Octet> opaqueBytes{
      static_cast<rti::Octet>(0xa5U),
      static_cast<rti::Octet>(0U),
      static_cast<rti::Octet>(0x5aU)};
  rti::HLAopaqueData opaque(opaqueBytes.data(), opaqueBytes.size());
  rti::HLAopaqueData decodedOpaque;
  decodedOpaque.decode(opaque.encode());
  require(decodedOpaque.dataLength() == opaqueBytes.size(),
          "HLAopaqueData length did not round-trip");
  require(
      std::equal(
          opaqueBytes.begin(), opaqueBytes.end(),
          decodedOpaque.get()),
      "HLAopaqueData payload did not round-trip");
  std::vector<std::uint8_t> truncatedOpaqueBytes{0U, 0U, 0U, 2U, 0xa5U};
  rti::VariableLengthData truncatedOpaqueData(
      truncatedOpaqueBytes.data(),
      truncatedOpaqueBytes.size());
  requireException(
      [&] { decodedOpaque.decode(truncatedOpaqueData); },
      L"EncoderException",
      "truncated HLAopaqueData encoding");

  auto const pair = rti::OctetPair{
      static_cast<rti::Octet>(0x12U),
      static_cast<rti::Octet>(0x34U)};
  rti::HLAoctetPairBE pairBe{pair};
  rti::HLAoctetPairLE pairLe{pair};
  requireEncodedBytes(pairBe, {0x12U, 0x34U}, "HLAoctetPairBE");
  requireEncodedBytes(pairLe, {0x34U, 0x12U}, "HLAoctetPairLE");

  rti::HLAfixedArrayT<rti::HLAbyte> fixedArray{3U};
  fixedArray.set(0U, rti::HLAbyte{static_cast<rti::Octet>(0x11U)});
  fixedArray.set(1U, rti::HLAbyte{static_cast<rti::Octet>(0x22U)});
  fixedArray.set(2U, rti::HLAbyte{static_cast<rti::Octet>(0x33U)});
  requireEncodedBytes(fixedArray, {0x11U, 0x22U, 0x33U}, "HLAfixedArray");
  rti::HLAfixedArrayT<rti::HLAbyte> decodedFixedArray{3U};
  decodedFixedArray.decode(fixedArray.encode());
  require(decodedFixedArray[1U].get() == static_cast<rti::Octet>(0x22U),
          "HLAfixedArray did not round-trip");

  rti::HLAvariableArrayT<rti::HLAbyte> variableArray;
  variableArray.addElement(rti::HLAbyte{static_cast<rti::Octet>(0x44U)});
  variableArray.addElement(rti::HLAbyte{static_cast<rti::Octet>(0x55U)});
  rti::HLAvariableArrayT<rti::HLAbyte> decodedVariableArray;
  decodedVariableArray.decode(variableArray.encode());
  require(decodedVariableArray.size() == 2U &&
              decodedVariableArray[0U].get() == static_cast<rti::Octet>(0x44U) &&
              decodedVariableArray[1U].get() == static_cast<rti::Octet>(0x55U),
          "HLAvariableArray did not round-trip");

  rti::HLAfixedRecord record;
  record.appendElement(rti::HLAinteger32BE{7});
  record.appendElement(rti::HLAboolean{true});
  rti::HLAfixedRecord decodedRecord;
  decodedRecord.appendElement(rti::HLAinteger32BE{});
  decodedRecord.appendElement(rti::HLAboolean{});
  decodedRecord.decode(record.encode());
  require(
      dynamic_cast<rti::HLAinteger32BE const&>(decodedRecord[0U]).get() == 7 &&
          dynamic_cast<rti::HLAboolean const&>(decodedRecord[1U]).get(),
      "HLAfixedRecord did not round-trip");

  rti::HLAvariantRecordT<rti::HLAinteger32BE> variant;
  rti::HLAinteger32BE ready{0};
  rti::HLAinteger32BE busy{1};
  variant.addVariant(ready, rti::HLAinteger32BE{});
  variant.addVariant(busy, rti::HLAASCIIstring{});
  variant.setVariant(ready, rti::HLAinteger32BE{41});
  variant.setDiscriminant(ready);
  rti::HLAvariantRecordT<rti::HLAinteger32BE> decodedVariant;
  decodedVariant.addVariant(ready, rti::HLAinteger32BE{});
  decodedVariant.addVariant(busy, rti::HLAASCIIstring{});
  decodedVariant.decode(variant.encode());
  require(
      dynamic_cast<rti::HLAinteger32BE const&>(decodedVariant.getVariant()).get() == 41,
      "HLAvariantRecord did not round-trip");

  rti::HLAfloat64BE selectedExtendableDiscriminant{1.5};
  rti::HLAextendableVariantRecord extendable{
      rti::HLAfloat64BE{}};
  extendable.addVariant(selectedExtendableDiscriminant, rti::HLAinteger32BE{});
  extendable.setVariant(
      selectedExtendableDiscriminant,
      rti::HLAinteger32BE{0x10203040});
  std::vector<std::uint8_t> extendableExpected{
      0x3fU, 0xf8U, 0U, 0U, 0U, 0U, 0U, 0U,
      0U, 0U, 0U, 4U,
      0U, 0U, 0U, 0U,
      0x10U, 0x20U, 0x30U, 0x40U};
  requireEncodedBytes(
      extendable,
      extendableExpected,
      "HLAextendableVariantRecord aligned encoding");
  require(
      extendable.getEncodedLength() == extendableExpected.size(),
      "HLAextendableVariantRecord encoded length did not include alignment");
  require(
      extendable.getOctetBoundary() == 8U,
      "HLAextendableVariantRecord did not expose its eight-octet boundary");

  std::vector<rti::Octet> extendablePrefixed{
      static_cast<rti::Octet>(0x7eU)};
  extendable.encodeInto(extendablePrefixed);
  require(
      extendablePrefixed.size() == extendableExpected.size() + 1U &&
          std::equal(
              extendableExpected.begin(),
              extendableExpected.end(),
              extendablePrefixed.begin() + 1,
              [](std::uint8_t expected, rti::Octet actual) {
                return expected == static_cast<std::uint8_t>(actual);
              }),
      "HLAextendableVariantRecord encodeInto did not append its complete value");

  rti::HLAextendableVariantRecord decodedExtendable{
      rti::HLAfloat64BE{}};
  decodedExtendable.addVariant(
      selectedExtendableDiscriminant,
      rti::HLAinteger32BE{});
  require(
      decodedExtendable.decodeFrom(extendablePrefixed, 1U) ==
          extendablePrefixed.size(),
      "HLAextendableVariantRecord decodeFrom did not consume the value");
  require(
      dynamic_cast<rti::HLAfloat64BE const&>(
          decodedExtendable.getDiscriminant()).get() == 1.5,
      "HLAextendableVariantRecord discriminant did not round-trip");
  require(
      dynamic_cast<rti::HLAinteger32BE const&>(
          decodedExtendable.getVariant()).get() == 0x10203040,
      "HLAextendableVariantRecord alternative did not round-trip");

  auto requireExtendableDecodeException =
      [&](std::vector<rti::Octet> const& bytes, std::string const& description) {
        rti::VariableLengthData data(bytes.data(), bytes.size());
        requireException(
            [&] { decodedExtendable.decode(data); },
            L"EncoderException",
            description);
      };
  auto nonzeroAlternativePadding = std::vector<rti::Octet>(
      extendableExpected.begin(), extendableExpected.end());
  nonzeroAlternativePadding[12U] = static_cast<rti::Octet>(1U);
  requireExtendableDecodeException(
      nonzeroAlternativePadding,
      "nonzero HLAextendableVariantRecord alternative padding");
  auto declaredLengthTooShort = std::vector<rti::Octet>(
      extendableExpected.begin(), extendableExpected.end());
  declaredLengthTooShort[11U] = static_cast<rti::Octet>(3U);
  requireExtendableDecodeException(
      declaredLengthTooShort,
      "short HLAextendableVariantRecord length field");
  auto truncatedAlternative = std::vector<rti::Octet>(
      extendableExpected.begin(), extendableExpected.end());
  truncatedAlternative.pop_back();
  requireExtendableDecodeException(
      truncatedAlternative,
      "truncated HLAextendableVariantRecord alternative");
  auto trailingExtendableData = std::vector<rti::Octet>(
      extendableExpected.begin(), extendableExpected.end());
  trailingExtendableData.push_back(static_cast<rti::Octet>(0U));
  requireExtendableDecodeException(
      trailingExtendableData,
      "trailing HLAextendableVariantRecord data");

  extendable.setDiscriminant(rti::HLAfloat64BE{-2.5});
  requireEncodedBytes(
      extendable,
      {0xc0U, 0x04U, 0U, 0U, 0U, 0U, 0U, 0U,
       0U, 0U, 0U, 0U,
       0U, 0U, 0U, 0U},
      "HLAextendableVariantRecord unknown-discriminant encoding");
  require(
      extendable.getEncodedLength() == 16U,
      "HLAextendableVariantRecord unknown-discriminant length was not minimal");
  requireException(
      [&] { (void)extendable.getVariant(); },
      L"EncoderException",
      "HLAextendableVariantRecord unknown-discriminant lookup");

  rti::HLAextendableVariantRecord octetExtendable{rti::HLAoctet{}};
  octetExtendable.addVariant(
      rti::HLAoctet{static_cast<rti::Octet>(1U)},
      rti::HLAfloat32LE{});
  octetExtendable.setVariant(
      rti::HLAoctet{static_cast<rti::Octet>(1U)},
      rti::HLAfloat32LE{1.0F});
  std::vector<std::uint8_t> octetExtendableExpected{
      1U, 0U, 0U, 0U,
      0U, 0U, 0U, 4U,
      0U, 0U, 0x80U, 0x3fU};
  requireEncodedBytes(
      octetExtendable,
      octetExtendableExpected,
      "HLAextendableVariantRecord octet-discriminant encoding");
  auto nonzeroLengthPadding = std::vector<rti::Octet>(
      octetExtendableExpected.begin(), octetExtendableExpected.end());
  nonzeroLengthPadding[1U] = static_cast<rti::Octet>(1U);
  rti::VariableLengthData nonzeroLengthPaddingData(
      nonzeroLengthPadding.data(),
      nonzeroLengthPadding.size());
  requireException(
      [&] { octetExtendable.decode(nonzeroLengthPaddingData); },
      L"EncoderException",
      "nonzero HLAextendableVariantRecord length padding");

  rti::HLAextendableVariantRecord unknownOctetExtendable{rti::HLAoctet{}};
  unknownOctetExtendable.addVariant(
      rti::HLAoctet{static_cast<rti::Octet>(1U)},
      rti::HLAfloat32LE{});
  std::vector<rti::Octet> unknownOctetAlternative{
      static_cast<rti::Octet>(0x7fU),
      static_cast<rti::Octet>(0U),
      static_cast<rti::Octet>(0U),
      static_cast<rti::Octet>(0U),
      static_cast<rti::Octet>(0U),
      static_cast<rti::Octet>(0U),
      static_cast<rti::Octet>(0U),
      static_cast<rti::Octet>(3U),
      static_cast<rti::Octet>(0xaaU),
      static_cast<rti::Octet>(0xbbU),
      static_cast<rti::Octet>(0xccU)};
  rti::VariableLengthData unknownOctetAlternativeData(
      unknownOctetAlternative.data(),
      unknownOctetAlternative.size());
  unknownOctetExtendable.decode(unknownOctetAlternativeData);
  require(
      dynamic_cast<rti::HLAoctet const&>(
          unknownOctetExtendable.getDiscriminant()).get() ==
          static_cast<rti::Octet>(0x7fU),
      "HLAextendableVariantRecord unknown octet discriminant did not decode");
  requireException(
      [&] { (void)unknownOctetExtendable.getVariant(); },
      L"EncoderException",
      "HLAextendableVariantRecord unknown octet alternative lookup");

  rti::HLAextendableVariantRecord extendableLifetime{rti::HLAinteger32BE{}};
  rti::HLAinteger32BE copiedSource{1};
  extendableLifetime.addVariant(rti::HLAinteger32BE{1}, copiedSource);
  copiedSource.set(2);
  requireEncodedBytes(
      extendableLifetime,
      {0U, 0U, 0U, 1U, 0U, 0U, 0U, 4U, 0U, 0U, 0U, 1U},
      "HLAextendableVariantRecord copied variant lifetime");

  rti::HLAinteger32BE borrowedVariant{3};
  extendableLifetime.addVariantPointer(
      rti::HLAinteger32BE{2},
      &borrowedVariant);
  borrowedVariant.set(4);
  requireEncodedBytes(
      extendableLifetime,
      {0U, 0U, 0U, 2U, 0U, 0U, 0U, 4U, 0U, 0U, 0U, 4U},
      "HLAextendableVariantRecord borrowed variant lifetime");
  rti::HLAinteger32BE replacementVariant{5};
  extendableLifetime.setVariant(
      rti::HLAinteger32BE{2},
      replacementVariant);
  require(
      borrowedVariant.get() == 5,
      "HLAextendableVariantRecord setVariant did not update borrowed storage");
  rti::HLAinteger32BE substituteVariant{6};
  extendableLifetime.setVariantPointer(
      rti::HLAinteger32BE{2},
      &substituteVariant);
  substituteVariant.set(7);
  rti::HLAextendableVariantRecord copiedExtendable{extendableLifetime};
  substituteVariant.set(8);
  requireEncodedBytes(
      copiedExtendable,
      {0U, 0U, 0U, 2U, 0U, 0U, 0U, 4U, 0U, 0U, 0U, 7U},
      "HLAextendableVariantRecord copied borrowed variant");

  rti::HLAextendableVariantRecord sameExtendableShape{
      rti::HLAinteger32BE{}};
  sameExtendableShape.addVariant(rti::HLAinteger32BE{1}, rti::HLAinteger32BE{});
  sameExtendableShape.addVariant(rti::HLAinteger32BE{2}, rti::HLAinteger32BE{});
  rti::HLAextendableVariantRecord differentExtendableVariant{
      rti::HLAinteger32BE{}};
  differentExtendableVariant.addVariant(
      rti::HLAinteger32BE{1},
      rti::HLAinteger32BE{});
  differentExtendableVariant.addVariant(
      rti::HLAinteger32BE{2},
      rti::HLAoctet{});
  rti::HLAextendableVariantRecord differentExtendableDiscriminant{
      rti::HLAoctet{}};
  differentExtendableDiscriminant.addVariant(
      rti::HLAoctet{static_cast<rti::Octet>(1U)},
      rti::HLAinteger32BE{});
  require(
      extendableLifetime.isSameTypeAs(sameExtendableShape),
      "HLAextendableVariantRecord same-shape comparison failed");
  require(
      !extendableLifetime.isSameTypeAs(differentExtendableVariant),
      "HLAextendableVariantRecord accepted a different alternative shape");
  require(
      !extendableLifetime.isSameTypeAs(differentExtendableDiscriminant),
      "HLAextendableVariantRecord accepted a different discriminant shape");
  require(
      extendableLifetime.isSameTypeAs(
          rti::HLAinteger32BE{1},
          rti::HLAinteger32BE{}),
      "HLAextendableVariantRecord typed alternative comparison failed");
  require(
      !extendableLifetime.isSameTypeAs(
          rti::HLAinteger32BE{1},
          rti::HLAoctet{}),
      "HLAextendableVariantRecord accepted a wrong alternative type");
  require(
      extendableLifetime.hasMatchingDiscriminantTypeAs(rti::HLAinteger32BE{}),
      "HLAextendableVariantRecord discriminant prototype comparison failed");
  require(
      !extendableLifetime.hasMatchingDiscriminantTypeAs(rti::HLAoctet{}),
      "HLAextendableVariantRecord accepted a wrong discriminant prototype");

  rti::HLAextendableVariantRecordT<rti::HLAoctet> typedExtendable;
  typedExtendable.addVariant(
      rti::HLAoctet{static_cast<rti::Octet>(1U)},
      rti::HLAinteger16BE{});
  typedExtendable.setVariant(
      rti::HLAoctet{static_cast<rti::Octet>(1U)},
      rti::HLAinteger16BE{9});
  requireEncodedBytes(
      typedExtendable,
      {1U, 0U, 0U, 0U, 0U, 0U, 0U, 2U, 0U, 9U},
      "HLAextendableVariantRecordT encoding");
  rti::HLAextendableVariantRecordT<rti::HLAoctet> sameTypedExtendable;
  rti::HLAextendableVariantRecordT<rti::HLAinteger32BE>
      differentTypedExtendable;
  require(
      typedExtendable.isSameTypeAs(sameTypedExtendable),
      "HLAextendableVariantRecordT same-type comparison failed");
  require(
      !typedExtendable.isSameTypeAs(differentTypedExtendable),
      "HLAextendableVariantRecordT accepted a different discriminant type");

  rti::HLAoctet wrongExtendableDiscriminant{
      static_cast<rti::Octet>(1U)};
  rti::HLAoctet wrongExtendableVariant{
      static_cast<rti::Octet>(2U)};
  requireException(
      [&] {
        extendableLifetime.addVariant(
            rti::HLAinteger32BE{2},
            rti::HLAinteger32BE{});
      },
      L"EncoderException",
      "HLAextendableVariantRecord duplicate discriminant");
  requireException(
      [&] {
        extendableLifetime.addVariant(
            wrongExtendableDiscriminant,
            rti::HLAinteger32BE{});
      },
      L"EncoderException",
      "HLAextendableVariantRecord wrong discriminant type");
  requireException(
      [&] {
        extendableLifetime.addVariantPointer(
            rti::HLAinteger32BE{3},
            nullptr);
      },
      L"EncoderException",
      "HLAextendableVariantRecord null added variant pointer");
  requireException(
      [&] { extendableLifetime.setDiscriminant(wrongExtendableDiscriminant); },
      L"EncoderException",
      "HLAextendableVariantRecord wrong selected discriminant type");
  requireException(
      [&] {
        extendableLifetime.setVariant(
            rti::HLAinteger32BE{3},
            rti::HLAinteger32BE{});
      },
      L"EncoderException",
      "HLAextendableVariantRecord unknown selected alternative");
  requireException(
      [&] {
        extendableLifetime.setVariant(
            rti::HLAinteger32BE{2},
            wrongExtendableVariant);
      },
      L"EncoderException",
      "HLAextendableVariantRecord wrong alternative type");
  requireException(
      [&] {
        extendableLifetime.setVariantPointer(
            rti::HLAinteger32BE{2},
            &wrongExtendableVariant);
      },
      L"EncoderException",
      "HLAextendableVariantRecord wrong pointer alternative type");
  requireException(
      [&] {
        extendableLifetime.setVariantPointer(
            rti::HLAinteger32BE{2},
            nullptr);
      },
      L"EncoderException",
      "HLAextendableVariantRecord null replacement pointer");

  rti::HLAoctet fixedRecordFirst{
      static_cast<rti::Octet>(0xa5U)};
  rti::HLAboolean fixedRecordSecond{true};
  rti::HLAfloat64BE fixedRecordThird{-2.5};
  rti::HLAfixedRecord alignedFixedRecord;
  alignedFixedRecord
      .appendElement(fixedRecordFirst)
      .appendElement(fixedRecordSecond)
      .appendElement(fixedRecordThird);
  std::vector<std::uint8_t> alignedFixedRecordExpected{
      0xa5U,
      0U, 0U, 0U,
      0U, 0U, 0U, 1U,
      0xc0U, 0x04U, 0U, 0U, 0U, 0U, 0U, 0U};
  requireEncodedBytes(
      alignedFixedRecord,
      alignedFixedRecordExpected,
      "HLAfixedRecord declaration-order encoding");
  require(
      alignedFixedRecord.getEncodedLength() == alignedFixedRecordExpected.size(),
      "HLAfixedRecord encoded length did not include field padding");
  require(
      alignedFixedRecord.getOctetBoundary() == 8U,
      "HLAfixedRecord did not expose the maximum field boundary");
  require(
      alignedFixedRecord.size() == 3U,
      "HLAfixedRecord did not retain all declared elements");
  requireEncodedInto(
      alignedFixedRecord,
      alignedFixedRecordExpected,
      "HLAfixedRecord encodeInto");

  std::vector<rti::Octet> alignedFixedRecordNested{
      static_cast<rti::Octet>(0x7eU)};
  alignedFixedRecord.encodeInto(alignedFixedRecordNested);
  rti::HLAfixedRecord decodedAlignedFixedRecord;
  decodedAlignedFixedRecord
      .appendElement(rti::HLAoctet{})
      .appendElement(rti::HLAboolean{})
      .appendElement(rti::HLAfloat64BE{});
  require(
      decodedAlignedFixedRecord.decodeFrom(
          alignedFixedRecordNested,
          1U) == alignedFixedRecordNested.size(),
      "HLAfixedRecord decodeFrom did not consume its value");
  require(
      dynamic_cast<rti::HLAoctet const&>(
          decodedAlignedFixedRecord.get(0U)).get() ==
          static_cast<rti::Octet>(0xa5U),
      "HLAfixedRecord octet field did not round-trip");
  require(
      dynamic_cast<rti::HLAboolean const&>(
          decodedAlignedFixedRecord.get(1U)).get(),
      "HLAfixedRecord boolean field did not round-trip");
  require(
      dynamic_cast<rti::HLAfloat64BE const&>(
          decodedAlignedFixedRecord.get(2U)).get() == -2.5,
      "HLAfixedRecord floating-point field did not round-trip");

  auto fixedRecordNonzeroPadding = std::vector<rti::Octet>(
      alignedFixedRecordExpected.begin(),
      alignedFixedRecordExpected.end());
  fixedRecordNonzeroPadding[1U] = static_cast<rti::Octet>(1U);
  rti::VariableLengthData fixedRecordNonzeroPaddingData(
      fixedRecordNonzeroPadding.data(),
      fixedRecordNonzeroPadding.size());
  requireException(
      [&] { decodedAlignedFixedRecord.decode(fixedRecordNonzeroPaddingData); },
      L"EncoderException",
      "HLAfixedRecord nonzero padding");
  std::vector<rti::Octet> fixedRecordTruncatedPadding{
      static_cast<rti::Octet>(0xa5U),
      static_cast<rti::Octet>(0U),
      static_cast<rti::Octet>(0U)};
  rti::VariableLengthData fixedRecordTruncatedPaddingData(
      fixedRecordTruncatedPadding.data(),
      fixedRecordTruncatedPadding.size());
  requireException(
      [&] { decodedAlignedFixedRecord.decode(fixedRecordTruncatedPaddingData); },
      L"EncoderException",
      "HLAfixedRecord truncated padding");
  auto fixedRecordTrailingData = std::vector<rti::Octet>(
      alignedFixedRecordExpected.begin(),
      alignedFixedRecordExpected.end());
  fixedRecordTrailingData.push_back(static_cast<rti::Octet>(0U));
  rti::VariableLengthData fixedRecordTrailingDataValue(
      fixedRecordTrailingData.data(),
      fixedRecordTrailingData.size());
  requireException(
      [&] { decodedAlignedFixedRecord.decode(fixedRecordTrailingDataValue); },
      L"EncoderException",
      "HLAfixedRecord trailing data");

  rti::HLAfixedRecord reverseFixedRecord;
  reverseFixedRecord
      .appendElement(fixedRecordThird)
      .appendElement(fixedRecordSecond)
      .appendElement(fixedRecordFirst);
  requireEncodedBytes(
      reverseFixedRecord,
      {0xc0U, 0x04U, 0U, 0U, 0U, 0U, 0U, 0U,
       0U, 0U, 0U, 1U, 0xa5U},
      "HLAfixedRecord reverse declaration order");
  require(
      reverseFixedRecord.getEncodedLength() == 13U,
      "HLAfixedRecord reverse declaration length was incorrect");

  rti::HLAoctet fixedRecordSource{
      static_cast<rti::Octet>(0x11U)};
  rti::HLAfixedRecord copiedFixedRecordElement;
  copiedFixedRecordElement.appendElement(fixedRecordSource);
  fixedRecordSource.set(static_cast<rti::Octet>(0x22U));
  requireEncodedBytes(
      copiedFixedRecordElement,
      {0x11U},
      "HLAfixedRecord copied element lifetime");

  rti::HLAinteger32BE borrowedFixedRecordElement{1};
  rti::HLAfixedRecord borrowedFixedRecord;
  borrowedFixedRecord.appendElementPointer(&borrowedFixedRecordElement);
  borrowedFixedRecordElement.set(2);
  requireEncodedBytes(
      borrowedFixedRecord,
      {0U, 0U, 0U, 2U},
      "HLAfixedRecord borrowed element lifetime");
  rti::HLAinteger32BE fixedRecordReplacement{3};
  borrowedFixedRecord.set(0U, fixedRecordReplacement);
  require(
      borrowedFixedRecordElement.get() == 3,
      "HLAfixedRecord set did not update borrowed element");
  rti::HLAinteger32BE fixedRecordSubstitute{4};
  borrowedFixedRecord.setElementPointer(0U, &fixedRecordSubstitute);
  fixedRecordSubstitute.set(5);
  requireEncodedBytes(
      borrowedFixedRecord,
      {0U, 0U, 0U, 5U},
      "HLAfixedRecord replacement pointer lifetime");
  rti::HLAfixedRecord copiedFixedRecord{borrowedFixedRecord};
  fixedRecordSubstitute.set(6);
  requireEncodedBytes(
      copiedFixedRecord,
      {0U, 0U, 0U, 5U},
      "HLAfixedRecord copied borrowed element");

  rti::HLAfixedRecord sameFixedRecordShape;
  sameFixedRecordShape.appendElement(rti::HLAinteger32BE{});
  rti::HLAfixedRecord differentFixedRecordShape;
  differentFixedRecordShape.appendElement(rti::HLAoctet{});
  require(
      borrowedFixedRecord.isSameTypeAs(sameFixedRecordShape),
      "HLAfixedRecord same-shape comparison failed");
  require(
      !borrowedFixedRecord.isSameTypeAs(differentFixedRecordShape),
      "HLAfixedRecord accepted a different element shape");
  require(
      borrowedFixedRecord.hasElementSameTypeAs(0U, fixedRecordReplacement),
      "HLAfixedRecord element type comparison failed");
  require(
      !borrowedFixedRecord.hasElementSameTypeAs(1U, fixedRecordReplacement),
      "HLAfixedRecord accepted an out-of-range element type query");
  rti::HLAboolean wrongFixedRecordType{false};
  requireException(
      [&] { borrowedFixedRecord.set(0U, wrongFixedRecordType); },
      L"EncoderException",
      "HLAfixedRecord wrong replacement type");
  requireException(
      [&] {
        borrowedFixedRecord.setElementPointer(0U, &wrongFixedRecordType);
      },
      L"EncoderException",
      "HLAfixedRecord wrong replacement pointer type");
  requireException(
      [&] { borrowedFixedRecord.appendElementPointer(nullptr); },
      L"EncoderException",
      "HLAfixedRecord null element pointer");
  requireException(
      [&] { (void)borrowedFixedRecord.get(1U); },
      L"EncoderException",
      "HLAfixedRecord out-of-range element lookup");

  rti::HLAfixedRecord fixedArrayPrototype;
  fixedArrayPrototype
      .appendElement(rti::HLAinteger32BE{})
      .appendElement(rti::HLAoctet{});
  rti::HLAfixedArray alignedFixedArray{fixedArrayPrototype, 2U};
  rti::HLAfixedRecord alignedFixedArrayFirst;
  alignedFixedArrayFirst
      .appendElement(rti::HLAinteger32BE{0x10203040})
      .appendElement(rti::HLAoctet{static_cast<rti::Octet>(0xa5U)});
  rti::HLAfixedRecord alignedFixedArraySecond;
  alignedFixedArraySecond
      .appendElement(rti::HLAinteger32BE{-2})
      .appendElement(rti::HLAoctet{static_cast<rti::Octet>(0x5aU)});
  alignedFixedArray
      .set(0U, alignedFixedArrayFirst)
      .set(1U, alignedFixedArraySecond);
  std::vector<std::uint8_t> alignedFixedArrayExpected{
      0x10U, 0x20U, 0x30U, 0x40U, 0xa5U,
      0U, 0U, 0U,
      0xffU, 0xffU, 0xffU, 0xfeU, 0x5aU};
  requireEncodedBytes(
      alignedFixedArray,
      alignedFixedArrayExpected,
      "HLAfixedArray fixed-cardinality encoding");
  require(
      alignedFixedArray.getEncodedLength() == alignedFixedArrayExpected.size(),
      "HLAfixedArray encoded length was incorrect");
  require(
      alignedFixedArray.getOctetBoundary() == 4U,
      "HLAfixedArray did not expose its prototype boundary");
  require(
      alignedFixedArray.size() == 2U,
      "HLAfixedArray cardinality changed unexpectedly");
  requireEncodedInto(
      alignedFixedArray,
      alignedFixedArrayExpected,
      "HLAfixedArray encodeInto");
  std::vector<rti::Octet> alignedFixedArrayNested{
      static_cast<rti::Octet>(0x7eU)};
  alignedFixedArray.encodeInto(alignedFixedArrayNested);
  rti::HLAfixedArray decodedAlignedFixedArray{
      fixedArrayPrototype,
      2U};
  require(
      decodedAlignedFixedArray.decodeFrom(
          alignedFixedArrayNested,
          1U) == alignedFixedArrayNested.size(),
      "HLAfixedArray decodeFrom did not consume its value");
  auto const& decodedAlignedFixedArrayFirst =
      dynamic_cast<rti::HLAfixedRecord const&>(
          decodedAlignedFixedArray.get(0U));
  auto const& decodedAlignedFixedArraySecond =
      dynamic_cast<rti::HLAfixedRecord const&>(
          decodedAlignedFixedArray.get(1U));
  require(
      dynamic_cast<rti::HLAinteger32BE const&>(
          decodedAlignedFixedArrayFirst.get(0U)).get() == 0x10203040 &&
          dynamic_cast<rti::HLAoctet const&>(
              decodedAlignedFixedArrayFirst.get(1U)).get() ==
              static_cast<rti::Octet>(0xa5U),
      "HLAfixedArray first element did not round-trip");
  require(
      dynamic_cast<rti::HLAinteger32BE const&>(
          decodedAlignedFixedArraySecond.get(0U)).get() == -2 &&
          dynamic_cast<rti::HLAoctet const&>(
              decodedAlignedFixedArraySecond.get(1U)).get() ==
              static_cast<rti::Octet>(0x5aU),
      "HLAfixedArray second element did not round-trip");
  auto fixedArrayNonzeroPadding = std::vector<rti::Octet>(
      alignedFixedArrayExpected.begin(),
      alignedFixedArrayExpected.end());
  fixedArrayNonzeroPadding[5U] = static_cast<rti::Octet>(1U);
  rti::VariableLengthData fixedArrayNonzeroPaddingData(
      fixedArrayNonzeroPadding.data(),
      fixedArrayNonzeroPadding.size());
  requireException(
      [&] { decodedAlignedFixedArray.decode(fixedArrayNonzeroPaddingData); },
      L"EncoderException",
      "HLAfixedArray nonzero inter-element padding");
  std::vector<rti::Octet> fixedArrayTruncatedPadding{
      static_cast<rti::Octet>(0x10U),
      static_cast<rti::Octet>(0x20U),
      static_cast<rti::Octet>(0x30U),
      static_cast<rti::Octet>(0x40U),
      static_cast<rti::Octet>(0xa5U),
      static_cast<rti::Octet>(0U),
      static_cast<rti::Octet>(0U)};
  rti::VariableLengthData fixedArrayTruncatedPaddingData(
      fixedArrayTruncatedPadding.data(),
      fixedArrayTruncatedPadding.size());
  requireException(
      [&] { decodedAlignedFixedArray.decode(fixedArrayTruncatedPaddingData); },
      L"EncoderException",
      "HLAfixedArray truncated inter-element padding");
  auto fixedArrayTrailingData = std::vector<rti::Octet>(
      alignedFixedArrayExpected.begin(),
      alignedFixedArrayExpected.end());
  fixedArrayTrailingData.push_back(static_cast<rti::Octet>(0U));
  rti::VariableLengthData fixedArrayTrailingDataValue(
      fixedArrayTrailingData.data(),
      fixedArrayTrailingData.size());
  requireException(
      [&] { decodedAlignedFixedArray.decode(fixedArrayTrailingDataValue); },
      L"EncoderException",
      "HLAfixedArray trailing data");
  rti::HLAfixedArrayT<rti::HLAinteger16BE> typedFixedArray{3U};
  typedFixedArray
      .set(0U, rti::HLAinteger16BE{1})
      .set(1U, rti::HLAinteger16BE{2})
      .set(2U, rti::HLAinteger16BE{3});
  requireEncodedBytes(
      typedFixedArray,
      {0U, 1U, 0U, 2U, 0U, 3U},
      "HLAfixedArrayT encoding");

  rti::HLAinteger32BE fixedArrayLifetimePrototype{9};
  rti::HLAfixedArray fixedArrayLifetime{
      fixedArrayLifetimePrototype,
      2U};
  require(
      dynamic_cast<rti::HLAinteger32BE const&>(
          fixedArrayLifetime.get(1U)).get() == 9,
      "HLAfixedArray did not clone its prototype");
  requireEncodedBytes(
      fixedArrayLifetime,
      {0U, 0U, 0U, 9U, 0U, 0U, 0U, 9U},
      "HLAfixedArray prototype clone");
  rti::HLAinteger32BE borrowedFixedArrayElement{1};
  fixedArrayLifetime.setElementPointer(0U, &borrowedFixedArrayElement);
  borrowedFixedArrayElement.set(2);
  requireEncodedBytes(
      fixedArrayLifetime,
      {0U, 0U, 0U, 2U, 0U, 0U, 0U, 9U},
      "HLAfixedArray borrowed element");
  rti::HLAinteger32BE fixedArrayReplacement{3};
  fixedArrayLifetime.set(0U, fixedArrayReplacement);
  require(
      borrowedFixedArrayElement.get() == 3,
      "HLAfixedArray set did not update borrowed storage");
  rti::HLAinteger32BE fixedArraySubstitute{4};
  fixedArrayLifetime.setElementPointer(0U, &fixedArraySubstitute);
  fixedArraySubstitute.set(5);
  requireEncodedBytes(
      fixedArrayLifetime,
      {0U, 0U, 0U, 5U, 0U, 0U, 0U, 9U},
      "HLAfixedArray replacement pointer");
  rti::HLAfixedArray copiedFixedArray{fixedArrayLifetime};
  fixedArraySubstitute.set(6);
  requireEncodedBytes(
      copiedFixedArray,
      {0U, 0U, 0U, 5U, 0U, 0U, 0U, 9U},
      "HLAfixedArray copied borrowed element");
  rti::HLAfixedArray sameFixedArrayShape{
      rti::HLAinteger32BE{},
      2U};
  rti::HLAfixedArray differentFixedArrayLength{
      rti::HLAinteger32BE{},
      1U};
  rti::HLAfixedArray differentFixedArrayPrototype{
      rti::HLAoctet{},
      2U};
  require(
      fixedArrayLifetime.isSameTypeAs(sameFixedArrayShape),
      "HLAfixedArray same-shape comparison failed");
  require(
      !fixedArrayLifetime.isSameTypeAs(differentFixedArrayLength),
      "HLAfixedArray accepted a different cardinality");
  require(
      !fixedArrayLifetime.isSameTypeAs(differentFixedArrayPrototype),
      "HLAfixedArray accepted a different prototype");
  require(
      fixedArrayLifetime.hasPrototypeSameTypeAs(fixedArrayReplacement),
      "HLAfixedArray prototype type comparison failed");
  require(
      !fixedArrayLifetime.hasPrototypeSameTypeAs(rti::HLAoctet{}),
      "HLAfixedArray accepted a wrong prototype type");
  rti::HLAfixedArrayT<rti::HLAinteger32BE> typedFixedArrayShape{2U};
  rti::HLAfixedArrayT<rti::HLAinteger32BE> sameTypedFixedArray{2U};
  rti::HLAfixedArrayT<rti::HLAinteger16BE> differentTypedFixedArray{2U};
  require(
      typedFixedArrayShape.isSameTypeAs(sameTypedFixedArray),
      "HLAfixedArrayT same-type comparison failed");
  require(
      !typedFixedArrayShape.isSameTypeAs(differentTypedFixedArray),
      "HLAfixedArrayT accepted a different prototype type");
  rti::HLAoctet wrongFixedArrayType{
      static_cast<rti::Octet>(1U)};
  requireException(
      [&] { fixedArrayLifetime.set(0U, wrongFixedArrayType); },
      L"EncoderException",
      "HLAfixedArray wrong element type");
  requireException(
      [&] { fixedArrayLifetime.setElementPointer(0U, &wrongFixedArrayType); },
      L"EncoderException",
      "HLAfixedArray wrong element pointer type");
  requireException(
      [&] { fixedArrayLifetime.setElementPointer(0U, nullptr); },
      L"EncoderException",
      "HLAfixedArray null element pointer");
  requireException(
      [&] { (void)fixedArrayLifetime.get(2U); },
      L"EncoderException",
      "HLAfixedArray out-of-range element lookup");

  rti::HLAvariableArray alignedVariableArray{rti::HLAfloat64BE{}};
  alignedVariableArray.addElement(rti::HLAfloat64BE{-2.5});
  std::vector<std::uint8_t> alignedVariableArrayExpected{
      0U, 0U, 0U, 1U,
      0U, 0U, 0U, 0U,
      0xc0U, 0x04U, 0U, 0U, 0U, 0U, 0U, 0U};
  requireEncodedBytes(
      alignedVariableArray,
      alignedVariableArrayExpected,
      "HLAvariableArray signed-count encoding");
  require(
      alignedVariableArray.getEncodedLength() == alignedVariableArrayExpected.size(),
      "HLAvariableArray encoded length was incorrect");
  require(
      alignedVariableArray.getOctetBoundary() == 8U,
      "HLAvariableArray did not expose its prototype boundary");
  require(
      alignedVariableArray.size() == 1U,
      "HLAvariableArray element count was incorrect");
  requireEncodedInto(
      alignedVariableArray,
      alignedVariableArrayExpected,
      "HLAvariableArray encodeInto");
  std::vector<rti::Octet> alignedVariableArrayNested{
      static_cast<rti::Octet>(0x7eU)};
  alignedVariableArray.encodeInto(alignedVariableArrayNested);
  rti::HLAvariableArray decodedAlignedVariableArray{rti::HLAfloat64BE{}};
  require(
      decodedAlignedVariableArray.decodeFrom(
          alignedVariableArrayNested,
          1U) == alignedVariableArrayNested.size(),
      "HLAvariableArray decodeFrom did not consume its value");
  require(
      decodedAlignedVariableArray.size() == 1U &&
          dynamic_cast<rti::HLAfloat64BE const&>(
              decodedAlignedVariableArray.get(0U)).get() == -2.5,
      "HLAvariableArray element did not round-trip");
  auto variableArrayNonzeroLeadingPadding = std::vector<rti::Octet>(
      alignedVariableArrayExpected.begin(),
      alignedVariableArrayExpected.end());
  variableArrayNonzeroLeadingPadding[4U] = static_cast<rti::Octet>(1U);
  rti::VariableLengthData variableArrayNonzeroLeadingPaddingData(
      variableArrayNonzeroLeadingPadding.data(),
      variableArrayNonzeroLeadingPadding.size());
  requireException(
      [&] {
        decodedAlignedVariableArray.decode(
            variableArrayNonzeroLeadingPaddingData);
      },
      L"EncoderException",
      "HLAvariableArray nonzero leading padding");
  std::vector<rti::Octet> variableArrayTruncatedLeadingPadding{
      static_cast<rti::Octet>(0U),
      static_cast<rti::Octet>(0U),
      static_cast<rti::Octet>(0U),
      static_cast<rti::Octet>(1U),
      static_cast<rti::Octet>(0U),
      static_cast<rti::Octet>(0U),
      static_cast<rti::Octet>(0U)};
  rti::VariableLengthData variableArrayTruncatedLeadingPaddingData(
      variableArrayTruncatedLeadingPadding.data(),
      variableArrayTruncatedLeadingPadding.size());
  requireException(
      [&] {
        decodedAlignedVariableArray.decode(
            variableArrayTruncatedLeadingPaddingData);
      },
      L"EncoderException",
      "HLAvariableArray truncated leading padding");
  std::vector<rti::Octet> variableArrayNegativeCount{
      static_cast<rti::Octet>(0x80U),
      static_cast<rti::Octet>(0U),
      static_cast<rti::Octet>(0U),
      static_cast<rti::Octet>(0U)};
  rti::VariableLengthData variableArrayNegativeCountData(
      variableArrayNegativeCount.data(),
      variableArrayNegativeCount.size());
  requireException(
      [&] { decodedAlignedVariableArray.decode(variableArrayNegativeCountData); },
      L"EncoderException",
      "HLAvariableArray negative element count");
  auto variableArrayTrailingData = std::vector<rti::Octet>(
      alignedVariableArrayExpected.begin(),
      alignedVariableArrayExpected.end());
  variableArrayTrailingData.push_back(static_cast<rti::Octet>(0U));
  rti::VariableLengthData variableArrayTrailingDataValue(
      variableArrayTrailingData.data(),
      variableArrayTrailingData.size());
  requireException(
      [&] { decodedAlignedVariableArray.decode(variableArrayTrailingDataValue); },
      L"EncoderException",
      "HLAvariableArray trailing data");
  rti::HLAvariableArray emptyVariableArray{rti::HLAfloat64BE{}};
  requireEncodedBytes(
      emptyVariableArray,
      {0U, 0U, 0U, 0U},
      "HLAvariableArray empty encoding");
  require(
      emptyVariableArray.getEncodedLength() == 4U,
      "HLAvariableArray empty encoded length was incorrect");

  rti::HLAfixedRecord variableRecordPrototype;
  variableRecordPrototype
      .appendElement(rti::HLAinteger32BE{})
      .appendElement(rti::HLAoctet{});
  rti::HLAvariableArray variableRecordArray{variableRecordPrototype};
  rti::HLAfixedRecord variableRecordFirst;
  variableRecordFirst
      .appendElement(rti::HLAinteger32BE{1})
      .appendElement(rti::HLAoctet{static_cast<rti::Octet>(0x11U)});
  rti::HLAfixedRecord variableRecordSecond;
  variableRecordSecond
      .appendElement(rti::HLAinteger32BE{2})
      .appendElement(rti::HLAoctet{static_cast<rti::Octet>(0x22U)});
  variableRecordArray
      .addElement(variableRecordFirst)
      .addElement(variableRecordSecond);
  std::vector<std::uint8_t> variableRecordArrayExpected{
      0U, 0U, 0U, 2U,
      0U, 0U, 0U, 1U, 0x11U,
      0U, 0U, 0U,
      0U, 0U, 0U, 2U, 0x22U};
  requireEncodedBytes(
      variableRecordArray,
      variableRecordArrayExpected,
      "HLAvariableArray record-element padding");
  require(
      variableRecordArray.getEncodedLength() == variableRecordArrayExpected.size(),
      "HLAvariableArray record-element length was incorrect");
  auto variableRecordNonzeroPadding = std::vector<rti::Octet>(
      variableRecordArrayExpected.begin(),
      variableRecordArrayExpected.end());
  variableRecordNonzeroPadding[9U] = static_cast<rti::Octet>(1U);
  rti::VariableLengthData variableRecordNonzeroPaddingData(
      variableRecordNonzeroPadding.data(),
      variableRecordNonzeroPadding.size());
  requireException(
      [&] { variableRecordArray.decode(variableRecordNonzeroPaddingData); },
      L"EncoderException",
      "HLAvariableArray nonzero record-element padding");
  rti::HLAinteger32BE copiedVariableSource{1};
  rti::HLAvariableArray variableLifetime{rti::HLAinteger32BE{}};
  variableLifetime.addElement(copiedVariableSource);
  copiedVariableSource.set(2);
  requireEncodedBytes(
      variableLifetime,
      {0U, 0U, 0U, 1U, 0U, 0U, 0U, 1U},
      "HLAvariableArray copied element lifetime");
  rti::HLAinteger32BE borrowedVariableElement{3};
  variableLifetime.addElementPointer(&borrowedVariableElement);
  borrowedVariableElement.set(4);
  requireEncodedBytes(
      variableLifetime,
      {0U, 0U, 0U, 2U, 0U, 0U, 0U, 1U, 0U, 0U, 0U, 4U},
      "HLAvariableArray borrowed element lifetime");
  rti::HLAinteger32BE variableReplacement{5};
  variableLifetime.set(1U, variableReplacement);
  require(
      borrowedVariableElement.get() == 5,
      "HLAvariableArray set did not update borrowed storage");
  rti::HLAinteger32BE variableSubstitute{6};
  variableLifetime.setElementPointer(1U, &variableSubstitute);
  variableSubstitute.set(7);
  rti::HLAvariableArray copiedVariableArray{variableLifetime};
  variableSubstitute.set(8);
  requireEncodedBytes(
      copiedVariableArray,
      {0U, 0U, 0U, 2U, 0U, 0U, 0U, 1U, 0U, 0U, 0U, 7U},
      "HLAvariableArray copied borrowed element");
  rti::HLAvariableArray sameVariablePrototype{rti::HLAinteger32BE{}};
  rti::HLAvariableArray differentVariablePrototype{rti::HLAoctet{}};
  require(
      variableLifetime.isSameTypeAs(sameVariablePrototype),
      "HLAvariableArray same-prototype comparison failed");
  require(
      !variableLifetime.isSameTypeAs(differentVariablePrototype),
      "HLAvariableArray accepted a different prototype");
  require(
      variableLifetime.hasPrototypeSameTypeAs(variableReplacement),
      "HLAvariableArray prototype type comparison failed");
  require(
      !variableLifetime.hasPrototypeSameTypeAs(rti::HLAoctet{}),
      "HLAvariableArray accepted a wrong prototype type");
  rti::HLAvariableArrayT<rti::HLAinteger16BE> typedVariableArray;
  typedVariableArray
      .addElement(rti::HLAinteger16BE{1})
      .addElement(rti::HLAinteger16BE{2});
  requireEncodedBytes(
      typedVariableArray,
      {0U, 0U, 0U, 2U, 0U, 1U, 0U, 2U},
      "HLAvariableArrayT encoding");
  rti::HLAvariableArrayT<rti::HLAinteger16BE> sameTypedVariableArray;
  rti::HLAvariableArrayT<rti::HLAinteger32BE> differentTypedVariableArray;
  require(
      typedVariableArray.isSameTypeAs(sameTypedVariableArray),
      "HLAvariableArrayT same-type comparison failed");
  require(
      !typedVariableArray.isSameTypeAs(differentTypedVariableArray),
      "HLAvariableArrayT accepted a different prototype type");
  rti::HLAoctet wrongVariableArrayType{
      static_cast<rti::Octet>(1U)};
  requireException(
      [&] { variableLifetime.addElement(wrongVariableArrayType); },
      L"EncoderException",
      "HLAvariableArray wrong element type");
  requireException(
      [&] { variableLifetime.addElementPointer(nullptr); },
      L"EncoderException",
      "HLAvariableArray null element pointer");
  requireException(
      [&] { variableLifetime.set(0U, wrongVariableArrayType); },
      L"EncoderException",
      "HLAvariableArray wrong replacement type");
  requireException(
      [&] { variableLifetime.setElementPointer(0U, &wrongVariableArrayType); },
      L"EncoderException",
      "HLAvariableArray wrong replacement pointer type");
  requireException(
      [&] { (void)variableLifetime.get(2U); },
      L"EncoderException",
      "HLAvariableArray out-of-range element lookup");
  std::vector<rti::Octet> oneVariableElement{
      static_cast<rti::Octet>(0U),
      static_cast<rti::Octet>(0U),
      static_cast<rti::Octet>(0U),
      static_cast<rti::Octet>(1U),
      static_cast<rti::Octet>(0U),
      static_cast<rti::Octet>(0U),
      static_cast<rti::Octet>(0U),
      static_cast<rti::Octet>(9U)};
  rti::VariableLengthData oneVariableElementData(
      oneVariableElement.data(),
      oneVariableElement.size());
  variableLifetime.decode(oneVariableElementData);
  require(
      variableLifetime.size() == 1U &&
          dynamic_cast<rti::HLAinteger32BE const&>(
              variableLifetime.get(0U)).get() == 9,
      "HLAvariableArray decode did not replace its elements");

  rti::HLAvariantRecord alignedVariantRecord{rti::HLAoctet{}};
  alignedVariantRecord.addVariant(
      rti::HLAoctet{static_cast<rti::Octet>(1U)},
      rti::HLAinteger32BE{});
  alignedVariantRecord.addVariant(
      rti::HLAoctet{static_cast<rti::Octet>(2U)},
      rti::HLAfloat64BE{});
  alignedVariantRecord.setVariant(
      rti::HLAoctet{static_cast<rti::Octet>(1U)},
      rti::HLAinteger32BE{0x10203040});
  std::vector<std::uint8_t> alignedVariantRecordExpected{
      1U,
      0U, 0U, 0U, 0U, 0U, 0U, 0U,
      0x10U, 0x20U, 0x30U, 0x40U};
  requireEncodedBytes(
      alignedVariantRecord,
      alignedVariantRecordExpected,
      "HLAvariantRecord mapped-alternative alignment");
  require(
      alignedVariantRecord.getEncodedLength() == alignedVariantRecordExpected.size(),
      "HLAvariantRecord encoded length was incorrect");
  require(
      alignedVariantRecord.getOctetBoundary() == 8U,
      "HLAvariantRecord did not expose the maximum alternative boundary");
  requireEncodedInto(
      alignedVariantRecord,
      alignedVariantRecordExpected,
      "HLAvariantRecord encodeInto");
  std::vector<rti::Octet> alignedVariantRecordNested{
      static_cast<rti::Octet>(0x7eU)};
  alignedVariantRecord.encodeInto(alignedVariantRecordNested);
  rti::HLAvariantRecord decodedAlignedVariantRecord{rti::HLAoctet{}};
  decodedAlignedVariantRecord.addVariant(
      rti::HLAoctet{static_cast<rti::Octet>(1U)},
      rti::HLAinteger32BE{});
  decodedAlignedVariantRecord.addVariant(
      rti::HLAoctet{static_cast<rti::Octet>(2U)},
      rti::HLAfloat64BE{});
  require(
      decodedAlignedVariantRecord.decodeFrom(
          alignedVariantRecordNested,
          1U) == alignedVariantRecordNested.size(),
      "HLAvariantRecord decodeFrom did not consume its value");
  require(
      dynamic_cast<rti::HLAoctet const&>(
          decodedAlignedVariantRecord.getDiscriminant()).get() ==
          static_cast<rti::Octet>(1U) &&
          dynamic_cast<rti::HLAinteger32BE const&>(
              decodedAlignedVariantRecord.getVariant()).get() == 0x10203040,
      "HLAvariantRecord mapped alternative did not round-trip");
  auto variantRecordNonzeroPadding = std::vector<rti::Octet>(
      alignedVariantRecordExpected.begin(),
      alignedVariantRecordExpected.end());
  variantRecordNonzeroPadding[1U] = static_cast<rti::Octet>(1U);
  rti::VariableLengthData variantRecordNonzeroPaddingData(
      variantRecordNonzeroPadding.data(),
      variantRecordNonzeroPadding.size());
  requireException(
      [&] { decodedAlignedVariantRecord.decode(variantRecordNonzeroPaddingData); },
      L"EncoderException",
      "HLAvariantRecord nonzero alignment padding");
  std::vector<rti::Octet> variantRecordTruncatedPadding{
      static_cast<rti::Octet>(1U),
      static_cast<rti::Octet>(0U),
      static_cast<rti::Octet>(0U),
      static_cast<rti::Octet>(0U),
      static_cast<rti::Octet>(0U),
      static_cast<rti::Octet>(0U),
      static_cast<rti::Octet>(0U)};
  rti::VariableLengthData variantRecordTruncatedPaddingData(
      variantRecordTruncatedPadding.data(),
      variantRecordTruncatedPadding.size());
  requireException(
      [&] { decodedAlignedVariantRecord.decode(variantRecordTruncatedPaddingData); },
      L"EncoderException",
      "HLAvariantRecord truncated alignment padding");
  auto variantRecordTrailingData = std::vector<rti::Octet>(
      alignedVariantRecordExpected.begin(),
      alignedVariantRecordExpected.end());
  variantRecordTrailingData.push_back(static_cast<rti::Octet>(0U));
  rti::VariableLengthData variantRecordTrailingDataValue(
      variantRecordTrailingData.data(),
      variantRecordTrailingData.size());
  requireException(
      [&] { decodedAlignedVariantRecord.decode(variantRecordTrailingDataValue); },
      L"EncoderException",
      "HLAvariantRecord trailing data");
  alignedVariantRecord.setDiscriminant(
      rti::HLAoctet{static_cast<rti::Octet>(0x7fU)});
  requireEncodedBytes(
      alignedVariantRecord,
      {0x7fU},
      "HLAvariantRecord unmapped discriminant encoding");
  require(
      alignedVariantRecord.getEncodedLength() == 1U,
      "HLAvariantRecord unmapped discriminant length was incorrect");
  requireException(
      [&] { (void)alignedVariantRecord.getVariant(); },
      L"EncoderException",
      "HLAvariantRecord unmapped variant lookup");
  std::vector<rti::Octet> unknownVariantDiscriminant{
      static_cast<rti::Octet>(0x7fU)};
  rti::VariableLengthData unknownVariantDiscriminantData(
      unknownVariantDiscriminant.data(),
      unknownVariantDiscriminant.size());
  decodedAlignedVariantRecord.decode(unknownVariantDiscriminantData);
  require(
      dynamic_cast<rti::HLAoctet const&>(
          decodedAlignedVariantRecord.getDiscriminant()).get() ==
          static_cast<rti::Octet>(0x7fU),
      "HLAvariantRecord unknown discriminant did not decode");
  requireException(
      [&] { (void)decodedAlignedVariantRecord.getVariant(); },
      L"EncoderException",
      "HLAvariantRecord decoded unknown variant lookup");

  rti::HLAvariantRecord variantLifetime{rti::HLAinteger32BE{}};
  rti::HLAinteger32BE copiedVariantSource{1};
  variantLifetime.addVariant(rti::HLAinteger32BE{1}, copiedVariantSource);
  copiedVariantSource.set(2);
  requireEncodedBytes(
      variantLifetime,
      {0U, 0U, 0U, 1U, 0U, 0U, 0U, 1U},
      "HLAvariantRecord copied variant lifetime");
  rti::HLAinteger32BE borrowedVariantRecordValue{3};
  variantLifetime.addVariantPointer(
      rti::HLAinteger32BE{2},
      &borrowedVariantRecordValue);
  borrowedVariantRecordValue.set(4);
  requireEncodedBytes(
      variantLifetime,
      {0U, 0U, 0U, 2U, 0U, 0U, 0U, 4U},
      "HLAvariantRecord borrowed variant lifetime");
  rti::HLAinteger32BE replacementVariantRecordValue{5};
  variantLifetime.setVariant(
      rti::HLAinteger32BE{2},
      replacementVariantRecordValue);
  require(
      borrowedVariantRecordValue.get() == 5,
      "HLAvariantRecord setVariant did not update borrowed storage");
  rti::HLAinteger32BE substituteVariantRecordValue{6};
  variantLifetime.setVariantPointer(
      rti::HLAinteger32BE{2},
      &substituteVariantRecordValue);
  substituteVariantRecordValue.set(7);
  rti::HLAvariantRecord copiedVariantRecord{variantLifetime};
  substituteVariantRecordValue.set(8);
  requireEncodedBytes(
      copiedVariantRecord,
      {0U, 0U, 0U, 2U, 0U, 0U, 0U, 7U},
      "HLAvariantRecord copied borrowed variant");
  rti::HLAvariantRecord sameVariantRecordShape{rti::HLAinteger32BE{}};
  sameVariantRecordShape.addVariant(
      rti::HLAinteger32BE{1},
      rti::HLAinteger32BE{});
  sameVariantRecordShape.addVariant(
      rti::HLAinteger32BE{2},
      rti::HLAinteger32BE{});
  rti::HLAvariantRecord differentVariantRecordAlternative{
      rti::HLAinteger32BE{}};
  differentVariantRecordAlternative.addVariant(
      rti::HLAinteger32BE{1},
      rti::HLAinteger32BE{});
  differentVariantRecordAlternative.addVariant(
      rti::HLAinteger32BE{2},
      rti::HLAoctet{});
  rti::HLAvariantRecord differentVariantRecordDiscriminant{
      rti::HLAoctet{}};
  differentVariantRecordDiscriminant.addVariant(
      rti::HLAoctet{static_cast<rti::Octet>(1U)},
      rti::HLAinteger32BE{});
  require(
      variantLifetime.isSameTypeAs(sameVariantRecordShape),
      "HLAvariantRecord same-shape comparison failed");
  require(
      !variantLifetime.isSameTypeAs(differentVariantRecordAlternative),
      "HLAvariantRecord accepted a different alternative shape");
  require(
      !variantLifetime.isSameTypeAs(differentVariantRecordDiscriminant),
      "HLAvariantRecord accepted a different discriminant shape");
  require(
      variantLifetime.isSameTypeAs(
          rti::HLAinteger32BE{1},
          rti::HLAinteger32BE{}),
      "HLAvariantRecord typed alternative comparison failed");
  require(
      !variantLifetime.isSameTypeAs(
          rti::HLAinteger32BE{1},
          rti::HLAoctet{}),
      "HLAvariantRecord accepted a wrong alternative type");
  require(
      variantLifetime.hasMatchingDiscriminantTypeAs(rti::HLAinteger32BE{}),
      "HLAvariantRecord discriminant prototype comparison failed");
  require(
      !variantLifetime.hasMatchingDiscriminantTypeAs(rti::HLAoctet{}),
      "HLAvariantRecord accepted a wrong discriminant prototype");
  rti::HLAvariantRecordT<rti::HLAoctet> typedVariantRecord;
  typedVariantRecord.addVariant(
      rti::HLAoctet{static_cast<rti::Octet>(1U)},
      rti::HLAinteger16BE{});
  typedVariantRecord.setVariant(
      rti::HLAoctet{static_cast<rti::Octet>(1U)},
      rti::HLAinteger16BE{9});
  requireEncodedBytes(
      typedVariantRecord,
      {1U, 0U, 0U, 9U},
      "HLAvariantRecordT encoding");
  rti::HLAvariantRecordT<rti::HLAoctet> sameTypedVariantRecord;
  rti::HLAvariantRecordT<rti::HLAinteger32BE> differentTypedVariantRecord;
  require(
      typedVariantRecord.isSameTypeAs(sameTypedVariantRecord),
      "HLAvariantRecordT same-type comparison failed");
  require(
      !typedVariantRecord.isSameTypeAs(differentTypedVariantRecord),
      "HLAvariantRecordT accepted a different discriminant type");
  rti::HLAoctet wrongVariantRecordDiscriminant{
      static_cast<rti::Octet>(1U)};
  rti::HLAoctet wrongVariantRecordValue{
      static_cast<rti::Octet>(2U)};
  requireException(
      [&] {
        variantLifetime.addVariant(
            rti::HLAinteger32BE{2},
            rti::HLAinteger32BE{});
      },
      L"EncoderException",
      "HLAvariantRecord duplicate discriminant");
  requireException(
      [&] {
        variantLifetime.addVariant(
            wrongVariantRecordDiscriminant,
            rti::HLAinteger32BE{});
      },
      L"EncoderException",
      "HLAvariantRecord wrong discriminant type");
  requireException(
      [&] {
        variantLifetime.addVariantPointer(
            rti::HLAinteger32BE{3},
            nullptr);
      },
      L"EncoderException",
      "HLAvariantRecord null added variant pointer");
  requireException(
      [&] {
        variantLifetime.setDiscriminant(wrongVariantRecordDiscriminant);
      },
      L"EncoderException",
      "HLAvariantRecord wrong selected discriminant type");
  requireException(
      [&] {
        variantLifetime.setVariant(
            rti::HLAinteger32BE{3},
            rti::HLAinteger32BE{});
      },
      L"EncoderException",
      "HLAvariantRecord unknown selected alternative");
  requireException(
      [&] {
        variantLifetime.setVariant(
            rti::HLAinteger32BE{2},
            wrongVariantRecordValue);
      },
      L"EncoderException",
      "HLAvariantRecord wrong alternative type");
  requireException(
      [&] {
        variantLifetime.setVariantPointer(
            rti::HLAinteger32BE{2},
            &wrongVariantRecordValue);
      },
      L"EncoderException",
      "HLAvariantRecord wrong pointer alternative type");
  requireException(
      [&] {
        variantLifetime.setVariantPointer(
            rti::HLAinteger32BE{2},
            nullptr);
      },
      L"EncoderException",
      "HLAvariantRecord null replacement pointer");

  std::vector<rti::Octet> opaquePayload{
      static_cast<rti::Octet>(0xa5U),
      static_cast<rti::Octet>(0U),
      static_cast<rti::Octet>(0x5aU)};
  rti::HLAopaqueData opaqueValue{opaquePayload.data(), opaquePayload.size()};
  requireEncodedBytes(
      opaqueValue,
      {0U, 0U, 0U, 3U, 0xa5U, 0U, 0x5aU},
      "HLAopaqueData payload");
  requireEncodedInto(
      opaqueValue,
      {0U, 0U, 0U, 3U, 0xa5U, 0U, 0x5aU},
      "HLAopaqueData payload");
  require(
      opaqueValue.getEncodedLength() == 7U &&
          opaqueValue.getOctetBoundary() == 4U &&
          opaqueValue.bufferLength() == 3U &&
          opaqueValue.dataLength() == 3U,
      "HLAopaqueData payload length and boundary mismatch");
  require(
      std::vector<rti::Octet>(
          opaqueValue.get(), opaqueValue.get() + opaqueValue.dataLength()) ==
          opaquePayload,
      "HLAopaqueData payload storage mismatch");

  rti::HLAopaqueData decodedOpaqueValue;
  decodedOpaqueValue.decode(opaqueValue.encode());
  require(
      decodedOpaqueValue.dataLength() == opaquePayload.size() &&
          std::vector<rti::Octet>(
              decodedOpaqueValue.get(),
              decodedOpaqueValue.get() + decodedOpaqueValue.dataLength()) ==
              opaquePayload,
      "HLAopaqueData decode round-trip mismatch");
  std::vector<rti::Octet> opaqueNested{
      static_cast<rti::Octet>(0x7eU),
      static_cast<rti::Octet>(0U),
      static_cast<rti::Octet>(0U),
      static_cast<rti::Octet>(0U),
      static_cast<rti::Octet>(2U),
      static_cast<rti::Octet>(0x12U),
      static_cast<rti::Octet>(0x34U),
      static_cast<rti::Octet>(0x7fU)};
  require(
      decodedOpaqueValue.decodeFrom(opaqueNested, 1U) == 7U &&
          decodedOpaqueValue.dataLength() == 2U &&
          std::vector<rti::Octet>(
              decodedOpaqueValue.get(),
              decodedOpaqueValue.get() + decodedOpaqueValue.dataLength()) ==
              std::vector<rti::Octet>{
                  static_cast<rti::Octet>(0x12U),
                  static_cast<rti::Octet>(0x34U)},
      "HLAopaqueData decodeFrom mismatch");
  requireException(
      [&] {
        std::vector<rti::Octet> bytes{
            static_cast<rti::Octet>(0U),
            static_cast<rti::Octet>(0U),
            static_cast<rti::Octet>(0U)};
        rti::VariableLengthData data(bytes.data(), bytes.size());
        decodedOpaqueValue.decode(data);
      },
      L"EncoderException",
      "HLAopaqueData truncated length");
  requireException(
      [&] {
        std::vector<rti::Octet> bytes{
            static_cast<rti::Octet>(0U),
            static_cast<rti::Octet>(0U),
            static_cast<rti::Octet>(0U),
            static_cast<rti::Octet>(2U),
            static_cast<rti::Octet>(0x12U)};
        rti::VariableLengthData data(bytes.data(), bytes.size());
        decodedOpaqueValue.decode(data);
      },
      L"EncoderException",
      "HLAopaqueData truncated payload");
  requireException(
      [&] {
        std::vector<rti::Octet> bytes{
            static_cast<rti::Octet>(0x80U),
            static_cast<rti::Octet>(0U),
            static_cast<rti::Octet>(0U),
            static_cast<rti::Octet>(0U)};
        rti::VariableLengthData data(bytes.data(), bytes.size());
        decodedOpaqueValue.decode(data);
      },
      L"EncoderException",
      "HLAopaqueData invalid signed length");
  requireException(
      [&] {
        std::vector<rti::Octet> bytes{
            static_cast<rti::Octet>(0U),
            static_cast<rti::Octet>(0U),
            static_cast<rti::Octet>(0U),
            static_cast<rti::Octet>(1U),
            static_cast<rti::Octet>(0x42U),
            static_cast<rti::Octet>(0U)};
        rti::VariableLengthData data(bytes.data(), bytes.size());
        decodedOpaqueValue.decode(data);
      },
      L"EncoderException",
      "HLAopaqueData trailing data");

  std::vector<rti::Octet> opaqueExternalStorage{
      static_cast<rti::Octet>(0x10U),
      static_cast<rti::Octet>(0x20U),
      static_cast<rti::Octet>(0x30U),
      static_cast<rti::Octet>(0x40U),
      static_cast<rti::Octet>(0x50U),
      static_cast<rti::Octet>(0x60U)};
  rti::Octet* opaqueExternalPointer = opaqueExternalStorage.data();
  rti::HLAopaqueData opaqueExternal{
      &opaqueExternalPointer, opaqueExternalStorage.size(), 3U};
  require(
      opaqueExternal.bufferLength() == opaqueExternalStorage.size() &&
          opaqueExternal.dataLength() == 3U,
      "HLAopaqueData external storage lengths mismatch");
  requireEncodedBytes(
      opaqueExternal,
      {0U, 0U, 0U, 3U, 0x10U, 0x20U, 0x30U},
      "HLAopaqueData external storage");
  opaqueExternalStorage[1U] = static_cast<rti::Octet>(0x99U);
  requireEncodedBytes(
      opaqueExternal,
      {0U, 0U, 0U, 3U, 0x10U, 0x99U, 0x30U},
      "HLAopaqueData external storage mutation");

  std::vector<rti::Octet> opaqueReplacement{
      static_cast<rti::Octet>(0xa5U),
      static_cast<rti::Octet>(0xb6U),
      static_cast<rti::Octet>(0xc7U),
      static_cast<rti::Octet>(0xd8U)};
  opaqueExternal.set(opaqueReplacement.data(), opaqueReplacement.size());
  require(
      opaqueExternal.dataLength() == opaqueReplacement.size() &&
          std::vector<rti::Octet>(
              opaqueExternalStorage.begin(),
              opaqueExternalStorage.begin() + opaqueReplacement.size()) ==
              opaqueReplacement,
      "HLAopaqueData external set did not write through");
  std::vector<rti::Octet> opaqueDecodedPayload{
      static_cast<rti::Octet>(0U),
      static_cast<rti::Octet>(0U),
      static_cast<rti::Octet>(0U),
      static_cast<rti::Octet>(2U),
      static_cast<rti::Octet>(0x55U),
      static_cast<rti::Octet>(0x66U)};
  rti::VariableLengthData opaqueDecodedData(
      opaqueDecodedPayload.data(), opaqueDecodedPayload.size());
  opaqueExternal.decode(opaqueDecodedData);
  require(
      opaqueExternal.dataLength() == 2U &&
          opaqueExternalStorage[0U] == static_cast<rti::Octet>(0x55U) &&
          opaqueExternalStorage[1U] == static_cast<rti::Octet>(0x66U),
      "HLAopaqueData external decode did not write through");

  rti::HLAopaqueData copiedOpaqueExternal{opaqueExternal};
  opaqueExternalStorage[0U] = static_cast<rti::Octet>(0xeeU);
  requireEncodedBytes(
      copiedOpaqueExternal,
      {0U, 0U, 0U, 2U, 0x55U, 0x66U},
      "HLAopaqueData copied external storage");
  std::vector<rti::Octet> opaqueTooLarge{
      static_cast<rti::Octet>(0U),
      static_cast<rti::Octet>(1U),
      static_cast<rti::Octet>(2U),
      static_cast<rti::Octet>(3U),
      static_cast<rti::Octet>(4U),
      static_cast<rti::Octet>(5U),
      static_cast<rti::Octet>(6U)};
  requireException(
      [&] {
        opaqueExternal.set(opaqueTooLarge.data(), opaqueTooLarge.size());
      },
      L"EncoderException",
      "HLAopaqueData external storage overflow");
  require(
      opaqueExternal.dataLength() == 2U &&
          opaqueExternalStorage[0U] == static_cast<rti::Octet>(0xeeU),
      "HLAopaqueData external overflow changed state");

  std::vector<rti::Octet> opaqueSecondStorage{
      static_cast<rti::Octet>(0x7aU),
      static_cast<rti::Octet>(0x7bU)};
  rti::Octet* opaqueSecondPointer = opaqueSecondStorage.data();
  opaqueExternal.setDataPointer(&opaqueSecondPointer, opaqueSecondStorage.size(), 1U);
  require(
      opaqueExternal.bufferLength() == opaqueSecondStorage.size() &&
          opaqueExternal.dataLength() == 1U,
      "HLAopaqueData setDataPointer lengths mismatch");
  requireEncodedBytes(
      opaqueExternal,
      {0U, 0U, 0U, 1U, 0x7aU},
      "HLAopaqueData setDataPointer");

  rti::Octet* opaqueNullPointer = nullptr;
  requireException(
      [&] {
        rti::HLAopaqueData invalid{&opaqueNullPointer, 1U, 0U};
      },
      L"EncoderException",
      "HLAopaqueData null external pointer");
  requireException(
      [&] {
        rti::HLAopaqueData invalid{
            &opaqueSecondPointer, 0U, 0U};
      },
      L"EncoderException",
      "HLAopaqueData zero external buffer");
  requireException(
      [&] {
        rti::HLAopaqueData invalid{
            &opaqueSecondPointer, 1U, 2U};
      },
      L"EncoderException",
      "HLAopaqueData oversized external payload");
  rti::HLAopaqueData emptyOpaque;
  emptyOpaque.set(nullptr, 0U);
  requireEncodedBytes(
      emptyOpaque,
      {0U, 0U, 0U, 0U},
      "HLAopaqueData empty payload");
  requireException(
      [&] { emptyOpaque.set(nullptr, 1U); },
      L"EncoderException",
      "HLAopaqueData null payload");

  Boundary16Element boundary16;
  rti::HLAextendableVariantRecord boundaryChecked{rti::HLAoctet{}};
  requireException(
      [&] {
        boundaryChecked.addVariant(
            rti::HLAoctet{static_cast<rti::Octet>(1U)},
            boundary16);
      },
      L"EncoderException",
      "HLAextendableVariantRecord alternative boundary");

  std::vector<std::uint8_t> truncated{0x01U, 0x02U, 0x03U};
  rti::VariableLengthData truncatedData(truncated.data(), truncated.size());
  requireException(
      [&] { decoded32.decode(truncatedData); },
      L"EncoderException",
      "truncated basic-data encoding");
}

}  // namespace hla_rti_cpp_tck::detail
