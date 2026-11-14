#pragma once

#include "internal/platform/socket_operations.hpp"

#include <atomic>
#include <chrono>
#include <cstddef>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <span>
#include <string>
#include <vector>

namespace umbra::detail::platform {

struct WebSocketIo final {
  std::function<void(std::span<std::uint8_t const>)> sendBytes;
  std::function<ReceiveExactResult(
      std::span<std::uint8_t>,
      std::optional<std::chrono::steady_clock::time_point>)>
      receiveBytes;
};

struct WebSocketClientConfiguration final {
  std::string hostHeader;
  std::string resource = "/";
  std::size_t maximumMessageBytes = 0U;
};

struct WebSocketServerConfiguration final {
  std::string resource = "/";
  std::size_t maximumMessageBytes = 0U;
};

// RFC 6455 message framing over an existing byte stream. The process transport
// owns the underlying TCP/TLS socket; this layer owns only WebSocket state.
class WebSocketSocket final {
 public:
  WebSocketSocket(WebSocketSocket const&) = delete;
  WebSocketSocket& operator=(WebSocketSocket const&) = delete;
  ~WebSocketSocket();

  [[nodiscard]] static std::shared_ptr<WebSocketSocket> connect(
      WebSocketIo io,
      WebSocketClientConfiguration const& configuration,
      std::optional<std::chrono::steady_clock::time_point> deadline =
          std::nullopt);
  [[nodiscard]] static std::shared_ptr<WebSocketSocket> accept(
      WebSocketIo io,
      WebSocketServerConfiguration const& configuration,
      std::optional<std::chrono::steady_clock::time_point> deadline =
          std::nullopt);

  void sendAll(std::span<std::uint8_t const> bytes);
  [[nodiscard]] ReceiveExactResult receiveExact(
      std::span<std::uint8_t> bytes,
      std::optional<std::chrono::steady_clock::time_point> deadline =
          std::nullopt);
  void close() noexcept;

 private:
  struct Frame final {
    bool final = false;
    std::uint8_t opcode = 0U;
    std::vector<std::uint8_t> payload;
  };

  WebSocketSocket(
      WebSocketIo io,
      bool clientRole,
      std::size_t maximumMessageBytes);
  void performClientHandshake(
      WebSocketClientConfiguration const& configuration,
      std::optional<std::chrono::steady_clock::time_point> deadline);
  void performServerHandshake(
      WebSocketServerConfiguration const& configuration,
      std::optional<std::chrono::steady_clock::time_point> deadline);
  [[nodiscard]] std::optional<Frame> readFrame(
      std::optional<std::chrono::steady_clock::time_point> deadline);
  [[nodiscard]] std::optional<std::vector<std::uint8_t>> readMessage(
      std::optional<std::chrono::steady_clock::time_point> deadline);
  void writeFrame(std::uint8_t opcode, std::span<std::uint8_t const> payload);
  void writeFrameUnderLock(
      std::uint8_t opcode,
      std::span<std::uint8_t const> payload);
  void sendClose(std::uint16_t statusCode) noexcept;
  [[noreturn]] void protocolError(char const* message, std::uint16_t statusCode);

  WebSocketIo io_;
  bool clientRole_ = false;
  std::size_t maximumMessageBytes_ = 0U;
  std::mutex sendMutex_;
  std::mutex receiveMutex_;
  std::vector<std::uint8_t> receiveBuffer_;
  std::size_t receiveOffset_ = 0U;
  std::vector<std::uint8_t> fragmentedMessage_;
  bool fragmentedMessageActive_ = false;
  bool handshakeComplete_ = false;
  bool closeFrameSent_ = false;
  std::atomic_bool closed_{false};
};

}  // namespace umbra::detail::platform
