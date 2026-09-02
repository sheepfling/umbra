#include "internal/time/federate_time_state.hpp"

#include <RTI/Exception.h>
#include <RTI/time/HLAlogicalTimeFactoryFactory.h>

#include <limits>
#include <chrono>
#include <stdexcept>
#include <utility>

namespace umbra::detail {

namespace {

std::shared_ptr<rti1516_2025::LogicalTime> cloneLogicalTime(
    std::shared_ptr<rti1516_2025::LogicalTime const> const& source,
    std::wstring const& implementationName) {
  if (!source) {
    return nullptr;
  }
  auto factory = rti1516_2025::HLAlogicalTimeFactoryFactory::makeLogicalTimeFactory(
      implementationName);
  if (!factory || factory->getName() != implementationName) {
    throw rti1516_2025::RTIinternalError(
        L"Federate time snapshot could not create its logical-time factory.");
  }
  auto decoded = factory->decodeLogicalTime(source->encode());
  if (!decoded || decoded->implementationName() != implementationName) {
    throw rti1516_2025::RTIinternalError(
        L"Federate time snapshot could not clone its logical time.");
  }
  return std::shared_ptr<rti1516_2025::LogicalTime>(std::move(decoded));
}

std::shared_ptr<rti1516_2025::LogicalTimeInterval> cloneLogicalTimeInterval(
    std::shared_ptr<rti1516_2025::LogicalTimeInterval const> const& source,
    std::wstring const& implementationName) {
  if (!source) {
    return nullptr;
  }
  auto factory = rti1516_2025::HLAlogicalTimeFactoryFactory::makeLogicalTimeFactory(
      implementationName);
  if (!factory || factory->getName() != implementationName) {
    throw rti1516_2025::RTIinternalError(
        L"Federate time snapshot could not create its logical-time factory.");
  }
  auto decoded = factory->decodeLogicalTimeInterval(source->encode());
  if (!decoded || decoded->implementationName() != implementationName) {
    throw rti1516_2025::RTIinternalError(
        L"Federate time snapshot could not clone its logical-time interval.");
  }
  return std::shared_ptr<rti1516_2025::LogicalTimeInterval>(std::move(decoded));
}

}  // namespace

FederateTimeState::FederateTimeState(
    std::wstring implementationName,
    std::shared_ptr<rti1516_2025::LogicalTime> initialTime)
    : implementationName_(std::move(implementationName)),
      currentTime_(std::move(initialTime)),
      momStateSince_(std::chrono::steady_clock::now()) {}

FederateTimeState::FederateTimeState(FederateTimeState const& other) {
  std::scoped_lock lock(other.mutex_);
  auto const now = std::chrono::steady_clock::now();
  auto const momDurations = other.momTimeDurationsLocked(now);
  implementationName_ = other.implementationName_;
  currentTime_ = cloneLogicalTime(other.currentTime_, implementationName_);
  pendingTime_ = cloneLogicalTime(other.pendingTime_, implementationName_);
  pendingAdvanceRequestTime_ =
      cloneLogicalTime(other.pendingAdvanceRequestTime_, implementationName_);
  optimisticTime_ = cloneLogicalTime(other.optimisticTime_, implementationName_);
  lookahead_ = cloneLogicalTimeInterval(other.lookahead_, implementationName_);
  pendingLookahead_ = cloneLogicalTimeInterval(other.pendingLookahead_, implementationName_);
  pendingModifiedLookahead_ =
      cloneLogicalTimeInterval(other.pendingModifiedLookahead_, implementationName_);
  advanceMode_ = other.advanceMode_;
  pendingGeneration_ = other.pendingGeneration_;
  pendingTimeRegulationGeneration_ = other.pendingTimeRegulationGeneration_;
  pendingTimeConstrainedGeneration_ = other.pendingTimeConstrainedGeneration_;
  nextGeneration_ = other.nextGeneration_;
  timeRegulating_ = other.timeRegulating_;
  timeConstrained_ = other.timeConstrained_;
  asynchronousDeliveryEnabled_ = other.asynchronousDeliveryEnabled_;
  minimumTimestampIsExclusive_ = other.minimumTimestampIsExclusive_;
  active_ = other.active_;
  timeAdvancing_ = other.timeAdvancing_;
  momGrantedMilliseconds_ = momDurations.grantedMilliseconds;
  momAdvancingMilliseconds_ = momDurations.advancingMilliseconds;
  momStateSince_ = now;
}

FederateTimeState& FederateTimeState::operator=(FederateTimeState const& other) {
  if (this == &other) {
    return *this;
  }
  std::scoped_lock lock(mutex_, other.mutex_);
  auto const now = std::chrono::steady_clock::now();
  auto const momDurations = other.momTimeDurationsLocked(now);
  implementationName_ = other.implementationName_;
  currentTime_ = cloneLogicalTime(other.currentTime_, implementationName_);
  pendingTime_ = cloneLogicalTime(other.pendingTime_, implementationName_);
  pendingAdvanceRequestTime_ =
      cloneLogicalTime(other.pendingAdvanceRequestTime_, implementationName_);
  optimisticTime_ = cloneLogicalTime(other.optimisticTime_, implementationName_);
  lookahead_ = cloneLogicalTimeInterval(other.lookahead_, implementationName_);
  pendingLookahead_ = cloneLogicalTimeInterval(other.pendingLookahead_, implementationName_);
  pendingModifiedLookahead_ =
      cloneLogicalTimeInterval(other.pendingModifiedLookahead_, implementationName_);
  advanceMode_ = other.advanceMode_;
  pendingGeneration_ = other.pendingGeneration_;
  pendingTimeRegulationGeneration_ = other.pendingTimeRegulationGeneration_;
  pendingTimeConstrainedGeneration_ = other.pendingTimeConstrainedGeneration_;
  nextGeneration_ = other.nextGeneration_;
  timeRegulating_ = other.timeRegulating_;
  timeConstrained_ = other.timeConstrained_;
  asynchronousDeliveryEnabled_ = other.asynchronousDeliveryEnabled_;
  minimumTimestampIsExclusive_ = other.minimumTimestampIsExclusive_;
  active_ = other.active_;
  timeAdvancing_ = other.timeAdvancing_;
  momGrantedMilliseconds_ = momDurations.grantedMilliseconds;
  momAdvancingMilliseconds_ = momDurations.advancingMilliseconds;
  momStateSince_ = now;
  // Deferred callbacks belong to the live callback session, not to a copied
  // save/restore temporal snapshot.  A restore must not replay a closure that
  // was created after the snapshot was taken.
  deferredAsynchronousReceives_.clear();
  return *this;
}

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

void FederateTimeState::restoreFromSnapshot(FederateTimeSnapshot const& snapshot) {
  std::scoped_lock lock(mutex_);
  if (snapshot.implementationName != implementationName_) {
    throw std::logic_error(
        "Federate time snapshot uses a different logical-time implementation.");
  }
  if (snapshot.nextGeneration == 0U ||
      (snapshot.timeAdvancePending &&
       (snapshot.pendingTimeAdvanceGeneration == 0U ||
        snapshot.advanceMode == FederateTimeAdvanceMode::none)) ||
      (!snapshot.timeAdvancePending &&
       (snapshot.pendingTimeAdvanceGeneration != 0U ||
        snapshot.advanceMode != FederateTimeAdvanceMode::none)) ||
      snapshot.timeRegulationPending !=
          (snapshot.pendingTimeRegulationGeneration != 0U) ||
      snapshot.timeConstrainedPending !=
          (snapshot.pendingTimeConstrainedGeneration != 0U)) {
    throw std::logic_error(
        "Federate time snapshot has inconsistent pending request state.");
  }
  if (snapshot.pendingModifiedLookahead && !snapshot.timeRegulating) {
    throw std::logic_error(
        "Federate time snapshot has a deferred lookahead without regulation.");
  }

  currentTime_ = cloneLogicalTime(snapshot.currentTime, implementationName_);
  pendingTime_ = cloneLogicalTime(snapshot.requestedTime, implementationName_);
  pendingAdvanceRequestTime_ =
      cloneLogicalTime(snapshot.advanceRequestTime, implementationName_);
  optimisticTime_ = cloneLogicalTime(snapshot.optimisticTime, implementationName_);
  lookahead_ = cloneLogicalTimeInterval(snapshot.lookahead, implementationName_);
  pendingLookahead_ =
      cloneLogicalTimeInterval(snapshot.requestedLookahead, implementationName_);
  pendingModifiedLookahead_ =
      cloneLogicalTimeInterval(snapshot.pendingModifiedLookahead, implementationName_);
  pendingGeneration_ = snapshot.pendingTimeAdvanceGeneration;
  advanceMode_ = snapshot.advanceMode;
  pendingTimeRegulationGeneration_ = snapshot.pendingTimeRegulationGeneration;
  pendingTimeConstrainedGeneration_ = snapshot.pendingTimeConstrainedGeneration;
  nextGeneration_ = snapshot.nextGeneration;
  timeRegulating_ = snapshot.timeRegulating;
  timeConstrained_ = snapshot.timeConstrained;
  asynchronousDeliveryEnabled_ = snapshot.asynchronousDeliveryEnabled;
  minimumTimestampIsExclusive_ = snapshot.minimumTimestampIsExclusive;
  active_ = snapshot.active;
  timeAdvancing_ = snapshot.timeAdvancePending;
  momGrantedMilliseconds_ = 0U;
  momAdvancingMilliseconds_ = 0U;
  momStateSince_ = std::chrono::steady_clock::now();
  deferredAsynchronousReceives_.clear();
}

std::uint64_t FederateTimeState::callbackEpoch() const noexcept {
  std::scoped_lock lock(mutex_);
  return callbackEpoch_;
}

bool FederateTimeState::callbackEpochMatches(std::uint64_t expected) const noexcept {
  std::scoped_lock lock(mutex_);
  return expected != 0U && callbackEpoch_ == expected;
}

void FederateTimeState::invalidateCallbacksForRestore() noexcept {
  std::scoped_lock lock(mutex_);
  if (callbackEpoch_ == std::numeric_limits<std::uint64_t>::max()) {
    // Zero is reserved as an invalid caller-supplied epoch. Wrapping to one
    // preserves that fence even after an artificial exhaustion test.
    callbackEpoch_ = 1U;
  } else {
    ++callbackEpoch_;
  }
}

FederateTimeSnapshot FederateTimeState::snapshot() const {
  std::scoped_lock lock(mutex_);
  return {
      implementationName_,
      active_,
      timeRegulating_,
      timeConstrained_,
      asynchronousDeliveryEnabled_,
      pendingTime_ != nullptr,
      pendingGeneration_,
      advanceMode_,
      pendingTimeRegulationGeneration_ != 0,
      pendingTimeConstrainedGeneration_ != 0,
      pendingTimeRegulationGeneration_,
      pendingTimeConstrainedGeneration_,
      nextGeneration_,
      minimumTimestampIsExclusive_,
      currentTime_,
      optimisticTime_,
      pendingTime_,
      pendingAdvanceRequestTime_,
      lookahead_,
      pendingLookahead_,
      pendingModifiedLookahead_,
  };
}

FederateTimeAdvanceResult FederateTimeState::requestAdvance(
    std::shared_ptr<rti1516_2025::LogicalTime> requestedTime) {
  return requestAdvanceImpl(
      std::move(requestedTime),
      nullptr,
      FederateTimeAdvanceMode::time_advance_request);
}

FederateTimeAdvanceResult FederateTimeState::requestNextMessageAdvance(
    std::shared_ptr<rti1516_2025::LogicalTime> requestedTime,
    std::shared_ptr<rti1516_2025::LogicalTime> effectiveTime) {
  return requestAdvanceImpl(
      std::move(requestedTime),
      std::move(effectiveTime),
      FederateTimeAdvanceMode::next_message_request);
}

FederateTimeAdvanceResult FederateTimeState::requestAdvanceAvailable(
    std::shared_ptr<rti1516_2025::LogicalTime> requestedTime) {
  return requestAdvanceImpl(
      std::move(requestedTime),
      nullptr,
      FederateTimeAdvanceMode::time_advance_request_available);
}

FederateTimeAdvanceResult FederateTimeState::requestNextMessageAvailableAdvance(
    std::shared_ptr<rti1516_2025::LogicalTime> requestedTime,
    std::shared_ptr<rti1516_2025::LogicalTime> effectiveTime) {
  return requestAdvanceImpl(
      std::move(requestedTime),
      std::move(effectiveTime),
      FederateTimeAdvanceMode::next_message_request_available);
}

FederateTimeAdvanceResult FederateTimeState::requestFlushQueueAdvance(
    std::shared_ptr<rti1516_2025::LogicalTime> requestedTime) {
  return requestAdvanceImpl(
      std::move(requestedTime),
      nullptr,
      FederateTimeAdvanceMode::flush_queue_request);
}

FederateTimeAdvanceResult FederateTimeState::requestAdvanceImpl(
    std::shared_ptr<rti1516_2025::LogicalTime> requestedTime,
    std::shared_ptr<rti1516_2025::LogicalTime> effectiveTime,
    FederateTimeAdvanceMode mode) {
  std::scoped_lock lock(mutex_);
  if (!active_ || !currentTime_) {
    return {FederateTimeAdvanceStatus::inactive, 0};
  }
  if (!requestedTime || requestedTime->implementationName() != implementationName_) {
    return {FederateTimeAdvanceStatus::invalid_logical_time, 0};
  }
  if (!effectiveTime) {
    // TAR has one target. Sharing the pointer here lets the snapshot retain
    // the caller boundary without manufacturing a second time value.
    effectiveTime = requestedTime;
  }
  if (effectiveTime->implementationName() != implementationName_) {
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
    if (optimisticTime_ && *requestedTime < *optimisticTime_) {
      return {FederateTimeAdvanceStatus::logical_time_already_passed, 0};
    }
    if (*effectiveTime < *currentTime_ || *effectiveTime > *requestedTime) {
      return {FederateTimeAdvanceStatus::invalid_logical_time, 0};
    }
    // A Flush Queue Grant may leave the federate below its optimistic
    // logical-time floor.  The supplied request must reach that floor before
    // any effective target is accepted as a new grant.  This matters for NMR
    // and NMRA, whose selected queued-message timestamp can be earlier than
    // the caller's requested boundary.
    if (optimisticTime_ && *effectiveTime < *optimisticTime_) {
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
  // With zero lookahead, a forward time-management request changes the normal
  // inclusive timestamp lower bound to an exclusive one. For NMR the caller
  // boundary is deliberately used here even when the effective grant target
  // is an earlier queued message; the separate request-time snapshot keeps
  // federation GALT/TSO calculations on that boundary.
  if (makesMinimumTimestampExclusive) {
    minimumTimestampIsExclusive_ = true;
  }
  pendingTime_ = std::move(effectiveTime);
  pendingAdvanceRequestTime_ = std::move(requestedTime);
  advanceMode_ = mode;
  accumulateMomTimeLocked(std::chrono::steady_clock::now());
  timeAdvancing_ = true;
  return {FederateTimeAdvanceStatus::applied, pendingGeneration_};
}

std::shared_ptr<rti1516_2025::LogicalTime const> FederateTimeState::grant(
    std::uint64_t generation) {
  std::scoped_lock lock(mutex_);
  return grantImpl(generation);
}

std::shared_ptr<rti1516_2025::LogicalTime const> FederateTimeState::grantImpl(
    std::uint64_t generation) {
  if (!active_ || !pendingTime_ || generation == 0 || generation != pendingGeneration_) {
    return nullptr;
  }

  // Validate every comparison that can still fail before changing the
  // callback-gated state below.  LogicalTime is an extensibility point in the
  // official API; a custom implementation may report InvalidLogicalTime for
  // an otherwise name-compatible comparison.  Returning nullptr after
  // currentTime_ or the pending-generation fields have already been mutated
  // would strand the federate in a state with no matching grant callback.
  bool clearsOptimisticFloor = false;
  if (optimisticTime_) {
    try {
      clearsOptimisticFloor = *pendingTime_ >= *optimisticTime_;
    } catch (rti1516_2025::Exception const&) {
      return nullptr;
    }
  }

  // A decreasing Modify Lookahead request is applied at the grant boundary.
  // Compute the revised interval on a private clone first: the official
  // LogicalTimeInterval operators are virtual and may reject a compatible-
  // name operation.  Mutating lookahead_ before the rest of the grant is
  // committed would leave a pending request with a partially applied
  // lookahead if one of those operations failed.
  std::shared_ptr<rti1516_2025::LogicalTimeInterval> revisedLookahead;
  bool applyPendingLookahead = false;
  if (pendingModifiedLookahead_ && lookahead_ && currentTime_) {
    try {
      auto factory = rti1516_2025::HLAlogicalTimeFactoryFactory::makeLogicalTimeFactory(
          implementationName_);
      if (!factory || factory->getName() != implementationName_) {
        return nullptr;
      }
      auto elapsed = factory->makeZero();
      auto remaining = factory->makeZero();
      if (!elapsed || !remaining) {
        return nullptr;
      }
      elapsed->setToDifference(*pendingTime_, *currentTime_);
      *remaining = *lookahead_;
      *remaining -= *pendingModifiedLookahead_;
      if (*elapsed >= *remaining) {
        applyPendingLookahead = true;
      } else {
        revisedLookahead = cloneLogicalTimeInterval(lookahead_, implementationName_);
        if (!revisedLookahead) {
          return nullptr;
        }
        *revisedLookahead -= *elapsed;
      }
    } catch (rti1516_2025::Exception const&) {
      return nullptr;
    }
  }

  accumulateMomTimeLocked(std::chrono::steady_clock::now());
  timeAdvancing_ = false;
  currentTime_ = std::move(pendingTime_);
  if (clearsOptimisticFloor) {
    optimisticTime_.reset();
  }
  if (pendingModifiedLookahead_ && lookahead_ && currentTime_) {
    if (applyPendingLookahead) {
      lookahead_ = std::move(pendingModifiedLookahead_);
      pendingModifiedLookahead_.reset();
    } else {
      lookahead_ = std::move(revisedLookahead);
    }
  }
  pendingAdvanceRequestTime_.reset();
  advanceMode_ = FederateTimeAdvanceMode::none;
  pendingGeneration_ = 0;
  return currentTime_;
}

FederateTimeAdvanceResult FederateTimeState::grantFlushQueue(
    std::uint64_t generation,
    std::shared_ptr<rti1516_2025::LogicalTime> grantedTime,
    std::shared_ptr<rti1516_2025::LogicalTime> optimisticTime) {
  std::scoped_lock lock(mutex_);
  if (!active_ || !pendingTime_ || generation == 0 || generation != pendingGeneration_ ||
      advanceMode_ != FederateTimeAdvanceMode::flush_queue_request || !grantedTime ||
      !optimisticTime || grantedTime->implementationName() != implementationName_ ||
      optimisticTime->implementationName() != implementationName_ || !currentTime_ ||
      !pendingAdvanceRequestTime_) {
    return {FederateTimeAdvanceStatus::invalid_logical_time, 0};
  }

  try {
    if (*grantedTime < *currentTime_ || *grantedTime > *pendingAdvanceRequestTime_ ||
        *optimisticTime < *grantedTime || *optimisticTime > *pendingAdvanceRequestTime_) {
      return {FederateTimeAdvanceStatus::invalid_logical_time, 0};
    }
  } catch (rti1516_2025::Exception const&) {
    return {FederateTimeAdvanceStatus::invalid_logical_time, 0};
  }

  // Resolve the optimistic-floor relation while the request is still
  // untouched.  grantImpl performs the analogous validation for the existing
  // floor; this one protects the newly supplied Flush Queue value from a
  // comparison failure after the grant has committed.
  bool clearsSuppliedOptimisticFloor = false;
  try {
    clearsSuppliedOptimisticFloor = *optimisticTime <= *grantedTime;
  } catch (rti1516_2025::Exception const&) {
    return {FederateTimeAdvanceStatus::invalid_logical_time, 0};
  }

  // The ordinary grant path owns the callback-gated state transition. Replace
  // only the pending target for this Flush Queue Grant, then retain the OLT
  // after the transition for the next request's precondition. Keep the prior
  // target until grantImpl has completed its no-throw commit boundary: a
  // failed deferred-lookahead clone must not rewrite the caller's pending
  // request with the provisional actual grant.
  auto previousPendingTime = std::move(pendingTime_);
  pendingTime_ = std::move(grantedTime);
  auto const granted = grantImpl(generation);
  if (!granted) {
    pendingTime_ = std::move(previousPendingTime);
    return {FederateTimeAdvanceStatus::invalid_logical_time, 0};
  }

  if (clearsSuppliedOptimisticFloor) {
    optimisticTime_.reset();
  } else {
    optimisticTime_ = std::move(optimisticTime);
  }
  return {FederateTimeAdvanceStatus::applied, generation};
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
  return grantTimeRegulationIfCurrent(generation, 0U);
}

std::shared_ptr<rti1516_2025::LogicalTime const>
FederateTimeState::grantTimeRegulationIfCurrent(
    std::uint64_t generation,
    std::uint64_t callbackEpoch) {
  std::scoped_lock lock(mutex_);
  if (!active_ || !currentTime_ || !pendingLookahead_ || generation == 0 ||
      generation != pendingTimeRegulationGeneration_ ||
      (callbackEpoch != 0U && callbackEpoch != callbackEpoch_)) {
    return nullptr;
  }

  timeRegulating_ = true;
  lookahead_ = std::move(pendingLookahead_);
  pendingModifiedLookahead_.reset();
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
  pendingModifiedLookahead_.reset();
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

FederateTimeModifyLookaheadStatus FederateTimeState::modifyLookahead(
    std::shared_ptr<rti1516_2025::LogicalTimeInterval> requestedLookahead) {
  std::scoped_lock lock(mutex_);
  if (!active_ || !currentTime_) {
    return FederateTimeModifyLookaheadStatus::inactive;
  }
  if (pendingTime_) {
    return FederateTimeModifyLookaheadStatus::time_advance_pending;
  }
  if (!timeRegulating_ || !lookahead_ || !requestedLookahead) {
    return FederateTimeModifyLookaheadStatus::not_enabled;
  }

  try {
    // A forward request made while the active lookahead was zero establishes
    // an exclusive minimum timestamp boundary.  Increasing that lookahead
    // removes the condition that required the epsilon; keep the boundary
    // marker aligned with the effective interval rather than carrying the
    // old zero-lookahead restriction into later federation bounds and TSO
    // validation.
    bool const requestedLookaheadIsZero = requestedLookahead->isZero();
    if (*requestedLookahead >= *lookahead_) {
      lookahead_ = std::move(requestedLookahead);
      pendingModifiedLookahead_.reset();
      if (!requestedLookaheadIsZero) {
        minimumTimestampIsExclusive_ = false;
      }
    } else {
      pendingModifiedLookahead_ = std::move(requestedLookahead);
    }
  } catch (rti1516_2025::Exception const&) {
    return FederateTimeModifyLookaheadStatus::not_enabled;
  }
  return FederateTimeModifyLookaheadStatus::applied;
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
  return grantTimeConstrainedIfCurrent(generation, 0U);
}

std::shared_ptr<rti1516_2025::LogicalTime const>
FederateTimeState::grantTimeConstrainedIfCurrent(
    std::uint64_t generation,
    std::uint64_t callbackEpoch) {
  std::scoped_lock lock(mutex_);
  if (!active_ || !currentTime_ || pendingTimeConstrainedGeneration_ == 0 ||
      generation == 0 || generation != pendingTimeConstrainedGeneration_ ||
      (callbackEpoch != 0U && callbackEpoch != callbackEpoch_)) {
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

FederateAsynchronousDeliveryStatus
FederateTimeState::enableAsynchronousDelivery() {
  std::scoped_lock lock(mutex_);
  if (!active_) {
    return FederateAsynchronousDeliveryStatus::inactive;
  }
  if (asynchronousDeliveryEnabled_) {
    return FederateAsynchronousDeliveryStatus::already_enabled;
  }
  asynchronousDeliveryEnabled_ = true;
  return FederateAsynchronousDeliveryStatus::applied;
}

FederateAsynchronousDeliveryStatus
FederateTimeState::disableAsynchronousDelivery() {
  std::scoped_lock lock(mutex_);
  if (!active_) {
    return FederateAsynchronousDeliveryStatus::inactive;
  }
  if (!asynchronousDeliveryEnabled_) {
    return FederateAsynchronousDeliveryStatus::already_disabled;
  }
  asynchronousDeliveryEnabled_ = false;
  return FederateAsynchronousDeliveryStatus::applied;
}

bool FederateTimeState::receiveOrderDeliveryAllowed() const {
  std::scoped_lock lock(mutex_);
  if (!active_) {
    return false;
  }
  return !timeConstrained_ || asynchronousDeliveryEnabled_ || pendingTime_ != nullptr;
}

void FederateTimeState::deferAsynchronousReceive(DeferredCallback callback) {
  if (!callback) {
    return;
  }
  std::scoped_lock lock(mutex_);
  if (active_) {
    deferredAsynchronousReceives_.push_back(std::move(callback));
  }
}

std::vector<FederateTimeState::DeferredCallback>
FederateTimeState::takeEligibleAsynchronousReceiveCallbacks() {
  std::scoped_lock lock(mutex_);
  if (!active_ ||
      (timeConstrained_ && !asynchronousDeliveryEnabled_ && !pendingTime_)) {
    return {};
  }
  std::vector<DeferredCallback> result;
  result.reserve(deferredAsynchronousReceives_.size());
  while (!deferredAsynchronousReceives_.empty()) {
    result.push_back(std::move(deferredAsynchronousReceives_.front()));
    deferredAsynchronousReceives_.pop_front();
  }
  return result;
}

std::size_t FederateTimeState::deferredAsynchronousReceiveCount() const {
  std::scoped_lock lock(mutex_);
  return deferredAsynchronousReceives_.size();
}

FederateMomTimeDurations FederateTimeState::momTimeDurations() const {
  std::scoped_lock lock(mutex_);
  return momTimeDurationsLocked(std::chrono::steady_clock::now());
}

FederateMomTimeDurations FederateTimeState::takeMomTimeDurations() {
  std::scoped_lock lock(mutex_);
  accumulateMomTimeLocked(std::chrono::steady_clock::now());
  FederateMomTimeDurations result{
      momGrantedMilliseconds_, momAdvancingMilliseconds_};
  momGrantedMilliseconds_ = 0;
  momAdvancingMilliseconds_ = 0;
  return result;
}

void FederateTimeState::accumulateMomTimeLocked(
    std::chrono::steady_clock::time_point now) {
  auto const elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
      now - momStateSince_);
  momStateSince_ = now;
  if (elapsed.count() <= 0) {
    return;
  }
  auto const milliseconds = static_cast<std::uint64_t>(elapsed.count());
  auto& selected = timeAdvancing_
      ? momAdvancingMilliseconds_
      : momGrantedMilliseconds_;
  if (std::numeric_limits<std::uint64_t>::max() - selected < milliseconds) {
    selected = std::numeric_limits<std::uint64_t>::max();
  } else {
    selected += milliseconds;
  }
}

FederateMomTimeDurations FederateTimeState::momTimeDurationsLocked(
    std::chrono::steady_clock::time_point now) const {
  auto result = FederateMomTimeDurations{
      momGrantedMilliseconds_, momAdvancingMilliseconds_};
  auto const elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
      now - momStateSince_);
  if (elapsed.count() <= 0) {
    return result;
  }
  auto const milliseconds = static_cast<std::uint64_t>(elapsed.count());
  auto& selected = timeAdvancing_
      ? result.advancingMilliseconds
      : result.grantedMilliseconds;
  if (std::numeric_limits<std::uint64_t>::max() - selected < milliseconds) {
    selected = std::numeric_limits<std::uint64_t>::max();
  } else {
    selected += milliseconds;
  }
  return result;
}

void FederateTimeState::deactivate() noexcept {
  std::scoped_lock lock(mutex_);
  accumulateMomTimeLocked(std::chrono::steady_clock::now());
  active_ = false;
  currentTime_.reset();
  pendingTime_.reset();
  pendingAdvanceRequestTime_.reset();
  optimisticTime_.reset();
  advanceMode_ = FederateTimeAdvanceMode::none;
  pendingGeneration_ = 0;
  lookahead_.reset();
  pendingLookahead_.reset();
  pendingModifiedLookahead_.reset();
  pendingTimeRegulationGeneration_ = 0;
  pendingTimeConstrainedGeneration_ = 0;
  timeRegulating_ = false;
  timeConstrained_ = false;
  asynchronousDeliveryEnabled_ = false;
  minimumTimestampIsExclusive_ = false;
  deferredAsynchronousReceives_.clear();
}

}  // namespace umbra::detail
