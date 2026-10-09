#include "internal/federation/federation_registry.hpp"
#include "internal/federation/federation_state_image.hpp"
#include "internal/federation/federation_registry_state_image_validation.hpp"
#include "internal/federation/federation_registry_state_image_restore_helpers.hpp"
#include "internal/federation/federation_registry_value_helpers.hpp"

#include <RTI/encoding/BasicDataElements.h>

#include <algorithm>
#include <map>
#include <optional>
#include <set>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace umbra::detail {

using federation_registry_state_image_validation::isRouteFreeDeliveredApplicationValue;
using federation_registry_state_image_validation::isRouteFreeNegotiatedOwnershipAssumptionLedger;
using federation_registry_state_image_validation::isRouteFreeObjectWithTsoAttributeUpdate;
using federation_registry_state_image_validation::isRouteFreePendingAttributeOwnershipQueryObject;
using federation_registry_state_image_validation::isRouteFreePendingAttributeTransportationTypeChangeObject;
using federation_registry_state_image_validation::isRouteFreePendingAttributeValueUpdateClassObject;
using federation_registry_state_image_validation::isRouteFreePendingAttributeValueUpdateObject;
using federation_registry_state_image_validation::isRouteFreePendingAttributeValueUpdateRegionalObject;
using federation_registry_state_image_validation::isRouteFreePendingConfirmDivestitureNotificationObject;
using federation_registry_state_image_validation::isRouteFreePendingConfirmDivestitureWithOwnershipAssumptionObject;
using federation_registry_state_image_validation::isRouteFreePendingDivestitureIfWantedNotificationObject;
using federation_registry_state_image_validation::isRouteFreePendingIfAvailableOwnershipObject;
using federation_registry_state_image_validation::isRouteFreePendingMixedNegotiatedConfirmationAsymmetricOwnershipObject;
using federation_registry_state_image_validation::isRouteFreePendingMixedNegotiatedConfirmationDeliveredOwnershipObject;
using federation_registry_state_image_validation::isRouteFreePendingMixedNegotiatedOwnershipObject;
using federation_registry_state_image_validation::isRouteFreePendingNegotiatedConfirmationDeliveredOwnershipObject;
using federation_registry_state_image_validation::isRouteFreePendingNegotiatedIfAvailableConfirmationDeliveredOwnershipObject;
using federation_registry_state_image_validation::isRouteFreePendingNegotiatedIfAvailableOwnershipObject;
using federation_registry_state_image_validation::isRouteFreePendingNegotiatedOwnershipObject;
using federation_registry_state_image_validation::isRouteFreePendingOwnershipAcquisitionCancellationObject;
using federation_registry_state_image_validation::isRouteFreePendingOwnershipAssumptionObject;
using federation_registry_state_image_validation::isRouteFreePendingRegularOwnershipObject;
using federation_registry_state_image_validation::pendingOwnershipAssumptionCallbackCount;
using federation_registry_state_image_restore_helpers::decodeSavedOrderType;
using federation_registry_state_image_restore_helpers::variableLengthDataFromBytes;

void EmbeddedFederationRegistry::restoreObjectOwnershipLedgersFromStateImage(
    Federation& federation,
    FederationStateImage const& image,
    Federation const& liveFederation,
    bool allowProcessRestartApplicationValues) {
  if (image.objectInstanceCount != image.objects.size()) {
    throw std::logic_error(
        "The saved object ownership ledger count does not match its records.");
  }
  bool const materializeProcessRestartObjects =
      allowProcessRestartApplicationValues &&
      federation.objectInstances.empty() &&
      !image.objects.empty();
  if (!materializeProcessRestartObjects &&
      federation.objectInstances.size() != image.objects.size()) {
    throw std::logic_error(
        "The saved object ownership ledger does not match the process snapshot.");
  }

  std::map<std::uint64_t, Federation::ObjectInstance> restoredObjects;
  std::map<std::uint64_t, Federation::PendingAttributeOwnershipQuery>
      restoredPendingAttributeOwnershipQueries;
  for (auto const& savedObject : image.objects) {
    if (savedObject.handle == 0U ||
        savedObject.registeredObjectClassHandle == 0U ||
        savedObject.producingFederateId == 0U ||
        !liveFederation.members.contains(savedObject.producingFederateId)) {
      throw std::logic_error(
          "The saved object ownership ledger has an invalid object identity.");
    }

    Federation::ObjectInstance restoredObject;
    auto const existing = federation.objectInstances.find(savedObject.handle);
    if (existing != federation.objectInstances.end()) {
      // Callback routes remain live-only in v1. Preserve the live object as a
      // base while replacing the durable identity and typed ledgers below.
      restoredObject = existing->second;
    } else if (!materializeProcessRestartObjects ||
               (!isRouteFreePendingRegularOwnershipObject(savedObject) &&
                !isRouteFreePendingIfAvailableOwnershipObject(savedObject) &&
                !isRouteFreePendingNegotiatedOwnershipObject(savedObject, image) &&
                !isRouteFreePendingNegotiatedIfAvailableOwnershipObject(savedObject, image) &&
                !isRouteFreePendingMixedNegotiatedOwnershipObject(savedObject, image) &&
                !isRouteFreePendingNegotiatedConfirmationDeliveredOwnershipObject(savedObject, image) &&
                !isRouteFreePendingNegotiatedIfAvailableConfirmationDeliveredOwnershipObject(savedObject, image) &&
                !isRouteFreePendingMixedNegotiatedConfirmationDeliveredOwnershipObject(savedObject, image) &&
                !isRouteFreePendingMixedNegotiatedConfirmationAsymmetricOwnershipObject(savedObject, image) &&
                !isRouteFreePendingAttributeValueUpdateObject(savedObject) &&
                !isRouteFreePendingAttributeValueUpdateClassObject(savedObject) &&
               !isRouteFreePendingAttributeValueUpdateRegionalObject(savedObject) &&
               !isRouteFreePendingAttributeOwnershipQueryObject(
                   savedObject,
                   image) &&
                !isRouteFreePendingOwnershipAssumptionObject(savedObject, image) &&
                !isRouteFreePendingDivestitureIfWantedNotificationObject(savedObject) &&
                !isRouteFreePendingConfirmDivestitureNotificationObject(savedObject) &&
                !isRouteFreePendingConfirmDivestitureWithOwnershipAssumptionObject(
                    savedObject,
                    image) &&
                !isRouteFreePendingOwnershipAcquisitionCancellationObject(savedObject) &&
                !isRouteFreePendingAttributeTransportationTypeChangeObject(savedObject) &&
                (!isRouteFreeDeliveredApplicationValue(savedObject) &&
                 !isRouteFreeObjectWithTsoAttributeUpdate(savedObject, image) &&
                 (!savedObject.attributeValuesPresent ||
                  savedObject.attributeValues.empty() ||
                  savedObject.deleteAccepted ||
                  savedObject.pendingOperationCount !=
                  savedObject.pendingAttributeOwnershipAcquisitionIfAvailableRequests.size() +
                  savedObject.pendingAttributeOwnershipAcquisitionRequests.size() +
                  savedObject.pendingAttributeOwnershipAcquisitionCancellations.size() +
                  savedObject.pendingAttributeOwnershipDivestitureIfWantedNotifications.size() +
                  savedObject.pendingConfirmDivestitureNotifications.size() +
                  savedObject.pendingAttributeTransportationTypeChanges.size() +
                  savedObject.pendingAttributeValueUpdateRequests.size() +
                  savedObject.pendingAttributeValueUpdateClassRequests.size() +
                  savedObject.pendingAttributeValueUpdateRegionalRequests.size() +
                  std::count_if(
                      image.pendingAttributeOwnershipQueries.begin(),
                      image.pendingAttributeOwnershipQueries.end(),
                      [&savedObject](auto const& query) {
                        return query.objectInstanceHandle == savedObject.handle;
                      }) +
                  savedObject.pendingNegotiatedAttributeOwnershipDivestitures.size() +
                  savedObject.ownershipAssumptionRecipientsByAttribute.size() +
                  savedObject.ownershipAssumptionUserSuppliedTagsByAttribute.size() +
                  pendingOwnershipAssumptionCallbackCount(image, savedObject.handle) +
                  savedObject.pendingDiscoveryFederateIds.size() +
                  savedObject.pendingRemovalFederateIds.size() +
                  savedObject.connectionLossAutomaticRemovalFederateIds.size() +
                  savedObject.deferredConnectionLossTsoRemovalFederateIds.size() +
                  savedObject.pendingTimestampedRemovalFederateIds.size() +
                  (savedObject.pendingTimestampedDeletionMessageId.has_value() ? 1U : 0U) ||
                  std::any_of(
                      savedObject.attributes.begin(),
                      savedObject.attributes.end(),
                      [](FederationStateImageObjectAttribute const& attribute) {
                        return !attribute.updateRegionHandles.empty();
                      }) ||
                  std::any_of(
                      image.deferredUpdateRegionAssociations.begin(),
                      image.deferredUpdateRegionAssociations.end(),
                      [&savedObject](
                          FederationStateImageDeferredUpdateRegionAssociation const& association) {
                        return association.objectInstanceHandle == savedObject.handle;
                      }))))) {
      // A process-restart image can materialize only a live object identity
      // whose latest value ledger is present and whose callback-bearing
      // ledgers are empty. Keep this boundary explicit instead of
      // manufacturing an incomplete object state.
      throw std::logic_error(
          "The saved object ownership ledger is not a route-free application-value object.");
    }
    restoredObject.handle = savedObject.handle;
    restoredObject.name = savedObject.name;
    restoredObject.registeredObjectClassHandle =
        savedObject.registeredObjectClassHandle;
    restoredObject.producingFederateId = savedObject.producingFederateId;
    restoredObject.deleteAccepted = savedObject.deleteAccepted;
    restoredObject.attributeOwnersByHandle.clear();
    restoredObject.attributeTransportationTypes.clear();
    restoredObject.attributeOrderTypes.clear();
    restoredObject.updateRegionsByAttribute.clear();
    if (image.deferredUpdateRegionAssociationsPresent) {
      restoredObject.deferredUpdateRegionsByFederate.clear();
    }
    if (savedObject.attributeValuesPresent) {
      restoredObject.attributeValues.clear();
    }
    // A legacy v1 object line has no typed reservation section. Preserve the
    // process-local reservation map in that case; newly encoded records are
    // authoritative and replace only this one typed ledger.
    auto const hasTypedIfAvailableLedger =
        !savedObject.pendingAttributeOwnershipAcquisitionIfAvailableRequests.empty() ||
        savedObject.pendingOperationCount == 0U;
    if (hasTypedIfAvailableLedger) {
      restoredObject.pendingAttributeOwnershipAcquisitionIfAvailableRequests.clear();
    }
    auto const hasTypedRegularLedger =
        !savedObject.pendingAttributeOwnershipAcquisitionRequests.empty() ||
        savedObject.pendingOperationCount == 0U;
    if (hasTypedRegularLedger) {
      restoredObject.pendingAttributeOwnershipAcquisitionRequests.clear();
    }
    auto const hasTypedCancellationLedger =
        !savedObject.pendingAttributeOwnershipAcquisitionCancellations.empty() ||
        savedObject.pendingOperationCount == 0U;
    if (hasTypedCancellationLedger) {
      restoredObject.pendingAttributeOwnershipAcquisitionCancellations.clear();
    }
    auto const hasTypedDivestitureIfWantedLedger =
        !savedObject.pendingAttributeOwnershipDivestitureIfWantedNotifications.empty() ||
        savedObject.pendingOperationCount == 0U;
    if (hasTypedDivestitureIfWantedLedger) {
      restoredObject.pendingAttributeOwnershipDivestitureIfWantedNotifications.clear();
    }
    auto const hasTypedConfirmDivestitureLedger =
        !savedObject.pendingConfirmDivestitureNotifications.empty() ||
        savedObject.pendingOperationCount == 0U;
    if (hasTypedConfirmDivestitureLedger) {
      restoredObject.pendingConfirmDivestitureNotifications.clear();
    }
    auto const hasTypedTransportationChangeLedger =
        !savedObject.pendingAttributeTransportationTypeChanges.empty() ||
        savedObject.pendingOperationCount == 0U;
    if (hasTypedTransportationChangeLedger) {
      restoredObject.pendingAttributeTransportationTypeChanges.clear();
    }
    auto const hasTypedNegotiatedDivestitureLedger =
        !savedObject.pendingNegotiatedAttributeOwnershipDivestitures.empty() ||
        savedObject.pendingOperationCount == 0U;
    if (hasTypedNegotiatedDivestitureLedger) {
      restoredObject.pendingNegotiatedAttributeOwnershipDivestitures.clear();
    }
    auto const hasTypedAssumptionRecipientLedger =
        !savedObject.ownershipAssumptionRecipientsByAttribute.empty() ||
        savedObject.pendingOperationCount == 0U;
    if (hasTypedAssumptionRecipientLedger) {
      restoredObject.ownershipAssumptionRecipientsByAttribute.clear();
    }
    auto const hasTypedAssumptionTagLedger =
        !savedObject.ownershipAssumptionUserSuppliedTagsByAttribute.empty() ||
        savedObject.pendingOperationCount == 0U;
    if (hasTypedAssumptionTagLedger) {
      restoredObject.ownershipAssumptionUserSuppliedTagsByAttribute.clear();
    }
    auto const hasTypedAssumptionCallbackLedger =
        image.pendingAttributeOwnershipAssumptionsPresent ||
        pendingOwnershipAssumptionCallbackCount(image, savedObject.handle) != 0U;
    if (hasTypedAssumptionCallbackLedger) {
      restoredObject.pendingAttributeOwnershipAssumptionCallbacks.clear();
    }
    auto const hasTypedPendingDiscoveryLedger =
        !savedObject.pendingDiscoveryFederateIds.empty() ||
        savedObject.pendingOperationCount == 0U;
    if (hasTypedPendingDiscoveryLedger) {
      restoredObject.pendingDiscoveryFederates.clear();
    }
    auto const hasTypedPendingRemovalLedger =
        !savedObject.pendingRemovalFederateIds.empty() ||
        savedObject.pendingOperationCount == 0U;
    if (hasTypedPendingRemovalLedger) {
      restoredObject.pendingRemovalFederates.clear();
    }
    auto const hasTypedConnectionLossAutomaticRemovalLedger =
        !savedObject.connectionLossAutomaticRemovalFederateIds.empty() ||
        savedObject.pendingOperationCount == 0U;
    if (hasTypedConnectionLossAutomaticRemovalLedger) {
      restoredObject.connectionLossAutomaticRemovalFederates.clear();
    }
    auto const hasTypedDeferredConnectionLossTsoRemovalLedger =
        !savedObject.deferredConnectionLossTsoRemovalFederateIds.empty() ||
        savedObject.pendingOperationCount == 0U;
    if (hasTypedDeferredConnectionLossTsoRemovalLedger) {
      restoredObject.deferredConnectionLossTsoRemovalFederates.clear();
    }
    auto const hasTypedPendingTimestampedRemovalLedger =
        !savedObject.pendingTimestampedRemovalFederateIds.empty() ||
        savedObject.pendingOperationCount == 0U;
    if (hasTypedPendingTimestampedRemovalLedger) {
      restoredObject.pendingTimestampedRemovalFederates.clear();
    }
    auto const hasTypedPendingTimestampedDeletionLedger =
        savedObject.pendingTimestampedDeletionMessageId.has_value() ||
        savedObject.pendingOperationCount == 0U;
    if (hasTypedPendingTimestampedDeletionLedger) {
      restoredObject.pendingTimestampedDeletionMessageId.reset();
    }

    // Known-class projections are a durable object-visibility basis rather
    // than a pending-operation fence. When a typed projection is present it
    // replaces the live copy; an older image with no projection leaves the
    // process-local map intact for backward compatibility.
    if (!savedObject.knownObjectClassHandlesByFederate.empty()) {
      restoredObject.knownObjectClassHandlesByFederate.clear();
    }
    std::uint64_t previousKnownFederateId = 0U;
    for (auto const& savedKnown : savedObject.knownObjectClassHandlesByFederate) {
      if (savedKnown.federateId == 0U ||
          savedKnown.federateId <= previousKnownFederateId ||
          savedKnown.objectClassHandle == 0U ||
          !liveFederation.members.contains(savedKnown.federateId)) {
        throw std::logic_error(
            "The saved object visibility projection has an invalid known federate.");
      }
      previousKnownFederateId = savedKnown.federateId;
      auto const [position, inserted] =
          restoredObject.knownObjectClassHandlesByFederate.emplace(
              savedKnown.federateId,
              savedKnown.objectClassHandle);
      static_cast<void>(position);
      if (!inserted) {
        throw std::logic_error(
            "The saved object visibility projection has duplicate known federates.");
      }
    }

    for (auto const& savedAttribute : savedObject.attributes) {
      if (savedAttribute.handle == 0U) {
        throw std::logic_error(
            "The saved object ownership ledger has an invalid attribute.");
      }
      if (savedAttribute.ownerFederateId != 0U) {
        if (!liveFederation.members.contains(savedAttribute.ownerFederateId)) {
          throw std::logic_error(
              "The saved object ownership ledger has a departed owner.");
        }
        restoredObject.attributeOwnersByHandle.emplace(
            savedAttribute.handle,
            savedAttribute.ownerFederateId);
      }
      if (!savedAttribute.transportationName.empty()) {
        restoredObject.attributeTransportationTypes.emplace(
            savedAttribute.handle,
            savedAttribute.transportationName);
      }
      if (savedAttribute.orderType != 0U) {
        auto const order = decodeSavedOrderType(savedAttribute.orderType);
        if (!order) {
          throw std::logic_error(
              "The saved object ownership ledger has an invalid order type.");
        }
        restoredObject.attributeOrderTypes.emplace(savedAttribute.handle, *order);
      }
      if (!savedAttribute.updateRegionHandles.empty()) {
        auto& regions = restoredObject.updateRegionsByAttribute[savedAttribute.handle];
        for (auto const regionHandle : savedAttribute.updateRegionHandles) {
          if (regionHandle == 0U || !federation.regions.contains(regionHandle) ||
              !regions.insert(regionHandle).second) {
            throw std::logic_error(
                "The saved object ownership ledger has an invalid update region.");
          }
        }
      }
    }

    std::set<std::uint64_t> knownAttributeHandles;
    for (auto const& savedAttribute : savedObject.attributes) {
      knownAttributeHandles.insert(savedAttribute.handle);
    }
    if (savedObject.attributeValuesPresent) {
      std::uint64_t previousAttributeHandle = 0U;
      for (auto const& savedValue : savedObject.attributeValues) {
        if (savedValue.attributeHandle == 0U ||
            savedValue.attributeHandle <= previousAttributeHandle ||
            !knownAttributeHandles.contains(savedValue.attributeHandle)) {
          throw std::logic_error(
              "The saved object application-value ledger has an invalid attribute.");
        }
        previousAttributeHandle = savedValue.attributeHandle;
        restoredObject.attributeValues.emplace(
            savedValue.attributeHandle,
            variableLengthDataFromBytes(savedValue.value));
      }
    }
    auto const hasTypedAttributeValueUpdateLedger =
        savedObject.pendingAttributeValueUpdateRequestsPresent ||
        !savedObject.pendingAttributeValueUpdateRequests.empty() ||
        savedObject.pendingOperationCount == 0U;
    if (hasTypedAttributeValueUpdateLedger) {
      restoredObject.pendingAttributeValueUpdateRequests.clear();
    }
    for (auto const& savedRequest :
         savedObject.pendingAttributeValueUpdateRequests) {
      if (savedRequest.requestId == 0U ||
          savedRequest.requestingFederateId == 0U ||
          savedRequest.providingFederateId == 0U ||
          savedRequest.requestingFederateId == savedRequest.providingFederateId ||
          !liveFederation.members.contains(savedRequest.requestingFederateId) ||
          !liveFederation.members.contains(savedRequest.providingFederateId) ||
          savedRequest.requestedAttributeHandles.empty()) {
        throw std::logic_error(
            "The saved attribute value update request has an invalid member identity.");
      }
      Federation::ObjectInstance::PendingAttributeValueUpdateRequest request;
      request.requestingFederateId = savedRequest.requestingFederateId;
      request.providingFederateId = savedRequest.providingFederateId;
      request.requestedAttributeHandles.insert(
          savedRequest.requestedAttributeHandles.begin(),
          savedRequest.requestedAttributeHandles.end());
      if (request.requestedAttributeHandles.size() !=
              savedRequest.requestedAttributeHandles.size() ||
          request.requestedAttributeHandles.empty() ||
          !std::all_of(
              savedRequest.requestedAttributeHandles.begin(),
              savedRequest.requestedAttributeHandles.end(),
              [&knownAttributeHandles](std::uint64_t attributeHandle) {
                return knownAttributeHandles.contains(attributeHandle);
              })) {
        throw std::logic_error(
            "The saved attribute value update request has unknown attributes.");
      }
      request.userSuppliedTag.assign(
          savedRequest.userSuppliedTag.begin(),
          savedRequest.userSuppliedTag.end());
      auto const [position, inserted] =
          restoredObject.pendingAttributeValueUpdateRequests.emplace(
              savedRequest.requestId,
              std::move(request));
      static_cast<void>(position);
      if (!inserted) {
        throw std::logic_error(
            "The saved object ownership ledger has duplicate attribute value update request IDs.");
      }
    }
    auto const hasTypedAttributeValueUpdateClassLedger =
        savedObject.pendingAttributeValueUpdateClassRequestsPresent ||
        !savedObject.pendingAttributeValueUpdateClassRequests.empty() ||
        savedObject.pendingOperationCount == 0U;
    if (hasTypedAttributeValueUpdateClassLedger) {
      restoredObject.pendingAttributeValueUpdateClassRequests.clear();
    }
    for (auto const& savedRequest :
         savedObject.pendingAttributeValueUpdateClassRequests) {
      if (savedRequest.requestId == 0U ||
          savedRequest.requestingFederateId == 0U ||
          savedRequest.providingFederateId == 0U ||
          savedRequest.requestingFederateId == savedRequest.providingFederateId ||
          savedRequest.requestedObjectClassHandle == 0U ||
          !liveFederation.members.contains(savedRequest.requestingFederateId) ||
          !liveFederation.members.contains(savedRequest.providingFederateId) ||
          savedRequest.requestedAttributeHandles.empty() ||
          !federation.objectClassHandles ||
          !federation.attributeHandles ||
          !validObjectClass(federation, savedRequest.requestedObjectClassHandle) ||
          !validObjectClassAttributes(
              federation,
              savedRequest.requestedObjectClassHandle,
              std::set<std::uint64_t>(
                  savedRequest.requestedAttributeHandles.begin(),
                  savedRequest.requestedAttributeHandles.end()))) {
        throw std::logic_error(
            "The saved object-class attribute value update request has an invalid identity.");
      }
      if (!objectInstanceRegisteredAtOrBelowClass(
              federation,
              savedObject.registeredObjectClassHandle,
              savedRequest.requestedObjectClassHandle) ||
          savedObject.deleteAccepted) {
        throw std::logic_error(
            "The saved object-class attribute value update request has an invalid object class.");
      }
      Federation::ObjectInstance::PendingAttributeValueUpdateClassRequest request;
      request.requestingFederateId = savedRequest.requestingFederateId;
      request.providingFederateId = savedRequest.providingFederateId;
      request.requestedObjectClassHandle = savedRequest.requestedObjectClassHandle;
      request.requestedAttributeHandles.insert(
          savedRequest.requestedAttributeHandles.begin(),
          savedRequest.requestedAttributeHandles.end());
      if (request.requestedAttributeHandles.size() !=
              savedRequest.requestedAttributeHandles.size()) {
        throw std::logic_error(
            "The saved object-class attribute value update request has duplicate attributes.");
      }
      for (auto const attributeHandle : request.requestedAttributeHandles) {
        auto const owner = restoredObject.attributeOwnersByHandle.find(attributeHandle);
        if (owner == restoredObject.attributeOwnersByHandle.end() ||
            owner->second != savedRequest.providingFederateId) {
          throw std::logic_error(
              "The saved object-class attribute value update request has a changed owner.");
        }
      }
      request.userSuppliedTag.assign(
          savedRequest.userSuppliedTag.begin(),
          savedRequest.userSuppliedTag.end());
      auto const [position, inserted] =
          restoredObject.pendingAttributeValueUpdateClassRequests.emplace(
              savedRequest.requestId,
              std::move(request));
      static_cast<void>(position);
      if (!inserted) {
        throw std::logic_error(
            "The saved object ownership ledger has duplicate object-class attribute value update request IDs.");
      }
    }
    auto const hasTypedAttributeValueUpdateRegionalLedger =
        savedObject.pendingAttributeValueUpdateRegionalRequestsPresent ||
        !savedObject.pendingAttributeValueUpdateRegionalRequests.empty() ||
        savedObject.pendingOperationCount == 0U;
    if (hasTypedAttributeValueUpdateRegionalLedger) {
      restoredObject.pendingAttributeValueUpdateRegionalRequests.clear();
    }
    for (auto const& savedRequest :
         savedObject.pendingAttributeValueUpdateRegionalRequests) {
      if (savedRequest.requestId == 0U ||
          savedRequest.requestingFederateId == 0U ||
          savedRequest.providingFederateId == 0U ||
          savedRequest.requestingFederateId == savedRequest.providingFederateId ||
          savedRequest.requestedObjectClassHandle == 0U ||
          !liveFederation.members.contains(savedRequest.requestingFederateId) ||
          !liveFederation.members.contains(savedRequest.providingFederateId) ||
          savedRequest.requestedAttributeHandles.empty() ||
          !federation.objectClassHandles ||
          !federation.attributeHandles ||
          !validObjectClass(federation, savedRequest.requestedObjectClassHandle) ||
          !validObjectClassAttributes(
              federation,
              savedRequest.requestedObjectClassHandle,
              std::set<std::uint64_t>(
                  savedRequest.requestedAttributeHandles.begin(),
                  savedRequest.requestedAttributeHandles.end())) ||
          !objectInstanceRegisteredAtOrBelowClass(
              federation,
              savedObject.registeredObjectClassHandle,
              savedRequest.requestedObjectClassHandle) ||
          savedObject.deleteAccepted) {
        throw std::logic_error(
            "The saved regional attribute value update request has an invalid identity.");
      }
      Federation::ObjectInstance::PendingAttributeValueUpdateRegionalRequest request;
      request.requestingFederateId = savedRequest.requestingFederateId;
      request.providingFederateId = savedRequest.providingFederateId;
      request.requestedObjectClassHandle = savedRequest.requestedObjectClassHandle;
      request.requestedAttributeHandles.insert(
          savedRequest.requestedAttributeHandles.begin(),
          savedRequest.requestedAttributeHandles.end());
      if (request.requestedAttributeHandles.size() !=
              savedRequest.requestedAttributeHandles.size()) {
        throw std::logic_error(
            "The saved regional attribute value update request has duplicate attributes.");
      }
      std::uint64_t previousAttributeHandle = 0U;
      for (auto const& [attributeHandle, regionHandles] :
           savedRequest.requestRegionsByAttribute) {
        if (attributeHandle == 0U || attributeHandle <= previousAttributeHandle ||
            !request.requestedAttributeHandles.contains(attributeHandle) ||
            regionHandles.empty()) {
          throw std::logic_error(
              "The saved regional attribute value update request has invalid region associations.");
        }
        previousAttributeHandle = attributeHandle;
        auto& restoredRegions = request.requestRegionsByAttribute[attributeHandle];
        for (auto const regionHandle : regionHandles) {
          auto const region = federation.regions.find(regionHandle);
          if (region == federation.regions.end() ||
              region->second.ownerFederateId != request.requestingFederateId ||
              !region->second.specificationCommitted ||
              !restoredRegions.insert(regionHandle).second) {
            throw std::logic_error(
                "The saved regional attribute value update request references an invalid region.");
          }
        }
      }
      request.userSuppliedTag.assign(
          savedRequest.userSuppliedTag.begin(),
          savedRequest.userSuppliedTag.end());
      for (auto const attributeHandle : request.requestedAttributeHandles) {
        auto const owner = restoredObject.attributeOwnersByHandle.find(attributeHandle);
        if (owner == restoredObject.attributeOwnersByHandle.end() ||
            owner->second != savedRequest.providingFederateId) {
          throw std::logic_error(
              "The saved regional attribute value update request has a changed owner.");
        }
      }
      auto const [position, inserted] =
          restoredObject.pendingAttributeValueUpdateRegionalRequests.emplace(
              savedRequest.requestId,
              std::move(request));
      static_cast<void>(position);
      if (!inserted) {
        throw std::logic_error(
            "The saved object ownership ledger has duplicate regional attribute value update request IDs.");
      }
    }
  for (auto const& savedRequest :
         savedObject.pendingAttributeOwnershipAcquisitionIfAvailableRequests) {
      if (savedRequest.requestId == 0U ||
          savedRequest.requestingFederateId == 0U ||
          savedRequest.requestSequence == 0U ||
          !liveFederation.members.contains(savedRequest.requestingFederateId)) {
        throw std::logic_error(
            "The saved If Available ownership reservation has an invalid requester.");
      }
      Federation::ObjectInstance::PendingAttributeOwnershipAcquisitionIfAvailable request;
      request.requestingFederateId = savedRequest.requestingFederateId;
      request.requestSequence = savedRequest.requestSequence;
      request.desiredAttributeHandles.insert(
          savedRequest.desiredAttributeHandles.begin(),
          savedRequest.desiredAttributeHandles.end());
      if (request.desiredAttributeHandles.size() !=
              savedRequest.desiredAttributeHandles.size() ||
          request.desiredAttributeHandles.empty() ||
          !std::all_of(
              savedRequest.desiredAttributeHandles.begin(),
              savedRequest.desiredAttributeHandles.end(),
              [&knownAttributeHandles](std::uint64_t attributeHandle) {
                return knownAttributeHandles.contains(attributeHandle);
              })) {
        throw std::logic_error(
            "The saved If Available ownership reservation has unknown attributes.");
      }
      request.userSuppliedTag.assign(
          savedRequest.userSuppliedTag.begin(),
          savedRequest.userSuppliedTag.end());
      auto const [position, inserted] =
          restoredObject.pendingAttributeOwnershipAcquisitionIfAvailableRequests.emplace(
              savedRequest.requestId,
              std::move(request));
      static_cast<void>(position);
      if (!inserted) {
        throw std::logic_error(
            "The saved object ownership ledger has duplicate request IDs.");
      }
    }

    for (auto const& savedRequest :
         savedObject.pendingAttributeOwnershipAcquisitionRequests) {
      if (savedRequest.requestId == 0U ||
          savedRequest.requestingFederateId == 0U ||
          savedRequest.requestSequence == 0U ||
          !liveFederation.members.contains(savedRequest.requestingFederateId)) {
        throw std::logic_error(
            "The saved regular ownership reservation has an invalid requester.");
      }
      Federation::ObjectInstance::PendingAttributeOwnershipAcquisition request;
      request.requestingFederateId = savedRequest.requestingFederateId;
      request.requestSequence = savedRequest.requestSequence;
      auto restoreAttributeSet = [&knownAttributeHandles](
                                    std::vector<std::uint64_t> const& savedHandles,
                                    std::set<std::uint64_t>& destination,
                                    bool requireNonEmpty,
                                    char const* error) {
        destination.insert(savedHandles.begin(), savedHandles.end());
        if (destination.size() != savedHandles.size() ||
            (requireNonEmpty && destination.empty()) ||
            !std::all_of(
                savedHandles.begin(),
                savedHandles.end(),
                [&knownAttributeHandles](std::uint64_t attributeHandle) {
                  return knownAttributeHandles.contains(attributeHandle);
                })) {
          throw std::logic_error(error);
        }
      };
      restoreAttributeSet(
          savedRequest.desiredAttributeHandles,
          request.desiredAttributeHandles,
          true,
          "The saved regular ownership reservation has unknown desired attributes.");
      restoreAttributeSet(
          savedRequest.notificationQueuedAttributeHandles,
          request.notificationQueuedAttributeHandles,
          false,
          "The saved regular ownership reservation has unknown notification attributes.");
      restoreAttributeSet(
          savedRequest.unavailableQueuedAttributeHandles,
          request.unavailableQueuedAttributeHandles,
          false,
          "The saved regular ownership reservation has unknown unavailable attributes.");
      for (auto const& [ownerFederateId, savedAttributes] :
           savedRequest.releaseCallbacksQueuedByOwningFederate) {
        if (ownerFederateId == 0U ||
            !liveFederation.members.contains(ownerFederateId)) {
          throw std::logic_error(
              "The saved regular ownership reservation has an invalid release owner.");
        }
        std::set<std::uint64_t> attributes;
        restoreAttributeSet(
            savedAttributes,
            attributes,
            true,
            "The saved regular ownership reservation has unknown release attributes.");
        for (auto const attributeHandle : attributes) {
          if (!request.desiredAttributeHandles.contains(attributeHandle)) {
            throw std::logic_error(
                "The saved regular ownership release attributes are outside the request.");
          }
        }
        auto const [releasePosition, releaseInserted] =
            request.releaseCallbacksQueuedByOwningFederate.emplace(
                ownerFederateId,
                std::move(attributes));
        static_cast<void>(releasePosition);
        if (!releaseInserted) {
          throw std::logic_error(
              "The saved regular ownership reservation has duplicate release owners.");
        }
      }
      for (auto const attributeHandle : request.notificationQueuedAttributeHandles) {
        if (!request.desiredAttributeHandles.contains(attributeHandle)) {
          throw std::logic_error(
              "The saved regular ownership notification attributes are outside the request.");
        }
      }
      for (auto const attributeHandle : request.unavailableQueuedAttributeHandles) {
        if (!request.desiredAttributeHandles.contains(attributeHandle)) {
          throw std::logic_error(
              "The saved regular ownership unavailable attributes are outside the request.");
        }
      }
      if (materializeProcessRestartObjects) {
        // The saved sets describe callback work in the old process. Keep the
        // request identity and desired attributes, but let the post-restore
        // planner reserve fresh notification/release callbacks against the
        // current live routes. The admission predicate excludes an
        // unavailable terminal callback from this replay path.
        request.notificationQueuedAttributeHandles.clear();
        request.unavailableQueuedAttributeHandles.clear();
        request.releaseCallbacksQueuedByOwningFederate.clear();
      }
      request.userSuppliedTag.assign(
          savedRequest.userSuppliedTag.begin(),
          savedRequest.userSuppliedTag.end());
      auto const [position, inserted] =
          restoredObject.pendingAttributeOwnershipAcquisitionRequests.emplace(
              savedRequest.requestId,
              std::move(request));
      static_cast<void>(position);
      if (!inserted) {
        throw std::logic_error(
            "The saved object ownership ledger has duplicate regular request IDs.");
      }
    }

    for (auto const& savedCancellation :
         savedObject.pendingAttributeOwnershipAcquisitionCancellations) {
      if (savedCancellation.cancellationId == 0U ||
          savedCancellation.requestingFederateId == 0U ||
          !liveFederation.members.contains(savedCancellation.requestingFederateId)) {
        throw std::logic_error(
            "The saved ownership acquisition cancellation has an invalid requester.");
      }
      Federation::ObjectInstance::PendingAttributeOwnershipAcquisitionCancellation cancellation;
      cancellation.requestingFederateId = savedCancellation.requestingFederateId;
      cancellation.attributeHandles.insert(
          savedCancellation.attributeHandles.begin(),
          savedCancellation.attributeHandles.end());
      if (cancellation.attributeHandles.size() !=
              savedCancellation.attributeHandles.size() ||
          cancellation.attributeHandles.empty() ||
          !std::all_of(
              savedCancellation.attributeHandles.begin(),
              savedCancellation.attributeHandles.end(),
              [&knownAttributeHandles](std::uint64_t attributeHandle) {
                return knownAttributeHandles.contains(attributeHandle);
              })) {
        throw std::logic_error(
            "The saved ownership acquisition cancellation has unknown attributes.");
      }
      auto const [position, inserted] =
          restoredObject.pendingAttributeOwnershipAcquisitionCancellations.emplace(
              savedCancellation.cancellationId,
              std::move(cancellation));
      static_cast<void>(position);
      if (!inserted) {
        throw std::logic_error(
            "The saved object ownership ledger has duplicate cancellation IDs.");
      }
    }

    for (auto const& savedNotification :
         savedObject.pendingAttributeOwnershipDivestitureIfWantedNotifications) {
      if (savedNotification.notificationId == 0U ||
          savedNotification.receivingFederateId == 0U ||
          !liveFederation.members.contains(savedNotification.receivingFederateId)) {
        throw std::logic_error(
            "The saved Divestiture If Wanted notification has an invalid recipient.");
      }
      Federation::ObjectInstance::PendingAttributeOwnershipDivestitureIfWantedNotification notification;
      notification.receivingFederateId = savedNotification.receivingFederateId;
      notification.attributeHandles.insert(
          savedNotification.attributeHandles.begin(),
          savedNotification.attributeHandles.end());
      notification.userSuppliedTag.assign(
          savedNotification.userSuppliedTag.begin(),
          savedNotification.userSuppliedTag.end());
      if (notification.attributeHandles.size() !=
              savedNotification.attributeHandles.size() ||
          notification.attributeHandles.empty() ||
          !std::all_of(
              savedNotification.attributeHandles.begin(),
              savedNotification.attributeHandles.end(),
              [&knownAttributeHandles](std::uint64_t attributeHandle) {
                return knownAttributeHandles.contains(attributeHandle);
              })) {
        throw std::logic_error(
            "The saved Divestiture If Wanted notification has unknown attributes.");
      }
      auto const [position, inserted] =
          restoredObject.pendingAttributeOwnershipDivestitureIfWantedNotifications.emplace(
              savedNotification.notificationId,
              std::move(notification));
      static_cast<void>(position);
      if (!inserted) {
        throw std::logic_error(
            "The saved object ownership ledger has duplicate notification IDs.");
      }
    }

    for (auto const& savedNotification : savedObject.pendingConfirmDivestitureNotifications) {
      if (savedNotification.notificationId == 0U ||
          savedNotification.receivingFederateId == 0U ||
          !liveFederation.members.contains(savedNotification.receivingFederateId)) {
        throw std::logic_error(
            "The saved Confirm Divestiture notification has an invalid recipient.");
      }
      Federation::ObjectInstance::PendingConfirmDivestitureNotification notification;
      notification.receivingFederateId = savedNotification.receivingFederateId;
      notification.attributeHandles.insert(
          savedNotification.attributeHandles.begin(),
          savedNotification.attributeHandles.end());
      notification.userSuppliedTag.assign(
          savedNotification.userSuppliedTag.begin(),
          savedNotification.userSuppliedTag.end());
      if (notification.attributeHandles.size() !=
              savedNotification.attributeHandles.size() ||
          notification.attributeHandles.empty() ||
          !std::all_of(
              savedNotification.attributeHandles.begin(),
              savedNotification.attributeHandles.end(),
              [&knownAttributeHandles](std::uint64_t attributeHandle) {
                return knownAttributeHandles.contains(attributeHandle);
              })) {
        throw std::logic_error(
            "The saved Confirm Divestiture notification has unknown attributes.");
      }
      auto const [position, inserted] =
          restoredObject.pendingConfirmDivestitureNotifications.emplace(
              savedNotification.notificationId,
              std::move(notification));
      static_cast<void>(position);
      if (!inserted) {
        throw std::logic_error(
            "The saved object ownership ledger has duplicate Confirm Divestiture notification IDs.");
      }
    }

    for (auto const& savedRequest :
         savedObject.pendingAttributeTransportationTypeChanges) {
      if (savedRequest.requestId == 0U ||
          savedRequest.requestingFederateId == 0U ||
          !liveFederation.members.contains(savedRequest.requestingFederateId) ||
          savedRequest.attributeHandles.empty() ||
          !isSupportedTransportationName(
              federation.definition.catalog.get(),
              savedRequest.transportationName)) {
        throw std::logic_error(
            "The saved attribute transportation-type change has an invalid requester or type.");
      }
      Federation::ObjectInstance::PendingAttributeTransportationTypeChange request;
      request.requestingFederateId = savedRequest.requestingFederateId;
      request.transportationName = savedRequest.transportationName;
      request.attributeHandles.insert(
          savedRequest.attributeHandles.begin(),
          savedRequest.attributeHandles.end());
      if (request.attributeHandles.size() !=
              savedRequest.attributeHandles.size() ||
          request.attributeHandles.empty() ||
          !std::all_of(
              savedRequest.attributeHandles.begin(),
              savedRequest.attributeHandles.end(),
              [&knownAttributeHandles](std::uint64_t attributeHandle) {
                return knownAttributeHandles.contains(attributeHandle);
              })) {
        throw std::logic_error(
            "The saved attribute transportation-type change has unknown attributes.");
      }
      for (auto const attributeHandle : request.attributeHandles) {
        auto const owner = restoredObject.attributeOwnersByHandle.find(attributeHandle);
        if (owner == restoredObject.attributeOwnersByHandle.end() ||
            owner->second != savedRequest.requestingFederateId) {
          throw std::logic_error(
              "The saved attribute transportation-type change has an attribute not owned by its requester.");
        }
      }
      auto const [position, inserted] =
          restoredObject.pendingAttributeTransportationTypeChanges.emplace(
              savedRequest.requestId,
              std::move(request));
      static_cast<void>(position);
      if (!inserted) {
        throw std::logic_error(
            "The saved object ownership ledger has duplicate transportation-type change request IDs.");
      }
    }

    for (auto const& savedDivestiture :
         savedObject.pendingNegotiatedAttributeOwnershipDivestitures) {
      if (savedDivestiture.attributeHandle == 0U ||
          !knownAttributeHandles.contains(savedDivestiture.attributeHandle) ||
          savedDivestiture.divestingFederateId == 0U ||
          !liveFederation.members.contains(savedDivestiture.divestingFederateId)) {
        throw std::logic_error(
            "The saved negotiated ownership divestiture has an invalid attribute or divesting federate.");
      }
      auto const owner = restoredObject.attributeOwnersByHandle.find(
          savedDivestiture.attributeHandle);
      if (owner == restoredObject.attributeOwnersByHandle.end() ||
          owner->second != savedDivestiture.divestingFederateId) {
        throw std::logic_error(
            "The saved negotiated ownership divestiture does not match the attribute owner.");
      }
      if ((savedDivestiture.acquiringFederateId == 0U &&
           (savedDivestiture.acquisitionRequestId != 0U ||
            savedDivestiture.acquiringFederateIsIfAvailable ||
            savedDivestiture.confirmationQueued ||
            savedDivestiture.confirmationDelivered)) ||
          (savedDivestiture.acquiringFederateId != 0U &&
           (savedDivestiture.acquisitionRequestId == 0U ||
            !liveFederation.members.contains(savedDivestiture.acquiringFederateId))) ||
          (savedDivestiture.confirmationDelivered &&
           !savedDivestiture.confirmationQueued)) {
        throw std::logic_error(
            "The saved negotiated ownership divestiture has inconsistent candidate state.");
      }

      if (savedDivestiture.acquiringFederateId != 0U) {
        bool candidateStillPending = false;
        if (savedDivestiture.acquiringFederateIsIfAvailable) {
          auto const candidate =
              restoredObject.pendingAttributeOwnershipAcquisitionIfAvailableRequests.find(
                  savedDivestiture.acquisitionRequestId);
          candidateStillPending =
              candidate !=
                  restoredObject.pendingAttributeOwnershipAcquisitionIfAvailableRequests.end() &&
              candidate->second.requestingFederateId ==
                  savedDivestiture.acquiringFederateId &&
              candidate->second.desiredAttributeHandles.contains(
                  savedDivestiture.attributeHandle);
        } else {
          auto const candidate =
              restoredObject.pendingAttributeOwnershipAcquisitionRequests.find(
                  savedDivestiture.acquisitionRequestId);
          candidateStillPending =
              candidate != restoredObject.pendingAttributeOwnershipAcquisitionRequests.end() &&
              candidate->second.requestingFederateId ==
                  savedDivestiture.acquiringFederateId &&
              candidate->second.desiredAttributeHandles.contains(
                  savedDivestiture.attributeHandle);
        }
        if (!candidateStillPending) {
          throw std::logic_error(
              "The saved negotiated ownership divestiture has no matching acquisition request.");
        }
      }

      Federation::ObjectInstance::PendingNegotiatedAttributeOwnershipDivestiture divestiture;
      divestiture.divestingFederateId = savedDivestiture.divestingFederateId;
      divestiture.acquiringFederateId = savedDivestiture.acquiringFederateId;
      divestiture.acquisitionRequestId = savedDivestiture.acquisitionRequestId;
      divestiture.acquiringFederateIsIfAvailable =
          savedDivestiture.acquiringFederateIsIfAvailable;
      divestiture.confirmationQueued = savedDivestiture.confirmationQueued;
      divestiture.confirmationDelivered = savedDivestiture.confirmationDelivered;
      if (materializeProcessRestartObjects && !savedDivestiture.confirmationDelivered) {
        // The saved flags describe a Request Divestiture Confirmation callback
        // in the old process.  Rebuild that callback against the current
        // owner/requester routes after restore; never replay a stale closure.
        divestiture.confirmationQueued = false;
        divestiture.confirmationDelivered = false;
      } else if (materializeProcessRestartObjects && savedDivestiture.confirmationDelivered) {
        // The callback already entered user code before the save.  Preserve
        // the durable delivered marker so the owner can continue directly to
        // Confirm Divestiture without receiving a duplicate confirmation.
        divestiture.confirmationQueued = true;
        divestiture.confirmationDelivered = true;
      }
      divestiture.userSuppliedTag.assign(
          savedDivestiture.userSuppliedTag.begin(),
          savedDivestiture.userSuppliedTag.end());
      auto const [position, inserted] =
          restoredObject.pendingNegotiatedAttributeOwnershipDivestitures.emplace(
              savedDivestiture.attributeHandle,
              std::move(divestiture));
      static_cast<void>(position);
      if (!inserted) {
        throw std::logic_error(
            "The saved object ownership ledger has duplicate negotiated divestiture attributes.");
      }
    }

    auto const isNegotiatedAssumptionAttribute =
        [&savedObject, &restoredObject](std::uint64_t attributeHandle) {
          auto const divestiture = std::ranges::find_if(
              savedObject.pendingNegotiatedAttributeOwnershipDivestitures,
              [attributeHandle](
                  FederationStateImagePendingNegotiatedAttributeOwnershipDivestiture const& candidate) {
                return candidate.attributeHandle == attributeHandle;
              });
          auto const owner = restoredObject.attributeOwnersByHandle.find(attributeHandle);
          return divestiture !=
                  savedObject.pendingNegotiatedAttributeOwnershipDivestitures.end() &&
              owner != restoredObject.attributeOwnersByHandle.end() &&
              owner->second == divestiture->divestingFederateId;
        };

    for (auto const& savedAssumption :
         savedObject.ownershipAssumptionRecipientsByAttribute) {
      if (savedAssumption.attributeHandle == 0U ||
          !knownAttributeHandles.contains(savedAssumption.attributeHandle) ||
          (restoredObject.attributeOwnersByHandle.contains(
               savedAssumption.attributeHandle) &&
           !isNegotiatedAssumptionAttribute(savedAssumption.attributeHandle))) {
        throw std::logic_error(
            "The saved ownership-assumption recipient ledger has an invalid attribute.");
      }
      std::set<std::uint64_t> recipients;
      recipients.insert(
          savedAssumption.recipientFederateIds.begin(),
          savedAssumption.recipientFederateIds.end());
      if (recipients.size() != savedAssumption.recipientFederateIds.size() ||
          !std::all_of(
              savedAssumption.recipientFederateIds.begin(),
              savedAssumption.recipientFederateIds.end(),
              [&liveFederation](std::uint64_t federateId) {
                return federateId != 0U && liveFederation.members.contains(federateId);
              })) {
        throw std::logic_error(
            "The saved ownership-assumption recipient ledger has an invalid recipient.");
      }
      auto const [position, inserted] =
          restoredObject.ownershipAssumptionRecipientsByAttribute.emplace(
              savedAssumption.attributeHandle,
              std::move(recipients));
      static_cast<void>(position);
      if (!inserted) {
        throw std::logic_error(
            "The saved object ownership ledger has duplicate assumption attributes.");
      }
    }

    for (auto const& savedAssumption :
         savedObject.ownershipAssumptionUserSuppliedTagsByAttribute) {
      if (savedAssumption.attributeHandle == 0U ||
          !knownAttributeHandles.contains(savedAssumption.attributeHandle) ||
          (restoredObject.attributeOwnersByHandle.contains(
               savedAssumption.attributeHandle) &&
           !isNegotiatedAssumptionAttribute(savedAssumption.attributeHandle))) {
        throw std::logic_error(
            "The saved ownership-assumption tag ledger has an invalid attribute.");
      }
      auto const [position, inserted] =
          restoredObject.ownershipAssumptionUserSuppliedTagsByAttribute.emplace(
              savedAssumption.attributeHandle,
              std::vector<unsigned char>(
                  savedAssumption.userSuppliedTag.begin(),
                  savedAssumption.userSuppliedTag.end()));
      static_cast<void>(position);
      if (!inserted) {
        throw std::logic_error(
            "The saved object ownership ledger has duplicate assumption tag attributes.");
      }
    }

    if (image.pendingAttributeOwnershipAssumptionsPresent) {
      for (auto const& savedCallback : image.pendingAttributeOwnershipAssumptions) {
        if (savedCallback.objectInstanceHandle != savedObject.handle) {
          continue;
        }
        if (savedCallback.receivingFederateId == 0U ||
            !liveFederation.members.contains(savedCallback.receivingFederateId) ||
            savedCallback.attributeHandles.empty() ||
            !std::is_sorted(
                savedCallback.attributeHandles.begin(),
                savedCallback.attributeHandles.end()) ||
            std::adjacent_find(
                savedCallback.attributeHandles.begin(),
                savedCallback.attributeHandles.end()) !=
                savedCallback.attributeHandles.end()) {
          throw std::logic_error(
              "The saved ownership-assumption callback has an invalid recipient or attribute set.");
        }
        PendingAttributeOwnershipAssumptionCallback callback;
        callback.receivingFederateId = savedCallback.receivingFederateId;
        callback.attributeHandles.insert(
            savedCallback.attributeHandles.begin(),
            savedCallback.attributeHandles.end());
        if (callback.attributeHandles.size() != savedCallback.attributeHandles.size() ||
            !std::all_of(
                savedCallback.attributeHandles.begin(),
                savedCallback.attributeHandles.end(),
                [&knownAttributeHandles](std::uint64_t attributeHandle) {
                  return knownAttributeHandles.contains(attributeHandle);
                })) {
          throw std::logic_error(
              "The saved ownership-assumption callback has unknown attributes.");
        }
        for (auto const attributeHandle : callback.attributeHandles) {
          auto const recipients =
              restoredObject.ownershipAssumptionRecipientsByAttribute.find(
                  attributeHandle);
          auto const tag =
              restoredObject.ownershipAssumptionUserSuppliedTagsByAttribute.find(
                  attributeHandle);
          if (recipients ==
                  restoredObject.ownershipAssumptionRecipientsByAttribute.end() ||
              !recipients->second.contains(callback.receivingFederateId) ||
              tag == restoredObject.ownershipAssumptionUserSuppliedTagsByAttribute.end() ||
              (restoredObject.attributeOwnersByHandle.contains(attributeHandle) &&
               !isNegotiatedAssumptionAttribute(attributeHandle))) {
            throw std::logic_error(
                "The saved ownership-assumption callback is detached from its search ledger.");
          }
        }
        callback.userSuppliedTag.assign(
            savedCallback.userSuppliedTag.begin(),
            savedCallback.userSuppliedTag.end());
        restoredObject.pendingAttributeOwnershipAssumptionCallbacks.push_back(
            std::move(callback));
      }
    }

    std::uint64_t previousPendingDiscoveryFederateId = 0U;
    for (auto const federateId : savedObject.pendingDiscoveryFederateIds) {
      if (federateId == 0U || federateId <= previousPendingDiscoveryFederateId ||
          !liveFederation.members.contains(federateId) ||
          restoredObject.knownObjectClassHandlesByFederate.contains(federateId)) {
        throw std::logic_error(
            "The saved object visibility projection has an invalid pending discovery federate.");
      }
      previousPendingDiscoveryFederateId = federateId;
      if (!restoredObject.pendingDiscoveryFederates.insert(federateId).second) {
        throw std::logic_error(
            "The saved object visibility projection has duplicate pending discovery federates.");
      }
    }
    std::uint64_t previousPendingRemovalFederateId = 0U;
    for (auto const federateId : savedObject.pendingRemovalFederateIds) {
      if (federateId == 0U || federateId <= previousPendingRemovalFederateId ||
          !liveFederation.members.contains(federateId) ||
          !restoredObject.knownObjectClassHandlesByFederate.contains(federateId)) {
        throw std::logic_error(
            "The saved object visibility projection has an invalid pending removal federate.");
      }
      previousPendingRemovalFederateId = federateId;
      if (!restoredObject.pendingRemovalFederates.insert(federateId).second) {
        throw std::logic_error(
          "The saved object visibility projection has duplicate pending removal federates.");
      }
    }

    std::uint64_t previousConnectionLossAutomaticRemovalFederateId = 0U;
    for (auto const federateId :
         savedObject.connectionLossAutomaticRemovalFederateIds) {
      if (federateId == 0U ||
          federateId <= previousConnectionLossAutomaticRemovalFederateId ||
          !liveFederation.members.contains(federateId) ||
          !restoredObject.pendingRemovalFederates.contains(federateId)) {
        throw std::logic_error(
            "The saved object lifecycle ledger has an invalid connection-loss automatic removal federate.");
      }
      previousConnectionLossAutomaticRemovalFederateId = federateId;
      if (!restoredObject.connectionLossAutomaticRemovalFederates.insert(
              federateId)
               .second) {
        throw std::logic_error(
            "The saved object lifecycle ledger has duplicate connection-loss automatic removals.");
      }
    }
    std::uint64_t previousDeferredConnectionLossTsoRemovalFederateId = 0U;
    for (auto const federateId :
         savedObject.deferredConnectionLossTsoRemovalFederateIds) {
      if (federateId == 0U ||
          federateId <= previousDeferredConnectionLossTsoRemovalFederateId ||
          !liveFederation.members.contains(federateId) ||
          !restoredObject.pendingRemovalFederates.contains(federateId) ||
          !restoredObject.connectionLossAutomaticRemovalFederates.contains(
              federateId)) {
        throw std::logic_error(
            "The saved object lifecycle ledger has an invalid deferred connection-loss TSO removal federate.");
      }
      previousDeferredConnectionLossTsoRemovalFederateId = federateId;
      if (!restoredObject.deferredConnectionLossTsoRemovalFederates.insert(
              federateId)
               .second) {
        throw std::logic_error(
            "The saved object lifecycle ledger has duplicate deferred connection-loss TSO removals.");
      }
    }
    std::uint64_t previousPendingTimestampedRemovalFederateId = 0U;
    for (auto const federateId : savedObject.pendingTimestampedRemovalFederateIds) {
      if (federateId == 0U ||
          federateId <= previousPendingTimestampedRemovalFederateId ||
          !liveFederation.members.contains(federateId)) {
        throw std::logic_error(
            "The saved object lifecycle ledger has an invalid pending timestamped removal federate.");
      }
      previousPendingTimestampedRemovalFederateId = federateId;
      if (!restoredObject.pendingTimestampedRemovalFederates.insert(federateId)
               .second) {
        throw std::logic_error(
            "The saved object lifecycle ledger has duplicate pending timestamped removals.");
      }
    }
    if (savedObject.pendingTimestampedDeletionMessageId.has_value()) {
      if (*savedObject.pendingTimestampedDeletionMessageId == 0U) {
        throw std::logic_error(
            "The saved object lifecycle ledger has an invalid pending timestamped deletion message.");
      }
      restoredObject.pendingTimestampedDeletionMessageId =
          savedObject.pendingTimestampedDeletionMessageId;
    } else if (!savedObject.pendingTimestampedRemovalFederateIds.empty()) {
      throw std::logic_error(
          "The saved object lifecycle ledger has timestamped removals without a deletion message.");
    }

    auto const [position, inserted] =
        restoredObjects.emplace(savedObject.handle, std::move(restoredObject));
    static_cast<void>(position);
    if (!inserted) {
      throw std::logic_error(
          "The saved object ownership ledger has duplicate object handles.");
    }
  }

  if (image.deferredUpdateRegionAssociationsPresent) {
    for (auto const& savedAssociation : image.deferredUpdateRegionAssociations) {
      auto object = restoredObjects.find(savedAssociation.objectInstanceHandle);
      if (object == restoredObjects.end() || object->second.deleteAccepted ||
          savedAssociation.federateId == 0U ||
          !liveFederation.members.contains(savedAssociation.federateId) ||
          savedAssociation.attributeHandle == 0U) {
        throw std::logic_error(
            "The saved deferred update-region association has an invalid identity.");
      }
      auto const savedObject = std::find_if(
          image.objects.begin(),
          image.objects.end(),
          [&savedAssociation](FederationStateImageObject const& candidate) {
            return candidate.handle == savedAssociation.objectInstanceHandle;
          });
      if (savedObject == image.objects.end() ||
          std::none_of(
              savedObject->attributes.begin(),
              savedObject->attributes.end(),
              [&savedAssociation](FederationStateImageObjectAttribute const& attribute) {
                return attribute.handle == savedAssociation.attributeHandle;
              })) {
        throw std::logic_error(
            "The saved deferred update-region association references an unknown attribute.");
      }
      auto const regionOwnerValid = std::all_of(
          savedAssociation.regionHandles.begin(),
          savedAssociation.regionHandles.end(),
          [&federation, &savedAssociation](std::uint64_t regionHandle) {
            auto const region = federation.regions.find(regionHandle);
            return region != federation.regions.end() &&
                   region->second.ownerFederateId == savedAssociation.federateId &&
                   region->second.specificationCommitted;
          });
      if (!regionOwnerValid) {
        throw std::logic_error(
            "The saved deferred update-region association references a region not owned by its federate.");
      }
      auto& associationsByAttribute =
          object->second.deferredUpdateRegionsByFederate[savedAssociation.federateId];
      auto const [position, inserted] = associationsByAttribute.emplace(
          savedAssociation.attributeHandle,
          std::set<std::uint64_t>(
              savedAssociation.regionHandles.begin(),
              savedAssociation.regionHandles.end()));
      static_cast<void>(position);
      if (!inserted) {
        throw std::logic_error(
            "The saved federation has duplicate deferred update-region associations.");
      }
    }
  }

  if (image.pendingAttributeOwnershipQueriesPresent) {
    for (auto const& savedQuery : image.pendingAttributeOwnershipQueries) {
      if (savedQuery.requestId == 0U ||
          savedQuery.requestingFederateId == 0U ||
          savedQuery.objectInstanceHandle == 0U || savedQuery.reportKind > 2U ||
          !liveFederation.members.contains(savedQuery.requestingFederateId) ||
          savedQuery.requestedAttributeHandles.empty()) {
        throw std::logic_error(
            "The saved Attribute Ownership query has an invalid identity.");
      }
      if ((savedQuery.reportKind == 0U && savedQuery.owningFederateId == 0U) ||
          (savedQuery.reportKind != 0U && savedQuery.owningFederateId != 0U)) {
        throw std::logic_error(
            "The saved Attribute Ownership query has an invalid owner report.");
      }
      Federation::PendingAttributeOwnershipQuery query;
      query.requestingFederateId = savedQuery.requestingFederateId;
      query.objectInstanceHandle = savedQuery.objectInstanceHandle;
      query.reportKind = static_cast<AttributeOwnershipQueryReportKind>(
          savedQuery.reportKind);
      query.owningFederateId = savedQuery.owningFederateId;
      query.requestedAttributeHandles.insert(
          savedQuery.requestedAttributeHandles.begin(),
          savedQuery.requestedAttributeHandles.end());
      if (query.requestedAttributeHandles.size() !=
              savedQuery.requestedAttributeHandles.size() ||
          !std::is_sorted(
              savedQuery.requestedAttributeHandles.begin(),
              savedQuery.requestedAttributeHandles.end()) ||
          std::adjacent_find(
              savedQuery.requestedAttributeHandles.begin(),
              savedQuery.requestedAttributeHandles.end()) !=
              savedQuery.requestedAttributeHandles.end()) {
        throw std::logic_error(
            "The saved Attribute Ownership query has duplicate attributes.");
      }
      auto const object = restoredObjects.find(savedQuery.objectInstanceHandle);
      auto const momObject = federation.rtiOwnedJoinedFederateMomObjects.find(
          savedQuery.objectInstanceHandle);
      if (object == restoredObjects.end() &&
          momObject == federation.rtiOwnedJoinedFederateMomObjects.end()) {
        throw std::logic_error(
            "The saved Attribute Ownership query targets an unknown object.");
      }
      if (object != restoredObjects.end()) {
        if (object->second.deleteAccepted || savedQuery.reportKind == 2U ||
            !object->second.knownObjectClassHandlesByFederate.contains(
                savedQuery.requestingFederateId)) {
          throw std::logic_error(
              "The saved Attribute Ownership query has an invalid object projection.");
        }
        std::set<std::uint64_t> knownAttributes;
        auto const savedObjectProjection = std::find_if(
            image.objects.begin(),
            image.objects.end(),
            [&savedQuery](FederationStateImageObject const& candidate) {
              return candidate.handle == savedQuery.objectInstanceHandle;
            });
        if (savedObjectProjection == image.objects.end()) {
          throw std::logic_error(
              "The saved Attribute Ownership query has no object projection.");
        }
        for (auto const& attribute : savedObjectProjection->attributes) {
          knownAttributes.insert(attribute.handle);
        }
        for (auto const& attribute : object->second.attributeOwnersByHandle) {
          knownAttributes.insert(attribute.first);
        }
        for (auto const& attribute : object->second.attributeTransportationTypes) {
          knownAttributes.insert(attribute.first);
        }
        for (auto const& attribute : object->second.attributeOrderTypes) {
          knownAttributes.insert(attribute.first);
        }
        for (auto const& attribute : object->second.attributeValues) {
          knownAttributes.insert(attribute.first);
        }
        for (auto const attributeHandle : query.requestedAttributeHandles) {
          if (!knownAttributes.contains(attributeHandle)) {
            throw std::logic_error(
                "The saved Attribute Ownership query has an unknown attribute.");
          }
        }
        if (query.reportKind == AttributeOwnershipQueryReportKind::federate &&
            !liveFederation.members.contains(query.owningFederateId)) {
          throw std::logic_error(
              "The saved Attribute Ownership query has a departed owner.");
        }
      } else if (savedQuery.reportKind != 2U ||
                 !momObject->second.knownFederateIds.contains(
                     savedQuery.requestingFederateId)) {
        throw std::logic_error(
            "The saved RTI-owned Attribute Ownership query has an invalid projection.");
      }
      auto const [position, inserted] =
          restoredPendingAttributeOwnershipQueries.emplace(
              savedQuery.requestId,
              std::move(query));
      static_cast<void>(position);
      if (!inserted) {
        throw std::logic_error(
            "The saved federation has duplicate Attribute Ownership query IDs.");
      }
    }
  }

  if (image.pendingAttributeOwnershipQueriesPresent) {
    federation.pendingAttributeOwnershipQueries =
        std::move(restoredPendingAttributeOwnershipQueries);
  }
  federation.objectInstances = std::move(restoredObjects);
  if (materializeProcessRestartObjects) {
    federation.objectInstanceHandlesByName.clear();
    for (auto const& [objectHandle, object] : federation.objectInstances) {
      if (object.name.empty() ||
          !federation.objectInstanceHandlesByName.emplace(object.name, objectHandle).second) {
        throw std::logic_error(
            "The saved object application-value ledger has an invalid or duplicate name.");
      }
    }
  }
}

} // namespace umbra::detail
