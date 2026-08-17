#include <catch2/catch_test_macros.hpp>

#include "internal/federation_time_grant_policy.hpp"

#include <memory>

#include <RTI/time/HLAinteger64Time.h>

namespace {

using umbra::detail::FederateTimeSnapshot;
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
