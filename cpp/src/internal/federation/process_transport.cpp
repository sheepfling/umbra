#include "internal/federation/process_transport.hpp"
#include "internal/observability/runtime_instrumentation.hpp"

#include "internal/encoding/transport_wire.hpp"
#include "internal/platform/socket_operations.hpp"

#include <array>
#include <atomic>
#include <exception>
#include <limits>
#include <new>
#include <optional>
#include <span>
#include <utility>
#include <vector>

namespace umbra::detail {
namespace {

constexpr std::uintptr_t kInvalidSocket = std::numeric_limits<std::uintptr_t>::max();
std::atomic_uint64_t kNextTransportSessionId{1U};

[[nodiscard]] std::uint64_t nextTransportSessionId() {
  auto const sessionId = kNextTransportSessionId.fetch_add(
      1U, std::memory_order_relaxed);
  if (sessionId == 0U) {
    throw ProcessTransportError(
        "The process transport session ID space is exhausted.");
  }
  return sessionId;
}

[[nodiscard]] std::shared_ptr<ProcessTransportSessionState>
requireSessionState(
    std::shared_ptr<ProcessTransportSessionState> sessionState) {
  if (!sessionState) {
    throw ProcessTransportError(
        "A process transport connection requires session reliability state.");
  }
  return sessionState;
}

[[noreturn]] void throwTransportError(char const* message) {
  throw ProcessTransportError(message);
}

[[nodiscard]] bool validSocket(std::uintptr_t socket) noexcept {
  return socket != kInvalidSocket;
}

void closeSocket(std::uintptr_t socket) noexcept {
  platform::closeSocket(socket);
}

[[nodiscard]] std::wstring wideAscii(std::string const& value) {
  return std::wstring(value.begin(), value.end());
}

[[nodiscard]] std::uintptr_t connectSocket(ProcessTransportAddress const& address) {
  try {
    return platform::connectSocket(address.host, address.port);
  } catch (std::exception const& error) {
    throw ProcessTransportError(error.what());
  }
}

[[nodiscard]] std::uintptr_t connectSocket(
    ProcessTransportAddress const& address,
    std::chrono::steady_clock::time_point deadline) {
  try {
    return platform::connectSocket(address.host, address.port, deadline);
  } catch (std::exception const& error) {
    throw ProcessTransportError(error.what());
  }
}

[[nodiscard]] std::uintptr_t listenSocket(ProcessTransportAddress& address) {
  try {
    return platform::listenSocket(address.host, address.port, address.port);
  } catch (std::exception const& error) {
    throw ProcessTransportError(error.what());
  }
}

[[nodiscard]] std::uintptr_t acceptSocket(std::uintptr_t listener) {
  try {
    return platform::acceptSocket(listener);
  } catch (std::exception const& error) {
    throw ProcessTransportError(error.what());
  }
}

[[nodiscard]] std::string socketPeerAddress(std::uintptr_t socket) {
  try {
    return platform::peerAddress(socket);
  } catch (std::exception const& error) {
    throw ProcessTransportError(error.what());
  }
}

void sendAll(
    std::uintptr_t socket,
    platform::TlsSocket* tlsSocket,
    platform::WebSocketSocket* websocketSocket,
    std::span<std::uint8_t const> bytes) {
  try {
    if (websocketSocket != nullptr) {
      websocketSocket->sendAll(bytes);
    } else if (tlsSocket != nullptr) {
      tlsSocket->sendAll(bytes);
    } else {
      platform::sendAll(socket, bytes);
    }
  } catch (std::exception const& error) {
    throw ProcessTransportError(error.what());
  }
}

enum class ReceiveResult { complete, end_of_stream };

[[nodiscard]] ReceiveResult receiveExact(
    std::uintptr_t socket,
    platform::TlsSocket* tlsSocket,
    platform::WebSocketSocket* websocketSocket,
    std::span<std::uint8_t> bytes,
    std::optional<std::chrono::steady_clock::time_point> deadline = std::nullopt) {
  try {
    auto const result = websocketSocket != nullptr
        ? websocketSocket->receiveExact(bytes, deadline)
        : tlsSocket != nullptr
        ? (deadline ? tlsSocket->receiveExact(bytes, deadline)
                    : tlsSocket->receiveExact(bytes))
        : (deadline ? platform::receiveExact(socket, bytes, *deadline)
                    : platform::receiveExact(socket, bytes));
    return result == platform::ReceiveExactResult::complete
        ? ReceiveResult::complete
        : ReceiveResult::end_of_stream;
  } catch (std::exception const& error) {
    throw ProcessTransportError(error.what());
  }
}

void sendFrameOnSocket(
    std::uintptr_t socket,
    TransportFrame const& frame,
    platform::TlsSocket* tlsSocket = nullptr,
    platform::WebSocketSocket* websocketSocket = nullptr) {
  auto const encoded = encodeTransportFrame(frame);
  sendAll(socket, tlsSocket, websocketSocket, encoded);
}

[[nodiscard]] ReceiveResult receiveFrameOnSocket(
    std::uintptr_t socket,
    TransportFrame& frame,
    std::optional<std::chrono::steady_clock::time_point> deadline = std::nullopt,
    platform::TlsSocket* tlsSocket = nullptr,
    platform::WebSocketSocket* websocketSocket = nullptr) {
  auto receive = [socket, deadline, tlsSocket, websocketSocket](
                     std::span<std::uint8_t> bytes) {
    return receiveExact(socket, tlsSocket, websocketSocket, bytes, deadline);
  };
  std::array<std::uint8_t, kTransportFrameHeaderSize> header{};
  if (receive(header) == ReceiveResult::end_of_stream) {
    return ReceiveResult::end_of_stream;
  }
  auto const packetSize = transport_wire::readBigEndian32(header, 0U);
  if (packetSize < kTransportFrameHeaderSize) {
    throwTransportError("The process transport peer announced a short packet.");
  }
  auto const payloadSize = packetSize - kTransportFrameHeaderSize;
  if (payloadSize > kTransportMaximumPayloadBytes) {
    throwTransportError("The process transport peer announced an oversized frame.");
  }
  std::vector<std::uint8_t> encoded(
      header.begin(),
      header.end());
  encoded.resize(packetSize);
  if (payloadSize != 0U &&
      receive(
          std::span<std::uint8_t>(
              encoded.data() + kTransportFrameHeaderSize,
              payloadSize)) == ReceiveResult::end_of_stream) {
    return ReceiveResult::end_of_stream;
  }
  frame = decodeTransportFrame(encoded);
  return ReceiveResult::complete;
}

}  // namespace

ProcessTransportConnection::ProcessTransportConnection(
    std::uintptr_t nativeSocket,
    void* owner,
    TransportEndpointIdentity localIdentity,
    TransportEndpointIdentity peerIdentity,
    FailureHandler failureHandler,
    ForcedResignationHandler forcedResignationHandler,
    std::shared_ptr<RuntimeInstrumentation> instrumentation,
    std::shared_ptr<ProcessTransportSessionState> sessionState,
    std::optional<TransportFrame> initialFrame,
    std::shared_ptr<platform::TlsSocket> tlsSocket,
    std::shared_ptr<platform::WebSocketSocket> websocketSocket)
    : nativeSocket_(nativeSocket),
      owner_(owner),
      localIdentity_(std::move(localIdentity)),
      peerIdentity_(std::move(peerIdentity)),
      failureHandler_(std::move(failureHandler)),
      forcedResignationHandler_(std::move(forcedResignationHandler)),
      instrumentation_(std::move(instrumentation)),
      sessionState_(requireSessionState(std::move(sessionState))),
      tlsSocket_(std::move(tlsSocket)),
      websocketSocket_(std::move(websocketSocket)),
      initialFrame_(std::move(initialFrame)),
      open_(validSocket(nativeSocket)) {}

ProcessTransportConnection::ProcessTransportConnection(
    TestSendFrameHandler sendFrameHandler,
    std::shared_ptr<ProcessTransportSessionState> sessionState)
    : localIdentity_{
          "test-process-transport",
          sessionState ? sessionState->sessionId() : 0U},
      testSendFrameHandler_(std::move(sendFrameHandler)),
      sessionState_(requireSessionState(std::move(sessionState))),
      open_(static_cast<bool>(testSendFrameHandler_)) {}

ProcessTransportConnection::~ProcessTransportConnection() {
  close();
}

std::shared_ptr<ProcessTransportConnection>
ProcessTransportConnection::connectClient(
    void* owner,
    ProcessTransportAddress address,
    TransportEndpointIdentity localIdentity,
    FailureHandler failureHandler,
    ForcedResignationHandler forcedResignationHandler,
    std::shared_ptr<RuntimeInstrumentation> instrumentation,
    std::shared_ptr<ProcessTransportSessionState> sessionState,
    ConnectedHandler connectedHandler,
    std::optional<std::chrono::steady_clock::time_point> deadline) {
  auto const connectedSocket = deadline
      ? connectSocket(address, *deadline)
      : connectSocket(address);
  std::shared_ptr<ProcessTransportConnection> connection;
  std::shared_ptr<platform::TlsSocket> tlsSocket;
  std::shared_ptr<platform::WebSocketSocket> websocketSocket;
  bool tlsOwnsSocket = false;

  try {
    if (address.security == ProcessTransportAddress::Security::tls13) {
      tlsOwnsSocket = true;
      try {
        tlsSocket = std::make_shared<platform::TlsSocket>(
            platform::TlsSocket::connect(
                connectedSocket,
                platform::TlsClientConfiguration{
                    address.tlsTrustStoreFile,
                    address.host,
                    address.tlsCertificateValidationMode},
                deadline));
      } catch (std::exception const& error) {
        throw ProcessTransportError(error.what());
      }
    }
    if (address.framing == ProcessTransportAddress::Framing::websocket) {
      auto websocketIo = platform::WebSocketIo{
          [connectedSocket, tlsSocket](
              std::span<std::uint8_t const> bytes) {
            if (tlsSocket) {
              tlsSocket->sendAll(bytes);
            } else {
              platform::sendAll(connectedSocket, bytes);
            }
          },
          [connectedSocket, tlsSocket](
              std::span<std::uint8_t> bytes,
              std::optional<std::chrono::steady_clock::time_point> receiveDeadline) {
            if (tlsSocket) {
              return tlsSocket->receiveExact(bytes, receiveDeadline);
            }
            return receiveDeadline
                ? platform::receiveExact(connectedSocket, bytes, *receiveDeadline)
                : platform::receiveExact(connectedSocket, bytes);
          }};
      auto hostHeader = address.websocketHostHeader;
      if (hostHeader.empty()) {
        hostHeader = address.host + ":" + std::to_string(address.port);
      }
      try {
        websocketSocket = platform::WebSocketSocket::connect(
            std::move(websocketIo),
            platform::WebSocketClientConfiguration{
                std::move(hostHeader),
                address.websocketResource,
                static_cast<std::size_t>(
                    kTransportMaximumPayloadBytes + kTransportFrameHeaderSize)},
            deadline);
      } catch (std::exception const& error) {
        throw ProcessTransportError(error.what());
      }
    }
    if (!sessionState) {
      sessionState = std::make_shared<ProcessTransportSessionState>();
    }
    localIdentity.sessionId = sessionState->sessionId();
    connection = std::shared_ptr<ProcessTransportConnection>(
        new ProcessTransportConnection(
            connectedSocket,
            owner,
            localIdentity,
            {},
            std::move(failureHandler),
            std::move(forcedResignationHandler),
            std::move(instrumentation),
            sessionState,
            std::nullopt,
            tlsSocket,
            websocketSocket));
    if (connectedHandler) {
      connectedHandler(connection);
    }
    if (!connection->open()) {
      throwTransportError("The process transport connection was closed during startup.");
    }

    if (sessionState->sessionId() == 0U) {
      sendFrameOnSocket(
          connectedSocket,
          makeTransportNewSessionRequest(),
          tlsSocket.get(),
          websocketSocket.get());
      TransportFrame statusFrame;
      if (receiveFrameOnSocket(
              connectedSocket,
              statusFrame,
              deadline,
              tlsSocket.get(),
              websocketSocket.get()) ==
              ReceiveResult::end_of_stream ||
          statusFrame.kind != TransportFrameKind::new_session_status) {
        throwTransportError(
            "The process transport peer did not return New Session Status.");
      }
      auto const status = decodeTransportNewSessionStatus(statusFrame);
      if (!sessionState->sequenceState().acceptIncomingMessageNumber(
              statusFrame.sequenceNumber)) {
        throwTransportError(
            "The process transport peer returned a stale New Session Status sequence number.");
      }
      if (status.status != TransportNewSessionStatusCode::success) {
        throw ProcessTransportError(
            "The process transport peer rejected New Session with status " +
            std::to_string(static_cast<std::uint32_t>(status.status)) + ".");
      }
      sessionState->assignSessionId(status.sessionId);
    }

    auto peerIdentity = TransportEndpointIdentity{
        socketPeerAddress(connectedSocket),
        sessionState->sessionId()};
    sessionState->bindPeerEndpointId(peerIdentity.endpointId);
    localIdentity.sessionId = sessionState->sessionId();
    connection->localIdentity_ = std::move(localIdentity);
    connection->peerIdentity_ = std::move(peerIdentity);
    return connection;
  } catch (...) {
    if (connection) {
      connection->close();
    } else if (websocketSocket) {
      websocketSocket->close();
      if (tlsSocket) {
        tlsSocket->close();
      } else {
        closeSocket(connectedSocket);
      }
    } else if (tlsSocket) {
      tlsSocket->close();
    } else if (!tlsOwnsSocket) {
      closeSocket(connectedSocket);
    }
    throw;
  }
}

std::shared_ptr<ProcessTransportConnection>
ProcessTransportConnection::createForTesting(
    TestSendFrameHandler sendFrameHandler,
    std::uint64_t sessionId) {
  if (!sendFrameHandler) {
    throw ProcessTransportError(
        "A socketless process transport test connection requires a send callback.");
  }
  if (sessionId == 0U) {
    throw ProcessTransportError(
        "A socketless process transport test connection requires a session ID.");
  }
  return std::shared_ptr<ProcessTransportConnection>(
      new ProcessTransportConnection(
          std::move(sendFrameHandler),
          std::make_shared<ProcessTransportSessionState>(sessionId)));
}

bool ProcessTransportConnection::sendFrame(TransportFrame const& frame) {
  std::scoped_lock sendLock(sendMutex_);
  return sendFrameUnderSendLock(frame);
}

bool ProcessTransportConnection::sendSequencedFrame(
    std::function<TransportFrame(std::uint32_t)> const& makeFrame) {
  if (!makeFrame) {
    throw std::invalid_argument(
        "A sequenced process transport send requires a frame factory.");
  }
  std::scoped_lock sendLock(sendMutex_);
  auto const sequenceNumber =
      sessionState_->sequenceState().nextOutgoingMessageNumber();
  auto const frame = makeFrame(sequenceNumber);
  return sendFrameUnderSendLock(frame);
}

bool ProcessTransportConnection::sendFrameUnderSendLock(
    TransportFrame const& frame) {
  auto instrumentationScope = instrumentation_
      ? instrumentation_->begin(InstrumentationLayer::transport, "send_frame")
      : RuntimeInstrumentation::Scope{};
  std::uintptr_t socket = kInvalidSocket;
  std::shared_ptr<platform::TlsSocket> tlsSocket;
  std::shared_ptr<platform::WebSocketSocket> websocketSocket;
  TestSendFrameHandler testSendFrameHandler;
  {
    std::scoped_lock stateLock(stateMutex_);
    if (!open_) {
      instrumentationScope.complete(InstrumentationOutcome::failure);
      return false;
    }
    socket = nativeSocket_;
    tlsSocket = tlsSocket_;
    websocketSocket = websocketSocket_;
    testSendFrameHandler = testSendFrameHandler_;
  }
  if (testSendFrameHandler) {
    auto const sent = testSendFrameHandler(frame);
    if (!sent) {
      instrumentationScope.complete(InstrumentationOutcome::failure);
    }
    return sent;
  }
  try {
    sendFrameOnSocket(
        socket, frame, tlsSocket.get(), websocketSocket.get());
    return true;
  } catch (ProcessTransportError const& error) {
    fail(wideAscii(error.what()));
    instrumentationScope.complete(InstrumentationOutcome::failure);
    return false;
  }
}

bool ProcessTransportConnection::receiveFrame(TransportFrame& frame) {
  auto instrumentationScope = instrumentation_
      ? instrumentation_->begin(InstrumentationLayer::transport, "receive_frame")
      : RuntimeInstrumentation::Scope{};
  std::uintptr_t socket = kInvalidSocket;
  std::shared_ptr<platform::TlsSocket> tlsSocket;
  std::shared_ptr<platform::WebSocketSocket> websocketSocket;
  bool hasInitialFrame = false;
  {
    std::scoped_lock stateLock(stateMutex_);
    if (!open_) {
      instrumentationScope.complete(InstrumentationOutcome::failure);
      return false;
    }
    if (testSendFrameHandler_) {
      instrumentationScope.complete(InstrumentationOutcome::failure);
      return false;
    }
    if (initialFrame_) {
      frame = std::move(*initialFrame_);
      initialFrame_.reset();
      hasInitialFrame = true;
    } else {
      socket = nativeSocket_;
      tlsSocket = tlsSocket_;
      websocketSocket = websocketSocket_;
    }
  }
  if (hasInitialFrame) {
    sessionState_->recordPeerActivity();
    return true;
  }
  try {
    if (receiveFrameOnSocket(
            socket,
            frame,
            std::nullopt,
            tlsSocket.get(),
            websocketSocket.get()) == ReceiveResult::end_of_stream) {
      fail(L"The process transport peer closed the connection.");
      instrumentationScope.complete(InstrumentationOutcome::failure);
      return false;
    }
    sessionState_->recordPeerActivity();
    return true;
  } catch (ProcessTransportError const& error) {
    fail(wideAscii(error.what()));
    instrumentationScope.complete(InstrumentationOutcome::failure);
    return false;
  }
}

void ProcessTransportConnection::close() noexcept {
  std::uintptr_t socket = kInvalidSocket;
  std::shared_ptr<platform::TlsSocket> tlsSocket;
  std::shared_ptr<platform::WebSocketSocket> websocketSocket;
  {
    std::scoped_lock stateLock(stateMutex_);
    if (!open_) {
      return;
    }
    open_ = false;
    socket = nativeSocket_;
    nativeSocket_ = kInvalidSocket;
    tlsSocket = std::move(tlsSocket_);
    websocketSocket = std::move(websocketSocket_);
    failureHandler_ = {};
    forcedResignationHandler_ = {};
    testSendFrameHandler_ = {};
  }
  if (websocketSocket) {
    websocketSocket->close();
  }
  if (tlsSocket) {
    tlsSocket->close();
  } else if (socket != kInvalidSocket) {
    closeSocket(socket);
  }
}

void ProcessTransportConnection::fail(std::wstring faultDescription) {
  auto instrumentationScope = instrumentation_
      ? instrumentation_->begin(InstrumentationLayer::transport, "fault")
      : RuntimeInstrumentation::Scope{};
  FailureHandler failureHandler;
  std::uintptr_t socket = kInvalidSocket;
  std::shared_ptr<platform::TlsSocket> tlsSocket;
  std::shared_ptr<platform::WebSocketSocket> websocketSocket;
  {
    std::scoped_lock stateLock(stateMutex_);
    if (!open_) {
      instrumentationScope.complete(InstrumentationOutcome::failure);
      return;
    }
    open_ = false;
    socket = nativeSocket_;
    nativeSocket_ = kInvalidSocket;
    tlsSocket = std::move(tlsSocket_);
    websocketSocket = std::move(websocketSocket_);
    failureHandler = std::move(failureHandler_);
    failureHandler_ = {};
    forcedResignationHandler_ = {};
    testSendFrameHandler_ = {};
  }
  if (websocketSocket) {
    websocketSocket->close();
  }
  if (tlsSocket) {
    tlsSocket->close();
  } else if (socket != kInvalidSocket) {
    closeSocket(socket);
  }
  if (failureHandler) {
    failureHandler(std::move(faultDescription));
  } else {
    instrumentationScope.complete(InstrumentationOutcome::failure);
  }
}

bool ProcessTransportConnection::forceFederateResignation(
    std::wstring reasonForResign) {
  auto instrumentationScope = instrumentation_
      ? instrumentation_->begin(InstrumentationLayer::transport, "forced_resignation")
      : RuntimeInstrumentation::Scope{};
  ForcedResignationHandler forcedResignationHandler;
  {
    std::scoped_lock stateLock(stateMutex_);
    if (!open_) {
      instrumentationScope.complete(InstrumentationOutcome::failure);
      return false;
    }
    forcedResignationHandler = forcedResignationHandler_;
  }
  if (!forcedResignationHandler) {
    instrumentationScope.complete(InstrumentationOutcome::failure);
    return false;
  }
  auto const result = forcedResignationHandler(std::move(reasonForResign));
  if (!result) {
    instrumentationScope.complete(InstrumentationOutcome::failure);
  }
  return result;
}

bool ProcessTransportConnection::open() const noexcept {
  std::scoped_lock stateLock(stateMutex_);
  return open_;
}

void* ProcessTransportConnection::owner() const noexcept {
  std::scoped_lock stateLock(stateMutex_);
  return owner_;
}

TransportEndpointIdentity const& ProcessTransportConnection::localIdentity() const noexcept {
  return localIdentity_;
}

TransportEndpointIdentity const& ProcessTransportConnection::peerIdentity() const noexcept {
  return peerIdentity_;
}

std::uint64_t ProcessTransportConnection::sessionId() const noexcept {
  return sessionState_->sessionId();
}

std::shared_ptr<ProcessTransportSessionState>
ProcessTransportConnection::sessionState() const noexcept {
  return sessionState_;
}

ProcessTransportMessageHistory& ProcessTransportConnection::messageHistory() noexcept {
  return sessionState_->messageHistory();
}

ProcessTransportSequenceState& ProcessTransportConnection::sequenceState() noexcept {
  return sessionState_->sequenceState();
}

ProcessTransportListener::ProcessTransportListener(
    std::uintptr_t nativeSocket,
    ProcessTransportAddress address,
    std::optional<platform::TlsServerConfiguration> tlsServerConfiguration)
    : nativeSocket_(nativeSocket),
      address_(std::move(address)),
      tlsServerConfiguration_(std::move(tlsServerConfiguration)) {}

ProcessTransportListener::~ProcessTransportListener() {
  close();
}

std::unique_ptr<ProcessTransportListener> ProcessTransportListener::listen(
    ProcessTransportAddress address,
    std::optional<platform::TlsServerConfiguration> tlsServerConfiguration) {
  if ((address.security == ProcessTransportAddress::Security::tls13) !=
      tlsServerConfiguration.has_value()) {
    throw std::invalid_argument(
        "TLS 1.3 listeners require server credentials, and TCP listeners do not accept TLS credentials.");
  }
  if (tlsServerConfiguration) {
    platform::TlsSocket::validateServerConfiguration(*tlsServerConfiguration);
  }
  auto const listeningSocket = listenSocket(address);
  return std::unique_ptr<ProcessTransportListener>(
      new ProcessTransportListener(
          listeningSocket,
          std::move(address),
          std::move(tlsServerConfiguration)));
}

std::shared_ptr<ProcessTransportConnection> ProcessTransportListener::accept(
    void* owner,
    TransportEndpointIdentity localIdentity,
    ProcessTransportConnection::FailureHandler failureHandler,
    ProcessTransportConnection::ForcedResignationHandler forcedResignationHandler,
    std::shared_ptr<RuntimeInstrumentation> instrumentation,
    std::shared_ptr<ProcessTransportSessionState> sessionState,
    SessionStateResolver resolveSessionState) {
  std::uintptr_t listeningSocket = kInvalidSocket;
  {
    std::scoped_lock lock(mutex_);
    listeningSocket = nativeSocket_;
  }
  if (!validSocket(listeningSocket)) {
    throw ProcessTransportError("The process transport listener is closed.");
  }
  auto const acceptedSocket = acceptSocket(listeningSocket);
  bool tlsOwnsSocket =
      address_.security == ProcessTransportAddress::Security::tls13;
  {
    std::scoped_lock lock(mutex_);
    if (nativeSocket_ != listeningSocket) {
      closeSocket(acceptedSocket);
      throw ProcessTransportError("The process transport listener is closed.");
    }
    try {
      pendingSockets_.insert(acceptedSocket);
    } catch (...) {
      closeSocket(acceptedSocket);
      throw;
    }
  }
  auto closePendingSocket = [this, acceptedSocket, &tlsOwnsSocket] {
    std::scoped_lock lock(mutex_);
    if (pendingSockets_.erase(acceptedSocket) != 0U && !tlsOwnsSocket) {
      closeSocket(acceptedSocket);
    }
  };
  std::shared_ptr<platform::TlsSocket> tlsSocket;
  std::shared_ptr<platform::WebSocketSocket> websocketSocket;
  auto const handshakeDeadline = std::chrono::steady_clock::now() +
      std::chrono::seconds(30);
  try {
    if (tlsOwnsSocket) {
      try {
        tlsSocket = std::make_shared<platform::TlsSocket>(
            platform::TlsSocket::accept(
                acceptedSocket,
                *tlsServerConfiguration_,
                handshakeDeadline));
      } catch (std::exception const&) {
        closePendingSocket();
        return {};
      }
    }
    if (address_.framing == ProcessTransportAddress::Framing::websocket) {
      auto websocketIo = platform::WebSocketIo{
          [acceptedSocket, tlsSocket](
              std::span<std::uint8_t const> bytes) {
            if (tlsSocket) {
              tlsSocket->sendAll(bytes);
            } else {
              platform::sendAll(acceptedSocket, bytes);
            }
          },
          [acceptedSocket, tlsSocket](
              std::span<std::uint8_t> bytes,
              std::optional<std::chrono::steady_clock::time_point> receiveDeadline) {
            if (tlsSocket) {
              return tlsSocket->receiveExact(bytes, receiveDeadline);
            }
            return receiveDeadline
                ? platform::receiveExact(acceptedSocket, bytes, *receiveDeadline)
                : platform::receiveExact(acceptedSocket, bytes);
          }};
      try {
        websocketSocket = platform::WebSocketSocket::accept(
            std::move(websocketIo),
            platform::WebSocketServerConfiguration{
                address_.websocketResource,
                static_cast<std::size_t>(
                    kTransportMaximumPayloadBytes + kTransportFrameHeaderSize)},
            handshakeDeadline);
      } catch (std::exception const&) {
        closePendingSocket();
        return {};
      }
    }
    TransportFrame initialFrame;
    if (receiveFrameOnSocket(
            acceptedSocket,
            initialFrame,
            std::nullopt,
            tlsSocket.get(),
            websocketSocket.get()) ==
        ReceiveResult::end_of_stream) {
      closePendingSocket();
      return {};
    }

    std::optional<TransportFrame> cachedInitialFrame;
    auto peerIdentity = TransportEndpointIdentity{
        socketPeerAddress(acceptedSocket), 0U};
    if (initialFrame.kind == TransportFrameKind::new_session) {
      std::uint32_t protocolVersion = 0U;
      try {
        protocolVersion = decodeTransportNewSessionRequest(initialFrame);
      } catch (TransportProtocolError const&) {
        closePendingSocket();
        return {};
      }
      if (protocolVersion != kTransportHla4ProtocolVersion) {
        sendFrameOnSocket(
            acceptedSocket,
            makeTransportNewSessionStatus({
                0U,
                TransportNewSessionStatusCode::unsupported_protocol_version},
                0U),
            tlsSocket.get(),
            websocketSocket.get());
        closePendingSocket();
        return {};
      }
      try {
        if (!sessionState) {
          sessionState = std::make_shared<ProcessTransportSessionState>();
        }
        auto const sessionId = nextTransportSessionId();
        sessionState->assignSessionId(sessionId);
        peerIdentity.sessionId = sessionId;
        localIdentity.sessionId = sessionId;
        sessionState->bindPeerEndpointId(peerIdentity.endpointId);
        auto const sequenceNumber =
            sessionState->sequenceState().nextOutgoingMessageNumber();
        sendFrameOnSocket(
            acceptedSocket,
            makeTransportNewSessionStatus({
                sessionId,
                TransportNewSessionStatusCode::success},
                sequenceNumber),
            tlsSocket.get(),
            websocketSocket.get());
      } catch (std::bad_alloc const&) {
        sendFrameOnSocket(
            acceptedSocket,
            makeTransportNewSessionStatus({
                0U,
                TransportNewSessionStatusCode::out_of_resources},
                0U),
            tlsSocket.get(),
            websocketSocket.get());
        closePendingSocket();
        return {};
      } catch (std::exception const&) {
        sendFrameOnSocket(
            acceptedSocket,
            makeTransportNewSessionStatus({
                0U,
                TransportNewSessionStatusCode::other_error},
                0U),
            tlsSocket.get(),
            websocketSocket.get());
        closePendingSocket();
        return {};
      }
    } else if (initialFrame.kind == TransportFrameKind::resume_request) {
      try {
        static_cast<void>(decodeTransportResumeRequest(initialFrame));
      } catch (TransportProtocolError const&) {
        closePendingSocket();
        return {};
      }
      if (!sessionState && resolveSessionState) {
        sessionState = resolveSessionState(initialFrame.sessionId);
      }
      if (!sessionState ||
          sessionState->sessionId() != initialFrame.sessionId) {
        closePendingSocket();
        return {};
      }
      peerIdentity.sessionId = initialFrame.sessionId;
      localIdentity.sessionId = initialFrame.sessionId;
      sessionState->bindPeerEndpointId(peerIdentity.endpointId);
      cachedInitialFrame = std::move(initialFrame);
    } else {
      closePendingSocket();
      return {};
    }

    std::scoped_lock lock(mutex_);
    if (!pendingSockets_.contains(acceptedSocket)) {
      throw ProcessTransportError(
          "The process transport listener closed during session startup.");
    }
    auto connection = std::shared_ptr<ProcessTransportConnection>(
        new ProcessTransportConnection(
            acceptedSocket,
            owner,
            std::move(localIdentity),
            std::move(peerIdentity),
            std::move(failureHandler),
            std::move(forcedResignationHandler),
            std::move(instrumentation),
            std::move(sessionState),
            std::move(cachedInitialFrame),
            tlsSocket,
            websocketSocket));
    pendingSockets_.erase(acceptedSocket);
    return connection;
  } catch (...) {
    std::scoped_lock lock(mutex_);
    if (pendingSockets_.erase(acceptedSocket) != 0U && !tlsOwnsSocket) {
      closeSocket(acceptedSocket);
    }
    throw;
  }
}

ProcessTransportAddress ProcessTransportListener::address() const noexcept {
  return address_;
}

void ProcessTransportListener::close() noexcept {
  std::scoped_lock lock(mutex_);
  auto const listeningSocket = nativeSocket_;
  nativeSocket_ = kInvalidSocket;
  closeSocket(listeningSocket);
  for (auto const pendingSocket : pendingSockets_) {
    if (address_.security == ProcessTransportAddress::Security::tls13) {
      platform::interruptSocket(pendingSocket);
    } else {
      closeSocket(pendingSocket);
    }
  }
  pendingSockets_.clear();
}

}  // namespace umbra::detail
