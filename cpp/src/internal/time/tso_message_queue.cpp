#include "internal/time/tso_message_queue.hpp"

#include <algorithm>
#include <limits>
#include <set>
#include <tuple>
#include <utility>

namespace umbra::detail {
namespace {

bool contains(std::vector<std::uint64_t> const& values, std::uint64_t value) noexcept {
  return std::find(values.begin(), values.end(), value) != values.end();
}

bool timestampLess(TsoQueuedMessage const& left, TsoQueuedMessage const& right) {
  if (*left.timestamp < *right.timestamp) {
    return true;
  }
  if (*right.timestamp < *left.timestamp) {
    return false;
  }
  return left.sequence < right.sequence;
}

}  // namespace

TsoMessageQueue::TsoMessageQueue(std::wstring implementationName)
    : implementationName_(std::move(implementationName)) {}

std::wstring const& TsoMessageQueue::implementationName() const noexcept {
  return implementationName_;
}

bool TsoMessageQueue::configureImplementationName(std::wstring implementationName) {
  // A queue may be bound exactly once before it admits temporal entries.  In
  // particular, an initially unconfigured queue must not be rebound after an
  // entry was accepted: allowing that path would let a HLAinteger64Time
  // payload sit behind a HLAfloat64Time boundary (or vice versa).
  if (!pending_.empty() ||
      (!implementationName_.empty() && implementationName_ != implementationName) ||
      (admittedImplementationName_ && *admittedImplementationName_ != implementationName)) {
    return false;
  }
  implementationName_ = std::move(implementationName);
  return true;
}

std::uint64_t TsoMessageQueue::allocateMessageId() noexcept {
  if (nextMessageId_ == 0 || nextMessageId_ == std::numeric_limits<std::uint64_t>::max()) {
    return 0;
  }
  return nextMessageId_++;
}

std::uint64_t TsoMessageQueue::messageIdAllocationFloor() const noexcept {
  return nextMessageId_;
}

void TsoMessageQueue::preserveMessageIdAllocationFloor(
    std::uint64_t allocationFloor) noexcept {
  // Zero is not a valid issued designator and is treated as a terminal
  // allocator state if encountered.  UINT64_MAX is also terminal for
  // allocateMessageId(), so the ordinary maximum preserves exhaustion.
  if (nextMessageId_ == 0 || allocationFloor == 0) {
    nextMessageId_ = 0;
    return;
  }
  nextMessageId_ = std::max(nextMessageId_, allocationFloor);
}

TsoMessageEnqueueResult TsoMessageQueue::enqueue(
    std::uint64_t messageId,
    std::uint64_t recipientFederateId,
    std::shared_ptr<rti1516_2025::LogicalTime const> timestamp) {
  if (messageId == 0) {
    return {TsoMessageQueueStatus::invalid_message_id};
  }
  if (recipientFederateId == 0) {
    return {TsoMessageQueueStatus::invalid_recipient};
  }
  if (!timestamp || timestamp->isInitial() || timestamp->isFinal()) {
    return {TsoMessageQueueStatus::invalid_timestamp};
  }
  if (!timestampMatches(timestamp)) {
    return {TsoMessageQueueStatus::logical_time_implementation_mismatch};
  }
  if (contains(retractedMessageIds_, messageId)) {
    return {TsoMessageQueueStatus::message_already_retracted};
  }
  if (contains(deliveredMessageIds_, messageId)) {
    return {TsoMessageQueueStatus::message_already_delivered};
  }
  if (nextSequence_ == 0) {
    return {TsoMessageQueueStatus::invalid_message_id};
  }

  if (implementationName_.empty()) {
    if (!admittedImplementationName_) {
      admittedImplementationName_ = timestamp->implementationName();
    } else if (*admittedImplementationName_ != timestamp->implementationName()) {
      return {TsoMessageQueueStatus::logical_time_implementation_mismatch};
    }
  }

  pending_.push_back({TsoQueuedMessage{
      messageId,
      recipientFederateId,
      nextSequence_++,
      std::move(timestamp),
  }});
  return {TsoMessageQueueStatus::applied};
}

TsoMessageRetractionResult TsoMessageQueue::retract(std::uint64_t messageId) {
  if (messageId == 0) {
    return {TsoMessageQueueStatus::invalid_message_id, 0};
  }
  // A federation-wide message id is no longer retractable once any recipient
  // entry has been delivered. Leave any other recipient entries pending; a
  // later coordinator will reconcile them through its delivery state.
  if (contains(deliveredMessageIds_, messageId)) {
    return {TsoMessageQueueStatus::message_already_delivered, 0};
  }
  if (contains(retractedMessageIds_, messageId)) {
    return {TsoMessageQueueStatus::message_already_retracted, 0};
  }
  auto const originalSize = pending_.size();
  pending_.erase(
      std::remove_if(
          pending_.begin(),
          pending_.end(),
          [messageId](PendingEntry const& entry) {
            return entry.message.messageId == messageId;
          }),
      pending_.end());
  if (pending_.size() != originalSize) {
    retractedMessageIds_.push_back(messageId);
    return {
        TsoMessageQueueStatus::applied,
        originalSize - pending_.size(),
    };
  }
  return {TsoMessageQueueStatus::message_not_found, 0};
}

TsoMessageRetractionResult TsoMessageQueue::retractPending(std::uint64_t messageId) {
  if (messageId == 0) {
    return {TsoMessageQueueStatus::invalid_message_id, 0};
  }
  if (contains(retractedMessageIds_, messageId)) {
    return {TsoMessageQueueStatus::message_already_retracted, 0};
  }

  auto const originalSize = pending_.size();
  pending_.erase(
      std::remove_if(
          pending_.begin(),
          pending_.end(),
          [messageId](PendingEntry const& entry) {
            return entry.message.messageId == messageId;
          }),
      pending_.end());
  auto const removedCount = originalSize - pending_.size();

  // A message can already have left the pending queue for one recipient while
  // another recipient is still queued (or while the callback is in transit).
  // The caller owns the per-recipient delivery ledger and will issue Request
  // Retraction where required; this queue must still suppress every remaining
  // pending entry.
  if (removedCount != 0 || contains(deliveredMessageIds_, messageId)) {
    retractedMessageIds_.push_back(messageId);
    return {TsoMessageQueueStatus::applied, removedCount};
  }
  return {TsoMessageQueueStatus::message_not_found, 0};
}

TsoMessageQueueRestoreResult TsoMessageQueue::restoreEntries(
    std::vector<TsoQueueRestoreEntry> const& entries) {
  std::set<std::tuple<std::uint64_t, std::uint64_t, std::uint64_t>> identities;
  std::set<std::uint64_t> sequences;
  std::uint64_t maximumMessageId = 0;
  std::uint64_t maximumSequence = 0;
  std::optional<std::wstring> admittedImplementation = admittedImplementationName_;

  for (auto const& entry : entries) {
    auto const& message = entry.message;
    if (message.messageId == 0 || message.recipientFederateId == 0 ||
        message.sequence == 0 || !message.timestamp ||
        message.timestamp->isInitial() || message.timestamp->isFinal()) {
      return {TsoMessageQueueRestoreStatus::invalid_entry, 0};
    }
    if (static_cast<std::uint32_t>(entry.phase) >
        static_cast<std::uint32_t>(TsoMessageQueuePhase::delivered)) {
      return {TsoMessageQueueRestoreStatus::invalid_phase, 0};
    }
    if (!timestampMatches(message.timestamp)) {
      return {
          TsoMessageQueueRestoreStatus::logical_time_implementation_mismatch,
          0};
    }
    if (!identities.emplace(
             message.messageId,
             message.recipientFederateId,
             message.sequence)
             .second ||
        !sequences.insert(message.sequence).second) {
      return {TsoMessageQueueRestoreStatus::duplicate_entry, 0};
    }
    if (admittedImplementation.has_value() &&
        *admittedImplementation != message.timestamp->implementationName()) {
      return {
          TsoMessageQueueRestoreStatus::logical_time_implementation_mismatch,
          0};
    }
    if (!admittedImplementation.has_value()) {
      admittedImplementation = message.timestamp->implementationName();
    }
    maximumMessageId = std::max(maximumMessageId, message.messageId);
    maximumSequence = std::max(maximumSequence, message.sequence);
  }

  auto pending = std::vector<PendingEntry>{};
  pending.reserve(entries.size());
  auto delivered = std::vector<std::uint64_t>{};
  delivered.reserve(entries.size());
  for (auto const& entry : entries) {
    if (entry.phase == TsoMessageQueuePhase::queued) {
      pending.push_back({entry.message});
    } else if (std::find(delivered.begin(), delivered.end(), entry.message.messageId) ==
               delivered.end()) {
      delivered.push_back(entry.message.messageId);
    }
  }

  // Keep designators issued after the saved boundary from being reused.  The
  // queue image carries active IDs, so also advance beyond its largest one.
  auto const messageIdAllocationFloor = nextMessageId_;
  std::uint64_t restoredMessageIdFloor = messageIdAllocationFloor;
  if (maximumMessageId == std::numeric_limits<std::uint64_t>::max()) {
    restoredMessageIdFloor = 0;
  } else if (maximumMessageId != 0) {
    auto const imageFloor = maximumMessageId + 1U;
    if (restoredMessageIdFloor != 0) {
      restoredMessageIdFloor = std::max(restoredMessageIdFloor, imageFloor);
    }
  }

  pending_ = std::move(pending);
  deliveredMessageIds_ = std::move(delivered);
  admittedImplementationName_ = std::move(admittedImplementation);
  if (maximumSequence == std::numeric_limits<std::uint64_t>::max()) {
    nextSequence_ = 0;
  } else if (maximumSequence != 0) {
    nextSequence_ = maximumSequence + 1U;
  }
  nextMessageId_ = restoredMessageIdFloor;
  return {TsoMessageQueueRestoreStatus::applied, entries.size()};
}

std::vector<TsoQueuedMessage> TsoMessageQueue::popEligible(
    std::uint64_t recipientFederateId,
    rti1516_2025::LogicalTime const& boundary,
    bool inclusive) {
  std::vector<TsoQueuedMessage> eligible;
  if (recipientFederateId == 0 ||
      (!implementationName_.empty() && boundary.implementationName() != implementationName_)) {
    return eligible;
  }

  std::stable_sort(
      pending_.begin(),
      pending_.end(),
      [](PendingEntry const& left, PendingEntry const& right) {
        return timestampLess(left.message, right.message);
      });

  std::vector<PendingEntry> retained;
  retained.reserve(pending_.size());
  for (auto& entry : pending_) {
    bool const beforeBoundary = *entry.message.timestamp < boundary;
    bool const atBoundary = *entry.message.timestamp == boundary;
    if (entry.message.recipientFederateId == recipientFederateId &&
        (beforeBoundary || (inclusive && atBoundary))) {
      deliveredMessageIds_.push_back(entry.message.messageId);
      eligible.push_back(std::move(entry.message));
    } else {
      retained.push_back(std::move(entry));
    }
  }
  pending_ = std::move(retained);
  return eligible;
}

std::optional<std::shared_ptr<rti1516_2025::LogicalTime const>>
TsoMessageQueue::earliestTimestampFor(std::uint64_t recipientFederateId) const {
  if (recipientFederateId == 0) {
    return std::nullopt;
  }

  std::shared_ptr<rti1516_2025::LogicalTime const> earliest;
  for (auto const& entry : pending_) {
    if (entry.message.recipientFederateId != recipientFederateId) {
      continue;
    }
    if (!earliest || *entry.message.timestamp < *earliest) {
      earliest = entry.message.timestamp;
    }
  }
  if (!earliest) {
    return std::nullopt;
  }
  return earliest;
}

std::vector<TsoQueuedMessage> TsoMessageQueue::pendingMessages() const {
  std::vector<TsoQueuedMessage> result;
  result.reserve(pending_.size());
  for (auto const& entry : pending_) {
    result.push_back(entry.message);
  }
  std::stable_sort(result.begin(), result.end(), timestampLess);
  return result;
}

std::size_t TsoMessageQueue::discardRecipient(std::uint64_t recipientFederateId) noexcept {
  if (recipientFederateId == 0) {
    return 0;
  }
  auto const originalSize = pending_.size();
  pending_.erase(
      std::remove_if(
          pending_.begin(),
          pending_.end(),
          [recipientFederateId](PendingEntry const& entry) {
            return entry.message.recipientFederateId == recipientFederateId;
          }),
      pending_.end());
  return originalSize - pending_.size();
}

std::size_t TsoMessageQueue::pendingCount() const noexcept {
  return pending_.size();
}

std::size_t TsoMessageQueue::pendingCountFor(std::uint64_t recipientFederateId) const noexcept {
  return static_cast<std::size_t>(std::count_if(
      pending_.begin(),
      pending_.end(),
      [recipientFederateId](PendingEntry const& entry) {
        return entry.message.recipientFederateId == recipientFederateId;
      }));
}

bool TsoMessageQueue::timestampMatches(
    std::shared_ptr<rti1516_2025::LogicalTime const> const& timestamp) const noexcept {
  return timestamp &&
      (implementationName_.empty() || timestamp->implementationName() == implementationName_);
}

}  // namespace umbra::detail
