#include <catch2/catch_test_macros.hpp>

#include "internal/federate_time_state.hpp"

#include <memory>

#include <RTI/time/HLAfloat64Interval.h>
#include <RTI/time/HLAfloat64Time.h>
#include <RTI/time/HLAinteger64Interval.h>
#include <RTI/time/HLAinteger64Time.h>

namespace {

using umbra::detail::FederateTimeAdvanceStatus;
using umbra::detail::FederateTimeDisableStatus;
using umbra::detail::FederateTimeEnableStatus;
using umbra::detail::FederateTimeLookaheadStatus;
using umbra::detail::FederateTimeState;
using rti1516_2025::HLAfloat64Time;
using rti1516_2025::HLAinteger64Interval;
using rti1516_2025::HLAinteger64Time;

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
    "Federate time state retains the initial value until its matching grant",
    "[unit][kernel][time-management]") {
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
    "[unit][kernel][time-management]") {
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
