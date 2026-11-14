#include <catch2/catch_test_macros.hpp>

#include "internal/encoding/transport_wire.hpp"
#include "internal/federation/process_transport.hpp"
#include "internal/federation/process_transport_session.hpp"
#include "internal/federation/transport_service_protocol.hpp"
#include "internal/federation/transport_protocol.hpp"
#include "internal/platform/socket_operations.hpp"

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <exception>
#include <future>
#include <map>
#include <optional>
#include <span>
#include <stdexcept>
#include <thread>
#include <utility>
#include <vector>

namespace {

using umbra::detail::ProcessTransportAddress;
using umbra::detail::ProcessTransportConnection;
using umbra::detail::ProcessTransportListener;
using umbra::detail::ProcessTransportSession;
using umbra::detail::ProcessTransportSessionState;
using umbra::detail::TransportEndpointIdentity;
using umbra::detail::TransportFrame;
using umbra::detail::TransportFrameKind;
using umbra::detail::TransportResumeRequest;
using umbra::detail::TransportResumeStatus;
using umbra::detail::TransportResumeStatusCode;
using umbra::detail::TransportServiceMessage;
using umbra::detail::TransportServiceMessageKind;
using umbra::detail::TransportServiceOperation;
using umbra::detail::TransportServiceStatus;
using umbra::detail::decodeTransportHeartbeatResponse;
using umbra::detail::kInvalidTransportSequenceNumber;
using umbra::detail::makeTransportHeartbeat;

}  // namespace

TEST_CASE(
    "Private process transport exchanges framed data after New Session negotiation",
    "[unit][foundation][transport][process-boundary][transport-contract]"
    "[new-session-negotiation][2025]") {
  auto listener = ProcessTransportListener::listen({"127.0.0.1", 0U});
  REQUIRE(listener != nullptr);
  REQUIRE(listener->address().port != 0U);

  TransportEndpointIdentity const serverIdentity{"server-process", 0x2002U};
  TransportEndpointIdentity const clientIdentity{"client-process", 0x1001U};
  std::exception_ptr serverFailure;
  TransportFrame serverReceived;
  std::uint64_t serverSessionId = 0U;

  std::thread server([&] {
    try {
      auto connection = listener->accept(
          nullptr,
          serverIdentity,
          [](std::wstring) {},
          [](std::wstring) { return false; });
      serverSessionId = connection->sessionId();
      TransportFrame request;
      if (!connection->receiveFrame(request)) {
        throw std::runtime_error("server could not receive the request frame");
      }
      serverReceived = request;
      if (!connection->sendFrame({
              TransportFrameKind::data,
              std::vector<std::uint8_t>{0x7aU, 0x7bU, 0x7cU},
              0U,
              connection->sessionId(),
              0U})) {
        throw std::runtime_error("server could not send the response frame");
      }
      connection->close();
    } catch (...) {
      serverFailure = std::current_exception();
    }
  });

  std::exception_ptr clientFailure;
  TransportFrame clientReceived;
  try {
    auto client = ProcessTransportConnection::connectClient(
        nullptr,
        {"127.0.0.1", listener->address().port},
        clientIdentity,
        [](std::wstring) {},
        [](std::wstring) { return false; });
    REQUIRE(client->open());
    REQUIRE_FALSE(client->peerIdentity().endpointId.empty());
    REQUIRE(client->sessionId() != 0U);
    REQUIRE(client->peerIdentity().sessionId == client->sessionId());
    REQUIRE(client->sendFrame({
        TransportFrameKind::data,
        std::vector<std::uint8_t>{0x01U, 0x02U, 0x03U},
        0U,
        client->sessionId(),
        kInvalidTransportSequenceNumber}));
    REQUIRE(client->receiveFrame(clientReceived));
    client->close();
  } catch (...) {
    clientFailure = std::current_exception();
  }

  listener.reset();
  if (server.joinable()) {
    server.join();
  }

  if (clientFailure) {
    std::rethrow_exception(clientFailure);
  }
  REQUIRE_FALSE(serverFailure);
  REQUIRE(serverSessionId != 0U);
  REQUIRE(clientReceived.sessionId == serverSessionId);
  REQUIRE(serverReceived.kind == TransportFrameKind::data);
  REQUIRE(serverReceived.payload == std::vector<std::uint8_t>{0x01U, 0x02U, 0x03U});
  REQUIRE(serverReceived.sequenceNumber == 0U);
  REQUIRE(serverReceived.sessionId == serverSessionId);
  REQUIRE(serverReceived.lastReceivedMessageNumber ==
          kInvalidTransportSequenceNumber);
  REQUIRE(clientReceived.kind == TransportFrameKind::data);
  REQUIRE(clientReceived.payload == std::vector<std::uint8_t>{0x7aU, 0x7bU, 0x7cU});
  REQUIRE(clientReceived.sequenceNumber == 0U);
  REQUIRE(clientReceived.lastReceivedMessageNumber == 0U);
}

TEST_CASE(
    "Private process transport rejects unsupported New Session versions and closes before session creation",
    "[unit][internal][foundation][transport][process-boundary]"
    "[new-session-negotiation][unsupported-protocol-version][2025]") {
  auto listener = ProcessTransportListener::listen({"127.0.0.1", 0U});
  REQUIRE(listener != nullptr);
  REQUIRE(listener->address().port != 0U);

  std::exception_ptr serverFailure;
  std::shared_ptr<ProcessTransportConnection> acceptedConnection;
  std::thread server([&] {
    try {
      acceptedConnection = listener->accept(
          nullptr,
          {"unsupported-version-server", 0U},
          [](std::wstring) {},
          [](std::wstring) { return false; });
    } catch (...) {
      serverFailure = std::current_exception();
    }
  });

  auto clientSocket = umbra::detail::platform::invalidSocket;
  std::exception_ptr clientFailure;
  umbra::detail::TransportNewSessionStatus status;
  bool responseReceived = false;
  bool serverClosedAfterResponse = false;
  try {
    clientSocket = umbra::detail::platform::connectSocket(
        "127.0.0.1", listener->address().port);
    auto const request = umbra::detail::encodeTransportFrame(
        umbra::detail::makeTransportNewSessionRequest(
            umbra::detail::kTransportHla4ProtocolVersion + 1U));
    umbra::detail::platform::sendAll(clientSocket, request);

    auto const responseDeadline = std::chrono::steady_clock::now() +
        std::chrono::seconds(5);
    std::array<std::uint8_t, 4U> sizePrefix{};
    if (umbra::detail::platform::receiveExact(
            clientSocket, sizePrefix, responseDeadline) !=
        umbra::detail::platform::ReceiveExactResult::complete) {
      throw std::runtime_error(
          "The process transport closed before returning New Session Status.");
    }
    auto const responseSize = umbra::detail::transport_wire::readBigEndian32(
        sizePrefix, 0U);
    if (responseSize < umbra::detail::kTransportFrameHeaderSize ||
        responseSize > umbra::detail::kTransportFrameHeaderSize +
                umbra::detail::kTransportMaximumPayloadBytes) {
      throw std::runtime_error(
          "The process transport returned an invalid New Session Status size.");
    }

    std::vector<std::uint8_t> responseBytes(responseSize);
    std::copy(sizePrefix.begin(), sizePrefix.end(), responseBytes.begin());
    if (umbra::detail::platform::receiveExact(
            clientSocket,
            std::span<std::uint8_t>(responseBytes).subspan(sizePrefix.size()),
            responseDeadline) !=
        umbra::detail::platform::ReceiveExactResult::complete) {
      throw std::runtime_error(
          "The process transport returned a truncated New Session Status.");
    }

    auto const responseFrame = umbra::detail::decodeTransportFrame(responseBytes);
    REQUIRE(responseFrame.sequenceNumber == 0U);
    status = umbra::detail::decodeTransportNewSessionStatus(responseFrame);
    responseReceived = true;
    std::array<std::uint8_t, 1U> trailingByte{};
    serverClosedAfterResponse =
        umbra::detail::platform::receiveExact(
            clientSocket, trailingByte, responseDeadline) ==
        umbra::detail::platform::ReceiveExactResult::end_of_stream;
  } catch (...) {
    clientFailure = std::current_exception();
  }

  if (clientSocket != umbra::detail::platform::invalidSocket) {
    umbra::detail::platform::closeSocket(clientSocket);
  }
  listener->close();
  if (server.joinable()) {
    server.join();
  }
  if (clientFailure) {
    std::rethrow_exception(clientFailure);
  }
  if (serverFailure) {
    std::rethrow_exception(serverFailure);
  }

  REQUIRE(responseReceived);
  REQUIRE(status.status ==
          umbra::detail::TransportNewSessionStatusCode::unsupported_protocol_version);
  REQUIRE(status.sessionId == 0U);
  REQUIRE(serverClosedAfterResponse);
  REQUIRE_FALSE(acceptedConnection);
}

TEST_CASE(
    "Private process transport close cancels a pending New Session Status wait",
    "[unit][internal][transport][process-boundary]"
    "[process-connection-fault-recovery-history][new-session-status-cancellation]") {
  auto listener = ProcessTransportListener::listen({"127.0.0.1", 0U});
  REQUIRE(listener != nullptr);

  std::promise<std::shared_ptr<ProcessTransportConnection>> pendingConnection;
  auto pendingConnectionFuture = pendingConnection.get_future();
  std::promise<void> connectionAttemptFinished;
  auto connectionAttemptFinishedFuture = connectionAttemptFinished.get_future();
  bool observedTransportCancellation = false;
  std::exception_ptr unexpectedFailure;
  std::thread connectingThread([&] {
    try {
      static_cast<void>(ProcessTransportConnection::connectClient(
          nullptr,
          {"127.0.0.1", listener->address().port},
          {"closing-client", 0xCA11U},
          [](std::wstring) {},
          [](std::wstring) { return false; },
          {},
          {},
          [&](std::shared_ptr<ProcessTransportConnection> const& connection) {
            pendingConnection.set_value(connection);
          }));
    } catch (umbra::detail::ProcessTransportError const&) {
      observedTransportCancellation = true;
    } catch (...) {
      unexpectedFailure = std::current_exception();
    }
    connectionAttemptFinished.set_value();
  });

  auto const connectionPublished =
      pendingConnectionFuture.wait_for(std::chrono::seconds(5)) ==
      std::future_status::ready;
  if (connectionPublished) {
    pendingConnectionFuture.get()->close();
  }
  auto const attemptFinished =
      connectionAttemptFinishedFuture.wait_for(std::chrono::seconds(5)) ==
      std::future_status::ready;
  if (!attemptFinished) {
    listener->close();
  }
  connectingThread.join();
  listener->close();

  REQUIRE(connectionPublished);
  REQUIRE(attemptFinished);
  REQUIRE(observedTransportCancellation);
  REQUIRE_FALSE(unexpectedFailure);
}

TEST_CASE(
    "Private process sessions sequence and acknowledge service-message history across wrappers",
    "[unit][foundation][transport][process-boundary][service-message-history][process-connection-fault-recovery-history]") {
  auto listener = ProcessTransportListener::listen({"127.0.0.1", 0U});
  REQUIRE(listener != nullptr);

  std::exception_ptr serverFailure;
  bool serverObservedRequest = false;
  std::size_t serverRetainedCount = 0U;
  std::thread server([&] {
    try {
      auto connection = listener->accept(
          nullptr,
          {"history-service", 0U},
          [](std::wstring) {},
          [](std::wstring) { return false; });
      TransportServiceMessage request;
      ProcessTransportSession requestSession(connection);
      if (!requestSession.receive(request)) {
        throw std::runtime_error("server could not receive the service request");
      }
      serverObservedRequest =
          request.kind == TransportServiceMessageKind::request &&
          request.requestId == 41U &&
          connection->messageHistory().lastReceivedSequenceNumber() == 0U;

      ProcessTransportSession responseSession(connection);
      TransportServiceMessage response{
          TransportServiceMessageKind::response,
          request.operation,
          TransportServiceStatus::ok,
          request.requestId,
          std::vector<std::uint8_t>{0xaaU, 0xbbU}};
      if (!responseSession.send(response)) {
        throw std::runtime_error("server could not send the service response");
      }
      serverRetainedCount = connection->messageHistory().retainedCount();
      connection->close();
    } catch (...) {
      serverFailure = std::current_exception();
    }
  });

  std::exception_ptr clientFailure;
  bool clientObservedResponse = false;
  std::size_t clientRetainedCount = 0U;
  std::optional<std::vector<umbra::detail::TransportMessageHistoryEntry>>
      currentSuffix;
  std::optional<std::vector<umbra::detail::TransportMessageHistoryEntry>>
      unavailableSuffix;
  try {
    auto connection = ProcessTransportConnection::connectClient(
        nullptr,
        {"127.0.0.1", listener->address().port},
        {"history-federate", 0U},
        [](std::wstring) {},
        [](std::wstring) { return false; });
    TransportServiceMessage request{
        TransportServiceMessageKind::request,
        TransportServiceOperation::get_federate_handle,
        TransportServiceStatus::ok,
        41U,
        std::vector<std::uint8_t>{0x01U}};
    ProcessTransportSession requestSession(connection);
    if (!requestSession.send(request)) {
      throw std::runtime_error("client could not send the service request");
    }
    REQUIRE(connection->messageHistory().retainedCount() == 1U);

    TransportServiceMessage response;
    ProcessTransportSession responseSession(connection);
    if (!responseSession.receive(response)) {
      throw std::runtime_error("client could not receive the service response");
    }
    clientObservedResponse =
        response.kind == TransportServiceMessageKind::response &&
        response.requestId == 41U &&
        response.payload == std::vector<std::uint8_t>{0xaaU, 0xbbU};
    clientRetainedCount = connection->messageHistory().retainedCount();
    currentSuffix = connection->messageHistory().replayAfter(0U);
    unavailableSuffix = connection->messageHistory().replayAfter(
        kInvalidTransportSequenceNumber);
  } catch (...) {
    clientFailure = std::current_exception();
  }

  listener.reset();
  if (server.joinable()) {
    server.join();
  }
  if (clientFailure) {
    std::rethrow_exception(clientFailure);
  }
  REQUIRE_FALSE(serverFailure);
  REQUIRE(serverObservedRequest);
  REQUIRE(clientObservedResponse);
  REQUIRE(serverRetainedCount == 1U);
  REQUIRE(clientRetainedCount == 0U);
  REQUIRE(currentSuffix.has_value());
  REQUIRE(currentSuffix->empty());
  REQUIRE_FALSE(unavailableSuffix.has_value());
}

TEST_CASE(
    "Private process transport sequences resume controls without advancing the service LRM",
    "[unit][internal][transport][process-boundary][process-connection-fault-recovery-history][resume-control-exchange]") {
  auto listener = ProcessTransportListener::listen({"127.0.0.1", 0U});
  REQUIRE(listener != nullptr);

  std::exception_ptr serverFailure;
  bool serverReceivedResume = false;
  bool serverResumeDidNotAdvanceServiceLrm = false;
  bool serverReceivedFirstServiceRequestAtSequenceOne = false;
  std::thread server([&] {
    try {
      auto connection = listener->accept(
          nullptr,
          {"resume-service", 0U},
          [](std::wstring) {},
          [](std::wstring) { return false; });
      ProcessTransportSession session(connection);

      TransportResumeRequest request;
      if (!session.receiveResumeRequest(request)) {
        throw std::runtime_error("server could not receive the Resume Request");
      }
      serverReceivedResume =
          request.sessionId == connection->sessionId() &&
          request.lastReceivedRtiSequenceNumber == kInvalidTransportSequenceNumber &&
          request.oldestAvailableFederateSequenceNumber ==
              kInvalidTransportSequenceNumber;
      serverResumeDidNotAdvanceServiceLrm =
          connection->messageHistory().lastReceivedSequenceNumber() ==
              kInvalidTransportSequenceNumber &&
          connection->sequenceState().lastReceivedMessageNumber() == 0U &&
          connection->messageHistory().retainedCount() == 0U;

      if (!session.sendResumeStatus(TransportResumeStatus{
              connection->sessionId(),
              TransportResumeStatusCode::ok,
              kInvalidTransportSequenceNumber})) {
        throw std::runtime_error("server could not send the Resume Status");
      }

      TransportServiceMessage requestMessage;
      if (!session.receive(requestMessage)) {
        throw std::runtime_error("server could not receive the first service message");
      }
      serverReceivedFirstServiceRequestAtSequenceOne =
          requestMessage.kind == TransportServiceMessageKind::request &&
          requestMessage.requestId == 73U &&
          connection->sequenceState().lastReceivedMessageNumber() == 1U &&
          connection->messageHistory().lastReceivedSequenceNumber() == 1U;

      TransportServiceMessage const response{
          TransportServiceMessageKind::response,
          requestMessage.operation,
          TransportServiceStatus::ok,
          requestMessage.requestId,
          std::vector<std::uint8_t>{0x51U}};
      if (!session.send(response)) {
        throw std::runtime_error("server could not send the first service response");
      }
      connection->close();
    } catch (...) {
      serverFailure = std::current_exception();
    }
  });

  std::exception_ptr clientFailure;
  bool clientReceivedSuccessfulResume = false;
  bool clientResumeDidNotAdvanceServiceLrm = false;
  bool clientReceivedFirstServiceResponseAtSequenceOne = false;
  try {
    auto connection = ProcessTransportConnection::connectClient(
        nullptr,
        {"127.0.0.1", listener->address().port},
        {"resume-federate", 0U},
        [](std::wstring) {},
        [](std::wstring) { return false; });
    ProcessTransportSession session(connection);

    if (!session.sendResumeRequest(TransportResumeRequest{
            connection->sessionId(),
            kInvalidTransportSequenceNumber,
            kInvalidTransportSequenceNumber})) {
      throw std::runtime_error("client could not send the Resume Request");
    }
    TransportResumeStatus status;
    if (!session.receiveResumeStatus(status)) {
      throw std::runtime_error("client could not receive the Resume Status");
    }
    clientReceivedSuccessfulResume =
        status.sessionId == connection->sessionId() &&
        status.status == TransportResumeStatusCode::ok &&
        status.lastReceivedFederateSequenceNumber ==
            kInvalidTransportSequenceNumber;
    clientResumeDidNotAdvanceServiceLrm =
        connection->messageHistory().lastReceivedSequenceNumber() ==
            kInvalidTransportSequenceNumber &&
        connection->sequenceState().lastReceivedMessageNumber() == 0U &&
        connection->messageHistory().retainedCount() == 0U;

    TransportServiceMessage const request{
        TransportServiceMessageKind::request,
        TransportServiceOperation::get_federate_handle,
        TransportServiceStatus::ok,
        73U,
        std::vector<std::uint8_t>{0x41U}};
    if (!session.send(request)) {
      throw std::runtime_error("client could not send the first service request");
    }
    TransportServiceMessage response;
    if (!session.receive(response)) {
      throw std::runtime_error("client could not receive the first service response");
    }
    clientReceivedFirstServiceResponseAtSequenceOne =
        response.kind == TransportServiceMessageKind::response &&
        response.requestId == request.requestId &&
        response.payload == std::vector<std::uint8_t>{0x51U} &&
        connection->sequenceState().lastReceivedMessageNumber() == 1U &&
        connection->messageHistory().lastReceivedSequenceNumber() == 1U;
  } catch (...) {
    clientFailure = std::current_exception();
  }

  listener.reset();
  if (server.joinable()) {
    server.join();
  }
  if (clientFailure) {
    std::rethrow_exception(clientFailure);
  }
  REQUIRE_FALSE(serverFailure);
  REQUIRE(serverReceivedResume);
  REQUIRE(serverResumeDidNotAdvanceServiceLrm);
  REQUIRE(serverReceivedFirstServiceRequestAtSequenceOne);
  REQUIRE(clientReceivedSuccessfulResume);
  REQUIRE(clientResumeDidNotAdvanceServiceLrm);
  REQUIRE(clientReceivedFirstServiceResponseAtSequenceOne);
}

TEST_CASE(
    "Private process transport preserves session state and replays retained services after reconnect",
    "[unit][internal][transport][process-boundary][process-connection-fault-recovery-history][resume-replay]") {
  auto listener = ProcessTransportListener::listen({"127.0.0.1", 0U});
  REQUIRE(listener != nullptr);

  auto clientState = std::make_shared<ProcessTransportSessionState>();
  std::map<std::uint64_t, std::shared_ptr<ProcessTransportSessionState>>
      serverStates;
  auto resolveServerState = [&serverStates](std::uint64_t sessionId) {
    auto const state = serverStates.find(sessionId);
    return state == serverStates.end()
        ? std::shared_ptr<ProcessTransportSessionState>{}
        : state->second;
  };
  std::shared_ptr<ProcessTransportSessionState> serverState;
  std::promise<void> releaseInitialServerConnection;
  auto releaseInitialServerConnectionFuture =
      releaseInitialServerConnection.get_future();
  std::promise<void> initialServerConnectionClosed;
  auto initialServerConnectionClosedFuture =
      initialServerConnectionClosed.get_future();

  std::exception_ptr serverFailure;
  bool serverAcceptedResumeForSameSession = false;
  bool serverReplayedPendingEvent = false;
  bool serverReceivedOriginalReplayOrder = false;
  bool serverAdvancedAfterReplayFence = false;
  std::thread server([&] {
    bool signaledInitialClose = false;
    try {
      auto initialConnection = listener->accept(
          nullptr,
          {"resume-service", 0U},
          [](std::wstring) {},
          [](std::wstring) { return false; },
          {},
          {},
          resolveServerState);
      serverState = initialConnection->sessionState();
      serverStates.insert_or_assign(
          initialConnection->sessionId(), serverState);
      ProcessTransportSession initialServerSession(initialConnection);
      if (!initialServerSession.send(TransportServiceMessage{
              TransportServiceMessageKind::event,
              TransportServiceOperation::receive_interaction,
              TransportServiceStatus::ok,
              0U,
              std::vector<std::uint8_t>{0xe1U}})) {
        throw std::runtime_error(
            "server could not retain a pending server-to-client event");
      }
      releaseInitialServerConnectionFuture.wait();
      initialConnection->close();
      initialServerConnectionClosed.set_value();
      signaledInitialClose = true;

      auto connection = listener->accept(
          nullptr,
          {"resume-service", 0U},
          [](std::wstring) {},
          [](std::wstring) { return false; },
          {},
          {},
          resolveServerState);
      if (connection->sessionState() != serverState) {
        throw std::runtime_error(
            "server did not resolve the replacement socket to retained session state");
      }
      ProcessTransportSession session(connection);

      TransportResumeRequest resumeRequest;
      if (!session.receiveResumeRequest(resumeRequest)) {
        throw std::runtime_error("server could not receive the Resume Request");
      }
      serverAcceptedResumeForSameSession =
          resumeRequest.sessionId == serverState->sessionId() &&
          resumeRequest.lastReceivedRtiSequenceNumber ==
              kInvalidTransportSequenceNumber &&
          resumeRequest.oldestAvailableFederateSequenceNumber == 0U &&
          connection->sequenceState().lastReceivedMessageNumber() == 2U;

      if (!session.sendResumeStatus(TransportResumeStatus{
              connection->sessionId(),
              TransportResumeStatusCode::ok,
              connection->messageHistory().lastReceivedSequenceNumber()})) {
        throw std::runtime_error("server could not send the Resume Status");
      }
      if (!session.replayOutgoingAfter(
              resumeRequest.lastReceivedRtiSequenceNumber)) {
        throw std::runtime_error(
            "server could not replay its pending server-to-client event");
      }
      serverReplayedPendingEvent = true;

      TransportServiceMessage firstReplay;
      TransportServiceMessage secondReplay;
      if (!session.receive(firstReplay) || !session.receive(secondReplay)) {
        throw std::runtime_error("server could not receive both replayed requests");
      }
      serverReceivedOriginalReplayOrder =
          firstReplay.kind == TransportServiceMessageKind::request &&
          firstReplay.requestId == 101U &&
          secondReplay.kind == TransportServiceMessageKind::request &&
          secondReplay.requestId == 102U &&
          connection->messageHistory().lastReceivedSequenceNumber() == 1U &&
          connection->sequenceState().lastReceivedMessageNumber() == 2U;

      for (auto const& request : {firstReplay, secondReplay}) {
        TransportServiceMessage const response{
            TransportServiceMessageKind::response,
            request.operation,
            TransportServiceStatus::ok,
            request.requestId,
            std::vector<std::uint8_t>{static_cast<std::uint8_t>(request.requestId)}};
        if (!session.send(response)) {
          throw std::runtime_error("server could not answer a replayed request");
        }
      }

      TransportServiceMessage continuedRequest;
      if (!session.receive(continuedRequest)) {
        throw std::runtime_error("server could not receive post-replay service traffic");
      }
      serverAdvancedAfterReplayFence =
          continuedRequest.kind == TransportServiceMessageKind::request &&
          continuedRequest.requestId == 103U &&
          connection->messageHistory().lastReceivedSequenceNumber() == 3U &&
          connection->sequenceState().lastReceivedMessageNumber() == 3U;
      TransportServiceMessage const continuedResponse{
          TransportServiceMessageKind::response,
          continuedRequest.operation,
          TransportServiceStatus::ok,
          continuedRequest.requestId,
          std::vector<std::uint8_t>{0x67U}};
      if (!session.send(continuedResponse)) {
        throw std::runtime_error("server could not answer post-replay traffic");
      }
    } catch (...) {
      serverFailure = std::current_exception();
      if (!signaledInitialClose) {
        try {
          initialServerConnectionClosed.set_value();
        } catch (std::future_error const&) {
        }
      }
    }
  });

  std::exception_ptr clientFailure;
  bool clientKeptTheSameSessionIdentity = false;
  bool clientReplayedRetainedHistory = false;
  bool clientReceivedPendingEvent = false;
  bool clientReceivedAllResponses = false;
  std::shared_ptr<ProcessTransportConnection> initialClientConnection;
  try {
    initialClientConnection = ProcessTransportConnection::connectClient(
        nullptr,
        {"127.0.0.1", listener->address().port},
        {"resume-federate", 0U},
        [](std::wstring) {},
        [](std::wstring) { return false; },
        {},
        clientState);
    auto const originalSessionId = initialClientConnection->sessionId();
    ProcessTransportSession initialSession(initialClientConnection);
    TransportServiceMessage const firstRequest{
        TransportServiceMessageKind::request,
        TransportServiceOperation::get_federate_handle,
        TransportServiceStatus::ok,
        101U,
        std::vector<std::uint8_t>{0x11U}};
    TransportServiceMessage const secondRequest{
        TransportServiceMessageKind::request,
        TransportServiceOperation::get_federate_handle,
        TransportServiceStatus::ok,
        102U,
        std::vector<std::uint8_t>{0x22U}};
    if (!initialSession.send(firstRequest) || !initialSession.send(secondRequest)) {
      throw std::runtime_error("client could not retain pre-fault service traffic");
    }
    if (initialClientConnection->messageHistory().retainedCount() != 2U) {
      throw std::runtime_error("client did not retain both unacknowledged requests");
    }

    releaseInitialServerConnection.set_value();
    if (initialServerConnectionClosedFuture.wait_for(std::chrono::seconds(5)) !=
        std::future_status::ready) {
      throw std::runtime_error("server did not close the initial connection");
    }
    initialClientConnection->close();

    auto replacementConnection = ProcessTransportConnection::connectClient(
        nullptr,
        {"127.0.0.1", listener->address().port},
        {"resume-federate", 0U},
        [](std::wstring) {},
        [](std::wstring) { return false; },
        {},
        clientState);
    clientKeptTheSameSessionIdentity =
        replacementConnection->sessionId() == originalSessionId &&
        replacementConnection->sessionState() == clientState;
    ProcessTransportSession replacementSession(replacementConnection);

    auto const oldestAvailable =
        replacementConnection->messageHistory().oldestAvailableSequenceNumber();
    if (!replacementSession.sendResumeRequest(TransportResumeRequest{
            replacementConnection->sessionId(),
            replacementConnection->messageHistory().lastReceivedSequenceNumber(),
            oldestAvailable.value_or(kInvalidTransportSequenceNumber)})) {
      throw std::runtime_error("client could not send the Resume Request");
    }
    TransportResumeStatus resumeStatus;
    if (!replacementSession.receiveResumeStatus(resumeStatus) ||
        resumeStatus.status != TransportResumeStatusCode::ok) {
      throw std::runtime_error("client did not receive a successful Resume Status");
    }
    if (!replacementSession.replayOutgoingAfter(
            resumeStatus.lastReceivedFederateSequenceNumber)) {
      throw std::runtime_error("client could not replay its available history");
    }
    clientReplayedRetainedHistory = true;

    TransportServiceMessage pendingEvent;
    TransportServiceMessage firstResponse;
    TransportServiceMessage secondResponse;
    if (!replacementSession.receive(pendingEvent) ||
        !replacementSession.receive(firstResponse) ||
        !replacementSession.receive(secondResponse)) {
      throw std::runtime_error(
          "client did not receive the pending event and both replay responses");
    }
    clientReceivedPendingEvent =
        pendingEvent.kind == TransportServiceMessageKind::event &&
        pendingEvent.operation == TransportServiceOperation::receive_interaction &&
        pendingEvent.payload == std::vector<std::uint8_t>{0xe1U};
    TransportServiceMessage const continuedRequest{
        TransportServiceMessageKind::request,
        TransportServiceOperation::get_federate_handle,
        TransportServiceStatus::ok,
        103U,
        std::vector<std::uint8_t>{0x33U}};
    if (!replacementSession.send(continuedRequest)) {
      throw std::runtime_error("client could not send post-replay service traffic");
    }
    TransportServiceMessage continuedResponse;
    if (!replacementSession.receive(continuedResponse)) {
      throw std::runtime_error("client did not receive the post-replay response");
    }
    clientReceivedAllResponses =
        firstResponse.requestId == 101U &&
        firstResponse.payload == std::vector<std::uint8_t>{101U} &&
        secondResponse.requestId == 102U &&
        secondResponse.payload == std::vector<std::uint8_t>{102U} &&
        continuedResponse.requestId == 103U &&
        continuedResponse.payload == std::vector<std::uint8_t>{0x67U} &&
        replacementConnection->messageHistory().retainedCount() == 0U;
    replacementConnection->close();
  } catch (...) {
    clientFailure = std::current_exception();
    try {
      releaseInitialServerConnection.set_value();
    } catch (std::future_error const&) {
    }
    if (initialClientConnection) {
      initialClientConnection->close();
    }
  }

  listener->close();
  listener.reset();
  if (server.joinable()) {
    server.join();
  }
  if (clientFailure) {
    std::rethrow_exception(clientFailure);
  }
  REQUIRE_FALSE(serverFailure);
  REQUIRE(clientKeptTheSameSessionIdentity);
  REQUIRE(clientReplayedRetainedHistory);
  REQUIRE(clientReceivedPendingEvent);
  REQUIRE(clientReceivedAllResponses);
  REQUIRE(serverAcceptedResumeForSameSession);
  REQUIRE(serverReplayedPendingEvent);
  REQUIRE(serverReceivedOriginalReplayOrder);
  REQUIRE(serverAdvancedAfterReplayFence);
}

TEST_CASE(
    "Private process transport excludes heartbeat controls from resumed service replay",
    "[unit][internal][transport][process-boundary][process-connection-fault-recovery-history][resume-control-replay-exclusion]") {
  std::vector<TransportFrame> sentFrames;
  auto connection = ProcessTransportConnection::createForTesting(
      [&sentFrames](TransportFrame const& frame) {
        sentFrames.push_back(frame);
        return true;
      });
  ProcessTransportSession session(connection);

  TransportServiceMessage const request{
      TransportServiceMessageKind::request,
      TransportServiceOperation::get_federate_handle,
      TransportServiceStatus::ok,
      1U,
      std::vector<std::uint8_t>{0x41U}};
  REQUIRE(session.send(request));
  REQUIRE(sentFrames.size() == 1U);
  auto const serviceFrame = sentFrames.front();
  REQUIRE(serviceFrame.kind == TransportFrameKind::data);

  REQUIRE(connection->sendSequencedFrame(
      [&connection](std::uint32_t sequenceNumber) {
        return makeTransportHeartbeat(
            connection->sessionId(),
            sequenceNumber,
            connection->messageHistory().lastReceivedSequenceNumber());
      }));
  REQUIRE(sentFrames.size() == 2U);
  REQUIRE(sentFrames.back().kind == TransportFrameKind::heartbeat);
  REQUIRE(connection->messageHistory().retainedCount() == 1U);

  REQUIRE(session.replayOutgoingAfter(kInvalidTransportSequenceNumber));
  REQUIRE(sentFrames.size() == 3U);
  REQUIRE(sentFrames.back().kind == TransportFrameKind::data);
  REQUIRE(sentFrames.back().sequenceNumber == serviceFrame.sequenceNumber);
  REQUIRE(sentFrames.back().payload == serviceFrame.payload);
  REQUIRE(connection->messageHistory().retainedCount() == 1U);
}

TEST_CASE(
    "Private process transport rejects successful Resume Status before receiving a request",
    "[unit][internal][transport][process-boundary][process-connection-fault-recovery-history][resume-control-exchange]") {
  bool statusFrameSent = false;
  auto connection = ProcessTransportConnection::createForTesting(
      [&](TransportFrame const&) {
        statusFrameSent = true;
        return true;
      });
  ProcessTransportSession session(connection);

  REQUIRE(connection != nullptr);
  REQUIRE_THROWS_AS(
      session.sendResumeStatus(TransportResumeStatus{
          connection->sessionId(),
          TransportResumeStatusCode::ok,
          kInvalidTransportSequenceNumber}),
      umbra::detail::ProcessTransportSessionError);
  REQUIRE_FALSE(statusFrameSent);
  REQUIRE(connection->sequenceState().nextOutgoingMessageNumber() == 0U);
}

TEST_CASE(
    "Private process transport resume checks RTI history without conflating control gaps",
    "[unit][internal][transport][process-boundary][process-connection-fault-recovery-history][resume-history-availability]") {
  auto assess = [](std::uint32_t lastReceivedFederate,
                   std::uint32_t lastReceivedRti,
                   std::uint32_t oldestAvailableFederate) {
    auto connection = ProcessTransportConnection::createForTesting(
        [](TransportFrame const&) { return true; });
    if (lastReceivedFederate != kInvalidTransportSequenceNumber) {
      static_cast<void>(
          connection->messageHistory().acceptIncoming(lastReceivedFederate));
    }
    ProcessTransportSession session(connection);
    return session.assessResumeRequest(TransportResumeRequest{
        connection->sessionId(),
        lastReceivedRti,
        oldestAvailableFederate});
  };

  auto connectionWithDiscardedRtiHistory =
      ProcessTransportConnection::createForTesting(
          [](TransportFrame const&) { return true; });
  std::vector<std::uint8_t> oversizedMessage(
      16U * 1024U * 1024U + 1U,
      0x5aU);
  static_cast<void>(connectionWithDiscardedRtiHistory->messageHistory().retainOutgoing(
      0U,
      oversizedMessage));
  ProcessTransportSession discardedRtiHistorySession(
      connectionWithDiscardedRtiHistory);
  REQUIRE(discardedRtiHistorySession.assessResumeRequest(
              TransportResumeRequest{
                  connectionWithDiscardedRtiHistory->sessionId(),
                  kInvalidTransportSequenceNumber,
                  kInvalidTransportSequenceNumber})
              .status ==
          TransportResumeStatusCode::insufficient_buffered_messages);

  auto invalidSessionConnection = ProcessTransportConnection::createForTesting(
      [](TransportFrame const&) { return true; });
  ProcessTransportSession invalidSession(invalidSessionConnection);
  REQUIRE(invalidSession.assessResumeRequest(TransportResumeRequest{
              invalidSessionConnection->sessionId() + 1U,
              kInvalidTransportSequenceNumber,
              kInvalidTransportSequenceNumber})
              .status == TransportResumeStatusCode::invalid_session);

  REQUIRE(assess(
              kInvalidTransportSequenceNumber,
              kInvalidTransportSequenceNumber,
              0U)
              .status == TransportResumeStatusCode::ok);
  REQUIRE(assess(
              kInvalidTransportSequenceNumber,
              kInvalidTransportSequenceNumber,
              kInvalidTransportSequenceNumber)
              .status == TransportResumeStatusCode::ok);
  REQUIRE(assess(3U, kInvalidTransportSequenceNumber, 2U).status ==
          TransportResumeStatusCode::ok);
  REQUIRE(assess(3U, kInvalidTransportSequenceNumber, 4U).status ==
          TransportResumeStatusCode::ok);
  REQUIRE(assess(0U, kInvalidTransportSequenceNumber, 0x7fffffffU).status ==
          TransportResumeStatusCode::ok);
  REQUIRE_THROWS_AS(
      assess(
          0U,
          kInvalidTransportSequenceNumber,
          kInvalidTransportSequenceNumber + 1U),
      umbra::detail::ProcessTransportSessionError);
}

TEST_CASE(
    "Private process sessions reply to heartbeats with a correlated Heartbeat Response",
    "[unit][internal][transport][process-boundary][heartbeat-response-table38][heartbeat-response-session]"
    "[process-connection-fault-recovery-history]") {
  auto listener = ProcessTransportListener::listen({"127.0.0.1", 0U});
  REQUIRE(listener != nullptr);

  std::exception_ptr serverFailure;
  bool serverReceivedRequest = false;
  std::thread server([&] {
    try {
      auto connection = listener->accept(
          nullptr,
          {"heartbeat-response-server", 0U},
          [](std::wstring) {},
          [](std::wstring) { return false; });
      ProcessTransportSession session(connection, true);
      TransportServiceMessage request;
      if (!session.receive(request)) {
        throw std::runtime_error(
            "server did not receive service traffic after the heartbeat");
      }
      serverReceivedRequest =
          request.kind == TransportServiceMessageKind::request &&
          request.requestId == 61U;
      TransportServiceMessage const response{
          TransportServiceMessageKind::response,
          request.operation,
          TransportServiceStatus::ok,
          request.requestId,
          std::vector<std::uint8_t>{0xaaU}};
      if (!session.send(response)) {
        throw std::runtime_error("server could not send its service response");
      }
      connection->close();
    } catch (...) {
      serverFailure = std::current_exception();
    }
  });

  std::exception_ptr clientFailure;
  bool clientReceivedExpectedHeartbeatResponse = false;
  bool clientReceivedServiceResponse = false;
  try {
    auto connection = ProcessTransportConnection::connectClient(
        nullptr,
        {"127.0.0.1", listener->address().port},
        {"heartbeat-response-client", 0U},
        [](std::wstring) {},
        [](std::wstring) { return false; });
    REQUIRE(connection->sendSequencedFrame(
        [&connection](std::uint32_t sequenceNumber) {
          return makeTransportHeartbeat(
              connection->sessionId(),
              sequenceNumber,
              connection->messageHistory().lastReceivedSequenceNumber());
        }));

    ProcessTransportSession session(connection);
    TransportServiceMessage const request{
        TransportServiceMessageKind::request,
        TransportServiceOperation::get_federate_handle,
        TransportServiceStatus::ok,
        61U,
        std::vector<std::uint8_t>{0x01U}};
    REQUIRE(session.send(request));

    TransportFrame responseFrame;
    REQUIRE(connection->receiveFrame(responseFrame));
    clientReceivedExpectedHeartbeatResponse =
        responseFrame.kind == TransportFrameKind::heartbeat_response &&
        decodeTransportHeartbeatResponse(responseFrame) == 0U;
    connection->messageHistory().acknowledgeOutgoing(
        responseFrame.lastReceivedMessageNumber);
    static_cast<void>(connection->sequenceState().acceptIncomingMessageNumber(
        responseFrame.sequenceNumber));

    TransportServiceMessage response;
    REQUIRE(session.receive(response));
    clientReceivedServiceResponse =
        response.kind == TransportServiceMessageKind::response &&
        response.requestId == 61U &&
        response.payload == std::vector<std::uint8_t>{0xaaU};
    connection->close();
  } catch (...) {
    clientFailure = std::current_exception();
  }

  listener.reset();
  if (server.joinable()) {
    server.join();
  }
  if (clientFailure) {
    std::rethrow_exception(clientFailure);
  }
  REQUIRE_FALSE(serverFailure);
  REQUIRE(serverReceivedRequest);
  REQUIRE(clientReceivedExpectedHeartbeatResponse);
  REQUIRE(clientReceivedServiceResponse);
}
