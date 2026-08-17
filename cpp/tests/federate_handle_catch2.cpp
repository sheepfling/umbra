#include <catch2/catch_test_macros.hpp>

#include "internal/federate_handle.hpp"

#include <array>
#include <cstring>
#include <sstream>

namespace {

using rti1516_2025::CouldNotDecode;
using rti1516_2025::CouldNotEncode;
using rti1516_2025::FederateHandle;
using rti1516_2025::VariableLengthData;
using rti1516_2025::umbra_binding_detail::decodeFederateHandle;
using rti1516_2025::umbra_binding_detail::federateHandleValue;
using rti1516_2025::umbra_binding_detail::makeFederateHandle;

}  // namespace

TEST_CASE("The official FederateHandle has stable embedded value semantics", "[unit][kernel][federate-handle]") {
  FederateHandle invalid;
  auto const handle = makeFederateHandle(0x0102030405060708ULL);
  FederateHandle copy(handle);

  REQUIRE_FALSE(invalid.isValid());
  REQUIRE(handle.isValid());
  REQUIRE(copy == handle);
  REQUIRE(invalid != handle);
  REQUIRE(invalid < handle);
  REQUIRE(copy.hash() == handle.hash());
  REQUIRE(handle.toString() == L"FederateHandle(72623859790382856)");
  REQUIRE_FALSE(federateHandleValue(invalid).has_value());
  REQUIRE(federateHandleValue(handle) == 0x0102030405060708ULL);

  std::wostringstream stream;
  stream << handle;
  REQUIRE(stream.str() == handle.toString());
}

TEST_CASE("The embedded FederateHandle encoding is a round-trippable eight-byte identity", "[unit][kernel][federate-handle]") {
  auto const handle = makeFederateHandle(0x0102030405060708ULL);
  std::array<unsigned char, 8> const expected{0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};

  VariableLengthData encoded = handle.encode();
  REQUIRE(encoded.size() == expected.size());
  REQUIRE(std::memcmp(encoded.data(), expected.data(), expected.size()) == 0);

  std::array<unsigned char, 8> destination{};
  REQUIRE(handle.encode(destination.data(), destination.size()) == destination.size());
  REQUIRE(destination == expected);

  VariableLengthData reused;
  handle.encode(reused);
  REQUIRE(std::memcmp(reused.data(), expected.data(), expected.size()) == 0);
  REQUIRE(decodeFederateHandle(encoded) == handle);
}

TEST_CASE("The embedded FederateHandle rejects malformed encodings and buffers", "[unit][kernel][federate-handle]") {
  auto const handle = makeFederateHandle(1);
  std::array<unsigned char, 7> tooSmall{};
  VariableLengthData malformed(tooSmall.data(), tooSmall.size());

  REQUIRE_THROWS_AS(handle.encode(tooSmall.data(), tooSmall.size()), CouldNotEncode);
  REQUIRE_THROWS_AS(handle.encode(nullptr, handle.encodedLength()), CouldNotEncode);
  REQUIRE_THROWS_AS(decodeFederateHandle(malformed), CouldNotDecode);
}
