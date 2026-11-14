#include "tck_data_encoding.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <string>
#include <string_view>

namespace hla_rti_cpp_tck::detail {
namespace {

template <typename Helper, typename Value>
void requireValueAssignmentAndConversion(
    Value const firstValue,
    Value const secondValue,
    std::string_view helperName) {
  Helper helper;

  helper = firstValue;
  Value const firstResult = helper;
  require(
      firstResult == firstValue,
      std::string(helperName) + " conversion did not return its assigned value");

  helper = secondValue;
  Value const secondResult = helper;
  require(
      secondResult == secondValue,
      std::string(helperName) + " assignment did not update its converted value");
}

template <typename Helper, typename Value>
void requireEncodeDecodeRoundTrips(
    Value const& firstValue,
    Value const& secondValue,
    std::string_view helperName) {
  auto requireOneRoundTrip = [&](Value const& value, std::string_view sample) {
    Helper source(value);
    auto const encoded = source.encode();
    Helper decoded;
    static_cast<void>(decoded.decode(encoded));
    Value const decodedValue = decoded;
    require(
        decodedValue == value,
        std::string(helperName) + " encode/decode did not preserve " +
            std::string(sample));
  };

  requireOneRoundTrip(firstValue, "the first value");
  requireOneRoundTrip(secondValue, "the updated value");
}

template <typename Helper, typename Value, std::size_t Size>
void requireExactEncodedBytes(
    Value const& value,
    std::array<std::uint8_t, Size> const& expected,
    std::string_view helperName) {
  Helper source(value);
  auto const encoded = source.encode();
  auto const* bytes = reinterpret_cast<std::uint8_t const*>(encoded.data());
  require(
      encoded.size() == expected.size() &&
          std::equal(expected.begin(), expected.end(), bytes),
      std::string(helperName) + " encoding did not match its declared endian order");
}

}  // namespace

void scenarioBasicDataElementEncodeDecodeContract(
    Options const&,
    rti::CallbackModel) {
  requireEncodeDecodeRoundTrips<rti::HLAASCIIchar>(
      'A', 'Z', "HLAASCIIchar");
  requireEncodeDecodeRoundTrips<rti::HLAASCIIstring>(
      std::string{"Amber"}, std::string{"1516"}, "HLAASCIIstring");
  requireEncodeDecodeRoundTrips<rti::HLAboolean>(
      false, true, "HLAboolean");

  auto const octetFirst = static_cast<rti::Octet>(0x2aU);
  auto const octetSecond = static_cast<rti::Octet>(0x6cU);
  requireEncodeDecodeRoundTrips<rti::HLAbyte>(
      octetFirst, octetSecond, "HLAbyte");
  requireEncodeDecodeRoundTrips<rti::HLAoctet>(
      octetFirst, octetSecond, "HLAoctet");

  requireEncodeDecodeRoundTrips<rti::HLAfloat32BE>(
      -2.5F, 6.25F, "HLAfloat32BE");
  requireEncodeDecodeRoundTrips<rti::HLAfloat32LE>(
      -2.5F, 6.25F, "HLAfloat32LE");
  requireEncodeDecodeRoundTrips<rti::HLAfloat64BE>(
      -2.5, 6.25, "HLAfloat64BE");
  requireEncodeDecodeRoundTrips<rti::HLAfloat64LE>(
      -2.5, 6.25, "HLAfloat64LE");

  requireEncodeDecodeRoundTrips<rti::HLAinteger16BE>(
      static_cast<rti::Integer16>(-2),
      static_cast<rti::Integer16>(17),
      "HLAinteger16BE");
  requireEncodeDecodeRoundTrips<rti::HLAinteger16LE>(
      static_cast<rti::Integer16>(-2),
      static_cast<rti::Integer16>(17),
      "HLAinteger16LE");
  requireEncodeDecodeRoundTrips<rti::HLAinteger32BE>(
      static_cast<rti::Integer32>(-2),
      static_cast<rti::Integer32>(17),
      "HLAinteger32BE");
  requireEncodeDecodeRoundTrips<rti::HLAinteger32LE>(
      static_cast<rti::Integer32>(-2),
      static_cast<rti::Integer32>(17),
      "HLAinteger32LE");
  requireEncodeDecodeRoundTrips<rti::HLAinteger64BE>(
      static_cast<rti::Integer64>(-2),
      static_cast<rti::Integer64>(17),
      "HLAinteger64BE");
  requireEncodeDecodeRoundTrips<rti::HLAinteger64LE>(
      static_cast<rti::Integer64>(-2),
      static_cast<rti::Integer64>(17),
      "HLAinteger64LE");

  requireExactEncodedBytes<rti::HLAinteger32BE>(
      static_cast<rti::Integer32>(-2),
      std::array<std::uint8_t, 4>{0xffU, 0xffU, 0xffU, 0xfeU},
      "HLAinteger32BE");
  requireExactEncodedBytes<rti::HLAinteger32LE>(
      static_cast<rti::Integer32>(-2),
      std::array<std::uint8_t, 4>{0xfeU, 0xffU, 0xffU, 0xffU},
      "HLAinteger32LE");
  requireExactEncodedBytes<rti::HLAfloat32BE>(
      -2.5F,
      std::array<std::uint8_t, 4>{0xc0U, 0x20U, 0x00U, 0x00U},
      "HLAfloat32BE");
  requireExactEncodedBytes<rti::HLAfloat32LE>(
      -2.5F,
      std::array<std::uint8_t, 4>{0x00U, 0x00U, 0x20U, 0xc0U},
      "HLAfloat32LE");
  requireExactEncodedBytes<rti::HLAfloat64BE>(
      -2.5,
      std::array<std::uint8_t, 8>{
          0xc0U, 0x04U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U},
      "HLAfloat64BE");
  requireExactEncodedBytes<rti::HLAfloat64LE>(
      -2.5,
      std::array<std::uint8_t, 8>{
          0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x04U, 0xc0U},
      "HLAfloat64LE");
  requireExactEncodedBytes<rti::HLAinteger16BE>(
      static_cast<rti::Integer16>(-2),
      std::array<std::uint8_t, 2>{0xffU, 0xfeU},
      "HLAinteger16BE");
  requireExactEncodedBytes<rti::HLAinteger16LE>(
      static_cast<rti::Integer16>(-2),
      std::array<std::uint8_t, 2>{0xfeU, 0xffU},
      "HLAinteger16LE");
  requireExactEncodedBytes<rti::HLAinteger64BE>(
      static_cast<rti::Integer64>(-2),
      std::array<std::uint8_t, 8>{
          0xffU, 0xffU, 0xffU, 0xffU, 0xffU, 0xffU, 0xffU, 0xfeU},
      "HLAinteger64BE");
  requireExactEncodedBytes<rti::HLAinteger64LE>(
      static_cast<rti::Integer64>(-2),
      std::array<std::uint8_t, 8>{
          0xfeU, 0xffU, 0xffU, 0xffU, 0xffU, 0xffU, 0xffU, 0xffU},
      "HLAinteger64LE");
  requireExactEncodedBytes<rti::HLAunsignedInteger16BE>(
      static_cast<rti::UnsignedInteger16>(0x1234U),
      std::array<std::uint8_t, 2>{0x12U, 0x34U},
      "HLAunsignedInteger16BE");
  requireExactEncodedBytes<rti::HLAunsignedInteger16LE>(
      static_cast<rti::UnsignedInteger16>(0x1234U),
      std::array<std::uint8_t, 2>{0x34U, 0x12U},
      "HLAunsignedInteger16LE");
  requireExactEncodedBytes<rti::HLAunsignedInteger32BE>(
      static_cast<rti::UnsignedInteger32>(0x12345678U),
      std::array<std::uint8_t, 4>{0x12U, 0x34U, 0x56U, 0x78U},
      "HLAunsignedInteger32BE");
  requireExactEncodedBytes<rti::HLAunsignedInteger32LE>(
      static_cast<rti::UnsignedInteger32>(0x12345678U),
      std::array<std::uint8_t, 4>{0x78U, 0x56U, 0x34U, 0x12U},
      "HLAunsignedInteger32LE");
  requireExactEncodedBytes<rti::HLAunsignedInteger64BE>(
      static_cast<rti::UnsignedInteger64>(0x0123456789abcdefULL),
      std::array<std::uint8_t, 8>{
          0x01U, 0x23U, 0x45U, 0x67U, 0x89U, 0xabU, 0xcdU, 0xefU},
      "HLAunsignedInteger64BE");
  requireExactEncodedBytes<rti::HLAunsignedInteger64LE>(
      static_cast<rti::UnsignedInteger64>(0x0123456789abcdefULL),
      std::array<std::uint8_t, 8>{
          0xefU, 0xcdU, 0xabU, 0x89U, 0x67U, 0x45U, 0x23U, 0x01U},
      "HLAunsignedInteger64LE");

  requireEncodeDecodeRoundTrips<rti::HLAunsignedInteger16BE>(
      static_cast<rti::UnsignedInteger16>(2U),
      static_cast<rti::UnsignedInteger16>(17U),
      "HLAunsignedInteger16BE");
  requireEncodeDecodeRoundTrips<rti::HLAunsignedInteger16LE>(
      static_cast<rti::UnsignedInteger16>(2U),
      static_cast<rti::UnsignedInteger16>(17U),
      "HLAunsignedInteger16LE");
  requireEncodeDecodeRoundTrips<rti::HLAunsignedInteger32BE>(
      static_cast<rti::UnsignedInteger32>(2U),
      static_cast<rti::UnsignedInteger32>(17U),
      "HLAunsignedInteger32BE");
  requireEncodeDecodeRoundTrips<rti::HLAunsignedInteger32LE>(
      static_cast<rti::UnsignedInteger32>(2U),
      static_cast<rti::UnsignedInteger32>(17U),
      "HLAunsignedInteger32LE");
  requireEncodeDecodeRoundTrips<rti::HLAunsignedInteger64BE>(
      static_cast<rti::UnsignedInteger64>(2U),
      static_cast<rti::UnsignedInteger64>(17U),
      "HLAunsignedInteger64BE");
  requireEncodeDecodeRoundTrips<rti::HLAunsignedInteger64LE>(
      static_cast<rti::UnsignedInteger64>(2U),
      static_cast<rti::UnsignedInteger64>(17U),
      "HLAunsignedInteger64LE");

  requireEncodeDecodeRoundTrips<rti::HLAoctetPairBE>(
      rti::OctetPair{octetFirst, octetSecond},
      rti::OctetPair{octetSecond, octetFirst},
      "HLAoctetPairBE");
  requireEncodeDecodeRoundTrips<rti::HLAoctetPairLE>(
      rti::OctetPair{octetFirst, octetSecond},
      rti::OctetPair{octetSecond, octetFirst},
      "HLAoctetPairLE");
  requireEncodeDecodeRoundTrips<rti::HLAunicodeChar>(
      L'\u03a9', L'\u2603', "HLAunicodeChar");
  requireEncodeDecodeRoundTrips<rti::HLAunicodeString>(
      std::wstring{L"Amber"}, std::wstring{L"1516"}, "HLAunicodeString");

  requireExactEncodedBytes<rti::HLAoctetPairBE>(
      rti::OctetPair{octetFirst, octetSecond},
      std::array<std::uint8_t, 2>{0x2aU, 0x6cU},
      "HLAoctetPairBE");
  requireExactEncodedBytes<rti::HLAoctetPairLE>(
      rti::OctetPair{octetFirst, octetSecond},
      std::array<std::uint8_t, 2>{0x6cU, 0x2aU},
      "HLAoctetPairLE");
  requireExactEncodedBytes<rti::HLAunicodeChar>(
      L'\u03a9',
      std::array<std::uint8_t, 2>{0x03U, 0xa9U},
      "HLAunicodeChar UTF-16BE U+03A9");
  requireExactEncodedBytes<rti::HLAunicodeChar>(
      L'\u2603',
      std::array<std::uint8_t, 2>{0x26U, 0x03U},
      "HLAunicodeChar UTF-16BE U+2603");
  requireExactEncodedBytes<rti::HLAASCIIstring>(
      std::string{"A\0Z", 3U},
      std::array<std::uint8_t, 7>{
          0x00U, 0x00U, 0x00U, 0x03U, 0x41U, 0x00U, 0x5aU},
      "HLAASCIIstring variable-array count and embedded NUL");
  requireExactEncodedBytes<rti::HLAunicodeString>(
      std::wstring{L"A\U0001f600"},
      std::array<std::uint8_t, 10>{
          0x00U, 0x00U, 0x00U, 0x03U, 0x00U, 0x41U, 0xd8U, 0x3dU, 0xdeU, 0x00U},
      "HLAunicodeString UTF-16BE code-unit count and supplementary character");
  requireExactEncodedBytes<rti::HLAboolean>(
      false,
      std::array<std::uint8_t, 4>{0x00U, 0x00U, 0x00U, 0x00U},
      "HLAboolean HLAfalse as HLAinteger32BE");
  requireExactEncodedBytes<rti::HLAboolean>(
      true,
      std::array<std::uint8_t, 4>{0x00U, 0x00U, 0x00U, 0x01U},
      "HLAboolean HLAtrue as HLAinteger32BE");
  auto const highBitOctet = static_cast<rti::Octet>(0xa5U);
  requireExactEncodedBytes<rti::HLAbyte>(
      highBitOctet,
      std::array<std::uint8_t, 1>{0xa5U},
      "HLAbyte raw octet");
  requireExactEncodedBytes<rti::HLAoctet>(
      highBitOctet,
      std::array<std::uint8_t, 1>{0xa5U},
      "HLAoctet raw octet");
  requireExactEncodedBytes<rti::HLAASCIIchar>(
      'A',
      std::array<std::uint8_t, 1>{0x41U},
      "HLAASCIIchar");
}

void scenarioBasicDataElementValueOperationsContract(
    Options const&,
    rti::CallbackModel) {
  requireValueAssignmentAndConversion<rti::HLAASCIIchar>(
      'A', 'Z', "HLAASCIIchar");
  requireValueAssignmentAndConversion<rti::HLAASCIIstring>(
      std::string{"Amber"}, std::string{"1516"}, "HLAASCIIstring");
  requireValueAssignmentAndConversion<rti::HLAboolean>(
      false, true, "HLAboolean");

  auto const octetFirst = static_cast<rti::Octet>(0x2aU);
  auto const octetSecond = static_cast<rti::Octet>(0x6cU);
  requireValueAssignmentAndConversion<rti::HLAbyte>(
      octetFirst, octetSecond, "HLAbyte");
  requireValueAssignmentAndConversion<rti::HLAoctet>(
      octetFirst, octetSecond, "HLAoctet");

  requireValueAssignmentAndConversion<rti::HLAfloat32BE>(
      -2.5F, 6.25F, "HLAfloat32BE");
  requireValueAssignmentAndConversion<rti::HLAfloat32LE>(
      -2.5F, 6.25F, "HLAfloat32LE");
  requireValueAssignmentAndConversion<rti::HLAfloat64BE>(
      -2.5, 6.25, "HLAfloat64BE");
  requireValueAssignmentAndConversion<rti::HLAfloat64LE>(
      -2.5, 6.25, "HLAfloat64LE");

  requireValueAssignmentAndConversion<rti::HLAinteger16BE>(
      static_cast<rti::Integer16>(-2),
      static_cast<rti::Integer16>(17),
      "HLAinteger16BE");
  requireValueAssignmentAndConversion<rti::HLAinteger16LE>(
      static_cast<rti::Integer16>(-2),
      static_cast<rti::Integer16>(17),
      "HLAinteger16LE");
  requireValueAssignmentAndConversion<rti::HLAinteger32BE>(
      static_cast<rti::Integer32>(-2),
      static_cast<rti::Integer32>(17),
      "HLAinteger32BE");
  requireValueAssignmentAndConversion<rti::HLAinteger32LE>(
      static_cast<rti::Integer32>(-2),
      static_cast<rti::Integer32>(17),
      "HLAinteger32LE");
  requireValueAssignmentAndConversion<rti::HLAinteger64BE>(
      static_cast<rti::Integer64>(-2),
      static_cast<rti::Integer64>(17),
      "HLAinteger64BE");
  requireValueAssignmentAndConversion<rti::HLAinteger64LE>(
      static_cast<rti::Integer64>(-2),
      static_cast<rti::Integer64>(17),
      "HLAinteger64LE");

  requireValueAssignmentAndConversion<rti::HLAunsignedInteger16BE>(
      static_cast<rti::UnsignedInteger16>(2U),
      static_cast<rti::UnsignedInteger16>(17U),
      "HLAunsignedInteger16BE");
  requireValueAssignmentAndConversion<rti::HLAunsignedInteger16LE>(
      static_cast<rti::UnsignedInteger16>(2U),
      static_cast<rti::UnsignedInteger16>(17U),
      "HLAunsignedInteger16LE");
  requireValueAssignmentAndConversion<rti::HLAunsignedInteger32BE>(
      static_cast<rti::UnsignedInteger32>(2U),
      static_cast<rti::UnsignedInteger32>(17U),
      "HLAunsignedInteger32BE");
  requireValueAssignmentAndConversion<rti::HLAunsignedInteger32LE>(
      static_cast<rti::UnsignedInteger32>(2U),
      static_cast<rti::UnsignedInteger32>(17U),
      "HLAunsignedInteger32LE");
  requireValueAssignmentAndConversion<rti::HLAunsignedInteger64BE>(
      static_cast<rti::UnsignedInteger64>(2U),
      static_cast<rti::UnsignedInteger64>(17U),
      "HLAunsignedInteger64BE");
  requireValueAssignmentAndConversion<rti::HLAunsignedInteger64LE>(
      static_cast<rti::UnsignedInteger64>(2U),
      static_cast<rti::UnsignedInteger64>(17U),
      "HLAunsignedInteger64LE");

  requireValueAssignmentAndConversion<rti::HLAoctetPairBE>(
      rti::OctetPair{octetFirst, octetSecond},
      rti::OctetPair{octetSecond, octetFirst},
      "HLAoctetPairBE");
  requireValueAssignmentAndConversion<rti::HLAoctetPairLE>(
      rti::OctetPair{octetFirst, octetSecond},
      rti::OctetPair{octetSecond, octetFirst},
      "HLAoctetPairLE");
  requireValueAssignmentAndConversion<rti::HLAunicodeChar>(
      L'\u03a9', L'\u2603', "HLAunicodeChar");
  requireValueAssignmentAndConversion<rti::HLAunicodeString>(
      std::wstring{L"Amber"}, std::wstring{L"1516"}, "HLAunicodeString");
}

}  // namespace hla_rti_cpp_tck::detail
