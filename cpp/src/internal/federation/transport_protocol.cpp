#include "internal/federation/transport_protocol.hpp"

#include "internal/encoding/transport_wire.hpp"

#include <algorithm>
#include <limits>

namespace umbra::detail {
namespace {

[[noreturn]] void fail(char const* message) {
  throw TransportProtocolError(message);
}

void validateFrameMetadata(TransportFrame const& frame) {
  if (isTransportServiceFrameKind(frame.kind)) {
    if (frame.sequenceNumber > kMaximumTransportSequenceNumber ||
        frame.sessionId == 0U ||
        (frame.lastReceivedMessageNumber != kInvalidTransportSequenceNumber &&
         frame.lastReceivedMessageNumber > kMaximumTransportSequenceNumber)) {
      fail("The transport data frame reliability metadata is invalid.");
    }
    return;
  }
  if (frame.kind == TransportFrameKind::resume_request ||
      frame.kind == TransportFrameKind::resume_status) {
    if (frame.sequenceNumber > kMaximumTransportSequenceNumber ||
        frame.sessionId == 0U || frame.payload.size() != kTransportResumePayloadSize ||
        (frame.lastReceivedMessageNumber != kInvalidTransportSequenceNumber &&
         frame.lastReceivedMessageNumber > kMaximumTransportSequenceNumber)) {
      fail("The transport resume control metadata is invalid.");
    }

    std::span<std::uint8_t const> payload(frame.payload);
    auto const firstValue = transport_wire::readBigEndian32(payload, 0U);
    auto const secondValue = transport_wire::readBigEndian32(payload, 4U);
    if (frame.kind == TransportFrameKind::resume_request) {
      if ((firstValue != kInvalidTransportSequenceNumber &&
           firstValue > kMaximumTransportSequenceNumber) ||
          (secondValue != kInvalidTransportSequenceNumber &&
           secondValue > kMaximumTransportSequenceNumber) ||
          firstValue != frame.lastReceivedMessageNumber) {
        fail("The transport Resume Request sequence metadata is invalid.");
      }
    } else if (!isTransportResumeStatusCode(
                   static_cast<TransportResumeStatusCode>(firstValue)) ||
               (secondValue != kInvalidTransportSequenceNumber &&
                secondValue > kMaximumTransportSequenceNumber) ||
               secondValue != frame.lastReceivedMessageNumber) {
      fail("The transport Resume Status sequence metadata is invalid.");
    }
    return;
  }
  if (frame.kind == TransportFrameKind::new_session) {
    if (frame.sequenceNumber != kInvalidTransportSequenceNumber ||
        frame.sessionId != 0U ||
        frame.lastReceivedMessageNumber != kInvalidTransportSequenceNumber ||
        frame.payload.size() != kTransportNewSessionPayloadSize) {
      fail("The transport New Session control metadata is invalid.");
    }
    return;
  }
  if (frame.kind == TransportFrameKind::new_session_status) {
    if (frame.sequenceNumber > kMaximumTransportSequenceNumber ||
        frame.lastReceivedMessageNumber != kInvalidTransportSequenceNumber ||
        frame.payload.size() != kTransportNewSessionPayloadSize) {
      fail("The transport New Session Status control metadata is invalid.");
    }
    auto const status = static_cast<TransportNewSessionStatusCode>(
        transport_wire::readBigEndian32(frame.payload, 0U));
    if (!isTransportNewSessionStatusCode(status) ||
        ((status == TransportNewSessionStatusCode::success) !=
         (frame.sessionId != 0U))) {
      fail("The transport New Session Status payload or session ID is invalid.");
    }
    return;
  }
  if (frame.kind == TransportFrameKind::heartbeat) {
    if (frame.sequenceNumber > kMaximumTransportSequenceNumber ||
        frame.sessionId == 0U || !frame.payload.empty() ||
        (frame.lastReceivedMessageNumber != kInvalidTransportSequenceNumber &&
         frame.lastReceivedMessageNumber > kMaximumTransportSequenceNumber)) {
      fail("The transport heartbeat metadata is invalid.");
    }
    return;
  }
  if (frame.kind == TransportFrameKind::heartbeat_response) {
    if (frame.sequenceNumber > kMaximumTransportSequenceNumber ||
        frame.sessionId == 0U ||
        frame.payload.size() != kTransportHeartbeatResponsePayloadSize ||
        (frame.lastReceivedMessageNumber != kInvalidTransportSequenceNumber &&
         frame.lastReceivedMessageNumber > kMaximumTransportSequenceNumber) ||
        transport_wire::readBigEndian32(frame.payload, 0U) >
            kMaximumTransportSequenceNumber) {
      fail("The transport heartbeat-response metadata is invalid.");
    }
    return;
  }
  if (frame.kind == TransportFrameKind::close ||
      frame.kind == TransportFrameKind::close_ack) {
    if (frame.sequenceNumber > kMaximumTransportSequenceNumber ||
        frame.sessionId == 0U || !frame.payload.empty() ||
        (frame.lastReceivedMessageNumber != kInvalidTransportSequenceNumber &&
         frame.lastReceivedMessageNumber > kMaximumTransportSequenceNumber)) {
      fail("The transport session-termination control metadata is invalid.");
    }
    return;
  }
  if (frame.sequenceNumber != kInvalidTransportSequenceNumber ||
      frame.sessionId != 0U ||
      frame.lastReceivedMessageNumber != kInvalidTransportSequenceNumber) {
    fail("A transport control frame cannot carry service-message metadata.");
  }
}

}  // namespace

bool isTransportFrameKind(TransportFrameKind kind) noexcept {
  switch (kind) {
    case TransportFrameKind::new_session:
    case TransportFrameKind::new_session_status:
    case TransportFrameKind::heartbeat:
    case TransportFrameKind::heartbeat_response:
    case TransportFrameKind::close:
    case TransportFrameKind::close_ack:
    case TransportFrameKind::resume_request:
    case TransportFrameKind::resume_status:
    case TransportFrameKind::data:
    case TransportFrameKind::call_response:
    case TransportFrameKind::callback_request:
    case TransportFrameKind::callback_response:
      return true;
  }
  return false;
}

bool isTransportServiceFrameKind(TransportFrameKind kind) noexcept {
  switch (kind) {
    case TransportFrameKind::data:
    case TransportFrameKind::call_response:
    case TransportFrameKind::callback_request:
    case TransportFrameKind::callback_response:
      return true;
    case TransportFrameKind::new_session:
    case TransportFrameKind::new_session_status:
    case TransportFrameKind::heartbeat:
    case TransportFrameKind::heartbeat_response:
    case TransportFrameKind::close:
    case TransportFrameKind::close_ack:
    case TransportFrameKind::resume_request:
    case TransportFrameKind::resume_status:
      return false;
  }
  return false;
}

bool isTransportNewSessionStatusCode(
    TransportNewSessionStatusCode status) noexcept {
  switch (status) {
    case TransportNewSessionStatusCode::success:
    case TransportNewSessionStatusCode::unsupported_protocol_version:
    case TransportNewSessionStatusCode::out_of_resources:
    case TransportNewSessionStatusCode::other_error:
      return true;
  }
  return false;
}

std::vector<std::uint8_t> encodeTransportFrame(TransportFrame const& frame) {
  if (!isTransportFrameKind(frame.kind)) {
    fail("The transport frame kind is not recognized.");
  }
  validateFrameMetadata(frame);
  if (frame.payload.size() > kTransportMaximumPayloadBytes ||
      frame.payload.size() > std::numeric_limits<std::uint32_t>::max()) {
    fail("The transport frame payload exceeds the protocol maximum.");
  }

  std::vector<std::uint8_t> encoded;
  encoded.reserve(kTransportFrameHeaderSize + frame.payload.size());
  transport_wire::appendBigEndian32(
      encoded,
      static_cast<std::uint32_t>(kTransportFrameHeaderSize + frame.payload.size()));
  transport_wire::appendBigEndian32(encoded, frame.sequenceNumber);
  transport_wire::appendBigEndian64(encoded, frame.sessionId);
  transport_wire::appendBigEndian32(encoded, frame.lastReceivedMessageNumber);
  transport_wire::appendBigEndian32(encoded, static_cast<std::uint32_t>(frame.kind));
  encoded.insert(encoded.end(), frame.payload.begin(), frame.payload.end());
  return encoded;
}

TransportFrame decodeTransportFrame(std::span<std::uint8_t const> encoded) {
  if (encoded.size() < kTransportFrameHeaderSize) {
    fail("The transport frame is shorter than its header.");
  }
  auto const kind = static_cast<TransportFrameKind>(
      transport_wire::readBigEndian32(encoded, 20U));
  if (!isTransportFrameKind(kind)) {
    fail("The transport frame kind is not recognized.");
  }
  auto const packetSize = transport_wire::readBigEndian32(encoded, 0U);
  if (packetSize < kTransportFrameHeaderSize) {
    fail("The transport packet size is shorter than its header.");
  }
  auto const payloadSize = packetSize - kTransportFrameHeaderSize;
  if (payloadSize > kTransportMaximumPayloadBytes) {
    fail("The transport frame payload exceeds the protocol maximum.");
  }
  if (encoded.size() != packetSize) {
    fail("The transport packet size does not match the received bytes.");
  }

  auto const sequenceNumber = transport_wire::readBigEndian32(encoded, 4U);
  auto const sessionId = transport_wire::readBigEndian64(encoded, 8U);
  auto const lastReceivedMessageNumber =
      transport_wire::readBigEndian32(encoded, 16U);
  TransportFrame frame{
      kind,
      {},
      sequenceNumber,
      sessionId,
      lastReceivedMessageNumber};
  auto const payloadBegin = encoded.begin() +
      static_cast<std::ptrdiff_t>(kTransportFrameHeaderSize);
  auto const payloadEnd = payloadBegin + static_cast<std::ptrdiff_t>(payloadSize);
  frame.payload.assign(payloadBegin, payloadEnd);
  validateFrameMetadata(frame);
  return frame;
}

TransportFrame makeTransportNewSessionRequest(std::uint32_t protocolVersion) {
  std::vector<std::uint8_t> payload;
  payload.reserve(kTransportNewSessionPayloadSize);
  transport_wire::appendBigEndian32(payload, protocolVersion);
  return TransportFrame{
      TransportFrameKind::new_session,
      std::move(payload)};
}

std::uint32_t decodeTransportNewSessionRequest(TransportFrame const& frame) {
  if (frame.kind != TransportFrameKind::new_session ||
      frame.sequenceNumber != kInvalidTransportSequenceNumber ||
      frame.sessionId != 0U ||
      frame.lastReceivedMessageNumber != kInvalidTransportSequenceNumber ||
      frame.payload.size() != kTransportNewSessionPayloadSize) {
    fail("Only a valid New Session control frame can be decoded as a request.");
  }
  return transport_wire::readBigEndian32(frame.payload, 0U);
}

TransportFrame makeTransportNewSessionStatus(
    TransportNewSessionStatus const& status,
    std::uint32_t sequenceNumber) {
  if (!isTransportNewSessionStatusCode(status.status) ||
      sequenceNumber > kMaximumTransportSequenceNumber ||
      ((status.status == TransportNewSessionStatusCode::success) !=
       (status.sessionId != 0U))) {
    fail("The transport New Session Status is invalid.");
  }
  std::vector<std::uint8_t> payload;
  payload.reserve(kTransportNewSessionPayloadSize);
  transport_wire::appendBigEndian32(
      payload, static_cast<std::uint32_t>(status.status));
  return TransportFrame{
      TransportFrameKind::new_session_status,
      std::move(payload),
      sequenceNumber,
      status.sessionId,
      kInvalidTransportSequenceNumber};
}

TransportNewSessionStatus decodeTransportNewSessionStatus(
    TransportFrame const& frame) {
  if (frame.kind != TransportFrameKind::new_session_status ||
      frame.sequenceNumber > kMaximumTransportSequenceNumber ||
      frame.lastReceivedMessageNumber != kInvalidTransportSequenceNumber ||
      frame.payload.size() != kTransportNewSessionPayloadSize) {
    fail("Only a valid New Session Status frame can be decoded as a response.");
  }
  auto const status = static_cast<TransportNewSessionStatusCode>(
      transport_wire::readBigEndian32(frame.payload, 0U));
  if (!isTransportNewSessionStatusCode(status) ||
      ((status == TransportNewSessionStatusCode::success) !=
       (frame.sessionId != 0U))) {
    fail("The transport New Session Status payload or session ID is invalid.");
  }
  return TransportNewSessionStatus{frame.sessionId, status};
}

bool isTransportResumeStatusCode(TransportResumeStatusCode status) noexcept {
  switch (status) {
    case TransportResumeStatusCode::ok:
    case TransportResumeStatusCode::invalid_session:
    case TransportResumeStatusCode::insufficient_buffered_messages:
    case TransportResumeStatusCode::other_error:
      return true;
  }
  return false;
}

TransportFrame makeTransportResumeRequest(
    TransportResumeRequest const& request,
    std::uint32_t sequenceNumber) {
  std::vector<std::uint8_t> payload;
  payload.reserve(kTransportResumePayloadSize);
  transport_wire::appendBigEndian32(
      payload, request.lastReceivedRtiSequenceNumber);
  transport_wire::appendBigEndian32(
      payload, request.oldestAvailableFederateSequenceNumber);
  TransportFrame frame{
      TransportFrameKind::resume_request,
      std::move(payload),
      sequenceNumber,
      request.sessionId,
      request.lastReceivedRtiSequenceNumber};
  validateFrameMetadata(frame);
  return frame;
}

TransportResumeRequest decodeTransportResumeRequest(
    TransportFrame const& frame) {
  if (frame.kind != TransportFrameKind::resume_request) {
    fail("Only a Resume Request frame can be decoded as a resume request.");
  }
  validateFrameMetadata(frame);
  std::span<std::uint8_t const> payload(frame.payload);
  return TransportResumeRequest{
      frame.sessionId,
      transport_wire::readBigEndian32(payload, 0U),
      transport_wire::readBigEndian32(payload, 4U)};
}

TransportFrame makeTransportResumeStatus(
    TransportResumeStatus const& status,
    std::uint32_t sequenceNumber) {
  std::vector<std::uint8_t> payload;
  payload.reserve(kTransportResumePayloadSize);
  transport_wire::appendBigEndian32(
      payload, static_cast<std::uint32_t>(status.status));
  transport_wire::appendBigEndian32(
      payload, status.lastReceivedFederateSequenceNumber);
  TransportFrame frame{
      TransportFrameKind::resume_status,
      std::move(payload),
      sequenceNumber,
      status.sessionId,
      status.lastReceivedFederateSequenceNumber};
  validateFrameMetadata(frame);
  return frame;
}

TransportResumeStatus decodeTransportResumeStatus(TransportFrame const& frame) {
  if (frame.kind != TransportFrameKind::resume_status) {
    fail("Only a Resume Status frame can be decoded as a resume status.");
  }
  validateFrameMetadata(frame);
  std::span<std::uint8_t const> payload(frame.payload);
  return TransportResumeStatus{
      frame.sessionId,
      static_cast<TransportResumeStatusCode>(
          transport_wire::readBigEndian32(payload, 0U)),
      transport_wire::readBigEndian32(payload, 4U)};
}

TransportFrame makeTransportHeartbeat(
    std::uint64_t sessionId,
    std::uint32_t sequenceNumber,
    std::uint32_t lastReceivedMessageNumber) {
  TransportFrame frame{
      TransportFrameKind::heartbeat,
      {},
      sequenceNumber,
      sessionId,
      lastReceivedMessageNumber};
  validateFrameMetadata(frame);
  return frame;
}

TransportFrame makeTransportHeartbeatResponse(
    std::uint64_t sessionId,
    std::uint32_t sequenceNumber,
    std::uint32_t lastReceivedMessageNumber,
    std::uint32_t responseToSequenceNumber) {
  std::vector<std::uint8_t> payload;
  payload.reserve(kTransportHeartbeatResponsePayloadSize);
  transport_wire::appendBigEndian32(payload, responseToSequenceNumber);
  TransportFrame frame{
      TransportFrameKind::heartbeat_response,
      std::move(payload),
      sequenceNumber,
      sessionId,
      lastReceivedMessageNumber};
  validateFrameMetadata(frame);
  return frame;
}

std::uint32_t decodeTransportHeartbeatResponse(TransportFrame const& frame) {
  if (frame.kind != TransportFrameKind::heartbeat_response) {
    fail("Only a Heartbeat Response frame can be decoded as a response.");
  }
  validateFrameMetadata(frame);
  return transport_wire::readBigEndian32(frame.payload, 0U);
}

TransportFrame makeTransportCloseRequest(
    std::uint64_t sessionId,
    std::uint32_t sequenceNumber,
    std::uint32_t lastReceivedMessageNumber) {
  TransportFrame frame{
      TransportFrameKind::close,
      {},
      sequenceNumber,
      sessionId,
      lastReceivedMessageNumber};
  validateFrameMetadata(frame);
  return frame;
}

TransportFrame makeTransportCloseAcknowledgement(
    std::uint64_t sessionId,
    std::uint32_t sequenceNumber,
    std::uint32_t lastReceivedMessageNumber) {
  TransportFrame frame{
      TransportFrameKind::close_ack,
      {},
      sequenceNumber,
      sessionId,
      lastReceivedMessageNumber};
  validateFrameMetadata(frame);
  return frame;
}

}  // namespace umbra::detail
