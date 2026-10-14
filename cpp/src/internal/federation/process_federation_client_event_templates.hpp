#pragma once

#include "internal/federation/process_federation_client.hpp"

#include <algorithm>
#include <utility>

namespace umbra::detail {

template <typename Event>
Event ProcessFederationClient::receivePendingEvent(bool receiveFromStream) {
  std::scoped_lock lock(transactionMutex_);
  auto const found = std::find_if(
      pendingEvents_.begin(), pendingEvents_.end(),
      [](PendingEvent const& event) { return std::holds_alternative<Event>(event); });
  if (found != pendingEvents_.end()) {
    auto event = std::move(std::get<Event>(*found));
    pendingEvents_.erase(found);
    return event;
  }
  if (receiveFromStream) {
    throw ProcessFederationClientError(
        "The process federation stream reader owns unsolicited event delivery.");
  }
  throw ProcessFederationClientError(
      "Process federation client has no queued event of the requested type.");
}

template <typename Event>
std::size_t ProcessFederationClient::pendingEventCount() const noexcept {
  std::scoped_lock lock(transactionMutex_);
  return static_cast<std::size_t>(std::count_if(
      pendingEvents_.begin(), pendingEvents_.end(),
      [](PendingEvent const& event) { return std::holds_alternative<Event>(event); }));
}

}  // namespace umbra::detail
