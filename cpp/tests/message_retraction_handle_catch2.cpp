#include <catch2/catch_test_macros.hpp>

#include "internal/message_retraction_handle.hpp"

#include <RTI/VariableLengthData.h>

#include <array>
#include <cstring>
#include <cstdint>

namespace {

using rti1516_2025::MessageRetractionHandle;
using rti1516_2025::VariableLengthData;
using rti1516_2025::umbra_binding_detail::decodeMessageRetractionHandle;
using rti1516_2025::umbra_binding_detail::makeMessageRetractionHandle;
using rti1516_2025::umbra_binding_detail::messageRetractionHandleValue;

TEST_CASE(
    "MessageRetractionHandle uses the standard HLAvariableArray handle encoding",
    "[unit][handles][message-retraction][foundation][time-management][retract]") {
  MessageRetractionHandle const invalid;
  REQUIRE_FALSE(invalid.isValid());
  REQUIRE_FALSE(messageRetractionHandleValue(invalid));
  REQUIRE(invalid.encode().size() == 12);

  auto const handle = makeMessageRetractionHandle(0x0102030405060708ULL);
  REQUIRE(handle.isValid());
  REQUIRE(messageRetractionHandleValue(handle) == 0x0102030405060708ULL);
  REQUIRE(handle.encodedLength() == 12);

  auto const encoded = handle.encode();
  std::array<unsigned char, 12> const expected{
      0x00, 0x00, 0x00, 0x08, 0x01, 0x02,
      0x03, 0x04, 0x05, 0x06, 0x07, 0x08};
  REQUIRE(encoded.size() == expected.size());
  REQUIRE(std::memcmp(encoded.data(), expected.data(), expected.size()) == 0);
  auto const decoded = decodeMessageRetractionHandle(encoded);
  REQUIRE(decoded == handle);
  REQUIRE(decoded.toString() == L"MessageRetractionHandle(72623859790382856)");
}

TEST_CASE(
    "MessageRetractionHandle rejects malformed HLAvariableArray encodings",
    "[unit][handles][message-retraction][foundation][time-management]") {
  std::array<unsigned char, 8> const bareIdentity{
      0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};
  std::array<unsigned char, 12> const wrongElementCount{
      0x00, 0x00, 0x00, 0x07, 0x01, 0x02,
      0x03, 0x04, 0x05, 0x06, 0x07, 0x08};
  REQUIRE_THROWS_AS(
      decodeMessageRetractionHandle(VariableLengthData()),
      rti1516_2025::CouldNotDecode);
  REQUIRE_THROWS_AS(
      decodeMessageRetractionHandle(
          VariableLengthData(bareIdentity.data(), bareIdentity.size())),
      rti1516_2025::CouldNotDecode);
  REQUIRE_THROWS_AS(
      decodeMessageRetractionHandle(
          VariableLengthData(wrongElementCount.data(), wrongElementCount.size())),
      rti1516_2025::CouldNotDecode);
}

}  // namespace
