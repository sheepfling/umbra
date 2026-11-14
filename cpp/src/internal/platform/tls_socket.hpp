#pragma once

#include "internal/platform/socket_operations.hpp"

#include <chrono>
#include <filesystem>
#include <memory>
#include <optional>
#include <span>
#include <string>

namespace umbra::detail::platform {

struct TlsClientConfiguration final {
  enum class CertificateValidationMode { encryption_only, server_certificate };

  std::filesystem::path trustStoreFile;
  std::string peerName;
  CertificateValidationMode certificateValidationMode =
      CertificateValidationMode::server_certificate;
};

struct TlsServerConfiguration final {
  std::filesystem::path certificateChainFile;
  std::filesystem::path privateKeyFile;
};

// Internal TLS 1.3 stream for the private process transport. The stream takes
// ownership of its already-connected native socket, serializes OpenSSL calls
// while permitting blocked reads and writes to make independent progress, and
// never falls back to plaintext if setup or negotiation fails.
class TlsSocket final {
 public:
  TlsSocket(TlsSocket const&) = delete;
  TlsSocket& operator=(TlsSocket const&) = delete;
  TlsSocket(TlsSocket&&) noexcept;
  TlsSocket& operator=(TlsSocket&&) noexcept;
  ~TlsSocket();

  [[nodiscard]] static TlsSocket connect(
      NativeSocket socket,
      TlsClientConfiguration const& configuration,
      std::optional<std::chrono::steady_clock::time_point> deadline = std::nullopt);
  [[nodiscard]] static TlsSocket accept(
      NativeSocket socket,
      TlsServerConfiguration const& configuration,
      std::optional<std::chrono::steady_clock::time_point> deadline = std::nullopt);
  static void validateServerConfiguration(
      TlsServerConfiguration const& configuration);

  void sendAll(std::span<std::uint8_t const> bytes);
  [[nodiscard]] ReceiveExactResult receiveExact(
      std::span<std::uint8_t> bytes,
      std::optional<std::chrono::steady_clock::time_point> deadline = std::nullopt);
  [[nodiscard]] int negotiatedProtocolVersion() const noexcept;
  [[nodiscard]] NativeSocket nativeSocket() const noexcept;
  void close() noexcept;

 private:
  struct Impl;
  explicit TlsSocket(std::unique_ptr<Impl> implementation) noexcept;

  std::unique_ptr<Impl> implementation_;
};

}  // namespace umbra::detail::platform
