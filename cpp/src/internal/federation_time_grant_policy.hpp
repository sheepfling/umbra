#pragma once

#include <cstdint>
#include <memory>

#include "internal/federate_time_state.hpp"
#include "internal/federation_time_bounds.hpp"

namespace umbra::detail {

// The policy result deliberately separates a safe-to-grant TAR from a request
// that must remain pending. Delivery and callback ownership belong to a later
// federation scheduler; this component has no callback or registry mutation.
enum class FederationTimeAdvanceGrantStatus {
  grant,
  wait_for_galt,
  wait_for_non_regulated_grant,
  no_time_advance_pending,
  inconsistent_temporal_state,
};

struct FederationTimeAdvanceGrantDecision {
  FederationTimeAdvanceGrantStatus status =
      FederationTimeAdvanceGrantStatus::inconsistent_temporal_state;

  [[nodiscard]] bool mayGrant() const noexcept {
    return status == FederationTimeAdvanceGrantStatus::grant;
  }
};

// Flush Queue Request has a callback-time actual grant, rather than the
// ordinary request target. Keep that calculation federation-owned and pure so
// the dispatcher and save-admission coordinator cannot disagree about whether
// a requested save boundary has been crossed.
enum class FederationFlushQueueGrantStatus {
  calculated,
  no_flush_queue_request,
  factory_unavailable,
  inconsistent_temporal_state,
};

struct FederationFlushQueueGrantCalculation {
  FederationFlushQueueGrantStatus status =
      FederationFlushQueueGrantStatus::inconsistent_temporal_state;
  std::shared_ptr<rti1516_2025::LogicalTime> grantedTime;
  std::shared_ptr<rti1516_2025::LogicalTime> optimisticTime;

  [[nodiscard]] bool calculated() const noexcept {
    return status == FederationFlushQueueGrantStatus::calculated && grantedTime &&
        optimisticTime;
  }
};

// Applies the bounded IEEE 1516.1 time-advance rules to a requesting
// federate's immutable state and a GALT/NRG calculation made for that same
// federate. Ordinary TAR/NMR forms are strict at an ordinary defined GALT;
// every advance form may use an equal queued TSO boundary, Available forms
// are additionally inclusive at an ordinary GALT, and Flush Queue bypasses
// waiting for another federate's GALT while computing its bounded grant at
// callback time. When GALT is undefined, NRG decides whether a time-
// constrained federate may advance beyond its current time. A non-time-
// constrained federate is never GALT-bounded.
class FederationTimeAdvanceGrantPolicy final {
 public:
  [[nodiscard]] FederationTimeAdvanceGrantDecision decide(
      FederateTimeSnapshot const& requester,
      FederationTimeBounds const& bounds) const;
};

class FederationFlushQueueGrantCalculator final {
 public:
  [[nodiscard]] FederationFlushQueueGrantCalculation calculate(
      FederationTimeExecutionSnapshot const& execution,
      std::uint64_t requestingFederateId) const;
};

}  // namespace umbra::detail
