#include "internal/federation/federation_registry.hpp"
#include "internal/fom/fom_catalog.hpp"

#include <cstdint>
#include <limits>
#include <map>
#include <mutex>
#include <optional>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace umbra::detail {
AttributeOwnershipAcquisitionPlan
EmbeddedFederationRegistry::planAttributeOwnershipAcquisition(
    std::wstring const& federationName,
    std::uint64_t requestingFederateId,
    std::uint64_t objectInstanceHandle,
    std::set<std::uint64_t> const& desiredAttributeHandles,
    std::vector<unsigned char> userSuppliedTag) {
  std::scoped_lock lock(mutex_);
  auto federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    return {AttributeOwnershipAcquisitionStatus::federation_does_not_exist};
  }
  if (!federation->second.members.contains(requestingFederateId)) {
    return {AttributeOwnershipAcquisitionStatus::requesting_federate_not_member};
  }
  // IEEE 1516.1-2025 11.4.2(k) prohibits ownership transfer of predefined
  // RTI-owned MOM attributes. Keep the MOM ledger separate from application
  // objectInstances so this boundary cannot be misreported as unknown.
  if (federation->second.rtiOwnedJoinedFederateMomObjects.contains(objectInstanceHandle)) {
    return {AttributeOwnershipAcquisitionStatus::attribute_owned_by_rti};
  }
  auto instance = federation->second.objectInstances.find(objectInstanceHandle);
  if (instance == federation->second.objectInstances.end() || instance->second.deleteAccepted ||
      !instance->second.knownObjectClassHandlesByFederate.contains(requestingFederateId)) {
    return {AttributeOwnershipAcquisitionStatus::object_instance_not_known};
  }
  if (!federation->second.definition.catalog ||
      !federation->second.objectClassHandles ||
      !federation->second.attributeHandles) {
    return {AttributeOwnershipAcquisitionStatus::inconsistent_catalog};
  }

  auto const knownClass = instance->second.knownObjectClassHandlesByFederate.find(
      requestingFederateId);
  auto const knownClassName = federation->second.objectClassHandles->nameFor(knownClass->second);
  if (!knownClassName ||
      federation->second.definition.catalog->objectClass(*knownClassName) == nullptr) {
    return {AttributeOwnershipAcquisitionStatus::inconsistent_catalog};
  }
  for (std::uint64_t const attributeHandle : desiredAttributeHandles) {
    if (!federation->second.attributeHandles->nameFor(
            federation->second.definition.catalog.get(),
            *knownClassName,
            attributeHandle)) {
      return {AttributeOwnershipAcquisitionStatus::attribute_not_defined};
    }
  }

  // An empty request changes neither ownership nor the private Acquisition
  // Pending state chart. Its object/federate preconditions above still apply.
  if (desiredAttributeHandles.empty()) {
    return {};
  }

  auto const publishedAttributes = publishedObjectClassAttributes(
      federation->second,
      requestingFederateId,
      knownClass->second);
  if (!publishedAttributes) {
    return {AttributeOwnershipAcquisitionStatus::inconsistent_catalog};
  }
  if (publishedAttributes->empty()) {
    return {AttributeOwnershipAcquisitionStatus::object_class_not_published};
  }
  for (std::uint64_t const attributeHandle : desiredAttributeHandles) {
    if (!publishedAttributes->contains(attributeHandle)) {
      return {AttributeOwnershipAcquisitionStatus::attribute_not_published};
    }
  }

  for (std::uint64_t const attributeHandle : desiredAttributeHandles) {
    auto const owner = instance->second.attributeOwnersByHandle.find(attributeHandle);
    if (owner == instance->second.attributeOwnersByHandle.end()) {
      continue;
    }
    if (owner->second == requestingFederateId) {
      return {AttributeOwnershipAcquisitionStatus::federate_owns_attributes};
    }
    if (!federation->second.members.contains(owner->second)) {
      // A completed resignation must not leave a stale owner record. Treat a
      // surviving record as an internal catalog inconsistency rather than
      // manufacturing a normal acquisition outcome from invalid state.
      return {AttributeOwnershipAcquisitionStatus::inconsistent_catalog};
    }
  }

  std::set<std::uint64_t> newlyRequestedAttributeHandles = desiredAttributeHandles;
  for (auto const& [requestId, pending] :
       instance->second.pendingAttributeOwnershipAcquisitionRequests) {
    static_cast<void>(requestId);
    if (pending.requestingFederateId != requestingFederateId) {
      continue;
    }
    for (std::uint64_t const attributeHandle : pending.desiredAttributeHandles) {
      newlyRequestedAttributeHandles.erase(attributeHandle);
    }
  }
  for (auto const& [cancellationId, cancellation] :
       instance->second.pendingAttributeOwnershipAcquisitionCancellations) {
    static_cast<void>(cancellationId);
    if (cancellation.requestingFederateId != requestingFederateId) {
      continue;
    }
    for (std::uint64_t const attributeHandle : cancellation.attributeHandles) {
      newlyRequestedAttributeHandles.erase(attributeHandle);
    }
  }

  // IEEE 1516.1-2025 7.8 preserves the Acquiring state for a repeat regular
  // request. The existing pending record remains authoritative and no second
  // owner-side release callback is queued for those attributes.
  if (newlyRequestedAttributeHandles.empty()) {
    return {};
  }

  if (federation->second.nextAttributeOwnershipAcquisitionRequestId == 0 ||
      federation->second.nextAttributeOwnershipAcquisitionRequestId ==
          std::numeric_limits<std::uint64_t>::max() ||
      federation->second.nextAttributeOwnershipAcquisitionRequestSequence == 0 ||
      federation->second.nextAttributeOwnershipAcquisitionRequestSequence ==
          std::numeric_limits<std::uint64_t>::max()) {
    return {AttributeOwnershipAcquisitionStatus::inconsistent_catalog};
  }
  auto const requesterCallbackRoute = federation->second.interactionCallbackRoutes.find(
      requestingFederateId);
  if (requesterCallbackRoute == federation->second.interactionCallbackRoutes.end() ||
      !requesterCallbackRoute->second) {
    return {AttributeOwnershipAcquisitionStatus::inconsistent_catalog};
  }
  for (std::uint64_t const attributeHandle : newlyRequestedAttributeHandles) {
    auto const owner = instance->second.attributeOwnersByHandle.find(attributeHandle);
    if (owner == instance->second.attributeOwnersByHandle.end()) {
      continue;
    }
    auto const ownerCallbackRoute = federation->second.interactionCallbackRoutes.find(owner->second);
    if (ownerCallbackRoute == federation->second.interactionCallbackRoutes.end() ||
        !ownerCallbackRoute->second) {
      return {AttributeOwnershipAcquisitionStatus::inconsistent_catalog};
    }
  }

  std::uint64_t const requestId = federation->second.nextAttributeOwnershipAcquisitionRequestId;
  std::uint64_t const requestSequence =
      federation->second.nextAttributeOwnershipAcquisitionRequestSequence;
  auto const [pending, inserted] =
      instance->second.pendingAttributeOwnershipAcquisitionRequests.try_emplace(
          requestId,
          Federation::ObjectInstance::PendingAttributeOwnershipAcquisition{
              requestingFederateId,
              requestSequence,
              std::move(newlyRequestedAttributeHandles),
              {},
              {},
              {},
              std::move(userSuppliedTag),
          });
  static_cast<void>(pending);
  if (!inserted) {
    return {AttributeOwnershipAcquisitionStatus::inconsistent_catalog};
  }
  ++federation->second.nextAttributeOwnershipAcquisitionRequestId;
  ++federation->second.nextAttributeOwnershipAcquisitionRequestSequence;

  // A regular acquisition by this same federate and attribute takes
  // precedence over an earlier If Available request. Erasing the private WTA
  // reservation also makes its already queued terminal work a harmless
  // no-delivery outcome.
  for (auto ifAvailable =
           instance->second.pendingAttributeOwnershipAcquisitionIfAvailableRequests.begin();
       ifAvailable !=
       instance->second.pendingAttributeOwnershipAcquisitionIfAvailableRequests.end();) {
    if (ifAvailable->second.requestingFederateId != requestingFederateId) {
      ++ifAvailable;
      continue;
    }
    for (std::uint64_t const attributeHandle : desiredAttributeHandles) {
      ifAvailable->second.desiredAttributeHandles.erase(attributeHandle);
    }
    if (ifAvailable->second.desiredAttributeHandles.empty()) {
      ifAvailable =
          instance->second.pendingAttributeOwnershipAcquisitionIfAvailableRequests.erase(
              ifAvailable);
    } else {
      ++ifAvailable;
    }
  }

  return {
      AttributeOwnershipAcquisitionStatus::applied,
      planPendingAttributeOwnershipAcquisitionWork(federation->second, instance->second),
  };
}

std::optional<AttributeOwnershipAcquisitionNotificationDelivery>
EmbeddedFederationRegistry::beginAttributeOwnershipAcquisitionNotification(
    std::wstring const& federationName,
    std::uint64_t requestingFederateId,
    std::uint64_t objectInstanceHandle,
    std::uint64_t requestId,
    std::set<std::uint64_t> const& scheduledAttributeHandles) {
  if (requestId == 0 || scheduledAttributeHandles.empty()) {
    return std::nullopt;
  }

  std::scoped_lock lock(mutex_);
  auto federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    return std::nullopt;
  }
  auto instance = federation->second.objectInstances.find(objectInstanceHandle);
  if (instance == federation->second.objectInstances.end()) {
    return std::nullopt;
  }
  auto pending = instance->second.pendingAttributeOwnershipAcquisitionRequests.find(requestId);
  if (pending == instance->second.pendingAttributeOwnershipAcquisitionRequests.end() ||
      pending->second.requestingFederateId != requestingFederateId) {
    return std::nullopt;
  }

  auto cancellationPending = [&](std::uint64_t attributeHandle) {
    for (auto const& [cancellationId, cancellation] :
         instance->second.pendingAttributeOwnershipAcquisitionCancellations) {
      static_cast<void>(cancellationId);
      if (cancellation.requestingFederateId == requestingFederateId &&
          cancellation.attributeHandles.contains(attributeHandle)) {
        return true;
      }
    }
    return false;
  };

  auto erasePendingAndPlanFollowup = [&] {
    instance->second.pendingAttributeOwnershipAcquisitionRequests.erase(pending);
    return planPendingAttributeOwnershipAcquisitionWork(federation->second, instance->second);
  };
  if (instance->second.deleteAccepted ||
      !federation->second.members.contains(requestingFederateId) ||
      !instance->second.knownObjectClassHandlesByFederate.contains(requestingFederateId) ||
      !federation->second.definition.catalog ||
      !federation->second.objectClassHandles ||
      !federation->second.attributeHandles) {
    auto followupWorkItems = erasePendingAndPlanFollowup();
    if (followupWorkItems.empty()) {
      return std::nullopt;
    }
    return AttributeOwnershipAcquisitionNotificationDelivery{
        objectInstanceHandle,
        {},
        std::move(followupWorkItems),
    };
  }

  auto const knownClass = instance->second.knownObjectClassHandlesByFederate.find(
      requestingFederateId);
  auto const knownClassName = federation->second.objectClassHandles->nameFor(knownClass->second);
  if (!knownClassName ||
      federation->second.definition.catalog->objectClass(*knownClassName) == nullptr) {
    auto followupWorkItems = erasePendingAndPlanFollowup();
    if (followupWorkItems.empty()) {
      return std::nullopt;
    }
    return AttributeOwnershipAcquisitionNotificationDelivery{
        objectInstanceHandle,
        {},
        std::move(followupWorkItems),
    };
  }
  auto const publishedAttributes = publishedObjectClassAttributes(
      federation->second,
      requestingFederateId,
      knownClass->second);
  if (!publishedAttributes) {
    auto followupWorkItems = erasePendingAndPlanFollowup();
    if (followupWorkItems.empty()) {
      return std::nullopt;
    }
    return AttributeOwnershipAcquisitionNotificationDelivery{
        objectInstanceHandle,
        {},
        std::move(followupWorkItems),
    };
  }
  for (std::uint64_t const attributeHandle : pending->second.desiredAttributeHandles) {
    if (!federation->second.attributeHandles->nameFor(
            federation->second.definition.catalog.get(),
            *knownClassName,
            attributeHandle) ||
        !publishedAttributes->contains(attributeHandle)) {
      auto followupWorkItems = erasePendingAndPlanFollowup();
      if (followupWorkItems.empty()) {
        return std::nullopt;
      }
      return AttributeOwnershipAcquisitionNotificationDelivery{
          objectInstanceHandle,
          {},
          std::move(followupWorkItems),
      };
    }
  }

  std::set<std::uint64_t> securedAttributeHandles;
  for (std::uint64_t const attributeHandle : scheduledAttributeHandles) {
    pending->second.notificationQueuedAttributeHandles.erase(attributeHandle);
    if (!pending->second.desiredAttributeHandles.contains(attributeHandle)) {
      continue;
    }
    if (pending->second.unavailableQueuedAttributeHandles.contains(attributeHandle) ||
        cancellationPending(attributeHandle)) {
      // Cancellation was accepted before this queued notification entered
      // user code. Leave the acquisition and cancellation reservations for
      // the confirmation boundary; this callback is stale, not a race win.
      continue;
    }
    if (instance->second.pendingNegotiatedAttributeOwnershipDivestitures.contains(
            attributeHandle)) {
      // A negotiated divestiture began after this regular notification was
      // queued. The attribute remains owned until Confirm Divestiture, so this
      // stale unowned-acquisition boundary cannot transfer it.
      continue;
    }
    auto const owner = instance->second.attributeOwnersByHandle.find(attributeHandle);
    if (owner != instance->second.attributeOwnersByHandle.end() &&
        owner->second != requestingFederateId) {
      continue;
    }
    if (owner == instance->second.attributeOwnersByHandle.end() &&
        !isFirstPendingAttributeOwnershipAcquisition(
            instance->second,
            requestId,
            attributeHandle)) {
      continue;
    }
    if (owner == instance->second.attributeOwnersByHandle.end()) {
      instance->second.attributeOwnersByHandle.emplace(attributeHandle, requestingFederateId);
      clearOwnershipAssumptionSearch(instance->second, attributeHandle);
      promoteDeferredUpdateRegionAssociation(
          instance->second,
          requestingFederateId,
          attributeHandle);
      auto const transportationName = attributeDefaultTransportationName(
          federation->second,
          requestingFederateId,
          knownClass->second,
          attributeHandle);
      if (transportationName) {
        instance->second.attributeTransportationTypes.insert_or_assign(
            attributeHandle,
            *transportationName);
      }
      auto const orderType = attributeDefaultOrderType(
          federation->second,
          requestingFederateId,
          knownClass->second,
          attributeHandle);
      if (orderType) {
        instance->second.attributeOrderTypes.insert_or_assign(attributeHandle, *orderType);
      } else {
        instance->second.attributeOrderTypes.erase(attributeHandle);
      }
    }
    pending->second.desiredAttributeHandles.erase(attributeHandle);
    securedAttributeHandles.insert(attributeHandle);
  }

  if (pending->second.desiredAttributeHandles.empty()) {
    instance->second.pendingAttributeOwnershipAcquisitionRequests.erase(pending);
  }
  auto followupWorkItems = planPendingAttributeOwnershipAcquisitionWork(
      federation->second,
      instance->second);
  if (securedAttributeHandles.empty() && followupWorkItems.empty()) {
    return std::nullopt;
  }
  return AttributeOwnershipAcquisitionNotificationDelivery{
      objectInstanceHandle,
      std::move(securedAttributeHandles),
      std::move(followupWorkItems),
  };
}

std::optional<AttributeOwnershipAcquisitionReleaseDelivery>
EmbeddedFederationRegistry::beginAttributeOwnershipAcquisitionRelease(
    std::wstring const& federationName,
    std::uint64_t requestingFederateId,
    std::uint64_t owningFederateId,
    std::uint64_t objectInstanceHandle,
    std::uint64_t requestId,
    std::set<std::uint64_t> const& scheduledAttributeHandles) {
  if (requestId == 0 || scheduledAttributeHandles.empty()) {
    return std::nullopt;
  }

  std::scoped_lock lock(mutex_);
  auto federation = federations_.find(federationName);
  if (federation == federations_.end() ||
      !federation->second.members.contains(requestingFederateId) ||
      !federation->second.members.contains(owningFederateId)) {
    return std::nullopt;
  }
  auto instance = federation->second.objectInstances.find(objectInstanceHandle);
  if (instance == federation->second.objectInstances.end() || instance->second.deleteAccepted ||
      !instance->second.knownObjectClassHandlesByFederate.contains(owningFederateId) ||
      !instance->second.knownObjectClassHandlesByFederate.contains(requestingFederateId)) {
    return std::nullopt;
  }
  auto pending = instance->second.pendingAttributeOwnershipAcquisitionRequests.find(requestId);
  if (pending == instance->second.pendingAttributeOwnershipAcquisitionRequests.end() ||
      pending->second.requestingFederateId != requestingFederateId) {
    return std::nullopt;
  }

  auto cancellationPending = [&](std::uint64_t attributeHandle) {
    for (auto const& [cancellationId, cancellation] :
         instance->second.pendingAttributeOwnershipAcquisitionCancellations) {
      static_cast<void>(cancellationId);
      if (cancellation.requestingFederateId == requestingFederateId &&
          cancellation.attributeHandles.contains(attributeHandle)) {
        return true;
      }
    }
    return false;
  };

  std::set<std::uint64_t> candidateAttributeHandles;
  for (std::uint64_t const attributeHandle : scheduledAttributeHandles) {
    if (!pending->second.desiredAttributeHandles.contains(attributeHandle)) {
      continue;
    }
    auto queuedAtOwner = pending->second.releaseCallbacksQueuedByOwningFederate.find(
        owningFederateId);
    if (queuedAtOwner == pending->second.releaseCallbacksQueuedByOwningFederate.end() ||
        !queuedAtOwner->second.contains(attributeHandle)) {
      // This work item was superseded before the callback boundary.
      continue;
    }

    if (cancellationPending(attributeHandle)) {
      // Cancellation won before this owner-side release callback entered
      // user code. Consume only the one-shot release reservation; the
      // cancellation confirmation remains the terminal callback.
      queuedAtOwner->second.erase(attributeHandle);
      if (queuedAtOwner->second.empty()) {
        pending->second.releaseCallbacksQueuedByOwningFederate.erase(queuedAtOwner);
      }
      continue;
    }

    if (instance->second.pendingNegotiatedAttributeOwnershipDivestitures.contains(
            attributeHandle)) {
      // The owner entered the negotiated Waiting state after this ordinary
      // release callback was queued. It is now consumed and suppressed; the
      // confirmation path owns the next callback decision for this attribute.
      // Ordinary release reservations otherwise remain in place after their
      // callback: the original acquisition is still pending until a terminal
      // ownership service responds, and normal follow-up planning must not
      // enqueue a duplicate release request.
      queuedAtOwner->second.erase(attributeHandle);
      if (queuedAtOwner->second.empty()) {
        pending->second.releaseCallbacksQueuedByOwningFederate.erase(queuedAtOwner);
      }
      continue;
    }
    auto const owner = instance->second.attributeOwnersByHandle.find(attributeHandle);
    if (owner != instance->second.attributeOwnersByHandle.end() &&
        owner->second == owningFederateId) {
      candidateAttributeHandles.insert(attributeHandle);
    }
  }
  if (candidateAttributeHandles.empty()) {
    return std::nullopt;
  }
  return AttributeOwnershipAcquisitionReleaseDelivery{
      objectInstanceHandle,
      std::move(candidateAttributeHandles),
  };
}

AttributeOwnershipReleaseDeniedPlan
EmbeddedFederationRegistry::planAttributeOwnershipReleaseDenied(
    std::wstring const& federationName,
    std::uint64_t owningFederateId,
    std::uint64_t objectInstanceHandle,
    std::set<std::uint64_t> const& attributeHandles) {
  std::scoped_lock lock(mutex_);
  auto federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    return {AttributeOwnershipReleaseDeniedStatus::federation_does_not_exist};
  }
  if (!federation->second.members.contains(owningFederateId)) {
    return {AttributeOwnershipReleaseDeniedStatus::owning_federate_not_member};
  }
  // IEEE 1516.1-2025 11.4.2(k) prohibits ownership transfer of predefined
  // RTI-owned MOM attributes.
  if (federation->second.rtiOwnedJoinedFederateMomObjects.contains(objectInstanceHandle)) {
    return {AttributeOwnershipReleaseDeniedStatus::attribute_owned_by_rti};
  }
  auto instance = federation->second.objectInstances.find(objectInstanceHandle);
  if (instance == federation->second.objectInstances.end() || instance->second.deleteAccepted ||
      !instance->second.knownObjectClassHandlesByFederate.contains(owningFederateId)) {
    return {AttributeOwnershipReleaseDeniedStatus::object_instance_not_known};
  }
  if (!federation->second.definition.catalog ||
      !federation->second.objectClassHandles ||
      !federation->second.attributeHandles) {
    return {AttributeOwnershipReleaseDeniedStatus::inconsistent_catalog};
  }

  auto const knownClass = instance->second.knownObjectClassHandlesByFederate.find(
      owningFederateId);
  auto const knownClassName = federation->second.objectClassHandles->nameFor(knownClass->second);
  if (!knownClassName ||
      federation->second.definition.catalog->objectClass(*knownClassName) == nullptr) {
    return {AttributeOwnershipReleaseDeniedStatus::inconsistent_catalog};
  }
  for (std::uint64_t const attributeHandle : attributeHandles) {
    if (!federation->second.attributeHandles->nameFor(
            federation->second.definition.catalog.get(),
            *knownClassName,
            attributeHandle)) {
      return {AttributeOwnershipReleaseDeniedStatus::attribute_not_defined};
    }
    auto const owner = instance->second.attributeOwnersByHandle.find(attributeHandle);
    if (owner == instance->second.attributeOwnersByHandle.end() ||
        owner->second != owningFederateId) {
      return {AttributeOwnershipReleaseDeniedStatus::attribute_not_owned};
    }
  }

  std::map<std::uint64_t, std::set<std::uint64_t>> attributesByRequestingFederate;
  for (auto const& [requestId, pending] :
       instance->second.pendingAttributeOwnershipAcquisitionRequests) {
    static_cast<void>(requestId);
    for (std::uint64_t const attributeHandle : pending.desiredAttributeHandles) {
      if (attributeHandles.contains(attributeHandle) &&
          !pending.unavailableQueuedAttributeHandles.contains(attributeHandle) &&
          !instance->second.pendingNegotiatedAttributeOwnershipDivestitures.contains(
              attributeHandle)) {
        attributesByRequestingFederate[pending.requestingFederateId].insert(attributeHandle);
      }
    }
  }
  for (auto const& [requestingFederateId, ignoredAttributes] :
       attributesByRequestingFederate) {
    static_cast<void>(ignoredAttributes);
    auto const callbackRoute = federation->second.interactionCallbackRoutes.find(
        requestingFederateId);
    if (!federation->second.members.contains(requestingFederateId) ||
        callbackRoute == federation->second.interactionCallbackRoutes.end() ||
        !callbackRoute->second) {
      return {AttributeOwnershipReleaseDeniedStatus::inconsistent_catalog};
    }
  }

  auto consumeCancellationReservation = [&](std::uint64_t requestingFederateId,
                                             std::uint64_t attributeHandle) {
    for (auto cancellation =
             instance->second.pendingAttributeOwnershipAcquisitionCancellations.begin();
         cancellation !=
             instance->second.pendingAttributeOwnershipAcquisitionCancellations.end();) {
      if (cancellation->second.requestingFederateId == requestingFederateId &&
          cancellation->second.attributeHandles.erase(attributeHandle) != 0U) {
        if (cancellation->second.attributeHandles.empty()) {
          instance->second.pendingAttributeOwnershipAcquisitionCancellations.erase(cancellation);
        }
        return;
      }
      ++cancellation;
    }
  };

  for (auto pending = instance->second.pendingAttributeOwnershipAcquisitionRequests.begin();
       pending != instance->second.pendingAttributeOwnershipAcquisitionRequests.end();) {
    for (std::uint64_t const attributeHandle : attributeHandles) {
      if (instance->second.pendingNegotiatedAttributeOwnershipDivestitures.contains(
              attributeHandle)) {
        continue;
      }
      if (!pending->second.desiredAttributeHandles.contains(attributeHandle) ||
          pending->second.unavailableQueuedAttributeHandles.contains(attributeHandle)) {
        continue;
      }
      pending->second.unavailableQueuedAttributeHandles.insert(attributeHandle);
      consumeCancellationReservation(pending->second.requestingFederateId, attributeHandle);
      pending->second.notificationQueuedAttributeHandles.erase(attributeHandle);
      for (auto release = pending->second.releaseCallbacksQueuedByOwningFederate.begin();
           release != pending->second.releaseCallbacksQueuedByOwningFederate.end();) {
        release->second.erase(attributeHandle);
        if (release->second.empty()) {
          release = pending->second.releaseCallbacksQueuedByOwningFederate.erase(release);
        } else {
          ++release;
        }
      }
    }
    ++pending;
  }

  AttributeOwnershipReleaseDeniedPlan result;
  for (auto& [requestingFederateId, unavailableAttributes] : attributesByRequestingFederate) {
    auto const callbackRoute = federation->second.interactionCallbackRoutes.find(
        requestingFederateId);
    result.recipients.push_back({
        requestingFederateId,
        objectInstanceHandle,
        std::move(unavailableAttributes),
        callbackRoute->second,
    });
  }
  result.followupWorkItems = planPendingAttributeOwnershipAcquisitionWork(
      federation->second,
      instance->second);
  return result;
}


}  // namespace umbra::detail
