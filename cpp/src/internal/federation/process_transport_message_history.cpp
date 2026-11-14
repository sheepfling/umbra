#include "internal/federation/process_transport_message_history.hpp"

#include <algorithm>
#include <stdexcept>

namespace umbra::detail {

ProcessTransportMessageHistory::ProcessTransportMessageHistory(
    std::size_t maximumBytes)
    : maximumBytes_(maximumBytes) {
  if (maximumBytes_ == 0U) {
    throw std::invalid_argument(
        "A process transport message history requires a nonzero byte limit.");
  }
}

std::uint32_t ProcessTransportMessageHistory::retainOutgoing(
    std::uint32_t sequenceNumber,
    std::span<std::uint8_t const> payload,
    TransportFrameKind frameKind) {
  if (sequenceNumber > kMaximumTransportSequenceNumber) {
    throw TransportProtocolError(
        "An outgoing process transport service message has an invalid sequence number.");
  }

  std::scoped_lock lock(mutex_);
  if (lastOutgoingSequenceNumber_ &&
      !sequenceAfter(sequenceNumber, *lastOutgoingSequenceNumber_)) {
    throw TransportProtocolError(
        "Outgoing process transport service-message sequence numbers must increase.");
  }
  lastOutgoingSequenceNumber_ = sequenceNumber;
  if (payload.size() > maximumBytes_) {
    lastDiscardedUnacknowledgedSequenceNumber_ = sequenceNumber;
    return sequenceNumber;
  }

  while (!outgoing_.empty() &&
         retainedBytes_ > maximumBytes_ - payload.size()) {
    lastDiscardedUnacknowledgedSequenceNumber_ =
        outgoing_.front().sequenceNumber;
    retainedBytes_ -= outgoing_.front().payload.size();
    outgoing_.pop_front();
  }
  TransportMessageHistoryEntry entry;
  entry.sequenceNumber = sequenceNumber;
  entry.payload.assign(payload.begin(), payload.end());
  entry.frameKind = frameKind;
  retainedBytes_ += entry.payload.size();
  outgoing_.push_back(std::move(entry));
  return sequenceNumber;
}

void ProcessTransportMessageHistory::acknowledgeOutgoing(
    std::uint32_t lastReceivedMessageNumber) {
  if (lastReceivedMessageNumber == kInvalidTransportSequenceNumber) {
    return;
  }
  if (lastReceivedMessageNumber > kMaximumTransportSequenceNumber) {
    throw TransportProtocolError(
        "The peer acknowledged an invalid process transport sequence number.");
  }

  std::scoped_lock lock(mutex_);
  if (!lastOutgoingSequenceNumber_ ||
      sequenceAfter(lastReceivedMessageNumber, *lastOutgoingSequenceNumber_)) {
    throw TransportProtocolError(
        "The peer acknowledged a process transport message that was not sent.");
  }
  if (lastAcknowledgedSequenceNumber_ &&
      !sequenceAfter(
          lastReceivedMessageNumber, *lastAcknowledgedSequenceNumber_)) {
    return;
  }
  lastAcknowledgedSequenceNumber_ = lastReceivedMessageNumber;
  while (!outgoing_.empty() && sequenceAtOrBefore(
                                   outgoing_.front().sequenceNumber,
                                   lastReceivedMessageNumber)) {
    retainedBytes_ -= outgoing_.front().payload.size();
    outgoing_.pop_front();
  }
}

bool ProcessTransportMessageHistory::acceptIncoming(
    std::uint32_t sequenceNumber) {
  if (sequenceNumber > kMaximumTransportSequenceNumber) {
    throw TransportProtocolError(
        "The received process transport sequence number is invalid.");
  }

  std::scoped_lock lock(mutex_);
  if (lastReceivedSequenceNumber_ == kInvalidTransportSequenceNumber ||
      sequenceAfter(sequenceNumber, lastReceivedSequenceNumber_)) {
    lastReceivedSequenceNumber_ = sequenceNumber;
    return true;
  }
  return false;
}

std::optional<std::vector<TransportMessageHistoryEntry>>
ProcessTransportMessageHistory::replayAfter(
    std::uint32_t peerLastReceived) const {
  std::scoped_lock lock(mutex_);
  if (!hasCompleteSuffixAfterLocked(peerLastReceived)) {
    return std::nullopt;
  }
  if (!lastOutgoingSequenceNumber_ ||
      peerLastReceived == *lastOutgoingSequenceNumber_) {
    return std::vector<TransportMessageHistoryEntry>{};
  }

  std::vector<TransportMessageHistoryEntry> replay;
  for (auto const& entry : outgoing_) {
    if (peerLastReceived == kInvalidTransportSequenceNumber ||
        sequenceAfter(entry.sequenceNumber, peerLastReceived)) {
      replay.push_back(entry);
    }
  }
  return replay;
}

bool ProcessTransportMessageHistory::canReplayAfter(
    std::uint32_t peerLastReceived) const {
  std::scoped_lock lock(mutex_);
  return hasCompleteSuffixAfterLocked(peerLastReceived);
}

bool ProcessTransportMessageHistory::hasCompleteSuffixAfterLocked(
    std::uint32_t peerLastReceived) const noexcept {
  if (peerLastReceived != kInvalidTransportSequenceNumber &&
      peerLastReceived > kMaximumTransportSequenceNumber) {
    return false;
  }
  if (!lastOutgoingSequenceNumber_) {
    return peerLastReceived == kInvalidTransportSequenceNumber;
  }
  if (peerLastReceived != kInvalidTransportSequenceNumber &&
      sequenceAfter(peerLastReceived, *lastOutgoingSequenceNumber_)) {
    return false;
  }
  if (lastAcknowledgedSequenceNumber_ &&
      (peerLastReceived == kInvalidTransportSequenceNumber ||
       (peerLastReceived != *lastAcknowledgedSequenceNumber_ &&
        sequenceAtOrBefore(
            peerLastReceived, *lastAcknowledgedSequenceNumber_)))) {
    return false;
  }
  if (lastDiscardedUnacknowledgedSequenceNumber_ &&
      (peerLastReceived == kInvalidTransportSequenceNumber ||
       sequenceAfter(
           *lastDiscardedUnacknowledgedSequenceNumber_, peerLastReceived))) {
    return false;
  }
  return true;
}

std::uint32_t
ProcessTransportMessageHistory::lastReceivedSequenceNumber() const {
  std::scoped_lock lock(mutex_);
  return lastReceivedSequenceNumber_;
}

std::optional<std::uint32_t>
ProcessTransportMessageHistory::oldestAvailableSequenceNumber() const {
  std::scoped_lock lock(mutex_);
  if (outgoing_.empty()) {
    return std::nullopt;
  }
  return outgoing_.front().sequenceNumber;
}

std::size_t ProcessTransportMessageHistory::retainedBytes() const {
  std::scoped_lock lock(mutex_);
  return retainedBytes_;
}

std::size_t ProcessTransportMessageHistory::retainedCount() const {
  std::scoped_lock lock(mutex_);
  return outgoing_.size();
}

bool ProcessTransportMessageHistory::sequenceAtOrBefore(
    std::uint32_t left,
    std::uint32_t right) noexcept {
  if (left == right) {
    return true;
  }
  auto const distance = (right - left) & kMaximumTransportSequenceNumber;
  return distance < (1U << 30U);
}

bool ProcessTransportMessageHistory::sequenceAfter(
    std::uint32_t left,
    std::uint32_t right) noexcept {
  return left != right && !sequenceAtOrBefore(left, right);
}

ProcessTransportSequenceState::ProcessTransportSequenceState(
    std::uint32_t firstSequenceNumber)
    : nextOutgoingSequenceNumber_(firstSequenceNumber) {
  if (firstSequenceNumber > kMaximumTransportSequenceNumber) {
    throw std::invalid_argument(
        "The first process transport sequence number is invalid.");
  }
}

std::uint32_t ProcessTransportSequenceState::nextOutgoingMessageNumber() {
  std::scoped_lock lock(mutex_);
  auto const sequenceNumber = nextOutgoingSequenceNumber_;
  nextOutgoingSequenceNumber_ = nextSequenceNumber(sequenceNumber);
  return sequenceNumber;
}

bool ProcessTransportSequenceState::acceptIncomingMessageNumber(
    std::uint32_t sequenceNumber) {
  if (sequenceNumber > kMaximumTransportSequenceNumber) {
    throw TransportProtocolError(
        "The received process transport message sequence number is invalid.");
  }

  std::scoped_lock lock(mutex_);
  if (lastReceivedSequenceNumber_ == kInvalidTransportSequenceNumber ||
      sequenceAfter(sequenceNumber, lastReceivedSequenceNumber_)) {
    lastReceivedSequenceNumber_ = sequenceNumber;
    return true;
  }
  return false;
}

void ProcessTransportSequenceState::beginIncomingReplay(
    std::uint32_t resumeControlSequenceNumber) {
  if (resumeControlSequenceNumber > kMaximumTransportSequenceNumber) {
    throw TransportProtocolError(
        "The process transport replay fence has an invalid sequence number.");
  }

  std::scoped_lock lock(mutex_);
  if (lastReceivedSequenceNumber_ != resumeControlSequenceNumber) {
    throw TransportProtocolError(
        "The process transport replay fence must match the received resume control.");
  }
  incomingReplayFence_ = resumeControlSequenceNumber;
}

bool ProcessTransportSequenceState::acceptIncomingServiceMessageNumber(
    std::uint32_t sequenceNumber,
    std::uint32_t lastReceivedServiceMessageNumber) {
  if (sequenceNumber > kMaximumTransportSequenceNumber ||
      (lastReceivedServiceMessageNumber != kInvalidTransportSequenceNumber &&
       lastReceivedServiceMessageNumber > kMaximumTransportSequenceNumber)) {
    throw TransportProtocolError(
        "The received process transport service-message sequence is invalid.");
  }

  std::scoped_lock lock(mutex_);
  if (lastReceivedServiceMessageNumber != kInvalidTransportSequenceNumber &&
      !sequenceAfter(sequenceNumber, lastReceivedServiceMessageNumber)) {
    return false;
  }

  if (incomingReplayFence_ &&
      !sequenceAfter(sequenceNumber, *incomingReplayFence_)) {
    // A service message before the Resume control's sequence is a replay
    // candidate. Its service-only LRM check above suppresses duplicates while
    // allowing messages lost before that control to be accepted in order.
    return true;
  }

  if (lastReceivedSequenceNumber_ == kInvalidTransportSequenceNumber ||
      sequenceAfter(sequenceNumber, lastReceivedSequenceNumber_)) {
    lastReceivedSequenceNumber_ = sequenceNumber;
    incomingReplayFence_.reset();
    return true;
  }
  return false;
}

std::uint32_t ProcessTransportSequenceState::lastReceivedMessageNumber() const {
  std::scoped_lock lock(mutex_);
  return lastReceivedSequenceNumber_;
}

std::uint32_t ProcessTransportSequenceState::nextSequenceNumber(
    std::uint32_t sequenceNumber) noexcept {
  return sequenceNumber == kMaximumTransportSequenceNumber
      ? 0U
      : sequenceNumber + 1U;
}

bool ProcessTransportSequenceState::sequenceAtOrBefore(
    std::uint32_t left,
    std::uint32_t right) noexcept {
  if (left == right) {
    return true;
  }
  auto const distance = (right - left) & kMaximumTransportSequenceNumber;
  return distance < (1U << 30U);
}

bool ProcessTransportSequenceState::sequenceAfter(
    std::uint32_t left,
    std::uint32_t right) noexcept {
  return left != right && !sequenceAtOrBefore(left, right);
}

ProcessTransportSessionState::ProcessTransportSessionState(
    std::uint64_t sessionId) {
  recordPeerActivity();
  if (sessionId != 0U) {
    sessionId_.store(sessionId, std::memory_order_relaxed);
  }
}

void ProcessTransportSessionState::recordPeerActivity() noexcept {
  auto const elapsed = std::chrono::steady_clock::now().time_since_epoch();
  lastPeerActivityNanoseconds_.store(
      std::chrono::duration_cast<std::chrono::nanoseconds>(elapsed).count(),
      std::memory_order_release);
}

std::chrono::steady_clock::time_point
ProcessTransportSessionState::lastPeerActivity() const noexcept {
  return std::chrono::steady_clock::time_point(
      std::chrono::nanoseconds(
          lastPeerActivityNanoseconds_.load(std::memory_order_acquire)));
}

void ProcessTransportSessionState::assignSessionId(std::uint64_t sessionId) {
  if (sessionId == 0U) {
    throw TransportProtocolError(
        "A process transport session state cannot use the invalid session ID.");
  }

  auto expectedSessionId = std::uint64_t{0U};
  if (sessionId_.compare_exchange_strong(
          expectedSessionId,
          sessionId,
          std::memory_order_release,
          std::memory_order_acquire) ||
      expectedSessionId == sessionId) {
    return;
  }
  throw TransportProtocolError(
      "A process transport session ID cannot change during its lifetime.");
}

void ProcessTransportSessionState::bindPeerEndpointId(
    std::string const& endpointId) {
  if (endpointId.empty()) {
    throw TransportProtocolError(
        "A process transport session requires a peer endpoint identity.");
  }

  std::scoped_lock lock(peerMutex_);
  if (peerEndpointId_.empty()) {
    peerEndpointId_ = endpointId;
    return;
  }
  if (peerEndpointId_ != endpointId) {
    throw TransportProtocolError(
        "A process transport session cannot resume from another endpoint identity.");
  }
}

std::uint64_t ProcessTransportSessionState::sessionId() const noexcept {
  return sessionId_.load(std::memory_order_acquire);
}

ProcessTransportMessageHistory&
ProcessTransportSessionState::messageHistory() noexcept {
  return messageHistory_;
}

ProcessTransportSequenceState&
ProcessTransportSessionState::sequenceState() noexcept {
  return sequenceState_;
}

}  // namespace umbra::detail
