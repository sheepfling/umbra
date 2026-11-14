#include "internal/federation/process_federation_service_host.hpp"
#include "internal/runtime/rti_initialization_data.hpp"

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <optional>
#include <stdexcept>
#include <utility>

namespace umbra::detail {

namespace {

std::shared_ptr<ProcessFederationService> requireService(
    std::shared_ptr<ProcessFederationService> service) {
  if (!service) {
    throw std::invalid_argument(
        "A process federation service host requires a shared service.");
  }
  return service;
}

std::optional<platform::TlsServerConfiguration> tlsConfigurationForListener(
    ProcessTransportAddress const& address,
    ProcessFederationServiceHostOptions const& options) {
  return address.security == ProcessTransportAddress::Security::tls13
      ? options.tlsServerConfiguration
      : std::nullopt;
}

}  // namespace

ProcessFederationServiceHostOptions
processFederationServiceHostOptionsFromRtiInitializationData(
    const std::wstring_view configurationName) {
  auto const settings =
      loadRtiProtocolTimeoutSettingsFromInitializationData(configurationName);
  ProcessFederationServiceHostOptions options;
  options.rtiTimeoutHeart = settings.rtiTimeoutHeart;
  options.rtiTimeoutPurge = settings.rtiTimeoutPurge;
  auto const* certificateChain =
      std::getenv("UMBRA_RTI_TLS_CERTIFICATE_CHAIN_FILE");
  auto const* privateKey = std::getenv("UMBRA_RTI_TLS_PRIVATE_KEY_FILE");
  bool const hasCertificate = certificateChain != nullptr && *certificateChain != '\0';
  bool const hasPrivateKey = privateKey != nullptr && *privateKey != '\0';
  if (hasCertificate != hasPrivateKey) {
    throw std::invalid_argument(
        "TLS listener configuration requires both UMBRA_RTI_TLS_CERTIFICATE_CHAIN_FILE and UMBRA_RTI_TLS_PRIVATE_KEY_FILE.");
  }
  if (hasCertificate) {
    options.tlsServerConfiguration = platform::TlsServerConfiguration{
        std::filesystem::path(certificateChain),
        std::filesystem::path(privateKey)};
  }
  return options;
}

struct ProcessFederationServiceHost::Worker final {
  Worker(
      std::shared_ptr<ProcessTransportConnection> connectionValue,
      std::shared_ptr<ProcessTransportSession> sessionValue,
      bool resumeRequiredValue)
      : connection(std::move(connectionValue)),
        session(std::move(sessionValue)),
        resumeRequired(resumeRequiredValue) {}

  std::shared_ptr<ProcessTransportConnection> connection;
  std::shared_ptr<ProcessTransportSession> session;
  bool resumeRequired = false;
  std::thread thread;
  std::atomic_bool finished{false};
};

struct ProcessFederationServiceHost::SessionRecord final {
  std::shared_ptr<ProcessTransportSession> session;
  bool activeConnection = true;
  std::chrono::steady_clock::time_point inactiveSince{};
};

ProcessFederationServiceHost::ProcessFederationServiceHost(
    EmbeddedFederationRegistry& registry,
    FederationDefinition federationDefinition,
    ProcessTransportAddress listenAddress,
    TransportEndpointIdentity localIdentity,
    ProcessFederationServiceOptions options)
    : ProcessFederationServiceHost(
          std::make_shared<ProcessFederationService>(
              registry,
              std::move(federationDefinition),
              std::move(options)),
          std::move(listenAddress),
          std::move(localIdentity)) {}

ProcessFederationServiceHost::ProcessFederationServiceHost(
    EmbeddedFederationRegistry& registry,
    FederationDefinition federationDefinition,
    ProcessTransportAddress listenAddress,
    TransportEndpointIdentity localIdentity,
    ProcessFederationServiceOptions options,
    ProcessFederationServiceHostOptions hostOptions)
    : ProcessFederationServiceHost(
          std::make_shared<ProcessFederationService>(
              registry,
              std::move(federationDefinition),
              std::move(options)),
          std::move(listenAddress),
          std::move(localIdentity),
          hostOptions) {}

ProcessFederationServiceHost::ProcessFederationServiceHost(
    std::shared_ptr<ProcessFederationService> service,
    ProcessTransportAddress listenAddress,
    TransportEndpointIdentity localIdentity)
    : ProcessFederationServiceHost(
          std::move(service),
          std::move(listenAddress),
          std::move(localIdentity),
          processFederationServiceHostOptionsFromRtiInitializationData()) {}

ProcessFederationServiceHost::ProcessFederationServiceHost(
    std::shared_ptr<ProcessFederationService> service,
    ProcessTransportAddress listenAddress,
    TransportEndpointIdentity localIdentity,
    ProcessFederationServiceHostOptions hostOptions)
    : service_(requireService(std::move(service))),
      listener_(ProcessTransportListener::listen(
          listenAddress,
          tlsConfigurationForListener(listenAddress, hostOptions))),
      localIdentity_(std::move(localIdentity)),
      hostOptions_(std::move(hostOptions)) {
  if (localIdentity_.endpointId.empty()) {
    throw std::invalid_argument(
        "A process federation service host requires an endpoint identity.");
  }
  if (hostOptions_.rtiTimeoutHeart <= std::chrono::milliseconds::zero() ||
      hostOptions_.rtiTimeoutPurge < hostOptions_.rtiTimeoutHeart) {
    throw std::invalid_argument(
        "Process RTI_TIMEOUT_HEART must be positive and RTI_TIMEOUT_PURGE must not be shorter.");
  }
}

ProcessFederationServiceHost::~ProcessFederationServiceHost() {
  stop();
}

void ProcessFederationServiceHost::start() {
  std::scoped_lock lock(workerMutex_);
  if (started_) {
    throw std::logic_error(
        "A process federation service host can only be started once.");
  }
  started_ = true;
  running_.store(true, std::memory_order_release);
  try {
    acceptThread_ = std::thread([this] { acceptLoop(); });
    maintenanceThread_ = std::thread([this] { maintenanceLoop(); });
  } catch (...) {
    running_.store(false, std::memory_order_release);
    listener_->close();
    maintenanceChanged_.notify_all();
    if (acceptThread_.joinable()) {
      acceptThread_.join();
    }
    started_ = false;
    throw;
  }
}

void ProcessFederationServiceHost::stop() noexcept {
  running_.store(false, std::memory_order_release);
  listener_->close();
  maintenanceChanged_.notify_all();

  std::vector<std::shared_ptr<ProcessTransportConnection>> connections;
  {
    std::scoped_lock lock(workerMutex_);
    connections.reserve(workers_.size());
    for (auto const& worker : workers_) {
      connections.push_back(worker->connection);
    }
  }
  for (auto const& connection : connections) {
    connection->close();
  }

  if (acceptThread_.joinable()) {
    acceptThread_.join();
  }
  if (maintenanceThread_.joinable()) {
    maintenanceThread_.join();
  }

  std::vector<std::unique_ptr<Worker>> workers;
  {
    std::scoped_lock lock(workerMutex_);
    workers.swap(workers_);
  }
  for (auto& worker : workers) {
    worker->connection->close();
    if (worker->thread.joinable()) {
      worker->thread.join();
    }
  }

  std::vector<std::shared_ptr<ProcessTransportSession>> sessions;
  {
    std::scoped_lock lock(workerMutex_);
    sessions.reserve(sessions_.size());
    for (auto const& [sessionId, record] : sessions_) {
      static_cast<void>(sessionId);
      sessions.push_back(record->session);
    }
    sessions_.clear();
  }
  for (auto const& session : sessions) {
    try {
      if (!service_->connectionLost(
              *session,
              L"The process federation service host was stopped.")) {
        recordFailure(std::make_exception_ptr(std::runtime_error(
            "The process federation service could not apply host-shutdown loss work.")));
      }
    } catch (...) {
      recordFailure(std::current_exception());
    }
    service_->detach(*session);
  }
}

ProcessTransportAddress ProcessFederationServiceHost::address() const noexcept {
  return listener_->address();
}

bool ProcessFederationServiceHost::running() const noexcept {
  return running_.load(std::memory_order_acquire);
}

void ProcessFederationServiceHost::rethrowIfFailed() const {
  std::exception_ptr failure;
  {
    std::scoped_lock lock(workerMutex_);
    failure = failure_;
  }
  if (failure) {
    std::rethrow_exception(failure);
  }
}

void ProcessFederationServiceHost::acceptLoop() noexcept {
  while (running()) {
    std::shared_ptr<ProcessTransportConnection> connection;
    try {
      connection = listener_->accept(
          this,
          localIdentity_,
          [](std::wstring) {},
          [](std::wstring) { return false; },
          {},
          {},
          [this](std::uint64_t sessionId) {
            if (sessionId == 0U) {
              return std::make_shared<ProcessTransportSessionState>();
            }
            std::scoped_lock lock(workerMutex_);
            auto const record = sessions_.find(sessionId);
            return record == sessions_.end()
                ? std::shared_ptr<ProcessTransportSessionState>{}
                : record->second->session->connection()->sessionState();
          });
    } catch (...) {
      if (running()) {
        recordFailure(std::current_exception());
        running_.store(false, std::memory_order_release);
      }
      return;
    }
    if (!connection) {
      continue;
    }

    if (!running()) {
      connection->close();
      return;
    }

    reapFinishedWorkers();
    std::shared_ptr<ProcessTransportSession> session;
    bool resumeRequired = false;
    bool duplicateActiveConnection = false;
    std::unique_ptr<Worker> worker;
    Worker* workerPointer = nullptr;
    {
      std::unique_lock lock(workerMutex_);
      if (!running()) {
        connection->close();
        return;
      }
      auto existing = sessions_.find(connection->sessionId());
      if (existing != sessions_.end() &&
          existing->second->activeConnection) {
        maintenanceChanged_.wait_for(
            lock,
            std::chrono::milliseconds(100),
            [this, sessionId = connection->sessionId()] {
              auto const current = sessions_.find(sessionId);
              return !running() || current == sessions_.end() ||
                  !current->second->activeConnection;
            });
        existing = sessions_.find(connection->sessionId());
      }
      if (existing == sessions_.end()) {
        session = std::make_shared<ProcessTransportSession>(
            connection,
            hostOptions_.replyToFederateHeartbeats,
            hostOptions_.replyToFederateHeartbeatsForTesting);
        auto record = std::make_shared<SessionRecord>();
        record->session = session;
        sessions_.emplace(connection->sessionId(), std::move(record));
      } else if (existing->second->activeConnection) {
        duplicateActiveConnection = true;
      } else {
        session = existing->second->session;
        session->replaceConnection(connection);
        existing->second->activeConnection = true;
        resumeRequired = true;
      }
      if (duplicateActiveConnection) {
        connection->close();
        continue;
      }
      worker = std::make_unique<Worker>(connection, session, resumeRequired);
      workerPointer = worker.get();
      workers_.push_back(std::move(worker));
    }

    if (resumeRequired && hostOptions_.onSessionResumedForTesting) {
      hostOptions_.onSessionResumedForTesting();
    }

    try {
      workerPointer->thread =
          std::thread([this, workerPointer] { serveConnection(*workerPointer); });
    } catch (...) {
      recordFailure(std::current_exception());
      terminateSession(
          connection->sessionId(),
          L"The process federation service could not start a session worker.");
      workerPointer->finished.store(true, std::memory_order_release);
      workerPointer->connection->close();
      running_.store(false, std::memory_order_release);
      return;
    }
  }
}

void ProcessFederationServiceHost::serveConnection(Worker& worker) noexcept {
  auto const sessionId = worker.connection->sessionId();
  auto& session = *worker.session;
  bool terminalFailure = false;
  try {
    TransportResumeRequest resumeRequest;
    auto const initialFrame = session.receiveInitialFrame(resumeRequest);
    if (initialFrame ==
        ProcessTransportSession::InitialFrame::terminate_session) {
      if (session.sendCloseAcknowledgement()) {
        purgeSession(sessionId);
      } else {
        terminalFailure = true;
      }
    } else if (initialFrame == ProcessTransportSession::InitialFrame::disconnected) {
      markSessionInactive(sessionId);
    } else if (worker.resumeRequired &&
               initialFrame !=
                   ProcessTransportSession::InitialFrame::resume_request) {
      terminalFailure = true;
    } else if (initialFrame ==
               ProcessTransportSession::InitialFrame::resume_request) {
      auto const status = worker.resumeRequired
          ? session.assessResumeRequest(resumeRequest)
          : TransportResumeStatus{
                session.connection()->sessionId(),
                TransportResumeStatusCode::invalid_session,
                session.connection()->messageHistory()
                    .lastReceivedSequenceNumber()};
      if (!session.sendResumeStatus(status)) {
        markSessionInactive(sessionId);
      } else if (status.status != TransportResumeStatusCode::ok) {
        terminalFailure = true;
      } else if (!session.replayOutgoingAfter(
                     resumeRequest.lastReceivedRtiSequenceNumber)) {
        markSessionInactive(sessionId);
      } else {
        auto const handler = service_->handlerFor(session);
        bool terminateSessionReceived = false;
        while (ProcessTransportServiceDispatcher::serveOne(
            session, handler, &terminateSessionReceived)) {
        }
        if (terminateSessionReceived) {
          if (session.sendCloseAcknowledgement()) {
            purgeSession(sessionId);
          } else {
            terminalFailure = true;
          }
        } else {
          markSessionInactive(sessionId);
        }
      }
    } else {
      auto const handler = service_->handlerFor(session);
      bool terminateSessionReceived = false;
      while (ProcessTransportServiceDispatcher::serveOne(
          session, handler, &terminateSessionReceived)) {
      }
      if (terminateSessionReceived) {
        if (session.sendCloseAcknowledgement()) {
          purgeSession(sessionId);
        } else {
          terminalFailure = true;
        }
      } else {
        markSessionInactive(sessionId);
      }
    }
  } catch (...) {
    recordFailure(std::current_exception());
    terminalFailure = true;
  }
  if (terminalFailure) {
    terminateSession(
        sessionId,
        L"The process federation service session could not be resumed.");
  }
  worker.connection->close();
  worker.finished.store(true, std::memory_order_release);
}

void ProcessFederationServiceHost::maintenanceLoop() noexcept {
  auto const pollInterval = std::min(
      std::chrono::duration_cast<std::chrono::milliseconds>(
          hostOptions_.rtiTimeoutHeart),
      std::chrono::milliseconds(50));
  while (running()) {
    std::vector<std::shared_ptr<ProcessTransportConnection>> inactiveConnections;
    std::vector<std::shared_ptr<ProcessTransportSession>> expiredSessions;
    std::chrono::steady_clock::time_point now;
    {
      std::unique_lock lock(workerMutex_);
      maintenanceChanged_.wait_for(lock, pollInterval, [this] {
        return !running();
      });
      if (!running()) {
        return;
      }
      now = std::chrono::steady_clock::now();
      for (auto record = sessions_.begin(); record != sessions_.end();) {
        auto& state = *record->second;
        if (state.activeConnection) {
          auto const lastActivity = state.session->connection()
                                        ->sessionState()
                                        ->lastPeerActivity();
          if (now - lastActivity >= hostOptions_.rtiTimeoutHeart) {
            state.activeConnection = false;
            state.inactiveSince = now;
            inactiveConnections.push_back(state.session->connection());
          }
          ++record;
          continue;
        }
        if (now - state.inactiveSince >= hostOptions_.rtiTimeoutPurge) {
          expiredSessions.push_back(state.session);
          record = sessions_.erase(record);
        } else {
          ++record;
        }
      }
    }
    for (auto const& connection : inactiveConnections) {
      if (hostOptions_.onSessionInactivatedForTesting) {
        try {
          hostOptions_.onSessionInactivatedForTesting();
        } catch (...) {
          recordFailure(std::current_exception());
        }
      }
      connection->close();
    }
    for (auto const& session : expiredSessions) {
      try {
        if (!service_->connectionLost(
                *session,
                L"The inactive process federation session was purged.")) {
          throw std::runtime_error(
              "The process federation service could not apply session-purge work.");
        }
        service_->detach(*session);
      } catch (...) {
        recordFailure(std::current_exception());
      }
    }
  }
}

void ProcessFederationServiceHost::markSessionInactive(
    std::uint64_t sessionId) noexcept {
  {
    std::scoped_lock lock(workerMutex_);
    auto const record = sessions_.find(sessionId);
    if (record == sessions_.end() || !record->second->activeConnection) {
      return;
    }
    record->second->activeConnection = false;
    record->second->inactiveSince = std::chrono::steady_clock::now();
  }
  maintenanceChanged_.notify_all();
}

void ProcessFederationServiceHost::purgeSession(
    std::uint64_t sessionId) noexcept {
  std::shared_ptr<ProcessTransportSession> session;
  {
    std::scoped_lock lock(workerMutex_);
    auto const record = sessions_.find(sessionId);
    if (record == sessions_.end()) {
      return;
    }
    session = record->second->session;
    sessions_.erase(record);
  }
  // Graceful termination is not a connection-loss event: the federate has
  // already resigned and the peer is explicitly closing this RTI session.
  service_->detach(*session);
}

void ProcessFederationServiceHost::terminateSession(
    std::uint64_t sessionId,
    std::wstring faultDescription) noexcept {
  std::shared_ptr<ProcessTransportSession> session;
  {
    std::scoped_lock lock(workerMutex_);
    auto const record = sessions_.find(sessionId);
    if (record == sessions_.end()) {
      return;
    }
    session = record->second->session;
    sessions_.erase(record);
  }
  session->connection()->close();
  try {
    if (!service_->connectionLost(*session, std::move(faultDescription))) {
      throw std::runtime_error(
          "The process federation service could not apply terminal session-loss work.");
    }
    service_->detach(*session);
  } catch (...) {
    recordFailure(std::current_exception());
  }
}

void ProcessFederationServiceHost::reapFinishedWorkers() {
  std::vector<std::unique_ptr<Worker>> finished;
  {
    std::scoped_lock lock(workerMutex_);
    for (auto position = workers_.begin(); position != workers_.end();) {
      if ((*position)->finished.load(std::memory_order_acquire)) {
        finished.push_back(std::move(*position));
        position = workers_.erase(position);
      } else {
        ++position;
      }
    }
  }
  for (auto& worker : finished) {
    if (worker->thread.joinable()) {
      worker->thread.join();
    }
  }
}

void ProcessFederationServiceHost::recordFailure(
    std::exception_ptr failure) noexcept {
  std::scoped_lock lock(workerMutex_);
  if (!failure_) {
    failure_ = std::move(failure);
  }
}

}  // namespace umbra::detail
