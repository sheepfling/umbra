#pragma once

#include "internal/federation/process_federation_client.hpp"

#include <algorithm>
#include <utility>

namespace umbra::detail {

template <typename Event>
Event ProcessFederationClient::receivePendingEvent(bool receiveFromStream) {
  std::unique_lock lock(transactionMutex_);
  while (true) {
    auto found = std::find_if(pendingEvents_.begin(), pendingEvents_.end(),
        [](PendingEvent const& event) { return std::holds_alternative<Event>(event); });
    if (found != pendingEvents_.end()) {
      auto event = std::move(std::get<Event>(*found));
      pendingEvents_.erase(found);
      return event;
    }
    if (!receiveFromStream) {
      throw ProcessFederationClientError(
          "Process federation client has no queued event of the requested type.");
    }
    if (!session_) {
      throw ProcessFederationClientError(
          "A process federation event was requested after client close.");
    }
    TransportServiceMessage message;
    ++requestDepth_;
    bool received = false;
    try {
      received = session_->receive(message);
    } catch (...) {
      --requestDepth_;
      lock.unlock();
      dispatchTransportFailures();
      throw;
    }
    --requestDepth_;
    if (!received) {
      lock.unlock();
      dispatchTransportFailures();
      throw ProcessFederationClientError(
          "Process federation client lost its pushed event.");
    }
    bufferEvent(std::move(message));
  }
}

template <typename Event>
std::size_t ProcessFederationClient::pendingEventCount() const noexcept {
  std::scoped_lock lock(transactionMutex_);
  return static_cast<std::size_t>(std::count_if(
      pendingEvents_.begin(), pendingEvents_.end(),
      [](PendingEvent const& event) { return std::holds_alternative<Event>(event); }));
}

}  // namespace umbra::detail
