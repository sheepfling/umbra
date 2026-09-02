#include "internal/federation/transport_protocol.hpp"

#include <algorithm>
#include <array>
#include <limits>

namespace umbra::detail {
namespace {

constexpr std::array<std::uint8_t, 4U> kTransportMagic{'U', 'M', 'T', 'R'};
constexpr std::size_t kHandshakeSessionIdBytes = sizeof(std::uint64_t);

[[noreturn]] void fail(char const* message) {
  throw TransportProtocolError(message);
}

void appendUnsigned16(std::vector<std::uint8_t>& bytes, std::uint16_t value) {
  bytes.push_back(static_cast<std::uint8_t>((value >> 8U) & 0xffU));
  bytes.push_back(static_cast<std::uint8_t>(value & 0xffU));
}

void appendUnsigned32(std::vector<std::uint8_t>& bytes, std::uint32_t value) {
  bytes.push_back(static_cast<std::uint8_t>((value >> 24U) & 0xffU));
  bytes.push_back(static_cast<std::uint8_t>((value >> 16U) & 0xffU));
  bytes.push_back(static_cast<std::uint8_t>((value >> 8U) & 0xffU));
  bytes.push_back(static_cast<std::uint8_t>(value & 0xffU));
}

void appendUnsigned64(std::vector<std::uint8_t>& bytes, std::uint64_t value) {
  for (std::size_t shift = 56U; shift != 0U; shift -= 8U) {
    bytes.push_back(static_cast<std::uint8_t>((value >> shift) & 0xffU));
  }
  bytes.push_back(static_cast<std::uint8_t>(value & 0xffU));
}

[[nodiscard]] std::uint16_t readUnsigned16(
    std::span<std::uint8_t const> bytes,
    std::size_t offset) {
  return static_cast<std::uint16_t>(
      (static_cast<std::uint16_t>(bytes[offset]) << 8U) |
      static_cast<std::uint16_t>(bytes[offset + 1U]));
}

[[nodiscard]] std::uint32_t readUnsigned32(
    std::span<std::uint8_t const> bytes,
    std::size_t offset) {
  return (static_cast<std::uint32_t>(bytes[offset]) << 24U) |
      (static_cast<std::uint32_t>(bytes[offset + 1U]) << 16U) |
      (static_cast<std::uint32_t>(bytes[offset + 2U]) << 8U) |
      static_cast<std::uint32_t>(bytes[offset + 3U]);
}

[[nodiscard]] std::uint64_t readUnsigned64(
    std::span<std::uint8_t const> bytes,
    std::size_t offset) {
  std::uint64_t value = 0U;
  for (std::size_t index = 0U; index < sizeof(std::uint64_t); ++index) {
    value = (value << 8U) | static_cast<std::uint64_t>(bytes[offset + index]);
  }
  return value;
}

void validateEndpointId(std::string const& endpointId) {
  if (endpointId.empty() || endpointId.size() > kTransportMaximumEndpointIdBytes) {
    fail("The transport endpoint identity must be nonempty and at most 256 bytes.");
  }
  if (std::find(endpointId.begin(), endpointId.end(), '\0') != endpointId.end()) {
    fail("The transport endpoint identity cannot contain a NUL byte.");
  }
}

[[nodiscard]] TransportFrame makeHandshakeFrame(
    TransportFrameKind kind,
    TransportEndpointIdentity const& identity) {
  validateEndpointId(identity.endpointId);
  if (identity.endpointId.size() > std::numeric_limits<std::uint16_t>::max()) {
    fail("The transport endpoint identity length does not fit the handshake field.");
  }

  std::vector<std::uint8_t> payload;
  payload.reserve(2U + identity.endpointId.size() + kHandshakeSessionIdBytes);
  appendUnsigned16(payload, static_cast<std::uint16_t>(identity.endpointId.size()));
  payload.insert(payload.end(), identity.endpointId.begin(), identity.endpointId.end());
  appendUnsigned64(payload, identity.sessionId);
  return TransportFrame{kind, std::move(payload)};
}

}  // namespace

bool isTransportFrameKind(TransportFrameKind kind) noexcept {
  switch (kind) {
    case TransportFrameKind::hello:
    case TransportFrameKind::hello_ack:
    case TransportFrameKind::data:
    case TransportFrameKind::close:
      return true;
  }
  return false;
}

std::vector<std::uint8_t> encodeTransportFrame(TransportFrame const& frame) {
  if (!isTransportFrameKind(frame.kind)) {
    fail("The transport frame kind is not recognized.");
  }
  if (frame.payload.size() > kTransportMaximumPayloadBytes ||
      frame.payload.size() > std::numeric_limits<std::uint32_t>::max()) {
    fail("The transport frame payload exceeds the protocol maximum.");
  }

  std::vector<std::uint8_t> encoded;
  encoded.reserve(kTransportFrameHeaderSize + frame.payload.size());
  encoded.insert(encoded.end(), kTransportMagic.begin(), kTransportMagic.end());
  appendUnsigned16(encoded, kTransportProtocolVersion);
  appendUnsigned16(encoded, static_cast<std::uint16_t>(frame.kind));
  appendUnsigned32(encoded, static_cast<std::uint32_t>(frame.payload.size()));
  encoded.insert(encoded.end(), frame.payload.begin(), frame.payload.end());
  return encoded;
}

TransportFrame decodeTransportFrame(std::span<std::uint8_t const> encoded) {
  if (encoded.size() < kTransportFrameHeaderSize) {
    fail("The transport frame is shorter than its header.");
  }
  if (!std::equal(kTransportMagic.begin(), kTransportMagic.end(), encoded.begin())) {
    fail("The transport frame magic is invalid.");
  }
  if (readUnsigned16(encoded, 4U) != kTransportProtocolVersion) {
    fail("The transport protocol version is unsupported.");
  }

  auto const kind = static_cast<TransportFrameKind>(readUnsigned16(encoded, 6U));
  if (!isTransportFrameKind(kind)) {
    fail("The transport frame kind is not recognized.");
  }
  auto const payloadSize = readUnsigned32(encoded, 8U);
  if (payloadSize > kTransportMaximumPayloadBytes) {
    fail("The transport frame payload exceeds the protocol maximum.");
  }
  if (encoded.size() != kTransportFrameHeaderSize + payloadSize) {
    fail("The transport frame length does not match its payload.");
  }

  auto const payloadBegin = encoded.begin() +
      static_cast<std::ptrdiff_t>(kTransportFrameHeaderSize);
  auto const payloadEnd = payloadBegin + static_cast<std::ptrdiff_t>(payloadSize);
  return TransportFrame{
      kind,
      std::vector<std::uint8_t>(payloadBegin, payloadEnd)};
}

TransportFrame makeTransportHello(TransportEndpointIdentity const& identity) {
  return makeHandshakeFrame(TransportFrameKind::hello, identity);
}

TransportFrame makeTransportHelloAck(TransportEndpointIdentity const& identity) {
  return makeHandshakeFrame(TransportFrameKind::hello_ack, identity);
}

TransportEndpointIdentity decodeTransportHandshake(TransportFrame const& frame) {
  if (frame.kind != TransportFrameKind::hello &&
      frame.kind != TransportFrameKind::hello_ack) {
    fail("Only hello and hello-ack frames carry endpoint identities.");
  }
  if (frame.payload.size() < 2U + kHandshakeSessionIdBytes) {
    fail("The transport handshake payload is truncated.");
  }

  std::span<std::uint8_t const> payload(frame.payload);
  auto const endpointIdSize = readUnsigned16(payload, 0U);
  auto const expectedSize = 2U + static_cast<std::size_t>(endpointIdSize) +
      kHandshakeSessionIdBytes;
  if (endpointIdSize == 0U || endpointIdSize > kTransportMaximumEndpointIdBytes ||
      payload.size() != expectedSize) {
    fail("The transport handshake endpoint identity length is invalid.");
  }

  std::string endpointId(
      payload.begin() + 2,
      payload.begin() + 2 + static_cast<std::ptrdiff_t>(endpointIdSize));
  validateEndpointId(endpointId);
  return TransportEndpointIdentity{
      std::move(endpointId),
      readUnsigned64(payload, 2U + endpointIdSize)};
}

}  // namespace umbra::detail
