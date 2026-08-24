#include <catch2/catch_test_macros.hpp>

#include "internal/time/federation_time_grant_policy.hpp"

#include <cstdint>
#include <memory>
#include <utility>

#include <RTI/time/HLAinteger64Time.h>

namespace {

using umbra::detail::FederateTimeSnapshot;
using umbra::detail::FederateTimeAdvanceMode;
using umbra::detail::FederationDefinition;
using umbra::detail::FederationFlushQueueGrantCalculator;
using umbra::detail::FederationTimeExecutionSnapshot;
using umbra::detail::FederationTimeFederateSnapshot;
using umbra::detail::FederationTimeAdvanceGrantPolicy;
using umbra::detail::FederationTimeAdvanceGrantStatus;
using umbra::detail::FederationTimeBoundStatus;
using umbra::detail::FederationTimeBounds;
using rti1516_2025::HLAinteger64Time;

FederateTimeSnapshot pendingRequester(
    bool timeConstrained,
    std::int64_t currentTime,
    std::int64_t requestedTime) {
  FederateTimeSnapshot result;
  result.implementationName = L"HLAinteger64Time";
  result.active = true;
  result.timeConstrained = timeConstrained;
  result.timeAdvancePending = true;
  result.currentTime = std::make_shared<HLAinteger64Time>(currentTime);
  result.requestedTime = std::make_shared<HLAinteger64Time>(requestedTime);
  return result;
}

FederationTimeBounds definedBounds(std::int64_t galt) {
  FederationTimeBounds result;
  result.status = FederationTimeBoundStatus::available;
  result.galt = std::make_shared<HLAinteger64Time>(galt);
  result.lits = result.galt;
  return result;
}

FederationTimeBounds undefinedBounds(bool nonRegulatedGrant) {
  FederationTimeBounds result;
  result.status = FederationTimeBoundStatus::undefined;
  result.nonRegulatedGrant = nonRegulatedGrant;
  return result;
}

HLAinteger64Time const& asIntegerTime(
    std::shared_ptr<rti1516_2025::LogicalTime> const& time) {
  auto const* value = dynamic_cast<HLAinteger64Time const*>(time.get());
  REQUIRE(value != nullptr);
  return *value;
}

}  // namespace

TEST_CASE(
    "No-TSO TAR grant policy applies the strict defined-GALT boundary",
    "[unit][kernel][time-management][galt][time-advance-grant]") {
  FederationTimeAdvanceGrantPolicy policy;
  auto requester = pendingRequester(true, 2, 5);

  REQUIRE(policy.decide(requester, definedBounds(6)).mayGrant());
  REQUIRE(
      policy.decide(requester, definedBounds(5)).status ==
      FederationTimeAdvanceGrantStatus::wait_for_galt);
  REQUIRE(
      policy.decide(requester, definedBounds(4)).status ==
      FederationTimeAdvanceGrantStatus::wait_for_galt);
}

TEST_CASE(
    "Available time-advance forms use the inclusive defined-GALT boundary",
    "[unit][kernel][time-management][galt][time-advance-request-available]"
    "[next-message-request-available]") {
  FederationTimeAdvanceGrantPolicy policy;
  auto requester = pendingRequester(true, 2, 5);

  requester.advanceMode = FederateTimeAdvanceMode::time_advance_request_available;
  REQUIRE(policy.decide(requester, definedBounds(5)).mayGrant());

  requester.advanceMode = FederateTimeAdvanceMode::next_message_request_available;
  REQUIRE(policy.decide(requester, definedBounds(5)).mayGrant());
}

TEST_CASE(
    "Next Message Request keeps a strict GALT boundary unless it is a queued TSO boundary",
    "[unit][kernel][time-management][galt][next-message-request]") {
  FederationTimeAdvanceGrantPolicy policy;
  auto requester = pendingRequester(true, 2, 5);
  requester.advanceMode = FederateTimeAdvanceMode::next_message_request;

  REQUIRE(
      policy.decide(requester, definedBounds(5)).status ==
      FederationTimeAdvanceGrantStatus::wait_for_galt);

  auto queuedBoundary = definedBounds(5);
  queuedBoundary.galtIsTsoBoundary = true;
  REQUIRE(policy.decide(requester, queuedBoundary).mayGrant());
}

TEST_CASE(
    "Flush Queue Request does not wait for another federate's GALT",
    "[unit][kernel][time-management][galt][flush-queue-request]") {
  FederationTimeAdvanceGrantPolicy policy;
  auto requester = pendingRequester(true, 2, 10);
  requester.advanceMode = FederateTimeAdvanceMode::flush_queue_request;

  REQUIRE(policy.decide(requester, definedBounds(3)).mayGrant());
  REQUIRE(policy.decide(requester, undefinedBounds(false)).mayGrant());
}

TEST_CASE(
    "Flush Queue grant calculation includes in-transit TSO payloads",
    "[unit][kernel][time-management][flush-queue-request][tso][in-transit]") {
  auto requester = pendingRequester(true, 2, 10);
  requester.advanceMode = FederateTimeAdvanceMode::flush_queue_request;

  FederationTimeExecutionSnapshot execution{
      FederationDefinition{{}, L"HLAinteger64Time", {}, {}},
      {},
  };
  FederationTimeFederateSnapshot federate;
  federate.membership = {1, L"requester", L"time-constrained"};
  federate.time = std::move(requester);
  federate.inTransitTsoMessages.push_back({
      41,
      1,
      1,
      std::make_shared<HLAinteger64Time const>(4),
  });
  execution.federates.push_back(std::move(federate));

  auto calculation = FederationFlushQueueGrantCalculator{}.calculate(execution, 1);
  REQUIRE(calculation.calculated());
  REQUIRE(asIntegerTime(calculation.grantedTime).getTime() == 4);
  REQUIRE(asIntegerTime(calculation.optimisticTime).getTime() == 4);
}

TEST_CASE(
    "No-TSO TAR grant policy applies NRG when GALT is undefined",
    "[unit][kernel][time-management][galt][non-regulated-grant][time-advance-grant]") {
  FederationTimeAdvanceGrantPolicy policy;
  auto requestingAdvance = pendingRequester(true, 2, 5);
  auto requestingCurrentTime = pendingRequester(true, 2, 2);

  REQUIRE(
      policy.decide(requestingAdvance, undefinedBounds(false)).status ==
      FederationTimeAdvanceGrantStatus::wait_for_non_regulated_grant);
  REQUIRE(policy.decide(requestingCurrentTime, undefinedBounds(false)).mayGrant());
  REQUIRE(policy.decide(requestingAdvance, undefinedBounds(true)).mayGrant());
}

TEST_CASE(
    "No-TSO TAR grant policy never GALT-bounds a nonconstrained federate",
    "[unit][kernel][time-management][galt][time-advance-grant]") {
  FederationTimeAdvanceGrantPolicy policy;
  auto requester = pendingRequester(false, 2, 100);

  REQUIRE(policy.decide(requester, definedBounds(3)).mayGrant());
  REQUIRE(policy.decide(requester, undefinedBounds(false)).mayGrant());
}

TEST_CASE(
    "No-TSO TAR grant policy rejects incomplete pending state",
    "[unit][kernel][time-management][time-advance-grant]") {
  FederationTimeAdvanceGrantPolicy policy;
  auto requester = pendingRequester(true, 2, 5);
  requester.requestedTime.reset();

  REQUIRE(
      policy.decide(requester, definedBounds(6)).status ==
      FederationTimeAdvanceGrantStatus::inconsistent_temporal_state);

  requester = pendingRequester(true, 2, 5);
  requester.timeAdvancePending = false;
  REQUIRE(
      policy.decide(requester, definedBounds(6)).status ==
      FederationTimeAdvanceGrantStatus::no_time_advance_pending);
}
