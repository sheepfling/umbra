#pragma once

#include <RTI/time/LogicalTime.h>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace umbra::detail {

// Private kernel state for the next IEEE 1516.1-2025 time-management slice.
// The queue owns only immutable, official LogicalTime values and federation
// identities. It does not expose a public RTI service, invoke callbacks, or
// manufacture a replacement time representation. The enclosing federation
// coordinator is responsible for serialization while it owns this object.
enum class TsoMessageQueueStatus {
  applied,
  invalid_message_id,
  invalid_recipient,
  invalid_timestamp,
  logical_time_implementation_mismatch,
  message_not_found,
  message_already_retracted,
  message_already_delivered,
};

struct TsoQueuedMessage {
  std::uint64_t messageId = 0;
  std::uint64_t recipientFederateId = 0;
  std::uint64_t sequence = 0;
  std::shared_ptr<rti1516_2025::LogicalTime const> timestamp;
};

// A route-free save image records the queue phase separately from the
// immutable message identity.  In-transit and delivered entries have already
// crossed the queue's pending boundary and therefore remain in its delivered
// designator fence after rehydration.
enum class TsoMessageQueuePhase : std::uint32_t {
  queued = 0,
  in_transit = 1,
  delivered = 2,
};

struct TsoQueueRestoreEntry final {
  TsoQueuedMessage message;
  TsoMessageQueuePhase phase = TsoMessageQueuePhase::queued;
};

enum class TsoMessageQueueRestoreStatus {
  applied,
  invalid_entry,
  invalid_phase,
  duplicate_entry,
  logical_time_implementation_mismatch,
};

struct TsoMessageQueueRestoreResult {
  TsoMessageQueueRestoreStatus status =
      TsoMessageQueueRestoreStatus::invalid_entry;
  std::size_t restoredCount = 0;
};

struct TsoMessageEnqueueResult {
  TsoMessageQueueStatus status = TsoMessageQueueStatus::invalid_message_id;
};

struct TsoMessageRetractionResult {
  TsoMessageQueueStatus status = TsoMessageQueueStatus::message_not_found;
  std::size_t removedCount = 0;
};

// A deterministic, recipient-scoped queue for future TSO integration. A
// message id is shared by all recipient entries created for one federation
// service invocation, while sequence preserves stable order for equal
// timestamps. The ordinary retract operation preserves the legacy terminal
// boundary used by message families that do not yet implement Request
// Retraction.  A separately named pending-fanout operation is available to a
// federation-owned service ledger that does implement the delivered-recipient
// callback rule: it removes every still-pending recipient entry even after a
// different recipient has reached a delivery boundary.
class TsoMessageQueue final {
 public:
  explicit TsoMessageQueue(std::wstring implementationName = {});

  TsoMessageQueue(TsoMessageQueue const&) = default;
  TsoMessageQueue& operator=(TsoMessageQueue const&) = default;
  TsoMessageQueue(TsoMessageQueue&&) noexcept = default;
  TsoMessageQueue& operator=(TsoMessageQueue&&) noexcept = default;

  [[nodiscard]] std::wstring const& implementationName() const noexcept;

  // Binds an initially unconfigured queue to the federation's selected
  // official logical-time implementation. A different implementation can
  // never be substituted, and configuration is only allowed before pending
  // entries exist.
  [[nodiscard]] bool configureImplementationName(std::wstring implementationName);

  // Allocates an execution-local id. Zero is reserved as an invalid id.
  [[nodiscard]] std::uint64_t allocateMessageId() noexcept;

  // The next allocation value is an execution-wide high-water mark, rather
  // than restorable message payload.  A save/restore must never reuse a
  // designator that was returned after the saved image was captured: callers
  // can retain a MessageRetractionHandle outside the saved federation image.
  [[nodiscard]] std::uint64_t messageIdAllocationFloor() const noexcept;
  void preserveMessageIdAllocationFloor(std::uint64_t allocationFloor) noexcept;

  [[nodiscard]] TsoMessageEnqueueResult enqueue(
      std::uint64_t messageId,
      std::uint64_t recipientFederateId,
      std::shared_ptr<rti1516_2025::LogicalTime const> timestamp);

  [[nodiscard]] TsoMessageRetractionResult retract(std::uint64_t messageId);

  // Removes every still-pending fanout entry and records a terminal
  // retraction even if another recipient has already been popped for
  // delivery.  This lower-level operation deliberately does not decide which
  // delivered recipients require Request Retraction; the owning federation
  // service ledger makes that decision atomically with this queue mutation.
  [[nodiscard]] TsoMessageRetractionResult retractPending(std::uint64_t messageId);

  // Replaces the active queue view from a route-free save image while
  // preserving the current allocator high-water mark.  The caller owns the
  // per-recipient in-transit/delivered vectors; this queue records pending
  // entries and the delivered-designator fence atomically after validation.
  [[nodiscard]] TsoMessageQueueRestoreResult restoreEntries(
      std::vector<TsoQueueRestoreEntry> const& entries);

  // Removes and marks as delivered all entries for one recipient whose time is
  // before the boundary, or equal to it when inclusive is true. Returned
  // entries remain immutable and are ready for a later callback coordinator.
  [[nodiscard]] std::vector<TsoQueuedMessage> popEligible(
      std::uint64_t recipientFederateId,
      rti1516_2025::LogicalTime const& boundary,
      bool inclusive);

  [[nodiscard]] std::optional<std::shared_ptr<rti1516_2025::LogicalTime const>>
  earliestTimestampFor(std::uint64_t recipientFederateId) const;

  // Returns a stable timestamp/sequence ordering without mutating delivery
  // state. The coordinator uses this immutable view when building its
  // federation-owned temporal snapshot.
  [[nodiscard]] std::vector<TsoQueuedMessage> pendingMessages() const;

  // Resignation/disconnect cleanup removes only pending entries for the
  // recipient. Other fanout recipients and their message-id terminal state
  // remain untouched.
  [[nodiscard]] std::size_t discardRecipient(std::uint64_t recipientFederateId) noexcept;

  [[nodiscard]] std::size_t pendingCount() const noexcept;
  [[nodiscard]] std::size_t pendingCountFor(std::uint64_t recipientFederateId) const noexcept;

 private:
  struct PendingEntry {
    TsoQueuedMessage message;
  };

  [[nodiscard]] bool timestampMatches(
      std::shared_ptr<rti1516_2025::LogicalTime const> const& timestamp) const noexcept;

  std::wstring implementationName_;
  // An initially unconfigured queue learns the implementation from its first
  // admitted timestamp.  Keep that identity even after the entry is popped:
  // otherwise a drained queue could be rebound to a different LogicalTime
  // family while copied temporal state still refers to the original one.
  std::optional<std::wstring> admittedImplementationName_;
  std::uint64_t nextMessageId_ = 1;
  std::uint64_t nextSequence_ = 1;
  std::vector<PendingEntry> pending_;
  std::vector<std::uint64_t> retractedMessageIds_;
  std::vector<std::uint64_t> deliveredMessageIds_;
};

}  // namespace umbra::detail
