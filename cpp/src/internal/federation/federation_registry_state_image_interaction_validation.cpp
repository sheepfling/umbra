#include "internal/federation/federation_registry_state_image_validation.hpp"

#include <RTI/Enums.h>

#include <algorithm>
#include <cstdint>
#include <iterator>
#include <map>
#include <optional>
#include <set>
#include <utility>

namespace umbra::detail::federation_registry_state_image_validation {

// Interaction declaration state is federation-owned, while callback routes
// are process-local.  The first fresh-registry slices admit one standalone
// one or two published interaction classes (the one-class form may carry one
// same-class subscription, one committed transportation-type override, and/or
// one pending transportation-type change), one standalone subscription, one
// standalone regional subscription, or one two-member directed
// publication/subscription pair; all broader declaration combinations remain
// behind their explicit lanes.
bool isRouteFreeInteractionDeclarationImage(FederationStateImage const& image) {
  if (image.interactionDeclarationCount != image.interactionDeclarations.size() ||
      image.interactionDeclarations.size() != 1U ||
      !image.objects.empty() ||
      !image.tsoInteractionMessages.empty() ||
      !image.tsoAttributeUpdateMessages.empty() ||
      !image.tsoObjectDeletionMessages.empty() ||
      !image.tsoDirectedInteractionMessages.empty() ||
      !image.tsoRequestRetractionRecords.empty() ||
      !image.tsoQueueEntries.empty()) {
    return false;
  }

  auto const& declaration = image.interactionDeclarations.front();
  if (declaration.federateId == 0U ||
      declaration.publishedInteractionClasses.size() > 2U ||
      declaration.subscribedInteractionClasses.size() > 1U ||
      declaration.regionalSubscribedInteractionClasses.size() > 1U ||
      !declaration.publishedObjectClassDirectedInteractions.empty() ||
      !declaration.subscribedObjectClassDirectedInteractions.empty() ||
      declaration.interactionTransportationTypes.size() > 1U ||
      !declaration.interactionOrderTypes.empty() ||
      declaration.pendingInteractionTransportationTypeChanges.size() > 1U) {
    return false;
  }

  if (declaration.publishedInteractionClasses.size() == 2U) {
    auto const first = declaration.publishedInteractionClasses[0];
    auto const second = declaration.publishedInteractionClasses[1];
    return first != 0U && second > first &&
        declaration.subscribedInteractionClasses.empty() &&
        declaration.regionalSubscribedInteractionClasses.empty() &&
        declaration.interactionTransportationTypes.empty() &&
        declaration.pendingInteractionTransportationTypeChanges.empty();
  }

  auto const publishedInteractionClassHandle =
      declaration.publishedInteractionClasses.empty()
      ? 0U
      : declaration.publishedInteractionClasses.front();
  auto const subscribedInteractionClassHandle =
      declaration.subscribedInteractionClasses.empty()
      ? 0U
      : declaration.subscribedInteractionClasses.front().interactionClassHandle;
  auto const regionalInteractionClassHandle =
      declaration.regionalSubscribedInteractionClasses.empty()
      ? 0U
      : declaration.regionalSubscribedInteractionClasses.front().interactionClassHandle;
  bool const hasPublishedInteraction = publishedInteractionClassHandle != 0U;
  bool const hasSubscribedInteraction = subscribedInteractionClassHandle != 0U;
  bool const hasRegionalInteraction = regionalInteractionClassHandle != 0U;
  if (!hasPublishedInteraction && !hasSubscribedInteraction && !hasRegionalInteraction) {
    return false;
  }
  auto const interactionClassHandle = publishedInteractionClassHandle != 0U
      ? publishedInteractionClassHandle
      : subscribedInteractionClassHandle != 0U
      ? subscribedInteractionClassHandle
      : regionalInteractionClassHandle;
  if (interactionClassHandle == 0U) {
    return false;
  }
  if (hasPublishedInteraction && hasSubscribedInteraction &&
      subscribedInteractionClassHandle != publishedInteractionClassHandle) {
    return false;
  }
  if (hasRegionalInteraction && (hasPublishedInteraction || hasSubscribedInteraction)) {
    return false;
  }
  if (hasRegionalInteraction &&
      declaration.regionalSubscribedInteractionClasses.front().regionHandle == 0U) {
    return false;
  }
  if (!declaration.interactionTransportationTypes.empty()) {
    auto const& transportation = declaration.interactionTransportationTypes.front();
    if (!hasPublishedInteraction || hasSubscribedInteraction || hasRegionalInteraction ||
        transportation.interactionClassHandle != interactionClassHandle ||
        transportation.value.empty() ||
        !declaration.pendingInteractionTransportationTypeChanges.empty()) {
      return false;
    }
  }
  if (declaration.pendingInteractionTransportationTypeChanges.empty()) {
    return true;
  }
  if (!hasPublishedInteraction || hasSubscribedInteraction || hasRegionalInteraction) {
    return false;
  }
  auto const& pending =
      declaration.pendingInteractionTransportationTypeChanges.front();
  return pending.interactionClassHandle == interactionClassHandle &&
      !pending.value.empty();
}

// A small multi-member companion keeps publication state independent per
// joined federate.  Each member is restricted to one publication and no
// callback-bearing declaration; the general interaction image remains gated
// until its own multi-member invariants are implemented.
bool isRouteFreeMultipleInteractionPublicationImage(FederationStateImage const& image) {
  if (image.interactionDeclarationCount != image.interactionDeclarations.size() ||
      image.interactionDeclarations.size() != 2U ||
      !image.objects.empty() ||
      !image.tsoInteractionMessages.empty() ||
      !image.tsoAttributeUpdateMessages.empty() ||
      !image.tsoObjectDeletionMessages.empty() ||
      !image.tsoDirectedInteractionMessages.empty() ||
      !image.tsoRequestRetractionRecords.empty() ||
      !image.tsoQueueEntries.empty()) {
    return false;
  }
  std::uint64_t previousFederateId = 0U;
  for (auto const& declaration : image.interactionDeclarations) {
    if (declaration.federateId == 0U || declaration.federateId <= previousFederateId ||
        declaration.publishedInteractionClasses.size() != 1U ||
        !declaration.subscribedInteractionClasses.empty() ||
        !declaration.regionalSubscribedInteractionClasses.empty() ||
        !declaration.publishedObjectClassDirectedInteractions.empty() ||
        !declaration.subscribedObjectClassDirectedInteractions.empty() ||
        !declaration.interactionTransportationTypes.empty() ||
        !declaration.interactionOrderTypes.empty() ||
        !declaration.pendingInteractionTransportationTypeChanges.empty() ||
        declaration.publishedInteractionClasses.front() == 0U) {
      return false;
    }
    previousFederateId = declaration.federateId;
  }
  return true;
}

// A focused process-restart companion admits one ordinary timestamped
// Send Interaction With Regions payload alongside its route-free declaration
// and region ledgers.  The callback closures remain live-only; the durable
// image carries the source-region snapshot, retraction recipients, and queue
// phases needed to rebind those routes after a fresh registry restart.  Keep
// this deliberately narrow so a general regional interaction matrix cannot
// become restartable without its own explicit state-image contract.
bool isRouteFreeRegionalTsoInteractionImage(FederationStateImage const& image) {
  if (image.interactionDeclarationCount != image.interactionDeclarations.size() ||
      image.interactionDeclarations.size() != 3U ||
      image.regions.size() != 3U ||
      image.tsoInteractionMessageCount != image.tsoInteractionMessages.size() ||
      image.tsoInteractionMessages.size() != 1U ||
      !image.tsoAttributeUpdateMessages.empty() ||
      !image.tsoObjectDeletionMessages.empty() ||
      !image.tsoDirectedInteractionMessages.empty() ||
      image.tsoRequestRetractionRecords.size() != 1U ||
      image.tsoQueueEntries.size() != 2U ||
      !image.objects.empty()) {
    return false;
  }

  auto const& message = image.tsoInteractionMessages.front();
  if (message.messageId == 0U || message.producingFederateId == 0U ||
      message.sentInteractionClassHandle == 0U ||
      !message.timestampEncoding.has_value() ||
      message.sentRegionHandles.size() != 1U ||
      message.sentRegionHandles.front() == 0U ||
      message.sentRegionSnapshots.size() != 1U ||
      message.sentRegionSnapshots.front().regionHandle !=
          message.sentRegionHandles.front() ||
      !message.sentRegionSnapshots.front().specificationCommitted ||
      message.sentRegionSnapshots.front().dimensionHandles.empty() ||
      message.sentRegionSnapshots.front().committedRangeBounds.size() !=
          message.sentRegionSnapshots.front().dimensionHandles.size()) {
    return false;
  }
  std::set<std::uint64_t> parameterHandles;
  for (auto const parameterHandle : message.sentParameterHandles) {
    if (parameterHandle == 0U || !parameterHandles.insert(parameterHandle).second) {
      return false;
    }
  }
  if (message.sentParameterHandles.size() != message.parameters.size()) {
    return false;
  }
  for (auto const& parameter : message.parameters) {
    if (parameter.parameterHandle == 0U ||
        !parameterHandles.contains(parameter.parameterHandle)) {
      return false;
    }
  }

  std::set<std::uint64_t> regionalDeclarationRegions;
  std::set<std::uint64_t> memberIds;
  std::size_t publicationCount = 0U;
  std::size_t regionalSubscriptionCount = 0U;
  for (auto const& declaration : image.interactionDeclarations) {
    if (declaration.federateId == 0U ||
        !memberIds.insert(declaration.federateId).second ||
        !declaration.subscribedInteractionClasses.empty() ||
        !declaration.publishedObjectClassDirectedInteractions.empty() ||
        !declaration.subscribedObjectClassDirectedInteractions.empty() ||
        !declaration.interactionTransportationTypes.empty() ||
        !declaration.pendingInteractionTransportationTypeChanges.empty()) {
      return false;
    }
    bool const publishes = declaration.publishedInteractionClasses.size() == 1U &&
        declaration.publishedInteractionClasses.front() ==
            message.sentInteractionClassHandle &&
        declaration.regionalSubscribedInteractionClasses.empty();
    bool const subscribesRegion = declaration.publishedInteractionClasses.empty() &&
        declaration.regionalSubscribedInteractionClasses.size() == 1U &&
        declaration.regionalSubscribedInteractionClasses.front().interactionClassHandle ==
            message.sentInteractionClassHandle &&
        declaration.regionalSubscribedInteractionClasses.front().regionHandle != 0U &&
        regionalDeclarationRegions.insert(
            declaration.regionalSubscribedInteractionClasses.front().regionHandle).second;
    if (publishes == subscribesRegion) {
      return false;
    }
    // A public publisher must retain the explicit timestamped order selected
    // before the save.  Subscribers in this narrow image have no local order
    // override; admitting any other order/declaration combination would
    // silently widen the route-free restart contract.
    if (declaration.interactionOrderTypes.size() > 1U) {
      return false;
    }
    if (!declaration.interactionOrderTypes.empty()) {
      auto const& order = declaration.interactionOrderTypes.front();
      if (!publishes || order.interactionClassHandle != message.sentInteractionClassHandle ||
          order.orderType != static_cast<std::uint32_t>(rti1516_2025::TIMESTAMP)) {
        return false;
      }
    }
    publicationCount += publishes ? 1U : 0U;
    regionalSubscriptionCount += subscribesRegion ? 1U : 0U;
  }
  if (publicationCount != 1U || regionalSubscriptionCount != 2U ||
      regionalDeclarationRegions.size() != 2U) {
    return false;
  }

  std::set<std::uint64_t> regionHandles;
  for (auto const& region : image.regions) {
    if (region.handle == 0U || region.ownerFederateId == 0U ||
        !regionHandles.insert(region.handle).second ||
        region.dimensionHandles.empty() || !region.specificationCommitted ||
        region.pendingRangeBounds.size() != region.dimensionHandles.size() ||
        region.committedRangeBounds.size() != region.dimensionHandles.size()) {
      return false;
    }
  }
  if (!regionHandles.contains(message.sentRegionHandles.front()) ||
      !std::includes(
          regionHandles.begin(),
          regionHandles.end(),
          regionalDeclarationRegions.begin(),
          regionalDeclarationRegions.end())) {
    return false;
  }

  auto const& retraction = image.tsoRequestRetractionRecords.front();
  if (retraction.messageId != message.messageId ||
      retraction.producingFederateId != message.producingFederateId ||
      !retraction.timestampEncoding.has_value() ||
      retraction.recipientStates.size() != image.tsoQueueEntries.size()) {
    return false;
  }
  std::set<std::uint64_t> recipients;
  for (auto const& recipient : retraction.recipientStates) {
    if (recipient.receivingFederateId == 0U ||
        recipient.receivingFederateId == message.producingFederateId ||
        recipient.state > 3U ||
        !recipients.insert(recipient.receivingFederateId).second) {
      return false;
    }
  }
  for (auto const& queueEntry : image.tsoQueueEntries) {
    if (queueEntry.messageId != message.messageId ||
        queueEntry.recipientFederateId == 0U ||
        queueEntry.recipientFederateId == message.producingFederateId ||
        queueEntry.phase > 2U ||
        !queueEntry.timestampEncoding.has_value() ||
        !recipients.contains(queueEntry.recipientFederateId)) {
      return false;
    }
  }
  return recipients.size() == 2U;
}

// The next mixed declaration slice keeps one publication and one ordinary
// subscription on separate joined federates. The publisher may carry one
// committed transportation override or one pending transportation change;
// the latter is rebound to the current publisher route by the restore loop.
// Regional state and order state remain outside this narrow declaration lane.
bool isRouteFreeMixedMultipleInteractionDeclarationImage(
    FederationStateImage const& image) {
  if (image.interactionDeclarationCount != image.interactionDeclarations.size() ||
      image.interactionDeclarations.size() != 2U ||
      !image.objects.empty() ||
      !image.tsoInteractionMessages.empty() ||
      !image.tsoAttributeUpdateMessages.empty() ||
      !image.tsoObjectDeletionMessages.empty() ||
      !image.tsoDirectedInteractionMessages.empty() ||
      !image.tsoRequestRetractionRecords.empty() ||
      !image.tsoQueueEntries.empty()) {
    return false;
  }
  std::uint64_t previousFederateId = 0U;
  bool hasPublication = false;
  bool hasSubscription = false;
  for (auto const& declaration : image.interactionDeclarations) {
    if (declaration.federateId == 0U || declaration.federateId <= previousFederateId ||
        declaration.publishedInteractionClasses.size() > 1U ||
        declaration.subscribedInteractionClasses.size() > 1U ||
        !declaration.regionalSubscribedInteractionClasses.empty() ||
        !declaration.publishedObjectClassDirectedInteractions.empty() ||
        !declaration.subscribedObjectClassDirectedInteractions.empty() ||
        declaration.interactionTransportationTypes.size() > 1U ||
        !declaration.interactionOrderTypes.empty() ||
        declaration.pendingInteractionTransportationTypeChanges.size() > 1U) {
      return false;
    }
    bool const publishes = declaration.publishedInteractionClasses.size() == 1U &&
        declaration.publishedInteractionClasses.front() != 0U &&
        declaration.subscribedInteractionClasses.empty();
    bool const subscribes = declaration.publishedInteractionClasses.empty() &&
        declaration.subscribedInteractionClasses.size() == 1U &&
        declaration.subscribedInteractionClasses.front().interactionClassHandle != 0U &&
        declaration.interactionTransportationTypes.empty();
    if (publishes == subscribes) {
      return false;
    }
    if (!declaration.interactionTransportationTypes.empty()) {
      auto const& transportation = declaration.interactionTransportationTypes.front();
      if (!publishes ||
          transportation.interactionClassHandle !=
              declaration.publishedInteractionClasses.front() ||
          transportation.value.empty() ||
          !declaration.pendingInteractionTransportationTypeChanges.empty()) {
        return false;
      }
    }
    if (!declaration.pendingInteractionTransportationTypeChanges.empty()) {
      if (!publishes || declaration.interactionTransportationTypes.size() != 0U) {
        return false;
      }
      auto const& pending =
          declaration.pendingInteractionTransportationTypeChanges.front();
      if (pending.interactionClassHandle !=
              declaration.publishedInteractionClasses.front() ||
          pending.value.empty()) {
        return false;
      }
    }
    hasPublication = hasPublication || publishes;
    hasSubscription = hasSubscription || subscribes;
    previousFederateId = declaration.federateId;
  }
  return hasPublication && hasSubscription;
}

// Directed interaction declarations are indexed by both the target object
// class and the interaction class.  Keep the fresh-registry slice route-free:
// one publisher and one or more subscribers for the same directed pair, with
// no ordinary declaration or callback-bearing state.  Multiple subscribers
// are needed to exercise the by-ownership handoff after restore; the publisher
// remains unique so this admission does not imply a general declaration
// arbitration policy.  A single route-free object plus the directed TSO
// payload/retraction/queue sections is also admitted.  This combined form is
// deliberately narrower than the independent declaration and payload forms:
// ordinary interactions, attribute updates, object deletions, or an orphaned
// retraction/queue record must not become process-restartable by accident.
bool isRouteFreeMultipleDirectedInteractionDeclarationImage(
    FederationStateImage const& image) {
  bool const hasDirectedTsoPayload =
      !image.tsoDirectedInteractionMessages.empty();
  if (image.interactionDeclarationCount != image.interactionDeclarations.size() ||
      image.interactionDeclarations.size() < 2U ||
      image.objects.size() > 1U ||
      !image.tsoInteractionMessages.empty() ||
      !image.tsoAttributeUpdateMessages.empty() ||
      !image.tsoObjectDeletionMessages.empty() ||
      image.tsoDirectedInteractionMessageCount !=
          image.tsoDirectedInteractionMessages.size() ||
      (!hasDirectedTsoPayload &&
       (!image.tsoRequestRetractionRecords.empty() ||
        !image.tsoQueueEntries.empty())) ||
      (hasDirectedTsoPayload && image.objects.size() != 1U)) {
    return false;
  }

  if (hasDirectedTsoPayload) {
    std::map<std::uint64_t, std::set<std::uint64_t>> directedRecipients;
    for (auto const& message : image.tsoDirectedInteractionMessages) {
      if (message.messageId == 0U || message.producingFederateId == 0U ||
          message.objectInstanceHandle == 0U ||
          message.sentInteractionClassHandle == 0U ||
          !message.timestampEncoding.has_value() || message.recipients.empty()) {
        return false;
      }
      auto& recipients = directedRecipients[message.messageId];
      for (auto const& recipient : message.recipients) {
        if (recipient.receivingFederateId == 0U ||
            recipient.receivingFederateId == message.producingFederateId ||
            recipient.objectInstanceHandle != message.objectInstanceHandle ||
            recipient.receivedInteractionClassHandle == 0U ||
            !recipients.insert(recipient.receivingFederateId).second) {
          return false;
        }
      }
    }
    std::set<std::uint64_t> retractionMessageIds;
    for (auto const& record : image.tsoRequestRetractionRecords) {
      if (record.messageId == 0U || record.producingFederateId == 0U ||
          !directedRecipients.contains(record.messageId) ||
          !retractionMessageIds.insert(record.messageId).second) {
        return false;
      }
      std::set<std::uint64_t> recordRecipients;
      for (auto const& recipient : record.recipientStates) {
        if (recipient.receivingFederateId == 0U ||
            recipient.state > 3U ||
            !recordRecipients.insert(recipient.receivingFederateId).second ||
            !directedRecipients.at(record.messageId).contains(
                recipient.receivingFederateId)) {
          return false;
        }
      }
      if (recordRecipients != directedRecipients.at(record.messageId)) {
        return false;
      }
    }
    if (retractionMessageIds.size() != directedRecipients.size()) {
      return false;
    }
    for (auto const& queueEntry : image.tsoQueueEntries) {
      if (queueEntry.messageId == 0U || queueEntry.recipientFederateId == 0U ||
          queueEntry.phase > 2U ||
          !directedRecipients.contains(queueEntry.messageId) ||
          !directedRecipients.at(queueEntry.messageId).contains(
              queueEntry.recipientFederateId) ||
          !queueEntry.timestampEncoding.has_value()) {
        return false;
      }
    }
  }

  std::uint64_t previousFederateId = 0U;
  std::optional<std::pair<std::uint64_t, std::uint64_t>> directedPair;
  bool hasPublication = false;
  bool hasSubscription = false;
  for (auto const& declaration : image.interactionDeclarations) {
    if (declaration.federateId == 0U || declaration.federateId <= previousFederateId) {
      return false;
    }
    // A joined federate may be neutral for this directed pair: it has no
    // ordinary or directed declaration, transportation override, order
    // state, or pending declaration transition.  Keep that member in the
    // identity/order fence, but do not make it invalidate an otherwise
    // route-free publisher/subscriber image.  This is important when delayed
    // subscription evaluation retains a live callback route for a member
    // that has not subscribed to the directed interaction.
    bool const neutral =
        declaration.publishedInteractionClasses.empty() &&
        declaration.subscribedInteractionClasses.empty() &&
        declaration.regionalSubscribedInteractionClasses.empty() &&
        declaration.publishedObjectClassDirectedInteractions.empty() &&
        declaration.subscribedObjectClassDirectedInteractions.empty() &&
        declaration.interactionTransportationTypes.empty() &&
        declaration.interactionOrderTypes.empty() &&
        declaration.pendingInteractionTransportationTypeChanges.empty();
    if (neutral) {
      previousFederateId = declaration.federateId;
      continue;
    }
    if (!declaration.publishedInteractionClasses.empty() ||
        !declaration.subscribedInteractionClasses.empty() ||
        !declaration.regionalSubscribedInteractionClasses.empty() ||
        !declaration.interactionTransportationTypes.empty() ||
        !declaration.pendingInteractionTransportationTypeChanges.empty()) {
      return false;
    }
    bool const publishes =
        declaration.publishedObjectClassDirectedInteractions.size() == 1U &&
        declaration.publishedObjectClassDirectedInteractions.front().objectClassHandle != 0U &&
        declaration.publishedObjectClassDirectedInteractions.front().interactionClassHandle != 0U &&
        declaration.subscribedObjectClassDirectedInteractions.empty();
    bool const subscribes =
        declaration.publishedObjectClassDirectedInteractions.empty() &&
        declaration.subscribedObjectClassDirectedInteractions.size() == 1U &&
        declaration.subscribedObjectClassDirectedInteractions.front().objectClassHandle != 0U &&
        declaration.subscribedObjectClassDirectedInteractions.front().interactionClassHandle != 0U;
    if (publishes == subscribes) {
      return false;
    }
    // A timestamped directed sender records its explicit interaction order
    // override in the saved declaration.  Preserve that narrow state while
    // rejecting subscriber-side or multi-class order ledgers.
    if (declaration.interactionOrderTypes.size() > 1U) {
      return false;
    }
    if (!declaration.interactionOrderTypes.empty()) {
      auto const& order = declaration.interactionOrderTypes.front();
      if (!publishes ||
          order.interactionClassHandle !=
              declaration.publishedObjectClassDirectedInteractions.front().interactionClassHandle ||
          order.orderType != static_cast<std::uint32_t>(rti1516_2025::TIMESTAMP)) {
        return false;
      }
    }
    auto const pairForDeclaration = publishes
        ? std::pair{
              declaration.publishedObjectClassDirectedInteractions.front().objectClassHandle,
              declaration.publishedObjectClassDirectedInteractions.front().interactionClassHandle}
        : std::pair{
              declaration.subscribedObjectClassDirectedInteractions.front().objectClassHandle,
              declaration.subscribedObjectClassDirectedInteractions.front().interactionClassHandle};
    if (directedPair.has_value() && directedPair != pairForDeclaration) {
      return false;
    }
    directedPair = pairForDeclaration;
    if (publishes) {
      if (hasPublication) {
        return false;
      }
      hasPublication = true;
    } else {
      hasSubscription = true;
    }
    previousFederateId = declaration.federateId;
  }
  return hasPublication && hasSubscription && directedPair.has_value();
}

}  // namespace umbra::detail::federation_registry_state_image_validation
