#include <catch2/catch_test_macros.hpp>

#include "internal/encoding/fnv1a.hpp"
#include "internal/encoding/variable_length_data_2025.hpp"
#include "internal/federation/transport_protocol.hpp"
#include "internal/federation/transport_service_protocol.hpp"

#include <RTI/VariableLengthData.h>

#include <array>
#include <cstdint>
#include <vector>

namespace {

using umbra::detail::TransportEndpointIdentity;
using umbra::detail::TransportFrame;
using umbra::detail::TransportFrameKind;
using umbra::detail::TransportProtocolError;
using umbra::detail::TransportServiceMessage;
using umbra::detail::TransportServiceMessageKind;
using umbra::detail::TransportServiceOperation;
using umbra::detail::TransportServiceProtocolError;
using umbra::detail::TransportServiceStatus;
using umbra::detail::decodeTransportFrame;
using umbra::detail::decodeTransportHandshake;
using umbra::detail::decodeTransportServiceMessage;
using umbra::detail::encodeTransportFrame;
using umbra::detail::encodeTransportServiceMessage;
using umbra::detail::kTransportFrameHeaderSize;
using umbra::detail::kTransportServiceHeaderSize;
using umbra::detail::makeTransportHello;
using umbra::detail::makeTransportHelloAck;

}  // namespace

TEST_CASE(
    "Private transport framing round-trips identities and rejects malformed process-boundary frames",
    "[unit][foundation][transport][process-boundary][transport-contract]") {
  TransportEndpointIdentity const identity{
      "federate-process-17",
      0x0102030405060708ULL};

  for (auto const handshake : {makeTransportHello(identity), makeTransportHelloAck(identity)}) {
    auto const encoded = encodeTransportFrame(handshake);
    REQUIRE(encoded.size() == kTransportFrameHeaderSize + handshake.payload.size());
    REQUIRE(std::array<std::uint8_t, 4U>{encoded[0], encoded[1], encoded[2], encoded[3]} ==
            std::array<std::uint8_t, 4U>{'U', 'M', 'T', 'R'});
    auto const decodedFrame = decodeTransportFrame(encoded);
    REQUIRE(decodedFrame.kind == handshake.kind);
    REQUIRE(decodedFrame.payload == handshake.payload);
    auto const decodedIdentity = decodeTransportHandshake(decodedFrame);
    REQUIRE(decodedIdentity.endpointId == identity.endpointId);
    REQUIRE(decodedIdentity.sessionId == identity.sessionId);
  }

  TransportFrame const dataFrame{
      TransportFrameKind::data,
      std::vector<std::uint8_t>{0x00, 0x10, 0x20, 0x7f, 0xff}};
  auto const dataEncoded = encodeTransportFrame(dataFrame);
  auto const dataDecoded = decodeTransportFrame(dataEncoded);
  REQUIRE(dataDecoded.kind == TransportFrameKind::data);
  REQUIRE(dataDecoded.payload == dataFrame.payload);

  auto truncated = dataEncoded;
  truncated.pop_back();
  REQUIRE_THROWS_AS(decodeTransportFrame(truncated), TransportProtocolError);

  auto badMagic = dataEncoded;
  badMagic[0] = 'X';
  REQUIRE_THROWS_AS(decodeTransportFrame(badMagic), TransportProtocolError);

  auto badVersion = dataEncoded;
  badVersion[5] = 0U;
  REQUIRE_THROWS_AS(decodeTransportFrame(badVersion), TransportProtocolError);

  auto badKind = dataEncoded;
  badKind[6] = 0xffU;
  badKind[7] = 0xffU;
  REQUIRE_THROWS_AS(decodeTransportFrame(badKind), TransportProtocolError);

  auto oversized = dataEncoded;
  oversized[8] = 0xffU;
  oversized[9] = 0xffU;
  oversized[10] = 0xffU;
  oversized[11] = 0xffU;
  REQUIRE_THROWS_AS(decodeTransportFrame(oversized), TransportProtocolError);

  REQUIRE_THROWS_AS(decodeTransportHandshake(dataDecoded), TransportProtocolError);

  auto truncatedHandshake = makeTransportHello(identity);
  truncatedHandshake.payload.resize(2U);
  REQUIRE_THROWS_AS(
      decodeTransportHandshake(truncatedHandshake),
      TransportProtocolError);
}

TEST_CASE(
    "Private transport service framing round-trips metadata and rejects malformed headers",
    "[unit][foundation][transport][process-boundary][transport-service-contract]") {
  TransportServiceMessage const request{
      TransportServiceMessageKind::request,
      TransportServiceOperation::get_federate_handle,
      TransportServiceStatus::ok,
      0x0102030405060708ULL,
      std::vector<std::uint8_t>{0x00, 0x10, 0x20, 0x7f}};

  auto const encoded = encodeTransportServiceMessage(request);
  REQUIRE(encoded.size() == kTransportServiceHeaderSize + request.payload.size());
  REQUIRE(std::array<std::uint8_t, 4U>{encoded[0], encoded[1], encoded[2], encoded[3]} ==
          std::array<std::uint8_t, 4U>{'U', 'M', 'S', 'V'});
  REQUIRE(encoded[4] == 0U);
  REQUIRE(encoded[5] == 1U);
  REQUIRE(std::array<std::uint8_t, 8U>{
              encoded[12], encoded[13], encoded[14], encoded[15],
              encoded[16], encoded[17], encoded[18], encoded[19]} ==
          std::array<std::uint8_t, 8U>{
              0x01U, 0x02U, 0x03U, 0x04U,
              0x05U, 0x06U, 0x07U, 0x08U});

  auto const decoded = decodeTransportServiceMessage(encoded);
  REQUIRE(decoded.kind == request.kind);
  REQUIRE(decoded.operation == request.operation);
  REQUIRE(decoded.status == request.status);
  REQUIRE(decoded.requestId == request.requestId);
  REQUIRE(decoded.payload == request.payload);

  auto badMagic = encoded;
  badMagic[0] = 'X';
  REQUIRE_THROWS_AS(
      decodeTransportServiceMessage(badMagic),
      TransportServiceProtocolError);

  auto badReserved = encoded;
  badReserved[7] = 1U;
  REQUIRE_THROWS_AS(
      decodeTransportServiceMessage(badReserved),
      TransportServiceProtocolError);

  auto badLength = encoded;
  badLength[23] = 5U;
  REQUIRE_THROWS_AS(
      decodeTransportServiceMessage(badLength),
      TransportServiceProtocolError);
}

TEST_CASE(
    "Private byte hashing uses the stable FNV-1a contract",
    "[unit][foundation][internal][encoding][hash]") {
  std::array<std::uint8_t, 5U> const source{'h', 'e', 'l', 'l', 'o'};

  REQUIRE(umbra::detail::fnv1aHash(nullptr, 0U) == 14695981039346656037ULL);
  REQUIRE(umbra::detail::fnv1aHash(source.data(), source.size()) ==
          0xa430d84680aabd0bULL);
}

TEST_CASE(
    "Private 2025 VariableLengthData copies preserve binary payloads and empty values",
    "[unit][foundation][internal][variable-length-data-2025]") {
  std::array<std::uint8_t, 4U> const source{0x00U, 0x10U, 0x80U, 0xffU};
  rti1516_2025::VariableLengthData const encoded(source.data(), source.size());

  auto const copied = umbra::detail::variable_length_data_2025::copyBytes(encoded);
  REQUIRE(copied == std::vector<std::uint8_t>{0x00U, 0x10U, 0x80U, 0xffU});
  REQUIRE(copied.data() != static_cast<std::uint8_t const*>(encoded.data()));

  auto const encodedCopied =
      umbra::detail::variable_length_data_2025::copyEncodedBytes(
          encoded, L"The encoded payload is invalid.");
  REQUIRE(encodedCopied == std::vector<rti1516_2025::Octet>{
                              static_cast<rti1516_2025::Octet>(0x00U),
                              static_cast<rti1516_2025::Octet>(0x10U),
                              static_cast<rti1516_2025::Octet>(0x80U),
                              static_cast<rti1516_2025::Octet>(0xffU)});

  std::vector<rti1516_2025::Octet> appended{
      static_cast<rti1516_2025::Octet>(0xaaU)};
  umbra::detail::variable_length_data_2025::appendEncodedBytes(
      appended, encoded, L"The encoded payload is invalid.");
  REQUIRE(appended == std::vector<rti1516_2025::Octet>{
                       static_cast<rti1516_2025::Octet>(0xaaU),
                       static_cast<rti1516_2025::Octet>(0x00U),
                       static_cast<rti1516_2025::Octet>(0x10U),
                       static_cast<rti1516_2025::Octet>(0x80U),
                       static_cast<rti1516_2025::Octet>(0xffU)});

  REQUIRE(umbra::detail::variable_length_data_2025::copyBytes(
              rti1516_2025::VariableLengthData{})
              .empty());
  REQUIRE(umbra::detail::variable_length_data_2025::copyEncodedBytes(
              rti1516_2025::VariableLengthData{},
              L"The encoded payload is invalid.")
              .empty());
}
