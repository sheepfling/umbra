#include <catch2/catch_test_macros.hpp>

#include "internal/message_retraction_handle.hpp"

#include <RTI/VariableLengthData.h>

#include <cstdint>

namespace {

using rti1516_2025::MessageRetractionHandle;
using rti1516_2025::VariableLengthData;
using rti1516_2025::umbra_binding_detail::decodeMessageRetractionHandle;
using rti1516_2025::umbra_binding_detail::makeMessageRetractionHandle;
using rti1516_2025::umbra_binding_detail::messageRetractionHandleValue;

TEST_CASE(
    "MessageRetractionHandle uses the official eight-byte value encoding",
    "[unit][handles][message-retraction]") {
  MessageRetractionHandle const invalid;
  REQUIRE_FALSE(invalid.isValid());
  REQUIRE_FALSE(messageRetractionHandleValue(invalid));
  REQUIRE(invalid.encode().size() == 8);

  auto const handle = makeMessageRetractionHandle(0x0102030405060708ULL);
  REQUIRE(handle.isValid());
  REQUIRE(messageRetractionHandleValue(handle) == 0x0102030405060708ULL);
  REQUIRE(handle.encodedLength() == 8);

  auto const encoded = handle.encode();
  REQUIRE(encoded.size() == 8);
  auto const decoded = decodeMessageRetractionHandle(encoded);
  REQUIRE(decoded == handle);
  REQUIRE(decoded.toString() == L"MessageRetractionHandle(72623859790382856)");
}

TEST_CASE(
    "MessageRetractionHandle rejects non-eight-byte encodings",
    "[unit][handles][message-retraction]") {
  REQUIRE_THROWS_AS(
      decodeMessageRetractionHandle(VariableLengthData()),
      rti1516_2025::CouldNotDecode);
}

}  // namespace
