#pragma once

#include "internal/federation/process_federation_service.hpp"

#include <atomic>
#include <exception>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

namespace umbra::detail {

// Runtime-owned accept and dispatch loop for one federation service process.
// A single service and registry own the federation state; multiple RTI clients
// connect over independent process-transport sessions.
class ProcessFederationServiceHost final {
 public:
  ProcessFederationServiceHost(
      EmbeddedFederationRegistry& registry,
      FederationDefinition federationDefinition,
      ProcessTransportAddress listenAddress,
      TransportEndpointIdentity localIdentity,
      ProcessFederationServiceOptions options = {});

  ProcessFederationServiceHost(ProcessFederationServiceHost const&) = delete;
  ProcessFederationServiceHost& operator=(
      ProcessFederationServiceHost const&) = delete;

  ~ProcessFederationServiceHost();

  void start();
  void stop() noexcept;

  [[nodiscard]] ProcessTransportAddress address() const noexcept;
  [[nodiscard]] bool running() const noexcept;
  void rethrowIfFailed() const;

 private:
  struct Worker;

  void acceptLoop() noexcept;
  void serveConnection(Worker& worker) noexcept;
  void reapFinishedWorkers();
  void recordFailure(std::exception_ptr failure) noexcept;

  ProcessFederationService service_;
  std::unique_ptr<ProcessTransportListener> listener_;
  TransportEndpointIdentity localIdentity_;
  mutable std::mutex workerMutex_;
  std::vector<std::unique_ptr<Worker>> workers_;
  std::exception_ptr failure_;
  std::atomic_bool running_{false};
  bool started_ = false;
  std::thread acceptThread_;
};

}  // namespace umbra::detail
