#include "internal/embedded_transport.hpp"

#include <utility>

namespace umbra::detail {

EmbeddedTransportConnection::EmbeddedTransportConnection(
    void* owner,
    FailureHandler failureHandler,
    ForcedResignationHandler forcedResignationHandler)
    : owner_(owner),
      failureHandler_(std::move(failureHandler)),
      forcedResignationHandler_(std::move(forcedResignationHandler)) {}

EmbeddedTransportConnection::~EmbeddedTransportConnection() {
  close();
}

void EmbeddedTransportConnection::close() noexcept {
  std::scoped_lock lock(mutex_);
  open_ = false;
  failureHandler_ = {};
  forcedResignationHandler_ = {};
}

void EmbeddedTransportConnection::fail(std::wstring faultDescription) {
  FailureHandler failureHandler;
  {
    std::scoped_lock lock(mutex_);
    if (!open_) {
      return;
    }
    open_ = false;
    failureHandler = std::move(failureHandler_);
    failureHandler_ = {};
    forcedResignationHandler_ = {};
  }

  if (failureHandler) {
    failureHandler(std::move(faultDescription));
  }
}

bool EmbeddedTransportConnection::forceFederateResignation(
    std::wstring reasonForResign) {
  ForcedResignationHandler forcedResignationHandler;
  {
    std::scoped_lock lock(mutex_);
    if (!open_) {
      return false;
    }
    // Do not consume this handler: the same live connection may join a later
    // federation after a successful RTI-initiated resignation.
    forcedResignationHandler = forcedResignationHandler_;
  }

  return forcedResignationHandler
      ? forcedResignationHandler(std::move(reasonForResign))
      : false;
}

bool EmbeddedTransportConnection::open() const noexcept {
  std::scoped_lock lock(mutex_);
  return open_;
}

void* EmbeddedTransportConnection::owner() const noexcept {
  std::scoped_lock lock(mutex_);
  return owner_;
}

std::shared_ptr<EmbeddedTransportConnection> EmbeddedTransportHub::connect(
    void* owner,
    EmbeddedTransportConnection::FailureHandler failureHandler,
    EmbeddedTransportConnection::ForcedResignationHandler forcedResignationHandler) {
  auto connection = std::make_shared<EmbeddedTransportConnection>(
      owner,
      std::move(failureHandler),
      std::move(forcedResignationHandler));
  std::weak_ptr<EmbeddedTransportConnection> previous;
  {
    std::scoped_lock lock(mutex_);
    auto const existing = connections_.find(owner);
    if (existing != connections_.end()) {
      previous = existing->second;
    }
    connections_[owner] = connection;
  }
  if (auto priorConnection = previous.lock()) {
    priorConnection->close();
  }
  return connection;
}

void EmbeddedTransportHub::disconnect(void* owner) noexcept {
  std::shared_ptr<EmbeddedTransportConnection> connection;
  {
    std::scoped_lock lock(mutex_);
    auto const existing = connections_.find(owner);
    if (existing == connections_.end()) {
      return;
    }
    connection = existing->second.lock();
    connections_.erase(existing);
  }
  if (connection) {
    connection->close();
  }
}

bool EmbeddedTransportHub::fail(
    void* owner,
    std::wstring faultDescription) {
  std::shared_ptr<EmbeddedTransportConnection> connection;
  {
    std::scoped_lock lock(mutex_);
    auto const existing = connections_.find(owner);
    if (existing == connections_.end()) {
      return false;
    }
    connection = existing->second.lock();
    if (!connection) {
      connections_.erase(existing);
      return false;
    }
  }
  connection->fail(std::move(faultDescription));
  return true;
}

bool EmbeddedTransportHub::forceFederateResignation(
    void* owner,
    std::wstring reasonForResign) {
  std::shared_ptr<EmbeddedTransportConnection> connection;
  {
    std::scoped_lock lock(mutex_);
    auto const existing = connections_.find(owner);
    if (existing == connections_.end()) {
      return false;
    }
    connection = existing->second.lock();
    if (!connection) {
      connections_.erase(existing);
      return false;
    }
  }
  return connection->forceFederateResignation(std::move(reasonForResign));
}

EmbeddedTransportHub& embeddedTransportHub() noexcept {
  static EmbeddedTransportHub hub;
  return hub;
}

bool failEmbeddedTransportConnectionForTesting(
    rti1516_2025::RTIambassador& owner,
    std::wstring faultDescription) {
  return embeddedTransportHub().fail(&owner, std::move(faultDescription));
}

bool forceEmbeddedFederateResignationForTesting(
    rti1516_2025::RTIambassador& owner,
    std::wstring reasonForResign) {
  return embeddedTransportHub().forceFederateResignation(
      &owner,
      std::move(reasonForResign));
}

}  // namespace umbra::detail
