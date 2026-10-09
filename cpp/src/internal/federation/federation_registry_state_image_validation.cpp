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

bool isRouteFreePendingAttributeValueUpdateObject(
    FederationStateImageObject const& object) {
  if (!object.attributeValuesPresent || object.attributeValues.empty() ||
      !object.pendingAttributeValueUpdateRequestsPresent ||
      object.pendingAttributeValueUpdateRequests.empty() ||
      object.deleteAccepted ||
      object.pendingOperationCount !=
          object.pendingAttributeValueUpdateRequests.size() ||
      !object.pendingAttributeOwnershipAcquisitionIfAvailableRequests.empty() ||
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
  std::uint64_t previousRequestId = 0U;
  return std::all_of(
      object.pendingAttributeValueUpdateRequests.begin(),
      object.pendingAttributeValueUpdateRequests.end(),
      [&previousRequestId](
          FederationStateImagePendingAttributeValueUpdate const& request) {
        if (request.requestId == 0U ||
            request.requestId <= previousRequestId ||
            request.requestingFederateId == 0U ||
            request.providingFederateId == 0U ||
            request.requestedAttributeHandles.empty() ||
            !std::is_sorted(
                request.requestedAttributeHandles.begin(),
                request.requestedAttributeHandles.end()) ||
            std::adjacent_find(
                request.requestedAttributeHandles.begin(),
                request.requestedAttributeHandles.end()) !=
                request.requestedAttributeHandles.end() ||
            request.requestedAttributeHandles.front() == 0U) {
          return false;
        }
        previousRequestId = request.requestId;
        return true;
      });
}

// The class-designator form expands to one durable request per selected
// object/provider group.  Its process-restart admission is intentionally
// separate from the object-instance ledger so a class handle cannot be lost
// at the callback-time hierarchy recheck.
bool isRouteFreePendingAttributeValueUpdateClassObject(
    FederationStateImageObject const& object) {
  if (!object.attributeValuesPresent || object.attributeValues.empty() ||
      !object.pendingAttributeValueUpdateClassRequestsPresent ||
      object.pendingAttributeValueUpdateClassRequests.empty() ||
      !object.pendingAttributeValueUpdateRequests.empty() ||
      object.deleteAccepted ||
      object.pendingOperationCount !=
          object.pendingAttributeValueUpdateClassRequests.size() ||
      !object.pendingAttributeOwnershipAcquisitionIfAvailableRequests.empty() ||
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
  std::uint64_t previousRequestId = 0U;
  return std::all_of(
      object.pendingAttributeValueUpdateClassRequests.begin(),
      object.pendingAttributeValueUpdateClassRequests.end(),
      [&previousRequestId](
          FederationStateImagePendingAttributeValueUpdateClass const& request) {
        if (request.requestId == 0U ||
            request.requestId <= previousRequestId ||
            request.requestingFederateId == 0U ||
            request.providingFederateId == 0U ||
            request.requestedObjectClassHandle == 0U ||
            request.requestedAttributeHandles.empty() ||
            !std::is_sorted(
                request.requestedAttributeHandles.begin(),
                request.requestedAttributeHandles.end()) ||
            std::adjacent_find(
                request.requestedAttributeHandles.begin(),
                request.requestedAttributeHandles.end()) !=
                request.requestedAttributeHandles.end() ||
            request.requestedAttributeHandles.front() == 0U) {
          return false;
        }
        previousRequestId = request.requestId;
        return true;
      });
}

// Regional class requests use the same expanded object/provider identity but
// additionally retain the requester's per-attribute region designators. The
// object's own update-region associations are part of the existing object
// image and therefore remain eligible for this narrowly bounded DDM slice.
bool isRouteFreePendingAttributeValueUpdateRegionalObject(
    FederationStateImageObject const& object) {
  if (!object.attributeValuesPresent || object.attributeValues.empty() ||
      !object.pendingAttributeValueUpdateRegionalRequestsPresent ||
      object.pendingAttributeValueUpdateRegionalRequests.empty() ||
      !object.pendingAttributeValueUpdateRequests.empty() ||
      !object.pendingAttributeValueUpdateClassRequests.empty() ||
      object.deleteAccepted ||
      object.pendingOperationCount !=
          object.pendingAttributeValueUpdateRegionalRequests.size() ||
      !object.pendingAttributeOwnershipAcquisitionIfAvailableRequests.empty() ||
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
  std::uint64_t previousRequestId = 0U;
  return std::all_of(
      object.pendingAttributeValueUpdateRegionalRequests.begin(),
      object.pendingAttributeValueUpdateRegionalRequests.end(),
      [&previousRequestId](
          FederationStateImagePendingAttributeValueUpdateRegional const& request) {
        if (request.requestId == 0U ||
            request.requestId <= previousRequestId ||
            request.requestingFederateId == 0U ||
            request.providingFederateId == 0U ||
            request.requestedObjectClassHandle == 0U ||
            request.requestedAttributeHandles.empty() ||
            !std::is_sorted(
                request.requestedAttributeHandles.begin(),
                request.requestedAttributeHandles.end()) ||
            std::adjacent_find(
                request.requestedAttributeHandles.begin(),
                request.requestedAttributeHandles.end()) !=
                request.requestedAttributeHandles.end() ||
            request.requestedAttributeHandles.front() == 0U) {
          return false;
        }
        previousRequestId = request.requestId;
        std::uint64_t previousAttributeHandle = 0U;
        for (auto const& [attributeHandle, regionHandles] :
             request.requestRegionsByAttribute) {
          if (attributeHandle == 0U || attributeHandle <= previousAttributeHandle ||
              !std::binary_search(
                  request.requestedAttributeHandles.begin(),
                  request.requestedAttributeHandles.end(),
                  attributeHandle) ||
              regionHandles.empty() ||
              !std::is_sorted(regionHandles.begin(), regionHandles.end()) ||
              std::adjacent_find(regionHandles.begin(), regionHandles.end()) !=
                  regionHandles.end() ||
              regionHandles.front() == 0U) {
            return false;
          }
          previousAttributeHandle = attributeHandle;
        }
        return true;
      });
}

// A receive-order regional response is application state once the provider
// callback has delivered it.  The object may still retain its committed
// update-region association; that association is route-free because the
// corresponding Region records are restored independently from the image.
// Keep this predicate disjoint from pending request ledgers so a fresh
// registry can admit only a delivered value, never a callback that still
// needs the source process's route.
bool isRouteFreeDeliveredApplicationValue(
    FederationStateImageObject const& object) {
  return object.attributeValuesPresent &&
      !object.attributeValues.empty() &&
      !object.deleteAccepted &&
      object.pendingOperationCount == 0U &&
      object.pendingAttributeOwnershipAcquisitionIfAvailableRequests.empty() &&
      object.pendingAttributeOwnershipAcquisitionRequests.empty() &&
      object.pendingAttributeOwnershipAcquisitionCancellations.empty() &&
      object.pendingAttributeOwnershipDivestitureIfWantedNotifications.empty() &&
      object.pendingConfirmDivestitureNotifications.empty() &&
      object.pendingAttributeTransportationTypeChanges.empty() &&
      object.pendingAttributeValueUpdateRequests.empty() &&
      object.pendingAttributeValueUpdateClassRequests.empty() &&
      object.pendingAttributeValueUpdateRegionalRequests.empty() &&
      object.pendingNegotiatedAttributeOwnershipDivestitures.empty() &&
      object.ownershipAssumptionRecipientsByAttribute.empty() &&
      object.ownershipAssumptionUserSuppliedTagsByAttribute.empty() &&
      object.pendingDiscoveryFederateIds.empty() &&
      object.pendingRemovalFederateIds.empty() &&
      object.connectionLossAutomaticRemovalFederateIds.empty() &&
      object.deferredConnectionLossTsoRemovalFederateIds.empty() &&
      object.pendingTimestampedRemovalFederateIds.empty() &&
      !object.pendingTimestampedDeletionMessageId.has_value();
}

// Query Attribute Ownership is a federation-scoped callback ledger, but a
// process-restart object can be materialized only when every pending request
// targeting that object is represented in the route-free image. Keep this
// predicate disjoint from the other object restart slices by requiring the
// latest-value projection and no unrelated callback/lifecycle state.
bool isRouteFreePendingAttributeOwnershipQueryObject(
    FederationStateImageObject const& object,
    FederationStateImage const& image) {
  if (!object.attributeValuesPresent || object.attributeValues.empty() ||
      object.deleteAccepted || object.pendingOperationCount == 0U ||
      !object.pendingAttributeValueUpdateRequests.empty() ||
      !object.pendingAttributeValueUpdateClassRequests.empty() ||
      !object.pendingAttributeValueUpdateRegionalRequests.empty() ||
      !object.pendingAttributeOwnershipAcquisitionIfAvailableRequests.empty() ||
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

  std::vector<FederationStateImagePendingAttributeOwnershipQuery const*> queries;
  for (auto const& query : image.pendingAttributeOwnershipQueries) {
    if (query.objectInstanceHandle == object.handle) {
      queries.push_back(&query);
    }
  }
  if (queries.empty() || queries.size() != object.pendingOperationCount) {
    return false;
  }
  std::uint64_t previousRequestId = 0U;
  return std::all_of(
      queries.begin(),
      queries.end(),
      [&previousRequestId](
          FederationStateImagePendingAttributeOwnershipQuery const* query) {
        if (query->requestId == 0U || query->requestId <= previousRequestId ||
            query->requestingFederateId == 0U ||
            query->objectInstanceHandle == 0U || query->reportKind > 2U ||
            (query->reportKind == 0U && query->owningFederateId == 0U) ||
            (query->reportKind != 0U && query->owningFederateId != 0U) ||
            query->requestedAttributeHandles.empty() ||
            !std::is_sorted(
                query->requestedAttributeHandles.begin(),
                query->requestedAttributeHandles.end()) ||
            std::adjacent_find(
                query->requestedAttributeHandles.begin(),
                query->requestedAttributeHandles.end()) !=
                query->requestedAttributeHandles.end() ||
            query->requestedAttributeHandles.front() == 0U) {
          return false;
        }
        previousRequestId = query->requestId;
        return true;
      });
}

// Request Attribute Transportation Type Change retains one requester
// callback reservation until the confirmation callback begins. The durable
// request identity and selected type are serialized; the fresh registry
// rebuilds only the live callback route. Admit this standalone form while
// keeping mixed callback-bearing object images behind their explicit lanes.
bool isRouteFreePendingAttributeTransportationTypeChangeObject(
    FederationStateImageObject const& object) {
  if (!object.attributeValuesPresent || object.attributeValues.empty() ||
      object.deleteAccepted ||
      object.pendingAttributeTransportationTypeChanges.size() != 1U ||
      object.pendingOperationCount != 1U ||
      !object.pendingAttributeOwnershipAcquisitionIfAvailableRequests.empty() ||
      !object.pendingAttributeOwnershipAcquisitionRequests.empty() ||
      !object.pendingAttributeOwnershipAcquisitionCancellations.empty() ||
      !object.pendingAttributeOwnershipDivestitureIfWantedNotifications.empty() ||
      !object.pendingConfirmDivestitureNotifications.empty() ||
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
  auto const& request = object.pendingAttributeTransportationTypeChanges.front();
  if (request.requestId == 0U || request.requestingFederateId == 0U ||
      request.attributeHandles.empty() || request.transportationName.empty()) {
    return false;
  }
  return std::all_of(
      request.attributeHandles.begin(),
      request.attributeHandles.end(),
      [](std::uint64_t attributeHandle) { return attributeHandle != 0U; });
}

// A timestamped Update Attribute Values passel retains the source-region
// association that was active at invocation time.  That association is part
// of the object ledger as well as the typed TSO payload, so a fresh-registry
// restore must admit the object only when every saved update region can be
// proven to belong to that same route-free payload.  Other callback-bearing
// object ledgers remain excluded from this narrow restart boundary.
bool isRouteFreeObjectWithTsoAttributeUpdate(
    FederationStateImageObject const& object,
    FederationStateImage const& image) {
  if (!object.attributeValuesPresent || object.attributeValues.empty() ||
      object.deleteAccepted || object.pendingOperationCount != 0U ||
      !object.pendingAttributeOwnershipAcquisitionIfAvailableRequests.empty() ||
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

  bool hasAssociatedRegion = false;
  for (auto const& savedAttribute : object.attributes) {
    for (auto const regionHandle : savedAttribute.updateRegionHandles) {
      hasAssociatedRegion = true;
      if (regionHandle == 0U) {
        return false;
      }
      bool matchedPayload = false;
      for (auto const& message : image.tsoAttributeUpdateMessages) {
        if (message.objectInstanceHandle != object.handle ||
            message.producingFederateId != object.producingFederateId) {
          continue;
        }
        bool carriesAttribute = std::any_of(
            message.attributes.begin(),
            message.attributes.end(),
            [&](FederationStateImageAttributeValue const& value) {
              return value.attributeHandle == savedAttribute.handle;
            });
        if (!carriesAttribute) {
          continue;
        }
        bool carriesSnapshot = std::any_of(
            message.sentRegionSnapshots.begin(),
            message.sentRegionSnapshots.end(),
            [regionHandle](
                FederationStateImageInteractionRegionSnapshot const& snapshot) {
              return snapshot.regionHandle == regionHandle;
            });
        if (!carriesSnapshot) {
          continue;
        }
        for (auto const& recipient : message.passelsByRecipient) {
          for (auto const& passel : recipient.passels) {
            if (std::find(
                    passel.sentAttributeHandles.begin(),
                    passel.sentAttributeHandles.end(),
                    savedAttribute.handle) != passel.sentAttributeHandles.end() &&
                std::find(
                    passel.sentRegionHandles.begin(),
                    passel.sentRegionHandles.end(),
                    regionHandle) != passel.sentRegionHandles.end()) {
              matchedPayload = true;
              break;
            }
          }
          if (matchedPayload) {
            break;
          }
        }
        if (matchedPayload) {
          break;
        }
      }
      if (!matchedPayload) {
        return false;
      }
    }
  }
  return hasAssociatedRegion;
}


}  // namespace umbra::detail::federation_registry_state_image_validation
