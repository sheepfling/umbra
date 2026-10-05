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
AttributeOwnershipDivestitureIfWantedPlan
EmbeddedFederationRegistry::planAttributeOwnershipDivestitureIfWanted(
    std::wstring const& federationName,
    std::uint64_t divestingFederateId,
    std::uint64_t objectInstanceHandle,
    std::set<std::uint64_t> const& attributeHandles,
    std::vector<unsigned char> userSuppliedTag) {
  std::scoped_lock lock(mutex_);
  auto federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    return {AttributeOwnershipDivestitureIfWantedStatus::federation_does_not_exist};
  }
  if (!federation->second.members.contains(divestingFederateId)) {
    return {AttributeOwnershipDivestitureIfWantedStatus::divesting_federate_not_member};
  }
  // IEEE 1516.1-2025 11.4.2(k) prohibits ownership transfer of predefined
  // RTI-owned MOM attributes.
  if (federation->second.rtiOwnedJoinedFederateMomObjects.contains(objectInstanceHandle)) {
    return {AttributeOwnershipDivestitureIfWantedStatus::attribute_owned_by_rti};
  }
  auto instance = federation->second.objectInstances.find(objectInstanceHandle);
  if (instance == federation->second.objectInstances.end() || instance->second.deleteAccepted ||
      !instance->second.knownObjectClassHandlesByFederate.contains(divestingFederateId)) {
    return {AttributeOwnershipDivestitureIfWantedStatus::object_instance_not_known};
  }
  if (!federation->second.definition.catalog ||
      !federation->second.objectClassHandles ||
      !federation->second.attributeHandles) {
    return {AttributeOwnershipDivestitureIfWantedStatus::inconsistent_catalog};
  }

  auto const knownClass = instance->second.knownObjectClassHandlesByFederate.find(
      divestingFederateId);
  auto const knownClassName = federation->second.objectClassHandles->nameFor(knownClass->second);
  if (!knownClassName ||
      federation->second.definition.catalog->objectClass(*knownClassName) == nullptr) {
    return {AttributeOwnershipDivestitureIfWantedStatus::inconsistent_catalog};
  }
  for (std::uint64_t const attributeHandle : attributeHandles) {
    if (!federation->second.attributeHandles->nameFor(
            federation->second.definition.catalog.get(),
            *knownClassName,
            attributeHandle)) {
      return {AttributeOwnershipDivestitureIfWantedStatus::attribute_not_defined};
    }
    auto const owner = instance->second.attributeOwnersByHandle.find(attributeHandle);
    if (owner == instance->second.attributeOwnersByHandle.end() ||
        owner->second != divestingFederateId) {
      return {AttributeOwnershipDivestitureIfWantedStatus::attribute_not_owned};
    }
  }

  // A Divestiture If Wanted call can legally conclude without moving any
  // ownership when no joined federate is attempting to acquire an attribute.
  // It nevertheless validates all supplied attributes above, because 7.1.5
  // makes a single invalid attribute fail the complete service invocation.
  if (attributeHandles.empty()) {
    return {};
  }

  enum class PendingRequestKind {
    regular,
    if_available,
  };
  struct SelectedAcquirer {
    PendingRequestKind kind = PendingRequestKind::regular;
    std::uint64_t requestId = 0;
    std::uint64_t receivingFederateId = 0;
    std::uint64_t requestSequence = 0;
  };

  // 1516.1-2025 requires a real joined acquirer before this service may
  // divest an attribute, but it does not prescribe an arbitration policy for
  // multiple eligible pending requests. This serial embedded profile chooses
  // the earliest accepted regular-or-WTA request using one private sequence,
  // which avoids implying a standards-level priority between request forms.
  auto const acquirerIsStillEligible = [
      &federation,
      &instance,
      divestingFederateId](std::uint64_t receivingFederateId,
                            std::uint64_t attributeHandle) {
    if (receivingFederateId == divestingFederateId ||
        !federation->second.members.contains(receivingFederateId)) {
      return false;
    }
    auto const knownReceivingClass =
        instance->second.knownObjectClassHandlesByFederate.find(receivingFederateId);
    if (knownReceivingClass == instance->second.knownObjectClassHandlesByFederate.end()) {
      return false;
    }
    auto const publishedAttributes = publishedObjectClassAttributes(
        federation->second,
        receivingFederateId,
        knownReceivingClass->second);
    return publishedAttributes && publishedAttributes->contains(attributeHandle);
  };

  std::map<std::uint64_t, SelectedAcquirer> selectedAcquirersByAttribute;
  for (std::uint64_t const attributeHandle : attributeHandles) {
    std::optional<SelectedAcquirer> selectedAcquirer;
    auto consider = [&selectedAcquirer, attributeHandle, &acquirerIsStillEligible](
                        PendingRequestKind kind,
                        std::uint64_t requestId,
                        auto const& pending) {
      if (!pending.desiredAttributeHandles.contains(attributeHandle) ||
          !acquirerIsStillEligible(pending.requestingFederateId, attributeHandle)) {
        return;
      }
      if (!selectedAcquirer || pending.requestSequence < selectedAcquirer->requestSequence) {
        selectedAcquirer = SelectedAcquirer{
            kind,
            requestId,
            pending.requestingFederateId,
            pending.requestSequence,
        };
      }
    };
    for (auto const& [requestId, pending] :
         instance->second.pendingAttributeOwnershipAcquisitionRequests) {
      consider(PendingRequestKind::regular, requestId, pending);
    }
    for (auto const& [requestId, pending] :
         instance->second.pendingAttributeOwnershipAcquisitionIfAvailableRequests) {
      consider(PendingRequestKind::if_available, requestId, pending);
    }
    if (selectedAcquirer) {
      selectedAcquirersByAttribute.emplace(attributeHandle, *selectedAcquirer);
    }
  }

  std::map<std::uint64_t, std::set<std::uint64_t>> attributesByReceivingFederate;
  for (auto const& [attributeHandle, selectedAcquirer] : selectedAcquirersByAttribute) {
    auto const callbackRoute = federation->second.interactionCallbackRoutes.find(
        selectedAcquirer.receivingFederateId);
    if (callbackRoute == federation->second.interactionCallbackRoutes.end() ||
        !callbackRoute->second) {
      return {AttributeOwnershipDivestitureIfWantedStatus::inconsistent_catalog};
    }
    attributesByReceivingFederate[selectedAcquirer.receivingFederateId].insert(attributeHandle);
  }

  if (attributesByReceivingFederate.empty()) {
    return {};
  }
  if (federation->second.nextAttributeOwnershipDivestitureIfWantedNotificationId == 0 ||
      federation->second.nextAttributeOwnershipDivestitureIfWantedNotificationId ==
          std::numeric_limits<std::uint64_t>::max() ||
      attributesByReceivingFederate.size() >
          std::numeric_limits<std::uint64_t>::max() -
              federation->second.nextAttributeOwnershipDivestitureIfWantedNotificationId) {
    return {AttributeOwnershipDivestitureIfWantedStatus::inconsistent_catalog};
  }

  struct PreparedNotification {
    std::uint64_t notificationId = 0;
    std::uint64_t receivingFederateId = 0;
    std::set<std::uint64_t> attributeHandles;
    std::vector<unsigned char> userSuppliedTag;
  };
  std::vector<PreparedNotification> preparedNotifications;
  preparedNotifications.reserve(attributesByReceivingFederate.size());

  AttributeOwnershipDivestitureIfWantedPlan result;
  result.notifications.reserve(attributesByReceivingFederate.size());
  for (auto const& [attributeHandle, selectedAcquirer] : selectedAcquirersByAttribute) {
    static_cast<void>(selectedAcquirer);
    result.divestedAttributeHandles.insert(attributeHandle);
  }

  std::uint64_t notificationId =
      federation->second.nextAttributeOwnershipDivestitureIfWantedNotificationId;
  for (auto const& [receivingFederateId, selectedAttributes] :
       attributesByReceivingFederate) {
    auto const callbackRoute = federation->second.interactionCallbackRoutes.find(
        receivingFederateId);
    // The route was checked while grouping. Retain the defensive branch so a
    // future change cannot turn an accepted transfer into an unrouteable one.
    if (callbackRoute == federation->second.interactionCallbackRoutes.end() ||
        !callbackRoute->second) {
      return {AttributeOwnershipDivestitureIfWantedStatus::inconsistent_catalog};
    }
    preparedNotifications.push_back({
        notificationId,
        receivingFederateId,
        selectedAttributes,
        userSuppliedTag,
    });
    result.notifications.push_back({
        notificationId,
        receivingFederateId,
        objectInstanceHandle,
        selectedAttributes,
        userSuppliedTag,
        callbackRoute->second,
    });
    ++notificationId;
  }

  // Reserve every notification before changing owner state. A failure to
  // allocate a reservation leaves this service with no partial transfer.
  std::vector<std::uint64_t> reservedNotificationIds;
  reservedNotificationIds.reserve(preparedNotifications.size());
  try {
    for (auto& prepared : preparedNotifications) {
      auto const [reservation, inserted] =
          instance->second.pendingAttributeOwnershipDivestitureIfWantedNotifications.try_emplace(
              prepared.notificationId,
              Federation::ObjectInstance::
              PendingAttributeOwnershipDivestitureIfWantedNotification{
                      prepared.receivingFederateId,
                      std::move(prepared.attributeHandles),
                      std::move(prepared.userSuppliedTag),
                  });
      static_cast<void>(reservation);
      if (!inserted) {
        for (std::uint64_t const reservedNotificationId : reservedNotificationIds) {
          instance->second.pendingAttributeOwnershipDivestitureIfWantedNotifications.erase(
              reservedNotificationId);
        }
        return {AttributeOwnershipDivestitureIfWantedStatus::inconsistent_catalog};
      }
      reservedNotificationIds.push_back(prepared.notificationId);
    }
  } catch (...) {
    for (std::uint64_t const reservedNotificationId : reservedNotificationIds) {
      instance->second.pendingAttributeOwnershipDivestitureIfWantedNotifications.erase(
          reservedNotificationId);
    }
    return {AttributeOwnershipDivestitureIfWantedStatus::inconsistent_catalog};
  }

  // A returned attribute moves directly from the divesting owner to the
  // selected acquirer. All older work addressed to the former owner is made
  // stale; requests made by other acquirers remain pending and are considered
  // after the selected acquirer's notification begins.
  auto consumeCancellationReservation = [&](std::uint64_t requestingFederateId,
                                             std::uint64_t attributeHandle) {
    // Cancel Attribute Ownership Acquisition may already have accepted a
    // cancellation while the owner-side release callback was in user code.
    // Divestiture If Wanted is a competing terminal transfer: once it moves
    // the attribute, the queued cancellation confirmation is stale and the
    // resulting Acquisition Notification is the required second-form reply.
    // Consume every matching reservation so a duplicate cancellation call
    // cannot resurrect a confirmation for an attribute that was transferred.
    for (auto cancellation =
             instance->second.pendingAttributeOwnershipAcquisitionCancellations.begin();
         cancellation !=
             instance->second.pendingAttributeOwnershipAcquisitionCancellations.end();) {
      if (cancellation->second.requestingFederateId == requestingFederateId) {
        cancellation->second.attributeHandles.erase(attributeHandle);
      }
      if (cancellation->second.attributeHandles.empty()) {
        cancellation = instance->second.pendingAttributeOwnershipAcquisitionCancellations.erase(
            cancellation);
      } else {
        ++cancellation;
      }
    }
  };

  for (auto const& [attributeHandle, selectedAcquirer] : selectedAcquirersByAttribute) {
    auto const owner = instance->second.attributeOwnersByHandle.find(attributeHandle);
    // The all-or-nothing validation above guarantees this owner record.
    owner->second = selectedAcquirer.receivingFederateId;
    if (selectedAcquirer.kind == PendingRequestKind::regular) {
      consumeCancellationReservation(
          selectedAcquirer.receivingFederateId,
          attributeHandle);
    }
    clearOwnershipAssumptionSearch(instance->second, attributeHandle);
    // Divestiture If Wanted transfers the attribute synchronously, but the
    // former owner's explicit update-region association still ends at the
    // ownership boundary. A pending association belonging to the selected
    // acquirer is promoted at that same boundary.
    clearUpdateRegionAssociation(instance->second, attributeHandle);
    promoteDeferredUpdateRegionAssociation(
        instance->second,
        selectedAcquirer.receivingFederateId,
        attributeHandle);
    instance->second.attributeOrderTypes.erase(attributeHandle);
    auto const acquiringClass = instance->second.knownObjectClassHandlesByFederate.find(
        selectedAcquirer.receivingFederateId);
    if (acquiringClass != instance->second.knownObjectClassHandlesByFederate.end()) {
      auto const orderType = attributeDefaultOrderType(
          federation->second,
          selectedAcquirer.receivingFederateId,
          acquiringClass->second,
          attributeHandle);
      if (orderType) {
        instance->second.attributeOrderTypes.insert_or_assign(attributeHandle, *orderType);
      }
    }
    // Divestiture If Wanted is an alternate terminal divestiture route. It
    // cancels a negotiated waiting state before that earlier confirmation can
    // transfer the same instance attribute.
    instance->second.pendingNegotiatedAttributeOwnershipDivestitures.erase(attributeHandle);

    for (auto pending = instance->second.pendingAttributeOwnershipAcquisitionRequests.begin();
         pending != instance->second.pendingAttributeOwnershipAcquisitionRequests.end();) {
      if (selectedAcquirer.kind == PendingRequestKind::regular &&
          pending->first == selectedAcquirer.requestId) {
        pending->second.desiredAttributeHandles.erase(attributeHandle);
        pending->second.notificationQueuedAttributeHandles.erase(attributeHandle);
      }
      auto release = pending->second.releaseCallbacksQueuedByOwningFederate.find(
          divestingFederateId);
      if (release != pending->second.releaseCallbacksQueuedByOwningFederate.end()) {
        release->second.erase(attributeHandle);
        if (release->second.empty()) {
          pending->second.releaseCallbacksQueuedByOwningFederate.erase(release);
        }
      }
      if (pending->second.desiredAttributeHandles.empty()) {
        pending = instance->second.pendingAttributeOwnershipAcquisitionRequests.erase(pending);
      } else {
        ++pending;
      }
    }

    if (selectedAcquirer.kind == PendingRequestKind::if_available) {
      auto pending =
          instance->second.pendingAttributeOwnershipAcquisitionIfAvailableRequests.find(
              selectedAcquirer.requestId);
      if (pending !=
          instance->second.pendingAttributeOwnershipAcquisitionIfAvailableRequests.end()) {
        pending->second.desiredAttributeHandles.erase(attributeHandle);
        if (pending->second.desiredAttributeHandles.empty()) {
          instance->second.pendingAttributeOwnershipAcquisitionIfAvailableRequests.erase(pending);
        }
      }
    }
  }
  refreshRegionUsage(federation->second);
  federation->second.nextAttributeOwnershipDivestitureIfWantedNotificationId = notificationId;
  return result;
}

std::optional<AttributeOwnershipDivestitureIfWantedDelivery>
EmbeddedFederationRegistry::beginAttributeOwnershipDivestitureIfWantedNotification(
    std::wstring const& federationName,
    std::uint64_t receivingFederateId,
    std::uint64_t objectInstanceHandle,
    std::uint64_t notificationId,
    std::set<std::uint64_t> const& scheduledAttributeHandles) {
  if (notificationId == 0 || scheduledAttributeHandles.empty()) {
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
  auto notification =
      instance->second.pendingAttributeOwnershipDivestitureIfWantedNotifications.find(
          notificationId);
  if (notification ==
          instance->second.pendingAttributeOwnershipDivestitureIfWantedNotifications.end() ||
      notification->second.receivingFederateId != receivingFederateId ||
      notification->second.attributeHandles != scheduledAttributeHandles) {
    return std::nullopt;
  }

  auto consumeNotificationAndPlanFollowup = [&] {
    auto securedAttributeHandles = notification->second.attributeHandles;
    instance->second.pendingAttributeOwnershipDivestitureIfWantedNotifications.erase(notification);
    return std::pair{
        std::move(securedAttributeHandles),
        planPendingAttributeOwnershipAcquisitionWork(federation->second, instance->second),
    };
  };
  if (instance->second.deleteAccepted ||
      !federation->second.members.contains(receivingFederateId) ||
      !instance->second.knownObjectClassHandlesByFederate.contains(receivingFederateId)) {
    auto [ignoredAttributes, followupWorkItems] = consumeNotificationAndPlanFollowup();
    static_cast<void>(ignoredAttributes);
    if (followupWorkItems.empty()) {
      return std::nullopt;
    }
    return AttributeOwnershipDivestitureIfWantedDelivery{
        objectInstanceHandle,
        {},
        std::move(followupWorkItems),
    };
  }

  auto [securedAttributeHandles, followupWorkItems] = consumeNotificationAndPlanFollowup();
  return AttributeOwnershipDivestitureIfWantedDelivery{
      objectInstanceHandle,
      std::move(securedAttributeHandles),
      std::move(followupWorkItems),
  };
}


}  // namespace umbra::detail
