#include <catch2/catch_test_macros.hpp>

#include "internal/federation/transport_protocol.hpp"

#include <array>
#include <cstdint>
#include <vector>

namespace {

using umbra::detail::TransportEndpointIdentity;
using umbra::detail::TransportFrame;
using umbra::detail::TransportFrameKind;
using umbra::detail::TransportProtocolError;
using umbra::detail::decodeTransportFrame;
using umbra::detail::decodeTransportHandshake;
using umbra::detail::encodeTransportFrame;
using umbra::detail::kTransportFrameHeaderSize;
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
