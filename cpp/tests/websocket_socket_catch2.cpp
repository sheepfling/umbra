#include "internal/platform/websocket_socket.hpp"

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {

struct ScriptedStream final {
  std::vector<std::uint8_t> incoming;
  std::vector<std::uint8_t> outgoing;
  std::size_t offset = 0U;

  [[nodiscard]] umbra::detail::platform::WebSocketIo io() {
    return {
        [this](std::span<std::uint8_t const> bytes) {
          outgoing.insert(outgoing.end(), bytes.begin(), bytes.end());
        },
        [this](
            std::span<std::uint8_t> bytes,
            std::optional<std::chrono::steady_clock::time_point>) {
          if (bytes.size() > incoming.size() - offset) {
            return umbra::detail::platform::ReceiveExactResult::end_of_stream;
          }
          std::copy_n(
              incoming.data() + offset,
              bytes.size(),
              bytes.data());
          offset += bytes.size();
          return umbra::detail::platform::ReceiveExactResult::complete;
        }};
  }
};

[[nodiscard]] std::vector<std::uint8_t> bytes(std::string_view value) {
  return {value.begin(), value.end()};
}

void appendMaskedFrame(
    std::vector<std::uint8_t>& output,
    std::uint8_t opcode,
    std::string_view payload) {
  constexpr std::array<std::uint8_t, 4U> mask{0x12U, 0x34U, 0x56U, 0x78U};
  output.push_back(static_cast<std::uint8_t>(0x80U | opcode));
  REQUIRE(payload.size() < 126U);
  output.push_back(static_cast<std::uint8_t>(0x80U | payload.size()));
  output.insert(output.end(), mask.begin(), mask.end());
  for (std::size_t index = 0U; index < payload.size(); ++index) {
    output.push_back(static_cast<std::uint8_t>(
        static_cast<std::uint8_t>(payload[index]) ^ mask[index % mask.size()]));
  }
}

[[nodiscard]] std::vector<std::uint8_t> requestWithRfcExampleKey() {
  return bytes(
      "GET /chat HTTP/1.1\r\n"
      "Host: server.example.com\r\n"
      "Upgrade: websocket\r\n"
      "Connection: Upgrade\r\n"
      "Sec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r\n"
      "Sec-WebSocket-Version: 13\r\n"
      "\r\n");
}

}  // namespace

TEST_CASE(
    "WebSocket server computes the RFC 6455 example accept value",
    "[internal][transport][websocket][websocket-handshake-rfc-vector]") {
  auto stream = ScriptedStream{requestWithRfcExampleKey(), {}, 0U};
  auto socket = umbra::detail::platform::WebSocketSocket::accept(
      stream.io(), {"/chat", 1024U});
  auto const response = std::string(
      stream.outgoing.begin(), stream.outgoing.end());
  CHECK(response.starts_with("HTTP/1.1 101 Switching Protocols\r\n"));
  CHECK(response.find(
            "Sec-WebSocket-Accept: s3pPLMBiTxaQ9kYGzzhZRbK+xOo=\r\n") !=
        std::string::npos);
}

TEST_CASE(
    "WebSocket server reads only masked binary messages and answers ping",
    "[internal][transport][websocket][websocket-binary-message][websocket-control-frame]") {
  auto incoming = requestWithRfcExampleKey();
  appendMaskedFrame(incoming, 0x9U, "ok");
  appendMaskedFrame(incoming, 0x2U, "HLA");
  auto stream = ScriptedStream{std::move(incoming), {}, 0U};
  auto socket = umbra::detail::platform::WebSocketSocket::accept(
      stream.io(), {"/chat", 1024U});
  std::array<std::uint8_t, 3U> message{};
  REQUIRE(socket->receiveExact(message) ==
          umbra::detail::platform::ReceiveExactResult::complete);
  CHECK(std::string(message.begin(), message.end()) == "HLA");

  auto const responseBegin = std::string(
      stream.outgoing.begin(), stream.outgoing.end()).find("\r\n\r\n") + 4U;
  REQUIRE(responseBegin < stream.outgoing.size());
  REQUIRE(stream.outgoing.size() >= responseBegin + 4U);
  CHECK(stream.outgoing[responseBegin] == 0x8aU);
  CHECK(stream.outgoing[responseBegin + 1U] == 0x02U);
  CHECK(stream.outgoing[responseBegin + 2U] == static_cast<std::uint8_t>('o'));
  CHECK(stream.outgoing[responseBegin + 3U] == static_cast<std::uint8_t>('k'));
}

TEST_CASE(
    "WebSocket server rejects text data frames with close status 1003",
    "[internal][transport][websocket][websocket-rejects-text]") {
  auto incoming = requestWithRfcExampleKey();
  appendMaskedFrame(incoming, 0x1U, "not binary");
  auto stream = ScriptedStream{std::move(incoming), {}, 0U};
  auto socket = umbra::detail::platform::WebSocketSocket::accept(
      stream.io(), {"/chat", 1024U});
  std::array<std::uint8_t, 1U> message{};
  CHECK_THROWS_AS(socket->receiveExact(message), std::runtime_error);

  auto const responseBegin = std::string(
      stream.outgoing.begin(), stream.outgoing.end()).find("\r\n\r\n") + 4U;
  REQUIRE(responseBegin < stream.outgoing.size());
  REQUIRE(stream.outgoing.size() >= responseBegin + 4U);
  CHECK(stream.outgoing[responseBegin] == 0x88U);
  CHECK(stream.outgoing[responseBegin + 1U] == 0x02U);
  CHECK(stream.outgoing[responseBegin + 2U] == 0x03U);
  CHECK(stream.outgoing[responseBegin + 3U] == 0xebU);
}
