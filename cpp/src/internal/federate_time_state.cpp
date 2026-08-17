#include "internal/federate_time_state.hpp"

#include <RTI/Exception.h>

#include <limits>
#include <utility>

namespace umbra::detail {

FederateTimeState::FederateTimeState(
    std::wstring implementationName,
    std::shared_ptr<rti1516_2025::LogicalTime> initialTime)
    : implementationName_(std::move(implementationName)),
      currentTime_(std::move(initialTime)) {}

std::wstring const& FederateTimeState::implementationName() const noexcept {
  return implementationName_;
}

std::shared_ptr<rti1516_2025::LogicalTime const> FederateTimeState::currentTime() const {
  std::scoped_lock lock(mutex_);
  if (!active_) {
    return nullptr;
  }
  return currentTime_;
}

FederateTimeSnapshot FederateTimeState::snapshot() const {
  std::scoped_lock lock(mutex_);
  return {
      implementationName_,
      active_,
      timeRegulating_,
      timeConstrained_,
      pendingTime_ != nullptr,
      pendingTimeRegulationGeneration_ != 0,
      pendingTimeConstrainedGeneration_ != 0,
      minimumTimestampIsExclusive_,
      currentTime_,
      pendingTime_,
      lookahead_,
      pendingLookahead_,
  };
}

FederateTimeAdvanceResult FederateTimeState::requestAdvance(
    std::shared_ptr<rti1516_2025::LogicalTime> requestedTime) {
  std::scoped_lock lock(mutex_);
  if (!active_ || !currentTime_) {
    return {FederateTimeAdvanceStatus::inactive, 0};
  }
  if (!requestedTime || requestedTime->implementationName() != implementationName_) {
    return {FederateTimeAdvanceStatus::invalid_logical_time, 0};
  }
  if (pendingTime_) {
    return {FederateTimeAdvanceStatus::time_advance_pending, 0};
  }
  if (pendingLookahead_) {
    return {FederateTimeAdvanceStatus::time_regulation_pending, 0};
  }
  if (pendingTimeConstrainedGeneration_ != 0) {
    return {FederateTimeAdvanceStatus::time_constrained_pending, 0};
  }

  bool makesMinimumTimestampExclusive = false;
  try {
    if (*requestedTime < *currentTime_) {
      return {FederateTimeAdvanceStatus::logical_time_already_passed, 0};
    }
    makesMinimumTimestampExclusive =
        timeRegulating_ && lookahead_ && lookahead_->isZero() && *requestedTime > *currentTime_;
  } catch (rti1516_2025::InvalidLogicalTime const&) {
    return {FederateTimeAdvanceStatus::invalid_logical_time, 0};
  }

  if (nextGeneration_ == std::numeric_limits<std::uint64_t>::max()) {
    return {FederateTimeAdvanceStatus::generation_exhausted, 0};
  }

  pendingGeneration_ = nextGeneration_++;
  // With zero lookahead, Time Advance Request changes the normal inclusive
  // timestamp lower bound to an exclusive one when it truly moves time
  // forward.  This development profile currently implements TAR only; later
  // NMR/FQR paths must update this same invariant according to their own
  // standard restrictions.
  if (makesMinimumTimestampExclusive) {
    minimumTimestampIsExclusive_ = true;
  }
  pendingTime_ = std::move(requestedTime);
  return {FederateTimeAdvanceStatus::applied, pendingGeneration_};
}

std::shared_ptr<rti1516_2025::LogicalTime const> FederateTimeState::grant(
    std::uint64_t generation) {
  std::scoped_lock lock(mutex_);
  if (!active_ || !pendingTime_ || generation == 0 || generation != pendingGeneration_) {
    return nullptr;
  }

  currentTime_ = std::move(pendingTime_);
  pendingGeneration_ = 0;
  return currentTime_;
}

FederateTimeEnableResult FederateTimeState::requestTimeRegulation(
    std::shared_ptr<rti1516_2025::LogicalTimeInterval> lookahead) {
  std::scoped_lock lock(mutex_);
  if (!active_ || !currentTime_) {
    return {FederateTimeEnableStatus::inactive, 0};
  }
  if (!lookahead || lookahead->implementationName() != implementationName_) {
    return {FederateTimeEnableStatus::invalid_lookahead, 0};
  }
  if (pendingTime_) {
    return {FederateTimeEnableStatus::time_advance_pending, 0};
  }
  if (pendingLookahead_) {
    return {FederateTimeEnableStatus::request_pending, 0};
  }
  if (timeRegulating_) {
    return {FederateTimeEnableStatus::already_enabled, 0};
  }
  if (nextGeneration_ == std::numeric_limits<std::uint64_t>::max()) {
    return {FederateTimeEnableStatus::generation_exhausted, 0};
  }

  pendingTimeRegulationGeneration_ = nextGeneration_++;
  pendingLookahead_ = std::move(lookahead);
  return {FederateTimeEnableStatus::applied, pendingTimeRegulationGeneration_};
}

std::shared_ptr<rti1516_2025::LogicalTime const> FederateTimeState::grantTimeRegulation(
    std::uint64_t generation) {
  std::scoped_lock lock(mutex_);
  if (!active_ || !currentTime_ || !pendingLookahead_ || generation == 0 ||
      generation != pendingTimeRegulationGeneration_) {
    return nullptr;
  }

  timeRegulating_ = true;
  lookahead_ = std::move(pendingLookahead_);
  minimumTimestampIsExclusive_ = false;
  pendingTimeRegulationGeneration_ = 0;
  return currentTime_;
}

FederateTimeDisableStatus FederateTimeState::disableTimeRegulation() {
  std::scoped_lock lock(mutex_);
  if (!active_) {
    return FederateTimeDisableStatus::inactive;
  }
  if (!timeRegulating_) {
    return FederateTimeDisableStatus::not_enabled;
  }

  timeRegulating_ = false;
  lookahead_.reset();
  minimumTimestampIsExclusive_ = false;
  return FederateTimeDisableStatus::applied;
}

FederateTimeLookaheadResult FederateTimeState::currentLookahead() const {
  std::scoped_lock lock(mutex_);
  if (!active_) {
    return {FederateTimeLookaheadStatus::inactive, nullptr};
  }
  if (!timeRegulating_ || !lookahead_) {
    return {FederateTimeLookaheadStatus::not_enabled, nullptr};
  }
  return {FederateTimeLookaheadStatus::applied, lookahead_};
}

FederateTimeEnableResult FederateTimeState::requestTimeConstrained() {
  std::scoped_lock lock(mutex_);
  if (!active_ || !currentTime_) {
    return {FederateTimeEnableStatus::inactive, 0};
  }
  if (pendingTime_) {
    return {FederateTimeEnableStatus::time_advance_pending, 0};
  }
  if (pendingTimeConstrainedGeneration_ != 0) {
    return {FederateTimeEnableStatus::request_pending, 0};
  }
  if (timeConstrained_) {
    return {FederateTimeEnableStatus::already_enabled, 0};
  }
  if (nextGeneration_ == std::numeric_limits<std::uint64_t>::max()) {
    return {FederateTimeEnableStatus::generation_exhausted, 0};
  }

  pendingTimeConstrainedGeneration_ = nextGeneration_++;
  return {FederateTimeEnableStatus::applied, pendingTimeConstrainedGeneration_};
}

std::shared_ptr<rti1516_2025::LogicalTime const> FederateTimeState::grantTimeConstrained(
    std::uint64_t generation) {
  std::scoped_lock lock(mutex_);
  if (!active_ || !currentTime_ || pendingTimeConstrainedGeneration_ == 0 ||
      generation == 0 || generation != pendingTimeConstrainedGeneration_) {
    return nullptr;
  }

  timeConstrained_ = true;
  pendingTimeConstrainedGeneration_ = 0;
  return currentTime_;
}

FederateTimeDisableStatus FederateTimeState::disableTimeConstrained() {
  std::scoped_lock lock(mutex_);
  if (!active_) {
    return FederateTimeDisableStatus::inactive;
  }
  if (!timeConstrained_) {
    return FederateTimeDisableStatus::not_enabled;
  }

  timeConstrained_ = false;
  return FederateTimeDisableStatus::applied;
}

void FederateTimeState::deactivate() noexcept {
  std::scoped_lock lock(mutex_);
  active_ = false;
  currentTime_.reset();
  pendingTime_.reset();
  pendingGeneration_ = 0;
  lookahead_.reset();
  pendingLookahead_.reset();
  pendingTimeRegulationGeneration_ = 0;
  pendingTimeConstrainedGeneration_ = 0;
  timeRegulating_ = false;
  timeConstrained_ = false;
  minimumTimestampIsExclusive_ = false;
}

}  // namespace umbra::detail
