#pragma once

#include "internal/time/federate_time_state.hpp"
#include "internal/time/tso_message_queue.hpp"

#include <cstddef>
#include <cstdint>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <vector>

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

enum class FederationTsoQueueRestoreStatus {
  applied,
  invalid_recipient,
  invalid_entry,
};

struct FederationTsoQueueRestoreResult {
  FederationTsoQueueRestoreStatus status =
      FederationTsoQueueRestoreStatus::invalid_entry;
  std::size_t restoredCount = 0;
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

  FederationTimeCoordinator(FederationTimeCoordinator const& other);
  FederationTimeCoordinator& operator=(FederationTimeCoordinator const& other);
  FederationTimeCoordinator(FederationTimeCoordinator&&) noexcept = default;
  FederationTimeCoordinator& operator=(FederationTimeCoordinator&&) noexcept = default;

  [[nodiscard]] std::wstring const& implementationName() const noexcept;

  // Creates the federation-owned implementation fence before a definition is
  // admitted. A different official LogicalTime implementation is rejected.
  [[nodiscard]] bool configureImplementationName(std::wstring implementationName);

  [[nodiscard]] FederationTimeCoordinatorResult registerFederate(
      std::uint64_t federateId,
      std::shared_ptr<FederateTimeState> timeState);

  // Every joined member is a valid TSO recipient, even when the member has
  // not enabled any time-management role. Keep that membership fence
  // separate from federates_, which contains only members with a live
  // FederateTimeState used by GALT/LITS calculations.
  [[nodiscard]] FederationTimeCoordinatorResult registerRecipient(
      std::uint64_t federateId);

  [[nodiscard]] FederationTimeCoordinatorResult unregisterFederate(
      std::uint64_t federateId) noexcept;

  [[nodiscard]] std::vector<RegisteredFederateTimeSnapshot> snapshot() const;

  // Returns the live state object for callback gating.  The registry owns the
  // coordinator lock boundary; callers must not invoke federate callbacks
  // while holding that boundary.
  [[nodiscard]] std::shared_ptr<FederateTimeState> timeStateFor(
      std::uint64_t federateId) const;

  // Restore a previously captured coordinator while preserving the live
  // FederateTimeState objects held by joined RTI ambassadors.  Replacing those
  // shared pointers would leave the public ambassadors observing stale time
  // state after a restore.
  void restoreFrom(FederationTimeCoordinator const& source);

  [[nodiscard]] std::uint64_t allocateTsoMessageId() noexcept;

  [[nodiscard]] TsoMessageEnqueueResult enqueueTsoMessage(
      std::uint64_t messageId,
      std::uint64_t recipientFederateId,
      std::shared_ptr<rti1516_2025::LogicalTime const> timestamp);

  [[nodiscard]] TsoMessageRetractionResult retractTsoMessage(std::uint64_t messageId);

  // Used only by a message family with a federation-owned delivery ledger.
  // It withdraws remaining queued fanout after another recipient has reached
  // a delivery boundary; the ledger decides whether a Request Retraction
  // callback is due for that recipient.
  [[nodiscard]] TsoMessageRetractionResult retractPendingTsoMessage(
      std::uint64_t messageId);

  // Returns the earliest currently queued TSO timestamp for one recipient.
  // The value is immutable and remains owned by the queue until delivery or
  // retraction; the caller uses it only to select an NMR grant target.
  [[nodiscard]] std::optional<std::shared_ptr<rti1516_2025::LogicalTime const>>
  earliestTsoTimestampFor(std::uint64_t recipientFederateId) const;

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

  // Rebuilds the queue phase state from a route-free save image.  The
  // coordinator keeps in-transit and delivered vectors; the underlying queue
  // retains pending entries and its delivered-designator fence.
  [[nodiscard]] FederationTsoQueueRestoreResult restoreTsoQueue(
      std::vector<TsoQueueRestoreEntry> const& entries);

  // Resign/disconnect cleanup removes all queue and temporal delivery state
  // owned by one recipient without disturbing other fanout recipients.
  [[nodiscard]] std::size_t discardTsoRecipient(
      std::uint64_t recipientFederateId) noexcept;

  [[nodiscard]] bool empty() const noexcept;
  [[nodiscard]] std::size_t size() const noexcept;

 private:
  std::wstring implementationName_;
  TsoMessageQueue tsoQueue_;
  std::set<std::uint64_t> recipients_;
  std::map<std::uint64_t, std::shared_ptr<FederateTimeState>> federates_;
  std::map<std::uint64_t, std::vector<TsoQueuedMessage>> inTransitTsoMessages_;
  std::map<std::uint64_t, std::vector<TsoQueuedMessage>>
      deliveredTsoMessagesSinceLastAdvance_;
};

}  // namespace umbra::detail
