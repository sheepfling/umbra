#pragma once

#include <RTI/time/LogicalTime.h>
#include <RTI/time/LogicalTimeInterval.h>

#include <cstdint>
#include <chrono>
#include <deque>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace umbra::detail {

enum class FederateTimeAdvanceStatus {
  applied,
  logical_time_already_passed,
  time_advance_pending,
  time_regulation_pending,
  time_constrained_pending,
  invalid_logical_time,
  inactive,
  generation_exhausted,
};

// The request form is part of the federation-owned temporal state because
// the available variants have an inclusive GALT boundary, while TAR/NMR use
// the strict boundary rules from IEEE 1516.1-2025.  The value is meaningful
// only while timeAdvancePending is true.
enum class FederateTimeAdvanceMode {
  none,
  time_advance_request,
  time_advance_request_available,
  next_message_request,
  next_message_request_available,
  flush_queue_request,
};

struct FederateTimeAdvanceResult {
  FederateTimeAdvanceStatus status = FederateTimeAdvanceStatus::inactive;
  std::uint64_t generation = 0;
};

enum class FederateTimeEnableStatus {
  applied,
  time_advance_pending,
  request_pending,
  already_enabled,
  invalid_lookahead,
  inactive,
  generation_exhausted,
};

struct FederateTimeEnableResult {
  FederateTimeEnableStatus status = FederateTimeEnableStatus::inactive;
  std::uint64_t generation = 0;
};

enum class FederateTimeDisableStatus {
  applied,
  not_enabled,
  inactive,
};

enum class FederateAsynchronousDeliveryStatus {
  applied,
  already_enabled,
  already_disabled,
  inactive,
};

enum class FederateTimeLookaheadStatus {
  applied,
  not_enabled,
  inactive,
};

struct FederateTimeLookaheadResult {
  FederateTimeLookaheadStatus status = FederateTimeLookaheadStatus::inactive;
  std::shared_ptr<rti1516_2025::LogicalTimeInterval const> lookahead;
};

enum class FederateTimeModifyLookaheadStatus {
  applied,
  time_advance_pending,
  not_enabled,
  inactive,
};

// An immutable view of one federate's private time state.  The contained
// reference-time objects are owned by FederateTimeState and are exposed as
// const shared pointers; every mutation replaces a pointer while holding the
// state mutex.  A federation-owned coordinator can therefore use this view
// without retaining caller-owned LogicalTime objects or taking a callback
// recipient lock.
struct FederateTimeSnapshot {
  std::wstring implementationName;
  bool active = false;
  bool timeRegulating = false;
  bool timeConstrained = false;
  bool asynchronousDeliveryEnabled = false;
  bool timeAdvancePending = false;
  // Internal callback-generation identity for a pending advance. It is never
  // public HLA state, but the restore scheduler needs it to recreate the
  // matching callback-gated grant without accepting stale work.
  std::uint64_t pendingTimeAdvanceGeneration = 0;
  FederateTimeAdvanceMode advanceMode = FederateTimeAdvanceMode::none;
  bool timeRegulationPending = false;
  bool timeConstrainedPending = false;
  // A zero-lookahead regulator with a forward TAR pending, or with that TAR
  // already granted, must not send at the boundary time. The no-TSO bounds
  // calculator applies one factory epsilon to this exclusive lower bound.
  bool minimumTimestampIsExclusive = false;
  std::shared_ptr<rti1516_2025::LogicalTime const> currentTime;
  // A Flush Queue Grant may advance the federate below the optimistic logical
  // time supplied with that grant.  Subsequent advances must not request a
  // value below this retained floor until the federate reaches it.
  std::shared_ptr<rti1516_2025::LogicalTime const> optimisticTime;
  // The effective target used by the pending grant. For TAR this is the
  // supplied request; for NMR it may be the next queued TSO timestamp.
  std::shared_ptr<rti1516_2025::LogicalTime const> requestedTime;
  // The logical time supplied by the caller. Keeping this separate from the
  // effective grant target preserves NMR's requested-time boundary for GALT
  // and timestamp validation when a message causes an earlier grant.
  std::shared_ptr<rti1516_2025::LogicalTime const> advanceRequestTime;
  std::shared_ptr<rti1516_2025::LogicalTimeInterval const> lookahead;
  std::shared_ptr<rti1516_2025::LogicalTimeInterval const> requestedLookahead;
};

// Wall-clock time spent in each temporal state since the previous MOM
// duration report. The values are kept wide internally and clamped to the
// standard HLAmsec HLAinteger32BE representation at the MOM boundary.
struct FederateMomTimeDurations {
  std::uint64_t grantedMilliseconds = 0;
  std::uint64_t advancingMilliseconds = 0;
};

// Per-federate logical-time state for the embedded development profile. The
// selected implementation and all stored values are official LogicalTime
// instances; this component owns no Umbra replacement time representation.
// A request remains pending until grant() is called from the dispatched Time
// Advance Grant callback, preserving the standard time-advancing interval.
// The same callback-gated model records time-regulating and time-constrained
// enable requests. Timestamped delivery is coordinated outside this per-
// federate state; role callbacks still preserve the current time, while the
// federation-owned coordinator releases bounded TSO payloads before grants.
// Broader federation-wide coordination and transport remain separate work.
class FederateTimeState final {
 public:
  using DeferredCallback = std::function<void()>;

  FederateTimeState(
      std::wstring implementationName,
      std::shared_ptr<rti1516_2025::LogicalTime> initialTime);

  // Save/restore snapshots need an independent copy of the selected
  // federation time state.  The copy constructor/assignment clone the
  // official LogicalTime values instead of sharing mutable interval objects
  // between the live federate and its snapshot.
  FederateTimeState(FederateTimeState const& other);
  FederateTimeState& operator=(FederateTimeState const& other);

  [[nodiscard]] std::wstring const& implementationName() const noexcept;
  [[nodiscard]] std::shared_ptr<rti1516_2025::LogicalTime const> currentTime() const;
  [[nodiscard]] FederateTimeSnapshot snapshot() const;

  [[nodiscard]] FederateTimeAdvanceResult requestAdvance(
      std::shared_ptr<rti1516_2025::LogicalTime> requestedTime);

  // Accepts a Next Message Request after the adapter has selected the next
  // currently queued TSO timestamp. A null effective target means that no
  // queued message is eligible at or below the requested time, so the grant
  // target remains the supplied request.
  [[nodiscard]] FederateTimeAdvanceResult requestNextMessageAdvance(
      std::shared_ptr<rti1516_2025::LogicalTime> requestedTime,
      std::shared_ptr<rti1516_2025::LogicalTime> effectiveTime);

  // The Available forms share the same bounded current-queue scheduler as
  // TAR/NMR but use the inclusive GALT rule.  A null effective target keeps
  // the supplied request as the grant target, just as for TAR.
  [[nodiscard]] FederateTimeAdvanceResult requestAdvanceAvailable(
      std::shared_ptr<rti1516_2025::LogicalTime> requestedTime);

  [[nodiscard]] FederateTimeAdvanceResult requestNextMessageAvailableAdvance(
      std::shared_ptr<rti1516_2025::LogicalTime> requestedTime,
      std::shared_ptr<rti1516_2025::LogicalTime> effectiveTime);

  [[nodiscard]] FederateTimeAdvanceResult requestFlushQueueAdvance(
      std::shared_ptr<rti1516_2025::LogicalTime> requestedTime);

  // Returns the newly granted time only for the matching, still-active request.
  // A canceled or stale queued callback receives nullptr and must not invoke a
  // FederateAmbassador callback.
  [[nodiscard]] std::shared_ptr<rti1516_2025::LogicalTime const> grant(
      std::uint64_t generation);

  // Completes a Flush Queue Request at a possibly earlier logical time and
  // records its optimistic logical-time floor for later advance validation.
  [[nodiscard]] FederateTimeAdvanceResult grantFlushQueue(
      std::uint64_t generation,
      std::shared_ptr<rti1516_2025::LogicalTime> grantedTime,
      std::shared_ptr<rti1516_2025::LogicalTime> optimisticTime);

  [[nodiscard]] FederateTimeEnableResult requestTimeRegulation(
      std::shared_ptr<rti1516_2025::LogicalTimeInterval> lookahead);

  // Returns the current time only for the matching, still-active regulation
  // request. The returned time is the argument to Time Regulation Enabled.
  [[nodiscard]] std::shared_ptr<rti1516_2025::LogicalTime const> grantTimeRegulation(
      std::uint64_t generation);

  [[nodiscard]] FederateTimeDisableStatus disableTimeRegulation();

  [[nodiscard]] FederateTimeLookaheadResult currentLookahead() const;

  // A nondecreasing request changes the actual lookahead immediately.  A
  // decreasing request is retained and applied gradually as logical time
  // advances, matching the IEEE 1516.1-2025 Modify Lookahead postcondition.
  [[nodiscard]] FederateTimeModifyLookaheadStatus modifyLookahead(
      std::shared_ptr<rti1516_2025::LogicalTimeInterval> requestedLookahead);

  [[nodiscard]] FederateTimeEnableResult requestTimeConstrained();

  // Returns the current time only for the matching, still-active constrained
  // request. The returned time is the argument to Time Constrained Enabled.
  [[nodiscard]] std::shared_ptr<rti1516_2025::LogicalTime const> grantTimeConstrained(
      std::uint64_t generation);

  [[nodiscard]] FederateTimeDisableStatus disableTimeConstrained();

  // Asynchronous delivery is a synchronous per-federate switch.  The public
  // adapter translates these precise state outcomes into the official
  // AsynchronousDeliveryAlreadyEnabled/Disabled exceptions.
  [[nodiscard]] FederateAsynchronousDeliveryStatus enableAsynchronousDelivery();
  [[nodiscard]] FederateAsynchronousDeliveryStatus disableAsynchronousDelivery();

  // Receive-order callbacks are admitted for an unconstrained federate in any
  // temporal state, for an asynchronously-enabled constrained federate in
  // Time Granted or Time Advancing, and for a constrained federate with the
  // switch disabled only while a time advance request is pending.
  [[nodiscard]] bool receiveOrderDeliveryAllowed() const;

  // A callback route uses this queue when an RO callback reaches the
  // dispatcher while the recipient is in a state where the standard forbids
  // delivery.  The queued task re-enters the route after a state transition;
  // it never invokes user code while this state mutex is held.
  void deferAsynchronousReceive(DeferredCallback callback);
  [[nodiscard]] std::vector<DeferredCallback>
  takeEligibleAsynchronousReceiveCallbacks();
  [[nodiscard]] std::size_t deferredAsynchronousReceiveCount() const;

  // Direct AVU observes the current interval without consuming it. Periodic
  // MOM delivery claims the interval exactly once through the non-const form.
  [[nodiscard]] FederateMomTimeDurations momTimeDurations() const;
  [[nodiscard]] FederateMomTimeDurations takeMomTimeDurations();

  void deactivate() noexcept;

 private:
  [[nodiscard]] FederateTimeAdvanceResult requestAdvanceImpl(
      std::shared_ptr<rti1516_2025::LogicalTime> requestedTime,
      std::shared_ptr<rti1516_2025::LogicalTime> effectiveTime,
      FederateTimeAdvanceMode mode);

  [[nodiscard]] std::shared_ptr<rti1516_2025::LogicalTime const> grantImpl(
      std::uint64_t generation);
  void accumulateMomTimeLocked(std::chrono::steady_clock::time_point now);
  [[nodiscard]] FederateMomTimeDurations momTimeDurationsLocked(
      std::chrono::steady_clock::time_point now) const;

  mutable std::mutex mutex_;
  std::wstring implementationName_;
  std::shared_ptr<rti1516_2025::LogicalTime> currentTime_;
  std::shared_ptr<rti1516_2025::LogicalTime> pendingTime_;
  std::shared_ptr<rti1516_2025::LogicalTime> pendingAdvanceRequestTime_;
  std::shared_ptr<rti1516_2025::LogicalTime> optimisticTime_;
  FederateTimeAdvanceMode advanceMode_ = FederateTimeAdvanceMode::none;
  std::uint64_t pendingGeneration_ = 0;
  std::shared_ptr<rti1516_2025::LogicalTimeInterval> lookahead_;
  std::shared_ptr<rti1516_2025::LogicalTimeInterval> pendingLookahead_;
  std::shared_ptr<rti1516_2025::LogicalTimeInterval> pendingModifiedLookahead_;
  std::uint64_t pendingTimeRegulationGeneration_ = 0;
  std::uint64_t pendingTimeConstrainedGeneration_ = 0;
  std::uint64_t nextGeneration_ = 1;
  bool timeRegulating_ = false;
  bool timeConstrained_ = false;
  bool asynchronousDeliveryEnabled_ = false;
  bool minimumTimestampIsExclusive_ = false;
  bool active_ = true;
  bool timeAdvancing_ = false;
  std::chrono::steady_clock::time_point momStateSince_ =
      std::chrono::steady_clock::now();
  std::uint64_t momGrantedMilliseconds_ = 0;
  std::uint64_t momAdvancingMilliseconds_ = 0;
  std::deque<DeferredCallback> deferredAsynchronousReceives_;
};

}  // namespace umbra::detail
