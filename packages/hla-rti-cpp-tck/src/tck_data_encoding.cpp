#include "tck_data_encoding.hpp"

#include <limits>

namespace hla_rti_cpp_tck::detail {

std::unique_ptr<rti::DataElement> Boundary16Element::clone() const {
  return std::make_unique<Boundary16Element>();
}

rti::VariableLengthData Boundary16Element::encode() const {
  rti::VariableLengthData value;
  encode(value);
  return value;
}

void Boundary16Element::encode(rti::VariableLengthData& value) const {
  std::vector<rti::Octet> bytes(
      16U,
      static_cast<rti::Octet>(0U));
  value.setData(bytes.data(), bytes.size());
}

void Boundary16Element::encodeInto(std::vector<rti::Octet>& bytes) const {
  bytes.insert(
      bytes.end(),
      16U,
      static_cast<rti::Octet>(0U));
}

Boundary16Element& Boundary16Element::decode(
    rti::VariableLengthData const& value) {
  if (value.size() != 16U) {
    throw rti::EncoderException(
        L"Boundary16Element has an invalid encoding length.");
  }
  return *this;
}

std::size_t Boundary16Element::decodeFrom(
    std::vector<rti::Octet> const& bytes,
    std::size_t index) {
  if (index > bytes.size() || bytes.size() - index < 16U) {
    throw rti::EncoderException(
        L"Boundary16Element encoding is truncated.");
  }
  return index + 16U;
}

std::size_t Boundary16Element::getEncodedLength() const {
  return 16U;
}

unsigned int Boundary16Element::getOctetBoundary() const {
  return 16U;
}

void verifyVariableLengthDataContract() {
  std::uint8_t bytes[] = {0x01U, 0x02U, 0x03U};
  rti::VariableLengthData data(bytes, sizeof(bytes));
  require(
      data.size() == sizeof(bytes) && copyBytes(data) == std::vector<std::uint8_t>{1U, 2U, 3U},
      "the standard VariableLengthData surface did not preserve its bytes");

  std::vector<rti::Octet> callerOwned{
      static_cast<rti::Octet>('H'),
      static_cast<rti::Octet>('L'),
      static_cast<rti::Octet>('A')};
  rti::VariableLengthData copiedData(callerOwned.data(), callerOwned.size());
  callerOwned[0] = static_cast<rti::Octet>('X');
  require(
      copyBytes(copiedData) == std::vector<std::uint8_t>{'H', 'L', 'A'},
      "VariableLengthData constructor did not copy caller-owned bytes");

  rti::VariableLengthData copiedAgain(copiedData);
  rti::VariableLengthData assigned;
  assigned = copiedData;
  std::vector<rti::Octet> replacement{
      static_cast<rti::Octet>('O'),
      static_cast<rti::Octet>('K')};
  copiedData.setData(replacement.data(), replacement.size());
  require(
      copyBytes(copiedAgain) == std::vector<std::uint8_t>{'H', 'L', 'A'} &&
          copyBytes(assigned) == std::vector<std::uint8_t>{'H', 'L', 'A'} &&
          copyBytes(copiedData) == std::vector<std::uint8_t>{'O', 'K'},
      "VariableLengthData copy and replacement semantics were not independent");

  rti::VariableLengthData borrowedData;
  borrowedData.setDataPointer(callerOwned.data(), callerOwned.size());
  rti::VariableLengthData borrowedCopy(borrowedData);
  callerOwned[1] = static_cast<rti::Octet>('E');
  callerOwned[2] = static_cast<rti::Octet>('W');
  require(
      copyBytes(borrowedData) == std::vector<std::uint8_t>{'X', 'E', 'W'} &&
          copyBytes(borrowedCopy) == std::vector<std::uint8_t>{'X', 'L', 'A'},
      "VariableLengthData borrowed-storage copy semantics were not preserved");

  adoptedDataDeleteCount = 0U;
  auto* adoptedBytes = new rti::Octet[3U]{
      static_cast<rti::Octet>('R'),
      static_cast<rti::Octet>('T'),
      static_cast<rti::Octet>('I')};
  {
    rti::VariableLengthData adoptedData;
    adoptedData.takeDataPointer(
        adoptedBytes,
        3U,
        &deleteAdoptedData);
    require(
        copyBytes(adoptedData) ==
            std::vector<std::uint8_t>{'R', 'T', 'I'},
        "VariableLengthData adopted storage did not preserve its bytes");
    rti::VariableLengthData adoptedCopy(adoptedData);
    std::vector<rti::Octet> adoptedReplacement{
        static_cast<rti::Octet>('!')};
    adoptedData.setData(
        adoptedReplacement.data(),
        adoptedReplacement.size());
    require(
        adoptedDataDeleteCount == 1U &&
            copyBytes(adoptedCopy) ==
                std::vector<std::uint8_t>{'R', 'T', 'I'} &&
            copyBytes(adoptedData) == std::vector<std::uint8_t>{'!'},
        "VariableLengthData adopted storage replacement semantics were not preserved");
  }
  require(
      adoptedDataDeleteCount == 1U,
      "VariableLengthData adopted storage was not released exactly once");

  rti::VariableLengthData emptyData;
  require(
      emptyData.size() == 0U,
      "VariableLengthData default construction did not produce an empty value");
  {
    auto* defaultAdoptedBytes = new rti::Octet[3U]{
        static_cast<rti::Octet>('D'),
        static_cast<rti::Octet>('E'),
        static_cast<rti::Octet>('F')};
    rti::VariableLengthData defaultAdoptedData;
    defaultAdoptedData.takeDataPointer(defaultAdoptedBytes, 3U);
    require(
        defaultAdoptedData.size() == 3U &&
            copyBytes(defaultAdoptedData) ==
                std::vector<std::uint8_t>{'D', 'E', 'F'},
        "VariableLengthData default adopted storage did not preserve its bytes");
  }
}


void verifyStandardDataElementContract() {
  Boundary16Element element;
  auto clone = element.clone();
  require(
      clone != nullptr &&
          dynamic_cast<Boundary16Element const*>(clone.get()) != nullptr,
      "standard DataElement clone did not preserve the dynamic type");
  require(
      element.isSameTypeAs(*clone) && clone->isSameTypeAs(element),
      "standard DataElement same-type comparison rejected an equivalent clone");
  rti::HLAinteger32BE differentType{0};
  require(
      !element.isSameTypeAs(differentType),
      "standard DataElement same-type comparison accepted a different type");

  auto encoded = element.encode();
  rti::VariableLengthData assignedEncoding;
  element.encode(assignedEncoding);
  require(
      copyBytes(encoded) == copyBytes(assignedEncoding) &&
          encoded.size() == 16U &&
          element.getEncodedLength() == 16U &&
          element.getOctetBoundary() == 16U &&
          element.hash() == clone->hash(),
      "standard DataElement encoding, length, boundary, or hash semantics were not preserved");

  std::vector<rti::Octet> appended{static_cast<rti::Octet>(0x7eU)};
  element.encodeInto(appended);
  require(
      appended.size() == 17U &&
          appended.front() == static_cast<rti::Octet>(0x7eU) &&
          std::all_of(
              appended.begin() + 1,
              appended.end(),
              [](rti::Octet value) { return value == static_cast<rti::Octet>(0U); }),
      "standard DataElement encodeInto did not append its complete encoding");

  Boundary16Element decoded;
  require(
      &decoded.decode(encoded) == &decoded,
      "standard DataElement decode did not return the target element");
  std::vector<rti::Octet> prefixed{static_cast<rti::Octet>(0x7eU)};
  element.encodeInto(prefixed);
  require(
      decoded.decodeFrom(prefixed, 1U) == prefixed.size(),
      "standard DataElement decodeFrom did not consume the appended encoding");
}

void verifyBasicDataElementsContract() {
  verifyBasicDataElementsIntegerContract();
  rti::HLAboolean trueValue{true};
  rti::HLAboolean falseValue{false};
  requireEncodedBytes(trueValue, {0U, 0U, 0U, 1U}, "HLAboolean true");
  requireEncodedBytes(falseValue, {0U, 0U, 0U, 0U}, "HLAboolean false");
  std::vector<rti::Octet> booleanRawBuffer{
      static_cast<rti::Octet>(0x7eU)};
  trueValue.encodeInto(booleanRawBuffer);
  std::vector<rti::Octet> booleanRawExpected{
      static_cast<rti::Octet>(0x7eU),
      static_cast<rti::Octet>(0U),
      static_cast<rti::Octet>(0U),
      static_cast<rti::Octet>(0U),
      static_cast<rti::Octet>(1U)};
  require(
      booleanRawBuffer == booleanRawExpected,
      "HLAboolean encodeInto did not append its four-byte true encoding");
  rti::HLAboolean rawDecodedBoolean;
  auto const rawBooleanCursor = rawDecodedBoolean.decodeFrom(booleanRawBuffer, 1U);
  require(
      rawBooleanCursor == booleanRawBuffer.size() && rawDecodedBoolean.get(),
      "HLAboolean decodeFrom did not decode the prefixed slice");
  rti::HLAboolean decodedBoolean;
  decodedBoolean.decode(trueValue.encode());
  require(decodedBoolean.get(), "HLAboolean did not round-trip");
  std::vector<std::uint8_t> invalidBooleanBytes{0U, 0U, 0U, 2U};
  rti::VariableLengthData invalidBoolean(
      invalidBooleanBytes.data(), invalidBooleanBytes.size());
  requireException(
      [&] { decodedBoolean.decode(invalidBoolean); },
      L"EncoderException",
      "invalid HLAboolean encoding");
  std::vector<rti::Octet> invalidBooleanRawBuffer{
      static_cast<rti::Octet>(0U), static_cast<rti::Octet>(0U),
      static_cast<rti::Octet>(0U), static_cast<rti::Octet>(2U)};
  rti::HLAboolean rawDecodedInvalidBoolean;
  requireException(
      [&] { rawDecodedInvalidBoolean.decodeFrom(invalidBooleanRawBuffer, 0U); },
      L"EncoderException",
      "invalid HLAboolean decodeFrom raw value");
  std::vector<rti::Octet> truncatedBooleanRawBuffer{
      static_cast<rti::Octet>(0U), static_cast<rti::Octet>(0U),
      static_cast<rti::Octet>(0U)};
  rti::HLAboolean truncatedDecodedBoolean;
  requireException(
      [&] { truncatedDecodedBoolean.decodeFrom(truncatedBooleanRawBuffer, 0U); },
      L"EncoderException",
      "truncated HLAboolean decodeFrom raw buffer");
  rti::HLAboolean outOfRangeDecodedBoolean;
  requireException(
      [&] {
        outOfRangeDecodedBoolean.decodeFrom(
            booleanRawBuffer, booleanRawBuffer.size() + 1U);
      },
      L"EncoderException",
      "out-of-range HLAboolean decodeFrom index");
  rti::HLAboolean endOfBufferDecodedBoolean;
  requireException(
      [&] {
        endOfBufferDecodedBoolean.decodeFrom(
            booleanRawBuffer, booleanRawBuffer.size());
      },
      L"EncoderException",
      "end-of-buffer HLAboolean decodeFrom index");

  rti::HLAoctet octet{static_cast<rti::Octet>(0xa5U)};
  rti::HLAbyte byte{static_cast<rti::Octet>(0xa5U)};
  requireEncodedBytes(octet, {0xa5U}, "HLAoctet");
  requireEncodedBytes(byte, {0xa5U}, "HLAbyte");
  std::vector<rti::Octet> octetRawBuffer{
      static_cast<rti::Octet>(0x7eU)};
  octet.encodeInto(octetRawBuffer);
  std::vector<rti::Octet> octetRawExpected{
      static_cast<rti::Octet>(0x7eU), static_cast<rti::Octet>(0xa5U)};
  require(
      octetRawBuffer == octetRawExpected,
      "HLAoctet encodeInto did not append its octet");
  rti::HLAoctet rawDecodedOctet;
  auto const rawOctetCursor = rawDecodedOctet.decodeFrom(octetRawBuffer, 1U);
  require(
      rawOctetCursor == octetRawBuffer.size() &&
          rawDecodedOctet.get() == static_cast<rti::Octet>(0xa5U),
      "HLAoctet decodeFrom did not decode the prefixed slice");
  rti::HLAoctet outOfRangeDecodedOctet;
  requireException(
      [&] {
        outOfRangeDecodedOctet.decodeFrom(
            octetRawBuffer, octetRawBuffer.size());
      },
      L"EncoderException",
      "out-of-range HLAoctet decodeFrom index");
  rti::HLAoctet beyondBufferDecodedOctet;
  requireException(
      [&] {
        beyondBufferDecodedOctet.decodeFrom(
            octetRawBuffer, octetRawBuffer.size() + 1U);
      },
      L"EncoderException",
      "beyond-buffer HLAoctet decodeFrom index");
  std::vector<rti::Octet> byteRawBuffer{
      static_cast<rti::Octet>(0x7eU)};
  byte.encodeInto(byteRawBuffer);
  std::vector<rti::Octet> byteRawExpected{
      static_cast<rti::Octet>(0x7eU), static_cast<rti::Octet>(0xa5U)};
  require(
      byteRawBuffer == byteRawExpected,
      "HLAbyte encodeInto did not append its octet");
  rti::HLAbyte rawDecodedByte;
  auto const rawByteCursor = rawDecodedByte.decodeFrom(byteRawBuffer, 1U);
  require(
      rawByteCursor == byteRawBuffer.size() &&
          rawDecodedByte.get() == static_cast<rti::Octet>(0xa5U),
      "HLAbyte decodeFrom did not decode the prefixed slice");
  rti::HLAbyte outOfRangeDecodedByte;
  requireException(
      [&] {
        outOfRangeDecodedByte.decodeFrom(
            byteRawBuffer, byteRawBuffer.size());
      },
      L"EncoderException",
      "out-of-range HLAbyte decodeFrom index");
  rti::HLAbyte beyondBufferDecodedByte;
  requireException(
      [&] {
        beyondBufferDecodedByte.decodeFrom(
            byteRawBuffer, byteRawBuffer.size() + 1U);
      },
      L"EncoderException",
      "beyond-buffer HLAbyte decodeFrom index");
  rti::HLAoctet decodedOctet;
  decodedOctet.decode(octet.encode());
  require(
      decodedOctet.get() == static_cast<rti::Octet>(0xa5U),
      "HLAoctet did not round-trip");

  rti::HLAfloat32BE float32Be{-2.5F};
  rti::HLAfloat32LE float32Le{-2.5F};
  rti::HLAfloat64BE float64Be{-2.5};
  rti::HLAfloat64LE float64Le{-2.5};
  requireEncodedBytes(float32Be, {0xc0U, 0x20U, 0U, 0U}, "HLAfloat32BE");
  std::vector<rti::Octet> float32BeRawBuffer{
      static_cast<rti::Octet>(0x7eU)};
  float32Be.encodeInto(float32BeRawBuffer);
  std::vector<rti::Octet> float32BeRawExpected{
      static_cast<rti::Octet>(0x7eU), static_cast<rti::Octet>(0xc0U),
      static_cast<rti::Octet>(0x20U), static_cast<rti::Octet>(0U),
      static_cast<rti::Octet>(0U)};
  require(
      float32BeRawBuffer == float32BeRawExpected,
      "HLAfloat32BE encodeInto did not append its big-endian value");
  rti::HLAfloat32BE rawDecodedFloat32Be;
  auto const rawFloat32BeCursor =
      rawDecodedFloat32Be.decodeFrom(float32BeRawBuffer, 1U);
  require(
      rawFloat32BeCursor == float32BeRawBuffer.size() &&
          rawDecodedFloat32Be.get() == -2.5F,
      "HLAfloat32BE decodeFrom did not decode the prefixed slice");
  std::vector<rti::Octet> truncatedFloat32BeRawBuffer{
      static_cast<rti::Octet>(0x7eU), static_cast<rti::Octet>(0xc0U),
      static_cast<rti::Octet>(0x20U), static_cast<rti::Octet>(0U)};
  rti::HLAfloat32BE truncatedDecodedFloat32Be;
  requireException(
      [&] {
        truncatedDecodedFloat32Be.decodeFrom(
            truncatedFloat32BeRawBuffer, 1U);
      },
      L"EncoderException",
      "truncated HLAfloat32BE decodeFrom buffer");
  rti::HLAfloat32BE outOfRangeDecodedFloat32Be;
  requireException(
      [&] {
        outOfRangeDecodedFloat32Be.decodeFrom(
            float32BeRawBuffer, float32BeRawBuffer.size() + 1U);
      },
      L"EncoderException",
      "out-of-range HLAfloat32BE decodeFrom index");
  rti::HLAfloat32BE endOfBufferDecodedFloat32Be;
  requireException(
      [&] {
        endOfBufferDecodedFloat32Be.decodeFrom(
            float32BeRawBuffer, float32BeRawBuffer.size());
      },
      L"EncoderException",
      "end-of-buffer HLAfloat32BE decodeFrom index");
  requireEncodedBytes(float32Le, {0U, 0U, 0x20U, 0xc0U}, "HLAfloat32LE");
  std::vector<rti::Octet> float32LeRawBuffer{
      static_cast<rti::Octet>(0x7eU)};
  float32Le.encodeInto(float32LeRawBuffer);
  std::vector<rti::Octet> float32LeRawExpected{
      static_cast<rti::Octet>(0x7eU), static_cast<rti::Octet>(0U),
      static_cast<rti::Octet>(0U), static_cast<rti::Octet>(0x20U),
      static_cast<rti::Octet>(0xc0U)};
  require(
      float32LeRawBuffer == float32LeRawExpected,
      "HLAfloat32LE encodeInto did not append its little-endian value");
  rti::HLAfloat32LE rawDecodedFloat32Le;
  auto const rawFloat32LeCursor =
      rawDecodedFloat32Le.decodeFrom(float32LeRawBuffer, 1U);
  require(
      rawFloat32LeCursor == float32LeRawBuffer.size() &&
          rawDecodedFloat32Le.get() == -2.5F,
      "HLAfloat32LE decodeFrom did not decode the prefixed slice");
  std::vector<rti::Octet> truncatedFloat32LeRawBuffer{
      static_cast<rti::Octet>(0x7eU), static_cast<rti::Octet>(0U),
      static_cast<rti::Octet>(0U), static_cast<rti::Octet>(0x20U)};
  rti::HLAfloat32LE truncatedDecodedFloat32Le;
  requireException(
      [&] {
        truncatedDecodedFloat32Le.decodeFrom(
            truncatedFloat32LeRawBuffer, 1U);
      },
      L"EncoderException",
      "truncated HLAfloat32LE decodeFrom buffer");
  rti::HLAfloat32LE outOfRangeDecodedFloat32Le;
  requireException(
      [&] {
        outOfRangeDecodedFloat32Le.decodeFrom(
            float32LeRawBuffer, float32LeRawBuffer.size() + 1U);
      },
      L"EncoderException",
      "out-of-range HLAfloat32LE decodeFrom index");
  rti::HLAfloat32LE endOfBufferDecodedFloat32Le;
  requireException(
      [&] {
        endOfBufferDecodedFloat32Le.decodeFrom(
            float32LeRawBuffer, float32LeRawBuffer.size());
      },
      L"EncoderException",
      "end-of-buffer HLAfloat32LE decodeFrom index");
  requireEncodedBytes(
      float64Be,
      {0xc0U, 0x04U, 0U, 0U, 0U, 0U, 0U, 0U},
      "HLAfloat64BE");
  std::vector<rti::Octet> float64BeRawBuffer{
      static_cast<rti::Octet>(0x7eU)};
  float64Be.encodeInto(float64BeRawBuffer);
  std::vector<rti::Octet> float64BeRawExpected{
      static_cast<rti::Octet>(0x7eU), static_cast<rti::Octet>(0xc0U),
      static_cast<rti::Octet>(0x04U), static_cast<rti::Octet>(0U),
      static_cast<rti::Octet>(0U), static_cast<rti::Octet>(0U),
      static_cast<rti::Octet>(0U), static_cast<rti::Octet>(0U),
      static_cast<rti::Octet>(0U)};
  require(
      float64BeRawBuffer == float64BeRawExpected,
      "HLAfloat64BE encodeInto did not append its big-endian value");
  rti::HLAfloat64BE rawDecodedFloat64Be;
  auto const rawFloat64BeCursor =
      rawDecodedFloat64Be.decodeFrom(float64BeRawBuffer, 1U);
  require(
      rawFloat64BeCursor == float64BeRawBuffer.size() &&
          rawDecodedFloat64Be.get() == -2.5,
      "HLAfloat64BE decodeFrom did not decode the prefixed slice");
  std::vector<rti::Octet> truncatedFloat64BeRawBuffer{
      static_cast<rti::Octet>(0x7eU), static_cast<rti::Octet>(0xc0U),
      static_cast<rti::Octet>(0x04U), static_cast<rti::Octet>(0U),
      static_cast<rti::Octet>(0U), static_cast<rti::Octet>(0U),
      static_cast<rti::Octet>(0U), static_cast<rti::Octet>(0U)};
  rti::HLAfloat64BE truncatedDecodedFloat64Be;
  requireException(
      [&] {
        truncatedDecodedFloat64Be.decodeFrom(
            truncatedFloat64BeRawBuffer, 1U);
      },
      L"EncoderException",
      "truncated HLAfloat64BE decodeFrom buffer");
  rti::HLAfloat64BE outOfRangeDecodedFloat64Be;
  requireException(
      [&] {
        outOfRangeDecodedFloat64Be.decodeFrom(
            float64BeRawBuffer, float64BeRawBuffer.size() + 1U);
      },
      L"EncoderException",
      "out-of-range HLAfloat64BE decodeFrom index");
  rti::HLAfloat64BE endOfBufferDecodedFloat64Be;
  requireException(
      [&] {
        endOfBufferDecodedFloat64Be.decodeFrom(
            float64BeRawBuffer, float64BeRawBuffer.size());
      },
      L"EncoderException",
      "end-of-buffer HLAfloat64BE decodeFrom index");
  requireEncodedBytes(
      float64Le,
      {0U, 0U, 0U, 0U, 0U, 0U, 0x04U, 0xc0U},
      "HLAfloat64LE");
  std::vector<rti::Octet> float64LeRawBuffer{
      static_cast<rti::Octet>(0x7eU)};
  float64Le.encodeInto(float64LeRawBuffer);
  std::vector<rti::Octet> float64LeRawExpected{
      static_cast<rti::Octet>(0x7eU), static_cast<rti::Octet>(0U),
      static_cast<rti::Octet>(0U), static_cast<rti::Octet>(0U),
      static_cast<rti::Octet>(0U), static_cast<rti::Octet>(0U),
      static_cast<rti::Octet>(0U), static_cast<rti::Octet>(0x04U),
      static_cast<rti::Octet>(0xc0U)};
  require(
      float64LeRawBuffer == float64LeRawExpected,
      "HLAfloat64LE encodeInto did not append its little-endian value");
  rti::HLAfloat64LE rawDecodedFloat64Le;
  auto const rawFloat64LeCursor =
      rawDecodedFloat64Le.decodeFrom(float64LeRawBuffer, 1U);
  require(
      rawFloat64LeCursor == float64LeRawBuffer.size() &&
          rawDecodedFloat64Le.get() == -2.5,
      "HLAfloat64LE decodeFrom did not decode the prefixed slice");
  std::vector<rti::Octet> truncatedFloat64LeRawBuffer{
      static_cast<rti::Octet>(0x7eU), static_cast<rti::Octet>(0U),
      static_cast<rti::Octet>(0U), static_cast<rti::Octet>(0U),
      static_cast<rti::Octet>(0U), static_cast<rti::Octet>(0U),
      static_cast<rti::Octet>(0U), static_cast<rti::Octet>(0x04U)};
  rti::HLAfloat64LE truncatedDecodedFloat64Le;
  requireException(
      [&] {
        truncatedDecodedFloat64Le.decodeFrom(
            truncatedFloat64LeRawBuffer, 1U);
      },
      L"EncoderException",
      "truncated HLAfloat64LE decodeFrom buffer");
  rti::HLAfloat64LE outOfRangeDecodedFloat64Le;
  requireException(
      [&] {
        outOfRangeDecodedFloat64Le.decodeFrom(
            float64LeRawBuffer, float64LeRawBuffer.size() + 1U);
      },
      L"EncoderException",
      "out-of-range HLAfloat64LE decodeFrom index");
  rti::HLAfloat64LE endOfBufferDecodedFloat64Le;
  requireException(
      [&] {
        endOfBufferDecodedFloat64Le.decodeFrom(
            float64LeRawBuffer, float64LeRawBuffer.size());
      },
      L"EncoderException",
      "end-of-buffer HLAfloat64LE decodeFrom index");
  rti::HLAfloat64BE decodedFloat64;
  decodedFloat64.decode(float64Be.encode());
  require(decodedFloat64.get() == -2.5, "HLAfloat64BE did not round-trip");

  auto const positiveInfinity32 = std::numeric_limits<float>::infinity();
  auto const negativeInfinity32 = -positiveInfinity32;
  auto const positiveInfinity64 = std::numeric_limits<double>::infinity();
  auto const negativeInfinity64 = -positiveInfinity64;
  rti::HLAfloat32BE float32PositiveZeroBe{0.0F};
  rti::HLAfloat32LE float32PositiveZeroLe{0.0F};
  rti::HLAfloat32BE float32NegativeZeroBe{-0.0F};
  rti::HLAfloat32LE float32NegativeZeroLe{-0.0F};
  rti::HLAfloat32BE float32PositiveInfinityBe{positiveInfinity32};
  rti::HLAfloat32LE float32PositiveInfinityLe{positiveInfinity32};
  rti::HLAfloat32BE float32NegativeInfinityBe{negativeInfinity32};
  rti::HLAfloat32LE float32NegativeInfinityLe{negativeInfinity32};
  requireEncodedBytes(
      float32PositiveZeroBe,
      {0U, 0U, 0U, 0U},
      "HLAfloat32BE positive zero");
  requireEncodedBytes(
      float32PositiveZeroLe,
      {0U, 0U, 0U, 0U},
      "HLAfloat32LE positive zero");
  requireEncodedBytes(
      float32NegativeZeroBe,
      {0x80U, 0U, 0U, 0U},
      "HLAfloat32BE negative zero");
  requireEncodedBytes(
      float32NegativeZeroLe,
      {0U, 0U, 0U, 0x80U},
      "HLAfloat32LE negative zero");
  requireEncodedBytes(
      float32PositiveInfinityBe,
      {0x7fU, 0x80U, 0U, 0U},
      "HLAfloat32BE positive infinity");
  requireEncodedBytes(
      float32PositiveInfinityLe,
      {0U, 0U, 0x80U, 0x7fU},
      "HLAfloat32LE positive infinity");
  requireEncodedBytes(
      float32NegativeInfinityBe,
      {0xffU, 0x80U, 0U, 0U},
      "HLAfloat32BE negative infinity");
  requireEncodedBytes(
      float32NegativeInfinityLe,
      {0U, 0U, 0x80U, 0xffU},
      "HLAfloat32LE negative infinity");

  rti::HLAfloat64BE float64PositiveZeroBe{0.0};
  rti::HLAfloat64LE float64PositiveZeroLe{0.0};
  rti::HLAfloat64BE float64NegativeZeroBe{-0.0};
  rti::HLAfloat64LE float64NegativeZeroLe{-0.0};
  rti::HLAfloat64BE float64PositiveInfinityBe{positiveInfinity64};
  rti::HLAfloat64LE float64PositiveInfinityLe{positiveInfinity64};
  rti::HLAfloat64BE float64NegativeInfinityBe{negativeInfinity64};
  rti::HLAfloat64LE float64NegativeInfinityLe{negativeInfinity64};
  requireEncodedBytes(
      float64PositiveZeroBe,
      {0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U},
      "HLAfloat64BE positive zero");
  requireEncodedBytes(
      float64PositiveZeroLe,
      {0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U},
      "HLAfloat64LE positive zero");
  requireEncodedBytes(
      float64NegativeZeroBe,
      {0x80U, 0U, 0U, 0U, 0U, 0U, 0U, 0U},
      "HLAfloat64BE negative zero");
  requireEncodedBytes(
      float64NegativeZeroLe,
      {0U, 0U, 0U, 0U, 0U, 0U, 0U, 0x80U},
      "HLAfloat64LE negative zero");
  requireEncodedBytes(
      float64PositiveInfinityBe,
      {0x7fU, 0xf0U, 0U, 0U, 0U, 0U, 0U, 0U},
      "HLAfloat64BE positive infinity");
  requireEncodedBytes(
      float64PositiveInfinityLe,
      {0U, 0U, 0U, 0U, 0U, 0U, 0xf0U, 0x7fU},
      "HLAfloat64LE positive infinity");
  requireEncodedBytes(
      float64NegativeInfinityBe,
      {0xffU, 0xf0U, 0U, 0U, 0U, 0U, 0U, 0U},
      "HLAfloat64BE negative infinity");
  requireEncodedBytes(
      float64NegativeInfinityLe,
      {0U, 0U, 0U, 0U, 0U, 0U, 0xf0U, 0xffU},
      "HLAfloat64LE negative infinity");

  auto const minSubnormal32 = std::numeric_limits<float>::denorm_min();
  auto const minNormal32 = std::numeric_limits<float>::min();
  auto const maxFinite32 = std::numeric_limits<float>::max();
  rti::HLAfloat32BE float32MinSubnormalBe{minSubnormal32};
  rti::HLAfloat32LE float32MinSubnormalLe{minSubnormal32};
  rti::HLAfloat32BE float32MinNormalBe{minNormal32};
  rti::HLAfloat32LE float32MinNormalLe{minNormal32};
  rti::HLAfloat32BE float32MaxFiniteBe{maxFinite32};
  rti::HLAfloat32LE float32MaxFiniteLe{maxFinite32};
  requireEncodedBytes(
      float32MinSubnormalBe,
      {0U, 0U, 0U, 1U},
      "HLAfloat32BE minimum positive subnormal");
  requireEncodedBytes(
      float32MinSubnormalLe,
      {1U, 0U, 0U, 0U},
      "HLAfloat32LE minimum positive subnormal");
  requireEncodedBytes(
      float32MinNormalBe,
      {0U, 0x80U, 0U, 0U},
      "HLAfloat32BE minimum positive normal");
  requireEncodedBytes(
      float32MinNormalLe,
      {0U, 0U, 0x80U, 0U},
      "HLAfloat32LE minimum positive normal");
  requireEncodedBytes(
      float32MaxFiniteBe,
      {0x7fU, 0x7fU, 0xffU, 0xffU},
      "HLAfloat32BE maximum finite");
  requireEncodedBytes(
      float32MaxFiniteLe,
      {0xffU, 0xffU, 0x7fU, 0x7fU},
      "HLAfloat32LE maximum finite");

  auto const minSubnormal64 = std::numeric_limits<double>::denorm_min();
  auto const minNormal64 = std::numeric_limits<double>::min();
  auto const maxFinite64 = std::numeric_limits<double>::max();
  rti::HLAfloat64BE float64MinSubnormalBe{minSubnormal64};
  rti::HLAfloat64LE float64MinSubnormalLe{minSubnormal64};
  rti::HLAfloat64BE float64MinNormalBe{minNormal64};
  rti::HLAfloat64LE float64MinNormalLe{minNormal64};
  rti::HLAfloat64BE float64MaxFiniteBe{maxFinite64};
  rti::HLAfloat64LE float64MaxFiniteLe{maxFinite64};
  requireEncodedBytes(
      float64MinSubnormalBe,
      {0U, 0U, 0U, 0U, 0U, 0U, 0U, 1U},
      "HLAfloat64BE minimum positive subnormal");
  requireEncodedBytes(
      float64MinSubnormalLe,
      {1U, 0U, 0U, 0U, 0U, 0U, 0U, 0U},
      "HLAfloat64LE minimum positive subnormal");
  requireEncodedBytes(
      float64MinNormalBe,
      {0U, 0x10U, 0U, 0U, 0U, 0U, 0U, 0U},
      "HLAfloat64BE minimum positive normal");
  requireEncodedBytes(
      float64MinNormalLe,
      {0U, 0U, 0U, 0U, 0U, 0U, 0x10U, 0U},
      "HLAfloat64LE minimum positive normal");
  requireEncodedBytes(
      float64MaxFiniteBe,
      {0x7fU, 0xefU, 0xffU, 0xffU, 0xffU, 0xffU, 0xffU, 0xffU},
      "HLAfloat64BE maximum finite");
  requireEncodedBytes(
      float64MaxFiniteLe,
      {0xffU, 0xffU, 0xffU, 0xffU, 0xffU, 0xffU, 0xefU, 0x7fU},
      "HLAfloat64LE maximum finite");

  rti::HLAfloat32BE decodedFloat32NegativeZero;
  decodedFloat32NegativeZero.decode(float32NegativeZeroBe.encode());
  requireEncodedBytes(
      decodedFloat32NegativeZero,
      {0x80U, 0U, 0U, 0U},
      "HLAfloat32BE negative-zero round-trip");
  rti::HLAfloat64LE decodedFloat64NegativeInfinity;
  decodedFloat64NegativeInfinity.decode(float64NegativeInfinityLe.encode());
  require(
      decodedFloat64NegativeInfinity.get() == negativeInfinity64,
      "HLAfloat64LE negative infinity did not round-trip");
  rti::HLAfloat32LE decodedFloat32MinSubnormal;
  decodedFloat32MinSubnormal.decode(float32MinSubnormalLe.encode());
  require(
      decodedFloat32MinSubnormal.get() == minSubnormal32,
      "HLAfloat32LE minimum positive subnormal did not round-trip");
  rti::HLAfloat64BE decodedFloat64MaxFinite;
  decodedFloat64MaxFinite.decode(float64MaxFiniteBe.encode());
  require(
      decodedFloat64MaxFinite.get() == maxFinite64,
      "HLAfloat64BE maximum finite did not round-trip");

  rti::HLAASCIIchar asciiCharacter{'A'};
  rti::HLAASCIIstring asciiString{std::string{"A\0Z", 3U}};
  requireEncodedBytes(asciiCharacter, {0x41U}, "HLAASCIIchar");
  std::vector<rti::Octet> asciiCharRawBuffer{
      static_cast<rti::Octet>(0x7eU)};
  asciiCharacter.encodeInto(asciiCharRawBuffer);
  std::vector<rti::Octet> asciiCharRawExpected{
      static_cast<rti::Octet>(0x7eU), static_cast<rti::Octet>(0x41U)};
  require(
      asciiCharRawBuffer == asciiCharRawExpected,
      "HLAASCIIchar encodeInto did not append its character byte");
  rti::HLAASCIIchar rawDecodedAsciiCharacter;
  auto const rawAsciiCharCursor =
      rawDecodedAsciiCharacter.decodeFrom(asciiCharRawBuffer, 1U);
  require(
      rawAsciiCharCursor == asciiCharRawBuffer.size() &&
          rawDecodedAsciiCharacter.get() == 'A',
      "HLAASCIIchar decodeFrom did not decode the prefixed slice");
  rti::HLAASCIIchar outOfRangeDecodedAsciiCharacter;
  requireException(
      [&] {
        outOfRangeDecodedAsciiCharacter.decodeFrom(
            asciiCharRawBuffer, asciiCharRawBuffer.size());
      },
      L"EncoderException",
      "out-of-range HLAASCIIchar decodeFrom index");
  rti::HLAASCIIchar beyondBufferDecodedAsciiCharacter;
  requireException(
      [&] {
        beyondBufferDecodedAsciiCharacter.decodeFrom(
            asciiCharRawBuffer, asciiCharRawBuffer.size() + 1U);
      },
      L"EncoderException",
      "beyond-buffer HLAASCIIchar decodeFrom index");
  requireEncodedBytes(
      asciiString,
      {0U, 0U, 0U, 3U, 0x41U, 0U, 0x5aU},
      "HLAASCIIstring");
  std::vector<rti::Octet> asciiStringRawBuffer{
      static_cast<rti::Octet>(0x7eU)};
  asciiString.encodeInto(asciiStringRawBuffer);
  std::vector<rti::Octet> asciiStringRawExpected{
      static_cast<rti::Octet>(0x7eU),
      0U, 0U, 0U, 3U, 0x41U, 0U, 0x5aU};
  require(
      asciiStringRawBuffer == asciiStringRawExpected,
      "HLAASCIIstring encodeInto did not append to a prefixed buffer");
  rti::HLAASCIIstring rawDecodedAsciiString;
  auto const rawAsciiStringCursor =
      rawDecodedAsciiString.decodeFrom(asciiStringRawBuffer, 1U);
  require(
      rawAsciiStringCursor == asciiStringRawBuffer.size() &&
          rawDecodedAsciiString.get() == std::string{"A\0Z", 3U},
      "HLAASCIIstring decodeFrom did not consume the prefixed slice");
  std::vector<rti::Octet> truncatedAsciiStringRawBuffer{
      static_cast<rti::Octet>(0x7eU), static_cast<rti::Octet>(0U),
      static_cast<rti::Octet>(0U), static_cast<rti::Octet>(0U),
      static_cast<rti::Octet>(2U), static_cast<rti::Octet>(0x41U)};
  rti::HLAASCIIstring truncatedDecodedAsciiString;
  requireException(
      [&] {
        truncatedDecodedAsciiString.decodeFrom(
            truncatedAsciiStringRawBuffer, 1U);
      },
      L"EncoderException",
      "truncated HLAASCIIstring decodeFrom payload");
  rti::HLAASCIIstring outOfRangeDecodedAsciiString;
  requireException(
      [&] {
        outOfRangeDecodedAsciiString.decodeFrom(
            asciiStringRawBuffer, asciiStringRawBuffer.size());
      },
      L"EncoderException",
      "out-of-range HLAASCIIstring decodeFrom index");
  rti::HLAASCIIstring beyondBufferDecodedAsciiString;
  requireException(
      [&] {
        beyondBufferDecodedAsciiString.decodeFrom(
            asciiStringRawBuffer, asciiStringRawBuffer.size() + 1U);
      },
      L"EncoderException",
      "beyond-buffer HLAASCIIstring decodeFrom index");
  rti::HLAASCIIstring decodedAsciiString;
  decodedAsciiString.decode(asciiString.encode());
  require(
      decodedAsciiString.get() == std::string{"A\0Z", 3U},
      "HLAASCIIstring did not round-trip");
  rti::HLAASCIIchar nonAsciiCharacter{static_cast<char>(0x80U)};
  requireException(
      [&] { nonAsciiCharacter.encode(); },
      L"EncoderException",
      "non-ASCII HLAASCIIchar encoding");
  std::vector<std::uint8_t> nonAsciiBytes{0x80U};
  rti::VariableLengthData nonAsciiData(nonAsciiBytes.data(), nonAsciiBytes.size());
  rti::HLAASCIIchar decodedAsciiCharacter;
  requireException(
      [&] { decodedAsciiCharacter.decode(nonAsciiData); },
      L"EncoderException",
      "non-ASCII HLAASCIIchar decoding");
  std::vector<std::uint8_t> truncatedAsciiStringBytes{0U, 0U, 0U, 1U};
  rti::VariableLengthData truncatedAsciiString(
      truncatedAsciiStringBytes.data(), truncatedAsciiStringBytes.size());
  requireException(
      [&] { decodedAsciiString.decode(truncatedAsciiString); },
      L"EncoderException",
      "truncated HLAASCIIstring encoding");

  rti::HLAunicodeChar unicodeCharacter{static_cast<wchar_t>(0x03a9U)};
  requireEncodedBytes(unicodeCharacter, {0x03U, 0xa9U}, "HLAunicodeChar");
  std::vector<rti::Octet> unicodeCharRawBuffer{
      static_cast<rti::Octet>(0x7eU)};
  unicodeCharacter.encodeInto(unicodeCharRawBuffer);
  require(
      unicodeCharRawBuffer ==
          std::vector<rti::Octet>{
              static_cast<rti::Octet>(0x7eU),
              static_cast<rti::Octet>(0x03U),
              static_cast<rti::Octet>(0xa9U)},
      "HLAunicodeChar encodeInto did not append its UTF-16BE code unit");
  rti::HLAunicodeChar rawDecodedUnicodeCharacter;
  auto const rawUnicodeCharCursor =
      rawDecodedUnicodeCharacter.decodeFrom(unicodeCharRawBuffer, 1U);
  require(
      rawUnicodeCharCursor == unicodeCharRawBuffer.size() &&
          rawDecodedUnicodeCharacter.get() == static_cast<wchar_t>(0x03a9U),
      "HLAunicodeChar decodeFrom did not consume the prefixed slice");
  std::vector<rti::Octet> truncatedUnicodeCharRawBuffer{
      static_cast<rti::Octet>(0x7eU), static_cast<rti::Octet>(0x03U)};
  rti::HLAunicodeChar truncatedDecodedUnicodeCharacter;
  requireException(
      [&] {
        truncatedDecodedUnicodeCharacter.decodeFrom(
            truncatedUnicodeCharRawBuffer, 1U);
      },
      L"EncoderException",
      "truncated HLAunicodeChar decodeFrom buffer");
  rti::HLAunicodeChar outOfRangeDecodedUnicodeCharacter;
  requireException(
      [&] {
        outOfRangeDecodedUnicodeCharacter.decodeFrom(
            unicodeCharRawBuffer, unicodeCharRawBuffer.size());
      },
      L"EncoderException",
      "out-of-range HLAunicodeChar decodeFrom index");
  rti::HLAunicodeChar beyondBufferDecodedUnicodeCharacter;
  requireException(
      [&] {
        beyondBufferDecodedUnicodeCharacter.decodeFrom(
            unicodeCharRawBuffer, unicodeCharRawBuffer.size() + 1U);
      },
      L"EncoderException",
      "beyond-buffer HLAunicodeChar decodeFrom index");
  rti::HLAunicodeChar decodedUnicodeCharacter;
  decodedUnicodeCharacter.decode(unicodeCharacter.encode());
  require(
      decodedUnicodeCharacter.get() == static_cast<wchar_t>(0x03a9U),
      "HLAunicodeChar did not round-trip");
  rti::HLAunicodeString unicodeString{L"A\U0001f600"};
  std::vector<std::uint8_t> unicodeExpected{
      0U, 0U, 0U, 3U, 0U, 0x41U, 0xd8U, 0x3dU, 0xdeU, 0U};
  requireEncodedBytes(
      unicodeString,
      unicodeExpected,
      "HLAunicodeString UTF-16BE element count");
  std::vector<rti::Octet> unicodeRawBuffer{
      static_cast<rti::Octet>(0x7eU)};
  unicodeString.encodeInto(unicodeRawBuffer);
  std::vector<rti::Octet> unicodeRawExpected{
      static_cast<rti::Octet>(0x7eU)};
  unicodeRawExpected.insert(
      unicodeRawExpected.end(), unicodeExpected.begin(), unicodeExpected.end());
  require(
      unicodeRawBuffer == unicodeRawExpected,
      "HLAunicodeString encodeInto did not append its UTF-16BE data");
  rti::HLAunicodeString rawDecodedUnicodeString;
  auto const rawUnicodeCursor =
      rawDecodedUnicodeString.decodeFrom(unicodeRawBuffer, 1U);
  require(
      rawUnicodeCursor == unicodeRawBuffer.size() &&
          rawDecodedUnicodeString.get() == L"A\U0001f600",
      "HLAunicodeString decodeFrom did not consume the prefixed slice");
  std::vector<rti::Octet> truncatedUnicodeRawBuffer{
      static_cast<rti::Octet>(0x7eU), static_cast<rti::Octet>(0U),
      static_cast<rti::Octet>(0U), static_cast<rti::Octet>(0U),
      static_cast<rti::Octet>(2U), static_cast<rti::Octet>(0U),
      static_cast<rti::Octet>(0x41U)};
  rti::HLAunicodeString truncatedDecodedUnicodeString;
  requireException(
      [&] {
        truncatedDecodedUnicodeString.decodeFrom(
            truncatedUnicodeRawBuffer, 1U);
      },
      L"EncoderException",
      "truncated HLAunicodeString decodeFrom payload");
  rti::HLAunicodeString outOfRangeDecodedUnicodeString;
  requireException(
      [&] {
        outOfRangeDecodedUnicodeString.decodeFrom(
            unicodeRawBuffer, unicodeRawBuffer.size());
      },
      L"EncoderException",
      "out-of-range HLAunicodeString decodeFrom index");
  rti::HLAunicodeString beyondBufferDecodedUnicodeString;
  requireException(
      [&] {
        beyondBufferDecodedUnicodeString.decodeFrom(
            unicodeRawBuffer, unicodeRawBuffer.size() + 1U);
      },
      L"EncoderException",
      "beyond-buffer HLAunicodeString decodeFrom index");
  rti::HLAunicodeString decodedUnicodeString;
  decodedUnicodeString.decode(unicodeString.encode());
  require(
      decodedUnicodeString.get() == L"A\U0001f600",
      "HLAunicodeString did not round-trip");
  std::vector<std::uint8_t> malformedUnicodeBytes{
      0U, 0U, 0U, 2U, 0xd8U, 0U};
  rti::VariableLengthData malformedUnicode(
      malformedUnicodeBytes.data(), malformedUnicodeBytes.size());
  requireException(
      [&] { decodedUnicodeString.decode(malformedUnicode); },
      L"EncoderException",
      "HLAunicodeString unmatched high surrogate");
  std::vector<std::uint8_t> wrongUnicodeCountBytes{
      0U, 0U, 0U, 6U, 0U, 0x41U, 0xd8U, 0x3dU, 0xdeU, 0U};
  rti::VariableLengthData wrongUnicodeCount(
      wrongUnicodeCountBytes.data(), wrongUnicodeCountBytes.size());
  requireException(
      [&] { decodedUnicodeString.decode(wrongUnicodeCount); },
      L"EncoderException",
      "HLAunicodeString byte count mistaken for element count");
  rti::HLAunicodeString unmatchedHighSurrogate{
      std::wstring(1U, static_cast<wchar_t>(0xd800U))};
  requireException(
      [&] { unmatchedHighSurrogate.encode(); },
      L"EncoderException",
      "HLAunicodeString unmatched high surrogate encoding");

  std::vector<rti::Octet> opaqueBytes{
      static_cast<rti::Octet>(0xa5U),
      static_cast<rti::Octet>(0U),
      static_cast<rti::Octet>(0x5aU)};
  rti::HLAopaqueData opaque(opaqueBytes.data(), opaqueBytes.size());
  require(
      opaque.getEncodedLength() == 7U && opaque.getOctetBoundary() == 4U &&
          opaque.bufferLength() == opaqueBytes.size() &&
          opaque.dataLength() == opaqueBytes.size(),
      "HLAopaqueData metadata was incorrect");
  rti::HLAopaqueData emptyOpaque;
  auto const emptyOpaqueEncoding = emptyOpaque.encode();
  require(
      emptyOpaque.dataLength() == 0U &&
          emptyOpaque.getEncodedLength() == 4U &&
          emptyOpaqueEncoding.size() == 4U &&
          copyBytes(emptyOpaqueEncoding) ==
              std::vector<std::uint8_t>{0U, 0U, 0U, 0U},
      "empty HLAopaqueData did not encode as a four-octet zero length");
  rti::HLAopaqueData decodedEmptyOpaque;
  decodedEmptyOpaque.decode(emptyOpaqueEncoding);
  std::vector<rti::Octet> emptyOpaqueRawEncoding{0U, 0U, 0U, 0U};
  rti::HLAopaqueData rawDecodedEmptyOpaque;
  auto const emptyOpaqueRawCursor =
      rawDecodedEmptyOpaque.decodeFrom(emptyOpaqueRawEncoding, 0U);
  require(
      decodedEmptyOpaque.dataLength() == 0U &&
          decodedEmptyOpaque.getEncodedLength() == 4U &&
          emptyOpaqueRawCursor == emptyOpaqueRawEncoding.size() &&
          rawDecodedEmptyOpaque.dataLength() == 0U &&
          rawDecodedEmptyOpaque.getEncodedLength() == 4U,
      "empty HLAopaqueData did not decode as a valid zero-length value");
  rti::HLAopaqueData decodedOpaque;
  decodedOpaque.decode(opaque.encode());
  require(
      decodedOpaque.dataLength() == opaqueBytes.size() &&
          std::equal(opaqueBytes.begin(), opaqueBytes.end(), decodedOpaque.get()),
      "HLAopaqueData did not round-trip");
  std::vector<rti::Octet> opaqueRawBuffer{
      static_cast<rti::Octet>(0x7eU)};
  opaque.encodeInto(opaqueRawBuffer);
  std::vector<rti::Octet> opaqueRawExpected{
      static_cast<rti::Octet>(0x7eU),
      0U, 0U, 0U, 3U,
      static_cast<rti::Octet>(0xa5U),
      static_cast<rti::Octet>(0U),
      static_cast<rti::Octet>(0x5aU)};
  require(
      opaqueRawBuffer == opaqueRawExpected,
      "HLAopaqueData encodeInto did not append its length-prefixed bytes");
  rti::HLAopaqueData rawDecodedOpaque;
  auto const rawOpaqueCursor = rawDecodedOpaque.decodeFrom(opaqueRawBuffer, 1U);
  require(
      rawOpaqueCursor == opaqueRawBuffer.size() &&
          rawDecodedOpaque.dataLength() == opaqueBytes.size() &&
          std::equal(
              opaqueBytes.begin(), opaqueBytes.end(), rawDecodedOpaque.get()),
      "HLAopaqueData decodeFrom did not consume the prefixed slice");
  std::vector<rti::Octet> truncatedOpaqueRawBuffer{
      static_cast<rti::Octet>(0x7eU), 0U, 0U, 0U, 2U,
      static_cast<rti::Octet>(0xa5U)};
  rti::HLAopaqueData truncatedRawDecodedOpaque;
  requireException(
      [&] {
        truncatedRawDecodedOpaque.decodeFrom(truncatedOpaqueRawBuffer, 1U);
      },
      L"EncoderException",
      "truncated HLAopaqueData decodeFrom raw payload");
  rti::HLAopaqueData endIndexDecodedOpaque;
  requireException(
      [&] {
        endIndexDecodedOpaque.decodeFrom(
            opaqueRawBuffer, opaqueRawBuffer.size());
      },
      L"EncoderException",
      "end-of-buffer HLAopaqueData decodeFrom index");
  rti::HLAopaqueData beyondBufferDecodedOpaque;
  requireException(
      [&] {
        beyondBufferDecodedOpaque.decodeFrom(
            opaqueRawBuffer, opaqueRawBuffer.size() + 1U);
      },
      L"EncoderException",
      "beyond-buffer HLAopaqueData decodeFrom index");

  std::vector<rti::Octet> replacementOpaqueBytes{
      static_cast<rti::Octet>(0x11U),
      static_cast<rti::Octet>(0x22U),
      static_cast<rti::Octet>(0x33U)};
  rti::HLAopaqueData setOpaque;
  require(
      &setOpaque.set(
          replacementOpaqueBytes.data(), replacementOpaqueBytes.size()) ==
              &setOpaque &&
          setOpaque.dataLength() == replacementOpaqueBytes.size() &&
          std::equal(
              replacementOpaqueBytes.begin(),
              replacementOpaqueBytes.end(),
              setOpaque.get()),
      "HLAopaqueData set did not retain the assigned data");

  rti::Octet* nullOpaqueData = nullptr;
  requireException(
      [&] {
        rti::HLAopaqueData invalidNullMemory(
            &nullOpaqueData, 1U, 0U);
        static_cast<void>(invalidNullMemory);
      },
      L"EncoderException",
      "HLAopaqueData external-memory constructor rejects null memory");
  rti::Octet nonNullOpaqueByte = static_cast<rti::Octet>(0U);
  rti::Octet* nonNullOpaqueData = &nonNullOpaqueByte;
  requireException(
      [&] {
        rti::HLAopaqueData invalidZeroBuffer(
            &nonNullOpaqueData, 0U, 0U);
        static_cast<void>(invalidZeroBuffer);
      },
      L"EncoderException",
      "HLAopaqueData external-memory constructor rejects zero buffer size");
  rti::HLAopaqueData invalidOpaquePointerTarget;
  requireException(
      [&] {
        invalidOpaquePointerTarget.setDataPointer(
            &nullOpaqueData, 1U, 0U);
      },
      L"EncoderException",
      "HLAopaqueData setDataPointer rejects null memory");
  requireException(
      [&] {
        invalidOpaquePointerTarget.setDataPointer(
            &nonNullOpaqueData, 0U, 0U);
      },
      L"EncoderException",
      "HLAopaqueData setDataPointer rejects zero buffer size");

  std::vector<rti::Octet> externalOpaqueBuffer(4U, static_cast<rti::Octet>(0U));
  rti::Octet* externalOpaqueData = externalOpaqueBuffer.data();
  rti::HLAopaqueData externalOpaque;
  require(
      &externalOpaque.setDataPointer(
          &externalOpaqueData, externalOpaqueBuffer.size(), 1U) ==
              &externalOpaque &&
          externalOpaque.bufferLength() == externalOpaqueBuffer.size() &&
          externalOpaque.dataLength() == 1U,
      "HLAopaqueData setDataPointer did not retain external storage");
  externalOpaqueBuffer[0] = static_cast<rti::Octet>(0x7aU);
  require(
      externalOpaque.get()[0] == static_cast<rti::Octet>(0x7aU),
      "HLAopaqueData did not observe external buffer changes");
  externalOpaque.set(
      replacementOpaqueBytes.data(), replacementOpaqueBytes.size());
  require(
      externalOpaque.dataLength() == replacementOpaqueBytes.size() &&
          externalOpaque.getEncodedLength() ==
              4U + replacementOpaqueBytes.size() &&
          std::equal(
              replacementOpaqueBytes.begin(),
              replacementOpaqueBytes.end(),
              externalOpaqueBuffer.begin()) &&
          std::equal(
              replacementOpaqueBytes.begin(),
              replacementOpaqueBytes.end(),
              externalOpaque.get()),
      "HLAopaqueData set did not update external storage");
  rti::HLAopaqueData copiedExternalOpaque{externalOpaque};
  auto clonedExternalOpaqueElement = externalOpaque.clone();
  auto* clonedExternalOpaque =
      dynamic_cast<rti::HLAopaqueData*>(clonedExternalOpaqueElement.get());
  require(
      copiedExternalOpaque.dataLength() == replacementOpaqueBytes.size() &&
          std::equal(
              replacementOpaqueBytes.begin(),
              replacementOpaqueBytes.end(),
              copiedExternalOpaque.get()) &&
          clonedExternalOpaque != nullptr &&
          clonedExternalOpaque->dataLength() == replacementOpaqueBytes.size() &&
          std::equal(
              replacementOpaqueBytes.begin(),
              replacementOpaqueBytes.end(),
              clonedExternalOpaque->get()),
      "HLAopaqueData copy or clone did not preserve the external value");
  externalOpaqueBuffer[0] = static_cast<rti::Octet>(0x7aU);
  require(
      externalOpaque.get()[0] == static_cast<rti::Octet>(0x7aU) &&
          copiedExternalOpaque.get()[0] ==
              static_cast<rti::Octet>(0x11U) &&
          clonedExternalOpaque->get()[0] == static_cast<rti::Octet>(0x11U),
      "HLAopaqueData copy or clone retained external-memory coupling");
  std::vector<rti::Octet> independentOpaqueCopyBytes{
      static_cast<rti::Octet>(0x44U),
      static_cast<rti::Octet>(0x55U),
      static_cast<rti::Octet>(0x66U)};
  copiedExternalOpaque.set(
      independentOpaqueCopyBytes.data(), independentOpaqueCopyBytes.size());
  require(
      externalOpaqueBuffer[0] == static_cast<rti::Octet>(0x7aU) &&
          clonedExternalOpaque->get()[0] == static_cast<rti::Octet>(0x11U) &&
          copiedExternalOpaque.get()[0] == static_cast<rti::Octet>(0x44U),
      "HLAopaqueData copy mutation affected another data element");
  std::vector<rti::Octet> decodedExternalOpaqueBytes{
      0U, 0U, 0U, 2U,
      static_cast<rti::Octet>(0x31U),
      static_cast<rti::Octet>(0x42U)};
  rti::VariableLengthData decodedExternalOpaqueEncoding(
      decodedExternalOpaqueBytes.data(), decodedExternalOpaqueBytes.size());
  rti::HLAopaqueData& decodedExternalOpaque =
      externalOpaque.decode(decodedExternalOpaqueEncoding);
  require(
      &decodedExternalOpaque == &externalOpaque &&
          externalOpaque.dataLength() == 2U &&
          externalOpaque.getEncodedLength() == 6U &&
          externalOpaqueBuffer[0] == static_cast<rti::Octet>(0x31U) &&
          externalOpaqueBuffer[1] == static_cast<rti::Octet>(0x42U) &&
          externalOpaque.get()[0] == static_cast<rti::Octet>(0x31U) &&
          externalOpaque.get()[1] == static_cast<rti::Octet>(0x42U),
      "HLAopaqueData decode did not update external memory and data length");
  externalOpaqueBuffer[0] = static_cast<rti::Octet>(0x53U);
  externalOpaqueBuffer[1] = static_cast<rti::Octet>(0x64U);
  std::vector<rti::Octet> externalOpaqueReencoded{static_cast<rti::Octet>(0x7eU)};
  externalOpaque.encodeInto(externalOpaqueReencoded);
  std::vector<rti::Octet> expectedExternalOpaqueReencoding{
      static_cast<rti::Octet>(0x7eU), 0U, 0U, 0U, 2U,
      static_cast<rti::Octet>(0x53U), static_cast<rti::Octet>(0x64U)};
  require(
      externalOpaqueReencoded == expectedExternalOpaqueReencoding &&
          externalOpaque.dataLength() == 2U &&
          externalOpaque.getEncodedLength() == 6U,
      "HLAopaqueData encoding did not observe external-memory changes");
  std::vector<rti::Octet> oversizedExternalOpaqueBytes{
      0U, 0U, 0U, 5U,
      static_cast<rti::Octet>(0x10U),
      static_cast<rti::Octet>(0x20U),
      static_cast<rti::Octet>(0x30U),
      static_cast<rti::Octet>(0x40U),
      static_cast<rti::Octet>(0x50U)};
  rti::VariableLengthData oversizedExternalOpaqueEncoding(
      oversizedExternalOpaqueBytes.data(), oversizedExternalOpaqueBytes.size());
  requireException(
      [&] { externalOpaque.decode(oversizedExternalOpaqueEncoding); },
      L"EncoderException",
      "HLAopaqueData decode rejects data larger than external buffer");
  require(
      externalOpaque.dataLength() == 2U &&
          externalOpaqueBuffer[0] == static_cast<rti::Octet>(0x53U) &&
          externalOpaqueBuffer[1] == static_cast<rti::Octet>(0x64U),
      "HLAopaqueData oversized decode changed the external value");

  std::vector<std::uint8_t> truncatedOpaqueBytes{0U, 0U, 0U, 2U, 0xa5U};
  rti::VariableLengthData truncatedOpaque(
      truncatedOpaqueBytes.data(), truncatedOpaqueBytes.size());
  requireException(
      [&] { decodedOpaque.decode(truncatedOpaque); },
      L"EncoderException",
      "truncated HLAopaqueData encoding");

  auto const pair = rti::OctetPair{
      static_cast<rti::Octet>(0x12U),
      static_cast<rti::Octet>(0x34U)};
  rti::HLAoctetPairBE pairBe{pair};
  rti::HLAoctetPairLE pairLe{pair};
  requireEncodedBytes(pairBe, {0x12U, 0x34U}, "HLAoctetPairBE");
  requireEncodedBytes(pairLe, {0x34U, 0x12U}, "HLAoctetPairLE");
  std::vector<rti::Octet> pairBeRawBuffer{
      static_cast<rti::Octet>(0x7eU)};
  pairBe.encodeInto(pairBeRawBuffer);
  std::vector<rti::Octet> pairBeRawExpected{
      static_cast<rti::Octet>(0x7eU), static_cast<rti::Octet>(0x12U),
      static_cast<rti::Octet>(0x34U)};
  require(
      pairBeRawBuffer == pairBeRawExpected,
      "HLAoctetPairBE encodeInto did not append its big-endian pair");
  rti::HLAoctetPairBE rawDecodedPair;
  auto const rawPairBeCursor = rawDecodedPair.decodeFrom(pairBeRawBuffer, 1U);
  require(
      rawPairBeCursor == pairBeRawBuffer.size() &&
          rawDecodedPair.get() == pair,
      "HLAoctetPairBE decodeFrom did not decode the prefixed slice");
  std::vector<rti::Octet> truncatedPairBeRawBuffer{
      static_cast<rti::Octet>(0x7eU), static_cast<rti::Octet>(0x12U)};
  rti::HLAoctetPairBE truncatedDecodedPairBe;
  requireException(
      [&] {
        truncatedDecodedPairBe.decodeFrom(truncatedPairBeRawBuffer, 1U);
      },
      L"EncoderException",
      "truncated HLAoctetPairBE decodeFrom buffer");
  rti::HLAoctetPairBE outOfRangeDecodedPairBe;
  requireException(
      [&] {
        outOfRangeDecodedPairBe.decodeFrom(
            pairBeRawBuffer, pairBeRawBuffer.size() + 1U);
      },
      L"EncoderException",
      "out-of-range HLAoctetPairBE decodeFrom index");
  rti::HLAoctetPairBE endOfBufferDecodedPairBe;
  requireException(
      [&] {
        endOfBufferDecodedPairBe.decodeFrom(
            pairBeRawBuffer, pairBeRawBuffer.size());
      },
      L"EncoderException",
      "end-of-buffer HLAoctetPairBE decodeFrom index");
  std::vector<rti::Octet> pairLeRawBuffer{
      static_cast<rti::Octet>(0x7eU)};
  pairLe.encodeInto(pairLeRawBuffer);
  std::vector<rti::Octet> pairLeRawExpected{
      static_cast<rti::Octet>(0x7eU), static_cast<rti::Octet>(0x34U),
      static_cast<rti::Octet>(0x12U)};
  require(
      pairLeRawBuffer == pairLeRawExpected,
      "HLAoctetPairLE encodeInto did not append its little-endian pair");
  rti::HLAoctetPairLE rawDecodedPairLe;
  auto const rawPairLeCursor =
      rawDecodedPairLe.decodeFrom(pairLeRawBuffer, 1U);
  require(
      rawPairLeCursor == pairLeRawBuffer.size() &&
          rawDecodedPairLe.get() == pair,
      "HLAoctetPairLE decodeFrom did not decode the prefixed slice");
  std::vector<rti::Octet> truncatedPairLeRawBuffer{
      static_cast<rti::Octet>(0x7eU), static_cast<rti::Octet>(0x34U)};
  rti::HLAoctetPairLE truncatedDecodedPairLe;
  requireException(
      [&] {
        truncatedDecodedPairLe.decodeFrom(truncatedPairLeRawBuffer, 1U);
      },
      L"EncoderException",
      "truncated HLAoctetPairLE decodeFrom buffer");
  rti::HLAoctetPairLE outOfRangeDecodedPairLe;
  requireException(
      [&] {
        outOfRangeDecodedPairLe.decodeFrom(
            pairLeRawBuffer, pairLeRawBuffer.size() + 1U);
      },
      L"EncoderException",
      "out-of-range HLAoctetPairLE decodeFrom index");
  rti::HLAoctetPairLE endOfBufferDecodedPairLe;
  requireException(
      [&] {
        endOfBufferDecodedPairLe.decodeFrom(
            pairLeRawBuffer, pairLeRawBuffer.size());
      },
      L"EncoderException",
      "end-of-buffer HLAoctetPairLE decodeFrom index");
  rti::OctetPair pairBeExternalStorage = pair;
  rti::HLAoctetPairBE externalPairBe{&pairBeExternalStorage};
  rti::OctetPair callerMutationPairBe{
      static_cast<rti::Octet>(0x56U),
      static_cast<rti::Octet>(0x78U)};
  pairBeExternalStorage = callerMutationPairBe;
  std::vector<rti::Octet> externalPairBeEncoding{
      static_cast<rti::Octet>(0x7eU)};
  externalPairBe.encodeInto(externalPairBeEncoding);
  require(
      externalPairBe.get() == callerMutationPairBe &&
          externalPairBeEncoding ==
              std::vector<rti::Octet>{
                  static_cast<rti::Octet>(0x7eU),
                  static_cast<rti::Octet>(0x56U),
                  static_cast<rti::Octet>(0x78U)},
      "HLAoctetPairBE did not observe caller-owned memory changes");
  rti::OctetPair setPairBe{
      static_cast<rti::Octet>(0x9aU),
      static_cast<rti::Octet>(0xbcU)};
  externalPairBe.set(setPairBe);
  require(
      pairBeExternalStorage == setPairBe,
      "HLAoctetPairBE set did not update caller-owned memory");
  rti::OctetPair decodedExternalPairBe{
      static_cast<rti::Octet>(0xdeU),
      static_cast<rti::Octet>(0xf0U)};
  rti::HLAoctetPairBE decodedExternalPairBeValue{decodedExternalPairBe};
  externalPairBe.decode(decodedExternalPairBeValue.encode());
  require(
      pairBeExternalStorage == decodedExternalPairBe &&
          externalPairBe.get() == decodedExternalPairBe,
      "HLAoctetPairBE decode did not update caller-owned memory");
  rti::HLAoctetPairBE copiedExternalPairBe{externalPairBe};
  auto clonedExternalPairBeElement = externalPairBe.clone();
  auto* clonedExternalPairBe =
      dynamic_cast<rti::HLAoctetPairBE*>(clonedExternalPairBeElement.get());
  require(
      copiedExternalPairBe.get() == decodedExternalPairBe &&
          clonedExternalPairBe != nullptr &&
          clonedExternalPairBe->get() == decodedExternalPairBe,
      "HLAoctetPairBE copy or clone did not preserve the external value");
  pairBeExternalStorage = callerMutationPairBe;
  require(
      externalPairBe.get() == callerMutationPairBe &&
          copiedExternalPairBe.get() == decodedExternalPairBe &&
          clonedExternalPairBe->get() == decodedExternalPairBe,
      "HLAoctetPairBE copy or clone retained external-memory coupling");
  rti::OctetPair independentPairBeValue{
      static_cast<rti::Octet>(0x44U),
      static_cast<rti::Octet>(0x55U)};
  copiedExternalPairBe.set(independentPairBeValue);
  require(
      pairBeExternalStorage == callerMutationPairBe &&
          clonedExternalPairBe->get() == decodedExternalPairBe &&
          copiedExternalPairBe.get() == independentPairBeValue,
      "HLAoctetPairBE copy mutation affected another data element");

  rti::OctetPair pairLeExternalStorage = pair;
  rti::HLAoctetPairLE externalPairLe{&pairLeExternalStorage};
  rti::OctetPair callerMutationPairLe{
      static_cast<rti::Octet>(0x56U),
      static_cast<rti::Octet>(0x78U)};
  pairLeExternalStorage = callerMutationPairLe;
  std::vector<rti::Octet> externalPairLeEncoding{
      static_cast<rti::Octet>(0x7eU)};
  externalPairLe.encodeInto(externalPairLeEncoding);
  require(
      externalPairLe.get() == callerMutationPairLe &&
          externalPairLeEncoding ==
              std::vector<rti::Octet>{
                  static_cast<rti::Octet>(0x7eU),
                  static_cast<rti::Octet>(0x78U),
                  static_cast<rti::Octet>(0x56U)},
      "HLAoctetPairLE did not observe caller-owned memory changes");
  rti::OctetPair setPairLe{
      static_cast<rti::Octet>(0x9aU),
      static_cast<rti::Octet>(0xbcU)};
  externalPairLe.set(setPairLe);
  require(
      pairLeExternalStorage == setPairLe,
      "HLAoctetPairLE set did not update caller-owned memory");
  rti::OctetPair decodedExternalPairLe{
      static_cast<rti::Octet>(0xdeU),
      static_cast<rti::Octet>(0xf0U)};
  rti::HLAoctetPairLE decodedExternalPairLeValue{decodedExternalPairLe};
  externalPairLe.decode(decodedExternalPairLeValue.encode());
  require(
      pairLeExternalStorage == decodedExternalPairLe &&
          externalPairLe.get() == decodedExternalPairLe,
      "HLAoctetPairLE decode did not update caller-owned memory");
  rti::HLAoctetPairLE copiedExternalPairLe{externalPairLe};
  auto clonedExternalPairLeElement = externalPairLe.clone();
  auto* clonedExternalPairLe =
      dynamic_cast<rti::HLAoctetPairLE*>(clonedExternalPairLeElement.get());
  require(
      copiedExternalPairLe.get() == decodedExternalPairLe &&
          clonedExternalPairLe != nullptr &&
          clonedExternalPairLe->get() == decodedExternalPairLe,
      "HLAoctetPairLE copy or clone did not preserve the external value");
  pairLeExternalStorage = callerMutationPairLe;
  require(
      externalPairLe.get() == callerMutationPairLe &&
          copiedExternalPairLe.get() == decodedExternalPairLe &&
          clonedExternalPairLe->get() == decodedExternalPairLe,
      "HLAoctetPairLE copy or clone retained external-memory coupling");
  rti::OctetPair independentPairLeValue{
      static_cast<rti::Octet>(0x44U),
      static_cast<rti::Octet>(0x55U)};
  copiedExternalPairLe.set(independentPairLeValue);
  require(
      pairLeExternalStorage == callerMutationPairLe &&
          clonedExternalPairLe->get() == decodedExternalPairLe &&
          copiedExternalPairLe.get() == independentPairLeValue,
      "HLAoctetPairLE copy mutation affected another data element");

  rti::HLAoctetPairBE decodedPair;
  decodedPair.decode(pairBe.encode());
  require(decodedPair.get() == pair, "HLAoctetPairBE did not round-trip");

  verifyFloatingBasicDataElementExternalMemoryContract();
  verifyScalarBasicDataElementExternalMemoryContract();
  verifyAsciiBasicDataElementExternalMemoryContract();
  verifyUnicodeBasicDataElementExternalMemoryContract();
  verifySixteenBitIntegerExternalMemoryContract();
}


void verifyCompositeDataElementsContract() {
  verifyArrayCopyCloneExternalMemoryContract();
  verifyRecordCopyCloneExternalMemoryContract();
  verifyExtendableVariantCopyCloneContract();
  verifyExtendableVariantRawBufferContract();
  rti::HLAfixedArrayT<rti::HLAbyte> typedFixedArray{3U};
  typedFixedArray
      .set(0U, rti::HLAbyte{static_cast<rti::Octet>(0x11U)})
      .set(1U, rti::HLAbyte{static_cast<rti::Octet>(0x22U)})
      .set(2U, rti::HLAbyte{static_cast<rti::Octet>(0x33U)});
  requireEncodedBytes(
      typedFixedArray,
      {0x11U, 0x22U, 0x33U},
      "HLAfixedArrayT wire encoding");
  rti::HLAfixedArrayT<rti::HLAbyte> decodedTypedFixedArray{3U};
  decodedTypedFixedArray.decode(typedFixedArray.encode());
  require(
      decodedTypedFixedArray.size() == 3U &&
          decodedTypedFixedArray[0U].get() ==
              static_cast<rti::Octet>(0x11U) &&
          decodedTypedFixedArray[2U].get() ==
              static_cast<rti::Octet>(0x33U),
      "HLAfixedArrayT did not round-trip");
  requireException(
      [&] {
        std::vector<rti::Octet> bytes{
            static_cast<rti::Octet>(0x11U),
            static_cast<rti::Octet>(0x22U),
            static_cast<rti::Octet>(0x33U),
            static_cast<rti::Octet>(0U)};
        rti::VariableLengthData data(bytes.data(), bytes.size());
        decodedTypedFixedArray.decode(data);
      },
      L"EncoderException",
      "HLAfixedArrayT trailing data");

  rti::HLAbyte externalTypedFixedElement{
      static_cast<rti::Octet>(0x44U)};
  require(
      &typedFixedArray.setElementPointer(1U, &externalTypedFixedElement) ==
              &typedFixedArray &&
          &typedFixedArray.get(1U) == &externalTypedFixedElement &&
          &typedFixedArray[1U] == &externalTypedFixedElement &&
          typedFixedArray[1U].get() == static_cast<rti::Octet>(0x44U),
      "HLAfixedArrayT typed pointer setter or accessors were incorrect");

  rti::HLAinteger32BE externalFixedElement{0x01020304};
  rti::HLAfixedArray dynamicFixedArray{rti::HLAinteger32BE{}, 2U};
  dynamicFixedArray
      .set(0U, rti::HLAinteger32BE{0x10203040})
      .set(1U, rti::HLAinteger32BE{-2});
  std::vector<std::uint8_t> dynamicFixedExpected{
      0x10U, 0x20U, 0x30U, 0x40U,
      0xffU, 0xffU, 0xffU, 0xfeU};
  requireEncodedBytes(
      dynamicFixedArray,
      dynamicFixedExpected,
      "HLAfixedArray dynamic wire encoding");
  std::vector<rti::Octet> fixedArrayRawBuffer{
      static_cast<rti::Octet>(0x7eU)};
  dynamicFixedArray.encodeInto(fixedArrayRawBuffer);
  std::vector<rti::Octet> fixedArrayRawExpected{
      static_cast<rti::Octet>(0x7eU)};
  fixedArrayRawExpected.insert(
      fixedArrayRawExpected.end(),
      dynamicFixedExpected.begin(),
      dynamicFixedExpected.end());
  require(
      fixedArrayRawBuffer == fixedArrayRawExpected,
      "HLAfixedArray encodeInto did not append to a prefixed buffer");
  rti::HLAfixedArray rawDecodedFixedArray{rti::HLAinteger32BE{}, 2U};
  auto const rawFixedArrayCursor =
      rawDecodedFixedArray.decodeFrom(fixedArrayRawBuffer, 1U);
  require(
      rawFixedArrayCursor == fixedArrayRawBuffer.size() &&
          dynamic_cast<rti::HLAinteger32BE const&>(
              rawDecodedFixedArray.get(0U)).get() == 0x10203040 &&
          dynamic_cast<rti::HLAinteger32BE const&>(
              rawDecodedFixedArray.get(1U)).get() == -2,
      "HLAfixedArray decodeFrom did not consume the prefixed slice");
  require(
      dynamicFixedArray.getOctetBoundary() == 4U &&
          dynamicFixedArray.size() == 2U &&
          dynamicFixedArray.getEncodedLength() == 8U,
      "HLAfixedArray metadata was incorrect");
  require(
      dynamicFixedArray.isSameTypeAs(
          rti::HLAfixedArray{rti::HLAinteger32BE{}, 2U}) &&
          !dynamicFixedArray.isSameTypeAs(
              rti::HLAfixedArray{rti::HLAinteger32BE{}, 1U}) &&
          !dynamicFixedArray.isSameTypeAs(
              rti::HLAfixedArray{rti::HLAoctet{}, 2U}),
      "HLAfixedArray type identity was incorrect");
  require(
      dynamicFixedArray.hasPrototypeSameTypeAs(rti::HLAinteger32BE{}) &&
          !dynamicFixedArray.hasPrototypeSameTypeAs(rti::HLAoctet{}),
      "HLAfixedArray prototype type identity was incorrect");
  auto const& firstDynamicElement = dynamicFixedArray.get(0U);
  require(
      &firstDynamicElement == &dynamicFixedArray[0U] &&
          dynamic_cast<rti::HLAinteger32BE const&>(firstDynamicElement).get() ==
              0x10203040,
      "HLAfixedArray get did not return its stored element");
  require(
      &dynamicFixedArray.setElementPointer(1U, &externalFixedElement) ==
              &dynamicFixedArray &&
          &dynamicFixedArray.get(1U) == &externalFixedElement,
      "HLAfixedArray setElementPointer did not retain the supplied element");

  rti::HLAvariableArrayT<rti::HLAinteger16BE> typedVariableArray;
  typedVariableArray
      .addElement(rti::HLAinteger16BE{1})
      .addElement(rti::HLAinteger16BE{2});
  requireEncodedBytes(
      typedVariableArray,
      {0U, 0U, 0U, 2U, 0U, 1U, 0U, 2U},
      "HLAvariableArrayT wire encoding");
  require(
      typedVariableArray.getEncodedLength() == 8U &&
          typedVariableArray.getOctetBoundary() == 4U,
      "HLAvariableArray length or boundary was incorrect");
  std::vector<rti::Octet> variableArrayRawBuffer{
      static_cast<rti::Octet>(0x7eU)};
  typedVariableArray.encodeInto(variableArrayRawBuffer);
  std::vector<rti::Octet> variableArrayRawExpected{
      static_cast<rti::Octet>(0x7eU),
      0U, 0U, 0U, 2U, 0U, 1U, 0U, 2U};
  require(
      variableArrayRawBuffer == variableArrayRawExpected,
      "HLAvariableArray encodeInto did not append to a prefixed buffer");
  rti::HLAvariableArray rawDecodedVariableArray{rti::HLAinteger16BE{}};
  auto const rawVariableArrayCursor =
      rawDecodedVariableArray.decodeFrom(variableArrayRawBuffer, 1U);
  require(
      rawVariableArrayCursor == variableArrayRawBuffer.size() &&
          rawDecodedVariableArray.size() == 2U &&
          dynamic_cast<rti::HLAinteger16BE const&>(
              rawDecodedVariableArray.get(0U)).get() == 1 &&
          dynamic_cast<rti::HLAinteger16BE const&>(
              rawDecodedVariableArray.get(1U)).get() == 2,
      "HLAvariableArray decodeFrom did not consume the prefixed slice");
  rti::HLAvariableArrayT<rti::HLAinteger16BE> decodedTypedVariableArray;
  decodedTypedVariableArray.decode(typedVariableArray.encode());
  require(
      decodedTypedVariableArray.size() == 2U &&
          decodedTypedVariableArray[0U].get() == 1 &&
          decodedTypedVariableArray[1U].get() == 2,
      "HLAvariableArrayT did not round-trip");
  require(
      typedVariableArray.hasPrototypeSameTypeAs(rti::HLAinteger16BE{}) &&
          !typedVariableArray.hasPrototypeSameTypeAs(rti::HLAinteger32BE{}),
      "HLAvariableArray prototype type identity was incorrect");
  auto const& firstVariableElement = typedVariableArray.get(0U);
  require(
      &firstVariableElement == &typedVariableArray[0U] &&
          firstVariableElement.get() == 1,
      "HLAvariableArray get did not return its stored element");

  rti::HLAinteger16BE firstExternalVariableElement{3};
  rti::HLAinteger16BE replacementExternalVariableElement{7};
  rti::HLAvariableArrayT<rti::HLAinteger16BE> pointerVariableArray;
  require(
      &pointerVariableArray.addElementPointer(&firstExternalVariableElement) ==
              &pointerVariableArray &&
          pointerVariableArray.size() == 1U &&
          &pointerVariableArray.get(0U) == &firstExternalVariableElement,
      "HLAvariableArray addElementPointer did not retain the supplied element");
  require(
      &pointerVariableArray.set(0U, rti::HLAinteger16BE{4}) ==
              &pointerVariableArray &&
          firstExternalVariableElement.get() == 4,
      "HLAvariableArray set did not update externally owned storage");
  require(
      &pointerVariableArray.setElementPointer(
          0U, &replacementExternalVariableElement) == &pointerVariableArray &&
          &pointerVariableArray.get(0U) == &replacementExternalVariableElement,
      "HLAvariableArray setElementPointer did not replace the stored element");

  requireException(
      [&] {
        std::vector<rti::Octet> negativeCount{
            static_cast<rti::Octet>(0x80U),
            static_cast<rti::Octet>(0U),
            static_cast<rti::Octet>(0U),
            static_cast<rti::Octet>(0U)};
        rti::VariableLengthData data(negativeCount.data(), negativeCount.size());
        decodedTypedVariableArray.decode(data);
      },
      L"EncoderException",
      "HLAvariableArrayT negative element count");
  requireEncodedBytes(
      rti::HLAvariableArrayT<rti::HLAinteger16BE>{},
      {0U, 0U, 0U, 0U},
      "HLAvariableArrayT empty encoding");

  rti::HLAfixedRecord alignedRecord;
  alignedRecord
      .appendElement(rti::HLAoctet{static_cast<rti::Octet>(0xa5U)})
      .appendElement(rti::HLAboolean{true})
      .appendElement(rti::HLAfloat64BE{-2.5});
  std::vector<std::uint8_t> alignedRecordExpected{
      0xa5U,
      0U, 0U, 0U,
      0U, 0U, 0U, 1U,
      0xc0U, 0x04U, 0U, 0U, 0U, 0U, 0U, 0U};
  requireEncodedBytes(
      alignedRecord,
      alignedRecordExpected,
      "HLAfixedRecord alignment and declaration order");
  std::vector<rti::Octet> alignedRecordRawBuffer{
      static_cast<rti::Octet>(0x7eU)};
  alignedRecord.encodeInto(alignedRecordRawBuffer);
  std::vector<rti::Octet> alignedRecordRawExpected{
      static_cast<rti::Octet>(0x7eU)};
  alignedRecordRawExpected.insert(
      alignedRecordRawExpected.end(),
      alignedRecordExpected.begin(),
      alignedRecordExpected.end());
  require(
      alignedRecordRawBuffer == alignedRecordRawExpected,
      "HLAfixedRecord encodeInto did not append to a prefixed buffer");
  require(
      alignedRecord.getOctetBoundary() == 8U &&
          alignedRecord.getEncodedLength() == alignedRecordExpected.size() &&
          alignedRecord.size() == 3U,
      "HLAfixedRecord metadata was incorrect");
  rti::HLAfixedRecord decodedRecord;
  decodedRecord
      .appendElement(rti::HLAoctet{})
      .appendElement(rti::HLAboolean{})
      .appendElement(rti::HLAfloat64BE{});
  decodedRecord.decode(alignedRecord.encode());
  require(
      dynamic_cast<rti::HLAoctet const&>(decodedRecord.get(0U)).get() ==
              static_cast<rti::Octet>(0xa5U) &&
          dynamic_cast<rti::HLAboolean const&>(decodedRecord.get(1U)).get() &&
          dynamic_cast<rti::HLAfloat64BE const&>(decodedRecord.get(2U)).get() ==
              -2.5,
      "HLAfixedRecord did not round-trip");
  rti::HLAfixedRecord rawDecodedRecord;
  rawDecodedRecord
      .appendElement(rti::HLAoctet{})
      .appendElement(rti::HLAboolean{})
      .appendElement(rti::HLAfloat64BE{});
  auto const rawRecordCursor =
      rawDecodedRecord.decodeFrom(alignedRecordRawBuffer, 1U);
  require(
      rawRecordCursor == alignedRecordRawBuffer.size() &&
          dynamic_cast<rti::HLAoctet const&>(rawDecodedRecord.get(0U)).get() ==
              static_cast<rti::Octet>(0xa5U) &&
          dynamic_cast<rti::HLAboolean const&>(rawDecodedRecord.get(1U)).get() &&
          dynamic_cast<rti::HLAfloat64BE const&>(
              rawDecodedRecord.get(2U)).get() == -2.5,
      "HLAfixedRecord decodeFrom did not consume the prefixed slice");
  requireException(
      [&] {
        auto invalidPadding = alignedRecordExpected;
        invalidPadding[1U] = 1U;
        rti::VariableLengthData data(
            invalidPadding.data(), invalidPadding.size());
        decodedRecord.decode(data);
      },
      L"EncoderException",
      "HLAfixedRecord nonzero padding");
  requireException(
      [&] {
        auto trailing = alignedRecordExpected;
        trailing.push_back(0U);
        rti::VariableLengthData data(trailing.data(), trailing.size());
        decodedRecord.decode(data);
      },
      L"EncoderException",
      "HLAfixedRecord trailing data");

  rti::HLAoctet firstExternalRecordElement{
      static_cast<rti::Octet>(0x11U)};
  rti::HLAoctet replacementExternalRecordElement{
      static_cast<rti::Octet>(0x22U)};
  rti::HLAfixedRecord externalElementRecord;
  require(
      &externalElementRecord.appendElementPointer(
          &firstExternalRecordElement) == &externalElementRecord &&
          &externalElementRecord.get(0U) == &firstExternalRecordElement &&
          &externalElementRecord[0U] == &firstExternalRecordElement,
      "HLAfixedRecord appendElementPointer did not retain external element");
  require(
      externalElementRecord.hasElementSameTypeAs(
          0U, rti::HLAoctet{}) &&
          !externalElementRecord.hasElementSameTypeAs(
              0U, rti::HLAboolean{}),
      "HLAfixedRecord indexed element type query was incorrect");
  require(
      &externalElementRecord.set(0U, rti::HLAoctet{
          static_cast<rti::Octet>(0x33U)}) == &externalElementRecord &&
          dynamic_cast<rti::HLAoctet const&>(externalElementRecord.get(0U))
                  .get() == static_cast<rti::Octet>(0x33U),
      "HLAfixedRecord set did not replace the indexed value");
  require(
      &externalElementRecord.setElementPointer(
          0U, &replacementExternalRecordElement) == &externalElementRecord &&
          &externalElementRecord.get(0U) ==
              &replacementExternalRecordElement,
      "HLAfixedRecord setElementPointer did not retain external element");
  rti::HLAfixedRecord sameTypeRecord;
  sameTypeRecord.appendElement(rti::HLAoctet{});
  rti::HLAfixedRecord differentTypeRecord;
  differentTypeRecord.appendElement(rti::HLAboolean{});
  require(
      externalElementRecord.isSameTypeAs(sameTypeRecord) &&
          !externalElementRecord.isSameTypeAs(differentTypeRecord),
      "HLAfixedRecord type identity was incorrect");

  rti::HLAfixedRecord recordPrototype;
  recordPrototype
      .appendElement(rti::HLAinteger32BE{})
      .appendElement(rti::HLAoctet{});
  rti::HLAvariableArray variableRecordArray{recordPrototype};
  rti::HLAfixedRecord firstRecord;
  firstRecord
      .appendElement(rti::HLAinteger32BE{1})
      .appendElement(rti::HLAoctet{static_cast<rti::Octet>(0x11U)});
  rti::HLAfixedRecord secondRecord;
  secondRecord
      .appendElement(rti::HLAinteger32BE{2})
      .appendElement(rti::HLAoctet{static_cast<rti::Octet>(0x22U)});
  variableRecordArray.addElement(firstRecord).addElement(secondRecord);
  std::vector<std::uint8_t> variableRecordExpected{
      0U, 0U, 0U, 2U,
      0U, 0U, 0U, 1U, 0x11U,
      0U, 0U, 0U,
      0U, 0U, 0U, 2U, 0x22U};
  requireEncodedBytes(
      variableRecordArray,
      variableRecordExpected,
      "HLAvariableArray nested-record padding");
  requireException(
      [&] {
        auto invalidPadding = variableRecordExpected;
        invalidPadding[9U] = 1U;
        rti::VariableLengthData data(
            invalidPadding.data(), invalidPadding.size());
        variableRecordArray.decode(data);
      },
      L"EncoderException",
      "HLAvariableArray nested-record nonzero padding");

  rti::HLAvariantRecord variant{rti::HLAoctet{}};
  variant.addVariant(
      rti::HLAoctet{static_cast<rti::Octet>(1U)},
      rti::HLAinteger32BE{});
  variant.addVariant(
      rti::HLAoctet{static_cast<rti::Octet>(2U)},
      rti::HLAfloat64BE{});
  require(
      variant.hasMatchingDiscriminantTypeAs(rti::HLAoctet{}) &&
          !variant.hasMatchingDiscriminantTypeAs(rti::HLAinteger32BE{}) &&
          variant.isSameTypeAs(
              rti::HLAoctet{static_cast<rti::Octet>(1U)},
              rti::HLAinteger32BE{}) &&
          !variant.isSameTypeAs(
              rti::HLAoctet{static_cast<rti::Octet>(1U)},
              rti::HLAfloat64BE{}),
      "HLAvariantRecord discriminant or variant type query was incorrect");
  variant.setVariant(
      rti::HLAoctet{static_cast<rti::Octet>(1U)},
      rti::HLAinteger32BE{0x10203040});
  std::vector<std::uint8_t> variantExpected{
      1U,
      0U, 0U, 0U, 0U, 0U, 0U, 0U,
      0x10U, 0x20U, 0x30U, 0x40U};
  require(
      variant.getEncodedLength() == variantExpected.size() &&
          variant.getOctetBoundary() == 8U,
      "HLAvariantRecord length or boundary was incorrect");
  requireEncodedBytes(
      variant,
      variantExpected,
      "HLAvariantRecord alternative alignment");
  std::vector<rti::Octet> variantRawBuffer{
      static_cast<rti::Octet>(0x7eU)};
  variant.encodeInto(variantRawBuffer);
  std::vector<rti::Octet> variantRawExpected{
      static_cast<rti::Octet>(0x7eU)};
  variantRawExpected.insert(
      variantRawExpected.end(), variantExpected.begin(), variantExpected.end());
  require(
      variantRawBuffer == variantRawExpected,
      "HLAvariantRecord encodeInto did not append its encoding to a prefixed buffer");
  rti::HLAvariantRecord decodedVariant{rti::HLAoctet{}};
  decodedVariant.addVariant(
      rti::HLAoctet{static_cast<rti::Octet>(1U)},
      rti::HLAinteger32BE{});
  decodedVariant.addVariant(
      rti::HLAoctet{static_cast<rti::Octet>(2U)},
      rti::HLAfloat64BE{});
  decodedVariant.decode(variant.encode());
  rti::HLAvariantRecord rawDecodedVariant{rti::HLAoctet{}};
  rawDecodedVariant.addVariant(
      rti::HLAoctet{static_cast<rti::Octet>(1U)},
      rti::HLAinteger32BE{});
  rawDecodedVariant.addVariant(
      rti::HLAoctet{static_cast<rti::Octet>(2U)},
      rti::HLAfloat64BE{});
  auto const rawDecodedCursor = rawDecodedVariant.decodeFrom(variantRawBuffer, 1U);
  require(
      rawDecodedCursor == variantRawBuffer.size() &&
          dynamic_cast<rti::HLAoctet const&>(
              rawDecodedVariant.getDiscriminant()).get() ==
              static_cast<rti::Octet>(1U) &&
          dynamic_cast<rti::HLAinteger32BE const&>(
              rawDecodedVariant.getVariant()).get() == 0x10203040,
      "HLAvariantRecord decodeFrom did not decode the prefixed slice and cursor");
  require(
      dynamic_cast<rti::HLAoctet const&>(decodedVariant.getDiscriminant()).get() ==
              static_cast<rti::Octet>(1U) &&
          dynamic_cast<rti::HLAinteger32BE const&>(decodedVariant.getVariant()).get() ==
              0x10203040,
      "HLAvariantRecord did not round-trip");
  rti::HLAvariantRecord sameTypeVariant{rti::HLAoctet{}};
  sameTypeVariant.addVariant(
      rti::HLAoctet{static_cast<rti::Octet>(1U)}, rti::HLAinteger32BE{});
  sameTypeVariant.addVariant(
      rti::HLAoctet{static_cast<rti::Octet>(2U)}, rti::HLAfloat64BE{});
  rti::HLAvariantRecord differentTypeVariant{rti::HLAoctet{}};
  differentTypeVariant.addVariant(
      rti::HLAoctet{static_cast<rti::Octet>(1U)}, rti::HLAfloat64BE{});
  differentTypeVariant.addVariant(
      rti::HLAoctet{static_cast<rti::Octet>(2U)}, rti::HLAfloat64BE{});
  require(
      variant.isSameTypeAs(sameTypeVariant) &&
          !variant.isSameTypeAs(differentTypeVariant),
      "HLAvariantRecord type identity was incorrect");

  rti::HLAinteger32BE firstExternalVariant{0x01020304};
  rti::HLAinteger32BE replacementExternalVariant{0x05060708};
  rti::HLAvariantRecord pointerVariant{rti::HLAoctet{}};
  auto const pointerVariantDiscriminant =
      rti::HLAoctet{static_cast<rti::Octet>(3U)};
  require(
      &pointerVariant.addVariantPointer(
          pointerVariantDiscriminant, &firstExternalVariant) ==
              &pointerVariant &&
          &pointerVariant.setDiscriminant(pointerVariantDiscriminant) ==
              &pointerVariant &&
          &pointerVariant.getVariant() == &firstExternalVariant,
      "HLAvariantRecord addVariantPointer did not retain external variant");
  require(
      &pointerVariant.setVariantPointer(
          pointerVariantDiscriminant, &replacementExternalVariant) ==
              &pointerVariant &&
          &pointerVariant.getVariant() == &replacementExternalVariant &&
          dynamic_cast<rti::HLAoctet const&>(
              pointerVariant.getDiscriminant()).get() ==
              static_cast<rti::Octet>(3U),
      "HLAvariantRecord setVariantPointer did not replace external variant");

  requireException(
      [&] {
        auto invalidPadding = variantExpected;
        invalidPadding[1U] = 1U;
        rti::VariableLengthData data(
            invalidPadding.data(), invalidPadding.size());
        decodedVariant.decode(data);
      },
      L"EncoderException",
      "HLAvariantRecord nonzero padding");
  variant.setDiscriminant(rti::HLAoctet{static_cast<rti::Octet>(0x7fU)});
  requireEncodedBytes(
      variant,
      {0x7fU},
      "HLAvariantRecord unknown discriminant");
  requireException(
      [&] { (void)variant.getVariant(); },
      L"EncoderException",
      "HLAvariantRecord unknown alternative");

  rti::HLAvariantRecordT<rti::HLAoctet> typedVariant;
  typedVariant.addVariant(
      rti::HLAoctet{static_cast<rti::Octet>(1U)},
      rti::HLAinteger16BE{});
  typedVariant.setVariant(
      rti::HLAoctet{static_cast<rti::Octet>(1U)},
      rti::HLAinteger16BE{9});
  requireEncodedBytes(
      typedVariant,
      {1U, 0U, 0U, 9U},
      "HLAvariantRecordT wire encoding");
  require(
      typedVariant.isSameTypeAs(
          rti::HLAvariantRecordT<rti::HLAoctet>{}) &&
          !typedVariant.isSameTypeAs(
              rti::HLAvariantRecordT<rti::HLAinteger32BE>{}),
      "HLAvariantRecordT type identity was incorrect");
  rti::HLAoctet typedVariantPointerDiscriminant{
      static_cast<rti::Octet>(2U)};
  rti::HLAinteger16BE firstTypedVariantPointer{17};
  rti::HLAinteger16BE replacementTypedVariantPointer{19};
  require(
      &typedVariant
           .addVariantPointer(
               typedVariantPointerDiscriminant,
               &firstTypedVariantPointer)
           .setVariantPointer(
               typedVariantPointerDiscriminant,
               &replacementTypedVariantPointer)
           .setDiscriminant(typedVariantPointerDiscriminant) ==
          &typedVariant &&
          typedVariant.getDiscriminant().get() ==
              static_cast<rti::Octet>(2U) &&
          &typedVariant.getVariant() == &replacementTypedVariantPointer &&
          typedVariant.isSameTypeAs(
              typedVariantPointerDiscriminant, rti::HLAinteger16BE{}) &&
          !typedVariant.isSameTypeAs(
              typedVariantPointerDiscriminant, rti::HLAfloat64BE{}),
      "HLAvariantRecordT typed pointer or discriminant API was incorrect");

  rti::HLAextendableVariantRecord extendable{rti::HLAfloat64BE{}};
  rti::HLAfloat64BE extendableDiscriminant{1.5};
  extendable.addVariant(extendableDiscriminant, rti::HLAinteger32BE{});
  extendable.setVariant(
      extendableDiscriminant,
      rti::HLAinteger32BE{0x10203040});
  std::vector<std::uint8_t> extendableExpected{
      0x3fU, 0xf8U, 0U, 0U, 0U, 0U, 0U, 0U,
      0U, 0U, 0U, 4U,
      0U, 0U, 0U, 0U,
      0x10U, 0x20U, 0x30U, 0x40U};
  requireEncodedBytes(
      extendable,
      extendableExpected,
      "HLAextendableVariantRecord wire encoding");
  std::vector<rti::Octet> extendableRawBuffer{
      static_cast<rti::Octet>(0x7eU)};
  extendable.encodeInto(extendableRawBuffer);
  std::vector<rti::Octet> extendableRawExpected{
      static_cast<rti::Octet>(0x7eU)};
  extendableRawExpected.insert(
      extendableRawExpected.end(),
      extendableExpected.begin(),
      extendableExpected.end());
  require(
      extendableRawBuffer == extendableRawExpected,
      "HLAextendableVariantRecord encodeInto did not append to a prefixed buffer");
  rti::HLAextendableVariantRecord decodedExtendable{rti::HLAfloat64BE{}};
  decodedExtendable.addVariant(
      extendableDiscriminant,
      rti::HLAinteger32BE{});
  decodedExtendable.decode(extendable.encode());
  require(
      dynamic_cast<rti::HLAfloat64BE const&>(
          decodedExtendable.getDiscriminant()).get() == 1.5 &&
          dynamic_cast<rti::HLAinteger32BE const&>(
              decodedExtendable.getVariant()).get() == 0x10203040,
      "HLAextendableVariantRecord did not round-trip");
  rti::HLAextendableVariantRecord rawDecodedExtendable{rti::HLAfloat64BE{}};
  rawDecodedExtendable.addVariant(
      extendableDiscriminant,
      rti::HLAinteger32BE{});
  auto const rawExtendableCursor =
      rawDecodedExtendable.decodeFrom(extendableRawBuffer, 1U);
  require(
      rawExtendableCursor == extendableRawBuffer.size() &&
          dynamic_cast<rti::HLAfloat64BE const&>(
              rawDecodedExtendable.getDiscriminant()).get() == 1.5 &&
          dynamic_cast<rti::HLAinteger32BE const&>(
              rawDecodedExtendable.getVariant()).get() == 0x10203040,
      "HLAextendableVariantRecord decodeFrom did not consume the prefixed slice");
  require(
      extendable.hasMatchingDiscriminantTypeAs(rti::HLAfloat64BE{}) &&
          !extendable.hasMatchingDiscriminantTypeAs(rti::HLAoctet{}) &&
          extendable.isSameTypeAs(
              extendableDiscriminant, rti::HLAinteger32BE{}) &&
          !extendable.isSameTypeAs(
              extendableDiscriminant, rti::HLAfloat64BE{}),
      "HLAextendableVariantRecord discriminant or alternative type query was incorrect");
  rti::HLAextendableVariantRecord sameTypeExtendable{rti::HLAfloat64BE{}};
  sameTypeExtendable.addVariant(extendableDiscriminant, rti::HLAinteger32BE{});
  rti::HLAextendableVariantRecord differentTypeExtendable{rti::HLAfloat64BE{}};
  differentTypeExtendable.addVariant(extendableDiscriminant, rti::HLAfloat64BE{});
  require(
      extendable.isSameTypeAs(sameTypeExtendable) &&
          !extendable.isSameTypeAs(differentTypeExtendable) &&
          extendable.getEncodedLength() == extendableExpected.size() &&
          extendable.getOctetBoundary() == 8U,
      "HLAextendableVariantRecord type, length, or boundary query was incorrect");

  rti::HLAinteger32BE firstExternalExtendable{0x01020304};
  rti::HLAinteger32BE replacementExternalExtendable{0x05060708};
  rti::HLAextendableVariantRecord pointerExtendable{rti::HLAfloat64BE{}};
  pointerExtendable.addVariantPointer(
      extendableDiscriminant, &firstExternalExtendable);
  require(
      &pointerExtendable.getVariant() == &firstExternalExtendable,
      "HLAextendableVariantRecord addVariantPointer did not retain external variant");
  pointerExtendable.setVariant(
      extendableDiscriminant, rti::HLAinteger32BE{0x11121314});
  require(
      &pointerExtendable.getVariant() == &firstExternalExtendable &&
          firstExternalExtendable.get() == 0x11121314,
      "HLAextendableVariantRecord setVariant did not update external variant in place");
  pointerExtendable.setVariantPointer(
      extendableDiscriminant, &replacementExternalExtendable);
  pointerExtendable.setDiscriminant(extendableDiscriminant);
  require(
      dynamic_cast<rti::HLAfloat64BE const&>(
          pointerExtendable.getDiscriminant()).get() == 1.5 &&
          &pointerExtendable.getVariant() == &replacementExternalExtendable,
      "HLAextendableVariantRecord pointer replacement or discriminant selection was incorrect");
  requireException(
      [&] {
        auto shortLength = extendableExpected;
        shortLength[11U] = 3U;
        rti::VariableLengthData data(shortLength.data(), shortLength.size());
        decodedExtendable.decode(data);
      },
      L"EncoderException",
      "HLAextendableVariantRecord short alternative length");
  requireException(
      [&] {
        auto trailing = extendableExpected;
        trailing.push_back(0U);
        rti::VariableLengthData data(trailing.data(), trailing.size());
        decodedExtendable.decode(data);
      },
      L"EncoderException",
      "HLAextendableVariantRecord trailing data");

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
      "HLAextendableVariantRecordT wire encoding");
  require(
      typedExtendable.isSameTypeAs(
          rti::HLAextendableVariantRecordT<rti::HLAoctet>{}) &&
          !typedExtendable.isSameTypeAs(
              rti::HLAextendableVariantRecordT<rti::HLAinteger32BE>{}),
      "HLAextendableVariantRecordT type identity was incorrect");
  rti::HLAoctet typedExtendablePointerDiscriminant{
      static_cast<rti::Octet>(2U)};
  rti::HLAinteger16BE firstTypedExtendablePointer{23};
  rti::HLAinteger16BE replacementTypedExtendablePointer{29};
  require(
      &typedExtendable
           .addVariantPointer(
               typedExtendablePointerDiscriminant,
               &firstTypedExtendablePointer)
           .setVariantPointer(
               typedExtendablePointerDiscriminant,
               &replacementTypedExtendablePointer)
           .setDiscriminant(typedExtendablePointerDiscriminant) ==
          &typedExtendable &&
          typedExtendable.getDiscriminant().get() ==
              static_cast<rti::Octet>(2U) &&
          &typedExtendable.getVariant() == &replacementTypedExtendablePointer &&
          typedExtendable.isSameTypeAs(
              typedExtendablePointerDiscriminant, rti::HLAinteger16BE{}) &&
          !typedExtendable.isSameTypeAs(
              typedExtendablePointerDiscriminant, rti::HLAfloat64BE{}),
      "HLAextendableVariantRecordT typed pointer or discriminant API was incorrect");
}

void scenarioFixedRecordAlignment(Options const&, rti::CallbackModel) {
  rti::HLAfixedRecord record;
  record
      .appendElement(rti::HLAoctet{static_cast<rti::Octet>(0xa5U)})
      .appendElement(rti::HLAinteger16BE{0x1234})
      .appendElement(rti::HLAfloat64BE{-2.5})
      .appendElement(rti::HLAoctet{static_cast<rti::Octet>(0x5aU)});

  std::vector<std::uint8_t> const expected{
      0xa5U,
      0U,
      0x12U, 0x34U,
      0U, 0U, 0U, 0U,
      0xc0U, 0x04U, 0U, 0U, 0U, 0U, 0U, 0U,
      0x5aU};
  requireEncodedBytes(
      record,
      expected,
      "HLAfixedRecord component order and inter-component alignment");
  require(
      record.getEncodedLength() == expected.size() &&
          expected.front() == 0xa5U && expected.back() == 0x5aU,
      "HLAfixedRecord added leading or trailing padding, or omitted internal padding");
}

}  // namespace hla_rti_cpp_tck::detail
