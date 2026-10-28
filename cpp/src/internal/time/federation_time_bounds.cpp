#include "internal/time/federation_time_bounds.hpp"

#include "internal/fom/fom_catalog.hpp"

#include <RTI/Exception.h>
#include "internal/time/federation_time_factory_provider.hpp"

#include <algorithm>
#include <memory>

namespace umbra::detail {
namespace {

std::shared_ptr<rti1516_2025::LogicalTime const> cloneAndAdvance(
    rti1516_2025::LogicalTimeFactory& factory,
    rti1516_2025::LogicalTime const& time,
    rti1516_2025::LogicalTimeInterval const& lookahead,
    bool minimumTimestampIsExclusive) {
  auto candidate = factory.decodeLogicalTime(time.encode());
  if (!candidate || candidate->implementationName() != factory.getName()) {
    return nullptr;
  }
  *candidate += lookahead;
  if (minimumTimestampIsExclusive) {
    auto const epsilon = factory.makeEpsilon();
    if (!epsilon || epsilon->implementationName() != factory.getName()) {
      return nullptr;
    }
    *candidate += *epsilon;
  }
  std::shared_ptr<rti1516_2025::LogicalTime> shared = std::move(candidate);
  return shared;
}

}  // namespace

FederationTimeBounds FederationTimeBoundsCalculator::calculate(
    FederationTimeExecutionSnapshot const& execution,
    std::uint64_t requestingFederateId) const {
  FederationTimeBounds result;
  if (requestingFederateId == 0 || execution.definition.logicalTimeImplementationName.empty()) {
    return result;
  }

  bool requesterPresent = false;
  for (FederationTimeFederateSnapshot const& federate : execution.federates) {
    if (federate.membership.id == requestingFederateId) {
      requesterPresent = true;
      break;
    }
  }
  if (!requesterPresent) {
    result.status = FederationTimeBoundStatus::requesting_federate_not_registered;
    return result;
  }

  auto factory = umbra::detail::makeFederationLogicalTimeFactory(
      execution.definition.logicalTimeImplementationName);
  if (!factory || factory->getName() != execution.definition.logicalTimeImplementationName) {
    result.status = FederationTimeBoundStatus::factory_unavailable;
    return result;
  }

  result.nonRegulatedGrant = execution.nonRegulatedGrant;

  auto const requester = std::find_if(
      execution.federates.begin(),
      execution.federates.end(),
      [requestingFederateId](FederationTimeFederateSnapshot const& candidate) {
        return candidate.membership.id == requestingFederateId;
      });
  if (requester == execution.federates.end()) {
    return result;
  }

  bool deliveredTsoAtPendingTarBoundary = false;
  if (requester->time.timeConstrained &&
      requester->time.timeAdvancePending && requester->time.requestedTime &&
      requester->time.advanceMode ==
          FederateTimeAdvanceMode::time_advance_request &&
      requester->time.requestedTime->implementationName() ==
          execution.definition.logicalTimeImplementationName) {
    for (auto const& message : requester->deliveredTsoMessagesSinceLastAdvance) {
      if (message.recipientFederateId != requestingFederateId ||
          !message.timestamp ||
          message.timestamp->implementationName() !=
              execution.definition.logicalTimeImplementationName) {
        continue;
      }
      try {
        if (*message.timestamp == *requester->time.requestedTime) {
          deliveredTsoAtPendingTarBoundary = true;
          break;
        }
      } catch (rti1516_2025::Exception const&) {
        return result;
      }
    }
  }

  bool hasOtherRegulator = false;
  std::shared_ptr<rti1516_2025::LogicalTime const> earliestFutureIncoming;
  bool const implicitRegulationAdvance = requester->time.timeRegulationPending &&
      requester->time.timeAdvancePending &&
      requester->time.advanceMode ==
          FederateTimeAdvanceMode::time_advance_request_available;
  auto considerPendingMessage = [&](TsoQueuedMessage const& message) {
    if (message.recipientFederateId != requestingFederateId || !message.timestamp ||
        message.timestamp->isInitial() || message.timestamp->isFinal() ||
        message.timestamp->implementationName() != execution.definition.logicalTimeImplementationName) {
      return false;
    }
    if (!earliestFutureIncoming || *message.timestamp < *earliestFutureIncoming) {
      earliestFutureIncoming = message.timestamp;
    }
    // A TSO strictly below a TAR target is part of its pre-grant delivery set
    // and must not hold the request below that target. A TSO exactly at the
    // TAR target remains a GALT boundary so the coordinator can deliver it
    // before an equal-time TAG. TARA instead removes timestamps at or below
    // its request from this bound. The service's delivery dispatch still
    // delivers each admitted message before invoking TAG.
    if (requester->time.timeAdvancePending &&
        requester->time.requestedTime &&
        (requester->time.advanceMode ==
             FederateTimeAdvanceMode::time_advance_request ||
         requester->time.advanceMode ==
             FederateTimeAdvanceMode::time_advance_request_available)) {
      try {
        bool const deliveredBeforeGrant =
            requester->time.advanceMode ==
                    FederateTimeAdvanceMode::time_advance_request
                ? *message.timestamp < *requester->time.requestedTime
                : *message.timestamp <= *requester->time.requestedTime;
        if (deliveredBeforeGrant) {
          return true;
        }
      } catch (rti1516_2025::Exception const&) {
        return false;
      }
    }
    // A message strictly behind the recipient's completed logical time is
    // stale for future delivery. An equal-time message may still be waiting
    // for its public callback boundary; keep it in the bound until callback
    // acknowledgement moves it out of the in-transit set.
    if (requester->time.currentTime) {
      try {
        if (*message.timestamp < *requester->time.currentTime) {
          return true;
        }
      } catch (rti1516_2025::Exception const&) {
        return false;
      }
    }
    // The implicit Available advance must deliver every already queued TSO
    // below its target before Time Regulation Enabled. Those local payloads
    // are delivery work, not a lower grant bound; the other regulators still
    // provide the actual GALT while LITS retains the earliest queued input.
    if (implicitRegulationAdvance) {
      return true;
    }
    if (!result.galt || *message.timestamp < *result.galt) {
      result.galt = message.timestamp;
      result.galtIsTsoBoundary = true;
    }
    return true;
  };
  try {
    // A delivered entry is callback-acknowledgement history, not an incoming
    // delivery frontier. Only queued or in-transit TSO can still arrive after
    // this snapshot; pending pre-grant records are delivered before TAG, while
    // retaining an acknowledged timestamp here can pin GALT below a later
    // safe grant indefinitely.
    for (auto const& message : requester->inTransitTsoMessages) {
      if (!considerPendingMessage(message)) {
        return result;
      }
    }
    for (auto const& message : requester->queuedTsoMessages) {
      if (!considerPendingMessage(message)) {
        return result;
      }
    }

    for (FederationTimeFederateSnapshot const& federate : execution.federates) {
      FederateTimeSnapshot const& temporal = federate.time;
      if (federate.membership.id == requestingFederateId || !temporal.active ||
          !temporal.timeRegulating) {
        continue;
      }
      if (temporal.implementationName != execution.definition.logicalTimeImplementationName ||
          !temporal.currentTime || !temporal.lookahead) {
        return result;
      }

      rti1516_2025::LogicalTime const* baseTime = temporal.currentTime.get();
      if (temporal.timeAdvancePending) {
        auto const& requestBoundary = temporal.advanceRequestTime
            ? temporal.advanceRequestTime
            : temporal.requestedTime;
        if (!requestBoundary) {
          return result;
        }
        baseTime = requestBoundary.get();
      }
      if (baseTime == nullptr ||
          baseTime->implementationName() != execution.definition.logicalTimeImplementationName ||
          temporal.lookahead->implementationName() != execution.definition.logicalTimeImplementationName) {
        return result;
      }

      auto candidate = cloneAndAdvance(
          *factory,
          *baseTime,
          *temporal.lookahead,
          temporal.minimumTimestampIsExclusive);
      if (!candidate) {
        return result;
      }
      hasOtherRegulator = true;
      if (!result.galt || *candidate < *result.galt) {
        result.galt = std::move(candidate);
        result.galtIsTsoBoundary = false;
      }
    }
  } catch (rti1516_2025::Exception const&) {
    return result;
  }

  if (!hasOtherRegulator) {
    result.lits = std::move(earliestFutureIncoming);
    result.galt.reset();
    result.status = FederationTimeBoundStatus::undefined;
    return result;
  }
  if (!result.galt) {
    return result;
  }

  result.lits = result.galt;
  if (earliestFutureIncoming && *earliestFutureIncoming < *result.lits) {
    result.lits = std::move(earliestFutureIncoming);
  }
  result.status = FederationTimeBoundStatus::available;
  if (deliveredTsoAtPendingTarBoundary && !result.galtIsTsoBoundary) {
    try {
      result.galtIsDeliveredTsoBoundary =
          *result.galt == *requester->time.requestedTime;
    } catch (rti1516_2025::Exception const&) {
      return FederationTimeBounds{};
    }
  }
  return result;
}

}  // namespace umbra::detail
