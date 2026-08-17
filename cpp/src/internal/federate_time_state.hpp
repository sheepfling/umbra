#pragma once

#include <RTI/time/LogicalTime.h>
#include <RTI/time/LogicalTimeInterval.h>

#include <cstdint>
#include <memory>
#include <mutex>
#include <string>

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

enum class FederateTimeLookaheadStatus {
  applied,
  not_enabled,
  inactive,
};

struct FederateTimeLookaheadResult {
  FederateTimeLookaheadStatus status = FederateTimeLookaheadStatus::inactive;
  std::shared_ptr<rti1516_2025::LogicalTimeInterval const> lookahead;
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
  bool timeAdvancePending = false;
  bool timeRegulationPending = false;
  bool timeConstrainedPending = false;
  // A zero-lookahead regulator with a forward TAR pending, or with that TAR
  // already granted, must not send at the boundary time. The no-TSO bounds
  // calculator applies one factory epsilon to this exclusive lower bound.
  bool minimumTimestampIsExclusive = false;
  std::shared_ptr<rti1516_2025::LogicalTime const> currentTime;
  std::shared_ptr<rti1516_2025::LogicalTime const> requestedTime;
  std::shared_ptr<rti1516_2025::LogicalTimeInterval const> lookahead;
  std::shared_ptr<rti1516_2025::LogicalTimeInterval const> requestedLookahead;
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
  FederateTimeState(
      std::wstring implementationName,
      std::shared_ptr<rti1516_2025::LogicalTime> initialTime);

  FederateTimeState(FederateTimeState const&) = delete;
  FederateTimeState& operator=(FederateTimeState const&) = delete;

  [[nodiscard]] std::wstring const& implementationName() const noexcept;
  [[nodiscard]] std::shared_ptr<rti1516_2025::LogicalTime const> currentTime() const;
  [[nodiscard]] FederateTimeSnapshot snapshot() const;

  [[nodiscard]] FederateTimeAdvanceResult requestAdvance(
      std::shared_ptr<rti1516_2025::LogicalTime> requestedTime);

  // Returns the newly granted time only for the matching, still-active request.
  // A canceled or stale queued callback receives nullptr and must not invoke a
  // FederateAmbassador callback.
  [[nodiscard]] std::shared_ptr<rti1516_2025::LogicalTime const> grant(
      std::uint64_t generation);

  [[nodiscard]] FederateTimeEnableResult requestTimeRegulation(
      std::shared_ptr<rti1516_2025::LogicalTimeInterval> lookahead);

  // Returns the current time only for the matching, still-active regulation
  // request. The returned time is the argument to Time Regulation Enabled.
  [[nodiscard]] std::shared_ptr<rti1516_2025::LogicalTime const> grantTimeRegulation(
      std::uint64_t generation);

  [[nodiscard]] FederateTimeDisableStatus disableTimeRegulation();

  [[nodiscard]] FederateTimeLookaheadResult currentLookahead() const;

  [[nodiscard]] FederateTimeEnableResult requestTimeConstrained();

  // Returns the current time only for the matching, still-active constrained
  // request. The returned time is the argument to Time Constrained Enabled.
  [[nodiscard]] std::shared_ptr<rti1516_2025::LogicalTime const> grantTimeConstrained(
      std::uint64_t generation);

  [[nodiscard]] FederateTimeDisableStatus disableTimeConstrained();

  void deactivate() noexcept;

 private:
  mutable std::mutex mutex_;
  std::wstring implementationName_;
  std::shared_ptr<rti1516_2025::LogicalTime> currentTime_;
  std::shared_ptr<rti1516_2025::LogicalTime> pendingTime_;
  std::uint64_t pendingGeneration_ = 0;
  std::shared_ptr<rti1516_2025::LogicalTimeInterval> lookahead_;
  std::shared_ptr<rti1516_2025::LogicalTimeInterval> pendingLookahead_;
  std::uint64_t pendingTimeRegulationGeneration_ = 0;
  std::uint64_t pendingTimeConstrainedGeneration_ = 0;
  std::uint64_t nextGeneration_ = 1;
  bool timeRegulating_ = false;
  bool timeConstrained_ = false;
  bool minimumTimestampIsExclusive_ = false;
  bool active_ = true;
};

}  // namespace umbra::detail
