#include <catch2/catch_test_macros.hpp>

#include "internal/time/federate_time_state.hpp"

#include <cstdint>
#include <memory>

#include <RTI/time/HLAfloat64Interval.h>
#include <RTI/time/HLAfloat64Time.h>
#include <RTI/time/HLAinteger64Interval.h>
#include <RTI/time/HLAinteger64Time.h>

namespace {

using umbra::detail::FederateTimeAdvanceStatus;
using umbra::detail::FederateAsynchronousDeliveryStatus;
using umbra::detail::FederateTimeDisableStatus;
using umbra::detail::FederateTimeEnableStatus;
using umbra::detail::FederateTimeLookaheadStatus;
using umbra::detail::FederateTimeState;
using rti1516_2025::HLAfloat64Time;
using rti1516_2025::HLAinteger64Interval;
using rti1516_2025::HLAinteger64Time;

class ThrowingGreaterEqualTime final : public HLAinteger64Time {
 public:
  ThrowingGreaterEqualTime(std::int64_t value, bool& throwOnComparison)
      : HLAinteger64Time(value), throwOnComparison_(&throwOnComparison) {}

  bool operator>=(rti1516_2025::LogicalTime const& value) const override {
    if (*throwOnComparison_) {
      throw rti1516_2025::InvalidLogicalTime(
          L"test logical-time implementation rejected operator>=.");
    }
    return HLAinteger64Time::operator>=(value);
  }

 private:
  bool* throwOnComparison_;
};

class ThrowingLessEqualTime final : public HLAinteger64Time {
 public:
  ThrowingLessEqualTime(std::int64_t value, bool& throwOnComparison)
      : HLAinteger64Time(value), throwOnComparison_(&throwOnComparison) {}

  bool operator<=(rti1516_2025::LogicalTime const& value) const override {
    if (*throwOnComparison_) {
      throw rti1516_2025::InvalidLogicalTime(
          L"test logical-time implementation rejected operator<=.");
    }
    return HLAinteger64Time::operator<=(value);
  }

 private:
  bool* throwOnComparison_;
};

class ThrowingEncodeInterval final : public HLAinteger64Interval {
 public:
  ThrowingEncodeInterval(std::int64_t value, bool& throwOnEncode)
      : HLAinteger64Interval(value), throwOnEncode_(&throwOnEncode) {}

  rti1516_2025::VariableLengthData encode() const override {
    if (*throwOnEncode_) {
      throw rti1516_2025::InvalidLogicalTimeInterval(
          L"test logical-time interval rejected cloning.");
    }
    return HLAinteger64Interval::encode();
  }

 private:
  bool* throwOnEncode_;
};

std::shared_ptr<FederateTimeState> integerTimeState() {
  return std::make_shared<FederateTimeState>(
      L"HLAinteger64Time",
      std::make_shared<HLAinteger64Time>());
}

HLAinteger64Time const& asIntegerTime(
    std::shared_ptr<rti1516_2025::LogicalTime const> const& time) {
  auto const* value = dynamic_cast<HLAinteger64Time const*>(time.get());
  REQUIRE(value != nullptr);
  return *value;
}

HLAinteger64Interval const& asIntegerInterval(
    std::shared_ptr<rti1516_2025::LogicalTimeInterval const> const& interval) {
  auto const* value = dynamic_cast<HLAinteger64Interval const*>(interval.get());
  REQUIRE(value != nullptr);
  return *value;
}

}  // namespace

TEST_CASE(
    "Federate time state gates deferred receive-order callbacks with asynchronous delivery",
    "[unit][kernel][time-management][asynchronous-delivery]") {
  auto state = integerTimeState();
  REQUIRE_FALSE(state->snapshot().asynchronousDeliveryEnabled);
  REQUIRE(state->receiveOrderDeliveryAllowed());

  auto constrained = state->requestTimeConstrained();
  REQUIRE(constrained.status == FederateTimeEnableStatus::applied);
  REQUIRE(state->grantTimeConstrained(constrained.generation));
  REQUIRE_FALSE(state->receiveOrderDeliveryAllowed());

  int deferredCount = 0;
  state->deferAsynchronousReceive([&deferredCount] { ++deferredCount; });
  REQUIRE(state->takeEligibleAsynchronousReceiveCallbacks().empty());

  REQUIRE(
      state->enableAsynchronousDelivery() ==
      FederateAsynchronousDeliveryStatus::applied);
  REQUIRE(state->snapshot().asynchronousDeliveryEnabled);
  REQUIRE(state->receiveOrderDeliveryAllowed());
  auto enabledCallbacks = state->takeEligibleAsynchronousReceiveCallbacks();
  REQUIRE(enabledCallbacks.size() == 1);
  enabledCallbacks.front()();
  REQUIRE(deferredCount == 1);
  REQUIRE(
      state->enableAsynchronousDelivery() ==
      FederateAsynchronousDeliveryStatus::already_enabled);

  REQUIRE(
      state->disableAsynchronousDelivery() ==
      FederateAsynchronousDeliveryStatus::applied);
  REQUIRE_FALSE(state->receiveOrderDeliveryAllowed());
  REQUIRE(
      state->disableAsynchronousDelivery() ==
      FederateAsynchronousDeliveryStatus::already_disabled);

  state->deferAsynchronousReceive([&deferredCount] { ++deferredCount; });
  auto advance = state->requestAdvance(std::make_shared<HLAinteger64Time>(1));
  REQUIRE(advance.status == FederateTimeAdvanceStatus::applied);
  REQUIRE(state->receiveOrderDeliveryAllowed());
  auto advancingCallbacks = state->takeEligibleAsynchronousReceiveCallbacks();
  REQUIRE(advancingCallbacks.size() == 1);
  advancingCallbacks.front()();
  REQUIRE(deferredCount == 2);

  REQUIRE(state->grant(advance.generation));
  REQUIRE_FALSE(state->receiveOrderDeliveryAllowed());
  state->deactivate();
  REQUIRE(state->takeEligibleAsynchronousReceiveCallbacks().empty());
}

TEST_CASE(
    "Federate time state retains the initial value until its matching grant",
    "[unit][kernel][time-management][time-advance-request]") {
  auto state = integerTimeState();
  REQUIRE(state->implementationName() == L"HLAinteger64Time");
  REQUIRE(asIntegerTime(state->currentTime()).isInitial());

  auto requested = std::make_shared<HLAinteger64Time>(7);
  auto request = state->requestAdvance(requested);
  REQUIRE(request.status == FederateTimeAdvanceStatus::applied);
  REQUIRE(request.generation != 0);
  REQUIRE(asIntegerTime(state->currentTime()).isInitial());

  REQUIRE_FALSE(state->grant(request.generation + 1));
  REQUIRE(asIntegerTime(state->currentTime()).isInitial());

  auto granted = state->grant(request.generation);
  REQUIRE(granted);
  REQUIRE(asIntegerTime(granted).getTime() == 7);
  REQUIRE(asIntegerTime(state->currentTime()).getTime() == 7);
}

TEST_CASE(
    "Federate time state keeps an NMR request boundary separate from its message grant target",
    "[unit][kernel][time-management][next-message-request]") {
  auto state = integerTimeState();

  auto request = state->requestNextMessageAdvance(
      std::make_shared<HLAinteger64Time>(10),
      std::make_shared<HLAinteger64Time>(7));
  REQUIRE(request.status == FederateTimeAdvanceStatus::applied);

  auto pending = state->snapshot();
  REQUIRE(pending.timeAdvancePending);
  REQUIRE(pending.requestedTime);
  REQUIRE(pending.advanceRequestTime);
  REQUIRE(
      pending.advanceMode ==
      umbra::detail::FederateTimeAdvanceMode::next_message_request);
  REQUIRE(asIntegerTime(pending.requestedTime).getTime() == 7);
  REQUIRE(asIntegerTime(pending.advanceRequestTime).getTime() == 10);
  REQUIRE(asIntegerTime(state->currentTime()).isInitial());

  REQUIRE(state->grant(request.generation));
  REQUIRE(asIntegerTime(state->currentTime()).getTime() == 7);
  REQUIRE_FALSE(state->snapshot().advanceRequestTime);
  REQUIRE(
      state->snapshot().advanceMode ==
      umbra::detail::FederateTimeAdvanceMode::none);
}

TEST_CASE(
    "Federate time state records Available time-advance request forms",
    "[unit][kernel][time-management][time-advance-request-available]"
    "[next-message-request-available]") {
  auto state = integerTimeState();

  auto available = state->requestAdvanceAvailable(
      std::make_shared<HLAinteger64Time>(6));
  REQUIRE(available.status == FederateTimeAdvanceStatus::applied);
  REQUIRE(
      state->snapshot().advanceMode ==
      umbra::detail::FederateTimeAdvanceMode::time_advance_request_available);
  REQUIRE(state->grant(available.generation));

  auto nmrAvailable = state->requestNextMessageAvailableAdvance(
      std::make_shared<HLAinteger64Time>(9),
      std::make_shared<HLAinteger64Time>(8));
  REQUIRE(nmrAvailable.status == FederateTimeAdvanceStatus::applied);
  auto pending = state->snapshot();
  REQUIRE(
      pending.advanceMode ==
      umbra::detail::FederateTimeAdvanceMode::next_message_request_available);
  REQUIRE(pending.requestedTime);
  REQUIRE(pending.advanceRequestTime);
  REQUIRE(asIntegerTime(pending.requestedTime).getTime() == 8);
  REQUIRE(asIntegerTime(pending.advanceRequestTime).getTime() == 9);
}

TEST_CASE(
    "Federate time state retains the Flush Queue optimistic floor",
    "[unit][kernel][time-management][flush-queue-request]") {
  auto state = integerTimeState();

  auto request = state->requestFlushQueueAdvance(
      std::make_shared<HLAinteger64Time>(10));
  REQUIRE(request.status == FederateTimeAdvanceStatus::applied);
  REQUIRE(
      state->snapshot().advanceMode ==
      umbra::detail::FederateTimeAdvanceMode::flush_queue_request);

  auto grant = state->grantFlushQueue(
      request.generation,
      std::make_shared<HLAinteger64Time>(5),
      std::make_shared<HLAinteger64Time>(8));
  REQUIRE(grant.status == FederateTimeAdvanceStatus::applied);
  REQUIRE(asIntegerTime(state->currentTime()).getTime() == 5);
  REQUIRE(state->snapshot().optimisticTime);
  REQUIRE(asIntegerTime(state->snapshot().optimisticTime).getTime() == 8);

  REQUIRE(
      state->requestAdvance(std::make_shared<HLAinteger64Time>(7)).status ==
      FederateTimeAdvanceStatus::logical_time_already_passed);
  auto next = state->requestAdvance(std::make_shared<HLAinteger64Time>(8));
  REQUIRE(next.status == FederateTimeAdvanceStatus::applied);
  REQUIRE(state->grant(next.generation));
  REQUIRE_FALSE(state->snapshot().optimisticTime);
}

TEST_CASE(
    "Federate time state rejects NMR targets below the Flush Queue optimistic floor",
    "[unit][kernel][time-management][flush-queue-request][next-message-request]"
    "[next-message-request-available]") {
  auto state = integerTimeState();

  auto flushRequest = state->requestFlushQueueAdvance(
      std::make_shared<HLAinteger64Time>(10));
  REQUIRE(flushRequest.status == FederateTimeAdvanceStatus::applied);
  auto flushGrant = state->grantFlushQueue(
      flushRequest.generation,
      std::make_shared<HLAinteger64Time>(5),
      std::make_shared<HLAinteger64Time>(8));
  REQUIRE(flushGrant.status == FederateTimeAdvanceStatus::applied);

  // NMR/NMRA retain the caller's request separately from the selected message
  // timestamp.  A valid request of ten must not be allowed to grant at seven,
  // because that effective target is below the retained optimistic floor.
  auto rejectedNmr = state->requestNextMessageAdvance(
      std::make_shared<HLAinteger64Time>(10),
      std::make_shared<HLAinteger64Time>(7));
  REQUIRE(rejectedNmr.status == FederateTimeAdvanceStatus::logical_time_already_passed);
  REQUIRE_FALSE(state->snapshot().timeAdvancePending);
  REQUIRE(asIntegerTime(state->currentTime()).getTime() == 5);

  // The floor itself is admissible for the available variant and is cleared
  // once the matching grant reaches it.
  auto acceptedNmra = state->requestNextMessageAvailableAdvance(
      std::make_shared<HLAinteger64Time>(10),
      std::make_shared<HLAinteger64Time>(8));
  REQUIRE(acceptedNmra.status == FederateTimeAdvanceStatus::applied);
  REQUIRE(state->grant(acceptedNmra.generation));
  REQUIRE(asIntegerTime(state->currentTime()).getTime() == 8);
  REQUIRE_FALSE(state->snapshot().optimisticTime);
}

TEST_CASE(
    "Federate time state leaves a pending grant intact when its optimistic comparison fails",
    "[unit][kernel][time-management][flush-queue-request][exception-safety]") {
  auto state = integerTimeState();

  auto firstRequest = state->requestFlushQueueAdvance(
      std::make_shared<HLAinteger64Time>(10));
  REQUIRE(firstRequest.status == FederateTimeAdvanceStatus::applied);
  REQUIRE(state->grantFlushQueue(
              firstRequest.generation,
              std::make_shared<HLAinteger64Time>(5),
              std::make_shared<HLAinteger64Time>(8))
              .status ==
          FederateTimeAdvanceStatus::applied);

  auto secondRequest = state->requestFlushQueueAdvance(
      std::make_shared<HLAinteger64Time>(10));
  REQUIRE(secondRequest.status == FederateTimeAdvanceStatus::applied);
  bool throwOnComparison = true;
  auto rejectedGrant = state->grantFlushQueue(
      secondRequest.generation,
      std::make_shared<ThrowingGreaterEqualTime>(9, throwOnComparison),
      std::make_shared<HLAinteger64Time>(10));
  REQUIRE(rejectedGrant.status == FederateTimeAdvanceStatus::invalid_logical_time);

  // The comparison failed before the callback-gated transition.  The
  // original current time, optimistic floor, and pending generation remain
  // available for a retry rather than being stranded half-committed.
  auto snapshot = state->snapshot();
  REQUIRE(snapshot.timeAdvancePending);
  REQUIRE(snapshot.pendingTimeAdvanceGeneration == secondRequest.generation);
  REQUIRE(asIntegerTime(snapshot.currentTime).getTime() == 5);
  REQUIRE(asIntegerTime(snapshot.optimisticTime).getTime() == 8);

  throwOnComparison = false;
  auto retry = state->grantFlushQueue(
      secondRequest.generation,
      std::make_shared<HLAinteger64Time>(9),
      std::make_shared<HLAinteger64Time>(10));
  REQUIRE(retry.status == FederateTimeAdvanceStatus::applied);
  REQUIRE(asIntegerTime(state->currentTime()).getTime() == 9);
  REQUIRE(asIntegerTime(state->snapshot().optimisticTime).getTime() == 10);
}

TEST_CASE(
    "Federate time state rejects a Flush Queue optimistic value before committing the grant",
    "[unit][kernel][time-management][flush-queue-request][exception-safety]") {
  auto state = integerTimeState();
  auto request = state->requestFlushQueueAdvance(
      std::make_shared<HLAinteger64Time>(10));
  REQUIRE(request.status == FederateTimeAdvanceStatus::applied);

  bool throwOnComparison = true;
  auto optimistic = std::make_shared<ThrowingLessEqualTime>(8, throwOnComparison);
  auto rejectedGrant = state->grantFlushQueue(
      request.generation,
      std::make_shared<HLAinteger64Time>(5),
      optimistic);
  REQUIRE(rejectedGrant.status == FederateTimeAdvanceStatus::invalid_logical_time);
  auto snapshot = state->snapshot();
  REQUIRE(snapshot.timeAdvancePending);
  REQUIRE(snapshot.pendingTimeAdvanceGeneration == request.generation);
  REQUIRE(asIntegerTime(snapshot.currentTime).isInitial());

  throwOnComparison = false;
  auto retry = state->grantFlushQueue(
      request.generation,
      std::make_shared<HLAinteger64Time>(5),
      optimistic);
  REQUIRE(retry.status == FederateTimeAdvanceStatus::applied);
  REQUIRE(asIntegerTime(state->currentTime()).getTime() == 5);
  REQUIRE(asIntegerTime(state->snapshot().optimisticTime).getTime() == 8);
}

TEST_CASE(
    "Federate time state keeps a deferred lookahead grant transactional when cloning fails",
    "[unit][kernel][time-management][modify-lookahead][exception-safety]") {
  auto state = integerTimeState();
  bool throwOnEncode = true;

  auto regulation = state->requestTimeRegulation(
      std::make_shared<ThrowingEncodeInterval>(5, throwOnEncode));
  REQUIRE(regulation.status == FederateTimeEnableStatus::applied);
  REQUIRE(state->grantTimeRegulation(regulation.generation));

  auto const modifyStatus = state->modifyLookahead(
      std::make_shared<HLAinteger64Interval>(1));
  REQUIRE(
      modifyStatus ==
      umbra::detail::FederateTimeModifyLookaheadStatus::applied);

  auto advance = state->requestAdvance(std::make_shared<HLAinteger64Time>(3));
  REQUIRE(advance.status == FederateTimeAdvanceStatus::applied);

  // The old implementation reduced the live interval before committing the
  // grant.  Cloning the custom interval now fails first, so the request and
  // the original actual lookahead remain available and unchanged.
  REQUIRE_FALSE(state->grant(advance.generation));
  auto snapshot = state->snapshot();
  REQUIRE(snapshot.timeAdvancePending);
  REQUIRE(snapshot.pendingTimeAdvanceGeneration == advance.generation);
  REQUIRE(asIntegerTime(snapshot.currentTime).isInitial());
  REQUIRE(snapshot.lookahead);
  auto const* lookahead =
      dynamic_cast<HLAinteger64Interval const*>(snapshot.lookahead.get());
  REQUIRE(lookahead != nullptr);
  REQUIRE(lookahead->getInterval() == 5);

  // The deferred request was not consumed by the failed attempt.  Let the
  // custom interval clone succeed and retry the same generation; the revised
  // interval is then applied exactly once from the original value.
  throwOnEncode = false;
  REQUIRE(state->grant(advance.generation));
  REQUIRE(asIntegerTime(state->currentTime()).getTime() == 3);
  auto revised = state->currentLookahead();
  REQUIRE(revised.status == FederateTimeLookaheadStatus::applied);
  REQUIRE(asIntegerInterval(revised.lookahead).getInterval() == 2);
}

TEST_CASE(
    "Federate time state keeps a Flush Queue request target when deferred lookahead cloning fails",
    "[unit][kernel][time-management][flush-queue-request][modify-lookahead]"
    "[exception-safety]") {
  auto state = integerTimeState();
  bool throwOnEncode = true;

  auto regulation = state->requestTimeRegulation(
      std::make_shared<ThrowingEncodeInterval>(5, throwOnEncode));
  REQUIRE(regulation.status == FederateTimeEnableStatus::applied);
  REQUIRE(state->grantTimeRegulation(regulation.generation));
  REQUIRE(
      state->modifyLookahead(std::make_shared<HLAinteger64Interval>(1)) ==
      umbra::detail::FederateTimeModifyLookaheadStatus::applied);

  auto request = state->requestFlushQueueAdvance(
      std::make_shared<HLAinteger64Time>(10));
  REQUIRE(request.status == FederateTimeAdvanceStatus::applied);

  // The provisional actual grant requires cloning the custom live lookahead.
  // A failed clone must leave the original FQG target (10), not the
  // provisional actual grant (3), in the pending state.
  auto rejected = state->grantFlushQueue(
      request.generation,
      std::make_shared<HLAinteger64Time>(3),
      std::make_shared<HLAinteger64Time>(8));
  REQUIRE(rejected.status == FederateTimeAdvanceStatus::invalid_logical_time);
  auto snapshot = state->snapshot();
  REQUIRE(snapshot.timeAdvancePending);
  REQUIRE(snapshot.pendingTimeAdvanceGeneration == request.generation);
  REQUIRE(snapshot.advanceRequestTime);
  REQUIRE(asIntegerTime(snapshot.advanceRequestTime).getTime() == 10);
  REQUIRE(snapshot.requestedTime);
  REQUIRE(asIntegerTime(snapshot.requestedTime).getTime() == 10);
  REQUIRE(asIntegerTime(snapshot.currentTime).isInitial());

  throwOnEncode = false;
  auto retry = state->grantFlushQueue(
      request.generation,
      std::make_shared<HLAinteger64Time>(3),
      std::make_shared<HLAinteger64Time>(8));
  REQUIRE(retry.status == FederateTimeAdvanceStatus::applied);
  REQUIRE(asIntegerTime(state->currentTime()).getTime() == 3);
  REQUIRE(asIntegerInterval(state->currentLookahead().lookahead).getInterval() == 2);
  REQUIRE(asIntegerTime(state->snapshot().optimisticTime).getTime() == 8);
}

TEST_CASE(
    "Federate time state rejects backward, mismatched, duplicate, and stale advances",
    "[unit][kernel][time-management]") {
  auto state = integerTimeState();
  auto first = state->requestAdvance(std::make_shared<HLAinteger64Time>(4));
  REQUIRE(first.status == FederateTimeAdvanceStatus::applied);
  REQUIRE(state->grant(first.generation));

  REQUIRE(
      state->requestAdvance(std::make_shared<HLAinteger64Time>(3)).status ==
      FederateTimeAdvanceStatus::logical_time_already_passed);
  REQUIRE(
      state->requestAdvance(std::make_shared<HLAfloat64Time>(4.0)).status ==
      FederateTimeAdvanceStatus::invalid_logical_time);

  auto pending = state->requestAdvance(std::make_shared<HLAinteger64Time>(9));
  REQUIRE(pending.status == FederateTimeAdvanceStatus::applied);
  REQUIRE(
      state->requestAdvance(std::make_shared<HLAinteger64Time>(10)).status ==
      FederateTimeAdvanceStatus::time_advance_pending);

  state->deactivate();
  REQUIRE_FALSE(state->currentTime());
  REQUIRE_FALSE(state->grant(pending.generation));
  REQUIRE(
      state->requestAdvance(std::make_shared<HLAinteger64Time>(11)).status ==
      FederateTimeAdvanceStatus::inactive);
}

TEST_CASE(
    "Federate time state keeps time-role enables callback-gated and blocks advances",
    "[unit][kernel][time-management][time-role]") {
  auto state = integerTimeState();

  REQUIRE(
      state->requestTimeRegulation(std::make_shared<rti1516_2025::HLAfloat64Interval>(2.0)).status ==
      FederateTimeEnableStatus::invalid_lookahead);

  auto regulation = state->requestTimeRegulation(
      std::make_shared<HLAinteger64Interval>(2));
  REQUIRE(regulation.status == FederateTimeEnableStatus::applied);
  REQUIRE(regulation.generation != 0);
  REQUIRE(
      state->requestAdvance(std::make_shared<HLAinteger64Time>(1)).status ==
      FederateTimeAdvanceStatus::time_regulation_pending);
  REQUIRE(state->currentLookahead().status == FederateTimeLookaheadStatus::not_enabled);

  REQUIRE_FALSE(state->grantTimeRegulation(regulation.generation + 1));
  auto regulationTime = state->grantTimeRegulation(regulation.generation);
  REQUIRE(regulationTime);
  REQUIRE(asIntegerTime(regulationTime).isInitial());
  auto lookahead = state->currentLookahead();
  REQUIRE(lookahead.status == FederateTimeLookaheadStatus::applied);
  REQUIRE(asIntegerInterval(lookahead.lookahead).getInterval() == 2);
  REQUIRE(
      state->requestTimeRegulation(std::make_shared<HLAinteger64Interval>(3)).status ==
      FederateTimeEnableStatus::already_enabled);
  REQUIRE(state->disableTimeRegulation() == FederateTimeDisableStatus::applied);
  REQUIRE(state->currentLookahead().status == FederateTimeLookaheadStatus::not_enabled);
  REQUIRE(state->disableTimeRegulation() == FederateTimeDisableStatus::not_enabled);

  auto advance = state->requestAdvance(std::make_shared<HLAinteger64Time>(4));
  REQUIRE(advance.status == FederateTimeAdvanceStatus::applied);
  REQUIRE(
      state->requestTimeConstrained().status ==
      FederateTimeEnableStatus::time_advance_pending);
  REQUIRE(state->grant(advance.generation));

  auto constrained = state->requestTimeConstrained();
  REQUIRE(constrained.status == FederateTimeEnableStatus::applied);
  REQUIRE(
      state->requestTimeConstrained().status ==
      FederateTimeEnableStatus::request_pending);
  REQUIRE(
      state->requestAdvance(std::make_shared<HLAinteger64Time>(5)).status ==
      FederateTimeAdvanceStatus::time_constrained_pending);
  auto constrainedTime = state->grantTimeConstrained(constrained.generation);
  REQUIRE(constrainedTime);
  REQUIRE(asIntegerTime(constrainedTime).getTime() == 4);
  REQUIRE(
      state->requestTimeConstrained().status ==
      FederateTimeEnableStatus::already_enabled);
  REQUIRE(state->disableTimeConstrained() == FederateTimeDisableStatus::applied);
  REQUIRE(state->disableTimeConstrained() == FederateTimeDisableStatus::not_enabled);

  auto staleState = integerTimeState();
  auto staleRegulation = staleState->requestTimeRegulation(
      std::make_shared<HLAinteger64Interval>(1));
  REQUIRE(staleRegulation.status == FederateTimeEnableStatus::applied);
  staleState->deactivate();
  REQUIRE_FALSE(staleState->grantTimeRegulation(staleRegulation.generation));
}
