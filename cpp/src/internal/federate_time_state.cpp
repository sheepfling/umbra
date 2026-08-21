#include "internal/federate_time_state.hpp"

#include <RTI/Exception.h>
#include <RTI/time/HLAlogicalTimeFactoryFactory.h>

#include <limits>
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
      currentTime_(std::move(initialTime)) {}

FederateTimeState::FederateTimeState(FederateTimeState const& other) {
  std::scoped_lock lock(other.mutex_);
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
}

FederateTimeState& FederateTimeState::operator=(FederateTimeState const& other) {
  if (this == &other) {
    return *this;
  }
  std::scoped_lock lock(mutex_, other.mutex_);
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
      minimumTimestampIsExclusive_,
      currentTime_,
      optimisticTime_,
      pendingTime_,
      pendingAdvanceRequestTime_,
      lookahead_,
      pendingLookahead_,
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
        lookahead_ = std::move(pendingModifiedLookahead_);
        pendingModifiedLookahead_.reset();
      } else {
        *lookahead_ -= *elapsed;
      }
    } catch (rti1516_2025::Exception const&) {
      return nullptr;
    }
  }

  currentTime_ = std::move(pendingTime_);
  if (optimisticTime_ && currentTime_) {
    try {
      if (*currentTime_ >= *optimisticTime_) {
        optimisticTime_.reset();
      }
    } catch (rti1516_2025::Exception const&) {
      return nullptr;
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

  // The ordinary grant path owns the callback-gated state transition. Replace
  // only the pending target for this Flush Queue Grant, then retain the OLT
  // after the transition for the next request's precondition.
  pendingTime_ = std::move(grantedTime);
  auto const granted = grantImpl(generation);
  if (!granted) {
    return {FederateTimeAdvanceStatus::invalid_logical_time, 0};
  }

  auto optimistic = std::move(optimisticTime);
  try {
    if (*optimistic <= *granted) {
      optimisticTime_.reset();
    } else {
      optimisticTime_ = std::move(optimistic);
    }
  } catch (rti1516_2025::Exception const&) {
    return {FederateTimeAdvanceStatus::invalid_logical_time, 0};
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
  std::scoped_lock lock(mutex_);
  if (!active_ || !currentTime_ || !pendingLookahead_ || generation == 0 ||
      generation != pendingTimeRegulationGeneration_) {
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
    if (*requestedLookahead >= *lookahead_) {
      lookahead_ = std::move(requestedLookahead);
      pendingModifiedLookahead_.reset();
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

void FederateTimeState::deactivate() noexcept {
  std::scoped_lock lock(mutex_);
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
