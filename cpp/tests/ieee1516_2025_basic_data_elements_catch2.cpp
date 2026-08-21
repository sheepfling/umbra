#include <catch2/catch_test_macros.hpp>

#include <RTI/VariableLengthData.h>
#include <RTI/encoding/BasicDataElements.h>
#include <RTI/encoding/EncodingExceptions.h>
#include <RTI/encoding/HLAfixedArray.h>
#include <RTI/encoding/HLAfixedRecord.h>
#include <RTI/encoding/HLAextendableVariantRecord.h>
#include <RTI/encoding/HLAopaqueData.h>
#include <RTI/encoding/HLAvariableArray.h>
#include <RTI/encoding/HLAvariantRecord.h>

#include <array>
#include <cstdint>
#include <initializer_list>
#include <memory>
#include <string>
#include <vector>

namespace {

std::vector<rti1516_2025::Octet> octets(rti1516_2025::VariableLengthData const& value) {
  auto const* data = static_cast<rti1516_2025::Octet const*>(value.data());
  return data == nullptr ? std::vector<rti1516_2025::Octet>{}
                         : std::vector<rti1516_2025::Octet>(data, data + value.size());
}

std::vector<rti1516_2025::Octet> byteValues(std::initializer_list<unsigned int> values) {
  std::vector<rti1516_2025::Octet> result;
  result.reserve(values.size());
  for (auto const value : values) {
    result.push_back(static_cast<rti1516_2025::Octet>(value));
  }
  return result;
}

class Boundary16Element final : public rti1516_2025::DataElement {
 public:
  std::unique_ptr<rti1516_2025::DataElement> clone() const override {
    return std::make_unique<Boundary16Element>();
  }

  rti1516_2025::VariableLengthData encode() const override {
    rti1516_2025::VariableLengthData value;
    encode(value);
    return value;
  }

  void encode(rti1516_2025::VariableLengthData& value) const override {
    std::array<rti1516_2025::Octet, 16U> bytes{};
    value.setData(bytes.data(), bytes.size());
  }

  void encodeInto(std::vector<rti1516_2025::Octet>& bytes) const override {
    bytes.insert(bytes.end(), 16U, static_cast<rti1516_2025::Octet>(0));
  }

  Boundary16Element& decode(rti1516_2025::VariableLengthData const& value) override {
    auto const bytes = octets(value);
    if (bytes.size() != 16U) {
      throw rti1516_2025::EncoderException(L"Boundary16Element has an invalid encoding length.");
    }
    return *this;
  }

  std::size_t decodeFrom(
      std::vector<rti1516_2025::Octet> const& bytes,
      std::size_t index) override {
    if (index > bytes.size() || bytes.size() - index < 16U) {
      throw rti1516_2025::EncoderException(L"Boundary16Element encoding is truncated.");
    }
    return index + 16U;
  }

  std::size_t getEncodedLength() const override {
    return 16U;
  }

  unsigned int getOctetBoundary() const override {
    return 16U;
  }
};

}  // namespace

TEST_CASE("Official basic encoding helpers use 1516.2 big-endian wire forms", "[baseline][encoding][unit][foundation]") {
  using namespace rti1516_2025;

  HLAinteger32BE signedValue{-2};
  REQUIRE(octets(signedValue.encode()) == byteValues({0xffU, 0xffU, 0xffU, 0xfeU}));
  HLAinteger32BE decodedSigned;
  decodedSigned.decode(signedValue.encode());
  REQUIRE(decodedSigned.get() == -2);

  HLAunsignedInteger32BE unsignedValue{0x1234abcdU};
  REQUIRE(octets(unsignedValue.encode()) == byteValues({0x12U, 0x34U, 0xabU, 0xcdU}));
  HLAunsignedInteger32BE decodedUnsigned;
  decodedUnsigned.decode(unsignedValue.encode());
  REQUIRE(decodedUnsigned.get() == 0x1234abcdU);

  HLAboolean trueValue{true};
  HLAboolean falseValue{false};
  REQUIRE(octets(trueValue.encode()) == byteValues({0U, 0U, 0U, 1U}));
  REQUIRE(octets(falseValue.encode()) == byteValues({0U, 0U, 0U, 0U}));

  auto const invalidBooleanBytes = byteValues({0U, 0U, 0U, 2U});
  VariableLengthData invalidBoolean(invalidBooleanBytes.data(), invalidBooleanBytes.size());
  REQUIRE_THROWS_AS(falseValue.decode(invalidBoolean), EncoderException);
}

TEST_CASE(
    "Official octet, byte, and ASCII basic encoding helpers use their 1516.2 wire representations",
    "[baseline][encoding][basic-data-elements][ascii][unit][foundation]") {
  using namespace rti1516_2025;

  auto const rawHighBitOctet = static_cast<Octet>(0xa5U);
  HLAoctet octet{rawHighBitOctet};
  HLAbyte byte{rawHighBitOctet};
  REQUIRE(octets(octet.encode()) == byteValues({0xa5U}));
  REQUIRE(octets(byte.encode()) == byteValues({0xa5U}));
  REQUIRE(octet.getEncodedLength() == 1U);
  REQUIRE(octet.getOctetBoundary() == 1U);
  REQUIRE(byte.getEncodedLength() == 1U);
  REQUIRE(byte.getOctetBoundary() == 1U);

  HLAoctet decodedOctet;
  HLAbyte decodedByte;
  decodedOctet.decode(octet.encode());
  decodedByte.decode(byte.encode());
  REQUIRE(decodedOctet.get() == rawHighBitOctet);
  REQUIRE(decodedByte.get() == rawHighBitOctet);

  HLAASCIIchar asciiCharacter{'A'};
  REQUIRE(octets(asciiCharacter.encode()) == byteValues({0x41U}));
  REQUIRE(asciiCharacter.getEncodedLength() == 1U);
  REQUIRE(asciiCharacter.getOctetBoundary() == 1U);
  HLAASCIIchar decodedAsciiCharacter;
  decodedAsciiCharacter.decode(asciiCharacter.encode());
  REQUIRE(decodedAsciiCharacter.get() == 'A');

  auto const asciiText = std::string{"A\0Z", 3U};
  HLAASCIIstring asciiString{asciiText};
  REQUIRE(octets(asciiString.encode()) == byteValues({0U, 0U, 0U, 3U, 0x41U, 0U, 0x5aU}));
  REQUIRE(asciiString.getEncodedLength() == 7U);
  REQUIRE(asciiString.getOctetBoundary() == 4U);
  HLAASCIIstring decodedAsciiString;
  decodedAsciiString.decode(asciiString.encode());
  REQUIRE(decodedAsciiString.get() == asciiText);

  HLAASCIIchar nonAsciiCharacter{static_cast<char>(0x80U)};
  REQUIRE_THROWS_AS(nonAsciiCharacter.encode(), EncoderException);
  auto const nonAsciiCharacterBytes = byteValues({0x80U});
  VariableLengthData nonAsciiCharacterData(
      nonAsciiCharacterBytes.data(), nonAsciiCharacterBytes.size());
  REQUIRE_THROWS_AS(decodedAsciiCharacter.decode(nonAsciiCharacterData), EncoderException);

  HLAASCIIstring nonAsciiString{std::string(1U, static_cast<char>(0x80U))};
  REQUIRE_THROWS_AS(nonAsciiString.encode(), EncoderException);
  auto const nonAsciiStringBytes = byteValues({0U, 0U, 0U, 1U, 0x80U});
  VariableLengthData nonAsciiStringData(nonAsciiStringBytes.data(), nonAsciiStringBytes.size());
  REQUIRE_THROWS_AS(decodedAsciiString.decode(nonAsciiStringData), EncoderException);

  auto const truncatedAsciiStringBytes = byteValues({0U, 0U, 0U, 1U});
  VariableLengthData truncatedAsciiString(
      truncatedAsciiStringBytes.data(), truncatedAsciiStringBytes.size());
  REQUIRE_THROWS_AS(decodedAsciiString.decode(truncatedAsciiString), EncoderException);

  auto const trailingOctetBytes = byteValues({0xa5U, 0U});
  VariableLengthData trailingOctet(trailingOctetBytes.data(), trailingOctetBytes.size());
  REQUIRE_THROWS_AS(decodedOctet.decode(trailingOctet), EncoderException);
}

TEST_CASE(
    "Official IEEE-754 basic encoding helpers use 1516.2 Table 29 wire forms",
    "[baseline][encoding][basic-data-elements][float][unit][foundation]") {
  using namespace rti1516_2025;

  HLAfloat32BE singleBigEndian{-2.5F};
  HLAfloat32LE singleLittleEndian{-2.5F};
  REQUIRE(octets(singleBigEndian.encode()) == byteValues({0xc0U, 0x20U, 0U, 0U}));
  REQUIRE(octets(singleLittleEndian.encode()) == byteValues({0U, 0U, 0x20U, 0xc0U}));
  REQUIRE(singleBigEndian.getEncodedLength() == 4U);
  REQUIRE(singleBigEndian.getOctetBoundary() == 4U);
  HLAfloat32BE decodedSingleBigEndian;
  HLAfloat32LE decodedSingleLittleEndian;
  decodedSingleBigEndian.decode(singleBigEndian.encode());
  decodedSingleLittleEndian.decode(singleLittleEndian.encode());
  REQUIRE(decodedSingleBigEndian.get() == -2.5F);
  REQUIRE(decodedSingleLittleEndian.get() == -2.5F);

  HLAfloat64BE doubleBigEndian{-2.5};
  HLAfloat64LE doubleLittleEndian{-2.5};
  REQUIRE(octets(doubleBigEndian.encode()) ==
          byteValues({0xc0U, 0x04U, 0U, 0U, 0U, 0U, 0U, 0U}));
  REQUIRE(octets(doubleLittleEndian.encode()) ==
          byteValues({0U, 0U, 0U, 0U, 0U, 0U, 0x04U, 0xc0U}));
  REQUIRE(doubleBigEndian.getEncodedLength() == 8U);
  REQUIRE(doubleBigEndian.getOctetBoundary() == 8U);
  HLAfloat64BE decodedDoubleBigEndian;
  HLAfloat64LE decodedDoubleLittleEndian;
  decodedDoubleBigEndian.decode(doubleBigEndian.encode());
  decodedDoubleLittleEndian.decode(doubleLittleEndian.encode());
  REQUIRE(decodedDoubleBigEndian.get() == -2.5);
  REQUIRE(decodedDoubleLittleEndian.get() == -2.5);

  auto const truncatedSingleBytes = byteValues({0xc0U, 0x20U, 0U});
  VariableLengthData truncatedSingle(truncatedSingleBytes.data(), truncatedSingleBytes.size());
  REQUIRE_THROWS_AS(decodedSingleBigEndian.decode(truncatedSingle), EncoderException);

  auto const truncatedDoubleBytes =
      byteValues({0xc0U, 0x04U, 0U, 0U, 0U, 0U, 0U});
  VariableLengthData truncatedDouble(truncatedDoubleBytes.data(), truncatedDoubleBytes.size());
  REQUIRE_THROWS_AS(decodedDoubleLittleEndian.decode(truncatedDouble), EncoderException);
}

TEST_CASE(
    "Official HLAunicodeChar uses one UTF-16BE code unit",
    "[baseline][encoding][basic-data-elements][unicode][unit][foundation]") {
  using namespace rti1516_2025;

  HLAunicodeChar bmpCharacter{static_cast<wchar_t>(0x03a9U)};
  REQUIRE(octets(bmpCharacter.encode()) == byteValues({0x03U, 0xa9U}));
  REQUIRE(bmpCharacter.getEncodedLength() == 2U);
  REQUIRE(bmpCharacter.getOctetBoundary() == 2U);
  HLAunicodeChar decodedBmpCharacter;
  decodedBmpCharacter.decode(bmpCharacter.encode());
  REQUIRE(decodedBmpCharacter.get() == static_cast<wchar_t>(0x03a9U));

  HLAunicodeChar highSurrogateUnit{static_cast<wchar_t>(0xd83dU)};
  REQUIRE(octets(highSurrogateUnit.encode()) == byteValues({0xd8U, 0x3dU}));
  HLAunicodeChar decodedSurrogateUnit;
  decodedSurrogateUnit.decode(highSurrogateUnit.encode());
  REQUIRE(decodedSurrogateUnit.get() == static_cast<wchar_t>(0xd83dU));

  auto const truncatedBytes = byteValues({0x03U});
  VariableLengthData truncated(truncatedBytes.data(), truncatedBytes.size());
  REQUIRE_THROWS_AS(decodedBmpCharacter.decode(truncated), EncoderException);

  auto const trailingBytes = byteValues({0x03U, 0xa9U, 0U});
  VariableLengthData trailing(trailingBytes.data(), trailingBytes.size());
  REQUIRE_THROWS_AS(decodedBmpCharacter.decode(trailing), EncoderException);

  if constexpr (sizeof(wchar_t) > 2U) {
    HLAunicodeChar supplementaryCharacter{static_cast<wchar_t>(0x1f600U)};
    REQUIRE_THROWS_AS(supplementaryCharacter.encode(), EncoderException);
  }
}

TEST_CASE(
    "Official 16-bit basic encoding helpers use 1516.2 Table 29 wire forms",
    "[baseline][encoding][basic-data-elements][unit][foundation]") {
  using namespace rti1516_2025;

  HLAinteger16BE signedBigEndian{-2};
  HLAinteger16LE signedLittleEndian{-2};
  REQUIRE(octets(signedBigEndian.encode()) == byteValues({0xffU, 0xfeU}));
  REQUIRE(octets(signedLittleEndian.encode()) == byteValues({0xfeU, 0xffU}));
  REQUIRE(signedBigEndian.getEncodedLength() == 2U);
  REQUIRE(signedBigEndian.getOctetBoundary() == 2U);

  HLAinteger16BE decodedSignedBigEndian;
  HLAinteger16LE decodedSignedLittleEndian;
  decodedSignedBigEndian.decode(signedBigEndian.encode());
  decodedSignedLittleEndian.decode(signedLittleEndian.encode());
  REQUIRE(decodedSignedBigEndian.get() == -2);
  REQUIRE(decodedSignedLittleEndian.get() == -2);

  HLAunsignedInteger16BE unsignedBigEndian{0x1234U};
  HLAunsignedInteger16LE unsignedLittleEndian{0x1234U};
  REQUIRE(octets(unsignedBigEndian.encode()) == byteValues({0x12U, 0x34U}));
  REQUIRE(octets(unsignedLittleEndian.encode()) == byteValues({0x34U, 0x12U}));
  HLAunsignedInteger16BE decodedUnsignedBigEndian;
  HLAunsignedInteger16LE decodedUnsignedLittleEndian;
  decodedUnsignedBigEndian.decode(unsignedBigEndian.encode());
  decodedUnsignedLittleEndian.decode(unsignedLittleEndian.encode());
  REQUIRE(decodedUnsignedBigEndian.get() == 0x1234U);
  REQUIRE(decodedUnsignedLittleEndian.get() == 0x1234U);

  auto const pair = OctetPair{
      static_cast<Octet>(0x12U),
      static_cast<Octet>(0x34U),
  };
  HLAoctetPairBE pairBigEndian{pair};
  HLAoctetPairLE pairLittleEndian{pair};
  REQUIRE(octets(pairBigEndian.encode()) == byteValues({0x12U, 0x34U}));
  REQUIRE(octets(pairLittleEndian.encode()) == byteValues({0x34U, 0x12U}));
  REQUIRE(pairBigEndian.getEncodedLength() == 2U);
  REQUIRE(pairBigEndian.getOctetBoundary() == 2U);
  HLAoctetPairBE decodedPairBigEndian;
  HLAoctetPairLE decodedPairLittleEndian;
  decodedPairBigEndian.decode(pairBigEndian.encode());
  decodedPairLittleEndian.decode(pairLittleEndian.encode());
  REQUIRE(decodedPairBigEndian.get() == pair);
  REQUIRE(decodedPairLittleEndian.get() == pair);

  auto const truncatedBytes = byteValues({0x12U});
  VariableLengthData truncated(truncatedBytes.data(), truncatedBytes.size());
  REQUIRE_THROWS_AS(decodedSignedBigEndian.decode(truncated), EncoderException);
  REQUIRE_THROWS_AS(decodedPairLittleEndian.decode(truncated), EncoderException);
}

TEST_CASE(
    "Official 32-bit little-endian basic encoding helpers use 1516.2 Table 29 wire forms",
    "[baseline][encoding][basic-data-elements][integer32][unit][foundation]") {
  using namespace rti1516_2025;

  HLAinteger32LE signedLittleEndian{-2};
  REQUIRE(octets(signedLittleEndian.encode()) == byteValues({0xfeU, 0xffU, 0xffU, 0xffU}));
  REQUIRE(signedLittleEndian.getEncodedLength() == 4U);
  REQUIRE(signedLittleEndian.getOctetBoundary() == 4U);
  HLAinteger32LE decodedSigned;
  decodedSigned.decode(signedLittleEndian.encode());
  REQUIRE(decodedSigned.get() == -2);

  HLAunsignedInteger32LE unsignedLittleEndian{0x1234abcdU};
  REQUIRE(octets(unsignedLittleEndian.encode()) ==
          byteValues({0xcdU, 0xabU, 0x34U, 0x12U}));
  HLAunsignedInteger32LE decodedUnsigned;
  decodedUnsigned.decode(unsignedLittleEndian.encode());
  REQUIRE(decodedUnsigned.get() == 0x1234abcdU);

  auto const truncatedBytes = byteValues({0xcdU, 0xabU, 0x34U});
  VariableLengthData truncated(truncatedBytes.data(), truncatedBytes.size());
  REQUIRE_THROWS_AS(decodedSigned.decode(truncated), EncoderException);
}

TEST_CASE(
    "Official 64-bit basic encoding helpers use 1516.2 Table 29 wire forms",
    "[baseline][encoding][basic-data-elements][integer64][unit][foundation]") {
  using namespace rti1516_2025;

  HLAinteger64BE signedBigEndian{-2};
  HLAinteger64LE signedLittleEndian{-2};
  REQUIRE(octets(signedBigEndian.encode()) ==
          byteValues({0xffU, 0xffU, 0xffU, 0xffU, 0xffU, 0xffU, 0xffU, 0xfeU}));
  REQUIRE(octets(signedLittleEndian.encode()) ==
          byteValues({0xfeU, 0xffU, 0xffU, 0xffU, 0xffU, 0xffU, 0xffU, 0xffU}));
  REQUIRE(signedBigEndian.getEncodedLength() == 8U);
  REQUIRE(signedBigEndian.getOctetBoundary() == 8U);
  HLAinteger64BE decodedSignedBigEndian;
  HLAinteger64LE decodedSignedLittleEndian;
  decodedSignedBigEndian.decode(signedBigEndian.encode());
  decodedSignedLittleEndian.decode(signedLittleEndian.encode());
  REQUIRE(decodedSignedBigEndian.get() == -2);
  REQUIRE(decodedSignedLittleEndian.get() == -2);

  HLAunsignedInteger64BE unsignedBigEndian{0x0123456789abcdefULL};
  HLAunsignedInteger64LE unsignedLittleEndian{0x0123456789abcdefULL};
  REQUIRE(octets(unsignedBigEndian.encode()) ==
          byteValues({0x01U, 0x23U, 0x45U, 0x67U, 0x89U, 0xabU, 0xcdU, 0xefU}));
  REQUIRE(octets(unsignedLittleEndian.encode()) ==
          byteValues({0xefU, 0xcdU, 0xabU, 0x89U, 0x67U, 0x45U, 0x23U, 0x01U}));
  HLAunsignedInteger64BE decodedUnsignedBigEndian;
  HLAunsignedInteger64LE decodedUnsignedLittleEndian;
  decodedUnsignedBigEndian.decode(unsignedBigEndian.encode());
  decodedUnsignedLittleEndian.decode(unsignedLittleEndian.encode());
  REQUIRE(decodedUnsignedBigEndian.get() == 0x0123456789abcdefULL);
  REQUIRE(decodedUnsignedLittleEndian.get() == 0x0123456789abcdefULL);

  auto const truncatedBytes =
      byteValues({0x01U, 0x23U, 0x45U, 0x67U, 0x89U, 0xabU, 0xcdU});
  VariableLengthData truncated(truncatedBytes.data(), truncatedBytes.size());
  REQUIRE_THROWS_AS(decodedSignedBigEndian.decode(truncated), EncoderException);
  REQUIRE_THROWS_AS(decodedUnsignedLittleEndian.decode(truncated), EncoderException);
}

TEST_CASE("Official HLAunicodeString uses a UTF-16BE element-count payload", "[baseline][encoding][unit][foundation]") {
  using namespace rti1516_2025;

  HLAunicodeString value{L"A\U0001f600"};
  REQUIRE(octets(value.encode()) == byteValues({
      0U, 0U, 0U, 3U,
      0U, 0x41U,
      0xd8U, 0x3dU,
      0xdeU, 0U,
  }));

  HLAunicodeString decoded;
  decoded.decode(value.encode());
  REQUIRE(decoded.get() == L"A\U0001f600");

  auto const malformedBytes = byteValues({0U, 0U, 0U, 2U, 0xd8U, 0U});
  VariableLengthData malformed(malformedBytes.data(), malformedBytes.size());
  REQUIRE_THROWS_AS(decoded.decode(malformed), EncoderException);

  auto const legacyByteCount = byteValues({
      0U, 0U, 0U, 6U,
      0U, 0x41U,
      0xd8U, 0x3dU,
      0xdeU, 0U,
  });
  VariableLengthData wrongCount(legacyByteCount.data(), legacyByteCount.size());
  REQUIRE_THROWS_AS(decoded.decode(wrongCount), EncoderException);

  HLAunicodeString unmatchedHighSurrogate{
      std::wstring(1U, static_cast<wchar_t>(0xd800U))};
  REQUIRE_THROWS_AS(unmatchedHighSurrogate.encode(), EncoderException);
}

TEST_CASE(
    "Official HLAopaqueData uses an HLAbyte HLAvariableArray payload",
    "[baseline][encoding][opaque-data][predefined-data][unit][foundation]") {
  using namespace rti1516_2025;

  auto const payload = byteValues({0xa5U, 0U, 0x5aU});
  HLAopaqueData value{payload.data(), payload.size()};
  REQUIRE(octets(value.encode()) == byteValues({0U, 0U, 0U, 3U, 0xa5U, 0U, 0x5aU}));
  REQUIRE(value.getEncodedLength() == 7U);
  REQUIRE(value.getOctetBoundary() == 4U);
  REQUIRE(value.bufferLength() == 3U);
  REQUIRE(value.dataLength() == 3U);
  REQUIRE(std::vector<Octet>(value.get(), value.get() + value.dataLength()) == payload);

  HLAopaqueData decoded;
  decoded.decode(value.encode());
  REQUIRE(decoded.dataLength() == payload.size());
  REQUIRE(std::vector<Octet>(decoded.get(), decoded.get() + decoded.dataLength()) == payload);

  auto const nested = byteValues({0x7eU, 0U, 0U, 0U, 2U, 0x12U, 0x34U, 0x7fU});
  REQUIRE(decoded.decodeFrom(nested, 1U) == 7U);
  REQUIRE(decoded.dataLength() == 2U);
  REQUIRE(std::vector<Octet>(decoded.get(), decoded.get() + decoded.dataLength()) ==
          byteValues({0x12U, 0x34U}));

  auto const truncatedCount = byteValues({0U, 0U, 0U});
  VariableLengthData truncatedCountValue(truncatedCount.data(), truncatedCount.size());
  REQUIRE_THROWS_AS(decoded.decode(truncatedCountValue), EncoderException);

  auto const truncatedPayload = byteValues({0U, 0U, 0U, 2U, 0x12U});
  VariableLengthData truncatedPayloadValue(truncatedPayload.data(), truncatedPayload.size());
  REQUIRE_THROWS_AS(decoded.decode(truncatedPayloadValue), EncoderException);

  auto const invalidSignedCount = byteValues({0x80U, 0U, 0U, 0U});
  VariableLengthData invalidSignedCountValue(invalidSignedCount.data(), invalidSignedCount.size());
  REQUIRE_THROWS_AS(decoded.decode(invalidSignedCountValue), EncoderException);

  auto const trailingData = byteValues({0U, 0U, 0U, 1U, 0x42U, 0U});
  VariableLengthData trailingDataValue(trailingData.data(), trailingData.size());
  REQUIRE_THROWS_AS(decoded.decode(trailingDataValue), EncoderException);
}

TEST_CASE(
    "Official HLAopaqueData retains caller-owned external storage without inventing ownership",
    "[baseline][encoding][opaque-data][external-storage][unit][foundation]") {
  using namespace rti1516_2025;

  std::array<Octet, 6U> firstStorage{
      static_cast<Octet>(0x10U),
      static_cast<Octet>(0x20U),
      static_cast<Octet>(0x30U),
      static_cast<Octet>(0x40U),
      static_cast<Octet>(0x50U),
      static_cast<Octet>(0x60U),
  };
  Octet* firstPointer = firstStorage.data();
  HLAopaqueData value{&firstPointer, firstStorage.size(), 3U};
  REQUIRE(value.bufferLength() == firstStorage.size());
  REQUIRE(value.dataLength() == 3U);
  REQUIRE(octets(value.encode()) == byteValues({0U, 0U, 0U, 3U, 0x10U, 0x20U, 0x30U}));

  firstStorage[1U] = static_cast<Octet>(0x99U);
  REQUIRE(octets(value.encode()) == byteValues({0U, 0U, 0U, 3U, 0x10U, 0x99U, 0x30U}));

  auto const replacement = byteValues({0xa5U, 0xb6U, 0xc7U, 0xd8U});
  value.set(replacement.data(), replacement.size());
  REQUIRE(value.dataLength() == replacement.size());
  REQUIRE(std::vector<Octet>(firstStorage.begin(), firstStorage.begin() + 4U) == replacement);

  auto const decodedPayload = byteValues({0U, 0U, 0U, 2U, 0x55U, 0x66U});
  VariableLengthData decodedPayloadValue(decodedPayload.data(), decodedPayload.size());
  value.decode(decodedPayloadValue);
  REQUIRE(value.dataLength() == 2U);
  REQUIRE(firstStorage[0U] == static_cast<Octet>(0x55U));
  REQUIRE(firstStorage[1U] == static_cast<Octet>(0x66U));

  HLAopaqueData copied{value};
  firstStorage[0U] = static_cast<Octet>(0xeeU);
  REQUIRE(octets(copied.encode()) == byteValues({0U, 0U, 0U, 2U, 0x55U, 0x66U}));

  auto const tooLarge = byteValues({0U, 1U, 2U, 3U, 4U, 5U, 6U});
  REQUIRE_THROWS_AS(value.set(tooLarge.data(), tooLarge.size()), EncoderException);
  REQUIRE(value.dataLength() == 2U);
  REQUIRE(firstStorage[0U] == static_cast<Octet>(0xeeU));

  std::array<Octet, 2U> secondStorage{
      static_cast<Octet>(0x7aU),
      static_cast<Octet>(0x7bU),
  };
  Octet* secondPointer = secondStorage.data();
  value.setDataPointer(&secondPointer, secondStorage.size(), 1U);
  REQUIRE(value.bufferLength() == secondStorage.size());
  REQUIRE(octets(value.encode()) == byteValues({0U, 0U, 0U, 1U, 0x7aU}));

  Octet* nullPointer = nullptr;
  REQUIRE_THROWS_AS(HLAopaqueData(&nullPointer, 1U, 0U), EncoderException);
  REQUIRE_THROWS_AS(HLAopaqueData(&secondPointer, 0U, 0U), EncoderException);
  REQUIRE_THROWS_AS(HLAopaqueData(&secondPointer, 1U, 2U), EncoderException);

  HLAopaqueData empty;
  empty.set(nullptr, 0U);
  REQUIRE(octets(empty.encode()) == byteValues({0U, 0U, 0U, 0U}));
  REQUIRE_THROWS_AS(empty.set(nullptr, 1U), EncoderException);
}

TEST_CASE(
    "Official HLAfixedRecord uses declaration order and zero padding for the next field",
    "[baseline][encoding][fixed-record][constructed-data][unit][foundation]") {
  using namespace rti1516_2025;

  HLAoctet first{static_cast<Octet>(0xa5U)};
  HLAboolean second{true};
  HLAfloat64BE third{-2.5};
  HLAfixedRecord record;
  record.appendElement(first).appendElement(second).appendElement(third);

  auto const expected = byteValues({
      0xa5U,
      0U, 0U, 0U,
      0U, 0U, 0U, 1U,
      0xc0U, 0x04U, 0U, 0U, 0U, 0U, 0U, 0U,
  });
  REQUIRE(octets(record.encode()) == expected);
  REQUIRE(record.getEncodedLength() == expected.size());
  REQUIRE(record.getOctetBoundary() == 8U);
  REQUIRE(record.size() == 3U);

  auto nested = byteValues({0x7eU});
  record.encodeInto(nested);
  REQUIRE(nested == [&] {
    auto result = byteValues({0x7eU});
    result.insert(result.end(), expected.begin(), expected.end());
    return result;
  }());

  HLAfixedRecord decoded;
  decoded.appendElement(HLAoctet{}).appendElement(HLAboolean{}).appendElement(HLAfloat64BE{});
  REQUIRE(decoded.decodeFrom(nested, 1U) == nested.size());
  REQUIRE(dynamic_cast<HLAoctet const&>(decoded.get(0U)).get() == static_cast<Octet>(0xa5U));
  REQUIRE(dynamic_cast<HLAboolean const&>(decoded.get(1U)).get());
  REQUIRE(dynamic_cast<HLAfloat64BE const&>(decoded.get(2U)).get() == -2.5);

  auto nonzeroPadding = expected;
  nonzeroPadding[1U] = static_cast<Octet>(1U);
  VariableLengthData nonzeroPaddingValue(nonzeroPadding.data(), nonzeroPadding.size());
  REQUIRE_THROWS_AS(decoded.decode(nonzeroPaddingValue), EncoderException);

  auto truncatedPadding = byteValues({0xa5U, 0U, 0U});
  VariableLengthData truncatedPaddingValue(truncatedPadding.data(), truncatedPadding.size());
  REQUIRE_THROWS_AS(decoded.decode(truncatedPaddingValue), EncoderException);

  auto trailingData = expected;
  trailingData.push_back(static_cast<Octet>(0));
  VariableLengthData trailingDataValue(trailingData.data(), trailingData.size());
  REQUIRE_THROWS_AS(decoded.decode(trailingDataValue), EncoderException);

  HLAfixedRecord reverse;
  reverse.appendElement(third).appendElement(second).appendElement(first);
  REQUIRE(octets(reverse.encode()) == byteValues({
      0xc0U, 0x04U, 0U, 0U, 0U, 0U, 0U, 0U,
      0U, 0U, 0U, 1U,
      0xa5U,
  }));
  REQUIRE(reverse.getEncodedLength() == 13U);
}

TEST_CASE(
    "Official HLAfixedRecord copies or borrows elements through its declared operations",
    "[baseline][encoding][fixed-record][element-lifetime][unit][foundation]") {
  using namespace rti1516_2025;

  HLAoctet source{static_cast<Octet>(0x11U)};
  HLAfixedRecord copiedElement;
  copiedElement.appendElement(source);
  source.set(static_cast<Octet>(0x22U));
  REQUIRE(octets(copiedElement.encode()) == byteValues({0x11U}));

  HLAinteger32BE borrowed{1};
  HLAfixedRecord borrowedElement;
  borrowedElement.appendElementPointer(&borrowed);
  borrowed.set(2);
  REQUIRE(octets(borrowedElement.encode()) == byteValues({0U, 0U, 0U, 2U}));

  HLAinteger32BE replacement{3};
  borrowedElement.set(0U, replacement);
  REQUIRE(borrowed.get() == 3);

  HLAinteger32BE substitute{4};
  borrowedElement.setElementPointer(0U, &substitute);
  substitute.set(5);
  REQUIRE(octets(borrowedElement.encode()) == byteValues({0U, 0U, 0U, 5U}));

  HLAfixedRecord copiedRecord{borrowedElement};
  substitute.set(6);
  REQUIRE(octets(copiedRecord.encode()) == byteValues({0U, 0U, 0U, 5U}));

  HLAfixedRecord sameShape;
  sameShape.appendElement(HLAinteger32BE{});
  HLAfixedRecord differentShape;
  differentShape.appendElement(HLAoctet{});
  REQUIRE(borrowedElement.isSameTypeAs(sameShape));
  REQUIRE_FALSE(borrowedElement.isSameTypeAs(differentShape));
  REQUIRE(borrowedElement.hasElementSameTypeAs(0U, replacement));
  REQUIRE_FALSE(borrowedElement.hasElementSameTypeAs(1U, replacement));

  HLAboolean wrongType{false};
  REQUIRE_THROWS_AS(borrowedElement.set(0U, wrongType), EncoderException);
  REQUIRE_THROWS_AS(borrowedElement.setElementPointer(0U, &wrongType), EncoderException);
  REQUIRE_THROWS_AS(borrowedElement.appendElementPointer(nullptr), EncoderException);
  REQUIRE_THROWS_AS(borrowedElement.get(1U), EncoderException);
}

TEST_CASE(
    "Official HLAfixedArray uses fixed cardinality and zero inter-element padding",
    "[baseline][encoding][fixed-array][constructed-data][unit][foundation]") {
  using namespace rti1516_2025;

  HLAfixedRecord prototype;
  prototype.appendElement(HLAinteger32BE{}).appendElement(HLAoctet{});
  HLAfixedArray array{prototype, 2U};

  HLAfixedRecord first;
  first.appendElement(HLAinteger32BE{0x10203040}).appendElement(HLAoctet{static_cast<Octet>(0xa5U)});
  HLAfixedRecord second;
  second.appendElement(HLAinteger32BE{-2}).appendElement(HLAoctet{static_cast<Octet>(0x5aU)});
  array.set(0U, first).set(1U, second);

  auto const expected = byteValues({
      0x10U, 0x20U, 0x30U, 0x40U, 0xa5U,
      0U, 0U, 0U,
      0xffU, 0xffU, 0xffU, 0xfeU, 0x5aU,
  });
  REQUIRE(octets(array.encode()) == expected);
  REQUIRE(array.getEncodedLength() == expected.size());
  REQUIRE(array.getOctetBoundary() == 4U);
  REQUIRE(array.size() == 2U);

  auto nested = byteValues({0x7eU});
  array.encodeInto(nested);
  REQUIRE(nested == [&] {
    auto result = byteValues({0x7eU});
    result.insert(result.end(), expected.begin(), expected.end());
    return result;
  }());

  HLAfixedArray decoded{prototype, 2U};
  REQUIRE(decoded.decodeFrom(nested, 1U) == nested.size());
  auto const& decodedFirst = dynamic_cast<HLAfixedRecord const&>(decoded.get(0U));
  auto const& decodedSecond = dynamic_cast<HLAfixedRecord const&>(decoded.get(1U));
  REQUIRE(dynamic_cast<HLAinteger32BE const&>(decodedFirst.get(0U)).get() == 0x10203040);
  REQUIRE(dynamic_cast<HLAoctet const&>(decodedFirst.get(1U)).get() == static_cast<Octet>(0xa5U));
  REQUIRE(dynamic_cast<HLAinteger32BE const&>(decodedSecond.get(0U)).get() == -2);
  REQUIRE(dynamic_cast<HLAoctet const&>(decodedSecond.get(1U)).get() == static_cast<Octet>(0x5aU));

  auto nonzeroPadding = expected;
  nonzeroPadding[5U] = static_cast<Octet>(1U);
  VariableLengthData nonzeroPaddingValue(nonzeroPadding.data(), nonzeroPadding.size());
  REQUIRE_THROWS_AS(decoded.decode(nonzeroPaddingValue), EncoderException);

  auto const truncatedPadding = byteValues({0x10U, 0x20U, 0x30U, 0x40U, 0xa5U, 0U, 0U});
  VariableLengthData truncatedPaddingValue(truncatedPadding.data(), truncatedPadding.size());
  REQUIRE_THROWS_AS(decoded.decode(truncatedPaddingValue), EncoderException);

  auto trailingData = expected;
  trailingData.push_back(static_cast<Octet>(0));
  VariableLengthData trailingDataValue(trailingData.data(), trailingData.size());
  REQUIRE_THROWS_AS(decoded.decode(trailingDataValue), EncoderException);

  HLAfixedArrayT<HLAinteger16BE> typed{3U};
  typed.set(0U, HLAinteger16BE{1}).set(1U, HLAinteger16BE{2}).set(2U, HLAinteger16BE{3});
  REQUIRE(octets(typed.encode()) == byteValues({0U, 1U, 0U, 2U, 0U, 3U}));
}

TEST_CASE(
    "Official HLAfixedArray clones its prototype and borrows caller pointer elements",
    "[baseline][encoding][fixed-array][element-lifetime][unit][foundation]") {
  using namespace rti1516_2025;

  HLAinteger32BE prototype{9};
  HLAfixedArray value{prototype, 2U};
  REQUIRE(dynamic_cast<HLAinteger32BE const&>(value.get(1U)).get() == 9);
  REQUIRE(octets(value.encode()) == byteValues({0U, 0U, 0U, 9U, 0U, 0U, 0U, 9U}));

  HLAinteger32BE borrowed{1};
  value.setElementPointer(0U, &borrowed);
  borrowed.set(2);
  REQUIRE(octets(value.encode()) == byteValues({0U, 0U, 0U, 2U, 0U, 0U, 0U, 9U}));

  HLAinteger32BE replacement{3};
  value.set(0U, replacement);
  REQUIRE(borrowed.get() == 3);

  HLAinteger32BE substitute{4};
  value.setElementPointer(0U, &substitute);
  substitute.set(5);
  REQUIRE(octets(value.encode()) == byteValues({0U, 0U, 0U, 5U, 0U, 0U, 0U, 9U}));

  HLAfixedArray copied{value};
  substitute.set(6);
  REQUIRE(octets(copied.encode()) == byteValues({0U, 0U, 0U, 5U, 0U, 0U, 0U, 9U}));

  HLAfixedArray sameShape{HLAinteger32BE{}, 2U};
  HLAfixedArray differentLength{HLAinteger32BE{}, 1U};
  HLAfixedArray differentPrototype{HLAoctet{}, 2U};
  REQUIRE(value.isSameTypeAs(sameShape));
  REQUIRE_FALSE(value.isSameTypeAs(differentLength));
  REQUIRE_FALSE(value.isSameTypeAs(differentPrototype));
  REQUIRE(value.hasPrototypeSameTypeAs(replacement));
  REQUIRE_FALSE(value.hasPrototypeSameTypeAs(HLAoctet{}));

  HLAfixedArrayT<HLAinteger32BE> typed{2U};
  HLAfixedArrayT<HLAinteger32BE> sameTyped{2U};
  HLAfixedArrayT<HLAinteger16BE> differentTyped{2U};
  REQUIRE(typed.isSameTypeAs(sameTyped));
  REQUIRE_FALSE(typed.isSameTypeAs(differentTyped));

  HLAoctet wrongType{static_cast<Octet>(1U)};
  REQUIRE_THROWS_AS(value.set(0U, wrongType), EncoderException);
  REQUIRE_THROWS_AS(value.setElementPointer(0U, &wrongType), EncoderException);
  REQUIRE_THROWS_AS(value.setElementPointer(0U, nullptr), EncoderException);
  REQUIRE_THROWS_AS(value.get(2U), EncoderException);
}

TEST_CASE(
    "Official HLAvariableArray uses a signed element count and zero padding to align elements",
    "[baseline][encoding][variable-array][constructed-data][unit][foundation]") {
  using namespace rti1516_2025;

  HLAvariableArray values{HLAfloat64BE{}};
  values.addElement(HLAfloat64BE{-2.5});
  auto const expected = byteValues({
      0U, 0U, 0U, 1U,
      0U, 0U, 0U, 0U,
      0xc0U, 0x04U, 0U, 0U, 0U, 0U, 0U, 0U,
  });
  REQUIRE(octets(values.encode()) == expected);
  REQUIRE(values.getEncodedLength() == expected.size());
  REQUIRE(values.getOctetBoundary() == 8U);
  REQUIRE(values.size() == 1U);

  auto nested = byteValues({0x7eU});
  values.encodeInto(nested);
  REQUIRE(nested == [&] {
    auto result = byteValues({0x7eU});
    result.insert(result.end(), expected.begin(), expected.end());
    return result;
  }());

  HLAvariableArray decoded{HLAfloat64BE{}};
  REQUIRE(decoded.decodeFrom(nested, 1U) == nested.size());
  REQUIRE(decoded.size() == 1U);
  REQUIRE(dynamic_cast<HLAfloat64BE const&>(decoded.get(0U)).get() == -2.5);

  auto nonzeroLeadingPadding = expected;
  nonzeroLeadingPadding[4U] = static_cast<Octet>(1U);
  VariableLengthData nonzeroLeadingPaddingValue(
      nonzeroLeadingPadding.data(), nonzeroLeadingPadding.size());
  REQUIRE_THROWS_AS(decoded.decode(nonzeroLeadingPaddingValue), EncoderException);

  auto const truncatedLeadingPadding = byteValues({0U, 0U, 0U, 1U, 0U, 0U, 0U});
  VariableLengthData truncatedLeadingPaddingValue(
      truncatedLeadingPadding.data(), truncatedLeadingPadding.size());
  REQUIRE_THROWS_AS(decoded.decode(truncatedLeadingPaddingValue), EncoderException);

  auto const negativeCount = byteValues({0x80U, 0U, 0U, 0U});
  VariableLengthData negativeCountValue(negativeCount.data(), negativeCount.size());
  REQUIRE_THROWS_AS(decoded.decode(negativeCountValue), EncoderException);

  auto trailingData = expected;
  trailingData.push_back(static_cast<Octet>(0));
  VariableLengthData trailingDataValue(trailingData.data(), trailingData.size());
  REQUIRE_THROWS_AS(decoded.decode(trailingDataValue), EncoderException);

  HLAvariableArray empty{HLAfloat64BE{}};
  REQUIRE(octets(empty.encode()) == byteValues({0U, 0U, 0U, 0U}));
  REQUIRE(empty.getEncodedLength() == 4U);
}

TEST_CASE(
    "Official HLAvariableArray copies or borrows element instances through its declared operations",
    "[baseline][encoding][variable-array][element-lifetime][unit][foundation]") {
  using namespace rti1516_2025;

  HLAfixedRecord recordPrototype;
  recordPrototype.appendElement(HLAinteger32BE{}).appendElement(HLAoctet{});
  HLAvariableArray records{recordPrototype};
  HLAfixedRecord first;
  first.appendElement(HLAinteger32BE{1}).appendElement(HLAoctet{static_cast<Octet>(0x11U)});
  HLAfixedRecord second;
  second.appendElement(HLAinteger32BE{2}).appendElement(HLAoctet{static_cast<Octet>(0x22U)});
  records.addElement(first).addElement(second);
  auto const paddedRecords = byteValues({
      0U, 0U, 0U, 2U,
      0U, 0U, 0U, 1U, 0x11U,
      0U, 0U, 0U,
      0U, 0U, 0U, 2U, 0x22U,
  });
  REQUIRE(octets(records.encode()) == paddedRecords);
  REQUIRE(records.getEncodedLength() == paddedRecords.size());

  auto nonzeroElementPadding = paddedRecords;
  nonzeroElementPadding[9U] = static_cast<Octet>(1U);
  VariableLengthData nonzeroElementPaddingValue(
      nonzeroElementPadding.data(), nonzeroElementPadding.size());
  REQUIRE_THROWS_AS(records.decode(nonzeroElementPaddingValue), EncoderException);

  HLAinteger32BE copiedSource{1};
  HLAvariableArray value{HLAinteger32BE{}};
  value.addElement(copiedSource);
  copiedSource.set(2);
  REQUIRE(octets(value.encode()) == byteValues({0U, 0U, 0U, 1U, 0U, 0U, 0U, 1U}));

  HLAinteger32BE borrowed{3};
  value.addElementPointer(&borrowed);
  borrowed.set(4);
  REQUIRE(octets(value.encode()) ==
          byteValues({0U, 0U, 0U, 2U, 0U, 0U, 0U, 1U, 0U, 0U, 0U, 4U}));

  HLAinteger32BE replacement{5};
  value.set(1U, replacement);
  REQUIRE(borrowed.get() == 5);

  HLAinteger32BE substitute{6};
  value.setElementPointer(1U, &substitute);
  substitute.set(7);
  HLAvariableArray copied{value};
  substitute.set(8);
  REQUIRE(octets(copied.encode()) ==
          byteValues({0U, 0U, 0U, 2U, 0U, 0U, 0U, 1U, 0U, 0U, 0U, 7U}));

  HLAvariableArray samePrototype{HLAinteger32BE{}};
  HLAvariableArray differentPrototype{HLAoctet{}};
  REQUIRE(value.isSameTypeAs(samePrototype));
  REQUIRE_FALSE(value.isSameTypeAs(differentPrototype));
  REQUIRE(value.hasPrototypeSameTypeAs(replacement));
  REQUIRE_FALSE(value.hasPrototypeSameTypeAs(HLAoctet{}));

  HLAvariableArrayT<HLAinteger16BE> typed;
  typed.addElement(HLAinteger16BE{1}).addElement(HLAinteger16BE{2});
  REQUIRE(octets(typed.encode()) == byteValues({0U, 0U, 0U, 2U, 0U, 1U, 0U, 2U}));
  HLAvariableArrayT<HLAinteger16BE> sameTyped;
  HLAvariableArrayT<HLAinteger32BE> differentTyped;
  REQUIRE(typed.isSameTypeAs(sameTyped));
  REQUIRE_FALSE(typed.isSameTypeAs(differentTyped));

  HLAoctet wrongType{static_cast<Octet>(1U)};
  REQUIRE_THROWS_AS(value.addElement(wrongType), EncoderException);
  REQUIRE_THROWS_AS(value.addElementPointer(nullptr), EncoderException);
  REQUIRE_THROWS_AS(value.set(0U, wrongType), EncoderException);
  REQUIRE_THROWS_AS(value.setElementPointer(0U, &wrongType), EncoderException);
  REQUIRE_THROWS_AS(value.get(2U), EncoderException);

  auto const oneElement = byteValues({0U, 0U, 0U, 1U, 0U, 0U, 0U, 9U});
  VariableLengthData oneElementValue(oneElement.data(), oneElement.size());
  value.decode(oneElementValue);
  REQUIRE(value.size() == 1U);
  REQUIRE(dynamic_cast<HLAinteger32BE const&>(value.get(0U)).get() == 9);
}

TEST_CASE(
    "Official HLAvariantRecord encodes mapped and unmapped discriminants with required alignment",
    "[baseline][encoding][variant-record][constructed-data][unit][foundation]") {
  using namespace rti1516_2025;

  HLAvariantRecord value{HLAoctet{}};
  value.addVariant(HLAoctet{static_cast<Octet>(1U)}, HLAinteger32BE{});
  value.addVariant(HLAoctet{static_cast<Octet>(2U)}, HLAfloat64BE{});
  value.setVariant(
      HLAoctet{static_cast<Octet>(1U)},
      HLAinteger32BE{0x10203040});

  // The mapped integer has a four-octet boundary, but the alternate float has
  // an eight-octet boundary.  IEEE 1516.2 §4.14.10.2 aligns this alternative
  // with the maximum boundary of all alternatives, so seven zero bytes follow
  // the one-octet discriminant.
  auto const expected = byteValues({
      1U,
      0U, 0U, 0U, 0U, 0U, 0U, 0U,
      0x10U, 0x20U, 0x30U, 0x40U,
  });
  REQUIRE(octets(value.encode()) == expected);
  REQUIRE(value.getEncodedLength() == expected.size());
  REQUIRE(value.getOctetBoundary() == 8U);

  auto nested = byteValues({0x7eU});
  value.encodeInto(nested);
  REQUIRE(nested == [&] {
    auto result = byteValues({0x7eU});
    result.insert(result.end(), expected.begin(), expected.end());
    return result;
  }());

  HLAvariantRecord decoded{HLAoctet{}};
  decoded.addVariant(HLAoctet{static_cast<Octet>(1U)}, HLAinteger32BE{});
  decoded.addVariant(HLAoctet{static_cast<Octet>(2U)}, HLAfloat64BE{});
  REQUIRE(decoded.decodeFrom(nested, 1U) == nested.size());
  REQUIRE(dynamic_cast<HLAoctet const&>(decoded.getDiscriminant()).get() == static_cast<Octet>(1U));
  REQUIRE(dynamic_cast<HLAinteger32BE const&>(decoded.getVariant()).get() == 0x10203040);

  auto nonzeroPadding = expected;
  nonzeroPadding[1U] = static_cast<Octet>(1U);
  VariableLengthData nonzeroPaddingValue(nonzeroPadding.data(), nonzeroPadding.size());
  REQUIRE_THROWS_AS(decoded.decode(nonzeroPaddingValue), EncoderException);

  auto const truncatedPadding = byteValues({1U, 0U, 0U, 0U, 0U, 0U, 0U});
  VariableLengthData truncatedPaddingValue(truncatedPadding.data(), truncatedPadding.size());
  REQUIRE_THROWS_AS(decoded.decode(truncatedPaddingValue), EncoderException);

  auto trailingData = expected;
  trailingData.push_back(static_cast<Octet>(0));
  VariableLengthData trailingDataValue(trailingData.data(), trailingData.size());
  REQUIRE_THROWS_AS(decoded.decode(trailingDataValue), EncoderException);

  value.setDiscriminant(HLAoctet{static_cast<Octet>(0x7fU)});
  REQUIRE(octets(value.encode()) == byteValues({0x7fU}));
  REQUIRE(value.getEncodedLength() == 1U);
  REQUIRE_THROWS_AS(value.getVariant(), EncoderException);

  auto const unknownDiscriminant = byteValues({0x7fU});
  VariableLengthData unknownDiscriminantValue(
      unknownDiscriminant.data(), unknownDiscriminant.size());
  decoded.decode(unknownDiscriminantValue);
  REQUIRE(dynamic_cast<HLAoctet const&>(decoded.getDiscriminant()).get() == static_cast<Octet>(0x7fU));
  REQUIRE_THROWS_AS(decoded.getVariant(), EncoderException);
}

TEST_CASE(
    "Official HLAvariantRecord copies mapping values and borrows caller pointer variants",
    "[baseline][encoding][variant-record][element-lifetime][unit][foundation]") {
  using namespace rti1516_2025;

  HLAvariantRecord value{HLAinteger32BE{}};
  HLAinteger32BE copiedSource{1};
  value.addVariant(HLAinteger32BE{1}, copiedSource);
  copiedSource.set(2);
  REQUIRE(octets(value.encode()) ==
          byteValues({0U, 0U, 0U, 1U, 0U, 0U, 0U, 1U}));

  HLAinteger32BE borrowed{3};
  value.addVariantPointer(HLAinteger32BE{2}, &borrowed);
  borrowed.set(4);
  REQUIRE(octets(value.encode()) ==
          byteValues({0U, 0U, 0U, 2U, 0U, 0U, 0U, 4U}));

  HLAinteger32BE replacement{5};
  value.setVariant(HLAinteger32BE{2}, replacement);
  REQUIRE(borrowed.get() == 5);

  HLAinteger32BE substitute{6};
  value.setVariantPointer(HLAinteger32BE{2}, &substitute);
  substitute.set(7);
  HLAvariantRecord copied{value};
  substitute.set(8);
  REQUIRE(octets(copied.encode()) ==
          byteValues({0U, 0U, 0U, 2U, 0U, 0U, 0U, 7U}));

  HLAvariantRecord sameShape{HLAinteger32BE{}};
  sameShape.addVariant(HLAinteger32BE{1}, HLAinteger32BE{});
  sameShape.addVariant(HLAinteger32BE{2}, HLAinteger32BE{});
  HLAvariantRecord differentVariant{HLAinteger32BE{}};
  differentVariant.addVariant(HLAinteger32BE{1}, HLAinteger32BE{});
  differentVariant.addVariant(HLAinteger32BE{2}, HLAoctet{});
  HLAvariantRecord differentDiscriminant{HLAoctet{}};
  differentDiscriminant.addVariant(HLAoctet{static_cast<Octet>(1U)}, HLAinteger32BE{});
  REQUIRE(value.isSameTypeAs(sameShape));
  REQUIRE_FALSE(value.isSameTypeAs(differentVariant));
  REQUIRE_FALSE(value.isSameTypeAs(differentDiscriminant));
  REQUIRE(value.isSameTypeAs(HLAinteger32BE{1}, HLAinteger32BE{}));
  REQUIRE_FALSE(value.isSameTypeAs(HLAinteger32BE{1}, HLAoctet{}));
  REQUIRE(value.hasMatchingDiscriminantTypeAs(HLAinteger32BE{}));
  REQUIRE_FALSE(value.hasMatchingDiscriminantTypeAs(HLAoctet{}));

  HLAvariantRecordT<HLAoctet> typed;
  typed.addVariant(HLAoctet{static_cast<Octet>(1U)}, HLAinteger16BE{});
  typed.setVariant(HLAoctet{static_cast<Octet>(1U)}, HLAinteger16BE{9});
  REQUIRE(octets(typed.encode()) == byteValues({1U, 0U, 0U, 9U}));
  HLAvariantRecordT<HLAoctet> sameTyped;
  HLAvariantRecordT<HLAinteger32BE> differentTyped;
  REQUIRE(typed.isSameTypeAs(sameTyped));
  REQUIRE_FALSE(typed.isSameTypeAs(differentTyped));

  HLAoctet wrongDiscriminant{static_cast<Octet>(1U)};
  HLAoctet wrongVariant{static_cast<Octet>(2U)};
  REQUIRE_THROWS_AS(value.addVariant(HLAinteger32BE{2}, HLAinteger32BE{}), EncoderException);
  REQUIRE_THROWS_AS(value.addVariant(wrongDiscriminant, HLAinteger32BE{}), EncoderException);
  REQUIRE_THROWS_AS(value.addVariantPointer(HLAinteger32BE{3}, nullptr), EncoderException);
  REQUIRE_THROWS_AS(value.setDiscriminant(wrongDiscriminant), EncoderException);
  REQUIRE_THROWS_AS(value.setVariant(HLAinteger32BE{3}, HLAinteger32BE{}), EncoderException);
  REQUIRE_THROWS_AS(value.setVariant(HLAinteger32BE{2}, wrongVariant), EncoderException);
  REQUIRE_THROWS_AS(value.setVariantPointer(HLAinteger32BE{2}, &wrongVariant), EncoderException);
  REQUIRE_THROWS_AS(value.setVariantPointer(HLAinteger32BE{2}, nullptr), EncoderException);
}

TEST_CASE(
    "Official HLAextendableVariantRecord uses its length field and predefined eight-octet alternative boundary",
    "[baseline][encoding][extendable-variant-record][constructed-data][unit][foundation]") {
  using namespace rti1516_2025;

  HLAfloat64BE selectedDiscriminant{1.5};
  HLAextendableVariantRecord value{HLAfloat64BE{}};
  value.addVariant(selectedDiscriminant, HLAinteger32BE{});
  value.setVariant(selectedDiscriminant, HLAinteger32BE{0x10203040});

  // The eight-octet discriminant is followed immediately by its four-byte
  // HLAinteger32BE encoded_length.  That leaves the alternative at offset 12,
  // so Equation (4) contributes four zero bytes to reach the predefined
  // eight-octet alternative boundary.  The encoded length remains four: it
  // deliberately excludes those four padding bytes.
  auto const expected = byteValues({
      0x3fU, 0xf8U, 0U, 0U, 0U, 0U, 0U, 0U,
      0U, 0U, 0U, 4U,
      0U, 0U, 0U, 0U,
      0x10U, 0x20U, 0x30U, 0x40U,
  });
  REQUIRE(octets(value.encode()) == expected);
  REQUIRE(value.getEncodedLength() == expected.size());
  REQUIRE(value.getOctetBoundary() == 8U);

  auto nested = byteValues({0x7eU});
  value.encodeInto(nested);
  REQUIRE(nested == [&] {
    auto result = byteValues({0x7eU});
    result.insert(result.end(), expected.begin(), expected.end());
    return result;
  }());

  HLAextendableVariantRecord decoded{HLAfloat64BE{}};
  decoded.addVariant(selectedDiscriminant, HLAinteger32BE{});
  REQUIRE(decoded.decodeFrom(nested, 1U) == nested.size());
  REQUIRE(dynamic_cast<HLAfloat64BE const&>(decoded.getDiscriminant()).get() == 1.5);
  REQUIRE(dynamic_cast<HLAinteger32BE const&>(decoded.getVariant()).get() == 0x10203040);

  auto nonzeroAlternativePadding = expected;
  nonzeroAlternativePadding[12U] = static_cast<Octet>(1U);
  VariableLengthData nonzeroAlternativePaddingValue(
      nonzeroAlternativePadding.data(), nonzeroAlternativePadding.size());
  REQUIRE_THROWS_AS(decoded.decode(nonzeroAlternativePaddingValue), EncoderException);

  auto declaredLengthTooShort = expected;
  declaredLengthTooShort[11U] = static_cast<Octet>(3U);
  VariableLengthData declaredLengthTooShortValue(
      declaredLengthTooShort.data(), declaredLengthTooShort.size());
  REQUIRE_THROWS_AS(decoded.decode(declaredLengthTooShortValue), EncoderException);

  auto truncatedAlternative = expected;
  truncatedAlternative.pop_back();
  VariableLengthData truncatedAlternativeValue(
      truncatedAlternative.data(), truncatedAlternative.size());
  REQUIRE_THROWS_AS(decoded.decode(truncatedAlternativeValue), EncoderException);

  auto trailingData = expected;
  trailingData.push_back(static_cast<Octet>(0));
  VariableLengthData trailingDataValue(trailingData.data(), trailingData.size());
  REQUIRE_THROWS_AS(decoded.decode(trailingDataValue), EncoderException);

  value.setDiscriminant(HLAfloat64BE{-2.5});
  REQUIRE(octets(value.encode()) == byteValues({
      0xc0U, 0x04U, 0U, 0U, 0U, 0U, 0U, 0U,
      0U, 0U, 0U, 0U,
      0U, 0U, 0U, 0U,
  }));
  REQUIRE(value.getEncodedLength() == 16U);
  REQUIRE_THROWS_AS(value.getVariant(), EncoderException);

  HLAextendableVariantRecord octetRecord{HLAoctet{}};
  octetRecord.addVariant(HLAoctet{static_cast<Octet>(1U)}, HLAfloat32LE{});
  octetRecord.setVariant(
      HLAoctet{static_cast<Octet>(1U)},
      HLAfloat32LE{1.0F});
  auto const octetExpected = byteValues({
      1U, 0U, 0U, 0U,
      0U, 0U, 0U, 4U,
      0U, 0U, 0x80U, 0x3fU,
  });
  REQUIRE(octets(octetRecord.encode()) == octetExpected);
  auto nonzeroLengthPadding = octetExpected;
  nonzeroLengthPadding[1U] = static_cast<Octet>(1U);
  VariableLengthData nonzeroLengthPaddingValue(
      nonzeroLengthPadding.data(), nonzeroLengthPadding.size());
  REQUIRE_THROWS_AS(octetRecord.decode(nonzeroLengthPaddingValue), EncoderException);

  HLAextendableVariantRecord unknownDecoded{HLAoctet{}};
  unknownDecoded.addVariant(HLAoctet{static_cast<Octet>(1U)}, HLAfloat32LE{});
  auto const unknownAlternative = byteValues({
      0x7fU, 0U, 0U, 0U,
      0U, 0U, 0U, 3U,
      0xaaU, 0xbbU, 0xccU,
  });
  VariableLengthData unknownAlternativeValue(
      unknownAlternative.data(), unknownAlternative.size());
  unknownDecoded.decode(unknownAlternativeValue);
  REQUIRE(dynamic_cast<HLAoctet const&>(unknownDecoded.getDiscriminant()).get() ==
          static_cast<Octet>(0x7fU));
  REQUIRE_THROWS_AS(unknownDecoded.getVariant(), EncoderException);
}

TEST_CASE(
    "Official HLAextendableVariantRecord copies mapping values and borrows caller pointer variants",
    "[baseline][encoding][extendable-variant-record][element-lifetime][unit][foundation]") {
  using namespace rti1516_2025;

  HLAextendableVariantRecord value{HLAinteger32BE{}};
  HLAinteger32BE copiedSource{1};
  value.addVariant(HLAinteger32BE{1}, copiedSource);
  copiedSource.set(2);
  REQUIRE(octets(value.encode()) ==
          byteValues({0U, 0U, 0U, 1U, 0U, 0U, 0U, 4U, 0U, 0U, 0U, 1U}));

  HLAinteger32BE borrowed{3};
  value.addVariantPointer(HLAinteger32BE{2}, &borrowed);
  borrowed.set(4);
  REQUIRE(octets(value.encode()) ==
          byteValues({0U, 0U, 0U, 2U, 0U, 0U, 0U, 4U, 0U, 0U, 0U, 4U}));

  HLAinteger32BE replacement{5};
  value.setVariant(HLAinteger32BE{2}, replacement);
  REQUIRE(borrowed.get() == 5);

  HLAinteger32BE substitute{6};
  value.setVariantPointer(HLAinteger32BE{2}, &substitute);
  substitute.set(7);
  HLAextendableVariantRecord copied{value};
  substitute.set(8);
  REQUIRE(octets(copied.encode()) ==
          byteValues({0U, 0U, 0U, 2U, 0U, 0U, 0U, 4U, 0U, 0U, 0U, 7U}));

  HLAextendableVariantRecord sameShape{HLAinteger32BE{}};
  sameShape.addVariant(HLAinteger32BE{1}, HLAinteger32BE{});
  sameShape.addVariant(HLAinteger32BE{2}, HLAinteger32BE{});
  HLAextendableVariantRecord differentVariant{HLAinteger32BE{}};
  differentVariant.addVariant(HLAinteger32BE{1}, HLAinteger32BE{});
  differentVariant.addVariant(HLAinteger32BE{2}, HLAoctet{});
  HLAextendableVariantRecord differentDiscriminant{HLAoctet{}};
  differentDiscriminant.addVariant(HLAoctet{static_cast<Octet>(1U)}, HLAinteger32BE{});
  REQUIRE(value.isSameTypeAs(sameShape));
  REQUIRE_FALSE(value.isSameTypeAs(differentVariant));
  REQUIRE_FALSE(value.isSameTypeAs(differentDiscriminant));
  REQUIRE(value.isSameTypeAs(HLAinteger32BE{1}, HLAinteger32BE{}));
  REQUIRE_FALSE(value.isSameTypeAs(HLAinteger32BE{1}, HLAoctet{}));
  REQUIRE(value.hasMatchingDiscriminantTypeAs(HLAinteger32BE{}));
  REQUIRE_FALSE(value.hasMatchingDiscriminantTypeAs(HLAoctet{}));

  HLAextendableVariantRecordT<HLAoctet> typed;
  typed.addVariant(HLAoctet{static_cast<Octet>(1U)}, HLAinteger16BE{});
  typed.setVariant(HLAoctet{static_cast<Octet>(1U)}, HLAinteger16BE{9});
  REQUIRE(octets(typed.encode()) ==
          byteValues({1U, 0U, 0U, 0U, 0U, 0U, 0U, 2U, 0U, 9U}));
  HLAextendableVariantRecordT<HLAoctet> sameTyped;
  HLAextendableVariantRecordT<HLAinteger32BE> differentTyped;
  REQUIRE(typed.isSameTypeAs(sameTyped));
  REQUIRE_FALSE(typed.isSameTypeAs(differentTyped));

  Boundary16Element boundary16;
  HLAextendableVariantRecord boundaryChecked{HLAoctet{}};
  REQUIRE_THROWS_AS(
      boundaryChecked.addVariant(HLAoctet{static_cast<Octet>(1U)}, boundary16),
      EncoderException);

  HLAoctet wrongDiscriminant{static_cast<Octet>(1U)};
  HLAoctet wrongVariant{static_cast<Octet>(2U)};
  REQUIRE_THROWS_AS(value.addVariant(HLAinteger32BE{2}, HLAinteger32BE{}), EncoderException);
  REQUIRE_THROWS_AS(value.addVariant(wrongDiscriminant, HLAinteger32BE{}), EncoderException);
  REQUIRE_THROWS_AS(value.addVariantPointer(HLAinteger32BE{3}, nullptr), EncoderException);
  REQUIRE_THROWS_AS(value.setDiscriminant(wrongDiscriminant), EncoderException);
  REQUIRE_THROWS_AS(value.setVariant(HLAinteger32BE{3}, HLAinteger32BE{}), EncoderException);
  REQUIRE_THROWS_AS(value.setVariant(HLAinteger32BE{2}, wrongVariant), EncoderException);
  REQUIRE_THROWS_AS(value.setVariantPointer(HLAinteger32BE{2}, &wrongVariant), EncoderException);
  REQUIRE_THROWS_AS(value.setVariantPointer(HLAinteger32BE{2}, nullptr), EncoderException);
}
