#pragma once

#include "internal/federation/process_transport.hpp"
#include "internal/federation/transport_service_protocol.hpp"

#include <functional>
#include <memory>
#include <stdexcept>

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
  explicit ProcessTransportSession(
      std::shared_ptr<ProcessTransportConnection> connection);

  [[nodiscard]] bool send(TransportServiceMessage const& message);
  [[nodiscard]] bool receive(TransportServiceMessage& message);
  [[nodiscard]] bool request(
      TransportServiceMessage request,
      TransportServiceMessage& response);

  [[nodiscard]] std::shared_ptr<ProcessTransportConnection> connection() const
      noexcept;

 private:
  std::shared_ptr<ProcessTransportConnection> connection_;
};

class ProcessTransportServiceDispatcher final {
 public:
  using Handler = std::function<TransportServiceMessage(
      TransportServiceMessage const&)>;

  ProcessTransportServiceDispatcher() = delete;

  [[nodiscard]] static bool serveOne(
      ProcessTransportSession& session,
      Handler const& handler);
};

}  // namespace umbra::detail
