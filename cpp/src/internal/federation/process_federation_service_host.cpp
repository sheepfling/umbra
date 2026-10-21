#include "internal/federation/process_federation_service_host.hpp"

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

}  // namespace

struct ProcessFederationServiceHost::Worker final {
  explicit Worker(std::shared_ptr<ProcessTransportConnection> connectionValue)
      : connection(std::move(connectionValue)) {}

  std::shared_ptr<ProcessTransportConnection> connection;
  std::thread thread;
  std::atomic_bool finished{false};
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
    std::shared_ptr<ProcessFederationService> service,
    ProcessTransportAddress listenAddress,
    TransportEndpointIdentity localIdentity)
    : service_(requireService(std::move(service))),
      listener_(ProcessTransportListener::listen(std::move(listenAddress))),
      localIdentity_(std::move(localIdentity)) {
  if (localIdentity_.endpointId.empty()) {
    throw std::invalid_argument(
        "A process federation service host requires an endpoint identity.");
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
  } catch (...) {
    running_.store(false, std::memory_order_release);
    started_ = false;
    throw;
  }
}

void ProcessFederationServiceHost::stop() noexcept {
  running_.store(false, std::memory_order_release);
  listener_->close();

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
          [](std::wstring) { return false; });
    } catch (...) {
      if (running()) {
        recordFailure(std::current_exception());
        running_.store(false, std::memory_order_release);
      }
      return;
    }

    if (!running()) {
      connection->close();
      return;
    }

    reapFinishedWorkers();
    auto worker = std::make_unique<Worker>(connection);
    auto* workerPointer = worker.get();
    {
      std::scoped_lock lock(workerMutex_);
      if (!running()) {
        connection->close();
        return;
      }
      workers_.push_back(std::move(worker));
    }

    try {
      workerPointer->thread =
          std::thread([this, workerPointer] { serveConnection(*workerPointer); });
    } catch (...) {
      recordFailure(std::current_exception());
      workerPointer->finished.store(true, std::memory_order_release);
      workerPointer->connection->close();
      running_.store(false, std::memory_order_release);
      return;
    }
  }
}

void ProcessFederationServiceHost::serveConnection(Worker& worker) noexcept {
  std::optional<ProcessTransportSession> session;
  try {
    session.emplace(worker.connection);
    auto const handler = service_->handlerFor(*session);
    while (ProcessTransportServiceDispatcher::serveOne(*session, handler)) {
    }
    if (!service_->connectionLost(
            *session, L"The process federation service connection was lost.")) {
      throw std::runtime_error(
          "The process federation service could not apply connection-loss work.");
    }
  } catch (...) {
    recordFailure(std::current_exception());
    if (session) {
      try {
        static_cast<void>(service_->connectionLost(
            *session, L"The process federation service connection was lost."));
      } catch (...) {
        recordFailure(std::current_exception());
      }
    }
  }
  if (session) {
    service_->detach(*session);
  }
  worker.connection->close();
  worker.finished.store(true, std::memory_order_release);
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
