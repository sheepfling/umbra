#include <catch2/catch_test_macros.hpp>

#include "internal/attribute_handle.hpp"

#include <array>
#include <cstring>
#include <sstream>

#include <RTI/Exception.h>

namespace {

using rti1516_2025::AttributeHandle;
using rti1516_2025::CouldNotDecode;
using rti1516_2025::CouldNotEncode;
using rti1516_2025::VariableLengthData;
using rti1516_2025::umbra_binding_detail::attributeHandleValue;
using rti1516_2025::umbra_binding_detail::decodeAttributeHandle;
using rti1516_2025::umbra_binding_detail::makeAttributeHandle;

}  // namespace

TEST_CASE("The official AttributeHandle has stable embedded value semantics", "[unit][kernel][attribute-handle][foundation]") {
  AttributeHandle invalid;
  auto const handle = makeAttributeHandle(0x0102030405060708ULL);
  AttributeHandle copy(handle);

  REQUIRE_FALSE(invalid.isValid());
  REQUIRE(handle.isValid());
  REQUIRE(copy == handle);
  REQUIRE(invalid != handle);
  REQUIRE(invalid < handle);
  REQUIRE(copy.hash() == handle.hash());
  REQUIRE(handle.toString() == L"AttributeHandle(72623859790382856)");
  REQUIRE_FALSE(attributeHandleValue(invalid).has_value());
  REQUIRE(attributeHandleValue(handle) == 0x0102030405060708ULL);

  std::wostringstream stream;
  stream << handle;
  REQUIRE(stream.str() == handle.toString());
}

TEST_CASE(
    "The embedded AttributeHandle encoding is an HLAvariableArray-wrapped identity",
    "[unit][kernel][attribute-handle][foundation]") {
  auto const handle = makeAttributeHandle(0x0102030405060708ULL);
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
  REQUIRE(decodeAttributeHandle(encoded) == handle);
}

TEST_CASE(
    "The embedded AttributeHandle rejects malformed encodings and buffers",
    "[unit][kernel][attribute-handle][foundation]") {
  auto const handle = makeAttributeHandle(1);
  std::array<unsigned char, 8> tooSmall{};
  VariableLengthData malformed(tooSmall.data(), tooSmall.size());

  REQUIRE_THROWS_AS(handle.encode(tooSmall.data(), tooSmall.size()), CouldNotEncode);
  REQUIRE_THROWS_AS(handle.encode(nullptr, handle.encodedLength()), CouldNotEncode);
  REQUIRE_THROWS_AS(decodeAttributeHandle(malformed), CouldNotDecode);
}
