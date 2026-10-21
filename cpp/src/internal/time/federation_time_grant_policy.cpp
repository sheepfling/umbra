#include "internal/time/federation_time_grant_policy.hpp"

#include <RTI/Exception.h>
#include <RTI/time/HLAlogicalTimeFactoryFactory.h>

#include <algorithm>

namespace umbra::detail {
namespace {

std::shared_ptr<rti1516_2025::LogicalTime> cloneLogicalTime(
    rti1516_2025::LogicalTimeFactory& factory,
    rti1516_2025::LogicalTime const& source) {
  auto clone = factory.decodeLogicalTime(source.encode());
  if (!clone || clone->implementationName() != factory.getName()) {
    return nullptr;
  }
  std::shared_ptr<rti1516_2025::LogicalTime> shared = std::move(clone);
  return shared;
}

}  // namespace

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

  // Flush Queue Request deliberately does not wait for another federate's
  // GALT. Its grant is computed from the requester's current bound and the
  // queued/in-transit delivery set by the callback dispatcher.
  if (requester.advanceMode == FederateTimeAdvanceMode::flush_queue_request) {
    return {FederationTimeAdvanceGrantStatus::grant};
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
          bool const inclusiveBoundary =
              requester.advanceMode == FederateTimeAdvanceMode::time_advance_request_available ||
              requester.advanceMode == FederateTimeAdvanceMode::next_message_request_available;
          // A queued/in-transit TSO boundary is already a known delivery
          // point. Preserve the established exact-bound behavior for all
          // advance forms when that queue-derived boundary is the selected
          // GALT; the Available forms additionally permit equality at an
          // ordinary regulator-only GALT.
          bool const queuedTsoBoundary = atTsoBoundary;
          return {
              (*requester.requestedTime < *bounds.galt || inclusiveBoundary &&
                  *requester.requestedTime == *bounds.galt || queuedTsoBoundary)
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

FederationFlushQueueGrantCalculation FederationFlushQueueGrantCalculator::calculate(
    FederationTimeExecutionSnapshot const& execution,
    std::uint64_t requestingFederateId) const {
  return calculateImpl(execution, requestingFederateId, nullptr, false);
}

FederationFlushQueueGrantCalculation
FederationFlushQueueGrantCalculator::calculateAfterTsoCallbacks(
    FederationTimeExecutionSnapshot const& execution,
    std::uint64_t requestingFederateId,
    std::shared_ptr<rti1516_2025::LogicalTime const>
        earliestDeliveredTsoTimestamp) const {
  return calculateImpl(
      execution,
      requestingFederateId,
      std::move(earliestDeliveredTsoTimestamp),
      true);
}

FederationFlushQueueGrantCalculation
FederationFlushQueueGrantCalculator::calculateImpl(
    FederationTimeExecutionSnapshot const& execution,
    std::uint64_t requestingFederateId,
    std::shared_ptr<rti1516_2025::LogicalTime const> deliveredTsoFrontier,
    bool useDeliveredTsoFrontier) const {
  FederationFlushQueueGrantCalculation result;
  if (requestingFederateId == 0 || execution.definition.logicalTimeImplementationName.empty()) {
    return result;
  }

  auto const requester = std::find_if(
      execution.federates.begin(),
      execution.federates.end(),
      [requestingFederateId](FederationTimeFederateSnapshot const& candidate) {
        return candidate.membership.id == requestingFederateId;
      });
  if (requester == execution.federates.end() || !requester->time.active ||
      !requester->time.timeAdvancePending ||
      requester->time.advanceMode != FederateTimeAdvanceMode::flush_queue_request) {
    result.status = FederationFlushQueueGrantStatus::no_flush_queue_request;
    return result;
  }

  auto const& time = requester->time;
  if (time.implementationName != execution.definition.logicalTimeImplementationName ||
      !time.currentTime || !time.requestedTime ||
      time.currentTime->implementationName() != time.implementationName ||
      time.requestedTime->implementationName() != time.implementationName) {
    return result;
  }

  auto factory = rti1516_2025::HLAlogicalTimeFactoryFactory::makeLogicalTimeFactory(
      time.implementationName);
  if (!factory || factory->getName() != time.implementationName) {
    result.status = FederationFlushQueueGrantStatus::factory_unavailable;
    return result;
  }

  auto const bounds = FederationTimeBoundsCalculator{}.calculate(execution, requestingFederateId);
  if (bounds.status == FederationTimeBoundStatus::factory_unavailable ||
      bounds.status == FederationTimeBoundStatus::inconsistent_temporal_state ||
      bounds.status == FederationTimeBoundStatus::requesting_federate_not_registered) {
    return result;
  }
  if (bounds.status == FederationTimeBoundStatus::available &&
      (!bounds.galt || bounds.galt->implementationName() != time.implementationName)) {
    return result;
  }

  try {
    // Provisional callers use all queued and in-transit messages as a safe
    // frontier. The public embedded FQR callback boundary instead supplies
    // the earliest timestamp whose federate callback actually began; a
    // retracted or recipient-suppressed candidate must not constrain either
    // Flush Queue Grant time.
    std::shared_ptr<rti1516_2025::LogicalTime const> timeFrontier;
    if (useDeliveredTsoFrontier) {
      if (deliveredTsoFrontier &&
          deliveredTsoFrontier->implementationName() != time.implementationName) {
        return result;
      }
      timeFrontier = std::move(deliveredTsoFrontier);
    }
    auto considerUndelivered = [&](std::vector<TsoQueuedMessage> const& messages) {
      for (auto const& message : messages) {
        if (!message.timestamp ||
            message.timestamp->implementationName() != time.implementationName) {
          return false;
        }
        if (!timeFrontier || *message.timestamp < *timeFrontier) {
          timeFrontier = message.timestamp;
        }
      }
      return true;
    };
    if (!useDeliveredTsoFrontier &&
        (!considerUndelivered(requester->queuedTsoMessages) ||
         !considerUndelivered(requester->inTransitTsoMessages))) {
      return result;
    }

    auto granted = cloneLogicalTime(*factory, *time.requestedTime);
    auto optimistic = cloneLogicalTime(*factory, *time.requestedTime);
    if (!granted || !optimistic) {
      result.status = FederationFlushQueueGrantStatus::factory_unavailable;
      return result;
    }
    if (timeFrontier && *timeFrontier < *optimistic) {
      optimistic = cloneLogicalTime(*factory, *timeFrontier);
      if (!optimistic) {
        result.status = FederationFlushQueueGrantStatus::factory_unavailable;
        return result;
      }
    }
    if (bounds.status == FederationTimeBoundStatus::available && *bounds.galt < *granted) {
      granted = cloneLogicalTime(*factory, *bounds.galt);
      if (!granted) {
        result.status = FederationFlushQueueGrantStatus::factory_unavailable;
        return result;
      }
    }
    if (timeFrontier && *timeFrontier < *granted) {
      granted = cloneLogicalTime(*factory, *timeFrontier);
      if (!granted) {
        result.status = FederationFlushQueueGrantStatus::factory_unavailable;
        return result;
      }
    }
    if (bounds.status == FederationTimeBoundStatus::undefined &&
        time.timeConstrained && !bounds.nonRegulatedGrant) {
      // FQR still completes without waiting for a regulator when GALT is
      // undefined, but disabled NRG forbids a time-constrained federate from
      // being granted beyond its current logical time. Keep the optimistic
      // Flush Queue Grant time derived from the request/queued TSO frontier;
      // this cap affects only the actual grant.
      granted = cloneLogicalTime(*factory, *time.currentTime);
      if (!granted) {
        result.status = FederationFlushQueueGrantStatus::factory_unavailable;
        return result;
      }
    }
    if (*granted < *time.currentTime) {
      granted = cloneLogicalTime(*factory, *time.currentTime);
      if (!granted) {
        result.status = FederationFlushQueueGrantStatus::factory_unavailable;
        return result;
      }
    }
    if (*optimistic < *granted) {
      optimistic = cloneLogicalTime(*factory, *granted);
      if (!optimistic) {
        result.status = FederationFlushQueueGrantStatus::factory_unavailable;
        return result;
      }
    }

    result.status = FederationFlushQueueGrantStatus::calculated;
    result.grantedTime = std::move(granted);
    result.optimisticTime = std::move(optimistic);
  } catch (rti1516_2025::Exception const&) {
    result.status = FederationFlushQueueGrantStatus::inconsistent_temporal_state;
  }
  return result;
}

}  // namespace umbra::detail
