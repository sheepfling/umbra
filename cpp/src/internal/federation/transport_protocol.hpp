#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <stdexcept>
#include <string>
#include <vector>

namespace umbra::detail {

// This protocol is private wire plumbing for the future process-boundary
// transport. It is deliberately separate from the public HLA encodings: HLA
// service payloads will be carried in data frames after the endpoint
// handshake, while the frame header remains an Umbra-internal envelope.
enum class TransportFrameKind : std::uint16_t {
  hello = 1U,
  hello_ack = 2U,
  data = 3U,
  close = 4U,
};

struct TransportFrame final {
  TransportFrameKind kind = TransportFrameKind::data;
  std::vector<std::uint8_t> payload;
};

struct TransportEndpointIdentity final {
  std::string endpointId;
  std::uint64_t sessionId = 0U;
};

class TransportProtocolError final : public std::runtime_error {
 public:
  using std::runtime_error::runtime_error;
};

inline constexpr std::uint16_t kTransportProtocolVersion = 1U;
inline constexpr std::size_t kTransportFrameHeaderSize = 12U;
inline constexpr std::uint32_t kTransportMaximumPayloadBytes = 4U * 1024U * 1024U;
inline constexpr std::size_t kTransportMaximumEndpointIdBytes = 256U;

[[nodiscard]] bool isTransportFrameKind(TransportFrameKind kind) noexcept;

[[nodiscard]] std::vector<std::uint8_t> encodeTransportFrame(
    TransportFrame const& frame);

[[nodiscard]] TransportFrame decodeTransportFrame(
    std::span<std::uint8_t const> encoded);

[[nodiscard]] TransportFrame makeTransportHello(
    TransportEndpointIdentity const& identity);

[[nodiscard]] TransportFrame makeTransportHelloAck(
    TransportEndpointIdentity const& identity);

[[nodiscard]] TransportEndpointIdentity decodeTransportHandshake(
    TransportFrame const& frame);

}  // namespace umbra::detail
