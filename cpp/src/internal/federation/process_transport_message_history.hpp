#pragma once

#include "internal/federation/transport_protocol.hpp"

#include <cstddef>
#include <cstdint>
#include <atomic>
#include <chrono>
#include <deque>
#include <mutex>
#include <optional>
#include <span>
#include <string>
#include <vector>

namespace umbra::detail {

struct TransportMessageHistoryEntry final {
  std::uint32_t sequenceNumber = kInvalidTransportSequenceNumber;
  std::vector<std::uint8_t> payload;
  TransportFrameKind frameKind = TransportFrameKind::data;
};

// Retains the unacknowledged HLA service-message bytes needed to replay an
// ordered suffix after a transport fault. Sequence gaps may represent control
// frames; those are sequenced but are not retained here.
class ProcessTransportMessageHistory final {
 public:
  explicit ProcessTransportMessageHistory(
      std::size_t maximumBytes = 16U * 1024U * 1024U);

  [[nodiscard]] std::uint32_t retainOutgoing(
      std::uint32_t sequenceNumber,
      std::span<std::uint8_t const> payload,
      TransportFrameKind frameKind = TransportFrameKind::data);
  void acknowledgeOutgoing(std::uint32_t lastReceivedMessageNumber);
  [[nodiscard]] bool acceptIncoming(std::uint32_t sequenceNumber);

  [[nodiscard]] std::optional<std::vector<TransportMessageHistoryEntry>>
  replayAfter(std::uint32_t peerLastReceived) const;
  [[nodiscard]] bool canReplayAfter(std::uint32_t peerLastReceived) const;
  [[nodiscard]] std::uint32_t lastReceivedSequenceNumber() const;
  [[nodiscard]] std::optional<std::uint32_t>
  oldestAvailableSequenceNumber() const;
  [[nodiscard]] std::size_t retainedBytes() const;
  [[nodiscard]] std::size_t retainedCount() const;

 private:
  [[nodiscard]] static bool sequenceAtOrBefore(
      std::uint32_t left,
      std::uint32_t right) noexcept;
  [[nodiscard]] static bool sequenceAfter(
      std::uint32_t left,
      std::uint32_t right) noexcept;
  [[nodiscard]] bool hasCompleteSuffixAfterLocked(
      std::uint32_t peerLastReceived) const noexcept;

  mutable std::mutex mutex_;
  std::size_t maximumBytes_;
  std::size_t retainedBytes_ = 0U;
  std::uint32_t lastReceivedSequenceNumber_ = kInvalidTransportSequenceNumber;
  std::optional<std::uint32_t> lastOutgoingSequenceNumber_;
  std::optional<std::uint32_t> lastAcknowledgedSequenceNumber_;
  std::optional<std::uint32_t> lastDiscardedUnacknowledgedSequenceNumber_;
  std::deque<TransportMessageHistoryEntry> outgoing_;
};

// Sequence numbers belong to every protocol message, including control
// messages. This direction-local state is separate from message history:
// only HLA service messages are retained or acknowledged through LRM.
class ProcessTransportSequenceState final {
 public:
  explicit ProcessTransportSequenceState(std::uint32_t firstSequenceNumber = 0U);

  [[nodiscard]] std::uint32_t nextOutgoingMessageNumber();
  [[nodiscard]] bool acceptIncomingMessageNumber(std::uint32_t sequenceNumber);
  void beginIncomingReplay(std::uint32_t resumeControlSequenceNumber);
  [[nodiscard]] bool acceptIncomingServiceMessageNumber(
      std::uint32_t sequenceNumber,
      std::uint32_t lastReceivedServiceMessageNumber);
  [[nodiscard]] std::uint32_t lastReceivedMessageNumber() const;

 private:
  [[nodiscard]] static std::uint32_t nextSequenceNumber(
      std::uint32_t sequenceNumber) noexcept;
  [[nodiscard]] static bool sequenceAtOrBefore(
      std::uint32_t left,
      std::uint32_t right) noexcept;
  [[nodiscard]] static bool sequenceAfter(
      std::uint32_t left,
      std::uint32_t right) noexcept;

  mutable std::mutex mutex_;
  std::uint32_t nextOutgoingSequenceNumber_ = 0U;
  std::uint32_t lastReceivedSequenceNumber_ = kInvalidTransportSequenceNumber;
  std::optional<std::uint32_t> incomingReplayFence_;
};

// Connection-independent identity and reliability state for one federate
// session. A replacement socket reuses this object; a new join creates a new
// one. The listener binds it to the same peer endpoint on each connection.
class ProcessTransportSessionState final {
 public:
  explicit ProcessTransportSessionState(std::uint64_t sessionId = 0U);

  void assignSessionId(std::uint64_t sessionId);
  void bindPeerEndpointId(std::string const& endpointId);
  void recordPeerActivity() noexcept;
  [[nodiscard]] std::chrono::steady_clock::time_point lastPeerActivity()
      const noexcept;
  [[nodiscard]] std::uint64_t sessionId() const noexcept;
  [[nodiscard]] ProcessTransportMessageHistory& messageHistory() noexcept;
  [[nodiscard]] ProcessTransportSequenceState& sequenceState() noexcept;

 private:
  std::atomic_uint64_t sessionId_{0U};
  std::atomic_int64_t lastPeerActivityNanoseconds_{0};
  mutable std::mutex peerMutex_;
  std::string peerEndpointId_;
  ProcessTransportMessageHistory messageHistory_;
  ProcessTransportSequenceState sequenceState_;
};

}  // namespace umbra::detail
