#include "internal/platform/socket_operations.hpp"

#include <algorithm>
#include <array>
#include <cerrno>
#include <chrono>
#include <limits>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>
#include <system_error>

#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#endif

namespace umbra::detail::platform {
namespace {

[[nodiscard]] int lastSocketError() noexcept {
#if defined(_WIN32)
  return WSAGetLastError();
#else
  return errno;
#endif
}

[[nodiscard]] std::string socketErrorMessage(
    char const* operation,
    int error) {
  std::string message(operation);
  message += " failed with socket error ";
  message += std::to_string(error);
  return message;
}

[[nodiscard]] NativeSocket makeSocket(
    addrinfo const& candidate,
    char const* operation) {
  auto const socket = ::socket(
      candidate.ai_family,
      candidate.ai_socktype,
      candidate.ai_protocol);
#if defined(_WIN32)
  if (socket == INVALID_SOCKET) {
#else
  if (socket < 0) {
#endif
    throw std::runtime_error(
        socketErrorMessage(operation, lastSocketError()));
  }
  return static_cast<NativeSocket>(socket);
}

class AddressList final {
 public:
  explicit AddressList(addrinfo* value) : value_(value) {}
  AddressList(AddressList const&) = delete;
  AddressList& operator=(AddressList const&) = delete;
  ~AddressList() {
    if (value_ != nullptr) {
      ::freeaddrinfo(value_);
    }
  }
  [[nodiscard]] addrinfo* get() const noexcept { return value_; }

 private:
  addrinfo* value_;
};

[[nodiscard]] AddressList resolveAddress(
    std::string_view host,
    std::uint16_t port,
    bool passive) {
  addrinfo hints{};
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_protocol = IPPROTO_TCP;
  if (passive) {
    hints.ai_flags = AI_PASSIVE;
  }
  auto const portText = std::to_string(port);
  auto const hostText = std::string{host};
  auto const hostPointer = host.empty() ? nullptr : hostText.c_str();
  addrinfo* results = nullptr;
  auto const error = ::getaddrinfo(
      hostPointer,
      portText.c_str(),
      &hints,
      &results);
  if (error != 0) {
#if defined(_WIN32)
    auto const* message = ::gai_strerrorA(error);
#else
    auto const* message = ::gai_strerror(error);
#endif
    throw std::runtime_error(
        std::string("getaddrinfo failed: ") +
        (message == nullptr ? "unknown" : message));
  }
  return AddressList(results);
}

void setReuseAddress(NativeSocket socket) {
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
    throw std::runtime_error(socketErrorMessage("setsockopt", error));
  }
}

[[nodiscard]] std::uint16_t queryBoundPort(NativeSocket socket) {
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
    throw std::runtime_error(socketErrorMessage("getsockname", error));
  }
  if (storage.ss_family == AF_INET) {
    return ntohs(reinterpret_cast<sockaddr_in const*>(&storage)->sin_port);
  }
  if (storage.ss_family == AF_INET6) {
    return ntohs(reinterpret_cast<sockaddr_in6 const*>(&storage)->sin6_port);
  }
  throw std::runtime_error(
      "The process transport listener returned an unsupported address family.");
}

[[nodiscard]] bool interruptedSocketCall(int error) noexcept {
#if defined(_WIN32)
  return error == WSAEINTR;
#else
  return error == EINTR;
#endif
}

[[nodiscard]] bool waitForSocketUntil(
    NativeSocket socket,
    bool waitForWrite,
    std::chrono::steady_clock::time_point deadline) {
  while (true) {
    auto const now = std::chrono::steady_clock::now();
    if (now >= deadline) {
      return false;
    }
    auto remaining = std::chrono::duration_cast<std::chrono::microseconds>(
        deadline - now);
    if (remaining <= std::chrono::microseconds::zero()) {
      remaining = std::chrono::microseconds(1);
    }
    timeval timeout{};
    timeout.tv_sec = static_cast<decltype(timeout.tv_sec)>(
        remaining.count() / 1000000);
    timeout.tv_usec = static_cast<decltype(timeout.tv_usec)>(
        remaining.count() % 1000000);

    fd_set readySockets;
    FD_ZERO(&readySockets);
#if defined(_WIN32)
    FD_SET(static_cast<SOCKET>(socket), &readySockets);
    auto const result = ::select(
        0,
        waitForWrite ? nullptr : &readySockets,
        waitForWrite ? &readySockets : nullptr,
        nullptr,
        &timeout);
#else
    auto const nativeSocket = static_cast<int>(socket);
    FD_SET(nativeSocket, &readySockets);
    auto const result = ::select(
        nativeSocket + 1,
        waitForWrite ? nullptr : &readySockets,
        waitForWrite ? &readySockets : nullptr,
        nullptr,
        &timeout);
#endif
    if (result > 0) {
      return true;
    }
    if (result == 0) {
      return false;
    }
    auto const error = lastSocketError();
    if (!interruptedSocketCall(error)) {
      throw std::runtime_error(socketErrorMessage("select", error));
    }
  }
}

void setSocketNonBlockingInternal(NativeSocket socket, bool nonBlocking) {
#if defined(_WIN32)
  u_long mode = nonBlocking ? 1UL : 0UL;
  if (::ioctlsocket(
          static_cast<SOCKET>(socket),
          FIONBIO,
          &mode) != 0) {
#else
  auto const descriptor = static_cast<int>(socket);
  auto const currentFlags = ::fcntl(descriptor, F_GETFL, 0);
  if (currentFlags < 0 ||
      ::fcntl(
          descriptor,
          F_SETFL,
          nonBlocking ? currentFlags | O_NONBLOCK
                      : currentFlags & ~O_NONBLOCK) < 0) {
#endif
    auto const error = lastSocketError();
    throw std::runtime_error(socketErrorMessage("set socket mode", error));
  }
}

[[nodiscard]] bool connectInProgress(int error) noexcept {
#if defined(_WIN32)
  return error == WSAEWOULDBLOCK || error == WSAEINPROGRESS ||
      error == WSAEALREADY || error == WSAEINTR;
#else
  return error == EINPROGRESS || error == EWOULDBLOCK || error == EAGAIN ||
      error == EINTR;
#endif
}

[[nodiscard]] int pendingConnectError(NativeSocket socket) {
  int error = 0;
#if defined(_WIN32)
  int length = sizeof(error);
  if (::getsockopt(
          static_cast<SOCKET>(socket),
          SOL_SOCKET,
          SO_ERROR,
          reinterpret_cast<char*>(&error),
          &length) != 0) {
#else
  socklen_t length = sizeof(error);
  if (::getsockopt(
          static_cast<int>(socket),
          SOL_SOCKET,
          SO_ERROR,
          &error,
          &length) != 0) {
#endif
    return lastSocketError();
  }
  return error;
}

[[nodiscard]] NativeSocket connectSocketUntil(
    std::string_view host,
    std::uint16_t port,
    std::chrono::steady_clock::time_point deadline) {
  auto const results = resolveAddress(host, port, false);
  std::string lastError;
  for (auto* candidate = results.get(); candidate != nullptr;
       candidate = candidate->ai_next) {
    if (std::chrono::steady_clock::now() >= deadline) {
      throw std::runtime_error(
          "The process transport connect exceeded the recovery deadline.");
    }
    auto const socket = makeSocket(*candidate, "socket");
    try {
      setSocketNonBlockingInternal(socket, true);
#if defined(_WIN32)
      auto const connectResult = ::connect(
          static_cast<SOCKET>(socket),
          candidate->ai_addr,
          static_cast<int>(candidate->ai_addrlen));
#else
      auto const connectResult = ::connect(
          static_cast<int>(socket),
          candidate->ai_addr,
          candidate->ai_addrlen);
#endif
      bool connected = connectResult == 0;
      if (!connected) {
        auto const connectError = lastSocketError();
        if (connectInProgress(connectError) &&
            waitForSocketUntil(socket, true, deadline)) {
          auto const completionError = pendingConnectError(socket);
          connected = completionError == 0;
          if (!connected) {
            lastError = socketErrorMessage("connect", completionError);
          }
        } else if (connectInProgress(connectError)) {
          throw std::runtime_error(
              "The process transport connect exceeded the recovery deadline.");
        } else {
          lastError = socketErrorMessage("connect", connectError);
        }
      }
      if (connected) {
        setSocketNonBlockingInternal(socket, false);
        return socket;
      }
    } catch (...) {
      closeSocket(socket);
      throw;
    }
    closeSocket(socket);
  }
  if (std::chrono::steady_clock::now() >= deadline) {
    throw std::runtime_error(
        "The process transport connect exceeded the recovery deadline.");
  }
  throw std::runtime_error(
      lastError.empty() ? "The process transport could not connect." : lastError);
}

}  // namespace

void initializeSocketRuntime() {
#if defined(_WIN32)
  static std::once_flag startup;
  static int startupResult = 0;
  std::call_once(startup, [] {
    WSADATA data{};
    startupResult = WSAStartup(MAKEWORD(2, 2), &data);
  });
  if (startupResult != 0) {
    throw std::runtime_error(
        "The process transport could not initialize Winsock.");
  }
#endif
}

void interruptSocket(NativeSocket socket) noexcept {
  if (socket == invalidSocket) {
    return;
  }
#if defined(_WIN32)
  ::shutdown(static_cast<SOCKET>(socket), SD_BOTH);
#else
  ::shutdown(static_cast<int>(socket), SHUT_RDWR);
#endif
}

void setSocketNonBlocking(NativeSocket socket, bool nonBlocking) {
  setSocketNonBlockingInternal(socket, nonBlocking);
}

void waitForSocketActivity(
    NativeSocket socket,
    bool waitForRead,
    bool waitForWrite) {
  if (socket == invalidSocket || (!waitForRead && !waitForWrite)) {
    throw std::invalid_argument(
        "Waiting for socket activity requires a socket and an event.");
  }
  while (true) {
    fd_set readySockets;
    FD_ZERO(&readySockets);
    FD_SET(
#if defined(_WIN32)
        static_cast<SOCKET>(socket),
#else
        static_cast<int>(socket),
#endif
        &readySockets);
#if defined(_WIN32)
    auto const result = ::select(
        0,
        waitForRead ? &readySockets : nullptr,
        waitForWrite ? &readySockets : nullptr,
        nullptr,
        nullptr);
#else
    auto const descriptor = static_cast<int>(socket);
    auto const result = ::select(
        descriptor + 1,
        waitForRead ? &readySockets : nullptr,
        waitForWrite ? &readySockets : nullptr,
        nullptr,
        nullptr);
#endif
    if (result > 0) {
      return;
    }
    if (result == 0) {
      continue;
    }
    auto const error = lastSocketError();
    if (!interruptedSocketCall(error)) {
      throw std::runtime_error(socketErrorMessage("select", error));
    }
  }
}

bool waitForSocketActivityUntil(
    NativeSocket socket,
    bool waitForRead,
    bool waitForWrite,
    std::chrono::steady_clock::time_point deadline) {
  if (socket == invalidSocket || waitForRead == waitForWrite) {
    throw std::invalid_argument(
        "Waiting for a socket deadline requires one socket event.");
  }
  return waitForSocketUntil(socket, waitForWrite, deadline);
}

void closeSocket(NativeSocket socket) noexcept {
  if (socket == invalidSocket) {
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

NativeSocket connectSocket(std::string_view host, std::uint16_t port) {
  initializeSocketRuntime();
  auto const results = resolveAddress(host, port, false);
  NativeSocket connected = invalidSocket;
  std::string lastError;
  for (auto* candidate = results.get(); candidate != nullptr;
       candidate = candidate->ai_next) {
    auto const socket = makeSocket(*candidate, "socket");
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
      connected = socket;
      break;
    }
    lastError = socketErrorMessage("connect", lastSocketError());
    closeSocket(socket);
  }
  if (connected == invalidSocket) {
    throw std::runtime_error(
        lastError.empty() ? "The process transport could not connect." : lastError);
  }
  return connected;
}

NativeSocket connectSocket(
    std::string_view host,
    std::uint16_t port,
    std::chrono::steady_clock::time_point deadline) {
  initializeSocketRuntime();
  if (std::chrono::steady_clock::now() >= deadline) {
    throw std::runtime_error(
        "The process transport connect exceeded the recovery deadline.");
  }
  return connectSocketUntil(host, port, deadline);
}

NativeSocket listenSocket(
    std::string_view host,
    std::uint16_t requestedPort,
    std::uint16_t& boundPort) {
  initializeSocketRuntime();
  auto const results = resolveAddress(host, requestedPort, true);
  NativeSocket listening = invalidSocket;
  std::string lastError;
  for (auto* candidate = results.get(); candidate != nullptr;
       candidate = candidate->ai_next) {
    auto const socket = makeSocket(*candidate, "socket");
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
        listening = socket;
        break;
      }
      lastError = socketErrorMessage("bind/listen", lastSocketError());
    } catch (...) {
      closeSocket(socket);
      throw;
    }
    closeSocket(socket);
  }
  if (listening == invalidSocket) {
    throw std::runtime_error(
        lastError.empty() ? "The process transport could not listen." : lastError);
  }
  try {
    boundPort = queryBoundPort(listening);
  } catch (...) {
    closeSocket(listening);
    throw;
  }
  return listening;
}

NativeSocket acceptSocket(NativeSocket listener) {
  initializeSocketRuntime();
#if defined(_WIN32)
  auto const accepted = ::accept(static_cast<SOCKET>(listener), nullptr, nullptr);
  if (accepted == INVALID_SOCKET) {
#else
  auto const accepted = ::accept(static_cast<int>(listener), nullptr, nullptr);
  if (accepted < 0) {
#endif
    throw std::runtime_error(socketErrorMessage("accept", lastSocketError()));
  }
  return static_cast<NativeSocket>(accepted);
}

std::string peerAddress(NativeSocket socket) {
  sockaddr_storage storage{};
#if defined(_WIN32)
  int length = sizeof(storage);
  auto const result = ::getpeername(
      static_cast<SOCKET>(socket),
      reinterpret_cast<sockaddr*>(&storage),
      &length);
#else
  socklen_t length = sizeof(storage);
  auto const result = ::getpeername(
      static_cast<int>(socket),
      reinterpret_cast<sockaddr*>(&storage),
      &length);
#endif
  if (result != 0) {
    throw std::runtime_error(
        socketErrorMessage("getpeername", lastSocketError()));
  }

  std::array<char, NI_MAXHOST> host{};
  auto const nameResult = ::getnameinfo(
      reinterpret_cast<sockaddr*>(&storage),
      static_cast<decltype(length)>(length),
      host.data(),
      static_cast<decltype(length)>(host.size()),
      nullptr,
      0,
      NI_NUMERICHOST);
  if (nameResult != 0) {
#if defined(_WIN32)
    auto const* message = ::gai_strerrorA(nameResult);
#else
    auto const* message = ::gai_strerror(nameResult);
#endif
    throw std::runtime_error(
        std::string("getnameinfo failed: ") +
        (message == nullptr ? "unknown" : message));
  }
  return std::string(host.data());
}

void sendAll(NativeSocket socket, std::span<std::uint8_t const> bytes) {
  std::size_t offset = 0U;
  while (offset < bytes.size()) {
#if defined(_WIN32)
    auto const requested = static_cast<int>(std::min<std::size_t>(
        bytes.size() - offset,
        static_cast<std::size_t>((std::numeric_limits<int>::max)())));
    auto const result = ::send(
        static_cast<SOCKET>(socket),
        reinterpret_cast<char const*>(bytes.data() + offset),
        requested,
        0);
#else
    auto const result = ::send(
        static_cast<int>(socket),
        bytes.data() + offset,
        bytes.size() - offset,
        MSG_NOSIGNAL);
#endif
    if (result <= 0) {
      throw std::runtime_error(socketErrorMessage("send", lastSocketError()));
    }
    offset += static_cast<std::size_t>(result);
  }
}

ReceiveExactResult receiveExact(
    NativeSocket socket,
    std::span<std::uint8_t> bytes) {
  std::size_t offset = 0U;
  while (offset < bytes.size()) {
#if defined(_WIN32)
    auto const requested = static_cast<int>(std::min<std::size_t>(
        bytes.size() - offset,
        static_cast<std::size_t>((std::numeric_limits<int>::max)())));
    auto const result = ::recv(
        static_cast<SOCKET>(socket),
        reinterpret_cast<char*>(bytes.data() + offset),
        requested,
        0);
#else
    auto const result = ::recv(
        static_cast<int>(socket),
        bytes.data() + offset,
        bytes.size() - offset,
        0);
#endif
    if (result == 0) {
      return ReceiveExactResult::end_of_stream;
    }
    if (result < 0) {
      throw std::runtime_error(socketErrorMessage("recv", lastSocketError()));
    }
    offset += static_cast<std::size_t>(result);
  }
  return ReceiveExactResult::complete;
}

ReceiveExactResult receiveExact(
    NativeSocket socket,
    std::span<std::uint8_t> bytes,
    std::chrono::steady_clock::time_point deadline) {
  std::size_t offset = 0U;
  while (offset < bytes.size()) {
    if (!waitForSocketUntil(socket, false, deadline)) {
      throw std::runtime_error(
          "The process transport handshake exceeded the recovery deadline.");
    }
#if defined(_WIN32)
    auto const requested = static_cast<int>(std::min<std::size_t>(
        bytes.size() - offset,
        static_cast<std::size_t>((std::numeric_limits<int>::max)())));
    auto const result = ::recv(
        static_cast<SOCKET>(socket),
        reinterpret_cast<char*>(bytes.data() + offset),
        requested,
        0);
#else
    auto const result = ::recv(
        static_cast<int>(socket),
        bytes.data() + offset,
        bytes.size() - offset,
        0);
#endif
    if (result == 0) {
      return ReceiveExactResult::end_of_stream;
    }
    if (result < 0) {
      auto const error = lastSocketError();
      if (interruptedSocketCall(error)) {
        continue;
      }
      throw std::runtime_error(socketErrorMessage("recv", error));
    }
    offset += static_cast<std::size_t>(result);
  }
  return ReceiveExactResult::complete;
}

}  // namespace umbra::detail::platform
