#include <catch2/catch_test_macros.hpp>

#include "internal/encoding/fnv1a.hpp"
#include "internal/encoding/variable_length_data_2025.hpp"
#include "internal/federation/process_transport_message_history.hpp"
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
using umbra::detail::TransportNewSessionStatus;
using umbra::detail::TransportNewSessionStatusCode;
using umbra::detail::TransportResumeRequest;
using umbra::detail::TransportResumeStatus;
using umbra::detail::TransportResumeStatusCode;
using umbra::detail::ProcessTransportMessageHistory;
using umbra::detail::ProcessTransportSequenceState;
using umbra::detail::TransportProtocolError;
using umbra::detail::TransportServiceMessage;
using umbra::detail::TransportServiceMessageKind;
using umbra::detail::TransportServiceOperation;
using umbra::detail::TransportServiceProtocolError;
using umbra::detail::TransportServiceStatus;
using umbra::detail::decodeTransportFrame;
using umbra::detail::decodeTransportHeartbeatResponse;
using umbra::detail::decodeTransportNewSessionRequest;
using umbra::detail::decodeTransportNewSessionStatus;
using umbra::detail::decodeTransportResumeRequest;
using umbra::detail::decodeTransportResumeStatus;
using umbra::detail::decodeTransportServiceMessage;
using umbra::detail::encodeTransportFrame;
using umbra::detail::encodeTransportServiceMessage;
using umbra::detail::kTransportFrameHeaderSize;
using umbra::detail::kTransportServiceHeaderSize;
using umbra::detail::makeTransportNewSessionRequest;
using umbra::detail::makeTransportNewSessionStatus;
using umbra::detail::makeTransportHeartbeat;
using umbra::detail::makeTransportHeartbeatResponse;
using umbra::detail::makeTransportCloseAcknowledgement;
using umbra::detail::makeTransportCloseRequest;
using umbra::detail::makeTransportResumeRequest;
using umbra::detail::makeTransportResumeStatus;

}  // namespace

TEST_CASE(
    "Process transport uses the 24-byte packet metadata header and rejects malformed frames",
    "[unit][internal][foundation][transport][process-boundary][transport-message-header][table33-header-layout]") {
  TransportEndpointIdentity const identity{"test-peer", 0x0102030405060708ULL};

  auto const newSession = makeTransportNewSessionRequest();
  auto const newSessionEncoded = encodeTransportFrame(newSession);
  REQUIRE(newSessionEncoded == std::vector<std::uint8_t>{
      0x00U, 0x00U, 0x00U, 0x1cU,
      0x80U, 0x00U, 0x00U, 0x00U,
      0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
      0x80U, 0x00U, 0x00U, 0x00U,
      0x00U, 0x00U, 0x00U, 0x01U,
      0x00U, 0x00U, 0x00U, 0x01U});
  auto const decodedNewSession = decodeTransportFrame(newSessionEncoded);
  REQUIRE(decodeTransportNewSessionRequest(decodedNewSession) ==
          umbra::detail::kTransportHla4ProtocolVersion);

  TransportFrame const dataFrame{
      TransportFrameKind::data,
      std::vector<std::uint8_t>{0x00, 0x10, 0x20, 0x7f, 0xff},
      0x01020304U,
      0x0102030405060708ULL,
      0x00010203U};
  auto const dataEncoded = encodeTransportFrame(dataFrame);
  REQUIRE(dataEncoded == std::vector<std::uint8_t>{
      0x00U, 0x00U, 0x00U, 0x1dU,
      0x01U, 0x02U, 0x03U, 0x04U,
      0x01U, 0x02U, 0x03U, 0x04U, 0x05U, 0x06U, 0x07U, 0x08U,
      0x00U, 0x01U, 0x02U, 0x03U,
      0x00U, 0x00U, 0x00U, 0x14U,
      0x00U, 0x10U, 0x20U, 0x7fU, 0xffU});
  auto const dataDecoded = decodeTransportFrame(dataEncoded);
  REQUIRE(dataDecoded.kind == TransportFrameKind::data);
  REQUIRE(dataDecoded.payload == dataFrame.payload);
  REQUIRE(dataDecoded.sequenceNumber == dataFrame.sequenceNumber);
  REQUIRE(dataDecoded.sessionId == dataFrame.sessionId);
  REQUIRE(dataDecoded.lastReceivedMessageNumber ==
          dataFrame.lastReceivedMessageNumber);

  auto const heartbeat = makeTransportHeartbeat(
      identity.sessionId, 0x01020305U, 0x00010203U);
  auto const heartbeatDecoded = decodeTransportFrame(
      encodeTransportFrame(heartbeat));
  REQUIRE(heartbeatDecoded.kind == TransportFrameKind::heartbeat);
  REQUIRE(heartbeatDecoded.sequenceNumber == 0x01020305U);
  REQUIRE(heartbeatDecoded.sessionId == identity.sessionId);
  REQUIRE(heartbeatDecoded.lastReceivedMessageNumber == 0x00010203U);
  REQUIRE(heartbeatDecoded.payload.empty());

  auto truncated = dataEncoded;
  truncated.pop_back();
  REQUIRE_THROWS_AS(decodeTransportFrame(truncated), TransportProtocolError);

  auto badPacketSize = dataEncoded;
  badPacketSize[3] += 1U;
  REQUIRE_THROWS_AS(decodeTransportFrame(badPacketSize), TransportProtocolError);

  auto badKind = dataEncoded;
  badKind[20] = 0xffU;
  badKind[21] = 0xffU;
  badKind[22] = 0xffU;
  badKind[23] = 0xffU;
  REQUIRE_THROWS_AS(decodeTransportFrame(badKind), TransportProtocolError);

  auto invalidSequence = dataFrame;
  invalidSequence.sequenceNumber = umbra::detail::kInvalidTransportSequenceNumber;
  REQUIRE_THROWS_AS(encodeTransportFrame(invalidSequence), TransportProtocolError);

  auto invalidSession = dataFrame;
  invalidSession.sessionId = 0U;
  REQUIRE_THROWS_AS(encodeTransportFrame(invalidSession), TransportProtocolError);

  auto invalidAcknowledgement = dataFrame;
  invalidAcknowledgement.lastReceivedMessageNumber =
      umbra::detail::kInvalidTransportSequenceNumber + 1U;
  REQUIRE_THROWS_AS(
      encodeTransportFrame(invalidAcknowledgement), TransportProtocolError);

  auto invalidHeartbeat = heartbeat;
  invalidHeartbeat.payload.push_back(0U);
  REQUIRE_THROWS_AS(
      encodeTransportFrame(invalidHeartbeat), TransportProtocolError);

  auto oversized = dataEncoded;
  oversized[0] = 0xffU;
  oversized[1] = 0xffU;
  oversized[2] = 0xffU;
  oversized[3] = 0xffU;
  REQUIRE_THROWS_AS(decodeTransportFrame(oversized), TransportProtocolError);

}

TEST_CASE(
    "Private transport header preserves zero and maximum valid sequence numbers",
    "[unit][internal][foundation][transport][process-boundary][transport-message-header][sequence-number-wire-boundary]") {
  constexpr std::uint64_t sessionId = 0x0102030405060708ULL;
  std::array<std::uint32_t, 2U> const sequenceNumbers{
      0U,
      umbra::detail::kMaximumTransportSequenceNumber};

  for (std::uint32_t const sequenceNumber : sequenceNumbers) {
    TransportFrame const frame{
        TransportFrameKind::data,
        std::vector<std::uint8_t>{0xa5U},
        sequenceNumber,
        sessionId,
        0U};
    auto const encoded = encodeTransportFrame(frame);
    REQUIRE(encoded.size() == kTransportFrameHeaderSize + 1U);

    std::array<std::uint8_t, 4U> const encodedSequence{
        encoded[4U], encoded[5U], encoded[6U], encoded[7U]};
    std::array<std::uint8_t, 4U> const expectedSequence{
        static_cast<std::uint8_t>((sequenceNumber >> 24U) & 0xffU),
        static_cast<std::uint8_t>((sequenceNumber >> 16U) & 0xffU),
        static_cast<std::uint8_t>((sequenceNumber >> 8U) & 0xffU),
        static_cast<std::uint8_t>(sequenceNumber & 0xffU)};
    REQUIRE(encodedSequence == expectedSequence);

    auto const decoded = decodeTransportFrame(encoded);
    REQUIRE(decoded.kind == TransportFrameKind::data);
    REQUIRE(decoded.sequenceNumber == sequenceNumber);
    REQUIRE(decoded.sessionId == sessionId);
    REQUIRE(decoded.payload == std::vector<std::uint8_t>{0xa5U});
  }
}

TEST_CASE(
    "Process transport encodes IEEE New Session and New Session Status payloads",
    "[unit][internal][foundation][transport][process-boundary][table35-new-session][table36-new-session-status]") {
  auto const unsupportedVersionRequest = makeTransportNewSessionRequest(2U);
  REQUIRE(decodeTransportNewSessionRequest(
              decodeTransportFrame(
                  encodeTransportFrame(unsupportedVersionRequest))) == 2U);

  TransportNewSessionStatus const success{
      0x0102030405060708ULL,
      TransportNewSessionStatusCode::success};
  auto const successFrame = makeTransportNewSessionStatus(success, 0U);
  REQUIRE(successFrame.sequenceNumber == 0U);
  auto const successBytes = encodeTransportFrame(successFrame);
  REQUIRE(successBytes == std::vector<std::uint8_t>{
      0x00U, 0x00U, 0x00U, 0x1cU,
      0x00U, 0x00U, 0x00U, 0x00U,
      0x01U, 0x02U, 0x03U, 0x04U, 0x05U, 0x06U, 0x07U, 0x08U,
      0x80U, 0x00U, 0x00U, 0x00U,
      0x00U, 0x00U, 0x00U, 0x02U,
      0x00U, 0x00U, 0x00U, 0x00U});
  auto const decodedSuccess = decodeTransportNewSessionStatus(
      decodeTransportFrame(successBytes));
  REQUIRE(decodedSuccess.sessionId == success.sessionId);
  REQUIRE(decodedSuccess.status == TransportNewSessionStatusCode::success);
  REQUIRE_THROWS_AS(
      makeTransportNewSessionStatus(success, 0x80000000U),
      TransportProtocolError);

  for (auto const [status, expectedReason] : {
           std::pair{
               TransportNewSessionStatusCode::unsupported_protocol_version,
               std::uint32_t{1U}},
           std::pair{
               TransportNewSessionStatusCode::out_of_resources,
               std::uint32_t{2U}},
           std::pair{
               TransportNewSessionStatusCode::other_error,
               std::uint32_t{99U}}}) {
    auto const frame = makeTransportNewSessionStatus({0U, status}, 0U);
    auto const encoded = encodeTransportFrame(frame);
    REQUIRE(std::array<std::uint8_t, 4U>{
                encoded[24], encoded[25], encoded[26], encoded[27]} ==
            std::array<std::uint8_t, 4U>{
                0U,
                0U,
                0U,
                static_cast<std::uint8_t>(expectedReason)});
    auto const decoded = decodeTransportNewSessionStatus(
        decodeTransportFrame(encoded));
    REQUIRE(decoded.sessionId == 0U);
    REQUIRE(decoded.status == status);
  }

  auto nonzeroNewSessionId = makeTransportNewSessionRequest();
  nonzeroNewSessionId.sessionId = 1U;
  REQUIRE_THROWS_AS(
      encodeTransportFrame(nonzeroNewSessionId), TransportProtocolError);

  auto successWithoutSessionId = successFrame;
  successWithoutSessionId.sessionId = 0U;
  REQUIRE_THROWS_AS(
      encodeTransportFrame(successWithoutSessionId), TransportProtocolError);

  auto failureWithSessionId = makeTransportNewSessionStatus({
      0U,
      TransportNewSessionStatusCode::other_error}, 0U);
  failureWithSessionId.sessionId = 7U;
  REQUIRE_THROWS_AS(
      encodeTransportFrame(failureWithSessionId), TransportProtocolError);

  auto invalidReason = makeTransportNewSessionStatus({
      0U,
      TransportNewSessionStatusCode::other_error}, 0U);
  invalidReason.payload[3U] = 3U;
  REQUIRE_THROWS_AS(
      encodeTransportFrame(invalidReason), TransportProtocolError);
}

TEST_CASE(
    "Private resume control frames preserve session and recovery sequence fields",
    "[unit][internal][transport][process-boundary][process-connection-fault-recovery-history][resume-control-framing]") {
  TransportResumeRequest const request{
      0x1122334455667788ULL,
      17U,
      21U};
  auto const requestFrame = makeTransportResumeRequest(request, 31U);
  REQUIRE(requestFrame.kind == TransportFrameKind::resume_request);
  REQUIRE(requestFrame.sequenceNumber == 31U);
  REQUIRE(requestFrame.sessionId == request.sessionId);
  REQUIRE(requestFrame.lastReceivedMessageNumber ==
          request.lastReceivedRtiSequenceNumber);

  auto const encodedRequest = encodeTransportFrame(requestFrame);
  auto const decodedRequestFrame = decodeTransportFrame(encodedRequest);
  auto const decodedRequest = decodeTransportResumeRequest(decodedRequestFrame);
  REQUIRE(decodedRequest.sessionId == request.sessionId);
  REQUIRE(decodedRequest.lastReceivedRtiSequenceNumber ==
          request.lastReceivedRtiSequenceNumber);
  REQUIRE(decodedRequest.oldestAvailableFederateSequenceNumber ==
          request.oldestAvailableFederateSequenceNumber);

  TransportResumeStatus const status{
      request.sessionId,
      TransportResumeStatusCode::insufficient_buffered_messages,
      13U};
  auto const statusFrame = makeTransportResumeStatus(status, 32U);
  REQUIRE(statusFrame.kind == TransportFrameKind::resume_status);
  REQUIRE(statusFrame.sequenceNumber == 32U);
  REQUIRE(statusFrame.lastReceivedMessageNumber ==
          status.lastReceivedFederateSequenceNumber);

  auto const encodedStatus = encodeTransportFrame(statusFrame);
  auto const decodedStatus = decodeTransportResumeStatus(
      decodeTransportFrame(encodedStatus));
  REQUIRE(decodedStatus.sessionId == status.sessionId);
  REQUIRE(decodedStatus.status == status.status);
  REQUIRE(decodedStatus.lastReceivedFederateSequenceNumber ==
          status.lastReceivedFederateSequenceNumber);

  auto mismatchedLrm = requestFrame;
  mismatchedLrm.lastReceivedMessageNumber = 18U;
  REQUIRE_THROWS_AS(encodeTransportFrame(mismatchedLrm), TransportProtocolError);

  auto invalidStatusCode = statusFrame;
  invalidStatusCode.payload[3U] = 3U;
  REQUIRE_THROWS_AS(encodeTransportFrame(invalidStatusCode), TransportProtocolError);

  auto invalidSession = request;
  invalidSession.sessionId = 0U;
  REQUIRE_THROWS_AS(
      makeTransportResumeRequest(invalidSession, 33U),
      TransportProtocolError);
  REQUIRE_THROWS_AS(
      makeTransportResumeRequest(
          request, umbra::detail::kInvalidTransportSequenceNumber),
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
    "Private service-message history replays only a complete unacknowledged suffix",
    "[unit][internal][transport][process-boundary][service-message-history][process-connection-fault-recovery-history]") {
  ProcessTransportMessageHistory history(2U);
  std::array<std::uint8_t, 1U> const first{0x10U};
  std::array<std::uint8_t, 1U> const second{0x20U};
  std::array<std::uint8_t, 1U> const third{0x30U};

  REQUIRE(history.retainOutgoing(0U, first) == 0U);
  REQUIRE(history.retainOutgoing(2U, second) == 2U);
  REQUIRE(history.retainOutgoing(4U, third) == 4U);
  REQUIRE(history.retainedBytes() == 2U);
  REQUIRE(history.retainedCount() == 2U);
  REQUIRE(history.oldestAvailableSequenceNumber() == 2U);
  REQUIRE_FALSE(history.replayAfter(umbra::detail::kInvalidTransportSequenceNumber));

  auto replay = history.replayAfter(0U);
  REQUIRE(replay.has_value());
  REQUIRE(replay->size() == 2U);
  REQUIRE((*replay)[0].sequenceNumber == 2U);
  REQUIRE((*replay)[0].payload == std::vector<std::uint8_t>{0x20U});
  REQUIRE((*replay)[1].sequenceNumber == 4U);
  REQUIRE((*replay)[1].payload == std::vector<std::uint8_t>{0x30U});

  history.acknowledgeOutgoing(2U);
  REQUIRE(history.retainedCount() == 1U);
  auto acknowledgedReplay = history.replayAfter(2U);
  REQUIRE(acknowledgedReplay.has_value());
  REQUIRE(acknowledgedReplay->size() == 1U);
  REQUIRE((*acknowledgedReplay)[0].sequenceNumber == 4U);
  REQUIRE_FALSE(history.replayAfter(0U));

  REQUIRE(history.acceptIncoming(0U));
  REQUIRE_FALSE(history.acceptIncoming(0U));
  REQUIRE(history.acceptIncoming(2U));
  REQUIRE_FALSE(history.acceptIncoming(1U));
  REQUIRE(history.lastReceivedSequenceNumber() == 2U);
}

TEST_CASE(
    "Private service-message history wraps sequence numbers without changing replay order",
    "[unit][internal][transport][process-boundary][service-message-history][process-connection-fault-recovery-history]") {
  ProcessTransportMessageHistory history(8U);
  std::array<std::uint8_t, 1U> const payload{0x7fU};

  REQUIRE(history.retainOutgoing(
              umbra::detail::kMaximumTransportSequenceNumber, payload) ==
          umbra::detail::kMaximumTransportSequenceNumber);
  REQUIRE(history.retainOutgoing(0U, payload) == 0U);
  auto replay = history.replayAfter(
      umbra::detail::kMaximumTransportSequenceNumber);
  REQUIRE(replay.has_value());
  REQUIRE(replay->size() == 1U);
  REQUIRE((*replay)[0].sequenceNumber == 0U);

  REQUIRE(history.acceptIncoming(
      umbra::detail::kMaximumTransportSequenceNumber));
  REQUIRE(history.acceptIncoming(0U));
  REQUIRE(history.lastReceivedSequenceNumber() == 0U);

  ProcessTransportSequenceState sequenceState(
      umbra::detail::kMaximumTransportSequenceNumber);
  REQUIRE(sequenceState.nextOutgoingMessageNumber() ==
          umbra::detail::kMaximumTransportSequenceNumber);
  REQUIRE(sequenceState.nextOutgoingMessageNumber() == 0U);
  REQUIRE(sequenceState.acceptIncomingMessageNumber(
      umbra::detail::kMaximumTransportSequenceNumber));
  REQUIRE(sequenceState.acceptIncomingMessageNumber(1U));
  REQUIRE_FALSE(sequenceState.acceptIncomingMessageNumber(1U));
  REQUIRE(sequenceState.lastReceivedMessageNumber() == 1U);
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

TEST_CASE(
    "Private process transport encodes and validates Table 38 Heartbeat Response payloads",
    "[unit][internal][transport][process-boundary][heartbeat-response-table38]") {
  auto const response = makeTransportHeartbeatResponse(
      0x0102030405060708ULL,
      0x11223344U,
      9U,
      0x0a0b0c0dU);
  auto const encoded = encodeTransportFrame(response);
  REQUIRE(encoded == std::vector<std::uint8_t>{
      0x00U, 0x00U, 0x00U, 0x1cU,
      0x11U, 0x22U, 0x33U, 0x44U,
      0x01U, 0x02U, 0x03U, 0x04U, 0x05U, 0x06U, 0x07U, 0x08U,
      0x00U, 0x00U, 0x00U, 0x09U,
      0x00U, 0x00U, 0x00U, 0x04U,
      0x0aU, 0x0bU, 0x0cU, 0x0dU});

  auto const decoded = decodeTransportFrame(encoded);
  REQUIRE(decoded.kind == TransportFrameKind::heartbeat_response);
  REQUIRE(decoded.sessionId == response.sessionId);
  REQUIRE(decoded.sequenceNumber == response.sequenceNumber);
  REQUIRE(decoded.lastReceivedMessageNumber ==
          response.lastReceivedMessageNumber);
  REQUIRE(decodeTransportHeartbeatResponse(decoded) == 0x0a0b0c0dU);

  auto invalidPayloadSize = response;
  invalidPayloadSize.payload.push_back(0U);
  REQUIRE_THROWS_AS(
      encodeTransportFrame(invalidPayloadSize), TransportProtocolError);

  auto invalidResponseSequence = response;
  invalidResponseSequence.payload = {0x80U, 0U, 0U, 0U};
  REQUIRE_THROWS_AS(
      encodeTransportFrame(invalidResponseSequence), TransportProtocolError);
}

TEST_CASE(
    "Private process transport encodes and validates Table 34 session termination controls",
    "[unit][internal][transport][process-boundary][table34-session-termination]") {
  auto const request = makeTransportCloseRequest(
      0x0102030405060708ULL,
      0x11223344U,
      0x0a0b0c0dU);
  auto const encodedRequest = encodeTransportFrame(request);
  REQUIRE(encodedRequest == std::vector<std::uint8_t>{
      0x00U, 0x00U, 0x00U, 0x18U,
      0x11U, 0x22U, 0x33U, 0x44U,
      0x01U, 0x02U, 0x03U, 0x04U, 0x05U, 0x06U, 0x07U, 0x08U,
      0x0aU, 0x0bU, 0x0cU, 0x0dU,
      0x00U, 0x00U, 0x00U, 0x05U});
  auto const decodedRequest = decodeTransportFrame(encodedRequest);
  REQUIRE(decodedRequest.kind == TransportFrameKind::close);
  REQUIRE(decodedRequest.sequenceNumber == request.sequenceNumber);
  REQUIRE(decodedRequest.sessionId == request.sessionId);
  REQUIRE(decodedRequest.lastReceivedMessageNumber ==
          request.lastReceivedMessageNumber);
  REQUIRE(decodedRequest.payload.empty());

  auto const acknowledgement = makeTransportCloseAcknowledgement(
      request.sessionId,
      0x11223345U,
      request.sequenceNumber);
  auto const decodedAcknowledgement = decodeTransportFrame(
      encodeTransportFrame(acknowledgement));
  REQUIRE(decodedAcknowledgement.kind == TransportFrameKind::close_ack);
  REQUIRE(decodedAcknowledgement.sessionId == request.sessionId);
  REQUIRE(decodedAcknowledgement.lastReceivedMessageNumber ==
          request.sequenceNumber);
  REQUIRE(decodedAcknowledgement.payload.empty());

  auto invalidSession = request;
  invalidSession.sessionId = 0U;
  REQUIRE_THROWS_AS(encodeTransportFrame(invalidSession), TransportProtocolError);
  auto invalidSequence = request;
  invalidSequence.sequenceNumber = umbra::detail::kInvalidTransportSequenceNumber;
  REQUIRE_THROWS_AS(encodeTransportFrame(invalidSequence), TransportProtocolError);
  auto invalidPayload = acknowledgement;
  invalidPayload.payload.push_back(0U);
  REQUIRE_THROWS_AS(encodeTransportFrame(invalidPayload), TransportProtocolError);
}
