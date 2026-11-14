#pragma once

#include "internal/federation/process_federation_service.hpp"
#include "internal/platform/tls_socket.hpp"

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <exception>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <string_view>
#include <thread>
#include <vector>

namespace umbra::detail {

// IEEE 1516.1-2025 §12.13.4.5 Table 31 RTI protocol settings. The defaults
// are the standard's RTI_TIMEOUT_HEART and RTI_TIMEOUT_PURGE values.
struct ProcessFederationServiceHostOptions final {
  std::chrono::milliseconds rtiTimeoutHeart{180000};
  std::chrono::milliseconds rtiTimeoutPurge{600000};
  // Required when the configured listener address selects TLS 1.3. The
  // certificate chain and private key are supplied by the coordinator owner.
  std::optional<platform::TlsServerConfiguration> tlsServerConfiguration;
  // The server replies to federate heartbeats so a silent client can detect a
  // broken RTI path. Tests can suppress replies to exercise the client timeout.
  bool replyToFederateHeartbeats = true;
  // When set by a process fixture, this dynamically overrides the fixed flag
  // and runs on the session worker that receives federate heartbeats.
  std::function<bool()> replyToFederateHeartbeatsForTesting;
  // Process fixtures can observe the exact point the RTI marks a session
  // inactive after RTI_TIMEOUT_HEART elapses without peer activity.
  std::function<void()> onSessionInactivatedForTesting;
  std::function<void()> onSessionResumedForTesting;
};

[[nodiscard]] ProcessFederationServiceHostOptions
processFederationServiceHostOptionsFromRtiInitializationData(
    std::wstring_view configurationName = {});

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
  ProcessFederationServiceHost(
      EmbeddedFederationRegistry& registry,
      FederationDefinition federationDefinition,
      ProcessTransportAddress listenAddress,
      TransportEndpointIdentity localIdentity,
      ProcessFederationServiceOptions options,
      ProcessFederationServiceHostOptions hostOptions);

  // Multiple listeners may share one authoritative coordinator. This keeps
  // joined-federate state and callback routing common across endpoints.
  ProcessFederationServiceHost(
      std::shared_ptr<ProcessFederationService> service,
      ProcessTransportAddress listenAddress,
      TransportEndpointIdentity localIdentity);
  ProcessFederationServiceHost(
      std::shared_ptr<ProcessFederationService> service,
      ProcessTransportAddress listenAddress,
      TransportEndpointIdentity localIdentity,
      ProcessFederationServiceHostOptions hostOptions);

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
  struct SessionRecord;

  void acceptLoop() noexcept;
  void serveConnection(Worker& worker) noexcept;
  void maintenanceLoop() noexcept;
  void markSessionInactive(std::uint64_t sessionId) noexcept;
  void purgeSession(std::uint64_t sessionId) noexcept;
  void terminateSession(
      std::uint64_t sessionId,
      std::wstring faultDescription) noexcept;
  void reapFinishedWorkers();
  void recordFailure(std::exception_ptr failure) noexcept;

  std::shared_ptr<ProcessFederationService> service_;
  std::unique_ptr<ProcessTransportListener> listener_;
  TransportEndpointIdentity localIdentity_;
  ProcessFederationServiceHostOptions hostOptions_;
  std::map<std::uint64_t, std::shared_ptr<SessionRecord>> sessions_;
  mutable std::mutex workerMutex_;
  std::condition_variable maintenanceChanged_;
  std::vector<std::unique_ptr<Worker>> workers_;
  std::exception_ptr failure_;
  std::atomic_bool running_{false};
  bool started_ = false;
  std::thread acceptThread_;
  std::thread maintenanceThread_;
};

}  // namespace umbra::detail
