#pragma once

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

// Applies the no-TSO subset of the IEEE 1516.1 time-advance rules to a
// requesting federate's immutable state and a GALT/NRG calculation made for
// that same federate. TAR is strict at a defined GALT: it may be granted only
// below the bound. When GALT is undefined, NRG decides whether a
// time-constrained federate may advance beyond its current time. A
// non-time-constrained federate is never GALT-bounded.
class FederationTimeAdvanceGrantPolicy final {
 public:
  [[nodiscard]] FederationTimeAdvanceGrantDecision decide(
      FederateTimeSnapshot const& requester,
      FederationTimeBounds const& bounds) const;
};

}  // namespace umbra::detail
