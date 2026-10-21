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

void verifyBasicDataElementsContract() {
  verifyFixedWidthIntegerBoundariesContract();
  rti::HLAinteger16BE signed16Be{-2};
  rti::HLAinteger16LE signed16Le{-2};
  rti::HLAinteger32BE signed32Be{-2};
  rti::HLAinteger32LE signed32Le{-2};
  rti::HLAinteger64BE signed64Be{-2};
  rti::HLAinteger64LE signed64Le{-2};
  requireEncodedBytes(signed16Be, {0xffU, 0xfeU}, "HLAinteger16BE");
  requireEncodedBytes(signed16Le, {0xfeU, 0xffU}, "HLAinteger16LE");
  requireEncodedBytes(
      signed32Be,
      {0xffU, 0xffU, 0xffU, 0xfeU},
      "HLAinteger32BE");
  requireEncodedBytes(
      signed32Le,
      {0xfeU, 0xffU, 0xffU, 0xffU},
      "HLAinteger32LE");
  requireEncodedBytes(
      signed64Be,
      {0xffU, 0xffU, 0xffU, 0xffU, 0xffU, 0xffU, 0xffU, 0xfeU},
      "HLAinteger64BE");
  requireEncodedBytes(
      signed64Le,
      {0xfeU, 0xffU, 0xffU, 0xffU, 0xffU, 0xffU, 0xffU, 0xffU},
      "HLAinteger64LE");
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
  requireEncodedBytes(unsigned16Le, {0x34U, 0x12U}, "HLAunsignedInteger16LE");
  requireEncodedBytes(
      unsigned32Be,
      {0x12U, 0x34U, 0xabU, 0xcdU},
      "HLAunsignedInteger32BE");
  requireEncodedBytes(
      unsigned32Le,
      {0xcdU, 0xabU, 0x34U, 0x12U},
      "HLAunsignedInteger32LE");
  requireEncodedBytes(
      unsigned64Be,
      {0x01U, 0x23U, 0x45U, 0x67U, 0x89U, 0xabU, 0xcdU, 0xefU},
      "HLAunsignedInteger64BE");
  requireEncodedBytes(
      unsigned64Le,
      {0xefU, 0xcdU, 0xabU, 0x89U, 0x67U, 0x45U, 0x23U, 0x01U},
      "HLAunsignedInteger64LE");
  rti::HLAunsignedInteger64LE decodedUnsigned64;
  decodedUnsigned64.decode(unsigned64Le.encode());
  require(
      decodedUnsigned64.get() == 0x0123456789abcdefULL,
      "HLAunsignedInteger64LE did not round-trip");

  rti::HLAboolean trueValue{true};
  rti::HLAboolean falseValue{false};
  requireEncodedBytes(trueValue, {0U, 0U, 0U, 1U}, "HLAboolean true");
  requireEncodedBytes(falseValue, {0U, 0U, 0U, 0U}, "HLAboolean false");
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

  rti::HLAoctet octet{static_cast<rti::Octet>(0xa5U)};
  rti::HLAbyte byte{static_cast<rti::Octet>(0xa5U)};
  requireEncodedBytes(octet, {0xa5U}, "HLAoctet");
  requireEncodedBytes(byte, {0xa5U}, "HLAbyte");
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
  requireEncodedBytes(float32Le, {0U, 0U, 0x20U, 0xc0U}, "HLAfloat32LE");
  requireEncodedBytes(
      float64Be,
      {0xc0U, 0x04U, 0U, 0U, 0U, 0U, 0U, 0U},
      "HLAfloat64BE");
  requireEncodedBytes(
      float64Le,
      {0U, 0U, 0U, 0U, 0U, 0U, 0x04U, 0xc0U},
      "HLAfloat64LE");
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
  requireEncodedBytes(
      asciiString,
      {0U, 0U, 0U, 3U, 0x41U, 0U, 0x5aU},
      "HLAASCIIstring");
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
  rti::HLAunicodeChar decodedUnicodeCharacter;
  decodedUnicodeCharacter.decode(unicodeCharacter.encode());
  require(
      decodedUnicodeCharacter.get() == static_cast<wchar_t>(0x03a9U),
      "HLAunicodeChar did not round-trip");
  rti::HLAunicodeString unicodeString{L"A\U0001f600"};
  requireEncodedBytes(
      unicodeString,
      {0U, 0U, 0U, 3U, 0U, 0x41U, 0xd8U, 0x3dU, 0xdeU, 0U},
      "HLAunicodeString UTF-16BE element count");
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
  rti::HLAopaqueData decodedOpaque;
  decodedOpaque.decode(opaque.encode());
  require(
      decodedOpaque.dataLength() == opaqueBytes.size() &&
          std::equal(opaqueBytes.begin(), opaqueBytes.end(), decodedOpaque.get()),
      "HLAopaqueData did not round-trip");
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
  rti::HLAoctetPairBE decodedPair;
  decodedPair.decode(pairBe.encode());
  require(decodedPair.get() == pair, "HLAoctetPairBE did not round-trip");
}


void verifyCompositeDataElementsContract() {
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

  rti::HLAfixedArray dynamicFixedArray{rti::HLAinteger32BE{}, 2U};
  dynamicFixedArray
      .set(0U, rti::HLAinteger32BE{0x10203040})
      .set(1U, rti::HLAinteger32BE{-2});
  requireEncodedBytes(
      dynamicFixedArray,
      {0x10U, 0x20U, 0x30U, 0x40U,
       0xffU, 0xffU, 0xffU, 0xfeU},
      "HLAfixedArray dynamic wire encoding");
  require(
      dynamicFixedArray.getOctetBoundary() == 4U &&
          dynamicFixedArray.size() == 2U,
      "HLAfixedArray metadata was incorrect");
  require(
      dynamicFixedArray.isSameTypeAs(
          rti::HLAfixedArray{rti::HLAinteger32BE{}, 2U}) &&
          !dynamicFixedArray.isSameTypeAs(
              rti::HLAfixedArray{rti::HLAinteger32BE{}, 1U}) &&
          !dynamicFixedArray.isSameTypeAs(
              rti::HLAfixedArray{rti::HLAoctet{}, 2U}),
      "HLAfixedArray type identity was incorrect");

  rti::HLAvariableArrayT<rti::HLAinteger16BE> typedVariableArray;
  typedVariableArray
      .addElement(rti::HLAinteger16BE{1})
      .addElement(rti::HLAinteger16BE{2});
  requireEncodedBytes(
      typedVariableArray,
      {0U, 0U, 0U, 2U, 0U, 1U, 0U, 2U},
      "HLAvariableArrayT wire encoding");
  rti::HLAvariableArrayT<rti::HLAinteger16BE> decodedTypedVariableArray;
  decodedTypedVariableArray.decode(typedVariableArray.encode());
  require(
      decodedTypedVariableArray.size() == 2U &&
          decodedTypedVariableArray[0U].get() == 1 &&
          decodedTypedVariableArray[1U].get() == 2,
      "HLAvariableArrayT did not round-trip");
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
  variant.setVariant(
      rti::HLAoctet{static_cast<rti::Octet>(1U)},
      rti::HLAinteger32BE{0x10203040});
  std::vector<std::uint8_t> variantExpected{
      1U,
      0U, 0U, 0U, 0U, 0U, 0U, 0U,
      0x10U, 0x20U, 0x30U, 0x40U};
  requireEncodedBytes(
      variant,
      variantExpected,
      "HLAvariantRecord alternative alignment");
  rti::HLAvariantRecord decodedVariant{rti::HLAoctet{}};
  decodedVariant.addVariant(
      rti::HLAoctet{static_cast<rti::Octet>(1U)},
      rti::HLAinteger32BE{});
  decodedVariant.addVariant(
      rti::HLAoctet{static_cast<rti::Octet>(2U)},
      rti::HLAfloat64BE{});
  decodedVariant.decode(variant.encode());
  require(
      dynamic_cast<rti::HLAoctet const&>(decodedVariant.getDiscriminant()).get() ==
              static_cast<rti::Octet>(1U) &&
          dynamic_cast<rti::HLAinteger32BE const&>(decodedVariant.getVariant()).get() ==
              0x10203040,
      "HLAvariantRecord did not round-trip");
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
}


}  // namespace hla_rti_cpp_tck::detail
