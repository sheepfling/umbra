#pragma once

#include "internal/federate_time_state.hpp"
#include "internal/tso_message_queue.hpp"

#include <cstddef>
#include <cstdint>
#include <map>
#include <memory>
#include <vector>
#include <string>

namespace umbra::detail {

enum class FederationTimeCoordinatorStatus {
  applied,
  federate_already_registered,
  federate_not_registered,
  invalid_time_state,
};

struct FederationTimeCoordinatorResult {
  FederationTimeCoordinatorStatus status = FederationTimeCoordinatorStatus::applied;
};

struct RegisteredFederateTimeSnapshot {
  std::uint64_t federateId = 0;
  FederateTimeSnapshot time;
};

enum class FederationTsoDeliveryStatus {
  applied,
  invalid_recipient,
  invalid_boundary,
  no_messages,
  message_not_in_transit,
  message_already_completed,
};

struct FederationTsoDeliveryResult {
  FederationTsoDeliveryStatus status = FederationTsoDeliveryStatus::no_messages;
  std::vector<TsoQueuedMessage> messages;
};

// The coordinator keeps the three temporal states that affect the 2025 GALT /
// LITS calculation. Pending entries remain in the queue, entries popped for a
// callback are in transit, and completed callbacks remain visible until the
// recipient's next logical-time advance.
struct FederationTsoSnapshot {
  std::vector<TsoQueuedMessage> queued;
  std::vector<TsoQueuedMessage> inTransit;
  std::vector<TsoQueuedMessage> deliveredSinceLastAdvance;
};

// Per-federation ownership boundary for time-management state.  The enclosing
// EmbeddedFederationRegistry holds its mutex whenever it registers, removes,
// or snapshots members, so this class deliberately has no second lock.  Each
// FederateTimeState has its own mutex, allowing a snapshot to remain coherent
// while an individual federate submits or receives a callback-gated request.
//
// This does not schedule cross-federate grants. It establishes the atomic
// input set used by the no-TSO GALT/LITS calculator and later scheduling:
// every runtime-backed member has one selected time implementation plus
// current, pending, role, lookahead, and timestamp-boundary state.
class FederationTimeCoordinator final {
 public:
  explicit FederationTimeCoordinator(std::wstring implementationName = {});

  FederationTimeCoordinator(FederationTimeCoordinator const&) = delete;
  FederationTimeCoordinator& operator=(FederationTimeCoordinator const&) = delete;
  FederationTimeCoordinator(FederationTimeCoordinator&&) noexcept = default;
  FederationTimeCoordinator& operator=(FederationTimeCoordinator&&) noexcept = default;

  [[nodiscard]] std::wstring const& implementationName() const noexcept;

  // Creates the federation-owned implementation fence before a definition is
  // admitted. A different official LogicalTime implementation is rejected.
  [[nodiscard]] bool configureImplementationName(std::wstring implementationName);

  [[nodiscard]] FederationTimeCoordinatorResult registerFederate(
      std::uint64_t federateId,
      std::shared_ptr<FederateTimeState> timeState);

  [[nodiscard]] FederationTimeCoordinatorResult unregisterFederate(
      std::uint64_t federateId) noexcept;

  [[nodiscard]] std::vector<RegisteredFederateTimeSnapshot> snapshot() const;

  [[nodiscard]] std::uint64_t allocateTsoMessageId() noexcept;

  [[nodiscard]] TsoMessageEnqueueResult enqueueTsoMessage(
      std::uint64_t messageId,
      std::uint64_t recipientFederateId,
      std::shared_ptr<rti1516_2025::LogicalTime const> timestamp);

  [[nodiscard]] TsoMessageRetractionResult retractTsoMessage(std::uint64_t messageId);

  // Moves eligible queued messages into an explicit in-transit state. No
  // public callback is invoked here; the later adapter/service slice owns
  // callback ordering and completion.
  [[nodiscard]] FederationTsoDeliveryResult beginTsoDelivery(
      std::uint64_t recipientFederateId,
      rti1516_2025::LogicalTime const& boundary,
      bool inclusive);

  [[nodiscard]] FederationTsoDeliveryStatus completeTsoDelivery(
      TsoQueuedMessage const& message);

  // The delivered-since-last-advance set is cleared at the private grant
  // boundary after the recipient's logical time changes.
  [[nodiscard]] std::size_t clearDeliveredTsoMessages(
      std::uint64_t recipientFederateId) noexcept;

  [[nodiscard]] FederationTsoSnapshot tsoSnapshotFor(
      std::uint64_t recipientFederateId) const;

  // Resign/disconnect cleanup removes all queue and temporal delivery state
  // owned by one recipient without disturbing other fanout recipients.
  [[nodiscard]] std::size_t discardTsoRecipient(
      std::uint64_t recipientFederateId) noexcept;

  [[nodiscard]] bool empty() const noexcept;
  [[nodiscard]] std::size_t size() const noexcept;

 private:
  std::wstring implementationName_;
  TsoMessageQueue tsoQueue_;
  std::map<std::uint64_t, std::shared_ptr<FederateTimeState>> federates_;
  std::map<std::uint64_t, std::vector<TsoQueuedMessage>> inTransitTsoMessages_;
  std::map<std::uint64_t, std::vector<TsoQueuedMessage>>
      deliveredTsoMessagesSinceLastAdvance_;
};

}  // namespace umbra::detail
