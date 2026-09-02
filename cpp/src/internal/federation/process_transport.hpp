#pragma once

#include "internal/federation/embedded_transport.hpp"
#include "internal/federation/transport_protocol.hpp"

#include <cstdint>
#include <limits>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>

namespace umbra::detail {

struct ProcessTransportAddress final {
  std::string host;
  std::uint16_t port = 0U;
};

class ProcessTransportError final : public std::runtime_error {
 public:
  using std::runtime_error::runtime_error;
};

// Private synchronous socket endpoint used to grow the process-boundary
// transport incrementally. The public RTI binding does not expose this type;
// service/session dispatch will layer on the framed data path later.
class ProcessTransportConnection final : public TransportConnection {
 public:
  using FailureHandler = TransportConnection::FailureHandler;
  using ForcedResignationHandler = TransportConnection::ForcedResignationHandler;

  ProcessTransportConnection(ProcessTransportConnection const&) = delete;
  ProcessTransportConnection& operator=(ProcessTransportConnection const&) = delete;

  ~ProcessTransportConnection() override;

  static std::shared_ptr<ProcessTransportConnection> connectClient(
      void* owner,
      ProcessTransportAddress address,
      TransportEndpointIdentity localIdentity,
      FailureHandler failureHandler,
      ForcedResignationHandler forcedResignationHandler,
      std::shared_ptr<RuntimeInstrumentation> instrumentation = {});

  // Send and receive one complete private protocol frame. These operations
  // are synchronous by design; a future session dispatcher can own the read
  // loop without changing the wire contract or the public RTI adapter.
  [[nodiscard]] bool sendFrame(TransportFrame const& frame);
  [[nodiscard]] bool receiveFrame(TransportFrame& frame);

  void close() noexcept override;
  void fail(std::wstring faultDescription) override;
  [[nodiscard]] bool forceFederateResignation(
      std::wstring reasonForResign) override;
  [[nodiscard]] bool open() const noexcept override;
  [[nodiscard]] void* owner() const noexcept override;

  [[nodiscard]] TransportEndpointIdentity const& localIdentity() const noexcept;
  [[nodiscard]] TransportEndpointIdentity const& peerIdentity() const noexcept;

 private:
  friend class ProcessTransportListener;

  ProcessTransportConnection(
      std::uintptr_t nativeSocket,
      void* owner,
      TransportEndpointIdentity localIdentity,
      TransportEndpointIdentity peerIdentity,
      FailureHandler failureHandler,
      ForcedResignationHandler forcedResignationHandler,
      std::shared_ptr<RuntimeInstrumentation> instrumentation);

  mutable std::mutex stateMutex_;
  mutable std::mutex sendMutex_;
  std::uintptr_t nativeSocket_ = std::numeric_limits<std::uintptr_t>::max();
  void* owner_ = nullptr;
  TransportEndpointIdentity localIdentity_;
  TransportEndpointIdentity peerIdentity_;
  FailureHandler failureHandler_;
  ForcedResignationHandler forcedResignationHandler_;
  std::shared_ptr<RuntimeInstrumentation> instrumentation_;
  bool open_ = false;
};

class ProcessTransportListener final {
 public:
  ProcessTransportListener(ProcessTransportListener const&) = delete;
  ProcessTransportListener& operator=(ProcessTransportListener const&) = delete;

  ~ProcessTransportListener();

  static std::unique_ptr<ProcessTransportListener> listen(
      ProcessTransportAddress address);

  [[nodiscard]] std::shared_ptr<ProcessTransportConnection> accept(
      void* owner,
      TransportEndpointIdentity localIdentity,
      ProcessTransportConnection::FailureHandler failureHandler,
      ProcessTransportConnection::ForcedResignationHandler forcedResignationHandler,
      std::shared_ptr<RuntimeInstrumentation> instrumentation = {});

  [[nodiscard]] ProcessTransportAddress address() const noexcept;

 private:
  ProcessTransportListener(std::uintptr_t nativeSocket, ProcessTransportAddress address);

  std::uintptr_t nativeSocket_ = std::numeric_limits<std::uintptr_t>::max();
  ProcessTransportAddress address_;
};

}  // namespace umbra::detail
