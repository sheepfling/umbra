#pragma once

#include "internal/federation/process_transport.hpp"
#include "internal/federation/transport_service_protocol.hpp"

#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>

namespace umbra::detail {

class ProcessTransportSessionError final : public std::runtime_error {
 public:
  using std::runtime_error::runtime_error;
};

// Synchronous request/response seam for private federation services. A later
// dispatcher can bind each operation to EmbeddedFederationRegistry without
// changing the endpoint, envelope, or official public C++ binding.
class ProcessTransportSession final {
 public:
  enum class InitialFrame : std::uint8_t {
    disconnected,
    service_message,
    resume_request,
    terminate_session,
  };

  explicit ProcessTransportSession(
      std::shared_ptr<ProcessTransportConnection> connection,
      bool replyToPeerHeartbeats = false,
      std::function<bool()> replyToPeerHeartbeatsForTesting = {});

  void replaceConnection(
      std::shared_ptr<ProcessTransportConnection> connection);

  [[nodiscard]] bool send(TransportServiceMessage const& message);
  [[nodiscard]] std::function<bool(TransportServiceMessage const&)>
  makeServiceSender() const;
  [[nodiscard]] bool receive(
      TransportServiceMessage& message,
      TransportFrameKind* receivedFrameKind = nullptr,
      std::uint32_t* receivedSequenceNumber = nullptr,
      std::uint32_t* receivedLastReceivedMessageNumber = nullptr);
  [[nodiscard]] bool sendCloseAcknowledgement();
  [[nodiscard]] static bool sendCallbackResponse(
      std::shared_ptr<ProcessTransportConnection> const& connection,
      std::uint32_t callbackRequestSequenceNumber,
      std::optional<std::pair<std::string, std::string>> exception =
          std::nullopt);
  [[nodiscard]] bool sendResumeRequest(TransportResumeRequest const& request);
  [[nodiscard]] bool receiveResumeRequest(TransportResumeRequest& request);
  [[nodiscard]] InitialFrame receiveInitialFrame(
      TransportResumeRequest& request);
  [[nodiscard]] TransportResumeStatus assessResumeRequest(
      TransportResumeRequest const& request) const;
  [[nodiscard]] bool sendResumeStatus(TransportResumeStatus const& status);
  [[nodiscard]] bool receiveResumeStatus(TransportResumeStatus& status);
  [[nodiscard]] bool replayOutgoingAfter(std::uint32_t peerLastReceived);
  [[nodiscard]] bool request(
      TransportServiceMessage request,
      TransportServiceMessage& response);

  [[nodiscard]] std::shared_ptr<ProcessTransportConnection> connection() const;

 private:
  struct ConnectionState final {
    mutable std::mutex mutex;
    std::shared_ptr<ProcessTransportConnection> connection;
  };

  [[nodiscard]] bool receiveResumeFrame(
      TransportFrameKind expectedKind,
      TransportFrame& frame);
  [[nodiscard]] bool acknowledgePeerHeartbeat(
      std::shared_ptr<ProcessTransportConnection> const& connection,
      std::uint32_t receivedSequenceNumber);

  std::shared_ptr<ConnectionState> connectionState_;
  bool replyToPeerHeartbeats_ = false;
  std::function<bool()> replyToPeerHeartbeatsForTesting_;
  std::optional<std::uint32_t> receivedResumeRequestSequenceNumber_;
  std::optional<std::uint32_t> receivedTerminateSessionSequenceNumber_;
  std::optional<TransportFrame> pendingServiceFrame_;
};

class ProcessTransportServiceDispatcher final {
 public:
  using Handler = std::function<TransportServiceMessage(
      TransportServiceMessage const&)>;

  ProcessTransportServiceDispatcher() = delete;

  [[nodiscard]] static bool serveOne(
      ProcessTransportSession& session,
      Handler const& handler,
      bool* terminateSessionReceived = nullptr);
};

}  // namespace umbra::detail
