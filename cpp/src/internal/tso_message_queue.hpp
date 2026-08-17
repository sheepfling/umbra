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
// timestamps. Retraction removes every still-pending recipient entry for that
// id; once any entry has been delivered, the id is no longer retractable.
class TsoMessageQueue final {
 public:
  explicit TsoMessageQueue(std::wstring implementationName = {});

  TsoMessageQueue(TsoMessageQueue const&) = delete;
  TsoMessageQueue& operator=(TsoMessageQueue const&) = delete;
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

  [[nodiscard]] TsoMessageEnqueueResult enqueue(
      std::uint64_t messageId,
      std::uint64_t recipientFederateId,
      std::shared_ptr<rti1516_2025::LogicalTime const> timestamp);

  [[nodiscard]] TsoMessageRetractionResult retract(std::uint64_t messageId);

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
  std::uint64_t nextMessageId_ = 1;
  std::uint64_t nextSequence_ = 1;
  std::vector<PendingEntry> pending_;
  std::vector<std::uint64_t> retractedMessageIds_;
  std::vector<std::uint64_t> deliveredMessageIds_;
};

}  // namespace umbra::detail
