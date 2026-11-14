#pragma once

#include "internal/federation/embedded_transport.hpp"
#include "internal/federation/process_transport_message_history.hpp"
#include "internal/federation/transport_protocol.hpp"
#include "internal/platform/tls_socket.hpp"
#include "internal/platform/websocket_socket.hpp"

#include <cstdint>
#include <chrono>
#include <filesystem>
#include <functional>
#include <limits>
#include <memory>
#include <mutex>
#include <optional>
#include <set>
#include <stdexcept>
#include <string>

namespace umbra::detail {

struct ProcessTransportAddress final {
  std::string host;
  std::uint16_t port = 0U;
  enum class Security { tcp, tls13 } security = Security::tcp;
  std::filesystem::path tlsTrustStoreFile;
  platform::TlsClientConfiguration::CertificateValidationMode
      tlsCertificateValidationMode =
          platform::TlsClientConfiguration::CertificateValidationMode::server_certificate;
  enum class Framing { process_stream, websocket } framing =
      Framing::process_stream;
  std::string websocketHostHeader;
  std::string websocketResource = "/";
};

class ProcessTransportError final : public std::runtime_error {
 public:
  using std::runtime_error::runtime_error;
};

// Private synchronous socket endpoint for process-boundary service traffic.
// The public RTI binding does not expose this type; service/session dispatch
// carries encoded messages through its framed data path.
class ProcessTransportConnection final : public TransportConnection {
 public:
  using FailureHandler = TransportConnection::FailureHandler;
  using ForcedResignationHandler = TransportConnection::ForcedResignationHandler;
  using TestSendFrameHandler =
      std::function<bool(TransportFrame const& frame)>;
  using ConnectedHandler =
      std::function<void(std::shared_ptr<ProcessTransportConnection> const&)>;

  ProcessTransportConnection(ProcessTransportConnection const&) = delete;
  ProcessTransportConnection& operator=(ProcessTransportConnection const&) = delete;

  ~ProcessTransportConnection() override;

  static std::shared_ptr<ProcessTransportConnection> connectClient(
      void* owner,
      ProcessTransportAddress address,
      TransportEndpointIdentity localIdentity,
      FailureHandler failureHandler,
      ForcedResignationHandler forcedResignationHandler,
      std::shared_ptr<RuntimeInstrumentation> instrumentation = {},
      std::shared_ptr<ProcessTransportSessionState> sessionState = {},
      ConnectedHandler connectedHandler = {},
      std::optional<std::chrono::steady_clock::time_point> deadline = std::nullopt);

  // Creates a send-only, socketless endpoint for private service tests. The
  // embedded/runtime construction paths remain listener/client socket-backed.
  [[nodiscard]] static std::shared_ptr<ProcessTransportConnection>
  createForTesting(
      TestSendFrameHandler sendFrameHandler,
      std::uint64_t sessionId = 1U);

  // Send and receive one complete private protocol frame. These operations
  // are synchronous by design; a future session dispatcher can own the read
  // loop without changing the wire contract or the public RTI adapter.
  [[nodiscard]] bool sendFrame(TransportFrame const& frame);
  // Allocates a protocol sequence number and puts its frame on the wire under
  // one send lock, preserving sequence order across service and control senders.
  [[nodiscard]] bool sendSequencedFrame(
      std::function<TransportFrame(std::uint32_t)> const& makeFrame);
  [[nodiscard]] bool receiveFrame(TransportFrame& frame);

  void close() noexcept override;
  void fail(std::wstring faultDescription) override;
  [[nodiscard]] bool forceFederateResignation(
      std::wstring reasonForResign) override;
  [[nodiscard]] bool open() const noexcept override;
  [[nodiscard]] void* owner() const noexcept override;

  [[nodiscard]] TransportEndpointIdentity const& localIdentity() const noexcept;
  [[nodiscard]] TransportEndpointIdentity const& peerIdentity() const noexcept;
  [[nodiscard]] std::uint64_t sessionId() const noexcept;
  [[nodiscard]] std::shared_ptr<ProcessTransportSessionState> sessionState()
      const noexcept;
  [[nodiscard]] ProcessTransportMessageHistory& messageHistory() noexcept;
  [[nodiscard]] ProcessTransportSequenceState& sequenceState() noexcept;

 private:
  friend class ProcessTransportListener;

  ProcessTransportConnection(
      std::uintptr_t nativeSocket,
      void* owner,
      TransportEndpointIdentity localIdentity,
      TransportEndpointIdentity peerIdentity,
      FailureHandler failureHandler,
      ForcedResignationHandler forcedResignationHandler,
      std::shared_ptr<RuntimeInstrumentation> instrumentation,
      std::shared_ptr<ProcessTransportSessionState> sessionState,
      std::optional<TransportFrame> initialFrame = std::nullopt,
      std::shared_ptr<platform::TlsSocket> tlsSocket = {},
      std::shared_ptr<platform::WebSocketSocket> websocketSocket = {});
  ProcessTransportConnection(
      TestSendFrameHandler sendFrameHandler,
      std::shared_ptr<ProcessTransportSessionState> sessionState);
  [[nodiscard]] bool sendFrameUnderSendLock(TransportFrame const& frame);

  mutable std::mutex stateMutex_;
  mutable std::mutex sendMutex_;
  std::uintptr_t nativeSocket_ = std::numeric_limits<std::uintptr_t>::max();
  void* owner_ = nullptr;
  TransportEndpointIdentity localIdentity_;
  TransportEndpointIdentity peerIdentity_;
  FailureHandler failureHandler_;
  ForcedResignationHandler forcedResignationHandler_;
  TestSendFrameHandler testSendFrameHandler_;
  std::shared_ptr<RuntimeInstrumentation> instrumentation_;
  std::shared_ptr<ProcessTransportSessionState> sessionState_;
  std::shared_ptr<platform::TlsSocket> tlsSocket_;
  std::shared_ptr<platform::WebSocketSocket> websocketSocket_;
  std::optional<TransportFrame> initialFrame_;
  bool open_ = false;
};

class ProcessTransportListener final {
 public:
  using SessionStateResolver = std::function<
      std::shared_ptr<ProcessTransportSessionState>(
          std::uint64_t sessionId)>;

  ProcessTransportListener(ProcessTransportListener const&) = delete;
  ProcessTransportListener& operator=(ProcessTransportListener const&) = delete;

  ~ProcessTransportListener();

  static std::unique_ptr<ProcessTransportListener> listen(
      ProcessTransportAddress address,
      std::optional<platform::TlsServerConfiguration> tlsServerConfiguration =
          std::nullopt);

  [[nodiscard]] std::shared_ptr<ProcessTransportConnection> accept(
      void* owner,
      TransportEndpointIdentity localIdentity,
      ProcessTransportConnection::FailureHandler failureHandler,
      ProcessTransportConnection::ForcedResignationHandler forcedResignationHandler,
      std::shared_ptr<RuntimeInstrumentation> instrumentation = {},
      std::shared_ptr<ProcessTransportSessionState> sessionState = {},
      SessionStateResolver resolveSessionState = {});

  [[nodiscard]] ProcessTransportAddress address() const noexcept;

  // Closes the listening socket without destroying the listener object. This
  // lets an owner safely release a thread blocked in accept() before joining
  // it, avoiding a native-socket lifetime race during error cleanup.
  void close() noexcept;

 private:
  ProcessTransportListener(
      std::uintptr_t nativeSocket,
      ProcessTransportAddress address,
      std::optional<platform::TlsServerConfiguration> tlsServerConfiguration);

  mutable std::mutex mutex_;
  std::uintptr_t nativeSocket_ = std::numeric_limits<std::uintptr_t>::max();
  std::set<std::uintptr_t> pendingSockets_;
  ProcessTransportAddress address_;
  std::optional<platform::TlsServerConfiguration> tlsServerConfiguration_;
};

}  // namespace umbra::detail
