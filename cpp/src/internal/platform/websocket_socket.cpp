#include "internal/platform/websocket_socket.hpp"

#include <openssl/evp.h>
#include <openssl/rand.h>

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdint>
#include <limits>
#include <map>
#include <stdexcept>
#include <string_view>
#include <utility>

namespace umbra::detail::platform {
namespace {

constexpr std::string_view kWebSocketGuid =
    "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
constexpr std::size_t kDefaultMaximumMessageBytes = 16U * 1024U * 1024U;
constexpr std::size_t kMaximumHttpHeaderBytes = 16U * 1024U;

struct HttpMessage final {
  std::string startLine;
  std::map<std::string, std::vector<std::string>, std::less<>> headers;
};

[[noreturn]] void fail(char const* message) {
  throw std::runtime_error(message);
}

[[nodiscard]] std::string lowerAscii(std::string_view value) {
  std::string result;
  result.reserve(value.size());
  for (unsigned char character : value) {
    result.push_back(static_cast<char>(std::tolower(character)));
  }
  return result;
}

[[nodiscard]] std::string_view trimHorizontalWhitespace(
    std::string_view value) noexcept {
  while (!value.empty() && (value.front() == ' ' || value.front() == '\t')) {
    value.remove_prefix(1U);
  }
  while (!value.empty() && (value.back() == ' ' || value.back() == '\t')) {
    value.remove_suffix(1U);
  }
  return value;
}

[[nodiscard]] bool isHttpTokenCharacter(unsigned char character) noexcept {
  if ((character >= 'a' && character <= 'z') ||
      (character >= 'A' && character <= 'Z') ||
      (character >= '0' && character <= '9')) {
    return true;
  }
  switch (character) {
    case '!':
    case '#':
    case '$':
    case '%':
    case '&':
    case '\'':
    case '*':
    case '+':
    case '-':
    case '.':
    case '^':
    case '_':
    case '`':
    case '|':
    case '~':
      return true;
    default:
      return false;
  }
}

void validateHttpFieldValue(std::string_view value) {
  for (unsigned char character : value) {
    if (character == '\r' || character == '\n' || character == 0U ||
        character == 0x7fU || character < 0x20U) {
      fail("The WebSocket handshake contains an invalid HTTP field value.");
    }
  }
}

void validateResource(std::string_view resource) {
  if (resource.empty() || resource.front() != '/') {
    fail("A WebSocket resource must use origin-form and begin with '/'.");
  }
  auto const queryBegin = resource.find('?');
  for (std::size_t index = 0U; index < resource.size(); ++index) {
    auto const character = static_cast<unsigned char>(resource[index]);
    auto const inQuery = queryBegin != std::string_view::npos && index >= queryBegin;
    auto const isUnreserved =
        (character >= 'a' && character <= 'z') ||
        (character >= 'A' && character <= 'Z') ||
        (character >= '0' && character <= '9') || character == '-' ||
        character == '.' || character == '_' || character == '~';
    auto const isSubDelimiter =
        character == '!' || character == '$' || character == '&' ||
        character == '\'' || character == '(' || character == ')' ||
        character == '*' || character == '+' || character == ',' ||
        character == ';' || character == '=';
    auto const isPchar =
        isUnreserved || isSubDelimiter || character == ':' || character == '@';
    if (character == '%') {
      auto const isHex = [](unsigned char value) {
        return (value >= '0' && value <= '9') ||
            (value >= 'a' && value <= 'f') ||
            (value >= 'A' && value <= 'F');
      };
      if (index + 2U >= resource.size() ||
          !isHex(static_cast<unsigned char>(resource[index + 1U])) ||
          !isHex(static_cast<unsigned char>(resource[index + 2U]))) {
        fail("The WebSocket resource contains an invalid percent escape.");
      }
      index += 2U;
      continue;
    }
    if (!isPchar && character != '/' && !(inQuery && character == '?')) {
      fail("The WebSocket resource contains a character invalid in an HTTP request target.");
    }
  }
}

[[nodiscard]] std::string base64Encode(std::span<std::uint8_t const> bytes) {
  if (bytes.size() > static_cast<std::size_t>(std::numeric_limits<int>::max())) {
    fail("The WebSocket handshake value is too large to encode.");
  }
  std::string encoded(4U * ((bytes.size() + 2U) / 3U), '\0');
  auto const count = EVP_EncodeBlock(
      reinterpret_cast<unsigned char*>(encoded.data()),
      bytes.data(),
      static_cast<int>(bytes.size()));
  if (count < 0 || static_cast<std::size_t>(count) != encoded.size()) {
    fail("The WebSocket handshake value could not be Base64 encoded.");
  }
  return encoded;
}

[[nodiscard]] std::vector<std::uint8_t> base64Decode(std::string_view encoded) {
  if (encoded.empty() || encoded.size() % 4U != 0U ||
      encoded.size() > static_cast<std::size_t>(std::numeric_limits<int>::max())) {
    fail("The WebSocket handshake key is not valid Base64.");
  }
  std::vector<std::uint8_t> decoded((encoded.size() / 4U) * 3U);
  auto const count = EVP_DecodeBlock(
      decoded.data(),
      reinterpret_cast<unsigned char const*>(encoded.data()),
      static_cast<int>(encoded.size()));
  if (count < 0) {
    fail("The WebSocket handshake key is not valid Base64.");
  }
  std::size_t padding = 0U;
  if (encoded.ends_with("==")) {
    padding = 2U;
  } else if (encoded.ends_with('=')) {
    padding = 1U;
  }
  if (static_cast<std::size_t>(count) < padding) {
    fail("The WebSocket handshake key is not valid Base64.");
  }
  decoded.resize(static_cast<std::size_t>(count) - padding);
  if (base64Encode(decoded) != encoded) {
    fail("The WebSocket handshake key is not canonical Base64.");
  }
  return decoded;
}

[[nodiscard]] std::string expectedAccept(std::string_view key) {
  std::string input(key);
  input.append(kWebSocketGuid);
  std::array<std::uint8_t, EVP_MAX_MD_SIZE> digest{};
  unsigned int digestSize = 0U;
  if (EVP_Digest(
          input.data(),
          input.size(),
          digest.data(),
          &digestSize,
          EVP_sha1(),
          nullptr) != 1 ||
      digestSize != 20U) {
    fail("The WebSocket handshake accept value could not be computed.");
  }
  return base64Encode(std::span<std::uint8_t const>(digest.data(), digestSize));
}

[[nodiscard]] HttpMessage parseHttpMessage(std::string_view bytes) {
  if (!bytes.ends_with("\r\n\r\n")) {
    fail("The WebSocket handshake did not end with the HTTP header terminator.");
  }
  HttpMessage message;
  auto lineEnd = bytes.find("\r\n");
  if (lineEnd == std::string_view::npos || lineEnd == 0U) {
    fail("The WebSocket handshake start line is invalid.");
  }
  message.startLine.assign(bytes.substr(0U, lineEnd));
  auto lineBegin = lineEnd + 2U;
  while (lineBegin < bytes.size() - 2U) {
    lineEnd = bytes.find("\r\n", lineBegin);
    if (lineEnd == std::string_view::npos || lineEnd == lineBegin) {
      break;
    }
    auto const line = bytes.substr(lineBegin, lineEnd - lineBegin);
    auto const colon = line.find(':');
    if (colon == std::string_view::npos || colon == 0U) {
      fail("The WebSocket handshake contains a malformed HTTP header.");
    }
    auto const name = line.substr(0U, colon);
    if (!std::all_of(name.begin(), name.end(), [](unsigned char character) {
          return isHttpTokenCharacter(character);
        })) {
      fail("The WebSocket handshake contains an invalid HTTP header name.");
    }
    auto const value = trimHorizontalWhitespace(line.substr(colon + 1U));
    validateHttpFieldValue(value);
    message.headers[lowerAscii(name)].emplace_back(value);
    lineBegin = lineEnd + 2U;
  }
  return message;
}

[[nodiscard]] std::string readHttpMessage(
    WebSocketIo const& io,
    std::optional<std::chrono::steady_clock::time_point> deadline) {
  std::string bytes;
  bytes.reserve(1024U);
  while (bytes.size() < kMaximumHttpHeaderBytes) {
    std::array<std::uint8_t, 1U> next{};
    if (io.receiveBytes(next, deadline) == ReceiveExactResult::end_of_stream) {
      fail("The WebSocket peer closed during the HTTP handshake.");
    }
    bytes.push_back(static_cast<char>(next.front()));
    if (bytes.size() >= 4U && bytes.ends_with("\r\n\r\n")) {
      return bytes;
    }
  }
  fail("The WebSocket handshake HTTP headers exceed the configured limit.");
}

[[nodiscard]] std::vector<std::string> const& requiredHeaderValues(
    HttpMessage const& message,
    std::string_view name) {
  auto const found = message.headers.find(name);
  if (found == message.headers.end() || found->second.empty()) {
    fail("The WebSocket handshake is missing a required HTTP header.");
  }
  return found->second;
}

[[nodiscard]] bool hasHeader(
    HttpMessage const& message,
    std::string_view name) noexcept {
  return message.headers.contains(name);
}

[[nodiscard]] bool containsHeaderToken(
    std::vector<std::string> const& values,
    std::string_view token) {
  for (auto const& value : values) {
    std::size_t begin = 0U;
    while (begin <= value.size()) {
      auto const end = value.find(',', begin);
      if (lowerAscii(trimHorizontalWhitespace(std::string_view(value).substr(
              begin,
              end == std::string::npos ? std::string::npos : end - begin))) ==
          token) {
        return true;
      }
      if (end == std::string::npos) {
        break;
      }
      begin = end + 1U;
    }
  }
  return false;
}

void requireNoExtensionsOrSubprotocol(HttpMessage const& message) {
  if (hasHeader(message, "sec-websocket-extensions") ||
      hasHeader(message, "sec-websocket-protocol")) {
    fail("Umbra's WebSocket process transport does not negotiate extensions or subprotocols.");
  }
}

void sendHttpMessage(WebSocketIo const& io, std::string const& message) {
  io.sendBytes(std::span<std::uint8_t const>(
      reinterpret_cast<std::uint8_t const*>(message.data()), message.size()));
}

[[nodiscard]] bool validCloseStatus(std::uint16_t status) noexcept {
  if (status >= 3000U && status <= 4999U) {
    return true;
  }
  if (status < 1000U || status > 1014U) {
    return false;
  }
  return status != 1004U && status != 1005U && status != 1006U;
}

}  // namespace

WebSocketSocket::WebSocketSocket(
    WebSocketIo io,
    bool clientRole,
    std::size_t maximumMessageBytes)
    : io_(std::move(io)),
      clientRole_(clientRole),
      maximumMessageBytes_(maximumMessageBytes == 0U
              ? kDefaultMaximumMessageBytes
              : maximumMessageBytes) {
  if (!io_.sendBytes || !io_.receiveBytes) {
    throw std::invalid_argument(
        "A WebSocket stream requires send and receive byte operations.");
  }
}

WebSocketSocket::~WebSocketSocket() {
  close();
}

std::shared_ptr<WebSocketSocket> WebSocketSocket::connect(
    WebSocketIo io,
    WebSocketClientConfiguration const& configuration,
    std::optional<std::chrono::steady_clock::time_point> deadline) {
  auto socket = std::shared_ptr<WebSocketSocket>(new WebSocketSocket(
      std::move(io), true, configuration.maximumMessageBytes));
  socket->performClientHandshake(configuration, deadline);
  return socket;
}

std::shared_ptr<WebSocketSocket> WebSocketSocket::accept(
    WebSocketIo io,
    WebSocketServerConfiguration const& configuration,
    std::optional<std::chrono::steady_clock::time_point> deadline) {
  auto socket = std::shared_ptr<WebSocketSocket>(new WebSocketSocket(
      std::move(io), false, configuration.maximumMessageBytes));
  socket->performServerHandshake(configuration, deadline);
  return socket;
}

void WebSocketSocket::performClientHandshake(
    WebSocketClientConfiguration const& configuration,
    std::optional<std::chrono::steady_clock::time_point> deadline) {
  if (configuration.hostHeader.empty()) {
    fail("A WebSocket client requires a nonempty Host field.");
  }
  validateHttpFieldValue(configuration.hostHeader);
  validateResource(configuration.resource);

  std::array<std::uint8_t, 16U> nonce{};
  if (RAND_bytes(nonce.data(), static_cast<int>(nonce.size())) != 1) {
    fail("The WebSocket client could not generate a handshake nonce.");
  }
  auto const key = base64Encode(nonce);
  std::string request;
  request.reserve(configuration.resource.size() +
                  configuration.hostHeader.size() + 256U);
  request.append("GET ");
  request.append(configuration.resource);
  request.append(" HTTP/1.1\r\nHost: ");
  request.append(configuration.hostHeader);
  request.append("\r\nUpgrade: websocket\r\nConnection: Upgrade\r\nSec-WebSocket-Key: ");
  request.append(key);
  request.append("\r\nSec-WebSocket-Version: 13\r\n\r\n");
  sendHttpMessage(io_, request);

  auto const response = parseHttpMessage(readHttpMessage(io_, deadline));
  if (!response.startLine.starts_with("HTTP/1.1 101 ")) {
    fail("The WebSocket server did not return HTTP 101 Switching Protocols.");
  }
  if (!containsHeaderToken(
          requiredHeaderValues(response, "upgrade"), "websocket") ||
      !containsHeaderToken(
          requiredHeaderValues(response, "connection"), "upgrade")) {
    fail("The WebSocket server response is missing Upgrade or Connection tokens.");
  }
  auto const& acceptValues = requiredHeaderValues(response, "sec-websocket-accept");
  if (acceptValues.size() != 1U || acceptValues.front() != expectedAccept(key)) {
    fail("The WebSocket server returned an invalid Sec-WebSocket-Accept value.");
  }
  requireNoExtensionsOrSubprotocol(response);
  handshakeComplete_ = true;
}

void WebSocketSocket::performServerHandshake(
    WebSocketServerConfiguration const& configuration,
    std::optional<std::chrono::steady_clock::time_point> deadline) {
  validateResource(configuration.resource);
  auto const request = parseHttpMessage(readHttpMessage(io_, deadline));
  auto const firstSpace = request.startLine.find(' ');
  auto const secondSpace = firstSpace == std::string::npos
      ? std::string::npos
      : request.startLine.find(' ', firstSpace + 1U);
  if (firstSpace == std::string::npos || secondSpace == std::string::npos ||
      request.startLine.substr(0U, firstSpace) != "GET" ||
      request.startLine.substr(
          firstSpace + 1U, secondSpace - firstSpace - 1U) !=
          configuration.resource ||
      request.startLine.substr(secondSpace + 1U) != "HTTP/1.1") {
    fail("The WebSocket request line does not match the configured resource.");
  }
  auto const& hosts = requiredHeaderValues(request, "host");
  if (hosts.size() != 1U || hosts.front().empty()) {
    fail("The WebSocket request must contain exactly one nonempty Host field.");
  }
  if (!containsHeaderToken(
          requiredHeaderValues(request, "upgrade"), "websocket") ||
      !containsHeaderToken(
          requiredHeaderValues(request, "connection"), "upgrade")) {
    fail("The WebSocket request is missing Upgrade or Connection tokens.");
  }
  if (hasHeader(request, "content-length") ||
      hasHeader(request, "transfer-encoding")) {
    fail("The WebSocket opening handshake cannot carry an HTTP request body.");
  }
  auto const& versions = requiredHeaderValues(request, "sec-websocket-version");
  if (versions.size() != 1U || versions.front() != "13") {
    fail("The WebSocket request does not use RFC 6455 version 13.");
  }
  requireNoExtensionsOrSubprotocol(request);
  auto const& keys = requiredHeaderValues(request, "sec-websocket-key");
  if (keys.size() != 1U || base64Decode(keys.front()).size() != 16U) {
    fail("The WebSocket request contains an invalid Sec-WebSocket-Key.");
  }
  std::string response;
  response.reserve(256U);
  response.append("HTTP/1.1 101 Switching Protocols\r\nUpgrade: websocket\r\nConnection: Upgrade\r\nSec-WebSocket-Accept: ");
  response.append(expectedAccept(keys.front()));
  response.append("\r\n\r\n");
  sendHttpMessage(io_, response);
  handshakeComplete_ = true;
}

std::optional<WebSocketSocket::Frame> WebSocketSocket::readFrame(
    std::optional<std::chrono::steady_clock::time_point> deadline) {
  if (closed_.load(std::memory_order_acquire)) {
    return std::nullopt;
  }
  std::array<std::uint8_t, 1U> firstByte{};
  if (io_.receiveBytes(firstByte, deadline) == ReceiveExactResult::end_of_stream) {
    return std::nullopt;
  }
  std::array<std::uint8_t, 1U> secondByte{};
  if (io_.receiveBytes(secondByte, deadline) == ReceiveExactResult::end_of_stream) {
    fail("The WebSocket peer closed inside a frame header.");
  }
  std::array<std::uint8_t, 2U> header{firstByte.front(), secondByte.front()};
  if ((header[0] & 0x70U) != 0U) {
    protocolError("WebSocket RSV bits must be zero without extensions.", 1002U);
  }
  Frame frame;
  frame.final = (header[0] & 0x80U) != 0U;
  frame.opcode = static_cast<std::uint8_t>(header[0] & 0x0fU);
  auto const masked = (header[1] & 0x80U) != 0U;
  if (masked != !clientRole_) {
    protocolError(
        clientRole_ ? "A WebSocket server frame must not be masked."
                    : "A WebSocket client frame must be masked.",
        1002U);
  }
  std::uint64_t payloadSize = header[1] & 0x7fU;
  if (payloadSize == 126U) {
    std::array<std::uint8_t, 2U> extended{};
    if (io_.receiveBytes(extended, deadline) == ReceiveExactResult::end_of_stream) {
      fail("The WebSocket peer closed inside an extended frame header.");
    }
    payloadSize = (static_cast<std::uint64_t>(extended[0]) << 8U) |
        static_cast<std::uint64_t>(extended[1]);
    if (payloadSize < 126U) {
      protocolError("A WebSocket frame uses a non-minimal payload length.", 1002U);
    }
  } else if (payloadSize == 127U) {
    std::array<std::uint8_t, 8U> extended{};
    if (io_.receiveBytes(extended, deadline) == ReceiveExactResult::end_of_stream) {
      fail("The WebSocket peer closed inside an extended frame header.");
    }
    if ((extended[0] & 0x80U) != 0U) {
      protocolError("A WebSocket 64-bit frame length has its reserved high bit set.", 1002U);
    }
    payloadSize = 0U;
    for (auto const byte : extended) {
      payloadSize = (payloadSize << 8U) | byte;
    }
    if (payloadSize <= 65535U) {
      protocolError("A WebSocket frame uses a non-minimal payload length.", 1002U);
    }
  }
  auto const isControl = (frame.opcode & 0x08U) != 0U;
  if (isControl && (!frame.final || payloadSize > 125U)) {
    protocolError("A WebSocket control frame is fragmented or oversized.", 1002U);
  }
  if (payloadSize > maximumMessageBytes_ ||
      payloadSize > static_cast<std::uint64_t>(
                        std::numeric_limits<std::size_t>::max())) {
    protocolError("The WebSocket frame exceeds the configured message limit.", 1009U);
  }
  switch (frame.opcode) {
    case 0x0U:
    case 0x1U:
    case 0x2U:
    case 0x8U:
    case 0x9U:
    case 0xaU:
      break;
    default:
      protocolError("The WebSocket frame uses an unknown opcode.", 1002U);
  }
  if (frame.opcode == 0x8U && payloadSize == 1U) {
    protocolError("A WebSocket Close frame cannot contain one payload byte.", 1002U);
  }
  std::array<std::uint8_t, 4U> mask{};
  if (masked &&
      io_.receiveBytes(mask, deadline) == ReceiveExactResult::end_of_stream) {
    fail("The WebSocket peer closed inside a masked frame header.");
  }
  frame.payload.resize(static_cast<std::size_t>(payloadSize));
  if (!frame.payload.empty() &&
      io_.receiveBytes(frame.payload, deadline) == ReceiveExactResult::end_of_stream) {
    fail("The WebSocket peer closed inside a frame payload.");
  }
  if (masked) {
    for (std::size_t index = 0U; index < frame.payload.size(); ++index) {
      frame.payload[index] ^= mask[index % mask.size()];
    }
  }
  if (frame.opcode == 0x8U && frame.payload.size() >= 2U) {
    auto const status = static_cast<std::uint16_t>(
        (static_cast<std::uint16_t>(frame.payload[0]) << 8U) |
        static_cast<std::uint16_t>(frame.payload[1]));
    if (!validCloseStatus(status)) {
      protocolError("A WebSocket Close frame contains an invalid status code.", 1002U);
    }
  }
  return frame;
}

std::optional<std::vector<std::uint8_t>> WebSocketSocket::readMessage(
    std::optional<std::chrono::steady_clock::time_point> deadline) {
  while (true) {
    auto frame = readFrame(deadline);
    if (!frame) {
      return std::nullopt;
    }
    if (frame->opcode == 0x9U) {
      writeFrame(0xaU, frame->payload);
      continue;
    }
    if (frame->opcode == 0xaU) {
      continue;
    }
    if (frame->opcode == 0x8U) {
      {
        std::scoped_lock sendLock(sendMutex_);
        if (!closeFrameSent_) {
          writeFrameUnderLock(0x8U, frame->payload);
          closeFrameSent_ = true;
        }
      }
      closed_.store(true, std::memory_order_release);
      return std::nullopt;
    }
    if (frame->opcode == 0x1U) {
      protocolError("WebSocket text data frames are not allowed.", 1003U);
    }
    if (frame->opcode == 0x2U) {
      if (fragmentedMessageActive_) {
        protocolError("A WebSocket data message began before the previous message ended.", 1002U);
      }
      if (frame->final) {
        return std::move(frame->payload);
      }
      fragmentedMessage_ = std::move(frame->payload);
      fragmentedMessageActive_ = true;
      continue;
    }
    if (frame->opcode == 0x0U) {
      if (!fragmentedMessageActive_) {
        protocolError("A WebSocket continuation frame has no open message.", 1002U);
      }
      if (frame->payload.size() > maximumMessageBytes_ - fragmentedMessage_.size()) {
        protocolError("The fragmented WebSocket message exceeds the configured limit.", 1009U);
      }
      fragmentedMessage_.insert(
          fragmentedMessage_.end(), frame->payload.begin(), frame->payload.end());
      if (frame->final) {
        fragmentedMessageActive_ = false;
        return std::exchange(fragmentedMessage_, {});
      }
    }
  }
}

void WebSocketSocket::sendAll(std::span<std::uint8_t const> bytes) {
  if (!handshakeComplete_) {
    fail("A WebSocket message cannot be sent before the opening handshake completes.");
  }
  if (bytes.size() > maximumMessageBytes_) {
    fail("The WebSocket message exceeds the configured size limit.");
  }
  if (bytes.empty()) {
    return;
  }
  writeFrame(0x2U, bytes);
}

ReceiveExactResult WebSocketSocket::receiveExact(
    std::span<std::uint8_t> bytes,
    std::optional<std::chrono::steady_clock::time_point> deadline) {
  if (!handshakeComplete_) {
    fail("A WebSocket message cannot be received before the opening handshake completes.");
  }
  if (bytes.empty()) {
    return ReceiveExactResult::complete;
  }
  std::scoped_lock receiveLock(receiveMutex_);
  std::size_t written = 0U;
  while (written < bytes.size()) {
    if (receiveOffset_ >= receiveBuffer_.size()) {
      receiveBuffer_.clear();
      receiveOffset_ = 0U;
      auto message = readMessage(deadline);
      if (!message) {
        if (written == 0U) {
          return ReceiveExactResult::end_of_stream;
        }
        fail("A WebSocket message stream ended before the requested bytes were received.");
      }
      receiveBuffer_ = std::move(*message);
      if (receiveBuffer_.empty()) {
        continue;
      }
    }
    auto const available = receiveBuffer_.size() - receiveOffset_;
    auto const count = std::min(available, bytes.size() - written);
    std::copy_n(
        receiveBuffer_.data() + receiveOffset_,
        count,
        bytes.data() + written);
    receiveOffset_ += count;
    written += count;
  }
  return ReceiveExactResult::complete;
}

void WebSocketSocket::writeFrame(
    std::uint8_t opcode,
    std::span<std::uint8_t const> payload) {
  std::scoped_lock sendLock(sendMutex_);
  if (closed_.load(std::memory_order_acquire)) {
    fail("The WebSocket connection is closed.");
  }
  writeFrameUnderLock(opcode, payload);
}

void WebSocketSocket::writeFrameUnderLock(
    std::uint8_t opcode,
    std::span<std::uint8_t const> payload) {
  std::vector<std::uint8_t> frame;
  auto const masked = clientRole_;
  auto const maskBit = masked ? 0x80U : 0U;
  frame.reserve(payload.size() + 14U);
  frame.push_back(static_cast<std::uint8_t>(0x80U | opcode));
  if (payload.size() < 126U) {
    frame.push_back(static_cast<std::uint8_t>(maskBit | payload.size()));
  } else if (payload.size() <= 65535U) {
    frame.push_back(static_cast<std::uint8_t>(maskBit | 126U));
    frame.push_back(static_cast<std::uint8_t>((payload.size() >> 8U) & 0xffU));
    frame.push_back(static_cast<std::uint8_t>(payload.size() & 0xffU));
  } else {
    frame.push_back(static_cast<std::uint8_t>(maskBit | 127U));
    auto const wideSize = static_cast<std::uint64_t>(payload.size());
    for (int shift = 56; shift >= 0; shift -= 8) {
      frame.push_back(static_cast<std::uint8_t>((wideSize >> shift) & 0xffU));
    }
  }
  std::array<std::uint8_t, 4U> mask{};
  if (masked) {
    if (RAND_bytes(mask.data(), static_cast<int>(mask.size())) != 1) {
      fail("The WebSocket client could not generate a frame mask.");
    }
    frame.insert(frame.end(), mask.begin(), mask.end());
  }
  auto const payloadBegin = frame.size();
  frame.insert(frame.end(), payload.begin(), payload.end());
  if (masked) {
    for (std::size_t index = 0U; index < payload.size(); ++index) {
      frame[payloadBegin + index] ^= mask[index % mask.size()];
    }
  }
  io_.sendBytes(frame);
}

void WebSocketSocket::sendClose(std::uint16_t statusCode) noexcept {
  try {
    std::scoped_lock sendLock(sendMutex_);
    if (!handshakeComplete_ || closeFrameSent_) {
      closed_.store(true, std::memory_order_release);
      return;
    }
    std::array<std::uint8_t, 2U> payload{
        static_cast<std::uint8_t>((statusCode >> 8U) & 0xffU),
        static_cast<std::uint8_t>(statusCode & 0xffU)};
    writeFrameUnderLock(0x8U, payload);
    closeFrameSent_ = true;
  } catch (...) {
  }
  closed_.store(true, std::memory_order_release);
}

[[noreturn]] void WebSocketSocket::protocolError(
    char const* message,
    std::uint16_t statusCode) {
  sendClose(statusCode);
  throw std::runtime_error(message);
}

void WebSocketSocket::close() noexcept {
  if (!closed_.load(std::memory_order_acquire)) {
    sendClose(1000U);
  }
}

}  // namespace umbra::detail::platform
