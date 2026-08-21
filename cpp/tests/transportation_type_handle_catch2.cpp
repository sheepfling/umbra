#include <catch2/catch_test_macros.hpp>

#include "internal/transportation_type_handle.hpp"

#include <array>
#include <cstring>
#include <sstream>
#include <string>

#include <RTI/Exception.h>

namespace {

using rti1516_2025::CouldNotDecode;
using rti1516_2025::CouldNotEncode;
using rti1516_2025::TransportationTypeHandle;
using rti1516_2025::VariableLengthData;
using rti1516_2025::umbra_binding_detail::decodeTransportationTypeHandle;
using rti1516_2025::umbra_binding_detail::makeTransportationTypeHandle;
using rti1516_2025::umbra_binding_detail::standardTransportationTypeName;
using rti1516_2025::umbra_binding_detail::standardTransportationTypeValue;
using rti1516_2025::umbra_binding_detail::transportationTypeHandleValue;

}  // namespace

TEST_CASE(
    "The official TransportationTypeHandle has stable embedded value semantics",
    "[unit][kernel][transportation-type-handle][foundation]") {
  TransportationTypeHandle invalid;
  auto const handle = makeTransportationTypeHandle(0x0102030405060708ULL);
  TransportationTypeHandle copy(handle);

  REQUIRE_FALSE(invalid.isValid());
  REQUIRE(handle.isValid());
  REQUIRE(copy == handle);
  REQUIRE(invalid != handle);
  REQUIRE(invalid < handle);
  REQUIRE(copy.hash() == handle.hash());
  REQUIRE(handle.toString() == L"TransportationTypeHandle(72623859790382856)");
  REQUIRE_FALSE(transportationTypeHandleValue(invalid).has_value());
  REQUIRE(transportationTypeHandleValue(handle) == 0x0102030405060708ULL);

  std::wostringstream stream;
  stream << handle;
  REQUIRE(stream.str() == handle.toString());
}

TEST_CASE(
    "The embedded TransportationTypeHandle encoding is an HLAvariableArray-wrapped identity",
    "[unit][kernel][transportation-type-handle][foundation]") {
  auto const handle = makeTransportationTypeHandle(0x0102030405060708ULL);
  std::array<unsigned char, 12> const expected{
      0x00, 0x00, 0x00, 0x08, 0x01, 0x02,
      0x03, 0x04, 0x05, 0x06, 0x07, 0x08};

  VariableLengthData encoded = handle.encode();
  REQUIRE(encoded.size() == expected.size());
  REQUIRE(std::memcmp(encoded.data(), expected.data(), expected.size()) == 0);

  std::array<unsigned char, 12> destination{};
  REQUIRE(handle.encode(destination.data(), destination.size()) == destination.size());
  REQUIRE(destination == expected);

  VariableLengthData reused;
  handle.encode(reused);
  REQUIRE(std::memcmp(reused.data(), expected.data(), expected.size()) == 0);
  REQUIRE(decodeTransportationTypeHandle(encoded) == handle);
}

TEST_CASE(
    "The embedded TransportationTypeHandle rejects malformed encodings and buffers",
    "[unit][kernel][transportation-type-handle][foundation]") {
  auto const handle = makeTransportationTypeHandle(1);
  std::array<unsigned char, 8> tooSmall{};
  VariableLengthData malformed(tooSmall.data(), tooSmall.size());

  REQUIRE_THROWS_AS(handle.encode(tooSmall.data(), tooSmall.size()), CouldNotEncode);
  REQUIRE_THROWS_AS(handle.encode(nullptr, handle.encodedLength()), CouldNotEncode);
  REQUIRE_THROWS_AS(decodeTransportationTypeHandle(malformed), CouldNotDecode);
}

TEST_CASE(
    "The embedded profile recognizes exactly the two mandatory transportation names",
    "[unit][kernel][transportation-type-handle][foundation]") {
  auto const reliable = standardTransportationTypeValue(L"HLAreliable");
  auto const bestEffort = standardTransportationTypeValue(L"HLAbestEffort");

  REQUIRE(reliable);
  REQUIRE(bestEffort);
  REQUIRE(*reliable != *bestEffort);
  auto const reliableName = standardTransportationTypeName(*reliable);
  auto const bestEffortName = standardTransportationTypeName(*bestEffort);
  REQUIRE(reliableName);
  REQUIRE(bestEffortName);
  REQUIRE(std::wstring(*reliableName) == L"HLAreliable");
  REQUIRE(std::wstring(*bestEffortName) == L"HLAbestEffort");
  REQUIRE_FALSE(standardTransportationTypeValue(L"UmbraCustomTransport").has_value());
  REQUIRE_FALSE(standardTransportationTypeName(9999).has_value());
}
