#include "internal/federation/process_transport.hpp"

#include <algorithm>
#include <array>
#include <charconv>
#include <cstring>
#include <limits>
#include <system_error>
#include <utility>

#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#include <cerrno>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#endif

namespace umbra::detail {
namespace {

constexpr std::uintptr_t kInvalidSocket = std::numeric_limits<std::uintptr_t>::max();

[[noreturn]] void throwTransportError(char const* message) {
  throw ProcessTransportError(message);
}

void ensureSocketRuntime() {
#if defined(_WIN32)
  static std::once_flag startup;
  static int startupResult = 0;
  std::call_once(startup, [] {
    WSADATA data{};
    startupResult = WSAStartup(MAKEWORD(2, 2), &data);
  });
  if (startupResult != 0) {
    throwTransportError("The process transport could not initialize Winsock.");
  }
#endif
}

[[nodiscard]] bool validSocket(std::uintptr_t socket) noexcept {
  return socket != kInvalidSocket;
}

void closeSocket(std::uintptr_t socket) noexcept {
  if (!validSocket(socket)) {
    return;
  }
#if defined(_WIN32)
  ::shutdown(static_cast<SOCKET>(socket), SD_BOTH);
  ::closesocket(static_cast<SOCKET>(socket));
#else
  ::shutdown(static_cast<int>(socket), SHUT_RDWR);
  ::close(static_cast<int>(socket));
#endif
}

[[nodiscard]] std::wstring wideAscii(std::string const& value) {
  return std::wstring(value.begin(), value.end());
}

[[nodiscard]] int lastSocketError() noexcept {
#if defined(_WIN32)
  return WSAGetLastError();
#else
  return errno;
#endif
}

[[nodiscard]] std::string socketErrorMessage(char const* operation, int error) {
  std::string message(operation);
  message += " failed with socket error ";
  message += std::to_string(error);
  return message;
}

[[nodiscard]] std::uintptr_t createSocket(
    addrinfo const& candidate,
    char const* operation) {
#if defined(_WIN32)
  auto const socket = ::socket(candidate.ai_family, candidate.ai_socktype, candidate.ai_protocol);
  if (socket == INVALID_SOCKET) {
    throw ProcessTransportError(socketErrorMessage(operation, lastSocketError()));
  }
  return static_cast<std::uintptr_t>(socket);
#else
  auto const socket = ::socket(candidate.ai_family, candidate.ai_socktype, candidate.ai_protocol);
  if (socket < 0) {
    throw ProcessTransportError(socketErrorMessage(operation, lastSocketError()));
  }
  return static_cast<std::uintptr_t>(socket);
#endif
}

void setReuseAddress(std::uintptr_t socket) {
  int enabled = 1;
#if defined(_WIN32)
  if (::setsockopt(
          static_cast<SOCKET>(socket),
          SOL_SOCKET,
          SO_REUSEADDR,
          reinterpret_cast<char const*>(&enabled),
          sizeof(enabled)) != 0) {
#else
  if (::setsockopt(
          static_cast<int>(socket),
          SOL_SOCKET,
          SO_REUSEADDR,
          &enabled,
          sizeof(enabled)) != 0) {
#endif
    auto const error = lastSocketError();
    closeSocket(socket);
    throw ProcessTransportError(socketErrorMessage("setsockopt", error));
  }
}

void sendAll(std::uintptr_t socket, std::span<std::uint8_t const> bytes) {
  std::size_t offset = 0U;
  while (offset < bytes.size()) {
#if defined(_WIN32)
    auto const result = ::send(
        static_cast<SOCKET>(socket),
        reinterpret_cast<char const*>(bytes.data() + offset),
        static_cast<int>(bytes.size() - offset),
        0);
#else
    auto const result = ::send(
        static_cast<int>(socket),
        bytes.data() + offset,
        bytes.size() - offset,
        MSG_NOSIGNAL);
#endif
    if (result <= 0) {
      throw ProcessTransportError(socketErrorMessage("send", lastSocketError()));
    }
    offset += static_cast<std::size_t>(result);
  }
}

enum class ReceiveResult { complete, end_of_stream };

[[nodiscard]] ReceiveResult receiveExact(
    std::uintptr_t socket,
    std::span<std::uint8_t> bytes) {
  std::size_t offset = 0U;
  while (offset < bytes.size()) {
#if defined(_WIN32)
    auto const result = ::recv(
        static_cast<SOCKET>(socket),
        reinterpret_cast<char*>(bytes.data() + offset),
        static_cast<int>(bytes.size() - offset),
        0);
#else
    auto const result = ::recv(
        static_cast<int>(socket),
        bytes.data() + offset,
        bytes.size() - offset,
        0);
#endif
    if (result == 0) {
      return ReceiveResult::end_of_stream;
    }
    if (result < 0) {
      throw ProcessTransportError(socketErrorMessage("recv", lastSocketError()));
    }
    offset += static_cast<std::size_t>(result);
  }
  return ReceiveResult::complete;
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
  auto const payloadSize =
      (static_cast<std::uint32_t>(header[8U]) << 24U) |
      (static_cast<std::uint32_t>(header[9U]) << 16U) |
      (static_cast<std::uint32_t>(header[10U]) << 8U) |
      static_cast<std::uint32_t>(header[11U]);
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

[[nodiscard]] addrinfo* resolveAddress(
    ProcessTransportAddress const& address,
    bool passive,
    addrinfo*& result) {
  addrinfo hints{};
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_protocol = IPPROTO_TCP;
  if (passive) {
    hints.ai_flags = AI_PASSIVE;
  }
  auto const host = address.host.empty() ? nullptr : address.host.c_str();
  auto const port = std::to_string(address.port);
  auto const error = ::getaddrinfo(host, port.c_str(), &hints, &result);
  if (error != 0) {
#if defined(_WIN32)
    auto const message = gai_strerrorA(error);
#else
    auto const message = gai_strerror(error);
#endif
    throw ProcessTransportError(
        std::string("getaddrinfo failed: ") + (message == nullptr ? "unknown" : message));
  }
  return result;
}

[[nodiscard]] std::uint16_t boundPort(std::uintptr_t socket) {
  sockaddr_storage storage{};
#if defined(_WIN32)
  int length = sizeof(storage);
  if (::getsockname(
          static_cast<SOCKET>(socket),
          reinterpret_cast<sockaddr*>(&storage),
          &length) != 0) {
#else
  socklen_t length = sizeof(storage);
  if (::getsockname(
          static_cast<int>(socket),
          reinterpret_cast<sockaddr*>(&storage),
          &length) != 0) {
#endif
    auto const error = lastSocketError();
    closeSocket(socket);
    throw ProcessTransportError(socketErrorMessage("getsockname", error));
  }
  if (storage.ss_family == AF_INET) {
    return ntohs(reinterpret_cast<sockaddr_in const*>(&storage)->sin_port);
  }
  if (storage.ss_family == AF_INET6) {
    return ntohs(reinterpret_cast<sockaddr_in6 const*>(&storage)->sin6_port);
  }
  closeSocket(socket);
  throwTransportError("The process transport listener returned an unsupported address family.");
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
  ensureSocketRuntime();
  addrinfo* results = nullptr;
  auto* resolvedResults = resolveAddress(address, false, results);
  std::uintptr_t connectedSocket = kInvalidSocket;
  std::string lastError;
  for (auto* candidate = resolvedResults; candidate != nullptr; candidate = candidate->ai_next) {
    auto const socket = createSocket(*candidate, "socket");
#if defined(_WIN32)
    auto const result = ::connect(
        static_cast<SOCKET>(socket),
        candidate->ai_addr,
        static_cast<int>(candidate->ai_addrlen));
#else
    auto const result = ::connect(
        static_cast<int>(socket),
        candidate->ai_addr,
        candidate->ai_addrlen);
#endif
    if (result == 0) {
      connectedSocket = socket;
      break;
    }
    lastError = socketErrorMessage("connect", lastSocketError());
    closeSocket(socket);
  }
  ::freeaddrinfo(results);
  if (!validSocket(connectedSocket)) {
    throw ProcessTransportError(lastError.empty() ? "The process transport could not connect." : lastError);
  }

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
  closeSocket(nativeSocket_);
}

std::unique_ptr<ProcessTransportListener> ProcessTransportListener::listen(
    ProcessTransportAddress address) {
  ensureSocketRuntime();
  addrinfo* results = nullptr;
  auto* resolvedResults = resolveAddress(address, true, results);
  std::uintptr_t listeningSocket = kInvalidSocket;
  std::string lastError;
  for (auto* candidate = resolvedResults; candidate != nullptr; candidate = candidate->ai_next) {
    auto const socket = createSocket(*candidate, "socket");
    try {
      setReuseAddress(socket);
#if defined(_WIN32)
      auto const bindResult = ::bind(
          static_cast<SOCKET>(socket),
          candidate->ai_addr,
          static_cast<int>(candidate->ai_addrlen));
      auto const listenResult = bindResult == 0
          ? ::listen(static_cast<SOCKET>(socket), 16)
          : SOCKET_ERROR;
#else
      auto const bindResult = ::bind(
          static_cast<int>(socket),
          candidate->ai_addr,
          candidate->ai_addrlen);
      auto const listenResult = bindResult == 0
          ? ::listen(static_cast<int>(socket), 16)
          : -1;
#endif
      if (bindResult == 0 && listenResult == 0) {
        listeningSocket = socket;
        break;
      }
      lastError = socketErrorMessage("bind/listen", lastSocketError());
    } catch (...) {
      closeSocket(socket);
      ::freeaddrinfo(results);
      throw;
    }
    closeSocket(socket);
  }
  ::freeaddrinfo(results);
  if (!validSocket(listeningSocket)) {
    throw ProcessTransportError(lastError.empty() ? "The process transport could not listen." : lastError);
  }
  address.port = boundPort(listeningSocket);
  return std::unique_ptr<ProcessTransportListener>(
      new ProcessTransportListener(listeningSocket, std::move(address)));
}

std::shared_ptr<ProcessTransportConnection> ProcessTransportListener::accept(
    void* owner,
    TransportEndpointIdentity localIdentity,
    ProcessTransportConnection::FailureHandler failureHandler,
    ProcessTransportConnection::ForcedResignationHandler forcedResignationHandler,
    std::shared_ptr<RuntimeInstrumentation> instrumentation) {
  if (!validSocket(nativeSocket_)) {
    throw ProcessTransportError("The process transport listener is closed.");
  }
#if defined(_WIN32)
  auto const accepted = ::accept(static_cast<SOCKET>(nativeSocket_), nullptr, nullptr);
  if (accepted == INVALID_SOCKET) {
#else
  auto const accepted = ::accept(static_cast<int>(nativeSocket_), nullptr, nullptr);
  if (accepted < 0) {
#endif
    throw ProcessTransportError(socketErrorMessage("accept", lastSocketError()));
  }
  auto const acceptedSocket = static_cast<std::uintptr_t>(accepted);
  try {
    TransportFrame hello;
    if (receiveFrameOnSocket(acceptedSocket, hello) == ReceiveResult::end_of_stream ||
        hello.kind != TransportFrameKind::hello) {
      throwTransportError("The process transport peer did not send a hello frame.");
    }
    auto peerIdentity = decodeTransportHandshake(hello);
    sendFrameOnSocket(acceptedSocket, makeTransportHelloAck(localIdentity));
    return std::shared_ptr<ProcessTransportConnection>(new ProcessTransportConnection(
        acceptedSocket,
        owner,
        std::move(localIdentity),
        std::move(peerIdentity),
        std::move(failureHandler),
        std::move(forcedResignationHandler),
        std::move(instrumentation)));
  } catch (...) {
    closeSocket(acceptedSocket);
    throw;
  }
}

ProcessTransportAddress ProcessTransportListener::address() const noexcept {
  return address_;
}

}  // namespace umbra::detail
