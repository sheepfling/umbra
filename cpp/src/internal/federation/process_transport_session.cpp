#include "internal/federation/process_transport_session.hpp"

#include <utility>

namespace umbra::detail {
namespace {

[[nodiscard]] TransportFrame dataFrameFor(TransportServiceMessage const& message) {
  return TransportFrame{
      TransportFrameKind::data,
      encodeTransportServiceMessage(message)};
}

[[nodiscard]] TransportServiceMessage messageFromFrame(TransportFrame const& frame) {
  if (frame.kind != TransportFrameKind::data) {
    throw ProcessTransportSessionError(
        "The process transport session received a non-data frame.");
  }
  try {
    return decodeTransportServiceMessage(frame.payload);
  } catch (TransportServiceProtocolError const& error) {
    throw ProcessTransportSessionError(error.what());
  }
}

}  // namespace

ProcessTransportSession::ProcessTransportSession(
    std::shared_ptr<ProcessTransportConnection> connection)
    : connection_(std::move(connection)) {
  if (!connection_) {
    throw ProcessTransportSessionError(
        "A process transport session requires an open connection object.");
  }
}

bool ProcessTransportSession::send(TransportServiceMessage const& message) {
  return connection_->sendFrame(dataFrameFor(message));
}

bool ProcessTransportSession::receive(TransportServiceMessage& message) {
  TransportFrame frame;
  if (!connection_->receiveFrame(frame)) {
    return false;
  }
  message = messageFromFrame(frame);
  return true;
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
ProcessTransportSession::connection() const noexcept {
  return connection_;
}

bool ProcessTransportServiceDispatcher::serveOne(
    ProcessTransportSession& session,
    Handler const& handler) {
  if (!handler) {
    throw ProcessTransportSessionError(
        "A process transport service dispatcher requires a handler.");
  }
  TransportServiceMessage request;
  if (!session.receive(request)) {
    return false;
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
