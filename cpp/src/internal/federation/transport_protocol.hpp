#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <stdexcept>
#include <string>
#include <vector>

namespace umbra::detail {

// Message type values are the IEEE 1516.1-2025 Table 34 assignments. The
// process transport codecs implement the Table 33 header and the currently
// supported message-specific payloads.
enum class TransportFrameKind : std::uint32_t {
  new_session = 1U,
  new_session_status = 2U,
  heartbeat = 3U,
  heartbeat_response = 4U,
  close = 5U,
  close_ack = 6U,
  resume_request = 10U,
  resume_status = 11U,
  data = 20U,
  call_response = 21U,
  callback_request = 22U,
  callback_response = 23U,
};

enum class TransportNewSessionStatusCode : std::uint32_t {
  success = 0U,
  unsupported_protocol_version = 1U,
  out_of_resources = 2U,
  other_error = 99U,
};

struct TransportNewSessionStatus final {
  std::uint64_t sessionId = 0U;
  TransportNewSessionStatusCode status =
      TransportNewSessionStatusCode::other_error;
};

enum class TransportResumeStatusCode : std::uint32_t {
  ok = 0U,
  invalid_session = 1U,
  insufficient_buffered_messages = 2U,
  other_error = 99U,
};

inline constexpr std::uint32_t kInvalidTransportSequenceNumber = 0x80000000U;
inline constexpr std::uint32_t kMaximumTransportSequenceNumber = 0x7fffffffU;

struct TransportResumeRequest final {
  std::uint64_t sessionId = 0U;
  std::uint32_t lastReceivedRtiSequenceNumber = kInvalidTransportSequenceNumber;
  std::uint32_t oldestAvailableFederateSequenceNumber =
      kInvalidTransportSequenceNumber;
};

struct TransportResumeStatus final {
  std::uint64_t sessionId = 0U;
  TransportResumeStatusCode status = TransportResumeStatusCode::other_error;
  std::uint32_t lastReceivedFederateSequenceNumber =
      kInvalidTransportSequenceNumber;
};

struct TransportFrame final {
  TransportFrameKind kind = TransportFrameKind::data;
  std::vector<std::uint8_t> payload;
  std::uint32_t sequenceNumber = kInvalidTransportSequenceNumber;
  std::uint64_t sessionId = 0U;
  std::uint32_t lastReceivedMessageNumber = kInvalidTransportSequenceNumber;
};

struct TransportEndpointIdentity final {
  std::string endpointId;
  std::uint64_t sessionId = 0U;
};

class TransportProtocolError final : public std::runtime_error {
 public:
  using std::runtime_error::runtime_error;
};

inline constexpr std::size_t kTransportFrameHeaderSize = 24U;
inline constexpr std::size_t kTransportNewSessionPayloadSize = 4U;
inline constexpr std::size_t kTransportHeartbeatResponsePayloadSize = 4U;
inline constexpr std::uint32_t kTransportHla4ProtocolVersion = 1U;
inline constexpr std::size_t kTransportResumePayloadSize = 8U;
inline constexpr std::uint32_t kTransportMaximumPayloadBytes = 4U * 1024U * 1024U;
inline constexpr std::size_t kTransportMaximumEndpointIdBytes = 256U;

[[nodiscard]] bool isTransportFrameKind(TransportFrameKind kind) noexcept;

[[nodiscard]] bool isTransportServiceFrameKind(TransportFrameKind kind) noexcept;

[[nodiscard]] bool isTransportNewSessionStatusCode(
    TransportNewSessionStatusCode status) noexcept;

[[nodiscard]] std::vector<std::uint8_t> encodeTransportFrame(
    TransportFrame const& frame);

[[nodiscard]] TransportFrame decodeTransportFrame(
    std::span<std::uint8_t const> encoded);

[[nodiscard]] TransportFrame makeTransportNewSessionRequest(
    std::uint32_t protocolVersion = kTransportHla4ProtocolVersion);

[[nodiscard]] std::uint32_t decodeTransportNewSessionRequest(
    TransportFrame const& frame);

[[nodiscard]] TransportFrame makeTransportNewSessionStatus(
    TransportNewSessionStatus const& status,
    std::uint32_t sequenceNumber);

[[nodiscard]] TransportNewSessionStatus decodeTransportNewSessionStatus(
    TransportFrame const& frame);

[[nodiscard]] bool isTransportResumeStatusCode(
    TransportResumeStatusCode status) noexcept;

[[nodiscard]] TransportFrame makeTransportResumeRequest(
    TransportResumeRequest const& request,
    std::uint32_t sequenceNumber);

[[nodiscard]] TransportResumeRequest decodeTransportResumeRequest(
    TransportFrame const& frame);

[[nodiscard]] TransportFrame makeTransportResumeStatus(
    TransportResumeStatus const& status,
    std::uint32_t sequenceNumber);

[[nodiscard]] TransportResumeStatus decodeTransportResumeStatus(
    TransportFrame const& frame);

[[nodiscard]] TransportFrame makeTransportHeartbeat(
    std::uint64_t sessionId,
    std::uint32_t sequenceNumber,
    std::uint32_t lastReceivedMessageNumber);

[[nodiscard]] TransportFrame makeTransportHeartbeatResponse(
    std::uint64_t sessionId,
    std::uint32_t sequenceNumber,
    std::uint32_t lastReceivedMessageNumber,
    std::uint32_t responseToSequenceNumber);

[[nodiscard]] TransportFrame makeTransportCloseRequest(
    std::uint64_t sessionId,
    std::uint32_t sequenceNumber,
    std::uint32_t lastReceivedMessageNumber);

[[nodiscard]] TransportFrame makeTransportCloseAcknowledgement(
    std::uint64_t sessionId,
    std::uint32_t sequenceNumber,
    std::uint32_t lastReceivedMessageNumber);

[[nodiscard]] std::uint32_t decodeTransportHeartbeatResponse(
    TransportFrame const& frame);

}  // namespace umbra::detail
