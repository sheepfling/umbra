#include <catch2/catch_test_macros.hpp>

#include "internal/federation_time_bounds.hpp"

#include <cstdint>
#include <memory>
#include <utility>
#include <vector>

#include <RTI/time/HLAinteger64Interval.h>
#include <RTI/time/HLAinteger64Time.h>

namespace {

using umbra::detail::FederateMembership;
using umbra::detail::FederateTimeEnableStatus;
using umbra::detail::FederateTimeState;
using umbra::detail::FederationDefinition;
using umbra::detail::FederationTimeBoundStatus;
using umbra::detail::FederationTimeBoundsCalculator;
using umbra::detail::FederationTimeExecutionSnapshot;
using umbra::detail::FederationTimeFederateSnapshot;
using rti1516_2025::HLAinteger64Interval;
using rti1516_2025::HLAinteger64Time;

std::shared_ptr<FederateTimeState> integerTimeState() {
  return std::make_shared<FederateTimeState>(
      L"HLAinteger64Time",
      std::make_shared<HLAinteger64Time>());
}

void advanceTo(FederateTimeState& state, std::int64_t time) {
  auto request = state.requestAdvance(std::make_shared<HLAinteger64Time>(time));
  REQUIRE(request.generation != 0);
  REQUIRE(state.grant(request.generation));
}

void enableRegulation(FederateTimeState& state, std::int64_t lookahead) {
  auto request = state.requestTimeRegulation(std::make_shared<HLAinteger64Interval>(lookahead));
  REQUIRE(request.status == FederateTimeEnableStatus::applied);
  REQUIRE(state.grantTimeRegulation(request.generation));
}

HLAinteger64Time const& asIntegerTime(
    std::shared_ptr<rti1516_2025::LogicalTime const> const& time) {
  auto const* value = dynamic_cast<HLAinteger64Time const*>(time.get());
  REQUIRE(value != nullptr);
  return *value;
}

FederationTimeExecutionSnapshot snapshot(
    std::vector<std::pair<std::uint64_t, std::shared_ptr<FederateTimeState>>> const& states) {
  FederationTimeExecutionSnapshot result{
      FederationDefinition{{}, L"HLAinteger64Time", {}, {}},
      {},
  };
  for (auto const& [id, state] : states) {
    result.federates.push_back({{id, L"federate-" + std::to_wstring(id), L"test"}, state->snapshot()});
  }
  return result;
}

}  // namespace

TEST_CASE(
    "No-TSO federation time bounds use other regulators current or pending time plus lookahead",
    "[unit][kernel][time-management][federation-time][galt][lits]") {
  auto recipient = integerTimeState();
  auto nearRegulator = integerTimeState();
  auto advancingRegulator = integerTimeState();
  advanceTo(*nearRegulator, 4);
  enableRegulation(*nearRegulator, 2);
  enableRegulation(*advancingRegulator, 3);
  auto pendingAdvance = advancingRegulator->requestAdvance(std::make_shared<HLAinteger64Time>(8));
  REQUIRE(pendingAdvance.generation != 0);

  auto execution = snapshot({
      {1, recipient},
      {2, nearRegulator},
      {3, advancingRegulator},
  });
  FederationTimeBoundsCalculator calculator;
  auto bounds = calculator.calculate(execution, 1);

  REQUIRE(bounds.status == FederationTimeBoundStatus::available);
  REQUIRE(bounds.galt);
  REQUIRE(bounds.lits);
  // min(4 + 2, pending(8) + 3) == 6.  The current no-TSO profile has no
  // queued receive timestamp that could make LITS lower than GALT.
  REQUIRE(asIntegerTime(bounds.galt).getTime() == 6);
  REQUIRE(asIntegerTime(bounds.lits).getTime() == 6);
  REQUIRE_FALSE(bounds.nonRegulatedGrant);

  auto selfOnly = calculator.calculate(execution, 2);
  REQUIRE(selfOnly.status == FederationTimeBoundStatus::available);
  REQUIRE(asIntegerTime(selfOnly.galt).getTime() == 11);
}

TEST_CASE(
    "No-TSO federation time bounds use a time-advancing regulator requested time",
    "[unit][kernel][time-management][federation-time][galt][lits][time-advance]") {
  auto recipient = integerTimeState();
  auto regulator = integerTimeState();
  advanceTo(*regulator, 4);
  enableRegulation(*regulator, 2);
  auto pendingAdvance = regulator->requestAdvance(std::make_shared<HLAinteger64Time>(10));
  REQUIRE(pendingAdvance.generation != 0);

  auto execution = snapshot({
      {1, recipient},
      {2, regulator},
  });
  auto bounds = FederationTimeBoundsCalculator{}.calculate(execution, 1);

  REQUIRE(bounds.status == FederationTimeBoundStatus::available);
  REQUIRE(bounds.galt);
  REQUIRE(bounds.lits);
  // In the Time Advancing state, the regulator's requested time—not its
  // previous current time—sets the no-TSO lower timestamp candidate.  With no
  // queued or in-transit TSO messages in this profile: 10 + 2 == 12.
  REQUIRE(asIntegerTime(bounds.galt).getTime() == 12);
  REQUIRE(asIntegerTime(bounds.lits).getTime() == 12);
}

TEST_CASE(
    "No-TSO federation time bounds are undefined without another regulator",
    "[unit][kernel][time-management][federation-time][galt][lits]") {
  auto onlyFederate = integerTimeState();
  enableRegulation(*onlyFederate, 1);
  auto execution = snapshot({{1, onlyFederate}});

  FederationTimeBoundsCalculator calculator;
  auto bounds = calculator.calculate(execution, 1);
  REQUIRE(bounds.status == FederationTimeBoundStatus::undefined);
  REQUIRE_FALSE(bounds.galt);
  REQUIRE_FALSE(bounds.lits);

  auto absent = calculator.calculate(execution, 2);
  REQUIRE(absent.status == FederationTimeBoundStatus::requesting_federate_not_registered);
}

TEST_CASE(
    "No-TSO federation time bounds honor a zero-lookahead TAR exclusive boundary",
    "[unit][kernel][time-management][federation-time][galt][lits][zero-lookahead]") {
  auto recipient = integerTimeState();
  auto zeroLookaheadRegulator = integerTimeState();
  enableRegulation(*zeroLookaheadRegulator, 0);

  auto advance = zeroLookaheadRegulator->requestAdvance(std::make_shared<HLAinteger64Time>(5));
  REQUIRE(advance.generation != 0);

  auto execution = snapshot({
      {1, recipient},
      {2, zeroLookaheadRegulator},
  });
  FederationTimeBoundsCalculator calculator;

  auto pendingBounds = calculator.calculate(execution, 1);
  REQUIRE(pendingBounds.status == FederationTimeBoundStatus::available);
  // A zero-lookahead TAR makes timestamp 5 unavailable while the request is
  // pending, so the earliest possible integer timestamp is 6.
  REQUIRE(asIntegerTime(pendingBounds.galt).getTime() == 6);
  REQUIRE(asIntegerTime(pendingBounds.lits).getTime() == 6);

  REQUIRE(zeroLookaheadRegulator->grant(advance.generation));
  auto grantedBounds = calculator.calculate(execution, 1);
  // The TAR restriction remains after its grant.
  REQUIRE(grantedBounds.status == FederationTimeBoundStatus::available);
  REQUIRE(asIntegerTime(grantedBounds.galt).getTime() == 6);
  REQUIRE(asIntegerTime(grantedBounds.lits).getTime() == 6);
}

TEST_CASE(
    "Federation time bounds include queued and in-transit TSO timestamps",
    "[unit][kernel][time-management][federation-time][galt][lits][tso]") {
  auto recipient = integerTimeState();
  auto regulator = integerTimeState();
  advanceTo(*regulator, 4);
  enableRegulation(*regulator, 8);

  auto execution = snapshot({{1, recipient}, {2, regulator}});
  execution.federates.front().queuedTsoMessages.push_back({
      11,
      1,
      2,
      std::make_shared<HLAinteger64Time const>(9),
  });
  execution.federates.front().inTransitTsoMessages.push_back({
      11,
      1,
      1,
      std::make_shared<HLAinteger64Time const>(7),
  });

  auto bounds = FederationTimeBoundsCalculator{}.calculate(execution, 1);
  REQUIRE(bounds.status == FederationTimeBoundStatus::available);
  // The other regulator contributes 12; the in-transit TSO timestamp lowers
  // both GALT and LITS to 7, while the later queued entry remains pending.
  REQUIRE(asIntegerTime(bounds.galt).getTime() == 7);
  REQUIRE(asIntegerTime(bounds.lits).getTime() == 7);

  execution.federates.front().deliveredTsoMessagesSinceLastAdvance.push_back({
      12,
      1,
      3,
      std::make_shared<HLAinteger64Time const>(5),
  });
  bounds = FederationTimeBoundsCalculator{}.calculate(execution, 1);
  REQUIRE(bounds.status == FederationTimeBoundStatus::available);
  REQUIRE(asIntegerTime(bounds.galt).getTime() == 5);
  REQUIRE(asIntegerTime(bounds.lits).getTime() == 5);
}

TEST_CASE(
    "LITS remains defined from queued TSO state when GALT is unregulated",
    "[unit][kernel][time-management][federation-time][galt][lits][tso][nrg]") {
  auto recipient = integerTimeState();
  auto execution = snapshot({{1, recipient}});
  execution.federates.front().queuedTsoMessages.push_back({
      17,
      1,
      1,
      std::make_shared<HLAinteger64Time const>(11),
  });

  auto bounds = FederationTimeBoundsCalculator{}.calculate(execution, 1);
  REQUIRE(bounds.status == FederationTimeBoundStatus::undefined);
  REQUIRE_FALSE(bounds.galt);
  REQUIRE(bounds.lits);
  REQUIRE(asIntegerTime(bounds.lits).getTime() == 11);
}

TEST_CASE(
    "Delivered TSO at the recipient's current time no longer pins a later GALT",
    "[unit][kernel][time-management][federation-time][galt][lits][tso]") {
  auto recipient = integerTimeState();
  auto regulator = integerTimeState();
  advanceTo(*recipient, 7);
  advanceTo(*regulator, 4);
  enableRegulation(*regulator, 8);

  auto execution = snapshot({{1, recipient}, {2, regulator}});
  execution.federates.front().deliveredTsoMessagesSinceLastAdvance.push_back({
      41,
      1,
      1,
      std::make_shared<HLAinteger64Time const>(7),
  });

  auto bounds = FederationTimeBoundsCalculator{}.calculate(execution, 1);
  REQUIRE(bounds.status == FederationTimeBoundStatus::available);
  REQUIRE(bounds.galt);
  REQUIRE(asIntegerTime(bounds.galt).getTime() == 12);
}
