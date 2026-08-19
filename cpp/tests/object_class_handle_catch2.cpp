#include <catch2/catch_test_macros.hpp>

#include "internal/object_class_handle.hpp"

#include <array>
#include <cstring>
#include <sstream>

#include <RTI/Exception.h>

namespace {

using rti1516_2025::CouldNotDecode;
using rti1516_2025::CouldNotEncode;
using rti1516_2025::ObjectClassHandle;
using rti1516_2025::VariableLengthData;
using rti1516_2025::umbra_binding_detail::decodeObjectClassHandle;
using rti1516_2025::umbra_binding_detail::makeObjectClassHandle;
using rti1516_2025::umbra_binding_detail::objectClassHandleValue;

}  // namespace

TEST_CASE("The official ObjectClassHandle has stable embedded value semantics", "[unit][kernel][object-class-handle]") {
  ObjectClassHandle invalid;
  auto const handle = makeObjectClassHandle(0x0102030405060708ULL);
  ObjectClassHandle copy(handle);

  REQUIRE_FALSE(invalid.isValid());
  REQUIRE(handle.isValid());
  REQUIRE(copy == handle);
  REQUIRE(invalid != handle);
  REQUIRE(invalid < handle);
  REQUIRE(copy.hash() == handle.hash());
  REQUIRE(handle.toString() == L"ObjectClassHandle(72623859790382856)");
  REQUIRE_FALSE(objectClassHandleValue(invalid).has_value());
  REQUIRE(objectClassHandleValue(handle) == 0x0102030405060708ULL);

  std::wostringstream stream;
  stream << handle;
  REQUIRE(stream.str() == handle.toString());
}

TEST_CASE("The embedded ObjectClassHandle encoding is an HLAvariableArray-wrapped identity", "[unit][kernel][object-class-handle]") {
  auto const handle = makeObjectClassHandle(0x0102030405060708ULL);
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
  REQUIRE(decodeObjectClassHandle(encoded) == handle);
}

TEST_CASE("The embedded ObjectClassHandle rejects malformed encodings and buffers", "[unit][kernel][object-class-handle]") {
  auto const handle = makeObjectClassHandle(1);
  std::array<unsigned char, 8> tooSmall{};
  VariableLengthData malformed(tooSmall.data(), tooSmall.size());

  REQUIRE_THROWS_AS(handle.encode(tooSmall.data(), tooSmall.size()), CouldNotEncode);
  REQUIRE_THROWS_AS(handle.encode(nullptr, handle.encodedLength()), CouldNotEncode);
  REQUIRE_THROWS_AS(decodeObjectClassHandle(malformed), CouldNotDecode);
}
