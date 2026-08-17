#include "internal/federation_time_grant_policy.hpp"

#include <RTI/Exception.h>

namespace umbra::detail {

FederationTimeAdvanceGrantDecision FederationTimeAdvanceGrantPolicy::decide(
    FederateTimeSnapshot const& requester,
    FederationTimeBounds const& bounds) const {
  if (!requester.active || !requester.timeAdvancePending) {
    return {FederationTimeAdvanceGrantStatus::no_time_advance_pending};
  }
  if (requester.implementationName.empty() || !requester.currentTime || !requester.requestedTime ||
      requester.currentTime->implementationName() != requester.implementationName ||
      requester.requestedTime->implementationName() != requester.implementationName) {
    return {FederationTimeAdvanceGrantStatus::inconsistent_temporal_state};
  }

  // GALT bounds only time-constrained joined federates. The state-request
  // invariant has already ensured that this is a valid nondecreasing TAR.
  if (!requester.timeConstrained) {
    return {FederationTimeAdvanceGrantStatus::grant};
  }

  try {
    switch (bounds.status) {
      case FederationTimeBoundStatus::available:
        if (!bounds.galt ||
            bounds.galt->implementationName() != requester.implementationName) {
          return {FederationTimeAdvanceGrantStatus::inconsistent_temporal_state};
        }
        {
          bool const atTsoBoundary = bounds.galtIsTsoBoundary &&
              *requester.requestedTime == *bounds.galt;
          return {
              (*requester.requestedTime < *bounds.galt || atTsoBoundary)
                  ? FederationTimeAdvanceGrantStatus::grant
                  : FederationTimeAdvanceGrantStatus::wait_for_galt,
          };
        }

      case FederationTimeBoundStatus::undefined:
        if (bounds.nonRegulatedGrant) {
          return {FederationTimeAdvanceGrantStatus::grant};
        }
        return {
            *requester.requestedTime <= *requester.currentTime
                ? FederationTimeAdvanceGrantStatus::grant
                : FederationTimeAdvanceGrantStatus::wait_for_non_regulated_grant,
        };

      case FederationTimeBoundStatus::requesting_federate_not_registered:
      case FederationTimeBoundStatus::factory_unavailable:
      case FederationTimeBoundStatus::inconsistent_temporal_state:
        return {FederationTimeAdvanceGrantStatus::inconsistent_temporal_state};
    }
  } catch (rti1516_2025::Exception const&) {
    return {FederationTimeAdvanceGrantStatus::inconsistent_temporal_state};
  }

  return {FederationTimeAdvanceGrantStatus::inconsistent_temporal_state};
}

}  // namespace umbra::detail
