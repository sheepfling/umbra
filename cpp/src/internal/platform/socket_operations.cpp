#include "internal/platform/socket_operations.hpp"

#include <algorithm>
#include <cerrno>
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
#include <netdb.h>
#include <netinet/in.h>
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

}  // namespace umbra::detail::platform
