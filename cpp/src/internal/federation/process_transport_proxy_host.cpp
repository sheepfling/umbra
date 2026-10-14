#include "internal/federation/process_transport_proxy_host.hpp"

#include <stdexcept>
#include <utility>

namespace umbra::detail {

struct ProcessTransportProxyHost::Worker final {
  explicit Worker(std::shared_ptr<ProcessTransportConnection> downstreamValue)
      : downstream(std::move(downstreamValue)) {}

  void closeConnections() noexcept {
    downstream->close();
    if (upstream) {
      upstream->close();
    }
  }

  std::shared_ptr<ProcessTransportConnection> downstream;
  std::shared_ptr<ProcessTransportConnection> upstream;
  std::thread clientToAuthority;
  std::thread authorityToClient;
  std::atomic_uint32_t remainingRelays{2U};
  std::atomic_bool finished{false};
};

ProcessTransportProxyHost::ProcessTransportProxyHost(
    ProcessTransportAddress listenAddress,
    ProcessTransportAddress upstreamAddress,
    TransportEndpointIdentity proxyIdentity)
    : listener_(ProcessTransportListener::listen(std::move(listenAddress))),
      upstreamAddress_(std::move(upstreamAddress)),
      proxyIdentity_(std::move(proxyIdentity)) {
  if (proxyIdentity_.endpointId.empty()) {
    throw std::invalid_argument(
        "A process transport proxy requires an endpoint identity.");
  }
}

ProcessTransportProxyHost::~ProcessTransportProxyHost() {
  stop();
}

void ProcessTransportProxyHost::start() {
  std::scoped_lock lock(workerMutex_);
  if (started_) {
    throw std::logic_error(
        "A process transport proxy can only be started once.");
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

void ProcessTransportProxyHost::stop() noexcept {
  running_.store(false, std::memory_order_release);
  listener_->close();

  {
    std::scoped_lock lock(workerMutex_);
    for (auto const& worker : workers_) {
      worker->closeConnections();
    }
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
    worker->closeConnections();
    if (worker->clientToAuthority.joinable()) {
      worker->clientToAuthority.join();
    }
    if (worker->authorityToClient.joinable()) {
      worker->authorityToClient.join();
    }
  }
}

ProcessTransportAddress ProcessTransportProxyHost::address() const noexcept {
  return listener_->address();
}

bool ProcessTransportProxyHost::running() const noexcept {
  return running_.load(std::memory_order_acquire);
}

void ProcessTransportProxyHost::rethrowIfFailed() const {
  std::exception_ptr failure;
  {
    std::scoped_lock lock(workerMutex_);
    failure = failure_;
  }
  if (failure) {
    std::rethrow_exception(failure);
  }
}

void ProcessTransportProxyHost::acceptLoop() noexcept {
  while (running()) {
    reapFinishedWorkers();

    std::shared_ptr<ProcessTransportConnection> downstream;
    try {
      downstream = listener_->accept(
          this,
          proxyIdentity_,
          [](std::wstring) {},
          [](std::wstring) { return false; });
    } catch (...) {
      if (running()) {
        recordFailure(std::current_exception());
        running_.store(false, std::memory_order_release);
      }
      return;
    }

    auto worker = std::make_unique<Worker>(downstream);
    auto* workerPointer = worker.get();
    {
      std::scoped_lock lock(workerMutex_);
      if (!running()) {
        downstream->close();
        return;
      }
      workers_.push_back(std::move(worker));
    }

    std::shared_ptr<ProcessTransportConnection> upstream;
    try {
      upstream = ProcessTransportConnection::connectClient(
          workerPointer,
          upstreamAddress_,
          downstream->peerIdentity(),
          [](std::wstring) {},
          [](std::wstring) { return false; });
    } catch (...) {
      downstream->close();
      workerPointer->finished.store(true, std::memory_order_release);
      continue;
    }

    {
      std::scoped_lock lock(workerMutex_);
      workerPointer->upstream = upstream;
      if (!running()) {
        workerPointer->closeConnections();
        workerPointer->finished.store(true, std::memory_order_release);
        continue;
      }
    }

    try {
      workerPointer->clientToAuthority = std::thread([this, workerPointer] {
        relayFrames(
            *workerPointer,
            workerPointer->downstream,
            workerPointer->upstream);
      });
      workerPointer->authorityToClient = std::thread([this, workerPointer] {
        relayFrames(
            *workerPointer,
            workerPointer->upstream,
            workerPointer->downstream);
      });
    } catch (...) {
      recordFailure(std::current_exception());
      running_.store(false, std::memory_order_release);
      workerPointer->closeConnections();
      if (workerPointer->clientToAuthority.joinable()) {
        workerPointer->clientToAuthority.join();
      }
      workerPointer->finished.store(true, std::memory_order_release);
      return;
    }
  }
}

void ProcessTransportProxyHost::relayFrames(
    Worker& worker,
    std::shared_ptr<ProcessTransportConnection> const& source,
    std::shared_ptr<ProcessTransportConnection> const& destination) noexcept {
  for (;;) {
    TransportFrame frame;
    if (!source->receiveFrame(frame) || !destination->sendFrame(frame)) {
      break;
    }
  }

  worker.closeConnections();
  if (worker.remainingRelays.fetch_sub(1U, std::memory_order_acq_rel) == 1U) {
    worker.finished.store(true, std::memory_order_release);
  }
}

void ProcessTransportProxyHost::reapFinishedWorkers() {
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
    if (worker->clientToAuthority.joinable()) {
      worker->clientToAuthority.join();
    }
    if (worker->authorityToClient.joinable()) {
      worker->authorityToClient.join();
    }
  }
}

void ProcessTransportProxyHost::recordFailure(
    std::exception_ptr failure) noexcept {
  std::scoped_lock lock(workerMutex_);
  if (!failure_) {
    failure_ = std::move(failure);
  }
}

}  // namespace umbra::detail
