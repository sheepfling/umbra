#include <catch2/catch_test_macros.hpp>

#include "internal/fom/fom_wire_encoding.hpp"

#include <RTI/VariableLengthData.h>
#include <RTI/encoding/BasicDataElements.h>
#include <RTI/encoding/HLAfixedRecord.h>
#include <RTI/encoding/HLAvariableArray.h>

#include <initializer_list>
#include <string_view>
#include <vector>

namespace {

using umbra::detail::FomByteOrder;
using umbra::detail::FomSourceCompatibility;
using umbra::detail::FomWireCodecStatus;
using umbra::detail::FomWireEncodingDescriptor;
using umbra::detail::FomWireEncodingKind;
using umbra::detail::normalizeFomWireEncoding;

std::vector<rti1516_2025::Octet> octets(
    rti1516_2025::VariableLengthData const& value) {
  auto const* data = static_cast<rti1516_2025::Octet const*>(value.data());
  return data == nullptr
      ? std::vector<rti1516_2025::Octet>{}
      : std::vector<rti1516_2025::Octet>(data, data + value.size());
}

std::vector<rti1516_2025::Octet> byteValues(
    std::initializer_list<unsigned int> values) {
  std::vector<rti1516_2025::Octet> result;
  result.reserve(values.size());
  for (auto const value : values) {
    result.push_back(static_cast<rti1516_2025::Octet>(value));
  }
  return result;
}

void requireSameStandardDescriptor(
    FomWireEncodingDescriptor const& expected,
    FomWireEncodingDescriptor const& actual) {
  REQUIRE(actual.sourceLabel == expected.sourceLabel);
  REQUIRE(actual.kind == expected.kind);
  REQUIRE(actual.codecStatus == expected.codecStatus);
  REQUIRE(actual.sizeBits == expected.sizeBits);
  REQUIRE(actual.byteOrder == expected.byteOrder);
  REQUIRE(actual.alignmentOctets == expected.alignmentOctets);
  REQUIRE(actual.elementCountInPayload == expected.elementCountInPayload);
  REQUIRE(actual.sentinelTerminated == expected.sentinelTerminated);
  REQUIRE(actual.extentSuppliedExternally == expected.extentSuppliedExternally);
  REQUIRE(actual.extensionLengthInPayload == expected.extensionLengthInPayload);
}

}  // namespace

TEST_CASE(
    "The RPR source profile is inert for standard wire-shape descriptors",
    "[unit][fom][rpr][isolation][standard-rti]") {
  for (auto const label : {
           std::string_view{"HLAfixedArray"},
           std::string_view{"HLAvariableArray"},
           std::string_view{"HLAfixedRecord"},
           std::string_view{"HLAvariantRecord"},
           std::string_view{"HLAextendableVariantRecord"},
       }) {
    auto const strict = normalizeFomWireEncoding(
        label,
        FomSourceCompatibility::strict);
    auto const rprProfile = normalizeFomWireEncoding(
        label,
        FomSourceCompatibility::rpr_2010);
    requireSameStandardDescriptor(strict, rprProfile);
  }

  auto const strictRpr = normalizeFomWireEncoding(
      "RPRlengthlessArray",
      FomSourceCompatibility::strict);
  REQUIRE(strictRpr.kind == FomWireEncodingKind::unrecognized);
  REQUIRE(strictRpr.codecStatus == FomWireCodecStatus::unsupported);

  // The neutral mapper can describe an RPR source shape for catalog review,
  // but it must not claim a runtime codec. That promotion belongs to the
  // validation-only RPR adapter and is therefore absent from this standard
  // RTI dependency cone.
  auto const neutralRpr = normalizeFomWireEncoding(
      "RPRlengthlessArray",
      FomSourceCompatibility::rpr_2010);
  REQUIRE(neutralRpr.kind == FomWireEncodingKind::lengthless_array);
  REQUIRE(neutralRpr.codecStatus == FomWireCodecStatus::metadata_only);
  REQUIRE(neutralRpr.extentSuppliedExternally);

  auto const neutralUnsigned = normalizeFomWireEncoding(
      "RPRunsignedInteger64BE",
      FomSourceCompatibility::rpr_2010);
  REQUIRE(neutralUnsigned.kind == FomWireEncodingKind::unrecognized);
  REQUIRE(neutralUnsigned.codecStatus == FomWireCodecStatus::unsupported);
}

TEST_CASE(
    "The public 2025 RTI keeps standard basic and composite encodings unchanged",
    "[baseline][encoding][unit][foundation][rpr][isolation][standard-rti]") {
  using namespace rti1516_2025;

  HLAinteger32BE integer{0x10203040};
  REQUIRE(octets(integer.encode()) ==
          byteValues({0x10U, 0x20U, 0x30U, 0x40U}));

  HLAboolean boolean{true};
  REQUIRE(octets(boolean.encode()) ==
          byteValues({0U, 0U, 0U, 1U}));

  HLAASCIIstring text{"RPR-isolated"};
  auto expectedText = byteValues({0U, 0U, 0U, 12U});
  expectedText.insert(expectedText.end(), {
      static_cast<Octet>('R'),
      static_cast<Octet>('P'),
      static_cast<Octet>('R'),
      static_cast<Octet>('-'),
      static_cast<Octet>('i'),
      static_cast<Octet>('s'),
      static_cast<Octet>('o'),
      static_cast<Octet>('l'),
      static_cast<Octet>('a'),
      static_cast<Octet>('t'),
      static_cast<Octet>('e'),
      static_cast<Octet>('d'),
  });
  REQUIRE(octets(text.encode()) == expectedText);

  HLAfixedRecord record;
  record.appendElement(HLAoctet{static_cast<Octet>(0xa5U)});
  record.appendElement(boolean);
  REQUIRE(octets(record.encode()) == byteValues({
      0xa5U,
      0U, 0U, 0U,
      0U, 0U, 0U, 1U,
  }));

  HLAvariableArrayT<HLAinteger16BE> array;
  array.addElement(HLAinteger16BE{1}).addElement(HLAinteger16BE{2});
  REQUIRE(octets(array.encode()) ==
          byteValues({0U, 0U, 0U, 2U, 0U, 1U, 0U, 2U}));
}
