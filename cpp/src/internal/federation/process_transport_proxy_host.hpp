#pragma once

#include "internal/federation/process_transport.hpp"

#include <atomic>
#include <exception>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

namespace umbra::detail {

// Registry-free ingress that relays complete private transport frames to one
// authoritative service endpoint. It owns no federation or time-management
// state; the upstream service remains the sole scheduler authority.
class ProcessTransportProxyHost final {
 public:
  ProcessTransportProxyHost(
      ProcessTransportAddress listenAddress,
      ProcessTransportAddress upstreamAddress,
      TransportEndpointIdentity proxyIdentity);

  ProcessTransportProxyHost(ProcessTransportProxyHost const&) = delete;
  ProcessTransportProxyHost& operator=(ProcessTransportProxyHost const&) = delete;

  ~ProcessTransportProxyHost();

  void start();
  void stop() noexcept;

  [[nodiscard]] ProcessTransportAddress address() const noexcept;
  [[nodiscard]] bool running() const noexcept;
  void rethrowIfFailed() const;

 private:
  struct Worker;

  void acceptLoop() noexcept;
  void relayFrames(
      Worker& worker,
      std::shared_ptr<ProcessTransportConnection> const& source,
      std::shared_ptr<ProcessTransportConnection> const& destination) noexcept;
  void reapFinishedWorkers();
  void recordFailure(std::exception_ptr failure) noexcept;

  std::unique_ptr<ProcessTransportListener> listener_;
  ProcessTransportAddress upstreamAddress_;
  TransportEndpointIdentity proxyIdentity_;
  mutable std::mutex workerMutex_;
  std::vector<std::unique_ptr<Worker>> workers_;
  std::exception_ptr failure_;
  std::atomic_bool running_{false};
  bool started_ = false;
  std::thread acceptThread_;
};

}  // namespace umbra::detail
