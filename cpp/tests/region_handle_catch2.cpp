#include <catch2/catch_test_macros.hpp>

#include "internal/handles/region_handle.hpp"

#include <array>
#include <cstring>
#include <sstream>

#include <RTI/Exception.h>
#include <RTI/RangeBounds.h>

namespace {

using rti1516_2025::CouldNotDecode;
using rti1516_2025::CouldNotEncode;
using rti1516_2025::RangeBounds;
using rti1516_2025::RegionHandle;
using rti1516_2025::VariableLengthData;
using rti1516_2025::umbra_binding_detail::decodeRegionHandle;
using rti1516_2025::umbra_binding_detail::makeRegionHandle;
using rti1516_2025::umbra_binding_detail::regionHandleValue;

}  // namespace

TEST_CASE(
    "The official RegionHandle has stable embedded value semantics",
    "[unit][kernel][region-handle][foundation][ddm]") {
  RegionHandle invalid;
  auto const handle = makeRegionHandle(0x0102030405060708ULL);
  RegionHandle copy(handle);

  REQUIRE_FALSE(invalid.isValid());
  REQUIRE(handle.isValid());
  REQUIRE(copy == handle);
  REQUIRE(invalid != handle);
  REQUIRE(invalid < handle);
  REQUIRE(copy.hash() == handle.hash());
  REQUIRE(handle.toString() == L"RegionHandle(72623859790382856)");
  REQUIRE_FALSE(regionHandleValue(invalid).has_value());
  REQUIRE(regionHandleValue(handle) == 0x0102030405060708ULL);

  std::wostringstream stream;
  stream << handle;
  REQUIRE(stream.str() == handle.toString());
}

TEST_CASE(
    "The embedded RegionHandle encoding is an HLAvariableArray-wrapped identity",
    "[unit][kernel][region-handle][foundation][ddm]") {
  auto const handle = makeRegionHandle(0x0102030405060708ULL);
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
  REQUIRE(decodeRegionHandle(encoded) == handle);
}

TEST_CASE(
    "The embedded RegionHandle rejects malformed encodings and buffers",
    "[unit][kernel][region-handle][foundation][ddm]") {
  auto const handle = makeRegionHandle(1);
  std::array<unsigned char, 8> tooSmall{};
  VariableLengthData malformed(tooSmall.data(), tooSmall.size());

  REQUIRE_THROWS_AS(handle.encode(tooSmall.data(), tooSmall.size()), CouldNotEncode);
  REQUIRE_THROWS_AS(handle.encode(nullptr, handle.encodedLength()), CouldNotEncode);
  REQUIRE_THROWS_AS(decodeRegionHandle(malformed), CouldNotDecode);
}

TEST_CASE(
    "The official RangeBounds preserves the lower and upper values",
    "[unit][kernel][range-bounds][foundation][ddm]") {
  RangeBounds defaults;
  REQUIRE(defaults.getLowerBound() == 0UL);
  REQUIRE(defaults.getUpperBound() == 0UL);

  RangeBounds bounds(10UL, 20UL);
  REQUIRE(bounds.getLowerBound() == 10UL);
  REQUIRE(bounds.getUpperBound() == 20UL);

  bounds.setLowerBound(5UL);
  bounds.setUpperBound(25UL);
  REQUIRE(bounds.getLowerBound() == 5UL);
  REQUIRE(bounds.getUpperBound() == 25UL);

  RangeBounds copy(bounds);
  RangeBounds assigned;
  assigned = copy;
  REQUIRE(copy.getLowerBound() == 5UL);
  REQUIRE(copy.getUpperBound() == 25UL);
  REQUIRE(assigned.getLowerBound() == 5UL);
  REQUIRE(assigned.getUpperBound() == 25UL);
}
