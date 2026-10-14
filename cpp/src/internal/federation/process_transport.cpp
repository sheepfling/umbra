#include "internal/federation/process_transport.hpp"
#include "internal/observability/runtime_instrumentation.hpp"

#include "internal/encoding/transport_wire.hpp"
#include "internal/platform/socket_operations.hpp"

#include <array>
#include <exception>
#include <limits>
#include <span>
#include <utility>
#include <vector>

namespace umbra::detail {
namespace {

constexpr std::uintptr_t kInvalidSocket = std::numeric_limits<std::uintptr_t>::max();

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

void sendAll(std::uintptr_t socket, std::span<std::uint8_t const> bytes) {
  try {
    platform::sendAll(socket, bytes);
  } catch (std::exception const& error) {
    throw ProcessTransportError(error.what());
  }
}

enum class ReceiveResult { complete, end_of_stream };

[[nodiscard]] ReceiveResult receiveExact(
    std::uintptr_t socket,
    std::span<std::uint8_t> bytes) {
  try {
    auto const result = platform::receiveExact(socket, bytes);
    return result == platform::ReceiveExactResult::complete
        ? ReceiveResult::complete
        : ReceiveResult::end_of_stream;
  } catch (std::exception const& error) {
    throw ProcessTransportError(error.what());
  }
}

void sendFrameOnSocket(std::uintptr_t socket, TransportFrame const& frame) {
  auto const encoded = encodeTransportFrame(frame);
  sendAll(socket, encoded);
}

[[nodiscard]] ReceiveResult receiveFrameOnSocket(
    std::uintptr_t socket,
    TransportFrame& frame) {
  std::array<std::uint8_t, kTransportFrameHeaderSize> header{};
  if (receiveExact(socket, header) == ReceiveResult::end_of_stream) {
    return ReceiveResult::end_of_stream;
  }
  auto const payloadSize = transport_wire::readBigEndian32(header, 8U);
  if (payloadSize > kTransportMaximumPayloadBytes) {
    throwTransportError("The process transport peer announced an oversized frame.");
  }
  std::vector<std::uint8_t> encoded(
      header.begin(),
      header.end());
  encoded.resize(kTransportFrameHeaderSize + payloadSize);
  if (payloadSize != 0U &&
      receiveExact(
          socket,
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
    std::shared_ptr<RuntimeInstrumentation> instrumentation)
    : nativeSocket_(nativeSocket),
      owner_(owner),
      localIdentity_(std::move(localIdentity)),
      peerIdentity_(std::move(peerIdentity)),
      failureHandler_(std::move(failureHandler)),
      forcedResignationHandler_(std::move(forcedResignationHandler)),
      instrumentation_(std::move(instrumentation)),
      open_(validSocket(nativeSocket)) {}

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
    std::shared_ptr<RuntimeInstrumentation> instrumentation) {
  auto const connectedSocket = connectSocket(address);

  try {
    sendFrameOnSocket(connectedSocket, makeTransportHello(localIdentity));
    TransportFrame acknowledgement;
    if (receiveFrameOnSocket(connectedSocket, acknowledgement) ==
        ReceiveResult::end_of_stream ||
        acknowledgement.kind != TransportFrameKind::hello_ack) {
      throwTransportError("The process transport peer did not acknowledge the hello frame.");
    }
    auto peerIdentity = decodeTransportHandshake(acknowledgement);
    return std::shared_ptr<ProcessTransportConnection>(new ProcessTransportConnection(
        connectedSocket,
        owner,
        std::move(localIdentity),
        std::move(peerIdentity),
        std::move(failureHandler),
        std::move(forcedResignationHandler),
        std::move(instrumentation)));
  } catch (...) {
    closeSocket(connectedSocket);
    throw;
  }
}

bool ProcessTransportConnection::sendFrame(TransportFrame const& frame) {
  auto instrumentationScope = instrumentation_
      ? instrumentation_->begin(InstrumentationLayer::transport, "send_frame")
      : RuntimeInstrumentation::Scope{};
  std::scoped_lock sendLock(sendMutex_);
  std::uintptr_t socket = kInvalidSocket;
  {
    std::scoped_lock stateLock(stateMutex_);
    if (!open_) {
      instrumentationScope.complete(InstrumentationOutcome::failure);
      return false;
    }
    socket = nativeSocket_;
  }
  try {
    sendFrameOnSocket(socket, frame);
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
  {
    std::scoped_lock stateLock(stateMutex_);
    if (!open_) {
      instrumentationScope.complete(InstrumentationOutcome::failure);
      return false;
    }
    socket = nativeSocket_;
  }
  try {
    if (receiveFrameOnSocket(socket, frame) == ReceiveResult::end_of_stream) {
      fail(L"The process transport peer closed the connection.");
      instrumentationScope.complete(InstrumentationOutcome::failure);
      return false;
    }
    return true;
  } catch (ProcessTransportError const& error) {
    fail(wideAscii(error.what()));
    instrumentationScope.complete(InstrumentationOutcome::failure);
    return false;
  }
}

void ProcessTransportConnection::close() noexcept {
  std::uintptr_t socket = kInvalidSocket;
  {
    std::scoped_lock stateLock(stateMutex_);
    if (!open_) {
      return;
    }
    open_ = false;
    socket = nativeSocket_;
    nativeSocket_ = kInvalidSocket;
    failureHandler_ = {};
    forcedResignationHandler_ = {};
  }
  closeSocket(socket);
}

void ProcessTransportConnection::fail(std::wstring faultDescription) {
  auto instrumentationScope = instrumentation_
      ? instrumentation_->begin(InstrumentationLayer::transport, "fault")
      : RuntimeInstrumentation::Scope{};
  FailureHandler failureHandler;
  std::uintptr_t socket = kInvalidSocket;
  {
    std::scoped_lock stateLock(stateMutex_);
    if (!open_) {
      instrumentationScope.complete(InstrumentationOutcome::failure);
      return;
    }
    open_ = false;
    socket = nativeSocket_;
    nativeSocket_ = kInvalidSocket;
    failureHandler = std::move(failureHandler_);
    failureHandler_ = {};
    forcedResignationHandler_ = {};
  }
  closeSocket(socket);
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

ProcessTransportListener::ProcessTransportListener(
    std::uintptr_t nativeSocket,
    ProcessTransportAddress address)
    : nativeSocket_(nativeSocket), address_(std::move(address)) {}

ProcessTransportListener::~ProcessTransportListener() {
  close();
}

std::unique_ptr<ProcessTransportListener> ProcessTransportListener::listen(
    ProcessTransportAddress address) {
  auto const listeningSocket = listenSocket(address);
  return std::unique_ptr<ProcessTransportListener>(
      new ProcessTransportListener(listeningSocket, std::move(address)));
}

std::shared_ptr<ProcessTransportConnection> ProcessTransportListener::accept(
    void* owner,
    TransportEndpointIdentity localIdentity,
    ProcessTransportConnection::FailureHandler failureHandler,
    ProcessTransportConnection::ForcedResignationHandler forcedResignationHandler,
    std::shared_ptr<RuntimeInstrumentation> instrumentation) {
  std::uintptr_t listeningSocket = kInvalidSocket;
  {
    std::scoped_lock lock(mutex_);
    listeningSocket = nativeSocket_;
  }
  if (!validSocket(listeningSocket)) {
    throw ProcessTransportError("The process transport listener is closed.");
  }
  auto const acceptedSocket = acceptSocket(listeningSocket);
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
  try {
    TransportFrame hello;
    if (receiveFrameOnSocket(acceptedSocket, hello) == ReceiveResult::end_of_stream ||
        hello.kind != TransportFrameKind::hello) {
      throwTransportError("The process transport peer did not send a hello frame.");
    }
    auto peerIdentity = decodeTransportHandshake(hello);
    sendFrameOnSocket(acceptedSocket, makeTransportHelloAck(localIdentity));
    std::scoped_lock lock(mutex_);
    if (!pendingSockets_.contains(acceptedSocket)) {
      throw ProcessTransportError(
          "The process transport listener closed during its hello handshake.");
    }
    auto connection = std::shared_ptr<ProcessTransportConnection>(
        new ProcessTransportConnection(
            acceptedSocket,
            owner,
            std::move(localIdentity),
            std::move(peerIdentity),
            std::move(failureHandler),
            std::move(forcedResignationHandler),
            std::move(instrumentation)));
    pendingSockets_.erase(acceptedSocket);
    return connection;
  } catch (...) {
    std::scoped_lock lock(mutex_);
    if (pendingSockets_.erase(acceptedSocket) != 0U) {
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
    closeSocket(pendingSocket);
  }
  pendingSockets_.clear();
}

}  // namespace umbra::detail
