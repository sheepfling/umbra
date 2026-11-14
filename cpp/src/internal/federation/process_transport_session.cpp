#include "internal/federation/process_transport_session.hpp"

#include <utility>

namespace umbra::detail {
namespace {

[[nodiscard]] bool validSequenceNumberOrInvalid(
    std::uint32_t sequenceNumber) noexcept {
  return sequenceNumber == kInvalidTransportSequenceNumber ||
      sequenceNumber <= kMaximumTransportSequenceNumber;
}

[[nodiscard]] TransportFrameKind frameKindForServiceMessage(
    TransportServiceMessageKind kind) {
  switch (kind) {
    case TransportServiceMessageKind::request:
      return TransportFrameKind::data;
    case TransportServiceMessageKind::response:
      return TransportFrameKind::call_response;
    case TransportServiceMessageKind::event:
      return TransportFrameKind::callback_request;
  }
  throw ProcessTransportSessionError(
      "The process transport service message kind is not supported.");
}

[[nodiscard]] TransportFrame dataFrameFor(
    ProcessTransportConnection& connection,
    TransportServiceMessage const& message,
    std::uint32_t sequenceNumber) {
  auto payload = encodeTransportServiceMessage(message);
  auto& history = connection.messageHistory();
  auto const frameKind = frameKindForServiceMessage(message.kind);
  (void)history.retainOutgoing(sequenceNumber, payload, frameKind);
  return TransportFrame{
      frameKind,
      std::move(payload),
      sequenceNumber,
      connection.sessionId(),
      history.lastReceivedSequenceNumber()};
}

void appendVarint(std::vector<std::uint8_t>& encoded, std::size_t value) {
  while (value >= 0x80U) {
    encoded.push_back(static_cast<std::uint8_t>((value & 0x7fU) | 0x80U));
    value >>= 7U;
  }
  encoded.push_back(static_cast<std::uint8_t>(value));
}

void appendExceptionStringField(
    std::vector<std::uint8_t>& encoded,
    std::uint8_t fieldTag,
    std::string const& value) {
  encoded.push_back(fieldTag);
  appendVarint(encoded, value.size());
  for (char const character : value) {
    encoded.push_back(static_cast<std::uint8_t>(
        static_cast<unsigned char>(character)));
  }
}

[[nodiscard]] std::uint32_t callbackRequestSequenceFromResponsePayload(
    std::vector<std::uint8_t> const& payload) {
  if (payload.size() < 6U) {
    throw ProcessTransportSessionError(
        "The process transport callback response is shorter than its correlation and result fields.");
  }
  auto const requestSequenceNumber =
      (static_cast<std::uint32_t>(payload[0]) << 24U) |
      (static_cast<std::uint32_t>(payload[1]) << 16U) |
      (static_cast<std::uint32_t>(payload[2]) << 8U) |
      static_cast<std::uint32_t>(payload[3]);
  if (requestSequenceNumber > kMaximumTransportSequenceNumber) {
    throw ProcessTransportSessionError(
        "The process transport callback response contains an invalid request sequence number.");
  }

  // The pinned FederateAmbassador.proto defines CallbackResponse as a oneof:
  // CallbackSucceeded (field 1) or ExceptionData (field 2). Table 46 carries
  // the correlated callback request number before this protobuf value.
  auto const resultTag = payload[4];
  if (resultTag == 0x0aU && payload[5] == 0U && payload.size() == 6U) {
    return requestSequenceNumber;
  }
  if (resultTag != 0x12U) {
    throw ProcessTransportSessionError(
        "The process transport callback response has no recognized protobuf result.");
  }

  std::size_t offset = 5U;
  std::size_t exceptionDataSize = 0U;
  std::size_t shift = 0U;
  while (offset < payload.size() && shift < 35U) {
    auto const byte = payload[offset++];
    exceptionDataSize |=
        static_cast<std::size_t>(byte & 0x7fU) << shift;
    if ((byte & 0x80U) == 0U) {
      if (exceptionDataSize == payload.size() - offset) {
        return requestSequenceNumber;
      }
      break;
    }
    shift += 7U;
  }
  throw ProcessTransportSessionError(
      "The process transport callback failure response has an invalid ExceptionData length.");
}

[[nodiscard]] TransportServiceMessage messageFromFrame(TransportFrame const& frame) {
  if (!isTransportServiceFrameKind(frame.kind) ||
      frame.kind == TransportFrameKind::callback_response) {
    throw ProcessTransportSessionError(
        "The process transport session received a non-data frame.");
  }
  try {
    auto message = decodeTransportServiceMessage(frame.payload);
    if (frame.kind != frameKindForServiceMessage(message.kind)) {
      throw ProcessTransportSessionError(
          "The process transport frame type does not match its service payload.");
    }
    return message;
  } catch (TransportServiceProtocolError const& error) {
    throw ProcessTransportSessionError(error.what());
  }
}

[[nodiscard]] bool sendServiceMessage(
    std::shared_ptr<ProcessTransportConnection> const& connection,
    TransportServiceMessage const& message) {
  if (!connection) {
    return false;
  }
  return connection->sendSequencedFrame(
      [&connection, &message](auto sequenceNumber) {
        return dataFrameFor(*connection, message, sequenceNumber);
      });
}

}  // namespace

ProcessTransportSession::ProcessTransportSession(
    std::shared_ptr<ProcessTransportConnection> connection,
    bool replyToPeerHeartbeats,
    std::function<bool()> replyToPeerHeartbeatsForTesting)
    : connectionState_(std::make_shared<ConnectionState>()),
      replyToPeerHeartbeats_(replyToPeerHeartbeats),
      replyToPeerHeartbeatsForTesting_(
          std::move(replyToPeerHeartbeatsForTesting)) {
  if (!connection) {
    throw ProcessTransportSessionError(
        "A process transport session requires an open connection object.");
  }
  connectionState_->connection = std::move(connection);
}

bool ProcessTransportSession::acknowledgePeerHeartbeat(
    std::shared_ptr<ProcessTransportConnection> const& connection,
    std::uint32_t receivedSequenceNumber) {
  auto const shouldReply = replyToPeerHeartbeatsForTesting_
      ? replyToPeerHeartbeatsForTesting_()
      : replyToPeerHeartbeats_;
  if (!shouldReply) {
    return true;
  }
  return connection->sendSequencedFrame(
      [&connection, receivedSequenceNumber](auto sequenceNumber) {
    return makeTransportHeartbeatResponse(
        connection->sessionId(),
        sequenceNumber,
        connection->messageHistory().lastReceivedSequenceNumber(),
        receivedSequenceNumber);
      });
}

void ProcessTransportSession::replaceConnection(
    std::shared_ptr<ProcessTransportConnection> connection) {
  std::scoped_lock lock(connectionState_->mutex);
  auto const& currentConnection = connectionState_->connection;
  if (!connection || !connection->open() || !currentConnection ||
      connection->sessionId() != currentConnection->sessionId() ||
      connection->sessionState() != currentConnection->sessionState()) {
    throw ProcessTransportSessionError(
        "A replacement socket must preserve the process transport session identity and state.");
  }
  connectionState_->connection = std::move(connection);
  receivedResumeRequestSequenceNumber_.reset();
  pendingServiceFrame_.reset();
}

bool ProcessTransportSession::send(TransportServiceMessage const& message) {
  return sendServiceMessage(connection(), message);
}

std::function<bool(TransportServiceMessage const&)>
ProcessTransportSession::makeServiceSender() const {
  auto const state = connectionState_;
  return [state](TransportServiceMessage const& message) {
    std::shared_ptr<ProcessTransportConnection> activeConnection;
    {
      std::scoped_lock lock(state->mutex);
      activeConnection = state->connection;
    }
    return sendServiceMessage(activeConnection, message);
  };
}

bool ProcessTransportSession::receive(
    TransportServiceMessage& message,
    TransportFrameKind* receivedFrameKind,
    std::uint32_t* receivedSequenceNumber,
    std::uint32_t* receivedLastReceivedMessageNumber) {
  auto const activeConnection = connection();
  auto& history = activeConnection->messageHistory();
  while (true) {
    TransportFrame frame;
    if (pendingServiceFrame_) {
      frame = std::move(*pendingServiceFrame_);
      pendingServiceFrame_.reset();
    } else if (!activeConnection->receiveFrame(frame)) {
      return false;
    }
    if (frame.kind == TransportFrameKind::heartbeat ||
        frame.kind == TransportFrameKind::heartbeat_response) {
      if (frame.sessionId != activeConnection->sessionId()) {
        throw ProcessTransportSessionError(
            "The process transport heartbeat belongs to a different session.");
      }
      try {
        history.acknowledgeOutgoing(frame.lastReceivedMessageNumber);
        (void)activeConnection->sequenceState().acceptIncomingMessageNumber(
            frame.sequenceNumber);
      } catch (TransportProtocolError const& error) {
        throw ProcessTransportSessionError(error.what());
      }
      if (frame.kind == TransportFrameKind::heartbeat &&
          !acknowledgePeerHeartbeat(activeConnection, frame.sequenceNumber)) {
        return false;
      }
      if (frame.kind == TransportFrameKind::heartbeat_response) {
        static_cast<void>(decodeTransportHeartbeatResponse(frame));
      }
      continue;
    }
    if (frame.kind == TransportFrameKind::close ||
        frame.kind == TransportFrameKind::close_ack) {
      if (frame.sessionId != activeConnection->sessionId()) {
        throw ProcessTransportSessionError(
            "The process transport termination control belongs to a different session.");
      }
      try {
        if (frame.kind == TransportFrameKind::close) {
          history.acknowledgeOutgoing(frame.lastReceivedMessageNumber);
        }
        if (!activeConnection->sequenceState().acceptIncomingMessageNumber(
                frame.sequenceNumber)) {
          continue;
        }
      } catch (TransportProtocolError const& error) {
        throw ProcessTransportSessionError(error.what());
      }
      if (frame.kind == TransportFrameKind::close) {
        receivedTerminateSessionSequenceNumber_ = frame.sequenceNumber;
      }
      if (receivedFrameKind != nullptr) {
        *receivedFrameKind = frame.kind;
      }
      if (receivedSequenceNumber != nullptr) {
        *receivedSequenceNumber = frame.sequenceNumber;
      }
      if (receivedLastReceivedMessageNumber != nullptr) {
        *receivedLastReceivedMessageNumber = frame.lastReceivedMessageNumber;
      }
      return true;
    }
    if (isTransportServiceFrameKind(frame.kind) &&
        frame.sessionId != activeConnection->sessionId()) {
      throw ProcessTransportSessionError(
          "The process transport frame belongs to a different session.");
    }
    if (frame.kind == TransportFrameKind::callback_response) {
      try {
        history.acknowledgeOutgoing(frame.lastReceivedMessageNumber);
        auto& sequenceState = activeConnection->sequenceState();
        if (!sequenceState.acceptIncomingServiceMessageNumber(
                frame.sequenceNumber,
                history.lastReceivedSequenceNumber())) {
          continue;
        }
        (void)history.acceptIncoming(frame.sequenceNumber);
        (void)callbackRequestSequenceFromResponsePayload(frame.payload);
      } catch (TransportProtocolError const& error) {
        throw ProcessTransportSessionError(error.what());
      }
      continue;
    }
    auto received = messageFromFrame(frame);
    history.acknowledgeOutgoing(frame.lastReceivedMessageNumber);
    auto& sequenceState = activeConnection->sequenceState();
    if (!sequenceState.acceptIncomingServiceMessageNumber(
            frame.sequenceNumber,
            history.lastReceivedSequenceNumber())) {
      continue;
    }
    (void)history.acceptIncoming(frame.sequenceNumber);
    message = std::move(received);
    if (receivedFrameKind != nullptr) {
      *receivedFrameKind = frame.kind;
    }
    if (receivedSequenceNumber != nullptr) {
      *receivedSequenceNumber = frame.sequenceNumber;
    }
    return true;
  }
}

bool ProcessTransportSession::sendCloseAcknowledgement() {
  if (!receivedTerminateSessionSequenceNumber_) {
    throw ProcessTransportSessionError(
        "A Session Terminated control requires a received Terminate Session control.");
  }
  auto const activeConnection = connection();
  return activeConnection->sendSequencedFrame(
      [&activeConnection, this](std::uint32_t sequenceNumber) {
        return makeTransportCloseAcknowledgement(
            activeConnection->sessionId(),
            sequenceNumber,
            *receivedTerminateSessionSequenceNumber_);
      });
}

bool ProcessTransportSession::sendCallbackResponse(
    std::shared_ptr<ProcessTransportConnection> const& connection,
    std::uint32_t callbackRequestSequenceNumber,
    std::optional<std::pair<std::string, std::string>> exception) {
  if (!connection ||
      callbackRequestSequenceNumber > kMaximumTransportSequenceNumber) {
    throw ProcessTransportSessionError(
        "A callback response requires an open session and a valid callback request sequence number.");
  }

  std::vector<std::uint8_t> payload;
  payload.reserve(64U);
  payload.push_back(static_cast<std::uint8_t>(
      (callbackRequestSequenceNumber >> 24U) & 0xffU));
  payload.push_back(static_cast<std::uint8_t>(
      (callbackRequestSequenceNumber >> 16U) & 0xffU));
  payload.push_back(static_cast<std::uint8_t>(
      (callbackRequestSequenceNumber >> 8U) & 0xffU));
  payload.push_back(static_cast<std::uint8_t>(
      callbackRequestSequenceNumber & 0xffU));

  if (!exception) {
    // CallbackSucceeded is an empty nested protobuf message in the pinned
    // IEEE 1516-2025 FederateAmbassador.proto.
    payload.push_back(0x0aU);
    payload.push_back(0U);
  } else {
    std::vector<std::uint8_t> exceptionData;
    appendExceptionStringField(exceptionData, 0x0aU, exception->first);
    appendExceptionStringField(exceptionData, 0x12U, exception->second);
    payload.push_back(0x12U);
    appendVarint(payload, exceptionData.size());
    payload.insert(payload.end(), exceptionData.begin(), exceptionData.end());
  }

  auto& history = connection->messageHistory();
  try {
    return connection->sendSequencedFrame(
        [&connection, &history, &payload](std::uint32_t sequenceNumber) {
          (void)history.retainOutgoing(
              sequenceNumber,
              payload,
              TransportFrameKind::callback_response);
          return TransportFrame{
              TransportFrameKind::callback_response,
              payload,
              sequenceNumber,
              connection->sessionId(),
              history.lastReceivedSequenceNumber()};
        });
  } catch (TransportProtocolError const& error) {
    throw ProcessTransportSessionError(error.what());
  }
}

bool ProcessTransportSession::sendResumeRequest(
    TransportResumeRequest const& request) {
  auto const activeConnection = connection();
  if (request.sessionId != activeConnection->sessionId()) {
    throw ProcessTransportSessionError(
        "A Resume Request must identify the active process transport session.");
  }
  try {
    return activeConnection->sendSequencedFrame([&request](auto sequenceNumber) {
      return makeTransportResumeRequest(request, sequenceNumber);
    });
  } catch (TransportProtocolError const& error) {
    throw ProcessTransportSessionError(error.what());
  }
}

bool ProcessTransportSession::receiveResumeRequest(
    TransportResumeRequest& request) {
  return receiveInitialFrame(request) == InitialFrame::resume_request;
}

ProcessTransportSession::InitialFrame ProcessTransportSession::receiveInitialFrame(
    TransportResumeRequest& request) {
  auto const activeConnection = connection();
  while (true) {
    TransportFrame frame;
    if (!activeConnection->receiveFrame(frame)) {
      return InitialFrame::disconnected;
    }
    if (frame.kind == TransportFrameKind::heartbeat ||
        frame.kind == TransportFrameKind::heartbeat_response) {
      if (frame.sessionId != activeConnection->sessionId()) {
        throw ProcessTransportSessionError(
            "The initial process transport heartbeat belongs to a different session.");
      }
      try {
        activeConnection->messageHistory().acknowledgeOutgoing(
            frame.lastReceivedMessageNumber);
        (void)activeConnection->sequenceState().acceptIncomingMessageNumber(
            frame.sequenceNumber);
      } catch (TransportProtocolError const& error) {
        throw ProcessTransportSessionError(error.what());
      }
      if (frame.kind == TransportFrameKind::heartbeat &&
          !acknowledgePeerHeartbeat(activeConnection, frame.sequenceNumber)) {
        return InitialFrame::disconnected;
      }
      if (frame.kind == TransportFrameKind::heartbeat_response) {
        static_cast<void>(decodeTransportHeartbeatResponse(frame));
      }
      continue;
    }
    if (isTransportServiceFrameKind(frame.kind)) {
      if (frame.sessionId != activeConnection->sessionId()) {
        throw ProcessTransportSessionError(
            "The first process transport service frame belongs to a different session.");
      }
      pendingServiceFrame_ = std::move(frame);
      return InitialFrame::service_message;
    }
    if (frame.kind == TransportFrameKind::close) {
      if (frame.sessionId != activeConnection->sessionId()) {
        throw ProcessTransportSessionError(
            "The initial Terminate Session control belongs to a different session.");
      }
      try {
        activeConnection->messageHistory().acknowledgeOutgoing(
            frame.lastReceivedMessageNumber);
        if (!activeConnection->sequenceState().acceptIncomingMessageNumber(
                frame.sequenceNumber)) {
          continue;
        }
      } catch (TransportProtocolError const& error) {
        throw ProcessTransportSessionError(error.what());
      }
      receivedTerminateSessionSequenceNumber_ = frame.sequenceNumber;
      return InitialFrame::terminate_session;
    }
    if (frame.kind != TransportFrameKind::resume_request ||
        frame.sessionId != activeConnection->sessionId()) {
      throw ProcessTransportSessionError(
          "The process transport received an unexpected initial control frame.");
    }
    try {
      activeConnection->messageHistory().acknowledgeOutgoing(
          frame.lastReceivedMessageNumber);
      if (!activeConnection->sequenceState().acceptIncomingMessageNumber(
              frame.sequenceNumber)) {
        continue;
      }
      request = decodeTransportResumeRequest(frame);
    } catch (TransportProtocolError const& error) {
      throw ProcessTransportSessionError(error.what());
    }
    receivedResumeRequestSequenceNumber_ = frame.sequenceNumber;
    return InitialFrame::resume_request;
  }
}

TransportResumeStatus ProcessTransportSession::assessResumeRequest(
    TransportResumeRequest const& request) const {
  auto const activeConnection = connection();
  auto const lastReceivedFederateSequenceNumber =
      activeConnection->messageHistory().lastReceivedSequenceNumber();
  auto const sessionId = activeConnection->sessionId();
  if (request.sessionId == 0U || request.sessionId != sessionId) {
    return TransportResumeStatus{
        sessionId,
        TransportResumeStatusCode::invalid_session,
        lastReceivedFederateSequenceNumber};
  }
  if (!validSequenceNumberOrInvalid(
          request.lastReceivedRtiSequenceNumber) ||
      !validSequenceNumberOrInvalid(
          request.oldestAvailableFederateSequenceNumber)) {
    throw ProcessTransportSessionError(
        "A Resume Request contains an invalid history sequence number.");
  }

  // The outgoing history check uses the RTI's local OAR and retained tail to
  // establish that every RTI service message after the peer's LRR is present.
  // The federate's OAF is validated by the federate after this status returns
  // the RTI's LRF. Numeric OAF/LRF comparison here is invalid because sequence
  // numbers also cover control frames that are not retained as service data.
  auto const rtiHistoryAvailable = activeConnection->messageHistory().canReplayAfter(
      request.lastReceivedRtiSequenceNumber);
  auto const status = rtiHistoryAvailable
      ? TransportResumeStatusCode::ok
      : TransportResumeStatusCode::insufficient_buffered_messages;
  return TransportResumeStatus{
      sessionId, status, lastReceivedFederateSequenceNumber};
}

bool ProcessTransportSession::sendResumeStatus(
    TransportResumeStatus const& status) {
  auto const activeConnection = connection();
  if (status.sessionId != activeConnection->sessionId()) {
    throw ProcessTransportSessionError(
        "A Resume Status must identify the active process transport session.");
  }
  if (status.status == TransportResumeStatusCode::ok &&
      !receivedResumeRequestSequenceNumber_) {
    throw ProcessTransportSessionError(
        "A successful Resume Status requires a received Resume Request.");
  }
  try {
    auto const sent = activeConnection->sendSequencedFrame([&status](auto sequenceNumber) {
      return makeTransportResumeStatus(status, sequenceNumber);
    });
    if (sent && status.status == TransportResumeStatusCode::ok) {
      activeConnection->sequenceState().beginIncomingReplay(
          *receivedResumeRequestSequenceNumber_);
      receivedResumeRequestSequenceNumber_.reset();
    }
    return sent;
  } catch (TransportProtocolError const& error) {
    throw ProcessTransportSessionError(error.what());
  }
}

bool ProcessTransportSession::receiveResumeStatus(
    TransportResumeStatus& status) {
  TransportFrame frame;
  if (!receiveResumeFrame(TransportFrameKind::resume_status, frame)) {
    return false;
  }
  try {
    status = decodeTransportResumeStatus(frame);
  } catch (TransportProtocolError const& error) {
    throw ProcessTransportSessionError(error.what());
  }
  if (status.status == TransportResumeStatusCode::ok) {
    try {
      connection()->sequenceState().beginIncomingReplay(frame.sequenceNumber);
    } catch (TransportProtocolError const& error) {
      throw ProcessTransportSessionError(error.what());
    }
  }
  return true;
}

bool ProcessTransportSession::replayOutgoingAfter(
    std::uint32_t peerLastReceived) {
  auto const activeConnection = connection();
  auto& history = activeConnection->messageHistory();
  auto const replay = history.replayAfter(peerLastReceived);
  if (!replay) {
    return false;
  }

  for (auto const& entry : *replay) {
    TransportFrame frame{
        entry.frameKind,
        entry.payload,
        entry.sequenceNumber,
        activeConnection->sessionId(),
        history.lastReceivedSequenceNumber()};
    if (!activeConnection->sendFrame(frame)) {
      return false;
    }
  }
  return true;
}

bool ProcessTransportSession::receiveResumeFrame(
    TransportFrameKind expectedKind,
    TransportFrame& frame) {
  auto const activeConnection = connection();
  while (activeConnection->receiveFrame(frame)) {
    if (frame.kind != expectedKind ||
        frame.sessionId != activeConnection->sessionId()) {
      throw ProcessTransportSessionError(
          "The process transport received an unexpected resume control frame.");
    }
    try {
      activeConnection->messageHistory().acknowledgeOutgoing(
          frame.lastReceivedMessageNumber);
    } catch (TransportProtocolError const& error) {
      throw ProcessTransportSessionError(error.what());
    }
    if (activeConnection->sequenceState().acceptIncomingMessageNumber(
            frame.sequenceNumber)) {
      return true;
    }
  }
  return false;
}

bool ProcessTransportSession::request(
    TransportServiceMessage requestMessage,
    TransportServiceMessage& response) {
  if (requestMessage.kind != TransportServiceMessageKind::request ||
      requestMessage.requestId == 0U) {
    throw ProcessTransportSessionError(
        "A process transport request must have request kind and identity.");
  }
  if (!send(requestMessage) || !receive(response)) {
    return false;
  }
  if (response.kind != TransportServiceMessageKind::response ||
      response.requestId != requestMessage.requestId ||
      response.operation != requestMessage.operation) {
    throw ProcessTransportSessionError(
        "The process transport response does not match its request.");
  }
  return true;
}

std::shared_ptr<ProcessTransportConnection>
ProcessTransportSession::connection() const {
  std::scoped_lock lock(connectionState_->mutex);
  return connectionState_->connection;
}

bool ProcessTransportServiceDispatcher::serveOne(
    ProcessTransportSession& session,
    Handler const& handler,
    bool* terminateSessionReceived) {
  if (terminateSessionReceived != nullptr) {
    *terminateSessionReceived = false;
  }
  if (!handler) {
    throw ProcessTransportSessionError(
        "A process transport service dispatcher requires a handler.");
  }
  TransportServiceMessage request;
  auto receivedFrameKind = TransportFrameKind::data;
  if (!session.receive(request, &receivedFrameKind)) {
    return false;
  }
  if (receivedFrameKind == TransportFrameKind::close) {
    if (terminateSessionReceived != nullptr) {
      *terminateSessionReceived = true;
      return false;
    }
    throw ProcessTransportSessionError(
        "A Terminate Session control was received outside the session host.");
  }
  if (receivedFrameKind == TransportFrameKind::close_ack) {
    throw ProcessTransportSessionError(
        "The process federation service received an unexpected Session Terminated control.");
  }
  if (request.kind != TransportServiceMessageKind::request ||
      request.requestId == 0U) {
    throw ProcessTransportSessionError(
        "The process transport service dispatcher received a non-request.");
  }
  auto response = handler(request);
  if (response.kind != TransportServiceMessageKind::response ||
      response.requestId != request.requestId ||
      response.operation != request.operation) {
    throw ProcessTransportSessionError(
        "The process transport service handler returned a mismatched response.");
  }
  return session.send(response);
}

}  // namespace umbra::detail
