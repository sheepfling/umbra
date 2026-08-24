#include "internal/time/federation_time_bounds.hpp"

#include "internal/fom/fom_catalog.hpp"

#include <RTI/Exception.h>
#include <RTI/time/HLAlogicalTimeFactoryFactory.h>

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
    auto epsilon = factory.makeEpsilon();
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

  auto factory = rti1516_2025::HLAlogicalTimeFactoryFactory::makeLogicalTimeFactory(
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

  bool hasOtherRegulator = false;
  std::shared_ptr<rti1516_2025::LogicalTime const> earliestFutureIncoming;
  auto considerMessage = [&](TsoQueuedMessage const& message, bool future) {
    if (message.recipientFederateId != requestingFederateId || !message.timestamp ||
        message.timestamp->isInitial() || message.timestamp->isFinal() ||
        message.timestamp->implementationName() != execution.definition.logicalTimeImplementationName) {
      return false;
    }
    // A delivered message can remain in the snapshot until the next grant so
    // the coordinator can expose its ordering/retraction boundary. Once the
    // recipient's current logical time has reached that timestamp, it must no
    // longer hold GALT at the old boundary for a subsequent advance.
    if (!future && requester->time.currentTime) {
      try {
        if (*message.timestamp <= *requester->time.currentTime) {
          return true;
        }
      } catch (rti1516_2025::Exception const&) {
        return false;
      }
    }
    if (future && (!earliestFutureIncoming || *message.timestamp < *earliestFutureIncoming)) {
      earliestFutureIncoming = message.timestamp;
    }
    if (!future && (!result.galt || *message.timestamp < *result.galt)) {
      result.galt = message.timestamp;
      result.galtIsTsoBoundary = true;
    }
    if (future && (!result.galt || *message.timestamp < *result.galt)) {
      result.galt = message.timestamp;
      result.galtIsTsoBoundary = true;
    }
    return true;
  };
  try {
    for (auto const& message : requester->deliveredTsoMessagesSinceLastAdvance) {
      if (!considerMessage(message, false)) {
        return result;
      }
    }
    for (auto const& message : requester->inTransitTsoMessages) {
      if (!considerMessage(message, true)) {
        return result;
      }
    }
    for (auto const& message : requester->queuedTsoMessages) {
      if (!considerMessage(message, true)) {
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
  return result;
}

}  // namespace umbra::detail
