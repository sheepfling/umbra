#include "internal/federation/federation_registry.hpp"
#include "internal/fom/fom_catalog.hpp"

#include <iterator>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <utility>

namespace umbra::detail {

FederationRegistryResult EmbeddedFederationRegistry::resignLocked(
    std::wstring const& federationName,
    std::uint64_t federateId,
    rti1516_2025::ResignAction resignAction,
    bool forcedConnectionLoss,
    std::optional<std::uint16_t> finalServiceReportGroup,
    bool reservePublicInteraction) {
  auto federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    return {FederationRegistryStatus::federation_does_not_exist};
  }

  auto member = federation->second.members.find(federateId);
  if (member == federation->second.members.end()) {
    return {FederationRegistryStatus::federate_not_member};
  }

  // Capture the time-regulating member's last granted position before the
  // forced resignation unregisters its temporal state. IEEE 1516.1-2025 4.4
  // requires delivery of its TSO messages at or before this point; messages
  // later than it intentionally receive no delivery guarantee.
  std::shared_ptr<rti1516_2025::LogicalTime const> connectionLossCutoff;
  if (forcedConnectionLoss) {
    for (auto const& candidate : federation->second.timeCoordinator.snapshot()) {
      if (candidate.federateId != federateId ||
          !candidate.time.timeRegulating ||
          !candidate.time.currentTime ||
          candidate.time.currentTime->implementationName() !=
              federation->second.definition.logicalTimeImplementationName) {
        continue;
      }
      connectionLossCutoff = candidate.time.currentTime;
      break;
    }
  }

  auto const retainPendingTimestampedDeletionForConnectionLoss =
      [&](Federation::ObjectInstance const& objectInstance) {
        if (!forcedConnectionLoss || !connectionLossCutoff ||
            !objectInstance.pendingTimestampedDeletionMessageId.has_value()) {
          return false;
        }
        auto const deletion = federation->second.tsoObjectDeletionMessages.find(
            *objectInstance.pendingTimestampedDeletionMessageId);
        if (deletion == federation->second.tsoObjectDeletionMessages.end() ||
            deletion->second.producingFederateId != federateId ||
            !deletion->second.timestamp) {
          return false;
        }
        try {
          // Clause 4.4's at-or-before-loss obligation applies to every TSO
          // message family. The accepted deletion and its object state must
          // survive every automatic-resign cleanup pass until the common
          // retraction-ledger pass marks its surviving recipients.
          return *deletion->second.timestamp <= *connectionLossCutoff;
        } catch (rti1516_2025::Exception const&) {
          // Do not manufacture the mandatory-delivery exception when a
          // private timestamp comparison is malformed.
          return false;
        }
      };

  FederationRegistryResult result;
  bool deleteObjects = false;
  bool divestAttributes = false;
  bool cancelPendingAcquisitions = false;
  switch (resignAction) {
    case rti1516_2025::UNCONDITIONALLY_DIVEST_ATTRIBUTES:
      divestAttributes = true;
      break;
    case rti1516_2025::DELETE_OBJECTS:
      deleteObjects = true;
      break;
    case rti1516_2025::CANCEL_PENDING_OWNERSHIP_ACQUISITIONS:
      cancelPendingAcquisitions = true;
      break;
    case rti1516_2025::DELETE_OBJECTS_THEN_DIVEST:
      deleteObjects = true;
      divestAttributes = true;
      break;
    case rti1516_2025::CANCEL_THEN_DELETE_THEN_DIVEST:
      cancelPendingAcquisitions = true;
      deleteObjects = true;
      divestAttributes = true;
      break;
    case rti1516_2025::NO_ACTION:
      break;
    default:
      return {FederationRegistryStatus::invalid_resign_action};
  }

  // A transport failure cannot leave private acquisition work attached to a
  // member that no longer exists. The automatic directive still controls
  // object deletion/divestiture, while this forced cleanup removes the lost
  // member's outstanding acquisition-side state.
  if (forcedConnectionLoss) {
    cancelPendingAcquisitions = true;
  }

  // IEEE 1516.1-2025 4.12.4 requires the RTI to process directive 2 when
  // the resigning federate is the last joined federate, regardless of the
  // action value supplied by that federate.  Keep the supplied directive's
  // other effects (for example, directive 1's divestiture) but add the
  // mandatory delete pass before the member is removed.
  bool const lastJoinedFederate = federation->second.members.size() == 1;
  if (lastJoinedFederate) {
    deleteObjects = true;
  }

  // A voluntary resignation that does not delete objects must leave an
  // accepted timestamped deletion payload available to every recipient that
  // still has the deletion queued.  The temporal queue is federation-owned;
  // removing the producing federate must not make those queue entries point at
  // a missing typed payload.  The common retraction-ledger pass below marks
  // the producer as departed, while each recipient consumes its own pending
  // removal at its normal timestamp boundary.
  auto const retainPendingTimestampedDeletionForVoluntaryDeparture =
      [&](Federation::ObjectInstance const& objectInstance) {
        if (forcedConnectionLoss || deleteObjects ||
            !objectInstance.pendingTimestampedDeletionMessageId.has_value()) {
          return false;
        }
        auto const deletion = federation->second.tsoObjectDeletionMessages.find(
            *objectInstance.pendingTimestampedDeletionMessageId);
        return deletion != federation->second.tsoObjectDeletionMessages.end() &&
            deletion->second.producingFederateId == federateId &&
            static_cast<bool>(deletion->second.timestamp);
      };

  // 4.12.3 requires the RTI to reject actions that would leave an acquisition
  // attempt unresolved.  The request maps below are the private state for
  // regular and If Available acquisition; negotiated acquisition is included
  // when this federate is the prospective acquiring federate.
  bool pendingAcquisition = false;
  for (auto const& [objectInstanceHandle, objectInstance] :
       federation->second.objectInstances) {
    static_cast<void>(objectInstanceHandle);
    for (auto const& [requestId, request] :
         objectInstance.pendingAttributeOwnershipAcquisitionIfAvailableRequests) {
      static_cast<void>(requestId);
      if (request.requestingFederateId == federateId) {
        pendingAcquisition = true;
        break;
      }
    }
    if (!pendingAcquisition) {
      for (auto const& [requestId, request] :
           objectInstance.pendingAttributeOwnershipAcquisitionRequests) {
        static_cast<void>(requestId);
        if (request.requestingFederateId == federateId) {
          pendingAcquisition = true;
          break;
        }
      }
    }
    if (!pendingAcquisition) {
      for (auto const& [cancellationId, cancellation] :
           objectInstance.pendingAttributeOwnershipAcquisitionCancellations) {
        static_cast<void>(cancellationId);
        if (cancellation.requestingFederateId == federateId) {
          pendingAcquisition = true;
          break;
        }
      }
    }
    if (!pendingAcquisition) {
      for (auto const& [attributeHandle, divestiture] :
           objectInstance.pendingNegotiatedAttributeOwnershipDivestitures) {
        static_cast<void>(attributeHandle);
        if (divestiture.acquiringFederateId == federateId) {
          pendingAcquisition = true;
          break;
        }
      }
    }
    if (!pendingAcquisition) {
      for (auto const& [notificationId, notification] :
           objectInstance.pendingConfirmDivestitureNotifications) {
        static_cast<void>(notificationId);
        if (notification.receivingFederateId == federateId) {
          pendingAcquisition = true;
          break;
        }
      }
    }
    if (pendingAcquisition) {
      break;
    }
  }
  if (!forcedConnectionLoss && pendingAcquisition &&
      (resignAction == rti1516_2025::UNCONDITIONALLY_DIVEST_ATTRIBUTES ||
       resignAction == rti1516_2025::DELETE_OBJECTS ||
       resignAction == rti1516_2025::DELETE_OBJECTS_THEN_DIVEST ||
       resignAction == rti1516_2025::NO_ACTION)) {
    return {FederationRegistryStatus::ownership_acquisition_pending};
  }

  std::set<std::uint64_t> objectsToDelete;
  std::map<std::uint64_t, std::set<std::uint64_t>> ownedAttributesByObject;
  std::map<std::uint64_t, bool> deletePrivilegeByObject;
  for (auto const& [objectInstanceHandle, objectInstance] :
       federation->second.objectInstances) {
    if (objectInstance.deleteAccepted) {
      continue;
    }
    if (!federation->second.definition.catalog ||
        !federation->second.objectClassHandles ||
        !federation->second.attributeHandles) {
      return {FederationRegistryStatus::invalid_request};
    }
    auto const objectClassName = federation->second.objectClassHandles->nameFor(
        objectInstance.registeredObjectClassHandle);
    if (!objectClassName) {
      return {FederationRegistryStatus::invalid_request};
    }
    auto const privilegeToDelete = federation->second.attributeHandles->handleFor(
        federation->second.definition.catalog.get(),
        *objectClassName,
        "HLAprivilegeToDeleteObject");
    if (!privilegeToDelete) {
      return {FederationRegistryStatus::invalid_request};
    }
    auto const privilegeOwner = objectInstance.attributeOwnersByHandle.find(*privilegeToDelete);
    bool const hasDeletePrivilege =
        privilegeOwner != objectInstance.attributeOwnersByHandle.end() &&
        privilegeOwner->second == federateId;
    deletePrivilegeByObject.emplace(objectInstanceHandle, hasDeletePrivilege);
    if (hasDeletePrivilege && deleteObjects &&
        !retainPendingTimestampedDeletionForConnectionLoss(objectInstance)) {
      objectsToDelete.insert(objectInstanceHandle);
    }
    for (auto const& [attributeHandle, owner] : objectInstance.attributeOwnersByHandle) {
      if (owner == federateId) {
        ownedAttributesByObject[objectInstanceHandle].insert(attributeHandle);
      }
    }
  }

  bool ownsAttributes = false;
  for (auto const& [objectInstanceHandle, attributes] : ownedAttributesByObject) {
    static_cast<void>(objectInstanceHandle);
    if (!attributes.empty()) {
      ownsAttributes = true;
      break;
    }
  }
  if (forcedConnectionLoss && ownsAttributes) {
    // Once the member is forced out, any attributes it still owns must leave
    // the member's ownership set even when its configured directive was
    // NO_ACTION or DELETE_OBJECTS without delete privilege.
    divestAttributes = true;
  }
  if (!forcedConnectionLoss && !deleteObjects && !divestAttributes && ownsAttributes) {
    return {FederationRegistryStatus::federate_owns_attributes};
  }
  // Directive 2 is intentionally stricter than the combined delete-then-
  // divest forms: it can only delete objects for which the resigning federate
  // owns the delete privilege, and it may not strand another owned attribute.
  // The same rule applies to the mandatory final-federate directive-2 pass.
  if (!forcedConnectionLoss && deleteObjects && !divestAttributes) {
    for (auto const& [objectInstanceHandle, attributes] : ownedAttributesByObject) {
      if (!attributes.empty() && !deletePrivilegeByObject[objectInstanceHandle]) {
        return {FederationRegistryStatus::federate_owns_attributes};
      }
    }
  }

  // Validate every callback route needed by the action before changing any
  // object or ownership state.  This keeps an embedded profile failure
  // atomic and avoids an object becoming deleted without a delivery route.
  if (deleteObjects) {
    for (std::uint64_t const objectInstanceHandle : objectsToDelete) {
      auto const objectInstance = federation->second.objectInstances.find(objectInstanceHandle);
      if (objectInstance == federation->second.objectInstances.end()) {
        return {FederationRegistryStatus::invalid_request};
      }
      for (auto const& [receivingFederateId, knownClassHandle] :
           objectInstance->second.knownObjectClassHandlesByFederate) {
        static_cast<void>(knownClassHandle);
        if (receivingFederateId == federateId ||
            !federation->second.members.contains(receivingFederateId)) {
          continue;
        }
        auto const route = federation->second.interactionCallbackRoutes.find(receivingFederateId);
        if (route == federation->second.interactionCallbackRoutes.end() || !route->second) {
          if (forcedConnectionLoss) {
            continue;
          }
          return {FederationRegistryStatus::invalid_request};
        }
      }
    }
  }

  // RTI-owned joined-federate MOM objects leave the execution whenever their
  // represented joined federate resigns, independent of the supplied
  // federate-created-object resign action. Validate the surviving callback
  // routes before mutating the MOM ledger so a normal resignation remains
  // atomic when a removal cannot be queued.
  for (auto const& [objectInstanceHandle, object] :
       federation->second.rtiOwnedJoinedFederateMomObjects) {
    static_cast<void>(objectInstanceHandle);
    if (object.federationExecutionObject || object.joinedFederateId != federateId) {
      continue;
    }
    for (std::uint64_t const receivingFederateId : object.knownFederateIds) {
      if (receivingFederateId == federateId ||
          !federation->second.members.contains(receivingFederateId)) {
        continue;
      }
      auto const route = federation->second.interactionCallbackRoutes.find(
          receivingFederateId);
      if (route == federation->second.interactionCallbackRoutes.end() ||
          !route->second) {
        if (forcedConnectionLoss) {
          continue;
        }
        return {FederationRegistryStatus::invalid_request};
      }
    }
  }

  // Build the complete assumption-offer set before any resignation mutation.
  // In particular, an unconditional or combined delete/divest action must
  // not clear an earlier attribute and then discover that a later eligible
  // recipient has no callback route.  The returned error is an internal
  // delivery-precondition failure, so the federation state remains atomic.
  std::map<std::pair<std::uint64_t, std::uint64_t>, std::set<std::uint64_t>>
      offeredAttributesByRecipient;
  std::map<std::pair<std::uint64_t, std::uint64_t>, FederateCallbackRoute>
      offeredCallbackRoutes;
  auto recipientHasPendingAcquisition = [](
                                             Federation::ObjectInstance const& instance,
                                             std::uint64_t receivingFederateId,
                                             std::uint64_t attributeHandle) {
    for (auto const& [requestId, pending] :
         instance.pendingAttributeOwnershipAcquisitionIfAvailableRequests) {
      static_cast<void>(requestId);
      if (pending.requestingFederateId == receivingFederateId &&
          pending.desiredAttributeHandles.contains(attributeHandle)) {
        return true;
      }
    }
    for (auto const& [requestId, pending] :
         instance.pendingAttributeOwnershipAcquisitionRequests) {
      static_cast<void>(requestId);
      if (pending.requestingFederateId == receivingFederateId &&
          pending.desiredAttributeHandles.contains(attributeHandle)) {
        return true;
      }
    }
    for (auto const& [cancellationId, cancellation] :
         instance.pendingAttributeOwnershipAcquisitionCancellations) {
      static_cast<void>(cancellationId);
      if (cancellation.requestingFederateId == receivingFederateId &&
          cancellation.attributeHandles.contains(attributeHandle)) {
        return true;
      }
    }
    return false;
  };

  if (divestAttributes) {
    for (auto const& [objectInstanceHandle, ownedAttributes] : ownedAttributesByObject) {
      // A delete pass makes this object unavailable for assumption search;
      // skip it while its removal route is still covered by the preflight
      // above.  A retained deleteAccepted object is skipped by the mutation
      // pass for the same reason.
      if (objectsToDelete.contains(objectInstanceHandle)) {
        continue;
      }
      auto const objectInstance = federation->second.objectInstances.find(objectInstanceHandle);
      if (objectInstance == federation->second.objectInstances.end() ||
          objectInstance->second.deleteAccepted) {
        continue;
      }

      for (std::uint64_t const attributeHandle : ownedAttributes) {
        for (auto const& [receivingFederateId, membership] : federation->second.members) {
          static_cast<void>(membership);
          if (receivingFederateId == federateId ||
              recipientHasPendingAcquisition(
                  objectInstance->second,
                  receivingFederateId,
                  attributeHandle)) {
            continue;
          }
          auto const knownClass = objectInstance->second.knownObjectClassHandlesByFederate.find(
              receivingFederateId);
          if (knownClass == objectInstance->second.knownObjectClassHandlesByFederate.end()) {
            continue;
          }
          auto const knownClassName = federation->second.objectClassHandles->nameFor(
              knownClass->second);
          auto const publishedAttributes = knownClassName
              ? publishedObjectClassAttributes(
                    federation->second,
                    receivingFederateId,
                    knownClass->second)
              : std::nullopt;
          if (!knownClassName || !publishedAttributes ||
              !federation->second.attributeHandles->nameFor(
                  federation->second.definition.catalog.get(),
                  *knownClassName,
                  attributeHandle) ||
              !publishedAttributes->contains(attributeHandle)) {
            continue;
          }
          auto const route = federation->second.interactionCallbackRoutes.find(
              receivingFederateId);
          if (route == federation->second.interactionCallbackRoutes.end() ||
              !route->second) {
            if (forcedConnectionLoss) {
              continue;
            }
            return {FederationRegistryStatus::invalid_request};
          }
          auto const recipientKey = std::make_pair(receivingFederateId, objectInstanceHandle);
          offeredAttributesByRecipient[recipientKey].insert(attributeHandle);
          offeredCallbackRoutes.emplace(recipientKey, route->second);
        }
      }
    }
  }

  if (federation->second.saveOperation.has_value()) {
    // A resigning participant makes the control-plane save fail.  Do this
    // before removing its callback route, but do not enqueue a callback back
    // to the federate that is leaving the execution.
    appendSaveCompletionNotifications(
        federation->second,
        false,
        rti1516_2025::FEDERATE_RESIGNED_DURING_SAVE,
        result.saveNotifications,
        federateId);
    federation->second.saveOperation.reset();
  }
  if (federation->second.restoreOperation.has_value()) {
    // A resigning participant invalidates an in-flight restore for the
    // remaining members before its callback route is removed.
    appendRestoreCompletionNotifications(
        federation->second,
        false,
        rti1516_2025::FEDERATE_RESIGNED_DURING_RESTORE,
        result.restoreNotifications,
        federateId);
    federation->second.restoreOperation.reset();
  }

  if (cancelPendingAcquisitions) {
    // Directive 3 (and the combined directive 5) cancels only acquisition
    // work initiated by the resigning federate.  Requests made by other
    // joined federates remain eligible for any attributes that are later
    // divested by this resignation.
    for (auto& [objectInstanceHandle, objectInstance] : federation->second.objectInstances) {
      static_cast<void>(objectInstanceHandle);
      for (auto pending =
               objectInstance.pendingAttributeOwnershipAcquisitionIfAvailableRequests.begin();
           pending != objectInstance.pendingAttributeOwnershipAcquisitionIfAvailableRequests.end();) {
        if (pending->second.requestingFederateId == federateId) {
          pending = objectInstance.pendingAttributeOwnershipAcquisitionIfAvailableRequests.erase(
              pending);
        } else {
          ++pending;
        }
      }
      for (auto pending = objectInstance.pendingAttributeOwnershipAcquisitionRequests.begin();
           pending != objectInstance.pendingAttributeOwnershipAcquisitionRequests.end();) {
        if (pending->second.requestingFederateId == federateId) {
          pending = objectInstance.pendingAttributeOwnershipAcquisitionRequests.erase(pending);
        } else {
          ++pending;
        }
      }
      for (auto pending =
               objectInstance.pendingAttributeOwnershipAcquisitionCancellations.begin();
           pending != objectInstance.pendingAttributeOwnershipAcquisitionCancellations.end();) {
        if (pending->second.requestingFederateId == federateId) {
          pending = objectInstance.pendingAttributeOwnershipAcquisitionCancellations.erase(pending);
        } else {
          ++pending;
        }
      }
      for (auto pending =
               objectInstance.pendingAttributeOwnershipDivestitureIfWantedNotifications.begin();
           pending != objectInstance.pendingAttributeOwnershipDivestitureIfWantedNotifications.end();) {
        if (pending->second.receivingFederateId == federateId) {
          pending = objectInstance.pendingAttributeOwnershipDivestitureIfWantedNotifications.erase(
              pending);
        } else {
          ++pending;
        }
      }
      for (auto pending =
               objectInstance.pendingNegotiatedAttributeOwnershipDivestitures.begin();
           pending != objectInstance.pendingNegotiatedAttributeOwnershipDivestitures.end();) {
        if (pending->second.acquiringFederateId == federateId) {
          auto const attributeHandle = pending->first;
          pending = objectInstance.pendingNegotiatedAttributeOwnershipDivestitures.erase(pending);
          // The negotiated candidate owned the assumption-search interval for
          // this still-owned attribute.  Once that candidate resigns, keep
          // the owner in place but remove the stale search/tag ledger so a
          // later save/restore cannot replay a transfer that no longer has a
          // negotiated recipient.
          clearOwnershipAssumptionSearch(objectInstance, attributeHandle);
        } else {
          ++pending;
        }
      }
      for (auto pending = objectInstance.pendingConfirmDivestitureNotifications.begin();
           pending != objectInstance.pendingConfirmDivestitureNotifications.end();) {
        if (pending->second.receivingFederateId == federateId) {
          pending = objectInstance.pendingConfirmDivestitureNotifications.erase(pending);
        } else {
          ++pending;
        }
      }
    }
  }

  // A queued assumption callback targeted at the resigning federate can no
  // longer reach a live route. Drop only that recipient's pending marker;
  // other candidates remain part of the unowned search epoch.
  for (auto& [objectInstanceHandle, objectInstance] : federation->second.objectInstances) {
    static_cast<void>(objectInstanceHandle);
    for (auto pending = objectInstance.pendingAttributeOwnershipAssumptionCallbacks.begin();
         pending != objectInstance.pendingAttributeOwnershipAssumptionCallbacks.end();) {
      if (pending->receivingFederateId == federateId) {
        pending = objectInstance.pendingAttributeOwnershipAssumptionCallbacks.erase(pending);
      } else {
        ++pending;
      }
    }
  }

  // Query Attribute Ownership callbacks are tied to the requesting federate
  // and (for concrete-owner reports) to the owner identity. A resignation
  // invalidates both relationships before the departing member route is
  // removed; stale queued callbacks will therefore be suppressed.
  for (auto query = federation->second.pendingAttributeOwnershipQueries.begin();
       query != federation->second.pendingAttributeOwnershipQueries.end();) {
    if (query->second.requestingFederateId == federateId ||
        query->second.owningFederateId == federateId) {
      query = federation->second.pendingAttributeOwnershipQueries.erase(query);
    } else {
      ++query;
    }
  }

  // Delete-privileged objects are handled before any remaining ownership is
  // divested, matching the ordering of directives 4 and 5.  The registry
  // reserves one Remove Object Instance callback per remaining known
  // federate; the adapter submits those callbacks after this lock is gone.
  if (deleteObjects) {
    for (std::uint64_t const objectInstanceHandle : objectsToDelete) {
      auto objectInstance = federation->second.objectInstances.find(objectInstanceHandle);
      if (objectInstance == federation->second.objectInstances.end() ||
          objectInstance->second.deleteAccepted) {
        continue;
      }

      auto& instance = objectInstance->second;
      for (auto const& [receivingFederateId, knownClassHandle] :
           instance.knownObjectClassHandlesByFederate) {
        static_cast<void>(knownClassHandle);
        if (receivingFederateId == federateId ||
            !federation->second.members.contains(receivingFederateId)) {
          continue;
        }
        auto const route = federation->second.interactionCallbackRoutes.find(receivingFederateId);
        if (route == federation->second.interactionCallbackRoutes.end() || !route->second) {
          if (forcedConnectionLoss) {
            continue;
          }
          return {FederationRegistryStatus::invalid_request};
        }
        instance.pendingRemovalFederates.insert(receivingFederateId);
        if (forcedConnectionLoss) {
          instance.connectionLossAutomaticRemovalFederates.insert(receivingFederateId);
        }
        auto const reportRoute = federation->second.serviceReportRoutes.find(
            receivingFederateId);
        result.resignObjectRemovals.push_back({
            receivingFederateId,
            objectInstanceHandle,
            route->second,
            reportRoute == federation->second.serviceReportRoutes.end()
                ? FederateServiceReportRoute{}
                : reportRoute->second,
        });
      }

      instance.pendingDiscoveryFederates.clear();
      instance.pendingAttributeValueUpdateRequests.clear();
      instance.pendingAttributeValueUpdateClassRequests.clear();
      instance.pendingAttributeValueUpdateRegionalRequests.clear();
      clearPendingAttributeOwnershipQueries(
          federation->second,
          objectInstanceHandle);
      instance.pendingAttributeOwnershipAcquisitionIfAvailableRequests.clear();
      instance.pendingAttributeOwnershipAcquisitionRequests.clear();
      instance.pendingAttributeOwnershipAcquisitionCancellations.clear();
      instance.pendingAttributeOwnershipDivestitureIfWantedNotifications.clear();
      instance.pendingNegotiatedAttributeOwnershipDivestitures.clear();
      instance.pendingConfirmDivestitureNotifications.clear();
      instance.pendingAttributeOwnershipAssumptionCallbacks.clear();
      if (instance.pendingTimestampedDeletionMessageId.has_value()) {
        auto const deletionMessageId = *instance.pendingTimestampedDeletionMessageId;
        federation->second.tsoObjectDeletionMessages.erase(
            deletionMessageId);
        federation->second.tsoObjectDeletionReconstitutionRecords.erase(deletionMessageId);
        federation->second.tsoRequestRetractionRecords.erase(deletionMessageId);
        instance.pendingTimestampedDeletionMessageId.reset();
      }
      instance.pendingTimestampedRemovalFederates.clear();
      instance.knownObjectClassHandlesByFederate.erase(federateId);
      instance.deleteAccepted = true;
      if (canPurgeDeletedObjectInstance(instance)) {
        federation->second.objectInstanceHandlesByName.erase(instance.name);
        federation->second.objectInstances.erase(objectInstance);
      }
    }
  }

  if (divestAttributes) {
    for (auto objectInstance = federation->second.objectInstances.begin();
         objectInstance != federation->second.objectInstances.end();) {
      auto& instance = objectInstance->second;
      if (instance.deleteAccepted) {
        ++objectInstance;
        continue;
      }
      auto owned = ownedAttributesByObject.find(instance.handle);
      if (owned == ownedAttributesByObject.end() || owned->second.empty()) {
        ++objectInstance;
        continue;
      }

      for (std::uint64_t const attributeHandle : owned->second) {
        // Preserve a search record even when every remaining federate is
        // currently unknown, unpublished, or already acquiring. Later
        // eligibility changes must be able to continue the search.
        instance.ownershipAssumptionRecipientsByAttribute.try_emplace(attributeHandle);
        // A forced resignation has no caller-supplied divestiture tag. Keep
        // that explicit empty value distinct from an earlier unconditional
        // search so later continuation callbacks do not inherit stale bytes.
        instance.ownershipAssumptionUserSuppliedTagsByAttribute.insert_or_assign(
            attributeHandle,
            std::vector<unsigned char>{});
      }

      for (std::uint64_t const attributeHandle : owned->second) {
        instance.attributeOwnersByHandle.erase(attributeHandle);
        clearUpdateRegionAssociation(instance, attributeHandle);
        instance.attributeTransportationTypes.erase(attributeHandle);
        instance.attributeOrderTypes.erase(attributeHandle);
        for (auto pending = instance.pendingAttributeTransportationTypeChanges.begin();
             pending != instance.pendingAttributeTransportationTypeChanges.end();) {
          pending->second.attributeHandles.erase(attributeHandle);
          if (pending->second.attributeHandles.empty()) {
            pending = instance.pendingAttributeTransportationTypeChanges.erase(pending);
          } else {
            ++pending;
          }
        }
        instance.pendingNegotiatedAttributeOwnershipDivestitures.erase(attributeHandle);
      }

      auto followupWorkItems = planPendingAttributeOwnershipAcquisitionWork(
          federation->second,
          instance);
      result.resignOwnershipAcquisitionWorkItems.insert(
          result.resignOwnershipAcquisitionWorkItems.end(),
          std::make_move_iterator(followupWorkItems.begin()),
          std::make_move_iterator(followupWorkItems.end()));
      ++objectInstance;
    }

    for (auto& [recipientKey, attributes] : offeredAttributesByRecipient) {
      // The callback route was captured by the preflight above while the
      // federation lock was held.  A missing entry here is unreachable unless
      // a future mutation violates that lock/lifetime invariant; ignore the
      // impossible work item rather than partially failing after mutation.
      auto const route = offeredCallbackRoutes.find(recipientKey);
      if (route == offeredCallbackRoutes.end()) {
        continue;
      }
      auto const instance = federation->second.objectInstances.find(recipientKey.second);
      if (instance == federation->second.objectInstances.end()) {
        continue;
      }
      for (std::uint64_t const attributeHandle : attributes) {
        instance->second.ownershipAssumptionRecipientsByAttribute[attributeHandle].insert(
            recipientKey.first);
      }
      instance->second.pendingAttributeOwnershipAssumptionCallbacks.push_back({
          recipientKey.first,
          attributes,
          {},
      });
      result.resignOwnershipAssumptions.push_back({
          recipientKey.first,
          recipientKey.second,
          std::move(attributes),
          route->second,
      });
    }
  }

  static_cast<void>(federation->second.timeCoordinator.unregisterFederate(federateId));
  federation->second.pendingTimeAdvanceGrants.erase(federateId);
  federation->second.timeAdvanceGrantDispatchFactories.erase(federateId);
  federation->second.timeRoleEnableDispatchFactories.erase(federateId);
  federation->second.interactionDeclarations.erase(federateId);
  federation->second.objectClassAttributeDeclarations.erase(federateId);
  federation->second.interactionCallbackRoutes.erase(federateId);
  federation->second.serviceReportRoutes.erase(federateId);
  federation->second.publicServiceReportRoutes.erase(federateId);
  for (auto relevance = federation->second.objectClassRegistrationRelevance.begin();
       relevance != federation->second.objectClassRegistrationRelevance.end();) {
    if (relevance->first == federateId) {
      relevance = federation->second.objectClassRegistrationRelevance.erase(relevance);
    } else {
      ++relevance;
    }
  }
  for (auto relevance = federation->second.interactionRelevance.begin();
       relevance != federation->second.interactionRelevance.end();) {
    if (relevance->first == federateId) {
      relevance = federation->second.interactionRelevance.erase(relevance);
    } else {
      ++relevance;
    }
  }
  for (auto reservation = federation->second.reservedObjectInstanceNamesByFederate.begin();
       reservation != federation->second.reservedObjectInstanceNamesByFederate.end();) {
    if (reservation->second == federateId) {
      reservation = federation->second.reservedObjectInstanceNamesByFederate.erase(reservation);
    } else {
      ++reservation;
    }
  }
  for (auto region = federation->second.regions.begin();
       region != federation->second.regions.end();) {
    if (region->second.ownerFederateId == federateId) {
      region = federation->second.regions.erase(region);
    } else {
      ++region;
    }
  }
  for (auto objectInstance = federation->second.objectInstances.begin();
       objectInstance != federation->second.objectInstances.end();) {
    objectInstance->second.knownObjectClassHandlesByFederate.erase(federateId);
    objectInstance->second.pendingDiscoveryFederates.erase(federateId);
    objectInstance->second.pendingRemovalFederates.erase(federateId);
    objectInstance->second.connectionLossAutomaticRemovalFederates.erase(federateId);
    objectInstance->second.deferredConnectionLossTsoRemovalFederates.erase(federateId);
    objectInstance->second.pendingTimestampedRemovalFederates.erase(federateId);
    bool const retainPendingDeletionForConnectionLoss =
        retainPendingTimestampedDeletionForConnectionLoss(objectInstance->second);
    bool const retainPendingDeletionForVoluntaryDeparture =
        retainPendingTimestampedDeletionForVoluntaryDeparture(objectInstance->second);
    if (objectInstance->second.producingFederateId == federateId &&
        objectInstance->second.pendingTimestampedDeletionMessageId.has_value() &&
        !retainPendingDeletionForConnectionLoss &&
        !retainPendingDeletionForVoluntaryDeparture) {
      auto const deletionMessageId =
          *objectInstance->second.pendingTimestampedDeletionMessageId;
      federation->second.tsoObjectDeletionMessages.erase(
          deletionMessageId);
      federation->second.tsoObjectDeletionReconstitutionRecords.erase(deletionMessageId);
      federation->second.tsoRequestRetractionRecords.erase(deletionMessageId);
      objectInstance->second.pendingTimestampedDeletionMessageId.reset();
      objectInstance->second.pendingTimestampedRemovalFederates.clear();
    }
    if (objectInstance->second.pendingTimestampedRemovalFederates.empty()) {
      objectInstance->second.pendingTimestampedDeletionMessageId.reset();
    }
    for (auto pending =
             objectInstance->second.pendingAttributeOwnershipAcquisitionIfAvailableRequests.begin();
         pending != objectInstance->second.pendingAttributeOwnershipAcquisitionIfAvailableRequests.end();) {
      if (pending->second.requestingFederateId == federateId) {
        pending = objectInstance->second.pendingAttributeOwnershipAcquisitionIfAvailableRequests.erase(
            pending);
      } else {
        ++pending;
      }
    }
    for (auto pending = objectInstance->second.pendingAttributeOwnershipAcquisitionRequests.begin();
         pending != objectInstance->second.pendingAttributeOwnershipAcquisitionRequests.end();) {
      if (pending->second.requestingFederateId == federateId) {
        pending = objectInstance->second.pendingAttributeOwnershipAcquisitionRequests.erase(pending);
      } else {
        ++pending;
      }
    }
    for (auto cancellation =
             objectInstance->second.pendingAttributeOwnershipAcquisitionCancellations.begin();
         cancellation !=
         objectInstance->second.pendingAttributeOwnershipAcquisitionCancellations.end();) {
      if (cancellation->second.requestingFederateId == federateId) {
        cancellation =
            objectInstance->second.pendingAttributeOwnershipAcquisitionCancellations.erase(
                cancellation);
      } else {
        ++cancellation;
      }
    }
    for (auto notification =
             objectInstance->second
                 .pendingAttributeOwnershipDivestitureIfWantedNotifications.begin();
         notification !=
         objectInstance->second.pendingAttributeOwnershipDivestitureIfWantedNotifications.end();) {
      if (notification->second.receivingFederateId == federateId) {
        notification =
            objectInstance->second.pendingAttributeOwnershipDivestitureIfWantedNotifications.erase(
                notification);
      } else {
        ++notification;
      }
    }
    for (auto divestiture =
             objectInstance->second.pendingNegotiatedAttributeOwnershipDivestitures.begin();
         divestiture !=
         objectInstance->second.pendingNegotiatedAttributeOwnershipDivestitures.end();) {
      if (divestiture->second.divestingFederateId == federateId ||
          divestiture->second.acquiringFederateId == federateId) {
        divestiture =
            objectInstance->second.pendingNegotiatedAttributeOwnershipDivestitures.erase(
                divestiture);
      } else {
        ++divestiture;
      }
    }
    for (auto notification =
             objectInstance->second.pendingConfirmDivestitureNotifications.begin();
         notification != objectInstance->second.pendingConfirmDivestitureNotifications.end();) {
      if (notification->second.receivingFederateId == federateId) {
        notification = objectInstance->second.pendingConfirmDivestitureNotifications.erase(notification);
      } else {
        ++notification;
      }
    }
    for (auto assumption =
             objectInstance->second.ownershipAssumptionRecipientsByAttribute.begin();
         assumption !=
             objectInstance->second.ownershipAssumptionRecipientsByAttribute.end();) {
      assumption->second.erase(federateId);
      ++assumption;
    }
    // Region templates owned by the resigning federate were removed above.
    // Drop only those stale object-attribute associations; associations to
    // regions owned by another remaining federate stay intact.
    for (auto association = objectInstance->second.updateRegionsByAttribute.begin();
         association != objectInstance->second.updateRegionsByAttribute.end();) {
      for (auto region = association->second.begin();
           region != association->second.end();) {
        if (!federation->second.regions.contains(*region)) {
          region = association->second.erase(region);
        } else {
          ++region;
        }
      }
      if (association->second.empty()) {
        association = objectInstance->second.updateRegionsByAttribute.erase(association);
      } else {
        ++association;
      }
    }
    objectInstance->second.deferredUpdateRegionsByFederate.erase(federateId);
    for (auto deferredByFederate =
             objectInstance->second.deferredUpdateRegionsByFederate.begin();
         deferredByFederate !=
             objectInstance->second.deferredUpdateRegionsByFederate.end();) {
      for (auto association = deferredByFederate->second.begin();
           association != deferredByFederate->second.end();) {
        for (auto region = association->second.begin();
             region != association->second.end();) {
          if (!federation->second.regions.contains(*region)) {
            region = association->second.erase(region);
          } else {
            ++region;
          }
        }
        if (association->second.empty()) {
          association = deferredByFederate->second.erase(association);
        } else {
          ++association;
        }
      }
      if (deferredByFederate->second.empty()) {
        deferredByFederate =
            objectInstance->second.deferredUpdateRegionsByFederate.erase(
                deferredByFederate);
      } else {
        ++deferredByFederate;
      }
    }
    if (canPurgeDeletedObjectInstance(objectInstance->second)) {
      federation->second.objectInstanceHandlesByName.erase(objectInstance->second.name);
      objectInstance = federation->second.objectInstances.erase(objectInstance);
    } else {
      ++objectInstance;
    }
  }
  for (auto point = federation->second.synchronizationPoints.begin();
       point != federation->second.synchronizationPoints.end();) {
    point->second.synchronizationSet.erase(federateId);
    point->second.announcedFederates.erase(federateId);
    point->second.achievedFederates.erase(federateId);

    bool complete = point->second.synchronizationSet.empty();
    if (!complete) {
      complete = true;
      for (std::uint64_t synchronizationFederateId : point->second.synchronizationSet) {
        if (!point->second.achievedFederates.contains(synchronizationFederateId)) {
          complete = false;
          break;
        }
      }
    }
    if (complete) {
      std::set<std::uint64_t> failedToSyncFederates;
      for (auto const& [synchronizationFederateId, federateSucceeded] :
           point->second.achievedFederates) {
        if (!federateSucceeded) {
          failedToSyncFederates.insert(synchronizationFederateId);
        }
      }
      for (std::uint64_t synchronizationFederateId : point->second.synchronizationSet) {
        auto const route = federation->second.interactionCallbackRoutes.find(
            synchronizationFederateId);
        auto const reportRoute = federation->second.serviceReportRoutes.find(
            synchronizationFederateId);
        auto const publicReportRoute = federation->second.publicServiceReportRoutes.find(
            synchronizationFederateId);
        if (route == federation->second.interactionCallbackRoutes.end() || !route->second) {
          continue;
        }
        result.synchronizationNotifications.push_back({
            point->first,
            failedToSyncFederates,
            route->second,
            reportRoute == federation->second.serviceReportRoutes.end()
                ? FederateServiceReportRoute{}
                : reportRoute->second,
            publicReportRoute == federation->second.publicServiceReportRoutes.end()
                ? FederatePublicServiceReportRoute{}
                : publicReportRoute->second,
            synchronizationFederateId,
        });
      }
      point = federation->second.synchronizationPoints.erase(point);
    } else {
      ++point;
    }
  }

  // A federate that has left the execution can never invoke Retract again.
  // Keep any existing record as a lightweight tombstone in case an already
  // queued Request Retraction callback still needs its recipient state, but
  // release its timestamp once no remaining recipient needs typed payload.
  auto timestampForTsoPayload = [&federation](std::uint64_t messageId) {
    if (auto const interaction = federation->second.tsoInteractionMessages.find(messageId);
        interaction != federation->second.tsoInteractionMessages.end()) {
      return interaction->second.timestamp;
    }
    if (auto const update = federation->second.tsoAttributeUpdateMessages.find(messageId);
        update != federation->second.tsoAttributeUpdateMessages.end()) {
      return update->second.timestamp;
    }
    if (auto const deletion = federation->second.tsoObjectDeletionMessages.find(messageId);
        deletion != federation->second.tsoObjectDeletionMessages.end()) {
      return deletion->second.timestamp;
    }
    if (auto const directed = federation->second.tsoDirectedInteractionMessages.find(messageId);
        directed != federation->second.tsoDirectedInteractionMessages.end()) {
      return directed->second.timestamp;
    }
    return std::shared_ptr<rti1516_2025::LogicalTime const>{};
  };
  for (auto& [messageId, record] : federation->second.tsoRequestRetractionRecords) {
    if (record.producingFederateId == federateId) {
      auto const messageTimestamp = record.timestamp
          ? record.timestamp
          : timestampForTsoPayload(messageId);
      if (connectionLossCutoff && messageTimestamp) {
        try {
          record.deliveryRequiredAfterConnectionLoss =
              *messageTimestamp <= *connectionLossCutoff;
        } catch (rti1516_2025::Exception const&) {
          // A malformed private time value must not prevent an authoritative
          // connection-loss cleanup. Without a valid comparison, retain no
          // manufactured mandatory-delivery marker.
          record.deliveryRequiredAfterConnectionLoss = false;
        }
      }
      record.producerResigned = !forcedConnectionLoss;
      record.terminal = true;
      record.timestamp.reset();
    }
  }
  // One RTI-owned joined-federate MOM object has exactly the active joined
  // membership's lifetime. It is deliberately not subjected to the
  // resign-action object/ownership machinery above: that machinery governs
  // federate-created instances. Retain a snapshot until every surviving
  // known recipient crosses its callback-time Remove Object Instance
  // boundary, just as the ordinary object ledger retains a deleted instance.
  for (auto momObject = federation->second.rtiOwnedJoinedFederateMomObjects.begin();
       momObject != federation->second.rtiOwnedJoinedFederateMomObjects.end();) {
    if (momObject->second.federationExecutionObject) {
      momObject->second.pendingDiscoveryFederateIds.erase(federateId);
      momObject->second.pendingRemovalFederateIds.erase(federateId);
      momObject->second.knownFederateIds.erase(federateId);
      ++momObject;
    } else if (momObject->second.joinedFederateId == federateId) {
      momObject->second.pendingDiscoveryFederateIds.clear();
      momObject->second.knownFederateIds.erase(federateId);
      for (std::uint64_t const receivingFederateId : momObject->second.knownFederateIds) {
        if (!federation->second.members.contains(receivingFederateId)) {
          continue;
        }
        auto const route = federation->second.interactionCallbackRoutes.find(
            receivingFederateId);
        if (route == federation->second.interactionCallbackRoutes.end() ||
            !route->second) {
          // Forced connection-loss cleanup has already validated that this
          // recipient may be skipped. A normal resignation cannot reach this
          // branch because the route validation above is atomic.
          continue;
        }
        momObject->second.pendingRemovalFederateIds.insert(receivingFederateId);
        auto const reportRoute = federation->second.serviceReportRoutes.find(
            receivingFederateId);
        result.resignObjectRemovals.push_back({
            receivingFederateId,
            momObject->second.objectInstanceHandle,
            route->second,
            reportRoute == federation->second.serviceReportRoutes.end()
                ? FederateServiceReportRoute{}
                : reportRoute->second,
            true,
        });
      }
      if (momObject->second.pendingRemovalFederateIds.empty()) {
        clearPendingAttributeOwnershipQueries(
            federation->second,
            momObject->second.objectInstanceHandle);
        momObject = federation->second.rtiOwnedJoinedFederateMomObjects.erase(momObject);
      } else {
        ++momObject;
      }
    } else {
      ++momObject;
    }
  }
  if (finalServiceReportGroup.has_value()) {
    // Every normal rejection path has already returned above.  Reserve only
    // the file route: interaction delivery remains separately source-gated,
    // exactly as it does in the ordinary post-service helper.  This has to
    // happen before the membership is erased so the final serial cannot be
    // lost or reused by a later joined-federate lifetime.
    auto const reportPlan = momServiceReportRoutingPlanFor(
        federation->second,
        federateId,
        *finalServiceReportGroup);
    if (reportPlan.disposition == MomServiceReportDisposition::report_to_file) {
      result.finalServiceReportFileSerialNumber =
          member->second.nextMomServiceReportSerialNumber;
      ++member->second.nextMomServiceReportSerialNumber;
    } else if (reservePublicInteraction && !forcedConnectionLoss &&
               reportPlan.disposition == MomServiceReportDisposition::interaction) {
      auto reservation = std::make_shared<ReservedMomServiceReport>();
      reservation->routing = std::move(reportPlan);
      reservation->serialNumber = member->second.nextMomServiceReportSerialNumber;
      ++member->second.nextMomServiceReportSerialNumber;
      reservation->acceptedForEmission = true;
      result.finalServiceReportInteractionReservation = std::move(reservation);
    }
  }
  federation->second.memberIdsByName.erase(member->second.name);
  federation->second.members.erase(member);
  reclaimTsoMessagePayloads(federation->second);
  refreshRegionUsage(federation->second);
  return result;
}

} // namespace umbra::detail
