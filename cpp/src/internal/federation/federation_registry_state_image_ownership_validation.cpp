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

std::size_t pendingOwnershipAssumptionCallbackCount(
    FederationStateImage const &image,
    std::uint64_t objectInstanceHandle) {
  return static_cast<std::size_t>(std::count_if(
      image.pendingAttributeOwnershipAssumptions.begin(),
      image.pendingAttributeOwnershipAssumptions.end(),
      [objectInstanceHandle](
          FederationStateImagePendingAttributeOwnershipAssumption const &callback) {
        return callback.objectInstanceHandle == objectInstanceHandle;
      }));
}

// A negotiated divestiture keeps the current owner in place while its
// ownership-assumption search remains active.  The search ledgers therefore
// legitimately coexist with the negotiated acquisition/divestiture records;
// they are not the standalone unowned-object assumption form below.  Keep the
// route-free validation shared by every negotiated restore predicate so the
// pending-operation fence accounts for the two per-attribute ledgers and any
// queued callback tuples as well.
bool isRouteFreeNegotiatedOwnershipAssumptionLedger(
    FederationStateImageObject const& object,
    FederationStateImage const& image) {
  auto const callbackCount = pendingOwnershipAssumptionCallbackCount(
      image, object.handle);
  if (object.pendingNegotiatedAttributeOwnershipDivestitures.empty()) {
    return object.ownershipAssumptionRecipientsByAttribute.empty() &&
        object.ownershipAssumptionUserSuppliedTagsByAttribute.empty() &&
        callbackCount == 0U;
  }

  std::set<std::uint64_t> knownAttributeHandles;
  for (auto const& attribute : object.attributes) {
    if (attribute.handle == 0U ||
        !knownAttributeHandles.insert(attribute.handle).second) {
      return false;
    }
  }
  auto const negotiatedFor = [&](std::uint64_t attributeHandle) {
    return std::find_if(
        object.pendingNegotiatedAttributeOwnershipDivestitures.begin(),
        object.pendingNegotiatedAttributeOwnershipDivestitures.end(),
        [attributeHandle](
            FederationStateImagePendingNegotiatedAttributeOwnershipDivestiture const& divestiture) {
          return divestiture.attributeHandle == attributeHandle;
        });
  };
  auto const ownerFor = [&](std::uint64_t attributeHandle) {
    return std::find_if(
        object.attributes.begin(),
        object.attributes.end(),
        [attributeHandle](FederationStateImageObjectAttribute const& attribute) {
          return attribute.handle == attributeHandle;
        });
  };

  std::set<std::uint64_t> recipientAttributes;
  std::uint64_t previousRecipientAttribute = 0U;
  for (auto const& assumption : object.ownershipAssumptionRecipientsByAttribute) {
    if (assumption.attributeHandle == 0U ||
        assumption.attributeHandle <= previousRecipientAttribute ||
        !knownAttributeHandles.contains(assumption.attributeHandle) ||
        !std::is_sorted(
            assumption.recipientFederateIds.begin(),
            assumption.recipientFederateIds.end()) ||
        std::adjacent_find(
            assumption.recipientFederateIds.begin(),
            assumption.recipientFederateIds.end()) !=
            assumption.recipientFederateIds.end() ||
        std::any_of(
            assumption.recipientFederateIds.begin(),
            assumption.recipientFederateIds.end(),
            [](std::uint64_t federateId) { return federateId == 0U; })) {
      return false;
    }
    auto const negotiated = negotiatedFor(assumption.attributeHandle);
    auto const owner = ownerFor(assumption.attributeHandle);
    if (negotiated == object.pendingNegotiatedAttributeOwnershipDivestitures.end() ||
        owner == object.attributes.end() ||
        owner->ownerFederateId != negotiated->divestingFederateId ||
        std::ranges::find(
            assumption.recipientFederateIds,
            negotiated->divestingFederateId) !=
            assumption.recipientFederateIds.end()) {
      return false;
    }
    previousRecipientAttribute = assumption.attributeHandle;
    recipientAttributes.insert(assumption.attributeHandle);
  }

  std::set<std::uint64_t> tagAttributes;
  std::uint64_t previousTagAttribute = 0U;
  for (auto const& tag : object.ownershipAssumptionUserSuppliedTagsByAttribute) {
    if (tag.attributeHandle == 0U ||
        tag.attributeHandle <= previousTagAttribute ||
        !knownAttributeHandles.contains(tag.attributeHandle) ||
        negotiatedFor(tag.attributeHandle) ==
            object.pendingNegotiatedAttributeOwnershipDivestitures.end()) {
      return false;
    }
    previousTagAttribute = tag.attributeHandle;
    tagAttributes.insert(tag.attributeHandle);
  }
  if (recipientAttributes != tagAttributes) {
    return false;
  }

  // The implementation seeds one assumption record for every negotiated
  // attribute.  Permit legacy images that predate that extension (both maps
  // empty), but reject a partial map that could detach a restored search.
  if (!recipientAttributes.empty()) {
    std::set<std::uint64_t> negotiatedAttributes;
    for (auto const& divestiture :
         object.pendingNegotiatedAttributeOwnershipDivestitures) {
      if (divestiture.attributeHandle == 0U ||
          !knownAttributeHandles.contains(divestiture.attributeHandle)) {
        return false;
      }
      negotiatedAttributes.insert(divestiture.attributeHandle);
    }
    if (recipientAttributes != negotiatedAttributes) {
      return false;
    }
  } else if (callbackCount != 0U) {
    return false;
  }

  for (auto const& callback : image.pendingAttributeOwnershipAssumptions) {
    if (callback.objectInstanceHandle != object.handle) {
      continue;
    }
    if (callback.receivingFederateId == 0U ||
        callback.attributeHandles.empty() ||
        !std::is_sorted(
            callback.attributeHandles.begin(),
            callback.attributeHandles.end()) ||
        std::adjacent_find(
            callback.attributeHandles.begin(),
            callback.attributeHandles.end()) !=
            callback.attributeHandles.end()) {
      return false;
    }
    for (auto const attributeHandle : callback.attributeHandles) {
      auto const recipients = std::ranges::find_if(
          object.ownershipAssumptionRecipientsByAttribute.begin(),
          object.ownershipAssumptionRecipientsByAttribute.end(),
          [attributeHandle](
              FederationStateImageOwnershipAssumptionRecipients const& assumption) {
            return assumption.attributeHandle == attributeHandle;
          });
      auto const tag = std::ranges::find_if(
          object.ownershipAssumptionUserSuppliedTagsByAttribute.begin(),
          object.ownershipAssumptionUserSuppliedTagsByAttribute.end(),
          [attributeHandle](FederationStateImageOwnershipAssumptionTag const& assumption) {
            return assumption.attributeHandle == attributeHandle;
          });
      if (recipients == object.ownershipAssumptionRecipientsByAttribute.end() ||
          std::ranges::find(
              recipients->recipientFederateIds,
              callback.receivingFederateId) ==
              recipients->recipientFederateIds.end() ||
          tag == object.ownershipAssumptionUserSuppliedTagsByAttribute.end() ||
          callback.userSuppliedTag != tag->userSuppliedTag) {
        return false;
      }
    }
  }
  return true;
}

bool isRouteFreePendingOwnershipAssumptionObject(
    FederationStateImageObject const& object,
    FederationStateImage const& image) {
  if (!object.attributeValuesPresent || object.attributeValues.empty() ||
      object.deleteAccepted ||
      object.ownershipAssumptionRecipientsByAttribute.empty() ||
      object.ownershipAssumptionUserSuppliedTagsByAttribute.empty() ||
      object.pendingOperationCount !=
          object.ownershipAssumptionRecipientsByAttribute.size() +
              object.ownershipAssumptionUserSuppliedTagsByAttribute.size() +
              pendingOwnershipAssumptionCallbackCount(image, object.handle) ||
      !object.pendingAttributeOwnershipAcquisitionIfAvailableRequests.empty() ||
      !object.pendingAttributeOwnershipAcquisitionRequests.empty() ||
      !object.pendingAttributeOwnershipAcquisitionCancellations.empty() ||
      !object.pendingAttributeOwnershipDivestitureIfWantedNotifications.empty() ||
      !object.pendingConfirmDivestitureNotifications.empty() ||
      !object.pendingAttributeTransportationTypeChanges.empty() ||
      !object.pendingNegotiatedAttributeOwnershipDivestitures.empty() ||
      !object.pendingAttributeValueUpdateRequests.empty() ||
      !object.pendingAttributeValueUpdateClassRequests.empty() ||
      !object.pendingAttributeValueUpdateRegionalRequests.empty() ||
      !object.pendingDiscoveryFederateIds.empty() ||
      !object.pendingRemovalFederateIds.empty() ||
      !object.connectionLossAutomaticRemovalFederateIds.empty() ||
      !object.deferredConnectionLossTsoRemovalFederateIds.empty() ||
      !object.pendingTimestampedRemovalFederateIds.empty() ||
      object.pendingTimestampedDeletionMessageId.has_value()) {
    return false;
  }

  std::set<std::uint64_t> knownAttributeHandles;
  for (auto const& attribute : object.attributes) {
    if (attribute.handle == 0U ||
        !knownAttributeHandles.insert(attribute.handle).second) {
      return false;
    }
  }

  std::set<std::uint64_t> recipientAttributes;
  std::uint64_t previousRecipientAttribute = 0U;
  for (auto const& assumption :
       object.ownershipAssumptionRecipientsByAttribute) {
    if (assumption.attributeHandle == 0U ||
        assumption.attributeHandle <= previousRecipientAttribute ||
        !knownAttributeHandles.contains(assumption.attributeHandle) ||
        !std::is_sorted(
            assumption.recipientFederateIds.begin(),
            assumption.recipientFederateIds.end()) ||
        std::adjacent_find(
            assumption.recipientFederateIds.begin(),
            assumption.recipientFederateIds.end()) !=
            assumption.recipientFederateIds.end() ||
        std::any_of(
            assumption.recipientFederateIds.begin(),
            assumption.recipientFederateIds.end(),
            [](std::uint64_t federateId) { return federateId == 0U; })) {
      return false;
    }
    previousRecipientAttribute = assumption.attributeHandle;
    recipientAttributes.insert(assumption.attributeHandle);
  }

  std::set<std::uint64_t> tagAttributes;
  std::uint64_t previousTagAttribute = 0U;
  for (auto const& tag : object.ownershipAssumptionUserSuppliedTagsByAttribute) {
    if (tag.attributeHandle == 0U ||
        tag.attributeHandle <= previousTagAttribute ||
        !knownAttributeHandles.contains(tag.attributeHandle)) {
      return false;
    }
    previousTagAttribute = tag.attributeHandle;
    tagAttributes.insert(tag.attributeHandle);
  }
  if (recipientAttributes != tagAttributes) {
    return false;
  }
  for (auto const attributeHandle : recipientAttributes) {
    auto const attribute = std::find_if(
        object.attributes.begin(),
        object.attributes.end(),
        [attributeHandle](FederationStateImageObjectAttribute const& candidate) {
          return candidate.handle == attributeHandle;
        });
    if (attribute == object.attributes.end() ||
        attribute->ownerFederateId != 0U ||
        !attribute->transportationName.empty() || attribute->orderType != 0U ||
        !attribute->updateRegionHandles.empty()) {
      return false;
    }
  }
  return true;
}

// A process-restart image may safely materialize a federate-created object
// with one or more pending regular acquisition reservations when the image
// contains the latest application-value ledger and no other callback-bearing
// object state. Callback routes and queued markers are execution-local; the
// restore boundary clears those markers and plans fresh work against the
// current joined federates. Keep this predicate deliberately narrow until
// each additional ownership ledger has an explicit restart rebind contract.
bool isRouteFreePendingRegularOwnershipObject(
    FederationStateImageObject const& object) {
  if (!object.attributeValuesPresent || object.attributeValues.empty() ||
      object.deleteAccepted ||
      object.pendingAttributeOwnershipAcquisitionRequests.empty() ||
      object.pendingOperationCount !=
          object.pendingAttributeOwnershipAcquisitionRequests.size() ||
      !object.pendingAttributeOwnershipAcquisitionIfAvailableRequests.empty() ||
      !object.pendingAttributeOwnershipAcquisitionCancellations.empty() ||
      !object.pendingAttributeOwnershipDivestitureIfWantedNotifications.empty() ||
      !object.pendingConfirmDivestitureNotifications.empty() ||
      !object.pendingAttributeTransportationTypeChanges.empty() ||
      !object.pendingNegotiatedAttributeOwnershipDivestitures.empty() ||
      !object.ownershipAssumptionRecipientsByAttribute.empty() ||
      !object.ownershipAssumptionUserSuppliedTagsByAttribute.empty() ||
      !object.pendingDiscoveryFederateIds.empty() ||
      !object.pendingRemovalFederateIds.empty() ||
      !object.connectionLossAutomaticRemovalFederateIds.empty() ||
      !object.deferredConnectionLossTsoRemovalFederateIds.empty() ||
      !object.pendingTimestampedRemovalFederateIds.empty() ||
      object.pendingTimestampedDeletionMessageId.has_value()) {
    return false;
  }
  if (std::any_of(
          object.attributes.begin(),
          object.attributes.end(),
          [](FederationStateImageObjectAttribute const& attribute) {
            return !attribute.updateRegionHandles.empty();
          })) {
    return false;
  }
  return std::all_of(
      object.pendingAttributeOwnershipAcquisitionRequests.begin(),
      object.pendingAttributeOwnershipAcquisitionRequests.end(),
      [](FederationStateImagePendingAttributeOwnershipAcquisition const& request) {
        // An unavailable callback is terminal for that attribute and cannot
        // be recreated without the original callback boundary. Notification
        // and owner-release markers are intentionally replayed from the
        // durable request identity instead.
        return request.requestId != 0U &&
            request.requestingFederateId != 0U &&
            request.requestSequence != 0U &&
            !request.desiredAttributeHandles.empty() &&
            request.unavailableQueuedAttributeHandles.empty();
      });
}

// A process-restart image may also safely materialize a federate-created
// object with one or more pending If Available reservations.  Unlike regular
// acquisition, Willing to Acquire has no owner-side release callback to
// rebuild; the accepted requester callback itself is rebound after restore.
// Keep this predicate disjoint from the regular form so a mixed callback
// ledger cannot be admitted until it has an explicit restart contract.
bool isRouteFreePendingIfAvailableOwnershipObject(
    FederationStateImageObject const& object) {
  if (!object.attributeValuesPresent || object.attributeValues.empty() ||
      object.deleteAccepted ||
      object.pendingAttributeOwnershipAcquisitionIfAvailableRequests.empty() ||
      object.pendingOperationCount !=
          object.pendingAttributeOwnershipAcquisitionIfAvailableRequests.size() ||
      !object.pendingAttributeOwnershipAcquisitionRequests.empty() ||
      !object.pendingAttributeOwnershipAcquisitionCancellations.empty() ||
      !object.pendingAttributeOwnershipDivestitureIfWantedNotifications.empty() ||
      !object.pendingConfirmDivestitureNotifications.empty() ||
      !object.pendingAttributeTransportationTypeChanges.empty() ||
      !object.pendingNegotiatedAttributeOwnershipDivestitures.empty() ||
      !object.ownershipAssumptionRecipientsByAttribute.empty() ||
      !object.ownershipAssumptionUserSuppliedTagsByAttribute.empty() ||
      !object.pendingDiscoveryFederateIds.empty() ||
      !object.pendingRemovalFederateIds.empty() ||
      !object.connectionLossAutomaticRemovalFederateIds.empty() ||
      !object.deferredConnectionLossTsoRemovalFederateIds.empty() ||
      !object.pendingTimestampedRemovalFederateIds.empty() ||
      object.pendingTimestampedDeletionMessageId.has_value()) {
    return false;
  }
  if (std::any_of(
          object.attributes.begin(),
          object.attributes.end(),
          [](FederationStateImageObjectAttribute const& attribute) {
            return !attribute.updateRegionHandles.empty();
          })) {
    return false;
  }
  return std::all_of(
      object.pendingAttributeOwnershipAcquisitionIfAvailableRequests.begin(),
      object.pendingAttributeOwnershipAcquisitionIfAvailableRequests.end(),
      [](FederationStateImagePendingAttributeOwnershipAcquisitionIfAvailable const& request) {
        return request.requestId != 0U &&
            request.requestingFederateId != 0U &&
            request.requestSequence != 0U &&
            !request.desiredAttributeHandles.empty();
      });
}

// A negotiated divestiture keeps the current owner in the waiting state while
// one already-pending regular acquisition is selected.  The durable image
// therefore contains the regular request plus one owner-confirmation record;
// the queued release and confirmation flags are process-local and rebuilt
// against the live owner/requester routes after restore.  Keep this contract
// deliberately limited to the regular-candidate form until the corresponding
// If Available/continuation combinations have their own restart evidence.
bool isRouteFreePendingNegotiatedOwnershipObject(
    FederationStateImageObject const& object,
    FederationStateImage const& image) {
  if (!object.attributeValuesPresent || object.attributeValues.empty() ||
      object.deleteAccepted ||
      object.pendingAttributeOwnershipAcquisitionRequests.empty() ||
      object.pendingAttributeOwnershipAcquisitionIfAvailableRequests.size() != 0U ||
      object.pendingNegotiatedAttributeOwnershipDivestitures.empty() ||
      object.pendingOperationCount !=
          object.pendingAttributeOwnershipAcquisitionRequests.size() +
              object.pendingNegotiatedAttributeOwnershipDivestitures.size() +
              object.ownershipAssumptionRecipientsByAttribute.size() +
              object.ownershipAssumptionUserSuppliedTagsByAttribute.size() +
              pendingOwnershipAssumptionCallbackCount(image, object.handle) ||
      !object.pendingAttributeOwnershipAcquisitionCancellations.empty() ||
      !object.pendingAttributeOwnershipDivestitureIfWantedNotifications.empty() ||
      !object.pendingConfirmDivestitureNotifications.empty() ||
      !object.pendingAttributeTransportationTypeChanges.empty() ||
      !object.pendingDiscoveryFederateIds.empty() ||
      !object.pendingRemovalFederateIds.empty() ||
      !object.connectionLossAutomaticRemovalFederateIds.empty() ||
      !object.deferredConnectionLossTsoRemovalFederateIds.empty() ||
      !object.pendingTimestampedRemovalFederateIds.empty() ||
      object.pendingTimestampedDeletionMessageId.has_value()) {
    return false;
  }
  if (!isRouteFreeNegotiatedOwnershipAssumptionLedger(object, image)) {
    return false;
  }
  if (std::any_of(
          object.attributes.begin(),
          object.attributes.end(),
          [](FederationStateImageObjectAttribute const& attribute) {
            return !attribute.updateRegionHandles.empty();
          })) {
    return false;
  }
  if (!std::all_of(
          object.pendingAttributeOwnershipAcquisitionRequests.begin(),
          object.pendingAttributeOwnershipAcquisitionRequests.end(),
          [](FederationStateImagePendingAttributeOwnershipAcquisition const& request) {
            return request.requestId != 0U &&
                request.requestingFederateId != 0U &&
                request.requestSequence != 0U &&
                !request.desiredAttributeHandles.empty() &&
                request.notificationQueuedAttributeHandles.empty() &&
                request.unavailableQueuedAttributeHandles.empty();
          })) {
    return false;
  }
  return std::all_of(
      object.pendingNegotiatedAttributeOwnershipDivestitures.begin(),
      object.pendingNegotiatedAttributeOwnershipDivestitures.end(),
      [](FederationStateImagePendingNegotiatedAttributeOwnershipDivestiture const& divestiture) {
        return divestiture.attributeHandle != 0U &&
            divestiture.divestingFederateId != 0U &&
            divestiture.acquiringFederateId != 0U &&
            divestiture.acquisitionRequestId != 0U &&
            !divestiture.acquiringFederateIsIfAvailable &&
            divestiture.confirmationQueued &&
            !divestiture.confirmationDelivered;
      });
}

// The corresponding process-restart form for a negotiated divestiture whose
// selected candidate entered Willing to Acquire.  Keep this disjoint from the
// regular-candidate form above: the two request ledgers have different
// callback/retraction semantics and must not be admitted by accident as a
// mixed image.  The pending If Available request remains durable while the
// owner-confirmation callback flags are rebuilt against live routes.
bool isRouteFreePendingNegotiatedIfAvailableOwnershipObject(
    FederationStateImageObject const& object,
    FederationStateImage const& image) {
  if (!object.attributeValuesPresent || object.attributeValues.empty() ||
      object.deleteAccepted ||
      object.pendingAttributeOwnershipAcquisitionIfAvailableRequests.empty() ||
      object.pendingAttributeOwnershipAcquisitionRequests.size() != 0U ||
      object.pendingNegotiatedAttributeOwnershipDivestitures.empty() ||
      object.pendingOperationCount !=
          object.pendingAttributeOwnershipAcquisitionIfAvailableRequests.size() +
              object.pendingNegotiatedAttributeOwnershipDivestitures.size() +
              object.ownershipAssumptionRecipientsByAttribute.size() +
              object.ownershipAssumptionUserSuppliedTagsByAttribute.size() +
              pendingOwnershipAssumptionCallbackCount(image, object.handle) ||
      !object.pendingAttributeOwnershipAcquisitionCancellations.empty() ||
      !object.pendingAttributeOwnershipDivestitureIfWantedNotifications.empty() ||
      !object.pendingConfirmDivestitureNotifications.empty() ||
      !object.pendingAttributeTransportationTypeChanges.empty() ||
      !object.pendingDiscoveryFederateIds.empty() ||
      !object.pendingRemovalFederateIds.empty() ||
      !object.connectionLossAutomaticRemovalFederateIds.empty() ||
      !object.deferredConnectionLossTsoRemovalFederateIds.empty() ||
      !object.pendingTimestampedRemovalFederateIds.empty() ||
      object.pendingTimestampedDeletionMessageId.has_value()) {
    return false;
  }
  if (!isRouteFreeNegotiatedOwnershipAssumptionLedger(object, image)) {
    return false;
  }
  if (std::any_of(
          object.attributes.begin(),
          object.attributes.end(),
          [](FederationStateImageObjectAttribute const& attribute) {
            return !attribute.updateRegionHandles.empty();
          })) {
    return false;
  }
  if (!std::all_of(
          object.pendingAttributeOwnershipAcquisitionIfAvailableRequests.begin(),
          object.pendingAttributeOwnershipAcquisitionIfAvailableRequests.end(),
          [](FederationStateImagePendingAttributeOwnershipAcquisitionIfAvailable const& request) {
            return request.requestId != 0U &&
                request.requestingFederateId != 0U &&
                request.requestSequence != 0U &&
                !request.desiredAttributeHandles.empty();
          })) {
    return false;
  }
  for (auto const& request :
       object.pendingAttributeOwnershipAcquisitionIfAvailableRequests) {
    auto const requestId = request.requestId;
    if (!std::all_of(
            request.desiredAttributeHandles.begin(),
            request.desiredAttributeHandles.end(),
            [&](std::uint64_t attributeHandle) {
              auto const divestiture = std::ranges::find_if(
                  object.pendingNegotiatedAttributeOwnershipDivestitures,
                  [attributeHandle](
                      FederationStateImagePendingNegotiatedAttributeOwnershipDivestiture const& candidate) {
                    return candidate.attributeHandle == attributeHandle;
                  });
              return divestiture !=
                      object.pendingNegotiatedAttributeOwnershipDivestitures.end() &&
                  divestiture->acquiringFederateIsIfAvailable &&
                  divestiture->acquiringFederateId == request.requestingFederateId &&
                  divestiture->acquisitionRequestId == requestId;
            })) {
      return false;
    }
  }
  return std::all_of(
      object.pendingNegotiatedAttributeOwnershipDivestitures.begin(),
      object.pendingNegotiatedAttributeOwnershipDivestitures.end(),
      [](FederationStateImagePendingNegotiatedAttributeOwnershipDivestiture const& divestiture) {
        return divestiture.attributeHandle != 0U &&
            divestiture.divestingFederateId != 0U &&
            divestiture.acquiringFederateId != 0U &&
            divestiture.acquisitionRequestId != 0U &&
            divestiture.acquiringFederateIsIfAvailable &&
            divestiture.confirmationQueued &&
            !divestiture.confirmationDelivered;
      });
}

// A mixed negotiated image contains one regular acquisition request and one
// Willing-to-Acquire request, with each selected by a separate negotiated
// divestiture record.  Admit only the complete, route-free form: every
// requested attribute must be represented by the matching candidate ledger so
// restore cannot suppress an ordinary callback for an unrelated remainder.
bool isRouteFreePendingMixedNegotiatedOwnershipObject(
    FederationStateImageObject const& object,
    FederationStateImage const& image) {
  if (!object.attributeValuesPresent || object.attributeValues.empty() ||
      object.deleteAccepted ||
      object.pendingAttributeOwnershipAcquisitionRequests.empty() ||
      object.pendingAttributeOwnershipAcquisitionIfAvailableRequests.empty() ||
      object.pendingNegotiatedAttributeOwnershipDivestitures.empty() ||
      object.pendingOperationCount !=
          object.pendingAttributeOwnershipAcquisitionRequests.size() +
              object.pendingAttributeOwnershipAcquisitionIfAvailableRequests.size() +
              object.pendingNegotiatedAttributeOwnershipDivestitures.size() +
              object.ownershipAssumptionRecipientsByAttribute.size() +
              object.ownershipAssumptionUserSuppliedTagsByAttribute.size() +
              pendingOwnershipAssumptionCallbackCount(image, object.handle) ||
      !object.pendingAttributeOwnershipAcquisitionCancellations.empty() ||
      !object.pendingAttributeOwnershipDivestitureIfWantedNotifications.empty() ||
      !object.pendingConfirmDivestitureNotifications.empty() ||
      !object.pendingAttributeTransportationTypeChanges.empty() ||
      !object.pendingDiscoveryFederateIds.empty() ||
      !object.pendingRemovalFederateIds.empty() ||
      !object.connectionLossAutomaticRemovalFederateIds.empty() ||
      !object.deferredConnectionLossTsoRemovalFederateIds.empty() ||
      !object.pendingTimestampedRemovalFederateIds.empty() ||
      object.pendingTimestampedDeletionMessageId.has_value()) {
    return false;
  }
  if (!isRouteFreeNegotiatedOwnershipAssumptionLedger(object, image)) {
    return false;
  }
  if (std::any_of(
          object.attributes.begin(),
          object.attributes.end(),
          [](FederationStateImageObjectAttribute const& attribute) {
            return !attribute.updateRegionHandles.empty();
          })) {
    return false;
  }
  if (!std::all_of(
          object.pendingAttributeOwnershipAcquisitionRequests.begin(),
          object.pendingAttributeOwnershipAcquisitionRequests.end(),
          [](FederationStateImagePendingAttributeOwnershipAcquisition const& request) {
            return request.requestId != 0U &&
                request.requestingFederateId != 0U &&
                request.requestSequence != 0U &&
                !request.desiredAttributeHandles.empty() &&
                request.notificationQueuedAttributeHandles.empty() &&
                request.unavailableQueuedAttributeHandles.empty();
          }) ||
      !std::all_of(
          object.pendingAttributeOwnershipAcquisitionIfAvailableRequests.begin(),
          object.pendingAttributeOwnershipAcquisitionIfAvailableRequests.end(),
          [](FederationStateImagePendingAttributeOwnershipAcquisitionIfAvailable const& request) {
            return request.requestId != 0U &&
                request.requestingFederateId != 0U &&
                request.requestSequence != 0U &&
                !request.desiredAttributeHandles.empty();
          })) {
    return false;
  }

  auto matchingDivestiture = [&](std::uint64_t attributeHandle,
                                 std::uint64_t requestId,
                                 std::uint64_t requestingFederateId,
                                 bool candidateIsIfAvailable) {
    return std::ranges::any_of(
        object.pendingNegotiatedAttributeOwnershipDivestitures,
        [&](FederationStateImagePendingNegotiatedAttributeOwnershipDivestiture const& divestiture) {
          return divestiture.attributeHandle == attributeHandle &&
              divestiture.acquisitionRequestId == requestId &&
              divestiture.acquiringFederateId == requestingFederateId &&
              divestiture.acquiringFederateIsIfAvailable == candidateIsIfAvailable;
        });
  };
  for (auto const& request : object.pendingAttributeOwnershipAcquisitionRequests) {
    if (!std::all_of(
            request.desiredAttributeHandles.begin(),
            request.desiredAttributeHandles.end(),
            [&](std::uint64_t attributeHandle) {
              return matchingDivestiture(
                  attributeHandle,
                  request.requestId,
                  request.requestingFederateId,
                  false);
            })) {
      return false;
    }
  }
  for (auto const& request : object.pendingAttributeOwnershipAcquisitionIfAvailableRequests) {
    if (!std::all_of(
            request.desiredAttributeHandles.begin(),
            request.desiredAttributeHandles.end(),
            [&](std::uint64_t attributeHandle) {
              return matchingDivestiture(
                  attributeHandle,
                  request.requestId,
                  request.requestingFederateId,
                  true);
            })) {
      return false;
    }
  }

  return std::all_of(
      object.pendingNegotiatedAttributeOwnershipDivestitures.begin(),
      object.pendingNegotiatedAttributeOwnershipDivestitures.end(),
      [&](FederationStateImagePendingNegotiatedAttributeOwnershipDivestiture const& divestiture) {
        if (divestiture.attributeHandle == 0U ||
            divestiture.divestingFederateId == 0U ||
            divestiture.acquiringFederateId == 0U ||
            divestiture.acquisitionRequestId == 0U ||
            !divestiture.confirmationQueued ||
            divestiture.confirmationDelivered) {
          return false;
        }
        if (divestiture.acquiringFederateIsIfAvailable) {
          return std::ranges::any_of(
              object.pendingAttributeOwnershipAcquisitionIfAvailableRequests,
              [&](FederationStateImagePendingAttributeOwnershipAcquisitionIfAvailable const& request) {
                return request.requestId == divestiture.acquisitionRequestId &&
                    request.requestingFederateId == divestiture.acquiringFederateId &&
                    std::ranges::find(
                        request.desiredAttributeHandles,
                        divestiture.attributeHandle) != request.desiredAttributeHandles.end();
              });
        }
        return std::ranges::any_of(
            object.pendingAttributeOwnershipAcquisitionRequests,
            [&](FederationStateImagePendingAttributeOwnershipAcquisition const& request) {
              return request.requestId == divestiture.acquisitionRequestId &&
                  request.requestingFederateId == divestiture.acquiringFederateId &&
                  std::ranges::find(
                      request.desiredAttributeHandles,
                      divestiture.attributeHandle) != request.desiredAttributeHandles.end();
            });
      });
}

// Once Request Divestiture Confirmation has reached the owner, the old
// process no longer owns a callback that needs rebinding.  A fresh registry
// may therefore materialize this narrower regular-candidate image with the
// delivered marker intact; the next public transition is Confirm Divestiture.
// Keep the candidate set complete and route-free so a partially delivered or
// mixed callback image cannot be mistaken for this lifecycle.
bool isRouteFreePendingNegotiatedConfirmationDeliveredOwnershipObject(
    FederationStateImageObject const& object,
    FederationStateImage const& image) {
  if (!object.attributeValuesPresent || object.attributeValues.empty() ||
      object.deleteAccepted ||
      object.pendingAttributeOwnershipAcquisitionRequests.empty() ||
      !object.pendingAttributeOwnershipAcquisitionIfAvailableRequests.empty() ||
      object.pendingNegotiatedAttributeOwnershipDivestitures.empty() ||
      object.pendingOperationCount !=
          object.pendingAttributeOwnershipAcquisitionRequests.size() +
              object.pendingNegotiatedAttributeOwnershipDivestitures.size() +
              object.ownershipAssumptionRecipientsByAttribute.size() +
              object.ownershipAssumptionUserSuppliedTagsByAttribute.size() +
              pendingOwnershipAssumptionCallbackCount(image, object.handle) ||
      !object.pendingAttributeOwnershipAcquisitionCancellations.empty() ||
      !object.pendingAttributeOwnershipDivestitureIfWantedNotifications.empty() ||
      !object.pendingConfirmDivestitureNotifications.empty() ||
      !object.pendingAttributeTransportationTypeChanges.empty() ||
      !object.pendingDiscoveryFederateIds.empty() ||
      !object.pendingRemovalFederateIds.empty() ||
      !object.connectionLossAutomaticRemovalFederateIds.empty() ||
      !object.deferredConnectionLossTsoRemovalFederateIds.empty() ||
      !object.pendingTimestampedRemovalFederateIds.empty() ||
      object.pendingTimestampedDeletionMessageId.has_value()) {
    return false;
  }
  if (!isRouteFreeNegotiatedOwnershipAssumptionLedger(object, image)) {
    return false;
  }
  if (std::any_of(
          object.attributes.begin(),
          object.attributes.end(),
          [](FederationStateImageObjectAttribute const& attribute) {
            return !attribute.updateRegionHandles.empty();
          })) {
    return false;
  }
  if (!std::all_of(
          object.pendingAttributeOwnershipAcquisitionRequests.begin(),
          object.pendingAttributeOwnershipAcquisitionRequests.end(),
          [](FederationStateImagePendingAttributeOwnershipAcquisition const& request) {
            return request.requestId != 0U &&
                request.requestingFederateId != 0U &&
                request.requestSequence != 0U &&
                !request.desiredAttributeHandles.empty() &&
                request.notificationQueuedAttributeHandles.empty() &&
                request.unavailableQueuedAttributeHandles.empty();
          })) {
    return false;
  }
  for (auto const& request : object.pendingAttributeOwnershipAcquisitionRequests) {
    if (!std::all_of(
            request.desiredAttributeHandles.begin(),
            request.desiredAttributeHandles.end(),
            [&](std::uint64_t attributeHandle) {
              return std::ranges::any_of(
                  object.pendingNegotiatedAttributeOwnershipDivestitures,
                  [&](FederationStateImagePendingNegotiatedAttributeOwnershipDivestiture const& divestiture) {
                    return divestiture.attributeHandle == attributeHandle &&
                        divestiture.acquiringFederateId == request.requestingFederateId &&
                        divestiture.acquisitionRequestId == request.requestId &&
                        !divestiture.acquiringFederateIsIfAvailable;
                  });
            })) {
      return false;
    }
  }
  return std::all_of(
      object.pendingNegotiatedAttributeOwnershipDivestitures.begin(),
      object.pendingNegotiatedAttributeOwnershipDivestitures.end(),
      [&](FederationStateImagePendingNegotiatedAttributeOwnershipDivestiture const& divestiture) {
        if (divestiture.attributeHandle == 0U ||
            divestiture.divestingFederateId == 0U ||
            divestiture.acquiringFederateId == 0U ||
            divestiture.acquisitionRequestId == 0U ||
            divestiture.acquiringFederateIsIfAvailable ||
            !divestiture.confirmationQueued ||
            !divestiture.confirmationDelivered) {
          return false;
        }
        return std::ranges::any_of(
            object.pendingAttributeOwnershipAcquisitionRequests,
            [&](FederationStateImagePendingAttributeOwnershipAcquisition const& request) {
              return request.requestId == divestiture.acquisitionRequestId &&
                  request.requestingFederateId == divestiture.acquiringFederateId &&
                  std::ranges::find(
                      request.desiredAttributeHandles,
                      divestiture.attributeHandle) != request.desiredAttributeHandles.end();
            });
      });
}

// The If Available counterpart keeps the same delivered-before-save contract,
// but binds the durable request to the WTA acquisition ledger.  No ordinary
// If Available callback is replayed after restore; the owner may proceed
// directly to Confirm Divestiture.
bool isRouteFreePendingNegotiatedIfAvailableConfirmationDeliveredOwnershipObject(
    FederationStateImageObject const& object,
    FederationStateImage const& image) {
  if (!object.attributeValuesPresent || object.attributeValues.empty() ||
      object.deleteAccepted ||
      object.pendingAttributeOwnershipAcquisitionIfAvailableRequests.empty() ||
      !object.pendingAttributeOwnershipAcquisitionRequests.empty() ||
      object.pendingNegotiatedAttributeOwnershipDivestitures.empty() ||
      object.pendingOperationCount !=
          object.pendingAttributeOwnershipAcquisitionIfAvailableRequests.size() +
              object.pendingNegotiatedAttributeOwnershipDivestitures.size() +
              object.ownershipAssumptionRecipientsByAttribute.size() +
              object.ownershipAssumptionUserSuppliedTagsByAttribute.size() +
              pendingOwnershipAssumptionCallbackCount(image, object.handle) ||
      !object.pendingAttributeOwnershipAcquisitionCancellations.empty() ||
      !object.pendingAttributeOwnershipDivestitureIfWantedNotifications.empty() ||
      !object.pendingConfirmDivestitureNotifications.empty() ||
      !object.pendingAttributeTransportationTypeChanges.empty() ||
      !object.pendingDiscoveryFederateIds.empty() ||
      !object.pendingRemovalFederateIds.empty() ||
      !object.connectionLossAutomaticRemovalFederateIds.empty() ||
      !object.deferredConnectionLossTsoRemovalFederateIds.empty() ||
      !object.pendingTimestampedRemovalFederateIds.empty() ||
      object.pendingTimestampedDeletionMessageId.has_value()) {
    return false;
  }
  if (!isRouteFreeNegotiatedOwnershipAssumptionLedger(object, image)) {
    return false;
  }
  if (std::any_of(
          object.attributes.begin(),
          object.attributes.end(),
          [](FederationStateImageObjectAttribute const& attribute) {
            return !attribute.updateRegionHandles.empty();
          })) {
    return false;
  }
  if (!std::all_of(
          object.pendingAttributeOwnershipAcquisitionIfAvailableRequests.begin(),
          object.pendingAttributeOwnershipAcquisitionIfAvailableRequests.end(),
          [](FederationStateImagePendingAttributeOwnershipAcquisitionIfAvailable const& request) {
            return request.requestId != 0U &&
                request.requestingFederateId != 0U &&
                request.requestSequence != 0U &&
                !request.desiredAttributeHandles.empty();
          })) {
    return false;
  }
  for (auto const& request : object.pendingAttributeOwnershipAcquisitionIfAvailableRequests) {
    if (!std::all_of(
            request.desiredAttributeHandles.begin(),
            request.desiredAttributeHandles.end(),
            [&](std::uint64_t attributeHandle) {
              return std::ranges::any_of(
                  object.pendingNegotiatedAttributeOwnershipDivestitures,
                  [&](FederationStateImagePendingNegotiatedAttributeOwnershipDivestiture const& divestiture) {
                    return divestiture.attributeHandle == attributeHandle &&
                        divestiture.acquiringFederateId == request.requestingFederateId &&
                        divestiture.acquisitionRequestId == request.requestId &&
                        divestiture.acquiringFederateIsIfAvailable;
                  });
            })) {
      return false;
    }
  }
  return std::all_of(
      object.pendingNegotiatedAttributeOwnershipDivestitures.begin(),
      object.pendingNegotiatedAttributeOwnershipDivestitures.end(),
      [&](FederationStateImagePendingNegotiatedAttributeOwnershipDivestiture const& divestiture) {
        if (divestiture.attributeHandle == 0U ||
            divestiture.divestingFederateId == 0U ||
            divestiture.acquiringFederateId == 0U ||
            divestiture.acquisitionRequestId == 0U ||
            !divestiture.acquiringFederateIsIfAvailable ||
            !divestiture.confirmationQueued ||
            !divestiture.confirmationDelivered) {
          return false;
        }
        return std::ranges::any_of(
            object.pendingAttributeOwnershipAcquisitionIfAvailableRequests,
            [&](FederationStateImagePendingAttributeOwnershipAcquisitionIfAvailable const& request) {
              return request.requestId == divestiture.acquisitionRequestId &&
                  request.requestingFederateId == divestiture.acquiringFederateId &&
                  std::ranges::find(
                      request.desiredAttributeHandles,
                      divestiture.attributeHandle) != request.desiredAttributeHandles.end();
            });
      });
}

// The mixed counterpart preserves a regular and a Willing-to-Acquire
// candidate whose owner-confirmation callbacks were both delivered before the
// save boundary.  Keep the candidate coverage complete and route-free so a
// fresh registry does not replay either callback or accidentally admit an
// unrelated remainder from one of the two acquisition ledgers.
bool isRouteFreePendingMixedNegotiatedConfirmationDeliveredOwnershipObject(
    FederationStateImageObject const& object,
    FederationStateImage const& image) {
  if (!object.attributeValuesPresent || object.attributeValues.empty() ||
      object.deleteAccepted ||
      object.pendingAttributeOwnershipAcquisitionRequests.empty() ||
      object.pendingAttributeOwnershipAcquisitionIfAvailableRequests.empty() ||
      object.pendingNegotiatedAttributeOwnershipDivestitures.empty() ||
      object.pendingOperationCount !=
          object.pendingAttributeOwnershipAcquisitionRequests.size() +
              object.pendingAttributeOwnershipAcquisitionIfAvailableRequests.size() +
              object.pendingNegotiatedAttributeOwnershipDivestitures.size() +
              object.ownershipAssumptionRecipientsByAttribute.size() +
              object.ownershipAssumptionUserSuppliedTagsByAttribute.size() +
              pendingOwnershipAssumptionCallbackCount(image, object.handle) ||
      !object.pendingAttributeOwnershipAcquisitionCancellations.empty() ||
      !object.pendingAttributeOwnershipDivestitureIfWantedNotifications.empty() ||
      !object.pendingConfirmDivestitureNotifications.empty() ||
      !object.pendingAttributeTransportationTypeChanges.empty() ||
      !object.pendingDiscoveryFederateIds.empty() ||
      !object.pendingRemovalFederateIds.empty() ||
      !object.connectionLossAutomaticRemovalFederateIds.empty() ||
      !object.deferredConnectionLossTsoRemovalFederateIds.empty() ||
      !object.pendingTimestampedRemovalFederateIds.empty() ||
      object.pendingTimestampedDeletionMessageId.has_value()) {
    return false;
  }
  if (!isRouteFreeNegotiatedOwnershipAssumptionLedger(object, image)) {
    return false;
  }
  if (std::any_of(
          object.attributes.begin(),
          object.attributes.end(),
          [](FederationStateImageObjectAttribute const& attribute) {
            return !attribute.updateRegionHandles.empty();
          })) {
    return false;
  }
  if (!std::all_of(
          object.pendingAttributeOwnershipAcquisitionRequests.begin(),
          object.pendingAttributeOwnershipAcquisitionRequests.end(),
          [](FederationStateImagePendingAttributeOwnershipAcquisition const& request) {
            return request.requestId != 0U &&
                request.requestingFederateId != 0U &&
                request.requestSequence != 0U &&
                !request.desiredAttributeHandles.empty() &&
                request.notificationQueuedAttributeHandles.empty() &&
                request.unavailableQueuedAttributeHandles.empty();
          }) ||
      !std::all_of(
          object.pendingAttributeOwnershipAcquisitionIfAvailableRequests.begin(),
          object.pendingAttributeOwnershipAcquisitionIfAvailableRequests.end(),
          [](FederationStateImagePendingAttributeOwnershipAcquisitionIfAvailable const& request) {
            return request.requestId != 0U &&
                request.requestingFederateId != 0U &&
                request.requestSequence != 0U &&
                !request.desiredAttributeHandles.empty();
          })) {
    return false;
  }

  auto matchingDivestiture = [&](std::uint64_t attributeHandle,
                                 std::uint64_t requestId,
                                 std::uint64_t requestingFederateId,
                                 bool candidateIsIfAvailable) {
    return std::ranges::any_of(
        object.pendingNegotiatedAttributeOwnershipDivestitures,
        [&](FederationStateImagePendingNegotiatedAttributeOwnershipDivestiture const& divestiture) {
          return divestiture.attributeHandle == attributeHandle &&
              divestiture.acquisitionRequestId == requestId &&
              divestiture.acquiringFederateId == requestingFederateId &&
              divestiture.acquiringFederateIsIfAvailable == candidateIsIfAvailable;
        });
  };
  for (auto const& request : object.pendingAttributeOwnershipAcquisitionRequests) {
    if (!std::all_of(
            request.desiredAttributeHandles.begin(),
            request.desiredAttributeHandles.end(),
            [&](std::uint64_t attributeHandle) {
              return matchingDivestiture(
                  attributeHandle,
                  request.requestId,
                  request.requestingFederateId,
                  false);
            })) {
      return false;
    }
  }
  for (auto const& request : object.pendingAttributeOwnershipAcquisitionIfAvailableRequests) {
    if (!std::all_of(
            request.desiredAttributeHandles.begin(),
            request.desiredAttributeHandles.end(),
            [&](std::uint64_t attributeHandle) {
              return matchingDivestiture(
                  attributeHandle,
                  request.requestId,
                  request.requestingFederateId,
                  true);
            })) {
      return false;
    }
  }

  return std::all_of(
      object.pendingNegotiatedAttributeOwnershipDivestitures.begin(),
      object.pendingNegotiatedAttributeOwnershipDivestitures.end(),
      [&](FederationStateImagePendingNegotiatedAttributeOwnershipDivestiture const& divestiture) {
        if (divestiture.attributeHandle == 0U ||
            divestiture.divestingFederateId == 0U ||
            divestiture.acquiringFederateId == 0U ||
            divestiture.acquisitionRequestId == 0U ||
            !divestiture.confirmationQueued ||
            !divestiture.confirmationDelivered) {
          return false;
        }
        if (divestiture.acquiringFederateIsIfAvailable) {
          return std::ranges::any_of(
              object.pendingAttributeOwnershipAcquisitionIfAvailableRequests,
              [&](FederationStateImagePendingAttributeOwnershipAcquisitionIfAvailable const& request) {
                return request.requestId == divestiture.acquisitionRequestId &&
                    request.requestingFederateId == divestiture.acquiringFederateId &&
                    std::ranges::find(
                        request.desiredAttributeHandles,
                        divestiture.attributeHandle) != request.desiredAttributeHandles.end();
              });
        }
        return std::ranges::any_of(
            object.pendingAttributeOwnershipAcquisitionRequests,
            [&](FederationStateImagePendingAttributeOwnershipAcquisition const& request) {
              return request.requestId == divestiture.acquisitionRequestId &&
                  request.requestingFederateId == divestiture.acquiringFederateId &&
                  std::ranges::find(
                      request.desiredAttributeHandles,
                      divestiture.attributeHandle) != request.desiredAttributeHandles.end();
            });
      });
}

// The asymmetric mixed form preserves one delivered confirmation alongside
// one still-pending confirmation.  Both candidate ledgers remain durable, but
// only the undelivered owner callback may be rebound after restore.  Require
// at least one record on each side of that boundary so a single-candidate
// image cannot enter this mixed admission path accidentally.
bool isRouteFreePendingMixedNegotiatedConfirmationAsymmetricOwnershipObject(
    FederationStateImageObject const& object,
    FederationStateImage const& image) {
  if (!object.attributeValuesPresent || object.attributeValues.empty() ||
      object.deleteAccepted ||
      object.pendingAttributeOwnershipAcquisitionRequests.empty() ||
      object.pendingAttributeOwnershipAcquisitionIfAvailableRequests.empty() ||
      object.pendingNegotiatedAttributeOwnershipDivestitures.empty() ||
      object.pendingOperationCount !=
          object.pendingAttributeOwnershipAcquisitionRequests.size() +
              object.pendingAttributeOwnershipAcquisitionIfAvailableRequests.size() +
              object.pendingNegotiatedAttributeOwnershipDivestitures.size() +
              object.ownershipAssumptionRecipientsByAttribute.size() +
              object.ownershipAssumptionUserSuppliedTagsByAttribute.size() +
              pendingOwnershipAssumptionCallbackCount(image, object.handle) ||
      !object.pendingAttributeOwnershipAcquisitionCancellations.empty() ||
      !object.pendingAttributeOwnershipDivestitureIfWantedNotifications.empty() ||
      !object.pendingConfirmDivestitureNotifications.empty() ||
      !object.pendingAttributeTransportationTypeChanges.empty() ||
      !object.pendingDiscoveryFederateIds.empty() ||
      !object.pendingRemovalFederateIds.empty() ||
      !object.connectionLossAutomaticRemovalFederateIds.empty() ||
      !object.deferredConnectionLossTsoRemovalFederateIds.empty() ||
      !object.pendingTimestampedRemovalFederateIds.empty() ||
      object.pendingTimestampedDeletionMessageId.has_value()) {
    return false;
  }
  if (!isRouteFreeNegotiatedOwnershipAssumptionLedger(object, image)) {
    return false;
  }
  if (std::any_of(
          object.attributes.begin(),
          object.attributes.end(),
          [](FederationStateImageObjectAttribute const& attribute) {
            return !attribute.updateRegionHandles.empty();
          })) {
    return false;
  }
  if (!std::all_of(
          object.pendingAttributeOwnershipAcquisitionRequests.begin(),
          object.pendingAttributeOwnershipAcquisitionRequests.end(),
          [](FederationStateImagePendingAttributeOwnershipAcquisition const& request) {
            return request.requestId != 0U &&
                request.requestingFederateId != 0U &&
                request.requestSequence != 0U &&
                !request.desiredAttributeHandles.empty() &&
                request.notificationQueuedAttributeHandles.empty() &&
                request.unavailableQueuedAttributeHandles.empty();
          }) ||
      !std::all_of(
          object.pendingAttributeOwnershipAcquisitionIfAvailableRequests.begin(),
          object.pendingAttributeOwnershipAcquisitionIfAvailableRequests.end(),
          [](FederationStateImagePendingAttributeOwnershipAcquisitionIfAvailable const& request) {
            return request.requestId != 0U &&
                request.requestingFederateId != 0U &&
                request.requestSequence != 0U &&
                !request.desiredAttributeHandles.empty();
          })) {
    return false;
  }

  auto matchingDivestiture = [&](std::uint64_t attributeHandle,
                                 std::uint64_t requestId,
                                 std::uint64_t requestingFederateId,
                                 bool candidateIsIfAvailable) {
    return std::ranges::any_of(
        object.pendingNegotiatedAttributeOwnershipDivestitures,
        [&](FederationStateImagePendingNegotiatedAttributeOwnershipDivestiture const& divestiture) {
          return divestiture.attributeHandle == attributeHandle &&
              divestiture.acquisitionRequestId == requestId &&
              divestiture.acquiringFederateId == requestingFederateId &&
              divestiture.acquiringFederateIsIfAvailable == candidateIsIfAvailable;
        });
  };
  for (auto const& request : object.pendingAttributeOwnershipAcquisitionRequests) {
    if (!std::all_of(
            request.desiredAttributeHandles.begin(),
            request.desiredAttributeHandles.end(),
            [&](std::uint64_t attributeHandle) {
              return matchingDivestiture(
                  attributeHandle,
                  request.requestId,
                  request.requestingFederateId,
                  false);
            })) {
      return false;
    }
  }
  for (auto const& request : object.pendingAttributeOwnershipAcquisitionIfAvailableRequests) {
    if (!std::all_of(
            request.desiredAttributeHandles.begin(),
            request.desiredAttributeHandles.end(),
            [&](std::uint64_t attributeHandle) {
              return matchingDivestiture(
                  attributeHandle,
                  request.requestId,
                  request.requestingFederateId,
                  true);
            })) {
      return false;
    }
  }

  bool delivered = false;
  bool undelivered = false;
  for (auto const& divestiture : object.pendingNegotiatedAttributeOwnershipDivestitures) {
    if (divestiture.attributeHandle == 0U ||
        divestiture.divestingFederateId == 0U ||
        divestiture.acquiringFederateId == 0U ||
        divestiture.acquisitionRequestId == 0U ||
        !divestiture.confirmationQueued) {
      return false;
    }
    if (divestiture.confirmationDelivered) {
      delivered = true;
    } else {
      undelivered = true;
    }
    if (divestiture.acquiringFederateIsIfAvailable) {
      if (!std::ranges::any_of(
              object.pendingAttributeOwnershipAcquisitionIfAvailableRequests,
              [&](FederationStateImagePendingAttributeOwnershipAcquisitionIfAvailable const& request) {
                return request.requestId == divestiture.acquisitionRequestId &&
                    request.requestingFederateId == divestiture.acquiringFederateId &&
                    std::ranges::find(
                        request.desiredAttributeHandles,
                        divestiture.attributeHandle) != request.desiredAttributeHandles.end();
              })) {
        return false;
      }
    } else if (!std::ranges::any_of(
                   object.pendingAttributeOwnershipAcquisitionRequests,
                   [&](FederationStateImagePendingAttributeOwnershipAcquisition const& request) {
                     return request.requestId == divestiture.acquisitionRequestId &&
                         request.requestingFederateId == divestiture.acquiringFederateId &&
                         std::ranges::find(
                             request.desiredAttributeHandles,
                             divestiture.attributeHandle) != request.desiredAttributeHandles.end();
                   })) {
      return false;
    }
  }
  return delivered && undelivered;
}

// A Divestiture If Wanted call transfers ownership synchronously, then leaves
// one callback reservation for each eligible recipient.  The reservation is
// route-free: its recipient and attribute set are durable, while the callback
// closure is rebound from the fresh registry's live route.  Admit only this
// standalone notification ledger until it is combined with another explicit
// restart contract.
bool isRouteFreePendingDivestitureIfWantedNotificationObject(
    FederationStateImageObject const& object) {
  if (!object.attributeValuesPresent || object.attributeValues.empty() ||
      object.deleteAccepted ||
      object.pendingAttributeOwnershipDivestitureIfWantedNotifications.empty() ||
      object.pendingOperationCount !=
          object.pendingAttributeOwnershipDivestitureIfWantedNotifications.size() ||
      !object.pendingAttributeOwnershipAcquisitionIfAvailableRequests.empty() ||
      !object.pendingAttributeOwnershipAcquisitionRequests.empty() ||
      !object.pendingAttributeOwnershipAcquisitionCancellations.empty() ||
      !object.pendingConfirmDivestitureNotifications.empty() ||
      !object.pendingAttributeTransportationTypeChanges.empty() ||
      !object.pendingNegotiatedAttributeOwnershipDivestitures.empty() ||
      !object.ownershipAssumptionRecipientsByAttribute.empty() ||
      !object.ownershipAssumptionUserSuppliedTagsByAttribute.empty() ||
      !object.pendingDiscoveryFederateIds.empty() ||
      !object.pendingRemovalFederateIds.empty() ||
      !object.connectionLossAutomaticRemovalFederateIds.empty() ||
      !object.deferredConnectionLossTsoRemovalFederateIds.empty() ||
      !object.pendingTimestampedRemovalFederateIds.empty() ||
      object.pendingTimestampedDeletionMessageId.has_value()) {
    return false;
  }
  if (std::any_of(
          object.attributes.begin(),
          object.attributes.end(),
          [](FederationStateImageObjectAttribute const& attribute) {
            return !attribute.updateRegionHandles.empty();
          })) {
    return false;
  }

  std::set<std::uint64_t> notificationIds;
  for (auto const& notification :
       object.pendingAttributeOwnershipDivestitureIfWantedNotifications) {
    if (notification.notificationId == 0U ||
        notification.receivingFederateId == 0U ||
        notification.attributeHandles.empty() ||
        !notificationIds.insert(notification.notificationId).second ||
        std::any_of(
            notification.attributeHandles.begin(),
            notification.attributeHandles.end(),
            [](std::uint64_t attributeHandle) {
              return attributeHandle == 0U;
            })) {
      return false;
    }
  }
  return true;
}

// Confirm Divestiture transfers ownership synchronously, then leaves one
// grouped acquisition-notification reservation for each acquiring federate.
// Keep the reservation route-free across a fresh-registry restore: the
// recipient and exact attribute set are durable, while the callback route is
// rebound from the live membership at notification entry.
bool isRouteFreePendingConfirmDivestitureNotificationObject(
    FederationStateImageObject const& object) {
  if (!object.attributeValuesPresent || object.attributeValues.empty() ||
      object.deleteAccepted ||
      object.pendingConfirmDivestitureNotifications.empty() ||
      object.pendingOperationCount !=
          object.pendingConfirmDivestitureNotifications.size() ||
      !object.pendingAttributeOwnershipAcquisitionIfAvailableRequests.empty() ||
      !object.pendingAttributeOwnershipAcquisitionRequests.empty() ||
      !object.pendingAttributeOwnershipAcquisitionCancellations.empty() ||
      !object.pendingAttributeOwnershipDivestitureIfWantedNotifications.empty() ||
      !object.pendingAttributeTransportationTypeChanges.empty() ||
      !object.pendingNegotiatedAttributeOwnershipDivestitures.empty() ||
      !object.ownershipAssumptionRecipientsByAttribute.empty() ||
      !object.ownershipAssumptionUserSuppliedTagsByAttribute.empty() ||
      !object.pendingDiscoveryFederateIds.empty() ||
      !object.pendingRemovalFederateIds.empty() ||
      !object.connectionLossAutomaticRemovalFederateIds.empty() ||
      !object.deferredConnectionLossTsoRemovalFederateIds.empty() ||
      !object.pendingTimestampedRemovalFederateIds.empty() ||
      object.pendingTimestampedDeletionMessageId.has_value()) {
    return false;
  }
  if (std::any_of(
          object.attributes.begin(),
          object.attributes.end(),
          [](FederationStateImageObjectAttribute const& attribute) {
            return !attribute.updateRegionHandles.empty();
          })) {
    return false;
  }

  std::set<std::uint64_t> notificationIds;
  for (auto const& notification : object.pendingConfirmDivestitureNotifications) {
    if (notification.notificationId == 0U ||
        notification.receivingFederateId == 0U ||
        notification.attributeHandles.empty() ||
        !notificationIds.insert(notification.notificationId).second ||
        std::any_of(
            notification.attributeHandles.begin(),
            notification.attributeHandles.end(),
            [](std::uint64_t attributeHandle) {
              return attributeHandle == 0U;
            })) {
      return false;
    }
  }
  return true;
}

// A post-confirmation resignation may leave two independent route-free
// ledgers on one object: a Confirm Divestiture notification for a surviving
// recipient and an unowned-assumption search for an attribute divested by the
// departing recipient.  These tuples are safe to rebind together when their
// attribute sets are disjoint.  Keep the predicate explicit rather than
// weakening the standalone predicates, so each mixed restart contract remains
// auditable and future ledger families cannot slip through accidentally.
bool isRouteFreePendingConfirmDivestitureWithOwnershipAssumptionObject(
    FederationStateImageObject const& object,
    FederationStateImage const& image) {
  if (!object.attributeValuesPresent || object.attributeValues.empty() ||
      object.deleteAccepted ||
      object.pendingConfirmDivestitureNotifications.empty() ||
      object.ownershipAssumptionRecipientsByAttribute.empty() ||
      object.ownershipAssumptionUserSuppliedTagsByAttribute.empty() ||
      object.pendingOperationCount !=
          object.pendingConfirmDivestitureNotifications.size() +
              object.ownershipAssumptionRecipientsByAttribute.size() +
              object.ownershipAssumptionUserSuppliedTagsByAttribute.size() +
              pendingOwnershipAssumptionCallbackCount(image, object.handle) ||
      !object.pendingAttributeOwnershipAcquisitionIfAvailableRequests.empty() ||
      !object.pendingAttributeOwnershipAcquisitionRequests.empty() ||
      !object.pendingAttributeOwnershipAcquisitionCancellations.empty() ||
      !object.pendingAttributeOwnershipDivestitureIfWantedNotifications.empty() ||
      !object.pendingAttributeTransportationTypeChanges.empty() ||
      !object.pendingNegotiatedAttributeOwnershipDivestitures.empty() ||
      !object.pendingAttributeValueUpdateRequests.empty() ||
      !object.pendingAttributeValueUpdateClassRequests.empty() ||
      !object.pendingAttributeValueUpdateRegionalRequests.empty() ||
      !object.pendingDiscoveryFederateIds.empty() ||
      !object.pendingRemovalFederateIds.empty() ||
      !object.connectionLossAutomaticRemovalFederateIds.empty() ||
      !object.deferredConnectionLossTsoRemovalFederateIds.empty() ||
      !object.pendingTimestampedRemovalFederateIds.empty() ||
      object.pendingTimestampedDeletionMessageId.has_value()) {
    return false;
  }
  if (std::any_of(
          object.attributes.begin(),
          object.attributes.end(),
          [](FederationStateImageObjectAttribute const& attribute) {
            return !attribute.updateRegionHandles.empty();
          })) {
    return false;
  }

  std::set<std::uint64_t> knownAttributeHandles;
  for (auto const& attribute : object.attributes) {
    if (attribute.handle == 0U ||
        !knownAttributeHandles.insert(attribute.handle).second) {
      return false;
    }
  }

  std::set<std::uint64_t> confirmedAttributes;
  std::uint64_t previousNotificationId = 0U;
  for (auto const& notification :
       object.pendingConfirmDivestitureNotifications) {
    if (notification.notificationId == 0U ||
        notification.notificationId <= previousNotificationId ||
        notification.receivingFederateId == 0U ||
        notification.attributeHandles.empty() ||
        !std::is_sorted(
            notification.attributeHandles.begin(),
            notification.attributeHandles.end()) ||
        std::adjacent_find(
            notification.attributeHandles.begin(),
            notification.attributeHandles.end()) !=
            notification.attributeHandles.end() ||
        std::any_of(
            notification.attributeHandles.begin(),
            notification.attributeHandles.end(),
            [&knownAttributeHandles](std::uint64_t attributeHandle) {
              return attributeHandle == 0U ||
                  !knownAttributeHandles.contains(attributeHandle);
            })) {
      return false;
    }
    previousNotificationId = notification.notificationId;
    for (auto const attributeHandle : notification.attributeHandles) {
      auto const attribute = std::find_if(
          object.attributes.begin(),
          object.attributes.end(),
          [attributeHandle](FederationStateImageObjectAttribute const& candidate) {
            return candidate.handle == attributeHandle;
          });
      if (attribute == object.attributes.end() ||
          attribute->ownerFederateId != notification.receivingFederateId ||
          !confirmedAttributes.insert(attributeHandle).second) {
        return false;
      }
    }
  }

  std::set<std::uint64_t> assumptionAttributes;
  std::uint64_t previousAssumptionAttribute = 0U;
  for (auto const& assumption :
       object.ownershipAssumptionRecipientsByAttribute) {
    if (assumption.attributeHandle == 0U ||
        assumption.attributeHandle <= previousAssumptionAttribute ||
        !knownAttributeHandles.contains(assumption.attributeHandle) ||
        !std::is_sorted(
            assumption.recipientFederateIds.begin(),
            assumption.recipientFederateIds.end()) ||
        std::adjacent_find(
            assumption.recipientFederateIds.begin(),
            assumption.recipientFederateIds.end()) !=
            assumption.recipientFederateIds.end() ||
        std::any_of(
            assumption.recipientFederateIds.begin(),
            assumption.recipientFederateIds.end(),
            [](std::uint64_t federateId) { return federateId == 0U; }) ||
        confirmedAttributes.contains(assumption.attributeHandle)) {
      return false;
    }
    previousAssumptionAttribute = assumption.attributeHandle;
    assumptionAttributes.insert(assumption.attributeHandle);
    auto const attribute = std::find_if(
        object.attributes.begin(),
        object.attributes.end(),
        [&assumption](FederationStateImageObjectAttribute const& candidate) {
          return candidate.handle == assumption.attributeHandle;
        });
    if (attribute == object.attributes.end() ||
        attribute->ownerFederateId != 0U ||
        !attribute->transportationName.empty() || attribute->orderType != 0U) {
      return false;
    }
  }

  std::set<std::uint64_t> assumptionTagAttributes;
  std::uint64_t previousTagAttribute = 0U;
  for (auto const& tag : object.ownershipAssumptionUserSuppliedTagsByAttribute) {
    if (tag.attributeHandle == 0U ||
        tag.attributeHandle <= previousTagAttribute ||
        !knownAttributeHandles.contains(tag.attributeHandle) ||
        confirmedAttributes.contains(tag.attributeHandle) ||
        !assumptionTagAttributes.insert(tag.attributeHandle).second) {
      return false;
    }
    previousTagAttribute = tag.attributeHandle;
  }
  return assumptionAttributes == assumptionTagAttributes;
}

// Cancel Attribute Ownership Acquisition keeps the original regular request
// plus a separate cancellation reservation until the confirmation callback
// begins.  This narrow restart contract admits exactly that pair and rebuilds
// the requester callback from the fresh registry's live route.
bool isRouteFreePendingOwnershipAcquisitionCancellationObject(
    FederationStateImageObject const& object) {
  if (!object.attributeValuesPresent || object.attributeValues.empty() ||
      object.deleteAccepted ||
      object.pendingAttributeOwnershipAcquisitionRequests.size() != 1U ||
      object.pendingAttributeOwnershipAcquisitionCancellations.size() != 1U ||
      object.pendingOperationCount != 2U ||
      !object.pendingAttributeOwnershipAcquisitionIfAvailableRequests.empty() ||
      !object.pendingAttributeOwnershipDivestitureIfWantedNotifications.empty() ||
      !object.pendingConfirmDivestitureNotifications.empty() ||
      !object.pendingAttributeTransportationTypeChanges.empty() ||
      !object.pendingNegotiatedAttributeOwnershipDivestitures.empty() ||
      !object.ownershipAssumptionRecipientsByAttribute.empty() ||
      !object.ownershipAssumptionUserSuppliedTagsByAttribute.empty() ||
      !object.pendingDiscoveryFederateIds.empty() ||
      !object.pendingRemovalFederateIds.empty() ||
      !object.connectionLossAutomaticRemovalFederateIds.empty() ||
      !object.deferredConnectionLossTsoRemovalFederateIds.empty() ||
      !object.pendingTimestampedRemovalFederateIds.empty() ||
      object.pendingTimestampedDeletionMessageId.has_value()) {
    return false;
  }
  if (std::any_of(
          object.attributes.begin(),
          object.attributes.end(),
          [](FederationStateImageObjectAttribute const& attribute) {
            return !attribute.updateRegionHandles.empty();
          })) {
    return false;
  }

  auto const& request = object.pendingAttributeOwnershipAcquisitionRequests.front();
  auto const& cancellation = object.pendingAttributeOwnershipAcquisitionCancellations.front();
  if (request.requestId == 0U ||
      request.requestingFederateId == 0U ||
      request.requestSequence == 0U ||
      request.desiredAttributeHandles.empty() ||
      !request.unavailableQueuedAttributeHandles.empty() ||
      cancellation.cancellationId == 0U ||
      cancellation.requestingFederateId != request.requestingFederateId ||
      cancellation.attributeHandles.empty()) {
    return false;
  }
  return std::all_of(
      cancellation.attributeHandles.begin(),
      cancellation.attributeHandles.end(),
      [&](std::uint64_t attributeHandle) {
        return attributeHandle != 0U &&
            std::ranges::find(
                request.desiredAttributeHandles,
                attributeHandle) != request.desiredAttributeHandles.end();
      });
}

// An object-instance Request Attribute Value Update is a durable application
// request whose provider callback can be rebuilt from the request ledger and
// current joined-federate routes. Keep the restart contract disjoint from all
// other object callback families: one request ledger, the latest value
// projection, and no region/lifecycle/ownership callback work.
}  // namespace umbra::detail::federation_registry_state_image_validation
