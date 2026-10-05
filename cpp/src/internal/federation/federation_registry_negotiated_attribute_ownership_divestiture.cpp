#include "internal/federation/federation_registry.hpp"
#include "internal/fom/fom_catalog.hpp"

#include <algorithm>
#include <cstdint>
#include <iterator>
#include <limits>
#include <map>
#include <mutex>
#include <optional>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace umbra::detail {
NegotiatedAttributeOwnershipDivestiturePlan
EmbeddedFederationRegistry::planNegotiatedAttributeOwnershipDivestiture(
    std::wstring const& federationName,
    std::uint64_t divestingFederateId,
    std::uint64_t objectInstanceHandle,
    std::set<std::uint64_t> const& attributeHandles,
    std::vector<unsigned char> userSuppliedTag) {
  std::scoped_lock lock(mutex_);
  auto federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    return {NegotiatedAttributeOwnershipDivestitureStatus::federation_does_not_exist};
  }
  if (!federation->second.members.contains(divestingFederateId)) {
    return {NegotiatedAttributeOwnershipDivestitureStatus::divesting_federate_not_member};
  }
  // IEEE 1516.1-2025 11.4.2(k) prohibits ownership transfer of predefined
  // RTI-owned MOM attributes.
  if (federation->second.rtiOwnedJoinedFederateMomObjects.contains(objectInstanceHandle)) {
    return {NegotiatedAttributeOwnershipDivestitureStatus::attribute_owned_by_rti};
  }
  auto instance = federation->second.objectInstances.find(objectInstanceHandle);
  if (instance == federation->second.objectInstances.end() || instance->second.deleteAccepted ||
      !instance->second.knownObjectClassHandlesByFederate.contains(divestingFederateId)) {
    return {NegotiatedAttributeOwnershipDivestitureStatus::object_instance_not_known};
  }
  if (!federation->second.definition.catalog ||
      !federation->second.objectClassHandles ||
      !federation->second.attributeHandles) {
    return {NegotiatedAttributeOwnershipDivestitureStatus::inconsistent_catalog};
  }

  auto const knownClass = instance->second.knownObjectClassHandlesByFederate.find(
      divestingFederateId);
  auto const knownClassName = federation->second.objectClassHandles->nameFor(knownClass->second);
  if (!knownClassName ||
      federation->second.definition.catalog->objectClass(*knownClassName) == nullptr) {
    return {NegotiatedAttributeOwnershipDivestitureStatus::inconsistent_catalog};
  }
  for (std::uint64_t const attributeHandle : attributeHandles) {
    if (!federation->second.attributeHandles->nameFor(
            federation->second.definition.catalog.get(),
            *knownClassName,
            attributeHandle)) {
      return {NegotiatedAttributeOwnershipDivestitureStatus::attribute_not_defined};
    }
    auto const owner = instance->second.attributeOwnersByHandle.find(attributeHandle);
    if (owner == instance->second.attributeOwnersByHandle.end() ||
        owner->second != divestingFederateId) {
      return {NegotiatedAttributeOwnershipDivestitureStatus::attribute_not_owned};
    }
    if (instance->second.pendingNegotiatedAttributeOwnershipDivestitures.contains(
            attributeHandle)) {
      return {NegotiatedAttributeOwnershipDivestitureStatus::attribute_already_being_divested};
    }
  }

  // An empty request has no ownership or pending-divestiture transition after
  // its normal membership/object preconditions have been established.
  if (attributeHandles.empty()) {
    return {};
  }

  // A new negotiated offer starts a fresh assumption interval. This also
  // invalidates any stale queued assumption callback left by an earlier
  // divestiture that transferred ownership before its callback crossed the
  // begin boundary.
  for (std::uint64_t const attributeHandle : attributeHandles) {
    clearOwnershipAssumptionSearch(instance->second, attributeHandle);
  }

  // Construct every state record before merging it into the federation so an
  // allocation failure cannot leave part of a supplied attribute set waiting
  // for divestiture.
  std::map<std::uint64_t,
           Federation::ObjectInstance::PendingNegotiatedAttributeOwnershipDivestiture>
      requestedDivestitures;
  for (std::uint64_t const attributeHandle : attributeHandles) {
    requestedDivestitures.emplace(
        attributeHandle,
        Federation::ObjectInstance::PendingNegotiatedAttributeOwnershipDivestiture{
            divestingFederateId,
            0,
            0,
            false,
            false,
            false,
            userSuppliedTag,
        });
  }
  instance->second.pendingNegotiatedAttributeOwnershipDivestitures.merge(requestedDivestitures);

  // A negotiated divestiture keeps the current owner in place, but it still
  // starts the standard Request Attribute Ownership Assumption search. Seed
  // the same per-attribute search ledger used by unconditional divestiture so
  // callback-entry rechecks, continuation after a non-accepting candidate, and
  // later join/publication events all share one durable path. The owner is
  // excluded by planAttributeOwnershipAssumptionsForFederateLocked below.
  for (std::uint64_t const attributeHandle : attributeHandles) {
    instance->second.ownershipAssumptionRecipientsByAttribute.try_emplace(attributeHandle);
    instance->second.ownershipAssumptionUserSuppliedTagsByAttribute.insert_or_assign(
        attributeHandle,
        userSuppliedTag);
  }

  // Keep an already queued ordinary release callback reserved until its
  // callback boundary. If it is invoked while this negotiated divestiture is
  // pending, beginAttributeOwnershipAcquisitionRelease consumes and suppresses
  // it. If cancellation occurs first, the original one-shot callback remains
  // the correct ordinary-release notification and no duplicate is queued.

  NegotiatedAttributeOwnershipDivestiturePlan result;
  result.workItems = planPendingAttributeOwnershipAcquisitionWork(
      federation->second,
      instance->second);
  for (auto const& [receivingFederateId, membership] : federation->second.members) {
    static_cast<void>(membership);
    auto assumptionRecipients = planAttributeOwnershipAssumptionsForFederateLocked(
        federation->second,
        receivingFederateId,
        instance->second.handle,
        &attributeHandles);
    result.assumptionRecipients.insert(
        result.assumptionRecipients.end(),
        std::make_move_iterator(assumptionRecipients.begin()),
        std::make_move_iterator(assumptionRecipients.end()));
  }
  return result;
}

std::optional<RequestDivestitureConfirmationDelivery>
EmbeddedFederationRegistry::beginRequestDivestitureConfirmation(
    std::wstring const& federationName,
    std::uint64_t divestingFederateId,
    std::uint64_t acquiringFederateId,
    std::uint64_t objectInstanceHandle,
    std::uint64_t acquisitionRequestId,
    bool candidateIsIfAvailable,
    std::set<std::uint64_t> const& scheduledAttributeHandles) {
  if (acquisitionRequestId == 0 || scheduledAttributeHandles.empty()) {
    return std::nullopt;
  }

  std::scoped_lock lock(mutex_);
  auto federation = federations_.find(federationName);
  if (federation == federations_.end() ||
      !federation->second.members.contains(divestingFederateId) ||
      !federation->second.members.contains(acquiringFederateId)) {
    return std::nullopt;
  }
  auto instance = federation->second.objectInstances.find(objectInstanceHandle);
  if (instance == federation->second.objectInstances.end() || instance->second.deleteAccepted ||
      !instance->second.knownObjectClassHandlesByFederate.contains(divestingFederateId) ||
      !instance->second.knownObjectClassHandlesByFederate.contains(acquiringFederateId)) {
    return std::nullopt;
  }
  auto const regularAcquisition = instance->second.pendingAttributeOwnershipAcquisitionRequests.find(
      acquisitionRequestId);
  auto const ifAvailableAcquisition =
      instance->second.pendingAttributeOwnershipAcquisitionIfAvailableRequests.find(
          acquisitionRequestId);
  if ((!candidateIsIfAvailable &&
       (regularAcquisition == instance->second.pendingAttributeOwnershipAcquisitionRequests.end() ||
        regularAcquisition->second.requestingFederateId != acquiringFederateId)) ||
      (candidateIsIfAvailable &&
       (ifAvailableAcquisition ==
            instance->second.pendingAttributeOwnershipAcquisitionIfAvailableRequests.end() ||
        ifAvailableAcquisition->second.requestingFederateId != acquiringFederateId))) {
    return std::nullopt;
  }

  auto cancellationPending = [&](std::uint64_t attributeHandle) {
    if (candidateIsIfAvailable) {
      return false;
    }
    return std::ranges::any_of(
        instance->second.pendingAttributeOwnershipAcquisitionCancellations,
        [&](auto const& cancellation) {
          return cancellation.second.requestingFederateId == acquiringFederateId &&
                 cancellation.second.attributeHandles.contains(attributeHandle);
        });
  };

  RequestDivestitureConfirmationDelivery delivery;
  delivery.objectInstanceHandle = objectInstanceHandle;
  for (std::uint64_t const attributeHandle : scheduledAttributeHandles) {
    auto divestiture = instance->second.pendingNegotiatedAttributeOwnershipDivestitures.find(
        attributeHandle);
    auto const owner = instance->second.attributeOwnersByHandle.find(attributeHandle);
    if (divestiture == instance->second.pendingNegotiatedAttributeOwnershipDivestitures.end() ||
        owner == instance->second.attributeOwnersByHandle.end() ||
        owner->second != divestingFederateId ||
        divestiture->second.divestingFederateId != divestingFederateId ||
        divestiture->second.acquiringFederateId != acquiringFederateId ||
        divestiture->second.acquisitionRequestId != acquisitionRequestId ||
        divestiture->second.acquiringFederateIsIfAvailable != candidateIsIfAvailable ||
        cancellationPending(attributeHandle) ||
        !divestiture->second.confirmationQueued ||
        divestiture->second.confirmationDelivered ||
        ((candidateIsIfAvailable &&
          !ifAvailableAcquisition->second.desiredAttributeHandles.contains(attributeHandle)) ||
         (!candidateIsIfAvailable &&
          !regularAcquisition->second.desiredAttributeHandles.contains(attributeHandle)))) {
      continue;
    }
    delivery.releasedAttributeHandles.insert(attributeHandle);
  }
  if (delivery.releasedAttributeHandles.empty()) {
    return std::nullopt;
  }
  for (std::uint64_t const attributeHandle : delivery.releasedAttributeHandles) {
    instance->second.pendingNegotiatedAttributeOwnershipDivestitures.at(attributeHandle)
        .confirmationDelivered = true;
  }
  return delivery;
}

ConfirmDivestiturePlan EmbeddedFederationRegistry::planConfirmDivestiture(
    std::wstring const& federationName,
    std::uint64_t divestingFederateId,
    std::uint64_t objectInstanceHandle,
    std::set<std::uint64_t> const& attributeHandles,
    std::vector<unsigned char> userSuppliedTag) {
  std::scoped_lock lock(mutex_);
  auto federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    return {ConfirmDivestitureStatus::federation_does_not_exist};
  }
  if (!federation->second.members.contains(divestingFederateId)) {
    return {ConfirmDivestitureStatus::divesting_federate_not_member};
  }
  // IEEE 1516.1-2025 11.4.2(k) prohibits ownership transfer of predefined
  // RTI-owned MOM attributes.
  if (federation->second.rtiOwnedJoinedFederateMomObjects.contains(objectInstanceHandle)) {
    return {ConfirmDivestitureStatus::attribute_owned_by_rti};
  }
  auto instance = federation->second.objectInstances.find(objectInstanceHandle);
  if (instance == federation->second.objectInstances.end() || instance->second.deleteAccepted ||
      !instance->second.knownObjectClassHandlesByFederate.contains(divestingFederateId)) {
    return {ConfirmDivestitureStatus::object_instance_not_known};
  }
  if (!federation->second.definition.catalog ||
      !federation->second.objectClassHandles ||
      !federation->second.attributeHandles) {
    return {ConfirmDivestitureStatus::inconsistent_catalog};
  }

  auto const knownClass = instance->second.knownObjectClassHandlesByFederate.find(
      divestingFederateId);
  auto const knownClassName = federation->second.objectClassHandles->nameFor(knownClass->second);
  if (!knownClassName ||
      federation->second.definition.catalog->objectClass(*knownClassName) == nullptr) {
    return {ConfirmDivestitureStatus::inconsistent_catalog};
  }
  for (std::uint64_t const attributeHandle : attributeHandles) {
    if (!federation->second.attributeHandles->nameFor(
            federation->second.definition.catalog.get(),
            *knownClassName,
            attributeHandle)) {
      return {ConfirmDivestitureStatus::attribute_not_defined};
    }
    auto const owner = instance->second.attributeOwnersByHandle.find(attributeHandle);
    if (owner == instance->second.attributeOwnersByHandle.end() ||
        owner->second != divestingFederateId) {
      return {ConfirmDivestitureStatus::attribute_not_owned};
    }
    auto const divestiture = instance->second.pendingNegotiatedAttributeOwnershipDivestitures.find(
        attributeHandle);
    if (divestiture == instance->second.pendingNegotiatedAttributeOwnershipDivestitures.end() ||
        divestiture->second.divestingFederateId != divestingFederateId ||
        !divestiture->second.confirmationDelivered) {
      return {ConfirmDivestitureStatus::attribute_divestiture_was_not_requested};
    }
  }

  auto cancellationPending = [&](std::uint64_t requestingFederateId,
                                 std::uint64_t attributeHandle) {
    return std::ranges::any_of(
        instance->second.pendingAttributeOwnershipAcquisitionCancellations,
        [=](auto const& cancellation) {
          return cancellation.second.requestingFederateId == requestingFederateId &&
                 cancellation.second.attributeHandles.contains(attributeHandle);
        });
  };

  if (attributeHandles.empty()) {
    return {};
  }

  std::map<std::uint64_t, std::set<std::uint64_t>> attributesByAcquiringFederate;
  std::map<std::pair<bool, std::uint64_t>, std::set<std::uint64_t>>
      attributesByAcquisitionRequest;
  std::set<std::uint64_t> attributesWithoutAnActiveAcquirer;
  for (std::uint64_t const attributeHandle : attributeHandles) {
    auto& divestiture =
        instance->second.pendingNegotiatedAttributeOwnershipDivestitures.at(attributeHandle);
    auto const regularAcquisition = instance->second.pendingAttributeOwnershipAcquisitionRequests.find(
        divestiture.acquisitionRequestId);
    auto const ifAvailableAcquisition =
        instance->second.pendingAttributeOwnershipAcquisitionIfAvailableRequests.find(
            divestiture.acquisitionRequestId);
    bool activeAcquirer = divestiture.acquiringFederateId != 0 &&
                          divestiture.acquisitionRequestId != 0 &&
                          federation->second.members.contains(divestiture.acquiringFederateId) &&
                          instance->second.knownObjectClassHandlesByFederate.contains(
                              divestiture.acquiringFederateId);
    if (activeAcquirer) {
      if (divestiture.acquiringFederateIsIfAvailable) {
        activeAcquirer =
            ifAvailableAcquisition !=
                instance->second.pendingAttributeOwnershipAcquisitionIfAvailableRequests.end() &&
            ifAvailableAcquisition->second.requestingFederateId ==
                divestiture.acquiringFederateId &&
            ifAvailableAcquisition->second.desiredAttributeHandles.contains(attributeHandle);
      } else {
        activeAcquirer =
            regularAcquisition != instance->second.pendingAttributeOwnershipAcquisitionRequests.end() &&
            regularAcquisition->second.requestingFederateId == divestiture.acquiringFederateId &&
            regularAcquisition->second.desiredAttributeHandles.contains(attributeHandle) &&
            !cancellationPending(divestiture.acquiringFederateId, attributeHandle);
      }
    }
    if (!activeAcquirer) {
      attributesWithoutAnActiveAcquirer.insert(attributeHandle);
      continue;
    }
    auto const callbackRoute = federation->second.interactionCallbackRoutes.find(
        divestiture.acquiringFederateId);
    if (callbackRoute == federation->second.interactionCallbackRoutes.end() ||
        !callbackRoute->second) {
      return {ConfirmDivestitureStatus::inconsistent_catalog};
    }
    attributesByAcquiringFederate[divestiture.acquiringFederateId].insert(attributeHandle);
    attributesByAcquisitionRequest[
        {divestiture.acquiringFederateIsIfAvailable, divestiture.acquisitionRequestId}]
        .insert(attributeHandle);
  }

  if (!attributesWithoutAnActiveAcquirer.empty()) {
    // IEEE 1516.1-2025 returns a confirming owner to Waiting for a New Owner
    // to be Found after NoAcquisitionPending. Keep ownership unchanged and
    // make a later regular request eligible for a fresh confirmation.
    for (std::uint64_t const attributeHandle : attributesWithoutAnActiveAcquirer) {
      auto& divestiture =
          instance->second.pendingNegotiatedAttributeOwnershipDivestitures.at(attributeHandle);
      divestiture.acquiringFederateId = 0;
      divestiture.acquisitionRequestId = 0;
      divestiture.confirmationQueued = false;
      divestiture.confirmationDelivered = false;
    }
    return {ConfirmDivestitureStatus::no_acquisition_pending};
  }

  if (federation->second.nextConfirmDivestitureNotificationId == 0 ||
      federation->second.nextConfirmDivestitureNotificationId ==
          std::numeric_limits<std::uint64_t>::max()) {
    return {ConfirmDivestitureStatus::inconsistent_catalog};
  }

  // Build all post-confirmation notification reservations before mutating
  // ownership. Merging the populated map below is non-allocating, preserving
  // the supplied-set failure boundary required by ownership management.
  std::map<std::uint64_t,
           Federation::ObjectInstance::PendingConfirmDivestitureNotification>
      notificationReservations;
  ConfirmDivestiturePlan result;
  result.notifications.reserve(attributesByAcquiringFederate.size());
  std::uint64_t nextNotificationId = federation->second.nextConfirmDivestitureNotificationId;
  for (auto const& [acquiringFederateId, confirmedAttributeHandles] :
       attributesByAcquiringFederate) {
    if (nextNotificationId == 0 ||
        nextNotificationId == std::numeric_limits<std::uint64_t>::max()) {
      return {ConfirmDivestitureStatus::inconsistent_catalog};
    }
    auto const callbackRoute = federation->second.interactionCallbackRoutes.find(acquiringFederateId);
    if (callbackRoute == federation->second.interactionCallbackRoutes.end() ||
        !callbackRoute->second) {
      return {ConfirmDivestitureStatus::inconsistent_catalog};
    }
    notificationReservations.emplace(
        nextNotificationId,
        Federation::ObjectInstance::PendingConfirmDivestitureNotification{
            acquiringFederateId,
            confirmedAttributeHandles,
            userSuppliedTag,
        });
    result.notifications.push_back({
        nextNotificationId,
        acquiringFederateId,
        objectInstanceHandle,
        confirmedAttributeHandles,
        userSuppliedTag,
        callbackRoute->second,
    });
    ++nextNotificationId;
  }

  for (auto const& [requestKey, confirmedAttributeHandles] :
       attributesByAcquisitionRequest) {
    auto const [candidateIfAvailable, acquisitionRequestId] = requestKey;
    if (candidateIfAvailable) {
      auto acquisition =
          instance->second.pendingAttributeOwnershipAcquisitionIfAvailableRequests.find(
              acquisitionRequestId);
      if (acquisition ==
          instance->second.pendingAttributeOwnershipAcquisitionIfAvailableRequests.end()) {
        return {ConfirmDivestitureStatus::inconsistent_catalog};
      }
      for (std::uint64_t const attributeHandle : confirmedAttributeHandles) {
        acquisition->second.desiredAttributeHandles.erase(attributeHandle);
      }
      if (acquisition->second.desiredAttributeHandles.empty()) {
        instance->second.pendingAttributeOwnershipAcquisitionIfAvailableRequests.erase(acquisition);
      }
    } else {
      auto acquisition = instance->second.pendingAttributeOwnershipAcquisitionRequests.find(
          acquisitionRequestId);
      if (acquisition == instance->second.pendingAttributeOwnershipAcquisitionRequests.end()) {
        return {ConfirmDivestitureStatus::inconsistent_catalog};
      }
      for (std::uint64_t const attributeHandle : confirmedAttributeHandles) {
        acquisition->second.desiredAttributeHandles.erase(attributeHandle);
        acquisition->second.notificationQueuedAttributeHandles.erase(attributeHandle);
        for (auto release = acquisition->second.releaseCallbacksQueuedByOwningFederate.begin();
             release != acquisition->second.releaseCallbacksQueuedByOwningFederate.end();) {
          release->second.erase(attributeHandle);
          if (release->second.empty()) {
            release = acquisition->second.releaseCallbacksQueuedByOwningFederate.erase(release);
          } else {
            ++release;
          }
        }
      }
      if (acquisition->second.desiredAttributeHandles.empty()) {
        instance->second.pendingAttributeOwnershipAcquisitionRequests.erase(acquisition);
      }
    }
  }
  for (auto const& [acquiringFederateId, confirmedAttributeHandles] :
       attributesByAcquiringFederate) {
    for (std::uint64_t const attributeHandle : confirmedAttributeHandles) {
      instance->second.attributeOwnersByHandle.at(attributeHandle) = acquiringFederateId;
      clearOwnershipAssumptionSearch(instance->second, attributeHandle);
      // A committed update-region association belongs to the former owner;
      // Confirm Divestiture transfers ownership without transferring that
      // association. A pending association established by the acquiring
      // federate is promoted at the same ownership boundary.
      clearUpdateRegionAssociation(instance->second, attributeHandle);
      promoteDeferredUpdateRegionAssociation(
          instance->second,
          acquiringFederateId,
          attributeHandle);
      auto const acquiringClass = instance->second.knownObjectClassHandlesByFederate.find(
          acquiringFederateId);
      if (acquiringClass != instance->second.knownObjectClassHandlesByFederate.end()) {
        auto const transportationName = attributeDefaultTransportationName(
            federation->second,
            acquiringFederateId,
            acquiringClass->second,
            attributeHandle);
        if (transportationName) {
          instance->second.attributeTransportationTypes.insert_or_assign(
              attributeHandle,
              *transportationName);
        }
        auto const orderType = attributeDefaultOrderType(
            federation->second,
            acquiringFederateId,
            acquiringClass->second,
            attributeHandle);
        if (orderType) {
          instance->second.attributeOrderTypes.insert_or_assign(attributeHandle, *orderType);
        } else {
          instance->second.attributeOrderTypes.erase(attributeHandle);
        }
      } else {
        instance->second.attributeOrderTypes.erase(attributeHandle);
      }
      instance->second.pendingNegotiatedAttributeOwnershipDivestitures.erase(attributeHandle);
    }
  }
  instance->second.pendingConfirmDivestitureNotifications.merge(notificationReservations);
  refreshRegionUsage(federation->second);
  federation->second.nextConfirmDivestitureNotificationId = nextNotificationId;
  return result;
}

std::optional<ConfirmDivestitureNotificationDelivery>
EmbeddedFederationRegistry::beginConfirmDivestitureNotification(
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
  auto notification = instance->second.pendingConfirmDivestitureNotifications.find(notificationId);
  if (notification == instance->second.pendingConfirmDivestitureNotifications.end() ||
      notification->second.receivingFederateId != receivingFederateId ||
      notification->second.attributeHandles != scheduledAttributeHandles ||
      instance->second.deleteAccepted ||
      !federation->second.members.contains(receivingFederateId) ||
      !instance->second.knownObjectClassHandlesByFederate.contains(receivingFederateId) ||
      !federation->second.definition.catalog ||
      !federation->second.objectClassHandles ||
      !federation->second.attributeHandles) {
    return std::nullopt;
  }

  auto const knownClass = instance->second.knownObjectClassHandlesByFederate.find(
      receivingFederateId);
  auto const knownClassName = federation->second.objectClassHandles->nameFor(knownClass->second);
  if (!knownClassName ||
      federation->second.definition.catalog->objectClass(*knownClassName) == nullptr) {
    return std::nullopt;
  }
  auto const publishedAttributes = publishedObjectClassAttributes(
      federation->second,
      receivingFederateId,
      knownClass->second);
  if (!publishedAttributes) {
    return std::nullopt;
  }
  for (std::uint64_t const attributeHandle : scheduledAttributeHandles) {
    auto const owner = instance->second.attributeOwnersByHandle.find(attributeHandle);
    if (owner == instance->second.attributeOwnersByHandle.end() ||
        owner->second != receivingFederateId ||
        !federation->second.attributeHandles->nameFor(
            federation->second.definition.catalog.get(),
            *knownClassName,
            attributeHandle) ||
        !publishedAttributes->contains(attributeHandle)) {
      return std::nullopt;
    }
  }

  auto securedAttributeHandles = std::move(notification->second.attributeHandles);
  instance->second.pendingConfirmDivestitureNotifications.erase(notification);
  return ConfirmDivestitureNotificationDelivery{
      objectInstanceHandle,
      std::move(securedAttributeHandles),
      planPendingAttributeOwnershipAcquisitionWork(federation->second, instance->second),
  };
}

CancelNegotiatedAttributeOwnershipDivestiturePlan
EmbeddedFederationRegistry::planCancelNegotiatedAttributeOwnershipDivestiture(
    std::wstring const& federationName,
    std::uint64_t divestingFederateId,
    std::uint64_t objectInstanceHandle,
    std::set<std::uint64_t> const& attributeHandles) {
  std::scoped_lock lock(mutex_);
  auto federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    return {CancelNegotiatedAttributeOwnershipDivestitureStatus::federation_does_not_exist};
  }
  if (!federation->second.members.contains(divestingFederateId)) {
    return {CancelNegotiatedAttributeOwnershipDivestitureStatus::divesting_federate_not_member};
  }
  // IEEE 1516.1-2025 11.4.2(k) prohibits ownership transfer of predefined
  // RTI-owned MOM attributes.
  if (federation->second.rtiOwnedJoinedFederateMomObjects.contains(objectInstanceHandle)) {
    return {CancelNegotiatedAttributeOwnershipDivestitureStatus::attribute_owned_by_rti};
  }
  auto instance = federation->second.objectInstances.find(objectInstanceHandle);
  if (instance == federation->second.objectInstances.end() || instance->second.deleteAccepted ||
      !instance->second.knownObjectClassHandlesByFederate.contains(divestingFederateId)) {
    return {CancelNegotiatedAttributeOwnershipDivestitureStatus::object_instance_not_known};
  }
  if (!federation->second.definition.catalog ||
      !federation->second.objectClassHandles ||
      !federation->second.attributeHandles) {
    return {CancelNegotiatedAttributeOwnershipDivestitureStatus::inconsistent_catalog};
  }

  auto const knownClass = instance->second.knownObjectClassHandlesByFederate.find(
      divestingFederateId);
  auto const knownClassName = federation->second.objectClassHandles->nameFor(knownClass->second);
  if (!knownClassName ||
      federation->second.definition.catalog->objectClass(*knownClassName) == nullptr) {
    return {CancelNegotiatedAttributeOwnershipDivestitureStatus::inconsistent_catalog};
  }
  for (std::uint64_t const attributeHandle : attributeHandles) {
    if (!federation->second.attributeHandles->nameFor(
            federation->second.definition.catalog.get(),
            *knownClassName,
            attributeHandle)) {
      return {CancelNegotiatedAttributeOwnershipDivestitureStatus::attribute_not_defined};
    }
    auto const owner = instance->second.attributeOwnersByHandle.find(attributeHandle);
    if (owner == instance->second.attributeOwnersByHandle.end() ||
        owner->second != divestingFederateId) {
      return {CancelNegotiatedAttributeOwnershipDivestitureStatus::attribute_not_owned};
    }
    auto const divestiture = instance->second.pendingNegotiatedAttributeOwnershipDivestitures.find(
        attributeHandle);
    if (divestiture == instance->second.pendingNegotiatedAttributeOwnershipDivestitures.end() ||
        divestiture->second.divestingFederateId != divestingFederateId) {
      return {
          CancelNegotiatedAttributeOwnershipDivestitureStatus::
              attribute_divestiture_was_not_requested};
    }
  }

  if (attributeHandles.empty()) {
    return {};
  }
  for (std::uint64_t const attributeHandle : attributeHandles) {
    instance->second.pendingNegotiatedAttributeOwnershipDivestitures.erase(attributeHandle);
    // Canceling the offer also terminates any queued Request Attribute
    // Ownership Assumption callback that was seeded for this negotiated
    // interval. The owner remains owner, so retaining the search ledger would
    // otherwise allow a stale callback or a later join to re-offer it.
    clearOwnershipAssumptionSearch(instance->second, attributeHandle);
  }
  CancelNegotiatedAttributeOwnershipDivestiturePlan result;
  result.followupWorkItems = planPendingAttributeOwnershipAcquisitionWork(
      federation->second,
      instance->second);
  return result;
}

}  // namespace umbra::detail
