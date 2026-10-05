#include "internal/federation/federation_registry.hpp"

#include <cstdint>
#include <limits>
#include <mutex>
#include <string>

namespace umbra::detail {
ObjectInstanceDeletionPlan EmbeddedFederationRegistry::deleteObjectInstance(
    std::wstring const& federationName,
    std::uint64_t deletingFederateId,
    std::uint64_t objectInstanceHandle) {
  auto instrumentationScope = beginInstrumentation("deleteObjectInstance");
  std::scoped_lock lock(mutex_);
  auto federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    return {ObjectInstanceDeletionStatus::federation_does_not_exist};
  }
  if (!federation->second.members.contains(deletingFederateId)) {
    return {ObjectInstanceDeletionStatus::federate_not_member};
  }
  auto instance = federation->second.objectInstances.find(objectInstanceHandle);
  if (instance == federation->second.objectInstances.end() ||
      instance->second.deleteAccepted ||
      instance->second.pendingTimestampedDeletionMessageId.has_value() ||
      !instance->second.knownObjectClassHandlesByFederate.contains(deletingFederateId)) {
    return {ObjectInstanceDeletionStatus::object_instance_not_known};
  }
  if (!federation->second.definition.catalog ||
      !federation->second.objectClassHandles ||
      !federation->second.attributeHandles) {
    return {ObjectInstanceDeletionStatus::inconsistent_catalog};
  }

  auto const objectClassName = federation->second.objectClassHandles->nameFor(
      instance->second.registeredObjectClassHandle);
  if (!objectClassName) {
    return {ObjectInstanceDeletionStatus::inconsistent_catalog};
  }
  auto const privilegeToDelete = federation->second.attributeHandles->handleFor(
      federation->second.definition.catalog.get(),
      *objectClassName,
      "HLAprivilegeToDeleteObject");
  if (!privilegeToDelete) {
    return {ObjectInstanceDeletionStatus::inconsistent_catalog};
  }
  auto const privilegeOwner = instance->second.attributeOwnersByHandle.find(*privilegeToDelete);
  if (privilegeOwner == instance->second.attributeOwnersByHandle.end() ||
      privilegeOwner->second != deletingFederateId) {
    return {ObjectInstanceDeletionStatus::delete_privilege_not_held};
  }

  ObjectInstanceDeletionPlan result;
  result.recipients.reserve(instance->second.knownObjectClassHandlesByFederate.size());
  for (auto const& [receivingFederateId, knownObjectClassHandle] :
       instance->second.knownObjectClassHandlesByFederate) {
    static_cast<void>(knownObjectClassHandle);
    if (receivingFederateId == deletingFederateId ||
        !federation->second.members.contains(receivingFederateId)) {
      continue;
    }
    auto const callbackRoute = federation->second.interactionCallbackRoutes.find(receivingFederateId);
    if (callbackRoute == federation->second.interactionCallbackRoutes.end() ||
        !callbackRoute->second) {
      continue;
    }
    auto const reportRoute = federation->second.serviceReportRoutes.find(receivingFederateId);
    result.recipients.push_back({
        receivingFederateId,
        instance->second.handle,
        callbackRoute->second,
        reportRoute == federation->second.serviceReportRoutes.end()
            ? FederateServiceReportRoute{}
            : reportRoute->second,
    });
  }

  try {
    for (auto const& recipient : result.recipients) {
      auto const [pending, insertedPending] =
          instance->second.pendingRemovalFederates.insert(recipient.receivingFederateId);
      static_cast<void>(pending);
      if (!insertedPending) {
        for (auto const& reserved : result.recipients) {
          instance->second.pendingRemovalFederates.erase(reserved.receivingFederateId);
        }
        return {ObjectInstanceDeletionStatus::inconsistent_catalog};
      }
    }
  } catch (...) {
    for (auto const& recipient : result.recipients) {
      instance->second.pendingRemovalFederates.erase(recipient.receivingFederateId);
    }
    throw;
  }

  // Delete is no longer eligible to induce discovery. The deleting federate
  // becomes unknown immediately; other known federates remain known only
  // until beginObjectInstanceRemoval commits their queued callback.
  instance->second.pendingDiscoveryFederates.clear();
  instance->second.pendingAttributeValueUpdateRequests.clear();
  instance->second.pendingAttributeValueUpdateClassRequests.clear();
  instance->second.pendingAttributeValueUpdateRegionalRequests.clear();
  clearPendingAttributeOwnershipQueries(
      federation->second,
      instance->second.handle);
  // A receive-order deletion invalidates every pending ownership-acquisition
  // callback before it can make a deleted object appear to change owner.
  instance->second.pendingAttributeOwnershipAcquisitionIfAvailableRequests.clear();
  instance->second.pendingAttributeOwnershipAcquisitionRequests.clear();
  instance->second.pendingAttributeOwnershipAcquisitionCancellations.clear();
  instance->second.pendingAttributeOwnershipDivestitureIfWantedNotifications.clear();
  instance->second.pendingNegotiatedAttributeOwnershipDivestitures.clear();
  instance->second.pendingConfirmDivestitureNotifications.clear();
  instance->second.knownObjectClassHandlesByFederate.erase(deletingFederateId);
  instance->second.deleteAccepted = true;
  auto const deletingMember = federation->second.members.find(deletingFederateId);
  if (deletingMember != federation->second.members.end() &&
      deletingMember->second.successfulObjectInstanceDeletionsCount !=
          std::numeric_limits<std::uint64_t>::max()) {
    ++deletingMember->second.successfulObjectInstanceDeletionsCount;
  }
  if (canPurgeDeletedObjectInstance(instance->second)) {
    federation->second.objectInstanceHandlesByName.erase(instance->second.name);
    federation->second.objectInstances.erase(instance);
  }
  // A receive-order deletion may purge the object immediately when no other
  // federate has a pending removal callback.  Recompute the region usage
  // ledger before returning so regions associated only with that object are
  // immediately deletable, while surviving object/subscription associations
  // remain protected by the in-use guard.
  refreshRegionUsage(federation->second);
  return result;
}

LocalObjectInstanceDeletionStatus EmbeddedFederationRegistry::localDeleteObjectInstance(
    std::wstring const& federationName,
    std::uint64_t deletingFederateId,
    std::uint64_t objectInstanceHandle) {
  auto instrumentationScope = beginInstrumentation("localDeleteObjectInstance");
  std::scoped_lock lock(mutex_);
  auto federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    return LocalObjectInstanceDeletionStatus::federation_does_not_exist;
  }
  if (!federation->second.members.contains(deletingFederateId)) {
    return LocalObjectInstanceDeletionStatus::federate_not_member;
  }

  auto instance = federation->second.objectInstances.find(objectInstanceHandle);
  if (instance == federation->second.objectInstances.end() ||
      instance->second.deleteAccepted ||
      !instance->second.knownObjectClassHandlesByFederate.contains(deletingFederateId)) {
    return LocalObjectInstanceDeletionStatus::object_instance_not_known;
  }

  // Local deletion is not allowed to discard an in-flight ownership request.
  // Check every private acquisition/divestiture reservation that can still
  // change ownership or deliver an ownership callback for this federate.
  for (auto const& [requestId, request] :
       instance->second.pendingAttributeOwnershipAcquisitionIfAvailableRequests) {
    static_cast<void>(requestId);
    if (request.requestingFederateId == deletingFederateId) {
      return LocalObjectInstanceDeletionStatus::ownership_acquisition_pending;
    }
  }
  for (auto const& [requestId, request] :
       instance->second.pendingAttributeOwnershipAcquisitionRequests) {
    static_cast<void>(requestId);
    if (request.requestingFederateId == deletingFederateId) {
      return LocalObjectInstanceDeletionStatus::ownership_acquisition_pending;
    }
  }
  for (auto const& [cancellationId, cancellation] :
       instance->second.pendingAttributeOwnershipAcquisitionCancellations) {
    static_cast<void>(cancellationId);
    if (cancellation.requestingFederateId == deletingFederateId) {
      return LocalObjectInstanceDeletionStatus::ownership_acquisition_pending;
    }
  }
  for (auto const& [notificationId, notification] :
       instance->second.pendingAttributeOwnershipDivestitureIfWantedNotifications) {
    static_cast<void>(notificationId);
    if (notification.receivingFederateId == deletingFederateId) {
      return LocalObjectInstanceDeletionStatus::ownership_acquisition_pending;
    }
  }
  for (auto const& [requestId, divestiture] :
       instance->second.pendingNegotiatedAttributeOwnershipDivestitures) {
    static_cast<void>(requestId);
    if (divestiture.divestingFederateId == deletingFederateId ||
        divestiture.acquiringFederateId == deletingFederateId) {
      return LocalObjectInstanceDeletionStatus::ownership_acquisition_pending;
    }
  }
  for (auto const& [notificationId, notification] :
       instance->second.pendingConfirmDivestitureNotifications) {
    static_cast<void>(notificationId);
    if (notification.receivingFederateId == deletingFederateId) {
      return LocalObjectInstanceDeletionStatus::ownership_acquisition_pending;
    }
  }

  for (auto const& [attributeHandle, owningFederateId] : instance->second.attributeOwnersByHandle) {
    static_cast<void>(attributeHandle);
    if (owningFederateId == deletingFederateId) {
      return LocalObjectInstanceDeletionStatus::federate_owns_attributes;
    }
  }

  // Keep the execution-wide instance and every other federate's knowledge
  // untouched. A later eligible subscription can plan a fresh discovery.
  instance->second.knownObjectClassHandlesByFederate.erase(deletingFederateId);
  instance->second.pendingDiscoveryFederates.erase(deletingFederateId);
  return LocalObjectInstanceDeletionStatus::applied;
}

}  // namespace umbra::detail
